/* MPTCP Scheduler module selector. Highly inspired by tcp_cong.c */

#include <linux/module.h>
#include <net/mptcp.h>

static DEFINE_SPINLOCK(mptcp_sched_list_lock);
static LIST_HEAD(mptcp_sched_list);

struct defsched_priv {
	u32	last_rbuf_opti;

	/* the members below is only use by defsched_adv */
	u32	rcv_bytes;
	u32	rcv_rate;
	u32	last_prio_opti;
	u32	end_seq;
	u8	is_prio_opti:1,
		recv_ofo:1;
};

/* Struct to store the data of the control block */
struct defsched_cb_data {
	u32	last_sampling_ts;
	u32	sampling_rtt;
	u32	end_seq;
};

static struct defsched_priv *defsched_get_priv(const struct tcp_sock *tp)
{
	return (struct defsched_priv *)&tp->mptcp->mptcp_sched[0];
}

/* Returns the control block data from a given meta socket */
static struct defsched_cb_data *defsched_get_cb_data(struct tcp_sock *tp)
{
	return (struct defsched_cb_data *)&tp->mpcb->mptcp_sched[0];
}

bool mptcp_is_def_unavailable(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	/* Set of states for which we are allowed to send data */
	if (!mptcp_sk_can_send(sk))
		return true;

	/* We do not send data on this subflow unless it is
	 * fully established, i.e. the 4th ack has been received.
	 */
	if (tp->mptcp->pre_established)
		return true;

	if (tp->pf)
		return true;

	return false;
}
EXPORT_SYMBOL_GPL(mptcp_is_def_unavailable);

static bool mptcp_is_temp_unavailable(struct sock *sk,
				      const struct sk_buff *skb,
				      bool zero_wnd_test)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	unsigned int mss_now, space, in_flight;

	if (inet_csk(sk)->icsk_ca_state == TCP_CA_Loss) {
		/* If SACK is disabled, and we got a loss, TCP does not exit
		 * the loss-state until something above high_seq has been
		 * acked. (see tcp_try_undo_recovery)
		 *
		 * high_seq is the snd_nxt at the moment of the RTO. As soon
		 * as we have an RTO, we won't push data on the subflow.
		 * Thus, snd_una can never go beyond high_seq.
		 */
		if (!tcp_is_reno(tp))
			return true;
		else if (tp->snd_una != tp->high_seq)
			return true;
	}

	if (!tp->mptcp->fully_established) {
		/* Make sure that we send in-order data */
		if (skb && tp->mptcp->second_packet &&
		    tp->mptcp->last_end_data_seq != TCP_SKB_CB(skb)->seq)
			return true;
	}

	in_flight = tcp_packets_in_flight(tp);
	/* Not even a single spot in the cwnd */
	if (in_flight >= tp->snd_cwnd)
		return true;

	/* Now, check if what is queued in the subflow's send-queue
	 * already fills the cwnd.
	 */
	space = (tp->snd_cwnd - in_flight) * tp->mss_cache;

	if (tp->write_seq - tp->snd_nxt > space)
		return true;

	if (zero_wnd_test && !before(tp->write_seq, tcp_wnd_end(tp)))
		return true;

	mss_now = tcp_current_mss(sk);

	/* Don't send on this subflow if we bypass the allowed send-window at
	 * the per-subflow level. Similar to tcp_snd_wnd_test, but manually
	 * calculated end_seq (because here at this point end_seq is still at
	 * the meta-level).
	 */
	if (skb && !zero_wnd_test &&
	    after(tp->write_seq + min(skb->len, mss_now), tcp_wnd_end(tp)))
		return true;

	return false;
}

/* Is the sub-socket sk available to send the skb? */
bool mptcp_is_available(struct sock *sk, const struct sk_buff *skb,
			bool zero_wnd_test)
{
	return !mptcp_is_def_unavailable(sk) &&
	       !mptcp_is_temp_unavailable(sk, skb, zero_wnd_test);
}
EXPORT_SYMBOL_GPL(mptcp_is_available);

