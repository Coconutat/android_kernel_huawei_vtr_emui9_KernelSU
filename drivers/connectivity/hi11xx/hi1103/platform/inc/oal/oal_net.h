

#ifndef __OAL_NET_H__
#define __OAL_NET_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "platform_spec.h"
#include "oal_types.h"
#include "oal_mm.h"
#include "oal_util.h"
#include "oal_schedule.h"
#include "oal_list.h"
#include "arch/oal_net.h"
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define OAL_IF_NAME_SIZE   16
#define OAL_NETBUF_DEFAULT_DATA_OFFSET 48  /* 5115上实际测得data比head大48，用于netbuf data指针复位 */

#define OAL_ASSOC_REQ_IE_OFFSET        28    /* 上报内核关联请求帧偏移量 */
#define OAL_ASSOC_RSP_IE_OFFSET        30    /* 上报内核关联响应帧偏移量 */
#define OAL_AUTH_IE_OFFSET             30
#define OAL_FT_ACTION_IE_OFFSET        40
#define OAL_ASSOC_RSP_FIXED_OFFSET     6     /* 关联响应帧帧长FIXED PARAMETERS偏移量 */
#define OAL_MAC_ADDR_LEN               6
#define OAL_PMKID_LEN                  16
#define OAL_WPA_KEY_LEN                32
#define OAL_WPA_SEQ_LEN                16
#define OAL_WLAN_SA_QUERY_TR_ID_LEN    2
#define OAL_MAX_FT_ALL_LEN             518   /* MD:5 FT:257 RSN:256 */
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
#define MAX_STA_NUM_OF_ONE_GROUP    32      /*一个组播组最大支持32个用户*/
#define MAX_NUM_OF_GROUP            256     /*最大支持256个组播组*/
#endif
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
#define MAX_BLACKLIST_NUM          128
#endif
/*
 * Byte order/swapping support.
 */
#define OAL_LITTLE_ENDIAN    1234
#define OAL_BIG_ENDIAN       4321
#define OAL_BYTE_ORDER OAL_BIG_ENDIAN

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined (_PRE_WLAN_FEATURE_DFR)
/* hi1102  由于内核注册端口28 NETLINK_HISI_WIFI_MSG已被占用，所以此处使用未使用的27 NETLINK_HISI_WIFI_PMF */
#define NETLINK_DEV_ERROR 27
#endif
/*****************************************************************************
  2.10 IP宏定义
*****************************************************************************/

#define WLAN_DSCP_PRI_SHIFT         2
#define WLAN_IP_PRI_SHIFT           5
#define WLAN_IPV6_PRIORITY_MASK     0x0FF00000
#define WLAN_IPV6_PRIORITY_SHIFT    20

/*****************************************************************************
  2.11 VLAN宏定义
*****************************************************************************/


/*****************************************************************************
  2.12 LLC SNAP宏定义
*****************************************************************************/
#define LLC_UI                  0x3
#define SNAP_LLC_FRAME_LEN      8
#define SNAP_LLC_LSAP           0xaa
#define SNAP_RFC1042_ORGCODE_0  0x00
#define SNAP_RFC1042_ORGCODE_1  0x00
#define SNAP_RFC1042_ORGCODE_2  0x00
#define SNAP_BTEP_ORGCODE_0     0x00
#define SNAP_BTEP_ORGCODE_1     0x00
#define SNAP_BTEP_ORGCODE_2     0xf8

/*****************************************************************************
  2.13 ETHER宏定义
*****************************************************************************/
#define ETHER_ADDR_LEN  6   /* length of an Ethernet address */
#define ETHER_TYPE_LEN  2   /* length of the Ethernet type field */
#define ETHER_CRC_LEN   4   /* length of the Ethernet CRC */
#define ETHER_HDR_LEN   14
#define ETHER_MAX_LEN   1518
#define ETHER_MTU        (ETHER_MAX_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)

/* #ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY */
OAL_STATIC OAL_INLINE oal_uint8 a2x(const char c)
{
    if (c >= '0' && c <= '9')
    {
        return (oal_uint8)oal_atoi(&c);
    }
    if (c >= 'a' && c <= 'f')
    {
        return (oal_uint8)0xa + (oal_uint8)(c-'a');
    }
    if (c >= 'A' && c <= 'F')
    {
        return (oal_uint8)0xa + (oal_uint8)(c-'A');
    }
    return 0;
}

/*convert a string,which length is 18, to a macaddress data type.*/
#define MAC_STR_LEN 18
#define COPY_STR2MAC(mac,str)  \
    do { \
        oal_uint8 i; \
        for(i = 0; i < OAL_MAC_ADDR_LEN; i++) {\
            mac[i] = (oal_uint8)(a2x(str[i*3]) << 4) + a2x(str[i*3 + 1]);\
        }\
    } while(0)
/* #endif */

/* ip头到协议类型字段的偏移 */
#define IP_PROTOCOL_TYPE_OFFSET  9
#define IP_HDR_LEN               20

/* CCMP加密字节数 */
#define WLAN_CCMP_ENCRYP_LEN 16
/* CCMP256加密字节数 */
#define WLAN_CCMP256_GCMP_ENCRYP_LEN 24

/* is address mcast? */
#define ETHER_IS_MULTICAST(_a)   (*(_a) & 0x01)

/* is address bcast? */
#define ETHER_IS_BROADCAST(_a)   \
    ((_a)[0] == 0xff &&          \
     (_a)[1] == 0xff &&          \
     (_a)[2] == 0xff &&          \
     (_a)[3] == 0xff &&          \
     (_a)[4] == 0xff &&          \
     (_a)[5] == 0xff)

#define ETHER_IS_ALL_ZERO(_a)    \
    ((_a)[0] == 0x00 &&          \
     (_a)[1] == 0x00 &&          \
     (_a)[2] == 0x00 &&          \
     (_a)[3] == 0x00 &&          \
     (_a)[4] == 0x00 &&          \
     (_a)[5] == 0x00)

#define ETHER_IS_IPV4_MULTICAST(_a)  ((_a[0]) == 0x01 &&    \
                                      (_a[1]) == 0x00 &&    \
                                      (_a[2]) == 0x5e)

#define WLAN_DATA_VIP_TID              WLAN_TIDNO_BCAST

/* wiphy  */
#define IEEE80211_HT_MCS_MASK_LEN   10

/* ICMP codes for neighbour discovery messages */
#define OAL_NDISC_ROUTER_SOLICITATION       133
#define OAL_NDISC_ROUTER_ADVERTISEMENT      134
#define OAL_NDISC_NEIGHBOUR_SOLICITATION    135
#define OAL_NDISC_NEIGHBOUR_ADVERTISEMENT   136
#define OAL_NDISC_REDIRECT                  137

#define OAL_ND_OPT_TARGET_LL_ADDR           2
#define OAL_ND_OPT_SOURCE_LL_ADDR           1
#define OAL_IPV6_ADDR_ANY                   0x0000U
#define OAL_IPV6_ADDR_MULTICAST             0x0002U
#define OAL_IPV6_MAC_ADDR_LEN               16


#define OAL_IPV4_ADDR_SIZE                    4
#define OAL_IPV6_ADDR_SIZE                    16
#define OAL_IP_ADDR_MAX_SIZE                  OAL_IPV6_ADDR_SIZE


/* IPv4多播范围: 224.0.0.0--239.255.255.255 */
#define OAL_IPV4_IS_MULTICAST(_a)             ((oal_uint8)((_a)[0]) >= 224 && ((oal_uint8)((_a)[0]) <= 239))

/* IPv4永久组地址判断: 224.0.0.0～224.0.0.255为永久组地址 */
#define OAL_IPV4_PERMANET_GROUP_ADDR           0x000000E0
#define OAL_IPV4_IS_PERMANENT_GROUP(_a)       ((((_a) & 0x00FFFFFF) ^ OAL_IPV4_PERMANET_GROUP_ADDR) == 0)

/* IPv6组播地址: FFXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX(第一个字节全一) */
#define OAL_IPV6_IS_MULTICAST(_a)             ((oal_uint8)((_a)[0]) == 0xff)
/*ipv6 组播mac地址 */
#define ETHER_IS_IPV6_MULTICAST(_a)  (((_a)[0]) == 0x33 && ((_a)[1]) == 0x33)
/* IPv6未指定地址: ::/128 ,该地址仅用于接口还没有被分配IPv6地址时与其它节点
   通讯作为源地址,例如在重复地址检测DAD中会出现. */
#define OAL_IPV6_IS_UNSPECIFIED_ADDR(_a)   \
     ((_a)[0]  == 0x00 &&          \
      (_a)[1]  == 0x00 &&          \
      (_a)[2]  == 0x00 &&          \
      (_a)[3]  == 0x00 &&          \
      (_a)[4]  == 0x00 &&          \
      (_a)[5]  == 0x00 &&          \
      (_a)[6]  == 0x00 &&          \
      (_a)[7]  == 0x00 &&          \
      (_a)[8]  == 0x00 &&          \
      (_a)[9]  == 0x00 &&          \
      (_a)[10] == 0x00 &&          \
      (_a)[11] == 0x00 &&          \
      (_a)[12] == 0x00 &&          \
      (_a)[13] == 0x00 &&          \
      (_a)[14] == 0x00 &&          \
      (_a)[15] == 0x00)


/* IPv6链路本地地址: 最高10位值为1111111010, 例如:FE80:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX  */
#define OAL_IPV6_IS_LINK_LOCAL_ADDR(_a)       (((_a)[0] == 0xFE) && ((_a)[1] >> 6 == 2))


/* IGMP record type */
#define MAC_IGMP_QUERY_TYPE       0x11
#define MAC_IGMPV1_REPORT_TYPE    0x12
#define MAC_IGMPV2_REPORT_TYPE    0x16
#define MAC_IGMPV2_LEAVE_TYPE     0x17
#define MAC_IGMPV3_REPORT_TYPE    0x22

/* Is packet type is either leave or report */
#define IS_IGMP_REPORT_LEAVE_PACKET(type) (\
    (MAC_IGMPV1_REPORT_TYPE == type)\
    || (MAC_IGMPV2_REPORT_TYPE == type)\
    || (MAC_IGMPV2_LEAVE_TYPE  == type)\
    || (MAC_IGMPV3_REPORT_TYPE == type)\
                                         )

/* V3 group record types [grec_type] */
#define IGMPV3_MODE_IS_INCLUDE        1
#define IGMPV3_MODE_IS_EXCLUDE        2
#define IGMPV3_CHANGE_TO_INCLUDE      3
#define IGMPV3_CHANGE_TO_EXCLUDE      4
#define IGMPV3_ALLOW_NEW_SOURCES      5
#define IGMPV3_BLOCK_OLD_SOURCES      6

/* Is packet type is either leave or report */
#define IS_IGMPV3_MODE(type) (\
    (IGMPV3_MODE_IS_INCLUDE == type)\
    || (IGMPV3_MODE_IS_EXCLUDE == type)\
    || (IGMPV3_CHANGE_TO_INCLUDE  == type)\
    || (IGMPV3_CHANGE_TO_EXCLUDE == type)\
    || (IGMPV3_ALLOW_NEW_SOURCES == type)\
    || (IGMPV3_BLOCK_OLD_SOURCES == type)\
                                          )

/* IGMP record type */
#define MLD_QUERY_TYPE            130
#define MLDV1_REPORT_TYPE      131
#define MLDV1_DONE_TYPE         132
#define MLDV2_REPORT_TYPE        143

/* Is packet type is either leave or report */
#define IS_MLD_REPORT_LEAVE_PACKET(type) (\
    (MLDV1_REPORT_TYPE == type)\
    || (MLDV1_DONE_TYPE == type)\
    || (MLDV2_REPORT_TYPE  == type)\
                                         )
/* MLD V2 group record types [grec_type] */
#define MLDV2_MODE_IS_INCLUDE        1
#define MLDV2_MODE_IS_EXCLUDE        2
#define MLDV2_CHANGE_TO_INCLUDE      3
#define MLDV2_CHANGE_TO_EXCLUDE      4
#define MLDV2_ALLOW_NEW_SOURCES      5
#define MLDV2_BLOCK_OLD_SOURCES      6

#define IS_MLDV2_MODE(type) (\
    (MLDV2_MODE_IS_INCLUDE == type)\
    || (MLDV2_MODE_IS_EXCLUDE == type)\
    || (MLDV2_CHANGE_TO_INCLUDE  == type)\
    || (MLDV2_CHANGE_TO_EXCLUDE == type)\
    || (MLDV2_ALLOW_NEW_SOURCES == type)\
    || (MLDV2_BLOCK_OLD_SOURCES == type)\
                                          )

/* Calculate the group record length*/
//#define IGMPV3_GRP_REC_LEN(x) (8 + (4 * x->us_grec_nsrcs) + (4 * x->uc_grec_auxwords) )
#define IGMPV3_GRP_REC_LEN(x) (8 + (((x)->us_grec_nsrcs + (x)->uc_grec_auxwords)<<2))
//#define MLDV2_GRP_REC_LEN(x) (OAL_SIZEOF(mac_mld_v2_group_record_stru)+ (OAL_IPV6_ADDR_SIZE * (x)->us_grec_srcaddr_num) + (x)->uc_grec_auxwords )
#define MLDV2_GRP_REC_LEN(x) (OAL_SIZEOF(mac_mld_v2_group_record_stru)+ ( (x)->us_grec_srcaddr_num<<4) + (x)->uc_grec_auxwords )

/*Probe Rsp APP IE长度超过该值，发送帧netbuf采用大包*/
#define OAL_MGMT_NETBUF_APP_PROBE_RSP_IE_LEN_LIMIT  450

#ifdef _PRE_WLAN_FEATURE_SPECIAL_PKT_LOG
/* dhcp */
#define DHCP_CHADDR_LEN         16
#define SERVERNAME_LEN          64
#define BOOTFILE_LEN            128

/* DHCP message type */
#define DHCP_DISCOVER           1
#define DHCP_OFFER              2
#define DHCP_REQUEST            3
#define DHCP_ACK                5
#define DHCP_NAK                6

#define DHO_PAD                 0
#define DHO_IPADDRESS           50
#define DHO_MESSAGETYPE         53
#define DHO_SERVERID            54
#define DHO_END                 255

#define DNS_GET_QR_FROM_FLAG(flag)        ((oal_uint8)(((flag & 0x8000U) > 0)? 1 : 0))
#define DNS_GET_OPCODE_FROM_FLAG(flag)    ((oal_uint8)((flag & 0x7400U) >> 11))
#define DNS_GET_RCODE_FROM_FLAG(flag)     ((oal_uint8)(flag & 0x000fU))

#define DNS_MAX_DOMAIN_LEN  (100)

#define DHCP_SERVER_PORT    (67)
#define DHCP_CLIENT_PORT    (68)
#define DNS_SERVER_PORT     (53)
#endif

 /*****************************************************************************
   枚举名  : oal_mem_state_enum_uint8
   协议表格:
   枚举说明: 内存块状态
 *****************************************************************************/
typedef enum
{
    OAL_MEM_STATE_FREE  = 0,            /* 该内存空闲 */
    OAL_MEM_STATE_ALLOC,                /* 该内存已分配 */

    OAL_MEM_STATE_BUTT
}oal_mem_state_enum;
typedef oal_uint8 oal_mem_state_enum_uint8;

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* 以下不区分操作系统 */
/* 内核下发的扫描类型 */
typedef enum
{
    OAL_PASSIVE_SCAN        = 0,
    OAL_ACTIVE_SCAN         = 1,

    OAL_SCAN_BUTT
}oal_scan_enum;
typedef oal_uint8 oal_scan_enum_uint8;

/* 内核下发的扫描频段 */
typedef enum
{
    OAL_SCAN_2G_BAND        = 1,
    OAL_SCAN_5G_BAND        = 2,
    OAL_SCAN_ALL_BAND       = 3,

    OAL_SCAN_BAND_BUTT
}oal_scan_band_enum;
typedef oal_uint8 oal_scan_band_enum_uint8;

typedef enum
{
    OAL_IEEE80211_MLME_AUTH     = 0,    /* MLME下发认证帧相关内容 */
    OAL_IEEE80211_MLME_ASSOC    = 1,    /* MLME下发关联帧相关内容 */
    OAL_IEEE80211_MLME_REASSOC  = 2,   /* MLME下发重关联帧相关内容 */
    OAL_IEEE80211_MLME_NUM
}en_mlme_type_enum;
typedef oal_uint8 en_mlme_type_enum_uint8;

/* hostapd 下发私有命令 */
enum HWIFI_IOCTL_CMD
{
    /*
     *IOCTL_CMD的起始值由0修改为0x8EE0，修改原因：51 WiFi模块和类似于dhdutil之类的其他模块共用同一个ioctl通道，
     *而51命令的枚举值从0开始，其他模块下发的ioctl命令也包含从0开始的这部分，这样就会同时“组播”到自己的模块和WiFi模块，
     *从而对WiFi模块的功能产生影响。所以将51 WiFi模块命令的枚举值调整到0x8EE0起，便规避了其他模块命令的影响。
     */
    HWIFI_IOCTL_CMD_GET_STA_ASSOC_REQ_IE = 0x8EE0,       /* get sta assocate request ie */
    HWIFI_IOCTL_CMD_SET_AP_AUTH_ALG,            /* set auth alg to driver */
    HWIFI_IOCTL_CMD_SET_COUNTRY,                /* 设置国家码 */
    HWIFI_IOCTL_CMD_SET_SSID,                   /* 设置ssid */
    HWIFI_IOCTL_CMD_SET_MAX_USER,               /* 设置最大用户数 */
    HWIFI_IOCTL_CMD_SET_FREQ,                   /* 设置频段 */
    HWIFI_IOCTL_CMD_SET_WPS_IE,                 /* 设置AP WPS 信息元素 */
    HWIFI_IOCTL_CMD_PRIV_CONNECT,               /* linux-2.6.30 sta发起connect */
    HWIFI_IOCTL_CMD_PRIV_DISCONNECT,            /* linux-2.6.30 sta发起disconnect */
    HWIFI_IOCTL_CMD_SET_FRAG,                   /* 设置分片门限值 */
    HWIFI_IOCTL_CMD_SET_RTS,                    /* 设置RTS 门限值 */
    // _PRE_WLAN_FEATURE_HILINK
    HWIFI_IOCTL_CMD_PRIV_KICK_USER,             /* AP剔除用户 */
    HWIFI_IOCTL_CMD_SET_VENDOR_IE,              /* AP 添加私有IE接口，原为HWIFI_IOCTL_CMD_SET_OKC_IE，是hilink设置okc ie专用 */
    HWIFI_IOCTL_CMD_SET_WHITE_LST_SSIDHIDEN,    /* 设置hlink白名单 */
    HWIFI_IOCTL_CMD_FBT_SCAN,                   /* 启动或停止hilink fbt侦听*/
    HWIFI_IOCTL_CMD_GET_ALL_STA_INFO,           /* 获取所有已关联STA链路信息 */
    HWIFI_IOCTL_CMD_GET_STA_INFO_BY_MAC,        /* 获取指定已关联STA链路信息 */
    HWIFI_IOCTL_CMD_GET_CUR_CHANNEL,            /* 获取工作信道 */
    HWIFI_IOCTL_CMD_SET_SCAN_STAY_TIME,         /* 设置扫描时工作信道驻留时间和侦听信道驻留时间 */
    //_PRE_WLAN_WEB_CMD_COMM
    HWIFI_IOCTL_CMD_SET_BEACON,                 /* hostapd只配置加密参数私有接口 */
    //_PRE_WLAN_FEATURE_11R_AP
    HWIFI_IOCTL_CMD_SET_MLME,                   /* 设置MLME操作（认证、关联） */
    //_PRE_WLAN_WEB_CMD_COMM
    HWIFI_IOCTL_CMD_GET_NEIGHB_INFO,            /* 获取邻居AP扫描信息 */
    HWIFI_IOCTL_CMD_GET_HW_STAT,                /* 获取硬件流量统计 */
    HWIFI_IOCTL_CMD_GET_WME_STAT,               /* 获取WME队列统计 */
    HWIFI_IOCTL_CMD_GET_STA_11V_ABILITY,        /* 获取指定STA的11v能力信息 */
    HWIFI_IOCTL_CMD_11V_CHANGE_AP,              /* 通知sta切换到指定ap */
    HWIFI_IOCTL_CMD_GET_STA_11K_ABILITY,        /* 获取指定STA的11v能力信息 */
    HWIFI_IOCTL_CMD_SET_STA_BCN_REQUEST,        /* 通知STA的上报beacon信息 */
    HWIFI_IOCTL_CMD_GET_SNOOP_TABLE,            /* 获取组播组及其成员mac */
    HWIFI_IOCTL_CMD_GET_ALL_STA_INFO_EXT,       /* 获取所有已关联STA链路信息，包含增量部分 */
    HWIFI_IOCTL_CMD_GET_VAP_WDS_INFO,           /* 获取WDS VAP节点信息 */
    HWIFI_IOCTL_CMD_GET_STA_11H_ABILITY,        /* 获取指定STA的11h能力信息 */
    HWIFI_IOCTL_CMD_GET_STA_11R_ABILITY,        /* 获取指定STA的11r能力信息 */
    HWIFI_IOCTL_CMD_GET_TX_DELAY_AC,            /* 获取指定所有AC的发送时延信息 */
    HWIFI_IOCTL_CMD_GET_CAR_INFO,               /* 获取指定device下面的car限速配置信息 */
    HWIFI_IOCTL_CMD_GET_BLKWHTLST,              /* 获取黑白名单 */

