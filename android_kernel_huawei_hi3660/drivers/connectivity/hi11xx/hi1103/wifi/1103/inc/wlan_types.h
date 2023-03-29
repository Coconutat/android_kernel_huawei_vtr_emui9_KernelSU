

#ifndef __WLAN_TYPES_H__
#define __WLAN_TYPES_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "platform_spec.h"
#include "oal_ext_if.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  2.1 基本宏定义
*****************************************************************************/

#define MAC_BYTE_ALIGN_VALUE                4           /* 4字节对齐 */

#define WLAN_MAC_ADDR_LEN                   6           /* MAC地址长度宏 */
#define WLAN_MAX_FRAME_HEADER_LEN           36          /* 最大的MAC帧头长度，数据帧36，管理帧为28 */
#define WLAN_MIN_FRAME_HEADER_LEN           10          /* ack与cts的帧头长度为10 */
#define WLAN_MAX_FRAME_LEN                  1600        /* 维测用，防止越界 */
#define WLAN_MGMT_FRAME_HEADER_LEN          24          /* 管理帧的MAC帧头长度，数据帧36，管理帧为28 */
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
#define WLAN_IWPRIV_MAX_BUFF_LEN            200         /* iwpriv上传的字符串最大长度 */
#else
#define WLAN_IWPRIV_MAX_BUFF_LEN            100         /* iwpriv上传的字符串最大长度 */
#endif
/* SSID最大长度, +1为\0预留空间 */
#define WLAN_SSID_MAX_LEN                   (32 + 1)

/* 80211MAC帧头FC字段宏定义 */
#define WLAN_PROTOCOL_VERSION               0x00        /* 协议版本 */
#define WLAN_FC0_TYPE_MGT                   0x00        /* 管理帧 */
#define WLAN_FC0_TYPE_CTL                   0x04        /* 控制帧 */
#define WLAN_FC0_TYPE_DATA                  0x08        /* 数据帧 */

/* 管理帧subtype */
#define WLAN_FC0_SUBTYPE_ASSOC_REQ          0x00
#define WLAN_FC0_SUBTYPE_ASSOC_RSP          0x10
#define WLAN_FC0_SUBTYPE_REASSOC_REQ        0x20
#define WLAN_FC0_SUBTYPE_REASSOC_RSP        0x30
#define WLAN_FC0_SUBTYPE_PROBE_REQ          0x40
#define WLAN_FC0_SUBTYPE_PROBE_RSP          0x50
#define WLAN_FC0_SUBTYPE_BEACON             0x80
#define WLAN_FC0_SUBTYPE_ATIM               0x90
#define WLAN_FC0_SUBTYPE_DISASSOC           0xa0
#define WLAN_FC0_SUBTYPE_AUTH               0xb0
#define WLAN_FC0_SUBTYPE_DEAUTH             0xc0
#define WLAN_FC0_SUBTYPE_ACTION             0xd0
#define WLAN_FC0_SUBTYPE_ACTION_NO_ACK      0xe0

/* 控制帧subtype */
#define WLAN_FC0_SUBTYPE_NDPA               0x50
#define WLAN_FC0_SUBTYPE_Control_Wrapper    0x70        /* For TxBF RC */
#define WLAN_FC0_SUBTYPE_BAR                0x80
#define WLAN_FC0_SUBTYPE_BA                 0x90

#define WLAN_FC0_SUBTYPE_PS_POLL            0xa0
#define WLAN_FC0_SUBTYPE_RTS                0xb0
#define WLAN_FC0_SUBTYPE_CTS                0xc0
#define WLAN_FC0_SUBTYPE_ACK                0xd0
#define WLAN_FC0_SUBTYPE_CF_END             0xe0
#define WLAN_FC0_SUBTYPE_CF_END_ACK         0xf0

/* 数据帧subtype */
#define WLAN_FC0_SUBTYPE_DATA               0x00
#define WLAN_FC0_SUBTYPE_CF_ACK             0x10
#define WLAN_FC0_SUBTYPE_CF_POLL            0x20
#define WLAN_FC0_SUBTYPE_CF_ACPL            0x30
#define WLAN_FC0_SUBTYPE_NODATA             0x40
#define WLAN_FC0_SUBTYPE_CFACK              0x50
#define WLAN_FC0_SUBTYPE_CFPOLL             0x60
#define WLAN_FC0_SUBTYPE_CF_ACK_CF_ACK      0x70
#define WLAN_FC0_SUBTYPE_QOS                0x80
#define WLAN_FC0_SUBTYPE_QOS_NULL           0xc0

#define WLAN_FC1_DIR_MASK                   0x03
#define WLAN_FC1_DIR_NODS                   0x00        /* STA->STA */
#define WLAN_FC1_DIR_TODS                   0x01        /* STA->AP  */
#define WLAN_FC1_DIR_FROMDS                 0x02        /* AP ->STA */
#define WLAN_FC1_DIR_DSTODS                 0x03        /* AP ->AP  */

#define WLAN_FC1_MORE_FRAG                  0x04
#define WLAN_FC1_RETRY                      0x08
#define WLAN_FC1_PWR_MGT                    0x10
#define WLAN_FC1_MORE_DATA                  0x20
#define WLAN_FC1_WEP                        0x40
#define WLAN_FC1_ORDER                      0x80

#define WLAN_HDR_DUR_OFFSET                 2           /* duartion相对于mac头的字节偏移 */
#define WLAN_HDR_ADDR1_OFFSET               4           /* addr1相对于mac头的字节偏移 */
#define WLAN_HDR_ADDR2_OFFSET               10          /* addr1相对于mac头的字节偏移 */
#define WLAN_HDR_ADDR3_OFFSET               16          /* addr1相对于mac头的字节偏移 */
#define WLAN_HDR_FRAG_OFFSET                22          /* 分片序号相对于mac的字节偏移 */

#define WLAN_REASON_CODE_LEN                2

/* 帧头DS位 */
#define WLAN_FRAME_TO_AP                   0x0100
#define WLAN_FRAME_FROM_AP                 0x0200
/* FCS长度(4字节) */
#define WLAN_HDR_FCS_LENGTH                 4

#define WLAN_RANDOM_MAC_OUI_LEN            3            /* 随机mac地址OUI长度 */

#define WLAN_MAX_BAR_DATA_LEN               20  /* BAR帧的最大长度 */
#define WLAN_CHTXT_SIZE                     128 /* challenge text的长度 */

#define WLAN_SEQ_SHIFT                      4
/* AMPDU Delimeter长度(4字节) */
#define WLAN_DELIMETER_LENGTH               4

/* 取绝对值 */
#define GET_ABS(val)                        ((val) > 0 ? (val) : -(val))

/* 配置命令最大长度: 从算法名称开始算起，不包括"alg" */
#define DMAC_ALG_CONFIG_MAX_ARG     7

/* 信道切换计数 */
#define WLAN_CHAN_SWITCH_DEFAULT_CNT        6
#define WLAN_CHAN_SWITCH_DETECT_RADAR_CNT   1

/* 5G子频段数目 */
#define WLAN_5G_SUB_BAND_NUM        7
#define WLAN_5G_20M_SUB_BAND_NUM    7
#define WLAN_5G_80M_SUB_BAND_NUM    7
#define WLAN_5G_CALI_SUB_BAND_NUM   (WLAN_5G_20M_SUB_BAND_NUM + WLAN_5G_80M_SUB_BAND_NUM)

#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
#define WLAN_DYNC_CALI_POW_PD_PARAM_NUM   3    /* 动态校准power&pdet表达式参数个数 */
#define WLAN_DYNC_CALI_2G_PD_PARAM_NUMS   18
#define WLAN_DYNC_CALI_5G_PD_PARAM_NUMS   36
#define WLAN_DYNC_CALI_5G_UPC_PARAM_NUMS  14
#define WLAN_DYNC_CALI_5G_UPC_PARAM_STR_LEN  69 /* 5G upc参数字符串长度:4(每个参数字符个数)*14(参数个数)+13(分隔符)=69 */
#define WLAN_DYNC_CALI_2G_UPC_PARAM_STR_LEN  29 /* 2G upc参数字符串长度:4(每个参数字符个数)*6(参数个数)+5(分隔符)=29 */
#define WLAN_DYNC_CALI_PDET_MAX           800  /* 动态校准pdet上报最大值 */
#endif

#define WLAN_DIEID_MAX_LEN   40

#define WLAN_FIELD_TYPE_AID            0xC000
#define WLAN_AID(AID)                  ((AID) &~ 0xc000)

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#define WLAN_MAX_VAP_NUM            4   /*设备支持的最大VAP数目*/
#define WLAN_SIFS_OFDM_POWLVL_NUM   1   /*OFDM SIFS帧的功率档位数目*/
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
#define WLAN_2G_SUB_BAND_NUM        13   /*2G子频段数目*/
#else
#define WLAN_2G_SUB_BAND_NUM        3   /*2G子频段数目*/
#endif

#else
#define WLAN_MAX_VAP_NUM            2
#define WLAN_2G_SUB_BAND_NUM        13
#define WLAN_SIFS_OFDM_POWLVL_NUM   4
#endif

#define WLAN_BW_CAP_TO_BANDWIDTH(_bw_cap) \
    ((WLAN_BW_CAP_20M == (_bw_cap)) ? WLAN_BAND_ASSEMBLE_20M :   \
     (WLAN_BW_CAP_40M == (_bw_cap)) ? WLAN_BAND_ASSEMBLE_40M :   \
     (WLAN_BW_CAP_80M == (_bw_cap)) ? WLAN_BAND_ASSEMBLE_80M :   \
     (WLAN_BW_CAP_160M == (_bw_cap)) ? WLAN_BAND_ASSEMBLE_160M : \
     (WLAN_BAND_ASSEMBLE_20M))

#define WLAN_INVALD_VHT_MCS     0xff
#define WLAN_GET_VHT_MAX_SUPPORT_MCS(_us_vht_mcs_map)   \
     ((3 == (_us_vht_mcs_map)) ? WLAN_INVALD_VHT_MCS:     \
      (2 == (_us_vht_mcs_map)) ? WLAN_VHT_MCS9:           \
      (1 == (_us_vht_mcs_map)) ? WLAN_VHT_MCS8: WLAN_VHT_MCS7)

/*****************************************************************************
  2.2 WME宏定义
*****************************************************************************/
#define WLAN_WME_AC_TO_TID(_ac) (       \
        ((_ac) == WLAN_WME_AC_VO) ? 6 : \
        ((_ac) == WLAN_WME_AC_VI) ? 5 : \
        ((_ac) == WLAN_WME_AC_BK) ? 1 : \
        0)



#define WLAN_WME_TID_TO_AC(_tid) (      \
        (((_tid) == 0) || ((_tid) == 3)) ? WLAN_WME_AC_BE : \
        (((_tid) == 1) || ((_tid) == 2)) ? WLAN_WME_AC_BK : \
        (((_tid) == 4) || ((_tid) == 5)) ? WLAN_WME_AC_VI : \
        WLAN_WME_AC_VO)

/*****************************************************************************
  2.3 HT宏定义
*****************************************************************************/
/* 11n: Maximum A-MSDU Length Indicates maximum A-MSDU length.See 9.11. Set to 0 for 3839 octetsSet to 1 for 7935 octets */
/* 11AC(9.11): A VHT STA that sets the Maximum MPDU Length in the VHT Capabilities element to indicate 3895 octets
   shall set the Maximum A-MSDU Length in the HT Capabilities element to indicate 3839 octets. A VHT STA
   that sets the Maximum MPDU Length in the VHT Capabilities element to indicate 7991 octets or 11 454 oc-
   tets shall set the Maximum A-MSDU Length in the HT Capabilities element to indicate 7935 octets. */

#define WLAN_AMSDU_FRAME_MAX_LEN_SHORT      3839
#define WLAN_AMSDU_FRAME_MAX_LEN_LONG       7935

/* Maximum A-MSDU Length Indicates maximum A-MSDU length.See 9.11. Set to 0 for 3839 octetsSet to 1 for 7935 octets */
#define WLAN_HT_GET_AMSDU_MAX_LEN(_bit_amsdu_max_length)  \
       ((0 == (_bit_amsdu_max_length)) ? WLAN_AMSDU_FRAME_MAX_LEN_SHORT : WLAN_AMSDU_FRAME_MAX_LEN_LONG)

/* RSSI统计滤波，RSSI范围是-128~127, 一般不会等于127这么大，所以将127设置为MARKER,即初始值 */
#define WLAN_RSSI_DUMMY_MARKER              0x7F

/*****************************************************************************
  2.4 安全相关宏定义
*****************************************************************************/
    /* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP 0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40     0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP      0x000FAC02
    /* reserved:                0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP      0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104    0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC  0x000FAC06
#define WLAN_CIPHER_SUITE_GCMP      0x000FAC08
#define WLAN_CIPHER_SUITE_GCMP_256  0x000FAC09
#define WLAN_CIPHER_SUITE_CCMP_256  0x000FAC0A
#define WLAN_CIPHER_SUITE_BIP_GMAC_128  0x000FAC0B
#define WLAN_CIPHER_SUITE_BIP_GMAC_256  0x000FAC0C
#define WLAN_CIPHER_SUITE_BIP_CMAC_256  0x000FAC0D


#undef  WLAN_CIPHER_SUITE_SMS4
#define WLAN_CIPHER_SUITE_SMS4      0x00147201

/* AKM suite selectors */
#define WITP_WLAN_AKM_SUITE_8021X        0x000FAC01
#define WITP_WLAN_AKM_SUITE_PSK          0x000FAC02
#define WITP_WLAN_AKM_SUITE_WAPI_PSK     0x000FAC04
#define WITP_WLAN_AKM_SUITE_WAPI_CERT    0x000FAC12



#define WLAN_PMKID_LEN           16
#define WLAN_PMKID_CACHE_SIZE    32
#define WLAN_NONCE_LEN           32
#define WLAN_PTK_PREFIX_LEN      22
#define WLAN_GTK_PREFIX_LEN      19
#define WLAN_GTK_DATA_LEN        (NONCE_LEN + WLAN_MAC_ADDR_LEN)
#define WLAN_PTK_DATA_LEN        (2 * NONCE_LEN + 2 * WLAN_MAC_ADDR_LEN)


#define WLAN_KCK_LENGTH          16
#define WLAN_KEK_LENGTH          16
#define WLAN_TEMPORAL_KEY_LENGTH 16
#define WLAN_MIC_KEY_LENGTH      8

#define WLAN_PMK_SIZE        32 /* In Bytes */
#define WLAN_PTK_SIZE        64 /* In Bytes */
#define WLAN_GTK_SIZE        32 /* In Bytes */
#define WLAN_GMK_SIZE        32 /* In Bytes */

#define WLAN_WEP40_KEY_LEN              5
#define WLAN_WEP104_KEY_LEN             13
#define WLAN_TKIP_KEY_LEN               32      /* TKIP KEY length 256 BIT */
#define WLAN_CCMP_KEY_LEN               16      /* CCMP KEY length 128 BIT */
#define WLAN_BIP_KEY_LEN                16      /* BIP KEY length 128 BIT */

#define WLAN_NUM_DOT11WEPDEFAULTKEYVALUE       4
#define WLAN_MAX_WEP_STR_SIZE                  27 /* 5 for WEP 40; 13 for WEP 104 */
#define WLAN_WEP_SIZE_OFFSET                   0
#define WLAN_WEP_KEY_VALUE_OFFSET              1
#define WLAN_WEP_40_KEY_SIZE                  40
#define WLAN_WEP_104_KEY_SIZE                104

#define WLAN_COUNTRY_STR_LEN        3

/* crypto status */
#define WLAN_ENCRYPT_BIT        (1 << 0)
#define WLAN_WEP_BIT            (1 << 1)
#define WLAN_WEP104_BIT         ((1 << 2) | (1 << 1))
#define WLAN_WPA_BIT            (1 << 3)
#define WLAN_WPA2_BIT           (1 << 4)
#define WLAN_CCMP_BIT           (1 << 5)
#define WLAN_TKIP_BIT           (1 << 6)

#define WLAN_WAPI_BIT           (1 << 5)

/* 11i参数 */
/*WPA 密钥长度*/
#define WLAN_WPA_KEY_LEN                    32
#define WLAN_CIPHER_KEY_LEN                 16
#define WLAN_TKIP_MIC_LEN                   16
/*WPA 序号长度*/
#define WLAN_WPA_SEQ_LEN                    16

/* auth */
#define WLAN_OPEN_SYS_BIT       (1 << 0)
#define WLAN_SHARED_KEY_BIT     (1 << 1)
#define WLAN_8021X_BIT          (1 << 2)

#define WLAN_WITP_CAPABILITY_PRIVACY BIT4

#define WLAN_NUM_TK             4
#define WLAN_NUM_IGTK           2
#define WLAN_MAX_IGTK_KEY_INDEX 5
#define WLAN_MAX_WEP_KEY_COUNT  4


/*****************************************************************************
  2.5 过滤命令宏定义
*****************************************************************************/
#define  WLAN_BIP_REPLAY_FAIL_FLT  BIT0               /* BIP重放攻击过滤 */
#define  WLAN_CCMP_REPLAY_FAIL_FLT  BIT1              /* CCMP重放攻击过滤 */
#define  WLAN_OTHER_CTRL_FRAME_FLT BIT2               /* direct控制帧过滤*/
#define  WLAN_BCMC_MGMT_OTHER_BSS_FLT BIT3            /*其他BSS网络的组播管理帧过滤*/
/*只有1151V100需要处理*/
#define  WLAN_UCAST_MGMT_OTHER_BSS_FLT BIT4           /*其他BSS网络的单播管理帧过滤*/