/* Are we not allowed to reinject this skb on tp? */
static int mptcp_dont_reinject_skb(const struct tcp_sock *tp, const struct sk_buff *skb)
{
	/* If the skb has already been enqueued in this sk, try to find
	 * another one.
	 */
	return skb &&
		/* Has the skb already been enqueued into this subsocket? */
		mptcp_pi_to_flag(tp->mptcp->path_index) & TCP_SKB_CB(skb)->path_mask;
}

bool subflow_is_backup(const struct tcp_sock *tp)
{
	return tp->mptcp->rcv_low_prio || tp->mptcp->low_prio;
}
EXPORT_SYMBOL_GPL(subflow_is_backup);

bool subflow_is_active(const struct tcp_sock *tp)
{
	return !tp->mptcp->rcv_low_prio && !tp->mptcp->low_prio;
}
EXPORT_SYMBOL_GPL(subflow_is_active);

/* Generic function to iterate over used and unused subflows and to select the
 * best one
 */
static struct sock
*get_subflow_from_selectors(struct mptcp_cb *mpcb, struct sk_buff *skb,
			    bool (*selector)(const struct tcp_sock *),
			    bool zero_wnd_test, bool *force)
{
	struct sock *bestsk = NULL;
	u32 min_srtt = 0xffffffff;
	bool found_unused = false;
	bool found_unused_una = false;
	struct sock *sk;

	mptcp_for_each_sk(mpcb, sk) {
		struct tcp_sock *tp = tcp_sk(sk);
		bool unused = false;

		/* First, we choose only the wanted sks */
		if (!(*selector)(tp))
			continue;

		if (!mptcp_dont_reinject_skb(tp, skb))
			unused = true;
		else if (found_unused)
			/* If a unused sk was found previously, we continue -
			 * no need to check used sks anymore.
			 */
			continue;

		if (mptcp_is_def_unavailable(sk))
			continue;

		if (mptcp_is_temp_unavailable(sk, skb, zero_wnd_test)) {
			if (unused)
				found_unused_una = true;
			continue;
		}

		if (unused) {
			if (!found_unused) {
				/* It's the first time we encounter an unused
				 * sk - thus we reset the bestsk (which might
				 * have been set to a used sk).
				 */
				min_srtt = 0xffffffff;
				bestsk = NULL;
			}
			found_unused = true;
		}

		if (tp->srtt_us < min_srtt) {
			min_srtt = tp->srtt_us;
			bestsk = sk;
		}
	}

	if (bestsk) {
		/* The force variable is used to mark the returned sk as
		 * previously used or not-used.
		 */
		if (found_unused)
			*force = true;
		else
			*force = false;
	} else {
		/* The force variable is used to mark if there are temporally
		 * unavailable not-used sks.
		 */
		if (found_unused_una)
			*force = true;
		else
			*force = false;
	}

	return bestsk;
}

/* This is the scheduler. This function decides on which flow to send
 * a given MSS. If all subflows are found to be busy, NULL is returned
 * The flow is selected based on the shortest RTT.
 * If all paths have full cong windows, we simply return NULL.
 *
 * Additionally, this function is aware of the backup-subflows.
 */
