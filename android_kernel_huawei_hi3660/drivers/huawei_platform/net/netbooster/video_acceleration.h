
#ifndef _VIDEO_ACCELERATION
#define _VIDEO_ACCELERATION


/**********************************
*	Macro definition
***********************************/
/*Statistics duration second*/
#define MAX_STAT_SEC (32)
/* Maximum search http string length, can cause single packet*/
/* processing Siyanyo 3us, maximum bandwidth about 4Gbps */
#define MAX_DATA_LEN (600)

/*32 bit variable maximum value*/
#define BIT32_MAX (2^26)

/*The starting point of the return code for http*/
/*HTTP\1.0 200OK **** */
/*         ^          */
#define HTTP_ACK_FROM_START	(9)

/*HTTP response associated with video character string*/
#define CONTENT_TYPE1 ("Content-Type: video")
#define CONTENT_TYPE2 ("CONTENT-TYPE: video")
#define CONTENT_TYPE_LEN (18)
#define MIN_HTTP_LEN (30)
#define MAX_HTTP_LEN (1500)
/*Segment information reporting period*/
#define REPORT_TIME (15*HZ)

/*TCP port for HTTP*/
#define HTTP_PORT (80)

/*Data service string and length*/
#define DS_NET					("rmnet0")
#define DS_NET_LEN				(6)

/*Filter multiplier*/
#define MULTIPLIER_X5           (5)
#define MULTIPLIER_X10          (10)
#define MULTIPLIER_X20          (20)
#define MULTIPLIER_X64          (64)
#define MULTIPLIER_X128         (128)

/*MULTIPLIER_X64 x MULTIPLIER_X128 x 2.5*/
#define VIDEO_THRESHOLD         (20480)

/*Maximum remaining time, In seconds*/
#define MAX_REMAIN_TIME         (30)

/*The downlink video frame freezing threshold*/
#define DOWN_PKT_THRESHOLD      (100)
#define ERR_PKT_THRESHOLD       (30)
#define PKT_VECTOR_LEN          (4)

/*Fragment duration*/
#define DURATION_TIME           (10)

/*Maximum ASCII value*/
#define MAX_ASCII               (0x7f)
#define ASCII_CR                (0x0d)
#define ASCII_LF                (0x0a)
#define STR_HTTP                ("HTTP")
#define STR_HTTP_LEN            (4)

#define UDP_PKT_THRESHOLD       (5000)
#define UDP_PKT_MAX_THRESHOLD       (20000)

/*Switch to end status timer*/
#define SWITCH_END_TIMER        (30*HZ)
#define STREAM_END_TIMER        (20*HZ)

/*loop ip addr:127.0.0.1*/
#define LOOP_IP     ((127<<24)+(0<<16)+(0<<8)+1)

/*initial segment timer*/
#define TIMER_CNT    (5)

#define CHOICE_RESULT_VIDEO_CHANGE (0x01)
#define CHOICE_RESULT_VIDEO_SEG (0x02)

/*To ensure information security*/
#define IP_MASK (0x000000FF)

/**********************************
*	Status Definition
***********************************/
enum report_enable_type_enum {
	MODULE_DISABLE = 0,
	MODULE_ENABLE = 1,
	MODULE_INVALID = 2,
	MODULE_BUFF
};

enum hook_enable_type_enum {
	HOOK_DISABLE = 0,
	HOOK_ENABLE = 1,
	HOOK_INVALID = 2,
	HOOK_BUFF
};

enum video_protocal_enum
{
	VIDEO_PROTOCOL_HLS =  0x00, /* HLS */
	VIDEO_PROTOCOL_HPD =  0x01, /* HPD */
	VIDEO_PROTOCOL_DASH = 0x02, /* DASH */
	VIDEO_PROTOCOL_INVALID = 0xFF, /* Invalid */
	VIDEO_PROTOCOL_BUTT
};

enum pkt_type_enum {
	OLD_SECOND = 0,
	NEW_SECOND = 1,
	SECOND_BUFF
};

enum stream_state_enum {
	STREAM_TCP = 0,
	STREAM_UDP = 1,
	STREAM_BUFF
};

enum video_state_enum {
	VIDEO_STATUS_PREPARED = 0x00,  /* Video initial buffering status*/
	VIDEO_STATUS_START = 0x01,  /* Video playing start status*/
	VIDEO_STATUS_PAUSE = 0x02,  /* Video playing pause status*/
	VIDEO_STATUS_STALL = 0x03,  /* video data frame freezing status*/
	VIDEO_STATUS_END = 0x04,  /* Video playback end status*/
	VIDEO_STATUS_INVALID = 0xFF,  /* Invalid video playback state status*/
	VIDEO_STATUS_BUTT
};


/**********************************
*	Parameter and structure definition
***********************************/
/*Packet statistics*/
struct pkt_cnt {
	unsigned int in_pkt;
	unsigned int tin_len;
	unsigned int disodr;
	unsigned int rts;
	unsigned int uin_len;
	unsigned int uin_pkt;
};

struct pkt_stat {
	struct pkt_cnt stat[MAX_STAT_SEC];
	unsigned long time_stamp;
	unsigned int idx;
};

/*Filter Information,It's mainly code rate.*/
struct filter_para {
	unsigned int talp;
	unsigned int ualp;
	unsigned int mult;
	unsigned long alp_x64;
};

/*Video playback status information*/
struct video_state {
	unsigned int t_u_flag;
	unsigned int t_stat;
	unsigned long t_time_stamp;
	unsigned int u_stat;
	unsigned long u_time_stamp;
};

/*Video acceleration parameters information to be reported*/
struct video_parameter {
	unsigned char videoSegState;
	unsigned char videoProtocol;
	unsigned char videoRemainingPlayTime;
	unsigned char videoStatus;
	unsigned short aveCodeRate;
	unsigned short segSize;
	unsigned int videoIP;
	unsigned short videoPort;
	unsigned char segDuration;
	unsigned char segIndex;
};

/*stream information,include service ip and source port*/
struct stream_info {
	unsigned int t_num;
	unsigned int t_ip[PKT_VECTOR_LEN];
	unsigned short t_port[PKT_VECTOR_LEN];
	unsigned int u_num;
	unsigned int u_cnt[PKT_VECTOR_LEN];
	unsigned int u_ip[PKT_VECTOR_LEN];
	unsigned short u_port[PKT_VECTOR_LEN];
	unsigned long u_time_stamp[PKT_VECTOR_LEN];
};

/*segment information*/
struct seg_info {
	unsigned int seg_duration;
	unsigned int seg_remain;
	unsigned int seg_idx;
};

/*CHR information*/
struct video_chr_para {
	unsigned int vod_avg_speed;
	unsigned int vod_freez_num;
	unsigned int vod_time;
	unsigned int uvod_avg_speed;
	unsigned int uvod_freez_num;
	unsigned int uvod_time;
};

/**********************************
*	Export the definition of external interfaces
***********************************/
extern int set_vod_enable(uint8_t nf_hook_enable, uint8_t nl_event_enable);
extern int video_acceleration_init(void);
extern void video_acceleration_exit(void);
int chr_video_stat(struct video_chr_para *report);

#endif /*_VIDEO_ACCELERATION*/

