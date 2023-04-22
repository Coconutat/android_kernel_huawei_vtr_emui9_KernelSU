

#ifndef __WLAN_SPEC_1103_H__
#define __WLAN_SPEC_1103_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  其他头文件包含
*****************************************************************************/


/*****************************************************************************
  0.0 定制化变量声明
*****************************************************************************/
/*定制化待设计为一个结构体，并对外提供内联函数访问形态，而不是预编译访问形态*/


/*****************************************************************************
  0.1.2 热点入网功能
*****************************************************************************/
/* 作为P2P GO 允许关联最大用户数 */
#define WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC 4

/*****************************************************************************
  0.5.3 AMSDU功能
*****************************************************************************/
#ifdef _PRE_WIFI_DMT
#define WLAN_AMSDU_MAX_NUM                  witp_dmt_get_amsdu_aggr_num()
#else
/* 一个amsdu下允许拥有的msdu的最大个数 */
#define WLAN_AMSDU_MAX_NUM                  4
#endif

/*****************************************************************************
  0.8.2 STA AP规格
*****************************************************************************/
/* 芯片版本区分 */
#define BOARD_VERSION                              2

/*****************************************************************************
  1.0 WLAN芯片对应的spec
*****************************************************************************/
/* 每个board支持chip的最大个数放入平台 */
/* 每个chip支持device的最大个数放入平台 */
/* 最多支持的MAC硬件设备个数放入平台 */


/*****************************************************************************
  1.3 oam相关的spec
*****************************************************************************/
/*oam 放入平台 */

/*****************************************************************************
  1.4 mem对应的spec
*****************************************************************************/
/*****************************************************************************
  1.4.1 内存池规格
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.2 共享描述符内存池配置信息
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.3 共享管理帧内存池配置信息
*****************************************************************************/
/*内存 spec 放入平台 */


/*****************************************************************************
  1.4.4 共享数据帧内存池配置信息
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.5 本地内存池配置信息
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.6 事件结构体内存池
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.7 用户内存池
*****************************************************************************/
/*****************************************************************************
  1.4.8 MIB内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/
/*内存 spec 放入平台 */

/*****************************************************************************
  1.4.9 netbuf内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/

/*内存 spec 放入平台 */


/*****************************************************************************
  1.4.9.1 sdt netbuf内存池
*****************************************************************************/
/*内存 spec 放入平台 */


/*****************************************************************************
 1.5 frw相关的spec
*****************************************************************************/
/*事件调度 spec 放入平台 */



/*****************************************************************************
  2 宏定义，分类和DR保持一致
*****************************************************************************/
/*****************************************************************************
  2.1 基础协议/定义物理层协议类别的spec
*****************************************************************************/
/*****************************************************************************
  2.1.1 扫描侧STA 功能
*****************************************************************************/
/* TBD 一次可以扫描的最大BSS个数，两个规格可以合并*/
#define WLAN_SCAN_REQ_MAX_BSSID                 2
#define WLAN_SCAN_REQ_MAX_SSID                  8

/* TBD 扫描延迟,us 未使用，可删除*/
#define WLAN_PROBE_DELAY_TIME                   10

/* 扫描时，最小的信道驻留时间，单位ms，变量命名有误*/
#define WLAN_DEFAULT_SCAN_MIN_TIME              110
/* 扫描时，最大的信道驻留时间，单位ms，变量命名有误*/
#define WLAN_DEFAULT_SCAN_MAX_TIME              500

/*TBD  一个device所记录的扫描到的最大BSS个数。过少，需要扩大到200 */
#define WLAN_MAX_SCAN_BSS_NUM                   32
/*TBD  一个信道下记录扫描到的最大BSS个数 。过少，需要扩大到200 */
#define WLAN_MAX_SCAN_BSS_PER_CH                8

#define WLAN_DEFAULT_FG_SCAN_COUNT_PER_CHANNEL         2       /* 前景扫描每信道扫描次数 */
#define WLAN_DEFAULT_BG_SCAN_COUNT_PER_CHANNEL         1       /* 背景扫描每信道扫描次数 */
#define WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL  1       /* 每次信道扫描发送probe req帧的次数 */

#define WLAN_DEFAULT_MAX_TIME_PER_SCAN                 (3 * 1500)  /* 扫描的默认的最大执行时间，超过此时间，做超时处理 */

/*TBD 扫描时，主被动扫描定时时间，单位ms，变量命名有误*/
#ifdef _PRE_WIFI_DMT
#define WLAN_DEFAULT_ACTIVE_SCAN_TIME           40
#define WLAN_DEFAULT_PASSIVE_SCAN_TIME          800
#define WLAN_DEFAULT_DFS_CHANNEL_SCAN_TIME      240         /*GC模式下指定雷达信道扫描监听时间*/
#else
#define WLAN_DEFAULT_ACTIVE_SCAN_TIME           20
#define WLAN_DEFAULT_PASSIVE_SCAN_TIME          60
#define WLAN_DEFAULT_DFS_CHANNEL_SCAN_TIME      240
#endif

#define WLAN_LONG_ACTIVE_SCAN_TIME              40             /* 指定SSID扫描个数超过3个时,1次扫描超时时间为40ms */

/*****************************************************************************
  2.1.1 STA入网功能
*****************************************************************************/
/* STA可同时关联的最大AP个数*/
#define WLAN_ASSOC_AP_MAX_NUM               2

/* TBD，入网延迟，单位ms。变量需要修订*/
#ifdef _PRE_WIFI_DMT
#define WLAN_JOIN_START_TIMEOUT                 10000
#define WLAN_AUTH_TIMEOUT                       500
#define WLAN_ASSOC_TIMEOUT                      500
#else
#define WLAN_JOIN_START_TIMEOUT                 10000
#define WLAN_AUTH_TIMEOUT                       300
#define WLAN_ASSOC_TIMEOUT                      600
#endif

/*****************************************************************************
  2.1.2 热点入网功能
*****************************************************************************/
/*
 * The 802.11 spec says at most 2007 stations may be
 * associated at once.  For most AP's this is way more
 * than is feasible so we use a default of 128.  This
 * number may be overridden by the driver and/or by
 * user configuration.
 */
