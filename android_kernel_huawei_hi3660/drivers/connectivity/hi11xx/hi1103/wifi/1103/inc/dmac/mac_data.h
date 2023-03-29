

#ifndef __MAC_DATA_H__
#define __MAC_DATA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_mib.h"
#include "mac_user.h"
#include "oam_ext_if.h"
#include "mac_regdomain.h"
#include "hal_ext_if.h"


/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 暂时放在这里，需要放入oal_net.h */
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#define OAL_SWAP_BYTEORDER_16(_val) ((((_val) & 0x00FF) << 8) + (((_val) & 0xFF00) >> 8))
#define OAL_HOST2NET_SHORT(_x)  OAL_SWAP_BYTEORDER_16(_x)
#define OAL_NET2HOST_SHORT(_x)  OAL_SWAP_BYTEORDER_16(_x)
#define OAL_HOST2NET_LONG(_x)   OAL_SWAP_BYTEORDER_32(_x)
#define OAL_NET2HOST_LONG(_x)   OAL_SWAP_BYTEORDER_32(_x)

#define OAL_IPPROTO_UDP        17         /* User Datagram Protocot */
#define OAL_IPPROTO_ICMPV6     58         /* ICMPv6 */
#endif

#define OAL_EAPOL_INFO_POS      13
#define OAL_EAPOL_TYPE_POS      9
#define OAL_EAPOL_TYPE_KEY      3

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* 数据帧子类型枚举定义 */
typedef enum
{
    MAC_DATA_DHCP                 = 0,
    MAC_DATA_EAPOL                ,
    MAC_DATA_ARP_RSP              ,
    MAC_DATA_ARP_REQ              ,  /* 注意: 前4个vip帧类型顺序勿变 */
    MAC_DATA_DHCPV6               ,
    MAC_DATA_PPPOE                ,
    MAC_DATA_WAPI                 ,
    MAC_DATA_HS20                 ,
    MAC_DATA_CHARIOT_SIG          ,  /* chariot 信令报文 */
    MAC_DATA_VIP_FRAME            = MAC_DATA_CHARIOT_SIG, /* 以上为VIP DATA FRAME */
    MAC_DATA_TDLS                 ,
    MAC_DATA_VLAN                 ,
    MAC_DATA_ND                   ,

    MAC_DATA_BUTT
}mac_data_type_enum_uint8;

typedef enum
{
    MAC_NETBUFF_PAYLOAD_ETH  = 0,
    MAC_NETBUFF_PAYLOAD_SNAP,

    MAC_NETBUFF_PAYLOAD_BUTT
}mac_netbuff_payload_type;
typedef oal_uint8 mac_netbuff_payload_type_uint8;

typedef enum
{
    PKT_TRACE_DATA_DHCP                 = 0,
    PKT_TRACE_DATA_ARP_REQ              ,
    PKT_TRACE_DATA_ARP_RSP              ,
    PKT_TRACE_DATA_EAPOL                ,
    PKT_TRACE_DATA_ICMP                 ,
    PKT_TRACE_MGMT_ASSOC_REQ            ,
    PKT_TRACE_MGMT_ASSOC_RSP            ,
    PKT_TRACE_MGMT_REASSOC_REQ          ,
    PKT_TRACE_MGMT_REASSOC_RSP          ,
    PKT_TRACE_MGMT_DISASOC              ,
    PKT_TRACE_MGMT_AUTH                 ,
    PKT_TRACE_MGMT_DEAUTH               ,
    PKT_TRACE_BUTT
}pkt_trace_type_enum;
typedef oal_uint8 pkt_trace_type_enum_uint8;


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
typedef oal_void (*data_type_from_8023)(oal_uint8 *puc_frame_hdr, mac_netbuff_payload_type uc_hdr_type, oal_uint8 *puc_datatype);



typedef struct
{
    data_type_from_8023        data_type_from_8023_cb;
}mac_data_cb;
extern mac_data_cb g_st_mac_data_rom_cb;
/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/* channel结构体 */

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint8 mac_get_dhcp_frame_type_etc(oal_ip_header_stru   *pst_rx_ip_hdr);
extern mac_eapol_type_enum_uint8 mac_get_eapol_key_type_etc(oal_uint8 *pst_payload);
extern oal_bool_enum_uint8 mac_is_dhcp_port_etc(mac_ip_header_stru *pst_ip_hdr);
extern oal_bool_enum_uint8 mac_is_nd_etc(oal_ipv6hdr_stru  *pst_ipv6hdr);
extern oal_bool_enum_uint8 mac_is_dhcp6_etc(oal_ipv6hdr_stru  *pst_ether_hdr);
extern oal_uint8 mac_get_data_type_etc(oal_netbuf_stru *pst_netbuff);
extern oal_uint8 mac_get_data_type_from_8023_etc(oal_uint8 *puc_frame_hdr, mac_netbuff_payload_type uc_hdr_type);
oal_bool_enum_uint8 mac_is_eapol_key_ptk_etc(mac_eapol_header_stru  *pst_eapol_header);
extern  oal_uint8 mac_get_data_type_from_80211_etc(oal_netbuf_stru *pst_netbuff, oal_uint16 us_mac_hdr_len);
extern oal_uint16 mac_get_eapol_keyinfo_etc(oal_netbuf_stru *pst_netbuff);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
extern oal_uint16 mac_rx_get_eapol_keyinfo_51(oal_netbuf_stru *pst_netbuff);
#endif
extern oal_uint8 mac_get_eapol_type_etc(oal_netbuf_stru *pst_netbuff);
extern oal_bool_enum_uint8 mac_is_eapol_key_ptk_4_4_etc(oal_netbuf_stru *pst_netbuff);
#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE
extern oal_bool_enum_uint8 mac_dhcp_frame_should_drop(oal_uint8 *puc_frame_hdr, wlan_vap_mode_enum_uint8 mode);
#endif
extern pkt_trace_type_enum_uint8 mac_pkt_should_trace(oal_uint8 *puc_frame_hdr, mac_netbuff_payload_type uc_hdr_type);
extern pkt_trace_type_enum_uint8 wifi_pkt_should_trace(oal_netbuf_stru *pst_netbuff, oal_uint16 us_mac_hdr_len);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


#endif /* end of mac_vap.h */

