

#ifndef __PLATFORM_SPEC_1151_H__
#define __PLATFORM_SPEC_1151_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/*****************************************************************************
  1.1.1 版本spec
*****************************************************************************/
/* 芯片版本 */
#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
/* hi1151V100H */
#define WLAN_CHIP_VERSION_HI1151V100H           0x11510100
#else
/* hi1151V100H */
#define WLAN_CHIP_VERSION_HI1151V100H           0x11510100
#endif
/* hi1151V100L */
#define WLAN_CHIP_VERSION_HI1151V100L           0x11510102

/*针对Host和Device判定的预编译，1151认为都为帧，属于临时预编译。待公共结构体整改后删除*/
#define IS_HOST 1
#define IS_DEVICE 1

/*****************************************************************************
  1.1.2 多Core对应spec
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    #define WLAN_FRW_MAX_NUM_CORES          2   /* CORE的数量 */
#else
    #define WLAN_FRW_MAX_NUM_CORES          1
#endif

/*****************************************************************************
  2.1 WLAN芯片对应的spec
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_DOUBLE_CHIP
#define WLAN_CHIP_MAX_NUM_PER_BOARD                 2              /* 双芯片board下chip个数 */
#else
#define WLAN_CHIP_MAX_NUM_PER_BOARD                 1              /* 单芯片board下chip个数 */
#endif

/* 默认情况下，2G chip id为0，5G chip id为1 */
#define WLAN_CHIP_PCIE0_ID                          0
#define WLAN_CHIP_PCIE1_ID                          1

// 仅荣耀2G单板有此场景。冯静:整改CHIP ID与band能力相关逻辑代码！
#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_SINGLE_BAND
#define WLAN_SINGLE_CHIP_ID 0
#define WLAN_SINGLE_CHIP_SINGLE_BAND_WORK_BAND   WLAN_BAND_2G
#endif

#define WLAN_DEVICE_MAX_NUM_PER_CHIP                1                       /* 每个chip支持device的最大个数，总数不会超过8个 */
#define WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP        WLAN_DEVICE_MAX_NUM_PER_CHIP /* 每个chip支持业务device的最大个数 */

/* 整个BOARD支持的最大的device数 */
#define WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC            (WLAN_CHIP_MAX_NUM_PER_BOARD * WLAN_DEVICE_MAX_NUM_PER_CHIP)

/* 整个BOARD支持的最大的业务device数目 */
#define WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC    (WLAN_CHIP_MAX_NUM_PER_BOARD * WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP)


#ifdef _PRE_WLAN_PRODUCT_1151V200
#if(_PRE_TARGET_PRODUCT_TYPE_5630HERA == _PRE_CONFIG_TARGET_PRODUCT)
#define WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE      3   /* AP的规格，5630内存受限，因此缩减AP个数*/
#define WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE     1    /* STA的规格 */
#else
#define WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE      16   /* AP的规格，将之前的WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE修改*/
#define WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE     3    /* STA的规格 */
#endif
#else
#define WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE      4   /* AP的规格，将之前的WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE修改*/
#define WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE     1   /* STA的规格 */
#endif

#define WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE      1   /* 配置VAP个数,一个业务device一个 */
#ifdef _PRE_WLAN_PRODUCT_1151V200
#if(_PRE_TARGET_PRODUCT_TYPE_5630HERA == _PRE_CONFIG_TARGET_PRODUCT)
#define WLAN_AP_STA_COEXIST_VAP_NUM             4   /* ap sta共存时vap数目 1个STA 3个AP */
#else
#define WLAN_AP_STA_COEXIST_VAP_NUM             19   /* ap sta共存时vap数目 3个STA 16个AP */
#endif
#else
#define WLAN_AP_STA_COEXIST_VAP_NUM             5    /* ap sta共存时vap数目 1个STA 4个AP */
#endif

/* PROXY STA模式下VAP规格宏定义 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define WLAN_PROXY_STA_MAX_NUM_PER_DEVICE             15   /* PROXY STA的个数 */
#else
#define WLAN_PROXY_STA_MAX_NUM_PER_DEVICE             0    /* PROXY STA没打开置0 */
#endif

