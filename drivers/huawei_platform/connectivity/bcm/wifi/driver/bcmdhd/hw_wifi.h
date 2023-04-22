#ifndef __HW_WIFI_H__
#define __HW_WIFI_H__

/************修改记录************************
版本:				日期:		解决问题:
HUAWEI-2014.001:  	0529		solve wifi panic
HUAWEI-2014.002:       0609		solve scan_done panic.
HUAWEI-2014.003:       0613             throughput optimize.
HUAWEI-2014.004:       0626             solve wdev_cleanup_work panic.
HUAWEI-2014.005:       0702             solve country code problem.
HUAWEI-2014.006:       0725             use huawei customize country code.
HUAWEI-2014.007:       0728             set bcn_timeout for beacon loss and roaming problem.
**********************************************/
#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <bcmutils.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <wlioctl.h>
#ifdef HW_WIFI_DMD_LOG
#include <dsm/dsm_pub.h>
#endif
#ifdef HW_NETDEVICE_PANIC
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#endif
#ifdef HW_DOZE_PKT_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif

#ifdef HW_OTP_CHECK
#include <linux/crc16.h>
#endif
#ifdef HW_AP_POWERSAVE
#include <linux/fb.h>
#endif
#include "hw_driver_register.h"
#define        HUAWEI_VERSION_STR ", HUAWEI-2014.007"

#ifdef HW_WIFI_WAKEUP_SRC_PARSE
#define WIFI_WAKESRC_TAG "WIFI wake src: "
extern volatile bool g_wifi_firstwake;
#endif

#define   HW_5G_CUSTOM_ROAM_TRIGGER_SETTING  -70  /* dBm default roam trigger 5 band , used by dhd_preinit_ioctls func*/

#ifdef HW_DNS_DHCP_PARSE

#define MAX_DOMAIN_LEN (100)

#define DHCP_SERVER_PORT    (67)
#define DHCP_CLIENT_PORT    (68)
#define DNS_SERVER_PORT     (53)

#define HWMACSTR "%02x:%02x:%02x:**:**:%02x"
#define HWMAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[5]

#define DHCP_CHADDR_LEN         16
#define SERVERNAME_LEN          64
#define BOOTFILE_LEN            128

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_ACK            5
#define DHCP_NAK            6

#define DHO_PAD             0
#define DHO_IPADDRESS       50
#define DHO_MESSAGETYPE     53
#define DHO_SERVERID        54
#define DHO_END             255


struct dhcp_message {
    uint8_t op;             /* message type */
    uint8_t hwtype;         /* hardware address type */
    uint8_t hwlen;          /* hardware address length */
    uint8_t hwopcount;      /* should be zero in client message */
    uint32_t xid;           /* transaction id */
    uint16_t secs;          /* elapsed time in sec. from boot */
    uint16_t flags;
    uint32_t ciaddr;        /* (previously allocated) client IP */
    uint32_t yiaddr;        /* 'your' client IP address */
    uint32_t siaddr;        /* should be zero in client's messages */
    uint32_t giaddr;        /* should be zero in client's messages */
    uint8_t chaddr[DHCP_CHADDR_LEN];    /* client's hardware address */
    uint8_t servername[SERVERNAME_LEN]; /* server host name */
    uint8_t bootfile[BOOTFILE_LEN];     /* boot file name */
    uint32_t cookie;
    uint8_t options[0]; /* message options - cookie */
};

struct dns_message_hdr {
    uint16_t id;    /* transaction id */
    uint16_t flags; /* message future*/
    uint16_t qdcount;   /* question record count */
    uint16_t ancount;   /* answer record count */
    uint16_t nscount;   /* authority record count */
    uint16_t arcount;   /* additional record count*/
};

typedef enum __ns_qrcode {
    ns_q_request = 0, /* request */
    ns_q_response = 1, /* response */
} ns_qrcode;

typedef enum __ns_opcode {
    ns_o_query = 0,     /* Standard query. */
    ns_o_iquery = 1,    /* Inverse query (deprecated/unsupported). */
} ns_opcode;

/*
 * Currently defined response codes.
 */
typedef enum __ns_rcode {
    ns_r_noerror = 0,   /* No error occurred. */
} ns_rcode;

typedef enum __ns_type {
    ns_t_invalid = 0,   /* Cookie. */
    ns_t_a = 1,         /* Host address. */
} ns_type;

