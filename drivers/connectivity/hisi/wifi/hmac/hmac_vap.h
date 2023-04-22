

#ifndef __HMAC_VAP_H__
#define __HMAC_VAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "mac_resource.h"
#ifdef _PRE_WLAN_TCP_OPT
#include "hmac_tcp_opt_struc.h"
#include "oal_hcc_host_if.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "hmac_btcoex.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VAP_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#ifdef _PRE_WLAN_DFT_STAT
#define   HMAC_VAP_DFT_STATS_PKT_INCR(_member, _cnt)        ((_member) += (_cnt))
#else
#define   HMAC_VAP_DFT_STATS_PKT_INCR(_member, _cnt)
#endif
#define   HMAC_VAP_STATS_PKT_INCR(_member, _cnt)            ((_member) += (_cnt))

#ifdef _PRE_WLAN_FEATURE_HS20
#define MAX_QOS_UP_RANGE  8
#define MAX_DSCP_EXCEPT   21  /* maximum of DSCP Exception fields for QoS Map set */
#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
#define  HMAC_HIPRIV_ACK_BUF_SIZE  3
#endif
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
    初始化vap特性枚举
*****************************************************************************/
typedef enum
{
    HMAC_ADDBA_MODE_AUTO,
    HMAC_ADDBA_MODE_MANUAL,

    HMAC_ADDBA_MODE_BUTT
}hmac_addba_mode_enum;
typedef oal_uint8 hmac_addba_mode_enum_uint8;


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
/* 声明hmac_vap 结构体定义 */
struct hmac_vap_tag;
typedef struct hmac_vap_tag hmac_vap_stru;


typedef struct
{
    oal_dlist_head_stru st_timeout_head;
}hmac_mgmt_timeout_stru;

typedef struct
{
    oal_uint16                  us_user_index;
    mac_vap_state_enum_uint8    en_state;
    oal_uint8                   uc_vap_id;
    mac_status_code_enum_uint16 en_status_code;
    oal_uint16                  auc_rsv[2];
}hmac_mgmt_timeout_param_stru;

#ifdef _PRE_WLAN_FEATURE_HS20
typedef struct
{
    oal_uint8  auc_up_low[MAX_QOS_UP_RANGE];             /* User Priority */
    oal_uint8  auc_up_high[MAX_QOS_UP_RANGE];
    oal_uint8  auc_dscp_exception_up[MAX_DSCP_EXCEPT];   /* User Priority of DSCP Exception field */
    oal_uint8  uc_valid;
    oal_uint8  uc_num_dscp_except;
    oal_uint8  auc_dscp_exception[MAX_DSCP_EXCEPT];      /* DSCP exception field  */
}hmac_cfg_qos_map_param_stru;
#endif

/*修改此结构体需要同步通知SDT，否则上报无法解析*/
typedef struct
{

    /***************************************************************************
                                收送包统计
    ***************************************************************************/
    /* 发往lan的数据包统计 */
    oal_uint32  ul_rx_pkt_to_lan;                               /* 接收流程发往以太网的数据包数目，MSDU */
    oal_uint32  ul_rx_bytes_to_lan;                             /* 接收流程发往以太网的字节数 */

   /***************************************************************************
                                发送包统计
    ***************************************************************************/
   /* 从lan接收到的数据包统计 */
   oal_uint32  ul_tx_pkt_num_from_lan;                         /* 从lan过来的包数目,MSDU */
   oal_uint32  ul_tx_bytes_from_lan;                           /* 从lan过来的字节数 */

}hmac_vap_query_stats_stru;
/*装备测试*/
typedef struct
{
    oal_uint32                       ul_rx_pkct_succ_num;                       /*接收数据包数*/
    oal_uint32                       ul_dbb_num;                                /*DBB版本号*/
    oal_uint32                       ul_check_fem_pa_status;                    /*fem和pa是否烧毁标志*/
    oal_int16                        s_rx_rssi;
    oal_bool_enum_uint8              uc_get_dbb_completed_flag;                 /*获取DBB版本号成功上报标志*/
    oal_bool_enum_uint8              uc_check_fem_pa_flag;                      /*fem和pa是否烧毁上报标志*/
    oal_bool_enum_uint8              uc_get_rx_pkct_flag;                       /*接收数据包上报标志位*/
    oal_bool_enum_uint8              uc_lte_gpio_check_flag;                    /*接收数据包上报标志位*/
    oal_bool_enum_uint8              uc_report_efuse_reg_flag;              /*efuse 寄存器读取*/
    oal_uint8                        uc_ant_status : 4,
                                     uc_get_ant_flag : 4;
}hmac_atcmdsrv_get_stats_stru;

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
/*51装备测试 hipriv命令回显*/
typedef struct
{
    oal_uint8                   auc_data[HMAC_HIPRIV_ACK_BUF_SIZE];
    oal_bool_enum_uint8         uc_get_hipriv_ack_flag;
    oal_int8                    *pc_buffer;
    oal_uint8                   auc_reserved[3];
}hmac_hipriv_ack_stats_stru;
#endif
typedef struct
{
    oal_dlist_head_stru           st_entry;
    oal_uint8                     auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_uint8                     uc_reserved[2];
    oal_uint8                     auc_pmkid[WLAN_PMKID_LEN];
}hmac_pmksa_cache_stru;