struct sock *get_available_subflow(struct sock *meta_sk, struct sk_buff *skb,
				   bool zero_wnd_test)
{
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct sock *sk;
	bool looping = false, force;

	/* if there is only one subflow, bypass the scheduling function */
	if (mpcb->cnt_subflows == 1) {
		sk = (struct sock *)mpcb->connection_list;
		if (!mptcp_is_available(sk, skb, zero_wnd_test))
			sk = NULL;
		return sk;
	}

	/* Answer data_fin on same subflow!!! */
	if (meta_sk->sk_shutdown & RCV_SHUTDOWN &&
	    skb && mptcp_is_data_fin(skb)) {
		mptcp_for_each_sk(mpcb, sk) {
			if (tcp_sk(sk)->mptcp->path_index == mpcb->dfin_path_index &&
			    mptcp_is_available(sk, skb, zero_wnd_test))
				return sk;
		}
	}

	/* Find the best subflow */
restart:
	sk = get_subflow_from_selectors(mpcb, skb, &subflow_is_active,
					zero_wnd_test, &force);
	if (force)
		/* one unused active sk or one NULL sk when there is at least
		 * one temporally unavailable unused active sk
		 */
		return sk;

	sk = get_subflow_from_selectors(mpcb, skb, &subflow_is_backup,
					zero_wnd_test, &force);
	if (!force && skb) {
		/* one used backup sk or one NULL sk where there is no one
		 * temporally unavailable unused backup sk
		 *
		 * the skb passed through all the available active and backups
		 * sks, so clean the path mask
		 */
		TCP_SKB_CB(skb)->path_mask = 0;

		if (!looping) {
			looping = true;
			goto restart;
		}
	}
	return sk;
}
EXPORT_SYMBOL_GPL(get_available_subflow);

static struct sk_buff *mptcp_rcv_buf_optimization(struct sock *sk, int penal)
{
	struct sock *meta_sk;
	const struct tcp_sock *tp = tcp_sk(sk);
	struct tcp_sock *tp_it;
	struct sk_buff *skb_head;
	struct defsched_priv *dsp = defsched_get_priv(tp);

	if (tp->mpcb->cnt_subflows == 1)
		return NULL;

	meta_sk = mptcp_meta_sk(sk);
	skb_head = tcp_write_queue_head(meta_sk);

	if (!skb_head || skb_head == tcp_send_head(meta_sk))
		return NULL;

	/* If penalization is optional (coming from mptcp_next_segment() and
	 * We are not send-buffer-limited we do not penalize. The retransmission
	 * is just an optimization to fix the idle-time due to the delay before
	 * we wake up the application.
	 */
	if (!penal && sk_stream_memory_free(meta_sk))
		goto retrans;

	/* Only penalize again after an RTT has elapsed */
	if (tcp_time_stamp - dsp->last_rbuf_opti < usecs_to_jiffies(tp->srtt_us >> 3))
		goto retrans;

	/* Half the cwnd of the slow flow */
	mptcp_for_each_tp(tp->mpcb, tp_it) {
		if (tp_it != tp &&
		    TCP_SKB_CB(skb_head)->path_mask & mptcp_pi_to_flag(tp_it->mptcp->path_index)) {
			if (tp->srtt_us < tp_it->srtt_us && inet_csk((struct sock *)tp_it)->icsk_ca_state == TCP_CA_Open) {
				u32 prior_cwnd = tp_it->snd_cwnd;

				tp_it->snd_cwnd = max(tp_it->snd_cwnd >> 1U, 1U);

				/* If in slow start, do not reduce the ssthresh */
				if (prior_cwnd >= tp_it->snd_ssthresh)
					tp_it->snd_ssthresh = max(tp_it->snd_ssthresh >> 1U, 2U);

				dsp->last_rbuf_opti = tcp_time_stamp;
			}
			break;
		}
	}

retrans:

	/* Segment not yet injected into this path? Take it!!! */
	if (!(TCP_SKB_CB(skb_head)->path_mask & mptcp_pi_to_flag(tp->mptcp->path_index))) {
		bool do_retrans = false;
		mptcp_for_each_tp(tp->mpcb, tp_it) {
			if (tp_it != tp &&
			    TCP_SKB_CB(skb_head)->path_mask & mptcp_pi_to_flag(tp_it->mptcp->path_index)) {
				if (tp_it->snd_cwnd <= 4) {
					do_retrans = true;
					break;
				}

				if (4 * tp->srtt_us >= tp_it->srtt_us) {
					do_retrans = false;
					break;
				} else {
					do_retrans = true;
				}
			}
		}

		if (do_retrans && mptcp_is_available(sk, skb_head, false))
			return skb_head;
	}
	return NULL;
}

/* Returns the next segment to be sent from the mptcp meta-queue.
 * (chooses the reinject queue if any segment is waiting in it, otherwise,
 * chooses the normal write queue).
 * Sets *@reinject to 1 if the returned segment comes from the
 * reinject queue. Sets it to 0 if it is the regular send-head of the meta-sk,
 * and sets it to -1 if it is a meta-level retransmission to optimize the
 * receive-buffer.
 */
static struct sk_buff *__mptcp_next_segment(struct sock *meta_sk, int *reinject)
{
	const struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct sk_buff *skb = NULL;

	*reinject = 0;

	/* If we are in fallback-mode, just take from the meta-send-queue */
	if (mpcb->infinite_mapping_snd || mpcb->send_infinite_mapping)
		return tcp_send_head(meta_sk);

	skb = skb_peek(&mpcb->reinject_queue);

	if (skb) {
		*reinject = 1;
	} else {
		skb = tcp_send_head(meta_sk);

		if (!skb && meta_sk->sk_socket &&
		    test_bit(SOCK_NOSPACE, &meta_sk->sk_socket->flags) &&
		    sk_stream_wspace(meta_sk) < sk_stream_min_wspace(meta_sk)) {
			struct sock *subsk = get_available_subflow(meta_sk, NULL,
								   false);
			if (!subsk)
				return NULL;

			skb = mptcp_rcv_buf_optimization(subsk, 0);
			if (skb)
				*reinject = -1;
		}
	}
	return skb;
}

static struct sk_buff *mptcp_next_segment(struct sock *meta_sk,
					  int *reinject,
					  struct sock **subsk,
					  unsigned int *limit)
{
	struct sk_buff *skb = __mptcp_next_segment(meta_sk, reinject);
	unsigned int mss_now;
	struct tcp_sock *subtp;
	u16 gso_max_segs;
	u32 max_len, max_segs, window, needed;

	/* As we set it, we have to reset it as well. */
	*limit = 0;

	if (!skb)
		return NULL;

	*subsk = get_available_subflow(meta_sk, skb, false);
	if (!*subsk)
		return NULL;

	subtp = tcp_sk(*subsk);
	mss_now = tcp_current_mss(*subsk);

	if (!*reinject && unlikely(!tcp_snd_wnd_test(tcp_sk(meta_sk), skb, mss_now))) {
		skb = mptcp_rcv_buf_optimization(*subsk, 1);
		if (skb)
			*reinject = -1;
		else
			return NULL;
	}

	/* No splitting required, as we will only send one single segment */
	if (skb->len <= mss_now)
		return skb;

	/* The following is similar to tcp_mss_split_point, but
	 * we do not care about nagle, because we will anyways
	 * use TCP_NAGLE_PUSH, which overrides this.
	 *
	 * So, we first limit according to the cwnd/gso-size and then according
	 * to the subflow's window.
	 */

	gso_max_segs = (*subsk)->sk_gso_max_segs;
	if (!gso_max_segs) /* No gso supported on the subflow's NIC */
		gso_max_segs = 1;
	max_segs = min_t(unsigned int, tcp_cwnd_test(subtp, skb), gso_max_segs);
	if (!max_segs)
		return NULL;

	max_len = mss_now * max_segs;
	window = tcp_wnd_end(subtp) - subtp->write_seq;

	needed = min(skb->len, window);
	if (max_len <= skb->len)
		/* Take max_win, which is actually the cwnd/gso-size */
		*limit = max_len;
	else
		/* Or, take the window */
		*limit = needed;

	return skb;
}

static void defsched_init(struct sock *sk)
{
	struct defsched_priv *dsp = defsched_get_priv(tcp_sk(sk));

	dsp->last_rbuf_opti = tcp_time_stamp;
}

#define MIN_SAMPLING_INTERVAL (20*1000u) /* 20ms */
#define MAX_SAMPLING_INTERVAL (300*1000u) /* 300ms */
#define PRIO_OPTI_INTERVAL (30*1000u) /* 30s */