/* 整个device支持的最大的业务VAP数目 */
#define WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE    (WLAN_PROXY_STA_MAX_NUM_PER_DEVICE + WLAN_AP_STA_COEXIST_VAP_NUM)        /* 业务VAP个数 */

/* 整个device支持的最大的VAP数目 */
#define WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT      (WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE + WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE) /* 21个:4个ap,1个sta,15个proxysta,1个配置vap */

/* 整个BOARD支持的最大的VAP数目 */
#define WLAN_VAP_SUPPORT_MAX_NUM_LIMIT      (WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC * WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT)  /* device个数*21个:1个ap,1个sta,15个proxysta,1个配置vap */

/* 整个BOARD支持的最大业务VAP的数目 */
#define WLAN_SERVICE_VAP_SUPPORT_MAX_NUM_LIMIT    (WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC * (WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT - WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE)) /* device个数*20个:4个ap,1个sta,15个proxysta */

/* 以下两个成员可以定制化: WLAN_ASSOC_USER_MAX_NUM对应g_us_assoc_max_user，WLAN_ACTIVE_USER_MAX_NUM对应g_us_active_max_user */
/* 关联用户的最大个数 */
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_ASSOC_USER_MAX_NUM       128    /* 关联用户的最大个数 */
#else
#define WLAN_ASSOC_USER_MAX_NUM       32     /* 关联用户的最大个数 */
#endif

/* 活跃用户的最大个数 */
#define WLAN_ACTIVE_USER_MAX_NUM      32

/* 活跃用户索引位图 */
#define WLAN_ACTIVE_USER_IDX_BMAP_LEN       ((WLAN_ACTIVE_USER_MAX_NUM + 7)>> 3)
 /* 关联用户索引位图 */
#define WLAN_ASSOC_USER_IDX_BMAP_LEN       ((WLAN_ASSOC_USER_MAX_NUM + 7)>> 3)
 /* 不可用的RA LUT IDX */
#define WLAN_INVALID_RA_LUT_IDX             WLAN_ACTIVE_USER_MAX_NUM

/* 以下三个用户规格，表示软件支持最大规格; 与之对应的是MAC_RES_XXX(大于等于RES_XXX)，为定制化得到，对应WLAN_ASSOC_USER_MAX_NUM为g_us_assoc_max_user */
/* 使用规则:这里三个用于oal oam hal初始化成员，或者数组下标，与之对应mac res的单播和组播，以及整board user个数会封装成函数供业务层代码调用 */
/*board最大关联用户数 = 1个CHIP支持的最大关联用户数 * board上面的CHIP数目*/
#define WLAN_ASOC_USER_MAX_NUM_LIMIT       (WLAN_ASSOC_USER_MAX_NUM * WLAN_CHIP_MAX_NUM_PER_BOARD)

/* board组播用户数 */
#define WLAN_MULTI_USER_MAX_NUM_LIMIT      (WLAN_SERVICE_VAP_SUPPORT_MAX_NUM_LIMIT)

/*board最大用户数 = 最大关联用户数 + 组播用户个数 */
#define WLAN_USER_MAX_USER_LIMIT           (WLAN_ASOC_USER_MAX_NUM_LIMIT + WLAN_MULTI_USER_MAX_NUM_LIMIT)

/* alg算法模块使用,指向user index, 1151 user包括active + noactive user，因此用assoc id做index */
#define WLAN_ALG_ASOC_USER_NUM_LIMIT       (WLAN_USER_MAX_USER_LIMIT)

/*****************************************************************************
  2.2 WLAN协议对应的spec
*****************************************************************************/

/*****************************************************************************
  2.3 oam相关的spec
*****************************************************************************/
#if (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) || (_PRE_OS_VERSION_WINDOWS == _PRE_OS_VERSION))
#define WLAN_OAM_FILE_PATH      "C:\\OAM.log"                   /* WIN32和WINDOWS下,LOG文件默认的保存位置 */
#elif ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION))
#define WLAN_OAM_FILE_PATH      "\\home\\oam.log"               /* LINUX和裸系统下,LOG文件默认的保存位置 */
#endif