#define WLAN_AID_MAX                        2007
#define WLAN_AID_DEFAULT                    128

/* 活跃定时器触发周期 */
#define WLAN_USER_ACTIVE_TRIGGER_TIME           1000
/* 老化定时器触发周期 */
#define WLAN_USER_AGING_TRIGGER_TIME            5000
/* 单位ms */
#define WLAN_USER_ACTIVE_TO_INACTIVE_TIME       5000

#ifdef _PRE_WIFI_DMT
#define WLAN_AP_USER_AGING_TIME                  witp_dmt_get_user_aging_time()
#else
/* 单位ms */
#define WLAN_AP_USER_AGING_TIME                 (60 * 1000)    /* AP 用户老化时间 60S */
#define WLAN_P2PGO_USER_AGING_TIME              (60 * 1000)     /* GO 用户老化时间 60S */
#endif

/* AP keepalive参数,单位ms */
#define WLAN_AP_KEEPALIVE_TRIGGER_TIME          (15 * 1000)       /* keepalive定时器触发周期 */
#define WLAN_AP_KEEPALIVE_INTERVAL              (25 * 1000)   /* ap发送keepalive null帧间隔 */
#define WLAN_GO_KEEPALIVE_INTERVAL              (25*1000) /* P2P GO发送keepalive null帧间隔  */

/* STA keepalive参数,单位ms */
#define WLAN_STA_KEEPALIVE_TIME                 (25*1000) /* wlan0发送keepalive null帧间隔,同1101 keepalive 25s */
#define WLAN_CL_KEEPALIVE_TIME                  (20*1000) /* P2P CL发送keepalive null帧间隔,避免CL被GO pvb唤醒,P2P cl 20s */

/* STA TBTT中断不产生时，驱动linkloss保护机制,单位ms */
#define WLAN_STA_TBTT_PROTECT_TIME             (60 * 1000)

/*****************************************************************************
  2.1.3 STA断网功能
*****************************************************************************/

#define WLAN_LINKLOSS_OFFSET_11H                5  /* 切信道时的延迟 */

/* Beacon Interval参数 */
/* max beacon interval, ms */
#define WLAN_BEACON_INTVAL_MAX              3500
/* min beacon interval */
#define WLAN_BEACON_INTVAL_MIN              40
/* min beacon interval */
#define WLAN_BEACON_INTVAL_DEFAULT          100
/*AP IDLE状态下beacon interval值*/
#define WLAN_BEACON_INTVAL_IDLE             1000


/*****************************************************************************
  2.1.6 保护模式功能
*****************************************************************************/
/*TBD RTS开启门限，实际可删除*/
#define WLAN_RTS_DEFAULT                    512
#define WLAN_RTS_MIN                        1
#define WLAN_RTS_MAX                        2346

/*****************************************************************************
  2.1.7 分片功能
*****************************************************************************/
/* Fragmentation limits */
/* default frag threshold */
#define WLAN_FRAG_THRESHOLD_DEFAULT         1544
/* min frag threshold */
#define WLAN_FRAG_THRESHOLD_MIN             200 /* 为了保证分片数小于16: (1472(下发最大长度)/16)+36(数据帧最大帧头) = 128  */
/* max frag threshold */
#define WLAN_FRAG_THRESHOLD_MAX             2346
/*****************************************************************************
  2.1.14 数据速率功能
*****************************************************************************/
/* 速率相关参数 */

/* 记录支持的速率 */
#define WLAN_SUPP_RATES                         8

/* 用于记录03支持的速率最大个数 */
#define WLAN_MAX_SUPP_RATES                     12

/* 每个用户支持的最大速率集个数 */
#define HAL_TX_RATE_MAX_NUM                4

/* HAL 描述符支持最大发送次数 */
#define HAL_TX_RATE_MAX_CNT                7

/*****************************************************************************
  2.1.16 TXBF功能
*****************************************************************************/


/*****************************************************************************
  2.2 其他协议/定义MAC 层协议类别的spec
*****************************************************************************/
/*****************************************************************************
  2.2.8 国家码功能
*****************************************************************************/
/* 管制类最大个数 */
#define WLAN_MAX_RC_NUM                         20
/* 管制类位图字数 */
#define WLAN_RC_BMAP_WORDS                      2

/*****************************************************************************
  2.2.9 WMM功能
*****************************************************************************/
/* EDCA参数 */
/*STA所用WLAN_EDCA_XXX参数同WLAN_QEDCA_XXX */
/*
#define WLAN_QEDCA_TABLE_INDEX_MIN           1
#define WLAN_QEDCA_TABLE_INDEX_MAX           4*/

#define WLAN_QEDCA_TABLE_CWMIN_MIN           1
#define WLAN_QEDCA_TABLE_CWMIN_MAX           10
#define WLAN_QEDCA_TABLE_CWMAX_MIN           1
#define WLAN_QEDCA_TABLE_CWMAX_MAX           10
#define WLAN_QEDCA_TABLE_AIFSN_MIN           2
#define WLAN_QEDCA_TABLE_AIFSN_MAX           15
#define WLAN_QEDCA_TABLE_TXOP_LIMIT_MIN      1
#define WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX      65535
#define WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX   500

/* TID个数放入平台SPEC */

/* 默认的数据类型业务的TID */
#define WLAN_TID_FOR_DATA                   0

/* 接收队列的个数 与HAL_RX_DSCR_QUEUE_ID_BUTT相等 */
#define HAL_RX_QUEUE_NUM                3
/* 发送队列的个数 */
#define HAL_TX_QUEUE_NUM                6
/* 存储硬件接收上报的描述符链表数目(ping pong使用) */
#define HAL_HW_RX_DSCR_LIST_NUM         2


/*****************************************************************************
  2.2.10 协议节能STA侧功能
*****************************************************************************/
/* PSM特性规格*/
/*TBD 1*/
/* default DTIM period */
#define WLAN_DTIM_DEFAULT                   3