#ifdef HW_SDIO_QUALITY_STATISTIC
typedef enum __sdio_cmd_type {
	CMD52_TOTAL			= 0,
	CMD53_TOTAL,
	/* below are for some specific command we focus on */
	CMD_SLEEPCSR_SET	= 2,
	CMD_SLEEPCSR_CLR,
}sdio_cmd_type;

typedef enum _sdio_cmd_dir {
	CMD_READ 	= 0,
	CMD_WRITE 	= 1
}sdio_cmd_dir;

typedef struct __sdio_quality {
	sdio_cmd_type type;
	sdio_cmd_dir dir;
	uint32 request_count;
	uint32 actual_count;
	struct list_head list;
}sdio_quality_t;
#endif

#ifdef HW_WL_COUNTERS_STATISTIC
/* notice: this must be same as wl_cnt_t
   array was splitted */
typedef enum __counters_cmd_type {
	/* transmit stat counters */
	txframe	= 0,	/* tx data frames */
	txbyte,		/* tx data bytes */
	txretrans,	/* tx mac retransmits */
	txerror,	/* tx data errors (derived: sum of others) */
	txctl,		/* tx management frames */
	txprshort,	/* tx short preamble frames */
	txserr,		/* tx status errors */
	txnobuf,	/* tx out of buffers errors */
	txnoassoc,	/* tx discard because we're not associated */
	txrunt,		/* tx runt frames */
	txchit,		/* tx header cache hit (fastpath) */
	txcmiss,	/* tx header cache miss (slowpath) ( = 11 ) */

	/* transmit chip error counters */
	txuflo,		/* tx fifo underflows */
	txphyerr,	/* tx phy errors (indicated in tx status) */
	txphycrs,	/* = 14 */

	/* receive stat counters */
	rxframe,	/* rx data frames */
	rxbyte,		/* rx data bytes */
	rxerror,	/* rx data errors (derived: sum of others) */
	rxctl,		/* rx management frames */
	rxnobuf,	/* rx out of buffers errors */
	rxnondata,	/* rx non data frames in the data channel errors */
	rxbadds,	/* rx bad DS errors */
	rxbadcm,	/* rx bad control or management frames */
	rxfragerr,	/* rx fragmentation errors */
	rxrunt,		/* rx runt frames */
	rxgiant,	/* rx giant frames */
	rxnoscb,	/* rx no scb error */
	rxbadproto,	/* rx invalid frames */
	rxbadsrcmac,	/* rx frames with Invalid Src Mac */
	rxbadda,	/* rx frames tossed for invalid da */
	rxfilter,	/* rx frames filtered out ( = 30 ) */

	/* receive chip error counters */
	rxoflo,		/* rx fifo overflow errors */
	rxuflo0,	/* rx dma descriptor underflow errors */
	rxuflo1,
	rxuflo2,
	rxuflo3,
	rxuflo4,
	rxuflo5,	/* = 37 */

	d11cnt_txrts_off,	/* d11cnt txrts value when reset d11cnt */
	d11cnt_rxcrc_off,	/* d11cnt rxcrc value when reset d11cnt */
	d11cnt_txnocts_off,	/* d11cnt txnocts value when reset d11cnt ( = 40 ) */

	/* misc counters */
	dmade,		/* tx/rx dma descriptor errors */
	dmada,		/* tx/rx dma data errors */
	dmape,		/* tx/rx dma descriptor protocol errors */
	reset,		/* reset count */
	tbtt,		/* cnts the TBTT int's */
	txdmawar,
	pkt_callback_reg_fail,	/* callbacks register failure ( = 47 ) */

	/* MAC counters: 32-bit version of d11.h's macstat_t */
	txallfrm,	/* total number of frames sent, incl. Data, ACK, RTS, CTS,
		 * Control Management (includes retransmissions)
		 */
	txrtsfrm,	/* number of RTS sent out by the MAC */
	txctsfrm,	/* number of CTS sent out by the MAC */
	txackfrm,	/* number of ACK frames sent out */
	txdnlfrm,	/* Not used */
	txbcnfrm,	/* beacons transmitted */
	txfunfl0,	/* per-fifo tx underflows */
	txfunfl1,
	txfunfl2,
	txfunfl3,
	txfunfl4,
	txfunfl5,	/* = 59 */

	rxtoolate,	/* receive too late */
	txfbw,		/* transmit at fallback bw (dynamic bw) */
	txtplunfl,	/* Template underflows (mac was too slow to transmit ACK/CTS
		 * or BCN)
		 */
	txphyerror,	/* Transmit phy error, type of error is reported in tx-status for
		 * driver enqueued frames
		 */
	rxfrmtoolong,	/* Received frame longer than legal limit (2346 bytes) */
	rxfrmtooshrt,	/* Received frame did not contain enough bytes for its frame type */
	rxinvmachdr,	/* Either the protocol version != 0 or frame type not
		 * data/control/management
		 */
	rxbadfcs,	/* number of frames for which the CRC check failed in the MAC */
	rxbadplcp,	/* parity check of the PLCP header failed */
	rxcrsglitch,	/* PHY was able to correlate the preamble but not the header */
	rxstrt,		/* Number of received frames with a good PLCP
		 * (i.e. passing parity check)
		 */
	rxdfrmucastmbss, /* Number of received DATA frames with good FCS and matching RA */
	rxmfrmucastmbss, /* number of received mgmt frames with good FCS and matching RA */
	rxcfrmucast,	/* number of received CNTRL frames with good FCS and matching RA */
	rxrtsucast,	/* number of unicast RTS addressed to the MAC (good FCS) */
	rxctsucast,	/* number of unicast CTS addressed to the MAC (good FCS) */
	rxackucast,	/* number of ucast ACKS received (good FCS) */
	rxdfrmocast,	/* number of received DATA frames (good FCS and not matching RA) */
	rxmfrmocast,	/* number of received MGMT frames (good FCS and not matching RA) */
	rxcfrmocast,	/* number of received CNTRL frame (good FCS and not matching RA) */
	rxrtsocast,	/* number of received RTS not addressed to the MAC */
	rxctsocast,	/* number of received CTS not addressed to the MAC */
	rxdfrmmcast,	/* number of RX Data multicast frames received by the MAC */
	rxmfrmmcast,	/* number of RX Management multicast frames received by the MAC */
	rxcfrmmcast,	/* number of RX Control multicast frames received by the MAC
		 * (unlikely to see these)   ( = 84 )
		 */
	rxbeaconmbss,	/* beacons received from member of BSS */
	rxdfrmucastobss, /* number of unicast frames addressed to the MAC from
		  * other BSS (WDS FRAME)
		  */
	rxbeaconobss,	/* beacons received from other BSS */
	rxrsptmout,	/* Number of response timeouts for transmitted frames
		 * expecting a response
		 */
	bcntxcancl,	/* transmit beacons canceled due to receipt of beacon (IBSS) */
	rxf0ovfl,	/* Number of receive fifo 0 overflows */
	rxf1ovfl,	/* Number of receive fifo 1 overflows (obsolete) */
	rxf2ovfl,	/* Number of receive fifo 2 overflows (obsolete) */
	txsfovfl,	/* Number of transmit status fifo overflows (obsolete) */
	pmqovfl,	/* Number of PMQ overflows */
	rxcgprqfrm,	/* Number of received Probe requests that made it into
		 * the PRQ fifo ( = 95 )
		 */
	rxcgprsqovfl,	/* Rx Probe Request Que overflow in the AP */
	txcgprsfail,	/* Tx Probe Response Fail. AP sent probe response but did
		 * not get ACK
		 */
	txcgprssuc,	/* Tx Probe Response Success (ACK was received) */
	prs_timeout,	/* Number of probe requests that were dropped from the PRQ
		 * fifo because a probe response could not be sent out within
		 * the time limit defined in M_PRS_MAXTIME
		 */
	rxnack,		/* obsolete */
	frmscons,	/* obsolete */
	txnack,		/* obsolete */
	rxback,		/* blockack rxcnt */
	txback,		/* blockack txcnt */

	/* 802.11 MIB counters, pp. 614 of 802.11 reaff doc. */
	txfrag,		/* dot11TransmittedFragmentCount */
	txmulti,	/* dot11MulticastTransmittedFrameCount */
	txfail,		/* dot11FailedCount */
	txretry,	/* dot11RetryCount */
	txretrie,	/* dot11MultipleRetryCount */
	rxdup,		/* dot11FrameduplicateCount */
	txrts,		/* dot11RTSSuccessCount */
	txnocts,	/* dot11RTSFailureCount */
	txnoack,	/* dot11ACKFailureCount */
	rxfrag,		/* dot11ReceivedFragmentCount */
	rxmulti,	/* dot11MulticastReceivedFrameCount */
	rxcrc,		/* dot11FCSErrorCount */
	txfrmsnt,	/* dot11TransmittedFrameCount (bogus MIB?) */
	rxundec,	/* dot11WEPUndecryptableCount ( = 118 )*/

	/* WPA2 counters (see rxundec for DecryptFailureCount) */
	tkipmicfaill,	/* TKIPLocalMICFailures */
	tkipcntrmsr,	/* TKIPCounterMeasuresInvoked */
	tkipreplay,	/* TKIPReplays */
	ccmpfmterr,	/* CCMPFormatErrors */
	ccmpreplay,	/* CCMPReplays */
	ccmpundec,	/* CCMPDecryptErrors */
	fourwayfail,	/* FourWayHandshakeFailures */
	wepundec,	/* dot11WEPUndecryptableCount */
	wepicverr,	/* dot11WEPICVErrorCount */
	decsuccess,	/* DecryptSuccessCount */
	tkipicverr,	/* TKIPICVErrorCount */
	wepexcluded,	/* dot11WEPExcludedCount ( = 130) */

	txchanrej,	/* Tx frames suppressed due to channel rejection */
	psmwds,		/* Count PSM watchdogs */
	phywatchdog,	/* Count Phy watchdogs (triggered by ucode) */

	/* MBSS counters, AP only */
	prq_entries_handled,	/* PRQ entries read in */
	prq_undirected_entries,	/*    which were bcast bss & ssid */
	prq_bad_entries,	/*    which could not be translated to info */
	atim_suppress_count,	/* TX suppressions on ATIM fifo */
	bcn_template_not_ready,	/* Template marked in use on send bcn ... */
	bcn_template_not_ready_done, /* ...but "DMA done" interrupt rcvd */
	late_tbtt_dpc,	/* TBTT DPC did not happen in time ( = 140 ) */

	/* per-rate receive stat counters */
	rx1mbps,	/* packets rx at 1Mbps */
	rx2mbps,	/* packets rx at 2Mbps */
	rx5mbps5,	/* packets rx at 5.5Mbps */
	rx6mbps,	/* packets rx at 6Mbps */
	rx9mbps,	/* packets rx at 9Mbps */
	rx11mbps,	/* packets rx at 11Mbps */
	rx12mbps,	/* packets rx at 12Mbps */
	rx18mbps,	/* packets rx at 18Mbps */
	rx24mbps,	/* packets rx at 24Mbps */
	rx36mbps,	/* packets rx at 36Mbps */
	rx48mbps,	/* packets rx at 48Mbps */
	rx54mbps,	/* packets rx at 54Mbps */
	rx108mbps,	/* packets rx at 108mbps */
	rx162mbps,	/* packets rx at 162mbps */
	rx216mbps,	/* packets rx at 216 mbps */
	rx270mbps,	/* packets rx at 270 mbps */
	rx324mbps,	/* packets rx at 324 mbps */
	rx378mbps,	/* packets rx at 378 mbps */
	rx432mbps,	/* packets rx at 432 mbps */
	rx486mbps,	/* packets rx at 486 mbps */
	rx540mbps,	/* packets rx at 540 mbps ( = 161 ) */

	/* pkteng rx frame stats */
	pktengrxducast, /* unicast frames rxed by the pkteng code */
	pktengrxdmcast, /* multicast frames rxed by the pkteng code */

	rfdisable,	/* count of radio disables */
	bphy_rxcrsglitch,	/* PHY count of bphy glitches */
	bphy_badplcp,

	txexptime,	/* Tx frames suppressed due to timer expiration */

	txmpdu_sgi,	/* count for sgi transmit */
	rxmpdu_sgi,	/* count for sgi received */
	txmpdu_stbc,	/* count for stbc transmit */
	rxmpdu_stbc,	/* count for stbc received */

	rxundec_mcst,	/* dot11WEPUndecryptableCount ( = 172 ) */

	/* WPA2 counters (see rxundec for DecryptFailureCount) */
	tkipmicfaill_mcst,	/* TKIPLocalMICFailures */
	tkipcntrmsr_mcst,	/* TKIPCounterMeasuresInvoked */
	tkipreplay_mcst,	/* TKIPReplays */
	ccmpfmterr_mcst,	/* CCMPFormatErrors */
	ccmpreplay_mcst,	/* CCMPReplays */
	ccmpundec_mcst,	/* CCMPDecryptErrors */
	fourwayfail_mcst,	/* FourWayHandshakeFailures */
	wepundec_mcst,	/* dot11WEPUndecryptableCount */
	wepicverr_mcst,	/* dot11WEPICVErrorCount */
	decsuccess_mcst,	/* DecryptSuccessCount */
	tkipicverr_mcst,	/* TKIPICVErrorCount */
	wepexcluded_mcst,	/* dot11WEPExcludedCount ( = 184 ) */

	dma_hang,	/* count for dma hang */
	reinit,		/* count for reinit */

	pstatxucast,	/* count of ucast frames xmitted on all psta assoc */
	pstatxnoassoc,	/* count of txnoassoc frames xmitted on all psta assoc */
	pstarxucast,	/* count of ucast frames received on all psta assoc */
	pstarxbcmc,	/* count of bcmc frames received on all psta */
	pstatxbcmc,	/* count of bcmc frames transmitted on all psta */

	cso_passthrough, /* hw cso required but passthrough */
	cso_normal,	/* hw cso hdr for normal process */
	chained,	/* number of frames chained */
	chainedsz1,	/* number of chain size 1 frames */
	unchained,	/* number of frames not chained */
	maxchainsz,	/* max chain size so far */
	currchainsz,	/* current chain size */
	rxdrop20s,	/* drop secondary cnt */
	pciereset,	/* Secondary Bus Reset issued by driver */
	cfgrestore,	/* configspace restore by driver ( = 201 ) */

	reinitreason0, /* reinitreason counters, 0: Unknown reason */
	reinitreason1,
	reinitreason2,
	reinitreason3,
	reinitreason4,
	reinitreason5,
	reinitreason6,
	reinitreason7,
}counters_cmd_type;
#endif