/*****************************************************************************
  2.4 mem对应的spec
*****************************************************************************/
/*****************************************************************************
  2.4.1
*****************************************************************************/

#define WLAN_MEM_MAX_BYTE_LEN               (32100 + 1)   /* 可分配最大内存块长度 */
#define WLAN_MEM_MAX_SUBPOOL_NUM            8             /* 内存池中最大子内存池个数 */
#define WLAN_MEM_MAX_USERS_NUM              4             /* 共享同一块内存的最大用户数 */

/*****************************************************************************
  2.4.2 共享描述符内存池配置信息
*****************************************************************************/
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
#define WLAN_TID_MPDU_NUM_BIT               11
#else
#define WLAN_TID_MPDU_NUM_BIT               10
#endif
/* 整个device所有TID的最大MPDU数量限制，暂定1024个，以防止多用户多优先级断流 */
#define WLAN_TID_MPDU_NUM_LIMIT             (1 << WLAN_TID_MPDU_NUM_BIT)

#ifdef _PRE_WLAN_PRODUCT_1151V200
#define WLAN_MEM_SHARED_RX_DSCR_SIZE        88             /*比实际接收描述符结构体稍大些，预留出后面对接收描述符的修改*/
#else
#define WLAN_MEM_SHARED_RX_DSCR_SIZE        64
#endif
#define WLAN_MEM_SHARED_RX_DSCR_CNT         (576 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 接收512(数据帧描述符) + 64(管理帧描述符) */ /* 注意! 新增一个子内存池要更新oal_mem.c里的OAL_MEM_BLK_TOTAL_CNT */
#define WLAN_MEM_SHARED_TX_DSCR_SIZE1       128            /*比实际发送描述符结构体稍大些，预留出后面对发送描述符的修改*/
#define WLAN_MEM_SHARED_TX_DSCR_CNT1        ((WLAN_TID_MPDU_NUM_LIMIT * 2) * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 发送描述符512 */
#define WLAN_MEM_SHARED_TX_DSCR_SIZE2       256                          /*比实际发送描述符结构体稍大些，预留出后面对发送描述符的修改*/
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
#define WLAN_MEM_SHARED_TX_DSCR_CNT2        (1024 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#else
#define WLAN_MEM_SHARED_TX_DSCR_CNT2        (256 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 发送amsdu的描述符 */
#endif

/*****************************************************************************
  2.4.3 共享管理帧内存池配置信息
*****************************************************************************/
#define WLAN_MEM_SHARED_MGMT_PKT_SIZE1      1300
#ifdef _PRE_WLAN_PRODUCT_1151V200
#define WLAN_MEM_SHARED_MGMT_PKT_CNT1       (WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE * 2 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 16个AP */
#else
#define WLAN_MEM_SHARED_MGMT_PKT_CNT1       (8 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#endif

/*****************************************************************************
  2.4.4 共享数据帧内存池配置信息
*****************************************************************************/
#define WLAN_MEM_SHARED_DATA_PKT_SIZE       44              /* 80211mac帧头大小 */
#define WLAN_MEM_SHARED_DATA_PKT_CNT        ((256 + 1024) * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* skb(接收的帧头个数) + 发送描述符个数(发送的帧头个数) 768 */

