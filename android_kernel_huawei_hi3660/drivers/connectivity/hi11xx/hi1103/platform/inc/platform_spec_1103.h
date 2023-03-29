

#ifndef __PLATFORM_SPEC_1103_H__
#define __PLATFORM_SPEC_1103_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"

/*****************************************************************************
  2 宏定义
*/
/*****************************************************************************
  1.1.1 版本spec
*****************************************************************************/
/* 芯片版本，1103待删除 */
#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
/* hi1151V100H */
#define WLAN_CHIP_VERSION_HI1151V100H           0x11510101
#else
/* hi1151V100H */
#define WLAN_CHIP_VERSION_HI1151V100H           0x11510100
#endif
/* hi1151V100L */
#define WLAN_CHIP_VERSION_HI1151V100L           0x11510102

/*增量针对Host和Device判定的预编译，限定用于mac_xxx的函数调用中。待公共结构体整改后删除*/
#define IS_HOST (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
#define IS_DEVICE (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

/*****************************************************************************
  1.1.2 多Core对应spec
*****************************************************************************/
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
    /* WiFi对应裸系统CORE的数量 为1*/
    #define WLAN_FRW_MAX_NUM_CORES          1
#else
    /* WiFi对应Linux系统CORE的数量 为1*/
    #define WLAN_FRW_MAX_NUM_CORES          1
#endif


/*****************************************************************************
  2 WLAN 宏定义
*****************************************************************************/
/*****************************************************************************
  2.1 WLAN芯片对应的spec
*****************************************************************************/
#define WLAN_CHIP_MAX_NUM_PER_BOARD         1                       /* 每个board支持chip的最大个数，总数不会超过8个 */

/* 静态dbdc和动态dbdc hal device个数都为2 */
#ifdef _PRE_WLAN_FEATURE_DBDC
#define WLAN_DEVICE_MAX_NUM_PER_CHIP            2                     /* 每个chip支持hal device的最大个数，总数不会超过8个,03应该为2,目前硬件不支持 */
#else
#define WLAN_DEVICE_MAX_NUM_PER_CHIP            1                     /* 每个chip支持hal device的最大个数，总数不会超过8个,03应该为2,目前硬件不支持 */
#endif

#define WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP    1                     /* 每个chip支持业务device的最大个数 */

/* 整个BOARD支持的最大的hal device数目 */
#define WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC    (WLAN_CHIP_MAX_NUM_PER_BOARD * WLAN_DEVICE_MAX_NUM_PER_CHIP)

/* 整个BOARD支持的最大的业务device数目 */
#define WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC    (WLAN_CHIP_MAX_NUM_PER_BOARD * WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP)


#define WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE      2   /* 业务AP的规格，将之前的WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE修改*/
#define WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE     3   /* 业务STA的规格 */
#define WLAN_AP_STA_COEXIST_VAP_NUM             0   /* 业务ap sta共存时vap数目,Hi1103后续有STA+AP共存的情况 */


/* 由于最大业务VAP个数需要按照设备同时创建的最大规格初始化，目前按照最大的规格设置即可 */
#define WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE      1   /* 配置VAP个数,一个业务device一个 */
#define WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE     4   /* 业务VAP个数(原本应该用AP个数+STA个数（这个表示芯片最大支持能力  */


/* 每个device支持vap的最大个数=最大业务VAP数目+配置VAP数量 */
/* 软件规格:P2P_dev/CL以STA模式存在，P2P_GO以AP模式存在
    1)AP 模式:  2个ap + 1个配置vap
    2)STA 模式: 3个sta + 1个配置vap
    3)STA+P2P共存模式:  1个sta + 1个P2P_dev + 1个P2P_GO/Client + 1个配置vap
    4)预留一个给NAN
*/

/* 4个业务VAP + 1个配置vap */
#define WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT         (WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE + WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE)

/* 整个BOARD支持的最大的VAP数目 */
#define WLAN_VAP_SUPPORT_MAX_NUM_LIMIT            (WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC * WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT)

/* 整个BOARD支持的最大业务VAP的数目 */
#define WLAN_SERVICE_VAP_SUPPORT_MAX_NUM_LIMIT    (WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC * (WLAN_VAP_MAX_NUM_PER_DEVICE_LIMIT - WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE))