#ifdef HW_NETDEVICE_PANIC
typedef struct __sbp_data {
    struct perf_event * __percpu *bpevent;
    u64 addr;
    int idx;
    char ifname[IFNAMSIZ];
} sdp_data;
#endif

extern void hw_parse_special_ipv4_packet(uint8_t *pktdata, uint datalen);

#endif /* HW_DNS_DHCP_PARSE */
extern void get_customized_country_code_for_hw(char *country_iso_code, wl_country_t *cspec);
extern uint hw_get_bcn_timeout(void);
extern void hw_register_wifi_dsm_client(void);
extern void hw_wifi_dsm_client_notify(int dsm_id, const char *fmt, ...);
extern int hw_skip_dpc_in_suspend(void);
extern void hw_resched_dpc_ifneed(struct net_device *ndev);
#ifdef HW_PATCH_DISABLE_TCP_TIMESTAMPS
extern void hw_set_connect_status(struct net_device *ndev, int status);
#endif
extern void hw_dhd_check_and_disable_timestamps(void); /* called in ipv4/tcp_input.c */
#ifdef HW_WIFI_WAKEUP_SRC_PARSE
extern void parse_packet(struct sk_buff *skb);
#endif
#ifdef HW_LINK_COUNTERS
extern void hw_counters_hex_dump(wl_cnt_t *counters);
#endif

