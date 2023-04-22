#include <linux/module.h>
#include <linux/proc_fs.h>

#include <net/mptcp.h>
#include <net/mptcp_v4.h>

#if IS_ENABLED(CONFIG_IPV6)
#include <net/mptcp_v6.h>
#include <net/addrconf.h>
#endif
#include <linux/inet.h>

enum {
	MPTCP_EVENT_ADD = 1,
	MPTCP_EVENT_DEL,
	MPTCP_EVENT_MOD,
};

#define MPTCP_SUBFLOW_RETRY_DELAY	1000

/* Max number of local or remote addresses we can store.
 * When changing, see the bitfield below in fullmesh_rem4/6.
 */
#define MPTCP_MAX_ADDR	8

struct fullmesh_rem4 {
	u8		rem4_id;
	u8		bitfield;
	u8		forbid_bitfield;
	u8		retry_bitfield;
	__be16		port;
	struct in_addr	addr;
};

struct fullmesh_rem6 {
	u8		rem6_id;
	u8		bitfield;
	u8		retry_bitfield;
	__be16		port;
	struct in6_addr	addr;
};

struct mptcp_loc_addr {
	struct mptcp_loc4 locaddr4[MPTCP_MAX_ADDR];
	u8 loc4_bits;
	u8 next_v4_index;

	struct mptcp_loc6 locaddr6[MPTCP_MAX_ADDR];
	u8 loc6_bits;
	u8 next_v6_index;
	struct rcu_head rcu;
};

struct mptcp_addr_event {
	struct list_head list;
	unsigned short	family;
	u8	code:7,
		low_prio:1;
	int	if_idx;
	union inet_addr addr;
};

struct fullmesh_priv {
	/* Worker struct for subflow establishment */
	struct work_struct subflow_work;
	/* Delayed worker, when the routing-tables are not yet ready. */
	struct delayed_work subflow_retry_work;

	/* Remote addresses */
	struct fullmesh_rem4 remaddr4[MPTCP_MAX_ADDR];
	struct fullmesh_rem6 remaddr6[MPTCP_MAX_ADDR];

	struct mptcp_cb *mpcb;

	u16 remove_addrs; /* Addresses to remove */
	u8 announced_addrs_v4; /* IPv4 Addresses we did announce */
	u8 announced_addrs_v6; /* IPv6 Addresses we did announce */

	u8	add_addr; /* Are we sending an add_addr? */

	u8 rem4_bits;
	u8 rem6_bits;
	u8 mptcp_rem_locid;
	/* Are we established the additional subflows for primary pair? */
	u8 first_pair:1;
};

struct mptcp_fm_ns {
	struct mptcp_loc_addr __rcu *local;
	spinlock_t local_lock; /* Protecting the above pointer */
	struct list_head events;
	struct delayed_work address_worker;

	struct net *net;
};

static int num_subflows __read_mostly = 1;
module_param(num_subflows, int, 0644);
MODULE_PARM_DESC(num_subflows, "choose the number of subflows per pair of IP addresses of MPTCP connection");

static int create_on_err __read_mostly = 1;
module_param(create_on_err, int, 0644);
MODULE_PARM_DESC(create_on_err, "recreate the subflow upon a timeout");

static struct mptcp_pm_ops full_mesh __read_mostly;

static void full_mesh_create_subflows(struct sock *meta_sk);

static struct mptcp_fm_ns *fm_get_ns(const struct net *net)
{
	return (struct mptcp_fm_ns *)net->mptcp.path_managers[MPTCP_PM_FULLMESH];
}

static struct fullmesh_priv *fullmesh_get_priv(const struct mptcp_cb *mpcb)
{
	return (struct fullmesh_priv *)&mpcb->mptcp_pm[0];
}

u8	fullmesh_get_rem_locid(const struct mptcp_cb *mpcb)
{
	return ++fullmesh_get_priv(mpcb)->mptcp_rem_locid;
}

/* Find the first free index in the bitfield */
static int __mptcp_find_free_index(u8 bitfield, u8 base)
{
	int i;

	/* There are anyways no free bits... */
	if (bitfield == 0xff)
		goto exit;

	i = ffs(~(bitfield >> base)) - 1;
	if (i < 0)
		goto exit;

	/* No free bits when starting at base, try from 0 on */
	if (i + base >= sizeof(bitfield) * 8)
		return __mptcp_find_free_index(bitfield, 0);

	return i + base;
exit:
	return -1;
}

static int mptcp_find_free_index(u8 bitfield)
{
	return __mptcp_find_free_index(bitfield, 0);
}

/* fixme: if both interface are to same operator we connect using same local interface */
static void full_mesh_update_forbid_path(struct fullmesh_rem4 *remaddr4, struct sock *meta_sk)
{
	int r_carrier_id;
	int l_carrier_id;
	int l_carrier_bit = 0;
	int i;
	struct net *net = sock_net(meta_sk);
	char ifname[IFNAMSIZ] = {0};
	struct mptcp_loc_addr *mptcp_local;
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(net);

	if (tcp_sk(meta_sk)->mptcp_cap_flag != MPTCP_CAP_UID_DIP_DPORT)
		return;
	r_carrier_id = mptcp_get_raddr_carrier_info(sock_i_uid(meta_sk).val, remaddr4->addr.s_addr);
	/* No forbid list */
	if (-1 == r_carrier_id)
		return;

	mptcp_debug("%s: r_carrier_id: %d\n", __func__, r_carrier_id);
	/* We need a local (stable) copy of the address-list. Really, it is not
	 * such a big deal, if the address-list is not 100% up-to-date.
	 */
	rcu_read_lock_bh();
	mptcp_local = rcu_dereference_bh(fm_ns->local);
	mptcp_local = kmemdup(mptcp_local, sizeof(*mptcp_local), GFP_ATOMIC);
	rcu_read_unlock_bh();

	if (!mptcp_local)
		return;

	/* if already connected then just update and skip, this may be case of new interface added */
	if (remaddr4->bitfield && (remaddr4->addr.s_addr != meta_sk->sk_daddr)) {
		remaddr4->forbid_bitfield = 0;
		remaddr4->forbid_bitfield |= mptcp_local->loc4_bits;
		remaddr4->forbid_bitfield &= ~remaddr4->bitfield;
		kfree(mptcp_local);
		return;
	}

	/* get local interface for carrier */
	mptcp_for_each_bit_set(mptcp_local->loc4_bits, i) {
		(void)netdev_get_name(net, ifname, mptcp_local->locaddr4[i].if_idx);
		l_carrier_id = mptcp_hw_get_carrier_info_of_iface(ifname);
		mptcp_debug("%s: r_carrier_id: %d, l_carrier_id: %d\n", __func__, r_carrier_id, l_carrier_id);
		if (l_carrier_id == r_carrier_id ||
		    (l_carrier_id == MPTCP_HW_EXT_CARRIER_INVALID &&
		     remaddr4->addr.s_addr == meta_sk->sk_daddr)) {
			/* For MPTCP_HW_EXT_CARRIER_INVALID, make sure it can
			 * start a subflow with the dst ip address of master flow
			 */
			l_carrier_bit |= (1 << i);
			mptcp_debug("%s: l_carrier_bit: %x, \n", __func__, l_carrier_bit);
		}
	}

	remaddr4->forbid_bitfield = 0;
	remaddr4->forbid_bitfield |= mptcp_local->loc4_bits;
	remaddr4->forbid_bitfield &= ~l_carrier_bit;
	mptcp_info("%s: new set connected: %x, prohibit: %x, mptcp_local->loc4_bits: %x, l_carrier_bit: %x\n", __func__, remaddr4->bitfield, remaddr4->forbid_bitfield, mptcp_local->loc4_bits, l_carrier_bit);
	kfree(mptcp_local);
	return;
}

/* fixme: if both interface are to same operator we connect using same local interface */
static void full_mesh_add_interface_update_forbid_path(struct sock *meta_sk)
{
	int i;
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	if (tcp_sk(meta_sk)->mptcp_cap_flag != MPTCP_CAP_UID_DIP_DPORT)
		return;

	mptcp_debug("%s: remote addr: %x\n", __func__, fmp->rem4_bits);
	mptcp_for_each_bit_set(fmp->rem4_bits, i)
		full_mesh_update_forbid_path(&fmp->remaddr4[i], meta_sk);

	return;
}

static void mptcp_addv4_raddr(struct mptcp_cb *mpcb,
			      const struct in_addr *addr,
			      __be16 port, u8 id)
{
	int i;
	struct fullmesh_rem4 *rem4;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	char src_ip_str[INET_ADDRSTRLEN];
	char dst_ip_str[INET_ADDRSTRLEN];

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		rem4 = &fmp->remaddr4[i];

		/* Address is already in the list --- continue */
		if (rem4->rem4_id == id &&
		    rem4->addr.s_addr == addr->s_addr && rem4->port == port)
			return;

		/* This may be the case, when the peer is behind a NAT. He is
		 * trying to JOIN, thus sending the JOIN with a certain ID.
		 * However the src_addr of the IP-packet has been changed. We
		 * update the addr in the list, because this is the address as
		 * OUR BOX sees it.
		 */
		if (rem4->rem4_id == id && rem4->addr.s_addr != addr->s_addr) {
			/* update the address */
			mptcp_debug("%s: updating old addr:%s to addr %s with id:%d\n",
				    __func__,
				    anonymousIPv4addr(rem4->addr.s_addr, src_ip_str, INET_ADDRSTRLEN),
				    anonymousIPv4addr(addr->s_addr, dst_ip_str, INET_ADDRSTRLEN),
				    id);
			rem4->addr.s_addr = addr->s_addr;
			rem4->port = port;
			mpcb->list_rcvd = 1;
			full_mesh_update_forbid_path(rem4, mpcb->meta_sk);
			return;
		}
	}

	i = mptcp_find_free_index(fmp->rem4_bits);
	/* Do we have already the maximum number of local/remote addresses? */
	if (i < 0) {
		mptcp_debug("%s: At max num of remote addresses: %d --- not adding address: %s\n",
			    __func__, MPTCP_MAX_ADDR,
				anonymousIPv4addr(addr->s_addr, dst_ip_str, INET_ADDRSTRLEN));
		return;
	}

	mptcp_debug("%s: add remote addresses id: %d address: %s\n",
		__func__, id, anonymousIPv4addr(addr->s_addr, dst_ip_str, INET_ADDRSTRLEN));

	rem4 = &fmp->remaddr4[i];

	/* Address is not known yet, store it */
	rem4->addr.s_addr = addr->s_addr;
	rem4->port = port;
	rem4->bitfield = 0;
	rem4->forbid_bitfield = 0;
	rem4->retry_bitfield = 0;
	rem4->rem4_id = id;
	mpcb->list_rcvd = 1;
	fmp->rem4_bits |= (1 << i);

	full_mesh_update_forbid_path(rem4, mpcb->meta_sk);
	return;
}