#define SHIFT_2X	1
#define SHIFT_4X	2
#define SHIFT_8X	3

static void defsched_adv_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct defsched_priv *dsp = defsched_get_priv(tp);
	struct sock *meta_sk = tp->meta_sk;
	struct mptcp_cb *mpcb	= tcp_sk(meta_sk)->mpcb;
	struct defsched_cb_data *cb_data;
	u32 now = tcp_time_stamp;

	dsp->last_rbuf_opti = now;
	dsp->last_prio_opti = now;
	dsp->rcv_bytes = 0;
	dsp->rcv_rate = 0;
	dsp->end_seq = tcp_sk(meta_sk)->rcv_nxt;
	dsp->is_prio_opti = 0;
	dsp->recv_ofo = 0;

	/* if the first sub flow, init the cb_data */
	if (!mpcb->cnt_subflows) {
		cb_data = defsched_get_cb_data(tp);
		cb_data->end_seq = dsp->end_seq;
		cb_data->last_sampling_ts = now;
		cb_data->sampling_rtt = tp->srtt_us >> SHIFT_8X;
		if (cb_data->sampling_rtt < MIN_SAMPLING_INTERVAL)
			cb_data->sampling_rtt = MIN_SAMPLING_INTERVAL;
		else if (cb_data->sampling_rtt > MAX_SAMPLING_INTERVAL)
			cb_data->sampling_rtt = MAX_SAMPLING_INTERVAL;
	}
}

static void defsched_adv_calc(struct sock *sk, u32 now, u32 *max_rate,
			      u32 *max_sampling_rtt, u32 *min_rtt, u32 delta_us)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct mptcp_cb *mpcb = tp->mpcb;
	struct sock *sub_sk;
	struct tcp_sock *sub_tp;
	struct defsched_priv *sub_dsp;
	u32 rtt_us, rate, tmp;
	struct dst_entry *dst;
	char iface[IFNAMSIZ];

	mptcp_for_each_sk(mpcb, sub_sk) {
		sub_tp = tcp_sk(sub_sk);
		if (mptcp_is_def_unavailable(sub_sk))
			continue;

		sub_dsp = defsched_get_priv(sub_tp);
		if (sub_dsp->is_prio_opti)
			continue;

		/* rate is bytes per ms */
		rate = (sub_dsp->rcv_bytes*1000)/delta_us;

		/* rcv_rate is 8X of rate, it is same as tcp_rtt_estimator  */
		if (!sub_dsp->rcv_rate) {
			sub_dsp->rcv_rate = rate << SHIFT_8X;
		} else {
			tmp = sub_dsp->rcv_rate - (sub_dsp->rcv_rate >> SHIFT_8X);
			sub_dsp->rcv_rate = rate + tmp; /* rate = 7/8 rate + 1/8 new */
		}

		if ((sub_dsp->rcv_rate >> SHIFT_8X) > *max_rate)
			*max_rate = (sub_dsp->rcv_rate >> SHIFT_8X);

		rtt_us = jiffies_to_usecs(sub_tp->rcv_rtt_est.rtt) >> SHIFT_8X;
		if (!rtt_us)
			rtt_us = sub_tp->srtt_us >> SHIFT_8X;

		if (!(*min_rtt) || (*min_rtt > rtt_us))
			*min_rtt = rtt_us;

		if (*max_sampling_rtt < rtt_us)
			*max_sampling_rtt = rtt_us;

		if (unlikely(sysctl_mptcp_debug > 1)) {
			dst = sk_dst_get(sub_sk);
			if (dst) {
				(void)strncpy(iface, dst->dev->name, IFNAMSIZ);
				iface[IFNAMSIZ - 1] = '\0';
				dst_release(dst);
			} else
				(void)strncpy(iface, "NULL", IFNAMSIZ);

			mptcp_debug("%s: sub_sk %pK iface %s rcv_rate %u recv_ofo %u rate %u rcv_bytes %u delta_us %u rtt_us %u min_rtt %u\n", __func__,
				sub_sk, iface, sub_dsp->rcv_rate >> SHIFT_8X, sub_dsp->recv_ofo, rate, sub_dsp->rcv_bytes, delta_us, rtt_us, *min_rtt);
		}
	}
}