#define  WLAN_UCAST_DATA_OTHER_BSS_FLT BIT5           /*其他BSS网络的单播数据帧过滤*/

/*****************************************************************************
  2.6 TXBF相关宏定义
*****************************************************************************/

/*****************************************************************************
  2.7  TX & RX Chain Macro
*****************************************************************************/
/* RF通道数规格*/
/* RF0 */
#define WLAN_RF_CHANNEL_ZERO        0
/* RF1 */
#define WLAN_RF_CHANNEL_ONE         1

/* PHY通道选择 */
/* 通道0 */
#define WLAN_PHY_CHAIN_ZERO_IDX     0
/* 通道1 */
#define WLAN_PHY_CHAIN_ONE_IDX      1

/* 双通道掩码 */
#define WLAN_RF_CHAIN_DOUBLE        3
#define WLAN_RF_CHAIN_ONE           2
#define WLAN_RF_CHAIN_ZERO          1

/* 双通道掩码 */
#define WLAN_PHY_CHAIN_DOUBLE       3
#define WLAN_TX_CHAIN_DOUBLE        WLAN_PHY_CHAIN_DOUBLE
#define WLAN_RX_CHAIN_DOUBLE        WLAN_PHY_CHAIN_DOUBLE
/*  通道0 掩码*/
#define WLAN_PHY_CHAIN_ZERO         1
#define WLAN_TX_CHAIN_ZERO          WLAN_PHY_CHAIN_ZERO
#define WLAN_RX_CHAIN_ZERO          WLAN_PHY_CHAIN_ZERO
/*  通道1 掩码*/
#define WLAN_PHY_CHAIN_ONE          2
#define WLAN_TX_CHAIN_ONE           WLAN_PHY_CHAIN_ONE
#define WLAN_RX_CHAIN_ONE           WLAN_PHY_CHAIN_ONE

#define WLAN_2G_CHANNEL_NUM         14
#define WLAN_5G_CHANNEL_NUM         29
/* wifi 5G 2.4G全部信道个数 */
#define WLAN_MAX_CHANNEL_NUM        (WLAN_2G_CHANNEL_NUM + WLAN_5G_CHANNEL_NUM)

/* PLL0 */
#define WLAN_RF_PLL_ID_0            0
/* PLL1 */
#define WLAN_RF_PLL_ID_1            1
/*****************************************************************************
  2.8 linkloss
*****************************************************************************/
#define WLAN_LINKLOSS_REPORT            10 /* linkloss每10次打印一次维测*/
#define WLAN_LINKLOSS_MIN_THRESHOLD     10 /* linkloss门限最小最低值 */

#ifdef _PRE_WLAN_FEATURE_11AX
/* 11AX MCS相关的内容 */
#define MAC_MAX_SUP_MCS7_11AX_EACH_NSS             0   /* 11AX各空间流支持的最大MCS序号，支持0-7*/
#define MAC_MAX_SUP_MCS9_11AX_EACH_NSS             1   /* 11AX各空间流支持的最大MCS序号，支持0-9*/
#define MAC_MAX_SUP_MCS11_11AX_EACH_NSS            2   /* 11AX各空间流支持的最大MCS序号，支持0-11*/
#define MAC_MAX_SUP_INVALID_11AX_EACH_NSS          3   /* 不支持*/


#define MAC_MAX_RATE_SINGLE_NSS_20M_11AX           106  /* 1个空间流20MHz的最大速率-见ax协议28.5-HE-MCSs*/
#define MAC_MAX_RATE_SINGLE_NSS_40M_11AX           212 /* 1个空间流40MHz的最大Long GI速率*/
#define MAC_MAX_RATE_SINGLE_NSS_80M_11AX           212 /* 1个空间流40MHz的最大Long GI速率*/

#define MAC_MAX_RATE_DOUBLE_NSS_20M_11AX           211 /* 2个空间流20MHz的最大Long GI速率*/
#define MAC_MAX_RATE_DOUBLE_NSS_40M_11AX           423 /* 2个空间流20MHz的最大Long GI速率*/
#define MAC_MAX_RATE_DOUBLE_NSS_80M_11AX           869 /* 2个空间流20MHz的最大Long GI速率*/

#define MAC_TRIGGER_FRAME_PADDING_DURATION0us      0
#define MAC_TRIGGER_FRAME_PADDING_DURATION8us      1
#define MAC_TRIGGER_FRAME_PADDING_DURATION16us     2
#endif

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
  3.1 基本枚举类型
*****************************************************************************/
/* 管理帧子类型 */
typedef enum
{
    WLAN_ASSOC_REQ              = 0,    /* 0000 */
    WLAN_ASSOC_RSP              = 1,    /* 0001 */
    WLAN_REASSOC_REQ            = 2,    /* 0010 */
    WLAN_REASSOC_RSP            = 3,    /* 0011 */
    WLAN_PROBE_REQ              = 4,    /* 0100 */
    WLAN_PROBE_RSP              = 5,    /* 0101 */
    WLAN_TIMING_AD              = 6,    /* 0110 */
    WLAN_MGMT_SUBTYPE_RESV1     = 7,    /* 0111 */
    WLAN_BEACON                 = 8,    /* 1000 */
    WLAN_ATIM                   = 9,    /* 1001 */
    WLAN_DISASOC                = 10,   /* 1010 */
    WLAN_AUTH                   = 11,   /* 1011 */
    WLAN_DEAUTH                 = 12,   /* 1100 */
    WLAN_ACTION                 = 13,   /* 1101 */
    WLAN_ACTION_NO_ACK          = 14,   /* 1110 */
    WLAN_MGMT_SUBTYPE_RESV2     = 15,   /* 1111 */

    WLAN_MGMT_SUBTYPE_BUTT      = 16,   /* 一共16种管理帧子类型 */
}wlan_frame_mgmt_subtype_enum;

/*TBD，不应该BUTT后续存在枚举*/
typedef enum
{
    WLAN_WME_AC_BE = 0,    /* best effort */
    WLAN_WME_AC_BK = 1,    /* background */
    WLAN_WME_AC_VI = 2,    /* video */
    WLAN_WME_AC_VO = 3,    /* voice */

    WLAN_WME_AC_BUTT = 4,
    WLAN_WME_AC_MGMT = WLAN_WME_AC_BUTT,   /* 管理AC，协议没有,对应硬件高优先级队列*/

    WLAN_WME_AC_PSM = 5    /* 节能AC, 协议没有，对应硬件组播队列 */
}wlan_wme_ac_type_enum;
typedef oal_uint8 wlan_wme_ac_type_enum_uint8;

/* TID个数为8,0~7 */
#define WLAN_TID_MAX_NUM                    WLAN_TIDNO_BUTT

/* TID编号类别 */
typedef enum
{
    WLAN_TIDNO_BEST_EFFORT              = 0, /* BE业务 */
    WLAN_TIDNO_BACKGROUND               = 1, /* BK业务 */
    WLAN_TIDNO_UAPSD                    = 2, /* U-APSD */
    WLAN_TIDNO_ANT_TRAINING_LOW_PRIO    = 3, /* 智能天线低优先级训练帧 */
    WLAN_TIDNO_ANT_TRAINING_HIGH_PRIO   = 4, /* 智能天线高优先级训练帧 */
    WLAN_TIDNO_VIDEO                    = 5, /* VI业务 */
    WLAN_TIDNO_VOICE                    = 6, /* VO业务 */
    WLAN_TIDNO_BCAST                    = 7, /* 广播用户的广播或者组播报文 */

    WLAN_TIDNO_BUTT
}wlan_tidno_enum;
typedef oal_uint8 wlan_tidno_enum_uint8;

/* TID编号类别放入平台 */

/* VAP的工作模式 */
typedef enum
{
    WLAN_VAP_MODE_CONFIG,        /* 配置模式 */
    WLAN_VAP_MODE_BSS_STA,       /* BSS STA模式 */
    WLAN_VAP_MODE_BSS_AP,        /* BSS AP模式 */
    WLAN_VAP_MODE_WDS,           /* WDS模式 */
    WLAN_VAP_MODE_MONITOER,      /* 侦听模式 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    WLAN_VAP_MODE_PROXYSTA,     /* proxysta模式，只是在写hal接口时使用 */
#endif
    WLAN_VAP_HW_TEST,

    WLAN_VAP_MODE_BUTT
}wlan_vap_mode_enum;
typedef oal_uint8 wlan_vap_mode_enum_uint8;

