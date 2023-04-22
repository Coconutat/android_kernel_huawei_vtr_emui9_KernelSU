

#ifndef __HW_FI_UTILS_H__
#define __HW_FI_UTILS_H__

#include <linux/spinlock_types.h>
#include <linux/netfilter_ipv4.h>

#define FI_TIMER_INTERVAL           1           /* 定时器时间间隔 */
#define FI_HB_STUDY_NUM             20          /* 收集20个包，然后判断是否为心跳流 */
#define FI_HB_PKT_SUB               4           /* 判断心跳流: 上下行报文个数相差不超过n个 */
#define FI_HB_MAX_PPS               2           /* 判断心跳流: 每秒报文个数不能大于该值 */
#define FI_HB_MAX_TIME_GAP          3           /* 判断心跳流: 心跳报文最大间隔 */
#define FI_HB_LOST_TOLERANCE        2           /* 心跳丢包的容忍度 */
#define FI_HB_PKT_LEN               100         /* 心跳报文的最大长度 */
#define FI_PKT_LOST_PUNISH          50          /* punish = lost_ptk_num * 50 */

#define FI_BATTLE_UP_PKT_PER_SEC    3           /* 判断对战流, 上行报文最小频率 */
#define FI_BATTLE_DOWN_PKT_PER_SEC  8           /* 判断对战流, 下行报文最小频率 */
#define FI_BATTLE_START_THRESH      2           /* 连续满足条件多少次才算对战开始 */
#define FI_AUDIO_BATTLE_BOUNDARY    120         /* 对战流与语音流平局报文长度的分界线 */

#define FI_SRTT_VAR                 25          /* SRTT平滑系数 */
#define FI_HB_SRTT_VAR              25          /* 心跳SRTT平滑系数 */
#define FI_STUDY_INTERVAL           (1*1000)    /* 每轮学习的时间间隔  */
#define FI_MAX_STUDY_TIME           (5*60*1000) /* 限制自学习的时间不超过5min */
#define FI_MIN_IP_PKT_LEN           20
#define FI_BATTLE_STOP_THRESH       5           /* 超过多少秒没有对战报文则认为对战结束 */
#define FI_BATTLE_FLOW_CON          4           /* 判断对战流: 连续几秒满足对战流的条件 */
#define FI_NO_DOWNLINK_TIME         3           /* 连续多少秒没有下行报文，用于判断断线重连 */
#define FI_RECONN_LIMIT             (10*1000)   /* 报断线重连的间隔不能小于10s */
#define FI_RECONN_THRESH            180         /* 发现重建连接且rtt大于该值报断线重连 */

#define FI_QQFC_PKT_AVG_LEN         400         /* 判断对战流: QQ飞车上行报文平均长度 */
#define FI_QQFC_UP_PPS_MIN          14          /* 判断对战流: QQ飞车上行最小报文频率 */
#define FI_QQFC_UP_PPS_MAX          16          /* 判断对战流: QQ飞车上行最大报文频率 */
#define FI_APP_DISCONN              5           /* 判断QQ飞车是否断线 */
#define FI_APP_DISCONN_STOP         30          /* 断线多少秒之后报对战结束 */

#define FI_HYXD_MIN_LEN             20          /* 荒野行动最小报文长度 */
#define FI_HYXD_SEQ_OFFSET          12          /* 荒野行动seq字段在报文中的偏移 */
#define FI_HYXD_ACK_OFFSET          16          /* 荒野行动ack字段在报文中的偏移 */
#define FI_UPPKT_WAIT               200         /* 将上行报文作为200ms的定时器 */
#define FI_ACK_MAX                  60000U      /* 用于判断ack反转 */
#define FI_RETRANS_PKT              2000        /* 用于判断报文重传 */
#define FI_ACK_MAX_WAIT             (10*1000)   /* 等待ack的最长时间10s */
#define FI_TCP_OPT_HDR_LEN          2           /* tcp选项首部的长度 */
#define FI_TCP_OPT_MPTCP            30          /* tcp选项类型 mptcp */
#define FI_MPTCP_DSS_MINLEN         8           /* mptcp dss最小长度 */
#define FI_MPTCP_SUBTYPE_DSS        2           /* mptcp dss选型类型 */
#define FI_GAME_UU_RTT              10          /* 如果一个游戏在使用uu，那么他算出的rtt小于10ms */
#define FI_UU_BASE_RTT              25          /* UU rtt的基础上加上一个base */
#define FI_UU_CACHE_NUM             8           /* UU rtt的基础上加上一个base */