typedef enum _hmac_tcp_opt_queue_
{
    HMAC_TCP_ACK_QUEUE = 0,
    HMAC_TCP_OPT_QUEUE_BUTT
} hmac_tcp_opt_queue;

#ifdef _PRE_WLAN_TCP_OPT
typedef oal_uint16 (* hmac_trans_cb_func)(hmac_vap_stru *pst_hmac_device, hmac_tcp_opt_queue type,hcc_chan_type dir, oal_netbuf_head_stru* data);
/*tcp_ack优化*/
typedef struct
{
    struct wlan_perform_tcp      hmac_tcp_ack;
    struct wlan_perform_tcp_list hmac_tcp_ack_list;
    wlan_perform_tcp_impls       filter_info;
    hmac_trans_cb_func           filter[HMAC_TCP_OPT_QUEUE_BUTT];	//过滤处理钩子函数
    oal_uint64                   all_ack_count[HMAC_TCP_OPT_QUEUE_BUTT];	//丢弃的TCP ACK统计
    oal_uint64                   drop_count[HMAC_TCP_OPT_QUEUE_BUTT];	//丢弃的TCP ACK统计
    oal_netbuf_head_stru         data_queue[HMAC_TCP_OPT_QUEUE_BUTT];
    oal_spin_lock_stru           data_queue_lock[HMAC_TCP_OPT_QUEUE_BUTT];
}hmac_tcp_ack_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
/* 记录剔除网卡信息结构体 */
typedef struct
{
    oal_uint8  auc_user_mac_addr[6];     /* 保存已删除用户的MAC地址 */
    oal_uint8  auc_rev[2];
}hmac_fbt_disable_user_info_stru;

/* 记录快速切换信息的结构体 */
typedef struct
{
    oal_uint8  uc_fbt_mode;              /* 配置快速切换运行的模式。0：表示关闭，1：表示开启AC集中控制 */
    oal_uint8  uc_disabled_user_cnt;     /* 记录禁止连接列表当前用户的个数 */
    oal_uint8  auc_rev[2];
    hmac_fbt_disable_user_info_stru ast_fbt_disable_connect_user_list[HMAC_FBT_MAX_USER_NUM];  /* 保存32个禁止连接用户的信息，32个资源不够，暂时支持3个用户 */
    frw_timeout_stru st_timer;           /* 已关联用户上报侦听结果使用的定时器 */
}hmac_fbt_mgmt_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
typedef struct hmac_psta_rep
{
    oal_rwlock_stru         st_lock;
    oal_dlist_head_stru     st_vsta_list;   // proxysta device list
    oal_dlist_head_stru     st_msta_list;   // main sta list
    oal_dlist_head_stru     st_pbss_list;   // proxy bss list
    oal_dlist_head_stru     ast_hash[MAC_VAP_PROXY_STA_HASH_MAX_VALUE];
    oal_bool_enum_uint8     en_isolation;
    oal_uint8               uc_use_cnt;
    oal_uint8               uc_resv[2];
} hmac_psta_rep_stru;

typedef struct hmac_psta_mgr
{
    hmac_psta_rep_stru       ast_rep[WLAN_PROXY_STA_MAX_REP];
    oal_proc_dir_entry_stru *pst_proc_entry;
} hmac_psta_mgr_stru;

typedef  struct
{
    oal_dlist_head_stru     st_hash_entry;
    oal_dlist_head_stru     st_xsta_entry;
    oal_uint8               auc_oma[WLAN_MAC_ADDR_LEN];
    oal_uint8               uc_rep_id;
    oal_uint8               auc_resv[1];
} hmac_psta_stru;

#define hmac_vap_psta_oma(vap)  ((vap)->st_psta.auc_oma)
#define hmac_vap_psta_in_rep(vap)   (!oal_dlist_is_empty(&(vap)->st_psta.st_hash_entry))
#endif

/* hmac vap结构体 */
/* 在向此结构体中增加成员的时候，请保持整个结构体8字节对齐*/
typedef struct hmac_vap_tag
{
    /* ap sta公共字段 */
    oal_net_device_stru            *pst_net_device;                             /* VAP对应的net_devices */
    oal_uint8                       auc_name[OAL_IF_NAME_SIZE];                 /* VAP名字*/
    hmac_vap_cfg_priv_stru          st_cfg_priv;                                /* wal hmac配置通信接口 */

    oal_spin_lock_stru              st_lock_state;                              /* 数据面和控制面对VAP状态进行互斥 */
    oal_uint16                      us_user_nums_max;                           /* VAP下可挂接的最大用户个数 */
    oal_uint8                       uc_classify_tid;                            /* 仅在基于vap的流分类使能后有效 */
    wlan_auth_alg_enum_uint8        en_auth_mode;                               /* 认证算法 */

    oal_mgmt_tx_stru                st_mgmt_tx;
    frw_timeout_stru                st_mgmt_timer;
    hmac_mgmt_timeout_param_stru    st_mgmt_timetout_param;

    frw_timeout_stru                st_scan_timeout;                            /* vap发起扫描时，会启动定时器，做超时保护处理 */

    hmac_addba_mode_enum_uint8      en_addba_mode;
#ifdef _PRE_WLAN_FEATURE_WMMAC
    oal_uint8                       uc_ts_dialog_token;                         /* TS会话创建伪随机值 */
#else
    oal_uint8                       uc_resv1;
#endif //_PRE_WLAN_FEATURE_WMMAC
    oal_uint8                       uc_80211i_mode;                             /* 指示当前的方式时WPA还是WPA2, bit0 = 1,WPA; bit1 = 1, RSN */
    oal_uint8                       uc_ba_dialog_token;                         /* BA会话创建伪随机值 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_stru                  st_psta;
#endif
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    mac_blacklist_info_stru         st_blacklist_info;                          /* 黑名单信息 */
    mac_isolation_info_stru         st_isolation_info;                          /* 用户隔离信息 */
#endif
#ifdef _PRE_WLAN_FEATURE_11D
    oal_bool_enum_uint8             en_updata_rd_by_ie_switch;                  /*是否根据关联的ap跟新自己的国家码*/
    oal_uint8                       auc_resv2[3];
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_net_device_stru            *pst_p2p0_net_device;                        /* 指向p2p0 net device */
    oal_net_device_stru            *pst_del_net_device;                         /* 指向需要通过cfg80211 接口删除的 net device */
    oal_work_stru                   st_del_virtual_inf_worker;                  /* 删除net_device 工作队列 */
#endif
#ifdef _PRE_WLAN_FEATURE_HS20
    hmac_cfg_qos_map_param_stru     st_cfg_qos_map_param;
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_netbuf_head_stru            st_tx_queue_head[2];                        /* 2个发送队列，2个线程pinpon操作 */
    oal_uint8                       uc_in_queue_id;
    oal_uint8                       uc_out_queue_id;
    oal_uint8                       auc_resv3[2];
    oal_atomic                      ul_tx_event_num;                            /* frw发包事件的个数 */
    oal_uint32                      ul_tx_quata;                                /* 发包配额，方便配置 */
    oal_spin_lock_stru              st_smp_lock;
#endif

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    oal_uint8                       bit_init_flag:1;                            /* 常发关闭再次打开标志 */
    oal_uint8                       bit_ack_policy:1;                           /* ack policy: 0:normal ack 1:normal ack */
    oal_uint8                       bit_reserved:6;
    oal_uint8                       auc_resv4[3];
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    oal_uint32                     *pul_roam_info;
#endif  //_PRE_WLAN_FEATURE_ROAM
    /* 组播转单播字段 */
#ifdef _PRE_WLAN_FEATURE_MCAST
    oal_void                        *pst_m2u;
#endif

#ifdef _PRE_WLAN_DFT_STAT
    oal_uint8                       uc_device_distance;
    oal_uint8                       uc_intf_state_cca;
    oal_uint8                       uc_intf_state_co;
    oal_uint8                       auc_resv[1];
#endif

    /* sta独有字段 */
    oal_uint8                       bit_sta_protocol_cfg    :   1;
	oal_uint8                       bit_protocol_fall       :   1;              /* 降协议标志位 */
    oal_uint8                       bit_reassoc_flag        :   1;             /* 关联过程中判断是否为重关联动作 */
#ifdef _PRE_WLAN_FEATURE_11K
    oal_uint8                       bit_11k_enable          :   1;
    oal_uint8                       bit_11v_enable          :   1;
    oal_uint8                       bit_11r_enable          :   1;
    oal_uint8                       bit_resv                :   2;
#else
    oal_uint8                       bit_resv                :   5;
#endif //_PRE_WLAN_FEATURE_11K
    oal_int8                        ac_desired_country[3];                      /* 要加入的AP的国家字符串，前两个字节为国家字母，第三个为\0 */
    oal_uint32                      ul_asoc_req_ie_len;
    oal_uint8                      *puc_asoc_req_ie_buff;

    oal_uint8                       uc_wmm_cap;                                 /* 保存与STA关联的AP是否支持wmm能力信息 */
#ifdef _PRE_WLAN_FEATURE_HS20
    oal_uint8                       uc_is_interworking;                         /* 保存与STA关联的AP是否支持interworking能力 */
    oal_uint8                       auc_resv51[3];
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
    oal_uint8                       uc_cfg_sta_pm_manual;                           /* 手动设置sta pm mode的标志 */
#else
    oal_uint8                       auc_resv5[1];
#endif
    oal_uint16                      us_rx_timeout[WLAN_WME_AC_BUTT];            /* 不同业务重排序超时时间 */
    oal_uint16                      us_del_timeout;                             /* 多长时间超时删除ba会话 如果是0则不删除 */
    mac_cfg_mode_param_stru         st_preset_para;                             /* STA协议变更时变更前的协议模式 */
    oal_uint8                       auc_supp_rates[WLAN_MAX_SUPP_RATES];        /* 支持的速率集 */
    oal_uint8                       uc_rs_nrates;   /* 速率个数 */

    oal_uint8                       uc_auth_cnt;                                    /* 记录STA发起关联的次数 */
    oal_uint8                       uc_asoc_cnt;
    oal_uint8                       auc_resv56[2];

    oal_dlist_head_stru             st_pmksa_list_head;

	/* 信息上报 */
    oal_wait_queue_head_stru         query_wait_q;                              /*查询等待队列*/
    oal_station_info_stru            station_info;
    station_info_extend_stru         st_station_info_extend;                    /*CHR2.0使用的STA统计信息*/
    oal_bool_enum_uint8              station_info_query_completed_flag;         /*查询结束标志，OAL_TRUE，查询结束，OAL_FALSE，查询未结束*/
    oal_int16                        s_free_power;                              /* 底噪 */
    oal_uint8                        auc_resv6[1];
    oal_int32                        center_freq;                               /* 中心频点 */
    hmac_atcmdsrv_get_stats_stru     st_atcmdsrv_get_status;

    oal_proc_dir_entry_stru         *pst_proc_dir;                              /* vap对应的proc目录 */

#ifdef _PRE_WLAN_DFT_STAT
    /*统计信息+信息上报新增字段，修改这个字段，必须修改SDT才能解析正确*/
    hmac_vap_query_stats_stru        st_query_stats;
#endif
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    frw_timeout_stru                 st_edca_opt_timer;                         /* edca参数调整定时器 */
    oal_uint32                       ul_edca_opt_time_ms;                       /* edca参数调整计时器周期 */
    oal_uint8                        uc_edca_opt_flag_ap;                       /* ap模式下是否使能edca优化特性 */
    oal_uint8                        uc_edca_opt_flag_sta;                      /* sta模式下是否使能edca优化特性 */
    oal_uint8                        uc_edca_opt_weight_sta;                    /* 调整beacon中edca参数的权重，最大值为 3*/
    oal_uint8                        auc_resv7[1];
#endif

    oal_uint32                        aul_40M_intol_user_bitmap[MAC_DEV_MAX_40M_INTOL_USER_BITMAP_LEN];        /* ap下关联的40M intolerant的sta bitmap */
    frw_timeout_stru                  st_40M_recovery_timer;                    /* 40M恢复定时器 */
    wlan_channel_bandwidth_enum_uint8 en_40M_bandwidth;                         /* 记录ap在切换到20M之前的速率 */

#ifdef _PRE_WLAN_TCP_OPT
    hmac_tcp_ack_stru          st_hamc_tcp_ack[HCC_DIR_COUNT];
#endif

    oal_bool_enum_uint8               en_no_beacon;
    oal_bool_enum_uint8               en_addr_filter;
    oal_bool_enum_uint8               en_amsdu_active;
    oal_bool_enum_uint8               en_amsdu_ampdu_active;
    oal_bool_enum_uint8               en_psm_active;
    oal_bool_enum_uint8               en_wme_active;
    oal_bool_enum_uint8               en_wps_active;
    oal_bool_enum_uint8               en_msdu_defrag_active;
    oal_bool_enum_uint8               en_2040_switch_prohibited;
    oal_bool_enum_uint8               en_tx_aggr_on;
    oal_bool_enum_uint8               en_ampdu_tx_on_switch;
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    oal_uint8                         uc_rx_ba_session_num;                   /* 该vap下，rx BA会话的数目 */
    oal_uint8                         uc_tx_ba_session_num;                   /* 该vap下，tx BA会话的数目 */
    oal_uint8                         auc_resv9[2];
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_h2d_protection_stru          st_prot;
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
    frw_timeout_stru                 st_ps_sw_timer;                             /* 低功耗开关 */
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN                                /* 业务识别功能开关 */
    oal_uint8                        uc_tx_traffic_classify_flag;
    oal_uint8                        auc_resv10[3];
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
    hmac_fbt_mgmt_stru               st_fbt_mgmt;                               /* 记录fbt配置信息、禁止连接列表 */
#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    hmac_hipriv_ack_stats_stru       st_hipriv_ack_stats;
#endif
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    oal_bool_enum_uint8             en_ant_info_query_completed_flag;           /*查询结束标志，OAL_TRUE，查询结束，OAL_FALSE，查询未结束*/
    oal_bool_enum_uint8             en_double_ant_switch_query_completed_flag;  /*查询结束标志，OAL_TRUE，查询结束，OAL_FALSE，查询未结束*/
    oal_uint8                       auc_rsv[2];
    oal_uint32                      ul_double_ant_switch_ret;
#endif
    mac_vap_stru                    st_vap_base_info;                           /* MAC vap，只能放在最后! */
}hmac_vap_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/



