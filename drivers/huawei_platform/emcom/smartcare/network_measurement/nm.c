#ifdef CONFIG_MPTCP
#include <net/mptcp.h>
#include <net/mptcp_v4.h>
#include <net/mptcp_v6.h>
#endif
#include <linux/highmem.h>
#include "huawei_platform/emcom/smartcare/network_measurement/nm.h"
#include "huawei_platform/emcom/smartcare/nse/psr_pub.h"
#include "huawei_platform/emcom/emcom_netlink.h"

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
#define SC_ISOLATION_SECONDS	10		/* Defined by smartcare */
#define MAX_NLMSG_PAYLOAD       1024
#define NL_MSG_HEADER_LEN       16
#define SMARTCARE_NL_TYPE       NETLINK_EMCOM_KD_SMARTCARE_NM
#define PROTOCOL_HTTPS		443

struct __nvec {
	void	*base;
	int	len;
	int	maxlen;
	int	err;
};

#define PUT_U16(iov, val) \
	do { \
		if (!(iov).err && (iov).len + sizeof(u16) <= (iov).maxlen) { \
			*(u16 *)((iov).base + (iov).len) = (u16)(val); \
			(iov).len += sizeof(u16); \
		} else { \
			(iov).err = -ENOMEM; \
		} \
	} while (0)
#define PUT_U32(iov, val) \
	do { \
		if (!(iov).err && (iov).len + sizeof(u32) <= (iov).maxlen) { \
			*(u32 *)((iov).base + (iov).len) = (u32)(val); \
			(iov).len += sizeof(u32); \
		} else { \
			(iov).err = -ENOMEM; \
		} \
	} while (0)
#define PUT_U64(iov, val) \
	do { \
		if (!(iov).err && (iov).len + sizeof(u64) <= (iov).maxlen) { \
			*(u64 *)((iov).base + (iov).len) = (u64)(val); \
			(iov).len += sizeof(u64); \
		} else { \
			(iov).err = -ENOMEM; \
		} \
	} while (0)
#define PUT_MEM(iov, src, l) \
	do { \
		if (!(iov).err && (iov).len + (l) <= (iov).maxlen) { \
			u8 *vp = (iov).base + (iov).len; \
			memcpy((void*)vp, (const void *)(src), (size_t)(l)); \
			(iov).len += (l); \
		} else { \
			(iov).err = -ENOMEM; \
		} \
	} while (0)

/*
 * The Global valve of network measurement is marked with a special marks
 * which has the following format:
 * 31								 1 0
 * +-------+-----------------------------------------------------+-+
 * |                         Unix epoch                          |V|
 * +-------+-----------------------------------------------------+-+
 *
 * Unix epoch (31 bits) : Timestamp of valve, specified by unix epoch seconds.
 * V (1 bit)		: Valve, 1 upon open and 0 upon closed.
 */
int network_measure __read_mostly = 0;
EXPORT_SYMBOL(network_measure);

/*
 * It is unreliable to rely on the userland daemon to close the valve.
 * We should start a guaranteed timeout mechanism in the kernel.
 */
int network_measure_timeouts __read_mostly = 150;
EXPORT_SYMBOL(network_measure_timeouts);

/* Switch of tcp statistic */
int network_measure_tcp __read_mostly = 0;
EXPORT_SYMBOL(network_measure_tcp);

static const int tcp_sta_len = sizeof(struct tcp_statistics);