#define FI_BH3_SEQ_CACHE_NUM        4           /* 崩坏3: 缓存的上行报文的个数 */
#define FI_BH3_KEY_WORD             0xFF        /* 崩坏3: 关键字 */
#define FI_BH3_KEY_OFFSET_UP        9           /* 崩坏3: 上行报文关键字的偏移 */
#define FI_BH3_KEY_OFFSET_DOWN      7           /* 崩坏3: 下行报文关键字的偏移 */
#define FI_BH3_SEQ_OFFSET_UP        10          /* 崩坏3: 上行报文提取seq的位置 */
#define FI_BH3_ACK_OFFSET_DOWN      8           /* 崩坏3: 下行报文提取ack的位置 */
#define FI_BH3_VRF_OFFSET_UP        2           /* 崩坏3: 上行报文提取verify的位置 */
#define FI_BH3_VRF_OFFSET_DOWN      12          /* 崩坏3: 下行报文提取verify的位置 */
#define FI_BH3_UP_LEN               12          /* 崩坏3: 上行带seq报文长度 */
#define FI_BH3_DOWN_LEN             14          /* 崩坏3: 下行带ack报文长度 */

#define FI_BATTLE_ONGOING           4           /* 判断对战是否还在进行：上行报文频率不低于4个/s */
#define FI_BATTLE_START_PORT_MIN    1025        /* 通过端口筛选对战报文，端口小于1025的不处理 */

#define FI_IPPROTO_UDP              17
#define FI_IPPROTO_TCP              6
#define FI_IP_VER_4                 4
#define FI_IP_HDR_LEN_BASE          4
#define FI_TCP_HDR_LEN_BASE         4
#define FI_LOOP_ADDR                127

#define FI_MS                       1000        /* 毫秒转秒 */
#define FI_PERCENT                  100         /* 百分比单位转换 */
#define FI_MAX_ORG_RTT              700         /* limit the original value of rtt */
#define FI_MIN_RTT                  40          /* limit the min value of rtt */
#define FI_MAX_RTT                  500         /* limit the max value of rtt */
#define FI_RTT_BASE                 50          /* limit the min value of rtt */

#define FI_APPID_NULL               0
#define FI_APPID_HYXD               1           /* 荒野行动appid */
#define FI_APPID_WZRY               2           /* 王者荣耀appid */
#define FI_APPID_CJZC               3           /* 刺激战场appid */
#define FI_APPID_QJCJ               4           /* 全军出击appid */
#define FI_APPID_CYHX               5           /* 穿越火线appid */
#define FI_APPID_QQFC               6           /* QQ飞车appid */
#define FI_APPID_BH3                7           /* 崩坏3appid */
#define FI_APPID_UU                 11          /* UU加速器 */
#define FI_APPID_XUNYOU             12          /* 迅游加速器 */
#define FI_APPID_MAX                16          /* appid上限 */

#define FI_TRUE                     1
#define FI_FALSE                    0
#define FI_FAILURE                  -1
#define FI_SUCCESS                  0

#define FI_FLOW_TABLE_SIZE          0x10                        /* hash桶的规模 16 */
#define FI_FLOW_TABLE_MASK          (FI_FLOW_TABLE_SIZE - 1)    /* hash桶的掩码 */
#define FI_FLOW_AGING_TIME          60000                       /* 流表老化时间, ms, 1分钟*/
#define FI_FLOW_NODE_LIMIT          64                          /* 流结点规模的上限 */

