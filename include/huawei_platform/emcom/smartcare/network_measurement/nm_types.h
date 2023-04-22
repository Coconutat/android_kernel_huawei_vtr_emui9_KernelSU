#ifndef __NM_TYPES_H__
#define __NM_TYPES_H__

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
#define NM_DNS_PORT		53
#define SA_STA_SIZE		32
#define SA_RES_SIZE		640
#define NM_FUNC_HTTP		(1 << 0)
#define NM_FUNC_DNSP		(1 << 1)
#define NM_UPLINK		0
#define NM_DOWNLINK		1

enum __NM_REPORT_MASK {
	NM_REPORT_HTTP,
	NM_REPORT_DNSP
};

enum __NM_SA_PROTOCOL {
	NM_TCP,
	NM_DNS
};

enum __NM_NETLINK_MSG_TYPE {
	TCPS_HEADER_V1,
	HTTP_RESULT_V1,
	TCPS_STATIS_V1,
	DNSP_HEADER_V1,
	DNSP_RESULT_V1
};

struct nm_http_entry {
	unsigned char		status[SA_STA_SIZE]	__aligned(8);
	unsigned char		result[SA_RES_SIZE]	__aligned(8);
};

struct nm_dnsp_entry {
	unsigned char		status[SA_STA_SIZE]	__aligned(8);
	unsigned char		result[SA_RES_SIZE]	__aligned(8);
	unsigned long		qstamp;
	unsigned long		qusing;
};

struct tcp_statistics {
	u16			snd_pkts;
	u16			rcv_pkts;
	u32			syn_rtt;
	u64			syn_epoch;
	union {
		u16		wrap_masks;
		struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
			u16	snd_pkts_wrap			: 1,
				rcv_pkts_wrap			: 1,
				snd_zero_win_cnts_wrap		: 1,
				rcv_zero_win_cnts_wrap		: 1,
				ul_fast_retrans_wrap		: 1,
				ul_timeout_retrans_wrap		: 1,
				dl_three_dup_acks_wrap		: 1,
				dl_disorder_pkts_wrap		: 1,
				reserved_wrap			: 8;
#elif defined(__BIG_ENDIAN_BITFIELD)
			u16	reserved_wrap			: 8,
				dl_disorder_pkts_wrap		: 1,
				dl_three_dup_acks_wrap		: 1,
				ul_timeout_retrans_wrap		: 1,
				ul_fast_retrans_wrap		: 1,
				rcv_zero_win_cnts_wrap		: 1,
				snd_zero_win_cnts_wrap		: 1,
				rcv_pkts_wrap			: 1,
				snd_pkts_wrap			: 1;
#else
#error  "Adjust your <asm/byteorder.h> defines"
#endif
		};
	};
	u8			syn_retrans;
	u8			end_flags;
	u8			snd_zero_win_cnts;
	u8			rcv_zero_win_cnts;
	u8			ul_fast_retrans;
	u8			ul_timeout_retrans;
	u8			dl_three_dup_acks;
	u8			dl_disorder_pkts;
	u8			reserved[6];
};
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
#endif /* __NM_TYPES_H__ */
