

#ifndef __HMAC_M2U_H__
#define __HMAC_M2U_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_net.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "mac_frame.h"
#include "hmac_main.h"
#include "hmac_user.h"
#include "hmac_vap.h"
#include "frw_ext_if.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_M2U_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_M2U_GRPADDR_FILTEROUT_NUM 8
#define HMAC_M2U_DENY_GROUP 4026531585UL     /* 组播组黑名单 239.255.255.1 */
#define HMAC_M2U_MIN_DENY_GROUP 3758096384UL /* 最小组播组地址 224.0.0.0 */
#define HMAC_M2U_MAX_DENY_GROUP 4026531839UL /* 最大组播组地址 239.255.255.255 */
#define HMAC_M2U_SPECIAL_GROUP1 3758096385UL    /* SPECIAL  GROUP1 224.0.0.1 */
#define HMAC_M2U_SPECIAL_GROUP2 3758096386UL    /* SPECIAL  GROUP2 224.0.0.2 */
#define HMAC_M2U_RIPV2_GROUP 3758096393UL       /* RIPV2  GROUP 224.0.0.9 */
#define HMAC_M2U_SPECIAL_GROUP3 3758096406UL    /* SPECIAL  GROUP3 224.0.0.22 */
#define HMAC_M2U_UPNP_GROUP 4026531834UL        /* UPNP GROUP 239.255.255.250 */

#define DEFAULT_IPV4_DENY_GROUP_COUNT 1      /* 默认额外添加1个ipv4组播黑名单*/
#define DEFAULT_IPV6_DENY_GROUP_COUNT 1      /* 默认添加1个ipv6组播黑名单*/
#define SPECIAL_M2U_GROUP_COUNT_IPV4  5      /* 默认添加5个特殊ipv4业务组播*/
#define SPECIAL_M2U_GROUP_COUNT_IPV6  5      /* 默认添加5个特殊ipv6业务组播*/
#define HMAC_M2U_ADAPTIVE_STA_HASHSIZE 16    /* 配网报文统计HASH桶为16*/


#define MAC_ETH_PROTOCOL_SUBTYPE  0x17
#define OAL_SNAP_LEN              8    /* SNAP 头的长度 */
#define MIN_IP_HDR_LEN            5    /* 最小IP头长度 */

#define HMAC_DEF_M2U_TIMER        30000     /* timer interval as 30 secs */
#define HMAC_DEF_M2U_TIMEOUT      120000    /* 2 minutes for timeout  */

#define HMAC_DEF_ADAPTIVE_TIMEOUT      1000      /* 配网模式老化时间  */
#define HMAC_DEF_THRESHOLD_TIME        500       /* 配网模式门限时间  */
#define HMAC_DEF_NUM_OF_ADAPTIVE       16        /* 配网模式门限个数  */

#define ETHER_TYPE_VLAN_88A8      0x88a8  /* VLAN TAG TPID ，有运营商的情况 */
#define ETHER_TYPE_VLAN_9100      0x9100  /* VLAN TAG TPID */
#define MAX_STA_NUM_OF_ALL_GROUP  1000    /*最多1000个叶子节点(sta个数)*/
#define MAX_STA_NUM_OF_ADAPTIVE   128     /*最多128个配网sta个数 */

#define oal_set_ip_addr(addr1, addr2) oal_memcopy((addr1), (addr2), 16UL)

#define ETHER_IS_WITH_VLAN_TAG(_a) \
        (((_a) == OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN_88A8)) ||    \
         ((_a) == OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN_9100)) ||    \
         ((_a) == OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN)))

#define OAL_IS_MDNSV4_MAC(_a,_b)   (((oal_uint8)((_a)[0]) == 0x01) && ((oal_uint8)((_a)[1]) == 0x00) && ((oal_uint8)((_a)[2]) == 0x5e) && \
                                   ((oal_uint8)((_a)[3]) == 0x00) && ((oal_uint8)((_a)[4]) == 0x00) && ((oal_uint8)((_a)[5]) == 0xfb) && \
                                   ((_b) == OAL_HOST2NET_SHORT(ETHER_TYPE_IP)))