#define GAME_SDK_STATE_DEFAULT      0                          /* 上层下发的游戏状态 */
#define GAME_SDK_STATE_BACKGROUND   3                          /* 上层下发的游戏状态: 游戏转入后台 */
#define GAME_SDK_STATE_FOREGROUND   4                          /* 上层下发的游戏状态: 游戏转到前台 */
#define GAME_SDK_STATE_DIE          5                          /* 上层下发的游戏状态: 游戏退出 */
#define FI_STATUS_BATTLE_START      1                          /* FI上报的游戏状态: 对战开始 */
#define FI_STATUS_BATTLE_STOP       0                          /* FI上报的游戏状态: 对战结束 */
#define FI_STATUS_BATTLE_RECONN     0x10                       /* FI上报的游戏状态: 断线重连 */
#define FI_STATUS_DISCONN           0x01                       /* 内部维护的游戏状态: 游戏断线 */

#define FI_APP_TYPE_GAME            1

#define FI_SWITCH_STATUS            0x80000000                 /* FI上报的游戏状态的开关 */
#define FI_SWITCH_RTT               0x40000000                 /* FI上报的rtt的开关 */

#define FI_DEBUG                    0
#define FI_INFO                     1

#define FI_APP_NAME_WZRY           "com.tencent.tmgp.sgame"     /* app名称: 王者荣耀 */
#define FI_APP_NAME_CJZC           "com.tencent.tmgp.pubgmhd"   /* app名称: 刺激战场 */
#define FI_APP_NAME_QJCJ           "com.tencent.tmgp.pubgm"     /* app名称: 全军出击 */
#define FI_APP_NAME_HYXD_HW        "com.netease.hyxd.huawei"    /* app名称: 荒野行动 */
#define FI_APP_NAME_HYXD           "com.netease.hyxd"           /* app名称: 荒野行动 */
#define FI_APP_NAME_CYHX           "com.tencent.tmgp.cf"        /* app名称: 穿越火线 */
#define FI_APP_NAME_QQFC           "com.tencent.tmgp.speedmobile"/* app名称: QQ飞车 */
#define FI_APP_NAME_BH3            "com.miHoYo.bh3.huawei"      /* app名称: 崩坏3 */
#define FI_APP_NAME_BH3_2          "com.miHoYo.bh3.qihoo"       /* app名称: 崩坏3 */
#define FI_APP_NAME_BH3_3          "com.miHoYo.bh3.uc"          /* app名称: 崩坏3 */
#define FI_APP_NAME_BH3_4          "com.tencent.tmgp.bh3"       /* app名称: 崩坏3 */
#define FI_APP_NAME_BH3_5          "com.miHoYo.enterprise.NGHSoD" /* app名称: 崩坏3 */
#define FI_APP_NAME_UU             "com.netease.uu"             /* app名称: UU加速器 */
#define FI_APP_NAME_XUNYOU         "cn.wsds.gamemaster"         /* app名称: 迅游加速器 */

/* mptcp DSS(Data Sequence Signal)类型消息的结构, 2 bytes */
typedef struct fi_mptcp_dss_t
{
#if defined(__BIG_ENDIAN_BITFIELD)
	uint16_t     subtype : 4;
	uint16_t     rev1 : 4;
	uint16_t     rev2 : 3;
	uint16_t     fin : 1;
	uint16_t     seq8 : 1;      /* seq是否为8字节, 默认4字节 */
	uint16_t     seqpre : 1;    /* 是否包含seq */
	uint16_t     ack8 : 1;      /* ack是否为8字节, 默认4字节 */
	uint16_t     ackpre : 1;    /* 是否包含ack */
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	uint16_t     rev1 : 4;
	uint16_t     subtype : 4;
	uint16_t     ackpre : 1;
	uint16_t     ack8 : 1;
	uint16_t     seqpre : 1;
	uint16_t     seq8 : 1;
	uint16_t     fin : 1;
	uint16_t     rev2 : 3;
#else
#error	"Adjust your <asm/byteorder.h> defines"
#endif
} fi_mptcp_dss;