/* 认证的transaction number */
typedef enum
{
    WLAN_AUTH_TRASACTION_NUM_ONE    = 0x0001,
    WLAN_AUTH_TRASACTION_NUM_TWO    = 0x0002,
    WLAN_AUTH_TRASACTION_NUM_THREE  = 0x0003,
    WLAN_AUTH_TRASACTION_NUM_FOUR   = 0x0004,

    WLAN_AUTH_TRASACTION_NUM_BUTT
}wlan_auth_transaction_number_enum;
typedef oal_uint16 wlan_auth_transaction_number_enum_uint16;
typedef enum
{
    WLAN_BAND_2G,
    WLAN_BAND_5G,

    WLAN_BAND_BUTT
} wlan_channel_band_enum;
typedef oal_uint8 wlan_channel_band_enum_uint8;

/* Protection mode for MAC */
typedef enum
{
    WLAN_PROT_NO,           /* Do not use any protection       */
    WLAN_PROT_ERP,          /* Protect all ERP frame exchanges */
    WLAN_PROT_HT,           /* Protect all HT frame exchanges  */
    WLAN_PROT_GF,           /* Protect all GF frame exchanges  */

    WLAN_PROT_BUTT
}wlan_prot_mode_enum;
typedef oal_uint8 wlan_prot_mode_enum_uint8;

typedef enum
{
    WLAN_RTS_RATE_SELECT_MODE_REG,  /* 0: RTS速率 = PROT_DATARATE的配置                           */
    WLAN_RTS_RATE_SELECT_MODE_DESC, /* 1: RTS速率 = 硬件根据TX描述符计算出的值                    */
    WLAN_RTS_RATE_SELECT_MODE_MIN,  /* 2: RTS速率 = min(PROT_DATARATE,硬件根据TX描述符计算出的值) */
    WLAN_RTS_RATE_SELECT_MODE_MAX,  /* 3: RTS速率 = max(PROT_DATARATE,硬件根据TX描述符计算出的值) */

    WLAN_RTS_RATE_SELECT_MODE_BUTT
}wlan_rts_rate_select_mode_enum;

typedef enum
{
    WLAN_WITP_AUTH_OPEN_SYSTEM = 0,
    WLAN_WITP_AUTH_SHARED_KEY,
    WLAN_WITP_AUTH_FT,
    WLAN_WITP_AUTH_NETWORK_EAP,
    WLAN_WITP_AUTH_SAE,
    WLAN_WITP_AUTH_NUM,
    WLAN_WITP_AUTH_MAX = WLAN_WITP_AUTH_NUM - 1,
    WLAN_WITP_AUTH_AUTOMATIC,

    WLAN_WITP_AUTH_BUTT
}wlan_auth_alg_mode_enum;
typedef oal_uint8 wlan_auth_alg_mode_enum_uint8;

typedef enum
{
    /*
    *  注意: wlan_cipher_key_type_enum和hal_key_type_enum 值一致,
    *        如果硬件有改变，则应该在HAL 层封装
    */

    /* TBD:此处保持和协议规定额秘钥类型一致 */
    WLAN_KEY_TYPE_TX_GTK              = 0,  /* TX GTK */
    WLAN_KEY_TYPE_PTK                 = 1,  /* PTK */
    WLAN_KEY_TYPE_RX_GTK              = 2,  /* RX GTK */
    WLAN_KEY_TYPE_RX_GTK2             = 3,  /* RX GTK2 51不用 */
    WLAN_KEY_TYPE_BUTT
} wlan_cipher_key_type_enum;
typedef oal_uint8 wlan_cipher_key_type_enum_uint8;

typedef enum
{
    /*
    *  注意: wlan_key_origin_enum_uint8和hal_key_origin_enum_uint8 值一致,
    *        如果硬件有改变，则应该在HAL 层封装
    */

    WLAN_AUTH_KEY = 0,      /* Indicates that for this key, this STA is the authenticator */
    WLAN_SUPP_KEY = 1,      /* Indicates that for this key, this STA is the supplicant */

    WLAN_KEY_ORIGIN_BUTT,
}wlan_key_origin_enum;
typedef oal_uint8 wlan_key_origin_enum_uint8;

typedef enum
{
    NARROW_BW_10M = 0x80,
    NARROW_BW_5M = 0x81,
    NARROW_BW_1M = 0x82,
    NARROW_BW_BUTT
}mac_narrow_bw_enum;
typedef oal_uint8 mac_narrow_bw_enum_uint8;

typedef enum
{
    WLAN_ADDBA_MODE_AUTO,
    WLAN_ADDBA_MODE_MANUAL,

    WLAN_ADDBA_MODE_BUTT
}wlan_addba_mode_enum;
typedef oal_uint8 wlan_addba_mode_enum_uint8;


typedef enum
{
    /* 按照80211-2012/ 11ac-2013 协议 Table 8-99 Cipher suite selectors 定义 */
    WLAN_80211_CIPHER_SUITE_GROUP_CIPHER = 0,
    WLAN_80211_CIPHER_SUITE_WEP_40       = 1,
    WLAN_80211_CIPHER_SUITE_TKIP         = 2,
    WLAN_80211_CIPHER_SUITE_RSV          = 3,
    WLAN_80211_CIPHER_SUITE_NO_ENCRYP    = WLAN_80211_CIPHER_SUITE_RSV, /* 采用协议定义的保留值做不加密类型 */
    WLAN_80211_CIPHER_SUITE_CCMP         = 4,
    WLAN_80211_CIPHER_SUITE_WEP_104      = 5,
    WLAN_80211_CIPHER_SUITE_BIP          = 6,
    WLAN_80211_CIPHER_SUITE_GROUP_DENYD  = 7,
    WLAN_80211_CIPHER_SUITE_GCMP         = 8,             /* GCMP-128 default for a DMG STA */
    WLAN_80211_CIPHER_SUITE_GCMP_256     = 9,
    WLAN_80211_CIPHER_SUITE_CCMP_256     = 10,
    WLAN_80211_CIPHER_SUITE_BIP_GMAC_128 = 11,           /* BIP GMAC 128 */
    WLAN_80211_CIPHER_SUITE_BIP_GMAC_256 = 12,           /* BIP GMAC 256 */
    WLAN_80211_CIPHER_SUITE_BIP_CMAC_256 = 13,           /* BIP CMAC 256 */

    WLAN_80211_CIPHER_SUITE_WAPI         = 14,           /* 随意定，不影响11i即可 */
} wlan_ciper_protocol_type_enum;
typedef oal_uint8 wlan_ciper_protocol_type_enum_uint8;

/* 按照80211-2012 协议 Table 8-101 AKM suite selectors 定义 */
#define WLAN_AUTH_SUITE_RSV              0
#define WLAN_AUTH_SUITE_1X               1
#define WLAN_AUTH_SUITE_PSK              2
#define WLAN_AUTH_SUITE_FT_1X            3
#define WLAN_AUTH_SUITE_FT_PSK           4
#define WLAN_AUTH_SUITE_1X_SHA256        5
#define WLAN_AUTH_SUITE_PSK_SHA256       6
#define WLAN_AUTH_SUITE_TDLS             7
#define WLAN_AUTH_SUITE_SAE_SHA256       8
#define WLAN_AUTH_SUITE_FT_SHA256        9

typedef enum
{
    WITP_WPA_VERSION_1 = 1,
    WITP_WPA_VERSION_2 = 2,
#ifdef _PRE_WLAN_WEB_CMD_COMM
    WITP_WPA_VERSION_C = 3,
#endif
#ifdef _PRE_WLAN_FEATURE_WAPI
    WITP_WAPI_VERSION = 1 << 2,
#endif
}witp_wpa_versions_enum;
typedef oal_uint8 witp_wpa_versions_enum_uint8;


typedef struct
{
    wlan_cipher_key_type_enum_uint8          en_cipher_key_type;      /* 密钥ID/类型 */
    wlan_ciper_protocol_type_enum_uint8      en_cipher_protocol_type;
    oal_uint8                                uc_cipher_key_id;
    oal_uint8                                auc_resv[1];
}wlan_security_txop_params_stru;

/* 协议能力枚举 */
typedef enum
{
    WLAN_PROTOCOL_CAP_LEGACY,
    WLAN_PROTOCOL_CAP_HT,
    WLAN_PROTOCOL_CAP_VHT,
#ifdef _PRE_WLAN_FEATURE_11AX
    WLAN_PROTOCOL_CAP_HE,
#endif

    WLAN_PROTOCOL_CAP_BUTT,
}wlan_protocol_cap_enum;
typedef oal_uint8 wlan_protocol_cap_enum_uint8;

/* 频带能力枚举 */
typedef enum
{
    WLAN_BAND_CAP_2G,        /* 只支持2G */
    WLAN_BAND_CAP_5G,        /* 只支持5G */
    WLAN_BAND_CAP_2G_5G,     /* 支持2G 5G */

    WLAN_BAND_CAP_BUTT
}wlan_band_cap_enum;
typedef oal_uint8 wlan_band_cap_enum_uint8;