/* 以下两个成员可以定制化: WLAN_ASSOC_USER_MAX_NUM对应g_us_assoc_max_user，WLAN_ACTIVE_USER_MAX_NUM对应g_us_active_max_user */
/* 关联用户的最大个数 */
#define WLAN_ASSOC_USER_MAX_NUM                    10  /* 关联用户的最大个数 */

/* 活跃用户的最大个数 */
#define WLAN_ACTIVE_USER_MAX_NUM                   10

/* 活跃用户索引位图长度 */
#define WLAN_ACTIVE_USER_IDX_BMAP_LEN      ((WLAN_ACTIVE_USER_MAX_NUM + 7)>> 3)
/* 关联用户索引位图长度 */
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

/* alg算法模块使用,指向user index,不包含组播用户数,用user lut index做索引 */
#define WLAN_ALG_ASOC_USER_NUM_LIMIT       (WLAN_ASOC_USER_MAX_NUM_LIMIT)

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

#define WLAN_MEM_MAX_BYTE_LEN               (65000 + 1)   /* 可分配最大内存块长度 03不作限制*/
#define WLAN_MAX_MAC_HDR_LEN                     36           /* 最大的mac头长度 oal_mem.h里面引用该宏 */

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
#define WLAN_MEM_MAX_SUBPOOL_NUM            6             /* 裸系统内存池中出netbuf内存池外最大子内存池个数 */
#else
#define WLAN_MEM_MAX_SUBPOOL_NUM            8             /* 内存池中最大子内存池个数 */
#endif
#define WLAN_MEM_MAX_USERS_NUM              4             /* 共享同一块内存的最大用户数 */

/*****************************************************************************
  2.4.2 共享描述符内存池配置信息
*****************************************************************************/
/* 整个device所有TID的最大MPDU数量限制
    需要重新定义，以支持调度逻辑
*/
#define WLAN_TID_MPDU_NUM_BIT               9
#define WLAN_TID_MPDU_NUM_LIMIT             (1 << WLAN_TID_MPDU_NUM_BIT)

#define WLAN_MEM_SHARED_RX_DSCR_SIZE        68              /*比实际接收描述符结构体稍大些，预留出后面对接收描述符的修改*/
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU                              /*等MAC amsdu描述符相关版本发布后打开该宏*/
#define WLAN_MEM_SHARED_TX_DSCR_SIZE1       112             /*PILOT amsdu增加8个字节*/
#define WLAN_MEM_SHARED_TX_DSCR_SIZE2       112             /*PILOT amsdu增加8个字节*/
#else
#define WLAN_MEM_SHARED_TX_DSCR_SIZE1       104             /*比实际发送描述符结构体稍大些，预留出后面对发送描述符的修改*/
#define WLAN_MEM_SHARED_TX_DSCR_SIZE2       104             /*比实际发送描述符结构体稍大些，预留出后面对发送描述符的修改*/
#endif

#if defined(_PRE_DEBUG_MODE)
#define WLAN_MEM_SHARED_RX_DSCR_CNT         110             /* 接收512(数据帧描述符) + 64(管理帧描述符) */ /* 注意! 新增一个子内存池要更新oal_mem.c里的OAL_MEM_BLK_TOTAL_CNT */
#define WLAN_MEM_SHARED_TX_DSCR_CNT1        172             /* 发送描述符512 */
#define WLAN_MEM_SHARED_TX_DSCR_CNT2        0               /* 发送amsdu的描述符 */
#else
#define WLAN_MEM_SHARED_RX_DSCR_CNT         0
#define WLAN_MEM_SHARED_TX_DSCR_CNT1        0
#define WLAN_MEM_SHARED_TX_DSCR_CNT2        0
#endif

/*****************************************************************************
  2.4.3 共享管理帧内存池配置信息
*****************************************************************************/
#define WLAN_MEM_SHARED_MGMT_PKT_SIZE1      800
#define WLAN_MEM_SHARED_MGMT_PKT_CNT1       0

/*****************************************************************************
  2.4.4 共享数据帧内存池配置信息
*****************************************************************************/
#define WLAN_MEM_SHARED_DATA_PKT_SIZE       44              /* 80211mac帧头大小 */
#define WLAN_MEM_SHARED_DATA_PKT_CNT        200             /* skb(接收的帧头个数) + 发送描述符个数(发送的帧头个数) 768 */

/*****************************************************************************
  2.4.5 本地内存池配置信息
*****************************************************************************/
#define WLAN_MEM_LOCAL_SIZE1                32
/*4+12+4 for event queue*/
#define WLAN_MEM_LOCAL_CNT1                 (69+4+12+4)     /* 256(32*8)个dmac_alg_tid_stru + 256个alg_tid_entry_stru + 5个事件队列(NON_RESET_ERR)*/

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define WLAN_MEM_LOCAL_SIZE2                144
#define WLAN_MEM_LOCAL_CNT2                 800             /* 200(杂用) */
#else
#define WLAN_MEM_LOCAL_SIZE2                104
#define WLAN_MEM_LOCAL_CNT2                 50              /* 200(杂用) */
#endif

#define WLAN_MEM_LOCAL_SIZE3                264             /* 存储hmac_vap_cfg_priv_stru，每个VAP一个 + 事件队列 FRW_EVENT_TYPE_BUTT * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT  */

/* 由于WL_L2_DRAM大小限制，目前暂时开放2个业务vap，整体规格开放待后续优化 TBD */
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
/*裸系统对应本地内存个数*/
#define WLAN_MEM_LOCAL_CNT3                 (WLAN_VAP_SUPPORT_MAX_NUM_LIMIT + 60)/* 资源池初始化需要1个 +1 */
#else
/*Linux系统对应本地内存个数，需要考虑事件队列的个数*/
#define WLAN_MEM_LOCAL_CNT3                 (WLAN_VAP_SUPPORT_MAX_NUM_LIMIT + 60)
#endif

#define WLAN_MEM_LOCAL_SIZE4                512             /* 长度为128的事件队列用 */
#define WLAN_MEM_LOCAL_CNT4                 (20+36)         /*36 for event queue*/

#define WLAN_MEM_LOCAL_SIZE5                4304
#define WLAN_MEM_LOCAL_CNT5                 64

#define WLAN_MEM_LOCAL_SIZE6                16000           /* 资源池用户初始化使用 */
#define WLAN_MEM_LOCAL_CNT6                 0               /* 目前0片*/
/*****************************************************************************
  2.4.6 高速本地内存池(放在TCM中)配置信息
*****************************************************************************/