static void mptcp_addv6_raddr(struct mptcp_cb *mpcb,
			      const struct in6_addr *addr,
			      __be16 port, u8 id)
{
	int i;
	struct fullmesh_rem6 *rem6;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		rem6 = &fmp->remaddr6[i];

		/* Address is already in the list --- continue */
		if (rem6->rem6_id == id &&
		    ipv6_addr_equal(&rem6->addr, addr) && rem6->port == port)
			return;

		/* This may be the case, when the peer is behind a NAT. He is
		 * trying to JOIN, thus sending the JOIN with a certain ID.
		 * However the src_addr of the IP-packet has been changed. We
		 * update the addr in the list, because this is the address as
		 * OUR BOX sees it.
		 */
		if (rem6->rem6_id == id) {
			/* update the address */
			mptcp_debug("%s: updating old addr: %pI6 to addr %pI6 with id:%d\n",
				    __func__, &rem6->addr, addr, id);
			rem6->addr = *addr;
			rem6->port = port;
			mpcb->list_rcvd = 1;
			return;
		}
	}

	i = mptcp_find_free_index(fmp->rem6_bits);
	/* Do we have already the maximum number of local/remote addresses? */
	if (i < 0) {
		mptcp_debug("%s: At max num of remote addresses: %d --- not adding address: %pI6\n",
			    __func__, MPTCP_MAX_ADDR, addr);
		return;
	}

	rem6 = &fmp->remaddr6[i];

	/* Address is not known yet, store it */
	rem6->addr = *addr;
	rem6->port = port;
	rem6->bitfield = 0;
	rem6->retry_bitfield = 0;
	rem6->rem6_id = id;
	mpcb->list_rcvd = 1;
	fmp->rem6_bits |= (1 << i);

	return;
}

static void mptcp_v4_rem_raddress(struct mptcp_cb *mpcb, u8 id)
{
	int i;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		if (fmp->remaddr4[i].rem4_id == id) {
			/* remove address from bitfield */
			fmp->rem4_bits &= ~(1 << i);

			break;
		}
	}
}

static void mptcp_v6_rem_raddress(const struct mptcp_cb *mpcb, u8 id)
{
	int i;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		if (fmp->remaddr6[i].rem6_id == id) {
			/* remove address from bitfield */
			fmp->rem6_bits &= ~(1 << i);

			break;
		}
	}
}

/* Sets the bitfield of the remote-address field */
static void mptcp_v4_set_init_addr_bit(const struct mptcp_cb *mpcb,
				       const struct in_addr *addr, u8 index)
{
	int i;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		if (fmp->remaddr4[i].addr.s_addr == addr->s_addr) {
			fmp->remaddr4[i].bitfield |= (1 << index);
			return;
		}
	}
}

/* Sets the bitfield of the remote-address field */
static void mptcp_v6_set_init_addr_bit(struct mptcp_cb *mpcb,
				       const struct in6_addr *addr, u8 index)
{
	int i;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		if (ipv6_addr_equal(&fmp->remaddr6[i].addr, addr)) {
			fmp->remaddr6[i].bitfield |= (1 << index);
			return;
		}
	}
}

static void mptcp_set_init_addr_bit(struct mptcp_cb *mpcb,
				    const union inet_addr *addr,
				    sa_family_t family, u8 id)
{
	if (family == AF_INET)
		mptcp_v4_set_init_addr_bit(mpcb, &addr->in, id);
	else
		mptcp_v6_set_init_addr_bit(mpcb, &addr->in6, id);
}

static void mptcp_v4_subflows(struct sock *meta_sk,
			      const struct mptcp_loc4 *loc,
			      struct mptcp_rem4 *rem)
{
	int i;

	for (i = 1; i < num_subflows; i++)
		mptcp_init4_subsockets(meta_sk, loc, rem);
}

#if IS_ENABLED(CONFIG_IPV6)
static void mptcp_v6_subflows(struct sock *meta_sk,
			      const struct mptcp_loc6 *loc,
			      struct mptcp_rem6 *rem)
{
	int i;

	for (i = 1; i < num_subflows; i++)
		mptcp_init6_subsockets(meta_sk, loc, rem);
}
#endif

static void retry_subflow_worker(struct work_struct *work)
{
	struct delayed_work *delayed_work = container_of(work,
							 struct delayed_work,
							 work);
	struct fullmesh_priv *fmp = container_of(delayed_work,
						 struct fullmesh_priv,
						 subflow_retry_work);
	struct mptcp_cb *mpcb = fmp->mpcb;
	struct sock *meta_sk = mpcb->meta_sk;
	struct mptcp_loc_addr *mptcp_local;
	struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(meta_sk));
	int iter = 0, i;

	/* We need a local (stable) copy of the address-list. Really, it is not
	 * such a big deal, if the address-list is not 100% up-to-date.
	 */
	rcu_read_lock_bh();
	mptcp_local = rcu_dereference_bh(fm_ns->local);
	mptcp_local = kmemdup(mptcp_local, sizeof(*mptcp_local), GFP_ATOMIC);
	rcu_read_unlock_bh();

	if (!mptcp_local)
		return;

next_subflow:
	if (iter) {
		release_sock(meta_sk);
		mutex_unlock(&mpcb->mpcb_mutex);

		cond_resched();
	}
	mutex_lock(&mpcb->mpcb_mutex);
	lock_sock_nested(meta_sk, SINGLE_DEPTH_NESTING);

	iter++;

	if (sock_flag(meta_sk, SOCK_DEAD))
		goto exit;

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		struct fullmesh_rem4 *rem = &fmp->remaddr4[i];
		/* Do we need to retry establishing a subflow ? */
		if (rem->retry_bitfield) {
			int i = mptcp_find_free_index(~rem->retry_bitfield);
			struct mptcp_rem4 rem4;

			rem->bitfield |= (1 << i);
			rem->retry_bitfield &= ~(1 << i);

			rem4.addr = rem->addr;
			rem4.port = rem->port;
			rem4.rem4_id = rem->rem4_id;

			mptcp_init4_subsockets(meta_sk, &mptcp_local->locaddr4[i], &rem4);
			mptcp_v4_subflows(meta_sk,
					  &mptcp_local->locaddr4[i],
					  &rem4);
			goto next_subflow;
		}
	}

#if IS_ENABLED(CONFIG_IPV6)
	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		struct fullmesh_rem6 *rem = &fmp->remaddr6[i];

		/* Do we need to retry establishing a subflow ? */
		if (rem->retry_bitfield) {
			int i = mptcp_find_free_index(~rem->retry_bitfield);
			struct mptcp_rem6 rem6;

			rem->bitfield |= (1 << i);
			rem->retry_bitfield &= ~(1 << i);

			rem6.addr = rem->addr;
			rem6.port = rem->port;
			rem6.rem6_id = rem->rem6_id;

			mptcp_init6_subsockets(meta_sk, &mptcp_local->locaddr6[i], &rem6);
			mptcp_v6_subflows(meta_sk,
					  &mptcp_local->locaddr6[i],
					  &rem6);
			goto next_subflow;
		}
	}
#endif

exit:
	kfree(mptcp_local);
	release_sock(meta_sk);
	mutex_unlock(&mpcb->mpcb_mutex);
	sock_put(meta_sk);
}

/**
 * Create all new subflows, by doing calls to mptcp_initX_subsockets
 *
 * This function uses a goto next_subflow, to allow releasing the lock between
 * new subflows and giving other processes a chance to do some work on the
 * socket and potentially finishing the communication.
 **/
static void create_subflow_worker(struct work_struct *work)
{
	struct fullmesh_priv *fmp = container_of(work, struct fullmesh_priv,
						 subflow_work);
	struct mptcp_cb *mpcb = fmp->mpcb;
	struct sock *meta_sk = mpcb->meta_sk;
	struct mptcp_loc_addr *mptcp_local;
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(meta_sk));
	int iter = 0, retry = 0;
	int i;

	/* We need a local (stable) copy of the address-list. Really, it is not
	 * such a big deal, if the address-list is not 100% up-to-date.
	 */
	rcu_read_lock_bh();
	mptcp_local = rcu_dereference_bh(fm_ns->local);
	mptcp_local = kmemdup(mptcp_local, sizeof(*mptcp_local), GFP_ATOMIC);
	rcu_read_unlock_bh();

	if (!mptcp_local)
		return;

next_subflow:
	if (iter) {
		release_sock(meta_sk);
		mutex_unlock(&mpcb->mpcb_mutex);

		cond_resched();
	}
	mutex_lock(&mpcb->mpcb_mutex);
	lock_sock_nested(meta_sk, SINGLE_DEPTH_NESTING);

	if (sock_flag(meta_sk, SOCK_DEAD))
		goto exit;

	if (mpcb->master_sk &&
	    !tcp_sk(mpcb->master_sk)->mptcp->fully_established)
		goto exit;

	/* Create the additional subflows for the first pair */
	if (fmp->first_pair == 0 && mpcb->master_sk) {
		struct mptcp_loc4 loc;
		struct mptcp_rem4 rem;

		loc.addr.s_addr = inet_sk(meta_sk)->inet_saddr;
		loc.loc4_id = 0;
		loc.low_prio = 0;
		loc.if_idx = mpcb->master_sk->sk_bound_dev_if;

		rem.addr.s_addr = inet_sk(meta_sk)->inet_daddr;
		rem.port = inet_sk(meta_sk)->inet_dport;
		rem.rem4_id = 0; /* Default 0 */

		mptcp_v4_subflows(meta_sk, &loc, &rem);

		fmp->first_pair = 1;
	}
	iter++;

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		struct fullmesh_rem4 *rem;
		u8 remaining_bits;

		rem = &fmp->remaddr4[i];
		remaining_bits = ~(rem->bitfield | rem->forbid_bitfield) & mptcp_local->loc4_bits;
		mptcp_debug("%s: remaining: %x, connected: %x, prohibit: %x, mptcp_local->loc4_bits: %x\n", __func__, remaining_bits, rem->bitfield, rem->forbid_bitfield, mptcp_local->loc4_bits);
		/* Are there still combinations to handle? */
		if (remaining_bits) {
			int i = mptcp_find_free_index(~remaining_bits);
			struct mptcp_rem4 rem4;

			rem->bitfield |= (1 << i);

			rem4.addr = rem->addr;
			rem4.port = rem->port;
			rem4.rem4_id = rem->rem4_id;

			/* If a route is not yet available then retry once */
			if (mptcp_init4_subsockets(meta_sk, &mptcp_local->locaddr4[i],
						   &rem4) == -ENETUNREACH)
				retry = rem->retry_bitfield |= (1 << i);
			else
				mptcp_v4_subflows(meta_sk,
						  &mptcp_local->locaddr4[i],
						  &rem4);
			goto next_subflow;
		}
	}

#if IS_ENABLED(CONFIG_IPV6)
	if (fmp->first_pair == 0 && mpcb->master_sk) {
			struct mptcp_loc6 loc;
			struct mptcp_rem6 rem;

			loc.addr = inet6_sk(meta_sk)->saddr;
			loc.loc6_id = 0;
			loc.low_prio = 0;
			loc.if_idx = mpcb->master_sk->sk_bound_dev_if;

			rem.addr = meta_sk->sk_v6_daddr;
			rem.port = inet_sk(meta_sk)->inet_dport;
			rem.rem6_id = 0; /* Default 0 */

			mptcp_v6_subflows(meta_sk, &loc, &rem);

			fmp->first_pair = 1;
	}
	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		struct fullmesh_rem6 *rem;
		u8 remaining_bits;

		rem = &fmp->remaddr6[i];
		remaining_bits = ~(rem->bitfield) & mptcp_local->loc6_bits;

		/* Are there still combinations to handle? */
		if (remaining_bits) {
			int i = mptcp_find_free_index(~remaining_bits);
			struct mptcp_rem6 rem6;

			rem->bitfield |= (1 << i);

			rem6.addr = rem->addr;
			rem6.port = rem->port;
			rem6.rem6_id = rem->rem6_id;

			/* If a route is not yet available then retry once */
			if (mptcp_init6_subsockets(meta_sk, &mptcp_local->locaddr6[i],
						   &rem6) == -ENETUNREACH)
				retry = rem->retry_bitfield |= (1 << i);
			else
				mptcp_v6_subflows(meta_sk,
						  &mptcp_local->locaddr6[i],
						  &rem6);
			goto next_subflow;
		}
	}
#endif

	if (retry && !delayed_work_pending(&fmp->subflow_retry_work)) {
		sock_hold(meta_sk);
		queue_delayed_work(mptcp_wq, &fmp->subflow_retry_work,
				   msecs_to_jiffies(MPTCP_SUBFLOW_RETRY_DELAY));
	}

exit:
	kfree(mptcp_local);
	release_sock(meta_sk);
	mutex_unlock(&mpcb->mpcb_mutex);
	sock_put(meta_sk);
}

static void announce_remove_addr(u8 addr_id, struct sock *meta_sk)
{
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	struct sock *sk = mptcp_select_ack_sock(meta_sk);

	fmp->remove_addrs |= (1 << addr_id);
	mpcb->addr_signal = 1;

	if (sk)
		tcp_send_ack(sk);
}

static void update_addr_bitfields(struct sock *meta_sk,
				  const struct mptcp_loc_addr *mptcp_local)
{
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	int i;

	/* The bits in announced_addrs_* always match with loc*_bits. So, a
	 * simple & operation unsets the correct bits, because these go from
	 * announced to non-announced
	 */
	fmp->announced_addrs_v4 &= mptcp_local->loc4_bits;

	mptcp_for_each_bit_set(fmp->rem4_bits, i) {
		fmp->remaddr4[i].bitfield &= mptcp_local->loc4_bits;
		fmp->remaddr4[i].retry_bitfield &= mptcp_local->loc4_bits;
	}

	fmp->announced_addrs_v6 &= mptcp_local->loc6_bits;

	mptcp_for_each_bit_set(fmp->rem6_bits, i) {
		fmp->remaddr6[i].bitfield &= mptcp_local->loc6_bits;
		fmp->remaddr6[i].retry_bitfield &= mptcp_local->loc6_bits;
	}
}

static int mptcp_find_address(const struct mptcp_loc_addr *mptcp_local,
			      sa_family_t family, const union inet_addr *addr,
			      int if_idx)
{
	int i;
	u8 loc_bits;
	bool found = false;

	if (family == AF_INET)
		loc_bits = mptcp_local->loc4_bits;
	else
		loc_bits = mptcp_local->loc6_bits;

	mptcp_for_each_bit_set(loc_bits, i) {
		if (family == AF_INET &&
		    (!if_idx || mptcp_local->locaddr4[i].if_idx == if_idx) &&
		    mptcp_local->locaddr4[i].addr.s_addr == addr->in.s_addr) {
			found = true;
			break;
		}
		if (family == AF_INET6 &&
		    (!if_idx || mptcp_local->locaddr6[i].if_idx == if_idx) &&
		    ipv6_addr_equal(&mptcp_local->locaddr6[i].addr,
				    &addr->in6)) {
			found = true;
			break;
		}
	}

	if (!found)
		return -1;

	return i;
}

static int mptcp_find_address_transp(const struct mptcp_loc_addr *mptcp_local,
				     sa_family_t family, int if_idx)
{
	bool found = false;
	u8 loc_bits;
	int i;

	if (family == AF_INET)
		loc_bits = mptcp_local->loc4_bits;
	else
		loc_bits = mptcp_local->loc6_bits;

	mptcp_for_each_bit_set(loc_bits, i) {
		if (family == AF_INET &&
		    (!if_idx || mptcp_local->locaddr4[i].if_idx == if_idx)) {
			found = true;
			break;
		}
		if (family == AF_INET6 &&
		    (!if_idx || mptcp_local->locaddr6[i].if_idx == if_idx)) {
			found = true;
			break;
		}
	}

	if (!found)
		return -1;

	return i;
}

static bool mptcp_is_sock_closed_for_prim_iface_on(struct sock *meta_sk, int if_idx)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct mptcp_cb *mpcb;

	mpcb = meta_tp->mpcb;

	if (!meta_tp->user_switch) {
		struct net_device *net_dev;
		struct sock *sk, *tmpsk;

		net_dev = dev_get_by_name(sock_net(meta_sk), meta_tp->prim_iface);
		if (net_dev) {
			if (if_idx == net_dev->ifindex) {
				/* prim iface is up, so close the subflows on the secondry iface,
				 * if all subflow is closed , the meta_sk alos will be closed.
				 * it is ok, for the user app will create a new socket on prim iface.
				 */
				mptcp_for_each_sk_safe(mpcb, sk, tmpsk) {
					mptcp_sub_force_close(sk);
				}
				mptcp_info("%s: close the secondry subflow for prim_iface is up\n", __func__);
				dev_put(net_dev);
				return true;
			}
			dev_put(net_dev);
		} else
			mptcp_info("%s: can't find the net_dev\n", __func__);
	}

	return false;
}

static void mptcp_address_worker(struct work_struct *work)
{
	const struct delayed_work *delayed_work = container_of(work,
							 struct delayed_work,
							 work);
	struct mptcp_fm_ns *fm_ns = container_of(delayed_work,
						 struct mptcp_fm_ns,
						 address_worker);
	struct net *net = fm_ns->net;
	struct mptcp_addr_event *event = NULL;
	struct mptcp_loc_addr *mptcp_local, *old;
	int i, id = -1; /* id is used in the socket-code on a delete-event */
	bool success; /* Used to indicate if we succeeded handling the event */

next_event:
	success = false;
	kfree(event);

	/* First, let's dequeue an event from our event-list */
	rcu_read_lock_bh();
	spin_lock(&fm_ns->local_lock);

	event = list_first_entry_or_null(&fm_ns->events,
					 struct mptcp_addr_event, list);
	if (!event) {
		spin_unlock(&fm_ns->local_lock);
		rcu_read_unlock_bh();
		return;
	}

	list_del(&event->list);

	mptcp_local = rcu_dereference_bh(fm_ns->local);

	if (event->code == MPTCP_EVENT_DEL) {
		id = mptcp_find_address(mptcp_local, event->family,
					&event->addr, event->if_idx);

		/* Not in the list - so we don't care */
		if (id < 0) {
			mptcp_debug("%s could not find id\n", __func__);
			goto duno;
		}

		old = mptcp_local;
		mptcp_local = kmemdup(mptcp_local, sizeof(*mptcp_local),
				      GFP_ATOMIC);
		if (!mptcp_local)
			goto duno;

		if (event->family == AF_INET)
			mptcp_local->loc4_bits &= ~(1 << id);
		else
			mptcp_local->loc6_bits &= ~(1 << id);

		rcu_assign_pointer(fm_ns->local, mptcp_local);
		kfree_rcu(old, rcu);
	} else {
		int i = mptcp_find_address(mptcp_local, event->family,
					   &event->addr, event->if_idx);
		int j = i;

		if (j < 0) {
			/* Not in the list, so we have to find an empty slot */
			if (event->family == AF_INET)
				i = __mptcp_find_free_index(mptcp_local->loc4_bits,
							    mptcp_local->next_v4_index);
			if (event->family == AF_INET6)
				i = __mptcp_find_free_index(mptcp_local->loc6_bits,
							    mptcp_local->next_v6_index);

			if (i < 0) {
				mptcp_debug("%s no more space\n", __func__);
				goto duno;
			}

			/* It might have been a MOD-event. */
			event->code = MPTCP_EVENT_ADD;
		} else {
			/* Let's check if anything changes */
			if (event->family == AF_INET &&
			    event->low_prio == mptcp_local->locaddr4[i].low_prio)
				goto duno;

			if (event->family == AF_INET6 &&
			    event->low_prio == mptcp_local->locaddr6[i].low_prio)
				goto duno;
		}

		old = mptcp_local;
		mptcp_local = kmemdup(mptcp_local, sizeof(*mptcp_local),
				      GFP_ATOMIC);
		if (!mptcp_local)
			goto duno;

		if (event->family == AF_INET) {
			mptcp_local->locaddr4[i].addr.s_addr = event->addr.in.s_addr;
			mptcp_local->locaddr4[i].loc4_id = i + 1;
			mptcp_local->locaddr4[i].low_prio = event->low_prio;
			mptcp_local->locaddr4[i].if_idx = event->if_idx;
		} else {
			mptcp_local->locaddr6[i].addr = event->addr.in6;
			mptcp_local->locaddr6[i].loc6_id = i + MPTCP_MAX_ADDR;
			mptcp_local->locaddr6[i].low_prio = event->low_prio;
			mptcp_local->locaddr6[i].if_idx = event->if_idx;
		}

		if (j < 0) {
			if (event->family == AF_INET) {
				mptcp_local->loc4_bits |= (1 << i);
				mptcp_local->next_v4_index = i + 1;
			} else {
				mptcp_local->loc6_bits |= (1 << i);
				mptcp_local->next_v6_index = i + 1;
			}
		}

		rcu_assign_pointer(fm_ns->local, mptcp_local);
		kfree_rcu(old, rcu);
	}
	success = true;

duno:
	spin_unlock(&fm_ns->local_lock);
	rcu_read_unlock_bh();

	if (!success)
		goto next_event;

	/* Now we iterate over the MPTCP-sockets and apply the event. */
	for (i = 0; i < MPTCP_HASH_SIZE; i++) {
		const struct hlist_nulls_node *node;
		struct tcp_sock *meta_tp;

		rcu_read_lock_bh();
		hlist_nulls_for_each_entry_rcu(meta_tp, node, &tk_hashtable[i],
					       tk_table) {
			struct sock *meta_sk = (struct sock *)meta_tp, *sk;
			bool meta_v4 = meta_sk->sk_family == AF_INET;
			struct mptcp_cb *mpcb;

			if (sock_net(meta_sk) != net)
				continue;

			if (meta_v4) {
				/* skip IPv6 events if meta is IPv4 */
				if (event->family == AF_INET6)
					continue;
			} else if (event->family == AF_INET && meta_sk->sk_ipv6only) {
				/* skip IPv4 events if IPV6_V6ONLY is set */
				continue;
			}

			if (unlikely(!atomic_inc_not_zero(&meta_sk->sk_refcnt)))
				continue;

			bh_lock_sock(meta_sk);

			mpcb = meta_tp->mpcb;
			if (!mpcb)
				goto next;

			if (!mptcp(meta_tp) || !is_meta_sk(meta_sk) ||
			    mpcb->infinite_mapping_snd ||
			    mpcb->infinite_mapping_rcv ||
			    mpcb->send_infinite_mapping)
				goto next;

			/* May be that the pm has changed in-between */
			if (mpcb->pm_ops != &full_mesh)
				goto next;

			if (sock_owned_by_user(meta_sk)) {
				if (!test_and_set_bit(MPTCP_PATH_MANAGER_DEFERRED,
						      &meta_tp->tsq_flags))
					sock_hold(meta_sk);

				goto next;
			}

			if (event->code == MPTCP_EVENT_ADD) {
				struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

				if (mptcp_is_sock_closed_for_prim_iface_on(meta_sk, event->if_idx))
					goto next;

				fmp->add_addr++;
				mpcb->addr_signal = 1;

				sk = mptcp_select_ack_sock(meta_sk);
				if (sk)
					tcp_send_ack(sk);

				mptcp_add_rem_addr_for_sk(meta_sk);

				full_mesh_add_interface_update_forbid_path(meta_sk);
				full_mesh_create_subflows(meta_sk);
			}

			if (event->code == MPTCP_EVENT_DEL) {
				struct sock *sk, *tmpsk;
				struct mptcp_loc_addr *mptcp_local;
				bool found = false;

				mptcp_local = rcu_dereference_bh(fm_ns->local);

				/* In any case, we need to update our bitfields */
				if (id >= 0)
					update_addr_bitfields(meta_sk, mptcp_local);

				/* Look for the socket and remove him */
				mptcp_for_each_sk_safe(mpcb, sk, tmpsk) {
					if ((event->family == AF_INET6 &&
					     (sk->sk_family == AF_INET ||
					      mptcp_v6_is_v4_mapped(sk))) ||
					    (event->family == AF_INET &&
					     (sk->sk_family == AF_INET6 &&
					      !mptcp_v6_is_v4_mapped(sk))))
						continue;

					if (event->family == AF_INET &&
					    (sk->sk_family == AF_INET ||
					     mptcp_v6_is_v4_mapped(sk)) &&
					     inet_sk(sk)->inet_saddr != event->addr.in.s_addr)
						continue;

					if (event->family == AF_INET6 &&
					    sk->sk_family == AF_INET6 &&
					    !ipv6_addr_equal(&inet6_sk(sk)->saddr, &event->addr.in6))
						continue;

					/* Reinject, so that pf = 1 and so we
					 * won't select this one as the
					 * ack-sock.
					 */
					mptcp_reinject_data(sk, 0);

					/* We announce the removal of this id */
					announce_remove_addr(tcp_sk(sk)->mptcp->loc_id, meta_sk);

					mptcp_sub_force_close(sk);
					found = true;
				}

				if (found)
					goto next;

				/* The id may have been given by the event,
				 * matching on a local address. And it may not
				 * have matched on one of the above sockets,
				 * because the client never created a subflow.
				 * So, we have to finally remove it here.
				 */
				if (id >= 0) {
					u8 loc_id = id
						+ (event->family == AF_INET ? 1 : MPTCP_MAX_ADDR);
					announce_remove_addr(loc_id, meta_sk);
				}
			}

			if (event->code == MPTCP_EVENT_MOD) {
				struct sock *sk;

				mptcp_for_each_sk(mpcb, sk) {
					struct tcp_sock *tp = tcp_sk(sk);
					if (event->family == AF_INET &&
					    (sk->sk_family == AF_INET ||
					     mptcp_v6_is_v4_mapped(sk)) &&
					     inet_sk(sk)->inet_saddr == event->addr.in.s_addr) {
						if (event->low_prio != tp->mptcp->low_prio) {
							tp->mptcp->send_mp_prio = 1;
							tp->mptcp->low_prio = event->low_prio;

							tcp_send_ack(sk);
						}
					}

					if (event->family == AF_INET6 &&
					    sk->sk_family == AF_INET6 &&
					    !ipv6_addr_equal(&inet6_sk(sk)->saddr, &event->addr.in6)) {
						if (event->low_prio != tp->mptcp->low_prio) {
							tp->mptcp->send_mp_prio = 1;
							tp->mptcp->low_prio = event->low_prio;

							tcp_send_ack(sk);
						}
					}
				}
			}
next:
			bh_unlock_sock(meta_sk);
			sock_put(meta_sk);
		}
		rcu_read_unlock_bh();
	}
	goto next_event;
}

static struct mptcp_addr_event *lookup_similar_event(const struct net *net,
						     const struct mptcp_addr_event *event)
{
	struct mptcp_addr_event *eventq;
	struct mptcp_fm_ns *fm_ns = fm_get_ns(net);

	list_for_each_entry(eventq, &fm_ns->events, list) {
		if (eventq->family != event->family)
			continue;
		if (eventq->if_idx != event->if_idx)
			continue;
		if (event->family == AF_INET) {
			if (eventq->addr.in.s_addr == event->addr.in.s_addr)
				return eventq;
		} else {
			if (ipv6_addr_equal(&eventq->addr.in6, &event->addr.in6))
				return eventq;
		}
	}
	return NULL;
}