/* 带宽能力枚举 */
typedef enum
{
    WLAN_BW_CAP_20M,
    WLAN_BW_CAP_40M,
    WLAN_BW_CAP_80M,
    WLAN_BW_CAP_160M,
    WLAN_BW_CAP_80PLUS80,    /* 工作在80+80 */

    WLAN_BW_CAP_BUTT
}wlan_bw_cap_enum;
typedef oal_uint8 wlan_bw_cap_enum_uint8;

/* 调制方式枚举 */
typedef enum
{
    WLAN_MOD_DSSS,
    WLAN_MOD_OFDM,

    WLAN_MOD_BUTT
}wlan_mod_enum;
typedef oal_uint8 wlan_mod_enum_uint8;

typedef enum
{
   REGDOMAIN_FCC        = 0,
   REGDOMAIN_ETSI       = 1,
   REGDOMAIN_JAPAN      = 2,
   REGDOMAIN_COMMON     = 3,

   REGDOMAIN_COUNT
} regdomain_enum;
typedef oal_uint8 regdomain_enum_uint8;


/*****************************************************************************
  3.4 VHT枚举类型
*****************************************************************************/

typedef enum
{
    WLAN_VHT_MCS0,
    WLAN_VHT_MCS1,
    WLAN_VHT_MCS2,
    WLAN_VHT_MCS3,
    WLAN_VHT_MCS4,
    WLAN_VHT_MCS5,
    WLAN_VHT_MCS6,
    WLAN_VHT_MCS7,
    WLAN_VHT_MCS8,
    WLAN_VHT_MCS9,
#ifdef _PRE_WLAN_FEATURE_1024QAM
    WLAN_VHT_MCS10,
    WLAN_VHT_MCS11,
#endif

    WLAN_VHT_MCS_BUTT,
}wlan_vht_mcs_enum;
typedef oal_uint8 wlan_vht_mcs_enum_uint8;

/*
    复用1101定义的顺序
    TBD，和周赟讨论后，修正速率的先后顺序
*/
typedef enum
{
    WLAN_LEGACY_11b_RESERVED1       = 0,
    WLAN_SHORT_11b_2M_BPS           = 1,
    WLAN_SHORT_11b_5_HALF_M_BPS     = 2,
    WLAN_SHORT_11b_11_M_BPS         = 3,

    WLAN_LONG_11b_1_M_BPS           = 4,
    WLAN_LONG_11b_2_M_BPS           = 5,
    WLAN_LONG_11b_5_HALF_M_BPS      = 6,
    WLAN_LONG_11b_11_M_BPS          = 7,

    WLAN_LEGACY_OFDM_48M_BPS        = 8,
    WLAN_LEGACY_OFDM_24M_BPS        = 9,
    WLAN_LEGACY_OFDM_12M_BPS        = 10,
    WLAN_LEGACY_OFDM_6M_BPS         = 11,
    WLAN_LEGACY_OFDM_54M_BPS        = 12,
    WLAN_LEGACY_OFDM_36M_BPS        = 13,
    WLAN_LEGACY_OFDM_18M_BPS        = 14,
    WLAN_LEGACY_OFDM_9M_BPS         = 15,

    WLAN_LEGACY_RATE_VALUE_BUTT
}wlan_legacy_rate_value_enum;
typedef oal_uint8 wlan_legacy_rate_value_enum_uint8;

/* WIFI协议类型定义 */
typedef enum
{
    WLAN_LEGACY_11A_MODE            = 0,    /* 11a, 5G, OFDM */
    WLAN_LEGACY_11B_MODE            = 1,    /* 11b, 2.4G */
    WLAN_LEGACY_11G_MODE            = 2,    /* 旧的11g only已废弃, 2.4G, OFDM */
    WLAN_MIXED_ONE_11G_MODE         = 3,    /* 11bg, 2.4G */
    WLAN_MIXED_TWO_11G_MODE         = 4,    /* 11g only, 2.4G */
    WLAN_HT_MODE                    = 5,    /* 11n(11bgn或者11an，根据频段判断) */
    WLAN_VHT_MODE                   = 6,    /* 11ac */
    WLAN_HT_ONLY_MODE               = 7,    /* 11n only mode,只有带HT的设备才可以接入 */
    WLAN_VHT_ONLY_MODE              = 8,    /* 11ac only mode 只有带VHT的设备才可以接入 */
    WLAN_HT_11G_MODE                = 9,    /* 11ng,不包括11b*/
#if defined(_PRE_WLAN_FEATURE_11AX) || defined(_PRE_WLAN_FEATURE_11AX_ROM)
    WLAN_HE_MODE                    = 10,   /*11ax*/
#endif

    WLAN_PROTOCOL_BUTT
}wlan_protocol_enum;
typedef oal_uint8 wlan_protocol_enum_uint8;

/* 重要:代表VAP的preamble协议能力的使用该枚举，0表示long preamble; 1表示short preamble */
typedef enum
{
    WLAN_LEGACY_11B_MIB_LONG_PREAMBLE    = 0,
    WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE   = 1,
}wlan_11b_mib_preamble_enum;
typedef oal_uint8 wlan_11b_mib_preamble_enum_uint8;

/* 重要:仅限描述符接口使用(表示发送该帧使用的pramble类型)，0表示short preamble; 1表示long preamble */
typedef enum
{
    WLAN_LEGACY_11B_DSCR_SHORT_PREAMBLE  = 0,
    WLAN_LEGACY_11B_DSCR_LONG_PREAMBLE   = 1,

    WLAN_LEGACY_11b_PREAMBLE_BUTT
}wlan_11b_dscr_preamble_enum;
typedef oal_uint8 wlan_11b_dscr_preamble_enum_uint8;

/*****************************************************************************
  3.3 HT枚举类型
*****************************************************************************/
typedef enum
{
    WLAN_BAND_WIDTH_20M                 = 0,
    WLAN_BAND_WIDTH_40PLUS,                     /* 从20信道+1 */
    WLAN_BAND_WIDTH_40MINUS,                    /* 从20信道-1 */
    WLAN_BAND_WIDTH_80PLUSPLUS,                 /* 从20信道+1, 从40信道+1 */
    WLAN_BAND_WIDTH_80PLUSMINUS,                /* 从20信道+1, 从40信道-1 */
    WLAN_BAND_WIDTH_80MINUSPLUS,                /* 从20信道-1, 从40信道+1 */
    WLAN_BAND_WIDTH_80MINUSMINUS,               /* 从20信道-1, 从40信道-1 */
#ifdef _PRE_WLAN_FEATURE_160M
    WLAN_BAND_WIDTH_160PLUSPLUSPLUS,            /* 从20信道+1, 从40信道+1, 从80信道+1 */
    WLAN_BAND_WIDTH_160PLUSPLUSMINUS,           /* 从20信道+1, 从40信道+1, 从80信道-1 */
    WLAN_BAND_WIDTH_160PLUSMINUSPLUS,           /* 从20信道+1, 从40信道-1, 从80信道+1 */
    WLAN_BAND_WIDTH_160PLUSMINUSMINUS,          /* 从20信道+1, 从40信道-1, 从80信道-1 */
    WLAN_BAND_WIDTH_160MINUSPLUSPLUS,           /* 从20信道-1, 从40信道+1, 从80信道+1 */
    WLAN_BAND_WIDTH_160MINUSPLUSMINUS,          /* 从20信道-1, 从40信道+1, 从80信道-1 */
    WLAN_BAND_WIDTH_160MINUSMINUSPLUS,          /* 从20信道-1, 从40信道-1, 从80信道+1 */
    WLAN_BAND_WIDTH_160MINUSMINUSMINUS,         /* 从20信道-1, 从40信道-1, 从80信道-1 */
#endif
    WLAN_BAND_WIDTH_40M,
    WLAN_BAND_WIDTH_80M,
    WLAN_BAND_WIDTH_BUTT
}wlan_channel_bandwidth_enum;
typedef oal_uint8 wlan_channel_bandwidth_enum_uint8;

typedef enum
{
    WLAN_CH_SWITCH_DONE     = 0,   /* 信道切换已经完成，AP在新信道运行 */
    WLAN_CH_SWITCH_STATUS_1 = 1,   /* AP还在当前信道，准备进行信道切换(发送CSA帧/IE) */
    WLAN_CH_SWITCH_STATUS_2 = 2,

    WLAN_CH_SWITCH_BUTT
}wlan_ch_switch_status_enum;
typedef oal_uint8 wlan_ch_switch_status_enum_uint8;
typedef enum
  {
    WLAN_AP_CHIP_OUI_NORMAL     = 0,
    WLAN_AP_CHIP_OUI_RALINK     = 1,   /* 芯片厂商为RALINK */
    WLAN_AP_CHIP_OUI_RALINK1    = 2,
    WLAN_AP_CHIP_OUI_ATHEROS    = 3,   /* 芯片厂商为ATHEROS */
    WLAN_AP_CHIP_OUI_BCM        = 4,   /* 芯片厂商为BROADCOM */

    WLAN_AP_CHIP_OUI_BUTT
}wlan_ap_chip_oui_enum;
typedef oal_uint8 wlan_ap_chip_oui_enum_uint8;


