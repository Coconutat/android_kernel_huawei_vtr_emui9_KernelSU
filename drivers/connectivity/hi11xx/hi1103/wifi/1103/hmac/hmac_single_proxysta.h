

#ifndef __HMAC_SINGLE_PROXYSTA_H__
#define __HMAC_SINGLE_PROXYSTA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SINGLE_PROXYSTA_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_PROXYSTA_MAP_IPV4_HASHSIZE        MAC_VAP_PROXYSTA_MAP_MAX_VALUE
#define HMAC_PROXYSTA_MAP_IPV6_HASHSIZE        MAC_VAP_PROXYSTA_MAP_MAX_VALUE
#define HMAC_PROXYSTA_MAP_UNKNOW_HASHSIZE        MAC_VAP_PROXYSTA_MAP_UNKNOW_VALUE

#define HMAC_PROXYSTA_MAP_AGING_TIME            120000          /* PROXYST MAP表格老化时间 120s */
#define HMAC_PROXYSTA_MAP_AGING_TRIGGER_TIME    60000           /* MAP老化计时器触发时间 60S */
#define HMAC_PROXYSTA_DHCP_BODY_FIX_LEN         236             /* DHCP报文固定字段长度 */
#define HMAC_PROXYSTA_DHCP_CLIENT_REQUEST       1               /* DHCP客户端请求报文 */
#define HMAC_PROXYSTA_DHCP_SERVER_RESPONSE      0               /* DHCP服务器响应报文 */
#define HMAC_PROXYSTA_IPV6_ADDR_LEN             16              /* IPV6地址长度 */
#define HMAC_PROXYSTA_ICMPV6_HEADER_VALUE       0x3a            /* ICMPV6的nexthead值 */

#define HMAC_PROXYSTA_MEMCMP_EQUAL              0               /* 两个指针内容比较:相等 */
#define HMAC_PROXYSTA_MAP_MAX_NUM               128             /* MAP表格大小，当前限制为128 */

#define HMAC_ETHERTYPE_PROTOCOL_START          0x0600           /* 以太网协议帧开始值 */

#define DHCP_PORT_BOOTPS   0x0043  /* DHCP 服务端接收报文端口,即客户端发送报文的目的端口 */
#define DHCP_PORT_BOOTPC   0x0044  /* DHCP 客户端接收报文端口,即服务器发送报文的目的端口*/

#define DHCP_FLAG_BCAST    0x8000       /* 请求DHCP服务器以广播形式回复DHCP response */

#define PROTOCOL_ICMPV6    0x3a         /* IPV6报文中的ICMPV6报文协议 */
#define PROTOCOL_ICMPV6_ROUTER_AD_OFFLOAD    8   /* 邻居通告报文option字段与ICMPV6头的偏移长度 */

/* 哈希函数定义 */
#define HMAC_PROXYSTA_CAL_IPV4_HASH(_puc_ip_addr)     \
    ((_puc_ip_addr)[ETH_TARGET_IP_ADDR_LEN - 1] & (HMAC_PROXYSTA_MAP_IPV4_HASHSIZE - 1))

#define HMAC_PROXYSTA_CAL_IPV6_HASH(_puc_ip_addr)     \
    ((_puc_ip_addr)[ETH_TARGET_IP_ADDR_LEN - 1] & (HMAC_PROXYSTA_MAP_IPV6_HASHSIZE - 1))

#define HMAC_PROXYSTA_CAL_UNKNOW_HASH(_us_protocol)     \
    (_us_protocol & (HMAC_PROXYSTA_MAP_UNKNOW_HASHSIZE - 1))

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

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
typedef struct
{
    oal_dlist_head_stru     st_entry;
    oal_uint8               auc_ipv4[ETH_TARGET_IP_ADDR_LEN];       /* 记录对应的ipv4地址 */
    oal_uint8               auc_mac[WLAN_MAC_ADDR_LEN];             /* 记录对应的mac地址 */
    oal_uint8               auc_rsv[2];
    oal_uint32              ul_last_active_timestamp;               /* 最近一次发送数据的时间 */
}hmac_proxysta_ipv4_hash_stru;

typedef struct
{
    oal_dlist_head_stru     st_entry;
    oal_uint8               auc_ipv6[HMAC_PROXYSTA_IPV6_ADDR_LEN];  /* 记录对应的ipv6地址 */
    oal_uint8               auc_mac[WLAN_MAC_ADDR_LEN];             /* 记录对应的mac地址 */
    oal_uint8               auc_rsv[2];
    oal_uint32              ul_last_active_timestamp;            /* 最近一次发送数据的时间 */
}hmac_proxysta_ipv6_hash_stru;

typedef struct
{
    oal_dlist_head_stru     st_entry;
    oal_uint16              us_protocol;                             /* 记录对应的未知协议类型 */
    oal_uint8               auc_mac[WLAN_MAC_ADDR_LEN];             /* 记录对应的mac地址 */
    oal_uint32              ul_last_active_timestamp;            /* 最近一次发送数据的时间 */
}hmac_proxysta_unknow_hash_stru;

typedef struct
{
  union {
            oal_uint8        auc_addr8[16];
            oal_uint16       aus_addr16[8];
            oal_uint32       aul_addr32[4];
        } union_ipv6;
#define auc_ipv6_union_addr        union_ipv6.auc_addr8
#define aus_ipv6_union_addr        union_ipv6.aus_addr16
#define aul_ipv6_union_addr        union_ipv6.aul_addr32
}oal_ipv6_addr_stru;

struct oal_ipv6_header {
#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)            /* LITTLE_ENDIAN */
    oal_uint32      ul_flow_label:20,
                    uc_traffic_class:8,
                    uc_version:4;
#else
    oal_uint32      uc_version:4,
                    uc_traffic_class:8,
                    ul_flow_label:20;
#endif
    oal_uint16      us_payload_len;
    oal_uint8       uc_nexthdr;
    oal_uint8       uc_hop_limit;

    oal_ipv6_addr_stru       st_saddr;
    oal_ipv6_addr_stru       st_daddr;
}__OAL_DECLARE_PACKED;
typedef struct oal_ipv6_header oal_ipv6_header_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/

/* 初始化Proxy STA指针，开启MAP表格老化定时器 */
extern oal_uint32  hmac_proxysta_init_vap(hmac_vap_stru *pst_hmac_vap, mac_cfg_add_vap_param_stru *pst_param);

/* 释放Proxy STA指针，关闭MAP表格老化定时器 */
extern oal_uint32  hmac_proxysta_exit_vap(hmac_vap_stru *pst_hmac_vap);

/* 下行数据报文处理 替换报文中的目的MAC地址并转给LAN */
extern oal_uint32  hmac_proxysta_rx_process(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap);

/* 上行数据报文处理 替换报文中的源MAC地址 转给发送接口 */
extern oal_uint32  hmac_proxysta_tx_process(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap);

#endif  //_PRE_WLAN_FEATURE_SINGLE_PROXYSTA

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


#endif /* end of hmac_unique_proxysta.h */