/* We already hold the net-namespace MPTCP-lock */
static void add_pm_event(struct net *net, const struct mptcp_addr_event *event)
{
	struct mptcp_addr_event *eventq = lookup_similar_event(net, event);
	struct mptcp_fm_ns *fm_ns = fm_get_ns(net);

	if (eventq) {
		switch (event->code) {
		case MPTCP_EVENT_DEL:
			mptcp_debug("%s del old_code %u\n", __func__, eventq->code);
			list_del(&eventq->list);
			kfree(eventq);
			break;
		case MPTCP_EVENT_ADD:
			mptcp_debug("%s add old_code %u\n", __func__, eventq->code);
			eventq->low_prio = event->low_prio;
			eventq->code = MPTCP_EVENT_ADD;
			return;
		case MPTCP_EVENT_MOD:
			mptcp_debug("%s mod old_code %u\n", __func__, eventq->code);
			eventq->low_prio = event->low_prio;
			eventq->code = MPTCP_EVENT_MOD;
			return;
		}
	}

	/* OK, we have to add the new address to the wait queue */
	eventq = kmemdup(event, sizeof(struct mptcp_addr_event), GFP_ATOMIC);
	if (!eventq)
		return;

	list_add_tail(&eventq->list, &fm_ns->events);

	/* Create work-queue */
	if (!delayed_work_pending(&fm_ns->address_worker))
		queue_delayed_work(mptcp_wq, &fm_ns->address_worker,
				   msecs_to_jiffies(500));
}

static void addr4_event_handler(const struct in_ifaddr *ifa, unsigned long event,
				struct net *net)
{
	const struct net_device *netdev = ifa->ifa_dev->dev;
	struct mptcp_fm_ns *fm_ns = fm_get_ns(net);
	struct mptcp_addr_event mpevent;

	if (ifa->ifa_scope > RT_SCOPE_LINK ||
	    ipv4_is_loopback(ifa->ifa_local))
		return;

	spin_lock_bh(&fm_ns->local_lock);

	mpevent.family = AF_INET;
	mpevent.addr.in.s_addr = ifa->ifa_local;
	mpevent.low_prio = (netdev->flags & IFF_MPBACKUP) ? 1 : 0;
	mpevent.if_idx  = netdev->ifindex;

	if (event == NETDEV_DOWN || !netif_running(netdev) ||
	    (netdev->flags & IFF_NOMULTIPATH) || !(netdev->flags & IFF_UP))
		mpevent.code = MPTCP_EVENT_DEL;
	else if (event == NETDEV_UP)
		mpevent.code = MPTCP_EVENT_ADD;
	else if (event == NETDEV_CHANGE)
		mpevent.code = MPTCP_EVENT_MOD;

	mptcp_debug("%s created event for %pI4, code %u prio %u idx %u\n", __func__,
		    &ifa->ifa_local, mpevent.code, mpevent.low_prio, mpevent.if_idx);
	add_pm_event(net, &mpevent);

	spin_unlock_bh(&fm_ns->local_lock);
	return;
}

/* React on IPv4-addr add/rem-events */
static int mptcp_pm_inetaddr_event(struct notifier_block *this,
				   unsigned long event, void *ptr)
{
	const struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
	struct net *net = dev_net(ifa->ifa_dev->dev);

	if (!(event == NETDEV_UP || event == NETDEV_DOWN ||
	      event == NETDEV_CHANGE))
		return NOTIFY_DONE;

	addr4_event_handler(ifa, event, net);

	return NOTIFY_DONE;
}

static struct notifier_block mptcp_pm_inetaddr_notifier = {
		.notifier_call = mptcp_pm_inetaddr_event,
};

#if IS_ENABLED(CONFIG_IPV6)

/* IPV6-related address/interface watchers */
struct mptcp_dad_data {
	struct timer_list timer;
	struct inet6_ifaddr *ifa;
};

static void dad_callback(unsigned long arg);
static int inet6_addr_event(struct notifier_block *this,
				     unsigned long event, void *ptr);

static bool ipv6_dad_finished(const struct inet6_ifaddr *ifa)
{
	return !(ifa->flags & IFA_F_TENTATIVE) ||
	       ifa->state > INET6_IFADDR_STATE_DAD;
}

static void dad_init_timer(struct mptcp_dad_data *data,
				 struct inet6_ifaddr *ifa)
{
	data->ifa = ifa;
	data->timer.data = (unsigned long)data;
	data->timer.function = dad_callback;
	if (ifa->idev->cnf.rtr_solicit_delay &&
	    (ifa->idev->cnf.rtr_solicit_delay < (HZ/100)))
		data->timer.expires = jiffies + ifa->idev->cnf.rtr_solicit_delay;
	else
		data->timer.expires = jiffies + (HZ/100);
}

static void dad_callback(unsigned long arg)
{
	struct mptcp_dad_data *data = (struct mptcp_dad_data *)arg;

	if (data->ifa->idev && data->ifa->idev->dev &&
	    data->ifa->idev->dev->reg_state != NETREG_REGISTERED)
		goto exit;

	/* DAD failed or IP brought down? */
	if (data->ifa->state == INET6_IFADDR_STATE_ERRDAD ||
	    data->ifa->state == INET6_IFADDR_STATE_DEAD)
		goto exit;

	if (!ipv6_dad_finished(data->ifa)) {
		dad_init_timer(data, data->ifa);
		add_timer(&data->timer);
		return;
	}

	inet6_addr_event(NULL, NETDEV_UP, data->ifa);

exit:
	in6_ifa_put(data->ifa);
	kfree(data);
}

static inline void dad_setup_timer(struct inet6_ifaddr *ifa)
{
	struct mptcp_dad_data *data;

	data = kmalloc(sizeof(*data), GFP_ATOMIC);

	if (!data)
		return;

	init_timer(&data->timer);
	dad_init_timer(data, ifa);
	add_timer(&data->timer);
	in6_ifa_hold(ifa);
}

static void addr6_event_handler(const struct inet6_ifaddr *ifa, unsigned long event,
				struct net *net)
{
	const struct net_device *netdev = ifa->idev->dev;
	int addr_type = ipv6_addr_type(&ifa->addr);
	struct mptcp_fm_ns *fm_ns = fm_get_ns(net);
	struct mptcp_addr_event mpevent;

	if (ifa->scope > RT_SCOPE_LINK ||
	    addr_type == IPV6_ADDR_ANY ||
	    (addr_type & IPV6_ADDR_LOOPBACK) ||
	    (addr_type & IPV6_ADDR_LINKLOCAL))
		return;

	spin_lock_bh(&fm_ns->local_lock);

	mpevent.family = AF_INET6;
	mpevent.addr.in6 = ifa->addr;
	mpevent.low_prio = (netdev->flags & IFF_MPBACKUP) ? 1 : 0;
	mpevent.if_idx = netdev->ifindex;

	if (event == NETDEV_DOWN || !netif_running(netdev) ||
	    (netdev->flags & IFF_NOMULTIPATH) || !(netdev->flags & IFF_UP))
		mpevent.code = MPTCP_EVENT_DEL;
	else if (event == NETDEV_UP)
		mpevent.code = MPTCP_EVENT_ADD;
	else if (event == NETDEV_CHANGE)
		mpevent.code = MPTCP_EVENT_MOD;

	mptcp_debug("%s created event for %pI6, code %u prio %u\n", __func__,
		    &ifa->addr, mpevent.code, mpevent.low_prio);
	add_pm_event(net, &mpevent);

	spin_unlock_bh(&fm_ns->local_lock);
	return;
}

/* React on IPv6-addr add/rem-events */
static int inet6_addr_event(struct notifier_block *this, unsigned long event,
			    void *ptr)
{
	struct inet6_ifaddr *ifa6 = (struct inet6_ifaddr *)ptr;
	struct net *net = dev_net(ifa6->idev->dev);

	if (!(event == NETDEV_UP || event == NETDEV_DOWN ||
	      event == NETDEV_CHANGE))
		return NOTIFY_DONE;

	if ((ifa6->idev->dev->reg_state == NETREG_REGISTERED) &&
	    !ipv6_dad_finished(ifa6))
		dad_setup_timer(ifa6);
	else
		addr6_event_handler(ifa6, event, net);

	return NOTIFY_DONE;
}

static struct notifier_block inet6_addr_notifier = {
		.notifier_call = inet6_addr_event,
};

#endif

/* React on ifup/down-events */
static int netdev_event(struct notifier_block *this, unsigned long event,
			void *ptr)
{
	const struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct in_device *in_dev;
#if IS_ENABLED(CONFIG_IPV6)
	struct inet6_dev *in6_dev;
#endif

	if (!(event == NETDEV_UP || event == NETDEV_DOWN ||
	      event == NETDEV_CHANGE))
		return NOTIFY_DONE;

	rcu_read_lock();
	in_dev = __in_dev_get_rtnl(dev);

	if (in_dev) {
		for_ifa(in_dev) {
			mptcp_pm_inetaddr_event(NULL, event, ifa);
		} endfor_ifa(in_dev);
	}

#if IS_ENABLED(CONFIG_IPV6)
	in6_dev = __in6_dev_get(dev);

	if (in6_dev) {
		struct inet6_ifaddr *ifa6;
		list_for_each_entry(ifa6, &in6_dev->addr_list, if_list)
			inet6_addr_event(NULL, event, ifa6);
	}
#endif

	rcu_read_unlock();
	return NOTIFY_DONE;
}

static struct notifier_block mptcp_pm_netdev_notifier = {
		.notifier_call = netdev_event,
};

static void full_mesh_add_raddr(struct mptcp_cb *mpcb,
				const union inet_addr *addr,
				sa_family_t family, __be16 port, u8 id)
{
	if (family == AF_INET)
		mptcp_addv4_raddr(mpcb, &addr->in, port, id);
	else
		mptcp_addv6_raddr(mpcb, &addr->in6, port, id);
}