/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  hmac_vap_destroy(hmac_vap_stru *pst_vap);
extern oal_uint32  hmac_vap_init(
                       hmac_vap_stru              *pst_hmac_vap,
                       oal_uint8                   uc_chip_id,
                       oal_uint8                   uc_device_id,
                       oal_uint8                   uc_vap_id,
                       mac_cfg_add_vap_param_stru *pst_param);

extern oal_uint32  hmac_vap_creat_netdev(hmac_vap_stru *pst_hmac_vap, oal_int8 *puc_netdev_name, oal_int8 *puc_mac_addr);

extern oal_uint16 hmac_vap_check_ht_capabilities_ap(
            hmac_vap_stru                  *pst_hmac_vap,
            oal_uint8                      *puc_payload,
            oal_uint16                      us_info_elem_offset,
            oal_uint32                      ul_msg_len,
            hmac_user_stru                 *pst_hmac_user_sta);
extern  oal_uint32  hmac_search_ht_cap_ie_ap(
                hmac_vap_stru               *pst_hmac_vap,
                hmac_user_stru              *pst_hmac_user_sta,
                oal_uint8                   *puc_payload,
                oal_uint16                   us_index,
                oal_bool_enum                en_prev_asoc_ht);
extern oal_uint16 hmac_vap_check_vht_capabilities_ap(
                hmac_vap_stru                   *pst_hmac_vap,
                oal_uint8                       *puc_payload,
                oal_uint16                       us_info_elem_offset,
                oal_uint32                       ul_msg_len,
                hmac_user_stru                  *pst_hmac_user_sta);