#define OAL_IS_MDNSV6_MAC(_a,_b)   (((oal_uint8)((_a)[0]) == 0x33) && ((oal_uint8)((_a)[1]) == 0x33) && ((oal_uint8)((_a)[2]) == 0x00) && \
                                   ((oal_uint8)((_a)[3]) == 0x00) && ((oal_uint8)((_a)[4]) == 0x00) && ((oal_uint8)((_a)[5]) == 0xfb) && \
                                   ((_b) == OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6)))
#define OAL_IS_ICMPV6_PROTO(_a,_b) (((_a) == OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6)) && (((oal_uint8)((_b)[6]) == 0x3a) || ((oal_uint8)((_b)[40]) == 0x3a)))
#define OAL_IS_IGMP_PROTO(_a,_b)   (((_a) == OAL_HOST2NET_SHORT(ETHER_TYPE_IP)) && ((oal_uint8)((_b)[9]) == 0x02))

#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
/* 配网报文哈希函数定义 */
#define HMAC_ADAPTIVE_CAL_HASH_VALUE(_puc_mac_addr)     \
    ((_puc_mac_addr)[ETHER_ADDR_LEN - 1] & (HMAC_M2U_ADAPTIVE_STA_HASHSIZE - 1))
#endif



#if 0
#define BITFIELD_LITTLE_ENDIAN      0
#define BITFIELD_BIG_ENDIAN         1
#endif
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    HMAC_M2U_CMD_EXCLUDE_LIST  = 0,
    HMAC_M2U_CMD_INCLUDE_LIST  = 1,

    HMAC_M2U_IGMP_CMD_BUTT
}hmac_m2u_update_cmd_enum;
typedef oal_uint8 hmac_m2u_igmp_cmd_enum_uint8;

typedef enum
{
    HMAC_M2U_MCAST_MAITAIN         = 0,
    HMAC_M2U_MCAST_TUNNEL          = 1,
    HMAC_M2U_MCAST_TRANSLATE       = 2,

    HMAC_M2U_MCAST_BUTT
}hmac_m2u_mcast_mode_enum;
typedef oal_uint8 hmac_m2u_mcast_mode_enum_uint8;

typedef struct
{
    #if (OAL_BYTE_ORDER == OAL_LITTLE_ENDIAN)
	oal_uint32  offset	                : 11,
	            seqNum       		    : 11,
	            optHdrLen32  		    : 2,
	            frameType    		    : 2,
	            proto        		    : 6;
    #else /* big endian */
	oal_uint32  proto	                : 6,
	            frameType    		    : 2,
	            optHdrLen32  		    : 2,
	            seqNum       		    : 11,
	            offset       		    : 11;
	#endif
}mcast_tunnel_hdr_stru;

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

/* hmac_m2u_grp_list_entry通过各组的不同地址来存储组，挂到snoop_list下 */
////组播组结构体
typedef struct
{
    oal_dlist_head_stru               st_src_list;                        /* 组播组内成员的链表头 */
    oal_uint8                         auc_group_mac[WLAN_MAC_ADDR_LEN];   /* 这个组播组的组mac地址 */
    oal_uint8                         uc_sta_num;                         /*记录当前组播组下sta个数*/
    oal_uint8                         uc_reserve;
    oal_dlist_head_stru               st_grp_entry;
    mac_vlan_tag_stru                 st_outer_vlan_tag;                  /*  外层vlan tag*/
    mac_vlan_tag_stru                 st_inner_vlan_tag;                  /*  内层vlan tag*/
}hmac_m2u_grp_list_entry_stru;

/* 管理组播组的snoop链表结构 */
typedef struct
{
    oal_uint32               aul_deny_group[HMAC_M2U_GRPADDR_FILTEROUT_NUM];
    oal_uint8                aul_deny_group_ipv6[HMAC_M2U_GRPADDR_FILTEROUT_NUM][OAL_IPV6_ADDR_SIZE];
    oal_dlist_head_stru      st_grp_list;            /* 组链表头 */
    oal_uint16               us_group_list_count;    /*组播组个数*/
    oal_uint16               us_misc;
    oal_uint16               us_total_sta_num;       /*记录当前各组播组下sta总数*/
    oal_uint8                uc_deny_count_ipv4;     /* ipv4黑名单个数*/
    oal_uint8                uc_deny_count_ipv6;     /* ipv6黑名单个数*/
    oal_uint32               aul_special_group_ipv4[SPECIAL_M2U_GROUP_COUNT_IPV4]; /* ipv4特殊组播IP*/
    oal_uint8                aul_special_group_ipv6[SPECIAL_M2U_GROUP_COUNT_IPV6][OAL_IPV6_ADDR_SIZE]; /* ipv6特殊组播IP*/
}hmac_m2u_snoop_list_stru;

/**
 * hmac_m2u_grp_member_stru 用来存储一个组成员的详细信息
 * 每一个成员拥有
 * ul_src_ip_addr - 源ip地址
 * grp_member_address - 报告报文发送者的地址
 * mode - include / exclude src_ip_address.
 **/
  ////组内成员站点信息 (STA )
typedef struct
{
    oal_uint8                        auc_src_ip_addr[OAL_IPV6_ADDR_SIZE];    //组播源IP地址
    oal_uint32                       ul_timestamp;
    hmac_user_stru                  *pst_hmac_user;
    oal_uint8                        auc_grp_member_mac[WLAN_MAC_ADDR_LEN]; //该STA MAC 地址
    oal_uint8                        en_mode;
    oal_uint8                        uc_src_ip_addr_len;                    //ip 地址长度，用于兼容IPV4、IPV6
    oal_dlist_head_stru              st_member_entry;
}hmac_m2u_grp_member_stru;



/* hmac_m2u_list_update_stru 结构用来传递参数给list update函数来完成特定group的成员更新 */
typedef struct
{
    oal_uint8                        auc_src_ip_addr[OAL_IPV6_ADDR_SIZE];   /* 源地址 */
    oal_uint8                        auc_grp_mac[WLAN_MAC_ADDR_LEN];        /* 需要加入的组播组mac地址 */
    oal_uint8                        auc_new_member_mac[WLAN_MAC_ADDR_LEN]; /* 需要进行更新的组播成员mac地址 */
    oal_uint32                       ul_timestamp;                          /* 时间戳 */
    hmac_vap_stru                   *pst_hmac_vap;                          /* vap指针 */
    hmac_user_stru                  *pst_hmac_user;                         /* user指针 */
    mac_vlan_tag_stru                st_outer_vlan_tag;                     /* 外层vlan tag*/
    mac_vlan_tag_stru                st_inner_vlan_tag;                     /* 内层vlan tag*/
    hmac_m2u_igmp_cmd_enum_uint8     en_cmd;                                /* 加入、删除命令 */
    oal_uint8                        uc_src_ip_addr_len;
    oal_uint8                        auc_reserve[2];
}hmac_m2u_list_update_stru;

#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
/* 管理配网模式STA链表结构 */
typedef struct
{
    oal_dlist_head_stru               st_adaptive_entry;
    oal_uint8                         auc_adaptive_mac[WLAN_MAC_ADDR_LEN];   /* 配网STA的mac地址 */
    oal_uint8                         uc_adaptive_num;                       /* 记录当前收到配网组播包的个数*/
    oal_bool_enum_uint8               en_m2u_adaptive;                       /* 配网模式判据 */
    oal_uint32                        ul_timestamp;
    mac_vlan_tag_stru                 st_outer_vlan_tag;                     /* 外层vlan tag */
    mac_vlan_tag_stru                 st_inner_vlan_tag;                     /* 内层vlan tag */
}hmac_m2u_adaptive_hash_stru;