    HWIFI_IOCTL_CMD_NUM
};

enum APP_IE_TYPE
{
    OAL_APP_BEACON_IE       = 0,
    OAL_APP_PROBE_REQ_IE    = 1,
    OAL_APP_PROBE_RSP_IE    = 2,
    OAL_APP_ASSOC_REQ_IE    = 3,
    OAL_APP_ASSOC_RSP_IE    = 4,
    OAL_APP_FT_IE           = 5,
    OAL_APP_REASSOC_REQ_IE  = 6,
#ifdef _PRE_WLAN_FEATURE_HILINK
    OAL_APP_OKC_BEACON_IE   = 7,
    OAL_APP_OKC_PROBE_IE    = 8,
#endif
    OAL_APP_VENDOR_IE,

    OAL_APP_IE_NUM
};
typedef oal_uint8 en_app_ie_type_uint8;

typedef enum _wlan_net_queue_type_
{
    WLAN_HI_QUEUE = 0,
	WLAN_NORMAL_QUEUE,

    WLAN_TCP_DATA_QUEUE,
    WLAN_TCP_ACK_QUEUE,

    WLAN_UDP_BK_QUEUE,
    WLAN_UDP_BE_QUEUE,
    WLAN_UDP_VI_QUEUE,
    WLAN_UDP_VO_QUEUE,

    WLAN_NET_QUEUE_BUTT
} wlan_net_queue_type;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/* net_device ioctl结构体定义 */
/* hostapd/wpa_supplicant 下发的信息元素结构 */
/* 该结构为事件内存池大小，保存从hostapd/wpa_supplicant下发的ie 信息 */
/* 注意: 整个结构体长度为事件内存池大小，如果事件内存池有修改，则需要同步修改app 数据结构 */
struct oal_app_ie
{
    oal_uint32              ul_ie_len;
    en_app_ie_type_uint8    en_app_ie_type;
    oal_uint8               auc_rsv[3];
    /*auc_ie 中保存信息元素，长度 = (事件内存池大小 - 保留长度) */
    oal_uint8               auc_ie[WLAN_WPS_IE_MAX_SIZE];
};
typedef struct oal_app_ie oal_app_ie_stru;

/*
wal 层到hmac侧传递使用该数据，1103 WLAN_WPS_IE_MAX_SIZE扩容到608字节，超过事件队列大小，
wal到hmac ie data采用指针传递
*/
struct oal_w2h_app_ie
{
    oal_uint32              ul_ie_len;
    en_app_ie_type_uint8    en_app_ie_type;
    oal_uint8               auc_rsv[3];
    oal_uint8              *puc_data_ie;
}__OAL_DECLARE_PACKED;
typedef struct oal_w2h_app_ie oal_w2h_app_ie_stru;

struct oal_mlme_ie
{
    en_mlme_type_enum_uint8    en_mlme_type;                    /* MLME类型*/
    oal_uint8                  uc_seq;                          /* 认证帧序列号 */
    oal_uint16                 us_reason;                       /* 原因码 */
    oal_uint8                  auc_macaddr[6];
    oal_uint16                 us_optie_len;
    oal_uint8                  auc_optie[OAL_MAX_FT_ALL_LEN];
};
typedef struct oal_mlme_ie oal_mlme_ie_stru;

/*
 * Structure of the IP frame
 */
struct oal_ip_header
{
/* liuming modifed proxyst begin */
//#if (_PRE_BIG_CPU_ENDIAN == _PRE_CPU_ENDIAN)            /* BIG_ENDIAN */
#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)            /* LITTLE_ENDIAN */
    oal_uint8    us_ihl:4,
                 uc_version_ihl:4;
#else
    oal_uint8    uc_version_ihl:4,
                 us_ihl:4;
#endif
/* liuming modifed proxyst end */

    oal_uint8    uc_tos;
    oal_uint16   us_tot_len;
    oal_uint16   us_id;
    oal_uint16   us_frag_off;
    oal_uint8    uc_ttl;
    oal_uint8    uc_protocol;
    oal_uint16   us_check;
    oal_uint32   ul_saddr;
    oal_uint32   ul_daddr;
    /*The options start here. */
}__OAL_DECLARE_PACKED;
typedef struct oal_ip_header oal_ip_header_stru;

typedef struct
{
    oal_uint16  us_sport;
    oal_uint16  us_dport;
    oal_uint32  ul_seqnum;
    oal_uint32  ul_acknum;
    oal_uint8   uc_offset;
    oal_uint8   uc_flags;
    oal_uint16  us_window;
    oal_uint16  us_check;
    oal_uint16  us_urgent;

}oal_tcp_header_stru;

typedef struct
{
    oal_uint16 source;
    oal_uint16 dest;
    oal_uint16 len;
    oal_uint16 check;
}oal_udp_header_stru;


/*
 *    Header in on cable format
 */
struct mac_igmp_header
{
    oal_uint8  uc_type;
    oal_uint8  uc_code;        /* For newer IGMP */
    oal_uint16 us_csum;
    oal_uint32 ul_group;

}__OAL_DECLARE_PACKED;
typedef struct mac_igmp_header mac_igmp_header_stru;

/*  Group record format
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Record Type  |  Aux Data Len |     Number of Sources (N)     |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Multicast Address                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Source Address [1]                      |
      +-                                                             -+
      |                       Source Address [2]                      |
      +-                                                             -+
      .                               .                               .
      .                               .                               .
      .                               .                               .
      +-                                                             -+
      |                       Source Address [N]                      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      .                                                               .
      .                         Auxiliary Data                        .
      .                                                               .
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct mac_igmp_v3_grec
{
    oal_uint8     uc_grec_type;
    oal_uint8     uc_grec_auxwords;
    oal_uint16    us_grec_nsrcs;
    oal_uint32    ul_grec_group_ip;

}__OAL_DECLARE_PACKED;
typedef struct mac_igmp_v3_grec mac_igmp_v3_grec_stru;

/* IGMPv3 report format
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Type = 0x22  |    Reserved   |           Checksum            |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |           Reserved            |  Number of Group Records (M)  |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      .                                                               .
      .                        Group Record [1]                       .
      .                                                               .
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      .                                                               .
      .                        Group Record [2]                       .
      .                                                               .
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                               .                               |
      .                               .                               .
      |                               .                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      .                                                               .
      .                        Group Record [M]                       .
      .                                                               .
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct mac_igmp_v3_report
{
    oal_uint8     uc_type;
    oal_uint8     uc_resv1;
    oal_uint16    us_csum;
    oal_uint16    us_resv2;
    oal_uint16    us_ngrec;

}__OAL_DECLARE_PACKED;
typedef struct mac_igmp_v3_report mac_igmp_v3_report_stru;


struct mac_mld_v1_head
{
    oal_uint8     uc_type;
    oal_uint8     uc_code;
    oal_uint16    us_check_sum;
    oal_uint16    us_max_response_delay;
    oal_uint16    us_reserved;
    oal_uint8     auc_group_ip[OAL_IPV6_ADDR_SIZE];

}__OAL_DECLARE_PACKED;
typedef struct mac_mld_v1_head mac_mld_v1_head_stru;

struct mac_mld_v2_report
{
    oal_uint8     uc_type;
    oal_uint8     uc_code;
    oal_uint16    us_check_sum;
    oal_uint16    us_reserved;
    oal_uint16    us_group_address_num;

}__OAL_DECLARE_PACKED;
typedef struct mac_mld_v2_report mac_mld_v2_report_stru;

struct mac_mld_v2_group_record
{
    oal_uint8     uc_grec_type;
    oal_uint8     uc_grec_auxwords;                                              //辅助数据长度
    oal_uint16    us_grec_srcaddr_num;                                           //  原地址个数
    oal_uint8    auc_group_ip[OAL_IPV6_ADDR_SIZE];                             // 组播组地址

}__OAL_DECLARE_PACKED;
typedef struct mac_mld_v2_group_record mac_mld_v2_group_record_stru;

struct mac_vlan_tag
{
    oal_uint16    us_tpid;              //tag   ID
    oal_uint16    bit_user_pri  :3,     //帧的优先级
                  bit_CFI       :1,
                  bit_vlan_id   :12;     //可配置的VLAN ID

}__OAL_DECLARE_PACKED;
typedef struct mac_vlan_tag mac_vlan_tag_stru;

/* WIN32和linux共用结构体  */
typedef struct
{
    oal_uint8 uc_type;
    oal_uint8 uc_len;
    oal_uint8 uc_addr[6];  /* hardware address */
}oal_eth_icmp6_lladdr_stru;

typedef struct
{
    oal_uint8           op;          /* packet opcode type */
    oal_uint8           htype;       /* hardware addr type */
    oal_uint8           hlen;        /* hardware addr length */
    oal_uint8           hops;        /* gateway hops */
    oal_uint32          xid;         /* transaction ID */
    oal_uint16          secs;        /* seconds since boot began */
    oal_uint16          flags;       /* flags */
    oal_uint32          ciaddr;      /* client IP address */
    oal_uint32          yiaddr;      /* 'your' IP address */
    oal_uint32          siaddr;      /* server IP address */
    oal_uint32          giaddr;      /* gateway IP address */
    oal_uint8           chaddr[16];  /* client hardware address */
    oal_uint8           sname[64];   /* server host name */
    oal_uint8           file[128];   /* boot file name */
    oal_uint8           options[4];  /* variable-length options field */
}oal_dhcp_packet_stru;

#ifdef _PRE_WLAN_FEATURE_SPECIAL_PKT_LOG
typedef struct
{
    oal_uint16          id;    /* transaction id */
    oal_uint16          flags; /* message future*/
    oal_uint16          qdcount;   /* question record count */
    oal_uint16          ancount;   /* answer record count */
    oal_uint16          nscount;   /* authority record count */
    oal_uint16          arcount;   /* additional record count*/
}oal_dns_hdr_stru;

typedef enum
{
    OAL_NS_Q_REQUEST = 0, /* request */
    OAL_NS_Q_RESPONSE = 1, /* response */
} oal_ns_qrcode;

typedef enum
{
    OAL_NS_O_QUERY = 0,     /* Standard query. */
    OAL_NS_O_IQUERY = 1,    /* Inverse query (deprecated/unsupported). */
}oal_ns_opcode;

/*
 * Currently defined response codes.
 */
typedef enum
{
    OAL_NS_R_NOERROR = 0,   /* No error occurred. */
}oal_ns_rcode;

typedef enum
{
    OAL_NS_T_INVALID = 0,   /* Cookie. */
    OAL_NS_T_A = 1,         /* Host address. */
}oal_ns_type;
#endif

/* 不分平台通用结构体 */
typedef struct
{
    oal_uint8   auc_ssid[OAL_IEEE80211_MAX_SSID_LEN];       /* ssid array */
    oal_uint8   uc_ssid_len;                                /* length of the array */
    oal_uint8   auc_arry[3];
}oal_ssids_stru;

struct hostap_sta_link_info {
    oal_uint8  addr[OAL_MAC_ADDR_LEN];
    oal_uint8  rx_rssi;     /* 0 ~ 100 ,0xff为无效值*/
    oal_uint8  tx_pwr;      /* 0 ~ 100 */
    oal_uint32 rx_rate;     /* avr nego rate in kpbs */
    oal_uint32 tx_rate;     /* avr nego rate in kpbs */
    oal_uint32 rx_minrate;  /* min nego rx rate in last duration, clean to 0 when app read over */
    oal_uint32 rx_maxrate;  /* max nego rx rate in last duration, clean to 0 when app read over */
    oal_uint32 tx_minrate;  /* min nego tx rate in last duration, clean to 0 when app read over */
    oal_uint32 tx_maxrate;  /* max nego tx rate in last duration, clean to 0 when app read over */
    oal_uint64 rx_bytes;
    oal_uint64 tx_bytes;
    oal_uint32 tx_frames_rty;  /* tx frame retry cnt */
    oal_uint32 tx_frames_all;  /* tx total frame cnt */
    oal_uint32 tx_frames_fail; /* tx fail */
    oal_uint32 SNR;            /* snr */
};

struct hostap_all_sta_link_info{
    unsigned long   buf_cnt;        /* input: sta_info count provided  */
    unsigned long   sta_cnt;        /* outpu: how many sta really */
    unsigned int    cur_channel;
    struct hostap_sta_link_info  *sta_info; /* output */
};
typedef struct hostap_sta_link_info oal_net_sta_link_info_stru;
typedef struct hostap_all_sta_link_info oal_net_all_sta_link_info_stru;

/* sta包含增量信息结构体 */
struct hostap_sta_link_info_ext{
    oal_uint8                       uc_auth_st;             /* 认证状态 */
    oal_bool_enum_uint8             en_band;                /* 工作频段 */
    oal_bool_enum_uint8             en_wmm_switch;          /* wmm是否使能 */
    oal_uint8                       uc_ps_st;               /* 节能状态 */
    oal_uint8                       uc_sta_num;             /* 空间流数 */
    oal_uint8                       uc_work_mode;           /* 协议模式 */
    oal_int8                        c_noise;                /* 节点的噪声值 */
    oal_uint8                       auc_resv[1];
    oal_uint32                      ul_associated_time;     /* 用户已连接的时长 */
};

struct hostap_all_sta_link_info_ext{
    unsigned long   buf_cnt;        /* input: sta_info count provided  */
    unsigned long   sta_cnt;        /* outpu: how many sta really */
    unsigned int    cur_channel;
    struct hostap_sta_link_info     *sta_info;
    struct hostap_sta_link_info_ext *sta_info_ext;
};
typedef struct hostap_sta_link_info_ext oal_net_sta_link_info_ext_stru;
typedef struct hostap_all_sta_link_info_ext oal_net_all_sta_link_info_ext_stru;


/*lint -e958*//* 屏蔽字节对齐错误 */
/* RRM ENABLED CAP信息元素结构体 */
struct oal_rrm_enabled_cap_ie
{
    oal_uint8   bit_link_cap            : 1,  /* bit0: Link Measurement capability enabled */
                bit_neighbor_rpt_cap    : 1,  /* bit1: Neighbor Report capability enabled */
                bit_parallel_cap        : 1,  /* bit2: Parallel Measurements capability enabled */
                bit_repeat_cap          : 1,  /* bit3: Repeated Measurements capability enabled */
                bit_bcn_passive_cap     : 1,  /* bit4: Beacon Passive Measurements capability enabled */
                bit_bcn_active_cap      : 1,  /* bit5: Beacon Active Measurements capability enabled */
                bit_bcn_table_cap       : 1,  /* bit6: Beacon Table Measurements capability enabled */
                bit_bcn_meas_rpt_cond_cap: 1; /* bit7: Beacon Measurement Reporting Conditions capability enabled */
    oal_uint8   bit_frame_cap           : 1,  /* bit8: Frame Measurement capability enabled */
                bit_chn_load_cap        : 1,  /* bit9: Channel Load Measurement capability enabled */
                bit_noise_histogram_cap : 1,  /* bit10: Noise Histogram Measurement capability enabled */
                bit_stat_cap            : 1,  /* bit11: Statistics Measurement capability enabled */
                bit_lci_cap             : 1,  /* bit12: LCI Measurement capability enabled */
                bit_lci_azimuth_cap     : 1,  /* bit13: LCI Azimuth capability enabled */
                bit_tsc_cap             : 1,  /* bit14: Transmit Stream/Category Measurement capability enabled */
                bit_triggered_tsc_cap   : 1;  /* bit15: Triggered  Transmit Stream/Category Measurement capability enabled*/
    oal_uint8   bit_ap_chn_rpt_cap                  : 1, /* bit16: AP Channel Report capability enabled */
                bit_rm_mib_cap                      : 1, /* bit17: RM MIB capability enabled */
                bit_oper_chn_max_meas_duration      : 3, /* bit18-20: Operating Channel Max Measurement Duration */
                bit_non_oper_chn_max_meas_duration  : 3; /* bit21-23: Non-operating Channel Max Measurement Durationg */
    oal_uint8   bit_meas_pilot_cap              : 3, /* bit24-26: Measurement Pilot capability */
                bit_meas_pilot_trans_info_cap   : 1, /* bit27: Measurement Pilot Transmission Information capability enabled */
                bit_neighbor_rpt_tsf_offset_cap : 1, /* bit28: Neighbor Report TSF Offset capability enabled */
                bit_rcpi_cap                    : 1, /* bit29: RCPI Measurement capability enabled */
                bit_rsni_cap                    : 1, /* bit30: RSNI Measurement capability enabled */
                bit_bss_avg_access_dly          : 1; /* bit31: BSS Average Access Delay capability enabled */
    oal_uint8   bit_avail_admission_capacity_cap: 1, /* bit32: BSS Available Admission Capacity capability enabled */
                bit_antenna_cap                 : 1, /* bit33: Antenna capability enabled */
                bit_ftm_range_report_cap        : 1, /* bit34: FTM range report capability enabled */
                bit_rsv                         : 5; /* bit35-39: Reserved */
}__OAL_DECLARE_PACKED;
typedef struct oal_rrm_enabled_cap_ie oal_rrm_enabled_cap_ie_stru;
/*lint +e958*/


/* 邻居AP列表的BSS描述信息结构体 */
struct oal_bssid_infomation
{
    oal_uint8       bit_ap_reachability:2,                                      /* AP的可到达性 */
                    bit_security:1,                                             /* 该AP的加密规则与当前连接是否一致 */
                    bit_key_scope:1,                                            /* 该AP的认证信息是否与当前上报一直 */
                    bit_spectrum_mgmt:1,                                        /* 能力位: 支持频谱管理 */    /* 能力位字段与beacon定义一致 */
                    bit_qos:1,                                                  /* 能力位: 支持QOS */
                    bit_apsd:1,                                                 /* 能力位: 支持APSD */
                    bit_radio_meas:1;                                           /* 能力位: 波长测量 */
    oal_uint8       bit_delay_block_ack:1,                                      /* 能力位: 阻塞延迟应答 */
                    bit_immediate_block_ack:1,                                  /* 能力位: 阻塞立即应答 */
                    bit_mobility_domain:1,                                      /* 该AP的beacon帧中是否含有MDE，且与此次上报一致 */
                    bit_high_throughput:1,                                      /* 该AP的beacon帧中是否含有高吞吐量元素，且与此次上报一致 */
                    bit_resv1:4;                                                /* 预留 */
    oal_uint8       bit_resv2;
    oal_uint8       bit_resv3;
}__OAL_DECLARE_PACKED;
typedef struct oal_bssid_infomation  oal_bssid_infomation_stru;

#ifdef _PRE_WLAN_FEATURE_HILINK
typedef enum list_action
{
    OAL_WHITE_LIST_OPERATE_DEL = 0,
    OAL_WHITE_LIST_OPERATE_ADD = 1,

    OAL_WHITE_LIST_OPERATE_BUTT
}white_list_operate;
typedef oal_uint8 oal_en_white_list_operate_uint8;

typedef struct oal_hilink_white_node
{
    oal_uint8                       auc_mac[OAL_MAC_ADDR_LEN];         /*目标MAC*/
    oal_en_white_list_operate_uint8 en_white_list_operate;
#ifndef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    oal_uint8                       uc_reserve;
#else
    oal_uint8                       uc_flag;   /*区分STA在白名单or已关联的STA列表*/
#endif
}oal_hilink_white_node_stru;

typedef struct
{
    oal_uint8       auc_mac[OAL_MAC_ADDR_LEN];                 /* 需要侦听的sta的mac地址 */
    oal_uint8       en_is_on;                                  /* 是否开启侦听 */
    oal_uint8       uc_reserve;
    oal_uint32      ul_channel;                                /* 需要侦听的信道号 */
    oal_uint32      ul_interval;                               /* 需要侦听时间 */
}oal_hilink_scan_params;

typedef struct
{
    oal_uint32        ul_cur_channel;                            /* 当前工作信道 */
    oal_uint32        ul_cur_freq;                               /* 当前工作频点 */
}oal_hilink_current_channel;

typedef struct
{
    oal_uint16        us_work_channel_stay_time;                  /* 工作信道驻留时长 */
    oal_uint16        us_scan_channel_stay_time;                  /* 侦听信道驻留时长 */
}oal_hilink_set_scan_time_param;

typedef struct
{
    oal_uint8           auc_sta_mac[OAL_MAC_ADDR_LEN];            /* 要获取的sta的mac */
    oal_bool_enum_uint8 en_support_11v;                           /* 保存sta是否支持11v，为出参 */
    oal_uint8           reserve;
}oal_hilink_get_sta_11v_ability;

typedef struct
{
    oal_uint8       auc_sta_mac_addr[OAL_MAC_ADDR_LEN];            /* 要切换的sta MAC地址 */
    oal_uint8       auc_target_ap_mac_addr[OAL_MAC_ADDR_LEN];      /* 目标ap MAC地址 */
    oal_uint8       uc_opt_class;                                  /* Operation Class */
    oal_uint8       uc_chl_num;                                    /* Channel number */
    oal_uint8       uc_phy_type;                                   /* PHY type */
    oal_uint8       uc_candidate_perf;                             /* perference data BSSID偏好值 */
    oal_bssid_infomation_stru  st_target_ap_info;                  /* 目标ap信息 */
}oal_hilink_change_sta_to_target_ap;

typedef struct
{
    oal_uint8                   auc_sta_mac[OAL_MAC_ADDR_LEN];     /* 要获取的sta的mac */
    oal_bool_enum_uint8         en_support_11k;                    /* 保存sta是否支持11k，为出参 */
    oal_uint8                   reserve;
    oal_rrm_enabled_cap_ie_stru st_sta_11k_ability;                /* sta 11k 能力信息 */
}oal_hilink_get_sta_11k_ability;

typedef  struct
{
    oal_uint8                   auc_sta_mac[OAL_MAC_ADDR_LEN];     /* 要获取的sta的mac */
    oal_uint8                   auc_target_ap_bssid[OAL_MAC_ADDR_LEN];
    oal_uint8                   uc_channel_num;
    oal_uint8                   auc_reserve[3];
}oal_hilink_neighbor_bcn_req;
#endif

#if defined(_PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
/************************************************************************/
/* err code mask of wifi                                                */
/************************************************************************/
#define WIFI_EVENT_START            (0x1000)

/*
 * WiFi event.
 * @ IEEE80211_EV_HI_DETECTED_STA sta在调用add_detect_sta接口后，上报侦听到的STA的RSSI信息
 * @ IEEE80211_EV_DRV_EXCEPTION wifi diag事件
 * @ IEEE80211_EV_AUTH_REJECT sta在auth阶段被add_reject_sta接口下发的规则拒连，上报该事件
 */
typedef enum ieee80211_event_en {
    IEEE80211_EV_HI_DETECTED_STA = WIFI_EVENT_START,
    IEEE80211_EV_DRV_EXCEPTION,
    IEEE80211_EV_AUTH_REJECT,
} IEEE80211_DRIVER_EVENT_EN;

/* ioctl的命令字枚举 */
enum ieee80211_vendor_req_sub_cmd_en
{
    IEEE80211_VENDOR_ADD_VENDOR_IE  = 0x1000,
    IEEE80211_VENDOR_MGMT_FILTER,
    IEEE80211_VENDOR_ADD_DETECT,
    IEEE80211_VENDOR_REMOVE_DETECT,
    IEEE80211_VENDOR_ADD_ACL_EXCEPT,
    IEEE80211_VENDOR_REMOVE_ACL_EXCEPT,
    IEEE80211_VENDOR_GET_VAP_STATUS,
    IEEE80211_VENDOR_GET_STA_STATUS,
    IEEE80211_VENDOR_GET_ONE_STA_STATUS,
    IEEE80211_VENDOR_ADD_REJECT_STA,
    IEEE80211_VENDOR_REMOVE_REJECT_STA,
    IEEE80211_VENDOR_SEND_RAW,
    IEEE80211_VENDOR_DISASSOC_STA,
    IEEE80211_VENDOR_GET_REG_DOMAIN,
    IEEE80211_VENDOR_SET_IE,
    IEEE80211_VENDOR_SET_CAP_INFO,
    IEEE80211_VENDOR_ADD_SENSING_MAC,
    IEEE80211_VENDOR_REMOVE_SENSING_MAC,
    IEEE80211_VENDOR_GET_SENSING_AP_RSSI,
    IEEE80211_VENDOR_SCAN2,
    IEEE80211_VENDOR_DUALBAND_COMMON,
    IEEE80211_VENDOR_DUALBAND_SILENT,
    IEEE80211_VENDOR_DUALBAND_DBGLVL,
    IEEE80211_VENDOR_DUALBAND_CONNSTAT_SYNC,
    IEEE80211_VENDOR_DUALBAND_PROTECT_CTL,
    IEEE80211_VENDOR_DUALBAND_STA_DEBUG,
    IEEE80211_VENDOR_DUALBAND_ONLINE,
    IEEE80211_VENDOR_DUALBAND_OFFLINE,
    IEEE80211_VENDOR_DUALBAND_OKC,
    IEEE80211_VENDOR_DUALBAND_STA,
    IEEE80211_VENDOR_SET_RISK_STA,
    IEEE80211_VENDOR_GET_LINK_INFO,
    IEEE80211_VENDOR_GET_USB_STATUS,
    IEEE80211_VENDOR_SET_UNRISK_STA,
    IEEE80211_VENDOR_SET_LST,
    IEEE80211_VENDOR_ADD_PROB_RATIO,
    IEEE80211_VENDOR_REMOVE_PROB_RATIO,
    IEEE80211_VENDOR_MAX,
};

typedef enum
{
    IEEE80211_FRAME_TYPE_PROBEREQ =   (1 << 0),
    IEEE80211_FRAME_TYPE_BEACON =   (1 << 1),
    IEEE80211_FRAME_TYPE_PROBERESP =   (1 << 2),
    IEEE80211_FRAME_TYPE_ASSOCREQ =   (1 << 3),
    IEEE80211_FRAME_TYPE_ASSOCRESP =   (1 << 4),
    IEEE80211_FRAME_TYPE_AUTH =   (1 << 5),
    IEEE80211_FRAME_TYPE_AP_OKC = (1 << 6),
    IEEE80211_FRAME_TYPE_STA_OKC = (1 << 7),
} oal_ieee80211_frame_type;
typedef oal_uint8 oal_ieee80211_frame_type_uint8;

#endif

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE

/* IEEE802.11 - Region domain code for hera */
typedef enum reg_domain{
    OAL_WIFI_REG_DOMAIN_FCC  = 1,
    OAL_WIFI_REG_DOMAIN_IC  = 2,
    OAL_WIFI_REG_DOMAIN_ETSI  = 3,
    OAL_WIFI_REG_DOMAIN_SPAIN = 4,
    OAL_WIFI_REG_DOMAIN_FRANCE = 5,
    OAL_WIFI_REG_DOMAIN_MKK  = 6,
    OAL_WIFI_REG_DOMAIN_ISRAEL = 7,
    OAL_WIFI_REG_DOMAIN_MKK1  = 8,
    OAL_WIFI_REG_DOMAIN_MKK2  = 9,
    OAL_WIFI_REG_DOMAIN_MKK3  = 10,
    OAL_WIFI_REG_DOMAIN_NCC  = 11,
    OAL_WIFI_REG_DOMAIN_RUSSIAN = 12,
    OAL_WIFI_REG_DOMAIN_CN  = 13,
    OAL_WIFI_REG_DOMAIN_GLOBAL = 14,
    OAL_WIFI_REG_DOMAIN_WORLD_WIDE = 15,
    OAL_WIFI_REG_DOMAIN_TEST  = 16,
    OAL_WIFI_REG_DOMAIN_5M10M = 17,
    OAL_WIFI_REG_DOMAIN_SG  = 18,
    OAL_WIFI_REG_DOMAIN_KR  = 19,
    OAL_WIFI_REG_DOMAIN_MAX
} oal_hera_reg_domain;
typedef oal_uint8 oal_en_hera_reg_domain_uint8;

/* IE和cap info的设置类型
   0表示：如果存在则更新已有，其中content部分与已有IE进行与运算
   1表示：如果存在则更新已有，其中content部分与已有IE进行或运算，如果没有，则添加到管理帧最后
   2表示：将content中的内容构造成IE，添加在管理帧最后
   3表示：忽略content中内容，按驱动默认配置添加对应的IE，主要是Power Constraint IE/Country IE */
typedef enum
{
    OAL_IE_SET_TYPE_AND     = 0,
    OAL_IE_SET_TYPE_OR      = 1,
    OAL_IE_SET_TYPE_ADD     = 2,
    OAL_IE_SET_TYPE_IGNORE  = 3,
    OAL_IE_SET_TYPE_BUTT
}oal_ie_set_type_enum;
typedef oal_uint8   oal_ie_set_type_enum_uint8;

typedef struct ieee80211req_send_raw
{
  oal_uint8  auc_mac_addr[OAL_MAC_ADDR_LEN];     /* STA MAC address */
  oal_uint16 us_len;                             /* Action frame length */
  oal_uint8  *puc_msg;                           /* Pointer to action frame to be sent */
} oal_ieee80211req_send_raw_stru;

typedef struct ieee80211req_get_domain
{
  /* Region domain code */
  oal_hera_reg_domain en_domain_code;
} oal_ieee80211req_get_domain_stru;

typedef struct ieee80211req_set_ie
{
  /* IE ID code */
  oal_uint32 ul_elment_id;
  /* IE set type,
  0表示：如果存在则更新已有，其中content部分与已有IE进行与运算
  1表示：如果存在则更新已有，其中content部分与已有IE进行或运算，如果没有，则添加到管理帧最后
  2表示：将content中的内容构造成IE，添加在管理帧最后
  3表示：忽略content中内容，按驱动默认配置添加对应的IE，主要是Power Constraint IE/Country IE */
  oal_uint32 ul_type;
  oal_ieee80211_frame_type  en_frame_type;
  oal_uint8  *puc_content;                /* IE content */
  oal_uint32 ul_len;                      /* IE content length */
} oal_ieee80211req_set_ie_stru;

typedef struct
{
  /* IE set type,
  0表示：content部分与已有capcibilities information进行与运算
  1表示：content部分与已有capcibilities information进行或运算 */
  oal_uint32 ul_type;
  /* capbility infor content */
  oal_uint16 us_capbility;
}oal_ieee80211req_set_cap_stru;

#endif  // _PRE_WLAN_FEATURE_11KV_INTERFACE

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
enum en_ieee80211_mgmt_frame_filters_enum
{
    IEEE80211_FILTER_TYPE_ASSOC_REQ   =   (1 << 0),
    IEEE80211_FILTER_TYPE_ASSOC_RESP  =   (1 << 1),
    IEEE80211_FILTER_TYPE_REASSOC_REQ =   (1 << 2),
    IEEE80211_FILTER_TYPE_REASSOC_RSP =   (1 << 3),
    IEEE80211_FILTER_TYPE_PROBE_REQ   =   (1 << 4),
    IEEE80211_FILTER_TYPE_PROBE_RESP  =   (1 << 5),
    IEEE80211_FILTER_TYPE_BEACON      =   (1 << 8),
    IEEE80211_FILTER_TYPE_DISASSOC    =   (1 << 10),
    IEEE80211_FILTER_TYPE_AUTH        =   (1 << 11),
    IEEE80211_FILTER_TYPE_DEAUTH      =   (1 << 12),
    IEEE80211_FILTER_TYPE_ACTION      =   (1 << 13),
    IEEE80211_FILTER_TYPE_ALL         =   0xFFFF,
};

typedef enum
{
    IEEE80211_MLME_PHASE_PROB  =  0x1,
    IEEE80211_MLME_PHASE_AUTH  =  0x2,
    IEEE80211_MLME_PHASE_ASSOC =  0x4,
} iee80211_mlme_phase;

typedef struct wifi_reject_auth_event_st {
    oal_uint8 auc_bssid[ETH_ALEN];
    oal_uint8 auc_sta[ETH_ALEN];
    oal_int32 l_rssi;
    oal_uint32 ul_type;
    oal_uint32 ul_params[1];
} oal_wifi_reject_auth_event_stru;

/**
 * struct vap_status_diag_info - 用于链路度量和可维可测
 *
 * @bssid: 当前AP的BSSID.
 */
