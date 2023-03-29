

#ifndef __HMAC_ROAM_MAIN_H__
#define __HMAC_ROAM_MAIN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_MAIN_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define ROAM_SCAN_TIME_MAX        (3 * 1000)       /* 扫描超时时间 单位ms*/
#define ROAM_CONNECT_TIME_MAX     (10 * 1000)       /* 关联超时时间 单位ms*/
#define ROAM_INVALID_SCAN_MAX     (5)              /* 连续无效扫描门限   */

/* 漫游场景识别一，识别出密集AP场景和低密AP场景 */
#define WLAN_FULL_CHANNEL_NUM           (20)       /* 判断全信道扫描的数目门限 */
#define ROAM_ENV_CANDIDATE_GOOD_NUM     (5)        /* 统计漫游环境中强信号强度AP的个数 */
#define ROAM_ENV_CANDIDATE_WEAK_NUM     (10)       /* 统计漫游环境中弱信号强度AP的个数 */
#define ROAM_ENV_RSSI_NE60_DB          (-60)       /* 漫游环境中强信号强度AP RSSI的门限 */
#define ROAM_ENV_DENSE_TO_SPARSE_PERIOD (5)        /* 从高密场景向低密场景切换的扫描周期 */

/* 漫游场景识别二，识别出不满足漫游门限、但是有更好信号强度AP的场景 */
#define ROAM_ENV_BETTER_RSSI_PERIOD      (4)        /* 不满足漫游门限，但是有更好信号强度AP的扫描周期 */
#define ROAM_ENV_BETTER_RSSI_NULL_PERIOD (1)        /* 不满足漫游门限，但是有更好信号强度AP的扫描周期扫不到合适AP的周期 */
#define ROAM_ENV_BETTER_RSSI_DISTANSE   (20)       /* 同一个ESS中最强信号强度AP和当前关联AP的RSSI间隔 */

#define ROAM_FAIL_FIVE_TIMES            (100)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* 漫游触发条件 */
typedef enum
{
    ROAM_TRIGGER_DMAC                 = 0,
    ROAM_TRIGGER_APP                  = 1,
    ROAM_TRIGGER_COEX                 = 2,
    ROAM_TRIGGER_11V                  = 3,
    ROAM_TRIGGER_M2S                  = 4,

    ROAM_TRIGGER_BUTT
}roam_trigger_condition_enum;
typedef oal_uint8 roam_trigger_enum_uint8;

/* DMAC触发的漫游场景类型 */
typedef enum
{
    ROAM_ENV_SPARSE_AP            = 0,  /* default AP roaming environment, based on [-78, -74] RSSI threshold */
    ROAM_ENV_LINKLOSS             = 1,
    ROAM_ENV_DENSE_AP             = 2,  /* dense AP roaming environment, based on [-72, -68] RSSI threshold */

    ROAM_ENV_BUTT
}roam_scenario_enum;
typedef oal_uint8 roam_scenario_enum_uint8;

/* 漫游主状态机状态 */
typedef enum
{
    ROAM_MAIN_STATE_INIT               = 0,
    ROAM_MAIN_STATE_FAIL               = ROAM_MAIN_STATE_INIT,
    ROAM_MAIN_STATE_SCANING            = 1,
    ROAM_MAIN_STATE_CONNECTING         = 2,
    ROAM_MAIN_STATE_UP                 = 3,

    ROAM_MAIN_STATE_BUTT
}roam_main_state_enum;
typedef oal_uint8  roam_main_state_enum_uint8;

/* 漫游主状态机事件类型 */
typedef enum
{
    ROAM_MAIN_FSM_EVENT_START          = 0,
    ROAM_MAIN_FSM_EVENT_SCAN_RESULT    = 1,
    ROAM_MAIN_FSM_EVENT_START_CONNECT  = 2,
    ROAM_MAIN_FSM_EVENT_CONNECT_FAIL   = 3,
    ROAM_MAIN_FSM_EVENT_HANDSHAKE_FAIL = 4,
    ROAM_MAIN_FSM_EVENT_CONNECT_SUCC   = 5,
    ROAM_MAIN_FSM_EVENT_TIMEOUT        = 6,
    ROAM_MAIN_FSM_EVENT_TYPE_BUTT
}roam_main_fsm_event_type_enum;

#define ROAM_BAND_2G_BIT BIT0
#define ROAM_BAND_5G_BIT BIT1