/* 收到的上层消息: 游戏app启动 */
typedef struct fi_msg_applaunch_t
{
	uint32_t    uid;            /* 游戏进程在手机中的uid */
	uint32_t    switchs;        /* 各种开关 bit:0 是否上报游戏状态变化; bit:1 是否上报rtt */
	char        appname[0];     /* 游戏名称的字符串 */
} fi_msg_applaunch;

/* 收到的上层消息: 游戏app状态变化 */
typedef struct fi_msg_appstatus_t
{
	uint32_t    uid;            /* 游戏进程在手机中的uid */
	uint32_t    appstatus;      /* 游戏的状态，主要关注游戏退出 */
	char        appname[0];     /* 游戏名称的字符串 */
} fi_msg_appstatus;

/* 发送到daemon的数据结构: rtt */
typedef struct fi_report_rtt_t
{
	uint32_t uid;
	uint32_t apptype;
	uint32_t rtt;
} fi_report_rtt;

/* 发送到daemon的数据结构: 游戏状态变化 */
typedef struct fi_report_status_t
{
	uint32_t uid;
	uint32_t apptype;
	uint32_t status;
} fi_report_status;

typedef struct fi_pkt_t
{
	uint8_t     *data;          /* payload data */

	uint16_t    len;            /* 负载的总长度，包括bufdatalen + 非线性链表中的负载的长度 */
	uint16_t    bufdatalen;     /* 线性连续空间中的负载的长度 */
	uint16_t    sport;          /* network byte order, big-endian */
	uint16_t    dport;          /* network byte order, big-endian */

	uint8_t     proto;          /* tcp or udp */
	uint8_t     dir : 2;        /* SA_DIR_UP or SA_DIR_DOWN */
	uint8_t     mptcp : 1;      /* mptcp or not */
	uint8_t     rev : 5;
	uint8_t     rev2[6];

	uint32_t    sip;            /* network byte order, big-endian */
	uint32_t    dip;            /* network byte order, big-endian */

	uint32_t    seq;            /* tcp seq num */
	uint32_t    ack;            /* tcp ack num */

	/* only for netdelay calculate */
	int64_t     msec;           /* time stamp of this pkt, millisecond */
} fi_pkt;

/* 崩坏3每条流私有的数据: 记录上行报文的序列号 */
typedef struct fi_flow_bh3_t
{
	uint16_t    seq;                /* 报文的序列号 */
	uint16_t    verify;             /* 校验码 */
	uint32_t    time;               /* 时间戳 */
} fi_flow_bh3;

/* 每个流结点中保存的与rtt相关的缓存 */
typedef struct fi_flowctx_st
{
	uint8_t     flowtype;           /* hb flow, nonhb flow, init */
	uint8_t     appid;
	uint8_t     battle_times;       /* 连续满足对战开始的条件的次数 */
	uint8_t     btflow_times;       /* 连续满足对战流的条件的次数 */
	uint16_t    uppktnum;           /* 用于对战流判定: 上行报文个数 */
	uint16_t    downpktnum;         /* 用于对战流判定: 下行报文个数 */

	uint16_t    hbupnum;            /* 用于心跳流判定: 上行报文个数 */
	uint16_t    hbdownnum;          /* 用于心跳流判定: 下行报文个数 */
	uint32_t    hbstudytime;        /* 用于心跳流判定: 开始进行心跳流学习的时间, ms */

	uint32_t    uppktbytes;
	uint32_t    seq;

	int64_t     studystarttime;     /* 每轮自学习开始的时间, ms */

	int64_t     flowstarttime;      /* time stamp of the first pkt of this flow, ms */
	int64_t     uppkttime;          /* 上行报文的时间戳 */

	union
	{                                                    /* 特定游戏相关私有数据 */
		fi_flow_bh3 flow_bh3[FI_BH3_SEQ_CACHE_NUM];      /* 崩坏3私有的数据 */
	};
} fi_flowctx;