typedef enum
{
    WLAN_CSA_MODE_TX_ENABLE = 0,
    WLAN_CSA_MODE_TX_DISABLE,

    WLAN_CSA_MODE_TX_BUTT
}wlan_csa_mode_tx_enum;
typedef oal_uint8 wlan_csa_mode_tx_enum_uint8;


typedef enum
{
    WLAN_BW_SWITCH_DONE     = 0,    /* 频宽切换已完成 */
    WLAN_BW_SWITCH_40_TO_20 = 1,    /* 从40MHz带宽切换至20MHz带宽 */
    WLAN_BW_SWITCH_20_TO_40 = 2,    /* 从20MHz带宽切换至40MHz带宽 */

    /* 后续添加 */

    WLAN_BW_SWITCH_BUTT
}wlan_bw_switch_status_enum;
typedef oal_uint8 wlan_bw_switch_status_enum_uint8;


typedef enum
{
    WLAN_BAND_ASSEMBLE_20M                   = 0,

    WLAN_BAND_ASSEMBLE_40M                   = 4,
    WLAN_BAND_ASSEMBLE_40M_DUP               = 5,

    WLAN_BAND_ASSEMBLE_80M                   = 8,
    WLAN_BAND_ASSEMBLE_80M_DUP               = 9,

    WLAN_BAND_ASSEMBLE_160M                  = 12,
    WLAN_BAND_ASSEMBLE_160M_DUP              = 13,

    WLAN_BAND_ASSEMBLE_80M_80M               = 15,

    WLAN_BAND_ASSEMBLE_AUTO,

    WLAN_BAND_ASSEMBLE_BUTT
}hal_channel_assemble_enum;
typedef oal_uint8 hal_channel_assemble_enum_uint8;


typedef enum
{
    WLAN_HT_MIXED_PREAMBLE          = 0,
    WLAN_HT_GF_PREAMBLE             = 1,

    WLAN_HT_PREAMBLE_BUTT
}wlan_ht_preamble_enum;
typedef oal_uint8 wlan_ht_preamble_enum_uint8;

typedef enum
{
    WLAN_HT_MCS0,
    WLAN_HT_MCS1,
    WLAN_HT_MCS2,
    WLAN_HT_MCS3,
    WLAN_HT_MCS4,
    WLAN_HT_MCS5,
    WLAN_HT_MCS6,
    WLAN_HT_MCS7,
    WLAN_HT_MCS8,
    WLAN_HT_MCS9,
    WLAN_HT_MCS10,
    WLAN_HT_MCS11,
    WLAN_HT_MCS12,
    WLAN_HT_MCS13,
    WLAN_HT_MCS14,
    WLAN_HT_MCS15,

    WLAN_HT_MCS_BUTT
}wlan_ht_mcs_enum;
typedef oal_uint8 wlan_ht_mcs_enum_uint8;
#define WLAN_HT_MCS32   32
#define WLAN_MIN_MPDU_LEN_FOR_MCS32   12
#define WLAN_MIN_MPDU_LEN_FOR_MCS32_SHORTGI   13

/* 空间流定义 */
#define WLAN_SINGLE_NSS                 0
#define WLAN_DOUBLE_NSS                 1
#define WLAN_TRIPLE_NSS                 2
#define WLAN_FOUR_NSS                   3
#define WLAN_NSS_LIMIT                   4
/* 因为要用作预编译，所以由枚举改成宏，为了便于理解，下面的类型转义先不变 */
typedef oal_uint8 wlan_nss_enum_uint8;

typedef enum
{
    WLAN_HT_NON_STBC                   = 0,
    WLAN_HT_ADD_ONE_NTS                = 1,
    WLAN_HT_ADD_TWO_NTS                = 2,

    WLAN_HT_STBC_BUTT
}wlan_ht_stbc_enum;
typedef oal_uint8 wlan_ht_stbc_enum_uint8;

typedef enum
{
    WLAN_NO_SOUNDING                = 0,
    WLAN_NDP_SOUNDING               = 1,
    WLAN_STAGGERED_SOUNDING         = 2,
    WLAN_LEGACY_SOUNDING            = 3,

    WLAN_SOUNDING_BUTT
}wlan_sounding_enum;
typedef oal_uint8 wlan_sounding_enum_uint8;

typedef struct
{
    oal_uint8                               uc_group_id;      /* group_id   */
    oal_uint8                               uc_txop_ps_not_allowed;
    oal_uint16                              us_partial_aid;   /* partial_aid */
}wlan_groupid_partial_aid_stru;


/* channel结构体 */
typedef struct
{
    oal_uint8                           uc_chan_number;     /* 主20MHz信道号 */
    wlan_channel_band_enum_uint8        en_band;            /* 频段 */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;       /* 带宽模式 */
    oal_uint8                           uc_chan_idx;        /* 信道索引号 */
}mac_channel_stru;

/*****************************************************************************
  3.4 算法宏,枚举类型
*****************************************************************************/
typedef enum
{
    WLAN_NON_TXBF                   = 0,
    WLAN_EXPLICIT_TXBF              = 1,
    WLAN_LEGACY_TXBF                = 2,

    WLAN_TXBF_BUTT
}wlan_txbf_enum;
typedef oal_uint8 wlan_txbf_enum_uint8;

/* TPC工作模式 */
typedef enum
{
    WLAN_TPC_WORK_MODE_DISABLE        = 0,   /* 禁用TPC动态调整功率模式: 直接采用固定功率模式，数据帧的Data0采用配置的, Data1~3以及管理帧、控制帧都用最大功率 */
    WLAN_TPC_WORK_MODE_ENABLE         = 1,   /* 自适应功率模式: 数据帧的Data0采用自适应功率, Data1~3以及管理帧、控制帧都用最大功率 */

    WLAN_TPC_WORK_MODE_BUTT
}wlan_tpc_work_mode_enum;
typedef oal_uint8 wlan_tpc_mode_enum_uint8;

/* CCA_OPT工作模式 */
#define WLAN_CCA_OPT_DISABLE                0   /* 不做任何优化 */
#define WLAN_CCA_OPT_ENABLE                 1   /* 不做同频识别的EDCA优化 */

/* EDCA_OPT STA模式下工作模式 */
#define WLAN_EDCA_OPT_STA_DISABLE           0   /* 不做任何优化 */
#define WLAN_EDCA_OPT_STA_ENABLE            1   /* 不做同频识别的EDCA优化 */

/* EDCA_OPT AP模式下工作模式 */
#define WLAN_EDCA_OPT_AP_EN_DISABLE         0   /* 不做任何优化 */
#define WLAN_EDCA_OPT_AP_EN_DEFAULT         1   /* 不做同频识别的EDCA优化 */
#define WLAN_EDCA_OPT_AP_EN_WITH_COCH       2   /* 带有同频识别的优化 */

/* 弱干扰免疫算法使能模式 */
#define WLAN_ANTI_INTF_EN_OFF               0   /* 算法关闭 */
#define WLAN_ANTI_INTF_EN_ON                1   /* 算法打开 */
#define WLAN_ANTI_INTF_EN_PROBE             2   /* 探测机制进行算法开/关 */

/* 动态byass外置LNA使能模式 */
#define WLAN_EXTLNA_BYPASS_EN_OFF               0   /* 动态bypass外置LNA关 */
#define WLAN_EXTLNA_BYPASS_EN_ON                1   /* 动态bypass外置LNA开 */
#define WLAN_EXTLNA_BYPASS_EN_FORCE             2   /* 动态bypass外置LNA强制开 */

/* DFS使能模式 */
#define WLAN_DFS_EN_OFF               0   /* 算法关闭 */
#define WLAN_DFS_EN_ON                1   /* 算法打开,检测到雷达切换信道 */

/*****************************************************************************
  3.5 WME枚举类型
*****************************************************************************/

/*WMM枚举类型放入平台*/

/* 帧类型 (2-bit) */
typedef enum
{
    WLAN_MANAGEMENT            = 0,
    WLAN_CONTROL               = 1,
    WLAN_DATA_BASICTYPE        = 2,
    WLAN_RESERVED              = 3,

    WLAN_FRAME_TYPE_BUTT
} wlan_frame_type_enum;
typedef oal_uint8 wlan_frame_type_enum_uint8;

/* 帧子类型 (4-bit) */
/* 管理帧子类型放入平台SPEC */