typedef struct oal_vap_status_diag_info
{
    /* baisc setting */
    oal_uint8 auc_bssid[ETH_ALEN];          //BSSID
    oal_uint8 uc_cur_channel;              //当前信道
    oal_uint8 uc_offset_second_channel;    //备选信道

    oal_uint8 uc_bandwidth;                //20M:20、40M:40、20/40M:60, 最好使用标准（或者驱动）的约定
    /* dynamic config parameters */
    oal_uint8 uc_dig;                       /*  reltek芯片寄存器，默认写0 */
    oal_uint8 uc_cca_shreshold;             /*  reltek芯片寄存器，默认写0 */
    oal_uint8 uc_channel_usage;                 //占空比：40%对应值40  zhoukedou band steering
    oal_uint32 ul_hisi_dig;                     //接收门限 跟mcs相关 3.11
    oal_uint8 auc_hisi_cca_shreshold[2];        //待确认含义   3.10.1

    oal_uint8 uc_sta_cnt;                  //终端数量
    oal_uint8 uc_scan_mode;                //在扫描：1，未扫描：0  //mac_device_stru->en_curr_scan_state

    oal_uint16 us_fa;                      /* invalid frame - crc err .etc*/  //1秒内无效帧计数，需要驱动统计  hal_get_rx_err_count
    oal_uint16 us_cca;                     /* valid frame, can be recongnized*/ //1秒内有效帧计数,需要驱动统计  //pst_hal_device->st_hal_device_base.ul_rx_normal_mdpu_succ_num++;
    oal_uint32 ul_channel_busy;            /** 信道繁忙度:(信道测量时间 - 信道发送时间 - 信道空闲时间 - 节点自身接收时间)*1000 / 信道测量时间 */

    /* agg setting */
    oal_uint32 ul_wait_agg_time;           //聚合等待时间（ms)
    /* buffer use status */
    oal_uint32 ul_skb_remain_buffer;       //skb剩余buff
    oal_uint32 ul_vo_remain_count;         //vo队列剩余buff
    oal_uint32 ul_vi_remain_count;         //vi队列剩余buff
    oal_uint32 ul_be_remain_count;         //be队列剩余buff
    oal_uint32 ul_bk_remain_count;         //bk队列剩余buff
    /* edca setting  */
    oal_uint32 ul_vo_edca;                 //1151的寄存器设置值
    oal_uint32 ul_vi_edca;                 //hi1151_vap_get_edca_machw_cw  3.9.1
    oal_uint32 ul_be_edca;
    oal_uint32 ul_bk_edca;
} oal_vap_status_diag_info_stru;

typedef struct oal_sta_status_diag_info{
    oal_uint8 auc_mac[ETH_ALEN];           /* sta的mac地址: */
    oal_uint8 uc_db_mode;                  /* Mandatory: dualband mode: 2.4G only, 5G only, 2.4G and 5G dualband */
    oal_uint8 auc_chip_vendor[3];          /* 芯片厂家代码: */
    oal_uint8 uc_wl_mode;                  /* 无线协议模式: b,g,n,bg,bgn, hmac_user_stru.st_user_base_info.en_protocol_mode */
    oal_uint8 uc_agg_mode;                 /* 聚合模式: 每bit表示1个tid，有聚合:1，无聚合:0, dmac_user_stru.st_tid.pst_ba_tx_hdl->en_ba_conn_status */

    oal_uint8 uc_rssi;                     /* 接收信号强度指示: */
    oal_uint8 uc_snr;                      /* 信噪比: 暂无计算公式，默认写0 */
    oal_uint8 uc_tx_bandwidth;             /* 发送带宽: hmac_user_stru.st_user_base_info.en_cur_bandwidth */
    oal_uint8 uc_rx_bandwidth;             /* 接收带宽: hmac_user_stru.st_user_base_info.en_cur_bandwidth */
    oal_uint16 us_tx_rate;                 /* 协商发送速率(MByte/s): hmac_user_stru.ul_tx_rate */
    oal_uint16 us_rx_rate;                 /* 协商接收速率(MByte/s): hmac_user_stru.ul_rx_rate */
    oal_uint16 us_tx_minrate;              /* Mandatory: 每次获取间隔内的最小协商速率(应用层每秒获取sta的协商速率)MByte/s */
    oal_uint16 us_tx_maxrate;              /* Mandatory: 每次获取间隔内的最大协商速率(应用层每秒获取sta的协商速率)MByte/s */
    oal_uint32 ul_sleep_times;             /* 累计休眠次数: dmac_user_stru.ul_sta_sleep_times */
    oal_uint32 ul_tx_retry_cnt;            /* 芯片重发次数: oam_user_stat_info_stru.ul_tx_ppdu_retries, 未实现 */
    oal_uint8 uc_per;                      /* 误帧率(百分比值)：40%为40 */
    oal_uint8 uc_max_agg_num;              /* 最大聚合个数: 基于TID 简单可以取TID0, dmac_tid_stru.dmac_ba_tx_stru.uc_ampdu_max_num  */
    oal_uint8 uc_rts_rate;                 /* 芯片RTS帧速率(MB/s): hal_set_rts_rate_params */
    oal_uint8 uc_rts_retry_cnt;            /* 芯片RTS帧重发次数:  dmac_tx_update_alg_param */

    oal_uint32 ul_tx_pkts;                 /* 发送报文数: oam_user_stat_info_stru */
    oal_uint32 ul_tx_fail;                 /* 发送失败报文数: oam_user_stat_info_stru */
    oal_uint32 ul_rx_pkts;                 /* 接收报文数: oam_user_stat_info_stru.ul_rx_mpdu_num */
    oal_uint32 ul_tx_unicast_bytes;        /* 发送单播报文字节数: dmac_thrpt_stat_info_stru, becareful invert */
    oal_uint32 ul_rx_unicast_bytes;        /* 接收单播报文字节数: dmac_thrpt_stat_info_stru, */
    oal_uint32 ul_tx_mcast_bytes;          /* 发送多播报文字节数: 已有组播转单播特性 */
    oal_uint32 ul_rx_mcast_bytes;          /* 接收多播报文字节数: 已有组播转单播特性 */
    oal_uint32 aul_sta_tx_mcs_cnt[16];     /* how many tx mcs count  两次读取间隔内的每个MCS统计值,读清0 */
    oal_uint32 aul_sta_rx_mcs_cnt[16];     /* how many rx mcs count 两次读取间隔内的每个MCS统计值,读清0  */
    oal_uint32 ul_tx_spending;             /* Mandatory: 链路度量: 链路协议开销 */
    oal_uint32 ul_tx_th;                   /* Mandatory: STA的1秒内tx吞吐值 */
    oal_uint32 ul_tx_spending_smoothing_rate;  /* 未用到，待删除 */
    oal_uint32 ul_current_tx_rate;         /* RTL特有的，代表tx链路速率的一个索引，后续会把此参数移回驱动，待删除*/
    oal_uint32 ul_tx_prev_rate;            /* Mandatory: STA的前1秒内tx吞吐值糜谇昂罅酱蝦ate差别较大的时候做平滑*/
} oal_sta_status_diag_info_stru;

typedef struct ieee80211req_wifi_rssi {
    oal_uint32   ul_timestamp;          /* 时间戳 */
    oal_int32    l_rssi;                /* rssi值  */
    oal_uint8    auc_local[ETH_ALEN];       /* AP的MAC地址 */
    oal_uint8    auc_bssid[ETH_ALEN];       /* 待收集RSSI AP的MAC地址 */
} ieee80211req_wifi_rssi_stru;

typedef struct ieee80211req_getset_buf {
    oal_uint32   ul_buf_len;           /* input: buffer len */
    oal_uint8   *puc_buf;               /* output */
} oal_ieee80211req_getset_buf_stru;

typedef struct sta_status_diag_req
{
    oal_uint32   ul_date_len;                 /* input: how many sta get */
    oal_uint32   ul_sta_cnt;                  /* output: how many sta get */
    oal_uint32   ul_channel;                  /* output: channel */
    oal_sta_status_diag_info_stru *pst_data;  /* output */
} oal_sta_status_diag_req_stru;

typedef struct ieee80211req_getset_appiebuf
{
    oal_uint32 ul_app_frmtype;     /*management frame type for which buffer is added*/
    oal_uint32 ul_app_buflen;      /*application supplied buffer length */
    oal_uint8 *puc_app_buf;            /*application ie data */
} oal_ieee80211req_getset_appiebuf_stru;

typedef struct ieee80211req_set_filter
{
    oal_uint32 ul_app_filterype;
} oal_ieee80211req_set_filter_stru;

#endif  // _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

#if defined(_PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)

typedef struct ieee80211req_action_sta
{
    /* the mac of station to be detect */
    oal_uint8  auc_mac_addr[OAL_MAC_ADDR_LEN];
    oal_uint16 us_pad;
    union
    {
        /* the channel on which to detect */
        oal_uint16 us_channel;
        oal_uint8  uc_flag;
        oal_uint16 us_reason_code;
        oal_uint8  uc_mlme_phase_mask;                 /*iee80211_mlme_phase phase;*/
        oal_sta_status_diag_info_stru *pst_sta_status;
    } param;
} oal_ieee80211req_action_sta_stru;

typedef enum ieee80211_dualband_mode {
    IEEE80211_MODE_2G_ONLY    = 0,
    IEEE80211_MODE_5G_ONLY    = 1,
    IEEE80211_MODE_DUALBAND   = 2,
} IEEE80211_DUALBAND_MODE_EN;

typedef struct dualband_para_set_to_ioctl
{
    oal_uint32 abpa_enable;
    oal_uint32 abpa_rssi_mid_threshold;
    oal_uint32 abpa_acc_intv_threshold;
    oal_uint32 abpa_silent_interval;
    oal_uint32 abpa_silent_ratio;
    oal_uint32 abpa_dbglvl;
    oal_uint32 abpa_sync_state;
    oal_uint32 abpa_protect_enable;
    oal_uint8  abpa_sync_sta[ETH_ALEN];
    oal_uint8  abpa_protect_sta[ETH_ALEN];
    oal_uint32 channel;
    oal_uint32 print_sta_type;
    oal_uint32 okc_state;
    oal_uint8 sta[ETH_ALEN];            /* 用于查询的双频网卡地址 */
    oal_uint8 sta_mode;                 /* 双频网卡查询结果 */
} oal_dualband_para_set_to_stru;