#ifdef HW_SDIO_QUALITY_STATISTIC
extern void sdio_quality_init(dhd_pub_t *dhd);
extern void sdio_quality_deinit(dhd_pub_t *dhd);
extern void sdio_quality_add(sdio_cmd_type type, sdio_cmd_dir dir, u8 is_retry);
extern const char *sdio_command_to_string(enum __sdio_cmd_type cmd);
#endif

#ifdef CONFIG_HW_WLANFTY_STATUS
#define WLANFTY_STATUS_HALTED		(1 << 1)
#define WLANFTY_STATUS_KSO_ERROR	(1 << 2)
#define WLANFTY_STATUS_RECOVERY		(1 << 3)
extern void hw_wlanfty_attach(dhd_pub_t *dhd);
extern void hw_dhd_wlanfty_detach(void);
extern void set_wlanfty_status(int value);
extern int wlanfty_status_value;
#endif

extern void dhd_unregister_handle(void);
extern void dhd_register_handle(void);

#ifdef HW_WL_COUNTERS_STATISTIC
extern const char *counters_command_to_string(enum __counters_cmd_type cmd);
#endif

#ifdef HW_NETDEVICE_PANIC
extern int hw_register_breakpoint(void *bpaddr, int ifidx, char *ifname);
extern int hw_unregister_breakpoint(int ifidx, char *ifname);
#endif