typedef struct
{
    oal_uint8                        auc_new_member_mac[WLAN_MAC_ADDR_LEN]; /* 需要进行更新的配网设备mac地址 */
    oal_uint8                        auc_reserve[2];
    oal_uint32                       ul_timestamp;                          /* 时间戳 */
    hmac_vap_stru                   *pst_hmac_vap;                          /* vap指针 */
    mac_vlan_tag_stru                st_outer_vlan_tag;                     /* 外层vlan tag*/
    mac_vlan_tag_stru                st_inner_vlan_tag;                     /* 内层vlan tag*/
}hmac_m2u_adaptive_list_update_stru;
#endif

/*管理整个snoop链表*/
typedef struct
{
    hmac_m2u_snoop_list_stru        st_m2u_snooplist;
    oal_bool_enum_uint8             en_snoop_enable;    /* 控制组播转单播是否使能 */
    hmac_m2u_mcast_mode_enum_uint8  en_mcast_mode;      /* 控制组播帧的发送方式 */
    oal_bool_enum_uint8             en_discard_mcast;   /* 控制组播帧是否直接丢弃 */
    wlan_tidno_enum_uint8           en_tid_num;
    frw_timeout_stru                st_snooplist_timer;
    oal_uint32                      ul_timeout;         /*组播组成员沉默时间*/
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
    oal_dlist_head_stru             ast_m2u_adaptive_hash[HMAC_M2U_ADAPTIVE_STA_HASHSIZE];/* 配网配网报文统计HASH表 */
    oal_bool_enum_uint8             en_frequency_enable;/* 控制异频组播帧转发开关使能 */
    oal_bool_enum_uint8             en_adaptive_enable; /* 控制配网模式识别使能 */
    frw_timeout_stru                st_adaptivelist_timer;
    oal_uint32                      ul_threshold_time;   /* 配网模式门限时间 */
    oal_uint32                      ul_adaptive_timeout; /* 配网模式老化时间*/
    oal_uint8                       uc_adaptive_num;     /* 配网模式门限个数*/
    oal_uint8                       uc_adaptive_sta_count; /* 配网STA个数 */
    oal_uint8                       auc_reserve[2];
#endif
}hmac_m2u_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_m2u_snoop_convert( hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf);
extern oal_void hmac_m2u_snoop_inspecting(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_buf);
extern oal_void hmac_m2u_attach(hmac_vap_stru *pst_hmac_vap);
extern oal_void hmac_m2u_detach(hmac_vap_stru * pst_hmac_vap);
extern oal_uint32 hmac_m2u_update_snoop_list(hmac_m2u_list_update_stru *pst_list_entry);
extern oal_void hmac_m2u_add_snoop_ipv4_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint32 *ul_grpaddr);
extern oal_void hmac_m2u_add_snoop_ipv6_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint8 *ul_grpaddr);
extern oal_void hmac_m2u_del_ipv4_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint32 *pul_grpaddr);
extern oal_void hmac_m2u_del_ipv6_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint8 *ul_grpaddr);
extern oal_void hmac_m2u_clear_deny_table(hmac_vap_stru *pst_hmac_vap);
extern oal_void hmac_m2u_show_snoop_deny_table(hmac_vap_stru *pst_hmac_vap);
extern oal_uint32 hmac_m2u_print_all_snoop_list(hmac_vap_stru *pst_hmac_vap, oal_snoop_all_group_stru *pst_snoop_all_grp);
extern oal_void hmac_m2u_cleanup_snoopwds_node(hmac_user_stru *pst_hmac_user);
extern oal_uint32 hmac_m2u_igmp_v1v2_update(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry, mac_igmp_header_stru *pst_igmp);
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
extern oal_uint32 hmac_m2u_multicast_drop(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf);
extern oal_void hmac_m2u_adaptive_inspecting(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_buf);
extern oal_void hmac_m2u_unicast_convert_multicast( hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf, dmac_msdu_stru *pst_msdu);
#endif


#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_m2u.h */