/* Check the sampling permission by uid and allocate memory. */
void tcp_measure_init(struct sock *sk)
{
	kuid_t uid;
	struct timespec ts;
	/* Use a copy against concurrent modification. */
	unsigned int nm = (unsigned int)network_measure;

	/* Check the lowest bit which is the status of global valve. */
	if (!(nm & VALVE_OPEN))
		goto out;

	uid = sock_i_uid(sk);	/* Fetch uid, Each APP has an unique uid. */
	if (!in_sample_uid_list(uid.val)) /* Check the sampling permission. */
		goto out;

	/*
	 * During the sampling period, the first http connection can only be
	 * established after opening valve for 10s. (SMARTCARE defined)
	 */
	getnstimeofday(&ts);
	if ((int)(ts.tv_sec - (nm >> 1)) < SC_ISOLATION_SECONDS) {
		network_measure = nm_shut_off_valve(NULL);
		goto out;
	}

	sk->sk_nm_http = kzalloc(sizeof(struct nm_http_entry), GFP_KERNEL);
	if (unlikely(!sk->sk_nm_http))
		goto out;

	if (network_measure_tcp) {
		sk->sk_tcp_statis = kzalloc(tcp_sta_len, GFP_KERNEL);
		if (unlikely(!sk->sk_tcp_statis))
			goto out;

		sk->sk_tcp_statis->syn_epoch = (unsigned long)timespec_to_ns(&ts);
	} else {
		sk->sk_tcp_statis = NULL;
	}

	sk->sk_nm_uid = uid.val;
	return;
out:
	if (sk->sk_nm_http)
		kfree(sk->sk_nm_http);

	sk->sk_nm_http    = NULL;
	sk->sk_tcp_statis = NULL;
	sk->sk_nm_uid     = 0;
}
EXPORT_SYMBOL(tcp_measure_init);

/* Reclaim */
void tcp_measure_deinit(struct sock *sk)
{
	if (sk->sk_nm_http)
		kfree(sk->sk_nm_http);

	if (sk->sk_tcp_statis)
		kfree(sk->sk_tcp_statis);

	sk->sk_nm_http    = NULL;
	sk->sk_tcp_statis = NULL;
	sk->sk_nm_uid     = 0;
}
EXPORT_SYMBOL(tcp_measure_deinit);

/* Check the sampling timeout and allocate memory. */
void udp_measure_init(struct sock *sk, struct sk_buff *skb)
{
	struct timespec ts;
	int index = 0;
	/* Use a copy against concurrent modification. */
	unsigned int nm = (unsigned int)network_measure;

	/* Check the lowest bit which is the status of global valve. */
	if (!(nm & VALVE_OPEN))
		goto out;

	getnstimeofday(&ts);
	if ((int)(ts.tv_sec - (nm >> 1)) > network_measure_timeouts && network_measure_timeouts != 0) {
		/* Userland daemon may have been killed. For service competition mode, timeouts is 0. */
		network_measure = nm_shut_off_valve(&ts);
		goto out;
	}

	sk->sk_nm_dnsp = kzalloc(sizeof(struct nm_dnsp_entry), GFP_ATOMIC);
	if (unlikely(!sk->sk_nm_dnsp))
		goto out;

	sk->sk_nm_dnsp->qstamp = (unsigned long)timespec_to_ns(&ts);
	/* Skip udp header and ip header */
	index = sizeof(struct udphdr) + (ip_hdr(skb)->ihl << 2);
	/* uid represents the "transaction id" field in the DNS protocol. */
	sk->sk_nm_uid = (skb->data[index] << 8) + skb->data[index + 1];

	return;
out:
	sk->sk_nm_dnsp = NULL;
	sk->sk_nm_uid  = 0;
}
EXPORT_SYMBOL(udp_measure_init);

/* Reclaim */
void udp_measure_deinit(struct sock *sk)
{
	if (sk->sk_nm_dnsp) {
		kfree(sk->sk_nm_dnsp);
		sk->sk_nm_dnsp = NULL;
		sk->sk_nm_uid  = 0;
	}
}
EXPORT_SYMBOL(udp_measure_deinit);

/*
 * Send TLV format message to userland daemon via netlink.
 * Message header has the following format:
 * +-----------------------------------+
 * | TCPS_HEADER_V1  |  DNSP_HEADER_V1 |
 * +-----------------+-----------------+
 * |  saddr  4bytes  |                 |
 * +-----------------|  epoch  8bytes  |
 * |  daddr  4bytes  |                 |
 * +-----------------+-----------------+
 * | s/dport 4bytes  |  using  4bytes  |
 * +-----------------+-----------------+
 * |   uid   4bytes  |  daddr  4bytes  |
 * +-----------------+-----------------+
 */