/* 流结点 */
typedef struct fi_flow_node_t
{
	struct list_head list;

	uint32_t    sip;
	uint32_t    dip;

	uint16_t    sport;
	uint16_t    dport;
	uint32_t    updatetime;

	fi_flowctx  flowctx;
} fi_flow_node;

/* 流表的表头 */
typedef struct fi_flow_head_t
{
	struct list_head list;
} fi_flow_head;

/* 流表相关的数据结构 */
typedef struct fi_flow_t
{
	fi_flow_head    flow_table[FI_FLOW_TABLE_SIZE];    /* 流表的表头 */
	atomic_t        nodenum;                           /* 流表中节点的总数 */
	spinlock_t      flow_lock;                         /* 流表的锁 */
} fi_flow;

#define FI_FLOW_SAME(a, b)    \
( \
	((a)->sip == (b)->sip && \
	(a)->dip == (b)->dip && \
	(a)->sport == (b)->sport && \
	(a)->dport == (b)->dport) \
	|| \
	((a)->sip == (b)->dip && \
	(a)->dip == (b)->sip && \
	(a)->sport == (b)->dport && \
	(a)->dport == (b)->sport) \
)

enum fi_pkt_dir
{
	FI_DIR_ANY = 0,
	FI_DIR_UP,
	FI_DIR_DOWN,
	FI_DIR_MAX
};

/* 本模块保存的app状态 */
typedef struct fi_app_info_t {
	uint32_t        uid;
	uint16_t        appid;
	uint8_t         valid;          /* 通过该标记位表示uid是否有效 */
	uint8_t         rev;
	uint32_t        appstatus;      /* app当前当前的状态，前后台等等 */
	uint32_t        switchs;        /* 各种开关,  */
} fi_app_info;

/* rtt cache data for every game */
typedef struct fi_gamectx_st
{
	uint8_t     appid;
	uint8_t     appstatus;
	uint8_t     applocalstatus;     /* FI内部保存的一些辅助状态 */
	uint8_t     preuplinkpktnum;    /* 对战流上一秒上行报文的个数 */
	uint8_t     predownlinkpktnum;  /* 对战流上一秒下行报文的个数 */
	uint8_t     uplinkpktnum;       /* 记录对战流当前一秒下行报文的个数 */
	uint8_t     downlinkpktnum;     /* 记录对战流当前一秒下行报文的个数 */
	uint8_t     nouplinkpkt;        /* 记录没有上行报文所持续的时间 */

	uint8_t     nodownlinkpkt;      /* 记录没有下行报文所持续的时间 */
	uint8_t     rev[3];
	int16_t     hbrtt;              /* 通过心跳算出的rtt */
	int16_t     hbsrtt;             /* 通过心跳算出的srtt */

	uint16_t    battle_flow_port;
	uint16_t    hb_flow_port;
	uint32_t    cliip;              /* 端侧的ip地址 */
	int32_t     rawrtt;             /* 通过seq/ack算出的最原始的rtt, 用于判断游戏是否被代理 */
	int32_t     rtt;                /* 通过对战流算出的rtt */
	int32_t     srtt;               /* 通过对战流算出的srtt */
	int32_t     battlertt;          /* 通过对战流算出的最终rtt */
	int32_t     final_rtt;          /* 综合对战和心跳之后的rtt */

	int64_t     downlast;           /* time stamp of last down pkt, ms */
	int64_t     updatetime;         /* time stamp */
	int64_t     rtt_integral;
	int64_t     reconntime;         /* 报断线重连的时间 */
} fi_gamectx;

typedef struct fi_mpctx_t
{
	uint32_t    seq;                /* 缓存的用于计算rtt的seq */
	uint32_t    preseq;             /* mptcp通道当前seq已经进行到哪里了 */
	int64_t     uppkttime;          /* 记录seq的报文的时间戳 */
} fi_mpctx;