static void defsched_adv_rcv_skb(struct sock *sk, unsigned int len,
			     unsigned int end_seq, bool valid)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct defsched_cb_data *cb_data = defsched_get_cb_data(tp);
	struct defsched_priv *dsp = defsched_get_priv(tp);
	struct mptcp_cb *mpcb = tp->mpcb;
	u32 now = tcp_time_stamp;
	u32 delta_us;
	u32 delta_prio_ts;
	u32 rtt_us;
	struct sock *sub_sk;
	struct tcp_sock *sub_tp;
	struct tcp_sock *good_tp = NULL;
	struct defsched_priv *sub_dsp;
	u32 max_rate = 0;
	u32 max_sampling_rtt = MIN_SAMPLING_INTERVAL;
	u32 min_rtt = 0;
	bool send_mp_other_prio = false;

	if (likely(valid)) {
		dsp->rcv_bytes += len;
	} else if (after(end_seq, dsp->end_seq)) {
		dsp->recv_ofo = 1;
		mptcp_debug("%s: sub_sk %pK seq %u\n", __func__, sk, end_seq - len);
	}

	if (after(end_seq, cb_data->end_seq))
		cb_data->end_seq = end_seq;

	if (likely(after(end_seq, dsp->end_seq)))
		dsp->end_seq = end_seq;

	delta_us = jiffies_to_usecs(now - cb_data->last_sampling_ts);
	if (delta_us < cb_data->sampling_rtt)
		return;

	defsched_adv_calc(sk, now, &max_rate, &max_sampling_rtt, &min_rtt,
			  delta_us);
	if (max_sampling_rtt > max(min_rtt, MAX_SAMPLING_INTERVAL))
		max_sampling_rtt = max(min_rtt, MAX_SAMPLING_INTERVAL);

	mptcp_for_each_sk(mpcb, sub_sk) {
		sub_tp = tcp_sk(sub_sk);
		if (mptcp_is_def_unavailable(sub_sk))
			continue;

		sub_dsp = defsched_get_priv(sub_tp);
		if (unlikely(sub_dsp->is_prio_opti)) {
			delta_prio_ts = jiffies_to_msecs(now - sub_dsp->last_prio_opti);
			if (delta_prio_ts > PRIO_OPTI_INTERVAL) {
				sub_dsp->rcv_bytes = 0;
				sub_dsp->rcv_rate = 0;
				sub_dsp->is_prio_opti = 0;
				sub_dsp->recv_ofo = 0;

				sub_tp->mptcp->low_prio = 0;
				sub_tp->mptcp->send_mp_prio = 1;
				send_mp_other_prio = true;
			}
			continue;
		}

		if (likely(!sub_dsp->recv_ofo)) {
			sub_dsp->rcv_bytes = 0;
			if (!good_tp)
				good_tp = sub_tp;
			continue;
		}

		rtt_us = jiffies_to_usecs(sub_tp->rcv_rtt_est.rtt) >> SHIFT_8X;
		if (!rtt_us)
			rtt_us = sub_tp->srtt_us >> SHIFT_8X;
		if ((sub_dsp->rcv_rate < max_rate) &&
		    (rtt_us > (min_rtt << SHIFT_4X))) {
			sub_tp->mptcp->low_prio = 1;
			sub_tp->mptcp->send_mp_prio = 1;
			send_mp_other_prio = true;
			sub_dsp->last_prio_opti = now;
			sub_dsp->is_prio_opti = 1;

			mptcp_debug("%s: sub_sk %pK change to low_prio for rcv_rate %u max_rate %u rtt_us %u min_rtt %u\n", __func__,
				sub_sk, sub_dsp->rcv_rate >> SHIFT_8X, max_rate, rtt_us, min_rtt);
		} else if (!good_tp)
			good_tp = sub_tp;

		sub_dsp->recv_ofo = 0;
		sub_dsp->rcv_bytes = 0;
	}

	if (send_mp_other_prio && good_tp)
		good_tp->mptcp->send_mp_other_prio = 1;

	cb_data->sampling_rtt = max_sampling_rtt;
	cb_data->last_sampling_ts = now;
}