/* 控制帧帧子类型 */
typedef enum
{
    /* 0~6 reserved */
    WLAN_HE_TRIG_FRAME          = 2,     /* 0010 */
    WLAN_VHT_NDPA               = 5,     /* 0101 */
    WLAN_CONTROL_WRAPPER        = 7,
    WLAN_BLOCKACK_REQ           = 8,
    WLAN_BLOCKACK               = 9,
    WLAN_PS_POLL                = 10,
    WLAN_RTS                    = 11,
    WLAN_CTS                    = 12,
    WLAN_ACK                    = 13,
    WLAN_CF_END                 = 14,
    WLAN_CF_END_CF_ACK          = 15,

    WLAN_CONTROL_SUBTYPE_BUTT   = 16,
}wlan_frame_control_subtype_enum;

/* 数据帧子类型 */
typedef enum
{
    WLAN_DATA                   = 0,
    WLAN_DATA_CF_ACK            = 1,
    WLAN_DATA_CF_POLL           = 2,
    WLAN_DATA_CF_ACK_POLL       = 3,
    WLAN_NULL_FRAME             = 4,
    WLAN_CF_ACK                 = 5,
    WLAN_CF_POLL                = 6,
    WLAN_CF_ACK_POLL            = 7,
    WLAN_QOS_DATA               = 8,
    WLAN_QOS_DATA_CF_ACK        = 9,
    WLAN_QOS_DATA_CF_POLL       = 10,
    WLAN_QOS_DATA_CF_ACK_POLL   = 11,
    WLAN_QOS_NULL_FRAME         = 12,
    WLAN_DATA_SUBTYPE_RESV1     = 13,
    WLAN_QOS_CF_POLL            = 14,
    WLAN_QOS_CF_ACK_POLL        = 15,

    WLAN_DATA_SUBTYPE_MGMT      = 16,
}wlan_frame_data_subtype_enum;

/* ACK类型定义 */
typedef enum
{
    WLAN_TX_NORMAL_ACK = 0,
    WLAN_TX_NO_ACK,
    WLAN_TX_NO_EXPLICIT_ACK,
    WLAN_TX_BLOCK_ACK,

    WLAN_TX_NUM_ACK_BUTT
}wlan_tx_ack_policy_enum;
typedef oal_uint8   wlan_tx_ack_policy_enum_uint8;

/* Trig帧帧子类型 */
typedef enum
{
    WLAN_HE_BASIC_TRIG              = 0,
    WLAN_BEAM_REPORT_POLL           = 1,
    WLAN_MU_BAR                     = 2,
    WLAN_MU_RTS                     = 3,
    WLAN_BUFFER_STATUS_REPORT_POLL  = 4,
    WLAN_GCR_MU_BAR                 = 5,
    WLAN_BW_QUERY_REPORT_POLL       = 6,
    WLAN_NDP_FEEDBACK_REPORT_POLL   = 7,

    WLAN_HE_TRIG_TYPE_BUTT
}wlan_frame_trig_type_enum;

/*****************************************************************************
  3.6 信道枚举
*****************************************************************************/

/* 信道编码方式 */
typedef enum
{
    WLAN_BCC_CODE                   = 0,
    WLAN_LDPC_CODE                  = 1,
    WLAN_CHANNEL_CODE_BUTT
}wlan_channel_code_enum;
typedef oal_uint8 wlan_channel_code_enum_uint8;

/* 扫描类型 */
typedef enum
{
    WLAN_SCAN_TYPE_PASSIVE       = 0,
    WLAN_SCAN_TYPE_ACTIVE        = 1,

    WLAN_SCAN_TYPE_BUTT
}wlan_scan_type_enum;
typedef oal_uint8 wlan_scan_type_enum_uint8;

/* 扫描模式 */
typedef enum
{
    WLAN_SCAN_MODE_FOREGROUND     = 0,          /* 前景扫描不分AP/STA(即初始扫描，连续式) */
    WLAN_SCAN_MODE_BACKGROUND_STA = 1,      /* STA背景扫描 */
    WLAN_SCAN_MODE_BACKGROUND_AP  = 2,       /* AP背景扫描(间隔式) */
    WLAN_SCAN_MODE_BACKGROUND_OBSS = 3,     /* 20/40MHz共存的obss扫描 */
    WLAN_SCAN_MODE_BACKGROUND_CCA  = 4,
    WLAN_SCAN_MODE_BACKGROUND_PNO  = 5,      /* PNO调度扫描 */
    WLAN_SCAN_MODE_RRM_BEACON_REQ  = 6,
    WLAN_SCAN_MODE_BACKGROUND_CSA  = 7,      /* 信道切换扫描 */
    WLAN_SCAN_MODE_BACKGROUND_HILINK = 8,   /* hilink扫描未关联用户 */
    WLAN_SCAN_MODE_FTM_REQ      = 9,
    WLAN_SCAN_MODE_GNSS_SCAN    = 10,
    WLAN_SCAN_MODE_ROAM_SCAN    = 11,

    WLAN_SCAN_MODE_BUTT
} wlan_scan_mode_enum;
typedef oal_uint8 wlan_scan_mode_enum_uint8;

/*Android P 增加，是否启动并发扫描标志位*/
typedef enum
{
    WLAN_SCAN_FLAG_LOW_PRIORITY  = 0,
    WLAN_SCAN_FLAG_LOW_FLUSH     = 1,
    WLAN_SCAN_FLAG_AP            = 2,
    WLAN_SCAN_FLAG_RANDOM_ADDR   = 3,
    WLAN_SCAN_FLAG_LOW_SPAN      = 4,/*并发扫描*/
    WLAN_SCAN_FLAG_LOW_POWER     = 5,
    WLAN_SCAN_FLAG_HIFH_ACCURACY = 6,/*顺序扫描,非并发 */
    WLAN_SCAN_FLAG_BUTT
} wlan_scan_flag_enum;

/*内核定义 flag标志位*/
typedef enum
{
    WLAN_NL80211_SCAN_FLAG_BIT_LOW_PRIORITY   = 0,
    WLAN_NL80211_SCAN_FLAG_BIT_FLUSH          = 1,
    WLAN_NL80211_SCAN_FLAG_BIT_AP             = 2,
    WLAN_NL80211_SCAN_FLAG_BIT_RANDOM_ADDR    = 3,
    WLAN_NL80211_SCAN_FLAG_BIT_LOW_SPAN       = 8,
    WLAN_NL80211_SCAN_FLAG_BIT_LOW_POWER      = 9,
    WLAN_NL80211_SCAN_FLAG_BIT_HIGH_ACCURACY  = 10,
    WLAN_NL80211_SCAN_FLAG_BIT_BUTT
}wlan_nl80211_scan_flag_bit_enum;

/* 扫描结果枚举 */
typedef enum
{
    WLAN_SCAN_EVENT_COMPLETE    = 0,
    WLAN_SCAN_EVENT_FAILED,
    WLAN_SCAN_EVENT_ABORT,  /* 强制终止，比如卸载 */
    WLAN_SCAN_EVENT_TIMEOUT,
    WLAN_SCAN_EVENT_BUTT
}dmac_scan_event_enum;
typedef oal_uint8 wlan_scan_event_enum_uint8;

typedef enum
{
    WLAN_LEGACY_VAP_MODE     = 0,    /* 非P2P设备 */
    WLAN_P2P_GO_MODE         = 1,    /* P2P_GO */
    WLAN_P2P_DEV_MODE        = 2,    /* P2P_Device */
    WLAN_P2P_CL_MODE         = 3,    /* P2P_CL */

    WLAN_P2P_BUTT
}wlan_p2p_mode_enum;
typedef oal_uint8 wlan_p2p_mode_enum_uint8;

/*****************************************************************************
  3.7 加密枚举
*****************************************************************************/

/* pmf的能力 */
typedef enum
{
    MAC_PMF_DISABLED  = 0, /* 不支持pmf能力 */
    MAC_PMF_ENABLED,       /* 支持pmf能力，且不强制 */
    MAC_PMF_REQUIRED,       /* 严格执行pmf能力 */

    MAC_PMF_BUTT
}wlan_pmf_cap_status;
typedef oal_uint8 wlan_pmf_cap_status_uint8;

/* 用户距离状态 */
typedef enum
{
    WLAN_DISTANCE_NEAR       = 0,
    WLAN_DISTANCE_NORMAL     = 1,
    WLAN_DISTANCE_FAR        = 2,

    WLAN_DISTANCE_BUTT
}wlan_user_distance_enum;
typedef oal_uint8 wlan_user_distance_enum_uint8;

/*****************************************************************************
  3.8 linkloss场景枚举
*****************************************************************************/

/*linkloss场景枚举*/
typedef enum
{
    WLAN_LINKLOSS_MODE_BT = 0,
    WLAN_LINKLOSS_MODE_DBAC,
    WLAN_LINKLOSS_MODE_NORMAL,

    WLAN_LINKLOSS_MODE_BUTT
}wlan_linkloss_mode_enum;
typedef oal_uint8 wlan_linkloss_mode_enum_uint8;