/* fi模块上下文 */
typedef struct fi_ctx_t
{
	struct timer_list tm;                       /* timer */
	struct mutex    appinfo_lock;               /* 操作appinfo的锁 */
	struct mutex    nf_mutex;                   /* 保护nf注册的锁 */
	uint32_t        memused;                    /* 模块使用的内存统计 */

	uint8_t         enable;                     /* 模块使能标记 */
	uint8_t         appidmin;                   /* 记录最小的appid */
	uint8_t         appidmax;                   /* 记录最大的appid */
	uint8_t         nf_exist;                   /* 是否已经存在钩子函数 */

	fi_app_info     appinfo[FI_APPID_MAX];      /* 保存配置appid-uid, 按appid索引 */
	fi_gamectx      gamectx[FI_APPID_MAX];      /* 每个游戏计算rtt的缓存, 按appid索引 */
	fi_mpctx        mpctx[FI_APPID_MAX];        /* mptcp场景下保存多路径的信息 */
} fi_ctx;

typedef enum
{
	FI_FLOWTYPE_INIT,
	FI_FLOWTYPE_HB,
	FI_FLOWTYPE_BATTLE,
	FI_FLOWTYPE_UNKNOWN
} fi_rtt_flowtype_enum;


#define FI_LOGD(fmt, ...) \
	do { \
		if (FI_DEBUG) { \
			hwlog_info("%s"fmt"\n", __func__, ##__VA_ARGS__); \
		} \
	} while (0)

#define FI_LOGI(fmt, ...) \
	do { \
		if (FI_INFO) { \
			hwlog_info("%s"fmt"\n", __func__, ##__VA_ARGS__); \
		} \
	} while (0)


#define FI_LOGE(fmt, ...) \
	do { \
			hwlog_err("%s"fmt"\n", __func__, ##__VA_ARGS__); \
	} while (0)

#define FI_MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define FI_MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define FI_RANGE(v,l,r) ((v) < (l) ? (l) : ((v) > (r) ? (r) : (v)))
#define FI_ABS_SUB(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define FI_SRVPORT(p) (((p)->dir == FI_DIR_UP) ? (p)->dport : (p)->sport)
#define FI_CLIPORT(p) (((p)->dir == FI_DIR_UP) ? (p)->sport : (p)->dport)
#define FI_CLIIP(p) (((p)->dir == FI_DIR_UP) ? (p)->sip : (p)->dip)
#define FI_MS2SEC(ms) ((ms) / FI_MS)
#define FI_APPID_VALID(id) ((id) < FI_APPID_MAX && (id) > FI_APPID_NULL)
#define FI_HAS_NO_APPID(ctx) ((ctx.appidmin == 0) && (ctx.appidmax == 0))
#define FI_BATTLING(s) (((s) & FI_STATUS_BATTLE_START) != 0)
#define FI_DISCONN(s) (((s) & FI_STATUS_DISCONN) != 0)

extern fi_ctx g_fi_ctx;
extern uint32_t fi_flow_node_num(void);
extern uint32_t fi_flow_hash(uint32_t sip, uint32_t dip, uint32_t sport, uint32_t dport);
extern fi_flow_node *fi_flow_get(fi_pkt *pktinfo, fi_flow_head *head, uint32_t addflow);
extern fi_flow_head *fi_flow_header(uint32_t index);
extern void  fi_rtt_timer(void);
extern void  fi_flow_init(void);
extern void  fi_flow_age(void);
extern void  fi_flow_clear(void);
extern void  fi_flow_lock(void);
extern void  fi_flow_unlock(void);
extern void  fi_free(void *ptr, uint32_t size);
extern void *fi_malloc(uint32_t size);
extern void  fi_rtt_status(uint32_t appid, uint32_t status);
extern void  fi_rtt_judge_reconn(fi_pkt *pktinfo, fi_gamectx *gamectx);
extern int   fi_rtt_judge_battle_stop(fi_gamectx *gamectx, fi_app_info *appinfo);
extern void  fi_rtt_cal_bh3(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx);
extern int   fi_rtt_entrance(fi_pkt *pktinfo, fi_flowctx *flowctx, uint32_t appid);
unsigned int fi_hook_out(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
unsigned int fi_hook_in(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

#endif /* __HW_FI_UTILS_H__ */