typedef struct ieee80211_vendor_req
{
    oal_uint16 us_cmd;    /* vendor command */
    oal_uint16 us_len;    /* buffer len */
    union
    {
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        oal_sta_status_diag_req_stru *pst_sta_status_req;
        oal_vap_status_diag_info_stru *pst_vap_status;
        oal_ieee80211req_getset_appiebuf_stru *pst_app_ie;
        oal_ieee80211req_set_filter_stru st_filter;
        oal_ieee80211req_getset_buf_stru st_getset_buf;
#endif
        oal_ieee80211req_action_sta_stru st_action_sta;     /* 两个接口共用结构体 */
#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
        oal_ieee80211req_send_raw_stru *pst_raw_msg;
        oal_ieee80211req_get_domain_stru st_domain;
        oal_ieee80211req_set_ie_stru *pst_ie;
        oal_ieee80211req_set_cap_stru st_cap_info;
#endif
        oal_dualband_para_set_to_stru *dualband_para;
    } param;
} oal_ieee80211_vendor_req_stru;

#endif  // defined(_PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)

typedef struct
{
    oal_uint8           auc_sta_mac[OAL_MAC_ADDR_LEN];            /* 要获取的sta的mac */
    oal_bool_enum_uint8 en_support_11h;                           /* 保存sta是否支持11h，为出参 */
    oal_uint8           reserve;
}oal_hilink_get_sta_11h_ability;

typedef struct
{
    oal_uint8           auc_sta_mac[OAL_MAC_ADDR_LEN];            /* 要获取的sta的mac */
    oal_bool_enum_uint8 en_support_11r;                           /* 保存sta是否支持11r，为出参 */
    oal_uint8           reserve;
}oal_hilink_get_sta_11r_ability;

#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
#define OAL_WIFI_LED_OPTIMIZED_FLAG         52

typedef enum
{
    HW_KER_LED_WLAN_OFF = 0, /* wifi down */
    HW_KER_LED_WLAN_ON, /* wifi up, and no data */
    HW_KER_LED_WLAN_BLINK_FAST, /* wifi up, and have data  */
    HW_KER_LED_WLAN_BLINK_SLOW, /* wifi up, station connected */
    HW_KER_LED_WLAN_BUTT
} HW_KER_LED_WLAN_STAT_E;

typedef enum
{
    HW_KER_WIFI_CHIP_BAND_2G,
    HW_KER_WIFI_CHIP_BAND_5G,
    HW_KER_WIFI_CHIP_BAND_BUTT
} HW_KER_WIFI_CHIP_BAND_E;

typedef enum
{
    HW_KER_WIFI_LED_WLAN = 0,        /* wlan灯 */
    HW_KER_WIFI_LED_WPS,             /* wps灯 */
    HW_KER_WIFI_LED_TYPE_BUTT
} HW_KER_WIFI_LED_TYPE_E;
extern unsigned int (*g_pf_wifi_led_set)(unsigned int, unsigned int, unsigned int, unsigned int);
#endif
#ifdef _PRE_WLAN_PRODUCT_1151V200
typedef enum
{
    OAL_WIFI_DOWN = 0,
    OAL_WIFI_UP,
    OAL_WIFI_TX,
    OAL_WIFI_RX,
    OAL_WIFI_STATE_BUTT
}oal_wifi_state_enum;
typedef oal_uint8 oal_wifi_state_enum_uint8;

typedef enum
{
    OAL_WIFI_BAND_2G = 0,
    OAL_WIFI_BAND_5G,
    OAL_WIFI_BAND_BUTT
}oal_wifi_band_enum;
typedef oal_uint8 oal_wifi_band_enum_uint8;
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
typedef enum
{
    OAL_WME_AC_BE = 0,    /* best effort */
    OAL_WME_AC_BK = 1,    /* background */
    OAL_WME_AC_VI = 2,    /* video */
    OAL_WME_AC_VO = 3,    /* voice */

    OAL_WME_AC_BUTT = 4,
    OAL_WME_AC_MGMT = OAL_WME_AC_BUTT,   /* 管理AC，协议没有,对应硬件高优先级队列*/

    OAL_WME_AC_PSM = 5    /* 节能AC, 协议没有，对应硬件组播队列 */
}oal_wme_ac_type_enum;

struct wlan_ap_settings {
    enum nl80211_auth_type auth_type;
    oal_uint8   auc_arry[3];
    const oal_uint8 *ssid;
    size_t  ssid_len;
    struct cfg80211_beacon_data beacon;
    struct cfg80211_crypto_settings crypto;
};

typedef struct
{
    enum nl80211_auth_type              auth_type;
    oal_uint8                           uc_dtim_period;                     /* dtime周期 */
    oal_uint16                          us_beacon_period;                   /* beacon周期 */
    oal_uint32                          privacy;
    oal_int8                            c_rssi;                             /* bss的信号强度 */
    oal_uint16                          us_noise;
    oal_uint8                           en_protocol;    /* 协议 */
    oal_uint8                           network_type;
    oal_uint8                           auc_bssid[OAL_MAC_ADDR_LEN];       /* 网络bssid */
    oal_uint8                           uc_num_supp_rates;                  /* 支持的速率集个数 */
    oal_uint8                           auc_supp_rates[12];/* 支持的速率集 */
    oal_uint8                           uc_channel;
    oal_uint8                           uc_bw;
    oal_uint8                           uc_max_nss;                         /* 该BSS支持的最大空间流数 */
    oal_uint8                           auc_resv[1];
    oal_uint8                           auc_ssid[OAL_IEEE80211_MAX_SSID_LEN];
    size_t                              ssid_len;
    struct cfg80211_crypto_settings     crypto;
    oal_uint32                          ul_max_rate_kbps;                   /* 该BSS支持的最大速率(单位:kbps) */
}oal_ap_info_stru;

typedef struct
{
    oal_uint8           uc_cnt; /*buffer cnt from the user space*/
    oal_uint8           uc_resv[3];
    oal_ap_info_stru   *pst_buf;
}oal_ap_scan_result_stru;

typedef struct
{
    oal_uint32          ul_rx_bytes;
    oal_uint32          ul_rx_pkts;
    oal_uint32          ul_rx_err;
    oal_uint32          ul_rx_discard;

    oal_uint32          ul_tx_bytes;
    oal_uint32          ul_tx_pkts;
    oal_uint32          ul_tx_err;
    oal_uint32          ul_tx_discard;
    oal_uint32          ul_tx_bcn_cnt;
}oal_machw_flow_stat_stru;

typedef struct
{
    oal_uint32              ul_tx_succ_num[OAL_WME_AC_BUTT];                /*发送成功个数*/
    oal_uint32              ul_rx_mpdu_num[OAL_WME_AC_BUTT];                    /*接收的MPDU个数*/
    oal_uint32              ul_tx_fail_num[OAL_WME_AC_BUTT];                /*发送次数超出限制的失败个数*/
    oal_uint32              ul_rx_fail_num[OAL_WME_AC_BUTT];
    oal_uint32              ul_tx_succ_bytes[OAL_WME_AC_BUTT];                  /*tx succ字节*/
    oal_uint32              ul_rx_succ_bytes[OAL_WME_AC_BUTT];                  /*rx succ字节*/
    oal_uint32              ul_tx_fail_bytes[OAL_WME_AC_BUTT];                  /*tx fail字节*/
    oal_uint32              ul_rx_fail_bytes[OAL_WME_AC_BUTT];                  /*rx fail字节*/
    oal_uint32              ul_forward_num[OAL_WME_AC_BUTT];
    oal_uint32              ul_forward_bytes[OAL_WME_AC_BUTT];                  /*forward字节*/
    oal_uint32              ul_tx_expire[OAL_WME_AC_BUTT];
}oal_wme_stat_stru;

typedef struct
{
    oal_uint32                      ul_max_tx_delay[OAL_WME_AC_BUTT];                    /*最大发送延时*/
    oal_uint32                      ul_min_tx_delay[OAL_WME_AC_BUTT];                    /*最小发送延时*/
    oal_uint32                      ul_ave_tx_delay[OAL_WME_AC_BUTT];                    /*平均发送延时*/
}oal_tx_delay_ac_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_WDS
typedef struct
{
    oal_uint8                           auc_mac[OAL_MAC_ADDR_LEN];
    oal_uint8                           uc_stas_num;
    oal_uint8                           auc_resv[1];
}oal_wds_node_stru;

typedef struct
{
    oal_uint32                          ul_last_pkt_age;
    oal_wds_node_stru                   st_related_node;
    oal_uint8                           auc_mac[OAL_MAC_ADDR_LEN];
    oal_uint8                           auc_resv[2];
}oal_wds_stas_stru;

typedef struct
{
    oal_uint32                          ul_last_pkt_age;
    oal_uint8                           auc_mac[OAL_MAC_ADDR_LEN];
    oal_uint8                           auc_resv[2];
}oal_wds_neigh_stru;

typedef struct
{
    oal_wds_node_stru                  *pst_peer_node;
    oal_wds_stas_stru                  *pst_wds_stas;
    oal_wds_neigh_stru                 *pst_neigh;
    oal_uint32                          ul_wds_aging;
    oal_uint8                           uc_wds_vap_mode;
    oal_uint8                           uc_wds_node_num;
    oal_uint16                          uc_wds_stas_num;
    oal_uint16                          uc_neigh_num;
    oal_uint8                           auc_resv[2];
}oal_wds_info_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_CAR
typedef enum
{
    OAL_CAR_UPLINK = 0,       /* 上行 */
    OAL_CAR_DOWNLINK = 1,     /* 下行 */

    OAL_CAR_BUTT
}oal_car_up_down_type_enum;

typedef struct
{
    oal_bool_enum_uint8                 en_car_limit_flag;                      /* 该device/vap/user是否限速 */
    oal_uint8                           auc_resv[3];
    oal_uint32                          ul_car_limit_kbps;                      /* 该device/vap/user限制带宽大小 */
}oal_car_limit_stru;

typedef struct
{
    oal_car_limit_stru                  st_car_device_ucast_cfg[OAL_CAR_BUTT];  /* device 单播限速结构体,0-上行 1-下行 */
    oal_car_limit_stru                  st_car_device_mcast_cfg;                /* 组播限速，device 下行 */
    oal_uint32                          ul_car_orgin_mcast_pps_num;             /* 每个device有一个pps的原始令牌桶，不可消耗*/
}oal_car_device_limit_stru;


typedef struct
{
    oal_uint8                           uc_vap_id;
    oal_uint8                           auc_resv[3];
    oal_car_limit_stru                  ast_vap_car_cfg[OAL_CAR_BUTT];
}oal_car_vap_limit_stru;

typedef struct
{
    oal_uint16                          us_assoc_id;
    oal_uint8                           auc_user_mac_addr[OAL_MAC_ADDR_LEN];    /* user对应的MAC地址 */
    oal_car_limit_stru                  ast_user_car_cfg[OAL_CAR_BUTT];
    oal_uint8                           uc_vap_id;                              /* user对应的vapid */
    oal_uint8                           auc_resv[3];
}oal_car_user_limit_stru;