/* DTIM Period参数 */
/* beacon interval的倍数 */
#define WLAN_DTIM_PERIOD_MAX                255
#define WLAN_DTIM_PERIOD_MIN                1

/*****************************************************************************
  2.2.11 协议节能AP侧功能
*****************************************************************************/

/*****************************************************************************
  2.3 校准类别的spec
*****************************************************************************/
#ifdef _PRE_WLAN_PHY_PLL_DIV
/* 支持手动设置分频系数的个数 */
#define WITP_RF_SUPP_NUMS                  4
#endif

/*****************************************************************************
  2.4 安全协议类别的spec
*****************************************************************************/

/*****************************************************************************
  2.4.7 PMF STA功能
*****************************************************************************/
/* SA Query流程间隔时间,老化时间的三分之一*/
//#define WLAN_SA_QUERY_RETRY_TIME                (WLAN_AP_USER_AGING_TIME / 3)
#define WLAN_SA_QUERY_RETRY_TIME                201

/* SA Query流程超时时间,小于老化时间*/
/*#define WLAN_SA_QUERY_MAXIMUM_TIME              (WLAN_SA_QUERY_RETRY_TIME * 3)*/
#define WLAN_SA_QUERY_MAXIMUM_TIME              1000

/*****************************************************************************
  2.4.9 WPA功能
*****************************************************************************/
/* 加密相关的宏定义 */
/* 硬件MAC 最多等待32us， 软件等待40us */
#define HAL_CE_LUT_UPDATE_TIMEOUT          4



/*****************************************************************************
  2.5 性能类别的spec
*****************************************************************************/
/*****************************************************************************
  2.5.1 块确认功能
*****************************************************************************/
#ifdef _PRE_WIFI_DMT
/* 与RSP侧一致，考虑最差的场景下最大超时时间 */
#define WLAN_ADDBA_TIMEOUT                      10000
#define WLAN_TX_PROT_TIMEOUT                    60000
#else
#define WLAN_ADDBA_TIMEOUT                      500
#define WLAN_TX_PROT_TIMEOUT                    6000
#endif

/* 支持的建立rx ba 的最大个数 */
#define WLAN_MAX_RX_BA                      32

/* 支持的建立tx ba 的最大个数 */
#define WLAN_MAX_TX_BA                      32

/*****************************************************************************
  2.5.2 AMPDU功能
*****************************************************************************/
#define WLAN_AMPDU_RX_BUFFER_SIZE       64  /* AMPDU接收端接收缓冲区的buffer size的大小 */
#define WLAN_AMPDU_RX_BA_LUT_WSIZE      64  /* AMPDU接收端用于填写BA RX LUT表的win size,
                                               要求大于等于WLAN_AMPDU_RX_BUFFER_SIZE */

#define WLAN_AMPDU_TX_MAX_NUM           64  /* AMPDU发送端最大聚合子MPDU个数 */

/* 1103由于内存资源限制 在聚amsdu+ampdu的情况下，限制只能最大聚合40个 */
#define WLAN_AMPDU_TX_MAX_NUM_ROM       40


#define WLAN_AMPDU_TX_MAX_BUF_SIZE      64  /* 发送端的buffer size */

#define WLAN_AMPDU_TX_SCHD_STRATEGY     2   /* 软件聚合逻辑，最大聚合设置为窗口大小的一半 */

/* MAC RX BA_LUT表共32行 */
#define HAL_MAX_RX_BA_LUT_SIZE                32
/* MAC TX BA_LUT表共32行 */
#define HAL_MAX_TX_BA_LUT_SIZE                32

/*TBD ，待修订为2*/
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define HAL_PROXYSTA_MAX_BA_LUT_SIZE    16
#endif

/*****************************************************************************
  2.5.3 AMSDU功能
*****************************************************************************/
#define AMSDU_ENABLE_ALL_TID                0xFF
/* amsdu下子msdu的最大长度 */
#define WLAN_MSDU_MAX_LEN                   128
/* 1103 spec amsdu最大长度，对应WLAN_LARGE_NETBUF_SIZE，受制于一个buffer长度 */
#define WLAN_AMSDU_FRAME_MAX_LEN            7935

/* >= WLAN_AMSDU_MAX_NUM/2  */
#define WLAN_DSCR_SUBTABEL_MAX_NUM          1

/*****************************************************************************
  2.5.6 小包优化
*****************************************************************************/
/* 管理帧长度  */
#define HAL_RX_MGMT_FRAME_LEN              WLAN_MGMT_NETBUF_SIZE
/* 短包长度 */
/*短包队列会造成乱序问题,先关掉*/
#define HAL_RX_SMALL_FRAME_LEN             WLAN_SHORT_NETBUF_SIZE

/* 长包长度 */
#ifdef _PRE_WLAN_PHY_PERFORMANCE
/* PHY性能测试使用帧长度 */
#define HAL_RX_FRAME_LEN               5100
#define HAL_RX_FRAME_MAX_LEN           8000
#else
/* 80211帧最大长度:软件最大为1600，流20字节的余量，防止硬件操作越界 */
#define HAL_RX_FRAME_LEN               WLAN_LARGE_NETBUF_SIZE
#define HAL_RX_FRAME_MAX_LEN           8000
#endif

/*****************************************************************************
 2.5.7  接收描述符个数的宏定义
        Notes: DEV0(SMALL + NORMAL + HI RX-Q) + DEV(SMALL + NORMAL + HI RX-Q)
               <= WLAN_MEM_SHARED_RX_DSCR_CNT
        Notes: 辅路只支持2.4G规格可以减小
*****************************************************************************/

/* 接收描述符个数的宏定义 主路 hal device */
/* 小包数据接收描述符队列中描述符最大个数 */
#define HAL_SMALL_RX_MAX_BUFFS             52
/* TBD 32 普通优先级接收描述符队列中描述符最大个数 32*2*3(amsdu占用buf的数目) */
#define HAL_NORMAL_RX_MAX_BUFFS            32
/* TBD 4 高优先级接收描述符队列中描述符最大个数:取决于并发用户数(32) */
#define HAL_HIGH_RX_MAX_BUFFS              4
/* 普通优先级描述符优化规格 */
#define HAL_NORMAL_RX_MAX_RX_OPT_BUFFS     64