/* 漫游扫描信道正交属性参数,命令行传入*/
typedef enum
{
    ROAM_SCAN_CHANNEL_ORG_0       = 0, /* no scan */
    ROAM_SCAN_CHANNEL_ORG_1       = 1, /* scan only one channel */
    ROAM_SCAN_CHANNEL_ORG_3       = 2, /* 2.4G channel 1\6\11 */
    ROAM_SCAN_CHANNEL_ORG_4       = 3, /* 2.4G channel 1\5\9\13 */
    ROAM_SCAN_CHANNEL_ORG_BUTT
}roam_channel_org_enum;
typedef oal_uint8  roam_channel_org_enum_uint8;
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
/* 漫游配置结构体 */
typedef struct
{
    oal_uint8                       uc_scan_band;                /* 扫描频段 */
    roam_channel_org_enum_uint8     uc_scan_orthogonal;          /* 扫描信道正交属性 */
    oal_int8                        c_trigger_rssi_2G;           /* 2G时的触发门限 */
    oal_int8                        c_trigger_rssi_5G;           /* 5G时的触发门限 */
    oal_uint8                       uc_delta_rssi_2G;            /* 2G时的增益门限 */
    oal_uint8                       uc_delta_rssi_5G;            /* 5G时的增益门限 */
    oal_int8                        c_dense_env_trigger_rssi_2G; /* 密集AP环境中2G时的触发门限 */
    oal_int8                        c_dense_env_trigger_rssi_5G; /* 密集AP环境中5G时的触发门限 */
    oal_bool_enum_uint8             uc_scenario_enable;          /* 是否使能场景识别 */
    oal_int8                        c_candidate_good_rssi;       /* 密集AP场景识别，强信号强度AP的门限 */
    oal_uint8                       uc_candidate_good_num;       /* 密集AP场景识别，强信号强度AP的数目 */
    oal_uint8                       uc_candidate_weak_num;       /* 密集AP场景识别，弱信号强度AP的数目 */
    oal_uint32                      ul_blacklist_expire_sec;     /* not used for now */
    oal_uint32                      ul_buffer_max;               /* not used for now */
}hmac_roam_config_stru;

/* 漫游统计结构体 */
typedef struct
{
    oal_uint32                 ul_trigger_rssi_cnt;             /* rssi触发漫游扫描计数 */
    oal_uint32                 ul_trigger_linkloss_cnt;         /* linkloss触发漫游扫描计数 */
    oal_uint32                 ul_scan_cnt;                     /* 漫游扫描次数 */
    oal_uint32                 ul_scan_result_cnt;              /* 漫游扫描返回次数 */
    oal_uint32                 ul_connect_cnt;                  /* 漫游连接计数 */
    oal_uint32                 ul_roam_old_cnt;                 /* 漫游失败计数 */
    oal_uint32                 ul_roam_new_cnt;                 /* 漫游成功计数 */
    oal_uint32                 ul_roam_scan_fail;
#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
    oal_uint32                 ul_roam_11v_scan_fail;
#endif
    oal_uint32                 ul_roam_eap_fail;
}hmac_roam_static_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
oal_uint32 hmac_roam_enable_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_enable);
oal_uint32 hmac_roam_band_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_scan_band);
oal_uint32 hmac_roam_org_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_scan_orthogonal);
oal_uint32 hmac_roam_check_signal_bridge_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_start_etc(hmac_vap_stru *pst_hmac_vap, roam_channel_org_enum uc_scan_type,
                           oal_bool_enum_uint8 en_current_bss_ignore, roam_trigger_enum_uint8 en_roam_trigger);
oal_uint32 hmac_roam_show_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_init_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_info_init_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_exit_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_test(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_resume_user_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_param);
oal_uint32 hmac_roam_pause_user_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_param);
oal_uint32 hmac_sta_roam_rx_mgmt_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_param);
oal_uint32 hmac_roam_trigger_handle_etc(hmac_vap_stru *pst_hmac_vap, oal_int8 c_rssi, oal_bool_enum_uint8 en_current_bss_ignore);
oal_void hmac_roam_tbtt_handle_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_scan_complete_etc(hmac_vap_stru *pst_hmac_vap);
oal_void hmac_roam_connect_complete_etc(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result);
oal_void  hmac_roam_add_key_done_etc(hmac_vap_stru *pst_hmac_vap);
oal_void  hmac_roam_wpas_connect_state_notify_etc(hmac_vap_stru *pst_hmac_vap, wpas_connect_state_enum_uint32 conn_state);
oal_uint32 hmac_roam_rssi_trigger_type(hmac_vap_stru *pst_hmac_vap, roam_scenario_enum_uint8 en_val);
oal_uint32 hmac_roam_check_bkscan_result_etc(hmac_vap_stru *pst_hmac_vap, void *p_scan_record);
oal_int8 hmac_get_rssi_from_scan_result(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_bssid);
#ifdef _PRE_WLAN_FEATURE_11R
oal_uint32 hmac_roam_reassoc_etc(hmac_vap_stru *pst_hmac_vap);
oal_uint32 hmac_roam_rx_ft_action_etc(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf);
#endif //_PRE_WLAN_FEATURE_11R
oal_void hmac_roam_timeout_test_etc(hmac_vap_stru *pst_hmac_vap);

#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_roam_main.h */