typedef enum
{
    WALN_LINKLOSS_SCAN_SWITCH_CHAN_DISABLE = 0,
    WALN_LINKLOSS_SCAN_SWITCH_CHAN_EN      = 1,

    WALN_LINKLOSS_SCAN_SWITCH_CHAN_BUTT
}wlan_linkloss_scan_switch_chan_enum;
typedef oal_uint8 wlan_linkloss_scan_switch_chan_enum_uint8;


/*****************************************************************************
  3.9 roc场景枚举
*****************************************************************************/

typedef enum {
    IEEE80211_ROC_TYPE_NORMAL  = 0,
    IEEE80211_ROC_TYPE_MGMT_TX,
    IEEE80211_ROC_TYPE_BUTT,
}wlan_ieee80211_roc_type;
typedef oal_uint8 wlan_ieee80211_roc_type_uint8;

/*****************************************************************************
  3.10 roam场景枚举
*****************************************************************************/
/* 漫游切换状态 */
typedef enum
{
    WLAN_ROAM_MAIN_BAND_STATE_2TO2          = 0,
    WLAN_ROAM_MAIN_BAND_STATE_5TO2          = 1,
    WLAN_ROAM_MAIN_BAND_STATE_2TO5          = 2,
    WLAN_ROAM_MAIN_BAND_STATE_5TO5          = 3,

    WLAN_ROAM_MAIN_BAND_STATE_BUTT
}wlan_roam_main_band_state_enum;
typedef oal_uint8  wlan_roam_main_band_state_enum_uint8;

/*****************************************************************************
  3.10 m2s切换枚举
*****************************************************************************/
/* mimo-siso切换tpye */
typedef enum
{
    WLAN_M2S_TYPE_SW = 0,
    WLAN_M2S_TYPE_HW = 1,

    WLAN_M2S_TYPE_BUTT,
}wlan_m2s_tpye_enum;
typedef oal_uint8 wlan_m2s_type_enum_uint8;

/*ldac m2s  门限场景枚举*/
typedef enum
{
    WLAN_M2S_LDAC_RSSI_TO_SISO = 0,
    WLAN_M2S_LDAC_RSSI_TO_MIMO,

    WLAN_M2S_LDAC_RSSI_BUTT
}wlan_m2s_ldac_rssi_th_enum;
typedef oal_uint8 wlan_m2s_ldac_rssi_th_enum_uint8;

/* mimo-siso切换mode */
typedef enum
{
    WLAN_M2S_TRIGGER_MODE_DBDC      = BIT0,
    WLAN_M2S_TRIGGER_MODE_FAST_SCAN = BIT1,
    WLAN_M2S_TRIGGER_MODE_RSSI      = BIT2,
    WLAN_M2S_TRIGGER_MODE_BTCOEX    = BIT3,
    WLAN_M2S_TRIGGER_MODE_COMMAND   = BIT4,
    WLAN_M2S_TRIGGER_MODE_TEST      = BIT5,
    WLAN_M2S_TRIGGER_MODE_CUSTOM    = BIT6,   /* 定制化优先级高，只能被蓝牙打断 */
    WLAN_M2S_TRIGGER_MODE_SPEC      = BIT7,   /* spec优先级最高，不允许打断 */

    WLAN_M2S_TRIGGER_MODE_BUTT,
}wlan_m2s_trigger_mode_enum;
typedef oal_uint8 wlan_m2s_trigger_mode_enum_uint8;

typedef enum
{
    WLAN_OFDM_ACK_CTS_TYPE_24M = 0,
    WLAN_OFDM_ACK_CTS_TYPE_36M = 1,
    WLAN_OFDM_ACK_CTS_TYPE_48M = 2,
    WLAN_OFDM_ACK_CTS_TYPE_54M = 3,
    WLAN_OFDM_ACK_CTS_TYPE_BUTT,
}wlan_ofdm_ack_cts_type_enum;
typedef oal_uint8 wlan_ofdm_ack_cts_type_enum_uint8;

/*****************************************************************************
  3.11 m2s切换枚举
*****************************************************************************/
typedef enum
{
    WLAN_SPECIAL_FRM_RTS,
    WLAN_SPECIAL_FRM_ONE_PKT,
    WLAN_SPECIAL_FRM_ABORT_SELFCTS,
    WLAN_SPECIAL_FRM_ABORT_CFEND,
    WLAN_SPECIAL_FRM_CFEND,
    WLAN_SPECIAL_FRM_NDP,
    WLAN_SPECIAL_FRM_VHT_REPORT,
    WLAN_SPECIAL_FRM_ABORT_NULL_DATA,
    WLAN_SPECIAL_FRM_BUTT,
}wlan_special_frm_enum;
typedef oal_uint8 wlan_special_frm_enum_uint8;


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
    oal_uint32                      ul_cipher;                      /*加密类型*/
    oal_uint32                      ul_key_len;                     /*密钥长*/
    oal_uint32                      ul_seq_len;                     /*sequnece长*/
    oal_uint8                       auc_key[WLAN_WPA_KEY_LEN];      /*密钥*/
    oal_uint8                       auc_seq[WLAN_WPA_SEQ_LEN];      /*sequence*/
}wlan_priv_key_param_stru;

/* DMAC SCAN 信道统计测量结果结构体 */
typedef struct
{
    oal_uint8   uc_channel_number;      /* 信道号 */
    oal_uint8   uc_stats_valid;
    oal_uint8   uc_stats_cnt;           /* 信道繁忙度统计次数 */
    oal_uint8   uc_free_power_cnt;      /* 信道空闲功率 */

    /* PHY信道测量统计 */
    oal_uint8   uc_bandwidth_mode;
    oal_uint8   auc_resv[1];
    oal_int16   s_free_power_stats_20M;
    oal_int16   s_free_power_stats_40M;
    oal_int16   s_free_power_stats_80M;

    /* MAC信道测量统计 */
    oal_uint32  ul_total_stats_time_us;
    oal_uint32  ul_total_free_time_20M_us;
    oal_uint32  ul_total_free_time_40M_us;
    oal_uint32  ul_total_free_time_80M_us;
    oal_uint32  ul_total_send_time_us;
    oal_uint32  ul_total_recv_time_us;

    oal_uint8   uc_radar_detected;  // FIXME: feed value
    oal_uint8   uc_radar_bw;
    oal_uint8   uc_radar_type;
    oal_uint8   uc_radar_freq_offset;

    oal_int16   s_free_power_stats_160M;
    oal_uint16  us_phy_total_stats_time_ms;
}wlan_scan_chan_stats_stru;

typedef struct
{
    oal_uint16  us_chan_ratio_20M;
    oal_uint16  us_chan_ratio_40M;
    oal_uint16  us_chan_ratio_80M;
    oal_int8    c_free_power_20M;
    oal_int8    c_free_power_40M;
    oal_int8    c_free_power_80M;
    oal_uint8   auc_resv[3];
    oal_uint8   _rom[4];
}wlan_chan_ratio_stru;

/* m2s触发条件 */
typedef struct
{
    oal_uint8   bit_dbdc         : 1,        /* dbdc触发 */
                bit_fast_on      : 1,        /* 并发扫描触发 */
                bit_rssi_snr     : 1,        /* rssi/snr触发 */
                bit_btcoex       : 1,        /* btcoex触发 */
                bit_command      : 1,        /* 上层命令触发 */
                bit_test         : 1,        /* 测试命令触发 */
                bit_custom       : 1,        /* 定制化触发 */
                bit_spec         : 1;        /* RF规格触发 */
}wlan_m2s_mode_stru;

/* action帧发送类型枚举 */
typedef enum
{
    WLAN_M2S_ACTION_TYPE_SMPS      = 0,         /* action采用smps */
    WLAN_M2S_ACTION_TYPE_OPMODE    = 1,         /* action采用opmode */
    WLAN_M2S_ACTION_TYPE_NONE      = 2,         /* 切换不发action帧 */

    WLAN_M2S_ACTION_TYPE_BUTT,
}wlan_m2s_action_type_enum;
typedef oal_uint8 wlan_m2s_action_type_enum_uint8;

typedef struct
{
    oal_uint8 auc_user_mac_addr[WLAN_MAC_ADDR_LEN];           /* 切换对应的AP MAC地址 */
    wlan_m2s_action_type_enum_uint8  en_action_type;          /* 切换对应需要发送的action帧类型，需要和vap下ori交互处理 */
    oal_bool_enum_uint8   en_m2s_result;   /* 切换是否符合初始action帧，作为vap切换成功失败标记 */
} wlan_m2s_mgr_vap_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_uint8  wlan_hdr_get_frame_type(oal_uint8 *puc_header)
{
    return ((puc_header[0] & (0x0c)) >> 2);
}

/*****************************************************************************
  10 函数声明
*****************************************************************************/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of wlan_types.h */