struct mptcp_sched_ops mptcp_sched_default = {
	.get_subflow = get_available_subflow,
	.next_segment = mptcp_next_segment,
	.init = defsched_init,
	.name = "default",
	.owner = THIS_MODULE,
};

struct mptcp_sched_ops mptcp_sched_default_adv = {
	.get_subflow = get_available_subflow,
	.next_segment = mptcp_next_segment,
	.rcv_skb = defsched_adv_rcv_skb,
	.init = defsched_adv_init,
	.name = "default_adv",
	.owner = THIS_MODULE,
};

static struct mptcp_sched_ops *mptcp_sched_find(const char *name)
{
	struct mptcp_sched_ops *e;
	char tmp_name[MPTCP_SCHED_NAME_MAX] = {0};
	if (strcmp(name, MPTCP_SCHED_NAME_HANDOVER) == 0)
		strncpy(tmp_name, MPTCP_SCHED_NAME_REDUNDANT, MPTCP_SCHED_NAME_MAX - 1);
	else
		strncpy(tmp_name, name, MPTCP_SCHED_NAME_MAX - 1);

	list_for_each_entry_rcu(e, &mptcp_sched_list, list) {
		if (strcmp(e->name, tmp_name) == 0)
			return e;
	}

	return NULL;
}

bool mptcp_sched_check_exist(const char *name)
{
	bool ret = false;

	if (!name)
		return ret;

	spin_lock(&mptcp_sched_list_lock);
	if (mptcp_sched_find(name))
		ret = true;
	spin_unlock(&mptcp_sched_list_lock);

	return ret;
}