extern int hw_iovar_int_get(dhd_pub_t *dhd_pub, char *name, int *retval);
extern int hw_iovar_int_set(dhd_pub_t *dhd_pub, char *name, int value);
extern uint is_beta_user(void);

#ifdef HW_REG_RECOVERY
extern void hw_record_reg_recovery(void);
extern int hw_need_reg_recovery(unsigned long request_stamp);
extern int hw_need_reg_panic(void);
#endif

#if (defined(CONFIG_BCMDHD_PCIE) && defined(HW_WIFI_DMD_LOG) && defined(CONFIG_ARCH_HISI))
#define DHD_PCIE_BUF_SIZE (384)
extern void dsm_pcie_dump_reginfo(char* buf, u32 buflen);
#endif

#if defined(HW_WIFI_DMD_LOG)
#define DMD_SDIO_CMD52_MAX_CNT (3)
#define DMD_DHD_STATE_STOP (0)
#define DMD_DHD_STATE_OPEN (1)

extern int   hw_dmd_trigger_sdio_cmd(int dsm_id);
extern void  hw_dmd_increase_count(int dsm_id);
extern void  hw_dmd_set_dhd_state(int state);
extern void  hw_dmd_trace_log(const char *fmt, ...);
extern char* hw_dmd_get_trace_log(void);