/*****************************************************************************
  2.4.6 事件结构体内存池
*****************************************************************************/
#define WLAN_MEM_EVENT_SIZE1                72              /* 注意: 事件内存长度包括4字节IPC头长度 */
#define WLAN_MEM_EVENT_SIZE2                528             /* 注意: 事件内存长度包括4字节IPC头长度 */

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION) || (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
/*裸系统对应本地内存个数*/
#define WLAN_MEM_EVENT_CNT1                 180
#define WLAN_MEM_EVENT_CNT2                 8   /* 03 APUT时候满规格比02大一倍，删除用户保证抛事件正常，
                                                   后续看这个事件是否需要优化使用第一片 */

#else
/*Linux系统对应本地内存个数，需要考虑事件队列大小*/
#define WLAN_MEM_EVENT_CNT1                 (180 * WLAN_FRW_MAX_NUM_CORES)
#define WLAN_MEM_EVENT_CNT2                 8   /* 03 APUT时候满规格比02大一倍，删除用户保证抛事件正常，
                                                   后续看这个事件是否需要优化使用第一片 */
#endif

#define WLAN_WPS_IE_MAX_SIZE                608   /*03规格扩容*/
#ifdef _PRE_WLAN_FEATURE_HILINK
#define WLAN_OKC_IE_MAX_SIZE                WLAN_MEM_EVENT_SIZE2 - 32   /* 32表示事件自身占用的空间 */
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
#define WLAN_M2S_IE_MAX_SIZE                WLAN_WPS_IE_MAX_SIZE
#endif

/*****************************************************************************
  2.4.7 用户内存池
*****************************************************************************/
/*****************************************************************************
  2.4.8 MIB内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/
#define WLAN_MEM_MIB_SIZE1                  1320           /* mib结构体大小 */
#define WLAN_MEM_MIB_CNT1                   WLAN_SERVICE_VAP_SUPPORT_MAX_NUM_LIMIT /* 原来为((WLAN_VAP_SUPPORT_MAX_NUM_LIMIT - 1) * 2) */ /* 配置VAP没有MIB */
#if (WLAN_VAP_SUPPORT_MAX_NUM_LIMIT < (WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE* WLAN_CHIP_MAX_NUM_PER_BOARD * WLAN_DEVICE_MAX_NUM_PER_CHIP))
#error invaild WLAN_MEM_MIB_CNT1
#endif
/*****************************************************************************
  2.4.9 netbuf内存池  TBD :最终个子池的空间大小及个数需要重新考虑
*****************************************************************************/

#if ((_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION))
#define  WLAN_MEM_NETBUF_SIZE1              180     /* 短帧netbufpayload长度 */
#define  WLAN_MEM_NETBUF_CNT1               70     /* 短帧netbufpayload个数2 */

#define  WLAN_MEM_NETBUF_SIZE2              800     /* 管理帧netbufpayload长度 */
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
#define  WLAN_MEM_NETBUF_CNT2               8      /* 管理帧netbufpayload个数 8*/
#elif (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
#define  WLAN_MEM_NETBUF_CNT2               16      /* 描述符用完所有管理帧内存，UT测试归还后与初始化不一致，导致UT测试内存检查出现泄漏，该问题不是问题 */
#endif

#define  WLAN_MEM_NETBUF_SIZE3              1568    /* 长帧netbufpayload长度 */
#define  WLAN_MEM_NETBUF_CNT3               176     /* 长帧netbufpayload个数 */

#define  WLAN_MEM_NETBUF_SIZE4              512     /* SDT 内存池长度 */
#define  WLAN_MEM_NETBUF_CNT4               4       /* SDT 内存个数 */

#define  OAL_NETBUF_MACHDR_BYTE_LEN         64      /* netbuf mac头长度 */

#define WLAN_LARGE_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE3 /* NETBUF内存池长帧的长度，统一用这个宏 */
#define WLAN_MGMT_NETBUF_SIZE         WLAN_MEM_NETBUF_SIZE2 /* NETBUF内存池管理帧的长度，统一用这个宏 */
#define WLAN_SHORT_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE1 /* NETBUF内存池短帧的长度，统一用这个宏 */
#define WLAN_SDT_NETBUF_SIZE          WLAN_MEM_NETBUF_SIZE4 /* NETBUF内存池SDT的长度，统一用这个宏 */

#define WLAN_MAX_NETBUF_SIZE         (WLAN_LARGE_NETBUF_SIZE + WLAN_MAX_MAC_HDR_LEN)  /* netbuf最大帧长，帧头 + payload */

#else
#define WLAN_MEM_NETBUF_SIZE1               0       /* 克隆用SKB */
#define WLAN_MEM_NETBUF_CNT1                192     /* 接收数据帧是AMSDU，其中的每个MSDU对应一个克隆netbuf */

#ifndef _PRE_WLAN_PHY_PERFORMANCE
#define WLAN_MEM_NETBUF_SIZE2               1600    /* 1500 + WLAN_MAX_FRAME_HEADER_LEN(36) + WLAN_HDR_FCS_LENGTH(4) + (解密失败的话,加密字段也会上报(20)) */
#define WLAN_MEM_NETBUF_CNT2                512     /* 接收192(接收数据帧) + 32(接收管理帧) + 32(发送管理帧) */
                                                            /* 考虑接收wlan2wlan转发场景，在上面的基础上x2 */
#define WLAN_MEM_NETBUF_SIZE3               2500    /* 解分片用重组报文的skb */
#define WLAN_MEM_NETBUF_CNT3                32      /* 活跃用户每个用户一个 */

#else
#define WLAN_MEM_NETBUF_SIZE2               5100
#define WLAN_MEM_NETBUF_CNT2                512

#define WLAN_MEM_NETBUF_SIZE3               5100    /* 解分片用重组报文的skb */
#define WLAN_MEM_NETBUF_CNT3                32      /* 活跃用户每个用户一个 */
#endif

#define  WLAN_MEM_NETBUF_SIZE4              512     /* SDT 内存池长度 */
#define  WLAN_MEM_NETBUF_CNT4               4       /* SDT 内存个数 */

#define WLAN_LARGE_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE2   /* NETBUF内存池长帧的长度，统一用这个宏 */
#define WLAN_MGMT_NETBUF_SIZE         WLAN_MEM_NETBUF_SIZE2   /* NETBUF内存池管理帧的长度，统一用这个宏 */
#define WLAN_SHORT_NETBUF_SIZE        WLAN_MEM_NETBUF_SIZE2   /* NETBUF内存池短帧的长度，统一用这个宏 */
#define WLAN_MAX_NETBUF_SIZE          WLAN_LARGE_NETBUF_SIZE  /* netbuf最大帧长，帧头 + payload */
#define WLAN_SDT_NETBUF_SIZE          WLAN_MEM_NETBUF_SIZE4   /* NETBUF内存池SDT的长度，统一用这个宏 */

#endif

#define WLAN_MEM_NETBUF_ALIGN               4       /* netbuf对齐 */
#define WLAN_MEM_ETH_HEADER_LEN             14      /* 以太网帧头长度 */

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
#define WLAN_MEM_SDT_NETBUF_SIZE1_CNT   250
#define WLAN_MEM_SDT_NETBUF_SIZE2       (WLAN_MEM_SDT_NETBUF_PAYLOAD2 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE2_CNT   250
#define WLAN_MEM_SDT_NETBUF_SIZE3       (WLAN_MEM_SDT_NETBUF_PAYLOAD3 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE3_CNT   250
#define WLAN_MEM_SDT_NETBUF_SIZE4       (WLAN_MEM_SDT_NETBUF_PAYLOAD4 + WLAN_SDT_SKB_RESERVE_LEN)
#define WLAN_MEM_SDT_NETBUF_SIZE4_CNT   256

#define WLAN_SDT_MSG_FLT_HIGH_THD           800
#define WLAN_SDT_MSG_QUEUE_MAX_LEN          (WLAN_MEM_SDT_NETBUF_SIZE1_CNT + \
                                                     WLAN_MEM_SDT_NETBUF_SIZE2_CNT + \
                                                     WLAN_MEM_SDT_NETBUF_SIZE3_CNT + \
                                                     WLAN_MEM_SDT_NETBUF_SIZE4_CNT - 6)  /* 入队数比内存池要少，此处取整1000 */

/*****************************************************************************
  2.4.10 RF通道数规格已放入WLAN_SPEC
*****************************************************************************/

/*****************************************************************************
  2.4.11 TCP ACK优化
*****************************************************************************/
/* tcp ack过滤功能不再默认打开，根据流量调整 */
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
/*host event cfg table*/
#define FRW_EVENT_MAX_NUM_QUEUES    (FRW_EVENT_TYPE_BUTT * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)

#define WLAN_FRW_EVENT_CFG_TABLE_PER_VAP \
    /* 事件类型       队列权重   队列所能容纳的最大事件个数   队列所属调度策略 */  \
    /* HIGH_PRIO */     {   1,               32,                      0, 0}, \
    /* HOST_CRX */      {   1,               64,                      1, 0}, \
    /* HOST_DRX */      {   1,               64,                      1, 0}, \
    /* HOST_CTX */      {   1,               64,                      1, 0}, \
    /* HOST_SDT */      {   1,               64,                      1, 0}, \
    /* WLAN_CRX */      {   1,               64,                      1, 0}, \
    /* WLAN_DRX */      {   1,               64,                      1, 0}, \
    /* WLAN_CTX */      {   1,               64,                      1, 0}, \
    /* WLAN_DTX */      {   1,               64,                      1, 0}, \
    /* WLAN_TX_COMP */  {   1,                0,                      1, 0}, \
    /* TBTT */          {   1,                0,                      1, 0}, \
    /* TIMEOUT */       {   1,                2,                      1, 0}, \
    /* HMAC MISC */     {   1,                0,                      1, 0}, \
    /* DMAC MISC */     {   1,               64,                      0, 0},

/*Host, double device */
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
  }

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
#endif /* #if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* #ifndef __PLATFORM_SPEC_1103_H__ */



