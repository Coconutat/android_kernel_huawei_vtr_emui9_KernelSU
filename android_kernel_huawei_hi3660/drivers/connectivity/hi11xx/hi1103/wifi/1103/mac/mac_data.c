


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_mem.h"
#include "oal_net.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "mac_data.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DATA_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint8 mac_get_data_type_from_80211_etc(oal_netbuf_stru *pst_netbuff, oal_uint16 us_mac_hdr_len)
{
    oal_uint8               uc_datatype = MAC_DATA_BUTT;
    mac_llc_snap_stru      *pst_snap;

    if(OAL_PTR_NULL == pst_netbuff)
    {
        return MAC_DATA_BUTT;
    }

    pst_snap = (mac_llc_snap_stru *)(OAL_NETBUF_DATA(pst_netbuff) + us_mac_hdr_len);

    uc_datatype = mac_get_data_type_from_8023_etc((oal_uint8 *)pst_snap, MAC_NETBUFF_PAYLOAD_SNAP);

    return uc_datatype;

}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint16 mac_rx_get_eapol_keyinfo_51(oal_netbuf_stru *pst_netbuff)
{
    oal_uint8                      uc_datatype = MAC_DATA_BUTT;
    oal_uint8                     *puc_payload;
    mac_rx_ctl_stru               *pst_rx_info;
    dmac_rx_ctl_stru              *pst_cb_ctrl;

    pst_cb_ctrl = (dmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuff);

    pst_rx_info = &(pst_cb_ctrl->st_rx_info);

    uc_datatype = mac_get_data_type_from_80211_etc(pst_netbuff, pst_rx_info->uc_mac_header_len);
    if(MAC_DATA_EAPOL != uc_datatype)
    {
        return 0;
    }

    puc_payload = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info, pst_netbuff);
    if(OAL_PTR_NULL == puc_payload)
    {
        return 0;
    }

    return *(oal_uint16 *)(puc_payload + OAL_EAPOL_INFO_POS);

}
#endif

oal_bool_enum_uint8 mac_is_eapol_key_ptk_etc(mac_eapol_header_stru  *pst_eapol_header)
{
    mac_eapol_key_stru *pst_key;

    if (IEEE802_1X_TYPE_EAPOL_KEY == pst_eapol_header->uc_type)
    {
        if ((oal_uint16)(OAL_NET2HOST_SHORT(pst_eapol_header->us_length)) >= (oal_uint16)OAL_SIZEOF(mac_eapol_key_stru))
        {
            pst_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);

            if (pst_key->auc_key_info[1] & WPA_KEY_INFO_KEY_TYPE)
            {
                return OAL_TRUE;
            }
        }
    }
    return OAL_FALSE;
}


oal_bool_enum_uint8 mac_is_eapol_key_ptk_4_4_etc(oal_netbuf_stru *pst_netbuff)
{
    mac_eapol_header_stru   *pst_eapol_header;
    mac_eapol_key_stru      *pst_eapol_key;

    if ((mac_get_data_type_etc(pst_netbuff) == MAC_DATA_EAPOL))
    {
        pst_eapol_header = (mac_eapol_header_stru *)(oal_netbuf_payload(pst_netbuff) + OAL_SIZEOF(mac_llc_snap_stru));
        if (mac_is_eapol_key_ptk_etc(pst_eapol_header) == OAL_TRUE)
        {
            pst_eapol_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);
            if (pst_eapol_key->auc_key_data_length[0] == 0
                && pst_eapol_key->auc_key_data_length[1] == 0)
            {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}

#ifndef _PRE_DEBUG_MODE

pkt_trace_type_enum_uint8 mac_pkt_should_trace(oal_uint8 *puc_frame_hdr, mac_netbuff_payload_type uc_hdr_type)
{
    oal_uint8                       uc_data_type         = MAC_DATA_BUTT;
    pkt_trace_type_enum_uint8       en_trace_data_type   = PKT_TRACE_BUTT;
    oal_uint16                      us_ether_type;
    oal_uint8                      *puc_frame_body;
    oal_ip_header_stru             *pst_ip;
    oal_uint8                      *puc_icmp_body;

    //识别出DHCP/ECHO/EAPOL/ARP

    uc_data_type = mac_get_data_type_from_8023_etc(puc_frame_hdr, uc_hdr_type);

    if(MAC_DATA_DHCP == uc_data_type)
    {
        en_trace_data_type   = PKT_TRACE_DATA_DHCP;
    }
    else if(MAC_DATA_ARP_REQ == uc_data_type)
    {
        en_trace_data_type   = PKT_TRACE_DATA_ARP_REQ;
    }
    else if(MAC_DATA_ARP_RSP == uc_data_type)
    {
        en_trace_data_type   = PKT_TRACE_DATA_ARP_RSP;
    }
    else if(MAC_DATA_EAPOL == uc_data_type)
    {
        en_trace_data_type   = PKT_TRACE_DATA_EAPOL;
    }
    else
    {
        if (MAC_NETBUFF_PAYLOAD_ETH == uc_hdr_type)
        {
            us_ether_type  = ((mac_ether_header_stru *)puc_frame_hdr)->us_ether_type;
            puc_frame_body = puc_frame_hdr + (oal_uint16)OAL_SIZEOF(mac_ether_header_stru);
        }
        else if (MAC_NETBUFF_PAYLOAD_SNAP == uc_hdr_type)
        {
            us_ether_type = ((mac_llc_snap_stru *)puc_frame_hdr)->us_ether_type;
            puc_frame_body = puc_frame_hdr + (oal_uint16)OAL_SIZEOF(mac_llc_snap_stru);
        }
        else
        {
            return en_trace_data_type;
        }

        /*lint -e778*/
        if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_type)
        {
            pst_ip = (oal_ip_header_stru *)puc_frame_body;      /* 偏移一个以太网头，取ip头 */

            if (MAC_ICMP_PROTOCAL == pst_ip->uc_protocol)
            {//判定为ICMP报文之后，进而筛选出ICMP REQ和ICMP REPLY
                puc_icmp_body = puc_frame_body + (oal_uint16)OAL_SIZEOF(oal_ip_header_stru);
                if(0 == *puc_icmp_body || 8 == *puc_icmp_body)
                {
                    en_trace_data_type = PKT_TRACE_DATA_ICMP;
                }
            }
        }
    }

    return en_trace_data_type;

}


pkt_trace_type_enum_uint8 wifi_pkt_should_trace(oal_netbuf_stru *pst_netbuff, oal_uint16 us_mac_hdr_len)
{
    pkt_trace_type_enum_uint8       en_trace_data_type   = PKT_TRACE_BUTT;
    mac_llc_snap_stru              *pst_snap;
    mac_ieee80211_frame_stru       *pst_mac_header;

    pst_mac_header = (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuff);
    if( WLAN_MANAGEMENT == pst_mac_header->st_frame_control.bit_type)
    {
        if(WLAN_ASSOC_REQ == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_ASSOC_REQ;
        }
        else if(WLAN_ASSOC_RSP == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_ASSOC_RSP;
        }
        else if(WLAN_REASSOC_REQ == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_REASSOC_REQ;
        }
        else if(WLAN_REASSOC_RSP == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_REASSOC_RSP;
        }
        else if(WLAN_DISASOC == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_DISASOC;
        }
        else if(WLAN_AUTH == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_AUTH;
        }
        else if(WLAN_DEAUTH == pst_mac_header->st_frame_control.bit_sub_type)
        {
            en_trace_data_type   = PKT_TRACE_MGMT_DEAUTH;
        }
    }
    else if( WLAN_DATA_BASICTYPE == pst_mac_header->st_frame_control.bit_type &&
        WLAN_NULL_FRAME != pst_mac_header->st_frame_control.bit_sub_type)
    {
        pst_snap = (mac_llc_snap_stru *)(OAL_NETBUF_DATA(pst_netbuff) + us_mac_hdr_len);

        en_trace_data_type = mac_pkt_should_trace((oal_uint8 *)pst_snap, MAC_NETBUFF_PAYLOAD_SNAP);
    }


    return en_trace_data_type;

}
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

oal_uint8 mac_get_dhcp_type_etc(oal_uint8 *puc_pos, oal_uint8 *puc_packet_end)
{
    oal_uint8       *puc_opt;
    while ((puc_pos < puc_packet_end) && (*puc_pos != 0xFF))
    {
        puc_opt = puc_pos++;
        if (0 == *puc_opt)
        {
            continue;  /* Padding */
        }
        puc_pos += *puc_pos + 1;
        if (puc_pos >= puc_packet_end)
        {
            break;
        }
        if ((53 == *puc_opt) && (puc_opt[1] != 0))  /* Message type */
        {
            return puc_opt[2];
        }
    }
    return 0xFF;//unknow type
}


oal_uint8 mac_get_dhcp_frame_type_etc(oal_ip_header_stru   *pst_rx_ip_hdr)
{
    oal_uint8 *puc_pos;
    oal_dhcp_packet_stru                *pst_rx_dhcp_hdr;
    oal_uint8                           *puc_packet_end;

    puc_pos = (oal_uint8*)pst_rx_ip_hdr;
    puc_pos        += (puc_pos[0] & 0x0F) << 2;  /* point udp header */
    pst_rx_dhcp_hdr = (oal_dhcp_packet_stru *)(puc_pos + 8);

    puc_packet_end  = (oal_uint8 *)pst_rx_ip_hdr + OAL_NET2HOST_SHORT(pst_rx_ip_hdr->us_tot_len);
    puc_pos         = &(pst_rx_dhcp_hdr->options[4]);

    return mac_get_dhcp_type_etc(puc_pos, puc_packet_end);
}



mac_eapol_type_enum_uint8 mac_get_eapol_key_type_etc(oal_uint8 *pst_payload)
{
    mac_eapol_header_stru   *pst_eapol_header;
    mac_eapol_key_stru      *pst_eapol_key;
    oal_bool_enum_uint8      en_key_ack_set = OAL_FALSE;
    oal_bool_enum_uint8      en_key_mic_set = OAL_FALSE;
    oal_bool_enum_uint8      en_key_len_set = OAL_FALSE;
    mac_eapol_type_enum_uint8 en_eapol_type = MAC_EAPOL_PTK_BUTT;

    /* 调用此接口,需保证已默认识别Ethertype为0x888E, EAPOL */
    /* 入参payload为LLC或者Ether头后 */
    pst_eapol_header = (mac_eapol_header_stru *)(pst_payload);

    if (mac_is_eapol_key_ptk_etc(pst_eapol_header) == OAL_TRUE)
    {
        pst_eapol_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);

        if (pst_eapol_key->auc_key_info[1] & WPA_KEY_INFO_KEY_ACK)
        {
            en_key_ack_set = OAL_TRUE;
        }

        if (pst_eapol_key->auc_key_info[0] & WPA_KEY_INFO_KEY_MIC)
        {
            en_key_mic_set = OAL_TRUE;
        }

        if (pst_eapol_key->auc_key_data_length[0] != 0 || pst_eapol_key->auc_key_data_length[1] != 0)
        {
            en_key_len_set = OAL_TRUE;
        }

        /* ack设置,表明STA发起期望响应 */
        if (OAL_TRUE == en_key_ack_set)
        {
            if ((OAL_FALSE == en_key_mic_set) && (OAL_FALSE == en_key_len_set))
            {
                /* 1/4识别:ACK=1,MIC=0,LEN=0 */
                en_eapol_type = MAC_EAPOL_PTK_1_4;
            }
            else if ((OAL_TRUE == en_key_mic_set) && (OAL_TRUE == en_key_len_set))
            {
                /* 3/4识别:ACK=1,MIC=1,LEN=1 */
                en_eapol_type = MAC_EAPOL_PTK_3_4;
            }
        }
        else
        {
            if ((OAL_TRUE == en_key_mic_set) && (OAL_TRUE == en_key_len_set))
            {
                /* 2/4识别:ACK=0,MIC=0,LEN=1 */
                en_eapol_type = MAC_EAPOL_PTK_2_4;
            }
            else if ((OAL_TRUE == en_key_mic_set) && (OAL_FALSE == en_key_len_set))
            {
                /* 4/4识别:ACK=0,MIC=1,LEN=0 */
                en_eapol_type = MAC_EAPOL_PTK_4_4;
            }
        }
    }

    return en_eapol_type;
}

#endif
/*lint -e19*/
oal_module_symbol(mac_get_data_type_from_80211_etc);
oal_module_symbol(mac_is_eapol_key_ptk_etc);

/*lint +e19*/




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