#endif

#ifdef HW_DOZE_PKT_FILTER
#define HW_FILTER_PATTERN "%d 0 0 12 0xFFFFFF0000000000000000FF000000000000000000000000FFFF 0x0800450000000000000000%02x000000000000000000000000%02x%02x"
#define HW_FILTER_PATTERN_LEN   (26)
#define HW_PKT_FILTER_LEN       (128)
#define HW_PKT_FILTER_MAX_NUM   (32)
#define HW_PKT_FILTER_MIN_IDX   (32)
#define HW_PKT_FILTER_MAX_IDX   (HW_PKT_FILTER_MIN_IDX + HW_PKT_FILTER_MAX_NUM)
#define HW_PKT_FILTER_ID_BASE   (300)
#define HW_PKT_FILTER_ID_MAX    (HW_PKT_FILTER_ID_BASE + HW_PKT_FILTER_MAX_NUM)

extern void hw_attach_dhd_pub_t(dhd_pub_t* dhd);
extern void hw_detach_dhd_pub_t(void);

extern int hw_set_filter_enable(int on);
extern int hw_add_filter_items(hw_wifi_filter_item *items, int count);
extern int hw_clear_filters(void);
extern int hw_get_filter_pkg_stat(hw_wifi_filter_item *list, int max_count, int* count);

extern int net_hw_clear_filters(dhd_pub_t *pub);
extern int net_hw_add_filter_items(dhd_pub_t *pub, hw_wifi_filter_item *items, int count);
extern int net_hw_set_filter_enable(dhd_pub_t *pub, int on);

extern void dhd_pktfilter_offload_list(dhd_pub_t *dhd, hw_wifi_filter_item *list, int max_count, int* count);
#endif

#ifdef HW_AP_POWERSAVE
extern void hw_suspend_work_handler(struct work_struct *work);
extern int hw_dhd_fb_notify(int blank, unsigned long action, dhd_pub_t *pub);
#endif

#ifdef HW_LP_OVERSEA
extern void hw_set_region_oversea(int oversea);
extern void hw_set_pmlock(dhd_pub_t *dhd);
#endif

#ifdef HW_LOG_PATCH1
extern void hw_dhd_log(const char *fmt, ...);
extern void hw_dhd_looplog_start(void);
extern void hw_dhd_looplog(const char *fmt, ...);
extern void hw_dhd_looplog_end(void);
#endif

#ifdef HW_OTP_CHECK
#ifdef CONFIG_BCMDHD_SDIO
#define SROM_MAX        (768)
#else
#define SROM_MAX        (1536)
#endif

#define CRC_16_SIZE     (sizeof(u16))
#define OTP_BUF_SIZE    (SROM_MAX+CRC_16_SIZE)
#define HW_OTP_CMD    "otpimage"
#define HW_OTP_FILENAME "/data/vendor/wifi/wifi_otp"

typedef struct _otp_check_info {
    int offset_start;
    int offset_end;
    int error_sum;
} otp_check_info_t;

extern void hw_check_chip_otp(dhd_pub_t *dhd);
#endif
#ifdef HW_SOFTAP_BUGFIX
extern void hw_reset_beacon_interval(struct net_device *ndev);
#endif
#ifdef HW_PATCH_FOR_HANG
extern int hw_need_hang_with_assoc_status(int status);
extern int hw_need_hang_with_scanbusy(int error);
#endif
#endif /* end of __HW_WIFI_H__ */