/* 接收描述符个数的宏定义 辅路 hal device */
#define HAL_DEV1_SMALL_RX_MAX_BUFFS        40
/* TBD 32 普通优先级接收描述符队列中描述符最大个数 32*2*3(amsdu占用buf的数目) */
#define HAL_DEV1_NORMAL_RX_MAX_BUFFS       24
/* TBD 4 高优先级接收描述符队列中描述符最大个数:取决于并发用户数(32) */
#define HAL_DEV1_HIGH_RX_MAX_BUFFS         4
/* 普通优先级描述符优化规格 */
#define HAL_DEV1_NORMAL_RX_MAX_RX_OPT_BUFFS  64
/* 小包连续5个中断后开始检查硬件队列资源剩余量，避免硬件断流 */
#define HAL_SMALL_CONTIUS_IRQ_THRES        2
#define HAL_NORMAL_CONTIUS_IRQ_THRES       2

#define HAL_SMALL_CONTIUS_AVAIL_IRQ_THRES  48
#define HAL_NORMAL_CONTIUS_AVAIL_IRQ_THRES 24
#define HAL_DBDC_RX_MAX_BUFFS              44


#define HAL_NORMAL_RX_MIN_BUFFS            24

/*****************************************************************************
  2.5.8 自动调频
*****************************************************************************/
/*未建立聚合时pps门限*/
#define NO_BA_PPS_VALUE_0              (0)
#define NO_BA_PPS_VALUE_1              (1000)
#define NO_BA_PPS_VALUE_2              (2500)
#define NO_BA_PPS_VALUE_3              (4000)
/*mate7规格*/
/*pps门限       CPU主频下限     DDR主频下限*/
/*mate7 pps门限*/
#define PPS_VALUE_0              (0)
#define PPS_VALUE_1              (2000)       /* 20M up limit */
#define PPS_VALUE_2              (4000)       /* 40M up limit */
#define PPS_VALUE_3              (10000)      /* 100M up limit */
/*mate7 CPU主频下限*/
#define CPU_MIN_FREQ_VALUE_0              (403200)
#define CPU_MIN_FREQ_VALUE_1              (604800)
#define CPU_MIN_FREQ_VALUE_2              (806400)
#define CPU_MIN_FREQ_VALUE_3              (1305600)
/*mate7 DDR主频下限*/
#define DDR_MIN_FREQ_VALUE_0              (0)
#define DDR_MIN_FREQ_VALUE_1              (3456)
#define DDR_MIN_FREQ_VALUE_2              (6403)
#define DDR_MIN_FREQ_VALUE_3              (9216)

/*****************************************************************************
  2.5.9 DEVICE 接收中断最大个数
*****************************************************************************/
#define WLAN_RX_INTERRUPT_MAX_NUM_PER_DEVICE  16          /* 一个device最多一次处理16个rx中断 */

/*****************************************************************************
  2.6 算法类别的spec
*****************************************************************************/
/*****************************************************************************
  2.6.1 Autorate
*****************************************************************************/
/* Autorate 1102不支持VO聚合 */
#define WLAN_AUTORATE_VO_AGGR_SUPPORT       0
/* Autorate 最小聚合时间索引 */
#define WLAN_AUTORATE_MIN_AGGR_TIME_IDX     3
/* 每个速率等级的平均重传次数 */
#define ALG_AUTORATE_AVG_RATE_RETRY_NUM     3


/*****************************************************************************
  2.6.2 intf det
*****************************************************************************/
/* 同频干扰信息统计的模式 */
#define WLAN_INTF_DET_COCH_MODE       1
/* 负增益探测工作模式 */
#define WLAN_NEG_DET_WORK_MODE        1

/*****************************************************************************
  2.6.3 算法工作模式区分ASIC和FPGA
*****************************************************************************/

//注意此处定义的算法开关宏，请不要在host使用
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
#define WLAN_CCA_OPT_WORK_MODE       WLAN_CCA_OPT_DISABLE         //CCA
#define WLAN_EDCA_OPT_MODE_STA       WLAN_EDCA_OPT_STA_DISABLE    //EDCA
#define WLAN_EDCA_OPT_MODE_AP        WLAN_EDCA_OPT_AP_EN_DISABLE
#define WLAN_TPC_WORK_MODE           WLAN_TPC_WORK_MODE_ENABLE    //TPC
#define WLAN_ANTI_INTF_WORK_MODE     WLAN_ANTI_INTF_EN_OFF        //weak intf opt

#else
#define WLAN_CCA_OPT_WORK_MODE       WLAN_CCA_OPT_ENABLE
#define WLAN_EDCA_OPT_MODE_STA       WLAN_EDCA_OPT_STA_ENABLE
#define WLAN_EDCA_OPT_MODE_AP        WLAN_EDCA_OPT_AP_EN_DISABLE
#define WLAN_TPC_WORK_MODE           WLAN_TPC_WORK_MODE_ENABLE
#define WLAN_ANTI_INTF_WORK_MODE     WLAN_ANTI_INTF_EN_PROBE

#endif

/*****************************************************************************
  2.6.6 TXBF功能
*****************************************************************************/
#define WLAN_MU_BFEE_ACTIVED    WLAN_MU_BFEE_ENABLE

/*****************************************************************************
  2.6.7 dbac
*****************************************************************************/
#define CFG_DBAC_TIMER_IDX                  0

/*****************************************************************************
  2.6.8 schedule
*****************************************************************************/
#define ALG_SCH_MEANS_NUM   ALG_SCH_PROPO_FAIR
#define WLAN_TX_QUEUE_UAPSD_DEPTH    5 /* 芯片省成本，BK预处理不同，最多5个就满 */

/*****************************************************************************
  2.6.15 TPC功能
*****************************************************************************/
/* TPC步进DB数 */
#define WLAN_TPC_STEP                       3
/* TBD 23 最大传输功率，单位dBm */
#define WLAN_MAX_TXPOWER                    30

