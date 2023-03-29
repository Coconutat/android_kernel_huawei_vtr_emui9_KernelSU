


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wal_regdb.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_REGDB_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
#include "wal_regdb_e5.c"
#elif (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
#include "wal_regdb_ont.c"
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
#include "wal_regdb_1103.c"
#else
#include "wal_regdb_default.c"
#endif

/* 默认管制域信息 */
OAL_CONST oal_ieee80211_regdomain_stru g_st_default_regdom_etc = {
    .n_reg_rules = 4,
    .alpha2 =  "99",
    .reg_rules = {
        /* IEEE 802.11b/g, 信道 1..13 */
        REG_RULE(2412-10, 2472+10, 40, 6, 25, 0),/*lint !e651*/

        /* 信道 36 - 64 */
        REG_RULE(5150-10, 5350+10, 160, 6, 25, 0),

        /* 信道 100 - 165 */
        REG_RULE(5470-10, 5850+10, 160, 6, 25, 0),

        /* IEEE 802.11 信道 184,188,192,196 ，对于日本4.9G */
        REG_RULE(4920-10, 4980+10, 80, 6, 25, 0),
    }
};


#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
/* Win32下代码只封装几个国家的管制域信息， 为了UT， */
OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_AU = {
    5,
    {'A', 'U'},
    {0, 0},
    {
        REG_RULE(2402, 2482, 40, 0, 20, 0),
        REG_RULE(5170, 5250, 80, 3, 17, 0),
        REG_RULE(5250, 5330, 80, 3, 24,
            NL80211_RRF_DFS | 0),
        REG_RULE(5490, 5710, 80, 3, 24, 0),
        REG_RULE(5735, 5835, 80, 3, 30, 0),
    }
};

OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_AT= {
    4,
    {'A', 'T'},
    {0, 0},
    {
        REG_RULE(2402, 2482, 40, 0, 20, 0),
        REG_RULE(5170, 5250, 80, 0, 23, 0),
        REG_RULE(5250, 5330, 80, 0, 23,
            NL80211_RRF_DFS | 0),
        REG_RULE(5490, 5710, 80, 0, 23,
            NL80211_RRF_DFS | 0),
    }
};

OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_CN = {
    2,
    {'C', 'N'},
    {0, 0},
    {
        REG_RULE(2402, 2482, 40, 0, 20, 0),
        REG_RULE(5735, 5835, 80, 0, 30, 0),
    }
};

OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_JP = {
    7,
    {'J', 'P'},
    {0, 0},
    {
        REG_RULE(2402, 2482, 40, 0, 20, 0),
        REG_RULE(2474, 2494, 20, 0, 20,
            NL80211_RRF_NO_OFDM | 0),
        REG_RULE(4910, 4990, 40, 0, 23, 0),
        REG_RULE(5030, 5090, 80, 0, 23, 0),
        REG_RULE(5170, 5250, 80, 0, 20, 0),
        REG_RULE(5250, 5330, 80, 0, 20,
            NL80211_RRF_DFS | 0),
        REG_RULE(5490, 5710, 80, 0, 23,
            NL80211_RRF_DFS | 0),
    }
};

OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_GB = {
    4,
    {'G', 'B'},
    {0, 0},
    {
        REG_RULE(2402, 2482, 40, 0, 20, 0),
        REG_RULE(5170, 5250, 80, 0, 20, 0),
        REG_RULE(5250, 5330, 80, 0, 20,
            NL80211_RRF_DFS | 0),
        REG_RULE(5490, 5710, 80, 0, 27,
            NL80211_RRF_DFS | 0),
    }
};

OAL_STATIC OAL_CONST oal_ieee80211_regdomain_stru regdom_US = {
    6,
    {'U', 'S'},
    {0, 0},
    {
        REG_RULE(2402, 2472, 40, 3, 27, 0),
        REG_RULE(5170, 5250, 80, 3, 17, 0),
        REG_RULE(5250, 5330, 80, 3, 20,
            NL80211_RRF_DFS | 0),
        REG_RULE(5490, 5600, 80, 3, 20,
            NL80211_RRF_DFS | 0),
        REG_RULE(5650, 5710, 80, 3, 20,
            NL80211_RRF_DFS | 0),
        REG_RULE(5735, 5835, 80, 3, 30, 0),
    },
};

OAL_CONST oal_ieee80211_regdomain_stru *reg_regdb_etc[] = {
    &regdom_AU,
    &regdom_AT,
    &regdom_CN,
    &regdom_JP,
    &regdom_GB,
    &regdom_US,
};

int reg_regdb_size_etc = OAL_ARRAY_SIZE(reg_regdb_etc);

/* 默认管制域信息 */
OAL_CONST oal_ieee80211_regdomain_stru g_st_default_regdom_etc = {
    4,              /* n_reg_rules */
    {'9', '9'},     /* alpha2 */
    {0, 0},
    {   /* reg_rules */
        /* IEEE 802.11b/g, 信道 1..13 */
        REG_RULE(2402, 2482, 40, 6, 20, 0),

        /* 信道 36 - 64 */
        REG_RULE(5150-10, 5350+10, 40, 6, 20, (0)),

        /* 信道 100 - 165 */
        REG_RULE(5470-10, 5850+10, 40, 6, 20, (0)),

        /* IEEE 802.11 信道 184,188,192,196 在日本使用 4.9G */
        REG_RULE(4920-10, 4980+10, 40, 6, 23, (0)),
    },
};

#endif



/*****************************************************************************
  3 函数实现
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_11D

OAL_CONST oal_ieee80211_regdomain_stru* wal_regdb_find_db_etc(oal_int8 *pc_str)
{
    oal_int32 l_db_idx;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_str))
    {
        return OAL_PTR_NULL;
    }

    /* 默认管制域 */
    if (('9' == pc_str[0]) && ('9' == pc_str[1]))
    {
        return &g_st_default_regdom_etc;
    }

    for (l_db_idx = 0; l_db_idx < reg_regdb_size_etc; l_db_idx++)
    {
        if ((pc_str[0] == reg_regdb_etc[l_db_idx]->alpha2[0]) &&
            (pc_str[1] == reg_regdb_etc[l_db_idx]->alpha2[1]))
        {
            return reg_regdb_etc[l_db_idx];
        }

    }

    return OAL_PTR_NULL;
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif






