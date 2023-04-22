
#ifndef __HAL_COMMOM_OPS_H__
#define __HAL_COMMOM_OPS_H__


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "frw_ext_if.h"


/*****************************************************************************/
/*****************************************************************************/
/*                        HI1102 产品宏定义、枚举                            */
/*****************************************************************************/
/*****************************************************************************/
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)) || ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
#define HAL_PUBLIC_HOOK_FUNC(_func) \
         hi1102##_func
#endif
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
#define HAL_PUBLIC_HOOK_FUNC(_func) \
         hi1103##_func
#endif


#define HAL_RX_DSCR_GET_SW_ADDR(_addr)     hal_rx_dscr_get_sw_addr(_addr)   /* 一个字节中包含的bit数目 */
#define HAL_RX_DSCR_GET_REAL_ADDR(_addr)   hal_rx_dscr_get_real_addr(_addr)   /* 一个字节中包含的bit数目 */

#define HAL_MAX_AP_NUM                     2      /* HAL AP个数 */
#define HAL_MAX_STA_NUM                    3      /* HAL STA个数 */
#define HAL_MAX_VAP_NUM                    (HAL_MAX_AP_NUM + HAL_MAX_STA_NUM)  /* HAL VAP???? */

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
#define HAL_VAP_ID_IS_VALID(_vap_id)      ((_vap_id == 0) || (_vap_id == 1) || (_vap_id == 4) || (_vap_id == 5) || (_vap_id == 6))
#elif ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
#define HAL_VAP_ID_IS_VALID(_vap_id)      ((_vap_id == 0) || (_vap_id == 1) || (_vap_id == 2) || (_vap_id == 3) || (_vap_id == 4))
#else
#define HAL_VAP_ID_IS_VALID(_vap_id)      (_vap_id)
#endif

/*****************************************************************************
  3 枚举
*****************************************************************************/
typedef enum
{
    HAL_TX_QUEUE_BE        = 0,        /* 尽力而为业务 */
    HAL_TX_QUEUE_BK        = 1,        /* 背景业务 */
    HAL_TX_QUEUE_VI        = 2,        /* 视频业务 */
    HAL_TX_QUEUE_VO        = 3,        /* 语音业务 */
    HAL_TX_QUEUE_HI        = 4,        /* 高优先级队列(管理帧/控制帧用此队列) */
#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
    HAL_TX_QUEUE_MC        = 5,        /* 组播帧发送队列 */
#endif
    HAL_TX_QUEUE_BUTT
}hal_tx_queue_type_enum;
typedef oal_uint8 hal_tx_queue_type_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
/* sw preempt机制下蓝牙业务状态，a2dp|transfer  page|inquiry 或者  both */
typedef enum
{
    HAL_BTCOEX_PS_STATUE_ACL       = 1,   /* only a2dp|数传 BIT0 */
    HAL_BTCOEX_PS_STATUE_PAGE_INQ  = 2,   /* only  page|inquiry BIT1 */
    HAL_BTCOEX_PS_STATUE_PAGE_ACL  = 3,   /* both a2dp|数传 and page|inquiry BIT0|BIT1 */
    HAL_BTCOEX_PS_STATUE_LDAC      = 4,   /* only ldac BIT2 */
    HAL_BTCOEX_PS_STATUE_LDAC_ACL  = 5,   /* ldac and a2dp|数传 BIT2|BIT0 */
    HAL_BTCOEX_PS_STATUE_LDAC_PAGE = 6,   /* ldac and page|inquiry BIT2|BIT1 */
    HAL_BTCOEX_PS_STATUE_TRIPLE    = 7,   /* ldac and page|inquiry and a2dp|数传 BIT2|BIT1|BIT0 */

    HAL_BTCOEX_PS_STATUE_BUTT
}hal_btcoex_ps_status_enum;
typedef oal_uint8 hal_btcoex_ps_status_enum_uint8;

typedef enum
{
    HAL_BTCOEX_HW_POWSAVE_NOFRAME   = 0,
    HAL_BTCOEX_HW_POWSAVE_SELFCTS   = 1,
    HAL_BTCOEX_HW_POWSAVE_NULLDATA  = 2,
    HAL_BTCOEX_HW_POWSAVE_QOSNULL   = 3,

    HAL_BTCOEX_HW_POWSAVE_BUTT
} hal_coex_hw_preempt_mode_enum;
typedef oal_uint8 hal_coex_hw_preempt_mode_enum_uint8;

typedef enum
{
    HAL_BTCOEX_SW_POWSAVE_IDLE       = 0,
    HAL_BTCOEX_SW_POWSAVE_WORK       = 1,
    HAL_BTCOEX_SW_POWSAVE_TIMEOUT    = 2,
    HAL_BTCOEX_SW_POWSAVE_SCAN       = 3,
    HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN = 4,
    HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT  = 5,
    HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT = 6,
    HAL_BTCOEX_SW_POWSAVE_SCAN_END   = 7,
    HAL_BTCOEX_SW_POWSAVE_PSM_START  = 8,
    HAL_BTCOEX_SW_POWSAVE_PSM_END    = 9,
    HAL_BTCOEX_SW_POWSAVE_PSM_STOP   = 10,

    HAL_BTCOEX_SW_POWSAVE_BUTT
} hal_coex_sw_preempt_type;
typedef oal_uint8 hal_coex_sw_preempt_type_uint8;

typedef enum
{
    HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE   = 0,
    HAL_BTCOEX_SW_POWSAVE_SUB_IDLE     = 1,
    HAL_BTCOEX_SW_POWSAVE_SUB_SCAN     = 2,
    HAL_BTCOEX_SW_POWSAVE_SUB_CONNECT  = 3,
    HAL_BTCOEX_SW_POWSAVE_SUB_PSM_FORBIT  = 4, /* 低功耗唤醒时，连续出现多次ps=1状态，要禁止psm 做状态判断，直到soc中断来更新 */

    HAL_BTCOEX_SW_POWSAVE_SUB_BUTT
} hal_coex_sw_preempt_subtype_enum;
typedef oal_uint8 hal_coex_sw_preempt_subtype_uint8;
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
/* HAL_DEVICE_WORK_STATE子状态 */
typedef enum {
    HAL_DEVICE_WORK_SUB_STATE_ACTIVE         = 0,              /* active子状态 */
    HAL_DEVICE_WORK_SUB_STATE_AWAKE          = 1,              /* awake子状态 */
    HAL_DEVICE_WORK_SUB_STATE_DOZE           = 2,              /* doze子状态 */
    HAL_DEVICE_WORK_SUB_STATE_INIT           = 3,              /* init子状态 */
    HAL_DEVICE_WORK_SUB_STATE_BUTT
} hal_device_work_sub_state_info;
#define HAL_WORK_SUB_STATE_NUM      (HAL_DEVICE_WORK_SUB_STATE_INIT - HAL_DEVICE_WORK_SUB_STATE_ACTIVE)
#endif

#endif /* end of #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)*/

/*****************************************************************************/
/*****************************************************************************/
/*                        HI1151 产品宏定义、枚举                            */
/*****************************************************************************/
/*****************************************************************************/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HAL_PUBLIC_HOOK_FUNC(_func) \
         hi1151##_func

#define HAL_RX_DSCR_GET_SW_ADDR(_addr)    (_addr)
#define HAL_RX_DSCR_GET_REAL_ADDR(_addr)  (_addr)
#define HAL_TX_DSCR_GET_SW_ADDR(_addr)    (_addr)
#define HAL_TX_DSCR_GET_REAL_ADDR(_addr)  (_addr)

#ifdef _PRE_WLAN_PRODUCT_1151V200
#define HAL_MAX_AP_NUM                     16       /* 支持多BSSID特性的规格，最大为4 AP*/
#define HAL_MAX_STA_NUM                    3        /* HAL STA个数 */
#define HAL_MAX_VAP_NUM                    (HAL_MAX_AP_NUM + HAL_MAX_STA_NUM)  /* HAL VAP总个数 */
#define HAL_VAP_ID_IS_VALID(_vap_id)      ((_vap_id <= 5) || ((_vap_id >= 16) && (_vap_id <= 28)))
#define HAL_VAP_ID_IS_VALID_PSTA(_vap_id) ((_vap_id < WLAN_HAL_OHTER_BSS_ID)||((_vap_id >= WLAN_PROXY_STA_START_ID) && (_vap_id <= WLAN_PROXY_STA_END_ID)))
#define HAL_AP_ID_IS_VALID(_vap_id)       ((_vap_id <= 3) || ((_vap_id >= 16) && (_vap_id <= 27)))
#define HAL_STA_ID_IS_VALID(_vap_id)      (((_vap_id >= 4) && (_vap_id <= 5)))
#else
#define HAL_MAX_AP_NUM                     4       /* 支持多BSSID特性的规格，最大为4 AP*/
#define HAL_MAX_STA_NUM                    1      /* HAL STA个数 */
#define HAL_MAX_VAP_NUM                    (HAL_MAX_AP_NUM + HAL_MAX_STA_NUM)  /* HAL VAP总个数 */
#define HAL_VAP_ID_IS_VALID(_vap_id)      ((_vap_id == 0) || (_vap_id == 1)|| (_vap_id == 2) || (_vap_id == 3) || (_vap_id == 4))
#define HAL_VAP_ID_IS_VALID_PSTA(_vap_id) ((_vap_id < WLAN_HAL_OHTER_BSS_ID)||((_vap_id >= WLAN_PROXY_STA_START_ID) && (_vap_id <= WLAN_PROXY_STA_END_ID)))
#endif
#define HAL_80211_FRAME_LEN                24   /* 帧头长度，刷cache用 */
#define HAL_PREP_DONE_TIME_OUT            4000  /* 等待prep done的超时时间 */

#define SIFSTIME             16
#define ACK_CTS_FRAME_LEN    14

/*****************************************************************************
  3 枚举
*****************************************************************************/
typedef enum
{
    HAL_TX_QUEUE_BK        = 0,        /* 背景业务 */
    HAL_TX_QUEUE_BE        = 1,        /* 尽力而为业务 */
    HAL_TX_QUEUE_VI        = 2,        /* 视频业务 */
    HAL_TX_QUEUE_VO        = 3,        /* 语音业务 */
    HAL_TX_QUEUE_HI        = 4,        /* 高优先级队列(管理帧/控制帧用此队列) */
    HAL_TX_QUEUE_BUTT
}hal_tx_queue_type_enum;
typedef oal_uint8 hal_tx_queue_type_enum_uint8;

#endif  /* end of #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)*/

/*****************************************************************************/
/*****************************************************************************/
/*                        公共宏定义、枚举、结构体                           */
/*****************************************************************************/
/*****************************************************************************/

#define HAL_POW_11B_RATE_NUM                4               /* 11b速率数目 */
#define HAL_POW_11G_RATE_NUM                8               /* 11g速率数目 */
#define HAL_POW_11A_RATE_NUM                8               /* 11a速率数目 */

#ifdef _PRE_WLAN_FEATURE_11AC_20M_MCS9
#ifdef _PRE_WLAN_FEATURE_1024QAM
#define HAL_POW_11AC_20M_NUM                12              /* 11n_11ac_2g速率数目 */
#else
#define HAL_POW_11AC_20M_NUM                10              /* 11n_11ac_2g速率数目 */
#endif
#else
#define HAL_POW_11AC_20M_NUM                9               /* 11n_11ac_2g速率数目 */
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM
#define HAL_POW_11AC_40M_NUM                13              /* 11n_11ac_2g速率数目 */
#define HAL_POW_11AC_80M_NUM                12              /* 11n_11ac_2g速率数目 */
#else
#define HAL_POW_11AC_40M_NUM                11              /* 11n_11ac_2g速率数目 */
#define HAL_POW_11AC_80M_NUM                10              /* 11n_11ac_2g速率数目 */
#endif

#ifdef _PRE_WLAN_FEATURE_160M
#ifdef _PRE_WLAN_FEATURE_1024QAM
#define HAL_POW_11AC_160M_NUM                12              /* 11n_11ac_2g速率数目 */
#else
#define HAL_POW_11AC_160M_NUM                10              /* 11n_11ac_2g速率数目 */
#endif
#else
#define HAL_POW_11AC_160M_NUM                0              /* 11n_11ac_2g速率数目(用于打桩计算HAL_POW_RATE_POW_CODE_TABLE_LEN) */
#endif

#define HAL_POW_RATE_POW_CODE_TABLE_LEN     (HAL_POW_11B_RATE_NUM+HAL_POW_11G_RATE_NUM+HAL_POW_11AC_20M_NUM \
                                                    +HAL_POW_11AC_40M_NUM+HAL_POW_11AC_80M_NUM+HAL_POW_11AC_160M_NUM)   /* rate-tpccode table中速率个数 */

#define HAL_POW_RATE_POW_CODE_TABLE_2G_LEN  (HAL_POW_11B_RATE_NUM+HAL_POW_11G_RATE_NUM+HAL_POW_11AC_20M_NUM \
                                                    +HAL_POW_11AC_40M_NUM)   /* rate-tpccode table中速率个数 */

/* 为了保证idx从0开始，个数中包含11B速率个数 */
#define HAL_POW_RATE_POW_CODE_TABLE_5G_LEN  (HAL_POW_11B_RATE_NUM+HAL_POW_11G_RATE_NUM+HAL_POW_11AC_20M_NUM \
                                                    +HAL_POW_11AC_40M_NUM+HAL_POW_11AC_80M_NUM+HAL_POW_11AC_160M_NUM)   /* rate-tpccode table中速率个数 */

#define HAL_VAP_ID_IS_VALID_PSTA(_vap_id) ((_vap_id < WLAN_HAL_OHTER_BSS_ID)||((_vap_id >= WLAN_PROXY_STA_START_ID) && (_vap_id <= WLAN_PROXY_STA_END_ID)))

#define HAL_POW_CUSTOM_24G_11B_RATE_NUM            2    /* 定制化11b速率数目 */
#define HAL_POW_CUSTOM_11G_11A_RATE_NUM            5    /* 定制化11g/11a速率数目 */
#define HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM         6    /* 定制化HT20_VHT20速率数目 */
#define HAL_POW_CUSTOM_24G_HT40_VHT40_RATE_NUM     8
#define HAL_POW_CUSTOM_5G_HT40_VHT40_RATE_NUM      7
#define HAL_POW_CUSTOM_5G_VHT80_RATE_NUM           6
/* 定制化全部速率 */
#define HAL_POW_CUSTOM_MCS9_10_11_RATE_NUM         3
#define HAL_POW_CUSTOM_MCS10_11_RATE_NUM           2
#define HAL_POW_CUSTOM_5G_VHT160_RATE_NUM          12   /* 定制化5G_11ac_VHT160速率数目 */
#define HAL_POW_CUSTOM_HT20_VHT20_DPD_RATE_NUM     5    /* 定制化DPD速率数目 */
#define HAL_POW_CUSTOM_HT40_VHT40_DPD_RATE_NUM     5


#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
#define HI1103_DYN_CALI_5G_SECTION                      2            /* 5G TX Power分 high & low power校准 */
#endif

/* 定制化相关宏 */
/* NVRAM中存储的各协议速率最大发射功率参数的个数 From:24G_11b_1M To:5G_VHT80_MCS7 */
#define NUM_OF_NV_NORMAL_MAX_TXPOWER   (HAL_POW_CUSTOM_24G_11B_RATE_NUM + \
                                        HAL_POW_CUSTOM_11G_11A_RATE_NUM + HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM + \
                                        HAL_POW_CUSTOM_24G_HT40_VHT40_RATE_NUM + HAL_POW_CUSTOM_11G_11A_RATE_NUM + \
                                        HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM + HAL_POW_CUSTOM_5G_HT40_VHT40_RATE_NUM + \
                                        HAL_POW_CUSTOM_5G_VHT80_RATE_NUM)
#define NUM_OF_NV_MAX_TXPOWER          (NUM_OF_NV_NORMAL_MAX_TXPOWER + \
                                        HAL_POW_CUSTOM_MCS9_10_11_RATE_NUM * 4 + HAL_POW_CUSTOM_5G_VHT160_RATE_NUM + HAL_POW_CUSTOM_MCS10_11_RATE_NUM)

#define NUM_OF_NV_DPD_MAX_TXPOWER      (HAL_POW_CUSTOM_HT20_VHT20_DPD_RATE_NUM + HAL_POW_CUSTOM_HT40_VHT40_DPD_RATE_NUM)
#define NUM_OF_NV_11B_DELTA_TXPOWER        (2)
#define NUM_OF_NV_5G_UPPER_UPC             (4)
#define NUM_OF_NV_24G_11G_6M_POWER_IDX     (2)
#define NUM_OF_NV_24G_20M_MCS0_POWER_IDX   (7)
#define NUM_OF_24G_11G_6M_RATE_IDX         (4)
#define NUM_OF_24G_20M_MCS0_RATE_IDX       (12)

#endif
#define HAL_CUS_NUM_5G_BW                        4   /* 定制化5g带宽数 */
#define HAL_CUS_NUM_FCC_2G_PRO                   3   /* 定制化2g FCC 11B+OFDM_20M+OFDM_40M */
#define HAL_CUS_NUM_OF_SAR_PARAMS                8   /* 定制化降SAR参数 5G_BAND1~7 2.4G */
#define HAL_NUM_5G_20M_SIDE_BAND                 6   /* 定制化5g边带数 */
#define HAL_NUM_5G_40M_SIDE_BAND                 6
#define HAL_NUM_5G_80M_SIDE_BAND                 5
#define HAL_NUM_5G_160M_SIDE_BAND                2

#define HAL_POW_MAX_CHAIN_NUM           2           /* 最大通道数 */
#define HAL_POW_PRECISION_SHIFT         10          /*TPC算法中功率的精度*/

#define HAL_POW_PA_LUT_NUM              4          /* 筛选使用的PA档位数目 */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
#define HAL_AL_TX_DYN_CAL_INTERVAL_NUM   10        /* 不指定数目常发动态校准帧间隔 */
#endif
#define HAL_POW_UPC_RF_LUT_NUM          256        /* UPC在RF中的最大档位数目 */
#define HAL_2G_POW_UPC_RF_LUT_NUM       256        /* UPC在RF中的档位数目 8bit */
#define HAL_5G_POW_UPC_RF_LUT_NUM       64

#ifdef _PRE_WLAN_1103_PILOT
#define HAL_POW_UPC_LUT_NUM             2     /* 筛选使用的UPC档位数目(03 pilot upc code 修订为1bit) */
#else
#define HAL_POW_UPC_LUT_NUM             32     /* 筛选使用的UPC档位数目(03 EVB2 upc code 修订为5bit) */
#endif
#else //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#define HAL_POW_UPC_RF_LUT_NUM          64        /* UPC在RF中的档位数目 */
#define HAL_POW_DBB_LUT_NUM             128       /* 数字DBB的档位数目 */
#define HAL_2G_POW_UPC_RF_LUT_NUM       64
#define HAL_5G_POW_UPC_RF_LUT_NUM       64

#define HAL_POW_UPC_LUT_NUM             16          /* 筛选使用的UPC档位数目 */
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

#define HAL_POW_PA_BASE_IDX                 2           /* 基准PA Index */
#define HAL_POW_11B_LPF_BASE_IDX            1           /* 11B基准LPF Index */

#define HAL_POW_2G_LPF_BASE_IDX             2           /* 2G基准LPF Index */
#define HAL_POW_2G_DAC_BASE_IDX             0           /* 2G基准DAC Index */
#define HAL_POW_5G_LPF_BASE_IDX             3           /* 5G基准LPF Index */
#define HAL_POW_5G_DAC_BASE_IDX             2           /* 5G基准DAC Index */
#define HAL_POW_LPF_BASE_IDX(_en_band)      ((_en_band == WLAN_BAND_2G) ? HAL_POW_2G_LPF_BASE_IDX : HAL_POW_5G_LPF_BASE_IDX)  /* 基准LPF Index */
#define HAL_POW_DAC_BASE_IDX(_en_band)      ((_en_band == WLAN_BAND_2G) ? HAL_POW_2G_DAC_BASE_IDX : HAL_POW_5G_DAC_BASE_IDX)  /* 基准DAC Index */


#ifdef _PRE_WLAN_1103_PILOT
#define HAL_UPC_DATA_REG_NUM                1
#define HAL_POW_UPC_LOW_START_IDX           0           /* UPC低档位的起始索引 */
#define HAL_POW_CFR_BASE_IDX                0           /* 基准cfr_index Index */
#define HAL_POW_DPD_TPC_BASE_IDX            0           /* 基准dpd_tpc_lv Index */
#define HAL_POW_DELTA_DBB_SCAL_BASE_IDX     HAL_DBB_SCALING_FOR_MAX_TXPWR_BASE  /* 基准delta_dbb_scaling Index */
#else
#define HAL_POW_UPC_LOW_START_IDX           1           /* UPC低档位的起始索引 */
#define HAL_POW_UPC_LUT_IDX_FOR_FAR_DIST    0
#define HAL_POW_UPC_LUT_IDX_FOR_CALI_CODE   1
#define HAL_POW_DBB_SCAL2_C0_BASE_IDX       0           /* 基准dbb_scale2_c0 Index */
#define HAL_POW_DBB_SCAL2_C1_BASE_IDX       0           /* 基准dbb_scale2_c1 Index */

/* 单个通道的UPC数据寄存器数目 */
#define HAL_UPC_DATA_REG_NUM      (HAL_POW_UPC_LUT_NUM >> 2)
#endif
/*TPC档位设置*/
#define HAL_POW_LEVEL_NUM               5                   /*算法总档位数目*/
#define HAL_POW_ADJUST_LEVEL_NUM        HAL_POW_LEVEL_NUM   /*算法支持可调整的档位数目*/
#define HAL_POW_MAX_POW_LEVEL           0                   /*算法的最大功率档位*/
#define HAL_POW_MIN_POW_LEVEL           (HAL_POW_LEVEL_NUM - 1)         /*算法的最小功率档位*/
#define HAL_POW_LEVEL_DOWN_LIMIT        (HAL_POW_ADJUST_LEVEL_NUM - 1)    /*功率衰减的最小档位*/
#define HAL_POW_LEVEL_UP_LIMIT          (HAL_POW_MAX_POW_LEVEL)           /*功率回升的最大档位*/
#define HAL_POW_RF_LIMIT_POW_LEVEL      (HAL_POW_LEVEL_NUM)    /*极远距离的功率档位标识*/
#define HAL_POW_INVALID_POW_LEVEL        0xff        /*无效功率档位*/

#define HAL_POW_2G_1MBPS_RATE_POW_IDX       0               /* 2G 1Mbps对应的功率表索引 */
#define HAL_POW_5G_6MBPS_RATE_POW_IDX       4               /* 5G 6Mbps对应的功率表索引 */
#define HAL_POW_5G_6P5MBPS_RATE_POW_IDX     12              /* 5G 6.5Mbps对应的功率表索引, ht20 mcs0 */

/* 发送队列异常检测描述符最大个数 */
#define HAL_TXQ_STALL_CHECK_DEPTH           2

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#ifdef _PRE_WLAN_1103_PILOT
/* Bit[22:22]: DPD enable; Bit[21:12]: delt dbb scaling; Bit[11:10]: DAC gain; Bit[9:8]: PA;
   Bit[7:5]: LPF gain ; Bit[4:4]: UPC gain ; Bit[3:2]: CFR index; Bit[1:0]: dpd_tpc_lv*/
#define HAL_POW_CODE_COMB(_en_dpd_enable, _uc_upc_idx, _uc_pa_idx, _uc_dac_idx, _uc_lpf_idx, _uc_delta_dbb_scaling, _uc_dpd_tpc_lv, _uc_cfr_index) \
((((oal_uint32)(_uc_dpd_tpc_lv) & 0x3) | (((oal_uint32)(_uc_cfr_index) & 0x3) << 2) | \
  (((oal_uint32)(_uc_upc_idx) & 0x1) << 4) | (((oal_uint32)(_uc_lpf_idx) & 0x7) << 5) | \
  (((oal_uint32)(_uc_pa_idx) & 0x3) << 8) | (((oal_uint32)(_uc_dac_idx) & 0x3) << 10) | \
  (((oal_uint32)(_uc_delta_dbb_scaling) & 0x3ff) << 12) | (((oal_uint32)(_en_dpd_enable) & 0x1) << 22)) & 0x7fffff)

#define HAL_INI_POW_CODE(_en_band)    (HAL_POW_CODE_COMB(OAL_TRUE, HAL_POW_UPC_LOW_START_IDX, HAL_POW_PA_BASE_IDX, HAL_POW_DAC_BASE_IDX(_en_band),  \
                                       HAL_POW_LPF_BASE_IDX(_en_band), HAL_POW_DELTA_DBB_SCAL_BASE_IDX, HAL_POW_DPD_TPC_BASE_IDX, \
                                       HAL_POW_CFR_BASE_IDX))

#define HAL_GET_DPD_ENABLE_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_1_BITS, BIT_OFFSET_22)))
#define HAL_GET_DELTA_DBB_SCALING_FROM_POW_CODE(_ul_pow_code) ((oal_uint16)(OAL_GET_BITS((_ul_pow_code), NUM_10_BITS, BIT_OFFSET_12)))
#define HAL_GET_DAC_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_2_BITS, BIT_OFFSET_10)))
#define HAL_GET_PA_GAIN_LVL_FROM_POW_CODE(_ul_pow_code)  ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_2_BITS, BIT_OFFSET_8)))
#define HAL_GET_LPF_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_3_BITS, BIT_OFFSET_5)))
#define HAL_GET_UPC_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_1_BITS, BIT_OFFSET_4)))
#define HAL_GET_CFR_IDEX_FROM_POW_CODE(_ul_pow_code)     ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_2_BITS, BIT_OFFSET_2)))
#define HAL_GET_DPD_TPC_LVL_FROM_POW_CODE(_ul_pow_code)  ((oal_uint8)(OAL_GET_BITS((_ul_pow_code), NUM_2_BITS, BIT_OFFSET_0)))

// TODO: pilot超远距暂不处理，后面考虑DPD来实现
#define HAL_POW_SET_RF_LIMIT_POW(_pow_level,_ul_code)    \
    if((_pow_level) == HAL_POW_RF_LIMIT_POW_LEVEL)  { ; } \


#else //#ifdef _PRE_WLAN_1103_PILOT
/* Bit[19:16]: DBB scale2 c1 gain; Bit[15:12]: DBB scale2 c0 gain;
   Bit[11:10]: DAC gain ; Bit[9:8]: PA gain ; Bit[7:5]: LPF gain; Bit[4:0]: UPC*/
#define HAL_POW_CODE_COMB(_uc_upc_idx, _uc_lpf_idx, _uc_pa_idx, _uc_dac_idx, _uc_dbb_scale2_c0_idx, _uc_dbb_scale2_c1_idx)    \
((((oal_uint32)(_uc_upc_idx) & 0x1f) | (((oal_uint32)(_uc_lpf_idx) & 0x7) << 5) | \
  (((oal_uint32)(_uc_pa_idx) & 0x3) << 8) | (((oal_uint32)(_uc_dac_idx) & 0x3) << 10) | \
  (((oal_uint32)(_uc_dbb_scale2_c0_idx) & 0xf) << 12) | (((oal_uint32)(_uc_dbb_scale2_c1_idx) & 0xf) << 16)) & 0xfffff)

#define HAL_INI_POW_CODE(_en_band)    (HAL_POW_CODE_COMB(HAL_POW_UPC_LOW_START_IDX, HAL_POW_LPF_BASE_IDX(_en_band), HAL_POW_PA_BASE_IDX, \
                                       HAL_POW_DAC_BASE_IDX(_en_band), HAL_POW_DBB_SCAL2_C0_BASE_IDX, HAL_POW_DBB_SCAL2_C1_BASE_IDX))

#define HAL_GET_UPC_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)((_ul_pow_code) & 0x1f))
#define HAL_GET_LPF_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(((_ul_pow_code) & 0xe0) >> 5))
#define HAL_GET_PA_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(((_ul_pow_code) & 0x300) >> 8))
#define HAL_GET_DAC_GAIN_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(((_ul_pow_code) & 0xc00) >> 10))
#define HAL_GET_DBB_SCALE2_C0_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(((_ul_pow_code) & 0xf000) >> 12))
#define HAL_GET_DBB_SCALE2_C1_LVL_FROM_POW_CODE(_ul_pow_code) ((oal_uint8)(((_ul_pow_code) & 0xf0000) >> 16))

#define HAL_POW_SET_RF_LIMIT_POW(_pow_level,_ul_code) \
         if(HAL_POW_RF_LIMIT_POW_LEVEL == (_pow_level)) \
         {(_ul_code) &=0xfffe0; \
          (_ul_code) |=(HAL_POW_UPC_LUT_IDX_FOR_FAR_DIST & 0x1F);}
#endif //#ifdef _PRE_WLAN_1103_PILOT

#define HAL_POW_GET_POW_CODE_FROM_TABLE(_pst_rate_pow_table, _uc_rate_to_pow_idx, _uc_pow_lvl)    \
    ((_pst_rate_pow_table)[(_uc_rate_to_pow_idx)].aul_pow_code_level[(_uc_pow_lvl)]);

#define HAL_GET_SUBBAND_IDX(_BAND, _CHAN_IDX) \
    ((WLAN_BAND_5G == (_BAND)) ? g_st_cali_chn_idx.auc_5g20m_norm2_dpa_idx[(_CHAN_IDX)] : (_CHAN_IDX))

#else //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

#define HAL_POW_CODE_COMB(_uc_code, _uc_upc)    \
    (((_uc_code) & 0xc3) | ((_uc_upc)<<2))
#define HAL_POW_SET_RF_LIMIT_POW(_pow_level,_ul_code) \
         if(HAL_POW_RF_LIMIT_POW_LEVEL == (_pow_level)) \
         {(_ul_code) &=0xc3; \
          (_ul_code) |=(HAL_POW_UPC_LUT_IDX_FOR_FAR_DIST & 0xF)<< 2;}

#define HAL_POW_GET_POW_CODE_FROM_TABLE(_pst_rate_pow_table, _uc_rate_to_pow_idx, _uc_pow_lvl)    \
        ((_pst_rate_pow_table)[(_uc_rate_to_pow_idx)].auc_pow_code_level[(_uc_pow_lvl)]);

#endif
#define HAL_POW_FILL_TX_FOUR_SAME_CODE(_uc_code)    \
    ((((oal_uint32)(_uc_code)) << 24) | (((oal_uint32)(_uc_code)) << 16)| \
      (((oal_uint32)(_uc_code)) << 8) | ((oal_uint32)(_uc_code)))

#define HAL_POW_FILL_TX_FOUR_DIFF_POW_CODE(_uc_pow_code1, _uc_pow_code2, _uc_pow_code3, _uc_pow_code4)    \
    ((((oal_uint32)(_uc_pow_code1)) << 24) | (((oal_uint32)(_uc_pow_code2)) << 16)| \
      (((oal_uint32)(_uc_pow_code3)) << 8) | ((oal_uint32)(_uc_pow_code4)))

#define HAL_POW_FILL_SIFS_TX_MODE_POW_CODE(_uc_code, _uc_num)    \
    ((_uc_code) << (8*(_uc_num)))
#define HAL_POW_SEP_SIFS_TX_MODE_POW_CODE(_ul_code, _uc_num)    \
    (((_ul_code) >> (8*(_uc_num))) & 0xFF)

#define HAL_POW_FILL_UPC_DATA_REG(_uc_data1, _uc_data2, _uc_data3, _uc_data4)    \
    (((oal_uint32)(_uc_data1<<(oal_uint32)24)) | ((oal_uint32)(_uc_data2<<(oal_uint32)16)) | ((oal_uint32)(_uc_data3)<<(oal_uint32)8) | (_uc_data4))

#define HAL_POW_GET_LEGACY_RATE(_pst_per_rate_params)\
    ((((_pst_per_rate_params)->rate_bit_stru.un_nss_rate.st_legacy_rate.bit_protocol_mode)<<6) | \
    ((_pst_per_rate_params)->rate_bit_stru.un_nss_rate.st_legacy_rate.bit_legacy_rate))

#define HAL_ABS(_a)  (((_a) > 0) ? (_a) : (-(_a)))

#define HAL_POW_FIND_NEAR_IDX(_uc_data1, _uc_data2, _uc_idx1, _uc_idx2, _uc_input)    \
    ((HAL_ABS((_uc_data1)-(_uc_input)))>(HAL_ABS((_uc_data2)-(_uc_input)))? (_uc_idx2):(_uc_idx1))

#define HAL_POW_GET_UPC_LUT_TARGET_LEN(_us_data, _uc_intvl)    \
        ((0 == ((_us_data)% (_uc_intvl)))? (((_us_data)/(_uc_intvl)) + 2) :(((_us_data)/(_uc_intvl)) + 3) )

#define HAL_POW_GET_RF_LIMIT_POW(_aus_rf_pow_limit, _uc_idx, _uc_start_chain, _uc_end_chain) \
        ((((_aus_rf_pow_limit)[(_uc_idx)][(_uc_start_chain)]) < \
          ((_aus_rf_pow_limit)[(_uc_idx)][(_uc_end_chain)]))? \
          ((_aus_rf_pow_limit)[(_uc_idx)][(_uc_start_chain)]) : \
          ((_aus_rf_pow_limit)[(_uc_idx)][(_uc_end_chain)]))


/* 获取当前帧所使用的协议模式 */
#define HAL_GET_DATA_PROTOCOL_MODE(_val)      ((_val) >> 6)

/* hal device下挂算法的私有结构体 */
#define HAL_DEV_ALG_PRIV(_pst_hal_dev)                  ((_pst_hal_dev)->p_alg_priv)

#define GET_DEV_RSSI_TRIGGER(_pst_hal_dev)       (_pst_hal_dev->st_rssi.en_rssi_trigger)
#define GET_DEV_RSSI_MISO_HOLD(_pst_hal_dev)     (_pst_hal_dev->st_rssi.en_miso_hold)
#define GET_DEV_RSSI_TBTT_CNT(_pst_hal_dev)      (_pst_hal_dev->st_rssi.uc_tbtt_cnt)
#define GET_DEV_RSSI_TBTT_TH(_pst_hal_dev)       (_pst_hal_dev->st_rssi.uc_tbtt_cnt_th)
#define GET_DEV_RSSI_MIMO_HOLD(_pst_hal_dev)     (_pst_hal_dev->st_rssi.en_mimo_hold)
#define GET_DEV_RSSI_MIMO_TBTT_CNT(_pst_hal_dev) (_pst_hal_dev->st_rssi.uc_mimo_tbtt_cnt)
#define GET_DEV_RSSI_MIMO_TBTT_OPEN_TH(_pst_hal_dev)  (_pst_hal_dev->st_rssi.uc_mimo_tbtt_open_th)
#define GET_DEV_RSSI_MIMO_TBTT_CLOSE_TH(_pst_hal_dev)  (_pst_hal_dev->st_rssi.uc_mimo_tbtt_close_th)
#define GET_RSSI_MIN(_c_rssi_ant0, _c_rssi_ant1)  \
    ((OAL_RSSI_INIT_VALUE == (_c_rssi_ant0)) ? (_c_rssi_ant1):     \
     (OAL_RSSI_INIT_VALUE == (_c_rssi_ant1)) ? (_c_rssi_ant0):     \
     OAL_MIN((_c_rssi_ant0), (_c_rssi_ant1)))

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#define GET_RSSI(_c_rssi_dbm, _c_rssi_ant0, _c_rssi_ant1) GET_RSSI_MIN(_c_rssi_ant0, _c_rssi_ant1)
#else
#define GET_RSSI(_c_rssi_dbm, _c_rssi_ant0, _c_rssi_ant1) _c_rssi_dbm
#endif

/* hal device下挂接收描述符控制相关宏定义 */
#define GET_DEV_RX_DSCR_MAX_USED_CNT(_pst_hal_dev, _uc_queue_id) \
    (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->us_rx_max_dscr_used_cnt)

#define GET_DEV_RX_DSCR_PREVS_STATS(_pst_hal_dev, _uc_queue_id) \
     (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->uc_previous_cycle_stats)
#define GET_DEV_RX_DSCR_CONTS_IDLE_CNT(_pst_hal_dev, _uc_queue_id)    \
    (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->uc_continus_idle_cnt)


#define GET_DEV_RX_DSCR_EVENT_CNT(_pst_hal_dev, _uc_queue_id)    \
    (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->us_rx_event_cnt)
#define GET_DEV_RX_DSCR_EVENT_PKTS(_pst_hal_dev, _uc_queue_id)   \
    (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->us_rx_event_pkts_sum)
#define GET_DEV_RX_DSCR_AVRAGE_PKTS(_pst_hal_dev, _uc_queue_id)  \
    ((GET_DEV_RX_DSCR_EVENT_PKTS(_pst_hal_dev, _uc_queue_id)) / (GET_DEV_RX_DSCR_EVENT_CNT(_pst_hal_dev, _uc_queue_id)))
#define GET_DEV_RX_DSCR_AVRG_PKTS(_pst_hal_dev, _uc_queue_id)    \
    ((GET_DEV_RX_DSCR_EVENT_CNT(_pst_hal_dev, _uc_queue_id) == 0) ? 0 : (GET_DEV_RX_DSCR_AVRAGE_PKTS(_pst_hal_dev, _uc_queue_id)))

#define GET_DEV_RX_DSCR_THRES(_pst_hal_dev, _uc_queue_id)        \
    (((alg_rx_dscr_ctl_alg_info_stru *)((_pst_hal_dev)->st_rx_dscr_ctl.pst_rx_dscr_ctl_alg_info + (_uc_queue_id)))->us_rx_dscr_alg_thres)
#define GET_DEV_RX_DSCR_LARGER_THAN(_us_ualue, _us_thres)        \
     (_us_ualue = ((_us_ualue) < (_us_thres)) ? (_us_thres) : (_us_ualue))

#define GET_DEV_RX_DSCR_RESET_STATIC(_pst_hal_dev, _uc_queue_id) \
                                  do{ \
                                      GET_DEV_RX_DSCR_MAX_USED_CNT(_pst_hal_dev, _uc_queue_id) = 0; \
                                      GET_DEV_RX_DSCR_EVENT_CNT(_pst_hal_dev, _uc_queue_id) = 0; \
                                      GET_DEV_RX_DSCR_EVENT_PKTS(_pst_hal_dev, _uc_queue_id) = 0; \
                                  } while(0)

#ifdef _PRE_WLAN_FEATURE_TPC_OPT

/* 获取hal dev下的TX功率控制相关的参数 */
#define HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev) (((hal_to_dmac_device_stru *)(_pst_hal_dev))->st_tx_pwr_ctrl)

/* 获取各速率最大发送功率 */
#define HAL_DEV_GET_PER_RATE_MAX_POW_CTRL(_pst_hal_dev, _uc_rate_idx, _en_freq_band, _uc_subband_idx) \
    ((oal_uint8)(((HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev)).pst_max_tx_pow_per_rate_ctrl->auc_rate_max_tx_pow_ctrl_table[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_LEN - 1, _uc_rate_idx)][(_en_freq_band)]) \
      + ((WLAN_BAND_5G == (_en_freq_band)) ? g_ac_5g_delt_txpwr_base_params[(_uc_subband_idx)] : 0)))

/* 获取各速率的dbb scaling */
#define HAL_DEV_GET_PER_RATE_DBB_SCALING_CTRL(_pst_hal_dev, _uc_scale_idx) \
        ((HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev)).aus_dbb_scale_per_rate_ctrl[(_uc_scale_idx)])

/* 获取各速率的lpf idx */
#define HAL_DEV_GET_PER_RATE_LPF_IDX_CTRL(_pst_hal_dev, _en_freq_band, _uc_rate_idx) \
 ((WLAN_BAND_2G == _en_freq_band) ? \
    (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).puc_lpf_idx_per_rate_ctrl_2g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_2G_LEN - 1, (_uc_rate_idx))]) : \
     (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).puc_lpf_idx_per_rate_ctrl_5g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_5G_LEN - 1, (_uc_rate_idx))]))

/* 设置各速率的lpf idx */
#define HAL_DEV_SET_PER_RATE_LPF_IDX_CTRL(_pst_hal_dev, _en_freq_band, _uc_rate_idx, _uc_lpf_idx) \
do {    \
    if (WLAN_BAND_2G == (_en_freq_band)) \
        {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).puc_lpf_idx_per_rate_ctrl_2g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_2G_LEN - 1, (_uc_rate_idx))]) = (oal_uint8)(_uc_lpf_idx);} \
         else \
         {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).puc_lpf_idx_per_rate_ctrl_5g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_5G_LEN - 1, (_uc_rate_idx))]) = (oal_uint8)(_uc_lpf_idx);} \
   } while(0)

/* 获取tpc档位对应的目标功率增益 */
#define HAL_DEV_GET_IDX_TARGET_POW_GAIN(_pst_hal_dev, _uc_pow_idx, _en_freq_band) \
            ((HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev)).pst_tpc_lvl_vs_gain_ctrl->as_pow_level_table[(_uc_pow_idx)][(_en_freq_band)])

/* 获取tpc档位对应的TPC CODE */
#define HAL_DEV_GET_PER_RATE_TPC_CODE_BY_LVL(_pst_hal_dev, _en_freq_band, _uc_rate_idx, _uc_pow_idx) \
     ((WLAN_BAND_2G == _en_freq_band) ? \
        (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_2g[_uc_rate_idx].aul_pow_code_level[(_uc_pow_idx)]) : \
         (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_5g[_uc_rate_idx].aul_pow_code_level[(_uc_pow_idx)]))

/* 设置tpc档位对应的TPC CODE */
#define HAL_DEV_SET_PER_RATE_TPC_CODE_BY_LVL(_pst_hal_dev, _en_freq_band, _uc_rate_idx, _uc_pow_idx, _ul_pow_code) \
 do {    \
     if (WLAN_BAND_2G == (_en_freq_band)) \
         {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_2g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_2G_LEN - 1, (_uc_rate_idx))].aul_pow_code_level[(_uc_pow_idx)]) = (oal_uint32)(_ul_pow_code);} \
          else \
          {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_5g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_5G_LEN - 1, (_uc_rate_idx))].aul_pow_code_level[(_uc_pow_idx)]) = (oal_uint32)(_ul_pow_code);} \
    } while(0)

/* 获取tpc档位对应的实际功率增益 */
#define HAL_DEV_GET_PER_RATE_TPC_GAIN_BY_LVL(_pst_hal_dev, _en_freq_band, _uc_rate_idx, _uc_pow_idx) \
    ((WLAN_BAND_2G == _en_freq_band) ? \
       (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_2g[_uc_rate_idx].as_pow_gain_level[(_uc_pow_idx)]) : \
        (HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_5g[_uc_rate_idx].as_pow_gain_level[(_uc_pow_idx)]))

/* 设置tpc档位对应的实际功率增益 */
#define HAL_DEV_SET_PER_RATE_TPC_GAIN_BY_LVL(_pst_hal_dev, _en_freq_band, _uc_rate_idx, _uc_pow_idx, _s_pow_gain) \
    do {    \
        if (WLAN_BAND_2G == (_en_freq_band)) \
            {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_2g[(_uc_rate_idx)].as_pow_gain_level[(_uc_pow_idx)]) = (oal_int16)(_s_pow_gain);} \
             else \
             {(HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev).pst_rate_pow_table_5g[(_uc_rate_idx)].as_pow_gain_level[(_uc_pow_idx)]) = (oal_int16)(_s_pow_gain);} \
       } while(0)

/* 设置各速率基准最大发送功率,信道切换时获取实际目标发射功率 */
#define HAL_DEV_SET_PER_RATE_MAX_POW_CTRL(_pst_hal_dev, _uc_rate_idx, _en_freq_band) \
        ((HAL_DEV_GET_TX_PWR_CTRL(_pst_hal_dev)).pst_max_tx_pow_per_rate_ctrl->auc_rate_max_tx_pow_ctrl_table[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_LEN - 1, _uc_rate_idx)][(_en_freq_band)])

/* 计算PAPA=5.5时，对应的功率补偿值 单位0.1dBm */
/* 根据实验室测试数据，一次拟合得到计算功率偏差公式0.25+0.03*delt_power，推导对应6M&MCS0有如下功率补偿值计算公式，小于-0.5dB时不需要补偿 */
#define HAL_DEV_CFR_GET_COMPSEN_BY_DELTPOW(_c_delt_pow) ((oal_uint8)(((_c_delt_pow) < -5) ? 0 : (((3*(_c_delt_pow) + 25) + 5) / 10)))

#else

/* 获取各速率的lpf idx */
#define HAL_DEV_GET_PER_RATE_LPF_IDX_CTRL(_pst_hal_dev, _en_freq_band, _uc_rate_idx) \
    ((WLAN_BAND_2G == _en_freq_band) ? \
    (g_uc_rate_lpf_code_tbl_2g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_2G_LEN - 1, (_uc_rate_idx))]) : \
    (g_uc_rate_lpf_code_tbl_5g[OAL_MIN(HAL_POW_RATE_POW_CODE_TABLE_5G_LEN - 1, (_uc_rate_idx))]))

#endif

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* 2.4GHz频段: 信道号对应的信道索引值 */
typedef enum
{
    HAL_2G_CHANNEL1  = 0,
    HAL_2G_CHANNEL2  = 1,
    HAL_2G_CHANNEL3  = 2,
    HAL_2G_CHANNEL4  = 3,
    HAL_2G_CHANNEL5  = 4,
    HAL_2G_CHANNEL6  = 5,
    HAL_2G_CHANNEL7  = 6,
    HAL_2G_CHANNEL8  = 7,
    HAL_2G_CHANNEL9  = 8,
    HAL_2G_CHANNEL10 = 9,
    HAL_2G_CHANNEL11 = 10,
    HAL_2G_CHANNEL12 = 11,
    HAL_2G_CHANNEL13 = 12,
    HAL_2G_CHANNEL14 = 13,

    HAL_CHANNEL_FREQ_2G_BUTT = 14
}hal_channel_freq_2g_enum;
typedef oal_uint8 hal_channel_freq_2g_enum_uint8;

typedef enum
{
    HAL_FCS_PROTECT_TYPE_NONE      = 0,    /* NONE        */
    HAL_FCS_PROTECT_TYPE_SELF_CTS,         /* SELF CTS    */
    HAL_FCS_PROTECT_TYPE_NULL_DATA,        /* NULL DATA   */

    HAL_FCS_PROTECT_TYPE_BUTT
}hal_fcs_protect_type_enum;
typedef oal_uint8 hal_fcs_protect_type_enum_uint8;

typedef enum
{
    HAL_FCS_SERVICE_TYPE_DBAC      = 0,    /* DBAC业务    */
    HAL_FCS_SERVICE_TYPE_SCAN,             /* 扫描业务    */
    HAL_FCS_SERVICE_TYPE_M2S,              /* m2s切换业务 */
    HAL_FCS_SERVICE_TYPE_BTCOEX_NORMAL,    /* btcoex共存业务 */
    HAL_FCS_SERVICE_TYPE_BTCOEX_LDAC,      /* btcoex共存业务 */

    HAL_FCS_PROTECT_NOTIFY_BUTT
}hal_fcs_service_type_enum;
typedef oal_uint8 hal_fcs_service_type_enum_uint8;

typedef enum
{
    HAL_FCS_PROTECT_COEX_PRI_NORMAL   = 0,    /* b00 */
    HAL_FCS_PROTECT_COEX_PRI_PRIORITY = 1,    /* b01 */
    HAL_FCS_PROTECT_COEX_PRI_OCCUPIED = 2,    /* b10 */

    HAL_FCS_PROTECT_COEX_PRI_BUTT
}hal_fcs_protect_coex_pri_enum;
typedef oal_uint8 hal_fcs_protect_coex_pri_enum_uint8;

typedef enum
{
    HAL_FCS_PROTECT_CNT_1 = 1,    /* 1 */
    HAL_FCS_PROTECT_CNT_2 = 2,    /* 2 */
    HAL_FCS_PROTECT_CNT_3 = 3,    /* 3 */
    HAL_FCS_PROTECT_CNT_6 = 6,    /* 10 */
    HAL_FCS_PROTECT_CNT_10 = 10,    /* 10 */
    HAL_FCS_PROTECT_CNT_20 = 20,    /* 20 */

    HAL_FCS_PROTECT_CNT_BUTT
}hal_fcs_protect_cnt_enum;
typedef oal_uint8 hal_fcs_protect_cnt_enum_uint8;

typedef enum
{
    HAL_OPER_MODE_NORMAL,
    HAL_OPER_MODE_HUT,

    HAL_OPER_MODE_BUTT
}hal_oper_mode_enum;
typedef oal_uint8 hal_oper_mode_enum_uint8;

/**** RF测试用，用于指示配置TX描述符字段 ****/
typedef enum
{
    HAL_RF_TEST_DATA_RATE_ZERO,
    HAL_RF_TEST_BAND_WIDTH,
    HAL_RF_TEST_CHAN_CODE,
    HAL_RF_TEST_POWER,
    HAL_RF_TEST_BUTT
}hal_rf_test_sect_enum;
typedef oal_uint8 hal_rf_test_sect_enum_uint8;

typedef enum
{
    HAL_PHY_MAX_BW_SECT_MAX_BANDWIDTH = 0,
    HAL_PHY_MAX_BW_SECT_MAX_NSS       = 1,
    HAL_PHY_MAX_BW_SECT_SINGLE_CH_SEL = 2,

    HAL_PHY_MAX_BW_SECT_BUTT
}hal_phy_max_bw_sect_enum;
typedef oal_uint8 hal_phy_max_bw_sect_enmu_uint8;

/*****************************************************************************
  3.1 队列相关枚举定义
*****************************************************************************/

#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
#define HAL_AC_TO_Q_NUM(_ac) (       \
        ((_ac) == WLAN_WME_AC_VO) ? HAL_TX_QUEUE_VO : \
        ((_ac) == WLAN_WME_AC_VI) ? HAL_TX_QUEUE_VI : \
        ((_ac) == WLAN_WME_AC_BK) ? HAL_TX_QUEUE_BK : \
        ((_ac) == WLAN_WME_AC_BE) ? HAL_TX_QUEUE_BE : \
        ((_ac) == WLAN_WME_AC_MGMT) ? HAL_TX_QUEUE_HI : \
        ((_ac) == WLAN_WME_AC_PSM) ? HAL_TX_QUEUE_MC : \
        HAL_TX_QUEUE_BK)
#else
#define HAL_AC_TO_Q_NUM(_ac) (       \
        ((_ac) == WLAN_WME_AC_VO) ? HAL_TX_QUEUE_VO : \
        ((_ac) == WLAN_WME_AC_VI) ? HAL_TX_QUEUE_VI : \
        ((_ac) == WLAN_WME_AC_BK) ? HAL_TX_QUEUE_BK : \
        ((_ac) == WLAN_WME_AC_BE) ? HAL_TX_QUEUE_BE : \
        ((_ac) == WLAN_WME_AC_MGMT) ? HAL_TX_QUEUE_HI : \
        HAL_TX_QUEUE_BK)
#endif

#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
#define HAL_Q_NUM_TO_AC(_q) (       \
        ((_q) == HAL_TX_QUEUE_VO) ? WLAN_WME_AC_VO : \
        ((_q) == HAL_TX_QUEUE_VI) ? WLAN_WME_AC_VI : \
        ((_q) == HAL_TX_QUEUE_BK) ? WLAN_WME_AC_BK : \
        ((_q) == HAL_TX_QUEUE_BE) ? WLAN_WME_AC_BE : \
        ((_q) == HAL_TX_QUEUE_HI) ? WLAN_WME_AC_MGMT : \
        ((_q) == HAL_TX_QUEUE_MC) ? WLAN_WME_AC_PSM : \
        WLAN_WME_AC_BE)
#else
#define HAL_Q_NUM_TO_AC(_q) (       \
        ((_q) == HAL_TX_QUEUE_VO) ? WLAN_WME_AC_VO : \
        ((_q) == HAL_TX_QUEUE_VI) ? WLAN_WME_AC_VI : \
        ((_q) == HAL_TX_QUEUE_BK) ? WLAN_WME_AC_BK : \
        ((_q) == HAL_TX_QUEUE_BE) ? WLAN_WME_AC_BE : \
        ((_q) == HAL_TX_QUEUE_HI) ? WLAN_WME_AC_MGMT : \
        WLAN_WME_AC_BE)
#endif

#define HAL_TX_QUEUE_MGMT               HAL_TX_QUEUE_HI     /* 0~3代表AC发送队列，4代表管理帧、控制帧发送队列 */

/*****************************************************************************
  3.3 描述符相关枚举定义
*****************************************************************************/
typedef enum
{
    HAL_TX_RATE_RANK_0 = 0,
    HAL_TX_RATE_RANK_1,
    HAL_TX_RATE_RANK_2,
    HAL_TX_RATE_RANK_3,

    HAL_TX_RATE_RANK_BUTT
}hal_tx_rate_rank_enum;
typedef oal_uint8 hal_tx_rate_rank_enum_uint8;
typedef enum
{
    HAL_DFS_RADAR_TYPE_NULL  = 0,
    HAL_DFS_RADAR_TYPE_FCC   = 1,
    HAL_DFS_RADAR_TYPE_ETSI  = 2,
    HAL_DFS_RADAR_TYPE_MKK   = 3,
    HAL_DFS_RADAR_TYPE_KOREA = 4,

    HAL_DFS_RADAR_TYPE_BUTT
}hal_dfs_radar_type_enum;
typedef oal_uint8 hal_dfs_radar_type_enum_uint8;

typedef enum
{
    HAL_RX_NEW                    = 0x0,
    HAL_RX_SUCCESS                = 0x1,
    HAL_RX_DUP_DETECTED           = 0x2,
    HAL_RX_FCS_ERROR              = 0x3,
    HAL_RX_KEY_SEARCH_FAILURE     = 0x4,
    HAL_RX_CCMP_MIC_FAILURE       = 0x5,
    HAL_RX_ICV_FAILURE            = 0x6,
    HAL_RX_TKIP_REPLAY_FAILURE    = 0x7,
    HAL_RX_CCMP_REPLAY_FAILURE    = 0x8,
    HAL_RX_TKIP_MIC_FAILURE       = 0x9,
    HAL_RX_BIP_MIC_FAILURE        = 0xA,
    HAL_RX_BIP_REPLAY_FAILURE     = 0xB,
    HAL_RX_MUTI_KEY_SEARCH_FAILURE= 0xC     /*组播广播*/
} hal_rx_status_enum;
typedef oal_uint8 hal_rx_status_enum_uint8;

typedef enum
{
    HAL_TX_INVALID   = 0,                /*无效*/
    HAL_TX_SUCC,                         /*成功*/
    HAL_TX_FAIL,                         /*发送失败（超过重传限制：接收响应帧超时）*/
    HAL_TX_TIMEOUT,                      /*lifetime超时（没法送出去）*/
    HAL_TX_RTS_FAIL,                     /*RTS 发送失败（超出重传限制：接收cts超时）*/
    HAL_TX_NOT_COMPRASS_BA,              /*收到的BA是非压缩块确认*/
    HAL_TX_TID_MISMATCH,                 /*收到的BA中TID与发送时填写在描述符中的TID不一致*/
    HAL_TX_KEY_SEARCH_FAIL,              /* Key search failed*/
    HAL_TX_AMPDU_MISMATCH,               /*描述符异常*/
    HAL_TX_PENDING,                      /*02:没有中断均为pending;03:发送过程中为pending */
    HAL_TX_FAIL_ACK_ERROR,               /*发送失败（超过重传限制：接收到的响应帧错误）*/
    HAL_TX_RTS_FAIL_CTS_ERROR,           /*RTS发送失败（超出重传限制：接收到的CTS错误）*/
    HAL_TX_FAIL_ABORT,                   /*发送失败（因为abort）*/
    HAL_TX_FAIL_STATEMACHINE_PHY_ERROR,  /*MAC发送该帧异常结束（状态机超时、phy提前结束等原因）*/
    HAL_TX_SOFT_PSM_BACK,                /*软件节能回退*/
    HAL_TX_AMPDU_BITMAP_MISMATCH,        /*硬件解析bitmap，当前mpdu未被确认*/
} hal_tx_dscr_status_enum;
typedef oal_uint8 hal_tx_status_enum_uint8;


/* 接收描述符队列状态 */
typedef enum
{
    HAL_DSCR_QUEUE_INVALID  = 0,
    HAL_DSCR_QUEUE_VALID,
    HAL_DSCR_QUEUE_SUSPENDED,

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    HAL_DSCR_QUEUE_BUSY,
    HAL_DSCR_QUEUE_IDLE,
#endif

    HAL_DSCR_QUEUE_STATUS_BUTT
}hal_dscr_queue_status_enum;
typedef oal_uint8 hal_dscr_queue_status_enum_uint8;

/* 接收描述符队列号 */
typedef enum
{
    HAL_RX_DSCR_NORMAL_PRI_QUEUE = 0,
    HAL_RX_DSCR_HIGH_PRI_QUEUE,
    HAL_RX_DSCR_SMALL_QUEUE,

    HAL_RX_DSCR_QUEUE_ID_BUTT
}hal_rx_dscr_queue_id_enum;
typedef oal_uint8 hal_rx_dscr_queue_id_enum_uint8;

/* HAL模块需要抛出的WLAN_DRX事件子类型的定义
 说明:该枚举需要和dmac_wlan_drx_event_sub_type_enum_uint8枚举一一对应 */
typedef enum
{
    HAL_WLAN_DRX_EVENT_SUB_TYPE_RX,     /* WLAN DRX 流程 */

    HAL_WLAN_DRX_EVENT_SUB_TYPE_BUTT
}hal_wlan_drx_event_sub_type_enum;
typedef oal_uint8 hal_wlan_drx_event_sub_type_enum_uint8;

/* HAL模块需要抛出的WLAN_CRX事件子类型的定义
   说明:该枚举需要和dmac_wlan_crx_event_sub_type_enum_uint8枚举一一对应 */
typedef enum
{
    HAL_WLAN_CRX_EVENT_SUB_TYPE_RX,    /* WLAN CRX 流程 */

#ifdef _PRE_WLAN_FEATURE_FTM
    HAL_EVENT_DMAC_MISC_FTM_ACK_COMPLETE,   /* FTM ACK发送完成中断 */
#endif

    HAL_WLAN_CRX_EVENT_SUB_TYPE_BUTT
}hal_wlan_crx_event_sub_type_enum;
typedef oal_uint8 hal_wlan_crx_event_sub_type_enum_uint8;

typedef enum
{
    HAL_TX_COMP_SUB_TYPE_TX,
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    HAL_TX_COMP_SUB_TYPE_AL_TX,
#endif
    HAL_TX_COMP_SUB_TYPE_BUTT
}hal_tx_comp_sub_type_enum;
typedef oal_uint8 hal_tx_comp_sub_type_enum_uint8;

typedef enum
{
    HAL_EVENT_TBTT_SUB_TYPE,

    HAL_EVENT_TBTT_SUB_TYPE_BUTT
}hal_event_tbtt_sub_type_enum;
typedef oal_uint8 hal_event_tbtt_sub_type_enum_uint8;

/* 功率模式 */
typedef enum
{
    HAL_POW_MODE_MARGIN        = 0,    /* 有余量模式: 默认 */
    HAL_POW_MODE_NO_MARGIN     = 1,    /* 没有余量模式 */

    HAL_POW_MODE_BUTT
}hal_pow_mode_enum;
typedef oal_uint8 hal_pow_mode_enum_uint8;

typedef enum
{
    HAL_POW_SET_TYPE_INIT           = 0,
    HAL_POW_SET_TYPE_REFRESH        = 1,
    HAL_POW_SET_TYPE_MAG_LVL_CHANGE = 2,
    HAL_POW_SET_TYPE_CTL_LVL_CHANGE = 3,
    HAL_POW_SET_TYPE_11B_REFRESH    = 4,

    HAL_POW_GEN_TYPE_BUTT
}hal_pow_set_type_enum;
typedef oal_uint8 hal_pow_set_type_enum_uint8;

typedef enum
{
    HAL_COEX_SW_IRQ_LTE_RX_ASSERT     = 0x1,  /* BIT0 */
    HAL_COEX_SW_IRQ_LTE_RX_DEASSERT   = 0x2,  /* BIT1 */
    HAL_COEX_SW_IRQ_LTE_TX_ASSERT     = 0x4,  /* BIT2 */
    HAL_COEX_SW_IRQ_LTE_TX_DEASSERT   = 0x8,  /* BIT3 */
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    HAL_COEX_SW_IRQ_BT                = 0x20,  /* 02芯片问题，需要配置为BIT5 */
#else
    HAL_COEX_SW_IRQ_BT                = 0x10,  /* BIT4 */
#endif

    HAL_COEX_SW_IRQ_TYPE_BUTT
}hal_coex_sw_irq_type_enum;
typedef oal_uint8 hal_coex_sw_irq_type_enum_uint8;

/* 检查是否为11b 1M速率 */
#define HAL_PHY_11B_1M_RATE(_a, _b)  ((WLAN_11B_PHY_PROTOCOL_MODE == _a) && (0 == _b))

/*****************************************************************************
  3.4 中断相关枚举定义
*****************************************************************************/
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
/* 因为mac error和dmac misc优先级一致，03将high prio做实时事件队列来处理，mac error并入dmac misc */
/****3.4.1 实时事件中断类型 ************************************************/
typedef enum
{
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_PS,        /* BTCOEX ps中断, 因为rom化，目前只能放置一个 */
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_LDAC,      /* BTCOEX LDAC中断 */

    HAL_EVENT_DMAC_HIGH_PRIO_SUB_TYPE_BUTT
}hal_event_dmac_high_prio_sub_type_enum;
typedef oal_uint8 hal_event_dmac_high_prio_sub_type_enum_uint8;

#else
/****3.4.1 芯片错误中断类型 ************************************************/
typedef enum
{
    HAL_EVENT_ERROR_IRQ_MAC_ERROR,      /* MAC错误中断时间*/
    HAL_EVENT_ERROR_IRQ_SOC_ERROR,      /* SOC错误中断事件*/

    HAL_EVENT_ERROR_IRQ_SUB_TYPE_BUTT
}hal_event_error_irq_sub_type_enum;
typedef oal_uint8 hal_event_error_irq_sub_type_enum_uint8;
#endif

/****3.4.2  MAC错误中断类型 (枚举值与错误中断状态寄存器的位一一对应!)********/
typedef enum
{
    HAL_MAC_ERROR_PARA_CFG_ERR                  = 0,        /*描述符参数配置异常,包括AMPDU长度配置不匹配,AMPDU中MPDU长度超长,sub msdu num错误*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL          = 1,        /*接收非AMSDU帧长大于RxBuff大小异常*/
#elif(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    HAL_MAC_ERROR_TX_VECTOR_ERR                 = 1,        /*发送vector中参数错误*/
#elif(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    HAL_MAC_ERROR_RESERVED_2                    = 1,
#endif
    HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND            = 2,        /*未找到BA会话表项异常0*/
    HAL_MAC_ERROR_PHY_TRLR_TIME_OUT             = 3,        /*PHY_RX_TRAILER超时*/
    HAL_MAC_ERROR_PHY_RX_FIFO_OVERRUN           = 4,        /*PHY_RX_FIFO满写异常*/
    HAL_MAC_ERROR_TX_DATAFLOW_BREAK             = 5,        /*发送帧数据断流*/
    HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT             = 6,        /*RX_FSM状态机超时*/
    HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT             = 7,        /*TX_FSM状态机超时*/
    HAL_MAC_ERROR_RX_HANDLER_ST_TIMEOUT         = 8,        /*RX_HANDLER状态机超时*/
    HAL_MAC_ERROR_TX_HANDLER_ST_TIMEOUT         = 9,        /*TX_HANDLER状态机超时*/
    HAL_MAC_ERROR_TX_INTR_FIFO_OVERRUN          = 10,       /*TX 中断FIFO满写*/
    HAL_MAC_ERROR_RX_INTR_FIFO_OVERRUN          = 11,       /*RX中断 FIFO满写*/
    HAL_MAC_ERROR_HIRX_INTR_FIFO_OVERRUN        = 12,       /*HIRX中断FIFO满写*/
    HAL_MAC_ERROR_UNEXPECTED_RX_Q_EMPTY         = 13,       /*接收到普通优先级帧但此时RX BUFFER指针为空*/
    HAL_MAC_ERROR_UNEXPECTED_HIRX_Q_EMPTY       = 14,       /*接收到高优先级帧但此时HI RX BUFFER指针为空*/
    HAL_MAC_ERROR_BUS_RLEN_ERR                  = 15,       /*总线读请求长度为0异常*/
    HAL_MAC_ERROR_BUS_RADDR_ERR                 = 16,       /*总线读请求地址无效异常*/
    HAL_MAC_ERROR_BUS_WLEN_ERR                  = 17,       /*总线写请求长度为0异常*/
    HAL_MAC_ERROR_BUS_WADDR_ERR                 = 18,       /*总线写请求地址无效异常*/
    HAL_MAC_ERROR_TX_ACBK_Q_OVERRUN             = 19,       /*tx acbk队列fifo满写*/
    HAL_MAC_ERROR_TX_ACBE_Q_OVERRUN             = 20,       /*tx acbe队列fifo满写*/
    HAL_MAC_ERROR_TX_ACVI_Q_OVERRUN             = 21,       /*tx acvi队列fifo满写*/
    HAL_MAC_ERROR_TX_ACVO_Q_OVERRUN             = 22,       /*tx acv0队列fifo满写*/
    HAL_MAC_ERROR_TX_HIPRI_Q_OVERRUN            = 23,       /*tx hipri队列fifo满写*/
    HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT           = 24,       /*matrix计算超时*/
    HAL_MAC_ERROR_CCA_TIME_OUT                  = 25,       /*cca超时*/
    HAL_MAC_ERROR_DCOL_DATA_OVERLAP             = 26,       /*数采overlap告警*/
    HAL_MAC_ERROR_BEACON_MISS                   = 27,       /*连续发送beacon失败*/
    HAL_MAC_ERROR_INTR_FIFO_UNEXPECTED_READ     = 28,       /*interrupt fifo空读异常*/
    HAL_MAC_ERROR_UNEXPECTED_RX_DESC_ADDR       = 29,       /*rx desc地址错误异常*/
    HAL_MAC_ERROR_RX_OVERLAP_ERR                = 30,       /*mac没有处理完前一帧,phy又上报了一帧异常*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && defined(_PRE_WLAN_PRODUCT_1151V200)
    HAL_MAC_ERROR_NAV_THRESHOLD_ERR             = 31,       /* NAV阈值超时 */
#else
    HAL_MAC_ERROR_RESERVED_31                   = 31,       /* 保留位 */
#endif
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    HAL_MAC_ERROR_TX_ACBE_BACKOFF_TIMEOUT       = 32,       /*发送BE队列退避超时*/
    HAL_MAC_ERROR_TX_ACBK_BACKOFF_TIMEOUT       = 33,       /*发送BK队列退避超时*/
    HAL_MAC_ERROR_TX_ACVI_BACKOFF_TIMEOUT       = 34,       /*发送VI队列退避超时*/
    HAL_MAC_ERROR_TX_ACVO_BACKOFF_TIMEOUT       = 35,       /*发送VO队列退避超时*/
    HAL_MAC_ERROR_TX_HIPRI_BACKOFF_TIMEOUT      = 36,       /*发送高优先级队列退避超时*/
    HAL_MAC_ERROR_RX_SMALL_Q_EMPTY              = 37,       /*接收普通队列的小包，但是小包队列指针为空*/
    HAL_MAC_ERROR_PARA_CFG_2ERR                 = 38,       /*发送描述符中AMPDU中MPDU长度过长*/
    HAL_MAC_ERROR_PARA_CFG_3ERR                 = 39,       /*发送描述符中11a，11b，11g发送时，mpdu配置长度超过4095*/
    HAL_MAC_ERROR_EDCA_ST_TIMEOUT               = 40,       /*CH_ACC_EDCA_CTRL状态机超时*/
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    HAL_MAC_ERROR_PARA_CFG_4ERR                 = 41,       /*发送描述符中11a/b/g发送时，打开了ampdu使能*/
    HAL_MAC_ERROR_TX_BC_Q_OVERRUN               = 42,       /* TX广播帧队列fifo满写错误，最后一次写的地址丢弃*/
    HAL_MAC_ERROR_BSS_NAV_PORT                  = 43,       /*接收到本bss的帧，duration很大，nav保护起作用*/
    HAL_MAC_ERROR_OBSS_NAV_PORT                 = 44,       /*接收到其他obss的帧，duration很大，nav保护起作用*/
    HAL_MAC_ERROR_BUS_RW_TIMEOUT                = 45,       /*读写访问超时*/
#endif
#endif
    HAL_MAC_ERROR_TYPE_BUTT
}hal_mac_error_type_enum;
typedef oal_uint8 hal_mac_error_type_enum_uint8;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
/****3.4.3 SOC错误中断类型 (需要在DMAC模块进行处理的error irq的类型定义)*****/
typedef enum
{
    /* SOC错误中断 */
    HAL_SOC_ERROR_BUCK_OCP,                                                 /* PMU BUCK过流中断 */
    HAL_SOC_ERROR_BUCK_SCP,                                                 /* PMU BUCK短路中断 */
    HAL_SOC_ERROR_OCP_RFLDO1,                                               /* PMU RFLDO1过流中断 */
    HAL_SOC_ERROR_OCP_RFLDO2,                                               /* PMU RFLDO2过流中断 */
    HAL_SOC_ERROR_OCP_CLDO,                                                 /* PMU CLDO过流中断 */
    HAL_SOC_ERROR_RF_OVER_TEMP,                                             /* RF过热中断 */
    HAL_SOC_ERROR_CMU_UNLOCK,                                               /* CMU PLL失锁中断 */
    HAL_SOC_ERROR_PCIE_SLV_ERR,                                             /* PCIE总线错误中断 */

    HAL_SOC_ERROR_TYPE_BUTT
}hal_soc_error_type_enum;
typedef oal_uint8 hal_soc_error_type_enum_uint8;
#endif

/* DMAC MISC 子事件枚举定义 */
typedef enum
{
    HAL_EVENT_DMAC_MISC_CH_STATICS_COMP,    /* 信道统计/测量完成中断 */
    HAL_EVENT_DMAC_MISC_RADAR_DETECTED,     /* 检测到雷达信号 */
    HAL_EVENT_DMAC_MISC_DFS_AUTH_CAC,       /* DFS认证CAC测试 */
    HAL_EVENT_DMAC_MISC_DBAC,               /* DBAC */
    HAL_EVENT_DMAC_MISC_MWO_DET,            /* 微波炉识别中断 */
#ifdef _PRE_WLAN_DFT_REG
    HAL_EVENT_DMAC_REG_REPORT,
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    HAL_EVENT_DMAC_BT_A2DP,
    HAL_EVENT_DMAC_BT_LDAC,
    HAL_EVENT_DMAC_BT_SCO,
    HAL_EVENT_DMAC_BT_TRANSFER,
    HAL_EVENT_DMAC_BT_PAGE_SCAN,
    HAL_EVENT_DMAC_BT_INQUIRY,
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    HAL_EVENT_DMAC_P2P_NOA_ABSENT_START,
    HAL_EVENT_DMAC_P2P_NOA_ABSENT_END,
    HAL_EVENT_DMAC_P2P_CTWINDOW_END,
#endif
    HAL_EVENT_DMAC_BEACON_TIMEOUT,          /* 等待beacon帧超时 */
    HAL_EVENT_DMAC_CALI_TO_HMAC,            /* 校准数据从dmac抛到hmac */

#ifdef _PRE_WLAN_ONLINE_DPD
    HAL_EVENT_DMAC_DPD_TO_HMAC,
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
    HAL_EVENT_DMAC_AUTOCALI_TO_HMAC,        /* 自动化校准数据从dmac抛到hmac */
#endif
    HAL_EVENT_DMAC_MISC_WOW_WAKE,

    HAL_EVENT_DMAC_MISC_GREEN_AP,           /* Green ap timer*/
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    HAL_EVENT_DMAC_DUAL_ANTENNA_SWITCH,
#endif
#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
    HAL_EVENT_DMAC_MISC_PSD_COMPLETE,       /* PSD采集完成完成中断 */
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
    HAL_EVENT_DMAC_MISC_CSI_COMPLETE,       /* CSI完成中断 */
#endif


#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    HAL_EVENT_DMAC_MISC_IPC_IRQ,       /* IPC中断 */
#endif

#ifdef _PRE_WLAN_FEATURE_NO_FRM_INT
    HAL_EVENT_DMAC_MISC_BCN_NO_FRM,          /* 硬件解析无缓存帧中断 */
#endif

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    HAL_EVENT_ERROR_IRQ_MAC_ERROR,          /* MAC错误中断时间*/
    HAL_EVENT_ERROR_IRQ_SOC_ERROR,          /* SOC错误中断事件*/
#endif
    HAL_EVENT_DMAC_MISC_CHR,                /* dmac处理上报的chr事件*/

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    HAL_EVENT_DMAC_BT_ASSOC_AP_CHECK,
#endif

    HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT
}hal_dmac_misc_sub_type_enum;
typedef oal_uint8  hal_dmac_misc_sub_type_enum_uint8;

/*****************************************************************************
  3.5 复位相关枚举定义
*****************************************************************************/
/****3.5.1  复位事件子类型定义 **********************************************/
typedef enum
{
    HAL_RESET_HW_TYPE_ALL = 0,
    HAL_RESET_HW_TYPE_PHY,
    HAL_RESET_HW_TYPE_MAC,
    HAL_RESET_HW_TYPE_RF,
    HAL_RESET_HW_TYPE_MAC_PHY,
    HAL_RESET_HW_TYPE_TCM,
    HAL_RESET_HW_TYPE_MAC_TSF,
    HAL_RESET_HW_TYPE_MAC_CRIPTO,
    HAL_RESET_HW_TYPE_MAC_NON_CRIPTO,
    HAL_RESET_HW_TYPE_PHY_RADAR,
    HAL_RESET_HW_NORMAL_TYPE_PHY,
    HAL_RESET_HW_NORMAL_TYPE_MAC,
    HAL_RESET_HW_TYPE_DUMP_MAC,
    HAL_RESET_HW_TYPE_DUMP_PHY,

    HAL_RESET_HW_TYPE_BUTT
}hal_reset_hw_type_enum;
typedef oal_uint8 hal_reset_hw_type_enum_uint8;

/****3.5.1  复位MAC子模块定义 **********************************************/
typedef enum
{
    HAL_RESET_MAC_ALL = 0,
    HAL_RESET_MAC_PA ,
    HAL_RESET_MAC_CE,
    HAL_RESET_MAC_TSF,
    HAL_RESET_MAC_DUP,
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    HAL_RESET_MAC_LOGIC,
#endif
    HAL_RESET_MAC_BUTT
}hal_reset_mac_submod_enum;
typedef oal_uint8 hal_reset_mac_submod_enum_uint8;

typedef enum
{
   HAL_LPM_SOC_BUS_GATING       = 0,
   HAL_LPM_SOC_PCIE_RD_BYPASS   = 1,
   HAL_LPM_SOC_MEM_PRECHARGE    = 2,
   HAL_LPM_SOC_PCIE_L0          = 3,
   HAL_LPM_SOC_PCIE_L1_PM       = 4,
   HAL_LPM_SOC_AUTOCG_ALL       = 5,
   HAL_LPM_SOC_ADC_FREQ         = 6,
   HAL_LPM_SOC_PCIE_L1S         = 7,

   HAL_LPM_SOC_SET_BUTT
}hal_lpm_soc_set_enum;
typedef oal_uint8   hal_lpm_soc_set_enum_uint8;

#if defined(_PRE_WLAN_FEATURE_SMPS) || defined(_PRE_WLAN_CHIP_TEST)
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
/* SMPS模式设置:  1151
  00 - SMPS_DISABLE
  01 - SMPS_STATIC
  10 - SMPS_DYNAMIC
  11 - RESERVED  */
typedef enum
{
    HAL_SMPS_MODE_DISABLE = 0,
    HAL_SMPS_MODE_STATIC = 1 ,
    HAL_SMPS_MODE_DYNAMIC = 2,

    HAL_SMPS_MODE_BUTT
}hal_smps_mode_enum;
#else
/*SMPS模式配置：  1103
  00：SMPS_STATIC（始终单路接收）
  01：SMPS_DYNAMIC
  10：reserved
  11：SMPS_DISABLE（始终多路接收）*/
typedef enum
{
    HAL_SMPS_MODE_STATIC = 0,
    HAL_SMPS_MODE_DYNAMIC = 1,
    HAL_SMPS_MODE_DISABLE = 3,

    HAL_SMPS_MODE_BUTT
}hal_smps_mode_enum;
#endif

typedef oal_uint8 hal_smps_mode_enum_uint8;
#endif

typedef enum
{
    HAL_ALG_ISR_NOTIFY_DBAC,
    HAL_ALG_ISR_NOTIFY_MWO_DET,
    HAL_ALG_ISR_NOTIFY_ANTI_INTF,

    HAL_ALG_ISR_NOTIFY_BUTT,
}hal_alg_noify_enum;
typedef oal_uint8 hal_alg_noify_enum_uint8;

typedef enum
{
    HAL_ISR_TYPE_TBTT,
    HAL_ISR_TYPE_ONE_PKT,
    HAL_ISR_TYPE_MWO_DET,
    HAL_ISR_TYPE_NOA_START,
    HAL_ISR_TYPE_NOA_END,

    HAL_ISR_TYPE_BUTT,
}hal_isr_type_enum;
typedef oal_uint8 hal_isr_type_enum_uint8;



/*性能测试相关*/
typedef enum {
    HAL_ALWAYS_TX_DISABLE,         /* 禁用常发 */
    HAL_ALWAYS_TX_RF,              /* 保留给RF测试广播报文*/
    HAL_ALWAYS_TX_AMPDU_ENABLE,    /* 使能AMPDU聚合包常发 */
    HAL_ALWAYS_TX_MPDU,            /* 使能非聚合包常发 */
    HAL_ALWAYS_TX_BUTT
}hal_device_always_tx_state_enum;
typedef oal_uint8 hal_device_always_tx_enum_uint8;


typedef enum {
    HAL_ALWAYS_RX_DISABLE,         /* 禁用常收 */
    HAL_ALWAYS_RX_RESERVED,        /* 保留给RF测试广播报文*/
    HAL_ALWAYS_RX_AMPDU_ENABLE,    /* 使能AMPDU聚合包常收 */
    HAL_ALWAYS_RX_ENABLE,          /* 使能非聚合包常收 */
    HAL_ALWAYS_RX_BUTT
}hal_device_always_rx_state_enum;
typedef oal_uint8 hal_device_always_rx_enum_uint8;

typedef enum
{
    WLAN_PHY_RATE_1M                = 0,    /* 0000 */
    WLAN_PHY_RATE_2M                = 1,    /* 0001 */
    WLAN_PHY_RATE_5HALF_M           = 2,    /* 0010 */
    WLAN_PHY_RATE_11M               = 3,    /* 0011 */

    WLAN_PHY_RATE_48M               = 8,    /* 1000 */
    WLAN_PHY_RATE_24M               = 9,    /* 1001 */
    WLAN_PHY_RATE_12M               = 10,   /* 1010 */
    WLAN_PHY_RATE_6M                = 11,   /* 1011 */

    WLAN_PHY_RATE_54M               = 12,   /* 1100 */
    WLAN_PHY_RATE_36M               = 13,   /* 1101 */
    WLAN_PHY_RATE_18M               = 14,   /* 1110 */
    WLAN_PHY_RATE_9M                = 15,   /* 1111 */

    WLAN_PHY_RATE_BUTT
}wlan_phy_rate_enum;

#ifdef _PRE_WLAN_FEATURE_DFS
typedef enum
{
    HAL_RADAR_NOT_REPORT = 0,
    HAL_RADAR_REPORT,
}hal_radar_filter_enum;
typedef oal_uint8 hal_radar_filter_enum_uint8;
#endif

typedef enum
{
    HAL_VAP_STATE_INIT               = 0,
    HAL_VAP_STATE_CONNECT            = 1,       /* sta独有 */
    HAL_VAP_STATE_UP                 = 2,       /* VAP UP */
    HAL_VAP_STATE_PAUSE              = 3,       /* pause , for ap &sta */

    HAL_VAP_STATE_BUTT
}hal_vap_state_enum;
typedef oal_uint8  hal_vap_state_enum_uint8;

/*****************************************************************************
  3.6 加密相关枚举定义
*****************************************************************************/
/****3.6.1  芯片密钥类型定义 ************************************************/

typedef enum
{
    HAL_KEY_TYPE_TX_GTK              = 0,       /*Hi1102:HAL_KEY_TYPE_TX_IGTK */
    HAL_KEY_TYPE_PTK                 = 1,
    HAL_KEY_TYPE_RX_GTK              = 2,
    HAL_KEY_TYPE_RX_GTK2             = 3,       /* 02使用，03和51不使用 */
    HAL_KEY_TYPE_BUTT
} hal_cipher_key_type_enum;
typedef oal_uint8 hal_cipher_key_type_enum_uint8;

/****3.6.2  芯片加密算法类型对应芯片中的值 **********************************/
typedef enum
{
    HAL_WEP40                      = 0,
    HAL_TKIP                       = 1,
    HAL_CCMP                       = 2,
    HAL_NO_ENCRYP                  = 3,
    HAL_WEP104                     = 4,
    HAL_BIP                        = 5,
    HAL_GCMP                       = 6,
    HAL_GCMP_256                   = 7,
    HAL_CCMP_256                   = 8,
    HAL_BIP_256                    = 9,
    HAL_CIPER_PROTOCOL_TYPE_BUTT
} hal_cipher_protocol_type_enum;
typedef oal_uint8 hal_cipher_protocol_type_enum_uint8;

/****3.6.3  芯片填写加密寄存器CE_LUT_CONFIG AP/STA **************************/
typedef enum
{
    HAL_AUTH_KEY = 0,      /* 表明该设备为认证者 */
    HAL_SUPP_KEY = 1,      /* 表明该设备为申请者 */

    HAL_KEY_ORIGIN_BUTT,
} hal_key_origin_enum;
typedef oal_uint8 hal_key_origin_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_SMARTANT
typedef enum
{
    HAL_DUAL_ANTENNA_FEATURE_CHANGE = 0,
    HAL_DUAL_ANTENNA_TEMP_INTERRUPT = 1,
    HAL_DUAL_ANTENNA_BUTT,
} hal_dual_antenna_switch_type_enum;
typedef oal_uint8 hal_dual_antenna_switch_type_enum_uint8;
#endif


#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI

typedef enum
{
    HAL_DYN_CALI_PDET_ADJUST_INIT = 0,
    HAL_DYN_CALI_PDET_ADJUST_ASCEND,       /* while real_pdet < expect_pdet */
    HAL_DYN_CALI_PDET_ADJUST_DECLINE,      /* while real_pdet > expect_pdet */
    HAL_DYN_CALI_PDET_ADJUST_VARIED,
    HAL_DYN_CALI_PDET_ADJUST_BUTT,
} hal_dyn_cali_adj_type_enum;
typedef oal_uint8 hal_dyn_cali_adj_type_enum_uint8;

#endif


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
/* NVRAM 参数结构体 FCC认证 非FCC */
#ifdef _PRE_WLAN_1103_PILOT
typedef struct
{
    oal_uint8       uc_max_txpower;     /* 最大发送功率 */
}hal_cfg_custom_nvram_params_stru;
#else
typedef struct
{
    oal_uint8       uc_index;           /* 下标表示偏移 */
    oal_uint8       uc_max_txpower;     /* 最大发送功率 */
    oal_uint16      us_dbb_scale;       /* DBB幅值 */
}hal_cfg_custom_nvram_params_stru;
#endif

typedef struct hal_pwr_fit_para_stru
{
    oal_int32 l_pow_par2;   /* 二次项系数 */
    oal_int32 l_pow_par1;   /* 一次 */
    oal_int32 l_pow_par0;   /* 常数项 */
}hal_pwr_fit_para_stru;

typedef struct hal_pwr_efuse_amend_stru
{
    oal_int16 s_efuse_gain;   /* pdbuf-VGA Gain */
    oal_int16 s_efuse_dc;     /* pdbuf-VGA offset */
}hal_pwr_efuse_amend_stru;

/* FCC边带功率定制项 */
typedef struct
{
    oal_uint8 auc_5g_fcc_txpwr_limit_params_20M[HAL_NUM_5G_20M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_40M[HAL_NUM_5G_40M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_80M[HAL_NUM_5G_80M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_160M[HAL_NUM_5G_160M_SIDE_BAND];
    oal_uint8 auc_2g_fcc_txpwr_limit_params[WLAN_2G_SUB_BAND_NUM][HAL_CUS_NUM_FCC_2G_PRO];
}hal_cfg_custom_fcc_txpwr_limit_stru;

typedef struct
{
    oal_int8                   c_delta_cca_ed_high_20th_2g;
    oal_int8                   c_delta_cca_ed_high_40th_2g;
    oal_int8                   c_delta_cca_ed_high_20th_5g;
    oal_int8                   c_delta_cca_ed_high_40th_5g;
}hal_cfg_custom_cca_stru;

typedef struct
{
    hal_fcs_protect_type_enum_uint8     en_protect_type;
    hal_fcs_protect_cnt_enum_uint8      en_protect_cnt;
    oal_uint16                          bit_protect_coex_pri          :2;     /* btcoex下使用，one pkt发送优先级 */
    oal_uint16                          bit_cfg_one_pkt_tx_vap_index  :4;
    oal_uint16                          bit_cfg_one_pkt_tx_peer_index :5;
    oal_uint16                          bit_rsv                       :5;
    oal_uint32                          ul_tx_mode;
    oal_uint32                          ul_tx_data_rate;
    oal_uint16                          us_duration;    /* 单位 us */
    oal_uint16                          us_timeout;
    oal_uint16                          us_wait_timeout;     /* 软件定时器超时时间 */
    oal_uint8                           auc_rsv[2];
    oal_uint8                           auc_protect_frame[HAL_FCS_PROT_MAX_FRAME_LEN];
}hal_one_packet_cfg_stru;

typedef struct
{
    oal_bool_enum_uint8     en_mac_in_one_pkt_mode  : 1;
    oal_bool_enum_uint8     en_self_cts_success     : 1;
    oal_bool_enum_uint8     en_null_data_success    : 1;
    oal_uint32              ul_resv                 : 5;
}hal_one_packet_status_stru;

typedef struct
{
    oal_uint8     uc_pn_tid;          /* tid,0~7, 对rx pn lut有效 */
    oal_uint8     uc_pn_peer_idx;     /* 对端peer索引,0~31 */
    oal_uint8     uc_pn_key_type;     /* 1151 0:multicast,1:unicast */
                                      /* 1102 tx pn: 0x0：GTK(multicast) 0x1：PTK(unicast) 0x2：IGTK others：reserved*/
                                      /* 1102 rx pn: 0x0：组播/广播数据帧 0x1：单播qos数据帧 0x2：单播非qos数据帧
                                         0x3：单播管理帧  0x4：组播/广播管理帧 others：保留 */
    oal_uint8     uc_all_tid;         /* 0:仅配置TID,1:所有TID 对rx pn lut有效*/
    oal_uint32    ul_pn_msb;          /* pn值的高32位,写操作时做入参，读操作时做返回值 */
    oal_uint32    ul_pn_lsb;          /* pn值的低32位，写操作时做入参，读操作时做返回值 */
}hal_pn_lut_cfg_stru;

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
/* pn号结构体 */
typedef struct
{
    oal_uint32  ul_pn_msb;              /* pn值的高32位 */
    oal_uint32  ul_pn_lsb;              /* pn值的低32位 */
}hal_pn_stru;
#endif

#ifdef _PRE_WLAN_PRODUCT_1151V200
/* 1151v200 WITP_PA_PEER_RESP_DIS_REG cfg struct */
typedef struct
{
    oal_uint8   uc_lut_index;           /* lut index */
    oal_uint8   uc_peer_resp_dis;       /* 1:表示无论ack policy为何均不回复响应帧 */
    oal_uint8   auc_resv[2];
}hal_peer_resp_dis_cfg_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct {
    oal_uint16 bit_bt_on            : 1,
               bit_bt_cali          : 1,
               bit_bt_ps            : 1,
               bit_bt_inquiry       : 1,
               bit_bt_page          : 1,
               bit_bt_acl           : 1,    /* data_send */
               bit_bt_a2dp          : 1,
               bit_bt_sco           : 1,
               bit_bt_data_trans    : 1,    /* data_resv */
               bit_bt_acl_num       : 3,
               bit_bt_link_role     : 4;
} bt_status_stru;

typedef union {
    oal_uint16 us_bt_status_reg;
    bt_status_stru st_bt_status;
} btcoex_bt_status_union;

typedef struct {
    oal_uint16 bit_ble_on           : 1,
               bit_ble_scan         : 1,
               bit_ble_con          : 1,
               bit_ble_adv          : 1,
               bit_bt_transfer      : 1,  /* not use (only wifi self) */
               bit_bt_6slot         : 2,
               bit_ble_init         : 1,
               bit_bt_acl           : 1,
               bit_bt_ldac          : 2,  /* 扩展1bit 0-sbc 1-APTXHD 2-660 3-990 */
               bit_bt_hid           : 1,
               bit_ble_hid          : 1,
               bit_resv             : 1,
               bit_bt_siso_ap       : 1,  /* 是否申请切换到切siso 当前考虑a2dp业务，其他业务暂时hold */
               bit_bt_ba            : 1;
} ble_status_stru;

typedef union {
    oal_uint16 us_ble_status_reg;
    ble_status_stru st_ble_status;
} btcoex_ble_status_union;

typedef struct hal_btcoex_btble_status {
    btcoex_bt_status_union un_bt_status;
    btcoex_ble_status_union un_ble_status;
} hal_btcoex_btble_status_stru;

typedef struct
{
    oal_uint32 ul_abort_start_cnt;
    oal_uint32 ul_abort_done_cnt;
    oal_uint32 ul_abort_end_cnt;
    oal_uint32 ul_preempt_cnt;
    oal_uint32 ul_post_preempt_cnt;
    oal_uint32 ul_post_premmpt_fail_cnt;
    oal_uint32 ul_abort_duration_on;
    oal_uint32 ul_abort_duration_start_us;
    oal_uint32 ul_abort_duration_us;
    oal_uint32 ul_abort_duration_s;
} hal_btcoex_statistics_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_SMARTANT
typedef struct
{
    oal_uint8 bit_lte_rx_act  : 1,
             bit_bt_on       : 1,
             bit_roam        : 1,
             bit_vap_mode    : 1,
             bit_ps_sleep    : 1,
             bit_ps_wake     : 1,
             bit_resv        : 2;
} hal_dual_antenna_check_status_stru;
#endif

/*****************************************************************************
  7.0 寄存器配置结构
*****************************************************************************/
/*lint -e958*/
#if (_PRE_WLAN_CHIP_VERSION == _PRE_WLAN_CHIP_FPGA_HI1101RF)
struct witp_reg_cfg
{
    oal_uint16    us_soft_index;
    oal_uint8     uc_addr;
    oal_uint32    ul_val;
}__OAL_DECLARE_PACKED;
#elif(_PRE_WLAN_CHIP_VERSION == _PRE_WLAN_CHIP_FPGA_HI1151RF) /* End of _PRE_WLAN_CHIP_FPGA_HI1101RF*/
struct witp_reg16_cfg
{
    oal_uint16    us_addr;
    oal_uint16    us_val;
}__OAL_DECLARE_PACKED;
typedef struct witp_reg16_cfg witp_reg16_cfg_stru;

struct witp_reg_cfg
{
    oal_uint16    us_addr;
    oal_uint16    us_val;
}__OAL_DECLARE_PACKED;
#elif(_PRE_WLAN_CHIP_VERSION == _PRE_WLAN_CHIP_FPGA) /* End of _PRE_WLAN_CHIP_FPGA_HI1151RF*/
struct witp_reg16_cfg
{
    oal_uint16    us_addr;
    oal_uint16    us_val;
}__OAL_DECLARE_PACKED;
typedef struct witp_reg16_cfg witp_reg16_cfg_stru;

struct witp_reg_cfg
{
    oal_uint16    us_addr;
    oal_uint16    us_val;
}__OAL_DECLARE_PACKED;
#elif(_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)    /* End of _PRE_WLAN_CHIP_FPGA*/
struct witp_reg16_cfg
{
    oal_uint16    us_addr;
    oal_uint16    us_val;
}__OAL_DECLARE_PACKED;
typedef struct witp_reg16_cfg witp_reg16_cfg_stru;

struct witp_reg_cfg
{
    oal_uint16   us_addr;
    oal_uint16   us_val;
}__OAL_DECLARE_PACKED;
#endif /* End of _PRE_WLAN_CHIP_ASIC */

typedef struct witp_reg_cfg witp_reg_cfg_stru;

struct witp_single_tune_reg_cfg
{
    oal_uint16    us_addr;
    oal_int32     ul_val;
}__OAL_DECLARE_PACKED;

typedef struct witp_single_tune_reg_cfg witp_single_tune_reg_cfg_stru;

/*lint +e958*/
/*****************************************************************************
  7.1 基准发送描述符定义
*****************************************************************************/
typedef struct tag_hal_tx_dscr_stru
{
    oal_dlist_head_stru                 st_entry;
    oal_netbuf_stru                    *pst_skb_start_addr;         /* Sub MSDU 0 Skb Address */
    oal_uint16                          us_original_mpdu_len;       /* mpdu长度 含帧头 */
    hal_tx_queue_type_enum_uint8        uc_q_num;                   /* 发送队列队列号 */
    oal_uint8                           bit_is_retried          :1; /* 是不是重传包 */
    oal_uint8                           bit_is_ampdu            :1; /* 是不是ampdu */
    oal_uint8                           bit_is_rifs             :1; /* 是不是rifs发送 */
    oal_uint8                           bit_is_first            :1; /* 标志是否是第一个描述符 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    oal_uint8                           bit_tx_hal_vap_id       :4; /* Proxy STA的tx hal_vap_id */
#else
    oal_uint8                           bit_resv                 : 4;
#endif
    oal_uint8                           data[4];
}hal_tx_dscr_stru;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define HAL_PSTA_ORI2ID(ori_id) ((ori_id) ? (ori_id) - HAL_PROXY_STA_START_IDX + 1 : 0)
#define HAL_PSTA_ID2ORI(id) ((id) + HAL_PROXY_STA_START_IDX - 1)
#endif
/*****************************************************************************
  7.2 基准接收描述符定义
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
typedef struct tag_hal_rx_dscr_stru
{
    oal_dlist_head_stru         st_entry;
    oal_netbuf_stru            *pst_skb_start_addr;          /* 描述符中保存的netbuf的首地址 */
    oal_uint8                   data[4];
}hal_rx_dscr_stru;
#else
typedef struct tag_hal_rx_dscr_stru
{
    oal_uint32                 *pul_prev_rx_dscr;           /* 前一个描述符的地址           */
    oal_uint32                  ul_skb_start_addr;          /* 描述符中保存的netbuf的首地址 */
    oal_uint32                 *pul_next_rx_dscr;           /* 前一个描述符的地址(物理地址) */
}hal_rx_dscr_stru;
#endif
/*****************************************************************************
  7.3 对外部发送提供接口所用数据结构
*****************************************************************************/

/*****************************************************************************
  结构名  : hal_channel_matrix_dsc_stru
  结构说明: 矩阵信息结构体
*****************************************************************************/
typedef struct
{
    /*(第10 23行) */
    oal_uint8                            bit_codebook              : 2;
    oal_uint8                            bit_grouping              : 2;
    oal_uint8                            bit_row_num               : 4;

    oal_uint8                            bit_column_num            : 4;
    oal_uint8                            bit_response_flag         : 1;     /* 在Tx 描述符中不用填写该字段;发送完成中断后，将有无信道矩阵信息存储在此 */
    oal_uint8                            bit_reserve1              : 3;

    oal_uint16                           us_channel_matrix_length;          /*信道矩阵的总字节(Byte)数 */
    oal_uint32                           ul_steering_matrix;                /* txbf需要使用的矩阵地址,填写发送描述符时候使用 */
}hal_channel_matrix_dsc_stru;

typedef struct
{
    /* PHY TX MODE 1(第13行) */
    /* (1) 速率自适应填写 */
    oal_uint8                               uc_extend_spatial_streams;      /* 扩展空间流个数 */
    wlan_channel_code_enum_uint8            en_channel_code;                /* 信道编码(BCC或LDPC) */

    /* (2) ACS填写 */
    hal_channel_assemble_enum_uint8         en_channel_bandwidth;           /* 工作带宽 */

    oal_uint8                               bit_lsig_txop       : 1;        /* L-SIG TXOP保护 0:不开启保护，1: 开启保护*/
    oal_uint8                               bit_reserved        : 4;
    oal_uint8                               bit_he_max_pe_fld         : 2;
    oal_uint8                               bit_uplink_flag           : 1;

    oal_uint8                               dyn_bandwidth_in_non_ht;        /* 如果是本设备或者对端设备不是VHT设备，或者寄存器设置的速率为non-HT速率，那么该字段无意义（可填0)*/
    oal_uint8                               dyn_bandwidth_in_non_ht_exist;  /* 如果本设备和对端设备都为VHT设备，并且寄存器设置的速率为non-HT速率，那么该字段填1 */
    oal_uint8                               ch_bandwidth_in_non_ht_exist;   /* 如果本设备和对端设备都为VHT设备，并且寄存器设置的速率为non-HT速率，那么该字段填1 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* 02芯片测试添加抗干扰变量，为tx描述符赋值 */
    oal_uint8                               bit_anti_intf_1thr              : 2;
    oal_uint8                               bit_anti_intf_0thr              : 2;
    oal_uint8                               bit_anti_intf_en                : 1;
    oal_uint8                               bit_reserve                     : 3;
#endif
    oal_uint8                               uc_smoothing;                   /* 通知接收端是否对信道矩阵做平滑 */
    wlan_sounding_enum_uint8                en_sounding_mode;               /* sounding模式 */
}hal_tx_txop_rate_params_stru;

typedef union
{
    oal_uint32  ul_value;
    /* (第14 19 20 21行) */
    struct
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        oal_uint8   bit_tx_count              : 2;                      /* 传输次数 */
        oal_uint8   bit_stbc_mode             : 2;                      /* 空时分组编码 */
#else
#ifdef _PRE_WLAN_1103_PILOT
        oal_uint8   bit_tx_count              : 3;                      /* 传输次数 */
        oal_uint8   bit_tx_he_enable          : 1;                      /*HE 使能，0==非HE,1==HE*/
#else
        oal_uint8   bit_tx_count              : 4;                      /* 传输次数 */
#endif
#endif
        oal_uint8   bit_tx_chain_selection    : 4;                      /* 发送通道选择 (单通道:0x1, 双通道:0x3, 三通道:0x7, 四通道:0xf) */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        oal_uint8   bit_stbc_mode             : 2;                      /* 1103 stbc mode */
        oal_uint8   bit_tx_antenna            : 2;                      /* 1103 发送天线选择 */
        oal_uint8   bit_reserve2              : 4;
#else
        oal_uint8   bit_tx_antenna            : 8;                      /* 1151描述符中定义的tx antenna为8bit,保留8个bit，命名统一 */
#endif

        union
        {
        #ifdef _PRE_WLAN_FEATURE_11AX
            struct
            {
                oal_uint8   bit_he_mcs       : 4;
                oal_uint8   bit_nss_mode     : 2;                       /* 该速率对应的空间流枚举值 */
                oal_uint8   bit_ppdu_format  : 2;                       /* HE发帧类型 */
            } st_he_nss_mcs;
        #endif
            struct
            {
                oal_uint8   bit_vht_mcs       : 4;
                oal_uint8   bit_nss_mode      : 2;                       /* 该速率对应的空间流枚举值 */
                oal_uint8   bit_protocol_mode : 2;                       /* 协议模式 */
            } st_vht_nss_mcs;
            struct
            {
                oal_uint8   bit_ht_mcs        : 6;
                oal_uint8   bit_protocol_mode : 2;                       /* 协议模式 */
            } st_ht_rate;
            struct
            {
                oal_uint8   bit_legacy_rate   : 4;
                oal_uint8   bit_reserved1     : 2;
                oal_uint8   bit_protocol_mode : 2;                       /* 协议模式 */
            } st_legacy_rate;
        } un_nss_rate;

        oal_uint8   bit_rts_cts_enable        : 1;                      /* 是否使能RTS */
        oal_uint8   bit_txbf_mode             : 2;                      /* txbf模式 */
        oal_uint8   bit_preamble_mode         : 1;                      /* 前导码 */
#ifdef _PRE_WLAN_1103_PILOT
        oal_uint8   bit_short_gi_enable       : 1;
        oal_uint8   bit_short_gi_type_tx      : 1;
        oal_uint8   bit_tx_he_ltf_type        : 2;
#else
        oal_uint8   bit_short_gi_enable     : 1;   /* 短保护间隔 */
        oal_uint8   bit_reserve             : 3;
#endif
    }rate_bit_stru;
}hal_tx_txop_per_rate_params_union;

typedef struct
{
    /* PHY TX MODE 2 (第15行) */
    oal_uint8                               uc_tx_rts_antenna;          /* 发送RTS使用的天线组合 */
    oal_uint8                               uc_rx_ctrl_antenna;         /* 接收CTS/ACK/BA使用的天线组合 */
    oal_uint8                               auc_reserve1[1];            /* TX VAP index 不是算法填写，故在此也填0 */
    oal_uint8                               bit_txop_ps_not_allowed: 1; /* 0代表允许TXOP POWER save，1代表不允许TXOP POWER save */
    oal_uint8                               bit_long_nav_enable:     1; /* NAV保护enable字段，1代表Long nav保护，0代表non long nav保护 */
    oal_uint8                               bit_group_id:            6; /* 这个字段暂时由软件填写，最终可能由算法填写，故先列出 */

}hal_tx_txop_antenna_params_stru;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
typedef struct
{
    /* TX POWER (第22行) */
    oal_uint8                               bit_lpf_gain_level0           : 1;
    oal_uint8                               bit_pa_gain_level0            : 1;
    oal_uint8                               bit_upc_gain_level0           : 4;
    oal_uint8                               bit_dac_gain_level0           : 2;

    oal_uint8                               bit_lpf_gain_level1           : 1;
    oal_uint8                               bit_pa_gain_level1            : 1;
    oal_uint8                               bit_upc_gain_level1           : 4;
    oal_uint8                               bit_dac_gain_level1           : 2;

    oal_uint8                               bit_lpf_gain_level2           : 1;
    oal_uint8                               bit_pa_gain_level2            : 1;
    oal_uint8                               bit_upc_gain_level2           : 4;
    oal_uint8                               bit_dac_gain_level2           : 2;

    oal_uint8                               bit_lpf_gain_level3           : 1;
    oal_uint8                               bit_pa_gain_level3            : 1;
    oal_uint8                               bit_upc_gain_level3           : 4;
    oal_uint8                               bit_dac_gain_level3           : 2;

}hal_tx_txop_tx_power_stru;
#elif ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))
typedef struct
{
    /* TX POWER (第14行) */
    oal_uint8                               bit_lpf_gain_level0           : 2;
    oal_uint8                               bit_upc_gain_level0           : 4;
    oal_uint8                               bit_pa_gain_level0            : 2;

    oal_uint8                               bit_lpf_gain_level1           : 2;
    oal_uint8                               bit_upc_gain_level1           : 4;
    oal_uint8                               bit_pa_gain_level1            : 2;

    oal_uint8                               bit_lpf_gain_level2           : 2;
    oal_uint8                               bit_upc_gain_level2           : 4;
    oal_uint8                               bit_pa_gain_level2            : 2;

    oal_uint8                               bit_lpf_gain_level3           : 2;
    oal_uint8                               bit_upc_gain_level3           : 4;
    oal_uint8                               bit_pa_gain_level3            : 2;
}hal_tx_txop_tx_power_stru;
#elif ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
typedef struct
{
#ifdef _PRE_WLAN_1103_PILOT
    /* TX POWER (第11行) */
    oal_uint8                               bit_dpd_enable                 : 1;

    /* TX POWER (第13行) */
    oal_uint8                               bit_upc_gain_level0            : 1;
    oal_uint8                               bit_upc_gain_level1            : 1;
    oal_uint8                               bit_upc_gain_level2            : 1;
    oal_uint8                               bit_upc_gain_level3            : 1;

    /* TX POWER (第15~17行) */
    oal_uint32                              bit_pa_gain_level0              : 2;
    oal_uint32                              bit_pa_gain_level1              : 2;
    oal_uint32                              bit_pa_gain_level2              : 2;
    oal_uint32                              bit_pa_gain_level3              : 2;
    oal_uint32                              bit_dac_gain_level0             : 2;
    oal_uint32                              bit_dac_gain_level1             : 2;
    oal_uint32                              bit_dac_gain_level2             : 2;
    oal_uint32                              bit_dac_gain_level3             : 2;
    oal_uint32                              bit_lpf_gain_level0             : 3;
    oal_uint32                              bit_lpf_gain_level1             : 3;
    oal_uint32                              bit_lpf_gain_level2             : 3;
    oal_uint32                              bit_lpf_gain_level3             : 3;
    oal_uint32                              bit_reserved8                   : 4;

    oal_uint32                              bit_delta_dbb_scaling0       : 10;
    oal_uint32                              bit_delta_dbb_scaling1       : 10;
    oal_uint32                              bit_delta_dbb_scaling2       : 10;
    oal_uint32                              bit_delta_dbb_scaling3       : 10;

    oal_uint32                              bit_dpd_tpc_lv0              : 2;
    oal_uint32                              bit_dpd_tpc_lv1              : 2;
    oal_uint32                              bit_dpd_tpc_lv2              : 2;
    oal_uint32                              bit_dpd_tpc_lv3              : 2;

    /* TX POWER (第18行) */
    oal_uint32                              bit_cfr_idx0                 : 2;
    oal_uint32                              bit_cfr_idx1                 : 2;
    oal_uint32                              bit_cfr_idx2                 : 2;
    oal_uint32                              bit_cfr_idx3                 : 2;
#else
    /* TX POWER (第15~17行) */
    oal_uint8                               bit_upc_gain_level0               : 5;
    oal_uint8                               bit_lpf_gain_level0               : 3;
    oal_uint8                               bit_upc_gain_level1               : 5;
    oal_uint8                               bit_lpf_gain_level1               : 3;
    oal_uint8                               bit_upc_gain_level2               : 5;
    oal_uint8                               bit_lpf_gain_level2               : 3;
    oal_uint8                               bit_upc_gain_level3               : 5;
    oal_uint8                               bit_lpf_gain_level3               : 3;

    oal_uint8                               bit_pa_gain_level0                : 2;
    oal_uint8                               bit_dac_gain_level0               : 2;
    oal_uint8                               bit_dbb_scale2_c0_level0          : 4;
    oal_uint8                               bit_pa_gain_level1                : 2;
    oal_uint8                               bit_dac_gain_level1               : 2;
    oal_uint8                               bit_dbb_scale2_c0_level1          : 4;
    oal_uint8                               bit_pa_gain_level2                : 2;
    oal_uint8                               bit_dac_gain_level2               : 2;
    oal_uint8                               bit_dbb_scale2_c0_level2          : 4;
    oal_uint8                               bit_pa_gain_level3                : 2;
    oal_uint8                               bit_dac_gain_level3               : 2;
    oal_uint8                               bit_dbb_scale2_c0_level3          : 4;

    oal_uint8                               bit_dbb_scale2_c1_level0          : 4;
    oal_uint8                               bit_dbb_scale2_c1_level1          : 4;
    oal_uint8                               bit_dbb_scale2_c1_level2          : 4;
    oal_uint8                               bit_dbb_scale2_c1_level3          : 4;
    //TBD:dpd
    oal_uint8                               uc_resv[2];
#endif
}hal_tx_txop_tx_power_stru;
#endif

typedef struct
{
    wlan_tx_ack_policy_enum_uint8           en_ack_policy;     /* ACK 策略 */
    oal_uint8                               uc_tid_no;        /* 通信标识符 */
    oal_uint8                               uc_qos_enable;    /* 是否开启QoS */
    oal_uint8                               uc_nonqos_seq_bypass;
}hal_wmm_txop_params_stru;

/* 第12 17行 */
typedef struct
{
    oal_uint16                              us_tsf_timestamp;
    oal_uint8                               uc_mac_hdr_len;
    oal_uint8                               uc_num_sub_msdu;
}hal_tx_mpdu_mac_hdr_params_stru;

typedef struct
{
    oal_uint32                              ul_mac_hdr_start_addr;
    oal_netbuf_stru                        *pst_skb_start_addr;
}hal_tx_mpdu_address_params_stru;

typedef struct
{
    oal_uint8                               uc_ra_lut_index;
    oal_uint8                               uc_tx_vap_index;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    oal_uint8                               uc_ori_tx_vap_index;
    oal_uint8                               auc_resv[1];
#else
    oal_uint8                               auc_resv[2];
#endif
}hal_tx_ppdu_addr_index_params_stru;


typedef struct
{
    oal_uint32                              ul_msdu_addr0;
    oal_uint16                              us_msdu0_len;
    oal_uint16                              us_msdu1_len;
    oal_uint32                              ul_msdu_addr1;
}hal_tx_msdu_address_params;


typedef struct
{
    oal_uint8                            uc_long_retry;
    oal_uint8                            uc_short_retry;
    oal_uint8                            uc_rts_succ;
    oal_uint8                            uc_cts_failure;
    oal_int8                             c_last_ack_rssi;
    oal_uint8                            uc_mpdu_num;
    oal_uint8                            uc_error_mpdu_num;
    oal_uint8                            uc_last_rate_rank;
    oal_uint8                            uc_tid;
    hal_tx_queue_type_enum_uint8         uc_ac;
    oal_uint16                           us_mpdu_len;
    oal_uint8                            uc_is_retried;
    oal_uint8                            uc_bandwidth;
    oal_uint8                            uc_sounding_mode;           /* 表示该帧sounding类型 */
    oal_uint8                            uc_status;                  /* 该帧的发送结果 */
    oal_uint8                            uc_ampdu_enable;            /* 表示该帧是否为AMPDU聚合帧 */
    oal_uint16                           us_origin_mpdu_lenth;       /* mpdu长度 */
    oal_uint8                            en_channel_code;

    oal_uint64                           ull_ampdu_result;
    hal_channel_matrix_dsc_stru          st_tx_dsc_chnl_matrix;      /*发送描述符中的信道矩阵信息*/
    hal_tx_txop_per_rate_params_union    ast_per_rate[HAL_TX_RATE_MAX_NUM];
    oal_uint32                           ul_ampdu_length;
    hal_tx_txop_tx_power_stru            st_tx_power;
    oal_uint32                           ul_tx_comp_mwo_cyc_time;/*   上报的微波炉周期寄存器的计数值*/
#ifdef _PRE_WLAN_FEATURE_PF_SCH
    oal_uint32                           ul_tx_consumed_airtime;      /* 调度算法使用一次发送的空口时间 */
#endif

    oal_uint32                           ul_now_time_ms;
    oal_uint8                            uc_normal_pkt_num;
    oal_uint8                            uc_tx_desc_rate_rank;       /* 发送成功时选择的速率等级，status=1时有效 */
    oal_uint8                            uc_first_succ_cnt;  /* 第一次发送速率级别成功的帧数 */
    oal_uint8                            uc_first_fail_cnt;  /* 第一次发送速率级别失败的帧数 */
}hal_tx_dscr_ctrl_one_param;


typedef struct
{
    /* 由安全特性更新 */
    wlan_security_txop_params_stru           st_security;         /* 第16行 MAC TX MODE 2 */

    /* groupid和partial_aid */
    wlan_groupid_partial_aid_stru            st_groupid_partial_aid;  /* 第12和15行部分 */

}hal_tx_txop_feature_stru;
/*****************************************************************************
  结构名  : hal_tx_txop_alg_stru
  结构说明: DMAC模块TXOP发送控制结构
*****************************************************************************/
typedef struct
{
    /*tx dscr中算法填写的参数 */
    hal_channel_matrix_dsc_stru          st_txbf_matrix;                     /* 第10 23行  */
    hal_tx_txop_rate_params_stru         st_rate;                            /* 第13行(HY TX MODE 1)*/
    hal_tx_txop_per_rate_params_union    ast_per_rate[HAL_TX_RATE_MAX_NUM];  /* 第14(PHY TX RATA 1) 19 20 21 行*/
    hal_tx_txop_antenna_params_stru      st_antenna_params;                  /* 第15行(PHY TX MODE 2) */
    hal_tx_txop_tx_power_stru            st_tx_power;                        /* 第22行(TX POWER)*/
}hal_tx_txop_alg_stru;

/*****************************************************************************
  结构名  : hal_rate_pow_code_gain_table_stru
  结构说明: HAL模块POW Code表结构
*****************************************************************************/
typedef struct
{
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    oal_uint32  aul_pow_code_level[HAL_POW_LEVEL_NUM];
#else
    oal_uint8   auc_pow_code_level[HAL_POW_LEVEL_NUM];
#endif
    oal_int16   as_pow_gain_level[HAL_POW_LEVEL_NUM];
}hal_rate_pow_code_gain_table_stru;

#ifndef _PRE_WLAN_FEATURE_TPC_OPT
/*****************************************************************************
  结构名  : hal_upc_gain_code_lut_stru
  结构说明: HAL模块UPC Code表结构
*****************************************************************************/
typedef struct
{
    oal_int16                            as_upc_gain_lut[HAL_POW_UPC_LUT_NUM];
    oal_uint8                            auc_upc_code_lut[HAL_POW_UPC_LUT_NUM];
}hal_upc_gain_code_lut_stru;
#endif

/*****************************************************************************
  结构名  : hal_vap_pow_info_stru
  结构说明: HAL模块VAP级别功率结构
*****************************************************************************/
typedef struct
{
    hal_rate_pow_code_gain_table_stru   *pst_rate_pow_table;  /* 速率-TPC code&TPC gain对应表 */
#ifndef _PRE_WLAN_FEATURE_TPC_OPT
    hal_upc_gain_code_lut_stru          *past_upc_gain_code_lut[HAL_POW_MAX_CHAIN_NUM];
    oal_uint8                            uc_active_upc_lut_len;
#endif  /* _PRE_WLAN_FEATURE_TPC_OPT */
    oal_uint16                           us_reg_pow;
    oal_bool_enum_uint8                  en_debug_flag;     /* rate_pow table调试开关 */
    oal_uint8                            _rom[4];
}hal_vap_pow_info_stru;

/*****************************************************************************
  结构名  : hal_user_pow_info_stru
  结构说明: HAL模块USER级别功率结构
*****************************************************************************/
typedef struct
{
    hal_rate_pow_code_gain_table_stru   *pst_rate_pow_table;        /* EVM功率表单结构体指针*/
    hal_tx_txop_alg_stru                *pst_txop_param;            /* 用户速率描述符信息结构体 */
    oal_bool_enum_uint8                  en_rf_limit_pow;           /*是否使能RF limit功率*/
    oal_uint8                            auc_resv[3];
}hal_user_pow_info_stru;

/*****************************************************************************
  结构名  : hal_tx_ppdu_feature_stru
  结构说明: DMAC模块PPDU发送控制结构
*****************************************************************************/
typedef struct
{
    /* 第15 16行 TX VAP index 和 RA LUT Index */
    hal_tx_ppdu_addr_index_params_stru  st_ppdu_addr_index;

    /* 第16 17行 */
    oal_uint32                          ul_ampdu_length;         /* 不包括null data的ampdu总长度 */
    oal_uint16                          us_min_mpdu_length;      /* 根据速率查表得到的ampdu最小mpdu的长度 */

    /* 第13行 */
    oal_uint8                           uc_ampdu_enable;         /* 是否使能AMPDU */
    oal_uint8                           uc_rifs_enable;          /* rifs模式下发送时，MPDU链最后是否挂一个BAR帧 */

    /* 第12行  MAC TX MODE 1 */
    oal_uint16                          us_tsf;
    oal_uint8                           en_retry_flag_hw_bypass;
    oal_uint8                           en_duration_hw_bypass;
    oal_uint8                           en_seq_ctl_hw_bypass;
    oal_uint8                           en_timestamp_hw_bypass;
    oal_uint8                           en_addba_ssn_hw_bypass;
    oal_uint8                           en_tx_pn_hw_bypass;
    oal_uint8                           en_long_nav_enable;
    oal_uint8                           uc_mpdu_num;             /* ampdu中mpdu的个数 */
    oal_uint8                           uc_tx_ampdu_session_index;
    oal_uint8                           auc_resv[1];
}hal_tx_ppdu_feature_stru;

/*****************************************************************************
  结构名  : hal_tx_mpdu_stru
  结构说明: DMAC模块MPDU发送控制结构
*****************************************************************************/
typedef struct
{
    /* 从11MAC帧头中获取 针对MPDU*/
    hal_wmm_txop_params_stru              st_wmm;
    hal_tx_mpdu_mac_hdr_params_stru       st_mpdu_mac_hdr;                          /* 第12 17行(PHY TX MODE 2) */
    hal_tx_mpdu_address_params_stru       st_mpdu_addr;                             /* 第18行(MAC TX MODE 2)*/
    hal_tx_msdu_address_params            ast_msdu_addr[WLAN_DSCR_SUBTABEL_MAX_NUM];/* 第24,25...行*/
    oal_uint16                            us_mpdu_len;                              /* 发送帧的长度,包括head */
    oal_uint16                            us_resv;
}hal_tx_mpdu_stru;

/* Beacon帧发送参数 */
typedef struct
{
    oal_uint32              ul_pkt_ptr;
    oal_uint32              us_pkt_len;
    hal_tx_txop_alg_stru   *pst_tx_param;
    oal_uint32              ul_tx_chain_mask;

    //dmac看不到描述符，这两个寄存器赋值放到hal
    //oal_uint32  ul_phy_tx_mode;     /* 同tx描述符 phy tx mode 1 */
    //oal_uint32  ul_tx_data_rate;    /* 同tx描述符 data rate 0 */

}hal_beacon_tx_params_stru;

/*****************************************************************************
  结构名  : hal_security_key_stru
  结构说明: DMAC模块安全密钥配置结构体
*****************************************************************************/
typedef struct
{
    oal_uint8                           uc_key_id;
    wlan_cipher_key_type_enum_uint8     en_key_type;
    oal_uint8                           uc_lut_idx;
    wlan_ciper_protocol_type_enum_uint8 en_cipher_type;
    oal_bool_enum_uint8                 en_update_key;
    wlan_key_origin_enum_uint8          en_key_origin;
    oal_uint8                           auc_reserve[2];
    oal_uint8                           *puc_cipher_key;
    oal_uint8                           *puc_mic_key;
}hal_security_key_stru;

/*****************************************************************************
  7.4 基准VAP和Device结构
*****************************************************************************/
typedef struct
{
    oal_uint8                            uc_is_trained;
    oal_uint16                           us_training_cnt;
    oal_uint32                           ul_training_data;
}hal_tbtt_offset_training_hdl_stru;

typedef struct
{
    oal_uint16                           us_inner_tbtt_offset_siso_base;
    oal_uint16                           us_inner_tbtt_offset_mimo_base;
    oal_uint32                           ul_probe_beacon_rx_cnt;
    oal_uint32                           ul_probe_tbtt_cnt;
    oal_uint8                            uc_beacon_rx_ratio;
    oal_uint8                            uc_best_beacon_rx_ratio ;
    oal_uint8                            uc_probe_state ;
    oal_uint8                            uc_probe_suspend ;
    oal_int8                             i_cur_probe_index ;
    oal_int8                             i_best_probe_index ;
}hal_tbtt_offset_probe_stru;

#define TBTT_OFFSET_PROBE_STEP_US  (30)
#define TBTT_OFFSET_PROBE_MAX      (10)    /*最多增加30*10 = 300us*/

#define TBTT_OFFSET_UP_PROBE_STEP     (2)    /*up probe*/
#define TBTT_OFFSET_DOWN_PROBE_STEP   (1)
#define TBTT_OFFSET_PROBE_ACCETP_DIF  (3)
#define TBTT_OFFSET_UP_PROBE_ACCETP_DIF    (TBTT_OFFSET_PROBE_ACCETP_DIF*TBTT_OFFSET_UP_PROBE_STEP)
#define TBTT_OFFSET_DOWN_PROBE_ACCETP_DIF  (TBTT_OFFSET_PROBE_ACCETP_DIF*TBTT_OFFSET_DOWN_PROBE_STEP)


#define TBTT_OFFSET_PROBE_CALCURATE_PERIOD   (100)    /*beacon接收率计算周期*/
#define TBTT_OFFSET_PROBE_RATIO              (100/TBTT_OFFSET_PROBE_CALCURATE_PERIOD)    /*beacon接收率百分比*/


/*state define*/
#define TBTT_OFFSET_PROBE_STATE_INIT    (0)
#define TBTT_OFFSET_PROBE_STATE_START   (1)
#define TBTT_OFFSET_PROBE_STATE_UP_DONE (2)
#define TBTT_OFFSET_PROBE_STATE_END     (3)



#ifdef _PRE_PRODUCT_ID_HI110X_DEV
typedef struct
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    oal_uint16                           us_inner_tbtt_offset_siso;
    oal_uint16                           us_inner_tbtt_offset_mimo;
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    oal_uint16                           us_inner_tbtt_offset;
#endif
    oal_uint8                            uc_bcn_rf_chain;                            /* 唤醒后收beacon的通道，在hal device状态机awake子状态时生效 */
#ifdef _PRE_PM_DYN_SET_TBTT_OFFSET
    hal_tbtt_offset_training_hdl_stru    ast_training_handle[2];
#endif

#ifdef _PRE_PM_TBTT_OFFSET_PROBE
    hal_tbtt_offset_probe_stru         *pst_tbtt_offset_probe;
#endif
    oal_uint8                            _rom[4];
}hal_pm_info_stru;
#endif

typedef struct tag_hal_to_dmac_vap_stru
{
    oal_uint8                            uc_chip_id;                                 /* 芯片ID */
    oal_uint8                            uc_device_id;                               /* 设备hal device ID */
    oal_uint8                            uc_vap_id;                                  /* VAP ID */
    wlan_vap_mode_enum_uint8             en_vap_mode;                                /* VAP工作模式 */
    wlan_p2p_mode_enum_uint8             en_p2p_mode;                                /* P2P */
    oal_uint8                            uc_mac_vap_id;                              /* 保存mac vap id */
    oal_uint8                            uc_dtim_cnt;                                /* dtim count */
    oal_uint8                            uc_sleep_time_too_short_cnt;                /* 计算所得睡眠时间连续不满足最小睡眠时间计数 */
    hal_vap_state_enum_uint8             en_hal_vap_state;                           /* hal vap state状态 */
#ifdef _PRE_PRODUCT_ID_HI110X_DEV
    hal_pm_info_stru                     st_pm_info;
#endif
    /* ROM化后资源扩展指针 */
    oal_void                            *_rom;
}hal_to_dmac_vap_stru;


/*****************************************************************************
  7.5 对外部接收提供接口所用数据结构
*****************************************************************************/
typedef struct
{
    /*byte 0*/
    oal_int8            c_rssi_dbm;

    /*byte 1*/
    union
    {
        struct
        {
            oal_uint8   bit_vht_mcs       : 4;
            oal_uint8   bit_nss_mode      : 2;
            oal_uint8   bit_protocol_mode : 2;
        } st_vht_nss_mcs;                                   /* 11ac的速率集定义 */
        struct
        {
            oal_uint8   bit_ht_mcs        : 6;
            oal_uint8   bit_protocol_mode : 2;
        } st_ht_rate;                                       /* 11n的速率集定义 */
        struct
        {
            oal_uint8   bit_legacy_rate   : 4;
            oal_uint8   bit_reserved1     : 2;
            oal_uint8   bit_protocol_mode : 2;
        } st_legacy_rate;                                   /* 11a/b/g的速率集定义 */
    } un_nss_rate;

    /*byte 2-3 */
    oal_int8           c_snr_ant0;                          /* ant0 SNR */
    oal_int8           c_snr_ant1;                          /* ant1 SNR */

    /*byte 4-5 */
#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
    oal_int16           s_rssi_ant0;
    oal_int16           s_rssi_ant1;
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    oal_int8           c_ant0_rssi;        /* ANT0上报当前帧RSSI */
    oal_int8           c_ant1_rssi;        /* ANT1上报当前帧RSSI */
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    oal_uint16         us_channel_matrix_len;
#endif

#if defined _PRE_WLAN_PRODUCT_1151V200
    oal_uint16          us_mpdu_num;                         /*AMPDU中子帧个数*/
    oal_uint8           auc_resv[2];                        /* 四字节对齐   */
#endif
}hal_rx_statistic_stru;

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
typedef enum
{
    HAL_TAS_MEASURE_INIT,
    HAL_TAS_MEASURING,
    HAL_TAS_ANT0_INVALID,
    HAL_TAS_ANT0_AVAILABLE,   //瞬态，上报内核后切回init状态
    HAL_TAS_NOTIFY_COMPLETED, //瞬态，上报内核后切回init状态
    HAL_TAS_STATUS_BUTT,
}hal_tas_rssi_measure_flag_enum;
typedef oal_uint8 hal_tas_rssi_measure_flag_enum_uint8;

typedef struct
{
    oal_int16             s_tas_rssi_smth_access;      /* tas天线测量 core0 RSSI */
    oal_uint8             uc_measure_vap_id;
    oal_uint8             uc_frame_cnt                              :5;
    hal_tas_rssi_measure_flag_enum_uint8   en_tas_rssi_measure_flag :3;  /* tas天线测量状态标志位 */
}hal_tas_rssi_measure_stru;
#endif

/* dmac_pkt_captures使用,tx rx均会使用 */
typedef struct
{
    oal_uint8        en_ant_rssi_sw;     /* 通过ANT RSSI切换使能。bit0:管理帧切换使能 bit1:数据帧切换使能 */
    oal_bool_enum_uint8 en_log_print;
    oal_uint8        uc_rssi_high;       /* RSSI高门限 */
    oal_uint8        uc_rssi_low;        /* RSSI低门限 */

    oal_int16        s_ant0_rssi_smth;   /* 平滑处理后历史RSSI */
    oal_int16        s_ant1_rssi_smth;

    oal_uint16       uc_rssi_high_cnt;
    oal_uint16       uc_rssi_high_cnt_th;

    oal_uint16       uc_rssi_low_cnt;
    oal_uint16       uc_rssi_low_cnt_th;

    oal_int8         c_min_rssi_th;      /* 最小rssi的切换门限 */
    oal_uint8        auc_resv[3];
}hal_rx_ant_rssi_stru;

typedef struct
{
    oal_bool_enum_uint8 en_ant_rssi_sw;     /* 通过ANT RSSI切换使能 */
    oal_bool_enum_uint8 en_log_print;
    oal_uint8           uc_rssi_th;         /* RSSI高门限 */
    oal_int8            c_ctrl_rssi_th;     /* 控制帧单双通道发送门限 */


    oal_int16        s_ant0_rssi_smth;   /* 平滑处理后历史RSSI */
    oal_int16        s_ant1_rssi_smth;
}hal_rx_ant_rssi_mgmt_stru;

typedef struct
{
    oal_int8            c_ant0_snr;        /* ANT0上报当前帧SNR */
    oal_int8            c_ant1_snr;        /* ANT1上报当前帧SNR */
    oal_uint8           uc_snr_high;       /* SNR高门限 */
    oal_uint8           uc_snr_low;        /* SNR低门限 */

    oal_uint16          uc_snr_high_cnt;
    oal_uint16          uc_snr_high_cnt_th;

    oal_uint16          uc_snr_low_cnt;
    oal_uint16          uc_snr_low_cnt_th;

    oal_bool_enum_uint8 en_ant_snr_sw;     /* 通过ANT SNR切换使能 */
    oal_bool_enum_uint8 en_log_print;
    oal_bool_enum_uint8 en_reserv[2];
}hal_rx_ant_snr_stru;

/* ant_detect结构体 */
typedef struct
{
    hal_rx_ant_rssi_stru   st_rx_rssi;
    hal_rx_ant_snr_stru    st_rx_snr;
    oal_void              *pst_cb;

    oal_bool_enum_uint8   en_rssi_trigger; /* 状态机是否由RSSI/SNR触发从MIMO切换至SISO */
    oal_bool_enum_uint8   en_miso_hold;     /* 是否保持在MISO状态 */
    oal_uint8             uc_tbtt_cnt;     /* 当前统计的tbtt中断数 */
    oal_uint8             uc_tbtt_cnt_th;  /* tbtt中断门限值 */

    oal_bool_enum_uint8   en_mimo_hold;     /* 是否保持在MIMO状态 */
    oal_uint8             uc_mimo_tbtt_cnt;     /* 当前统计的tbtt中断数 */
    oal_uint8             uc_mimo_tbtt_open_th;  /* tbtt中断开启探测门限值 */
    oal_uint8             uc_mimo_tbtt_close_th; /* tbtt中断关闭探测门限值 */
    oal_int8              c_ori_min_th;
    oal_int8              c_cur_min_th;
    oal_uint8             uc_diff_th;
    oal_uint8             _rom[1];
}hal_rssi_stru;


/* dmac_pkt_captures使用,tx rx均会使用 */
typedef struct
{
    union
    {
        struct
        {
            oal_uint8   bit_vht_mcs       : 4;
            oal_uint8   bit_nss_mode      : 2;
            oal_uint8   bit_protocol_mode : 2;
        } st_vht_nss_mcs;                                   /* 11ac的速率集定义 */
        struct
        {
            oal_uint8   bit_ht_mcs        : 6;
            oal_uint8   bit_protocol_mode : 2;
        } st_ht_rate;                                       /* 11n的速率集定义 */
        struct
        {
            oal_uint8   bit_legacy_rate   : 4;
            oal_uint8   bit_reserved1     : 2;
            oal_uint8   bit_protocol_mode : 2;
        } st_legacy_rate;                                   /* 11a/b/g的速率集定义 */
    } un_nss_rate;

    oal_uint8           uc_short_gi;
    oal_uint8           uc_bandwidth;

    oal_uint8           bit_preamble       : 1,
                        bit_channel_code  : 1,
                        bit_stbc          : 2,
                        bit_reserved2     : 4;
}hal_statistic_stru;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* HOST专属 */
typedef struct
{
    oal_uint32                 *pul_mac_hdr_start_addr;     /* 对应的帧的帧头地址,虚拟地址 */
    oal_uint16                  us_da_user_idx;             /* 目的地址用户索引 */
    oal_uint16                  us_rsv;                     /* 对齐 */
}mac_rx_expand_cb_stru;

/* 裸系统下需要传输给HMAC模块的信息 */
/* hal_rx_ctl_stru结构的修改要考虑hi110x_rx_get_info_dscr函数中的优化 */
/* 1字节对齐 */
#pragma pack(push,1)
struct mac_rx_ctl
{
    /* byte 0*/
    oal_uint8                   bit_vap_id            :5;
    oal_uint8                   bit_amsdu_enable      :1;   /* 是否为amsdu帧,每个skb标记 */
    oal_uint8                   bit_is_first_buffer   :1;   /* 当前skb是否为amsdu的首个skb */
    oal_uint8                   bit_is_fragmented     :1;

    /* byte 1*/
    oal_uint8                   uc_msdu_in_buffer;          /* 每个skb包含的msdu数,amsdu用,每帧标记 */

    /* byte 2*/
    oal_uint8                   bit_ta_user_idx       :5;
    oal_uint8                   bit_tid               :2;
    oal_uint8                   bit_is_key_frame      :1;

    /* byte 3*/
    oal_uint8                   uc_mac_header_len     :6;   /* mac header帧头长度 */
    oal_uint8                   bit_is_beacon         :1;
    oal_uint8                   bit_is_last_buffer    :1;
    /* byte 4-5 */
    oal_uint16                  us_frame_len;                /* 帧头与帧体的总长度,AMSDU非首帧不填 */

    /* byte 6 */
    oal_uint8                   uc_mac_vap_id         :4;   /* 业务侧vap id号 */
    oal_uint8                   bit_buff_nums         :4;   /* 每个MPDU占用的SKB数,AMSDU帧占多个 */
    /* byte 7 */
    oal_uint8                   uc_channel_number;          /* 接收帧的信道 */

#ifndef _PRE_PRODUCT_ID_HI110X_DEV
    /* OFFLOAD架构下，HOST相对DEVICE的CB增量 */
    mac_rx_expand_cb_stru       st_expand_cb;
#endif
}__OAL_DECLARE_PACKED;
typedef struct mac_rx_ctl  hal_rx_ctl_stru;
#pragma pack(pop)

#else
/* 存放于对应的netbuf的CB字段中，最大值为48字节 */
/* 后续需要传输给HMAC模块的信息，与mac_rx_ctl_stru结构体保持一致*/

typedef struct
{
    /*word 0*/
#ifdef _PRE_WLAN_PRODUCT_1151V200
    oal_uint8                   bit_vap_id            :6;   /* 对应hal vap id */
    oal_uint8                   bit_mgmt_to_hostapd   :1;
    oal_uint8                   bit_reserved1         :1;
#else
    oal_uint8                   bit_vap_id            :5;   /* 对应hal vap id */
    oal_uint8                   bit_mgmt_to_hostapd   :1;
    oal_uint8                   bit_reserved1         :2;
#endif
    oal_uint8                   uc_msdu_in_buffer;

    oal_uint8                   bit_amsdu_enable      :1;
    oal_uint8                   bit_is_first_buffer   :1;
    oal_uint8                   bit_is_fragmented     :1;
    oal_uint8                   bit_is_beacon         :1;
    oal_uint8                   bit_buff_nums         :4;   /* 每个MPDU占用的buf数目 */


    oal_uint8                   uc_mac_header_len;          /* mac header帧头长度 */
    /*word 1*/
    oal_uint16                  us_frame_len;               /* 帧头与帧体的总长度 */
    oal_uint16                  us_da_user_idx;             /* 目的地址用户索引 */
    /*word 2*/
    oal_uint32                 *pul_mac_hdr_start_addr;     /* 对应的帧的帧头地址,虚拟地址 */
    /*word 3*/
    oal_uint16                  us_ta_user_idx;             /* 发送端地址用户索引 */
    oal_uint8                   uc_mac_vap_id;
    oal_uint8                   uc_channel_number;          /* 接收帧的信道 */
}hal_rx_ctl_stru;
#endif

/* 对DMAC SCAN 模块提供的硬件MAC/PHY信道测量结果结构体 */
typedef struct
{
    /* MAC信道统计 */
    oal_uint32  ul_ch_stats_time_us;
    oal_uint32  ul_pri20_free_time_us;
    oal_uint32  ul_pri40_free_time_us;
    oal_uint32  ul_pri80_free_time_us;
    oal_uint32  ul_ch_rx_time_us;
    oal_uint32  ul_ch_tx_time_us;

    /* PHY信道测量 */
    oal_uint8  uc_phy_ch_estimate_time_ms;
    oal_int8   c_pri20_idle_power;
    oal_int8   c_pri40_idle_power;
    oal_int8   c_pri80_idle_power;

    oal_int8   c_pri160_idle_power;
    oal_int8   c_resv[3];
    oal_uint32 ul_stats_cnt;
    oal_uint32 ul_meas_cnt;

}hal_ch_statics_irq_event_stru;

#define RADAR_INFO_FLAG_DUMMY   0x10

/* 脉冲信息结构体 */
typedef struct
{
    oal_uint32  ul_timestamp;
    oal_uint16  us_duration;
    oal_uint16  us_power;
    oal_uint16  us_max_fft;
    oal_uint8   uc_type;
    oal_uint8   auc_resv[1];
}hal_pulse_info_stru;

#define MAX_RADAR_PULSE_NUM   32            /* 最大雷达脉冲数 */
#define RADAR_TYPE_CHIRP      10            /* 芯片上报的chirp雷达类型标号 */
#define RADAR_MIN_ETSI_CHIRP_PULSE_NUM  5   /*ETSI chirp最小脉冲个数*/

/* 保存多个脉冲信息结构体 */
typedef struct
{
    hal_pulse_info_stru     ast_pulse_info[MAX_RADAR_PULSE_NUM];
    oal_uint32              ul_pulse_cnt;
}hal_radar_pulse_info_stru;

/* 对DMAC SCAN模块提供的硬件雷达检测信息结构体 */
typedef struct
{
#if defined(_PRE_WLAN_FEATURE_DFS_OPTIMIZE) || defined(_PRE_WLAN_FEATURE_DFS)
    hal_radar_pulse_info_stru st_radar_pulse_info;
#endif
    oal_uint8   uc_radar_type;
    oal_uint8   uc_radar_freq_offset;
    oal_uint8   uc_radar_bw;
    oal_uint8   uc_band;
    oal_uint8   uc_channel_num;
    oal_uint8   uc_working_bw;
    oal_uint8   uc_flag;
    oal_uint8   uc_resv;
}hal_radar_det_event_stru;

typedef struct
{
    oal_uint32      ul_reg_band_info;
    oal_uint32      ul_reg_bw_info;
    oal_uint32      ul_reg_ch_info;
}hal_radar_irq_reg_list_stru;

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
/*
 *裸系统下针对接收，提供读取接口
 * frame_len长度
 * 802.11帧头长度(uc_mac_hdr_len)
*/
#pragma pack(push,1)

typedef struct
{
    /*byte 0*/
    oal_uint8   bit_cipher_protocol_type  : 4;      /* 接收帧加密类型 */
    oal_uint8   bit_dscr_status           : 4;      /* 接收状态 */

    /*byte 1*/
    oal_uint8   bit_channel_code          : 1;
    oal_uint8   bit_STBC                  : 2;
#ifdef _PRE_WLAN_1103_PILOT
    oal_uint8   bit_GI                    : 2;
#else
    oal_uint8   bit_GI                    : 1;
    oal_uint8   bit_rsvd                  : 1;
#endif
    oal_uint8   bit_AMPDU                 : 1;
    oal_uint8   bit_sounding_mode         : 2;

    /*byte 2*/
    oal_uint8   bit_ext_spatial_streams   : 2;
    oal_uint8   bit_smoothing             : 1;
    oal_uint8   bit_freq_bandwidth_mode   : 4;
    oal_uint8   bit_preabmle              : 1;

    /*byte 3*/
    oal_uint8   bit_rsp_flag              : 1;
    oal_uint8   bit_reserved2             : 1;
    oal_uint8   bit_he_flag               : 1;
    oal_uint8   bit_last_mpdu_flag        : 1;
#if  defined(_PRE_WLAN_FEATURE_TXBF_HW) || defined(_PRE_WLAN_1103_PILOT)
    oal_uint8   bit_he_ltf_type           : 2;
    oal_uint8   bit_dcm                   : 1;
    oal_uint8   bit_reserved3             : 1;
    /* byte 4 */
    oal_uint8   bit_is_rx_vip             : 1;
    oal_uint8   bit_resv4                 : 7;
#else
    oal_uint8   bit_column_number         : 4;

    oal_uint8   bit_code_book             : 2;
    oal_uint8   bit_grouping              : 2;
    oal_uint8   bit_row_number            : 4;
#endif
}hal_rx_status_stru;
#pragma pack(pop)

#else
/*
 *针对接收，提供读取接口
 * frame_len长度
 * 802.11帧头长度(uc_mac_hdr_len)
*/

typedef struct
{
    /*word 0*/
    oal_uint8   bit_cipher_protocol_type  : 4;  /* 帧的加密类型*/
    oal_uint8   bit_dscr_status           : 4;     /* 描述符接收状态 */
    oal_uint8   bit_ext_spatial_streams   : 2;
    oal_uint8   bit_smoothing             : 1;
    oal_uint8   bit_freq_bandwidth_mode   : 4;
    oal_uint8   bit_preabmle              : 1;
    oal_uint8   bit_channel_code          : 1;
    oal_uint8   bit_STBC                  : 2;
    oal_uint8   bit_GI                    : 1;
    oal_uint8   bit_reserved1             : 1;
    oal_uint8   bit_AMPDU                 : 1;
    oal_uint8   bit_sounding_mode         : 2;
    oal_uint8   uc_reserved1;
    /*word 1*/
    oal_uint8   bit_code_book             : 2;              /* 信道矩阵相关信息 */
    oal_uint8   bit_grouping              : 2;
    oal_uint8   bit_row_number            : 4;
    oal_uint8   bit_column_number         : 4;
#if defined _PRE_WLAN_PRODUCT_1151V200
    oal_uint8   uc_reserved2              : 2;
    oal_uint8   bit_rsp_flag              : 1;
#else
    oal_uint8   bit_rsp_flag              : 3;
#endif
    oal_uint8   bit_last_mpdu_flag        : 1;
    oal_uint16  us_channel_matrix_len;                   /* 信道矩阵长度 */

    /*word 2*/
    oal_uint32  ul_tsf_timestamp;                        /* TSF时间戳 */
    /*word 3*/

}hal_rx_status_stru;
#endif


/*****************************************************************************
  7.6 对外部保留的VAP级接口列表，建议外部不做直接调用，而是调用对应的内联函数
*****************************************************************************/
typedef struct
{
    hal_to_dmac_vap_stru    st_vap_base;
    oal_uint32              ul_vap_base_addr;
}hal_vap_stru;

/*****************************************************************************
  结构名  : hal_rx_dscr_queue_header_stru
  结构说明: 接收描述符队列头的结构体
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
typedef struct
{
    oal_dlist_head_stru                     st_header;          /* 发送描述符队列头结点 */
    oal_uint16                              us_element_cnt;     /* 接收描述符队列中元素的个数 */
    hal_dscr_queue_status_enum_uint8        uc_queue_status;    /* 接收描述符队列的状态 */
    oal_uint8                               uc_available_res_cnt; /* 当前队列中硬件可用描述符个数 */
}hal_rx_dscr_queue_header_stru;
#else
typedef struct
{
    oal_uint32                             *pul_element_head;   /* 指向接收描述符链表的第一个元素 */
    oal_uint32                             *pul_element_tail;   /* 指向接收描述符链表的最后一个元素 */
    oal_uint16                              us_element_cnt;     /* 接收描述符队列中元素的个数 */
    hal_dscr_queue_status_enum_uint8        uc_queue_status;    /* 接收描述符队列的状态 */
    oal_uint8                               auc_resv[1];
}hal_rx_dscr_queue_header_stru;
#endif
/*****************************************************************************
  结构名  : dmac_tx_dscr_queue_header_stru
  结构说明: 发送描述符队列头的结构体
*****************************************************************************/
typedef struct
{
    oal_dlist_head_stru                     st_header;          /* 发送描述符队列头结点 */
    hal_dscr_queue_status_enum_uint8        en_queue_status;    /* 发送描述符队列状态 */
    oal_uint8                               uc_ppdu_cnt;     /* 发送描述符队列中元素的个数 */
    oal_uint8                               auc_resv[2];
}hal_tx_dscr_queue_header_stru;

/*****************************************************************************
  结构名  : dmac_tx_dscr_queue_header_stru
  结构说明: 发送描述符队列头的结构体
*****************************************************************************/
typedef struct
{
    oal_uint8         uc_nulldata_awake;              /* AP时收到节能位为0的null data是否唤醒*/
    oal_uint8         uc_rsv[3];
    oal_uint32        ul_nulldata_phy_mode;           /* STA时发送null data的phy mode */
    oal_uint32        ul_nulldata_rate;               /* STA时发送null data的速率*/
    oal_uint32        ul_nulldata_interval;           /* STA时发送null data的间隔*/
    oal_uint32        ul_nulldata_address;            /* STA时发送null data的速率*/
    oal_uint32        ul_ap0_probe_resp_address;      /* AP0的probe response内存地址*/
    oal_uint32        ul_ap0_probe_resp_len;          /* AP0的probe response长度*/
    oal_uint32        ul_ap1_probe_resp_address;      /* AP1的probe response内存地址*/
    oal_uint32        ul_ap1_probe_resp_len;          /* AP1的probe response长度*/
    oal_uint32        ul_ap0_probe_resp_phy;          /* AP0的probe response发送phy模式*/
    oal_uint32        ul_ap0_probe_resp_rate;         /* AP0的probe response发送reate*/
    oal_uint32        ul_ap1_probe_resp_phy;          /* AP1的probe response发送phy模式*/
    oal_uint32        ul_ap1_probe_resp_rate;         /* AP1的probe response发送reate*/

    oal_uint32        ul_set_bitmap;                  /* wow开关 */
}hal_wow_param_stru;

typedef struct
{
    oal_uint8  uc_lut_index;
    oal_uint8  uc_peer_lut_index;
    oal_uint8  uc_tid;
    oal_uint8  uc_mmss;

    oal_uint16 us_win_size;
    oal_uint8  uc_rsv[2];

    oal_uint16 us_ssn;
    oal_uint16 us_seq_num;

    oal_uint32 ul_bitmap_lsb;
    oal_uint32 ul_bitmap_msb;
}hal_ba_para_stru;


typedef enum
{
    HAL_WOW_PARA_EN                    = BIT0,
    HAL_WOW_PARA_NULLDATA              = BIT1,
    HAL_WOW_PARA_NULLDATA_INTERVAL     = BIT2,
    HAL_WOW_PARA_NULLDATA_AWAKE        = BIT3,
    HAL_WOW_PARA_AP0_PROBE_RESP        = BIT4,
    HAL_WOW_PARA_AP1_PROBE_RESP        = BIT5,
    HAL_WOW_PARA_AP0_IS_FAKE_VAP       = BIT6,
    HAL_WOW_PARA_AP1_IS_FAKE_VAP       = BIT7,

    HAL_WOW_PARA_BUTT
} hal_wow_para_set_enum;
typedef oal_uint32 hal_tx_status_enum_uint32;

/*****************************************************************************
  结构名  : hal_lpm_chip_state
  结构说明: 芯片低功耗状态枚举
*****************************************************************************/
typedef enum
{
    HAL_LPM_STATE_POWER_DOWN,
    HAL_LPM_STATE_IDLE,
    HAL_LPM_STATE_LIGHT_SLEEP,
    HAL_LPM_STATE_DEEP_SLEEP,
    HAL_LPM_STATE_NORMAL_WORK,
    HAL_LPM_STATE_WOW,

    HAL_LPM_STATE_BUTT
}hal_lpm_state_enum;
typedef oal_uint8 hal_lpm_state_enum_uint8;

/*****************************************************************************
  结构名  : hal_lpm_state_para
  结构说明: 芯片低功耗状态设置参数
*****************************************************************************/
typedef struct
{
    oal_uint8         uc_dtim_count;              /* 当前的DTIM count值，STA节能时设置相位*/
    oal_uint8         uc_dtim_period;
    oal_uint8         bit_gpio_sleep_en:1,        /* soc睡眠唤醒的方式,GPIO管脚方式使能*/
                      bit_soft_sleep_en:1,        /* soc睡眠睡眠的方式,软睡眠方式使能*/
                      bit_set_bcn_interval:1,     /* 是否调整beacon inter*/
                      bit_rsv          :6;
    oal_uint8         uc_rx_chain;               /* 接收通道值*/
    oal_uint32        ul_idle_bcn_interval;      /* idle状态下beaon inter*/
    oal_uint32        ul_sleep_time;             /* 软定时睡眠时间，单位ms*/
}hal_lpm_state_param_stru;


/*****************************************************************************
  结构名  : hal_cfg_rts_tx_param_stru
  结构说明: RTS设置发送参数
*****************************************************************************/
typedef struct
{
    wlan_legacy_rate_value_enum_uint8   auc_rate[HAL_TX_RATE_MAX_NUM];           /*发送速率，单位mpbs*/
    wlan_phy_protocol_enum_uint8        auc_protocol_mode[HAL_TX_RATE_MAX_NUM] ; /*协议模式, 取值参见wlan_phy_protocol_enum_uint8*/
    wlan_channel_band_enum_uint8        en_band;
    oal_uint8                           auc_recv[3];
}hal_cfg_rts_tx_param_stru;
/* txbf sounding帧速率，支持6M/24M/VHT MCS0/MCS3可配 */
typedef enum
{
    HAL_TXBF_REPORT_LEGACY_6M,
    HAL_TXBF_REPORT_LEGACY_24M,
    HAL_TXBF_REPORT_VHT_MCS0,
    HAL_TXBF_REPORT_VHT_MCS3,

    HAL_TXBF_REPORT_BUTT
}hal_txbf_sounding_rate_enum;
typedef oal_uint8 hal_txbf_sounding_rate_enum_uint8;


/*****************************************************************************
  7.7 对外部保留的设备级接口列表，建议外部不做直接调用，而是调用对应的内联函数
*****************************************************************************/
typedef oal_void (* p_hal_alg_isr_func)(oal_uint8 uc_vap_id, oal_void *p_hal_to_dmac_device);
typedef oal_void (* p_hal_gap_isr_func)(oal_uint8 uc_vap_id, oal_void *p_hal_to_dmac_device);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
typedef oal_uint32 (*p_tbtt_ap_isr_func)(oal_uint8 uc_mac_vap_id);
#endif /* #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)  */


typedef struct
{

    oal_uint32      ul_phy_addr;
    oal_uint8       uc_status;
    oal_uint8       uc_q;
    oal_uint8       auc_resv[2];
    oal_uint32      ul_timestamp;

    oal_uint32      ul_arrive_time;     /* 下半部到来时间 */
    oal_uint32      ul_handle_time;     /* 下半部处理时间 */
    oal_uint32      ul_irq_cnt;
    oal_cpu_usage_stat_stru st_cpu_state;
}hal_rx_dpart_track_stru;

/* 保存硬件上报接收中断信息结构 */
typedef struct
{
    oal_dlist_head_stru         st_dlist_head;
    oal_uint32                 *pul_base_dscr;      /* 本次中断描述符地址 */
    oal_uint16                  us_dscr_num;        /* 接收到的描述符的个数 */
    oal_uint8                   uc_channel_num;     /* 本次中断时，所在的信道号 */
    oal_uint8                   uc_queue_id;
}hal_hw_rx_dscr_info_stru;

/* 2g/5g rf定制化对应得到phy+rf chain能力定制化 */
typedef struct
{
    wlan_nss_enum_uint8                     en_2g_nss_num;              /* 2G Nss 空间流个数 */
    wlan_nss_enum_uint8                     en_5g_nss_num;              /* 5G Nss 空间流个数 */
    oal_uint8                               uc_2g_support_rf_chain;     /* rf通道1/2/3(双通道),解决方案和芯片都不支持交叉 */
    oal_uint8                               uc_5g_support_rf_chain;     /* rf通道1/2/3(双通道),解决方案和芯片都不支持交叉 */
}hal_cfg_rf_custom_cap_info_stru;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct
{
    oal_timer_list_stru         st_btcoex_ps_slot_timer;
    oal_void                   *p_drv_arg;              /* 中断处理函数参数,对应的pst_dmac_vap */
}hal_chip_btcoex_mgr_stru;
#endif

typedef struct
{
    hal_cfg_rf_custom_cap_info_stru   st_2g5g_rf_custom_mgr;
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_chip_btcoex_mgr_stru          st_btcoex_mgr;
#endif
}hal_chip_rom_stru;

extern hal_chip_rom_stru g_st_hal_chip_rom[];

typedef struct tag_hal_to_dmac_chip_stru
{
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_btcoex_btble_status_stru   st_btcoex_btble_status;
    hal_btcoex_statistics_stru     st_btcoex_statistics;
#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX
    oal_uint32                     ul_lte_coex_status;
#endif

#ifdef _PRE_WLAN_FEATURE_SMARTANT
    hal_dual_antenna_check_status_stru st_dual_antenna_check_status;
    oal_uint32                         ul_dual_antenna_status;
#endif
    oal_uint8                      uc_chip_id;

    oal_bool_enum_uint8            en_dbdc_running_flag;
}hal_to_dmac_chip_stru;

typedef enum
{
    HAL_DFR_TIMER_STEP_1 = 0,
    HAL_DFR_TIMER_STEP_2 = 1,

}hal_dfr_timer_step_enum;
typedef oal_uint8 hal_dfr_timer_step_enum_uint8;

typedef struct
{
    frw_timeout_stru                 st_tx_prot_timer;        /* 检测无发送完成中断定时器 */
    hal_dfr_timer_step_enum_uint8    en_tx_prot_timer_step;   /* 超时标志，表明第几次超时 */
    oal_uint16                       us_tx_prot_time;         /* 超时时间 */
    oal_uint8                        auc_resv[1];
}hal_dfr_tx_prot_stru;

typedef struct
{
    oal_uint16    us_tbtt_cnt;   /* TBTT中断计数，每10次TBTT中断，将us_err_cnt清零 */
    oal_uint16    us_err_cnt;    /* 每10次TBTT中断，产生的MAC错误中断个数 */
}hal_dfr_err_opern_stru;

typedef struct
{
    oal_uint32                      ul_error1_val; /* 错误1中断状态 */
    oal_uint32                      ul_error2_val; /* 错误2中断状态 */
}hal_error_state_stru;

typedef struct
{
    oal_dlist_head_stru   st_entry;
    oal_uint32            ul_phy_addr;    /* 接收描述符物理地址 */
}witp_rx_dscr_recd_stru;


/*****************************************************************************
  结构名  : hal_phy_pow_param_stru
  结构说明: PHY 功率相关寄存器参数, 在2.4G/5G频点切换时使用
*****************************************************************************/
typedef struct
{
#ifndef _PRE_WLAN_FEATURE_TPC_OPT
    oal_uint32                      ul_pa_bias_addr;                        /* PA_BIAS地址 */
    oal_uint32                      aul_pa_bias_gain_code[WLAN_BAND_BUTT];  /* 2G/5G PA_BIAS CODE */
    oal_uint32                      ul_pa_addr;                             /* PA地址 */
    oal_uint32                      aul_2g_upc_addr[WLAN_2G_SUB_BAND_NUM];  /* 2G UPC地址 */
    oal_uint32                      aul_5g_upc_addr[WLAN_5G_SUB_BAND_NUM];  /* 5G UPC地址 */
    oal_uint32                      aul_2g_upc1_data[WLAN_2G_SUB_BAND_NUM][HAL_UPC_DATA_REG_NUM];/* 2G 通道1 UPC DATA */
    oal_uint32                      aul_2g_upc2_data[WLAN_2G_SUB_BAND_NUM][HAL_UPC_DATA_REG_NUM];/* 2G 通道2 UPC DATA */
    oal_uint32                      aul_5g_upc1_data[WLAN_5G_SUB_BAND_NUM][HAL_UPC_DATA_REG_NUM];/* 5G 通道1 UPC DATA */
    oal_uint32                      aul_5g_upc2_data[WLAN_5G_SUB_BAND_NUM][HAL_UPC_DATA_REG_NUM];/* 5G 通道2 UPC DATA */
    oal_uint8                       auc_2g_cali_upc_code[WLAN_RF_CHANNEL_NUMS][WLAN_2G_SUB_BAND_NUM];/* 2G校准的UPC Code */
    oal_uint8                       auc_5g_cali_upc_code[WLAN_RF_CHANNEL_NUMS][WLAN_5G_CALI_SUB_BAND_NUM];  /* 5G校准的UPC Code(区分20M和80M) */
    oal_uint32                      aul_pa_gain_code[WLAN_BAND_BUTT];       /* 2G/5G PAIN CODE */
    oal_uint32                      ul_dac_addr;                            /* DAC地址 */
    oal_uint32                      aul_dac_data[WLAN_BAND_BUTT];           /* 2G/5G DAC DATA */
    oal_uint32                      ul_lpf_addr;                            /* DAC地址 */
    oal_uint32                      aul_lpf_data[WLAN_BAND_BUTT];           /* 2G/5G LPF DATA */
    oal_uint8                       auc_reserve_addr[2];
#endif  /* _PRE_WLAN_FEATURE_TPC_OPT */

    /* V200新增通道1 dac and pa code */
#ifdef _PRE_WLAN_PRODUCT_1151V200
    oal_uint32                      aul_dac_pa_code_ch1[WLAN_BAND_BUTT];       /* 通道1 2G/5G dac and PA CODE */
#endif

    /*不同帧的tpc code*/
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    oal_uint32                      aul_ack_cts_pow_code[WLAN_OFDM_ACK_CTS_TYPE_BUTT][WLAN_2G_SUB_BAND_NUM+WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_2g_dsss_ack_cts_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint32                      aul_rts_pow_code[WLAN_2G_SUB_BAND_NUM+WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_one_pkt_pow_code[WLAN_2G_SUB_BAND_NUM+WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_bar_pow_code[WLAN_2G_SUB_BAND_NUM+WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_resv2[WLAN_2G_SUB_BAND_NUM];
    oal_uint32                      aul_resv3[WLAN_2G_SUB_BAND_NUM];
    oal_uint32                      aul_cfend_pow_code[WLAN_2G_SUB_BAND_NUM+WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_resv5[WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_5g_vht_report_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint32                      aul_resv6[WLAN_5G_SUB_BAND_NUM];
#else
    oal_uint32                      aul_2g_ofdm_ack_cts_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint32                      aul_5g_ack_cts_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_dsss_ack_cts_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_rts_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_one_pkt_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_abort_selfcts_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_abort_cfend_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_abort_null_data_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_2g_cfend_pow_code[WLAN_2G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_rts_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_one_pkt_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_abort_cfend_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_cfend_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_ndp_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_vht_report_pow_code[WLAN_5G_SUB_BAND_NUM];
    oal_uint8                       auc_5g_abort_null_data_pow_code[WLAN_5G_SUB_BAND_NUM];
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_uint8                       auc_reserve[2];
#endif
#endif
    /*读取不同帧格式的速率*/
    oal_uint8                       uc_rate_bar;
    oal_uint8                       uc_rsv[3];
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    oal_uint8                       auc_rate_ofdm_ack_cts[WLAN_OFDM_ACK_CTS_TYPE_BUTT];
#else
    oal_uint32                      ul_rate_ofdm_ack_cts;
#endif
    oal_uint8                       uc_rate_rts;
    oal_uint8                       uc_rate_one_pkt;
    oal_uint8                       uc_rate_abort_selfcts;
    oal_uint8                       uc_rate_abort_cfend;
    oal_uint8                       uc_rate_cfend;
    oal_uint8                       uc_rate_ndp;
    oal_uint8                       uc_rate_vht_report;
    oal_uint8                       uc_rate_abort_null_data;
}hal_phy_pow_param_stru;

#ifdef _PRE_WLAN_FEATURE_DFS
typedef struct
{
    /*normal pulse det*/
    oal_uint8                       uc_irq_cnt;
    oal_uint8                       uc_radar_cnt;
    oal_uint8                       uc_irq_cnt_old;
    oal_bool_enum_uint8             en_timeout;

    oal_bool_enum_uint8             en_timer_start;
    oal_bool_enum_uint8             en_is_enabled;
    oal_uint8                       auc_reserv1[2];

    oal_uint32                      ul_period_cnt;
    frw_timeout_stru                st_timer;
}hal_dfs_normal_pulse_det_stru;

typedef struct
{
    /*误报新增过滤条件*/
    oal_bool_enum_uint8             en_fcc_chirp_duration_diff;
    oal_bool_enum_uint8             en_fcc_chirp_pow_diff;
    oal_bool_enum_uint8             en_fcc_type4_duration_diff;
    oal_bool_enum_uint8             en_fcc_chirp_eq_duration_num;
}hal_dfs_pulse_check_filter_stru;

typedef struct
{
    oal_bool_enum_uint8  en_chirp_enable;
    oal_uint8            uc_chirp_wow_wake_flag;      //在wow的gpio中断上半部标记，表示刚从wow唤醒
    oal_uint8            uc_chirp_detect_flag;        //在wow的gpio中断使用
#if defined(_PRE_WLAN_FEATURE_DFS_OPTIMIZE) || defined(_PRE_WLAN_FEATURE_DFS)
    oal_bool_enum_uint8             en_log_switch;

    hal_dfs_radar_type_enum_uint8   en_radar_type;
    oal_uint8                       uc_chirp_cnt;
    oal_uint8                       uc_chirp_cnt_total;
    oal_bool_enum_uint8             en_chirp_timeout;

    oal_uint32                      ul_last_timestamp_for_chirp_pulse;

    frw_timeout_stru                st_timer;
    oal_uint32                      ul_min_pri;

    /*crazy report det*/
    oal_uint8                       uc_chirp_cnt_for_crazy_report_det;
    oal_bool_enum_uint8             en_crazy_report_cnt;
    oal_bool_enum_uint8             en_crazy_report_is_enabled;
    oal_uint8                       auc_resv[1];

    frw_timeout_stru                st_timer_crazy_report_det;
    frw_timeout_stru                st_timer_disable_chirp_det;

    /*normal pulse det timer*/
    hal_dfs_normal_pulse_det_stru   st_dfs_normal_pulse_det;

    /*误报过滤条件*/
    hal_dfs_pulse_check_filter_stru st_dfs_pulse_check_filter;

#else
    oal_uint8            uc_resv;
#endif
    oal_uint32           ul_chirp_time_threshold;
    oal_uint32           ul_chirp_cnt_threshold;
    oal_uint32           ul_time_threshold;
    oal_uint32           ul_last_burst_timestamp;
    oal_uint32           ul_last_burst_timestamp_for_chirp;
}hal_dfs_radar_filter_stru;
#endif

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
typedef struct
{
    oal_uint16     us_max_offset_tsf;     /* 最长耗时的时间 */
    oal_uint16     us_mpdu_len;           /* 最长耗时的mpdu长度 */
    oal_uint16     us_tx_excp_cnt;        /* 发送完成耗时异常统计 */
    oal_uint8      uc_q_num;              /* 最长耗时的q_num*/
    oal_uint8      auc_resv;
}hal_tx_excp_info_stru;
#endif

typedef struct
{
    oal_uint32          ul_tkipccmp_rep_fail_cnt;    /* 重放攻击检测计数TKIP + CCMP */
    oal_uint32          ul_tx_mpdu_cnt;              /* 发送计数非ampdu高优先级 + 普通优先级 + ampdu中mpdu */
    oal_uint32          ul_rx_passed_mpdu_cnt;       /* 属于AMPDU MPDU的FCS正确的MPDU数量 */
    oal_uint32          ul_rx_failed_mpdu_cnt;       /* 属于AMPDU MPDU的FCS错误的MPDU数量 */
    oal_uint32          ul_rx_tkipccmp_mic_fail_cnt; /* kip mic + ccmp mic fail的帧数 */
    oal_uint32          ul_key_search_fail_cnt;      /* 接收key serach fail的帧数 */
    oal_uint32          ul_phy_rx_dotb_ok_frm_cnt;       /* PHY接收dotb ok的帧个数 */
    oal_uint32          ul_phy_rx_htvht_ok_frm_cnt;      /* PHY接收vht ht ok的帧个数 */
    oal_uint32          ul_phy_rx_lega_ok_frm_cnt;       /* PHY接收legace ok的帧个数 */
    oal_uint32          ul_phy_rx_dotb_err_frm_cnt;      /* PHY接收dotb err的帧个数 */
    oal_uint32          ul_phy_rx_htvht_err_frm_cnt;     /* PHY接收vht ht err的帧个数 */
    oal_uint32          ul_phy_rx_lega_err_frm_cnt;      /* PHY接收legace err的帧个数 */

}hal_mac_key_statis_info_stru;

/* 会影响目标vdet值的参数集合 */
typedef union                                                   /* todo */
{
    struct
    {
        wlan_channel_band_enum_uint8    en_freq         :4;
        wlan_bw_cap_enum_uint8          en_band_width   :4;
        oal_uint8                       uc_channel;
        oal_uint8                       uc_pdet_chain   :4;
        wlan_mod_enum_uint8             en_mod          :4;
        oal_uint8                       uc_tx_pow;
    } st_rf_core_para;
    oal_uint32 ul_para;
} hal_dyn_cali_record_union;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
typedef struct
{
    hal_dyn_cali_record_union           un_record_para;
    wlan_phy_protocol_enum_uint8        en_cur_protocol;
    oal_int16                           s_real_pdet;
    oal_uint8                           uc_upc_gain_idx;
    oal_uint8                           uc_rate_idx;
    oal_bool_enum_uint8                 en_flag;
    oal_bool_enum_uint8                 en_pdbuf_opt;
    oal_uint8                           auc_rsv[1];
}hal_pdet_info_stru;

#else

typedef struct
{
    wlan_channel_band_enum_uint8        en_freq;
    wlan_bw_cap_enum_uint8              en_cur_band_width;
    oal_uint8                           uc_cur_channel;
    wlan_phy_protocol_enum_uint8        en_cur_protocol;
    oal_uint8                           uc_pdet_chain;
    oal_int16                           s_tx_pow;
    oal_int16                           s_real_pdet;
}hal_pdet_info_stru;
#endif

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
typedef struct
{
    hal_dyn_cali_record_union          un_record_para;
    oal_int16                          s_real_pdet;
    oal_int16                          s_exp_pdet;
    oal_uint8                          auc_resv[4];
}hal_dyn_cali_usr_record_stru;

typedef struct
{
    oal_bool_enum_uint8     aen_dyn_cali_chain_comp_flag[WLAN_RF_CHANNEL_NUMS];
    oal_uint8               uc_tx_chain_sel;
    oal_uint8               auc_resv[1];
}hal_dyn_cali_chain_status_stru;

typedef struct hal_dyn_cali_val{
    oal_uint16              aus_cali_en_cnt[WLAN_BAND_BUTT];
    oal_uint16              aus_cali_en_interval[WLAN_BAND_BUTT];      /* 两次动态校准间隔描述符个数  */

    oal_uint16              us_cali_ch_sw_interval;   /* 通道切换间隔校准使能描述符个数  */
    oal_uint16              us_cali_ch_sw_cnt;        /*通道发送动态校准使能描述符个数*/

    oal_uint8               uc_cali_pdet_min_th;      /* 动态校准极小阈值  */
    oal_uint8               uc_cali_pdet_max_th;      /* 动态校准极大阈值  */
    oal_uint8               uc_cali_chain;            /*  当前动态校准校准通道号  */
    oal_bool_enum_uint8     en_realtime_cali_adjust;      /* 动态校准使能补偿开关 */

    hal_dyn_cali_usr_record_stru   ast_vap_pow_info[WLAN_RF_CHANNEL_NUMS];
    hal_dyn_cali_chain_status_stru st_cali_chain_status_info;

    oal_int16               s_temp;                       /* Tsensor检测温度 */
    oal_bool_enum_uint8     en_realtime_cali_comp_flag;   /* 动态校准处理完成标志 */
}hal_dyn_cali_val_stru;
#endif

/* 扫描状态，通过判断当前扫描的状态，判断多个扫描请求的处理策略以及上报扫描结果的策略 */
typedef enum
{
    MAC_SCAN_STATE_IDLE,
    MAC_SCAN_STATE_RUNNING,

    MAC_SCAN_STATE_BUTT
}mac_scan_state_enum;
typedef oal_uint8 mac_scan_state_enum_uint8;

/* 扫描状态，通过判断当前扫描的状态，判断多个扫描请求的处理策略以及上报扫描结果的策略 */
typedef enum
{
    HAL_SCAN_PASUE_TYPE_CHAN_CONFLICT,
    HAL_SCAN_PASUE_TYPE_SWITCH_BACK,

    HAL_SCAN_PASUE_TYPE_BUTT
}hal_scan_pasue_enum;
typedef oal_uint8 hal_scan_pause_type_enum_uint8;

/* 扫描状态，打印tx dscr控制 */
typedef enum
{
    HAL_SCAN_TX_DSCR_DBG_CTL_INIT,
    HAL_SCAN_TX_DSCR_DBG_CTL_PRINT_PERMIT,
    HAL_SCAN_TX_DSCR_DBG_CTL_PRINTED,

    HAL_SCAN_TX_DSCR_DBG_CTL_BUTT
}hal_scan_tx_dscr_dbg_ctl_enum;
typedef oal_uint8 hal_scan_tx_dscr_dbg_ctl_enum_uint8;

typedef struct
{
    frw_timeout_stru             st_scan_timer;                             /* 扫描定时器用于切换信道 */
    mac_channel_stru             st_home_channel;                           /* 记录home channel */

    oal_uint8                    auc_original_mac_addr[WLAN_MAC_ADDR_LEN];  /* 扫描开始前保存原始的MAC地址 */
    wlan_scan_mode_enum_uint8    en_scan_mode;                              /* 扫描模式*/
    oal_uint8                    uc_scan_pause_bitmap;                         /* 扫描是否被暂停 */

    oal_uint8                    uc_start_chan_idx;                         /* 扫描起始idx */
    oal_uint8                    uc_scan_chan_idx;                          /* 当前扫描信道索引累加值 */
    oal_uint8                    uc_channel_nums;                           /* 此hal扫描的信道个数 */
    mac_scan_state_enum_uint8    en_curr_scan_state;                        /* 此hal扫描状态 */

    oal_uint8                    uc_scan_ap_num_in_2p4;                     /* 在2.4g上扫到ap的信道个数 */
    oal_uint8                    uc_last_channel_band;                      /* 上一个channel的频段 */
    oal_bool_enum_uint8          en_scan_curr_chan_find_bss_flag;           /* 本信道是否扫到bss */
    oal_bool_enum_uint8          en_need_switch_back_home_channel;          /* 背景扫描时，扫描完一个信道，判断是否需要切回工作信道工作 */

    oal_uint8                    uc_curr_channel_scan_count;                /* 记录当前信道的扫描次数，第一次发送广播ssid的probe req帧，后面发送指定ssid的probe req帧 */
    oal_uint8                    uc_max_scan_count_per_channel;             /* 每个信道的扫描次数 */
    oal_uint8                    uc_scan_channel_interval;                  /* 间隔n个信道，切回工作信道工作一段时间 */
    oal_bool_enum_uint8          en_working_in_home_chan;                   /* 是否工作在工作信道 */

    oal_uint16                   us_scan_time;                              /* 扫描在某一信道停留此时间后，扫描结束, ms，必须配置为MAC负载统计周期的整数倍 */
    oal_uint16                   us_work_time_on_home_channel;              /* 背景扫描时，返回工作信道工作的时间 */

    oal_uint32                   ul_scan_timestamp;                         /* 记录最新一次扫描开始的时间 */

    oal_uint8                    uc_scan_ap_num_in_5p0;                     /* 在5g上扫到ap的信道个数 */
    oal_bool_enum_uint8          en_is_fast_scan;
    wlan_channel_band_enum_uint8 en_scan_band;                              /* 扫描的频段,支持fast scan使用 */
    hal_scan_tx_dscr_dbg_ctl_enum_uint8 en_tx_dscr_debug_ctl;               /* 控制扫描时打印部分tx dscr */
}hal_scan_params_stru;

typedef struct
{
    oal_uint8                    uc_num_channels_2G;
    oal_uint8                    uc_num_channels_5G;
    oal_uint8                    uc_max_scan_count_per_channel;   /* 每个信道的扫描次数 */
    oal_bool_enum_uint8          en_is_fast_scan;

    oal_uint8                    uc_scan_channel_interval;       /* 间隔n个信道，切回工作信道工作一段时间 */
    wlan_scan_mode_enum_uint8    en_scan_mode;
    wlan_channel_band_enum_uint8 en_scan_band;                   /* 扫描的频段,支持fast scan使用 */
    oal_uint8                    auc_resv[1];

    oal_uint16                   us_scan_time;                   /* 扫描在某一信道停留此时间后，扫描结束, ms，必须配置为MAC负载统计周期的整数倍 */
    oal_uint16                   us_work_time_on_home_channel;   /* 背景扫描时，返回工作信道工作的时间 */
}hal_scan_info_stru;

struct tag_hal_to_dmac_device_stru;

typedef oal_void (*p_dbdc_handle_stop_event_cb)(struct tag_hal_to_dmac_device_stru *pst_hal_device);
typedef oal_void (*p_dmac_scan_one_channel_start_cb)(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_is_scan_start);

typedef struct
{
    p_dbdc_handle_stop_event_cb              p_dbdc_handle_stop_event;
    p_dmac_scan_one_channel_start_cb         p_dmac_scan_one_channel_start;
}hal_device_fsm_cb;

/* hal device状态机结构体 */
typedef struct _hal_device_handle_handler
{
    oal_fsm_stru                     st_oal_fsm;                     /* 状态机 */
    oal_uint8                        uc_is_fsm_attached;             /* 状态机是否已经注册 */
#ifdef _PRE_WLAN_FEATURE_STA_PM
    oal_uint8                        bit_wait_hw_txq_empty    :1;    /* 标记是否等待硬件队列为空后关闭mac pa */
    oal_uint8                        bit_mac_pa_switch_en     :1;
    oal_uint8                        bit_resv                 :6;
    oal_uint8                        uc_is_work_sub_fsm_attached;    /* 子状态机是否已经注册 */

    oal_fsm_stru                     st_oal_work_sub_fsm;            /* 子状态机 */
    oal_uint32                       aul_vap_pm_state_bitmap[HAL_WORK_SUB_STATE_NUM];     /* vap ps state bitmap */
    oal_uint8                        auc_last_set_vap_id[HAL_WORK_SUB_STATE_NUM];
    oal_uint16                       aus_last_event[HAL_WORK_SUB_STATE_NUM];
#endif
    hal_device_fsm_cb                st_hal_device_fsm_cb;
    oal_uint8                        _rom[4];
} hal_dev_fsm_stru;

/* hal device m2s状态机结构体 */
typedef struct
{
    oal_fsm_stru                     st_oal_fsm;                     /* 状态机 */
    oal_uint8                        uc_is_fsm_attached;             /* 状态机是否已经注册 */
    wlan_m2s_type_enum_uint8         en_m2s_type;                    /*0:软切换 1:硬切换*/
    wlan_m2s_mode_stru               st_m2s_mode;                    /* m2s业务申请占用bitmap，用户优先级管理 */
    oal_bool_enum_uint8              en_command_scan_on;             /* command下优化扫描是否在执行，防止miso时误end */
    oal_uint8                        _rom[4];
} hal_m2s_fsm_stru;

/* 配置相关信息(包含定制化) */
typedef struct
{
    wlan_nss_enum_uint8                     en_nss_num;              /* Nss 空间流个数 */
    oal_uint8                               uc_phy_chain;            /* phy通道1/2/3 */
    oal_uint8                               uc_single_tx_chain;
    /* 管理帧采用单通道发送时选择的通道(单通道时要配置和uc_phy_chain一致),或者用于配置phy接收通道寄存器 */
    oal_uint8                               uc_rf_chain;             /* rf通道1/2/3(双通道),解决方案和芯片都不支持交叉 */

    oal_bool_enum_uint8                     en_tx_stbc_is_supp;      /* 是否支持最少2x1 STBC发送*/
    oal_bool_enum_uint8                     en_rx_stbc_is_supp;      /* 是否支持stbc接收,支持2个空间流 */
    oal_bool_enum_uint8                     uc_dpd_switch2G;
    oal_bool_enum_uint8                     uc_dpd_switch5G;

    oal_bool_enum_uint8                     en_su_bfmer_is_supp;     /* 是否支持单用户beamformer */
    oal_bool_enum_uint8                     en_su_bfmee_is_supp;     /* 是否支持单用户beamformee */
    oal_bool_enum_uint8                     en_mu_bfmer_is_supp;     /* 是否支持多用户beamformer */
    oal_bool_enum_uint8                     en_mu_bfmee_is_supp;     /* 是否支持多用户beamformee */

    oal_bool_enum_uint8                     en_is_supp_11ax;
    oal_bool_enum_uint8                     en_radar_detector_is_supp;
    oal_bool_enum_uint8                     en_dpd_is_supp;
    wlan_bw_cap_enum_uint8                  en_wlan_bw_max;

    oal_bool_enum_uint8                     en_11n_sounding;
    oal_bool_enum_uint8                     en_green_field;
    oal_bool_enum_uint8                     en_txopps_is_supp;        /*是否使能TXOP PS*/
    oal_bool_enum_uint8                     en_1024qam_is_supp;      /* 支持1024QAM速率 */
    /* ROM化预留hal device能力 */
    oal_bool_enum_uint8                     en_nb_is_supp;
    oal_uint8                               uc_su_bfee_num;
    oal_uint8                               uc_phy2dscr_chain;         /* 和uc_phy_chain对应，这里是配置发送描述符 */
    oal_bool_enum_uint8                     en_11n_txbf_is_supp;       /* 是否支持11n txbf*/

    oal_uint8                               uc_ctrl_frm_tx_double_chain_flag;
    oal_bool_enum_uint8                     en_is_supp_rev5;
    oal_bool_enum_uint8                     en_is_supp_rev6;
    oal_bool_enum_uint8                     en_is_supp_rev7;
}hal_cfg_cap_info_stru;

/* hal device alg结构体定义 */
typedef enum
{
    HAL_ALG_DEVICE_STRU_ID_SCHEDULE,
    HAL_ALG_DEVICE_STRU_ID_AUTORATE,
    HAL_ALG_DEVICE_STRU_ID_SMARTANT,
    HAL_ALG_DEVICE_STRU_ID_DBAC,
    HAL_ALG_DEVICE_STRU_ID_TXBF,
    HAL_ALG_DEVICE_STRU_ID_ANTI_INTF,
    HAL_ALG_DEVICE_STRU_ID_MWO_DET,
    HAL_ALG_DEVICE_STRU_ID_TPC,        //TPC结构体
    HAL_ALG_DEVICE_STRU_ID_EDCA_OPT,
    HAL_ALG_DEVICE_STRU_ID_CCA_OPT,
    HAL_ALG_DEVICE_STRU_ID_INTF_DET,

    HAL_ALG_DEVICE_STRU_ID_BUTT
}hal_alg_device_stru_id_enum;
typedef oal_uint8 hal_alg_device_stru_id_enum_uint8;

/* 挂在各个hal device上的数据结构 */
typedef struct
{
    oal_void       *p_alg_info[HAL_ALG_DEVICE_STRU_ID_BUTT];
}hal_alg_device_stru;

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
/*干扰检测模块PK模式探测阶段枚举*/
typedef enum
{
    HAL_ALG_INTF_PKADJ_STAGE_INIT      = 0,     /* 用于统计跑流的初始干扰 */
    HAL_ALG_INTF_PKADJ_STAGE_TXOP      = 1,
    HAL_ALG_INTF_PKADJ_STAGE_AIFSN     = 2,
    HAL_ALG_INTF_PKADJ_STAGE_CW        = 3,

    HAL_ALG_INTF_PK_STAGE_BUTT
}hal_alg_intf_pk_stage_enum;
typedef oal_uint8 hal_alg_pk_intf_stage_enum_uint8;
#endif

/*干扰检测模块检测到的邻频叠频干扰模式改变时回调函数枚举定义*/
typedef enum
{
    HAL_ALG_INTF_DET_ADJINTF_NO        = 0,    /* 无干扰 */
    HAL_ALG_INTF_DET_ADJINTF_MEDIUM    = 1,    /* 中等强度干扰 */
    HAL_ALG_INTF_DET_ADJINTF_STRONG    = 2,    /* 强干扰 */
    HAL_ALG_INTF_DET_ADJINTF_CERTIFY   = 3,    /* 认证干扰 */
    HAL_ALG_INTF_DET_STATE_PKADJ       = 4,    /* PK参数调整状态 */
    HAL_ALG_INTF_DET_STATE_PK          = 5,    /* PK状态 */
    HAL_ALG_INTF_DET_ADJINTF_BUTT
}hal_alg_intf_det_mode_enum;
typedef oal_uint8 hal_alg_intf_det_mode_enum_uint8;

/* 算法使用的用户距离状态 */
typedef enum
{
    HAL_ALG_USER_DISTANCE_NEAR       = 0,
    HAL_ALG_USER_DISTANCE_NORMAL     = 1,
    HAL_ALG_USER_DISTANCE_FAR        = 2,

    HAL_ALG_USER_DISTANCE_BUTT
}hal_alg_user_distance_enum;
typedef oal_uint8 hal_alg_user_distance_enum_uint8;

/* 设备距离、干扰状态等信息 */
typedef struct
{
    hal_alg_user_distance_enum_uint8           en_alg_distance_stat;    /* 距离状态  */
    hal_alg_intf_det_mode_enum_uint8           en_adj_intf_state;       /* 邻频干扰状态 */
    oal_bool_enum_uint8                        en_co_intf_state;        /* 同频干扰状态 */
    oal_uint8                                  uc_reserv;
} hal_alg_stat_info_stru;

typedef struct
{
    oal_uint32              ul_rx_rate;
    oal_uint16              us_rx_rate_stat_count;
    oal_bool_enum_uint8     en_compatibility_enable;
    oal_bool_enum_uint8     en_compatibility_stat;
    oal_uint32              aul_compatibility_rate_limit[WLAN_BW_CAP_BUTT][WLAN_PROTOCOL_BUTT];
} hal_compatibility_stat_stru;

typedef struct
{
    oal_uint8  uc_fe_print_ctrl;      //寄存器输出计数
    oal_uint8  auc_rev[3];
}hal_device_dft_stat_stru;


typedef struct
{
    oal_uint16  us_last_sn;          //发送失败维测输出控制避免sdt过多打印导致死机
    oal_uint8   uc_print_ctl;
    oal_uint8   auc_rev[1];
}hal_device_sdt_stat_stru;


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
typedef enum
{
    HAL_WORK_FUNC_ID_CALI             = 0,
    HAL_WORK_FUNC_ID_BUTT
}hal_work_func_id_enum;
typedef oal_uint8 hal_work_func_id_enum_uint8;
#endif

#ifdef _PRE_WLAN_FEATURE_TPC_OPT
typedef struct
{
    oal_uint8 auc_rate_max_tx_pow_ctrl_table[HAL_POW_RATE_POW_CODE_TABLE_LEN][WLAN_BAND_BUTT];
    oal_uint8 auc_rev[2];
}hal_cfg_rate_max_tx_pow_stru;

typedef struct
{
    oal_int16 as_pow_level_table[HAL_POW_LEVEL_NUM][WLAN_BAND_BUTT];
}hal_cfg_tpc_lvl_vs_gain_stru;

/* DMAC_VAP发射功率TX功率控制相关的参数结构体 */
typedef struct
{
    /* 1) MAX_Power@rate */
    hal_cfg_rate_max_tx_pow_stru          *pst_max_tx_pow_per_rate_ctrl;
#ifdef _PRE_WLAN_1103_PILOT
    oal_uint16                             aus_resv[NUM_OF_NV_MAX_TXPOWER-3];
    oal_int16                              s_11b_ofdm_delt_pow;    /* 11b和OFDM在相同upc下的功率差，精度0.01dBm */
    oal_uint8                              uc_reg_pow;
    oal_uint8                              uc_tx_power;
    /* PAPR=5.5时的功率偏差值 */
    oal_uint8                              uc_pwr_compens_val_6m;
    oal_uint8                              uc_pwr_compens_val_mcs0;
#else
    /* 2) DBB_SCALING@rate */
    oal_uint16                             aus_dbb_scale_per_rate_ctrl[NUM_OF_NV_MAX_TXPOWER];
#endif
    /* 3) LPF Index@rate */
    oal_uint8                             *puc_lpf_idx_per_rate_ctrl_2g;
    oal_uint8                             *puc_lpf_idx_per_rate_ctrl_5g;
    /* 4) TPC档位 vs Target Gain表 */
    hal_cfg_tpc_lvl_vs_gain_stru  *pst_tpc_lvl_vs_gain_ctrl;

    //TODO:
    /* 5) 各速率DBB scaling增值(两个，最多支持两个VAP共存，Pilot用)
       6) 各通道DBB scaling增值(两个，最多支持两个VAP共存，Pilot用) */

    /* 7)  TPC档位 vs TPC Code/Gain表 */
    hal_rate_pow_code_gain_table_stru      *pst_rate_pow_table_2g;
    hal_rate_pow_code_gain_table_stru      *pst_rate_pow_table_5g;

    //TODO:
      // 8) TPC档位 vs DPD Code表(两个，最多支持两个VAP共存)
    oal_void                              *_rom;
}hal_device_tx_pwr_ctrl_stru;

#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
/* ps mode管理结构体 */
typedef struct
{
    oal_uint8   bit_ps_on         : 1,        /* ps软件机制: 0=关闭, 1=打开 */
                bit_delba_on      : 1,        /* 删减聚合逻辑: 0=关闭, 1=打开 */
                bit_reply_cts     : 1,        /* 是否回复CTS， 0=不回复， 1=回复 */
                bit_rsp_frame_ps  : 1,        /* resp帧节能位是否设置 0=不设置， 1=设置 */
                bit_ps_slot_detect  : 1,        /* 是否使能动态slot探测功能，delba逻辑触发速率达到标准时打开vap侧 0=不使能， 1=使能 */
                bit_btcoex_wl0_tx_slv_on  : 1,         /* btcoex下wl0 tx slv配置开关 */
                bit_resv          : 2;
}hal_coex_sw_preempt_mode_stru;

typedef struct
{
    hal_coex_sw_preempt_mode_stru       st_sw_preempt_mode;
    hal_coex_sw_preempt_type_uint8      en_sw_preempt_type;
    hal_coex_sw_preempt_subtype_uint8   en_sw_preempt_subtype;
    hal_fcs_protect_coex_pri_enum_uint8 en_protect_coex_pri;      /* one pkt帧发送优先级 */
    oal_uint16                          us_timeout_ms;            /* ps超时时间，page扫描190slot 音乐和数传30slot */
    oal_bool_enum_uint8                 en_last_acl_status;       /* 保存上一次acl状态 */
    oal_bool_enum_uint8                 en_ps_stop;               /* 特定业务下，不需要开启ps，通知蓝牙不要发送ps中断 */
    oal_uint32                          ul_ps_cur_time;           /* 用于ps中断上下半部执行时间统计 */
    oal_atomic                          ul_ps_event_num;          /* ps中断event数目 */
    oal_atomic                          ul_acl_en_cnt;            /* 连续acl cnt的计数，达到一定次数时，可能是蓝牙长时间为恢复 */
    oal_bool_enum_uint8                 en_ps_pause;              /* 特定业务下，需要暂停ps，不影响ps中断处理，防止和wifi特定业务冲突 */
    oal_bool_enum_uint8                 en_coex_pri_forbit;       /* coex pri控制开关，ldac下需要关闭该功能 */
    oal_uint8                           _rom[2];
}hal_device_btcoex_sw_preempt_stru;
#endif

/*接收端描述符分配算法进行吞吐量统计的结构体*/
typedef struct
{
    oal_uint16                      us_rx_dscr_alg_thres;         /* 记录算法门限 */
    oal_uint16                      us_rx_event_pkts_sum;         /* 记录一个周期(100ms)内接收队列接收帧的数量 */
    oal_uint16                      us_rx_event_cnt;              /* 记录一个周期(100ms)内接收队列产生isr info数量 */
    oal_uint16                      us_rx_cur_min_thres;          /* 记录上个周期(100ms)结束队列处于忙闲的状态 */
    oal_uint16                      us_rx_max_netbuff_used_cnt;   /* 记录一个周期(100ms)内接收队列挂载的资源个数的峰值 */
    oal_uint16                      us_rx_max_dscr_used_cnt;      /* 记录一个周期(100ms)内接收队列挂载的资源个数的峰值 */
    oal_uint8                       uc_resv;
    oal_uint8                       uc_resv1;
    oal_uint8                       uc_previous_cycle_stats;      /* 记录上个周期(100ms)结束队列处于忙闲的状态 */
    oal_uint8                       uc_continus_idle_cnt;         /* 连续若干个周期空闲后才恢复接收队列未空闲，防止算法参数太灵敏 */
}alg_rx_dscr_ctl_alg_info_stru;


/*接收端描述符分配算法进行吞吐量统计的结构体*/
typedef struct
{
    oal_void                       *pst_rx_dscr_ctl;              /* hal dev下保存chip级别描述符管理结构的指针 */
    alg_rx_dscr_ctl_alg_info_stru  *pst_rx_dscr_ctl_alg_info;
    oal_uint16                      aus_pending_isr_cnt[HAL_RX_DSCR_QUEUE_ID_BUTT]; /* 记录pinding在乒乓队列里的isr info数量 */
    oal_uint16                      aus_pending_pkt_cnt[HAL_RX_DSCR_QUEUE_ID_BUTT]; /* 记录pinding在乒乓队列里的帧数量 */
}alg_rx_dscr_ctl_device_info_stru;

typedef struct
{
    /* MAC统计 */
    oal_uint32 ul_rx_direct_time;
    oal_uint32 ul_rx_nondir_time;
    oal_uint32 ul_tx_time;

}hal_ch_mac_statics_stru;
typedef struct
{
    /* 干扰相关统计 */
    oal_uint16 us_duty_cyc_ratio_20;/* 20M干扰繁忙度 */
    oal_uint16 us_duty_cyc_ratio_40;/* 40M干扰繁忙度 */
    oal_uint16 us_duty_cyc_ratio_80;/* 80M干扰繁忙度 */
    oal_uint16 us_sync_err_ratio;   /* 同步错误率 */
    oal_uint32 ul_rx_time;          /* rx总时间 */
    oal_uint32 ul_tx_time;          /* tx总时间 */
    oal_uint32 ul_free_time;        /* 空闲时间 */
    oal_uint32 ul_abort_time_us;    /* 被打断时间，包括共存和扫描 */

}hal_ch_intf_statics_stru;

#ifdef _PRE_WLAN_FEATURE_M2S
typedef oal_void (*p_m2s_back_to_mimo_check_cb)(struct tag_hal_to_dmac_device_stru *pst_hal_device);

typedef struct
{
    p_m2s_back_to_mimo_check_cb    p_m2s_back_to_mimo_check;
}hal_device_m2s_mgr_cb;

typedef struct
{
    wlan_m2s_mgr_vap_stru             ast_m2s_blacklist[WLAN_M2S_BLACKLIST_MAX_NUM];    /* 处于管理用户时，需要调整action方案 */
    oal_uint8                         uc_blacklist_bss_index;                       /* 黑名单MAC地址的数组下标 */
    oal_uint8                         uc_blacklist_bss_cnt;                         /* 黑名单用户个数 */
    oal_bool_enum_uint8               en_m2s_switch_protect;                        /* 是否使能m2s切换保护，针对sta模式 */
    oal_bool_enum_uint8               en_delay_swi_miso_hold;  /* 存在关联ap用户是blacklist none的，需要刷新标记为切换保持miso稳定态 */

    oal_bool_enum_uint8               en_mss_on;               /* 是否使能mss上报和下发功能 */
    oal_uint8                         uc_rssi_mgmt_single_txchain;   /* 最近一次cur rssi mgmt逻辑学习到的合适single_txchain */
    oal_bool_enum_uint8               en_blacklist_assoc_ap_on;      /* 是否sta关联上黑名单ap，和mss功能互斥 */
    hal_device_m2s_mgr_cb             st_hal_device_m2s_mgr_cb;
} hal_device_m2s_mgr_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
/* coex siso业务管理结构体 */
typedef struct
{
    oal_uint8   bit_6slot_m2s_on         : 1,        /* 6slot申请siso */
                bit_ldac_m2s_on          : 1,        /* ldac申请siso */
                bit_m2s_resv             : 6;
}hal_coex_m2s_mode_bitmap_stru;

/* coex mimo业务管理结构体 */
typedef struct
{
    oal_uint8   bit_6slot_s2m_on         : 1,        /* sco申请回mimo */
                bit_ldac_s2m_on          : 1,        /* ldac申请回mimo */
                bit_a2dp_s2m_on          : 1,        /* a2dp申请回mimo */
                bit_s2m_resv             : 5;
}hal_coex_s2m_mode_bitmap_stru;

typedef struct
{
    frw_timeout_stru                  st_s2m_resume_timer;                /* 切回mimo状态超时等待定时器 */
    frw_timeout_stru                  bt_coex_s2m_siso_ap_timer;          /* siso ap切换回mimo探测时间，防止音乐切歌造成太过频繁 */
    frw_timeout_stru                  st_wl0_tx_slv_conf_resume_timer;    /* 动态辅天线发送硬件状态恢复定时器,非周期 */
    frw_timeout_stru                  st_m2s_rssi_detect_timer;           /* rssi逻辑周期定时器 */

    oal_uint16                        us_timeout_ms;                      /* 当前业务下的定时器超时时间 */
    oal_bool_enum_uint8               en_siso_ap_excute_on;               /* siso ap切换siso执行标志 */
    oal_bool_enum_uint8               en_timer_need_restart;              /* one pkt之后是否需要恢复定时器 */

    oal_bool_enum_uint8               en_wl0_tx_slv_allow;                /* 功能暂停开关，主开关是关着，这里必须关 */
    hal_coex_m2s_mode_bitmap_stru     st_m2s_mode_bitmap;    /* 申请siso切换的业务bitmap，目前是ldac=1 和6slot=2 */
    hal_coex_s2m_mode_bitmap_stru     st_s2m_mode_bitmap;    /* 申请切换回mimo的业务bitmap，目前是ldac=0 a2dp=0 和6slot=0 3者都满足 */
    hal_coex_s2m_mode_bitmap_stru     st_s2m_wait_bitmap;    /* 需要切回mimo的超时处理函数里面才用此bit来清m2s标记，防止ldac结束时，6slot来了又切siso流程 */

    oal_bool_enum_uint8               en_ps_occu_down_delay;        /* ps帧occu拉低是否需要有动态slot来拉低 */
    oal_bool_enum_uint8               en_log_print_on;              /* slot detect日志打印是否开启 */
    oal_uint8                         uc_btcoex_tx_max_aggr_num;    /* 用于c1 siso切换场景下 wifi 上行ldac和6slot下限制聚合个数，提高兼容性 */
    oal_bool_enum_uint8               en_dynamic_slot_pause;        /* sta出现需要发self-cts时，硬件的此功能暂时pause */

    oal_bool_enum_uint8               en_rssi_limit_on;     /* 是否开启rssi限制切换功能 */
    oal_bool_enum_uint8               en_rssi_log_on;       /* slot detect日志打印是否开启 */
    oal_int8                          c_m2s_threshold;       /* m2s的rssi门限 */
    oal_int8                          c_s2m_threshold;       /* s2m的rssi门限 */

    oal_uint8                         uc_detect_cnt_threshold;  /* 连续rssi满足门限的cnt th */
    oal_uint8                         uc_rssi_valid_cnt;         /* 连续rssi满足门限的cnt */
    oal_uint8                         uc_m2s_rssi_det_timeout_cnt;  /* 连续50ms定时器检查次数 */
    oal_bool_enum_uint8               en_rssi_s2m_on;             /* 只考虑ldac的话，要临时回mimo置标志，也同步认为a2dp结束 */

    oal_bool_enum_uint8               en_rssi_freq_fix_on;        /* 只考虑ldac的话 在mimo+ldac下需要固定频率，调试时候维测打开看 */
    oal_uint8                         bit_m2s_6slot         :1;  /*0: m2s; 1:开m2s */
    oal_uint8                         bit_m2s_ldac          :1;
    oal_uint8                         bit_m2s_siso_ap       :1;
    oal_uint8                         bit_m2s_resv          :5;
} hal_device_btcoex_mgr_stru;
#endif

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
typedef struct
{
    oal_int16                           s_vdet_val;
    oal_uint8                           en_ppa_working;  /* 0:PA；1:PPA */
    oal_uint8                           uc_rsv;
    hal_dyn_cali_record_union           un_record_para;
}hal_dyn_cali_pa_ppa_asjust_stru;
#endif

typedef struct
{
    /* 同频干扰算法 */
    oal_uint32                          ul_duty_ratio;                          /* 占空比统计 */
    oal_uint32                          ul_tx_ratio_lp;                         /*进入低功耗前发送占空比*/
    oal_uint32                          ul_rx_nondir_duty_lp;                   /*进入低功耗前接收non-direct包的占空比*/
    oal_uint32                          ul_rx_dir_duty_lp;                      /*进入低功耗前接收direct包的占空比*/
    oal_uint32                          ul_tx_sch_enqueue_time;                 /*记录硬件开始发送一个帧的起始时间*/

    /* MAC测量信息 */
    hal_ch_mac_statics_stru             st_mac_ch_stats;
    hal_ch_intf_statics_stru            st_intf_statics_stru;
#ifdef _PRE_WLAN_FEATURE_M2S
    hal_device_m2s_mgr_stru             st_device_m2s_mgr;
    /* 管理帧rssi相关 */
    hal_rx_ant_rssi_mgmt_stru           st_hal_rx_ant_rssi_mgmt;
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
    frw_timeout_stru                    st_dyn_cali_per_frame_timer;    /* 动态校准每帧的定时器 */
    hal_dyn_cali_pa_ppa_asjust_stru     st_dyn_cali_vdet_stru;
#endif

    frw_timeout_stru                    st_check_pll_lock_sts_timer;    /* 检查PLL是否失锁的定时器 */

#ifdef _PRE_WLAN_FEATURE_TEMP_PROTECT
    oal_uint8                           uc_ping_pong_disable;        /* 是否需要减少一次调度,过温保护中添加*/
    oal_uint8                           uc_temp_pro_aggr_size;       /* 过温度保护中聚合子帧数门限的调整值 */
    oal_uint8                           auc_resv[2];
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_device_btcoex_mgr_stru           st_device_btcoex_mgr;
#endif

#ifdef _PRE_WLAN_FEATURE_DFS_ENABLE
    oal_uint8                            uc_dfsmode;
    oal_uint8                            auc_resv1[3];
#endif
    /* txbf回复sounding帧的速率调整变量 */
    hal_txbf_sounding_rate_enum_uint8    uc_txbf_sounding_report_rate;
    oal_uint8                            uc_is_open_noise_est;
    oal_uint8                            uc_is_close_noise_comb;
    oal_uint8                            uc_is_open_zf;
    oal_int8                             c_evm_ant0;
    oal_int8                             c_evm_ant1;
    oal_uint8                            auc_resv2[1];
    oal_bool_enum_uint8                  en_rx_bcn_flag;
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    hal_tas_rssi_measure_stru            st_tas_rssi_report_status;
#endif
    oal_uint8                            uc_hw_txq_stall_det_timeout_cnt;
    oal_uint8                            uc_hw_txq_stall_det_abort_cnt;
}hal_to_dmac_device_rom_stru;
extern hal_to_dmac_device_rom_stru g_st_hal_to_dmac_device_rom[];

typedef struct tag_hal_to_dmac_device_stru
{
    oal_uint8                       uc_chip_id;
    oal_uint8                       uc_device_id;                               /* 此id用来获取hal device */
    oal_uint8                       uc_mac_device_id;                           /* 此id用来获取mac dmac hmac device */
    hal_lpm_state_enum_uint8        uc_curr_state;                              /* 当前芯片的低功耗状态*/
    oal_uint32                      ul_core_id;

    hal_cfg_cap_info_stru           st_cfg_cap_info;
    oal_dlist_head_stru             ast_rx_dscr_hash[HAL_RX_MAX_BUFFS];

    hal_dfr_tx_prot_stru            st_dfr_tx_prot;                             /* 用于无法送完成中断检测及恢复 */
    hal_dfr_err_opern_stru          st_dfr_err_opern[HAL_MAC_ERROR_TYPE_BUTT];  /* 用于MAC异常中断恢复 */

    hal_rx_dscr_queue_header_stru   ast_rx_dscr_queue[HAL_RX_QUEUE_NUM];
    hal_tx_dscr_queue_header_stru   ast_tx_dscr_queue[HAL_TX_QUEUE_NUM];

    oal_uint32                      ul_rx_machw_dscr_addr[HAL_RX_DSCR_QUEUE_ID_BUTT]; /* 接收硬件队列状态寄存器 */

    oal_uint16                      us_rx_normal_dscr_cnt;
    oal_uint16                      us_rx_small_dscr_cnt;
    oal_uint32                      ul_track_stop_flag;

    hal_alg_stat_info_stru          st_hal_alg_stat;       /* 算法使用的统计信息结构体 */
    hal_compatibility_stat_stru     st_hal_compatibility_stat;

    oal_uint8                       uc_al_tx_hw;
    oal_bool_enum_uint8             en_narrow_bw_open;
    oal_uint8                       uc_narrow_bw;
    oal_bool_enum_uint8             en_is_mac_pa_enabled;      /* pa是否使能 */

    oal_uint32                      bit_al_tx_flag        :3;  /*0: 关闭常发; 1:保留给RF测试; 2: ampdu聚合帧常发; 3:非聚合帧常发*/
    oal_uint32                      bit_al_rx_flag        :3;  /*0: 关闭常收; 1:保留给RF测试；2: ampdu聚合帧常收; 3:非聚合帧常收*/
    oal_uint32                      bit_one_packet_st     :1;  /* 0表示DBC结束 1表示DBAC正在执行 */
    oal_uint32                      bit_clear_fifo_st     :1;  /* 0表示无clear fifo状态，1表示clear fifo状态 */
    oal_uint32                      bit_mac_phy_freq_down :1;  /* 是否开启MAC/PHY降频特性 */
    oal_uint32                      bit_al_txrx_ampdu_num :7;  /*指示用于常发常收的聚合帧数目*/
    oal_uint32                      bit_track_cnt         :8;  /* 遇到qempty 剩余记录条目数 */
    oal_uint32                      bit_track_cnt_down    :8;  /* 剩余记录条目减少标志 */
    oal_netbuf_stru                *pst_altx_netbuf;           /* 记录常发时，描述符所共用的内存 */
    oal_netbuf_stru                *pst_alrx_netbuf;           /* 记录常收时，描述符所共用的内存 */
    oal_uint32                      ul_rx_normal_mdpu_succ_num;
    oal_uint32                      ul_rx_ampdu_succ_num;
    oal_uint32                      ul_tx_ppdu_succ_num;
    oal_uint32                      ul_rx_ppdu_fail_num;
    oal_uint32                      ul_tx_ppdu_fail_num;

    oal_uint8                       uc_reg_info_flag;  /*分两次打印寄存器信息，0:上报soc寄存器的值,1:上报rf寄存器的值,2:上报MAC寄存器的值,3:补充 ABB_CALI_WL_CTRL 信息,4:上报phy寄存器的值*/
    oal_bool_enum_uint8             en_dync_txpower_flag;
    oal_bool_enum_uint8             en_dync_pow_debug_switch;   /* 动态校准维测开关 */
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    oal_uint8                       uc_pkt_cap_flag;
#else
    oal_uint8                       uc_resv[1];
#endif

    oal_dlist_head_stru             ast_rx_isr_info_list[HAL_HW_RX_DSCR_LIST_NUM];
    hal_hw_rx_dscr_info_stru        ast_rx_isr_info_member[HAL_HW_RX_ISR_INFO_MAX_NUMS];
    oal_dlist_head_stru             st_rx_isr_info_res_list;  /* 接收中断信息存储资源链表 */

    oal_uint8                       uc_current_rx_list_index;
    oal_uint8                       uc_current_chan_number;
    wlan_p2p_mode_enum_uint8        en_p2p_mode;                   /* 当前创建的P2P 是处于CL/GO 模式,算法注册中断时调用 */
    oal_uint8                       uc_p2p_hal_vap_idx;            /* 用于P2P 中断产生后指示对应P2P设备的hal vap */

    regdomain_enum_uint8            en_regdomain;
    oal_uint8                       uc_full_phy_freq_user_cnt; //device下需要满频的vap(ap)/sta(user) 个数
    oal_uint8                       uc_over_temp;
#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
    oal_uint8                       uc_psd_status;
    oal_netbuf_stru *               pst_psd_netbuf;                /* 记录psd采集时，Dmac中分配1k内存暂存数据 */
#else
    oal_uint8                       auc_resv[1];
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
    hal_dyn_cali_val_stru           st_dyn_cali_val;
#endif

#ifdef _PRE_DEBUG_MODE
    /* 记录接收描述符队列中，各个描述符的地址 */
    witp_rx_dscr_recd_stru          st_nor_rx_dscr_recd[HAL_NORMAL_RX_MAX_BUFFS];
    witp_rx_dscr_recd_stru          st_hi_rx_dscr_recd[HAL_HIGH_RX_MAX_BUFFS];

    /* 原始描述符物理地址 */
    oal_uint32                      aul_nor_rx_dscr[HAL_NORMAL_RX_MAX_BUFFS];
    oal_uint32                      aul_hi_rx_dscr[HAL_HIGH_RX_MAX_BUFFS];

    oal_uint32                      ul_dpart_save_idx;
    oal_uint32                      ul_rx_irq_loss_cnt;
    hal_rx_dpart_track_stru         ast_dpart_track[HAL_DOWM_PART_RX_TRACK_MEM];

    /* 描述符异常还回统计 */
    oal_uint32                      ul_exception_free;
    oal_uint32                      ul_irq_cnt;

    oal_uint32                      ul_pcie_reg110_timeout_counter;
    oal_uint32                      ul_pcie_read_counter;

    frw_timeout_stru                st_exception_report_timer;
#endif

#ifdef _PRE_WLAN_DFT_REG
    frw_timeout_stru                st_reg_prd_timer;                       /* 读取寄存器周期定时器 */
#endif

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    oal_uint32                      ul_al_tx_thr;
    oal_uint32                      ul_al_tx_num;
#endif

#ifdef _PRE_DEBUG_MODE_USER_TRACK
    oam_user_track_rx_ampdu_stat    st_user_track_rx_ampdu_stat;
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
    hal_dfs_radar_filter_stru       st_dfs_radar_filter;
    oal_uint8                       uc_radar_type;
#endif

    /* hi1103重构出来了rf dev，校准参数全部都从rf dev中获取 */
    /* 功率相关PHY参数*/
    hal_phy_pow_param_stru          st_phy_pow_param;

    /* RTS速率相关参数 */
    wlan_legacy_rate_value_enum_uint8   auc_rts_rate[WLAN_BAND_BUTT][HAL_TX_RATE_MAX_NUM];      /* 两个频段的RTS发送速率 */
    wlan_phy_protocol_enum_uint8        auc_rts_protocol[WLAN_BAND_BUTT][HAL_TX_RATE_MAX_NUM] ; /* 两个频段的RTS协议模式 */

    /* 字节对齐 */
    p_hal_alg_isr_func  pa_hal_alg_isr_func_table[HAL_ISR_TYPE_BUTT][HAL_ALG_ISR_NOTIFY_BUTT];

    p_hal_gap_isr_func  pa_hal_gap_isr_func_table[HAL_ISR_TYPE_BUTT];

    oal_uint8                     *puc_mac_reset_reg;
    oal_uint8                     *puc_phy_reset_reg;

    oal_uint16                     uc_cali_check_hw_status;    /*FEM&PA失效检测*/
    oal_int16                      s_always_rx_rssi;

    mac_channel_stru               st_wifi_channel_status;
    oal_uint32                     ul_rx_rate;

    hal_rssi_stru                  st_rssi;

    oal_int8                       c_rx_last_rssi;             //保存上次rssi
    oal_uint8                      uc_fix_adc_en;        /* 测试用固定频率 */
    oal_uint8                      uc_adc_dac_type;      /* 测试用固定adc dac组合 */
#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
    oal_uint8                      uc_cali_type;
#else
    oal_int8                       auc_recv2;
#endif

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
    hal_tx_excp_info_stru          st_tx_excp_info;
    wlan_chan_ratio_stru            st_chan_ratio;                          /* 信道繁忙度相关统计量 */
#endif
    wlan_scan_chan_stats_stru       st_chan_result;                         /* 扫描时 一个信道的信道测量记录 */

#ifdef _PRE_WLAN_FEATURE_PF_SCH
    oal_uint32                     ul_tx_consumed_airtime;        /* 调度算法使用一次发送的空口时间 */
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
    oal_uint32                     ul_cfg_loss_tx_comp_cnt;              /* 通过配置命令手动丢失发送完成中断 */
#endif

    oal_bool_enum_uint8            en_dfr_enable;                        /* dfr是否enable */
    oal_bool_enum_uint8            en_dfr_hw_reset_enable;               //device侧hw reset的使能开关
    oal_bool_enum_uint8            en_rf_cali_doing;
#ifdef _PRE_WLAN_FEATURE_CSI
    oal_uint8                      uc_csi_status;
#else
    oal_uint8                      auc_rev[1];
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    frw_timeout_stru               st_pcie_err_timer;                      /* 检测pcie err_nonfatal 定时器 */
    oal_uint32                     ul_pcie_err_cnt;                        /* pcie err计数 */
#endif
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_uint32                     ul_rx_buf_too_small_show_counter;
    oal_uint32                     ul_beacon_miss_show_counter;
    oal_uint32                     ul_rx_fsm_st_timeout_show_counter;
    oal_uint32                     ul_tx_fsm_st_timeout_show_counter;
#ifdef _PRE_WLAN_PRODUCT_1151V200
    oal_uint32                     ul_nav_threshold_err_show_counter;
#endif
#endif

#if defined _PRE_WLAN_ONLINE_DPD || defined _PRE_WLAN_1103_ATE
    oal_uint32                     ul_rsv0;
    oal_bool_enum_uint8            en_adc_4t01_flag;   /* ADC使用4路80M合成为320M */
#endif
    hal_dev_fsm_stru               st_hal_dev_fsm;                       /* hal device 状态机结构体 */
    oal_uint32                     ul_work_vap_bitmap;                   /* 需要在work状态工作的vap的个数 */

    hal_m2s_fsm_stru               st_hal_m2s_fsm;                       /* hal device m2s状态机 */

    oal_bool_enum_uint8            en_is_master_hal_device;              /* 是否是master hal device */
    oal_bool_enum_uint8            en_rx_intr_fifo_resume_flag;    /* RX INTR FIFO OVERRUN是否恢复标识 */
    oal_bool_enum_uint8            en_ampdu_tx_hw_en;                     /* AMPDU TX硬化开关 */
    oal_bool_enum_uint8            en_ampdu_partial_resnd;                /* 硬件聚合时,MAC是否做部分帧重传 */

    oal_uint8                      uc_assoc_user_nums;         /* hal device上所有vap上用户数总和，方便统计本hal device上用户数 */
    oal_uint8                      uc_fix_power_level;                    /* 固定功率等级(取值为0, 1, 2, 3) */
    oal_uint8                      uc_mag_mcast_frm_power_level;          /* 管理帧的功率等级 */
    oal_uint8                      uc_control_frm_power_level;            /* 控制帧的功率等级 */
#ifndef _PRE_WLAN_FEATURE_TPC_OPT
    oal_int16                      s_upc_amend;                           /* UPC修正值 */
#else
    oal_uint8                      auc_rsv[2];
#endif
    oal_bool_enum_uint8            en_dpd_cali_doing;
    oal_bool_enum_uint8            en_dpd_clk_on;

    oal_bool_enum_uint8            en_pow_rf_reg_ctl_flag;                /* 功率RF寄存器控使能时，固定功率配置不生效 */
    oal_uint8                      bit_autCG_on : 1,                      /* autoCG开关 */
                                   bit_l0s_on   : 1,                      /* l0s开关 */
                                   bit_l1pm_on  : 1,                      /* l1pm开关 */
                                   bit_resv     : 5;
#ifdef _PRE_WLAN_FEATURE_DBDC
    oal_bool_enum_uint8            en_wait_for_s2m;       /* 是否等待此路释放rf,另一路siso切回mimo状态 */
#else
    oal_uint8                      uc_rsv[1];
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
    oal_bool_enum_uint8            en_m2s_excute_flag;        /* m2s正在执行的标记 */
#else
    oal_uint8                      uc_rsv1[1];
#endif /* #ifdef _PRE_WLAN_FEATURE_M2S */

#ifdef _PRE_WLAN_FEATURE_STA_PM
    oal_uint8                      uc_service_id;                     /* hal device向平台投票的id，初始化时分配并注册到平台 */
    oal_bool_enum_uint8            en_intf_det_invalid;               /* 指示当前的检测结果是否有效 */
    oal_uint8                      auc_rev1[2];
#endif

    oal_void                       *p_alg_priv;                       /* 算法私有结构体 */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) && defined(_PRE_FEATURE_WAVEAPP_CLASSIFY)
    oal_uint32                     ul_rx_mcs_cnt;   /*记录识别仪器场景时的连续MCS计数*/
    oal_uint16                     us_rx_assoc_id;  /*记录识别仪器场景时的11ac用户的assoc id*/
    oal_uint8                      uc_rx_expect_mcs;  /*记录识别仪器场景时的预期用户采用的MCS*/
    oal_bool_enum_uint8            en_test_is_on_waveapp_flag; /*仪器场景标志位*/
#else
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    oal_bool_enum_uint8            en_test_is_on_waveapp_flag;
    oal_uint8                      auc_rev3[3];
#endif
#endif

    hal_device_dft_stat_stru       st_hal_dev_dft_stat;                 /* hal device维测结构体 */

    hal_scan_params_stru           st_hal_scan_params;
    hal_device_sdt_stat_stru       st_hal_dev_sdt_stat;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_work_stru                  st_hal_file_work;
    hal_work_func_id_enum_uint8    en_func_id;
#endif
#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
    oal_int16                       s_rssi_ant0;
    oal_int16                       s_rssi_ant1;
    oal_bool_enum_uint8             en_rssi_update;
    oal_uint8                       auc_resv2[3];
#endif
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
    hal_device_tx_pwr_ctrl_stru    st_tx_pwr_ctrl;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    frw_timeout_stru                    st_btcoex_powersave_timer;
    hal_device_btcoex_sw_preempt_stru   st_btcoex_sw_preempt;
#endif

    oal_uint8                       bit_srb_switch           :1;
    oal_uint8                       bit_dyn_offset_switch    :1;
    oal_uint8                       bit_no_ps_frm_int_switch :1;
    oal_uint8                       bit_pm_switch_resv       :5;

    oal_bool_enum_uint8             en_fix_sifs_resp_rate;   /* 是否限制响应帧速率 */
    oal_uint8                       en_lifetime_chk;
    oal_bool_enum_uint8             en_txbf;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    p_tbtt_ap_isr_func                  p_tbtt_update_beacon_func;

    oal_bool_enum_uint8                 aen_last_over_temperature_flag[WLAN_RF_CHANNEL_NUMS];  /* 上一次查询的过温状态 */
    oal_uint32                          ul_temper_err_cnt;                                     /* 过温err计数 */
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    alg_rx_dscr_ctl_device_info_stru    st_rx_dscr_ctl;
#endif
//#ifdef _PRE_WLAN_FEATURE_FTM
    oal_uint64              ull_t1;
    oal_uint64              ull_t4;
//#endif

    /* ROM化后资源扩展指针 */
    oal_void                           *_rom;
    #ifdef _PRE_WLAN_FEATURE_MWO_DET
    oal_uint32                     ul_tx_comp_mwo_cyc_time; /* 上报微波炉周期寄存器的计数值*/
    #endif

    oal_uint8               uc_already_tx_num;
#if defined(_PRE_WLAN_FEATURE_11AX) || defined(_PRE_WLAN_FEATURE_11AX_ROM)
    oal_bool_enum_uint8     en_current_11ax_working;     /* 当前hal dev是否有工作的11ax vap */
    oal_uint8               uc_trig_downlink_delta_pwr;   /* AP侧到STA侧空口功率损耗 */
    oal_int8                c_trig_frame;              /*  */
#endif
} hal_to_dmac_device_stru;

/* HAL模块和DMAC模块共用的WLAN RX结构体 */
typedef struct
{
    oal_uint32                 *pul_base_dscr;      /* 描述符基地址 */
    oal_uint16                  us_dscr_num;        /* 接收到的描述符的个数 */
    oal_bool_enum_uint8         en_sync_req;        /* 队列同步标识 */
    oal_uint8                   uc_queue_id;        /* 接收队列号 */
    hal_to_dmac_device_stru    *pst_hal_device;
    oal_uint8                   uc_channel_num;
    oal_uint8                   auc_resv[3];
}hal_wlan_rx_event_stru;

#ifdef _PRE_WLAN_FEATURE_FTM
/* HAL模块和DMAC模块共用的FTM TIME RX结构体 */
typedef struct
{
    oal_uint64      ull_t2;
    oal_uint64      ull_t3;
    oal_uint8       uc_dialog_token;
}hal_wlan_ftm_t2t3_rx_event_stru;
#endif

typedef struct
{
    hal_tx_dscr_stru        *pst_base_dscr;/* 发送完成中断硬件所上报的描述符指针 */
    hal_to_dmac_device_stru *pst_hal_device;
    oal_uint8                uc_dscr_num; /* 硬件一次发送所发出的描述符个数 */

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
    oal_int16                s_pdet_val;
    oal_bool_enum_uint8      en_pdet_enable     :1;
    oal_uint8                uc_chain_num       :1;
    oal_bool_enum_uint8      en_invalid         :1;
    oal_bool_enum_uint8      en_pdbuf_opt       :1;
    oal_uint8                uc_resv            :4;
#else
    oal_uint8                auc_resv[3];
#endif

//#ifdef _PRE_WLAN_FEATURE_MWO_DET
    oal_uint32               ul_tx_comp_mwo_cyc_time; /* 上报微波炉周期寄存器的计数值*/
//#endif
#ifdef _PRE_WLAN_FEATURE_PF_SCH
    oal_uint32               ul_tx_consumed_airtime;   /* 调度算法使用一次发送的空口时间 */
#endif
//#ifdef _PRE_WLAN_FEATURE_FTM
    oal_uint64              ull_t1;
    oal_uint64              ull_t4;
//#endif
}hal_tx_complete_event_stru;

typedef struct
{
    hal_error_state_stru            st_error_state;
    hal_to_dmac_device_stru        *pst_hal_device;
}hal_error_irq_event_stru;

typedef struct
{
    oal_uint8                       p2p_noa_status; /* 0: 表示noa 定时器停止，1: 表示noa 定时器正在工作 */
    oal_uint8                       auc_resv[3];
}hal_p2p_pm_event_stru;

/* 硬件常发配置参数结构体 */
typedef struct
{
    union
    {
        struct
        {
            oal_uint32               bit_en         :1;
            oal_uint32               bit_rsv1       :3;
            oal_uint32               bit_mode       :4;
            oal_uint32               bit_content    :8;
            oal_uint32               bit_rsv2       :16;
        } st_ctrl;
        oal_uint32                   ul_ctrl;
    }un_ctrl;

    oal_uint32                       ul_times;
    oal_uint32                       ul_mpdu_len;
    oal_uint32                       ul_ifs;        /* 单位0.1us */
    hal_tx_txop_per_rate_params_union  *pst_rate;
}hal_al_tx_hw_stru;

typedef struct
{
    hal_to_dmac_device_stru        *pst_hal_device;
}hal_common_irq_event_stru;

#ifdef _PRE_WLAN_1103_CHR
typedef struct
{
    oal_uint32 ul_phy_err_rpt;              /*phy错误上报寄存器的值*/
    oal_uint32 ul_rx_desc_status;           /*当前帧的接受状态*/
    oal_uint32 ul_rpt_aver_evm_ant1;        /*平均evm*/
    oal_uint32 ul_rpt_aver_evm_ant0;        /*平均evm*/
    oal_uint32 ul_dotb_err_frm_num;         /*接受到错误的11b帧统计*/
    oal_uint32 ul_ht_err_frm_num;           /*接受到错误的HT帧统计*/
    oal_uint32 ul_vht_err_frm_num;          /*接受到错误的VHT帧统计*/
    oal_uint32 ul_lega_err_frm_num;         /*接受到错误的LEGACY帧统计*/
    oal_uint32 ul_rpt_phase_est;            /*频偏估计角度上报*/
    oal_uint32 ul_rpt_2x2_chan_cond;        /*2*2频偏估计角度上报*/
    oal_uint32 ul_rpt_mac_auto_rst_cnt;     /*mac自动自复位次数统计*/
}hal_phy_mac_chr_info_stru;
#endif
/*****************************************************************************
  4 函数实现
*****************************************************************************/
OAL_STATIC OAL_INLINE oal_uint32*  hal_rx_dscr_get_real_addr(oal_uint32 *pul_rx_dscr)
{
    /* 注意数字2代表hi1102_rx_buffer_addr_stru结构体中的prev指针，移动到next指针位置 */
    if (OAL_PTR_NULL == pul_rx_dscr)
    {
        return pul_rx_dscr;
    }
    return (oal_uint32 *)((oal_uint8 *)pul_rx_dscr + (OAL_SIZEOF(hal_rx_dscr_stru) - 4));
}

OAL_STATIC OAL_INLINE oal_uint32*  hal_rx_dscr_get_sw_addr(oal_uint32 *pul_rx_dscr)
{
    /* 注意数字2代表hi1102_rx_buffer_addr_stru结构体中的prev指针，移动到next指针位置 */
    if (OAL_PTR_NULL == pul_rx_dscr)
    {
        return pul_rx_dscr;
    }
    return (oal_uint32 *)((oal_uint8 *)pul_rx_dscr - (OAL_SIZEOF(hal_rx_dscr_stru) - 4));
}

/*****************************************************************************
  10.2 对外暴露的配置接口
*****************************************************************************/
/************************  1151  ********************************************/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
extern oal_void  hi1151_set_pcie_pm_level(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_pcie_pm_level);
#define HAL_CHIP_LEVEL_FUNC_EXTERN
extern oal_uint32 hal_chip_get_device_num_impl(oal_bus_dev_stru * pst_bus_stru, oal_uint8 * puc_device_nums);
#define HAL_DEVICE_LEVEL_FUNC_EXTERN
extern oal_uint32 hal_chip_get_device_impl(oal_bus_dev_stru * pst_bus_stru, oal_uint8 uc_device_id, hal_to_dmac_device_stru **ppst_device_stru);
extern oal_void hi1151_get_chip_version(hal_to_dmac_chip_stru *pst_hal_chip, oal_uint32 *pul_chip_ver);
extern oal_void hi1151_rx_init_dscr_queue(hal_to_dmac_device_stru * pst_hal_device,oal_uint8 uc_set_hw);
extern oal_void  hi1151_rx_dscr_add_netbuf(hal_to_dmac_device_stru *pst_device, oal_uint32  *pul_ret);
extern oal_void  hi1151_rx_add_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint16 us_rx_buff);
extern oal_void hi1151_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1151_al_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1151_al_rx_init_dscr_queue(hal_to_dmac_device_stru * pst_device);

#ifdef _PRE_WLAN_FEATUER_PCIE_TEST
extern oal_void hi1151_pcie_test_write_burst(hal_to_dmac_device_stru *pst_device, oal_uint16 us_data);
extern oal_void hi1151_pcie_test_rdata_bit(hal_to_dmac_device_stru *pst_device, oal_uint8 ul_data);
extern oal_void hi1151_pcie_test_rdata_addr(hal_to_dmac_device_stru *pst_device, oal_uint32 ul_addr);
extern oal_void hi1151_pcie_test_wdata_bit(hal_to_dmac_device_stru *pst_device, oal_uint8 uc_data);
extern oal_void hi1151_pcie_test_wdata_addr(hal_to_dmac_device_stru *pst_device, oal_uint32 ul_addr);
#endif
extern oal_void hi1151_tx_init_dscr_queue(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1151_tx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1151_init_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1151_free_rx_isr_list(oal_dlist_head_stru  *pst_rx_isr_list);
extern oal_void hi1151_destroy_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1151_get_tx_dscr_queue_total_ppdu_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 *pus_ppdu_cnt);
extern oal_void hi1151_tx_fill_basic_ctrl_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru * p_tx_dscr, hal_tx_mpdu_stru *pst_mpdu);
extern oal_void hi1151_tx_ctrl_dscr_link(hal_tx_dscr_stru *pst_tx_dscr_prev, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1151_get_tx_dscr_next(hal_tx_dscr_stru *pst_tx_dscr, hal_tx_dscr_stru **ppst_tx_dscr_next);
extern oal_void hi1151_tx_ctrl_dscr_unlink(hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1151_tx_seqnum_set_dscr(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 us_seqnum);
extern oal_void hi1151_tx_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1151_tx_non_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1151_tx_set_dscr_modify_mac_header_length(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_mac_header_length);
extern oal_void hi1151_tx_set_dscr_seqno_sw_generate(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_sw_seqno_generate);
extern oal_void hi1151_tx_get_size_dscr(oal_uint8 us_msdu_num, oal_uint32 * pul_dscr_one_size, oal_uint32 * pul_dscr_two_size);
extern oal_void hi1151_tx_get_vap_id(hal_tx_dscr_stru * pst_tx_dscr, oal_uint8 *puc_vap_id);
extern oal_void hi1151_tx_get_dscr_ctrl_one_param(hal_tx_dscr_stru * pst_tx_dscr, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one_param);
extern oal_void hi1151_tx_get_dscr_seq_num(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_seq_num);
extern oal_void  hi1151_tx_get_dscr_tx_cnt(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_tx_count);
extern oal_void hi1151_tx_get_dscr_mpdu_num(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_mpdu_num);
extern oal_void hi1151_tx_set_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_status);
extern oal_void hi1151_tx_get_dscr_chiper_type(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_chiper_type, oal_uint8 *puc_chiper_key_id);
extern oal_void hi1151_tx_get_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_status);
extern oal_void  hi1151_tx_get_dscr_send_rate_rank(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_send_rate_rank);
extern oal_void hi1151_tx_get_dscr_ba_ssn(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_ba_ssn);
extern oal_void hi1151_tx_get_dscr_ba_bitmap(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_ba_bitmap);
extern oal_void hi1151_tx_put_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_queue_type_enum_uint8 en_tx_queue_type, hal_tx_dscr_stru *past_tx_dscr);
#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)
extern oal_void hi1151_set_is_waveapp_test(hal_to_dmac_device_stru * pst_hal_device, oal_bool_enum_uint8 en_is_waveapp_test);
#endif
extern oal_void hi1151_get_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 * pul_status, oal_uint8 uc_qnum);
extern oal_void hi1151_tx_get_ampdu_len(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_ampdu_len);
extern oal_void hi1151_soc_set_pcie_l1s(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);


extern oal_void hi1151_tx_get_protocol_mode(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_protocol_mode);

extern oal_void hi1151_rx_record_frame_status_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_id);
extern oal_void hi1151_rx_get_size_dscr(oal_uint32 * pul_dscr_size);
extern oal_void hi1151_rx_get_info_dscr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_ctl_stru * pst_rx_ctl, hal_rx_status_stru * pst_rx_status, hal_rx_statistic_stru * pst_rx_statistics);
extern oal_void hi1151_get_hal_vap(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_vap_id, hal_to_dmac_vap_stru **ppst_hal_vap);
extern oal_void hi1151_rx_get_netbuffer_addr_dscr(oal_uint32 *pul_rx_dscr, oal_netbuf_stru ** ppul_mac_hdr_addr);
extern oal_void hi1151_rx_get_mac_hdr_addr_dscr(oal_uint32 *pul_rx_dscr, oal_uint32 ** ppul_mac_hdr_address_dscr);
extern oal_void hi1151_rx_show_dscr_queue_info(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_rx_dscr_type);
extern oal_void hi1151_rx_sync_invalid_dscr(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dscr, oal_uint8 en_queue_num);
extern oal_void hi1151_rx_free_dscr_list(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint32 *pul_rx_dscr);
extern oal_void hi1151_dump_rx_dscr(oal_uint32 *pul_rx_dscr);
extern oal_void hi1151_dump_tx_dscr(oal_uint32 *pul_tx_dscr);
extern oal_void hi1151_reg_write(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 ul_val);
extern oal_void hi1151_set_machw_rx_buff_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_void hi1151_resume_mac_rx_dscr_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
//extern oal_uint32 hi1151_set_machw_rx_buff_addr_sync(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_void hi1151_set_machw_tx_suspend(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_machw_tx_resume(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_reset_phy_machw(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type,
                                     oal_uint8 sub_mod,oal_uint8 uc_reset_phy_reg,oal_uint8 uc_reset_mac_reg);
extern oal_void hi1151_disable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_recover_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_initialize_machw(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_initialize_machw_common(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_freq_band(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1151_set_bandwidth_mode(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_bandwidth_enum_uint8 en_bandwidth);

#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
extern oal_void hi1151_set_ant_rssi_report(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_switch);
extern oal_void hi1151_get_ant_rssi_rep_sw(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_switch);
extern oal_void hi1151_get_ant_rssi_value(hal_to_dmac_device_stru *pst_hal_device, oal_int16 *ps_ant0, oal_int16 *ps_ant1);
extern oal_void hi1151_update_ant_rssi_value(hal_to_dmac_device_stru *pst_hal_device, oal_int16 s_ant0, oal_int16 s_ant1);
#endif

extern oal_void hi1151_set_upc_data(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_subband_idx);
extern oal_void hi1151_set_tpc_params(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_channel_num,wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1151_process_phy_freq(hal_to_dmac_device_stru *pst_hal_device);

extern oal_void hi1151_set_primary_channel(
                    hal_to_dmac_device_stru          *pst_hal_device,
                    oal_uint8                         uc_channel_num,
                    wlan_channel_band_enum_uint8      en_band,
                    oal_uint8                         uc_channel_idx,
                    wlan_channel_bandwidth_enum_uint8 en_bandwidth);
#ifdef _PRE_WLAN_HW_TEST
extern oal_void hi1151_set_phy_tx_scale(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_2g_11ac);
#endif

extern oal_void hi1151_freq_adjust(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_pll_int, oal_uint16 us_pll_frac);
extern oal_void hi1151_set_rx_multi_ant(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rf_chain);
extern oal_void  hi1151_add_machw_tx_ba_lut_entry(hal_to_dmac_device_stru* pst_hal_device,
        oal_uint8 uc_lut_index, oal_uint8 uc_tid,
        oal_uint16 uc_seq_no, oal_uint8 uc_win_size, oal_uint8 uc_mmss);
extern oal_void hi1151_add_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device,
                oal_uint8 uc_lut_index, oal_uint8 *puc_dst_addr, oal_uint8 uc_tid,
                oal_uint16 uc_seq_no, oal_uint8 uc_win_size);
extern oal_void hi1151_remove_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void  hi1151_get_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_index,
                                                oal_uint32* ul_addr_h,oal_uint32* ul_addr_l,
                                                oal_uint32* ul_bitmap_h,oal_uint32* ul_bitmap_l,oal_uint32* ul_ba_para);
extern oal_void hi1151_restore_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_index,
                                             oal_uint32 ul_addr_h,oal_uint32 ul_addr_l,oal_uint32 ul_ba_para);
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
extern oal_void  hi1151_restore_machw_ba_params_with_bitmap(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_index,
                                                oal_uint32 ul_addr_h,oal_uint32 ul_addr_l,oal_uint32 ul_ba_para,oal_uint32 ul_bitmap_h,oal_uint32 ul_bitmap_l);
#endif
extern oal_void hi1151_machw_seq_num_index_update_per_tid(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_qos_flag);
extern oal_void hi1151_set_tx_sequence_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag,oal_uint32 ul_val_write,oal_uint8 uc_vap_index);
extern oal_void hi1151_get_tx_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag, oal_uint8 uc_vap_index,oal_uint16 *pst_val_read);
extern oal_void hi1151_reset_init(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_reset_destroy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_reset_reg_restore(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1151_reset_reg_save(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1151_reset_reg_dma_save(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1151_reset_reg_dma_restore(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1151_disable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_disable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_initialize_phy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_radar_config_reg(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_dfs_domain);
extern oal_void hi1151_radar_config_reg_bw(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_radar_type, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1151_radar_enable_chirp_det(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_chirp_det);
extern oal_uint32 hi1151_set_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_th);
extern oal_void hi1151_get_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 *pl_th);
extern oal_void hi1151_trig_dummy_radar(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_radar_type);
extern oal_void hi1151_initialize_rf_sys(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_pow_sw_initialize_tx_power(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_pow_initialize_tx_power(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_pow_set_rf_regctl_enable(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rf_linectl);
extern oal_void  hi1151_pow_set_pow_to_pow_code(hal_to_dmac_device_stru *pst_hal_device,
                                                hal_vap_pow_info_stru *pst_vap_pow_info,
                                                oal_uint8 uc_start_chain, wlan_channel_band_enum_uint8 en_freq_band);
extern oal_void hi1151_pow_set_resp_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_num,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);

extern oal_void hi1151_pow_set_band_spec_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_num,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);
extern oal_void hi1151_pow_get_spec_frame_data_rate_idx(oal_uint8 uc_rate,  oal_uint8 *puc_rate_idx);
extern oal_void hi1151_pow_set_pow_code_idx_same_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 ul_pow_code);
extern oal_void hi1151_pow_set_pow_code_idx_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 *aul_pow_code);
extern oal_void hi1151_pow_get_pow_index(hal_user_pow_info_stru *pst_hal_user_pow_info,
                     oal_uint8 uc_cur_rate_pow_idx, hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint8 *puc_pow_level);
extern oal_void hi1151_pow_set_four_rate_tx_dscr_power(hal_user_pow_info_stru *pst_hal_user_pow_info,
                                oal_uint8 *puc_rate_level_idx, oal_uint8 *pauc_pow_level,
                                hal_tx_txop_tx_power_stru *pst_tx_power);

extern oal_void hi1151_pow_cfg_no_margin_pow_mode(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_pow_mode);
extern oal_void  hi1151_pow_cfg_show_log(hal_to_dmac_device_stru *pst_hal_device, hal_vap_pow_info_stru *pst_vap_pow_info,
                                                                wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_rate_idx);
extern oal_void hi1151_set_rf_custom_reg(hal_to_dmac_device_stru *pst_hal_device);

#ifdef _PRE_WLAN_REALTIME_CALI
extern oal_void hi1151_rf_cali_realtime(hal_to_dmac_device_stru * pst_hal_device);
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_void hi1151_config_set_dyn_cali_dscr_interval(hal_to_dmac_device_stru * pst_hal_device, wlan_channel_band_enum_uint8 uc_band, oal_uint16 us_param_val);
extern oal_void hi1151_rf_cali_realtime_entrance(hal_to_dmac_device_stru * OAL_CONST pst_hal_device, hal_pdet_info_stru * OAL_CONST pst_pdet_info, hal_dyn_cali_usr_record_stru * OAL_CONST pst_user_pow);
extern oal_void hi1151_init_dyn_cali_tx_pow(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_config_get_dyn_cali_dpinit_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_param_val);
#endif
extern oal_void  hi1151_get_hw_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_cali_check_hw_status);

extern oal_void hi1151_initialize_soc(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_get_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);
extern oal_void hi1151_clear_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1151_get_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_state);
extern oal_void hi1151_clear_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_status);
extern oal_void hi1151_unmask_mac_error_init_status(hal_to_dmac_device_stru * pst_hal_device, hal_error_state_stru *pst_status);
extern oal_void hi1151_unmask_mac_init_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1151_show_irq_info(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_param);
extern oal_void hi1151_dump_all_rx_dscr(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_clear_irq_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_get_irq_stat(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 *puc_param, oal_uint32 ul_len, oal_uint8 uc_type);
extern oal_void hi1151_get_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 vap_id, hal_to_dmac_vap_stru ** ppst_hal_vap);
extern oal_void hi1151_add_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 uc_mac_vap_id, hal_to_dmac_vap_stru ** ppst_hal_vap);
extern oal_void hi1151_del_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 vap_id);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_void hi1151_set_proxysta_enable(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable);
#endif
extern oal_void hi1151_config_eifs_time(hal_to_dmac_device_stru *pst_hal_device, wlan_protocol_enum_uint8 en_protocol);
extern oal_void hi1151_register_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                           hal_alg_noify_enum_uint8 en_alg_notify,p_hal_alg_isr_func p_func);
extern oal_void hi1151_unregister_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                             hal_alg_noify_enum_uint8 en_alg_notify);
extern oal_void hi1151_register_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,p_hal_alg_isr_func p_func);
extern oal_void hi1151_unregister_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type);
extern oal_void hi1151_one_packet_start(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_cfg_stru *pst_cfg);
extern oal_void hi1151_one_packet_stop(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_one_packet_get_status(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_status_stru *pst_status);
extern oal_void hi1151_reset_nav_timer(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_clear_hw_fifo(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_mask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1151_unmask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1151_reg_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 *pul_val);
extern oal_void hi1151_get_all_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_val);
extern oal_void hi1151_get_ampdu_bytes(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_tx_bytes, oal_uint32 *pul_rx_bytes);
extern oal_void hi1151_get_rx_err_count(hal_to_dmac_device_stru* pst_hal_device,
                                        oal_uint32* pul_cnt1,
                                        oal_uint32* pul_cnt2,
                                        oal_uint32* pul_cnt3,
                                        oal_uint32* pul_cnt4,
                                        oal_uint32* pul_cnt5,
                                        oal_uint32* pul_cnt6);
extern oal_void hi1151_show_fsm_info(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_mac_error_msg_report(hal_to_dmac_device_stru *pst_hal_device, hal_mac_error_type_enum_uint8 en_error_type);
extern oal_void hi1151_get_dieid(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dieid, oal_uint32 *pul_length);
extern oal_void hi1151_en_soc_intr(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_disable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_non_frame_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_disable_non_frame_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_disable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1151_enable_rx_amsdu_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1151_disable_rx_amsdu_mode(hal_to_dmac_device_stru *pst_hal_device);

extern oal_void hi1151_ce_add_key(hal_to_dmac_device_stru *pst_hal_device,hal_security_key_stru *pst_security_key,oal_uint8 *puc_addr);
extern oal_void hi1151_ce_get_key(hal_to_dmac_device_stru *pst_device,hal_security_key_stru *pst_security_key);
extern oal_void hi1151_disable_ce(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1151_ce_del_key(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key);
extern oal_void hi1151_ce_add_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx,oal_uint8 * puc_addr);
extern oal_void hi1151_ce_del_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx);
extern oal_void hi1151_set_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1151_get_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1151_set_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1151_get_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
#ifdef _PRE_WLAN_INIT_PTK_TX_PN
extern oal_void hi1151_tx_get_dscr_phy_mode_one(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_phy_mode_one);
extern oal_void hi1151_tx_get_ra_lut_index(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_ra_lut_index);
extern oal_void hi1151_init_ptk_tx_pn(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key,oal_uint32 ul_pn_msb);
#endif
extern oal_void hi1151_get_rate_80211g_table(oal_void **pst_rate);
extern oal_void hi1151_get_rate_80211g_num(oal_uint32 *pst_data_num);
extern oal_void hi1151_get_hw_addr(oal_uint8 *puc_addr);
extern oal_void hi1151_enable_ch_statics(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1151_set_ch_statics_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_period);
extern oal_void hi1151_set_ch_measurement_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_period);
extern oal_void hi1151_get_ch_statics_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1151_get_ch_measurement_result_ram(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1151_get_ch_measurement_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1151_enable_radar_det(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1151_get_radar_det_result(hal_to_dmac_device_stru *pst_hal_device, hal_radar_det_event_stru *pst_radar_info);
extern oal_void hi1151_update_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1151_set_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, hal_cfg_rts_tx_param_stru *pst_hal_rts_tx_param);
extern oal_void hi1151_set_rts_rate_selection_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rts_rate_select_mode);
extern oal_void hi1151_set_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_agc_track_ant_sel);
extern oal_void hi1151_get_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_agc_track_ant_sel);
#ifdef _PRE_WLAN_PRODUCT_1151V200
extern oal_void hi1151_get_rf_temp_tsens(hal_to_dmac_device_stru *pst_hal_device, oal_int16 *ps_cur_temp);
extern oal_void hi1151_disable_direct_mgmt_filter(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1151_get_rf_temp(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_cur_temp);
extern oal_void hi1151_set_pow_init_rate_dac_lpf_table(oal_uint8 *pauc_rate_pow_table_2G, oal_uint8 *pauc_rate_pow_table_5G,
                        oal_uint8 *pauc_mode_len, oal_uint8 uc_pow_mode);
extern oal_void hi1151_get_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
        oal_uint16 *pus_dac_val, oal_uint16 *pus_pa_val, oal_uint16 *pus_lpf_val,
        oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void hi1151_set_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
        oal_uint16 us_dac_val, oal_uint16 us_pa_val, oal_uint16 us_lpf_val,
        oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void  hi1151_set_pow_phy_reg_param(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1151_set_dac_lpc_pa_upc_level(oal_uint8 uc_dac_lpf_code, oal_int16  *pas_tpc_level_table,
        oal_uint8 uc_tpc_level_num, oal_uint8 *pauc_dac_lpf_pa_code_table, oal_int16 *pas_upc_gain_table, oal_int16 *pas_other_gain_table,wlan_channel_band_enum_uint8 en_freq_band);
extern oal_void hi1151_set_bcn_phy_tx_mode(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_pow_code);
extern oal_void  hi1151_get_spec_frm_rate(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_spec_frm_phy_tx_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_subband_idx);
extern oal_void hi1151_get_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 *pul_phy_tx_up_down_time_reg,  oal_uint32 *pul_phy_rx_up_down_time_reg,
            oal_uint32 *pul_rf_reg_wr_delay1, oal_uint32 *pul_rf_reg_wr_delay2);
extern oal_void hi1151_set_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 ul_phy_tx_up_down_time_reg,  oal_uint32 ul_phy_rx_up_down_time_reg,
            oal_uint32 ul_rf_reg_wr_delay1, oal_uint32 ul_rf_reg_wr_delay2);
extern oal_void  hi1151_set_resp_pow_level(hal_to_dmac_device_stru *pst_hal_device,
                   oal_int8 c_near_distance_rssi, oal_int8 c_far_distance_rssi);
extern oal_uint32 hi1151_get_subband_index(wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_channel_num, oal_uint8 *puc_subband_idx);
#ifdef _PRE_WLAN_REALTIME_CALI
extern oal_void  hi1151_get_tpc_cali_upc_gain_in_rf_list(oal_int8* pac_upc_gain_in_rf_list);
#endif
extern oal_void hi1151_rf_regctl_enable_set_regs(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_band_enum_uint8 en_freq_band,oal_uint8 uc_cur_ch_num,wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1151_irq_affinity_init(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_core_id);
extern oal_void hi1151_get_bcn_rate(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_data_rate);
#ifdef _PRE_WLAN_FEATURE_TXBF
#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
extern oal_void hi1151_get_fake_vap_id(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_fake_vap_id);
extern oal_void hi1151_set_fake_vap(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_vap_id);
extern oal_void hi1151_clr_fake_vap(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_vap_id);
#if (_PRE_WLAN_REAL_CHIP == _PRE_WLAN_CHIP_SIM)
extern oal_void hi1151_set_2g_rf_txdriver(hal_to_dmac_device_stru *pst_hal_device);
#endif
#endif
extern oal_void hi1151_set_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_matrix);
extern oal_void hi1151_get_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_matrix);
extern oal_void hi1151_set_vht_report_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1151_set_vht_report_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1151_set_ndp_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1151_set_ndp_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1151_set_ndp_max_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_ndp_time);
extern oal_void hi1151_set_ndpa_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_ndpa_duration);
extern oal_void hi1151_set_ndp_group_id(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_group_id, oal_uint16 us_partial_id);
extern oal_void hi1151_set_phy_legacy_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_set_phy_txbf_legacy_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_set_phy_pilot_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_set_ht_buffer_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1151_set_ht_buffer_step(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_reg_value);
extern oal_void hi1151_set_ht_buffer_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_delete_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1151_set_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32 ul_reg_value);
extern oal_void hi1151_get_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);
extern oal_void hi1151_set_h_matrix_timeout(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1151_set_sta_membership_status_63_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 value);
extern oal_void hi1151_set_sta_membership_status_31_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1151_set_sta_user_p_63_48(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1151_set_sta_user_p_47_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1151_set_sta_user_p_31_16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1151_set_sta_user_p_15_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1151_set_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable);
extern oal_void hi1151_get_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_enable);
extern oal_void hi1151_set_mu_aid_matrix_info(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_aid);
extern oal_void hi1151_set_bfee_sounding_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_txbf_protocol_type, oal_uint8 uc_bfee_enable);
extern oal_void hi1151_set_bfee_h2v_beamforming_ng(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_user_bw);
extern oal_void hi1151_set_bfee_grouping_codebook(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_min_group, oal_uint8 uc_txbf_mode,oal_uint8 en_user_bw);
extern oal_void hi1151_set_txbf_vht_buff_addr(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_addr, oal_uint16 us_buffer_len);
extern oal_void hi1151_set_bfer_subcarrier_ng(hal_to_dmac_device_stru *pst_hal_device, wlan_bw_cap_enum_uint8 en_user_bw);
#endif /* _PRE_WLAN_FEATURE_TXBF */

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
extern oal_void hi1151_enable_tx_comp(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1151_enable_smart_antenna_gpio_set_default_antenna(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_delete_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1151_set_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint16 ul_reg_value);
extern oal_void hi1151_get_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);

/* 设置phy_debug trailer上报MAC寄存器 */
extern oal_void hi1151_rx_set_trlr_report_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *auc_config_val, oal_uint8 uc_trlr_switch);
#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
extern oal_void hi1151_set_weak_intf_rssi_th(hal_to_dmac_device_stru *pst_device, oal_int32 l_reg_val);
extern oal_void hi1151_set_agc_unlock_min_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_tx_reg_val, oal_int32 l_rx_reg_val);
extern oal_void hi1151_set_nav_max_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_bss_dur, oal_uint16 us_obss_dur);
#endif
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT
extern oal_void hi1151_set_counter1_clear(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1151_get_txrx_frame_time(hal_to_dmac_device_stru *pst_hal_device, hal_ch_mac_statics_stru *pst_ch_statics);
extern oal_void hi1151_set_mac_clken(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_wctrl_enable);
#endif
extern oal_void hi1151_get_mac_statistics_data(hal_to_dmac_device_stru *pst_hal_device, hal_mac_key_statis_info_stru *pst_mac_key_statis);
extern oal_void hi1151_set_80m_resp_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_debug_en);

#ifdef _PRE_WLAN_FEATURE_CCA_OPT
extern oal_void hi1151_set_ed_high_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_ed_high_20_reg_val, oal_int32 l_ed_high_40_reg_val, oal_bool_enum_uint8 en_is_default_th);
extern oal_void hi1151_enable_sync_error_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_cnt_reg_val);
extern oal_void hi1151_get_sync_error_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_reg_val);
extern oal_void hi1151_set_sync_err_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_get_cca_reg_th(hal_to_dmac_device_stru *pst_hal_device, oal_int8 *ac_reg_val);

#endif
extern oal_void hi1151_set_soc_lpm(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_soc_set_enum_uint8 en_type ,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);
extern oal_void hi1151_set_psm_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1151_set_psm_wakeup_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_mode);
extern oal_void  hi1151_set_psm_listen_interval_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval_count);
extern oal_void  hi1151_set_psm_listen_interval(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval);
extern oal_void  hi1151_set_psm_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1151_set_psm_ext_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);

#if defined(_PRE_WLAN_FEATURE_SMPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1151_set_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 en_smps_mode);
extern oal_void hi1151_get_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
#endif
#if defined(_PRE_WLAN_FEATURE_TXOPPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1151_set_txop_ps_enable(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1151_set_txop_ps_condition1(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1151_set_txop_ps_condition2(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1151_set_txop_ps_partial_aid(hal_to_dmac_vap_stru  *pst_hal_vap, oal_uint16 us_partial_aid);
#endif
extern oal_void hi1151_set_wow_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_set_bitmap,hal_wow_param_stru* pst_para);
extern oal_void hi1151_set_lpm_state(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_state_enum_uint8 uc_state_from, hal_lpm_state_enum_uint8 uc_state_to,oal_void* pst_para, oal_void* pst_wow_para);
extern oal_void hi1151_disable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_enable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_tx_abort_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_abort_en);
extern oal_void hi1151_set_coex_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_mac_ctrl, oal_uint32 ul_rf_ctrl);
extern oal_void hi1151_get_hw_version(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_hw_vsn, oal_uint32 *pul_hw_vsn_data,oal_uint32 *pul_hw_vsn_num);
extern oal_void hi1151_psm_rf_sleep (hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_restore_reg);
extern oal_void hi1151_psm_rf_awake (hal_to_dmac_device_stru  *pst_hal_device,oal_uint8 uc_restore_reg);
extern oal_void hi1151_read_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_reg_addr, oal_uint16 *pus_reg_val);
extern oal_void hi1151_write_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16  us_rf_addr_offset, oal_uint16 us_rf_16bit_data);

#ifdef _PRE_DEBUG_MODE
extern oal_void hi1151_get_all_reg_value(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_get_cali_data(hal_to_dmac_device_stru * pst_hal_device);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_void hi1151_get_cali_data_work(hal_to_dmac_device_stru * pst_hal_device);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
extern oal_void hi1151_get_vap_diag_info(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_out_info);
#endif
extern oal_void hi1151_set_tx_dscr_field(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_data, hal_rf_test_sect_enum_uint8 en_sect);
extern oal_void hi1151_get_tx_dscr_field(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1151_rf_test_disable_al_tx(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_rf_test_enable_al_tx(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr);
#ifdef _PRE_WLAN_PHY_PLL_DIV
extern oal_void hi1151_rf_set_freq_skew(oal_uint16 us_idx, oal_uint16 us_chn, oal_int16 as_corr_data[]);
#endif
extern oal_void hi1151_rf_adjust_ppm(hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_ppm, wlan_channel_bandwidth_enum_uint8  en_bandwidth, oal_uint8 uc_clock);
extern oal_void hi1151_set_daq_mac_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pul_addr, oal_uint16 us_unit_len, oal_uint16 us_unit_num, oal_uint16 us_depth);
extern oal_void hi1151_set_daq_phy_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1151_set_daq_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1151_get_daq_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_value);
extern oal_void hi1151_set_dac_lpf_gain(hal_to_dmac_device_stru *pst_hal_device,
                                    oal_uint8 en_band, oal_uint8 en_bandwidth,oal_uint8 uc_chan_number,oal_uint8 en_protocol_mode,oal_uint8 en_rate);
extern oal_void  hi1151_radar_get_pulse_info(hal_to_dmac_device_stru *pst_hal_device, hal_radar_pulse_info_stru *pst_pulse_info);
extern oal_void  hi1151_radar_clean_pulse_buf(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_set_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_val);
extern oal_void hi1151_set_rx_filter_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_command);
extern oal_void hi1151_get_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void  hi1151_set_beacon_timeout_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value);
extern oal_void  hi1151_set_psm_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_period);
extern oal_void  hi1151_psm_clear_mac_rx_isr(hal_to_dmac_device_stru *pst_hal_device);

#define HAL_VAP_LEVEL_FUNC_EXTERN
extern oal_void hi1151_vap_tsf_get_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_lo);
extern oal_void hi1151_vap_tsf_set_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_lo);
extern oal_void hi1151_vap_tsf_get_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_hi, oal_uint32 *pul_tsf_lo);
extern oal_void hi1151_vap_tsf_set_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_hi, oal_uint32 ul_tsf_lo);
extern oal_void hi1151_vap_send_beacon_pkt(hal_to_dmac_vap_stru *pst_hal_vap, hal_beacon_tx_params_stru *pst_params);
extern oal_void hi1151_vap_set_beacon_rate(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_rate);
extern oal_void hi1151_vap_beacon_suspend(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1151_vap_beacon_resume(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1151_vap_set_machw_prot_params(hal_to_dmac_vap_stru *pst_hal_vap, hal_tx_txop_rate_params_stru *pst_phy_tx_mode, hal_tx_txop_per_rate_params_union *pst_data_rate);
extern oal_void hi1151_vap_set_macaddr(hal_to_dmac_vap_stru * pst_hal_vap, oal_uint8 *puc_mac_addr);
extern oal_void hi1151_vap_set_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);

extern oal_void hi1151_vap_clr_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);
extern oal_void hi1151_vap_set_machw_aifsn_all_ac(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               uc_bk,
                oal_uint8               uc_be,
                oal_uint8               uc_vi,
                oal_uint8               uc_vo);
extern oal_void hi1151_vap_set_machw_aifsn_ac(hal_to_dmac_vap_stru         *pst_hal_vap,
                                            wlan_wme_ac_type_enum_uint8   en_ac,
                                            oal_uint8                     uc_aifs);
extern oal_void hi1151_vap_set_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin, oal_uint8 uc_ac_type);
extern oal_void hi1151_vap_get_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin, oal_uint8 uc_ac_type);
#if 0
extern oal_void hi1151_vap_set_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1151_vap_get_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1151_vap_set_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1151_vap_get_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1151_vap_set_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1151_vap_get_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1151_vap_set_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1151_vap_get_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
#endif
extern oal_void hi1151_vap_set_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);
extern oal_void hi1151_vap_get_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1151_vap_set_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1151_vap_get_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1151_vap_set_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);
extern oal_void hi1151_vap_get_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1151_vap_set_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1151_vap_get_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1151_vap_set_machw_prng_seed_val_all_ac(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1151_vap_read_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_value);
extern oal_void hi1151_vap_write_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_value);
extern oal_void hi1151_vap_set_machw_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void hi1151_vap_update_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void hi1151_vap_get_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_beacon_period);
extern oal_void  hi1151_vap_set_noa(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint32              ul_start_tsf,
                oal_uint32              ul_duration,
                oal_uint32              ul_interval,
                oal_uint8               uc_count);
#ifdef _PRE_WLAN_FEATURE_P2P
extern oal_void  hi1151_vap_set_ops(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               en_ops_ctrl,
                oal_uint8               uc_ct_window);
extern oal_void hi1151_vap_enable_p2p_absent_suspend(
                hal_to_dmac_vap_stru * pst_hal_vap,
                oal_bool_enum_uint8 en_suspend_enable);
#endif
extern oal_void hi1151_set_sta_bssid(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_byte);
extern oal_void  hi1151_set_psm_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_period,
                                                oal_uint8 uc_listen_intvl_to_dtim_times, oal_bool_enum_uint8 en_receive_dtim);
extern oal_void hi1151_set_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_period);
extern oal_void hi1151_get_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_dtim_period);

extern oal_void  hi1151_get_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *uc_dtim_count);
extern oal_void  hi1151_set_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_count);
extern oal_void hi1151_set_sta_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_count);
extern oal_void hi1151_pm_wlan_servid_register(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_pm_wlan_servid_unregister(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_disable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1151_enable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap, oal_bool_enum_uint8 en_dbac_enable);
extern oal_void hi1151_tx_get_dscr_iv_word(hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_iv_ms_word, oal_uint32 *pul_iv_ls_word, oal_uint8 uc_chiper_type, oal_uint8 uc_chiper_keyid);
extern oal_void hi1151_mwo_det_enable_mac_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_reg_val);
extern oal_void  hi1151_tx_enable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void  hi1151_tx_disable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void  hi1151_get_beacon_miss_status(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1151_cfg_slottime_type(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_slottime_type);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
extern oal_void hi1151_set_2g_txrx_path(hal_to_dmac_device_stru *pst_hal_device,
                                 oal_uint8 uc_channel_idx, wlan_channel_bandwidth_enum_uint8 en_bandwidth,
                                 oal_uint8 uc_2g_path);
extern oal_void hi1151_set_txrx_chain(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1151_set_prot_resp_frame_chain(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_chain_val);
extern oal_void hi1151_cfg_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1151_get_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1151_revert_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_band_enum_uint8 en_band);
#ifdef _PRE_WLAN_PRODUCT_1151V200
extern oal_void hi1151_set_peer_resp_dis(hal_to_dmac_device_stru * pst_hal_device, hal_peer_resp_dis_cfg_stru *pst_peer_resp_dis_cfg);
extern oal_void hi1151_get_peer_resp_dis(hal_to_dmac_device_stru * pst_hal_device, hal_peer_resp_dis_cfg_stru *pst_peer_resp_dis_cfg);
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_PRODUCT_1151V200
extern oal_void  hi1151_config_adc_target(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint32 value);
#endif
#endif

extern oal_void hi1151_check_test_value_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value, oal_uint32 *pul_result);
extern oal_void hi1151_config_always_rx(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint8 uc_switch);
extern oal_void hi1151_get_cali_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_param);
/* 用于产线中的烧片版本设置常收模式 */
extern oal_void hi1151_config_always_rx_new(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint8 uc_switch);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_void hi1151_load_ini_power_gain(oal_void);
extern oal_void hi1151_config_update_scaling_reg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint16* paus_dbb_scale);
extern oal_void hi1151_config_update_dsss_scaling_reg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint16* paus_dbb_scale, oal_uint8  uc_distance);
extern oal_uint32 hi1151_config_custom_rf(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_param);
extern oal_uint32 hi1151_config_custom_dts_cali(oal_uint8 * puc_param);
extern oal_void hi1151_config_set_cus_nvram_params(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 * puc_param);
extern oal_void hi1151_config_get_cus_nvram_params(hal_cfg_custom_nvram_params_stru **ppst_cfg_nvram);
extern oal_void hi1151_config_get_cus_cca_param(hal_cfg_custom_cca_stru **ppst_cfg_cca);
extern oal_void hi1151_config_get_far_dist_dsss_scale_promote_switch(oal_uint8 *puc_switch);
extern oal_void hi1151_config_update_rate_pow_table(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1151_rf_cali_custom_info(oal_void);
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_uint32 hi1151_config_custom_dyn_cali(oal_uint8 *puc_param);
#endif
#endif
extern oal_void hi1151_get_rate_idx_pow(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_pow_idx,
                                                 wlan_channel_band_enum_uint8 en_freq_band, oal_uint16 *pus_powr, oal_uint8 uc_channel_idx);
extern oal_void hi1151_get_target_tx_power_by_tx_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one,
                                                                   hal_pdet_info_stru *pst_pdet_info, oal_int16 *ps_tx_pow);
extern oal_uint32 hi1151_rf_get_pll_div_idx(wlan_channel_band_enum_uint8        en_band,
                                               oal_uint8                           uc_channel_idx,
                                               wlan_channel_bandwidth_enum_uint8   en_bandwidth,
                                               oal_uint8                           *puc_pll_div_idx);
extern oal_void  hi1151_vap_get_gtk_rx_lut_idx(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_lut_idx);
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
extern oal_void  hi1151_set_cali_power(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_ch, oal_uint8 uc_freq, oal_int16 *ps_power, wlan_band_cap_enum_uint8 en_band_cap, oal_uint32 *pul_ret);
extern oal_void  hi1151_get_cali_power(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_ch, oal_uint8 uc_freq, oal_int16 *ps_power, wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_set_polynomial_param(hal_to_dmac_device_stru * pst_hal_device, oal_int16 *ps_polynomial_para, oal_uint8 uc_freq, wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_get_polynomial_params(hal_to_dmac_device_stru * pst_hal_device, oal_int16 *ps_polynomial, oal_int32 *pl_length, wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_get_all_cali_power(hal_to_dmac_device_stru * pst_hal_device, oal_int16 *ps_polynomial, oal_int32 *pl_length, wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_get_upc_params(hal_to_dmac_device_stru * pst_hal_device, oal_uint16 *pus_polynomial, oal_uint32 *pul_length, wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_set_upc_params(hal_to_dmac_device_stru * pst_hal_device, oal_uint16 *pus_upc_param,  wlan_band_cap_enum_uint8 en_band_cap);
extern oal_void  hi1151_set_dyn_cali_pow_offset(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_load_mode);
#endif
#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
extern oal_void hi1151_rf_cali_set_vref(wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chain_idx,
                                    oal_uint8  uc_band_idx, oal_uint16 us_vref_value);
extern oal_void hi1151_rf_auto_cali(hal_to_dmac_device_stru * pst_hal_device);
#endif
extern oal_void  hi1151_al_tx_hw(hal_to_dmac_device_stru *pst_hal_device_base, hal_al_tx_hw_stru *pst_al_tx_hw);
extern oal_void  hi1151_al_tx_hw_cfg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint32 ul_mode, oal_uint32 ul_rate);
extern oal_uint32 hi1151_dbb_scaling_amend(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_offset_addr_a, oal_uint8 uc_offset_addr_b, oal_uint16 us_delta_gain);

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
extern oal_void hi1151_packet_capture_write_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_circle_buf_start, oal_uint16 us_circle_buf_depth);
extern oal_void hi1151_packet_capture_switch_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_capture_switch);
extern oal_void hi1151_get_bcn_info(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_bcn_rate, oal_uint32 *pul_phy_tx_mode);
extern oal_void hi1151_tx_get_dscr_phy_tx_mode_param( hal_tx_dscr_stru * pst_tx_dscr, hal_tx_txop_rate_params_stru *pst_phy_tx_mode_param);
#endif

#ifdef _PRE_WLAN_DFT_REG
extern oal_void hi1151_debug_refresh_reg_ext(hal_to_dmac_device_stru *pst_hal_device, oam_reg_evt_enum_uint32 en_evt_type, oal_uint32 *pul_ret);
extern oal_void hi1151_debug_frw_evt(hal_to_dmac_device_stru *pst_hal_device);
#endif

extern oal_void hi1151_get_wow_enable_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);

#ifdef _PRE_WLAN_CHIP_TEST
extern oal_void hi1151_set_tx_dscr_long_nav_enable(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_en_status);
#endif

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
extern oal_void hi1151_get_tx_msdu_address_params(hal_tx_dscr_stru *pst_dscr, hal_tx_msdu_address_params **ppst_tx_dscr_msdu_subtable, oal_uint8 *puc_msdu_num);
#endif
extern oal_void hi1151_show_wow_state_info(hal_to_dmac_device_stru *pst_hal_device);


#ifdef _PRE_WLAN_DFT_STAT
extern oal_void hi1151_dft_get_extlna_gain(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_extlna_gain0_cfg, oal_uint32 *pul_extlna_gain1_cfg);
extern oal_void hi1151_dft_get_chan_stat_result(hal_to_dmac_device_stru *pst_hal_device, oam_stats_dbb_env_param_stru *pst_dbb_env_param);
extern oal_void hi1151_dft_enable_mac_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1151_dft_get_power0_ref(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_val);
extern oal_void hi1151_dft_get_phy_pin_code_rpt(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_val);
extern oal_void hi1151_dft_get_machw_stat_info_ext(hal_to_dmac_device_stru *pst_hal_device, oam_stats_machw_stat_stru *pst_machw_stat);
extern oal_void hi1151_dft_get_beacon_miss_stat_info(hal_to_dmac_device_stru *pst_hal_device, oam_stats_dbb_env_param_stru *pst_dbb_env_param);
extern oal_void hi1151_dft_set_phy_stat_node(hal_to_dmac_device_stru * pst_hal_device, oam_stats_phy_node_idx_stru *pst_phy_node_idx);
extern oal_void hi1151_get_phy_stat_info(hal_to_dmac_device_stru *pst_hal_device, oam_stats_phy_stat_stru *pst_phy_stat);
extern oal_void hi1151_set_counter_clear_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_val);
extern oal_void hi1151_get_tx_hi_pri_mpdu_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_cnt);
extern oal_void hi1151_get_tx_bcn_count(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_cnt);
extern oal_void hi1151_flush_tx_complete_irq(hal_to_dmac_device_stru *pst_hal_dev);

#endif

extern oal_void  hi1151_device_init_vap_pow_code(hal_to_dmac_device_stru   *pst_hal_device,
                                            hal_vap_pow_info_stru            *pst_vap_pow_info,
                                            oal_uint8                         uc_cur_ch_num,
                                            wlan_channel_band_enum_uint8      en_freq_band,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth,
                                            hal_pow_set_type_enum_uint8       uc_type);

extern oal_void hi1151_device_get_tx_pow_from_rate_idx(hal_to_dmac_device_stru * pst_hal_device, hal_user_pow_info_stru *pst_hal_user_pow_info,
                                wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_cur_ch_num, oal_uint8 uc_cur_rate_pow_idx,
                                hal_tx_txop_tx_power_stru *pst_tx_power, oal_int16 *ps_tx_pow);

#elif ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST))

/************************  1102  CHIP********************************************/
#define HAL_CHIP_LEVEL_FUNC_EXTERN
extern oal_void hi1102_get_chip_version(hal_to_dmac_chip_stru *pst_hal_chip, oal_uint32 *pul_chip_ver);
/************************  1102  DEVICE********************************************/
#define HAL_DEVICE_LEVEL_FUNC_EXTERN
extern oal_void hi1102_rx_init_dscr_queue(hal_to_dmac_device_stru *pst_device,oal_uint8 uc_set_hw);
extern oal_void hi1102_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1102_al_rx_init_dscr_queue(hal_to_dmac_device_stru * pst_device);

extern oal_void hi1102_al_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1102_tx_init_dscr_queue(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1102_tx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1102_init_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1102_free_rx_isr_list(oal_dlist_head_stru  *pst_rx_isr_list);
extern oal_void hi1102_destroy_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);

extern oal_void hi1102_tx_fill_basic_ctrl_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru * p_tx_dscr, hal_tx_mpdu_stru *pst_mpdu);
extern oal_void hi1102_tx_ctrl_dscr_link(hal_tx_dscr_stru *pst_tx_dscr_prev, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1102_get_tx_dscr_next(hal_tx_dscr_stru *pst_tx_dscr, hal_tx_dscr_stru **ppst_tx_dscr_next);
extern oal_void hi1102_tx_ctrl_dscr_unlink(hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1102_tx_seqnum_set_dscr(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 us_seqnum);
extern oal_void hi1102_tx_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1102_tx_non_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1102_tx_set_dscr_modify_mac_header_length(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_mac_header_length);
extern oal_void hi1102_tx_set_dscr_seqno_sw_generate(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_sw_seqno_generate);
extern oal_void hi1102_tx_get_size_dscr(oal_uint8 us_msdu_num, oal_uint32 * pul_dscr_one_size, oal_uint32 * pul_dscr_two_size);
extern oal_void hi1102_tx_get_vap_id(hal_tx_dscr_stru * pst_tx_dscr, oal_uint8 *puc_vap_id);
extern oal_void hi1102_tx_get_dscr_ctrl_one_param(hal_tx_dscr_stru * pst_tx_dscr, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one_param);
extern oal_void hi1102_tx_get_dscr_seq_num(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_seq_num);
extern oal_void hi1102_tx_get_dscr_tx_cnt(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_tx_count);
extern oal_void hi1102_tx_dscr_get_rate3(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_rate);
extern oal_void hi1102_tx_set_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_status);
extern oal_void hi1102_tx_get_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_status);
extern oal_void  hi1102_tx_get_dscr_send_rate_rank(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_send_rate_rank);
extern oal_void hi1102_tx_get_dscr_chiper_type(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_chiper_type, oal_uint8 *puc_chiper_key_id);
extern oal_void hi1102_tx_get_dscr_ba_ssn(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_ba_ssn);
extern oal_void hi1102_tx_get_dscr_ba_bitmap(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_ba_bitmap);
extern oal_void hi1102_tx_put_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_queue_type_enum_uint8 en_tx_queue_type, hal_tx_dscr_stru *past_tx_dscr);
extern oal_void hi1102_get_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 * pul_status, oal_uint8 uc_qnum);
extern oal_void hi1102_tx_get_ampdu_len(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_ampdu_len);

extern oal_void hi1102_tx_get_protocol_mode(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_protocol_mode);

extern oal_void hi1102_rx_get_info_dscr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_ctl_stru * pst_rx_ctl, hal_rx_status_stru * pst_rx_status, hal_rx_statistic_stru * pst_rx_statistics);
extern oal_void hi1102_get_hal_vap(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_vap_id, hal_to_dmac_vap_stru **ppst_hal_vap);
extern oal_void hi1102_rx_get_netbuffer_addr_dscr(oal_uint32 *pul_rx_dscr, oal_netbuf_stru ** ppul_mac_hdr_addr);
extern oal_void hi1102_rx_show_dscr_queue_info(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_rx_dscr_type);
extern oal_void hi1102_rx_print_phy_debug_info(hal_to_dmac_device_stru *pst_hal_device,oal_uint32 *pul_rx_dscr, hal_rx_statistic_stru *pst_rx_statistics);
extern oal_void hi1102_rx_record_frame_status_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_id);
//extern oal_void hi1102_rx_sync_invalid_dscr(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dscr, oal_uint8 en_queue_num);
extern oal_void hi1102_rx_free_dscr_list(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint32 *pul_rx_dscr);
extern oal_void hi1102_dump_tx_dscr(oal_uint32 *pul_tx_dscr);
extern oal_void hi1102_reg_write(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 ul_val);
extern oal_void hi1102_reg_write16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint16 us_val);
extern oal_void hi1102_set_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_set_machw_rx_buff_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_uint32 hi1102_set_machw_rx_buff_addr_sync(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_void  hi1102_rx_add_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint16 us_rx_dscr_num);
extern oal_void hi1102_set_machw_tx_suspend(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_set_machw_tx_resume(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_reset_phy_machw(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type,
                                     oal_uint8 sub_mod,oal_uint8 uc_reset_phy_reg,oal_uint8 uc_reset_mac_reg);
extern oal_void hi1102_disable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1102_recover_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_initialize_machw(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_initialize_machw_common(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_set_freq_band(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1102_set_bandwidth_mode(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1102_set_upc_data(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_subband_idx);
extern oal_void hi1102_set_tpc_params(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_channel_num);
extern oal_void hi1102_process_phy_freq(hal_to_dmac_device_stru *pst_hal_device);

extern oal_void hi1102_set_primary_channel(
                    hal_to_dmac_device_stru          *pst_hal_device,
                    oal_uint8                         uc_channel_num,
                    wlan_channel_band_enum_uint8      en_band,
                    oal_uint8                         uc_channel_idx,
                    wlan_channel_bandwidth_enum_uint8 en_bandwidth);

#ifdef _PRE_WLAN_PHY_PERFORMANCE
extern oal_void hi1102_set_phy_tx_scale(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_2g_11ac);
#endif

extern oal_void hi1102_set_rx_multi_ant(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_phy_chain);
extern oal_void  hi1102_add_machw_tx_ba_lut_entry(hal_to_dmac_device_stru* pst_hal_device,
        oal_uint8 uc_lut_index, oal_uint8 uc_tid,
        oal_uint16 uc_seq_no, oal_uint8 uc_win_size, oal_uint8 uc_mmss);
extern oal_void hi1102_add_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device,
                oal_uint8 uc_lut_index, oal_uint8 *puc_dst_addr, oal_uint8 uc_tid,
                oal_uint16 uc_seq_no, oal_uint8 uc_win_size);
extern oal_void hi1102_remove_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1102_get_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_index,
                        oal_uint32* pst_addr_h,oal_uint32* pst_addr_l,oal_uint32* pst_bitmap_h,oal_uint32* pst_bitmap_l,oal_uint32* pst_ba_para);
extern oal_void hi1102_restore_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_index,
                                             oal_uint32 ul_addr_h,oal_uint32 ul_addr_l,oal_uint32 ul_ba_para);
extern oal_void hi1102_machw_seq_num_index_update_per_tid(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_qos_flag);
extern oal_void hi1102_set_tx_sequence_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag,oal_uint32 ul_val_write,oal_uint8 uc_vap_index);
extern oal_void hi1102_get_tx_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag, oal_uint8 uc_vap_index,oal_uint16 *pst_val_read);
extern oal_void hi1102_reset_init(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_reset_destroy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_reset_reg_restore(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1102_reset_reg_save(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1102_reset_reg_dma_save(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1102_reset_reg_dma_restore(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1102_disable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_disable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_initialize_phy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_radar_config_reg(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_dfs_domain);
extern oal_void hi1102_radar_config_reg_bw(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_radar_type, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1102_radar_enable_chirp_det(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_chirp_det);
extern oal_void hi1102_radar_get_pulse_info(hal_to_dmac_device_stru *pst_hal_device, hal_radar_pulse_info_stru *pst_pulse_info);
extern oal_void hi1102_radar_clean_pulse_buf(hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32 hi1102_set_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_th);
extern oal_void hi1102_get_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 *pl_th);
extern oal_void hi1102_trig_dummy_radar(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_radar_type);
extern oal_void hi1102_initialize_rf_sys(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_pow_initialize_tx_power(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_pow_set_rf_regctl_enable(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rf_linectl);
extern oal_void hi1102_pow_set_pow_to_pow_code(hal_to_dmac_device_stru *pst_hal_device,
                                                hal_vap_pow_info_stru *pst_vap_pow_info,
                                                oal_uint8 uc_start_chain, wlan_channel_band_enum_uint8 en_freq_band);
extern oal_void hi1102_pow_set_resp_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_num,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);
extern oal_void hi1102_pow_set_band_spec_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_num,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);
extern oal_void hi1102_pow_get_spec_frame_data_rate_idx(oal_uint8 uc_rate,  oal_uint8 *puc_rate_idx);
extern oal_void hi1102_pow_set_pow_code_idx_same_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 ul_pow_code);
extern oal_void hi1102_pow_set_pow_code_idx_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 *aul_pow_code);
extern oal_void hi1102_pow_get_pow_index(hal_user_pow_info_stru *pst_hal_user_pow_info,
                     oal_uint8 uc_cur_rate_pow_idx, hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint8 *puc_pow_level);
extern oal_void hi1102_pow_set_four_rate_tx_dscr_power(hal_user_pow_info_stru *pst_hal_user_pow_info,
                                oal_uint8 *puc_rate_level_idx, oal_uint8 *pauc_pow_level,
                                hal_tx_txop_tx_power_stru *pst_tx_power);

extern oal_void hi1102_pow_cfg_no_margin_pow_mode(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_pow_mode);
extern oal_void  hi1102_pow_cfg_show_log(hal_to_dmac_device_stru *pst_hal_device, hal_vap_pow_info_stru *pst_vap_pow_info,
                                                    wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_rate_idx);

#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
extern oal_void hi1102_set_rf_custom_reg(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1102_cali_send_func(hal_to_dmac_device_stru *pst_hal_device, oal_uint8* puc_cali_data, oal_uint16 us_frame_len, oal_uint16 us_remain);
extern oal_void hi1102_psm_rf_sleep (hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_restore_reg);
extern oal_void hi1102_psm_rf_awake (hal_to_dmac_device_stru  *pst_hal_device,oal_uint8 uc_restore_reg);
extern oal_void hi1102_initialize_soc(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_get_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);
extern oal_void hi1102_clear_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1102_get_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_state);
extern oal_void hi1102_clear_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_status);
extern oal_void hi1102_unmask_mac_error_init_status(hal_to_dmac_device_stru * pst_hal_device, hal_error_state_stru *pst_status);
extern oal_void hi1102_unmask_mac_init_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1102_show_irq_info(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_param);
extern oal_void hi1102_dump_all_rx_dscr(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_clear_irq_stat(hal_to_dmac_device_stru * pst_hal_device);

extern oal_void hi1102_get_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 vap_id, hal_to_dmac_vap_stru ** ppst_hal_vap);
extern oal_void hi1102_add_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 uc_mac_vap_id, hal_to_dmac_vap_stru ** ppst_hal_vap);
extern oal_void hi1102_del_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 vap_id);


#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_void hi1102_set_proxysta_enable(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable);
#endif
extern oal_void hi1102_config_eifs_time(hal_to_dmac_device_stru *pst_hal_device, wlan_protocol_enum_uint8 en_protocol);
extern oal_void hi1102_register_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                           hal_alg_noify_enum_uint8 en_alg_notify,p_hal_alg_isr_func p_func);
extern oal_void hi1102_unregister_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                             hal_alg_noify_enum_uint8 en_alg_notify);
extern oal_void hi1102_register_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type, p_hal_gap_isr_func p_func);
extern oal_void hi1102_unregister_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type);
extern oal_void hi1102_one_packet_start(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_cfg_stru *pst_cfg);
extern oal_void hi1102_one_packet_stop(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_one_packet_get_status(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_status_stru *pst_status);
extern oal_void hi1102_reset_nav_timer(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_clear_hw_fifo(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_mask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1102_unmask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1102_reg_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 *pul_val);
extern oal_void hi1102_reg_info16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint16 *pus_val);
extern oal_void hi1102_read_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_reg_addr, oal_uint16 *pus_reg_val);
extern oal_void hi1102_write_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16  us_rf_addr_offset, oal_uint16 us_rf_16bit_data);

#ifdef _PRE_WLAN_FEATURE_DATA_SAMPLE
extern oal_void hi1102_set_sample_memory(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 **pul_start_addr, oal_uint32 *ul_reg_num);
extern oal_void hi1102_get_sample_state(hal_to_dmac_device_stru * pst_hal_device, oal_uint16 *pus_reg_val);
extern oal_void hi1102_set_pktmem_bus_access(hal_to_dmac_device_stru * pst_hal_device);
#endif
extern oal_void hi1102_get_all_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_val);
extern oal_void hi1102_get_ampdu_bytes(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_tx_bytes, oal_uint32 *pul_rx_bytes);
extern oal_void hi1102_get_rx_err_count(hal_to_dmac_device_stru* pst_hal_device,
                                        oal_uint32* pul_cnt1,
                                        oal_uint32* pul_cnt2,
                                        oal_uint32* pul_cnt3,
                                        oal_uint32* pul_cnt4,
                                        oal_uint32* pul_cnt5,
                                        oal_uint32* pul_cnt6);


#ifdef _PRE_WLAN_ONLINE_DPD
extern oal_void  hi1102_dpd_config(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 *puc_val);
extern oal_void hi1102_dpd_set_tpc_level(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_value);
extern oal_void hi1102_dpd_set_bw(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_value);

#endif
extern oal_void hi1102_show_fsm_info(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_mac_error_msg_report(hal_to_dmac_device_stru *pst_hal_device, hal_mac_error_type_enum_uint8 en_error_type);
extern oal_void hi1102_get_dieid(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dieid, oal_uint32 *pul_length);
extern oal_void hi1102_en_soc_intr(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_disable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_non_frame_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_disable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1102_enable_rx_amsdu_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1102_disable_rx_amsdu_mode(hal_to_dmac_device_stru *pst_hal_device);


extern oal_void hi1102_set_pmf_crypto(hal_to_dmac_vap_stru *pst_hal_vap, oal_bool_enum_uint8 en_crypto);
extern oal_void hi1102_ce_add_key(hal_to_dmac_device_stru *pst_hal_device,hal_security_key_stru *pst_security_key,oal_uint8 *puc_addr);
extern oal_void hi1102_ce_get_key(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key);
extern oal_void hi1102_ce_del_key(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key);
extern oal_void hi1102_disable_ce(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1102_ce_add_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx,oal_uint8 * puc_addr);
extern oal_void hi1102_ce_del_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx);
extern oal_void hi1102_set_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1102_get_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1102_set_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1102_get_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
#ifdef _PRE_WLAN_INIT_PTK_TX_PN
extern oal_void hi1102_tx_get_dscr_phy_mode_one(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_phy_mode_one);
extern oal_void hi1102_tx_get_ra_lut_index(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_ra_lut_index);
extern oal_void hi1102_init_ptk_tx_pn(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key,oal_uint32 ul_pn_msb);
#endif
extern oal_void hi1102_get_rate_80211g_table(oal_void **pst_rate);
extern oal_void hi1102_get_rate_80211g_num(oal_uint32 *pst_data_num);
extern oal_void hi1102_get_hw_addr(oal_uint8 *puc_addr);
extern oal_void hi1102_enable_ch_statics(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1102_set_ch_statics_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_period);
extern oal_void hi1102_set_ch_measurement_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_period);
extern oal_void hi1102_get_ch_statics_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1102_get_ch_measurement_result_ram(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1102_get_ch_measurement_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1102_enable_radar_det(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
#ifdef _PRE_WLAN_PHY_BUGFIX_VHT_SIG_B
extern oal_void hi1102_enable_sigB(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1102_enable_improve_ce(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
#endif
#ifdef _PRE_WLAN_PHY_BUGFIX_IMPROVE_CE_TH
extern oal_void hi1102_set_acc_symb_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_num);
extern oal_void  hi1102_set_improve_ce_threshold(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_val);
#endif
extern oal_void hi1102_get_radar_det_result(hal_to_dmac_device_stru *pst_hal_device, hal_radar_det_event_stru *pst_radar_info);
extern oal_void hi1102_update_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1102_set_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, hal_cfg_rts_tx_param_stru *pst_hal_rts_tx_param);
extern oal_void hi1102_set_rts_rate_selection_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rts_rate_select_mode);
extern oal_void hi1102_set_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_agc_track_ant_sel);
extern oal_void hi1102_get_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_agc_track_ant_sel);
extern oal_void hi1102_set_prot_resp_frame_chain(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_chain_val);
extern oal_void  hi1102_get_rf_temp(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_cur_temp);
extern oal_void  hi1102_set_dac_lpc_pa_upc_level(oal_uint8 uc_dac_lpf_code,
            oal_int16  *pas_tpc_level_table, oal_uint8 uc_tpc_level_num,
            oal_uint8 *pauc_dac_lpf_pa_code_table, oal_int16 *pas_upc_gain_table,
            oal_int16 *pas_other_gain_table,wlan_channel_band_enum_uint8 en_freq_band);

extern oal_void  hi1102_get_spec_frm_rate(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_set_bcn_phy_tx_mode(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_pow_code);
extern oal_void hi1102_set_spec_frm_phy_tx_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_subband_idx);
extern oal_void hi1102_get_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 *pul_phy_tx_up_down_time_reg,  oal_uint32 *pul_phy_rx_up_down_time_reg,
            oal_uint32 *pul_rf_reg_wr_delay1, oal_uint32 *pul_rf_reg_wr_delay2);
extern oal_void hi1102_set_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 ul_phy_tx_up_down_time_reg,  oal_uint32 ul_phy_rx_up_down_time_reg,
            oal_uint32 ul_rf_reg_wr_delay1, oal_uint32 ul_rf_reg_wr_delay2);
extern oal_void hi1102_get_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
        oal_uint16 *pus_dac_val, oal_uint16 *pus_pa_val, oal_uint16 *pus_lpf_val,
        oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void hi1102_set_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
      oal_uint16 us_dac_val, oal_uint16 us_pa_val, oal_uint16 us_lpf_val,
      oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void  hi1102_set_pow_phy_reg_param(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1102_set_resp_pow_level(hal_to_dmac_device_stru *pst_hal_device,
                       oal_int8 c_near_distance_rssi, oal_int8 c_far_distance_rssi);
extern oal_uint32 hi1102_get_subband_index(wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_channel_num, oal_uint8 *puc_subband_idx);
extern oal_void hi1102_rf_regctl_enable_set_regs(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_band_enum_uint8 en_freq_band,oal_uint8 uc_cur_ch_num,wlan_channel_bandwidth_enum_uint8 en_bandwidth);


extern oal_void  hi1102_get_bcn_rate(hal_to_dmac_vap_stru *pst_hal_vap,oal_uint8 *puc_data_rate);
extern oal_void hi1102_irq_affinity_init(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_core_id);

#ifdef _PRE_WLAN_FEATURE_TXBF
extern oal_void hi1102_set_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_matrix);
extern oal_void hi1102_get_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_matrix);
extern oal_void hi1102_set_vht_report_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1102_set_vht_report_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1102_set_ndp_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1102_set_ndp_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1102_set_ndp_max_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_ndp_time);
extern oal_void hi1102_set_ndpa_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_ndpa_duration);
extern oal_void hi1102_set_ndp_group_id(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_group_id, oal_uint16 us_partial_id);
extern oal_void hi1102_set_phy_legacy_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_set_phy_txbf_legacy_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_set_phy_pilot_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_set_ht_buffer_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 ul_reg_value);
extern oal_void hi1102_set_ht_buffer_step(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 ul_reg_value);
extern oal_void hi1102_set_ht_buffer_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_delete_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1102_set_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32 ul_reg_val);
extern oal_void hi1102_get_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);
extern oal_void hi1102_set_h_matrix_timeout(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_set_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable);
extern oal_void  hi1102_get_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_enable);
extern oal_void hi1102_set_mu_aid_matrix_info(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_aid);
extern oal_void hi1102_set_txbf_vht_buff_addr(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_addr, oal_uint16 us_buffer_len);
extern oal_void hi1102_set_sta_membership_status_63_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 value);
extern oal_void hi1102_set_sta_membership_status_31_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1102_set_sta_user_p_63_48(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1102_set_sta_user_p_47_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1102_set_sta_user_p_31_16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1102_set_sta_user_p_15_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1102_set_bfee_sounding_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_txbf_protocol_type, oal_uint8 uc_bfee_enable);
extern oal_void hi1102_set_bfee_h2v_beamforming_ng(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_user_bw);
extern oal_void hi1102_set_bfee_grouping_codebook(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_min_group, oal_uint8 uc_txbf_mode,oal_uint8 en_user_bw);
extern oal_void hi1102_set_bfer_subcarrier_ng(hal_to_dmac_device_stru *pst_hal_device, wlan_bw_cap_enum_uint8 en_user_bw);
extern oal_void hi1102_set_2g_rf_txdriver(hal_to_dmac_device_stru *pst_hal_device);

#endif

extern oal_void hi1102_enable_smart_antenna_gpio_set_default_antenna(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_delete_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1102_set_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint16 ul_reg_value);
extern oal_void hi1102_get_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);

/* phy debug信息中trailer信息上报设置寄存器 */
extern oal_void hi1102_rx_set_trlr_report_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *auc_config_val, oal_uint8 uc_trlr_switch);
#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
extern oal_void hi1102_set_weak_intf_rssi_th(hal_to_dmac_device_stru *pst_device, oal_int32 l_reg_val);
extern oal_void hi1102_get_weak_intf_rssi_th(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void hi1102_set_agc_unlock_min_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_tx_reg_val, oal_int32 l_rx_reg_val);
extern oal_void hi1102_get_agc_unlock_min_th(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void hi1102_set_nav_max_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_bss_dur, oal_uint32 us_obss_dur);
#endif
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT
extern oal_void hi1102_set_counter1_clear(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_get_txrx_frame_time(hal_to_dmac_device_stru *pst_hal_device, hal_ch_mac_statics_stru *pst_ch_statics);
extern oal_void hi1102_set_mac_clken(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_wctrl_enable);
#endif
extern oal_void hi1102_set_80m_resp_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_debug_en);
extern oal_void hi1102_get_mac_statistics_data(hal_to_dmac_device_stru *pst_hal_device, hal_mac_key_statis_info_stru *pst_mac_key_statis);

#ifdef _PRE_WLAN_FEATURE_CCA_OPT
extern oal_void hi1102_set_ed_high_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_ed_high_20_reg_val, oal_int32 l_ed_high_40_reg_val, oal_bool_enum_uint8 en_is_default_th);
extern oal_void hi1102_enable_sync_error_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_cnt_reg_val);
extern oal_void hi1102_get_sync_error_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_reg_val);
extern oal_void hi1102_set_sync_err_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_get_cca_reg_th(hal_to_dmac_device_stru *pst_hal_device, oal_int8 *ac_reg_val);
#endif
#ifdef _PRE_WLAN_FEATURE_MWO_DET
extern oal_void hi1102_set_mac_anti_intf_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1102_get_mac_anti_intf_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
extern oal_void hi1102_set_mac_anti_intf_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1102_get_mac_mwo_cycle_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_tx_comp_mwo_cyc_time);
extern oal_void hi1102_set_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device,
                                                          oal_int8                 c_start_rssi,
                                                          oal_int8                 c_end_rssi,
                                                          oal_bool_enum_uint8      en_enable_mwo,
                                                          oal_uint8                uc_cfg_power_sel);
extern oal_void hi1102_get_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
extern oal_void hi1102_restore_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1102_set_phy_mwo_det_timethr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1102_get_phy_mwo_det_timethr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
#endif
extern oal_void  hi1102_set_soc_lpm(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_soc_set_enum_uint8 en_type ,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);
extern oal_void hi1102_set_psm_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1102_set_psm_wakeup_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_mode);
extern oal_void  hi1102_set_psm_listen_interval(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval);
extern oal_void  hi1102_set_psm_listen_interval_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval_count);
extern oal_void hi1102_set_psm_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1102_set_psm_ext_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1102_set_psm_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_period);
extern oal_void hi1102_soc_set_pcie_l1s(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);
#if defined(_PRE_WLAN_FEATURE_SMPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1102_set_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_mode);
extern oal_void hi1102_get_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
#endif
#if defined(_PRE_WLAN_FEATURE_TXOPPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1102_set_txop_ps_enable(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1102_set_txop_ps_condition1(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1102_set_txop_ps_condition2(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1102_set_txop_ps_partial_aid(hal_to_dmac_vap_stru  *pst_hal_vap, oal_uint16 us_partial_aid);
#endif
extern oal_void hi1102_set_wow_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_set_bitmap,hal_wow_param_stru* pst_para);
extern oal_void hi1102_set_lpm_state(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_state_enum_uint8 uc_state_from, hal_lpm_state_enum_uint8 uc_state_to,oal_void* pst_para, oal_void* pst_wow_para);
extern oal_void hi1102_disable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_enable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_set_tx_abort_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_abort_en);
extern oal_void hi1102_set_coex_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_mac_ctrl, oal_uint32 ul_rf_ctrl);
extern oal_void hi1102_get_hw_version(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_hw_vsn, oal_uint32 *pul_hw_vsn_data,oal_uint32 *pul_hw_vsn_num);

#ifdef _PRE_DEBUG_MODE
extern oal_void hi1102_get_all_reg_value(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_get_cali_data(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1102_freq_adjust(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_pll_int, oal_uint16 us_pll_frac);
#endif
extern oal_void hi1102_set_tx_dscr_field(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_data, hal_rf_test_sect_enum_uint8 en_sect);
extern oal_void hi1102_get_tx_dscr_field(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1102_rf_test_disable_al_tx(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_rf_test_enable_al_tx(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr);
extern oal_void hi1102_enable_tx_comp(hal_to_dmac_device_stru *pst_hal_device);
#ifdef _PRE_WLAN_PHY_PLL_DIV
extern oal_void hi1102_rf_set_freq_skew(oal_uint16 us_idx, oal_uint16 us_chn, oal_int16 as_corr_data[]);
#endif
extern oal_void hi1102_set_daq_mac_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pul_addr, oal_uint16 us_unit_len, oal_uint16 us_unit_num, oal_uint16 us_depth);
extern oal_void hi1102_set_daq_phy_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1102_set_daq_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1102_get_daq_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_value);

extern oal_void hi1102_set_dac_lpf_gain(hal_to_dmac_device_stru *pst_hal_device,
                                    oal_uint8 en_band, oal_uint8 en_bandwidth,oal_uint8 uc_chan_number,oal_uint8 en_protocol_mode,oal_uint8 en_rate);
extern oal_void hi1102_get_pwr_comp_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_tx_ratio, oal_int16 * ps_pwr_comp_val);
extern oal_void hi1102_over_temp_handler(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_agc_threshold_handle(hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_rssi);

extern oal_void hi1102_set_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_val);
extern oal_void  hi1102_set_rx_filter_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_command);
extern oal_void hi1102_get_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void  hi1102_set_beacon_timeout_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value);
extern oal_void  hi1102_psm_clear_mac_rx_isr(hal_to_dmac_device_stru *pst_hal_device);

#define HAL_VAP_LEVEL_FUNC_EXTERN
extern oal_void hi1102_vap_tsf_get_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_lo);
extern oal_void hi1102_vap_tsf_set_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_lo);
extern oal_void hi1102_vap_tsf_get_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_hi, oal_uint32 *pul_tsf_lo);
extern oal_void hi1102_vap_tsf_set_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_hi, oal_uint32 ul_tsf_lo);
extern oal_void hi1102_vap_send_beacon_pkt(hal_to_dmac_vap_stru *pst_hal_vap, hal_beacon_tx_params_stru *pst_params);
extern oal_void hi1102_vap_set_beacon_rate(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_rate);
extern oal_void hi1102_vap_beacon_suspend(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_vap_beacon_resume(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_vap_set_machw_prot_params(hal_to_dmac_vap_stru *pst_hal_vap, hal_tx_txop_rate_params_stru *pst_phy_tx_mode, hal_tx_txop_per_rate_params_union *pst_data_rate);


extern oal_void hi1102_vap_set_macaddr(hal_to_dmac_vap_stru * pst_hal_vap, oal_uint8 *puc_mac_addr);
extern oal_void hi1102_vap_set_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);

extern oal_void hi1102_vap_clr_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);
extern oal_void hi1102_vap_set_machw_aifsn_all_ac(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               uc_bk,
                oal_uint8               uc_be,
                oal_uint8               uc_vi,
                oal_uint8               uc_vo);
extern oal_void hi1102_vap_set_machw_aifsn_ac(hal_to_dmac_vap_stru         *pst_hal_vap,
                                            wlan_wme_ac_type_enum_uint8   en_ac,
                                            oal_uint8                     uc_aifs);
extern oal_void  hi1102_vap_set_machw_aifsn_ac_wfa(hal_to_dmac_vap_stru         *pst_hal_vap,
                                      wlan_wme_ac_type_enum_uint8   en_ac,
                                      oal_uint8                     uc_aifs,
                                      wlan_wme_ac_type_enum_uint8   en_wfa_lock);
extern oal_void hi1102_vap_set_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin, oal_uint8 uc_ac_type);
extern oal_void  hi1102_vap_set_edca_machw_cw_wfa(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmaxmin, oal_uint8 uc_ac_type, wlan_wme_ac_type_enum_uint8   en_wfa_lock);
extern oal_void hi1102_vap_get_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin, oal_uint8 uc_ac_type);
#if 0
extern oal_void hi1102_vap_set_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1102_vap_get_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1102_vap_set_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1102_vap_get_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1102_vap_set_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1102_vap_get_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1102_vap_set_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1102_vap_get_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
#endif
extern oal_void hi1102_vap_set_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);


extern oal_void hi1102_vap_get_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1102_vap_set_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1102_vap_get_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1102_vap_set_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);
extern oal_void hi1102_vap_get_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1102_vap_set_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1102_vap_get_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1102_vap_set_machw_prng_seed_val_all_ac(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_vap_read_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_value);
extern oal_void hi1102_vap_write_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_value);
extern oal_void hi1102_vap_set_machw_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void hi1102_vap_update_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void  hi1102_vap_get_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_beacon_period);
extern oal_void  hi1102_vap_set_noa(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint32              ul_start_tsf,
                oal_uint32              ul_duration,
                oal_uint32              ul_interval,
                oal_uint8               uc_count);

extern oal_void  hi1102_sta_tsf_restore(hal_to_dmac_vap_stru   *pst_hal_vap);
extern oal_void  hi1102_sta_tsf_save(hal_to_dmac_vap_stru   *pst_hal_vap, oal_bool_enum_uint8 en_need_restore);
#ifdef _PRE_WLAN_FEATURE_P2P
extern oal_void  hi1102_vap_set_ops(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               en_ops_ctrl,
                oal_uint8               uc_ct_window);
extern oal_void  hi1102_vap_enable_p2p_absent_suspend(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_bool_enum_uint8     en_suspend_enable);
#endif
extern oal_void hi1102_set_sta_bssid(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_byte);
extern oal_void hi1102_set_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_period);
extern oal_void hi1102_get_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_dtim_period);
extern oal_void hi1102_set_sta_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_count);
extern oal_void  hi1102_get_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *uc_dtim_count);
extern oal_void  hi1102_set_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_count);
extern oal_void  hi1102_set_psm_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_period,
                                                oal_uint8 uc_listen_intvl_to_dtim_times, oal_bool_enum_uint8 en_receive_dtim);
extern oal_bool_enum hi1102_check_sleep_time(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_aon_tsf_disable_all(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_disable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_enable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap, oal_bool_enum_uint8 en_dbac_enable);
extern oal_void hi1102_mwo_det_enable_mac_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_reg_val);
extern oal_void hi1102_tx_enable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1102_tx_disable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1102_cfg_slottime_type(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_slottime_type);
extern oal_void hi1102_cfg_rsp_dyn_bw(oal_bool_enum_uint8 en_set, wlan_bw_cap_enum_uint8 en_dyn_bw);
extern oal_void hi1102_get_cfg_rsp_rate_mode(oal_uint32 *pul_rsp_rate_cfg_mode);
extern oal_void hi1102_set_rsp_rate(oal_uint32 ul_rsp_rate_val);
extern oal_void  hi1102_get_hw_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_cali_check_hw_status);
extern oal_void hi1102_pm_enable_front_end(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable_paldo);
extern oal_void hi1102_pm_disable_front_end_tx(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_pm_wlan_servid_register(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_pm_wlan_servid_unregister(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_pm_vote2platform(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_vote_state);
extern oal_void hi1102_init_pm_info(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1102_pm_set_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_adjust_val);
extern oal_void hi1102_pm_set_bcn_rf_chain(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_bcn_rf_chain);
#ifdef _PRE_WLAN_FEATURE_BTCOEX
extern oal_void hi1102_coex_irq_en_set(oal_uint8 uc_intr_en);
extern oal_void hi1102_coex_sw_irq_clr_set(oal_uint8 uc_irq_clr);
extern oal_void hi1102_coex_sw_irq_set(hal_coex_sw_irq_type_enum_uint8 en_coex_irq_type);
extern oal_void hi1102_coex_sw_irq_status_get(oal_uint8 *uc_irq_status);
extern oal_void hi1102_get_btcoex_abort_qos_null_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_qosnull_seq_num);
extern oal_void hi1102_get_btcoex_occupied_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 *us_occupied_period);
extern oal_void hi1102_get_btcoex_pa_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_pa_status);
#ifdef _PRE_WLAN_FEATURE_BTCOEX_SLV_TX_BUGFIX
extern oal_void hi1102_btcoex_set_wl0_tx_slv_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_tx_slv);
extern oal_void hi1102_btcoex_set_wl0_antc_switch(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_tx_slv);
extern oal_void hi1102_btcoex_set_wl0_rx_status_byp(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rx_byp);
#endif
extern oal_void hi1102_btcoex_get_bt_acl_status(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_acl_status);
extern oal_void hi1102_btcoex_get_bt_sco_status(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_sco_status);
extern oal_void hi1102_btcoex_get_ps_service_status(hal_to_dmac_device_stru *pst_hal_device, hal_btcoex_ps_status_enum_uint8 *en_ps_status);
extern oal_void hi1102_update_btcoex_btble_status(hal_to_dmac_chip_stru *pst_hal_chip);
extern oal_uint32 hi1102_btcoex_init(hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32 hi1102_btcoex_sw_preempt_init(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_get_btcoex_statistic(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable_abort_stat);
extern oal_uint32 hi1102_mpw_soc_write_reg(oal_uint32 ulQuryRegAddrTemp, oal_uint16 usQuryRegValueTemp);
extern oal_void hi1102_btcoex_update_ap_beacon_count(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_beacon_count);
extern oal_void hi1102_btcoex_post_event(hal_to_dmac_chip_stru *pst_hal_chip, hal_dmac_misc_sub_type_enum_uint8 en_sub_type);
extern oal_void hi1102_btcoex_have_small_ampdu(hal_to_dmac_device_stru *pst_hal_base_device, oal_uint32 *pul_have_ampdu);
extern oal_void hi1102_btcoex_process_bt_status(hal_to_dmac_chip_stru *pst_hal_chip, oal_uint8 uc_print);
extern oal_void hi1102_btcoex_set_slna_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_slna);
#ifdef _PRE_WLAN_FEATURE_LTECOEX
extern oal_void  hi1102_ltecoex_req_mask_ctrl(oal_uint16 req_mask_ctrl);
#endif
extern oal_void hi1102_set_btcoex_abort_preempt_frame_param(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_preempt_param);
extern oal_void hi1102_set_btcoex_abort_null_buff_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_abort_null_buff_addr);
extern oal_void hi1102_set_btcoex_tx_abort_preempt_type(hal_to_dmac_device_stru *pst_hal_device, hal_coex_hw_preempt_mode_enum_uint8 en_preempt_type);
extern oal_void hi1102_set_btcoex_abort_qos_null_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_qosnull_seq_num);
extern oal_void hi1102_set_btcoex_hw_rx_priority_dis(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_hw_rx_prio_dis);
extern oal_void hi1102_set_btcoex_hw_priority_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_hw_prio_en);
extern oal_void hi1102_set_btcoex_priority_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_priority_period);
extern oal_void hi1102_set_btcoex_occupied_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_occupied_period);
extern oal_void hi1102_btcoex_get_rf_control(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 ul_occupied_period, oal_uint32 *pul_wlbt_mode_sel, oal_uint16 us_wait_cnt);
extern oal_void hi1102_set_btcoex_sw_all_abort_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_sw_abort_ctrl);
extern oal_void hi1102_set_btcoex_sw_priority_flag(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_sw_prio_flag);
extern oal_void hi1102_set_btcoex_soc_gpreg0(oal_uint8 uc_val, oal_uint16 us_mask, oal_uint8 uc_offset);
extern oal_void hi1102_set_btcoex_soc_gpreg1(oal_uint8 uc_val, oal_uint16 us_mask, oal_uint8 uc_offset);
#endif
#ifdef _PRE_WLAN_FEATURE_SMARTANT
extern oal_void hi1102_dual_antenna_switch(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_value, oal_uint8 uc_by_alg, oal_uint32 *pul_result);
extern oal_void hi1102_dual_antenna_switch_at(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_value, oal_uint32 *pul_result);
extern oal_void hi1102_dual_antenna_check(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_result);
extern oal_void hi1102_dual_antenna_init(oal_void);
#endif
extern oal_void hi1102_tx_get_dscr_iv_word(hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_iv_ms_word, oal_uint32 *pul_iv_ls_word, oal_uint8 uc_chiper_type, oal_uint8 uc_chiper_keyid);
#ifdef _PRE_WLAN_DFT_STAT
extern oal_void  hi1102_dft_get_machw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pst_machw_stat,oal_uint8 us_bank_select, oal_uint32 *pul_len);
extern oal_void  hi1102_dft_set_phy_stat_node(hal_to_dmac_device_stru * pst_hal_device,oam_stats_phy_node_idx_stru *pst_phy_node_idx);
extern oal_void  hi1102_dft_get_phyhw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pst_phyhw_stat,oal_uint8 us_bank_select, oal_uint32 *pul_len);
extern oal_void  hi1102_dft_get_rfhw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pst_rfhw_stat, oal_uint32 *pul_len, oal_uint8 uc_rf_chain);
extern oal_void hi1102_dft_get_sochw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint16 *pst_sochw_stat, oal_uint32 *pul_len);
extern oal_void  hi1102_dft_print_machw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1102_dft_print_phyhw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1102_dft_print_rfhw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1102_dft_report_all_reg_state(hal_to_dmac_device_stru   *pst_hal_device);

#endif
extern oal_void hi1102_set_lte_gpio_mode(oal_uint32 ul_mode_value);

extern oal_void hi1102_cfg_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1102_get_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1102_revert_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1102_check_test_value_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value, oal_uint32 *pul_result);
extern oal_void hi1102_config_always_rx(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint8 uc_switch);
extern oal_uint32 hi1102_rf_get_pll_div_idx(wlan_channel_band_enum_uint8 en_band,oal_uint8  uc_channel_idx,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth,oal_uint8  *puc_pll_div_idx);
extern oal_void hi1102_get_cali_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_param);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_void hi1102_load_ini_power_gain(oal_void);
extern oal_void hi1102_config_update_scaling_reg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint16* paus_dbb_scale);
extern oal_void hi1102_config_update_dsss_scaling_reg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint16* paus_dbb_scale, oal_uint8  uc_distance);
extern oal_uint32 hi1102_config_custom_rf(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 * puc_param);
extern oal_uint32 hi1102_config_custom_dts_cali(oal_uint8 * puc_param);
extern oal_void hi1102_config_set_cus_nvram_params(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 * puc_param);
extern oal_void hi1102_config_update_rate_pow_table(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_config_get_cus_nvram_params(hal_cfg_custom_nvram_params_stru **ppst_cfg_nvram);
extern oal_void hi1102_config_get_cus_cca_param(hal_cfg_custom_cca_stru **ppst_cfg_cca);
extern oal_void hi1102_config_get_far_dist_dsss_scale_promote_switch(oal_uint8 *puc_switch);
#endif
extern oal_void hi1102_get_rate_idx_pow(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_pow_idx,
                                                 wlan_channel_band_enum_uint8 en_freq_band, oal_uint16 *pus_powr, oal_uint8 uc_chan_idx);
extern oal_void hi1102_get_target_tx_power_by_tx_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one,
                                                                   hal_pdet_info_stru *pst_pdet_info, oal_int16 *ps_tx_pow);
extern oal_void  hi1102_al_tx_hw(hal_to_dmac_device_stru *pst_hal_device_base, hal_al_tx_hw_stru *pst_al_tx_hw);
extern oal_void  hi1102_al_tx_hw_cfg(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint32 ul_mode, oal_uint32 ul_rate);
extern oal_void  hi1102_vap_get_gtk_rx_lut_idx(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_lut_idx);

#ifdef _PRE_WLAN_DFT_REG
extern oal_uint32 hi1102_debug_refresh_reg_ext(hal_to_dmac_device_stru *pst_hal_device, oam_reg_evt_enum_uint32 en_evt_type, oal_uint32 *pul_ret);
extern oal_void hi1102_debug_frw_evt(hal_to_dmac_device_stru *pst_hal_device);
#endif

extern oal_void hi1102_get_wow_enable_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);

#ifdef _PRE_WLAN_CHIP_TEST
extern oal_void hi1102_set_tx_dscr_long_nav_enable(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_en_status);
#endif

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
extern oal_void hi1102_get_tx_msdu_address_params(hal_tx_dscr_stru *pst_dscr, hal_tx_msdu_address_params **ppst_tx_dscr_msdu_subtable, oal_uint8 *puc_msdu_num);
#endif
extern oal_void hi1102_show_wow_state_info(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1102_flush_tx_complete_irq(hal_to_dmac_device_stru *pst_hal_dev);

extern oal_void  hi1102_device_init_vap_pow_code(hal_to_dmac_device_stru   *pst_hal_device,
                                            hal_vap_pow_info_stru            *pst_vap_pow_info,
                                            oal_uint8                         uc_cur_ch_num,
                                            wlan_channel_band_enum_uint8      en_freq_band,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth,
                                            hal_pow_set_type_enum_uint8       uc_type);
extern oal_void hi1102_device_get_tx_pow_from_rate_idx(hal_to_dmac_device_stru * pst_hal_device, hal_user_pow_info_stru *pst_hal_user_pow_info,
                                wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_cur_ch_num, oal_uint8 uc_cur_rate_pow_idx,
                                hal_tx_txop_tx_power_stru *pst_tx_power, oal_int16 *ps_tx_pow);


#elif ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))

/************************  1103  CHIP********************************************/
#define HAL_CHIP_LEVEL_FUNC_EXTERN
extern oal_void hi1103_get_chip_version(hal_to_dmac_chip_stru *pst_hal_chip, oal_uint32 *pul_chip_ver);
/************************  1103  DEVICE********************************************/
#define HAL_DEVICE_LEVEL_FUNC_EXTERN
extern oal_void hi1103_rx_init_dscr_queue(hal_to_dmac_device_stru *pst_device,oal_uint8 uc_set_hw);
extern oal_void hi1103_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_al_rx_init_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_set_sifs_resp_rate(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rate_restrict);
extern oal_void hi1103_set_tx_lifetime_check(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rate_restrict);
extern oal_void hi1103_al_rx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_tx_init_dscr_queue(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1103_tx_destroy_dscr_queue(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_init_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1103_free_rx_isr_list(oal_dlist_head_stru  *pst_rx_isr_list);
extern oal_void hi1103_destroy_hw_rx_isr_list(hal_to_dmac_device_stru *pst_device);

extern oal_void hi1103_tx_fill_basic_ctrl_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru * p_tx_dscr, hal_tx_mpdu_stru *pst_mpdu);
extern oal_void hi1103_tx_ctrl_dscr_link(hal_tx_dscr_stru *pst_tx_dscr_prev, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1103_get_tx_dscr_next(hal_tx_dscr_stru *pst_tx_dscr, hal_tx_dscr_stru **ppst_tx_dscr_next);
extern oal_void hi1103_tx_ctrl_dscr_unlink(hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1103_tx_seqnum_set_dscr(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 us_seqnum);
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
extern oal_void hi1103_tx_get_dscr_seqnum(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_seqnum, oal_uint8 *puc_valid);
#endif
extern oal_void hi1103_tx_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1103_tx_non_ucast_data_set_dscr(hal_to_dmac_device_stru     *pst_hal_device,
                                                   hal_tx_dscr_stru            *pst_tx_dscr,
                                                   hal_tx_txop_feature_stru   *pst_txop_feature,
                                                   hal_tx_txop_alg_stru       *pst_txop_alg,
                                                   hal_tx_ppdu_feature_stru   *pst_ppdu_feature);
extern oal_void hi1103_tx_set_dscr_modify_mac_header_length(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_mac_header_length);
extern oal_void hi1103_tx_set_dscr_seqno_sw_generate(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_sw_seqno_generate);
extern oal_void hi1103_tx_get_size_dscr(oal_uint8 us_msdu_num, oal_uint32 * pul_dscr_one_size, oal_uint32 * pul_dscr_two_size);
extern oal_void hi1103_tx_get_vap_id(hal_tx_dscr_stru * pst_tx_dscr, oal_uint8 *puc_vap_id);
extern oal_void hi1103_tx_get_dscr_ctrl_one_param(hal_tx_dscr_stru * pst_tx_dscr, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one_param);
extern oal_void hi1103_tx_update_dscr_para_hw(hal_tx_dscr_stru * pst_tx_dscr, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one_param);
extern oal_void hi1103_tx_get_dscr_seq_num(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_seq_num);
extern oal_void hi1103_tx_get_dscr_tx_cnt(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_tx_count);
extern oal_void hi1103_tx_dscr_get_rate3(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_rate);
extern oal_void hi1103_tx_set_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_status);
extern oal_void  hi1103_tx_retry_clear_dscr(hal_to_dmac_device_stru *pst_hal_device,hal_tx_dscr_stru *pst_tx_dscr);
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_void hi1103_tx_set_pdet_en(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr, oal_bool_enum_uint8 en_pdet_en_flag);
extern oal_void hi1103_dyn_cali_tx_pow_ch_set(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void  hi1103_dyn_cali_tx_pa_ppa_swtich(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_ppa_working);
extern oal_void  hi1103_dyn_cali_al_tx_config_amend(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void  hi1103_dyn_cali_vdet_val_amend(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_freq,
                                                         oal_uint8 uc_rf_id, oal_int16 s_vdet_val_in, oal_int16 *ps_det_val_out);
extern oal_int16  hi1103_dyn_cali_get_gm_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rf_id);
extern oal_void  hi1103_dyn_cali_get_tx_power_dc(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rf_id, oal_int16 *ps_tx_power_dc);
#endif
extern oal_void  hi1103_tx_get_bw_mode(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, wlan_bw_cap_enum_uint8 *pen_bw_mode);
extern oal_void hi1103_tx_get_dscr_status(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_status);
extern oal_void  hi1103_tx_get_dscr_send_rate_rank(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_send_rate_rank);
extern oal_void hi1103_tx_get_dscr_chiper_type(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_chiper_type, oal_uint8 *puc_chiper_key_id);
extern oal_void hi1103_tx_get_dscr_ba_ssn(hal_tx_dscr_stru *pst_tx_dscr, oal_uint16 *pus_ba_ssn);
extern oal_void hi1103_tx_get_dscr_ba_bitmap(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_ba_bitmap);
extern oal_void hi1103_tx_put_dscr(hal_to_dmac_device_stru * pst_hal_device, hal_tx_queue_type_enum_uint8 en_tx_queue_type, hal_tx_dscr_stru *past_tx_dscr);
extern oal_void hi1103_get_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 * pul_status, oal_uint8 uc_qnum);
extern oal_void  hi1103_get_tx_multi_q_status(hal_to_dmac_device_stru* pst_hal_device, oal_uint32 *pul_status, oal_uint8 uc_qnum);
extern oal_void hi1103_tx_get_ampdu_len(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_ampdu_len);
extern oal_void  hi1103_set_bcn_timeout_multi_q_enable(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_enable);

extern oal_void hi1103_tx_get_protocol_mode(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_protocol_mode);
extern oal_void hi1103_rx_get_info_dscr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_ctl_stru * pst_rx_ctl, hal_rx_status_stru * pst_rx_status, hal_rx_statistic_stru * pst_rx_statistics);
extern oal_void hi1103_get_hal_vap(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_vap_id, hal_to_dmac_vap_stru **ppst_hal_vap);
extern oal_void hi1103_rx_get_netbuffer_addr_dscr(oal_uint32 *pul_rx_dscr, oal_netbuf_stru ** ppul_mac_hdr_addr);
extern oal_void hi1103_rx_show_dscr_queue_info(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_rx_dscr_type);
extern oal_void hi1103_rx_print_phy_debug_info(hal_to_dmac_device_stru *pst_hal_device,oal_uint32 *pul_rx_dscr, hal_rx_statistic_stru *pst_rx_statistics);
extern oal_void hi1103_rx_record_frame_status_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_id);
//extern oal_void hi1103_rx_sync_invalid_dscr(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dscr, oal_uint8 en_queue_num);
extern oal_void hi1103_rx_free_dscr_list(hal_to_dmac_device_stru * pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint32 *pul_rx_dscr);
extern oal_void hi1103_dump_tx_dscr(oal_uint32 *pul_tx_dscr);
extern oal_void hi1103_reg_write(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 ul_val);
extern oal_void hi1103_reg_write16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint16 us_val);
#ifdef _PRE_WLAN_FEATURE_DATA_SAMPLE
extern oal_void hi1103_set_sample_memory(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 **pul_start_addr, oal_uint32 *pul_reg_num);
extern oal_void hi1103_free_sample_mem(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_set_pktmem_bus_access(hal_to_dmac_device_stru * pst_hal_device);

#endif
extern oal_void hi1103_get_sample_state(hal_to_dmac_device_stru * pst_hal_device, oal_uint16 *pus_reg_val);

#ifdef _PRE_WLAN_RF_AUTOCALI
extern oal_void hi1103_rf_cali_auto_switch(oal_uint32 ul_switch_mask);
extern oal_void hi1103_rf_cali_auto_mea_done(oal_uint8 uc_freq,
                                             oal_uint8 uc_chn_idx,
                                             oal_uint8 uc_cali_type,
                                             oal_uint8 uc_cali_state);
#endif

#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
extern oal_void hi1103_set_psd_memory(hal_to_dmac_device_stru *pst_hal_device, oal_int8 **pc_start_addr);
extern oal_void hi1103_free_psd_mem(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_set_psd_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_get_single_psd_sample(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_index, oal_int8 *pc_psd_val);
extern oal_void hi1103_set_up_fft_psd_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_nb_det_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_11b_det_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_ofdm_det_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_wifi_work_en(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_psd_the_bottom_noise(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_reg_value);
extern oal_void hi1103_set_psd_the_num_nb(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_the_power_nb(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_psd_the_rssi_nb(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_reg_value);
extern oal_void hi1103_get_psd_data(hal_to_dmac_device_stru * pst_hal_device, oal_int8 *pc_start_addr, oal_uint32 *pul_psd_data_len);
extern oal_void hi1103_set_sync_data_path_div_num(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_force_reg_clk_on(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_get_psd_info(hal_to_dmac_device_stru * pst_hal_device);

extern oal_void hi1103_set_fft_sample(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
#endif
#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_void hi1103_set_dev_support_11ax(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1103_set_mu_edca_lifetime(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lifetime);
extern oal_void hi1103_set_mu_edca_aifsn(hal_to_dmac_device_stru *pst_hal_device,
                                            oal_uint8               uc_bk,
                                            oal_uint8               uc_be,
                                            oal_uint8               uc_vi,
                                            oal_uint8               uc_vo);
extern oal_void hi1103_set_mu_edca_cw(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_ac_type, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void  hi1103_set_bss_color(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_bss_color);
extern oal_void  hi1103_set_partial_bss_color(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_partial_bss_color);

#endif
#ifdef _PRE_WLAN_FEATURE_CSI
extern oal_void hi1103_set_csi_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_csi_ta(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_addr);
extern oal_void hi1103_set_csi_ta_check(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
//extern oal_void hi1103_set_csi_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_get_mac_csi_info(hal_to_dmac_device_stru *pst_hal_device);

extern oal_void hi1103_get_phy_csi_info(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_bandwidth_enum_uint8 *pen_bandwidth, oal_uint8 *puc_frame_type);

extern oal_void hi1103_prepare_csi_sample_setup(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_pktmem_csi_bus_access(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_get_mac_csi_ta(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_addr);

extern oal_void hi1103_get_csi_end_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 **pul_reg_num);
extern oal_void hi1103_get_pktmem_start_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 **pul_reg_num);
extern oal_void hi1103_get_pktmem_end_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 **pul_reg_num);

extern oal_void hi1103_get_csi_frame_type(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 *puc_he_flag, oal_uint8 *puc_frame_type);
extern oal_void hi1103_free_csi_sample_mem(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_csi_memory(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_num);
extern oal_void hi1103_disable_csi_sample(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1103_clear_phy_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1103_get_phy_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);
extern oal_void hi1103_set_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_machw_rx_buff_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_uint32 hi1103_set_machw_rx_buff_addr_sync(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_rx_dscr, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_void  hi1103_rx_add_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint16 us_rx_dscr_num);
extern oal_void  hi1103_rx_add_dscr_th(hal_to_dmac_device_stru *pst_hal_device, hal_rx_dscr_queue_id_enum_uint8 en_queue_num, oal_uint16 us_rx_dscr_num);
extern oal_void hi1103_rx_update_dscr(hal_to_dmac_device_stru  *pst_hal_device,
                               hal_rx_dscr_queue_id_enum_uint8   en_queue_num,
                               oal_uint16                        us_rx_dscr_num);
extern oal_void  hi1103_recycle_rx_isr_list(hal_to_dmac_device_stru  *pst_hal_device, oal_dlist_head_stru  *pst_rx_isr_list);
extern oal_void hi1103_set_machw_tx_suspend(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_machw_tx_resume(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_reset_phy_machw(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type,
                                     oal_uint8 sub_mod,oal_uint8 uc_reset_phy_reg,oal_uint8 uc_reset_mac_reg);
extern oal_void hi1103_disable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_recover_machw_phy_and_pa(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_clear_user_ptk_key(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_idx);
extern oal_void hi1103_initialize_machw(hal_to_dmac_device_stru *pst_hal_device);
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
extern oal_void hi1103_set_ampdu_tx_hw_on(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable, oal_uint8 uc_snd_type);
#endif
extern oal_void hi1103_initialize_machw_common(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_freq_band(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1103_set_bandwidth_mode(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1103_set_upc_data(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_process_phy_freq(hal_to_dmac_device_stru *pst_hal_device);
#ifdef _PRE_WLAN_1103_PILOT
extern oal_void hi1103_set_txop_check_cca(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 en_txop_check_cca);
#endif
extern oal_void hi1103_set_primary_channel(
                    hal_to_dmac_device_stru          *pst_hal_device,
                    oal_uint8                         uc_channel_num,
                    wlan_channel_band_enum_uint8      en_band,
                    oal_uint8                         uc_channel_idx,
                    wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_uint8 hi1103_set_machw_phy_adc_freq(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1103_set_power_test(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_en);
#ifdef _PRE_WLAN_PHY_PERFORMANCE
extern oal_void hi1103_set_phy_tx_scale(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_2g_11ac);
#endif

extern oal_void hi1103_set_rx_multi_ant(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_phy_chain);
extern oal_void hi1103_recover_rx_multi_and_one_ant(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1103_reset_slave_ana_dbb_ch_sel(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1103_set_ana_dbb_ch_sel(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_update_cbb_cfg(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_11b_reuse_sel(hal_to_dmac_device_stru  *pst_hal_device);
extern oal_void  hi1103_add_machw_tx_ba_lut_entry(hal_to_dmac_device_stru* pst_hal_device,
        oal_uint8 uc_lut_index, oal_uint8 uc_tid,
        oal_uint16 uc_seq_no, oal_uint8 uc_win_size, oal_uint8 uc_mmss);
extern oal_void hi1103_add_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device,
                oal_uint8 uc_lut_index, oal_uint8 *puc_dst_addr, oal_uint8 uc_tid,
                oal_uint16 uc_seq_no, oal_uint8 uc_win_size);
extern oal_void hi1103_remove_machw_ba_lut_entry(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1103_get_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_index,
                        oal_uint32* pst_addr_h,oal_uint32* pst_addr_l,oal_uint32* pst_bitmap_h,oal_uint32* pst_bitmap_l,oal_uint32* pst_ba_para);
extern oal_void hi1103_restore_machw_ba_params(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_index,
                                             oal_uint32 ul_addr_h,oal_uint32 ul_addr_l,oal_uint32 ul_ba_para);
extern oal_void hi1103_machw_seq_num_index_update_per_tid(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_qos_flag);
extern oal_void hi1103_set_tx_sequence_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag,oal_uint32 ul_val_write,oal_uint8 uc_vap_index);
extern oal_void hi1103_get_tx_sequence_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag, oal_uint8 uc_vap_index,oal_uint32 *pst_val_read);
extern oal_void  hi1103_get_tx_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index,oal_uint8 uc_tid, oal_uint8 uc_qos_flag, oal_uint8 uc_vap_index,oal_uint16 *pst_val_read);
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
extern oal_void  hi1103_save_tx_ba_para(hal_to_dmac_device_stru *pst_hal_device, hal_ba_para_stru *pst_ba_para);
extern oal_void  hi1103_get_tx_ba_para(hal_to_dmac_device_stru *pst_hal_device, hal_ba_para_stru *pst_ba_para);
#endif
extern oal_void hi1103_reset_init(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_reset_destroy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_reset_reg_restore(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1103_reset_reg_save(hal_to_dmac_device_stru * pst_hal_device,hal_reset_hw_type_enum_uint8 en_type);
extern oal_void hi1103_reset_reg_dma_save(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1103_reset_reg_dma_restore(hal_to_dmac_device_stru* pst_hal,oal_uint8* uc_dmach0,oal_uint8* uc_dmach1,oal_uint8* uc_dmach2);
extern oal_void hi1103_disable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_machw_ack_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_disable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_machw_cts_trans(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_initialize_phy(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_radar_config_reg(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_dfs_domain);
extern oal_void hi1103_radar_config_reg_bw(hal_to_dmac_device_stru *pst_hal_device, hal_dfs_radar_type_enum_uint8 en_radar_type, wlan_channel_bandwidth_enum_uint8 en_bandwidth);
extern oal_void hi1103_radar_enable_chirp_det(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_chirp_det);
extern oal_uint32 hi1103_set_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_th);
extern oal_void hi1103_get_radar_th_reg(hal_to_dmac_device_stru *pst_hal_device, oal_int32 *pl_th);
extern oal_void hi1103_trig_dummy_radar(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_radar_type);
extern oal_void hi1103_initialize_rf_sys(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_pow_sw_initialize_tx_power(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_pow_initialize_tx_power(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_pow_set_rf_regctl_enable(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rf_linectl);
extern oal_void hi1103_pow_set_resp_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_idx,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);
extern oal_void hi1103_pow_set_band_spec_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device,
                                wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_chan_idx,
                                hal_rate_pow_code_gain_table_stru *pst_rate_pow_table);
extern oal_void hi1103_pow_get_spec_frame_data_rate_idx(oal_uint8 uc_rate,  oal_uint8 *puc_rate_idx);
extern oal_void hi1103_pow_set_pow_code_idx_same_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 *pul_pow_code);
extern oal_void hi1103_pow_set_pow_code_idx_in_tx_power(hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint32 *aul_pow_code);
#ifdef _PRE_WLAN_FEATURE_USER_RESP_POWER
extern oal_void  hi1103_pow_set_user_resp_frame_tx_power(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_rssi_distance);
extern oal_void hi1103_pow_del_machw_resp_power_lut_entry(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
#endif
extern oal_void hi1103_pow_get_pow_index(hal_user_pow_info_stru *pst_hal_user_pow_info,
                     oal_uint8 uc_cur_rate_pow_idx, hal_tx_txop_tx_power_stru *pst_tx_power, oal_uint8 *puc_pow_level);
extern oal_void hi1103_pow_set_four_rate_tx_dscr_power(hal_user_pow_info_stru *pst_hal_user_pow_info,
                                oal_uint8 *puc_rate_level_idx, oal_uint8 *pauc_pow_level,
                                hal_tx_txop_tx_power_stru *pst_tx_power);


extern oal_void hi1103_pow_cfg_no_margin_pow_mode(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_pow_mode);
extern oal_void  hi1103_pow_cfg_show_log(hal_to_dmac_device_stru *pst_hal_device, hal_vap_pow_info_stru *pst_vap_pow_info,
                                                    wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_rate_idx);
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
extern oal_void hi1103_rf_init_upc_amend(oal_void);
extern oal_void hi1103_update_upc_amend_by_tas(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_chn_idx,
                                                             oal_bool_enum_uint8 en_need_improved);
#endif

#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
extern oal_void hi1103_set_rf_custom_reg(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1103_cali_matrix_data_send_func(hal_to_dmac_device_stru *pst_hal_device, oal_uint8* puc_matrix_data, oal_uint16 us_frame_len, oal_uint16 us_remain);
extern oal_void hi1103_cali_send_func(hal_to_dmac_device_stru *pst_hal_device, oal_uint8* puc_cali_data, oal_uint16 us_frame_len, oal_uint16 us_remain);
extern oal_void hi1103_psm_rf_sleep (hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_restore_reg);
extern oal_void hi1103_psm_rf_awake (hal_to_dmac_device_stru  *pst_hal_device,oal_uint8 uc_restore_reg);
extern oal_void hi1103_initialize_soc(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_get_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);
extern oal_void hi1103_clear_mac_int_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1103_get_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_state);
extern oal_void hi1103_clear_mac_error_int_status(hal_to_dmac_device_stru *pst_hal_device, hal_error_state_stru *pst_status);
//extern oal_void hi1103_unmask_mac_error_init_status(hal_to_dmac_device_stru * pst_hal_device, hal_error_state_stru *pst_status);
extern oal_void hi1103_unmask_mac_init_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_status);
extern oal_void hi1103_show_irq_info(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 uc_param);
extern oal_void hi1103_dump_all_rx_dscr(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_clear_irq_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_add_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 uc_mac_vap_id, hal_to_dmac_vap_stru ** ppst_hal_vap);
extern oal_void hi1103_del_vap(hal_to_dmac_device_stru *pst_hal_device, wlan_vap_mode_enum_uint8 vap_mode, oal_uint8 vap_id);
#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_void hi1103_update_datarate_by_chain(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_resp_tx_chain);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_void hi1103_set_proxysta_enable(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable);
#endif
extern oal_void hi1103_config_eifs_time(hal_to_dmac_device_stru *pst_hal_device, wlan_protocol_enum_uint8 en_protocol);
extern oal_void hi1103_register_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                           hal_alg_noify_enum_uint8 en_alg_notify,p_hal_alg_isr_func p_func);
extern oal_void hi1103_unregister_alg_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,
                                             hal_alg_noify_enum_uint8 en_alg_notify);
extern oal_void hi1103_register_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type,p_hal_alg_isr_func p_func);
extern oal_void hi1103_unregister_gap_isr_hook(hal_to_dmac_device_stru *pst_hal_device, hal_isr_type_enum_uint8 en_isr_type);
extern oal_void hi1103_rx_set_trlr_report_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *auc_config_val, oal_uint8 uc_trlr_switch);
extern oal_void hi1103_one_packet_start(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_cfg_stru *pst_cfg);
extern oal_void hi1103_one_packet_stop(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_one_packet_get_status(struct tag_hal_to_dmac_device_stru *pst_hal_device, hal_one_packet_status_stru *pst_status);
extern oal_void hi1103_reset_nav_timer(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_clear_hw_fifo(struct tag_hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_mask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1103_unmask_interrupt(struct tag_hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_offset);
extern oal_void hi1103_reg_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint32 *pul_val);
extern oal_void hi1103_reg_info16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_addr, oal_uint16 *pus_val);
extern oal_void hi1103_read_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_reg_addr, oal_uint16 *pus_reg_val);
extern oal_void hi1103_write_rf_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16  us_rf_addr_offset, oal_uint16 us_rf_16bit_data);

#ifdef _PRE_WLAN_ONLINE_DPD
extern oal_void  hi1103_dpd_config(hal_to_dmac_device_stru * pst_hal_device, oal_uint8 *puc_val);
extern oal_void hi1103_dpd_cfr_set_tpc(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_tpc);
extern oal_void hi1103_dpd_cfr_set_bw(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_bw);
extern oal_void hi1103_dpd_cfr_set_mcs(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_mcs);
extern oal_void hi1103_dpd_cfr_set_freq(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_freq);
extern oal_void hi1103_dpd_cfr_set_11b(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 en_11b);
extern oal_void hi1103_dpd_cfr_set_11b(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 en_11b);
#endif

extern oal_void hi1103_get_all_tx_q_status(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_val);
extern oal_void hi1103_get_ampdu_bytes(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_tx_bytes, oal_uint32 *pul_rx_bytes);
extern oal_void hi1103_get_rx_err_count(hal_to_dmac_device_stru* pst_hal_device,
                                        oal_uint32* pul_cnt1,
                                        oal_uint32* pul_cnt2,
                                        oal_uint32* pul_cnt3,
                                        oal_uint32* pul_cnt4,
                                        oal_uint32* pul_cnt5,
                                        oal_uint32* pul_cnt6);
extern oal_void hi1103_show_fsm_info(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_mac_error_msg_report(hal_to_dmac_device_stru *pst_hal_device, hal_mac_error_type_enum_uint8 en_error_type);
extern oal_void hi1103_get_dieid(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 *pul_dieid, oal_uint32 *pul_length);
extern oal_void hi1103_en_soc_intr(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_disable_beacon_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_non_frame_filter(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_disable_monitor_mode(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_pmf_crypto(hal_to_dmac_vap_stru *pst_hal_vap, oal_bool_enum_uint8 en_crypto);
extern oal_void hi1103_ce_enable_key(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_cus_over_temper_rf(oal_uint8 *puc_param);
extern oal_void hi1103_ce_add_key(hal_to_dmac_device_stru *pst_hal_device,hal_security_key_stru *pst_security_key,oal_uint8 *puc_addr);
extern oal_void hi1103_ce_del_key(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key);
extern oal_void hi1103_ce_get_key(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key);
extern oal_void hi1103_disable_ce(hal_to_dmac_device_stru *pst_device);
extern oal_void hi1103_ce_add_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx,oal_uint8 * puc_addr);
extern oal_void hi1103_ce_del_peer_macaddr(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_lut_idx);
extern oal_void hi1103_set_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1103_get_rx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1103_set_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
extern oal_void hi1103_get_tx_pn(hal_to_dmac_device_stru *pst_hal_device,hal_pn_lut_cfg_stru* pst_pn_lut_cfg);
#ifdef _PRE_WLAN_INIT_PTK_TX_PN
extern oal_void hi1103_tx_get_dscr_phy_mode_one(hal_tx_dscr_stru *pst_tx_dscr, oal_uint32 *pul_phy_mode_one);
extern oal_void hi1103_tx_get_ra_lut_index(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_dscr, oal_uint8 *puc_ra_lut_index);
extern oal_void hi1103_init_ptk_tx_pn(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key,oal_uint32 ul_pn_msb);
#endif
extern oal_void hi1103_get_rate_80211g_table(oal_void **pst_rate);
extern oal_void hi1103_get_rate_80211g_num(oal_uint32 *pst_data_num);
extern oal_void hi1103_get_hw_addr(oal_uint8 *puc_addr);
extern oal_void hi1103_enable_ch_statics(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1103_set_ch_statics_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_period);
extern oal_void hi1103_set_ch_measurement_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_period);
extern oal_void hi1103_get_ch_statics_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1103_get_ch_measurement_result_ram(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1103_get_ch_measurement_result(hal_to_dmac_device_stru *pst_hal_device, hal_ch_statics_irq_event_stru *pst_ch_statics);
extern oal_void hi1103_enable_radar_det(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
extern oal_void hi1103_read_max_temperature(oal_int16 *ps_temperature);
extern oal_void hi1103_get_radar_det_result(hal_to_dmac_device_stru *pst_hal_device, hal_radar_det_event_stru *pst_radar_info);
extern oal_void hi1103_radar_get_pulse_info(hal_to_dmac_device_stru *pst_hal_device, hal_radar_pulse_info_stru *pst_pulse_info);
extern oal_void hi1103_radar_clean_pulse_buf(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_update_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1103_set_rts_rate_params(hal_to_dmac_device_stru *pst_hal_device, hal_cfg_rts_tx_param_stru *pst_hal_rts_tx_param);
#ifdef _PRE_WLAN_1103_PILOT
extern oal_void hi1103_set_txbf_sounding_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_txbf_sounding_rate);
#endif
extern oal_void hi1103_set_rts_rate_selection_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_rts_rate_select_mode);
extern oal_void hi1103_set_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_agc_track_ant_sel);
extern oal_void hi1103_get_agc_track_ant_sel(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_agc_track_ant_sel);
extern oal_void hi1103_set_prot_resp_frame_chain(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_chain_val);
extern oal_void  hi1103_get_rf_temp(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_cur_temp);
extern oal_void  hi1103_get_spec_frm_rate(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1103_set_bcn_phy_tx_mode(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_pow_code);
extern oal_void hi1103_set_spec_frm_phy_tx_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band, oal_uint8 uc_subband_idx);
extern oal_void hi1103_get_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 *pul_phy_tx_up_down_time_reg,  oal_uint32 *pul_phy_rx_up_down_time_reg,
            oal_uint32 *pul_rf_reg_wr_delay1, oal_uint32 *pul_rf_reg_wr_delay2);
extern oal_void hi1103_set_pow_delay_reg_param(hal_to_dmac_device_stru *pst_hal_device,
            oal_uint32 ul_phy_tx_up_down_time_reg,  oal_uint32 ul_phy_rx_up_down_time_reg,
            oal_uint32 ul_rf_reg_wr_delay1, oal_uint32 ul_rf_reg_wr_delay2);
extern oal_void hi1103_get_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
        oal_uint16 *pus_dac_val, oal_uint16 *pus_pa_val, oal_uint16 *pus_lpf_val,
        oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void hi1103_set_pow_rf_reg_param(hal_to_dmac_device_stru *pst_hal_device,
      oal_uint16 us_dac_val, oal_uint16 us_pa_val, oal_uint16 us_lpf_val,
      oal_uint16* paus_2g_upc_val, oal_uint16* paus_5g_upc_val, oal_uint8 uc_chain_idx);
extern oal_void  hi1103_set_resp_pow_level(hal_to_dmac_device_stru *pst_hal_device,
                   oal_int8 c_near_distance_rssi, oal_int8 c_far_distance_rssi);
extern oal_void hi1103_rf_regctl_enable_set_regs(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_freq_band,oal_uint8 uc_cur_ch_num,wlan_channel_bandwidth_enum_uint8 en_bandwidth);

extern oal_void  hi1103_get_bcn_rate(hal_to_dmac_vap_stru *pst_hal_vap,oal_uint8 *puc_data_rate);
extern oal_void hi1103_irq_affinity_init(hal_to_dmac_device_stru * pst_hal_device, oal_uint32 ul_core_id);

#ifdef _PRE_WLAN_FEATURE_TXBF
#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
extern oal_void hi1103_get_fake_vap_id(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_fake_vap_id);
extern oal_void hi1103_set_fake_vap(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_vap_id);
extern oal_void hi1103_clr_fake_vap(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_vap_id);
#if (_PRE_WLAN_REAL_CHIP == _PRE_WLAN_CHIP_SIM)
extern oal_void hi1103_set_2g_rf_txdriver(hal_to_dmac_device_stru *pst_hal_device);
#endif
#endif
extern oal_void hi1103_set_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_matrix);
extern oal_void hi1103_get_legacy_matrix_buf_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_matrix);
extern oal_void hi1103_set_vht_report_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1103_set_vht_report_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1103_set_ndp_rate(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rate);
extern oal_void hi1103_set_ndp_phy_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_phy_mode);
extern oal_void hi1103_set_ndp_max_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_ndp_time);
extern oal_void hi1103_set_ndpa_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_ndpa_duration);
extern oal_void hi1103_set_ndp_group_id(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_group_id, oal_uint16 us_partial_id);
extern oal_void hi1103_set_phy_legacy_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_phy_txbf_legacy_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_phy_pilot_bf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_ht_buffer_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1103_set_ht_buffer_step(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_reg_value);
extern oal_void hi1103_set_ht_buffer_pointer(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_delete_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1103_set_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32 ul_reg_value);
extern oal_void hi1103_get_txbf_lut_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);
extern oal_void hi1103_set_h_matrix_timeout(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable);
extern oal_void  hi1103_get_dl_mumimo_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_enable);
extern oal_void hi1103_set_mu_aid_matrix_info(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_aid);
extern oal_void hi1103_set_sta_membership_status_63_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 value);
extern oal_void hi1103_set_sta_membership_status_31_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1103_set_sta_user_p_63_48(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1103_set_sta_user_p_47_32(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1103_set_sta_user_p_31_16(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1103_set_sta_user_p_15_0(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_value);
extern oal_void hi1103_set_bfer_subcarrier_ng(hal_to_dmac_device_stru *pst_hal_device, wlan_bw_cap_enum_uint8 en_user_bw);
extern oal_void hi1103_set_txbf_vht_buff_addr(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_addr, oal_uint16 us_buffer_len);
#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_void hi1103_set_txbf_he_buff_addr(hal_to_dmac_device_stru *pst_hal_device, hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_addr, oal_uint16 us_buffer_len);
#endif
extern oal_void hi1103_set_bfee_h2v_beamforming_ng(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_user_bw);
extern oal_void hi1103_set_bfee_grouping_codebook(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint8 uc_min_group, oal_uint8 uc_txbf_mode,oal_uint8 uc_user_bw);

extern oal_void hi1103_set_bfee_sounding_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_txbf_mode, oal_uint8 uc_bfee_enable);
extern oal_void  hi1103_rf_second_agc_ctrl(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_void hi1103_set_bfee_bypass_clk_gating(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable);
#endif
#endif

extern oal_void hi1103_enable_smart_antenna_gpio_set_default_antenna(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_delete_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1103_set_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint16 ul_reg_value);
extern oal_void hi1103_get_smart_antenna_value(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index, oal_uint32*  pst_reg_value);
/* phy debug信息中trailer信息上报设置寄存器 */
extern oal_void hi1103_rx_set_trlr_report_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *auc_config_val, oal_uint8 uc_trlr_switch);
extern oal_void hi1103_rf_tone_transmit_entrance(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_data_len, oal_uint8 uc_chain_idx);
extern oal_void hi1103_rf_tone_transmit_exit(hal_to_dmac_device_stru *pst_hal_device);
#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
extern oal_void hi1103_set_weak_intf_rssi_th(hal_to_dmac_device_stru *pst_device, oal_int32 l_reg_val);
extern oal_void hi1103_get_weak_intf_rssi_th(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pst_reg_value);
extern oal_void hi1103_set_agc_unlock_min_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_tx_reg_val, oal_int32 l_rx_reg_val);
extern oal_void hi1103_get_agc_unlock_min_th(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void hi1103_set_nav_max_duration(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_bss_dur, oal_uint32 us_obss_dur);
#endif
extern oal_void hi1103_report_gm_val(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_extlna_bypass_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_switch);
extern oal_void hi1103_set_extlna_chg_cfg(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_extlna_chg_bypass);
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT
extern oal_void hi1103_set_counter1_clear(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_get_txrx_frame_time(hal_to_dmac_device_stru *pst_hal_device, hal_ch_mac_statics_stru *pst_ch_statics);
extern oal_void hi1103_set_mac_clken(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_wctrl_enable);
#endif

extern oal_void hi1103_set_80m_resp_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_debug_en);
extern oal_void hi1103_get_mac_statistics_data(hal_to_dmac_device_stru *pst_hal_device, hal_mac_key_statis_info_stru *pst_mac_key_statis);
extern oal_void hi1103_set_ddc_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_noise_est_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_noise_comb_close(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_zf_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_dyn_bypass_extlna_pm_flag(hal_to_dmac_device_stru *pst_hal_device, wlan_channel_band_enum_uint8 en_band, oal_bool_enum_uint8 en_value);
extern oal_bool_enum_uint8 hi1103_get_dyn_bypass_extlna_pm_flag(hal_to_dmac_device_stru *pst_hal_device);
#ifdef _PRE_WLAN_FEATURE_CCA_OPT
extern oal_void hi1103_set_ed_high_th(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_ed_high_20_reg_val, oal_int32 l_ed_high_40_reg_val, oal_bool_enum_uint8 en_is_default_th);
extern oal_void hi1103_set_cca_prot_th(hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_ed_low_th_dsss_reg_val, oal_int8 c_ed_low_th_ofdm_reg_val);
extern oal_void hi1103_enable_sync_error_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_cnt_reg_val);
extern oal_void hi1103_get_sync_error_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_reg_val);
extern oal_void hi1103_set_sync_err_counter_clear(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_get_cca_reg_th(hal_to_dmac_device_stru *pst_hal_device, oal_int8 *ac_reg_val);
#endif
#ifdef _PRE_WLAN_FEATURE_MWO_DET
extern oal_void hi1103_set_mac_anti_intf_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1103_get_mac_anti_intf_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
extern oal_void hi1103_set_mac_anti_intf_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1103_get_mac_mwo_cycle_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_tx_comp_mwo_cyc_time);
extern oal_void hi1103_set_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device,
                                                          oal_int8                 c_start_rssi,
                                                          oal_int8                 c_end_rssi,
                                                          oal_bool_enum_uint8      en_enable_mwo,
                                                          oal_uint8                uc_cfg_power_sel);
extern oal_void hi1103_get_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
extern oal_void hi1103_restore_phy_mwo_det_rssithr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1103_set_phy_mwo_det_timethr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_val);
extern oal_void hi1103_get_phy_mwo_det_timethr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_val);
#endif
extern oal_void  hi1103_set_soc_lpm(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_soc_set_enum_uint8 en_type ,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);
extern oal_void hi1103_set_psm_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1103_set_psm_wakeup_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_mode);
extern oal_void  hi1103_set_psm_listen_interval(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval);
extern oal_void  hi1103_set_psm_listen_interval_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_interval_count);
extern oal_void hi1103_set_psm_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1103_set_psm_ext_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1103_set_psm_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_period);
extern oal_void hi1103_soc_set_pcie_l1s(hal_to_dmac_device_stru *pst_hal_device,oal_uint8 uc_on_off,oal_uint8 uc_pcie_idle);
#if defined(_PRE_WLAN_FEATURE_SMPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1103_set_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 en_smps_mode);
extern oal_void hi1103_get_smps_mode(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
#endif
#if defined(_PRE_WLAN_FEATURE_TXOPPS) || defined(_PRE_WLAN_CHIP_TEST)
extern oal_void hi1103_set_txop_ps_enable(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1103_set_txop_ps_condition1(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1103_set_txop_ps_condition2(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_on_off);
extern oal_void hi1103_set_txop_ps_partial_aid(hal_to_dmac_vap_stru  *pst_hal_vap, oal_uint32 ul_partial_aid);
extern oal_void hi1103_get_txop_ps_partial_aid(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_partial_aid);
#endif
#ifdef _PRE_WLAN_FEATURE_MAC_PARSE_TIM
extern oal_void  hi1103_mac_set_bcn_tim_pos(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_pos);
#endif
extern oal_void  hi1103_set_mac_aid(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_aid);

extern oal_void hi1103_enable_tx_comp(hal_to_dmac_device_stru *pst_hal_device);

extern oal_void hi1103_set_wow_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_set_bitmap,hal_wow_param_stru* pst_para);
extern oal_void hi1103_set_lpm_state(hal_to_dmac_device_stru *pst_hal_device,hal_lpm_state_enum_uint8 uc_state_from, hal_lpm_state_enum_uint8 uc_state_to,oal_void* pst_para, oal_void* pst_wow_para);
extern oal_void hi1103_disable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_machw_edca(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_set_tx_abort_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_abort_en);
extern oal_void hi1103_set_coex_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_mac_ctrl, oal_uint32 ul_rf_ctrl);
extern oal_void hi1103_get_hw_version(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_hw_vsn, oal_uint32 *pul_hw_vsn_data,oal_uint32 *pul_hw_vsn_num);

#ifdef _PRE_DEBUG_MODE
extern oal_void hi1103_get_all_reg_value(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_get_cali_data(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void hi1103_freq_adjust(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_pll_int, oal_uint16 us_pll_frac);
#endif
extern oal_void hi1103_set_tx_dscr_field(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_data, hal_rf_test_sect_enum_uint8 en_sect);
extern oal_void hi1103_get_tx_dscr_field(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr);
extern oal_void hi1103_set_m2s_tx_dscr_field(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr, hal_tx_txop_alg_stru *pst_txop_alg);
extern oal_void hi1103_set_phy_max_bw_field(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_data, hal_phy_max_bw_sect_enmu_uint8 en_sect);
extern oal_void hi1103_rf_test_disable_al_tx(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_rf_test_enable_al_tx(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr);
#ifdef _PRE_WLAN_PHY_PLL_DIV
extern oal_void hi1103_rf_set_freq_skew(oal_uint16 us_idx, oal_uint16 us_chn, oal_int16 as_corr_data[]);
#endif
extern oal_void hi1103_set_daq_mac_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pul_addr, oal_uint16 us_unit_len, oal_uint16 us_unit_num, oal_uint16 us_depth);
extern oal_void hi1103_set_daq_phy_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_reg_value);
extern oal_void hi1103_set_daq_en(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_reg_value);
extern oal_void hi1103_get_daq_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_reg_value);

extern oal_void hi1103_set_dac_lpf_gain(hal_to_dmac_device_stru *pst_hal_device,
                                    oal_uint8 en_band, oal_uint8 en_bandwidth,oal_uint8 uc_chan_number,oal_uint8 en_protocol_mode,oal_uint8 en_rate);
extern oal_void hi1103_get_pwr_comp_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_tx_ratio, oal_int16 * ps_pwr_comp_val);
extern oal_void hi1103_over_temp_handler(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_agc_threshold_handle(hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_rssi);
extern oal_void hi1103_set_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_val);
extern oal_void  hi1103_set_rx_filter_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_rx_filter_command);
extern oal_void hi1103_get_rx_filter(hal_to_dmac_device_stru *pst_hal_device, oal_uint32* pst_reg_value);
extern oal_void  hi1103_set_beacon_timeout_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value);
extern oal_void  hi1103_psm_clear_mac_rx_isr(hal_to_dmac_device_stru *pst_hal_device);

#define HAL_VAP_LEVEL_FUNC_EXTERN
extern oal_void hi1103_vap_tsf_get_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_lo);
extern oal_void hi1103_vap_tsf_set_32bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_lo);
extern oal_void hi1103_vap_tsf_get_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_tsf_hi, oal_uint32 *pul_tsf_lo);
extern oal_void hi1103_vap_tsf_set_64bit(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_tsf_hi, oal_uint32 ul_tsf_lo);
extern oal_void hi1103_vap_send_beacon_pkt(hal_to_dmac_vap_stru *pst_hal_vap, hal_beacon_tx_params_stru *pst_params);
extern oal_void hi1103_vap_set_beacon_rate(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_beacon_rate);
extern oal_void hi1103_vap_beacon_suspend(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_vap_beacon_resume(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_vap_set_machw_prot_params(hal_to_dmac_vap_stru *pst_hal_vap, hal_tx_txop_rate_params_stru *pst_phy_tx_mode, hal_tx_txop_per_rate_params_union *pst_data_rate);


extern oal_void hi1103_vap_set_macaddr(hal_to_dmac_vap_stru * pst_hal_vap, oal_uint8 *puc_mac_addr);
extern oal_void hi1103_vap_set_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);

extern oal_void hi1103_vap_clr_opmode(hal_to_dmac_vap_stru *pst_hal_vap, wlan_vap_mode_enum_uint8 en_vap_mode);
extern oal_void hi1103_vap_set_machw_aifsn_all_ac(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               uc_bk,
                oal_uint8               uc_be,
                oal_uint8               uc_vi,
                oal_uint8               uc_vo);
extern oal_void hi1103_vap_set_machw_aifsn_ac(hal_to_dmac_vap_stru         *pst_hal_vap,
                                            wlan_wme_ac_type_enum_uint8   en_ac,
                                            oal_uint8                     uc_aifs);
extern oal_void  hi1103_vap_set_machw_aifsn_ac_wfa(hal_to_dmac_vap_stru         *pst_hal_vap,
                                      wlan_wme_ac_type_enum_uint8   en_ac,
                                      oal_uint8                     uc_aifs,
                                      wlan_wme_ac_type_enum_uint8   en_wfa_lock);
extern oal_void hi1103_vap_set_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin, oal_uint8 uc_ac_type);
extern oal_void  hi1103_vap_set_edca_machw_cw_wfa(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmaxmin, oal_uint8 uc_ac_type, wlan_wme_ac_type_enum_uint8   en_wfa_lock);
extern oal_void hi1103_vap_get_edca_machw_cw(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin, oal_uint8 uc_ac_type);
#if 0
extern oal_void hi1103_vap_set_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1103_vap_get_machw_cw_bk(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1103_vap_set_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1103_vap_get_machw_cw_be(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1103_vap_set_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1103_vap_get_machw_cw_vi(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
extern oal_void hi1103_vap_set_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_cwmax, oal_uint8 uc_cwmin);
extern oal_void hi1103_vap_get_machw_cw_vo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_cwmax, oal_uint8 *puc_cwmin);
#endif
extern oal_void hi1103_vap_set_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);
extern oal_void hi1103_vap_get_machw_txop_limit_bkbe(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1103_vap_set_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1103_vap_get_machw_txop_limit_vivo(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1103_vap_set_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_be, oal_uint16 us_bk);
extern oal_void hi1103_vap_get_machw_edca_bkbe_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_be, oal_uint16 *pus_bk);
extern oal_void hi1103_vap_set_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_vo, oal_uint16 us_vi);
extern oal_void hi1103_vap_get_machw_edca_vivo_lifetime(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 *pus_vo, oal_uint16 *pus_vi);
extern oal_void hi1103_vap_set_machw_prng_seed_val_all_ac(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_vap_read_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_value);
extern oal_void hi1103_vap_write_tbtt_timer(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_value);
extern oal_void hi1103_vap_set_machw_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void hi1103_vap_update_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_beacon_period);
extern oal_void  hi1103_vap_get_beacon_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_beacon_period);
extern oal_void hi1103_aon_tsf_disable(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void  hi1103_vap_set_noa(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint32              ul_start_tsf,
                oal_uint32              ul_duration,
                oal_uint32              ul_interval,
                oal_uint8               uc_count);
extern oal_void  hi1103_vap_set_noa_timeout_val(hal_to_dmac_vap_stru   *pst_hal_vap, oal_uint16 us_value);
extern oal_void  hi1103_vap_set_noa_offset(hal_to_dmac_vap_stru   *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1103_vap_set_ext_noa_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_offset);
extern oal_void  hi1103_vap_set_ext_noa_disable(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void  hi1103_vap_set_ext_noa_enable(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_vap_set_ext_noa_para(hal_to_dmac_vap_stru   *pst_hal_vap,
                                                   oal_uint32       ul_duration,
                                                   oal_uint32       ul_interval);


#ifdef _PRE_WLAN_FEATURE_P2P
extern oal_void  hi1103_vap_set_ops(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_uint8               en_ops_ctrl,
                oal_uint8               uc_ct_window);
extern oal_void  hi1103_vap_enable_p2p_absent_suspend(
                hal_to_dmac_vap_stru   *pst_hal_vap,
                oal_bool_enum_uint8     en_suspend_enable);
#endif
extern oal_void hi1103_set_sta_bssid(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_byte);
extern oal_void hi1103_set_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_period);
extern oal_void hi1103_get_sta_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 *pul_dtim_period);
extern oal_void hi1103_set_sta_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint32 ul_dtim_count);
extern oal_void  hi1103_get_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *uc_dtim_count);
extern oal_void  hi1103_set_psm_dtim_count(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_count);
extern oal_void  hi1103_set_psm_dtim_period(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_dtim_period,
                                                oal_uint8 uc_listen_intvl_to_dtim_times, oal_bool_enum_uint8 en_receive_dtim);
extern oal_bool_enum hi1103_check_sleep_time(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_aon_tsf_disable_all(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_enable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap, oal_bool_enum_uint8 en_dbac_enable);
extern oal_void hi1103_disable_tsf_tbtt(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_mwo_det_enable_mac_counter(hal_to_dmac_device_stru *pst_hal_device, oal_int32 l_enable_reg_val);
#ifdef _PRE_WLAN_1103_PILOT
extern oal_void  hi1103_tx_enable_resp_ps_bit_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void  hi1103_tx_enable_resp_ps_bit_ctrl_all(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void  hi1103_tx_disable_resp_ps_bit_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void  hi1103_tx_disable_resp_ps_bit_ctrl_all(hal_to_dmac_device_stru *pst_hal_device);
#endif
extern oal_void hi1103_tx_enable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1103_tx_disable_peer_sta_ps_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_lut_index);
extern oal_void hi1103_cfg_slottime_type(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_slottime_type);
extern oal_void  hi1103_get_hw_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_cali_check_hw_status);
extern oal_void hi1103_pm_wlan_get_service_id(hal_to_dmac_vap_stru  *pst_hal_vap, oal_uint8 *uc_service_id);
extern oal_void hi1103_pm_enable_front_end(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_enable_paldo);
extern oal_void hi1103_pm_disable_front_end_tx(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_pm_wlan_servid_register(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_pm_wlan_servid_unregister(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_pm_vote2platform(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_vote_state);
extern oal_void hi1103_init_pm_info(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_pm_set_tbtt_offset(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint16 us_adjust_val);
extern oal_void hi1103_pm_set_bcn_rf_chain(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 uc_bcn_rf_chain);
extern oal_void hi1103_dyn_tbtt_offset_switch(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_switch);

#ifdef _PRE_PM_TBTT_OFFSET_PROBE
extern oal_void hi1103_tbtt_offset_probe_init(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe_destroy(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe_suspend(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe_resume(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe_tbtt_cnt_incr(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe_beacon_cnt_incr(hal_to_dmac_vap_stru *pst_hal_vap);
extern oal_void hi1103_tbtt_offset_probe(hal_to_dmac_vap_stru *pst_hal_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
extern oal_void hi1103_coex_irq_en_set(oal_uint8 uc_intr_en);
extern oal_void hi1103_coex_sw_irq_clr_set(oal_uint8 uc_irq_clr);
extern oal_void hi1103_coex_sw_irq_set(hal_coex_sw_irq_type_enum_uint8 en_coex_irq_type);
extern oal_void hi1103_coex_sw_irq_status_get(oal_uint8 *uc_irq_status);
extern oal_void hi1103_get_btcoex_abort_qos_null_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_qosnull_seq_num);
extern oal_void hi1103_get_btcoex_occupied_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 *us_occupied_period);
extern oal_void hi1103_get_btcoex_pa_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *ul_pa_status);
extern oal_void hi1103_update_btcoex_btble_status(hal_to_dmac_chip_stru *pst_hal_chip);
extern oal_void hi1103_btcoex_update_btble_status(hal_to_dmac_chip_stru *pst_hal_chip);
extern oal_uint32 hi1103_btcoex_init(hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32 hi1103_btcoex_sw_preempt_init(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_get_btcoex_statistic(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable_abort_stat);
extern oal_uint32 hi1103_mpw_soc_write_reg(oal_uint32 ulQuryRegAddrTemp, oal_uint16 usQuryRegValueTemp);
extern oal_void hi1103_btcoex_update_ap_beacon_count(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_beacon_count);
extern oal_void hi1103_btcoex_post_event(hal_to_dmac_chip_stru *pst_hal_chip, hal_dmac_misc_sub_type_enum_uint8 en_sub_type);
extern oal_void hi1103_btcoex_have_small_ampdu(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_have_ampdu);
extern oal_void hi1103_btcoex_process_bt_status(hal_to_dmac_chip_stru *pst_hal_chip, oal_uint8 uc_print);
extern oal_void hi1103_btcoex_set_wl0_antc_switch(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_tx_slv);
#ifdef _PRE_WLAN_FEATURE_BTCOEX_SLV_TX_BUGFIX
extern oal_void hi1103_btcoex_set_wl0_tx_slv_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_tx_slv);
extern oal_void hi1103_btcoex_set_pta0_wl0_selected_sel(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_sw_ctl);
extern oal_void hi1103_btcoex_set_wl0_rx_status_byp(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_rx_byp);
#endif
extern oal_void hi1103_btcoex_get_bt_acl_status(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_acl_status);
extern oal_void hi1103_btcoex_get_bt_sco_status(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_sco_status);
extern oal_void hi1103_btcoex_get_slna_status(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 *en_sco_status);
extern oal_void hi1103_btcoex_get_ps_service_status(hal_to_dmac_device_stru *pst_hal_device, hal_btcoex_ps_status_enum_uint8 *en_ps_status);
extern oal_void hi1103_btcoex_set_slna_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_slna);
extern oal_void hi1103_btcoex_set_ba_resp_pri(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_occu);
#ifdef _PRE_WLAN_1103_PILOT

#else
extern oal_void hi1103_btcoex_open_5g_upc(oal_void);
#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX
extern oal_void  hi1103_ltecoex_req_mask_ctrl(oal_uint16 req_mask_ctrl);
#endif
extern oal_void hi1103_set_btcoex_abort_preempt_frame_param(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_preempt_param);
extern oal_void hi1103_set_btcoex_abort_null_buff_addr(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_abort_null_buff_addr);
extern oal_void hi1103_set_btcoex_tx_abort_preempt_type(hal_to_dmac_device_stru *pst_hal_device, hal_coex_hw_preempt_mode_enum_uint8 en_preempt_type);
extern oal_void hi1103_set_btcoex_abort_qos_null_seq_num(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_qosnull_seq_num);
extern oal_void hi1103_set_btcoex_hw_rx_priority_dis(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_hw_rx_prio_dis);
extern oal_void hi1103_set_btcoex_hw_priority_en(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_hw_prio_en);
extern oal_void hi1103_set_btcoex_priority_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_priority_period);
extern oal_void hi1103_set_btcoex_occupied_period(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_occupied_period);
extern oal_void hi1103_btcoex_get_rf_control(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 ul_occupied_period, oal_uint32 *pul_wlbt_mode_sel, oal_uint16 us_wait_cnt);
extern oal_void hi1103_set_btcoex_sw_all_abort_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_sw_abort_ctrl);
extern oal_void hi1103_set_btcoex_sw_priority_flag(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_sw_prio_flag);
extern oal_void hi1103_set_btcoex_soc_gpreg0(oal_uint8 uc_val, oal_uint16 us_mask, oal_uint8 uc_offset);
extern oal_void hi1103_set_btcoex_soc_gpreg1(oal_uint8 uc_val, oal_uint16 us_mask, oal_uint8 uc_offset);
extern oal_void hi1103_btcoex_restore_reg(oal_void);
extern oal_void hi1103_btcoex_recover_reg(oal_void);
#endif

extern oal_void hi1103_tx_get_dscr_iv_word(hal_tx_dscr_stru *pst_dscr, oal_uint32 *pul_iv_ms_word, oal_uint32 *pul_iv_ls_word, oal_uint8 uc_chiper_type, oal_uint8 uc_chiper_keyid);
#ifdef _PRE_WLAN_DFT_STAT
extern oal_void  hi1103_dft_get_machw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pst_machw_stat,oal_uint8 us_bank_select, oal_uint32 *pul_len);
extern oal_void  hi1103_dft_set_phy_stat_node(hal_to_dmac_device_stru * pst_hal_device,oam_stats_phy_node_idx_stru *pst_phy_node_idx);
extern oal_void  hi1103_dft_get_phyhw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pst_phyhw_stat,oal_uint8 us_bank_select, oal_uint32 *pul_len);
extern oal_void  hi1103_dft_get_rfhw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint32 *pul_rfhw_stat, oal_uint32 *pul_len, oal_uint8 uc_rf_select);
extern oal_void hi1103_dft_get_sochw_stat_info(hal_to_dmac_device_stru * pst_hal_device,oal_uint16 *pst_sochw_stat, oal_uint32 *pul_len, oal_uint8 uc_bank_select);
extern oal_void  hi1103_dft_print_machw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1103_dft_print_phyhw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1103_dft_print_rfhw_stat(hal_to_dmac_device_stru * pst_hal_device);
extern oal_void  hi1103_dft_report_all_reg_state(hal_to_dmac_device_stru   *pst_hal_device);

#endif
extern oal_void hi1103_set_lte_gpio_mode(oal_uint32 ul_mode_value);

extern oal_void hi1103_cfg_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1103_get_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,
                    oal_uint8 uc_chain_idx, wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1103_revert_cw_signal_reg(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_band_enum_uint8 en_band);
extern oal_void hi1103_check_test_value_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_value, oal_uint32 *pul_result);
extern oal_void hi1103_config_always_rx(hal_to_dmac_device_stru *pst_hal_device_base, oal_uint8 uc_switch);
extern oal_uint32 hi1103_rf_get_pll_div_idx(wlan_channel_band_enum_uint8 en_band,oal_uint8  uc_channel_idx,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth,oal_uint8  *puc_pll_div_idx);
extern oal_void hi1103_get_cali_info(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_param);
extern oal_void hi1103_get_rate_idx_pow(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_pow_idx,
                                         wlan_channel_band_enum_uint8 en_freq_band, oal_uint16 *pus_powr, oal_uint8 uc_channel_idx);
extern oal_void hi1103_get_target_tx_power_by_tx_dscr(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_ctrl_one_param *pst_tx_dscr_one,
                                                         hal_pdet_info_stru *pst_pdet_info, oal_int16 *ps_tx_pow);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_int16  hi1103_rf_cali_cal_20log(oal_int16 s_vdet_val);
extern oal_void hi1103_load_ini_power_gain(oal_void);
extern oal_void hi1103_config_update_scaling_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16* paus_dbb_scale);
extern oal_void hi1103_config_update_dsss_scaling_reg(hal_to_dmac_device_stru *pst_hal_device, oal_uint16* paus_dbb_scale, oal_uint8  uc_distance);
extern oal_uint32 hi1103_config_custom_rf(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 * puc_param);
extern oal_uint32 hi1103_config_custom_dts_cali(oal_uint8 * puc_param);
extern oal_void hi1103_config_set_cus_nvram_params(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 * puc_param);
extern oal_void hi1103_config_update_rate_pow_table(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_config_get_cus_nvram_params(hal_cfg_custom_nvram_params_stru **ppst_cfg_nvram);
extern oal_void hi1103_config_get_cus_cca_param(hal_cfg_custom_cca_stru **ppst_cfg_cca);
extern oal_void hi1103_config_get_far_dist_dsss_scale_promote_switch(oal_uint8 *puc_switch);
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_uint32 hi1103_config_custom_dyn_cali(oal_uint8 * puc_param);
#endif
#endif

extern oal_void  hi1103_al_tx_hw(hal_to_dmac_device_stru *pst_hal_device, hal_al_tx_hw_stru *pst_al_tx_hw);
extern oal_void  hi1103_al_tx_hw_cfg(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_mode, oal_uint32 ul_rate);
#ifdef _PRE_WLAN_FEATURE_FTM
extern oal_uint64  hi1103_get_ftm_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 ull_time);
extern oal_uint64  hi1103_check_ftm_t4(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 ull_time);
extern oal_int8  hi1103_get_ftm_t4_intp(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 ull_time);
extern oal_uint64  hi1103_check_ftm_t2(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 ull_time);
extern oal_int8  hi1103_get_ftm_t2_intp(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 ull_time);
extern oal_void hi1103_get_ftm_tod(hal_to_dmac_device_stru *pst_hal_device, oal_uint64 * pull_tod);
extern oal_void hi1103_get_ftm_toa(hal_to_dmac_device_stru * pst_hal_device, oal_uint64 * pull_toa);
extern oal_void hi1103_get_ftm_t2(hal_to_dmac_device_stru * pst_hal_device, oal_uint64 * pull_t2);
extern oal_void hi1103_get_ftm_t3(hal_to_dmac_device_stru * pst_hal_device, oal_uint64 * pull_t3);
extern oal_void hi1103_get_ftm_ctrl_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_ftm_status);
extern oal_void hi1103_get_ftm_config_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_ftm_status);
extern oal_void hi1103_set_ftm_ctrl_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_ftm_status);
extern oal_void hi1103_set_ftm_config_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 ul_ftm_status);
extern oal_void hi1103_set_ftm_enable(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_ftm_status);
extern oal_void hi1103_set_ftm_sample (hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_ftm_status);
extern oal_void hi1103_get_ftm_dialog(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_dialog);
extern oal_void hi1103_get_ftm_cali_rx_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_ftm_cali_rx_time);
extern oal_void hi1103_get_ftm_cali_rx_intp_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_ftm_cali_rx_time);
extern oal_void hi1103_get_ftm_cali_tx_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_ftm_cali_tx_time);
extern oal_void hi1103_set_ftm_cali(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr, oal_bool_enum_uint8 en_ftm_cali);
extern oal_void hi1103_set_ftm_tx_cnt(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr, oal_uint8 uc_ftm_tx_cnt);
extern oal_void hi1103_set_ftm_bandwidth(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr, wlan_bw_cap_enum_uint8 en_band_cap);
extern oal_void hi1103_set_ftm_protocol(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru * pst_tx_dscr, wlan_phy_protocol_enum_uint8 uc_prot_format);
extern oal_void hi1103_set_ftm_m2s(hal_to_dmac_device_stru * pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_tx_chain_selection);
extern oal_void hi1103_set_ftm_m2s_phy(hal_to_dmac_device_stru * pst_hal_device, oal_bool_enum_uint8 en_is_mimo, oal_uint8 uc_tx_chain_selection);
extern oal_void hi1103_get_ftm_rtp_reg(hal_to_dmac_device_stru *pst_hal_device,
                                              oal_uint32 *pul_reg0,
                                              oal_uint32 *pul_reg1,
                                              oal_uint32 *pul_reg2,
                                              oal_uint32 *pul_reg3,
                                              oal_uint32 *pul_reg4);

#endif
extern oal_void  hi1103_vap_get_gtk_rx_lut_idx(hal_to_dmac_vap_stru *pst_hal_vap, oal_uint8 *puc_lut_idx);

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_void hi1103_rf_init_dyn_cali_reg_conf(hal_to_dmac_device_stru *pst_hal_device);
extern oal_int16 hi1103_get_tx_pdet_by_pow(hal_to_dmac_device_stru * OAL_CONST pst_hal_device, hal_pdet_info_stru * OAL_CONST pst_pdet_info,
                                                hal_dyn_cali_usr_record_stru * OAL_CONST pst_user_pow, oal_int16 *pst_exp_pdet);
extern oal_void hi1103_rf_dyn_cali_enable(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_rf_dyn_cali_disable(hal_to_dmac_device_stru * pst_device);
extern oal_void hi1103_config_set_dyn_cali_dscr_interval(hal_to_dmac_device_stru * pst_hal_device, wlan_channel_band_enum_uint8 uc_band, oal_uint16 us_param_val);
#ifdef _PRE_WLAN_DPINIT_CALI
extern oal_void  hi1103_config_get_dyn_cali_dpinit_val(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_param_val);
#endif
extern oal_void hi1103_rf_cali_realtime_entrance(hal_to_dmac_device_stru * OAL_CONST pst_hal_device, hal_pdet_info_stru * OAL_CONST pst_pdet_info,
                                                            hal_dyn_cali_usr_record_stru * OAL_CONST pst_user_pow,  hal_tx_dscr_stru * OAL_CONST pst_base_dscr);
extern oal_void hi1103_init_dyn_cali_tx_pow(hal_to_dmac_device_stru *pst_hal_device);
#endif
#ifdef _PRE_WLAN_DFT_REG
extern oal_uint32 hi1103_debug_refresh_reg_ext(hal_to_dmac_device_stru *pst_hal_device, oam_reg_evt_enum_uint32 en_evt_type, oal_uint32 *pul_ret);
extern oal_void hi1103_debug_frw_evt(hal_to_dmac_device_stru *pst_hal_device);
#endif

extern oal_void hi1103_get_wow_enable_status(hal_to_dmac_device_stru *pst_hal_device, oal_uint32 *pul_status);

#ifdef _PRE_WLAN_CHIP_TEST
extern oal_void hi1103_set_tx_dscr_long_nav_enable(hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 uc_en_status);
#endif

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
extern oal_void hi1103_get_tx_msdu_address_params(hal_tx_dscr_stru *pst_dscr, hal_tx_msdu_address_params **ppst_tx_dscr_msdu_subtable, oal_uint8 *puc_msdu_num);
#endif
extern oal_void hi1103_show_wow_state_info(hal_to_dmac_device_stru *pst_hal_device);
extern oal_void hi1103_flush_tx_complete_irq(hal_to_dmac_device_stru *pst_hal_dev);
extern oal_void hi1103_flush_rx_queue_complete_irq(hal_to_dmac_device_stru *pst_hal_dev, hal_rx_dscr_queue_id_enum_uint8 en_queue_num);
extern oal_bool_enum_uint8  hi1103_check_mac_int_status(hal_to_dmac_device_stru *pst_hal_dev);
extern oal_void  hi1103_device_init_vap_pow_code(hal_to_dmac_device_stru   *pst_hal_device,
                                            hal_vap_pow_info_stru            *pst_vap_pow_info,
                                            oal_uint8                         uc_chan_idx,
                                            wlan_channel_band_enum_uint8      en_freq_band,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth,
                                            hal_pow_set_type_enum_uint8       uc_type,
                                            oal_uint8                         uc_chan_num);
extern oal_void hi1103_device_get_tx_pow_from_rate_idx(hal_to_dmac_device_stru * pst_hal_device, hal_user_pow_info_stru *pst_hal_user_pow_info,
                                wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_cur_ch_num, oal_uint8 uc_cur_rate_pow_idx,
                                hal_tx_txop_tx_power_stru *pst_tx_power, oal_int16 *ps_tx_pow);

extern oal_void hi1103_device_enable_mac1(oal_void);
extern oal_void hi1103_device_disable_mac1(oal_void);
extern oal_void hi1103_device_enable_phy1(oal_void);
extern oal_void hi1103_device_disable_phy1(oal_void);
#ifdef _PRE_WLAN_1103_CHR
extern oal_void hi1103_get_phy_mac_chr_info(hal_to_dmac_device_stru *pst_hal_device, hal_phy_mac_chr_info_stru *pst_phy_mac_chr_info);
#endif
extern oal_uint32 hi1103_get_subband_index(wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_channel_idx, oal_uint8 *puc_subband_idx);
#endif
#ifdef _PRE_WLAN_1103_PILOT
extern oal_void hi1103_pow_set_pow_to_pow_code(hal_to_dmac_device_stru *pst_hal_device,
                                               wlan_channel_band_enum_uint8 en_freq_band, oal_uint8 uc_chan_idx,
                                               oal_uint8 uc_chan_num, wlan_channel_bandwidth_enum_uint8 en_bandwidth,
                                               hal_pow_set_type_enum_uint8 uc_type);
#endif
extern oal_void hi1103_set_abort_timers_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_abort_cnt);
extern oal_void hi1103_get_abort_timers_cnt(hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *uc_abort_cnt);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif

