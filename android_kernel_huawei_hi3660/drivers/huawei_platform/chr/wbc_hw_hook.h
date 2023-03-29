
#ifndef _WEB_HW_HOOK
#define _WEB_HW_HOOK

/*Zero*/
#define ZERO						0
/*initial reset*/
#define INIT_NUM				0
/*The length of the hash table*/
#define HASH_MAX				256
/*The length of the http header recoded*/
#define HTTP_ACK_HEAD_NUM		18
/*The starting point of the return code for http*/
#define HTTP_ACK_FROM_START	9
/*Timer length*/
#define CHECK_TIME				(15*HZ)
/*Determines how long the page has timed out*/
#define EXPIRE_TIME				(30*HZ)
/*Determines how long the page has timed out*/
#define DELETE_TIME				(30*HZ)
/*The period in which the page statistics are reported*/
#define REPORT_TIME			(15*60*HZ)
/*The period in which the page statistics are forbid*/
#define FORBID_TIME				(48*60*60*HZ)
/*http protocal default tcp port*/
#define HTTP_PORT				80
/*Web page statistics reporting chr events*/
#define CHR_WEB_STAT_EVENT	9
/*RTT threshold*/
#define RTT_THRESHOLD			(600)
/*RTT threshold for count*/
#define RTT_THRESHOLD_L1		(0)
#define RTT_THRESHOLD_L2		(50)
#define RTT_THRESHOLD_L3		(100)
#define RTT_THRESHOLD_L4		(150)
#define RTT_THRESHOLD_L5		(200)
/*Web delay threshold*/
#define WEB_DELAY_THRESHOLD	(8000)
/*Web delay threshold for count*/
#define DELAY_THRESHOLD_L1	(0)
#define DELAY_THRESHOLD_L2	(300)
#define DELAY_THRESHOLD_L3	(400)
#define DELAY_THRESHOLD_L4	(500)
#define DELAY_THRESHOLD_L5	(700)
#define DELAY_THRESHOLD_L6	(1000)
/*The largest RTT*/
#define MAX_RTT					(REPORT_TIME*2*HZ)
/*HTTP response string length*/
#define HTTP_RSP_LEN			(16)
/*HTTP str length for Comparison*/
#define STR_HTTP_LEN			(4)
#define STR_GET_LEN				(3)
#define STR_POST_LEN			(4)
/*cs conversion ratio*/
#define MULTIPLE				(1000/HZ)
/*data service string length*/
#define DS_NET					("rmnet")
#define DS_NET_LEN				(5)

#define SYN_NO_ACK_REPORT_TIME	(60*HZ)
#define WEB_NO_ACK_REPORT_TIME	(60*HZ)
#define WEB_DELAY_REPORT_TIME	(60*HZ)
#define WEB_FAIL_REPORT_TIME	(60*HZ)
#define TCP_RTT_LARGE_REPORT_TIME	(60*HZ)
#define SYN_NO_ACK_MAX	(3)
#define WEB_NO_ACK_MAX	(3)
#define WEB_FAIL_MAX	(3)
#define WEB_DELAY_MAX	(3)
#define TCP_RTT_LARGE_MAX	(3)
#define IPV4ADDR_MASK	(0x0000FFFF)
#define RMNET_INTERFACE (0)
#define WLAN_INTERFACE (1)
#define RNT_STAT_SIZE (2)
#define MAX_JIFFIES	(0xFFFFFFFFFFFFFFFF)
#define MAX_VALID_U16     (65534)
#define NS_CONVERT_TO_MS  (1000000)
#define FILTER_TIME_LIMIT (HZ/4)
#define ALPHA_FILTER_PARA (8)

/*response and report type*/
enum {
	WEB_STAT = 0,
	WEB_NO_ACK,
	WEB_FAIL,
	WEB_DELAY,
	TCP_RTT_LARGE,
	DNS_FAIL,
	DNS_BIG_LATENCY,
	SYN_NO_ACK,
	SYN_SUCC,
	WEB_SUCC,
	UN_KNOW,
};

/*TCP response type*/
enum {
	HTTP_GET = 0,
	HTTP_SYN,
	OTHER,
};

/*Whether the parameter is use*/
enum {
	IS_UNUSE = 0,
	IS_USE,
};

int web_chr_init(void);
void web_chr_exit(void);
void web_proc(void);
u32 http_timer(void);

/*Hook package for data storage structures*/
struct http_stream {
	u32 src_addr;
	u32 dst_addr;
	u16 tcp_port;
	u8 is_valid;
	u8 type;
	unsigned long time_stamp;
	unsigned long get_time_stamp;
	u32 uid;
	u32 resp_code;
	u8 interface;
	unsigned long ack_time_stamp;
};

#define CHR_MAX_REPORT_APP_COUNT (10)

#define DATA_REG_TECH_TAG (20)
#define GET_AP_REPORT_TAG (21)

struct report_app_stat {
	u32 tcp_rtt;
	u32 web_delay;
	u32 succ_num;
	u32 fail_num;
	u32 no_ack_num;
	u32 total_num;
	u32 tcp_total_num;
	u32 tcp_succ_num;
	u32 delay_num_L1;
	u32 delay_num_L2;
	u32 delay_num_L3;
	u32 delay_num_L4;
	u32 delay_num_L5;
	u32 delay_num_L6;
	u32 rtt_num_L1;
	u32 rtt_num_L2;
	u32 rtt_num_L3;
	u32 rtt_num_L4;
	u32 rtt_num_L5;
};

/*Web page statistics structure*/
struct http_return {
	u32 tcp_rtt;
	u32 web_delay;
	u32 succ_num;
	u32 fail_num;
	u32 no_ack_num;
	u32 total_num;
	u32 report_type;
	u32 tcp_total_num;
	u32 delay_num_L1;
	u32 delay_num_L2;
	u32 delay_num_L3;
	u32 delay_num_L4;
	u32 delay_num_L5;
	u32 delay_num_L6;
	u32 rtt_num_L1;
	u32 rtt_num_L2;
	u32 rtt_num_L3;
	u32 rtt_num_L4;
	u32 rtt_num_L5;
	u32 tcp_succ_num;
	u32 uid;
	u32 http_resp;
	struct report_app_stat report_app_stat_list[CHR_MAX_REPORT_APP_COUNT];
	u32 highest_tcp_rtt;
	u32 lowest_tcp_rtt;
	u32 last_tcp_rtt;
	u32 highest_web_delay;
	u32 lowest_web_delay;
	u32 last_web_delay;
	u32 server_addr;
	u32 rtt_abn_server_addr;
	u32 vod_avg_speed;
	u32 vod_freez_num;
	u32 vod_time;
	u32 uvod_avg_speed;
	u32 uvod_freez_num;
	u32 uvod_time;
	u32 tcp_handshake_delay;
	u32 http_get_delay;
	u32 http_send_get_num;
};

/*this is temporarily stores the RTT*/
struct rtt_from_stack {
	u32 tcp_rtt;
	u32 is_valid;
	u32 uid;
	u32 rtt_dst_addr;
};

/*CHR Key parameters*/
struct chr_key_val {
	atomic64_t tcp_buf_time;
	atomic64_t udp_buf_time;
	volatile unsigned long tcp_last;
	volatile unsigned long udp_last;
};


void wifi_disconnect_report(void);
int set_report_app_uid(int index, u32 uid);

#endif /*_WEB_HW_HOOK*/