/*****************************************************************************
  2.6.22 STA P2P异频调度
*****************************************************************************/
/*  虚假队列个数，用于切离一个信道时，将原信道上放到硬件队列里的帧保存起来
当前只有2个场景: DBAC 与 背景扫描 DBAC占用2个队列，编号0 1; 背景扫描占用一个，编号2 */
/*HAL 打头待修订为WLAN*/
#define HAL_TX_FAKE_QUEUE_NUM              3
#define HAL_TX_FAKE_QUEUE_BGSCAN_ID        2
#define HAL_FCS_PROT_MAX_FRAME_LEN         24

/*****************************************************************************
  2.8 架构形态类别的spec
*****************************************************************************/
/*****************************************************************************
  2.8.1 芯片适配规格
*****************************************************************************/
/* 芯片最大空间流数目 */
#define WLAN_MAX_NSS_NUM           (WLAN_DOUBLE_NSS)

/* 芯片无效动态功率 */
#define WLAN_DYN_POW_INVALID        250

/* 2.4G 芯片动态功率调整范围 */
#ifdef _PRE_WLAN_1103_PILOT
#define WLAN_2G_DYN_POW_UPPER_RANGE    30
#else
#define WLAN_2G_DYN_POW_UPPER_RANGE    10
#endif
#define WLAN_2G_DYN_POW_LOWER_RANGE    100
#define WLAN_2G_DYN_POW_RANGE_MIN      100

/* 5G 芯片动态功率动态调整范围 */
#define WLAN_5G_DYN_POW_RANGE_MIN      50
#define WLAN_5G_DYN_POW_UPPER_RANGE    20

/*****************************************************************************
  2.8.2 HAL Device0芯片适配规格
*****************************************************************************/
/* HAL DEV0支持的空间流数 */
#if (WLAN_SINGLE_NSS == WLAN_MAX_NSS_NUM)
#define WLAN_HAL0_NSS_NUM           WLAN_SINGLE_NSS
#elif (WLAN_DOUBLE_NSS == WLAN_MAX_NSS_NUM)
#define WLAN_HAL0_NSS_NUM           WLAN_DOUBLE_NSS
#endif

/* HAL DEV0支持的最大带宽 FPGA只支持80M*/
#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
#if defined(_PRE_WLAN_1103_PILOT) && defined(_PRE_WLAN_FEATURE_160M)
#define WLAN_HAL0_BW_MAX_WIDTH      WLAN_BW_CAP_160M
#else
#define WLAN_HAL0_BW_MAX_WIDTH      WLAN_BW_CAP_80M
#endif
#else
#define WLAN_HAL0_BW_MAX_WIDTH      WLAN_BW_CAP_40M
#endif

/*HAL DEV0 支持SOUNDING功能 */
#define WLAN_HAL0_11N_SOUNDING      OAL_TRUE

/*HAL DEV0 支持Green Field功能 */
#define WLAN_HAL0_GREEN_FIELD       OAL_TRUE

/* HAL DEV0是否支持窄带 */
#define WLAN_HAL0_NB_IS_EN          OAL_FALSE

/* HAL DEV0是否支持1024QAM */
#define WLAN_HAL0_1024QAM_IS_EN     OAL_TRUE

/* HAL DEV0的SU_BFEE能力 */
#define WLAN_HAL0_SU_BFEE_NUM       4

/* HAL DEV0是否支持11MC */
#define WLAN_HAL0_11MC_IS_EN        OAL_FALSE

/* HAL DEV0的通道选择 */
#if (WLAN_SINGLE_NSS == WLAN_MAX_NSS_NUM)
#define WLAN_HAL0_PHY_CHAIN_SEL      WLAN_PHY_CHAIN_ZERO
#elif (WLAN_DOUBLE_NSS == WLAN_MAX_NSS_NUM)
#define WLAN_HAL0_PHY_CHAIN_SEL      WLAN_PHY_CHAIN_DOUBLE
#endif

/* HAL DEV0的需要用单天线发送11b等帧时的TX通道选择 */
#define WLAN_HAL0_SNGL_TX_CHAIN_SEL WLAN_TX_CHAIN_ZERO

/* HAL DEV0的RF通道选择 */
#define WLAN_HAL0_RF_CHAIN_SEL      WLAN_RF_CHAIN_DOUBLE

/* HAL DEV0是否support 2.4g dpd */
#define WLAN_HAL0_DPD_2G_IS_EN      OAL_FALSE

/* HAL DEV0是否support 5g dpd */
#define WLAN_HAL0_DPD_5G_IS_EN      OAL_FALSE

/* HAL DEV0是否support tx stbc, su/mu txbfer */
#if (WLAN_SINGLE_NSS == WLAN_MAX_NSS_NUM)
#define WLAN_HAL0_TX_STBC_IS_EN     OAL_FALSE
#define WLAN_HAL0_SU_BFER_IS_EN     OAL_FALSE
#define WLAN_HAL0_MU_BFER_IS_EN     OAL_FALSE

#elif (WLAN_DOUBLE_NSS == WLAN_MAX_NSS_NUM)
/* 当前double nss下的能力以ASIC定义，FPGA可通过私有定制化文件ini来覆盖刷新 */
#define WLAN_HAL0_TX_STBC_IS_EN     OAL_TRUE
#define WLAN_HAL0_SU_BFER_IS_EN     OAL_TRUE
#define WLAN_HAL0_MU_BFER_IS_EN     OAL_FALSE
#endif

/* HAL DEV0是否support rx stbc, su/mu txbfee */
/* 当前能力以ASIC的能力来定义，FPGA可通过私有定制化文件ini来覆盖刷新 */
#define WLAN_HAL0_RX_STBC_IS_EN     OAL_TRUE
#define WLAN_HAL0_SU_BFEE_IS_EN     OAL_TRUE
#define WLAN_HAL0_MU_BFEE_IS_EN     OAL_FALSE
#define WLAN_HAL0_11N_TXBF_IS_EN    OAL_FALSE
#define WLAN_HAL0_CONTROL_FRM_TX_DOUBLE_CHAIN_FLAG    OAL_FALSE

#define WLAN_HAL0_LDPC_IS_EN        OAL_TRUE
#ifdef _PRE_WLAN_1103_PILOT
#define WLAN_HAL0_11AX_IS_EN        OAL_TRUE
#else
#define WLAN_HAL0_11AX_IS_EN        OAL_FALSE
#endif
#define WLAN_HAL0_DPD_IS_EN         OAL_TRUE

#define WLAN_HAL0_RADAR_DETECTOR_IS_EN      OAL_TRUE
/*HAL DEV0支持TXOP PS*/
#define WLAN_HAL0_TXOPPS_IS_EN        OAL_TRUE


/* HAL DEV0是否支持160M和80+80M的short GI */
#ifdef _PRE_WLAN_FEATURE_160M
#define WLAN_HAL0_VHT_SGI_SUPP_160_80P80    OAL_TRUE
#else
#define WLAN_HAL0_VHT_SGI_SUPP_160_80P80    OAL_FALSE
#endif


#if (2 == WLAN_DEVICE_MAX_NUM_PER_CHIP)
/*****************************************************************************
  2.8.3 HAL Device1芯片适配规格
*****************************************************************************/
/* HAL DEV1支持的空间流数 */
#define WLAN_HAL1_NSS_NUM           WLAN_SINGLE_NSS

/* HAL DEV1是否支持窄带 */
#define WLAN_HAL1_NB_IS_EN          OAL_FALSE

/* HAL DEV1是否支持1024QAM */
#define WLAN_HAL1_1024QAM_IS_EN     OAL_TRUE

/* HAL DEV1的SU_BFEE能力 */
#define WLAN_HAL1_SU_BFEE_NUM       2

/* HAL DEV1是否支持11MC */
#define WLAN_HAL1_11MC_IS_EN        OAL_FALSE

/* HAL DEV1的通道选择 */
#define WLAN_HAL1_PHY_CHAIN_SEL      WLAN_PHY_CHAIN_ZERO  /* 辅phy约束，只能配置为1，不能修改 */

/* HAL DEV1的需要用单天线发送11b等帧时的TX通道选择 */
#define WLAN_HAL1_SNGL_TX_CHAIN_SEL WLAN_TX_CHAIN_ZERO

/* HAL DEV1的Rf通道选择 */
#define WLAN_HAL1_RF_CHAIN_SEL      WLAN_RF_CHAIN_ONE

/* HAL DEV1是否support 2.4g dpd */
#define WLAN_HAL1_DPD_2G_IS_EN      OAL_FALSE

/* HAL DEV1是否support 5g dpd */
#define WLAN_HAL1_DPD_5G_IS_EN      OAL_FALSE

/* HAL DEV1是否support su txbfer */
#define WLAN_HAL1_SU_BFER_IS_EN     OAL_FALSE

/* HAL DEV1是否support su txbfee */
#define WLAN_HAL1_SU_BFEE_IS_EN     OAL_FALSE

/* HAL DEV1是否support mu txbfer */
#define WLAN_HAL1_MU_BFER_IS_EN     OAL_FALSE

/* HAL DEV1是否support mu txbfee */
#define WLAN_HAL1_MU_BFEE_IS_EN     OAL_FALSE

/* 是否支持11n txbf */
#define WLAN_HAL1_11N_TXBF_IS_EN    OAL_FALSE
/* 是否支持控制帧双通道发送 */
#define WLAN_HAL1_CONTROL_FRM_TX_DOUBLE_CHAIN_FLAG    OAL_FALSE


/* HAL DEV1是否support tx stbc */
#define WLAN_HAL1_TX_STBC_IS_EN     OAL_FALSE

/* HAL DEV1是否support rx stbc */
#define WLAN_HAL1_RX_STBC_IS_EN     OAL_FALSE

/* HAL DEV1是否支持LDPC */
#define WLAN_HAL1_LDPC_IS_EN        OAL_FALSE

/*HAL DEV1是否支持11AX*/
#define WLAN_HAL1_11AX_IS_EN        OAL_FALSE

/*HAL DEV1是否支持dpd*/
#define WLAN_HAL1_DPD_IS_EN         OAL_FALSE

/*HAL DEV1 支持带宽最大能力*/
#define WLAN_HAL1_BW_MAX_WIDTH      WLAN_BW_CAP_40M

/*HAL DEV0 支持SOUNDING功能 */
#define WLAN_HAL1_11N_SOUNDING      OAL_FALSE

/*HAL DEV0 支持Green Field功能 */
#define WLAN_HAL1_GREEN_FIELD       OAL_FALSE

/*HAL DEV1是否支持radar*/
#define WLAN_HAL1_RADAR_DETECTOR_IS_EN    OAL_FALSE

/*HAL DEV1支持TXOP PS*/
#define WLAN_HAL1_TXOPPS_IS_EN        OAL_TRUE



#endif  /* 2 == WLAN_DEVICE_MAX_NUM_PER_CHIP */

/*芯片版本已放入平台的定制化*/


/*****************************************************************************
  2.8.2 STA AP规格
*****************************************************************************/
/* 通道数 */
#define WLAN_RF_CHANNEL_NUMS   2
/* RF PLL个数 */
#define WLAN_RF_PLL_NUMS   2

#if 0
#define WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE     2
#endif
 /* AP VAP的规格、STA VAP的规格、STA P2P共存的规格放入平台*/

/* PROXY STA模式下VAP规格宏定义放入平台 */

/* 每个device支持vap的最大个数已放入平台
*/
//#define WLAN_VAP_MAX_NUM_PER_DEVICE         (WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + 1)