static void full_mesh_new_session(const struct sock *meta_sk)
{
	struct mptcp_loc_addr *mptcp_local;
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(meta_sk));
	struct tcp_sock *master_tp = tcp_sk(mpcb->master_sk);
	int i, index, if_idx = 0;
	union inet_addr saddr, daddr;
	sa_family_t family = AF_INET;
	bool meta_v4 = meta_sk->sk_family == AF_INET;

	/* Init local variables necessary for the rest */
	if (meta_sk->sk_family == AF_INET || mptcp_v6_is_v4_mapped(meta_sk)) {
		saddr.ip = inet_sk(meta_sk)->inet_saddr;
		daddr.ip = inet_sk(meta_sk)->inet_daddr;
		if_idx = mpcb->master_sk->sk_bound_dev_if;
		family = AF_INET;
#if IS_ENABLED(CONFIG_IPV6)
	} else {
		saddr.in6 = inet6_sk(meta_sk)->saddr;
		daddr.in6 = meta_sk->sk_v6_daddr;
		if_idx = mpcb->master_sk->sk_bound_dev_if;
		family = AF_INET6;
#endif
	}

	if (inet_sk(meta_sk)->transparent)
		if_idx = inet_sk(meta_sk)->rx_dst_ifindex;

	rcu_read_lock_bh();
	mptcp_local = rcu_dereference(fm_ns->local);

	if (inet_sk(meta_sk)->transparent)
		index = mptcp_find_address_transp(mptcp_local, family, if_idx);
	else
		index = mptcp_find_address(mptcp_local, family, &saddr, if_idx);
	if (index < 0)
		goto fallback;

	if (family == AF_INET) {
		if (mptcp_local->locaddr4[index].low_prio)
			master_tp->mptcp->low_prio = mptcp_local->locaddr4[index].low_prio;
	} else {
		if (mptcp_local->locaddr6[index].low_prio)
			master_tp->mptcp->low_prio = mptcp_local->locaddr6[index].low_prio;
	}
	mptcp_debug("%s master_tp->mptcp->low_prio:%u\n", __func__, master_tp->mptcp->low_prio);
	master_tp->mptcp->send_mp_prio = master_tp->mptcp->low_prio;

	full_mesh_add_raddr(mpcb, &daddr, family, 0, 0);
	mptcp_set_init_addr_bit(mpcb, &daddr, family, index);

	/* Initialize workqueue-struct */
	INIT_WORK(&fmp->subflow_work, create_subflow_worker);
	INIT_DELAYED_WORK(&fmp->subflow_retry_work, retry_subflow_worker);
	fmp->mpcb = mpcb;

	if (!meta_v4 && meta_sk->sk_ipv6only)
		goto skip_ipv4;

	/* Look for the address among the local addresses */
	mptcp_for_each_bit_set(mptcp_local->loc4_bits, i) {
		__be32 ifa_address = mptcp_local->locaddr4[i].addr.s_addr;

		/* We do not need to announce the initial subflow's address again */
		if (family == AF_INET &&
		    (!if_idx || mptcp_local->locaddr4[i].if_idx == if_idx) &&
		    saddr.ip == ifa_address)
			continue;

		fmp->add_addr++;
		mpcb->addr_signal = 1;
	}

skip_ipv4:
#if IS_ENABLED(CONFIG_IPV6)
	/* skip IPv6 addresses if meta-socket is IPv4 */
	if (meta_v4)
		goto skip_ipv6;

	mptcp_for_each_bit_set(mptcp_local->loc6_bits, i) {
		const struct in6_addr *ifa6 = &mptcp_local->locaddr6[i].addr;

		/* We do not need to announce the initial subflow's address again */
		if (family == AF_INET6 &&
		    (!if_idx || mptcp_local->locaddr6[i].if_idx == if_idx) &&
		    ipv6_addr_equal(&saddr.in6, ifa6))
			continue;

		fmp->add_addr++;
		mpcb->addr_signal = 1;
	}

skip_ipv6:
#endif

	rcu_read_unlock_bh();

	if (family == AF_INET)
		fmp->announced_addrs_v4 |= (1 << index);
	else
		fmp->announced_addrs_v6 |= (1 << index);

	for (i = fmp->add_addr; i && fmp->add_addr; i--)
		tcp_send_ack(mpcb->master_sk);

	if (master_tp->mptcp->send_mp_prio)
		tcp_send_ack(mpcb->master_sk);

	return;

fallback:
	rcu_read_unlock_bh();
	mptcp_fallback_default(mpcb);
	return;
}

static void full_mesh_create_subflows(struct sock *meta_sk)
{
	const struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);

	if (mpcb->infinite_mapping_snd || mpcb->infinite_mapping_rcv ||
	    mpcb->send_infinite_mapping ||
	    mpcb->server_side || sock_flag(meta_sk, SOCK_DEAD))
		return;

	if (mpcb->master_sk &&
	    !tcp_sk(mpcb->master_sk)->mptcp->fully_established)
		return;

	if (!tcp_sk(meta_sk)->user_switch) {
		mptcp_info("%s: user_switch is off\n", __func__);
		return;
	}

	if (!work_pending(&fmp->subflow_work)) {
		sock_hold(meta_sk);
		queue_work(mptcp_wq, &fmp->subflow_work);
	}
}

static void full_mesh_subflow_error(struct sock *meta_sk, struct sock *sk)
{
	const struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;

	if (!create_on_err)
		return;

	if (mpcb->infinite_mapping_snd || mpcb->infinite_mapping_rcv ||
	    mpcb->send_infinite_mapping ||
	    mpcb->server_side || sock_flag(meta_sk, SOCK_DEAD))
		return;

	if (sk->sk_err != ETIMEDOUT)
		return;

	full_mesh_create_subflows(meta_sk);
}

/* Called upon release_sock, if the socket was owned by the user during
 * a path-management event.
 */
static void full_mesh_release_sock(struct sock *meta_sk)
{
	struct mptcp_loc_addr *mptcp_local;
	struct mptcp_cb *mpcb = tcp_sk(meta_sk)->mpcb;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(meta_sk));
	struct sock *sk, *tmpsk;
	bool meta_v4 = meta_sk->sk_family == AF_INET;
	int i;

	rcu_read_lock_bh();
	mptcp_local = rcu_dereference(fm_ns->local);

	if (!meta_v4 && meta_sk->sk_ipv6only)
		goto skip_ipv4;

	mptcp_debug("%s: before remote addr: %x\n", __func__, fmp->rem4_bits);

	/* First, detect modifications or additions */
	mptcp_for_each_bit_set(mptcp_local->loc4_bits, i) {
		struct in_addr ifa = mptcp_local->locaddr4[i].addr;
		bool found = false;

		mptcp_for_each_sk(mpcb, sk) {
			struct tcp_sock *tp = tcp_sk(sk);

			if (sk->sk_family == AF_INET6 &&
			    !mptcp_v6_is_v4_mapped(sk))
				continue;

			if (inet_sk(sk)->inet_saddr != ifa.s_addr)
				continue;

			found = true;

			if (mptcp_local->locaddr4[i].low_prio != tp->mptcp->low_prio) {
				tp->mptcp->send_mp_prio = 1;
				tp->mptcp->low_prio = mptcp_local->locaddr4[i].low_prio;

				tcp_send_ack(sk);
			}
		}

		if (!found) {
			fmp->add_addr++;
			mpcb->addr_signal = 1;

			sk = mptcp_select_ack_sock(meta_sk);
			if (sk)
				tcp_send_ack(sk);
			full_mesh_create_subflows(meta_sk);
		}
	}

skip_ipv4:
#if IS_ENABLED(CONFIG_IPV6)
	/* skip IPv6 addresses if meta-socket is IPv4 */
	if (meta_v4)
		goto removal;

	mptcp_for_each_bit_set(mptcp_local->loc6_bits, i) {
		struct in6_addr ifa = mptcp_local->locaddr6[i].addr;
		bool found = false;

		mptcp_for_each_sk(mpcb, sk) {
			struct tcp_sock *tp = tcp_sk(sk);

			if (sk->sk_family == AF_INET ||
			    mptcp_v6_is_v4_mapped(sk))
				continue;

			if (!ipv6_addr_equal(&inet6_sk(sk)->saddr, &ifa))
				continue;

			found = true;

			if (mptcp_local->locaddr6[i].low_prio != tp->mptcp->low_prio) {
				tp->mptcp->send_mp_prio = 1;
				tp->mptcp->low_prio = mptcp_local->locaddr6[i].low_prio;

				tcp_send_ack(sk);
			}
		}

		if (!found) {
			fmp->add_addr++;
			mpcb->addr_signal = 1;

			sk = mptcp_select_ack_sock(meta_sk);
			if (sk)
				tcp_send_ack(sk);
			full_mesh_create_subflows(meta_sk);
		}
	}

removal:
#endif

	/* Now, detect address-removals */
	mptcp_for_each_sk_safe(mpcb, sk, tmpsk) {
		bool shall_remove = true;

		if (sk->sk_family == AF_INET || mptcp_v6_is_v4_mapped(sk)) {
			mptcp_for_each_bit_set(mptcp_local->loc4_bits, i) {
				if (inet_sk(sk)->inet_saddr == mptcp_local->locaddr4[i].addr.s_addr) {
					shall_remove = false;
					break;
				}
			}
		} else {
			mptcp_for_each_bit_set(mptcp_local->loc6_bits, i) {
				if (ipv6_addr_equal(&inet6_sk(sk)->saddr, &mptcp_local->locaddr6[i].addr)) {
					shall_remove = false;
					break;
				}
			}
		}

		if (shall_remove) {
			/* Reinject, so that pf = 1 and so we
			 * won't select this one as the
			 * ack-sock.
			 */
			mptcp_reinject_data(sk, 0);

			announce_remove_addr(tcp_sk(sk)->mptcp->loc_id,
					     meta_sk);

			mptcp_sub_force_close(sk);
		}
	}

	/* Just call it optimistically. It actually cannot do any harm */
	update_addr_bitfields(meta_sk, mptcp_local);
	mptcp_debug("%s: after remote addr: %x\n", __func__, fmp->rem4_bits);

	rcu_read_unlock_bh();
}

