#ifndef _NB_NETLINK_H
#define _NB_NETLINK_H

#define MAX_RTT_LIST_LEN 32

enum nb_cmd_type
{
	NBMSG_REG = 0x10,  /* NLMSG_MIN_TYPE */
	NBMSG_UNREG,
	NBMSG_VOD_REQ,
	NBMSG_KSI_REQ,
	NBMSG_NATIVE_REG,
	NBMSG_NATIVE_UNREG,
	NBMSG_NATIVE_GET_RTT,
	NBMSG_APP_QOE_PARAMS_REQ,
	NBMSG_REQ_BUTT,
};

enum app_qoe_cmd_type
{
	APP_QOE_MSG_UID_REQ = 3,/*DATA_SEND_TO_KERNEL_APP_QOE_UID  3*/
	APP_QOE_MSG_RSRP_REQ = 4,/*DATA_SEND_TO_KERNEL_APP_QOE_RSRP 4*/
	APP_QOE_MSG_BUTT,
};

enum nb_evt_type
{
	NBMSG_EVT_INVALID = 0,
	NBMSG_VOD_EVT,
	NBMSG_KSI_EVT,
	NBMSG_NATIVE_RTT,
	NBMSG_EVT_BUTT,
};

struct vod_event {
	uint8_t videoSegState;
	uint8_t videoProtocol;
	uint8_t videoRemainingPlayTime;
	uint8_t videoStatus;
	uint16_t aveCodeRate;
	uint16_t segSize;
	uint32_t videoIP;
	uint16_t videoPort;
	uint8_t segDuration;
	uint8_t segIndex;
};

struct ksi_event {
	uint8_t slowType;
	uint8_t avgAmp;
	uint8_t duration;
	uint8_t timeStart;
};

struct native_event {
	unsigned int rtt_list[MAX_RTT_LIST_LEN];
	unsigned int max_list[MAX_RTT_LIST_LEN];
	unsigned int avg_list[MAX_RTT_LIST_LEN];
	unsigned int len;
};

struct ksi_request {
	int8_t nf_hook_enable;
	int8_t nl_event_enable;
};

struct app_qoe_request {
	int msg_type;
	int app_uid;
	int report_period;
	int rsrp;
	int rsrq;
};

struct vod_request {
	int8_t nf_hook_enable;
	int8_t nl_event_enable;
};

struct native_requst {
	int8_t len;
	int8_t rcv;
};

void nb_notify_event(enum nb_evt_type event_type, void *data, int size);
extern int get_rtt_list(struct native_event *rtt_event, unsigned int list_len);

#endif /*_NB_NETLINK_H*/