#define WLAN_HAL_OHTER_BSS_ID                   14   /* 其他BSS的ID */
#define WLAN_HAL_OTHER_BSS_UCAST_ID             7    /* 来自其他bss的单播管理帧和数据帧，维测用 */
#define WLAN_VAP_MAX_ID_PER_DEVICE_LIMIT        5    /* hal层，0 1 2 3 4 */
/* MAC上报的tbtt中断类别最大值，2个ap的tbtt中断(0-1)+3个sta的tbtt中断(4-6) */
#define WLAN_MAC_REPORT_TBTT_IRQ_MAX            7
/* 整个BOARD支持的最大的device数目放入平台 */


/* 整个BOARD支持的最大的VAP数目已放入平台 */

/* 整个BOARD支持的最大业务VAP的数目 已放入平台*/


/*****************************************************************************
  2.8.3 低成本约束
*****************************************************************************/
/* 接收描述符个数的宏定义 */
/*HAL 打头待修订为WLAN*/
/* 接收队列描述符最大个数 */
#define HAL_RX_MAX_BUFFS                  (HAL_NORMAL_RX_MAX_BUFFS + HAL_SMALL_RX_MAX_BUFFS + HAL_HIGH_RX_MAX_BUFFS)
/*接收描述符做ping pong处理*/
#define HAL_HW_MAX_RX_DSCR_LIST_IDX        1
/* 用于存储接收完成中断最大个数 */
#define HAL_HW_RX_ISR_INFO_MAX_NUMS       (HAL_NORMAL_RX_MAX_BUFFS + HAL_SMALL_RX_MAX_BUFFS)
#define HAL_DOWM_PART_RX_TRACK_MEM         200
#ifdef _PRE_DEBUG_MODE
/* 接收描述符软件可见为第14行，用于打时间戳，调试用 */
#define HAL_DEBUG_RX_DSCR_LINE            (12 + 2)
#endif

/*****************************************************************************
  RX描述符动态调整
*****************************************************************************/
#define WLAN_PKT_MEM_PKT_OPT_LIMIT   2000
#define WLAN_PKT_MEM_PKT_RESET_LIMIT 500
#define WLAN_PKT_MEM_OPT_TIME_MS     1000
#define WLAN_PKT_MEM_OPT_MIN_PKT_LEN HAL_RX_SMALL_FRAME_LEN

/*****************************************************************************
  2.8.7 特性默认开启关闭定义
*****************************************************************************/
/*TBD Feature动态当前未使用，待清理。能力没有使用*/
#define WLAN_FEATURE_AMPDU_IS_OPEN              OAL_TRUE
#define WLAN_FEATURE_AMSDU_IS_OPEN              OAL_TRUE
//#define WLAN_FEATURE_WME_IS_OPEN                OAL_TRUE
#define WLAN_FEATURE_DSSS_CCK_IS_OPEN           OAL_FALSE
#define WLAN_FEATURE_UAPSD_IS_OPEN              OAL_TRUE
//#define WLAN_FEATURE_PSM_IS_OPEN                OAL_TRUE
#define WLAN_FEATURE_WPA_IS_OPEN                OAL_TRUE
#define WLAN_FEATURE_TXBF_IS_OPEN               OAL_TRUE
//#define WLAN_FEATURE_MSDU_DEFRAG_IS_OPEN        OAL_TRUE


/*****************************************************************************
  2.9 WiFi应用类别的spec
*****************************************************************************/
/*****************************************************************************
  2.9.4 P2P特性
*****************************************************************************/
/* Hi1103 P2P特性中P2P vap设备的大小限制(PER_DEVICE) */
#ifdef _PRE_WLAN_FEATURE_P2P
#define WLAN_MAX_SERVICE_P2P_DEV_NUM          1
#define WLAN_MAX_SERVICE_P2P_GOCLIENT_NUM     1
#define WLAN_MAX_SERVICE_CFG_VAP_NUM          1
#endif

/*****************************************************************************
  2.9.12 私有安全增强
*****************************************************************************/
#define WLAN_BLACKLIST_MAX     (32)

/*****************************************************************************
  2.9.18 Proxy STA特性
*****************************************************************************/
/* 每个DEVICE支持的最大业务VAP数目: 场景一有4个AP VAP；场景二有1个AP VAP + 1个STA VAP */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define WLAN_MAX_PROXY_STA_NUM                  15
#define WLAN_STA0_HAL_VAP_ID                    4
#define WLAN_PROXY_STA_START_ID                 16
#define WLAN_PROXY_STA_END_ID                   31
#define WLAN_PROXY_STA_MAX_REP                  2
#endif
/*****************************************************************************
  2.10 MAC FRAME特性
*****************************************************************************/
/*****************************************************************************
  2.10.1 ht cap info
*****************************************************************************/
#define HT_GREEN_FILED_DEFAULT_VALUE            0
#define HT_TX_STBC_DEFAULT_VALUE                0
#define HT_BFEE_NTX_SUPP_ANTA_NUM              (4)     /* 11n支持的bfer发送sounding时的最大天线个数 */
/*****************************************************************************
  2.10.2 vht cap info
*****************************************************************************/
#define VHT_TX_STBC_DEFAULT_VALUE               0
#define VHT_BFEE_NTX_SUPP_STS_CAP              (4)     /* 协议中表示最大接收NDP Nsts个数 */

/*****************************************************************************
  2.10.3 RSSI
*****************************************************************************/
#define WLAN_NEAR_DISTANCE_RSSI        (-20)             /*默认近距离信号门限-35dBm*/
#define WLAN_NEAR_DISTANCE_ADJUST_RSSI (15)             /*近距离默认校准15dB*/
#define WLAN_CLOSE_DISTANCE_RSSI       (-15)             /*关联前距离判断门限-25dBm*/
#define WLAN_FAR_DISTANCE_RSSI         (-73)             /*默认远距离信号门限-73dBm*/
#define WLAN_NORMAL_DISTANCE_RSSI_UP   (-42)             /*信号强度小于-42dBm时，才认为非超近距离*/
#define WLAN_NORMAL_DISTANCE_RSSI_DOWN (-66)             /*信号强度大于-66dBm时，才认为是非超远距离*/
#define WLAN_NEAR_DISTANCE_IMPROVE_RSSI_UP      (-40)    /*improve 1*1问题规避,要求近距离判断上门限为-44dBm*/
#define WLAN_NEAR_DISTANCE_IMPROVE_RSSI_DOWN    (-48)    /*improve 1*1问题规避,要求近距离判断下门限为-50dBm*/
#define WLAN_FIX_MAX_POWER_RSSI        (-55)             /*固定最大功率信号门限*/
#define WLAN_RSSI_ADJUST_TH            (-88)             /* 软件上报RSSI的调整阈值, 低于改阈值rssi-=2 */
#define WLAN_PHY_EXTLNA_CHGPTDBM_TH    (-25)             /* 方向性攻关，修改切换门限为-25dBm */
#define WLAN_LOW_RATE_OPEN_NOISEEST_TH (200)             /* 每100ms打开二次噪声估计收到低速率包的个数 */
#define WLAN_LOW_RATE_OPEN_MLD_TH      (100)             /* 每100ms收到低速率包打开MLD算法的门限 */
#define WLAN_LOW_RATE_OPEN_MLD_COUNT    (10)             /* 每100ms收到低速率包打开MLD算法的次数门限 */