static int full_mesh_get_local_id(sa_family_t family, union inet_addr *addr,
				  struct net *net, bool *low_prio)
{
	struct mptcp_loc_addr *mptcp_local;
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(net);
	int index, id = -1;

	/* Handle the backup-flows */
	rcu_read_lock_bh();
	mptcp_local = rcu_dereference(fm_ns->local);

	index = mptcp_find_address(mptcp_local, family, addr, 0);

	if (index != -1) {
		if (family == AF_INET) {
			id = mptcp_local->locaddr4[index].loc4_id;
			*low_prio = mptcp_local->locaddr4[index].low_prio;
		} else {
			id = mptcp_local->locaddr6[index].loc6_id;
			*low_prio = mptcp_local->locaddr6[index].low_prio;
		}
	}


	rcu_read_unlock_bh();

	return id;
}

static void full_mesh_addr_signal(struct sock *sk, unsigned *size,
				  struct tcp_out_options *opts,
				  struct sk_buff *skb)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct mptcp_cb *mpcb = tp->mpcb;
	struct sock *meta_sk = mpcb->meta_sk;
	struct fullmesh_priv *fmp = fullmesh_get_priv(mpcb);
	struct mptcp_loc_addr *mptcp_local;
	struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(sk));
	int remove_addr_len;
	u8 unannouncedv4 = 0, unannouncedv6 = 0;
	bool meta_v4 = meta_sk->sk_family == AF_INET;

	mpcb->addr_signal = 0;

	if (likely(!fmp->add_addr))
		goto remove_addr;

	rcu_read_lock_bh();
	mptcp_local = rcu_dereference(fm_ns->local);

	if (!meta_v4 && meta_sk->sk_ipv6only)
		goto skip_ipv4;

	/* IPv4 */
	unannouncedv4 = (~fmp->announced_addrs_v4) & mptcp_local->loc4_bits;
	if (unannouncedv4 &&
	    ((mpcb->mptcp_ver == MPTCP_VERSION_0 &&
	    MAX_TCP_OPTION_SPACE - *size >= MPTCP_SUB_LEN_ADD_ADDR4_ALIGN) ||
	    (mpcb->mptcp_ver >= MPTCP_VERSION_1 &&
	    MAX_TCP_OPTION_SPACE - *size >= MPTCP_SUB_LEN_ADD_ADDR4_ALIGN_VER1))) {
		int ind = mptcp_find_free_index(~unannouncedv4);

		opts->options |= OPTION_MPTCP;
		opts->mptcp_options |= OPTION_ADD_ADDR;
		opts->add_addr4.addr_id = mptcp_local->locaddr4[ind].loc4_id;
		opts->add_addr4.addr = mptcp_local->locaddr4[ind].addr;
		opts->add_addr_v4 = 1;
		if (mpcb->mptcp_ver >= MPTCP_VERSION_1) {
			u8 mptcp_hash_mac[20];
			u8 no_key[8];

			*(u64 *)no_key = 0;
			mptcp_hmac_sha1((u8 *)&mpcb->mptcp_loc_key,
					(u8 *)no_key,
					(u32 *)mptcp_hash_mac, 2,
					1, (u8 *)&mptcp_local->locaddr4[ind].loc4_id,
					4, (u8 *)&opts->add_addr4.addr.s_addr);
			opts->add_addr4.trunc_mac = *(u64 *)mptcp_hash_mac;
		}

		if (skb) {
			fmp->announced_addrs_v4 |= (1 << ind);
			fmp->add_addr--;
		}

		if (mpcb->mptcp_ver < MPTCP_VERSION_1)
			*size += MPTCP_SUB_LEN_ADD_ADDR4_ALIGN;
		if (mpcb->mptcp_ver >= MPTCP_VERSION_1)
			*size += MPTCP_SUB_LEN_ADD_ADDR4_ALIGN_VER1;

		goto skip_ipv6;
	}

	if (meta_v4)
		goto skip_ipv6;
skip_ipv4:
	/* IPv6 */
	unannouncedv6 = (~fmp->announced_addrs_v6) & mptcp_local->loc6_bits;
	if (unannouncedv6 &&
	    ((mpcb->mptcp_ver == MPTCP_VERSION_0 &&
	    MAX_TCP_OPTION_SPACE - *size >= MPTCP_SUB_LEN_ADD_ADDR6_ALIGN) ||
	    (mpcb->mptcp_ver >= MPTCP_VERSION_1 &&
	    MAX_TCP_OPTION_SPACE - *size >= MPTCP_SUB_LEN_ADD_ADDR6_ALIGN_VER1))) {
		int ind = mptcp_find_free_index(~unannouncedv6);

		opts->options |= OPTION_MPTCP;
		opts->mptcp_options |= OPTION_ADD_ADDR;
		opts->add_addr6.addr_id = mptcp_local->locaddr6[ind].loc6_id;
		opts->add_addr6.addr = mptcp_local->locaddr6[ind].addr;
		opts->add_addr_v6 = 1;
		if (mpcb->mptcp_ver >= MPTCP_VERSION_1) {
			u8 mptcp_hash_mac[20];
			u8 no_key[8];

			*(u64 *)no_key = 0;
			mptcp_hmac_sha1((u8 *)&mpcb->mptcp_loc_key,
					(u8 *)no_key,
					(u32 *)mptcp_hash_mac, 2,
					1, (u8 *)&mptcp_local->locaddr6[ind].loc6_id,
					16, (u8 *)&opts->add_addr6.addr.s6_addr);
			opts->add_addr6.trunc_mac = *(u64 *)mptcp_hash_mac;
		}

		if (skb) {
			fmp->announced_addrs_v6 |= (1 << ind);
			fmp->add_addr--;
		}
		if (mpcb->mptcp_ver < MPTCP_VERSION_1)
			*size += MPTCP_SUB_LEN_ADD_ADDR6_ALIGN;
		if (mpcb->mptcp_ver >= MPTCP_VERSION_1)
			*size += MPTCP_SUB_LEN_ADD_ADDR6_ALIGN_VER1;
	}

skip_ipv6:
	rcu_read_unlock_bh();

	if (!unannouncedv4 && !unannouncedv6 && skb)
		fmp->add_addr--;

remove_addr:
	if (likely(!fmp->remove_addrs))
		goto exit;

	remove_addr_len = mptcp_sub_len_remove_addr_align(fmp->remove_addrs);
	if (MAX_TCP_OPTION_SPACE - *size < remove_addr_len)
		goto exit;

	opts->options |= OPTION_MPTCP;
	opts->mptcp_options |= OPTION_REMOVE_ADDR;
	opts->remove_addrs = fmp->remove_addrs;
	*size += remove_addr_len;
	if (skb)
		fmp->remove_addrs = 0;

exit:
	mpcb->addr_signal = !!(fmp->add_addr || fmp->remove_addrs);
}

static void full_mesh_rem_raddr(struct mptcp_cb *mpcb, u8 rem_id)
{
	mptcp_v4_rem_raddress(mpcb, rem_id);
	mptcp_v6_rem_raddress(mpcb, rem_id);
}

static void full_mesh_delete_subflow(struct sock *sk)
{
	struct fullmesh_priv *fmp = fullmesh_get_priv(tcp_sk(sk)->mpcb);
	struct mptcp_fm_ns *fm_ns = fm_get_ns(sock_net(sk));
	struct mptcp_loc_addr *mptcp_local;
	int index, i;

	if (!create_on_err)
		return;

	rcu_read_lock_bh();
	mptcp_local = rcu_dereference_bh(fm_ns->local);

	if (sk->sk_family == AF_INET || mptcp_v6_is_v4_mapped(sk)) {
		union inet_addr saddr;
		u32 mptcp_cap_flag = tcp_sk(mptcp_meta_sk(sk))->mptcp_cap_flag;

		saddr.ip = inet_sk(sk)->inet_saddr;
		index = mptcp_find_address(mptcp_local, AF_INET, &saddr,
					   sk->sk_bound_dev_if);
		if (index < 0 && mptcp_cap_flag != MPTCP_CAP_UID_DIP_DPORT)
			goto out;

		mptcp_for_each_bit_set(fmp->rem4_bits, i) {
			struct fullmesh_rem4 *rem4 = &fmp->remaddr4[i];

			if (rem4->addr.s_addr != sk->sk_daddr)
				continue;

			if (rem4->port && rem4->port != inet_sk(sk)->inet_dport)
				continue;

			if (index < 0) {
				rem4->bitfield = rem4->bitfield & mptcp_local->loc4_bits;
				if (!rem4->bitfield)
					fmp->rem4_bits &= ~(1 << i);
			} else
				rem4->bitfield &= ~(1 << index);

			mptcp_debug("%s: rem4->bitfield:%u, index:%d, fmp->rem4_bits:%u \n",
							__func__, rem4->bitfield, i, fmp->rem4_bits);
		}
#if IS_ENABLED(CONFIG_IPV6)
	} else {
		union inet_addr saddr;

		saddr.in6 = inet6_sk(sk)->saddr;
		index = mptcp_find_address(mptcp_local, AF_INET6, &saddr,
					   sk->sk_bound_dev_if);
		if (index < 0)
			goto out;

		mptcp_for_each_bit_set(fmp->rem6_bits, i) {
			struct fullmesh_rem6 *rem6 = &fmp->remaddr6[i];

			if (!ipv6_addr_equal(&rem6->addr, &sk->sk_v6_daddr))
				continue;

			if (rem6->port && rem6->port != inet_sk(sk)->inet_dport)
				continue;

			rem6->bitfield &= ~(1 << index);
		}
#endif
	}

out:
	rcu_read_unlock_bh();
}