typedef struct
{
    oal_uint8                           uc_ori_vap_id;          /* 上面传下来的vap对应驱动的vapid,上层根据该vapid进行vap和user过滤；比如vap3 对应驱动vapid为5 */
    oal_uint8                           auc_resv[3];
    oal_uint8                           uc_device_id;
    oal_uint16                          us_car_ctl_cycle_ms;    /* 限速控制定时器周期, 100ms*/
    oal_bool_enum_uint8                 en_car_enable_flag;     /* car使能标志 */
    oal_uint32                          aul_car_packet_drop_num[OAL_CAR_BUTT];
    oal_uint32                          ul_car_mcast_packet_drop_num;
    oal_uint32                          ul_car_mcast_packet_pps_drop_num;

    oal_car_device_limit_stru           st_car_device_cfg;
    oal_car_vap_limit_stru              ast_car_vap_cfg[4];         // ont每个device最大支持4个vap
    oal_uint16                          us_user_buf_max;            /* 上层分配的user信息个数  */
    oal_uint16                          us_user_cnt;                /* 上报的user信息个数 */
    oal_car_user_limit_stru            *pst_car_user_cfg;           /* 指向user信息内存块，每个device最大128用户 */

}oal_car_info_stru;
#endif

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
typedef struct
{
    oal_uint8       uc_blkwhtlst_cnt;                                       /* 黑白名单个数 */
    oal_uint8       uc_mode;                                                /* 黑白名单模式 */
    oal_uint8       auc_resv[2];
    oal_uint8       auc_blkwhtlst_mac[MAX_BLACKLIST_NUM][OAL_MAC_ADDR_LEN];
}oal_blkwhtlst_stru;
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
typedef struct
{
    oal_uint8 auc_group_mac[OAL_MAC_ADDR_LEN];
    oal_uint8 uc_sta_num;
    oal_uint8 uc_reserve;
    oal_uint8 auc_sta_mac[MAX_STA_NUM_OF_ONE_GROUP][OAL_MAC_ADDR_LEN];
}oal_snoop_group_stru;

typedef struct
{
    oal_uint16              us_group_cnt;
    oal_uint8               auc_resv[3];
    oal_snoop_group_stru    *pst_buf;
}oal_snoop_all_group_stru;
#endif

/* net_device ioctl结构体定义 */
typedef struct oal_net_dev_ioctl_data_tag
{
    oal_int32 l_cmd;                                  /* 命令号 */
    union
    {
        struct
        {
            oal_uint8    auc_mac[OAL_MAC_ADDR_LEN];
            oal_uint8    auc_rsv[2];
            oal_uint32   ul_buf_size;            /* 用户空间ie 缓冲大小 */
            oal_uint8   *puc_buf;                /* 用户空间ie 缓冲地址 */
        }assoc_req_ie;                           /* AP 模式，用于获取STA 关联请求ie 信息 */

        struct
        {
            oal_uint32    auth_alg;
        }auth_params;

        struct
        {
            oal_uint8    auc_country_code[4];
        }country_code;

		oal_uint8     ssid[OAL_IEEE80211_MAX_SSID_LEN+4];
        oal_uint32    ul_vap_max_user;

        struct
         {
             oal_int32    l_freq;
             oal_int32    l_channel;
             oal_int32    l_ht_enabled;
             oal_int32    l_sec_channel_offset;
             oal_int32    l_vht_enabled;
             oal_int32    l_center_freq1;
             oal_int32    l_center_freq2;
             oal_int32    l_bandwidth;
         }freq;

         oal_app_ie_stru  st_app_ie;          /* beacon ie,index 0; proberesp ie index 1; assocresp ie index 2 */

        struct
        {
            oal_int32                           l_freq;              /* ap所在频段，与linux-2.6.34内核中定义不同 */
            oal_uint32                          ssid_len;            /* SSID 长度 */
            oal_uint32                          ie_len;

            oal_uint8                          *puc_ie;
            OAL_CONST oal_uint8                *puc_ssid;               /* 期望关联的AP SSID  */
            OAL_CONST oal_uint8                *puc_bssid;              /* 期望关联的AP BSSID  */

            oal_uint8                           en_privacy;             /* 是否加密标志 */
            oal_nl80211_auth_type_enum_uint8    en_auth_type;           /* 认证类型，OPEN or SHARE-KEY */

            oal_uint8                           uc_wep_key_len;         /* WEP KEY长度 */
            oal_uint8                           uc_wep_key_index;       /* WEP KEY索引 */
            OAL_CONST oal_uint8                *puc_wep_key;            /* WEP KEY密钥 */

            oal_cfg80211_crypto_settings_stru   st_crypto;              /* 密钥套件信息 */
        }cfg80211_connect_params;
        struct
        {
            oal_uint8            auc_mac[OAL_MAC_ADDR_LEN];
            oal_uint16           us_reason_code;                        /* 去关联 reason code */
        }kick_user_params;

        oal_net_all_sta_link_info_stru all_sta_link_info;

#ifdef _PRE_WLAN_FEATURE_HILINK
        struct
        {
            oal_uint8               auc_mac[OAL_MAC_ADDR_LEN];
            oal_uint16              us_reason_code;                        /* 去关联 reason code */
            oal_uint8               uc_rej_user;                           /* 禁止sta连接的时间 */
            oal_uint8               uc_kick_user;                          /* 类型:允许/禁止连接 */
            oal_uint8               auc_rsv[2];
        }fbt_kick_user_params;

        oal_hilink_white_node_stru          set_white_lst_ssidhiden_params;
        oal_hilink_scan_params              fbt_scan_params;
        oal_hilink_current_channel          fbt_get_cur_channel;
        oal_hilink_set_scan_time_param      fbt_set_scan_stay_time;
        oal_hilink_get_sta_11v_ability      fbt_get_sta_11v_ability;
        oal_hilink_change_sta_to_target_ap  fbt_change_sta_to_target_ap;
        oal_hilink_get_sta_11k_ability      fbt_get_sta_11k_ability;
        oal_hilink_neighbor_bcn_req         fbt_11k_sta_neighbor_bcn_req;

#endif
        oal_hilink_get_sta_11h_ability      fbt_get_sta_11h_ability;
#ifdef _PRE_WLAN_FEATURE_11R_AP
        oal_hilink_get_sta_11r_ability      fbt_get_sta_11r_ability;
        struct
        {
            en_mlme_type_enum_uint8    en_mlme_type;                    /* MLME类型*/
            oal_uint8                  uc_seq;                          /* 认证帧序列号 */
            oal_uint16                 us_reason;                       /* 原因码 */
            oal_uint8                  auc_macaddr[6];
            oal_uint16                 us_optie_len;
            oal_uint8                  auc_optie[OAL_MAX_FT_ALL_LEN];
        }set_mlme;

#endif

        oal_int32                l_frag;                                /* 分片门限值 */
        oal_int32                l_rts;                                 /* RTS 门限值 */

#ifdef _PRE_WLAN_WEB_CMD_COMM
        struct wlan_ap_settings         st_cfg_ap;                             /*ap parameter*/
        oal_ap_scan_result_stru         st_ap_info;
        oal_machw_flow_stat_stru        st_machw_stat;
        oal_wme_stat_stru               st_wme_stat;
        oal_tx_delay_ac_stru            st_tx_delay_ac;
#endif
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
        oal_snoop_all_group_stru        st_all_snoop_group;
#endif
#ifdef _PRE_WLAN_FEATURE_WDS
        oal_wds_info_stru               st_wds_info;
#endif
        oal_net_all_sta_link_info_ext_stru  all_sta_link_info_ext;

#ifdef _PRE_WLAN_FEATURE_CAR
        oal_car_info_stru               st_car_info;
#endif
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
        oal_blkwhtlst_stru              st_blkwhtlst;
#endif


    }pri_data;
}oal_net_dev_ioctl_data_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_bool_enum_uint8 oal_netbuf_is_dhcp_port_etc(oal_udp_header_stru *pst_udp_hdr);
extern oal_bool_enum_uint8 oal_netbuf_is_nd_etc(oal_ipv6hdr_stru  *pst_ipv6hdr);
extern oal_bool_enum_uint8 oal_netbuf_is_dhcp6_etc(oal_ipv6hdr_stru  *pst_ether_hdr);

#ifdef _PRE_WLAN_FEATURE_FLOWCTL
extern oal_void  oal_netbuf_get_txtid(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tos);
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
extern oal_bool_enum_uint8 oal_netbuf_is_tcp_ack6_etc(oal_ipv6hdr_stru  *pst_ipv6hdr);
extern oal_uint16 oal_netbuf_select_queue_etc(oal_netbuf_stru *pst_buf);
#endif
extern oal_bool_enum_uint8 oal_netbuf_is_tcp_ack_etc(oal_ip_header_stru  *pst_ip_hdr);
extern oal_bool_enum_uint8 oal_netbuf_is_icmp_etc(oal_ip_header_stru  *pst_ip_hdr);

//extern oal_int genKernel_init(oal_void);
//extern oal_void genKernel_exit(oal_void);
extern oal_int32 dev_netlink_send_etc (oal_uint8 *data, oal_int data_len);
extern oal_int32 init_dev_excp_handler_etc(oal_void);
extern oal_void deinit_dev_excp_handler_etc(oal_void);
extern oal_int genl_msg_send_to_user(oal_void *data, oal_int i_len);
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT || _PRE_TARGET_PRODUCT_TYPE_CPE == _PRE_CONFIG_TARGET_PRODUCT)
extern int wlan_check_arp_spoofing(struct net_device *port_dev, struct sk_buff *pskb);
#endif
#ifdef _PRE_WLAN_PRODUCT_1151V200

OAL_STATIC OAL_INLINE oal_void oal_notify_wlan_status(oal_wifi_band_enum_uint8 uc_band, oal_wifi_state_enum_uint8 uc_stat)
{
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
    oal_uint i_band = HW_KER_WIFI_CHIP_BAND_BUTT;

    if(OAL_WIFI_BAND_2G == uc_band)
    {
        i_band = HW_KER_WIFI_CHIP_BAND_2G;
    }
    else if (OAL_WIFI_BAND_5G == uc_band)
    {
        i_band = HW_KER_WIFI_CHIP_BAND_5G;
    }
    else
    {
    }

    switch (uc_stat)
    {
        case OAL_WIFI_DOWN:
            g_pf_wifi_led_set(i_band, HW_KER_WIFI_LED_WLAN, HW_KER_LED_WLAN_OFF, OAL_WIFI_LED_OPTIMIZED_FLAG);
            break;
        case OAL_WIFI_UP:
            g_pf_wifi_led_set(i_band, HW_KER_WIFI_LED_WLAN, HW_KER_LED_WLAN_ON, OAL_WIFI_LED_OPTIMIZED_FLAG);
            break;
        case OAL_WIFI_TX:
        case OAL_WIFI_RX:
            g_pf_wifi_led_set(i_band, HW_KER_WIFI_LED_WLAN, HW_KER_LED_WLAN_BLINK_FAST, OAL_WIFI_LED_OPTIMIZED_FLAG);
            break;
        default:
            break;
    }
#endif
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_net.h */