extern oal_bool_enum_uint8 hmac_vap_addba_check(
                hmac_vap_stru      *pst_hmac_vap,
                hmac_user_stru     *pst_hmac_user,
                oal_uint8           uc_tidno);

extern oal_void hmac_vap_net_stopall(oal_void);
extern oal_void hmac_vap_net_startall(oal_void);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
extern oal_bool_enum_uint8 hmac_flowctl_check_device_is_sta_mode(oal_void);
extern oal_void hmac_vap_net_start_subqueue(oal_uint16 us_queue_idx);
extern oal_void hmac_vap_net_stop_subqueue(oal_uint16 us_queue_idx);
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
extern oal_uint32 hmac_check_opmode_notify(
                hmac_vap_stru                   *pst_hmac_vap,
                oal_uint8                       *puc_mac_hdr,
                oal_uint8                       *puc_payload,
                oal_uint16                       us_info_elem_offset,
                oal_uint32                       ul_msg_len,
                hmac_user_stru                  *pst_hmac_user);
#endif
extern oal_void hmac_handle_disconnect_rsp(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
                                                  mac_reason_code_enum_uint16  en_disasoc_reason);
extern oal_uint8 * hmac_vap_get_pmksa(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_bssid);
oal_uint32 hmac_tx_get_mac_vap(oal_uint8 uc_vap_id, mac_vap_stru **pst_vap_stru);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_vap.h */