static void nm_report(int report, struct sock *sk)
{
	struct __nvec iov = {
		.len = 0,
		.maxlen = MAX_NLMSG_PAYLOAD,
		.err = 0,
	};

	/* May be called in any context. */
	iov.base = kzalloc(iov.maxlen, GFP_ATOMIC);
	if (!iov.base)
		return;

	switch (report) {
	case (1U << NM_REPORT_HTTP):
		/* TLV chunk 1: message header */
		PUT_U16(iov, TCPS_HEADER_V1);			   /* Type */
		PUT_U16(iov, NL_MSG_HEADER_LEN);		   /* Length */
		PUT_U32(iov, inet_sk(sk)->inet_saddr);		   /* Values */
		PUT_U32(iov, inet_sk(sk)->inet_daddr);
		PUT_U16(iov, inet_sk(sk)->inet_sport);
		PUT_U16(iov, inet_sk(sk)->inet_dport);
		PUT_U32(iov, sk->sk_nm_uid);

		/* TLV chunk 2: http sa results */
		PUT_U16(iov, HTTP_RESULT_V1);			   /* Type */
		PUT_U16(iov, SA_RES_SIZE);			   /* Length */
		PUT_MEM(iov, sk->sk_nm_http->result, SA_RES_SIZE); /* Values */

		/* TLV chunk 3: tcp statistics */
		if (sk->sk_tcp_statis) {
			PUT_U16(iov, TCPS_STATIS_V1);			/* Type */
			PUT_U16(iov, tcp_sta_len);			/* Length */
			PUT_MEM(iov, sk->sk_tcp_statis, tcp_sta_len);	/* Values */
		}

		if (!iov.err)
			emcom_send_msg2daemon(SMARTCARE_NL_TYPE, iov.base, iov.len);
		break;
	case (1 << NM_REPORT_DNSP):
		/* TLV chunk 1: message header */
		PUT_U16(iov, DNSP_HEADER_V1);			   /* Type */
		PUT_U16(iov, NL_MSG_HEADER_LEN);		   /* Length */
		PUT_U64(iov, sk->sk_nm_dnsp->qstamp);		   /* Values */
		PUT_U32(iov, sk->sk_nm_dnsp->qusing);
		PUT_U32(iov, inet_sk(sk)->inet_saddr);

		/* TLV chunk 2: dns sa results */
		PUT_U16(iov, DNSP_RESULT_V1);			   /* Type */
		PUT_U16(iov, SA_RES_SIZE);			   /* Length */
		PUT_MEM(iov, sk->sk_nm_dnsp->result, SA_RES_SIZE); /* Values */

		if (!iov.err)
			emcom_send_msg2daemon(SMARTCARE_NL_TYPE, iov.base, iov.len);
		break;
	default:
		break;
	}

	kfree(iov.base);
	return;
}

static int nm_nse_http_helper(struct sock *sk, struct sk_buff *skb, int offset,
			      int len, int protocol, int direction)
{
	int start = skb_headlen(skb);
	int i, parse = start - offset;
	struct timespec ts;
	unsigned long epoch = 0;
	int ret = 0;
	int rep = 0;
	int reset_mem = 0;
	int is_reported = 0;

	getnstimeofday(&ts);
	epoch = (unsigned long)timespec_to_ns(&ts);

	if (parse > 0) {
		if (parse > len)
			parse = len;

		if ((!virt_addr_valid(skb->data + offset)) ||
		    (!virt_addr_valid(skb->data + offset + parse))) {
			NM_DEBUG("Invalid data address.");
			return 0;
		}

		ret = psr_proc(protocol, direction, epoch, parse,
			       skb->data + offset,
			       sk->sk_nm_http->status,
			       sk->sk_nm_http->result);
		reset_mem = ret == 2 ? 1 : 0;
		/* !!ret is equal to either 0 or 1 */
		rep = !!ret << NM_REPORT_HTTP;
		if (rep) {
			nm_report(rep, sk);
			is_reported = 1;
		}

		if (reset_mem)
			memset(sk->sk_nm_http, 0x00,
			       sizeof(struct nm_http_entry));

		if ((len -= parse) == 0)
			return is_reported;
		offset += parse;
	}

	/* Copy paged appendix. Hmm... why does this look so complicated? */
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		void *kaddr, *parse_addr;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((parse = end - offset) > 0) {
			if (parse > len)
				parse = len;

			kaddr = kmap(skb_frag_page(frag));
			parse_addr = kaddr + frag->page_offset + offset - start;
			if ((!virt_addr_valid(parse_addr)) ||
			    (!virt_addr_valid(parse_addr + parse))) {
				NM_DEBUG("Invalid data address.");
				return is_reported;
			}
			ret = psr_proc(protocol, direction, epoch, parse, parse_addr,
				       sk->sk_nm_http->status,
				       sk->sk_nm_http->result);
			kunmap(kaddr);
			reset_mem = ret == 2 ? 1 : 0;
			/* !!ret is equal to either 0 or 1 */
			rep = !!ret << NM_REPORT_HTTP;
			if (rep) {
				nm_report(rep, sk);
				is_reported = 1;
			}

			if (reset_mem)
				memset(sk->sk_nm_http, 0x00,
				       sizeof(struct nm_http_entry));

			if (!(len -= parse))
				return is_reported;
			offset += parse;
		}
		start = end;
	}
	return is_reported;
}

void nm_nse(struct sock *sk, struct sk_buff *skb, int offset, int len,
	    int protocol, int direction, unsigned char func)
{
	unsigned char *data = NULL;
	int pktlen = 0;
	struct timespec ts;
	unsigned long epoch = 0;
	int ret = 0;
	int rep = 0;
	int reset_uid = 0;
	int reset_mem = 0;

	/* Fetch payload address and payload size */
	if (protocol == NM_TCP) {
		/* https is not supported */
		if (htons(sk->sk_dport) == PROTOCOL_HTTPS)
			return;

		pktlen = skb->len;
		if (direction == NM_UPLINK)
			update_snd_pkts(sk, pktlen);
		else if (direction == NM_DOWNLINK)
			update_rcv_pkts(sk, pktlen);
		else
			NM_DEBUG("Unknown direction=%u.", direction);

		if (skb_transport_header_was_set(skb) &&
			(unlikely(tcp_hdr(skb)->fin || tcp_hdr(skb)->rst))) {
#ifdef CONFIT_MPTCP
			if (!mptcp(tcp_sk(sk)))
#endif
			{
				rep |= 1 << NM_REPORT_HTTP;
				reset_uid = 1;
				reset_mem = 1;
			}
			if (sk->sk_tcp_statis)
				sk->sk_tcp_statis->end_flags = 1;
		}
#ifdef CONFIT_MPTCP
		else if (mptcp(tcp_sk(sk) && mptcp_is_data_fin(skb))) {
			rep |= 1 << NM_REPORT_HTTP;
			reset_uid = 1;
			reset_mem = 1;
		}
#endif

		if ((NM_FUNC_HTTP & func) && sk->sk_nm_http) {
			if (len > 0) {
				ret = nm_nse_http_helper(sk, skb, offset,
							 len, protocol,
						   	 direction);
				if (ret)
					rep &= ~(1 << NM_REPORT_HTTP);
			}

			if (rep)
				nm_report(rep, sk);

			if (reset_mem)
				memset(sk->sk_nm_http, 0x00,
				       sizeof(struct nm_http_entry));

			if (unlikely(reset_uid))
				sk->sk_nm_uid = 0;
		}
	} else if (protocol == NM_DNS) {
		pktlen = skb->len - sizeof(struct udphdr);
		/* skb->len may be less than hdrlen */
		pktlen = pktlen > 0 ? pktlen : 0;
		data = skb->data + sizeof(struct udphdr);

		if ((!virt_addr_valid(data)) ||
		    (!virt_addr_valid(data + pktlen))) {
			NM_DEBUG("Invalid data address.");
			return;
		}

		if ((NM_FUNC_DNSP & func) && sk->sk_nm_dnsp && pktlen) {
			getnstimeofday(&ts);
			epoch = (unsigned long)timespec_to_ns(&ts);

			ret = psr_proc(protocol, direction, epoch, pktlen, data,
				       sk->sk_nm_dnsp->status,
				       sk->sk_nm_dnsp->result);
			sk->sk_nm_dnsp->qusing = epoch - sk->sk_nm_dnsp->qstamp;
			/* !!ret is equal to either 0 or 1 */
			rep |= !!ret << NM_REPORT_DNSP;

			if (rep)
				nm_report(rep, sk);
		}
	} else {
		NM_DEBUG("Unknown protocol=%u.", protocol);
	}
}
EXPORT_SYMBOL(nm_nse);
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