/*****************************************************************************
  2.4.5 本地内存池配置信息
*****************************************************************************/
#define WLAN_MEM_LOCAL_SIZE1                32

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_MEM_LOCAL_CNT1                 (2320 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 1024(128*8)个dmac_alg_tid_stru + 1024个alg_tid_entry_stru + 5个事件队列(NON_RESET_ERR)*/
#else
#define WLAN_MEM_LOCAL_CNT1                 (580 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 256(32*8)个dmac_alg_tid_stru + 256个alg_tid_entry_stru + 5个事件队列(NON_RESET_ERR)*/
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define WLAN_MEM_LOCAL_SIZE2                140

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_MEM_LOCAL_CNT2                 (1200 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 200(杂用) */    /* 128用户多了>76*96=7296bytes，先+200块 */
#else
#define WLAN_MEM_LOCAL_CNT2                 (800 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 200(杂用) */
#endif  /* #ifdef _PRE_WLAN_FEATURE_USER_EXTEND */
#else
#define WLAN_MEM_LOCAL_SIZE2                100                                /* proxysta是否需要从800增大到更多，待后续满规格用户关联测试后再定 */
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_MEM_LOCAL_CNT2                 (1200 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 部分算法结构申请内存块(CCA算法要为每个用户申请76字节内存)，软件相关结构申请内存块等 */
#else
#define WLAN_MEM_LOCAL_CNT2                 (800 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 部分算法结构申请内存块(CCA算法要为每个用户申请76字节内存)，软件相关结构申请内存块等 */
#endif /* #ifdef _PRE_WLAN_FEATURE_USER_EXTEND */
#endif

#define WLAN_MEM_LOCAL_SIZE3                260             /* 存储hmac_vap_cfg_priv_stru，每个VAP一个 + 事件队列 14 *５ */
#define WLAN_MEM_LOCAL_CNT3                 (WLAN_VAP_SUPPORT_MAX_NUM_LIMIT + 580)   /* 部分软件相关结构申请内存块，32深度事件队列申请内存块等 */

#define WLAN_MEM_LOCAL_SIZE4                500

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_MEM_LOCAL_CNT4                 (550 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)  /* 64深度事件队列申请内存块、智能天线alg_smartant_per_user_info_stru申请内存块等 */
#else
#define WLAN_MEM_LOCAL_CNT4                 (350 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)  /* 64深度事件队列申请内存块、智能天线alg_smartant_per_user_info_stru申请内存块等 */
#endif

#define WLAN_MEM_LOCAL_SIZE5                2200

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#define WLAN_MEM_LOCAL_CNT5                 (180 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 算法(TPC/autorate等)申请内存块、杂用等(杂用部分后续仍需要优化) */
#else
#define WLAN_MEM_LOCAL_CNT5                 (120 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 算法(TPC/autorate等)申请内存块、杂用等(杂用部分后续仍需要优化) */
#endif

#define WLAN_MEM_LOCAL_SIZE6                9500           /* autorate */
#define WLAN_MEM_LOCAL_CNT6                 (64 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /*  */


/*****************************************************************************
  2.4.6 事件结构体内存池
*****************************************************************************/
#define WLAN_MEM_EVENT_SIZE1                64              /* 注意: 事件内存长度包括4字节IPC头长度 */
#define WLAN_MEM_EVENT_CNT1                 (180 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#ifdef _PRE_WLAN_FEATURE_11R_AP
#define WLAN_MEM_EVENT_SIZE2                556              /* 11r下发ie最大530，原事件内存长度不够 */
#else
#define WLAN_MEM_EVENT_SIZE2                528              /* 注意: 事件内存长度包括4字节IPC头长度 */
#endif
#define WLAN_MEM_EVENT_CNT2                 6               /*hmac_main_init_etc同步mac_chip_stru使用一次*/

#define WLAN_WPS_IE_MAX_SIZE                WLAN_MEM_EVENT_SIZE2 - 32   /* 32表示事件自身占用的空间 */
#ifdef _PRE_WLAN_FEATURE_HILINK
#define WLAN_OKC_IE_MAX_SIZE                WLAN_MEM_EVENT_SIZE2 - 32   /* 32表示事件自身占用的空间 */
#endif
/*****************************************************************************
  2.4.7 用户内存池
*****************************************************************************/


/*****************************************************************************
  2.4.8 MIB内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/
#define WLAN_MEM_MIB_SIZE1                  32000           /* mib结构体大小 */
#define WLAN_MEM_MIB_CNT1                   ((WLAN_VAP_SUPPORT_MAX_NUM_LIMIT - 1) * 2)    /* 配置VAP没有MIB */

/*****************************************************************************
  2.4.9 netbuf内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/
#define WLAN_MEM_NETBUF_SIZE1               0       /* 克隆用SKB */
#define WLAN_MEM_NETBUF_CNT1                (192 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC) /* 接收数据帧是AMSDU，其中的每个MSDU对应一个克隆netbuf */

#ifndef _PRE_WLAN_PHY_PERFORMANCE
#define WLAN_MEM_NETBUF_SIZE2               1600    /* 1500 + WLAN_MAX_FRAME_HEADER_LEN(36) + WLAN_HDR_FCS_LENGTH(4) + (解密失败的话,加密字段也会上报(20)) */
#define WLAN_MEM_NETBUF_CNT2                (512 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)      /* 接收192(接收数据帧) + 32(接收管理帧) + 32(发送管理帧) */
                                                        /* 考虑接收wlan2wlan转发场景，在上面的基础上x2 */
#define WLAN_MEM_NETBUF_SIZE3               2500    /* 解分片用重组报文的skb */
#define WLAN_MEM_NETBUF_CNT3                (32 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)       /* 活跃用户每个用户一个 */

#else
#define WLAN_MEM_NETBUF_SIZE2               5100
#define WLAN_MEM_NETBUF_CNT2                (512 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)

#define WLAN_MEM_NETBUF_SIZE3               5100    /* 解分片用重组报文的skb */
#define WLAN_MEM_NETBUF_CNT3                (32 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)       /* 活跃用户每个用户一个 */
#endif


#define WLAN_MEM_NETBUF_ALIGN               4       /* netbuf对齐 */
#define WLAN_MEM_ETH_HEADER_LEN             14      /* 以太网帧头长度 */

#define WLAN_LARGE_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE2 /* NETBUF内存池长帧的长度，统一用这个宏 */
#define WLAN_MGMT_NETBUF_SIZE         WLAN_MEM_NETBUF_SIZE2 /* NETBUF内存池管理帧的长度 ， 统一用这个宏 */
#define WLAN_SHORT_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE2 /* NETBUF内存池短帧的长度 ，统一用这个宏 */
#define WLAN_MAX_NETBUF_SIZE          WLAN_LARGE_NETBUF_SIZE /* netbuf最大帧长，帧头 + payload */
#define WLAN_SMGMT_NETBUF_SIZE        WLAN_MGMT_NETBUF_SIZE  /* NETBUF内存池短管理帧的长度 ， 统一用这个宏 */

/*****************************************************************************
  2.4.9.1 sdt netbuf内存池
*****************************************************************************/

/*  sdt消息预分配好内存块，在netbuf入队后，工作队列出队时不需要额外处理，直接send即可
    外部函数申请长度为Payload的长度
*/
/************************* sdt report msg format*********************************/
/* NETLINK_HEAD     | SDT_MSG_HEAD  | Payload    | SDT_MSG_TAIL  |    pad       */
/* ---------------------------------------------------------------------------- */
/* NLMSG_HDRLEN     |    8Byte      |     ...    |   1Byte       |    ...       */
/********************************************************************************/
#define WLAN_SDT_SKB_HEADROOM_LEN       8
#define WLAN_SDT_SKB_TAILROOM_LEN       1
#define WLAN_SDT_SKB_RESERVE_LEN        (WLAN_SDT_SKB_HEADROOM_LEN + WLAN_SDT_SKB_TAILROOM_LEN)

/*
    SDT内存池需要根据SDT消息的实际进行调整
*/
#define WLAN_MEM_SDT_NETBUF_PAYLOAD1            37          //日志消息长度
#define WLAN_MEM_SDT_NETBUF_PAYLOAD2            100
#define WLAN_MEM_SDT_NETBUF_PAYLOAD3            512
#define WLAN_MEM_SDT_NETBUF_PAYLOAD4            1600

#define WLAN_SDT_NETBUF_MAX_PAYLOAD             WLAN_MEM_SDT_NETBUF_PAYLOAD4

#define WLAN_MEM_SDT_NETBUF_SIZE1       (WLAN_MEM_SDT_NETBUF_PAYLOAD1 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE1_CNT   (250 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#define WLAN_MEM_SDT_NETBUF_SIZE2       (WLAN_MEM_SDT_NETBUF_PAYLOAD2 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE2_CNT   (250 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#define WLAN_MEM_SDT_NETBUF_SIZE3       (WLAN_MEM_SDT_NETBUF_PAYLOAD3 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE3_CNT   (250 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)
#define WLAN_MEM_SDT_NETBUF_SIZE4       (WLAN_MEM_SDT_NETBUF_PAYLOAD4 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE4_CNT   (256 * WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC)

#define WLAN_SDT_MSG_FLT_HIGH_THD           800
#define WLAN_SDT_MSG_QUEUE_MAX_LEN          (WLAN_MEM_SDT_NETBUF_SIZE1_CNT + \
                                                 WLAN_MEM_SDT_NETBUF_SIZE2_CNT + \
                                                 WLAN_MEM_SDT_NETBUF_SIZE3_CNT + \
                                                 WLAN_MEM_SDT_NETBUF_SIZE4_CNT - 6)  /* 入队数比内存池要少，此处取整1000 */

/*****************************************************************************
  2.4.10 RF通道数规格
*****************************************************************************/

/*****************************************************************************
  2.4.11 TCP ACK优化
*****************************************************************************/
#define DEFAULT_TX_TCP_ACK_OPT_ENABLE (OAL_FALSE)
#define DEFAULT_RX_TCP_ACK_OPT_ENABLE (OAL_FALSE)
#define DEFAULT_TX_TCP_ACK_THRESHOLD (1) /*丢弃发送ack 的门限*/
#define DEFAULT_RX_TCP_ACK_THRESHOLD (1) /*丢弃接收ack 的门限*/


/*****************************************************************************
  2.5 frw相关的spec
*****************************************************************************/

/******************************************************************************
    事件队列配置信息表
    注意: 每个队列所能容纳的最大事件个数必须是2的整数次幂
*******************************************************************************/

#define FRW_EVENT_MAX_NUM_QUEUES    (FRW_EVENT_TYPE_BUTT * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)

#define WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    /* 事件类型       队列权重   队列所能容纳的最大事件个数   队列所属调度策略 */  \
    /* HIGH_PRIO */     {   1,               32,                      0, 0}, \
    /* HOST_CRX */      {   1,               64,                      1, 0}, \
    /* HOST_DRX */      {   1,               64,                      1, 0}, \
    /* HOST_CTX */      {   1,               64,                      1, 0}, \
    /* HOST_SDT */      {   1,               64,                      1, 0}, \
    /* WLAN_CRX */      {   1,               64,                      1, 0}, \
    /* WLAN_DRX */      {   1,               64,                      0, 0}, \
    /* WLAN_CTX */      {   1,               64,                      1, 0}, \
    /* WLAN_DTX */      {   1,               64,                      0, 0}, \
    /* WLAN_TX_COMP */  {   1,               64,                      0, 0}, \
    /* TBTT */          {   1,               64,                      1, 0}, \
    /* TIMEOUT */       {   1,                2,                      1, 0}, \
    /* HMAC MISC */     {   1,               64,                      1, 0}, \
    /* DMAC MISC */     {   1,               64,                      0, 0},
#ifdef _PRE_WLAN_FEATURE_DOUBLE_CHIP /* 双核双芯片配置表 */

#define WLAN_FRW_EVENT_CFG_TABLE \
  { \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
  }
#else /* 双核单芯片 */
#ifdef _PRE_WLAN_PRODUCT_1151V200
#define WLAN_FRW_EVENT_CFG_TABLE \
  { \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
  }
#else
#define WLAN_FRW_EVENT_CFG_TABLE \
  { \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
  }
#endif
#endif

/*****************************************************************************
  2.8.0 虚拟OS适配
*****************************************************************************/

/*****************************************************************************
  2.9 DFT
*****************************************************************************/
/*****************************************************************************
  2.9.0 日志
*****************************************************************************/
/*****************************************************************************
  2.9.15 WiFi关键信息检测
*****************************************************************************/



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



/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
#endif /* #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* #ifndef __PLATFORM_SPEC_1151_H__ */