static void full_mesh_user_switch(struct sock *meta_sk, bool on, const char *iface)
{
	struct mptcp_cb *mpcb;
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);

	if (sock_flag(meta_sk, SOCK_DEAD) ||
	    meta_sk->sk_state != TCP_ESTABLISHED) {
		mptcp_debug("%s: meta_sk is not in TCP_ESTABLISHED\n", __func__);
		return;
	}

	mpcb = meta_tp->mpcb;
	if (!mptcp(meta_tp) || !is_meta_sk(meta_sk) ||
	    mpcb->infinite_mapping_snd ||
	    mpcb->infinite_mapping_rcv ||
	    mpcb->send_infinite_mapping)
		return;

	meta_tp->user_switch = on;

	if (on) {
		mptcp_add_rem_addr_for_sk(meta_sk);

		full_mesh_create_subflows(meta_sk);
		mptcp_debug("%s: try full_mesh_create_subflows\n", __func__);
	} else {
		struct sock *sk, *tmpsk;
		struct sock *sk_keep = NULL;
		struct net_device *net_dev;
		bool is_prim_iface_on = false;

		if (iface && (iface != meta_tp->prim_iface)) {
			(void)strncpy(meta_tp->prim_iface, iface, IFNAMSIZ);
			meta_tp->prim_iface[IFNAMSIZ - 1] = '\0';
		}
		net_dev = dev_get_by_name(sock_net(meta_sk), meta_tp->prim_iface);
		if (net_dev) {
			unsigned int flags = dev_get_flags(net_dev);
			if (flags & IFF_RUNNING)
				is_prim_iface_on = true;
			dev_put(net_dev);
		}

		if (mpcb->cnt_subflows < 2 && !is_prim_iface_on) {
			mptcp_debug("%s: cnt_subflows %d is less than 2 and prim_iface is off\n",
				__func__, mpcb->cnt_subflows);
			return;
		}
		/* Look for the socket and remove him */
		mptcp_for_each_sk_safe(mpcb, sk, tmpsk) {
			struct dst_entry *dst;
			bool is_prim = false;

			dst = sk_dst_get(sk);
			if (dst) {
				is_prim = !strncmp(meta_tp->prim_iface, dst->dev->name, IFNAMSIZ);
				dst_release(dst);
			}

			tcp_sk(sk)->mptcp_sched_prim_intf = is_prim;

			if (is_prim) {
				/* Reinject, so that pf = 1 and so we
				 * won't select this one as the
				 * ack-sock.
				 */
				if (sk_keep) {
					mptcp_reinject_data(sk_keep, 0);
					tcp_sk(sk_keep)->ops->send_active_reset(sk_keep, GFP_ATOMIC);
					mptcp_sub_force_close(sk_keep);
				}
				sk_keep = sk;
				continue;
			} else if (!sk_keep && !is_prim_iface_on) {
				sk_keep = sk;
				continue;
			}

			mptcp_debug("%s: close subflows\n", __func__);
			/* Reinject, so that pf = 1 and so we
			 * won't select this one as the
			 * ack-sock.
			 */
			mptcp_reinject_data(sk, 0);
			tcp_sk(sk)->ops->send_active_reset(sk, GFP_ATOMIC);
			mptcp_sub_force_close(sk);
		}
	}
}

/* Output /proc/net/mptcp_fullmesh */
static int mptcp_fm_seq_show(struct seq_file *seq, void *v)
{
	const struct net *net = seq->private;
	struct mptcp_loc_addr *mptcp_local;
	const struct mptcp_fm_ns *fm_ns = fm_get_ns(net);
	int i;

	seq_printf(seq, "Index, Address-ID, Backup, IP-address\n");

	rcu_read_lock_bh();
	mptcp_local = rcu_dereference(fm_ns->local);

	seq_printf(seq, "IPv4, next v4-index: %u\n", mptcp_local->next_v4_index);

	mptcp_for_each_bit_set(mptcp_local->loc4_bits, i) {
		struct mptcp_loc4 *loc4 = &mptcp_local->locaddr4[i];

		seq_printf(seq, "%u, %u, %u, %pI4\n", i, loc4->loc4_id,
			   loc4->low_prio, &loc4->addr);
	}

	seq_printf(seq, "IPv6, next v6-index: %u\n", mptcp_local->next_v6_index);

	mptcp_for_each_bit_set(mptcp_local->loc6_bits, i) {
		struct mptcp_loc6 *loc6 = &mptcp_local->locaddr6[i];

		seq_printf(seq, "%u, %u, %u, %pI6\n", i, loc6->loc6_id,
			   loc6->low_prio, &loc6->addr);
	}
	rcu_read_unlock_bh();

	return 0;
}

static int mptcp_fm_seq_open(struct inode *inode, struct file *file)
{
	return single_open_net(inode, file, mptcp_fm_seq_show);
}

static const struct file_operations mptcp_fm_seq_fops = {
	.owner = THIS_MODULE,
	.open = mptcp_fm_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release_net,
};

static int mptcp_fm_init_net(struct net *net)
{
	struct mptcp_loc_addr *mptcp_local;
	struct mptcp_fm_ns *fm_ns;
	int err = 0;

	fm_ns = kzalloc(sizeof(*fm_ns), GFP_KERNEL);
	if (!fm_ns)
		return -ENOBUFS;

	mptcp_local = kzalloc(sizeof(*mptcp_local), GFP_KERNEL);
	if (!mptcp_local) {
		err = -ENOBUFS;
		goto err_mptcp_local;
	}

	if (!proc_create("mptcp_fullmesh", S_IRUGO, net->proc_net,
			 &mptcp_fm_seq_fops)) {
		err = -ENOMEM;
		goto err_seq_fops;
	}

	mptcp_local->next_v4_index = 1;

	rcu_assign_pointer(fm_ns->local, mptcp_local);
	INIT_DELAYED_WORK(&fm_ns->address_worker, mptcp_address_worker);
	INIT_LIST_HEAD(&fm_ns->events);
	spin_lock_init(&fm_ns->local_lock);
	fm_ns->net = net;
	net->mptcp.path_managers[MPTCP_PM_FULLMESH] = fm_ns;

	return 0;
err_seq_fops:
	kfree(mptcp_local);
err_mptcp_local:
	kfree(fm_ns);
	return err;
}

static void mptcp_fm_exit_net(struct net *net)
{
	struct mptcp_addr_event *eventq, *tmp;
	struct mptcp_fm_ns *fm_ns;
	struct mptcp_loc_addr *mptcp_local;

	fm_ns = fm_get_ns(net);
	cancel_delayed_work_sync(&fm_ns->address_worker);

	rcu_read_lock_bh();

	mptcp_local = rcu_dereference_bh(fm_ns->local);
	kfree_rcu(mptcp_local, rcu);

	spin_lock(&fm_ns->local_lock);
	list_for_each_entry_safe(eventq, tmp, &fm_ns->events, list) {
		list_del(&eventq->list);
		kfree(eventq);
	}
	spin_unlock(&fm_ns->local_lock);

	rcu_read_unlock_bh();

	remove_proc_entry("mptcp_fullmesh", net->proc_net);

	kfree(fm_ns);
}

static struct pernet_operations full_mesh_net_ops = {
	.init = mptcp_fm_init_net,
	.exit = mptcp_fm_exit_net,
};

static struct mptcp_pm_ops full_mesh __read_mostly = {
	.new_session = full_mesh_new_session,
	.release_sock = full_mesh_release_sock,
	.fully_established = full_mesh_create_subflows,
	.new_remote_address = full_mesh_create_subflows,
	.subflow_error = full_mesh_subflow_error,
	.get_local_id = full_mesh_get_local_id,
	.addr_signal = full_mesh_addr_signal,
	.add_raddr = full_mesh_add_raddr,
	.rem_raddr = full_mesh_rem_raddr,
	.delete_subflow = full_mesh_delete_subflow,
	.user_switch = full_mesh_user_switch,
	.name = "fullmesh",
	.owner = THIS_MODULE,
};

/* General initialization of MPTCP_PM */
static int __init full_mesh_register(void)
{
	int ret;

	BUILD_BUG_ON(sizeof(struct fullmesh_priv) > MPTCP_PM_SIZE);

	ret = register_pernet_subsys(&full_mesh_net_ops);
	if (ret)
		goto out;

	ret = register_inetaddr_notifier(&mptcp_pm_inetaddr_notifier);
	if (ret)
		goto err_reg_inetaddr;
	ret = register_netdevice_notifier(&mptcp_pm_netdev_notifier);
	if (ret)
		goto err_reg_netdev;

#if IS_ENABLED(CONFIG_IPV6)
	ret = register_inet6addr_notifier(&inet6_addr_notifier);
	if (ret)
		goto err_reg_inet6addr;
#endif

	ret = mptcp_register_path_manager(&full_mesh);
	if (ret)
		goto err_reg_pm;

out:
	return ret;


err_reg_pm:
#if IS_ENABLED(CONFIG_IPV6)
	unregister_inet6addr_notifier(&inet6_addr_notifier);
err_reg_inet6addr:
#endif
	unregister_netdevice_notifier(&mptcp_pm_netdev_notifier);
err_reg_netdev:
	unregister_inetaddr_notifier(&mptcp_pm_inetaddr_notifier);
err_reg_inetaddr:
	unregister_pernet_subsys(&full_mesh_net_ops);
	goto out;
}

static void full_mesh_unregister(void)
{
#if IS_ENABLED(CONFIG_IPV6)
	unregister_inet6addr_notifier(&inet6_addr_notifier);
#endif
	unregister_netdevice_notifier(&mptcp_pm_netdev_notifier);
	unregister_inetaddr_notifier(&mptcp_pm_inetaddr_notifier);
	unregister_pernet_subsys(&full_mesh_net_ops);
	mptcp_unregister_path_manager(&full_mesh);
}

module_init(full_mesh_register);
module_exit(full_mesh_unregister);

MODULE_AUTHOR("Christoph Paasch");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Full-Mesh MPTCP");
MODULE_VERSION("0.88");