int mptcp_register_scheduler(struct mptcp_sched_ops *sched)
{
	int ret = 0;

	if (!sched->get_subflow || !sched->next_segment)
		return -EINVAL;

	spin_lock(&mptcp_sched_list_lock);
	if (mptcp_sched_find(sched->name)) {
		pr_notice("%s already registered\n", sched->name);
		ret = -EEXIST;
	} else {
		list_add_tail_rcu(&sched->list, &mptcp_sched_list);
		pr_info("%s registered\n", sched->name);
	}
	spin_unlock(&mptcp_sched_list_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(mptcp_register_scheduler);

void mptcp_unregister_scheduler(struct mptcp_sched_ops *sched)
{
	spin_lock(&mptcp_sched_list_lock);
	list_del_rcu(&sched->list);
	spin_unlock(&mptcp_sched_list_lock);

	/* Wait for outstanding readers to complete before the
	 * module gets removed entirely.
	 *
	 * A try_module_get() should fail by now as our module is
	 * in "going" state since no refs are held anymore and
	 * module_exit() handler being called.
	 */
	synchronize_rcu();
}
EXPORT_SYMBOL_GPL(mptcp_unregister_scheduler);

void mptcp_get_default_scheduler(char *name)
{
	struct mptcp_sched_ops *sched;

	BUG_ON(list_empty(&mptcp_sched_list));

	rcu_read_lock();
	sched = list_entry(mptcp_sched_list.next, struct mptcp_sched_ops, list);
	(void)strncpy(name, sched->name, MPTCP_SCHED_NAME_MAX);
	name[MPTCP_SCHED_NAME_MAX - 1] = '\0';
	rcu_read_unlock();
}

int mptcp_set_default_scheduler(const char *name)
{
	struct mptcp_sched_ops *sched;
	int ret = -ENOENT;

	spin_lock(&mptcp_sched_list_lock);
	sched = mptcp_sched_find(name);
#ifdef CONFIG_MODULES
	if (!sched && capable(CAP_NET_ADMIN)) {
		spin_unlock(&mptcp_sched_list_lock);

		request_module("mptcp_%s", name);
		spin_lock(&mptcp_sched_list_lock);
		sched = mptcp_sched_find(name);
	}
#endif

	if (sched) {
		list_move(&sched->list, &mptcp_sched_list);
		ret = 0;
	} else {
		pr_info("%s is not available\n", name);
	}
	spin_unlock(&mptcp_sched_list_lock);

	return ret;
}

/* Must be called with rcu lock held */
static struct mptcp_sched_ops *__mptcp_sched_find_autoload(const char *name)
{
	struct mptcp_sched_ops *sched = mptcp_sched_find(name);
#ifdef CONFIG_MODULES
	if (!sched && capable(CAP_NET_ADMIN)) {
		rcu_read_unlock();
		request_module("mptcp_%s", name);
		rcu_read_lock();
		sched = mptcp_sched_find(name);
	}
#endif
	return sched;
}

void mptcp_init_scheduler(struct mptcp_cb *mpcb)
{
	struct mptcp_sched_ops *sched;
	struct sock *meta_sk = mpcb->meta_sk;
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);

	rcu_read_lock();
	/* if scheduler was set using socket option */
	if (meta_tp->mptcp_sched_setsockopt) {
		sched = __mptcp_sched_find_autoload(meta_tp->mptcp_sched_name);
		if (sched && try_module_get(sched->owner)) {
			mpcb->sched_ops = sched;
			goto out;
		}
	}

	list_for_each_entry_rcu(sched, &mptcp_sched_list, list) {
		if (try_module_get(sched->owner)) {
			mpcb->sched_ops = sched;
			break;
		}
	}
out:
	rcu_read_unlock();
}

/* Change scheduler for socket */
int mptcp_set_scheduler(struct sock *sk, const char *name)
{
	struct mptcp_sched_ops *sched;
	int err = 0;

	rcu_read_lock();
	sched = __mptcp_sched_find_autoload(name);

	if (!sched) {
		err = -ENOENT;
	}/* else if (!ns_capable(sock_net(sk)->user_ns, CAP_NET_ADMIN)) {
		err = -EPERM;
	} */else {
		(void)strncpy(tcp_sk(sk)->mptcp_sched_name, name, MPTCP_SCHED_NAME_MAX);
		tcp_sk(sk)->mptcp_sched_name[MPTCP_SCHED_NAME_MAX - 1] = '\0';
		tcp_sk(sk)->mptcp_sched_setsockopt = 1;

		if (0 == strcmp(name, MPTCP_SCHED_NAME_HANDOVER)) {
			if (0 == *(u32 *)(tcp_sk(sk)->mptcp_sched_params))
				*(u32 *)(tcp_sk(sk)->mptcp_sched_params) =
					MPTCP_HANDOVER_DEFAULT_RTT_THR;
		}

		if (0 == strcmp(name, MPTCP_SCHED_NAME_REDUNDANT))
			*(u32 *)(tcp_sk(sk)->mptcp_sched_params) = 0;
	}
	rcu_read_unlock();

	return err;
}

/* Manage refcounts on socket close. */
void mptcp_cleanup_scheduler(struct mptcp_cb *mpcb)
{
	module_put(mpcb->sched_ops->owner);
}

/* Set default value from kernel configuration at bootup */
static int __init mptcp_scheduler_default(void)
{
	BUILD_BUG_ON(sizeof(struct defsched_priv) > MPTCP_SCHED_SIZE);
	BUILD_BUG_ON(sizeof(struct defsched_cb_data) > MPTCP_SCHED_DATA_SIZE);

	return mptcp_set_default_scheduler(CONFIG_DEFAULT_MPTCP_SCHED);
}
late_initcall(mptcp_scheduler_default);