#define WLAN_PHY_EXTLNA_CHGPTDBM_TH_VAL(_en_band, _en_pm_flag) (      \
            ((_en_pm_flag) == OAL_FALSE) ? (WLAN_PHY_EXTLNA_CHGPTDBM_TH): \
            (((_en_band) == WLAN_BAND_2G) ? -52:-40))

/*****************************************************************************
  2.10.4 TXBF cap
*****************************************************************************/
#define WLAN_TXBF_BFER_LEGACY_ENABLE            (0)      /* legacy txbf disable */
#define WLAN_TXBF_BFER_LOG_ENABLE               (0)      /* alg txbf log enable */
/* bfer 2*2规格缓存的bfee report矩阵 */
/* buffer size = 2*(4+6)/2*468/8(160M) = 590bytes,另外为snr值预留10byte,预留部分内存到640bytes */
#define WLAN_TXBF_BFER_MATRIX_BUFFER_SIZE       (640)
/* bfee 4*2规格缓存的mu bfee延时反馈矩阵 */
/* buffer size = 10*(7+9)/2*468/8(160M) = 4680bytes,另外为snr值预留10byte,预留部分内存到5000bytes */
#define WLAN_TXBF_BFEE_MATRIX_BUFFER_SIZE       (5000)
/*****************************************************************************

  2.11 描述符偏移
*****************************************************************************/
#define WLAN_RX_DSCR_SIZE        WLAN_MEM_SHARED_RX_DSCR_SIZE      //实际接收描述符大小



/*****************************************************************************
  2.12 COEX FEATURE
*****************************************************************************/
#define BTCOEX_RX_COUNT_LIMIT               (128)
#define BTCOEX_RX_STATISTICS_TIME           (2000)    //双链接建议保持3s，目前测试出来最多是1s多
#define BTCOEX_PRI_DURATION_TIME            (30) //ms

#define BT_POSTPREEMPT_MAX_TIMES            (15)
#define BT_PREEMPT_MAX_TIMES                (1)
#define BT_POSTPREEMPT_TIMEOUT_US           (150)
#define BT_ABORT_RETRY_TIMES_MAX            (10)
#define BT_ABORT_RETRY_TIMES_MIN            (1)
#define BT_ABORT_RETRY_TIMES_DETECT_INVALID (3)  /* abort timers连续MIN异常判定门限 */

#define BT_PREEMPT_TIMEOUT_US               (50)
#define BLE_PREEMPT_TIMEOUT_US              (10)

#define BTCOEX_BLACKLIST_BA_SIZE_LIMIT       0x0002

#define BTCOEX_BT_SCO_DURATION
#define BTCOEX_BT_DATATRANS_DURATION
#define BTCOEX_BT_A2DP_DURATION
#define BTCOEX_BT_DEFAULT_DURATION          (0xFF)

#define BTCOEX_PHY_TXRX_ALL_EN              (0x0000000F)
#define BTCOEX_BT2WIFI_RF_STABLE_TIME_US    (50)

#define BT_WLAN_COEX_UNAVAIL_PAYLOAD_THRES  (8)
#define BT_WLAN_COEX_SMALL_PKT_THRES        (200)
#define BT_WLAN_COEX_SMALL_FIFO_THRES       (1023)

#define OCCUPIED_TIMES                      (3)
#define OCCUPIED_INTERVAL                   (60)
#define OCCUPIED_PERIOD                     (60000)

#define COEX_LINKLOSS_OCCUP_TIMES           (15)
#define COEX_LINKLOSS_OCCUP_PERIOD          (20000)

#ifdef _PRE_WLAN_FEATURE_SMARTANT
#define DUAL_ANTENNA_AVAILABLE                      (OAL_SUCC)  //必须与OAL_SUCC一致
#define DUAL_ANTENNA_ALG_CLOSE                      (DUAL_ANTENNA_AVAILABLE+1)
#define DUAL_ANTENNA_ALG_INTERRUPT                  (DUAL_ANTENNA_AVAILABLE+2)
#define DUAL_ANTENNA_SWITCH_FAIL                    (DUAL_ANTENNA_AVAILABLE+7)
#endif

#define BTCOEX_M2S_RSSI_THRESHOLD            (-45)
#define BTCOEX_S2M_RSSI_THRESHOLD            (-75)
#define BTCOEX_RSSI_DETECT_TIMEOUT           (200)
#define BTCOEX_RSSI_DETECT_VALID_TH          (3)
#define BTCOEX_RSSI_DETECT_CHECK_INTERVAL    (4)

/*****************************************************************************
  2.13 Calibration FEATURE spec
*****************************************************************************/
    /* 校准数据上传下发MASK*/
#define CALI_DATA_REFRESH_MASK              BIT0
#define CALI_DATA_REUPLOAD_MASK             BIT1
#define CALI_POWER_LVL_DBG_MASK             BIT3
#define CALI_INTVL_MASK                     (0xe0)
#define CALI_INTVL_OFFSET                   (5)


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* #ifndef __WLAN_SPEC_1103_H__ */



