/******************************************************************************

  Copyright(C)2008,Hisilicon Co. LTD.

 ******************************************************************************
  File Name       : NasNvInterface.h
  Description     : NasNvInterface.h header file
  History         :

******************************************************************************/

#ifndef __NASNVINTERFACE_H__
#define __NASNVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

#include "PsTypeDef.h"

/*****************************************************************************
  2 Macro
*****************************************************************************/
#define NAS_MMC_NV_ITEM_ACTIVE          (1)                                     /* NV项激活 */
#define NAS_MMC_NV_ITEM_DEACTIVE        (0)                                     /* NV项未激活 */

/* GPRS GEA 算法支持宏定义 */
#define NAS_MMC_GPRS_GEA1_SUPPORT       (0x01)                                  /* 支持GPRS GEA1算法 */
#define NAS_MMC_GPRS_GEA1_VALUE         (0x80)                                  /* 支持GEA1时的网络能力值 */
#define NAS_MMC_GPRS_GEA2_VALUE         (0x40)                                  /* 支持GEA2时的网络能力值 */
#define NAS_MMC_GPRS_GEA3_VALUE         (0x20)                                  /* 支持GEA3时的网络能力值 */
#define NAS_MMC_GPRS_GEA4_VALUE         (0x10)                                  /* 支持GEA4时的网络能力值 */
#define NAS_MMC_GPRS_GEA5_VALUE         (0x08)                                  /* 支持GEA5时的网络能力值 */
#define NAS_MMC_GPRS_GEA6_VALUE         (0x04)                                  /* 支持GEA6时的网络能力值 */
#define NAS_MMC_GPRS_GEA7_VALUE         (0x02)                                  /* 支持GEA7时的网络能力值 */

#define NAS_MMC_GPRS_GEA2_SUPPORT       (0x02)                                  /* 支持GPRS GEA2算法 */
#define NAS_MMC_GPRS_GEA3_SUPPORT       (0x04)                                  /* 支持GPRS GEA3算法 */
#define NAS_MMC_GPRS_GEA4_SUPPORT       (0x08)                                  /* 支持GPRS GEA4算法 */
#define NAS_MMC_GPRS_GEA5_SUPPORT       (0x10)                                  /* 支持GPRS GEA5算法 */
#define NAS_MMC_GPRS_GEA6_SUPPORT       (0x20)                                  /* 支持GPRS GEA6算法 */
#define NAS_MMC_GPRS_GEA7_SUPPORT       (0x40)                                  /* 支持GPRS GEA7算法 */
#define NAS_MMC_NVIM_MAX_EPLMN_NUM      (16)                                    /* en_NV_Item_EquivalentPlmn NV中等效plmn个数 */
#define NAS_MMC_NVIM_MAX_MCC_SIZE       (3)                                     /* plmn中Mcc最大长度 */
#define NAS_MMC_NVIM_MAX_MNC_SIZE       (3)                                     /* plmn中Mnc最大长度 */
#define NAS_MMC_LOW_BYTE_MASK           (0x0f)

#define NAS_MMC_A5_1_SUPPORT            (0x0001)                                  /* 支持A5-1算法 */
#define NAS_MMC_A5_2_SUPPORT            (0x0002)                                  /* 支持A5-2算法 */
#define NAS_MMC_A5_3_SUPPORT            (0x0004)                                  /* 支持A5-3算法 */
#define NAS_MMC_A5_4_SUPPORT            (0x0008)                                  /* 支持A5-4算法 */
#define NAS_MMC_A5_5_SUPPORT            (0x0010)                                  /* 支持A5-5算法 */
#define NAS_MMC_A5_6_SUPPORT            (0x0020)                                  /* 支持A5-6算法 */
#define NAS_MMC_A5_7_SUPPORT            (0x0040)                                  /* 支持A5-7算法 */

#define NAS_MMC_NVIM_MAX_USER_CFG_IMSI_PLMN_NUM                  (6)                 /* 用户配置的最多可支持的USIM/SIM卡的个数 */
#define NAS_MMC_NVIM_MAX_USER_CFG_EHPLMN_NUM                     (6)                 /* 用户配置的EHplmn的个数 */
#define NAS_MMC_MAX_BLACK_LOCK_PLMN_WITH_RAT_NUM            (8)                 /* 禁止接入技术的PLMN ID的最大个数 */
#define NAS_MMC_NVIM_MAX_USER_CFG_EXT_EHPLMN_NUM              (8)                /* 扩展的NV项的EHplmn组的最大个数*/
#define NAS_MMC_NVIM_MAX_USER_CFG_FORB_PLMN_GROUP_NUM         (8)                /* 用户配置的forb plmn的最大组数 */

#define NAS_MMC_NVIM_MAX_USER_OPLMN_VERSION_LEN               (8)               /* 用户配置的OPLMN版本号最大长度 */
#define NAS_MMC_NVIM_MAX_USER_OPLMN_IMSI_NUM                  (6)               /* 用户配置的OPLMN最多可支持的USIM/SIM卡的个数 */
#define NAS_MMC_NVIM_MAX_USER_CFG_OPLMN_NUM                   (256)             /* 用户配置的OPLMN的最大个数 */
#define NAS_MMC_NVIM_OPLMN_WITH_RAT_UNIT_LEN                  (5)               /* 用户配置的带接入技术OPLMN基本单元长度，如6F61文件的基本长度单元为5 */
#define NAS_MMC_NVIM_MAX_USER_CFG_OPLMN_DATA_LEN              (500)             /* 用户配置OPLMN的最大字节数,扩容前只支持500*/
#define NAS_MMC_NVIM_MAX_USER_CFG_OPLMN_DATA_EXTEND_LEN       (1280)            /* 扩展后的用户配置OPLMN的最大字节数*/


/* 扩容:由128改256; 增加预置类型信息,由6改7*/
#define NAS_MMC_NVIM_MAX_CFG_DPLMN_DATA_LEN             (7*256)            /* 扩展后的用户配置DPLMN的最大字节数 */
#define NAS_MMC_NVIM_MAX_CFG_NPLMN_DATA_LEN             (7*256)            /* 扩展后的用户配置NPLMN的最大字节数 */

#define NAS_MMC_NVIM_MAX_CFG_DPLMN_DATA_EXTEND_LEN       (6*128)            /* 扩展后的用户配置DPLMN的最大字节数*/
#define NAS_MMC_NVIM_MAX_CFG_NPLMN_DATA_EXTEND_LEN       (6*128)            /* 扩展后的用户配置NPLMN的最大字节数*/
#define NAS_MMC_NVIM_MAX_CFG_HPLMN_NUM                   (3*8)
#define NAS_MCC_NVIM_VERSION_LEN                         (9)                /* 版本号，固定为xx.xx.xxx */

#define NAS_MMC_NVIM_MAX_CFG_DPLMN_NUM                   (256)            /* 扩展后的用户配置DPLMN个数 */
#define NAS_MMC_NVIM_MAX_CFG_NPLMN_NUM                   (256)            /* 扩展后的用户配置NPLMN个数 */
/* 边境信息数据,每7个字节代表一条边境信息，第1-3个字节为sim卡格式plmn id，
   第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，第6-7字节为LAC或TAC信息
   接入技术为WG时，6-7字节表示LAC;接入技术为LTE时，6-7字节表示TAC */
#define NAS_MMC_EVERY_BORDER_INFO_LEN                    (7)              /* en_NV_Item_Ap_Preset_Border_Info nv中每条边境信息占用字节数 */
#define NAS_MMC_NVIM_MAX_CFG_BORDER_DATA_LEN             (7*256)          /* AP预置边境信息的最大数据长度，每条边境信息占用7字节，最多预置256条边境信息 */
#define NAS_NVIM_MAX_OPER_SHORT_NAME_LEN                36
#define NAS_NVIM_MAX_OPER_LONG_NAME_LEN                 40

#define NAS_NVIM_ITEM_MAX_IMSI_LEN          (9)                     /* 最大的IMSI的长度 */
#define NAS_MML_NVIM_PS_LOCI_SIM_FILE_LEN                    (14)                    /* USIM中PSLOCI文件的长度 */
#define NAS_NVIM_PS_LOCI_SIM_FILE_LEN                    (14)                    /* USIM中PSLOCI文件的长度 */

#define NAS_UTRANCTRL_MAX_NVIM_CFG_TD_MCC_LIST_NUM          (6)                 /* 模式自动切换中最大的可配置国家码个数 */
#define NAS_UTRANCTRL_MAX_NVIM_CFG_IMSI_PLMN_LIST_NUM       (6)                 /* 模式自动切换中最大的可配置USIM/IMSI的网络号个数 */

#define NAS_MML_NVIM_MAX_DISABLED_RAT_PLMN_NUM              (8)                 /* 禁止接入技术的PLMN ID的最大个数 */

#define NAS_MML_NVIM_MAX_SKIP_BAND_TYPE_SEARCH_MCC_NUM      (8)

#define NAS_MML_MAX_EXTENDED_FORB_PLMN_NUM                  (32)

#define NAS_MML_NVIM_FRAT_MAX_IMSI_NUM                       (6)                 /* FRAT特性最大支持IMSI个数 */
#define NAS_MML_NVIM_FRAT_MAX_PLMN_ID_NUM                    (40)                /* FRAT特性最大支持PLMN个数 */

#define NAS_SIM_FORMAT_PLMN_LEN                     (3)                     /* Sim卡格式的Plmn长度 */

#define NAS_MML_NVIM_MAX_BLACK_LOCK_PLMN_NUM                 (16)                    /* 黑名单锁网支持的PLMN ID的最大个数 */

#define NAS_MML_NVIM_MAX_WHITE_LOCK_PLMN_NUM                 (16)                    /* 白名单锁网支持的PLMN ID的最大个数 */

#define NAS_MML_BG_SEARCH_REGARDLESS_MCC_NUMBER         (10)                     /* BG搜不考虑国家码的国家码最大个数 */
#define NAS_MML_SINGLE_DOMAIN_FAIL_ACTION_MAX_LIST      (5)                     /* 定制原因值最大列表 */

#define NV_ITEM_NET_CAPABILITY_MAX_SIZE                     (10)

#define NAS_MMC_NVIM_SUPPORTED_3GPP_RELEASE_SIZE            (2)                 /* 当前支持的协议版本 */

#define NAS_MMC_NVIM_MAX_IMSI_LEN                           (9)                     /* 最大的IMSI的长度 */

#define NVIM_MAX_EPLMN_NUM                                  (16)
#define NVIM_MAX_MCC_SIZE                                   (3)
#define NVIM_MAX_MNC_SIZE                                   (3)

#define NVIM_MAX_FDD_FREQ_BANDS_NUM                         (12)

/* Add by z60575 for multi_ssid, 2012-9-5 end */
/*+CGMI - 获取制造商名称*/

#define NAS_NVIM_MAX_IMSI_FORBIDDEN_LIST_NUM         (16)
#define NAS_NVIM_MAX_RAT_FORBIDDEN_LIST_NUM          (8)    /* 预留拓展 */
#define NAS_NVIM_MAX_SUPPORTED_FORBIDDEN_RAT_NUM     (2)

#define NAS_NVIM_FORBIDDEN_RAT_NUM_0                 (0)
#define NAS_NVIM_FORBIDDEN_RAT_NUM_1                 (1)
#define NAS_NVIM_FORBIDDEN_RAT_NUM_2                 (2)


#define NAS_NVIM_MAX_REJECT_NO_RETRY_CAUSE_NUM              (8)

#define NAS_NVIM_MAX_LAU_REJ_TRIG_PLMN_SEARCH_CAUSE_NUM     (12)

/* 对NVID枚举的转定义(PS_NV_ID_ENUM, SYS_NV_ID_ENUM, RF_NV_ID_ENUM) */
typedef VOS_UINT16  NV_ID_ENUM_U16;
#define NV_ITEM_IMEI_SIZE                      16
#define NV_ITEM_MMA_OPERATORNAME_SIZE          84

#define NV_ITEM_OPER_NAME_LONG          (40)
#define NV_ITEM_OPER_NAME_SHORT         (36)
#define NV_ITEM_PLMN_ID_LEN             (8)

#define NV_ITEM_AT_PARA_SIZE                   100
#define NV_ITEM_HPLMN_FIRST_SEARCH_SIZE        1  /* 第一次HPLMN搜索的时间间隔 */
#define NVIM_ITEM_MAX_IMSI_LEN          (9)                     /* 最大的IMSI的长度 */

#define CNAS_NVIM_MAX_1X_MRU_SYS_NUM                            (12)

#define NAS_MMC_NVIM_MAX_CAUSE_NUM      (10)     /* NV配置原因值最大个数 */

#define CNAS_NVIM_PRL_SIZE                                      (4096) /* PRL NV size: 4K byte */

#define CNAS_NVIM_CBT_PRL_SIZE                                  (48) /* PRL NV size: 45 bytes */

#define CNAS_NVIM_MAX_1X_BANDCLASS_NUM                          (32)

#define CNAS_NVIM_MAX_1X_HOME_SID_NID_NUM                       (20)
#define CNAS_NVIM_MAX_OOC_SCHEDULE_PHASE_NUM                    (8)

#define CNAS_NVIM_MAX_1X_OOC_SCHEDULE_PHASE_NUM                    (8)

#define CNAS_NVIM_MAX_HRPD_MRU_SYS_NUM                           (12)
#define CNAS_NVIM_HRPD_SUBNET_LEN                                (16)

#define CNAS_NVIM_MAX_STORAGE_BLOB_LEN                      ( 255 )

#define NAS_NVIM_BYTES_IN_SUBNET                            (16)
#define NAS_NVIM_MAX_RAT_NUM                                (7)                 /* 预留出1X和HRPD */

#define NAS_MSCC_NVIM_MLPL_SIZE                                      (1024) /* PRL NV size: 1K byte */
#define NAS_MSCC_NVIM_MSPL_SIZE                                      (1024) /* PRL NV size: 1K byte */

#define NAS_NV_TRI_MODE_CHAN_PARA_PROFILE_NUM      ( 8 )                       /*  包含全网通性特的通道配置场景数目 */

#define CNAS_NVIM_ICCID_OCTET_LEN                            (10)
#define CNAS_NVIM_MEID_OCTET_NUM                             (7)
#define CNAS_NVIM_UATI_OCTET_LENGTH                          (16)

#define CNAS_NVIM_MAX_WHITE_LOCK_SYS_NUM                     (20)

#define CNAS_NVIM_MAX_HRPD_CUSTOMIZE_FREQ_NUM                (10)

#define CNAS_NVIM_MAX_CDMA_1X_CUSTOM_PREF_CHANNELS_NUM              (10)
#define CNAS_NVIM_MAX_CDMA_1X_CUSTOMIZE_PREF_CHANNELS_NUM           (20)

/* 高优先级PLMN refresh 触发背景搜默认延迟时长: 单位 秒 */
#define NV_ITEM_HIGH_PRIO_PLMN_REFRESH_TRIGGER_BG_SEARCH_DEFAULT_DELAY_LEN    (5)

#define NAS_NVIM_MAX_PLMN_CSG_ID_NUM             (35)
#define NAS_NVIM_MAX_CSG_REJ_CAUSE_NUM           (10)

#define NAS_NVIM_LTE_OOS_2G_PREF_PLMN_SEL_MAX_IMSI_LIST_NUM      (16)    /* SIM卡列表 (LTE OOS后先搜2G再搜3G) */

#define NAS_MML_NVIM_MAX_REG_FAIL_CAUSE_NUM        (16)        /*支持的禁止LA列表大小*/
#define CNAS_NVIM_MAX_AUTHDATA_USERNAME_LEN                      (253)   /* HRPD ppp AN鉴权用户名最长用户名，
                                                                            参考C.S0023-D section 3.4.53以及
                                                                            C.S0016-D,section 3.5.8.13,
                                                                            该长度理论最长为255-2(NAI长度以
                                                                            及鉴权算法保留字节占用的字节) */

#define CNAS_NVIM_1X_MAX_MRU_SYS_NUM                   (12)

#define CNAS_NVIM_1X_AVOID_MAX_PHASE_NUM               (8)
#define CNAS_NVIM_1X_AVOID_REASON_MAX                  (20)

#define CNAS_NVIM_MAX_HRPD_OOC_SCHEDULE_PHASE_NUM                    (8)

#define CNAS_NVIM_HRPD_AVOID_MAX_PHASE_NUM               (8)
#define CNAS_NVIM_HRPD_AVOID_REASON_MAX                  (16)

#define CNAS_NVIM_HOME_MCC_MAX_NUM                      (5)

#define NAS_NVIM_MAX_BSR_PHASE_NUM                      (2)

#define NAS_MMC_NVIM_MAX_EXTENDED_FORB_PLMN_NUM         (32)
#define NAS_NVIM_MAX_LIMIT_PDP_ACT_PLMN_NUM             (8)
#define NAS_NVIM_MAX_LIMIT_PDP_ACT_CAUSE_NUM            (8)

#define NAS_MAX_TMSI_LEN                                (4)                     /* 最大的TMSI的长度 */
#define NAS_SIM_MAX_LAI_LEN                             (6)                     /* SIM卡中保存的LAI最大长度 */
#define NAS_MMC_NVIM_MAX_CUSTOM_SUPPLEMENT_OPLMN_NUM    (16)

#define CNAS_NVIM_MAX_CDMA_HRPD_CUSTOMIZE_PREF_CHANNELS_NUM           (20)

#define NAS_NVIM_MODE_SELECTION_RETRY_TIMER_PHASE_NUM_MAX             (10)

#define NAS_NVIM_MODE_SELECTION_PUNISH_TIMER_PHASE_NUM_MAX            (10)

#define NAS_NVIM_MODE_SELECTION_RETRY_SYS_ACQ_PHASE_NUM_MAX           (4)

#define NAS_NVIM_CUSTOMIZE_MAX_CL_ACQ_SCENE_NUM                       (40)

#define NAS_NVIM_CHINA_BOUNDARY_NETWORK_NUM_MAX             (30)

#define NAS_NVIM_CHINA_HOME_NETWORK_NUM_MAX                 (5)

#define NAS_NVIM_1X_MT_EST_CNF_REEST_CAUSE_MAX_NUM          (10)
#define NAS_NVIM_1X_MT_TERMINATE_IND_REEST_CAUSE_MAX_NUM    (9)
/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/

enum NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_ENUM
{
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_PLMN_SELECTION                    = 0,            /* 触发搜网 */
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_NORMAL_CAMP_ON                    = 1,            /* 正常驻留 */
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_OPTIONAL_PLMN_SELECTION           = 2,            /* 触发可选搜网 */
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_LIMITED_CAMP_ON                   = 3,            /* 限制驻留 */

    NAS_MMC_NVIM_SINGLE_DOMAIN_ROAMING_REG_FAIL_ACTION_PLMN_SELECTION            = 4,            /* 在漫游网络上注册发起搜网，在HOME网络上不生效 */

    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_BUTT
};
typedef VOS_UINT8 NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_ENUM_UINT8;


enum NAS_MMC_NVIM_REG_FAIL_CAUSE_ENUM
{
    NAS_MMC_NVIM_REG_FAIL_CAUSE_GPRS_SERV_NOT_ALLOW_IN_PLMN = 14,
    NAS_MMC_NVIM_REG_FAIL_CAUSE_TIMER_TIMEOUT               = 301,                                 /* 等待网侧结果定时器超时 */
    NAS_MMC_NVIM_REG_FAIL_CAUSE_BUTT
};
typedef VOS_UINT16 NAS_MMC_NVIM_REG_FAIL_CAUSE_ENUM_UINT16;


enum NAS_MMC_NVIM_REG_DOMAIN_ENUM
{
    NAS_MMC_NVIM_REG_DOMAIN_CS = 1,
    NAS_MMC_NVIM_REG_DOMAIN_PS = 2,                                 /* 等待网侧结果定时器超时 */
    NAS_MMC_NVIM_REG_DOMAIN_BUTT
};
typedef VOS_UINT8 NAS_MMC_NVIM_REG_DOMAIN_ENUM_UINT8;



enum NAS_MMC_UCS2_ENUM
{
    NAS_MMC_UCS2_HAS_PREFER                                = 0,
    NAS_MMC_UCS2_NO_PREFER                                 = 1,

    NAS_MMC_UCS2_BUTT
};

typedef VOS_UINT16 NAS_MMC_UCS2_ENUM_UINT16;


enum NV_MS_MODE_ENUM
{
    NV_MS_MODE_CS_ONLY,                                                 /* 仅支持CS域 */
    NV_MS_MODE_PS_ONLY,                                                 /* 仅支持PS域 */
    NV_MS_MODE_CS_PS,                                                   /* CS和PS都支持 */

    NV_MS_MODE_ANY,                                                     /* ANY,相当于仅支持CS域 */

    NV_MS_MODE_BUTT
};
typedef VOS_UINT8 NV_MS_MODE_ENUM_UINT8;


enum NAS_NV_LTE_CS_SERVICE_CFG_ENUM
{
    NAS_NV_LTE_SUPPORT_CSFB_AND_SMS_OVER_SGS = 1,           /* 支持cs fallback和sms over sgs*/
    NAS_NV_LTE_SUPPORT_SMS_OVER_SGS_ONLY,                   /* 支持sms over sgs only*/
    NAS_NV_LTE_SUPPORT_1XCSFB,                              /* 支持1XCSFB */
    NAS_NV_LTE_SUPPORT_BUTT
};
typedef VOS_UINT8 NAS_NV_LTE_CS_SERVICE_CFG_ENUM_UINT8;



enum NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_ENUM
{
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_INACTIVE     = 0,        /* 功能不生效 */
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_CS_PS,                   /* 修改CS+PS的拒绝原因值 */
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_CS_ONLY,                 /* 仅修改CS域的拒绝原因值 */
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_PS_ONLY,                 /* 仅修改PS域的拒绝原因值 */
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_BUTT
};
typedef VOS_UINT8 NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_ENUM_UINT8;


enum NAS_NVIM_CHAN_REPEAT_SCAN
{
    NAS_NVIM_CHAN_SCAN_NORMAL,
    NAS_NVIM_CHAN_REPEAT_SCAN_REACQ_0_1_2_3_4S,
    NAS_NVIM_CHAN_REPEAT_SCAN_PING_5,
    NAS_NVIM_CHAN_REPEAT_SCAN_2_7,

    NAS_NVIM_CHAN_REPEAT_SCAN_BUTT
};
typedef VOS_UINT8 NAS_NVIM_CHAN_REPEAT_SCAN_ENUM_UINT8;


enum NAS_NVIM_EPDSZID_SUPPORT_TYPE_ENUM
{
    NAS_NVIM_EPDSZID_SUPPORT_TYPE_PDSZID,
    NAS_NVIM_EPDSZID_SUPPORT_TYPE_PDSZID_SID,
    NAS_NVIM_EPDSZID_SUPPORT_TYPE_PDSZID_SID_NID,

    NAS_NVIM_EPDSZID_SUPPORT_TYPE_BUTT
};
typedef VOS_UINT8 NAS_NVIM_EPDSZID_SUPPORT_TYPE_ENUM_UINT8;


enum NAS_NVIM_LC_RAT_COMBINED_ENUM
{
    NAS_NVIM_LC_RAT_COMBINED_GUL,
    NAS_NVIM_LC_RAT_COMBINED_CL,

    NAS_NVIM_LC_RAT_COMBINED_BUTT
};
typedef VOS_UINT8 NAS_NVIM_LC_RAT_COMBINED_ENUM_UINT8;


enum CNAS_NVIM_1X_NEG_PREF_SYS_CMP_TYPE_ENUM
{
    CNAS_NVIM_1X_NEG_PREF_SYS_CMP_BAND_CHAN_AMBIGUOUS_MATCH,                /* Band Channel 模糊匹配 */
    CNAS_NVIM_1X_NEG_PREF_SYS_CMP_BAND_CHAN_ACCURATE_MATCH,                 /* Band Channel 精确匹配*/
    CNAS_NVIM_1X_NEG_PREF_SYS_CMP_BUTT
};
typedef VOS_UINT8 CNAS_NVIM_1X_NEG_PREF_SYS_CMP_TYPE_ENUM_UINT8;


enum NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_ENUM
{
    NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_SWITCH_ON                 = 0,        /* 开机 */

    NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_SWITCH_ON_AND_SYSCFG_SET  = 0x1,      /* 开机和syscfg设置场景 */

    NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_ANY                       = 0xFE,     /* CL系统捕获的任意场景 */

    NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_ENUM_BUTT
};
typedef VOS_UINT8 NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_ENUM_UINT8;



enum NAS_MSCC_NVIM_SYS_ACQ_SCENE_ENUM
{
    NAS_MSCC_NVIM_SYS_ACQ_SCENE_SWITCH_ON                                        = 0,       /* 开机 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_HRPD_LOST,                                                  /* hrpd 丢网 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_HRPD_LOST_NO_RF,                                            /* hrpd no rf丢网 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_SLEEP_TIMER_EXPIRED,                                        /* sleep 定时器超时 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_SYS_CFG_SET,                                                /* system configure设置触发搜网 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_LTE_RF_AVAILABLE,                                           /* LTE RF资源可用 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_HRPD_RF_AVAILABLE,                                          /* HRPD RF资源可用 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_MO_TRIGGER,                                                 /* 主叫触发 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_AVAILABLE_TIMER_EXPIRED_1XSRVEXIST_HISTORY,                 /* 1x有服务时available定时器超时历史搜 */
    NAS_MSCC_NVIM_SYS_ACQ_SCENE_AVAILABLE_TIMER_EXPIRED_1XSRVEXIST_PREFBAND,                /* 1x有服务时available定时器超时pref band搜 */
    NAS_MSCC_NVIM_SYS_ACQ_SCENE_AVAILABLE_TIMER_EXPIRED_1XSRVEXIST_FULLBAND,                /* 1x有服务时available定时器超时full band搜 */

    NAS_MSCC_NVIM_SYS_ACQ_BSR,                                                              /* 背景搜场景 */

    NAS_MSCC_NVIM_SYS_ACQ_SCENE_BUTT
};
typedef VOS_UINT32 NAS_MSCC_NVIM_SYS_ACQ_SCENE_ENUM_UINT32;
/*****************************************************************************
  5 STRUCT
*****************************************************************************/
/*****************************************************************************
*                                                                            *
*                           参数设置消息结构                                 *
*                                                                            *
******************************************************************************/

typedef struct
{
    VOS_UINT16                          usManualSearchHplmnFlg; /*Range:[0,1]*/
}NAS_MMC_NVIM_MANUAL_SEARCH_HPLMN_FLG_STRU;


typedef struct
{
    VOS_UINT16                          usAutoSearchHplmnFlg; /*Range:[0,3]*/
}NAS_MMC_NVIM_AUTO_SEARCH_HPLMN_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucActiveFlag;       /* ucActiveFlag 是否激活，VOS_TRUE:激活；VOS_FALSE:未激活 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
}NAS_NVIM_ADD_EHPLMN_WHEN_SRCH_HPLMN_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usEHPlmnSupportFlg; /*Range:[0,1]*/
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_EHPLMN_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT16                          usStkSteeringOfRoamingSupportFlg; /*Range:[0,1]*/
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_STK_STEERING_OF_ROAMING_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;                        /* en_NV_Item_Scan_Ctrl_Para NV项是否激活，VOS_TRUE:激活；VOS_FALSE:未激活 */
    VOS_UINT8                           ucReserved1;                            /* 保留 */
    VOS_UINT8                           ucReserved2;                            /* 保留 */
    VOS_UINT8                           ucReserved3;                            /* 保留 */
}NVIM_SCAN_CTRL_STRU;


typedef struct
{
    VOS_UINT32                          ulMcc;                                  /* MCC,3 bytes */
    VOS_UINT32                          ulMnc;                                  /* MNC,2 or 3 bytes */
}NAS_NVIM_PLMN_ID_STRU;


typedef struct
{
    NAS_NVIM_PLMN_ID_STRU               stOperatorPlmnId;
    VOS_UINT8                           aucOperatorNameShort[NAS_NVIM_MAX_OPER_SHORT_NAME_LEN];/* 当前驻留网络运营商的短名称 */
    VOS_UINT8                           aucOperatorNameLong[NAS_NVIM_MAX_OPER_LONG_NAME_LEN];  /* 当前驻留网络运营商的长名称 */
}NAS_MMC_NVIM_OPERATOR_NAME_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucNotReadFileEnableFlg;                  /* 软开机读卡优化使能标志，默认开启，表示软开时候，卡状态前后无变化，不需要重新读取卡文件 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_POWER_ON_READ_USIM_OPTIMIZE_INFO_STRU;



typedef struct
{
    VOS_UINT32                          ulBlackPlmnLockNum;                     /* 支持黑名单的个数,个数为0时表示不支持黑名单 */
    NAS_NVIM_PLMN_ID_STRU               astBlackPlmnId[NAS_MML_NVIM_MAX_BLACK_LOCK_PLMN_NUM];
}NAS_MMC_NVIM_OPER_LOCK_BLACKPLMN_STRU;

typedef struct
{
    VOS_UINT8                           ucEnableFlg;                            /* 该特性是否打开 */
    VOS_UINT8                           ucHighPrioRatType;                              /* 漫游支持的接入技术 */
    VOS_UINT8                           aucReserve[2];
    NAS_NVIM_PLMN_ID_STRU               stHighPrioPlmnId;                       /* 漫游支持的高优先级的PLMN ID ,即使驻留在HPLMN上,此PLMNID的优先级也较高*/
    NAS_NVIM_PLMN_ID_STRU               stSimHPlmnId;                           /* SIM卡的HPLMN ID */
}NAS_MMC_NVIM_AIS_ROAMING_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucAutoReselActiveFlg;                   /* 是否允许LTE国际漫游标记:VOS_TRUE 表示允许LTE国际漫游 VOS_FALSE 表示禁止LTE国际漫游 */
    VOS_UINT8                           ucReserve;
}NAS_MMC_NVIM_USER_AUTO_RESEL_CFG_STRU;


typedef struct
{
    VOS_UINT8                            ucStatus;        /* NV有效标志, 1: 有效，0：无效 */
	VOS_UINT8                            ucReserved;      /* 四字节对齐 */
    VOS_UINT16                           usPrioHplmnAct;      /*定制的优先接入技术*/
}NAS_MMC_NVIM_PRIO_HPLMNACT_CFG_STRU;


typedef struct
{
    VOS_UINT32                          ulFirstSearchTimeLen;                   /* available timer定时器第一次的时长 */
    VOS_UINT32                          ulFirstSearchTimeCount;                 /* available timer定时器第一次的次数 */
    VOS_UINT32                          ulDeepSearchTimeLen;                    /* available timer定时器深睡的时长 */
    VOS_UINT32                          ulDeepSearchTimeCount;
}NAS_MMC_NVIM_AVAIL_TIMER_CFG_STRU;



typedef struct
{
    VOS_UINT32                          ulNvActiveFlg;                          /* 控制当前NV是否使能 */
    VOS_UINT32                          ulT3212StartSceneCtrlBitMask;           /* BIT0~BIT31,用于控制链路失败后启动T3212时，是否使用当前NV配置的时长 */
    VOS_UINT32                          ulT3212Phase1TimeLen;                   /* t3212定时器第1阶段的时长,单位为s */
    VOS_UINT32                          ulT3212Phase1Count;                     /* t3212定时器第1阶段的次数 */
    VOS_UINT32                          ulT3212Phase2TimeLen;                   /* t3212定时器第2阶段的时长,单位为s  */
    VOS_UINT32                          ulT3212Phase2Count;                     /* t3212定时器第2阶段的次数 */
}NAS_MMC_NVIM_T3212_TIMER_CFG_STRU;


typedef struct
{
    VOS_UINT32                          ulWhitePlmnLockNum;                     /* 支持白名单的个数,个数为0时表示不支持白名单 */
    NAS_NVIM_PLMN_ID_STRU               astWhitePlmnId[NAS_MML_NVIM_MAX_WHITE_LOCK_PLMN_NUM];
}NAS_MMC_NVIM_OPER_LOCK_WHITEPLMN_STRU;


typedef struct
{
    VOS_UINT16                          usSupportFlg; /*Range:[0,1]*/
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_CPHS_SUPPORT_FLG_STRU;

/* Added by l60609 for B060 Project, 2012-2-20, Begin   */

typedef struct
{
    VOS_UINT8                           ucStatus;                               /*是否激活，0不激活，1激活 */
    VOS_UINT8                           ucActFlg;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_PREVENT_TEST_IMSI_REG_STRU;
/* Added by l60609 for B060 Project, 2012-2-20, End   */
/*****************************************************************************
*                                                                            *
*                           参数设置消息结构                                 *
*                                                                            *
******************************************************************************/


typedef struct
{
    VOS_UINT8   ucHplmnSearchPowerOn;
    VOS_UINT8   ucReserve1;
    VOS_UINT8   ucReserve2;
    VOS_UINT8   ucReserve3;
}NAS_MMC_NVIM_HPLMN_SEARCH_POWERON_STRU;



typedef struct
{
    VOS_UINT8                           ucTinType;                              /* TIN类型 */
    VOS_UINT8                           aucReserve[2];
    VOS_UINT8                           aucImsi[NAS_NVIM_ITEM_MAX_IMSI_LEN];        /* 上次保存的IMSI的内容 */
}NAS_NVIM_TIN_INFO_STRU;


typedef struct
{
    VOS_UINT32 ulAutoStart;
}NAS_MMA_NVIM_AUTO_START_STRU;



typedef struct
{
    VOS_UINT8                           ucSingleDomainFailPlmnSrchFlag;         /* DT定制需求，单域注册被拒后，需要出发搜网 */
    VOS_UINT8                           ucReserved;                             /* 保留*/
}NAS_MMC_NVIM_SINGLE_DOMAIN_FAIL_CNT_STRU;


typedef struct
{
    VOS_UINT8                           aucPsLocInfo[NAS_NVIM_PS_LOCI_SIM_FILE_LEN];
}NAS_NVIM_PS_LOCI_SIM_FILES_STRU;


typedef struct
{
    VOS_UINT8                           aucTmsi[NAS_MAX_TMSI_LEN];
    VOS_UINT8                           aucLastLai[NAS_SIM_MAX_LAI_LEN];
    VOS_UINT8                           ucLauStaus;
    VOS_UINT8                           ucReserved;
}NAS_NVIM_CS_LOCI_SIM_FILES_STRU;


typedef struct
{
    VOS_UINT8                          ucStatus;                                /* NV是否激活标志, 0: 不激活，1: 激活 */
    VOS_UINT8                          ucGeaSupportCtrl;                        /* 终端配置:GPRS GEA算法支持控制 */
}NAS_MMC_NVIM_GPRS_GEA_ALG_CTRL_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;    /* en_NV_Item_Lte_Cs_Service_Config NV项是否激活，VOS_TRUE:激活；VOS_FALSE:未激活 */
    VOS_UINT8                           ucLteCsServiceCfg;  /* LTE支持的 cs域业务能力*/
}NAS_NVIM_LTE_CS_SERVICE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;
    VOS_UINT8                           ucWaitSysinfoTimeLen;
}NAS_MMC_NVIM_HO_WAIT_SYSINFO_TIMER_CFG_STRU;




typedef struct
{
    VOS_UINT8                           ucLteRoamAllowedFlg;
    VOS_UINT8                           aucReserve[1];
    VOS_UINT8                           aucRoamEnabledMccList[20];/* 允许漫游的国家码列表 */
}NAS_MMC_NVIM_LTE_INTERNATIONAL_ROAM_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucRoamRplmnflg;
    VOS_UINT8                           aucReserve[3];
    VOS_UINT32                         aucRoamEnabledMccList[5];/* 允许漫游的国家码列表 */
}NAS_MMC_NVIM_ROAM_SEARCH_RPLMN_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucActFlg;
    VOS_UINT8                           aucReserved[3];
}NAS_NVIM_CLOSE_SMS_CAPABILITY_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucWcdmaPriorityGsmFlg;                  /* H3G定制需求，W网络优先于G*/

    VOS_UINT8                           ucSortAvailalePlmnListRatPrioFlg;    /* 是否按syscfg设置接入技术优先级排序高低质量搜网列表标识，1:是; 0:不是高质量网络按随机排序低质量网络不排序*/
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_WCDMA_PRIORITY_GSM_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucPsOnlyCsServiceSupportFlg;            /* 服务域设置为PS ONLY时，是否支持CS域短信和呼叫业务(紧急呼叫除外)*/
    VOS_UINT8                           ucReserved1;                            /* 保留*/
    VOS_UINT8                           ucReserved2;                            /* 保留*/
    VOS_UINT8                           ucReserved3;                            /* 保留*/
}NAS_NVIM_PS_ONLY_CS_SERVICE_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucCcbsSupportFlg;                       /* CCBS(遇忙呼叫完成)业务*/
    VOS_UINT8                           ucReserved1;                            /* 保留*/
    VOS_UINT8                           ucReserved2;                            /* 保留*/
    VOS_UINT8                           ucReserved3;                            /* 保留*/
}NAS_NVIM_CCBS_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;
    VOS_UINT8                           ucCustomMccNum;
    VOS_UINT8                           aucReserve[2];
    VOS_UINT32                          aulCustommMccList[10];                   /* 允许漫游的国家码列表 */
}NAS_MMC_NVIM_HPLMN_SEARCH_REGARDLESS_MCC_SUPPORT_STRU;



typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NVIM_ACTING_HPLMN_SUPPORT_FLAG_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlg;
    VOS_UINT8                           ucReserved1;                            /* 保留*/
    VOS_UINT8                           ucReserved2;                            /* 保留*/
    VOS_UINT8                           ucReserved3;                            /* 保留*/
}NAS_MMC_NVIM_REG_FAIL_NETWORK_FAILURE_CUSTOM_FLG_STRU;




typedef struct
{
    NAS_MMC_NVIM_REG_FAIL_CAUSE_ENUM_UINT16                 enRegCause;
    NAS_MMC_NVIM_REG_DOMAIN_ENUM_UINT8                      enDomain;
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_ENUM_UINT8   enAction;
}NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_STRU;



typedef struct
{
    VOS_UINT8                                               ucActiveFlag;
    VOS_UINT8                                               ucCount;
    VOS_UINT8                                               auReserv[2];
    NAS_MMC_NVIM_SINGLE_DOMAIN_REG_FAIL_ACTION_STRU         astSingleDomainFailActionList[NAS_MML_SINGLE_DOMAIN_FAIL_ACTION_MAX_LIST];
}NAS_MMC_NVIM_SINGLE_DOMAIN_FAIL_ACTION_LIST_STRU;


typedef struct {
    VOS_UINT8                           aucSimPlmn[NAS_SIM_FORMAT_PLMN_LEN];
    VOS_UINT8                           aucReserve[1];
}NAS_SIM_FORMAT_PLMN_ID;


typedef struct
{
    VOS_UINT8                           ucUtranMode;                            /* 当前支持的UTRAN模式 */
    VOS_UINT8                           ucReserve;
}NAS_UTRANCTRL_NVIM_UTRAN_MODE_STRU;


typedef struct
{
    VOS_UINT8                           ucUtranSwitchMode;                      /* 0：UTRAN模式为FDD 1：UTRAN模式为TDD 2:AUTO SWITCH */
    VOS_UINT8                           ucTdMccListNum;                         /* 支持TD网络的国家号个数 */
    VOS_UINT8                           ucImsiPlmnListNum;                      /* 根据IMS PLMN决定UTRAN模式是否AUTO SWITCH时，当前IMSI的PLMN在此列表中的都支持自动切换，否则固定为W模 */
    VOS_UINT8                           aucReserve[1];

    VOS_UINT32                          aulTdMccList[NAS_UTRANCTRL_MAX_NVIM_CFG_TD_MCC_LIST_NUM];
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_UTRANCTRL_MAX_NVIM_CFG_IMSI_PLMN_LIST_NUM];
}NAS_UTRANCTRL_NVIM_UTRAN_MODE_AUTO_SWITCH_STRU;





typedef struct
{
    VOS_UINT8                           ucIsUtranTddCsSmcNeeded;                /* 控制在3G TDD模式下是否需要开启CS SMC验证标记:0-不需要；1-需要 */
    VOS_UINT8                           ucIsUtranTddPsSmcNeeded;                /* 控制在3G TDD模式下是否需要开启PS SMC验证标记:0-不需要；1-需要 */
    VOS_UINT8                           aucReserved[2];                         /* 保留 */
}NAS_UTRANCTRL_NVIM_SMC_CTRL_FLAG_STRU;



typedef struct
{
    VOS_UINT8   ucAccBarPlmnSearchFlg;
    VOS_UINT8   aucReserve[3];
}NAS_MMC_NVIM_ACC_BAR_PLMN_SEARCH_FLG_STRU;



typedef struct
{
    VOS_UINT8                           ucActiveFlg;                                          /* 定制项使能标志 */
    VOS_UINT8                           ucImsiCheckFlg;                                       /* 是否有IMSI列表的白名单，0:不需要 1:需要 */
    VOS_UINT8                           aucVersion[NAS_MMC_NVIM_MAX_USER_OPLMN_VERSION_LEN];  /* 本地配置的版本号 */
    VOS_UINT8                           ucImsiPlmnListNum;                                    /* 定制的IMSI列表个数 */
    VOS_UINT8                           ucOplmnListNum;                                       /* 本地配置的Oplmn的个数 */
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_MMC_NVIM_MAX_USER_OPLMN_IMSI_NUM];
    VOS_UINT8                           aucOPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_OPLMN_DATA_LEN];/* OPLMN的PDU数据，和EFOplmn文件一致 */
}NAS_MMC_NVIM_USER_CFG_OPLMN_INFO_STRU;

/*****************************************************************************
 结构名    : NAS_MMC_NVIM_USER_CFG_OPLMN_EXTEND_STRU
 结构说明  : NAS_MMC_NVIM_USER_CFG_OPLMN_EXTEND_STRU NVIM项中的用户设置的OPLMN
 1.日    期   : 2013年11月26日
   修改内容   : 将NV支持的最大OPLMN个数扩展到256个
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucActiveFlg;                                          /* 定制项使能标志 */
    VOS_UINT8                           ucImsiCheckFlg;                                       /* 是否有IMSI列表的白名单，0:不需要 1:需要 */
    VOS_UINT8                           aucVersion[NAS_MMC_NVIM_MAX_USER_OPLMN_VERSION_LEN];  /* 本地配置的版本号 */
    VOS_UINT16                          usOplmnListNum;                                       /* 本地配置的Oplmn的个数 */
    VOS_UINT8                           ucImsiPlmnListNum;                                    /* 定制的IMSI列表个数 */
    VOS_UINT8                           aucReserve[3];
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_MMC_NVIM_MAX_USER_OPLMN_IMSI_NUM];
    VOS_UINT8                           aucOPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_OPLMN_DATA_EXTEND_LEN];/* OPLMN的PDU数据，和EFOplmn文件一致 */
}NAS_MMC_NVIM_USER_CFG_OPLMN_EXTEND_STRU;



typedef struct
{
    VOS_UINT8                           ucEnableFlg;                                                    /* NV使能标志 */
    VOS_UINT8                           ucExtendedForbPlmnNum;                                          /* 扩展的Forb Plmn个数 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    NAS_NVIM_PLMN_ID_STRU               astForbPlmnIdList[NAS_MMC_NVIM_MAX_EXTENDED_FORB_PLMN_NUM];     /* 扩展的Forb Plmn列表 */
}NAS_MMC_NVIM_EXTENDED_FORBIDDEN_PLMN_LIST_CFG_STRU;


typedef struct
{
    VOS_UINT16                         usDplmnListNum;                                       /* 本地配置的Dplmn的个数 */
    VOS_UINT16                         usNplmnListNum;                                       /* 本地配置的Nplmn的个数 */

    /* DPLMN数据,每7个字节代表一个dplmn信息，第1-3个字节为sim卡格式plmn id，
       第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，
       第6字节为域信息:1(cs域注册成功)；2(ps域注册成功)；3(cs ps均注册成功)
       第7直接为预置标示信息: 1(预置Dplmn), 0(自学习到的DPLMN) */
    VOS_UINT8                          aucDPlmnList[NAS_MMC_NVIM_MAX_CFG_DPLMN_DATA_LEN];

    /* NPLMN数据,每7个字节代表一个nplmn信息，第1-3个字节为sim卡格式plmn id，
       第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，
       第6字节为域信息:1(cs域注册成功)；2(ps域注册成功)；3(cs ps均注册成功)
       第7直接为预置标示信息: 1(预置nplmn), 0(自学习到的nplmn) */
    VOS_UINT8                          aucNPlmnList[NAS_MMC_NVIM_MAX_CFG_NPLMN_DATA_LEN];/* NPLMN数据*/
}NAS_MMC_NVIM_CFG_DPLMN_NPLMN_INFO_STRU;


typedef struct
{
    VOS_UINT16                         usDplmnListNum;                                       /* 本地配置的Dplmn的个数 */
    VOS_UINT16                         usNplmnListNum;                                       /* 本地配置的Nplmn的个数 */

    /* DPLMN数据,每6个字节代表一个dplmn信息，第1-3个字节为sim卡格式plmn id，
       第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，第6字节为域信息:1(cs域注册成功)；2(ps域注册成功)；3(cs ps均注册成功)*/
    VOS_UINT8                          aucDPlmnList[NAS_MMC_NVIM_MAX_CFG_DPLMN_DATA_EXTEND_LEN];

    /* NPLMN数据,每6个字节代表一个nplmn信息，第1-3个字节为sim卡格式plmn id，
       第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，第6字节为域信息:1(cs域注册成功)；2(ps域注册成功)；3(cs ps均注册成功)*/
    VOS_UINT8                          aucNPlmnList[NAS_MMC_NVIM_MAX_CFG_NPLMN_DATA_EXTEND_LEN];/* NPLMN数据*/
}NAS_MMC_NVIM_CFG_DPLMN_NPLMN_INFO_OLD_STRU;


typedef struct
{
    VOS_UINT16                         usCfgDplmnNplmnFlag;
    VOS_UINT8                          ucCMCCHplmnNum;
    VOS_UINT8                          aucCMCCHplmnList[NAS_MMC_NVIM_MAX_CFG_HPLMN_NUM];
    VOS_UINT8                          ucUNICOMHplmnNum;
    VOS_UINT8                          aucUNICOMHplmnList[NAS_MMC_NVIM_MAX_CFG_HPLMN_NUM];
    VOS_UINT8                          ucCTHplmnNum;
    VOS_UINT8                          aucCTHplmnList[NAS_MMC_NVIM_MAX_CFG_HPLMN_NUM];
    VOS_UINT8                          aucReserve[3];
}NAS_MMC_NVIM_CFG_DPLMN_NPLMN_FLAG_STRU;


typedef struct
{
    VOS_UINT8                   ucRestrainHighPrioRatHPlmnSrch;         /* Home网络高优先接入技术背景搜是否受抑制，1:抑制；0:不抑制 Dallas保留 */
    VOS_UINT8                   ucRestrainAnyCellSrch;                  /* AnyCell搜网是否受抑制，1:抑制；0:不抑制 */
    VOS_UINT16                  usHighPrioRatHPlmnSrchTimerRetryLen;    /* HighPrioRatHPlmnSrch被抑制后，重试定时器时常，单位:秒  Dallas保留*/

    VOS_UINT8                   ucRsv1;                                /* 保留位1 */
    VOS_UINT8                   ucRsv2;                                /* 保留位2 */
    VOS_UINT8                   ucRsv3;                                /* 保留位3 */
    VOS_UINT8                   ucRsv4;                                /* 保留位4 */
}NV_NAS_HIGH_PRIO_PS_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucEHplmnNum;
    VOS_UINT8                           aucEHplmnList[NAS_MMC_NVIM_MAX_CFG_HPLMN_NUM];
    VOS_UINT8                           aucVersionId[NAS_MCC_NVIM_VERSION_LEN];
    VOS_UINT8                           aucReserved[2];
}NAS_MMC_NVIM_DPLMN_NPLMN_CFG_STRU;


typedef struct
{
    VOS_UINT8                           aucVersionId[NAS_MCC_NVIM_VERSION_LEN];
    VOS_UINT8                           ucReserved;                                                 /* 保留位 */
    VOS_UINT16                          usBorderNum;                                                /* 边境信息的个数 */
    /* 边境信息数据,每7个字节代表一条边境信息，第1-3个字节为sim卡格式plmn id，
       第4-5字节为支持的接入技术(0x8000为支持w，0x4000为支持lte，0x0080为支持gsm)，第6-7字节为LAC或TAC信息
       接入技术为WG时，6-7字节表示LAC;接入技术为LTE时，6-7字节表示TAC */
    VOS_UINT8                           aucBorderList[NAS_MMC_NVIM_MAX_CFG_BORDER_DATA_LEN];        /* 边境信息列表 */
}NAS_NVIM_BORDER_INFO_STRU;



typedef struct
{
    VOS_UINT8                           ucImsiPlmnListNum;                      /* 定制的IMSI列表个数 */
    VOS_UINT8                           ucEhplmnListNum;                        /* 用户配置的EHplmn的个数 */
    VOS_UINT8                           aucReserve[2];
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_IMSI_PLMN_NUM];
    NAS_SIM_FORMAT_PLMN_ID              astEhPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_EHPLMN_NUM];
}NAS_MMC_NVIM_USER_CFG_EHPLMN_INFO_STRU;


typedef struct
{
    VOS_UINT32                                ulNvimEhplmnNum;
    NAS_MMC_NVIM_USER_CFG_EHPLMN_INFO_STRU    astNvimEhplmnInfo[NAS_MMC_NVIM_MAX_USER_CFG_EXT_EHPLMN_NUM];
}NAS_MMC_NVIM_USER_CFG_EXT_EHPLMN_INFO_STRU;



typedef struct
{
    VOS_UINT8                           ucImsiPlmnListNum;                      /* 每一组里定制的IMSI列表个数 */
    VOS_UINT8                           ucForbPlmnListNum;                      /* 每一组里用户配置的Forb Plmn的个数 */
    VOS_UINT8                           aucReserve[2];
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_IMSI_PLMN_NUM];
    NAS_SIM_FORMAT_PLMN_ID              astForbPlmnList[NAS_MMC_MAX_BLACK_LOCK_PLMN_WITH_RAT_NUM];
}NAS_MMC_NVIM_FORB_PLMN_INFO_STRU;



typedef struct
{
    VOS_UINT32                          ulGroupNum;
    NAS_MMC_NVIM_FORB_PLMN_INFO_STRU    astForbPlmnInfo[NAS_MMC_NVIM_MAX_USER_CFG_FORB_PLMN_GROUP_NUM];
}NAS_MMC_NVIM_USER_CFG_FORB_PLMN_INFO_STRU;


typedef struct
{
    VOS_UINT8                          ucCause18EnableLteSupportFlg; /*是否支持Cause18 Enable Lte*/
    VOS_UINT8                          aucReserved0;
    VOS_UINT8                          aucReserved1;
    VOS_UINT8                          aucReserved2;
}NAS_MMC_NVIM_CAUSE18_ENABLE_LTE_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT32                          ulMcc;                                  /* MCC,3 bytes */
    VOS_UINT32                          ulMnc;                                  /* MNC,2 or 3 bytes */
    VOS_UINT8                           enRat;
    VOS_UINT8                           aucReserve[3];
}NAS_MMC_NVIM_PLMN_WITH_RAT_STRU;



typedef struct
{
    VOS_UINT32                          ulDisabledRatPlmnNum;                   /* 支持禁止接入技术的PLMN个数,个数为0表示不支持该特性 */

    NAS_MMC_NVIM_PLMN_WITH_RAT_STRU     astDisabledRatPlmnId[NAS_MML_NVIM_MAX_DISABLED_RAT_PLMN_NUM];/* 禁止接入技术的PLMN和RAT信息 */

}NAS_MMC_NVIM_DISABLED_RAT_PLMN_INFO_STRU;


enum NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_FLAG_ENUM
{
    NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_INACTIVE                   = 0,           /* 功能未激活 */
    NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_BLACK                      = 1,           /* 开启黑名单功能 */
    NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_WHITE                      = 2,           /* 开启白名单功能 */
    NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_BUTT
};
typedef VOS_UINT8 NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_FLAG_ENUM_UINT8;


enum NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_ENUM
{
    NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_GERAN                   = 0,           /* GERAN */
    NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_UTRAN                   = 1,           /* UTRAN包括WCDMA/TDS-CDMA */
    NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_EUTRAN                  = 2,           /* E-UTRAN */
    NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_BUTT
};
typedef VOS_UINT8 NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_ENUM_UINT8;


enum NAS_MSCC_NVIM_SYS_PRI_CLASS_ENUM
{
    NAS_MSCC_NVIM_SYS_PRI_CLASS_HOME             = 0,    /* home or ehome plmn */
    NAS_MSCC_NVIM_SYS_PRI_CLASS_PREF             = 1,    /* UPLMN or OPLMN */
    NAS_MSCC_NVIM_SYS_PRI_CLASS_ANY              = 2,    /* Acceptable PLMN */
    NAS_MSCC_NVIM_SYS_PRI_CLASS_BUTT
};
typedef VOS_UINT8 NAS_MSCC_NVIM_SYS_PRI_CLASS_ENUM_UINT8;


typedef struct
{
    NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_SWITCH_FLAG_ENUM_UINT8  enSwitchFlag;                                         /*功能是否有效及功能的类型  */
    VOS_UINT8                                               ucImsiListNum;                                        /*功能起效的SIM卡数目(黑名单/白名单)  */
    VOS_UINT8                                               ucForbidRatNum;                                       /*禁止RAT的数目  */
    VOS_UINT8                                               aucReserve[1];
    NAS_SIM_FORMAT_PLMN_ID                                  astImsiList[NAS_NVIM_MAX_IMSI_FORBIDDEN_LIST_NUM];        /* SIM卡列表 (黑名单/白名单) */
    NAS_MMC_NVIM_PLATFORM_SUPPORT_RAT_ENUM_UINT8            aenForbidRatList[NAS_NVIM_MAX_RAT_FORBIDDEN_LIST_NUM];    /*禁止的接入技术  */
}NAS_MMC_NVIM_RAT_FORBIDDEN_LIST_STRU;


typedef struct
{
    VOS_UINT8                           ucCsfbEmgCallLaiChgLauFirstFlg;
    VOS_UINT8                           aucRserved[1];
}NAS_MMC_NVIM_CSFB_EMG_CALL_LAI_CHG_LAU_FIRST_CFG_STRU;

typedef struct
{
    VOS_UINT8                           ucPlmnExactlyCompareFlag;
    VOS_UINT8                           aucRsv[3];                         /* 保留*/
}NAS_MMC_NVIM_PLMN_EXACTLY_COMPARE_FLAG_STRU;


typedef struct
{
    VOS_UINT8                           ucHplmnRegisterCtrlFlg;                 /* HPLMN注册控制标记 */
    VOS_UINT8                           aucRsv[3];                              /* 保留 */
}NAS_MMC_NVIM_HPLMN_REGISTER_CTRL_FLAG_STRU;

typedef struct
{
    VOS_UINT8                                               ucSignThreshold;    /* 信号变化门限,当RSSI变化超过该值，
                                                                                  接入层需要主动上报信号质量，取值0表示接入层按默认值处理 */
    VOS_UINT8                                               ucMinRptTimerInterval;     /* 间隔上报的时间   */
    VOS_UINT8                                               ucRserved1;
    VOS_UINT8                                               ucRserved2;
} NAS_NVIM_CELL_SIGN_REPORT_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucH3gCtrlFlg;                           /* H3G定制标记 */
    VOS_UINT8                           aucRsv[3];                              /* 保留 */
}NAS_MMC_NVIM_H3G_CTRL_FLAG_STRU;


typedef struct
{
    VOS_UINT16                          usUcs2Customization;
    VOS_UINT8                           ucRserved1;
    VOS_UINT8                           ucRserved2;
}NAS_MMC_NVIM_UCS2_CUSTOMIZATION_STRU;


typedef struct
{
    VOS_UINT16                          usTc1mLength;
    VOS_UINT16                          usTr1mLength;
    VOS_UINT16                          usTr2mLength;
    VOS_UINT16                          usTramLength;
}SMS_NVIM_TIMER_LENGTH_STRU;



typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV是否激活标志, 0: 不激活，1: 激活 */
    VOS_UINT8                           ucStatusRptGeneralControl;        /* 私有命令是否允许状态上报 0:不上报，1:上报 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NVIM_PRIVATE_CMD_STATUS_RPT_STRU;


typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV是否激活标志, 0: 不激活，1: 激活  */
    VOS_UINT8                           ucSpecialRoamFlg;                         /* Vplmn与Hplmn不同国家码时,是否允许回到Hplmn,1:允许，0:不允许 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_SPECIAL_ROAM_STRU;


typedef struct
{
    VOS_UINT16                          usEnhancedHplmnSrchFlg;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_ENHANCED_HPLMN_SRCH_FLG_STRU;


typedef struct
{
    VOS_UINT8   ucT305Len;
    VOS_UINT8   ucT308Len;
    VOS_UINT8   aucReserve[2];
}NAS_CC_NVIM_TIMER_LEN_STRU;


typedef struct
{
    VOS_UINT8                           ucT303ActiveFlag;                       /* 是否开启T303定时器。0:关闭，1:开启。*/
    VOS_UINT8                           ucT303Len;                              /* T303定时器时长 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_CC_NVIM_T303_LEN_CFG_STRU;


typedef struct
{
    VOS_UINT8   ucCmSrvExistTrigPlmnSearch;                                                     /* 业务存在时是否触发搜网 */
    VOS_UINT8   ucCmSrvTrigPlmnSearchCauseNum;                                                  /* 配置业务存在时触发搜网的被拒原因值个数 */
    VOS_UINT8   ucReserve1;
    VOS_UINT8   ucReserve2;
    VOS_UINT8   aucCmSrvTrigPlmnSearchCause[NAS_NVIM_MAX_LAU_REJ_TRIG_PLMN_SEARCH_CAUSE_NUM];   /* 配置业务存在时触发搜网的被拒原因值 */
    VOS_UINT8   aucReserve[NAS_NVIM_MAX_LAU_REJ_TRIG_PLMN_SEARCH_CAUSE_NUM];                    /* 预留给注册被拒触发搜网使用 */
}NAS_NVIM_LAU_REJ_TRIG_PLMN_SEARCH_CFG_STRU;


typedef struct
{
    VOS_UINT8   ucLauRejCauseNum;
    VOS_UINT8   ucLauRejTimes;
    VOS_UINT8   ucReserve1;
    VOS_UINT8   ucReserve2;
    VOS_UINT8   aucLauRejCause[NAS_NVIM_MAX_REJECT_NO_RETRY_CAUSE_NUM];
}NAS_NVIM_LAU_REJ_NORETRY_WHEN_CM_SRV_EXIST_CFG_STRU;


typedef struct
{
    NAS_NVIM_CHANGE_REG_REJ_CAUSE_TYPE_ENUM_UINT8           enChangeRegRejCauCfg;
    VOS_UINT8   ucPreferredRegRejCau_HPLMN_EHPLMN;             /* HPLMN/EHPLMN时使用的拒绝原因值 */
    VOS_UINT8   ucPreferredRegRejCau_NOT_HPLMN_EHPLMN;         /* 非HPLMN/EHPLMN时使用的拒绝原因值 */
    VOS_UINT8   aucReserve[1];
}NAS_NVIM_CHANGE_REG_REJECT_CAUSE_FLG_STRU;


typedef struct
{
    VOS_UINT8   ucNoRetryRejectCauseNum;
    VOS_UINT8   aucNoRetryRejectCause[NAS_NVIM_MAX_REJECT_NO_RETRY_CAUSE_NUM];
    VOS_UINT8   aucReserve[3];
}NAS_NVIM_ROAMINGREJECT_NORETYR_CFG_STRU;


typedef struct
{
   VOS_UINT8                           ucIgnoreAuthRejFlg;
   VOS_UINT8                           ucMaxAuthRejNo;
   VOS_UINT8                           aucReserved[2];
}NAS_MMC_NVIM_IGNORE_AUTH_REJ_CFG_STRU;


typedef struct
{
    VOS_UINT8 ucStatus;
    VOS_UINT8 ucReserved1;
    VOS_UINT8 aucE5GwMacAddr[18];
}NAS_NV_GWMAC_ADDR_STRU;


typedef struct
{
    VOS_UINT8 aucE5_RoamingWhiteList_Support_Flg[2];
}NAS_NVIM_E5_ROAMING_WHITE_LIST_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT32 ulNDIS_DIALUP_ADDRESS;
}NAS_NVIM_NDIS_DIALUP_ADDRESS_STRU;


typedef struct
{
    VOS_UINT8 ucBreOnTime; /*Range:[0,7]*/
    VOS_UINT8 ucBreOffTime; /*Range:[0,7]*/
    VOS_UINT8 ucBreRiseTime; /*Range:[0,5]*/
    VOS_UINT8 ucBreFallTime; /*Range:[0,5]*/
}NAS_NVIM_NV_BREATH_LED_STR_STRU;


typedef struct
{
    VOS_UINT8                           ucIsManualModeRegHplmnFlg;              /* 手动搜网模式下用户指定网络非HPLMN，搜到HPLMN时是否允许注册的开关 */
    VOS_UINT8                           ucReserved1;                            /* 保留位 */
    VOS_UINT8                           ucReserved2;                            /* 保留位 */
    VOS_UINT8                           ucReserved3;                            /* 保留位 */
}NAS_NVIM_MANUAL_MODE_REG_HPLMN_CFG_STRU;


typedef struct
{
VOS_UINT8  aucwlAuthMode[16];
VOS_UINT8  aucBasicEncryptionModes[5];
VOS_UINT8  aucWPAEncryptionModes[5];
VOS_UINT8  aucwlKeys1[27];
VOS_UINT8  aucwlKeys2[27];
VOS_UINT8  aucwlKeys3[27];
VOS_UINT8  aucwlKeys4[27];
VOS_UINT32 ulwlKeyIndex;
VOS_UINT8  aucwlWpaPsk[65];
VOS_UINT8  ucwlWpsEnbl;
VOS_UINT8  ucwlWpsCfg;
VOS_UINT8  ucReserved;
}NAS_NVIM_NV_WIFI_KEY_STRU;


typedef struct
{
VOS_UINT8 aucPRIVersion[32];
VOS_UINT8 aucReserve[32];
}NAS_NVIM_NV_PRI_VERSION_STRU;


typedef struct
{
    VOS_UINT16                          usSysAppConfigType;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_NVIM_SYSTEM_APP_CONFIG_STRU;


typedef struct
{
    VOS_UINT8                           aucNetworkCapability[NV_ITEM_NET_CAPABILITY_MAX_SIZE];
}NAS_MMC_NVIM_NETWORK_CAPABILITY_STRU;


typedef struct
{
    VOS_UINT16                          usAutoattachFlag;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_NVIM_AUTOATTACH_STRU;


typedef struct
{
    VOS_UINT16                          usSelPlmnMode;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_NVIM_SELPLMN_MODE_STRU;


typedef struct
{
    VOS_UINT8                           aucAccessMode[2];
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMA_NVIM_ACCESS_MODE_STRU;


typedef struct
{
    VOS_UINT8                           ucMsClass;
    VOS_UINT8                           ucReserved;
}NAS_NVIM_MS_CLASS_STRU;


typedef struct
{
    VOS_UINT16                          usRfAutoTestFlg;
}NAS_MMA_NVIM_RF_AUTO_TEST_FLAG_STRU;


typedef struct
{
    VOS_UINT8   ucHplmnTimerLen;
    VOS_UINT8   ucReserved1;
    VOS_UINT8   ucReserved2;
    VOS_UINT8   ucReserved3;
}NAS_NVIM_HPLMN_FIRST_TIMER_STRU;


typedef struct
{
    VOS_UINT8   aucSupported3GppRelease[NAS_MMC_NVIM_SUPPORTED_3GPP_RELEASE_SIZE];
}NAS_MMC_NVIM_SUPPORT_3GPP_RELEASE_STRU;


typedef struct
{
    VOS_UINT8   ucValid;
    VOS_UINT8   aucImsi[NAS_MMC_NVIM_MAX_IMSI_LEN];
    VOS_UINT8   ucReserved1;
    VOS_UINT8   ucReserved2;
}NAS_MMC_NVIM_LAST_IMSI_STRU;


typedef struct
{
    VOS_UINT8   aucRoamingBroker[2];
}NAS_MMA_NVIM_ROAMING_BROKER_STRU;


typedef struct
{
    VOS_UINT16  usUseSingleRplmnFlag;
    VOS_UINT8   ucReserved1;
    VOS_UINT8   ucReserved2;
}NAS_MMC_NVIM_USE_SINGLE_RPLMN_STRU;

/* en_NV_Item_EquivalentPlmn 8215 */

typedef struct
{
    VOS_UINT8    ucMcc[NVIM_MAX_MCC_SIZE];
    VOS_UINT8    ucMnc[NVIM_MAX_MNC_SIZE];
}NVIM_PLMN_VALUE_STRU;


typedef struct
{
    VOS_UINT8             ucCount;
    NVIM_PLMN_VALUE_STRU  struPlmnList[NVIM_MAX_EPLMN_NUM];
    VOS_UINT8             aucReserve[3];   /*NV项相关的结构体，在4字节方式下，需手动补齐空洞*/
}NVIM_EQUIVALENT_PLMN_LIST_STRU;

/*en_NV_Item_Support_Freqbands 8229*/
/*
NVIM_UE_SUPPORT_FREQ_BAND_STRU结构说明:
usWcdmaBand和usGsmBand用Bit位表示用户设置的频段，bit1代表频段I,bit2代表频段II,
依次类推,比特位为1,表示支持该频段.下表是比特位和频段对应关系:
-------------------------------------------------------------------------------
        bit8       bit7      bit6     bit5    bit4     bit3      bit2     bit1
-------------------------------------------------------------------------------
WCDMA   900(VIII)  2600(VII) 800(VI)  850(V)  1700(IV) 1800(III) 1900(II) 2100(I) oct1
        spare      spare     spare    spare   spare    spare     spare   J1700(IX)oct2
-------------------------------------------------------------------------------
GSM频段 1900(VIII) 1800(VII) E900(VI) R900(V) P900(IV) 850(III)  480(II)  450(I)  oct3
        spare      spare     spare    spare   spare    spare     spare    700(IX) oct4
-------------------------------------------------------------------------------
aucUeSupportWcdmaBand和aucUeSupportGsmBand用数组表示UE支持的频段,并以存储顺序的
先后表示频段优先顺序,用0xff表示无效.

例如:
oct1-oct4分别是：0x03,0x00,0x7B,0x00
   则代表用户设置频段为：W：WCDMA-I-2100, WCDMA-II-1900
                         G：850(III),P900(IV),R900(V),E900(VI),1800(VII)
oct5-oct16分别是:2,5,1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
   则代表UE支持W频段I,II,V,优先顺序是:II,V,I.
oct17-oct28分别是:4,5,8,7,6,3,0xff,0xff,0xff,0xff,0xff,0xff
   则代表UE支持G频段III,IV,V,VI,VII,VIII,优先顺序是:IV,V,VIII,VII,VI,III.
*/

typedef struct
{
    VOS_UINT32                  ulWcdmaBand;
    VOS_UINT32                  ulGsmBand;
    VOS_UINT8                   aucReserved1[12];
    VOS_UINT8                   aucReserved2[12];
    VOS_UINT8                   aucReserved[24];        /* 为保证nv长度一致保留 */
}NVIM_UE_SUPPORT_FREQ_BAND_STRU;

/*en_NV_Item_Roam_Capa 8266*/

typedef struct
{
    VOS_UINT8                               ucRoamFeatureFlg;                   /*记录漫游特性是否激活,VOS_FALSE:不激活,VOS_TRUE:激活*/
    VOS_UINT8                               ucRoamCapability;                   /*记录用户设置的漫游属性*/
    VOS_UINT8                               ucReserve1;
    VOS_UINT8                               ucReserve2;
}NAS_NVIM_ROAM_CFG_INFO_STRU;

/*en_NV_Item_CustomizeService 8271*/

typedef struct
{
    VOS_UINT32                          ulStatus;           /*是否激活，0不激活，1激活 */
    VOS_UINT32                          ulCustomizeService; /*终端说明书是1个byte，为了没有空洞，扩充成4byte，高3byte保留*/
}NAS_NVIM_CUSTOMIZE_SERVICE_STRU;

/*en_NV_Item_RPlmnWithRat 8275*/

typedef struct
{
    NAS_NVIM_PLMN_ID_STRU               stGRplmn;                               /* G RPLMN信息 */
    NAS_NVIM_PLMN_ID_STRU               stWRplmn;                               /* W RPLMN信息*/
    VOS_UINT8                           ucLastRplmnRat;                         /* 上次关机时驻留网络的接入技术0:GSM;1:WCDMA;0xFF:无效值 */
    VOS_UINT8                           ucLastRplmnRatEnableFlg;                /* 0:NV 未激活; 1:NV激活 */
    VOS_UINT8                           aucReserved[2];                          /* 保留 */
}NAS_NVIM_RPLMN_WITH_RAT_STRU;


typedef struct
{
    VOS_UINT8                            ucStatus;                              /* 0:NV 未激活; 1:NV 激活 */
    VOS_UINT8                            ucReserve;                             /* 保留未用 */
    VOS_UINT16                           usSrchHplmnTimerValue;                 /* VPLMN下非首次搜HPLMN时间,单位:分钟 */

}NAS_MMC_NVIM_SEARCH_HPLMN_TIMER_STRU;


typedef struct
{
    VOS_UINT8                           ucTriggerBGSearchFlag;  /* 0: mmc收到高优先级PLMN更新时不启动BG SEARCH; 1:mmc收到高优先级PLMN更新时启动BG SEARCH */
    VOS_UINT8                           ucReserved;
    VOS_UINT16                          usSearchDelayLen;             /* BG SEARCH Delay 时长, 单位: 秒  */
}NAS_MMC_NVIM_HIGH_PRIO_PLMN_REFRESH_TRIGGER_BG_SEARCH_STRU;



/*en_NV_Item_EFust_Service_Cfg 8285*/

typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV项是否激活0：未激活；1：激活 */
    VOS_UINT8                           ucForbidReg2GNetWork;                   /* 0：EFust GSM接入标志定制项去使能。定制项不激活，不检查EFust 27、38。允许GSM接入。
                                                                                   定制项激活，该定制项激活后，UE根据SIM卡中的EFust 27、38位，确定是否允许GSM接入，
                                                                                   如果EFUST文件禁止这两位（同时设置为0），则禁止GSM接入注册，否则GSM可以接入。
                                                                                   注意：SIM卡中不存在EFust 27、38位，因此该功能仅针对USIM卡有效。1：EFust GSM接入标志定制项使能 */
    VOS_UINT8                           ucForbidSndMsg;                         /* 0：EFust短信发送标志定制项去使能。定制项不激活，不检查EFust 10、12。允许SMS发送。
                                                                                   1：EFust短信发送标志定制项使能。NV项激活，该定制项激活后，UE根据SIM卡中EFUST的10、12位，
                                                                                   确定是否禁止短信发送,如果禁止这两位（同时设置为0），则禁止发送SMS；否则，短信可以发送。*/
    VOS_UINT8                           ucReserved[13];
}NAS_MMC_NVIM_EFUST_SERVICE_CFG_STRU;

/*en_NV_Item_UE_MSCR_VERSION 8289*/

typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV是否激活标志, 0: 不激活，1: 激活 */
    VOS_UINT8                           ucUeMscrVersion;                        /* 终端上报的SGSN版本 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_MSCR_VERSION_STRU;

/*en_NV_Item_UE_SGSNR_VERSION 8290*/

typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV是否激活标志, 0: 不激活，1: 激活 */
    VOS_UINT8                           ucUeSgsnrVersion;                       /* 终端上报的SGSN版本 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_SGSNR_VERSION_STRU;

/*en_NV_Item_SteeringofRoaming_SUPPORT_CTRL 8292*/

typedef struct
{
    VOS_UINT8                           ucStatus;                               /* NV是否激活标志, 0: 不激活，1: 激活 */
    VOS_UINT8                           ucCsRejSearchSupportFlg;                /* vodafone的搜网定制需求,CS域失败四次后下发搜网请求,VOS_FALSE:不支持,VOS_TRUE:支持 */
}NAS_MMC_NVIM_CS_REJ_SEARCH_SUPPORT_STRU;

/*en_NV_Item_Max_Forb_Roam_La 8320*/

typedef struct
{
    VOS_UINT8                           ucMaxForbRoamLaFlg;                     /* NV中设置的最大禁止LA个数是否有效: VOS_TRUE:valid;VOS_FALSE:INVALID */
    VOS_UINT8                           ucMaxForbRoamLaNum;                     /* NV中设置的最大禁止LA个数 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_MAX_FORB_ROAM_LA_STRU;

/*en_NV_Item_Default_Max_Hplmn_Srch_Peri 8321*/

typedef struct
{
    VOS_UINT8                           ucDefaultMaxHplmnPeriFlg;               /* 默认最大的HPLMN搜索周期是否有效标志,VOS_TRUE:valid;VOS_FALSE:INVALID */
    VOS_UINT8                           ucDefaultMaxHplmnTim;                   /* 用户可以定义默认的最大的HPLMN搜索周期 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_DEFAULT_MAX_HPLMN_PERIOD_STRU;

/*en_NV_Item_USSD_Apha_To_Ascii 8327*/
/*控制USSD转换字符表*/

typedef struct
{
    VOS_UINT8                       ucStatus;            /*是否激活，0不激活，1激活 */
    VOS_UINT8                       ucAlphaTransMode;    /* 字符表转换*/
}NAS_SSA_NVIM_ALPHA_to_ASCII_STRU;

/*en_NV_Item_Register_Fail_Cnt 8338*/

typedef struct
{
    VOS_UINT8                           ucNvActiveFlag;                         /* 当前NV项是否激活 */
    VOS_UINT8                           ucRegFailCnt;                           /* NV中设置的注册失败的次数，默认值为2，即注册失败两次后发起搜网。 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_MMC_NVIM_REG_FAIL_CNT_STRU;

/*en_NV_Item_CREG_CGREG_CI_Four_Byte_Rpt 8345*/
/* VDF需求: CREG/CGREG命令<CI>域是否以4字节上报的NV项控制结构体 */

typedef struct
{
    VOS_UINT8                           ucStatus;                        /* 1: NV有效标志位，0：无效 */
    VOS_UINT8                           ucCiBytesRpt;                    /* <CI>域上报字节数标志，0：2字节上报, 1：4字节上报 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}NAS_NV_CREG_CGREG_CI_FOUR_BYTE_RPT_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableFlg;                            /* 拨号被拒是否使能 VOS-TRUE:拨号被拒支持 VOS_FALSE:拨号被拒不支持 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
}NAS_MMC_NVIM_DAIL_REJECT_CFG_STRU;

/* en_NV_Item_NDIS_DHCP_DEF_LEASE_TIME 8344 */
typedef struct
{
    VOS_UINT32                          ulDhcpLeaseHour;    /*Range:[0x1,0x2250]*/
}NDIS_NV_DHCP_LEASE_HOUR_STRU;

typedef struct
{
    VOS_UINT32                          ulIpv6Mtu;          /*Range:[1280,65535]*/
}NDIS_NV_IPV6_MTU_STRU;


typedef struct
{
    VOS_UINT8                           ucStatus;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_CCALLSTATE_RPT_STATUS_STRU;


typedef struct
{
   VOS_UINT8                            ucPlmnExactlyCompareFlag;               /* PLMN比较是否进行精确比较的方式的标记 */
   VOS_UINT8                            aucRsv[3];                              /* 保留位 */
}NVIM_PLMN_EXACTLY_COMPARE_FLAG_STRU;


enum PLATFORM_RAT_TYPE_ENUM
{
    PLATFORM_RAT_GSM,                                                       /*GSM接入技术 */
    PLATFORM_RAT_WCDMA,                                                     /* WCDMA接入技术 */
    PLATFORM_RAT_LTE,                                                       /* LTE接入技术 */
    PLATFORM_RAT_TDS,                                                       /* TDS接入技术 */
    PLATFORM_RAT_1X,                                                        /* CDMA-1X接入技术 */
    PLATFORM_RAT_HRPD,                                                      /* CDMA-EV_DO接入技术 */

    PLATFORM_RAT_BUTT
};
typedef VOS_UINT16 PLATFORM_RAT_TYPE_ENUM_UINT16;

#define PLATFORM_MAX_RAT_NUM            (7)                                    /* 接入技术最大值 */


typedef struct
{
    VOS_UINT16                           usRatNum;                              /* 接入技术的数目*/
    PLATFORM_RAT_TYPE_ENUM_UINT16        aenRatList[PLATFORM_MAX_RAT_NUM];  /* 接入技术 */
}PLATAFORM_RAT_CAPABILITY_STRU;

/*en_NV_Item_Rplmn 8216*/

typedef struct
{
    VOS_UINT8                           aucRplmnInfo[56];
}NAS_NVIM_RPLMN_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucSvlteSupportFlag;                     /* SVLTE功能是否支持:0-不支持；1-支持 */
    VOS_UINT8                           aucReserved[3];                         /* 保留 */
}SVLTE_SUPPORT_FLAG_STRU;



typedef struct
{
    VOS_UINT8                           ucNvimActiveFlag;                       /* 0: nv项未激活；1:nv项激活 */
    VOS_UINT8                           ucCsPsMode1EnableLteTimerLen;           /* 1)cs ps mode1 L联合注册eps only成功cs被拒#16/#17/#22达最大次数场景disable lte时启动enable lte定时器时长
                                                                                   2)cs ps mode1 L联合注册cs eps均失败原因值other cause 达最大次数场景disable lte启动enable lte定时器时长,单位:分钟， nv项激活时如果为0默认54分钟 */
    VOS_UINT8                           ucCsfbEmgCallEnableLteTimerLen;         /* L下紧急呼叫无法正常csfb到gu，通过搜网到gu场景disable lte启动enable lte定时器时长，单位:分钟， nv项激活时如果为0默认5分钟 */

    /* 被拒原因值为#16、#17或#18或联合注册成功，Additional Update Result IE消息中
    指示SMS Only或CSFB Not Preferred需要disable lte时，如果协议版本大于等于R11，
    配置被拒绝的PLMN和RAT记录在禁止接入技术网络列表中的惩罚时间，如果为0xFF表示永久惩罚
    单位:分钟 */
    VOS_UINT8                           ucLteVoiceNotAvailPlmnForbiddenPeriod;
}NAS_MMC_NVIM_ENABLE_LTE_TIMER_LEN_STRU;



typedef struct
{
    VOS_UINT8                           ucActiveFlag;                           /* 0: nv项未激活；1:nv项激活 */
    VOS_UINT8                           ucEnableLteTimerLen;                    /* EnableLte timer len, 单位:分钟， nv项激活时如果为0则不使能 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
}NAS_MMC_NVIM_DCM_CUSTOM_DISABLE_LTE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucCsfbMtForceAuthFlag;                  /* 0: nv项未激活；1:nv项激活 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MMC_NVIM_SMC_FAIL_CSFB_MT_FORCE_AUTH_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucT3402Flag;                           /* 0: 不使用LMM_MMC_T3402_LEN_NOTIFY消息中的长度; 1:使用LMM_MMC_T3402_LEN_NOTIFY消息中的长度 */
    VOS_UINT8                           ucHighPrioRatTimerNotEnableLteFlag;    /* 1:高优先级RAT HPLMN TIMER 超时不重新ENABLE lte；0: 高优先级RAT HPLMN TIMER 超时重新ENABLE lte */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
}NAS_MMC_NVIM_DISABLE_LTE_START_T3402_ENABLE_LTE_CFG_STRU;




typedef struct
{
    VOS_UINT8                           ucRejMaxTimesDisableLte;          /* 0: 被拒达最大次数时不按照r12版本disable lte; 1: 被拒达最大次数时按照r12版本disable lte */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MMC_NVIM_REJ_MAX_TIMES_DISABLE_LTE_CFG_STRU;



typedef struct
{
    /* 根据24007 11.2.3.2.3.1.1
       after successful completion of SRVCC handover (see 3GPP TS 23.216 [27]),
       the mobile station shall perform modulo 4 arithmetic operations on V(SD).
       The mobile station shall keep using modulo 4 until the release of the RR
       connection established at SRVCC handover.

       During SRVCC handover the MSCR bit is not provided to the mobile station,
       and therefore the mobile station assumes to access to a Release 99 or
       later core network.
    */
    VOS_UINT8                           ucSrvccSnModuloCfg;                     /* 0: 分配SN按照SRVCC前g_stMmNsd.ucNsdMod处理; 1: 按照协议，分配SN统一按照R99模4处理 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_SRVCC_SN_MODULO_CFG_STRU;


typedef struct
{
    VOS_UINT8                                               ucIsrSupport;       /* ISR ???? */
    VOS_UINT8                                               ucReserve1;
} NAS_NVIM_ISR_CFG_STRU;



typedef struct
{
    VOS_UINT8                                               ucEmcBarTriggerPlmnSearch;       /* VOS_FALSE: 不搜网;  VOS_TRUE: 搜网 */
    VOS_UINT8                                               ucReserve1;
    VOS_UINT8                                               ucReserve2;
    VOS_UINT8                                               ucReserve3;
} NAS_NVIM_EMC_BAR_TRIGGER_PLMN_SEARCH_STRU;



typedef struct
{
    VOS_UINT8                           ucIsRauNeedFollowOnCsfbMtFlg;           /* Csfb mt流程中，RAU是否需要带follow on标记:0-不需要；1-需要 */
    VOS_UINT8                           ucIsRauNeedFollowOnCsfbMoFlg;           /* Csfb mo流程中，RAU是否需要带follow on标记:0-不需要；1-需要 */
    VOS_UINT8                           aucReserved[2];                         /* 保留 */
}NAS_MMC_CSFB_RAU_FOLLOW_ON_FLAG_STRU;


typedef struct
{
   VOS_UINT16                           usSolutionMask;/*控制通过两个Modem的信息交互的增强型的搜索策略；Bit位控制各个搜网策略；bitn=0：第n个策略关闭；bitn=1：第n个策略开启；
                                                         目前只有bit0、bit1 有效；
                                                         BIT0：控制双Modem下通过另一Modem的PLMN信息控制FDD搜网是否跳过的策略是否启动。
                                                         BIT1：通过Modem1的GSM上报的L、TDS邻区信息, Modem0不支持GSM的情况下，T/L丢网后，能够根据传递的邻区频点快速搜索到TDS/LTE；
                                                               如果Modem1传递过来的邻区信息不存在的情况下，也能通过历史频点支持NCELL搜索邻区快速搜索的频率由ucSolution2NcellSearchTimer决定。
                                                         BIT2~BIT15:预留*/
   VOS_UINT8                            ucSolution2NcellQuickSearchTimer;       /*邻区频点快速搜索策略的一阶段时间间隔（单位秒）。*/

   VOS_UINT8                            ucSolution2NcellQuickSearchTimer2;      /* 邻区频点快速搜索策略的二阶段时间间隔（单位秒）。*/
   VOS_UINT8                            aucAdditonCfg[4];
}NV_DSDA_PLMN_SEARCH_ENHANCED_CFG_STRU;



typedef struct
{
    VOS_UINT8   ucRelFlg;
    VOS_UINT8   ucReserve;
}NV_NAS_GMM_REL_CONN_AFTER_PDP_DEACT_STRU;


typedef struct
{
    VOS_UINT32                          ulWband;                                /* 支持的WCDMA主集通路 */
    VOS_UINT32                          ulWbandExt;                             /* 支持的WCDMA主集通路扩展字段 */
    VOS_UINT32                          ulGband;                                /* 支持的GSM主集通路 */
}NAS_NVIM_WG_RF_MAIN_BAND_STRU;



typedef struct
{
    VOS_UINT8                           ucImsVoiceInterSysLauEnable;           /* ISR激活后，异系统从L变换到GU，LAI未改变，是否需要强制LAU */
    VOS_UINT8                           ucImsVoiceMMEnable;         /* IMS移动性管理 NV */
    VOS_UINT8                           aucReserved[2];             /* 保留 */
}NAS_MMC_IMS_VOICE_MOBILE_MANAGEMENT;


typedef struct
{
    VOS_UINT8                           ucLDisabledRauUseLInfoFlag;             /* l disabled后rau是否需要从l获取安全上下文或guti映射信息，vos_true:需要获取；vos_false:无需获取*/
    VOS_UINT8                           ucReserved[3];
}NAS_MMC_LTE_DISABLED_USE_LTE_INFO_FLAG_STRU;



typedef struct
{
    VOS_UINT8                           ucActiveFlg;                            /* 是否激活功能 */
    VOS_UINT8                           ucCsOnlyDataServiceSupportFlg;          /* PS注册被禁止情况下，是否允许数据业务触发注册的标志 */
}NAS_MML_CS_ONLY_DATA_SERVICE_SUPPORT_FLG_STRU;

typedef struct
{
    VOS_UINT8                           ucActiveFLg;                             /* 该定时器是否使能 */                       /* TD开始背景搜的次数 */
    VOS_UINT8                           aucRsv[3];
    VOS_UINT32                          ulFirstSearchTimeLen;                   /* high prio rat timer定时器第一次的时长 单位:秒 */
    VOS_UINT32                          ulFirstSearchTimeCount;                 /* high prio rat timer定时器第一次时长的限制搜索次数 */
    VOS_UINT32                          ulNonFirstSearchTimeLen;                /* high prio rat timer定时器非首次的时长 单位:秒 */
    VOS_UINT32                          ulRetrySearchTimeLen;                   /* high prio rat 搜被中止或不能立即发起重试的时长 单位:秒*/
}NAS_MMC_NVIM_HIGH_PRIO_RAT_HPLMN_TIMER_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucActiveFLg;                            /* 该定时器是否使能 */
    VOS_UINT8                           ucTdThreshold;                          /* TD开始背景搜的次数 */
    VOS_UINT8                           aucRsv[2];
    VOS_UINT32                          ulFirstSearchTimeLen;                   /* high prio rat timer定时器第一次的时长 单位:秒 */
    VOS_UINT32                          ulFirstSearchTimeCount;                 /* high prio rat timer定时器第一次时长的限制搜索次数 */
    VOS_UINT32                          ulNonFirstSearchTimeLen;                /* high prio rat timer定时器非首次的时长 单位:秒 */
    VOS_UINT32                          ulRetrySearchTimeLen;                   /* high prio rat 搜被中止或不能立即发起重试的时长 单位:秒*/
}NAS_MMC_NVIM_HIGH_PRIO_RAT_HPLMN_TIMER_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucUltraFlashCsfbSupportFLg;                 /* 是否支持ultra flash csfb */
    VOS_UINT8                           aucRsv[3];
}NAS_MMC_NVIM_ULTRA_FLASH_CSFB_SUPPORT_FLG_STRU;


typedef struct
{
    VOS_UINT8                           uc3GPP2UplmnNotPrefFlg;                    /* 是否开启3GPP2 pref plmn */
    VOS_UINT8                           aucRsv[3];
}NAS_MMC_NVIM_3GPP2_UPLMN_NOT_PREF_STRU;


typedef struct
{
    VOS_UINT8                           ucHighPrioRatPlmnSrchFlg;                  /* 是否开启高优先级接入技术搜网 */
    VOS_UINT8                           aucReserved1[3];
}NAS_MMC_NVIM_SYSCFG_TRIGGER_PLMN_SEARCH_CFG_STRU;


typedef struct
{
    VOS_UINT32                           ulCsRegEndSessionDelayTime;              /* Lau延迟时长，单位:毫秒 */
    VOS_UINT32                           ulPsRegEndSessionDelayTime;              /* Rau延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsfbCallEndSessionDelayTime;               /* CSFB延迟时长，单位:毫秒 */
    VOS_UINT32                           ulReserve2;
} NAS_MMC_NVIM_DSDS_END_SESSION_DELAY_STRU;



typedef struct
{
    VOS_UINT32                           ulCsRegEndSessionDelayTime;            /* Lau延迟时长，单位:毫秒 */
    VOS_UINT32                           ulPsRegEndSessionDelayTime;            /* Rau延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsfbCallEndSessionDelayTime;         /* CSFB延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsCallEndSessionDelayTime;           /* CS CALL延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsSmsEndSessionDelayTime;            /* CS SMS延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsSsEndSessionDelayTime;             /* CS SS延迟时长，单位:毫秒 */
    VOS_UINT32                           ulCsRegEnableRfOccupyDelayTime;        /* LAU延迟释放抢占保护时长，单位:毫秒 */
    VOS_UINT32                           ulReserve1;
    VOS_UINT32                           ulReserve2;
    VOS_UINT32                           ulReserve3;
    VOS_UINT32                           ulReserve4;
} NAS_MMC_NVIM_DSDS_DELAY_TIME_STRU;


typedef struct
{
    VOS_UINT16                                              usSid;
    VOS_UINT16                                              usNid;
    VOS_UINT16                                              usBandClass;
    VOS_UINT16                                              usChannel;
}CNAS_NVIM_1X_SYSTEM_STRU;


typedef struct
{
    VOS_UINT8                           ucSysNum;
    VOS_UINT8                           aucReserve[3];
    CNAS_NVIM_1X_SYSTEM_STRU            astSystem[CNAS_NVIM_MAX_1X_MRU_SYS_NUM];
}CNAS_NVIM_1X_MRU_LIST_STRU;



typedef struct
{
    VOS_UINT8                           ucReadNvPrlDirectly;
    VOS_UINT8                           ucReadDefaultPrl;        /* 读取Default Prl */
    VOS_UINT8                           ucIsMod1xAvailTimerLen;
    VOS_UINT8                           ucNvPrlCombinedFlag;     /* NV PRL级联标志 */
    VOS_UINT8                           aucReserve[12];

}CNAS_NVIM_TEST_CONFIG_STRU;


typedef struct
{
    VOS_UINT32                          ulCallBackEnableFlg;
    VOS_UINT32                          ulCallBackModeTimerLen;
} CNAS_NVIM_1X_CALLBACK_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucIsAllowCallInForeign;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MSCC_NVIM_CTCC_ROAM_EMC_CALL_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucIsNegSysAdd;
    VOS_UINT8                           aucReserve[15];
}CNAS_NVIM_1X_ADD_AVOID_LIST_CFG_STRU;


typedef struct
{
    CNAS_NVIM_1X_NEG_PREF_SYS_CMP_TYPE_ENUM_UINT8           enNegPrefSysCmpType;
    VOS_UINT8                                               aucReserve[15];
}CNAS_NVIM_1X_NEG_PREF_SYS_CMP_CTRL_STRU;


typedef struct
{
    VOS_UINT8                                               ucIsL3ErrReOrigCount;
    VOS_UINT8                                               ucPrivacyMode;      /* privacy mode flag: 0 - disable 1 - enable */

    VOS_UINT8                                               aucReserve[14];
}CNAS_NVIM_1X_CALL_NVIM_CFG_STRU;


/*****************************************************************************
 结构名    : CNAS_NVIM_1X_SUPPORT_BANDCLASS_MASK_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 终端支持的频段能力掩码  3601
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulSupportBandclassMask;               /* 终端支持的频段能力掩码，每bit表示是否支持对应的频段能力，比如0x00000001表示只支持频段0 */
}CNAS_NVIM_1X_SUPPORT_BANDCLASS_MASK_STRU;

/*****************************************************************************
 结构名    : CNAS_NVIM_HRPD_SUPPORT_BANDCLASS_MASK_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 终端支持的频段能力掩码  3601
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulSupportBandclassMask;               /* 终端支持的频段能力掩码，每bit表示是否支持对应的频段能力，比如0x00000001表示只支持频段0 */
}CNAS_NVIM_HRPD_SUPPORT_BANDCLASS_MASK_STRU;


typedef struct
{
    VOS_UINT16                          ausAvoidTimerLen[CNAS_NVIM_HRPD_AVOID_MAX_PHASE_NUM];
}CNAS_NVIM_HRPD_AVOID_PHASE_STRU;


typedef struct
{
    CNAS_NVIM_HRPD_AVOID_PHASE_STRU     astAvoidPhaseNum[CNAS_NVIM_HRPD_AVOID_REASON_MAX];
}CNAS_NVIM_HRPD_AVOID_SCHEDULE_INFO_STRU;

/*****************************************************************************
结构名称    :NAS_MMC_NVIM_ADAPTION_CAUSE_STRU
使用说明    :用户配置网侧原因的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucCnCause;     /* 网侧原因值 */
    VOS_UINT8                           ucHplmnCause;  /* 用户配置匹配HPLMN的原因值 */
    VOS_UINT8                           ucVplmnCause;  /* 用户配置匹配VPLMN的原因值 */
    VOS_UINT8                           aucReserved[1];
}NAS_MMC_NVIM_ADAPTION_CAUSE_STRU;

/*****************************************************************************
结构名称    :NAS_MMC_NVIM_CHANGE_NW_CAUSE_CFG_STRU
使用说明    :en_NV_Item_ChangeNWCause_CFG NV项结构
*****************************************************************************/
typedef struct
{
    /* CS域注册流程(LU)拒绝原因值替换信息 */
    VOS_UINT8                           ucCsRegCauseNum;
    VOS_UINT8                           aucReserved1[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astCsRegAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    /* PS域注册流程(ATTACH/RAU)拒绝原因值替换信息 */
    VOS_UINT8                           ucPsRegCauseNum;
    VOS_UINT8                           aucReserved2[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astPsRegAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    /* 网络GPRS Detach 流程拒绝原因值替换信息 */
    VOS_UINT8                           ucDetachCauseNum;
    VOS_UINT8                           aucReserved3[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astDetachAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    /* GMM service request流程拒绝原因值替换信息 */
    VOS_UINT8                           ucPsSerRejCauseNum;
    VOS_UINT8                           aucReserved4[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astPsSerRejAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    /* MM ABORT流程拒绝原因值替换信息 */
    VOS_UINT8                           ucMmAbortCauseNum;
    VOS_UINT8                           aucReserved5[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astMmAbortAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    /* CM Service流程拒绝原因值替换信息 */
    VOS_UINT8                           ucCmSerRejCauseNum;
    VOS_UINT8                           aucReserved6[3];
    NAS_MMC_NVIM_ADAPTION_CAUSE_STRU    astCmSerRejAdaptCause[NAS_MMC_NVIM_MAX_CAUSE_NUM];

    VOS_UINT8                           ucHplmnPsRejCauseChangTo17MaxNum; /* HPLMN PS/EPS域拒绝原因值修改为#17的最大次数 */
    VOS_UINT8                           ucHplmnCsRejCauseChangTo17MaxNum; /* HPLMN CS域拒绝原因值修改为#17的最大次数 */
    VOS_UINT8                           ucVplmnPsRejCauseChangTo17MaxNum; /* VPLMN PS/EPS域拒绝原因值修改为#17的最大次数 */
    VOS_UINT8                           ucVplmnCsRejCauseChangTo17MaxNum; /* VPLMN CS域拒绝原因值修改为#17的最大次数 */
}NAS_MMC_NVIM_CHANGE_NW_CAUSE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucRelPsSignalConFlg;/* 是否开启数据域网络防呆功能 */

    VOS_UINT8                           ucPdpExistNotStartT3340Flag; /* rau或attach请求不带follow on，网络回复attach accept或rau accept也不带follow on，存在pdp上下文场景是否需要启动T3340,0:需要启动T3340; 1:不需要启动 */
    VOS_UINT8                           aucReserved[2];

    VOS_UINT32                          ulT3340Len;         /* 配置的GMM T3340的时长,单位:秒 */
}NAS_MMC_NVIM_REL_PS_SIGNAL_CON_CFG_STRU;


typedef struct
{
    VOS_UINT8   ucEnableFlag;/*使能开关 */
    VOS_UINT8   aucReserve[3];
    VOS_UINT32  ulMcc;    /*用于测试使用指定MCC*/
    VOS_UINT32  ulMnc;    /*用于测试使用指定MNC*/
}NAS_RABM_NVIM_WCDMA_VOICE_PREFER_STRU;

typedef struct
{
    VOS_UINT16                              usEnable;                          /* 全网通特性开关 */
    VOS_UINT16                              usReserved;
}NAS_NV_TRI_MODE_ENABLE_STRU;


typedef struct
{
    VOS_UINT32                              ulProfileId;                        /* 根据使用场景，控制前端器件的上下电（ABB，TCXO，RF）以及RF通道的控制。
                                                                                   由AT命令下发配置。默认值为0。取值范围0-7。 */
    VOS_UINT32                              ulReserved[3];                     /* 保留，将来扩展使用 */
}NAS_NV_TRI_MODE_FEM_PROFILE_ID_STRU;




typedef struct
{
    VOS_UINT16                              usABBSwitch;                       /* 用于ABB PLL开关的控制。0:ABB CH0 1:ABB CH1 2:ABB CH0&CH1都打开 */
    VOS_UINT16                              usRFSwitch;                        /* 用于RFIC电源开关的控制。0:RFIC使用MIPI0控制供电方式 1：RFIC使用MIPI1控制供电方式 2：同时打开两路电源。*/
    VOS_UINT16                              usTCXOSwitch;                      /* 0:TCXO0 1:TCXO1 */
    VOS_UINT16                              usReserved;                        /* 保留，将来扩展使用 */
}NAS_NV_MODE_BASIC_PARA_STRU;

/*****************************************************************************
 结构名    : NV_TRI_MODE_CHAN_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 全网通通道参数配置参数  (不能改变)
*****************************************************************************/
typedef struct
{
    NAS_NV_MODE_BASIC_PARA_STRU            stModeBasicPara[2];                  /* 下标[0]:表示GSM模式下的前端器件的上下电控制。
                                                                                   下标[1]:表示WCDMA模式下的前端器件的上下电控制。
                                                                                    注：副卡时，不使用WCDMA模式的配置。*/
    VOS_UINT32                              ulRfSwitch;                         /* 用于控制共分集的开关 */
    VOS_UINT32                              ulGsmRficSel;                       /* 信令模式下当前使用的通道（0：RF0,1：RF1） */
    VOS_UINT32                              ulGpioCtrl;                         /* gpio */
    VOS_UINT32                              aulReserved[14];                    /* 保留，将来扩展使用 */
}NAS_NV_TRI_MODE_CHAN_PARA_STRU;

/*****************************************************************************
 结构名    : NAS_NV_TRI_MODE_FEM_CHAN_PROFILE_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 8种场景的通道配置
*****************************************************************************/
typedef struct
{
    NAS_NV_TRI_MODE_CHAN_PARA_STRU          stPara[NAS_NV_TRI_MODE_CHAN_PARA_PROFILE_NUM];  /* 最多支持8个场景的配置 */
}NAS_NV_TRI_MODE_FEM_CHAN_PROFILE_STRU;






typedef struct
{
    VOS_UINT8                           ucCsmoSupportedFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MMC_NVIM_CSMO_SUPPORTED_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucHplmnInEplmnDisplayHomeFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MMC_NVIM_ROAM_DISPLAY_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usMtCsfbPagingProcedureLen;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
}NAS_MMC_NVIM_PROTECT_MT_CSFB_PAGING_PROCEDURE_LEN_STRU;


typedef struct
{
    VOS_UINT8                           ucRoamPlmnSelectionSortFlg;
    VOS_UINT8                           ucSrchUOplmnPriorToDplmnFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
}NAS_MMC_NVIM_ROAM_PLMN_SELECTION_SORT_CFG_STRU;



typedef struct
{
    VOS_UINT16                          usTotalTimerLen;                        /* 各阶段搜网总时长,单位:s */
    VOS_UINT16                          usSleepTimerLen;                        /* 搜网间隔的睡眠时长,单位:s */
    VOS_UINT16                          usReserve1;
    VOS_UINT16                          usReserve2;
    VOS_UINT8                           ucHistoryNum;                           /* 第几次的历史搜 会变成 PrefBand/FullBand搜 */
    VOS_UINT8                           ucPrefBandNum;                          /* 第几次的PrefBand搜 会变成 FullBand搜 */
    VOS_UINT8                           ucFullBandNum;                          /* 第几次FullBand搜後, 此阶段结束, 进入下一阶段 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
}NAS_MMC_NVIM_OOS_PLMN_SEARCH_PATTERN_CFG_STRU;


typedef struct
{
    NAS_MMC_NVIM_OOS_PLMN_SEARCH_PATTERN_CFG_STRU           stPhaseOnePatternCfg;    /* 第一阶段的搜网类型配置与睡眠时长 */
    NAS_MMC_NVIM_OOS_PLMN_SEARCH_PATTERN_CFG_STRU           stPhaseTwoPatternCfg;    /* 第二阶段的搜网类型配置与睡眠时长 */
    NAS_MMC_NVIM_OOS_PLMN_SEARCH_PATTERN_CFG_STRU           stPhaseThreePatternCfg;  /* 保留：第三阶段的搜网类型配置与睡眠时长 */
    NAS_MMC_NVIM_OOS_PLMN_SEARCH_PATTERN_CFG_STRU           stPhaseFourPatternCfg;   /* 保留：第四阶段的搜网类型配置与睡眠时长 */
}NAS_MMC_NVIM_OOS_PLMN_SEARCH_STRATEGY_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucIsSupportSkipBandTypeSearch;                              /* 是否支持跳过band类型的搜网 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucMccNum;                                                   /* 支持跳过band类型搜网的国家码个数 */
    VOS_UINT32                          aulMccList[NAS_MML_NVIM_MAX_SKIP_BAND_TYPE_SEARCH_MCC_NUM]; /* 支持跳过band类型搜网的国家码 */
}NAS_MMC_NVIM_SKIP_BAND_TYPE_PLMN_SEARCH_CFG_STRU;


typedef struct
{
    VOS_UINT8           ucPrefbandCfg;
    VOS_UINT8           ucSearchTypeAfterHistoryInAreaLostScene;
    VOS_UINT8           ucSearchTypeAfterGetGeoFail;
    VOS_UINT8           ucReserved1;

    VOS_UINT32          ulHistoryCfg;
    VOS_UINT32          ulReserved2;
}NAS_MMC_NVIM_NON_OOS_PLMN_SEARCH_FEATURE_SUPPORT_CFG_STRU;



typedef struct
{
    VOS_UINT32                          ulDelayNetworkSearchTimerLenAfterEmc;   /* 紧急呼之后不搜网时长，单位:s，时长为0表示要搜网 */
    VOS_UINT8                           ucBgHistorySupportFlg;                  /* BG方式history搜是否支持 */
    VOS_UINT8                           ucPrefBandListSupportFlg;               /* 列表方式pref band搜是否支持 */
    VOS_UINT8                           ucFullBandListSupportFlg;               /* 列表方式full band搜是否支持 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
}NAS_MMC_NVIM_OOS_BG_NETWORK_SEARCH_CUSTOM_STRU;


typedef struct
{
    VOS_UINT8                           ucExtendT3240LenFlg;                    /* 是否开启T3240时长的定制 0:未开启 1:开启 */
    VOS_UINT8                           ucReserve;                              /* 保留位 */
    VOS_UINT16                          usCustomizedT3240Len;                   /* 定制的T3240定时器时长。单位为毫秒，最大支持65535毫秒 */
}NAS_NVIM_EXTEND_T3240_LEN_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucRatOrderNum;                          /* 获取地理位置信息的接入技术个数 */
    VOS_UINT8                           aucRatOrder[NAS_NVIM_MAX_RAT_NUM];      /* 获取地理位置信息的接入技术优先级 */
}NAS_NVIM_GET_GEO_PRIO_RAT_LIST_STRU;


typedef struct
{
    VOS_UINT8                                               ucGetGeoFlag;   /* 使用比特位标记各场景下是否发起GET GEO; 最低位表示上电是否GET GEO; 次低位表示飞行模式是否GET GEO; 第3低位表示OOC是否GET GEO
                                                                                        比如: 0x07, 表示上述所有场景都GET GEO */
    VOS_UINT8                                               ucGetGeoTimerlen;               /* 获取国家码定时器时长，单位是秒 */
    VOS_UINT8                                               ucScanTypeOfFlightModeGetGeo;   /* 飞行模式GET GEO时FFT扫频类型*/
    VOS_UINT8                                               ucScanTypeOfOocGetGeo;          /* OOC GET GEO时FFT扫频类型*/
    VOS_UINT32                                              ulGeoEffectiveTimeLen;          /* 国家码的有效性时长，单位是分钟 */
    NAS_NVIM_GET_GEO_PRIO_RAT_LIST_STRU                     stGetGeoPrioRatList;            /* 获取地理位置信息的接入技术优先级列表信息 */
}NAS_NVIM_GET_GEO_CFG_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucLowPrioAnycellSearchLteFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_LOW_PRIO_ANYCELL_SEARCH_LTE_FLG_STRU;


typedef struct
{
    VOS_UINT8                           ucDeleteRplmnFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_REFRESH_RPLMN_WHEN_EPLMN_INVALID_CFG_STRU;


typedef struct
{
    VOS_UINT8                                               ucActiveFlag;       /* NV项是否激活 */
    VOS_UINT8                                               ucReserved1;
    VOS_UINT8                                               ucReserved2;
    VOS_UINT8                                               ucReserved3;
    VOS_UINT32                                              ulRecordNum;        /* 记录的次数 */
}NAS_NVIM_NW_SEARCH_CHR_RECORD_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucPrlData[CNAS_NVIM_PRL_SIZE];
}CNAS_NVIM_1X_EVDO_PRL_LIST_STRU;


typedef struct
{
    VOS_UINT8                           ucPrlData[CNAS_NVIM_CBT_PRL_SIZE];
}CNAS_NVIM_CBT_PRL_LIST_STRU;


typedef struct
{
    VOS_UINT16                          usSid;
    VOS_UINT16                          usNid;
    VOS_UINT16                          usBand;
    VOS_UINT16                          usReserved;
}CNAS_NVIM_1X_HOME_SID_NID_STRU;


typedef struct
{
    VOS_UINT8                           ucSysNum;
    VOS_UINT8                           aucReserve[3];
    CNAS_NVIM_1X_HOME_SID_NID_STRU      astHomeSidNid[CNAS_NVIM_MAX_1X_HOME_SID_NID_NUM];
}CNAS_NVIM_1X_HOME_SID_NID_LIST_STRU;


typedef struct
{
    VOS_UINT16                          usTimes;
    VOS_UINT16                          usTimerLen;
}CNAS_NVIM_OOC_TIMER_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucMru0SearchTimerLen;
    VOS_UINT8                           ucPhaseNum;
    VOS_UINT8                           uc1xOocDoTchPhase1TimerLen;              /* Do TCH，前4次尝试 Ooc Timer 最短时长 */
    VOS_UINT8                           uc1xOocDoTchPhase2TimerLen;              /* Do TCH，4次以上尝试 Ooc Timer 最短时长 */
    CNAS_NVIM_OOC_TIMER_INFO_STRU       astOocTimerInfo[CNAS_NVIM_MAX_OOC_SCHEDULE_PHASE_NUM];
}CNAS_NVIM_OOC_TIMER_SCHEDULE_INFO_STRU;

typedef struct
{
    VOS_UINT8                                               ucInsertOrigChanFlg;        /* 在同步请求前是否插入先前驻留频点 */
    NAS_NVIM_CHAN_REPEAT_SCAN_ENUM_UINT8                    enChanRepeatScanStrategy;   /*频点重复搜索策略 */
    VOS_UINT8                                               aucReserved[2];
}CNAS_NVIM_OOC_REPEAT_SCAN_STRATEGY_INFO_STRU;


typedef struct
{
    NAS_NVIM_EPDSZID_SUPPORT_TYPE_ENUM_UINT8                ucEpdszidType;  /* EPDSZID支持类型 */
    VOS_UINT8                                               ucHatLen;       /* HAT时长，单位s */
    VOS_UINT8                                               ucHtLen;        /* HT时长，单位s */
    VOS_UINT8                                               aucReserved[5];
}CNAS_NVIM_1X_EPDSZID_FEATURE_CFG_STRU;


typedef struct
{
    VOS_UINT8                          aucSubnet[CNAS_NVIM_HRPD_SUBNET_LEN];
    VOS_UINT16                         usBandClass;
    VOS_UINT16                         usChannel;
}CNAS_NVIM_HRPD_SYSTEM_STRU;


typedef struct
{
    VOS_UINT8                           ucSysNum;
    VOS_UINT8                           aucRsv[3];
    CNAS_NVIM_HRPD_SYSTEM_STRU          astSystem[CNAS_NVIM_MAX_HRPD_MRU_SYS_NUM];
}CNAS_NVIM_HRPD_MRU_LIST_STRU;


typedef struct
{
    VOS_UINT32                          ulSicValue;
}CNAS_NVIM_1X_LAST_SCI_STRU;


typedef struct
{
    VOS_UINT8                           ucNvimActiveFlag;                       /* 0: nv项未激活；1:nv项激活 */
    VOS_UINT8                           ucWaitImsVoiceAvailTimerLen;            /* 等待IMS VOICE的可用指示的定时器时长,单位为秒级,需要转换为毫秒 */
    VOS_UINT8                           ucWaitImsWithWifiVoiceAvailTimerLen;    /* 支持IMS WIFI时,等待IMS VOICE的可用指示的定时器时长,单位为秒级,需要转换为毫秒 */
    VOS_UINT8                           ucReserved;
}NAS_NVIM_WAIT_IMS_VOICE_AVAIL_TIMER_LEN_STRU;

typedef struct
{
    VOS_UINT32                          ulIsValid;
    VOS_UINT8                           ucLocType;
    VOS_UINT8                           ucLocLen;
    VOS_UINT16                          usSID;
    VOS_UINT16                          usNID;
    VOS_UINT8                           ucPacketZoneID;
    VOS_UINT8                           ucReserve;
}CNAS_NVIM_HRPD_LOC_INFO_STRU;


typedef struct
{
    VOS_UINT32                          ulIsValid;
    VOS_UINT16                          usStrgBLOBType;
    VOS_UINT8                           ucStrgBLOBLen;
    VOS_UINT8                           aucStorageBLOB[CNAS_NVIM_MAX_STORAGE_BLOB_LEN];
    VOS_UINT8                           aucReserve[2];
}CNAS_NVIM_HRPD_STORAGE_BLOB_STRU;


typedef struct
{
    VOS_UINT8                           ucLteRejCause14Flg;               /* 是否开启LTE #14原因拒绝优化，0: 未开启；1:开启 */
    VOS_UINT8                           aucReserved[1];
    VOS_UINT16                          usLteRejCause14EnableLteTimerLen; /* LTE #14原因拒绝时，通过搜网到gu场景disable lte启动enable lte定时器时长，单位:分钟 */
}NAS_MMC_NVIM_LTE_REJ_CAUSE_14_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucIsT3396RunningDisableLteFlg;            /* LMM->MMC ATTACH IND结果T3396定时器运行是否需要disable lte的配置，0: 未开启；1:开启 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_MMC_NVIM_T3396_RUNNING_DISABLE_LTE_CFG_STRU;


typedef struct
{
    VOS_UINT32                          ulActiveFlag;
    VOS_UINT32                          ulExpireTimerLen;
    VOS_UINT16                          usMaxNoOfRetry;
    VOS_UINT8                           ucMaxConnFail;
    VOS_UINT8                           aucRsv[1];
}CNAS_EHSM_RETRY_CONN_EST_NVIM_INFO_STRU;


typedef struct
{
    VOS_UINT32                          ulActiveFlag;
    VOS_UINT32                          ulExpireTimerLen;
    VOS_UINT16                          usMaxNoOfRetry;
    VOS_UINT8                           aucRsv[2];
}CNAS_NVIM_EHRPD_PDN_SETUP_RETRY_STRU;


enum NAS_SMS_PS_CONCATENATE_ENUM
{
    NAS_SMS_PS_CONCATENATE_DISABLE      = 0,
    NAS_SMS_PS_CONCATENATE_ENABLE,

    NAS_SMS_PS_CONCATENATE_BUTT
};
typedef VOS_UINT8 NAS_SMS_PS_CONCATENATE_ENUM_UINT8;


typedef struct
{
    NAS_SMS_PS_CONCATENATE_ENUM_UINT8   enSmsConcatenateFlag;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
} NAS_NV_SMS_PS_CTRL_STRU;

typedef struct
{
    VOS_UINT8                           ucFilterEnableFlg;
    VOS_UINT8                           aucReserved[3];
} NAS_NV_PRIVACY_FILTER_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucCLOosTimerMaxExpiredTimes;    /* 低功耗模式下Available Timer定时器时长超时次数 */

    VOS_UINT8                           ucCLSysAcqWaitHsdCnfTimerLen;   /* MSCC CL搜网等待HSD搜网结果定时器时长 */

    VOS_UINT16                          usReserved1;/* 原始系统不在MSPL表中时,bsr定时器时长 */

    VOS_UINT32                          ulFirstSearchAvailTimerLen; /* Contains the Available timer length to be used , when the
                                                                    number of successive triggers is less than ulFirstSearchAvailTimerCount */

    VOS_UINT32                          ulFirstSearchAvailTimerCount; /* For the number successive triggers of avaiable timer less
                                                                      than or equal to ulFirstSearchAvailTimerCount , MSCC uses a timer
                                                                      length value of ulFirstSearchAvailTimerLen */

    VOS_UINT32                          ulDeepSearchAvailTimerLen;  /* For the number successive triggers of avaiable timer greater
                                                                    than ulFirstSearchAvailTimerCount , MSCC uses a timer
                                                                    length value of ulDeepSearchAvailTimerLen */

    VOS_UINT32                          ulScanTimerLen; /* Contains the scan timer length */
    VOS_UINT8                           ucBsrCtrlDoEnterIdleRstLen;     /* BSR 10秒尝试定时器运行时，DO从CONN态变为IDLE态，不立即发起bsr，
                                                                                而是重新启动尝试定时器，定时器时长，定时器超时后，再发起bsr，防止底层异频切换，导致短暂进入idle态这种情况  */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
    VOS_UINT32                          ulSleepTimerLen; /* Contains the sleep timer length */
}NAS_NVIM_MSCC_SYS_ACQ_TIMER_CFG_STRU;



typedef struct
{
    VOS_UINT32                                              ulMcc;
    VOS_UINT32                                              ulMnc;
    VOS_UINT16                                              usSid;
    VOS_UINT16                                              usNid;

    NAS_MSCC_NVIM_SYS_PRI_CLASS_ENUM_UINT8                  en1xPrioClass;
    NAS_MSCC_NVIM_SYS_PRI_CLASS_ENUM_UINT8                  enAIPrioClass;
    VOS_UINT8                                               aucRsv[2];  /* remain four bytes in future */
}NAS_NVIM_1X_LOC_INFO_STRU;


typedef struct
{
    VOS_UINT32                                              ulMcc;                                  /* MCC,3 bytes */
    VOS_UINT32                                              ulMnc;                                  /* MNC,2 or 3 bytes */
    NAS_MSCC_NVIM_SYS_PRI_CLASS_ENUM_UINT8                  enPrioClass;
    VOS_UINT8                                               aucRsv[3];  /* remain four bytes in future */
}NAS_NVIM_3GPP_LOC_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucIsLocInfoUsedInSwitchOn;
    NAS_NVIM_LC_RAT_COMBINED_ENUM_UINT8 enSysAcqMode;
    VOS_UINT8                           ucIs1xLocInfoValid;
    VOS_UINT8                           ucIsLteLocInfoValid;
    NAS_NVIM_1X_LOC_INFO_STRU           st1xLocInfo;
    NAS_NVIM_3GPP_LOC_INFO_STRU         st3gppLocInfo;
}NAS_NVIM_MMSS_LAST_LOCATION_INFO_STRU;


typedef struct
{

    VOS_UINT8                                               ucReAcqLteOnHrpdSyncIndFlag; /* The NVIM Flag controls if
                                                                                                       MSCC must search  for LTE Service
                                                                                                       when HSD sends Sync Ind, if LTE
                                                                                                       is preferred */

    VOS_UINT8                                               ucIs1xLocInfoPrefThanLte;  /* The NVIM Flag controls if CDMA 1x
                                                                                                     Loc info is more preferred than LTe
                                                                                                     Loc Info */
    VOS_UINT8                                               aucReserved[2];
    NAS_NVIM_MSCC_SYS_ACQ_TIMER_CFG_STRU                    stMmssSysAcqTimerCfg;  /* Contains the Timer Configuration
                                                                                                for MMSS System Acquire */
}NAS_NVIM_MMSS_SYSTEM_ACQUIRE_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucMlplMsplActiveFlag;
    VOS_UINT8                           aucRsv[3];
    VOS_UINT16                          usMlplBufSize;
    VOS_UINT16                          usMsplBufSize;
    VOS_UINT8                           aucMlplBuf[NAS_MSCC_NVIM_MLPL_SIZE];
    VOS_UINT8                           aucMsplBuf[NAS_MSCC_NVIM_MSPL_SIZE];
}NAS_MSCC_NVIM_MLPL_MSPL_STRU;


typedef struct
{
    VOS_UINT8                           ucMobTermForNid;
    VOS_UINT8                           ucMobTermForSid;
    VOS_UINT8                           ucMobTermHome;
    VOS_UINT8                           ucRsv;
}CNAS_NVIM_1X_MOB_TERM_STRU;


typedef struct
{
    VOS_UINT8                           ucActiveFlag;           /* NV item is active not not */
    VOS_UINT8                           ucRsv1;
    VOS_UINT16                          usActTimerLen;          /* Session activate timer length, unit is second */
    VOS_UINT8                           ucMaxActCountConnFail;  /* Max session activate count of reason conntion fail */
    VOS_UINT8                           ucMaxActCountOtherFail; /* Max session activate count of reason other fail */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}CNAS_HSM_NVIM_SESSION_RETRY_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucStartUatiReqAfterSectorIdChgFlg;   /* 为了通过RF测试和CCF2.2.2.8用例，增加NV控制
                                                                                          NV打开(默认):参照标杆只要sector ID发生变化就启动UATI申请流程
                                                                                          NV关闭:      严格按照协议C.S0024 7.3.7.1.6.1处理，即只有在HO或者
                                                                                                       Conn Close后，sector ID发生变化才启动UATI申请流程 */

    VOS_UINT8                           ucWaitUatiAssignTimerLenInAmpSetup; /* 单位:秒(s),默认5s.在某些场景和设备商的网络上，为了降低UE的接入时长，
                                                                                        缩短UE等UATI assign的时长，该时长最短为5s，最长为120s(协议定时器时长) */
    VOS_UINT8                           ucWaitUatiAssignTimerLenInAmpOpen; /* 单位:秒(s),默认120s(协议定时器时长)。该时长最短5s，最长120s。*/

    VOS_UINT8                           ucUatiReqRetryTimesWhenUatiAssignTimerExpireInAmpOpen;/* 增加NV项控制在open态时当UATI req发送成功后
                                                                                                             等网侧UATI assign超时后的重试次数,默认不重试，最大重试4次 */
    VOS_UINT8                           ucClearKATimerInConnOpenFlg;                          /* 增加NV项控制，是否在连接建立成功后清除keep alive流程
                                                                                                             NV打开(默认): 链接建立成功后，停止keep alive定时器，重置keep alive相关的计数
                                                                                                                           链接断开后，开启keep alive定时器，重新启动keep alive流程。
                                                                                                             NV关闭:       链接建立成功后，不清除keep alive流程，按照协议处理 */
    VOS_UINT8                           ucRecoverEhrpdAvailFlg;                              /* 增加NV项控制，如果UE平台能力支持ehrpd,是否在开关机后恢复ehrpd能力
                                                                                                             NV打开:        开关机后恢复ehrpd能力，如果关机前是hrpd的会话，则开机后需要重新协商
                                                                                                             NV关闭(默认):  开关机后不恢复ehrpd能力，如果关机前是hrpd的会话，则开机后执行recover会话流程 */

    VOS_UINT8                           ucRecoverEhrpdCapAfterSessionCloseFlg;              /* 增加NV项控制，网络主动session close后，是否需要重新开启eHRPD能力标记，默认关闭
                                                                                                             NV打开:       网络主动发送session close后，UE恢复eHRPD能力标记(该NV目前只在电信准入测试时打开)
                                                                                                             NV关闭(默认):  网络主动发送session close后，UE不恢复eHRPD能力标记*/
    VOS_UINT8                           ucCloseEhrpdCapAfterSyscfgNotSupportLteFlg;         /* 增加NV项控制，产品线非标定制。关闭4G开关后，从下一次session激活（包括恢复）开始，是否关闭eHRPD能力标记，默认关闭
                                                                                                             NV打开:       4G开关关闭，关闭UE eHRPD能力(该NV目前只针对产品线电信入网测试，进出飞行模式性能用例测试时打开)
                                                                                                             NV关闭(默认): 如果UE平台能力支持eHRPD,4G开关即使关闭，UE仍支持eHRPD能力*/

    VOS_UINT32                          ulUatiSigProtectTimeLen;                             /*  UATI更新过程中的信令保护时长(收到CTTF对于Uati req的SNP_DATA_CNF后，等UATI assignment的保护时长)，
                                                                                                             默认1200ms,单位ms */

    VOS_UINT8                           aucRsv1[20];
}CNAS_HSM_NVIM_SESSION_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucIsKeepAliveInfoValid;    /* If TRUE, then the Keep alive paramters are valid */
    VOS_UINT8                           ucRsv1;                    /* for padding */
    VOS_UINT16                          usTsmpClose;               /* stores the TsmpClose value of the last session. Unit is minutes */
    VOS_UINT32                          ulTsmpCloseRemainTime;     /* Stores the time remaining for Tsmpclose minutes to
                                                                               expire. Unit is seconds */
    VOS_UINT32                          aulLastPowerOffSysTime[2]; /* Stores the CDMA system time at last Power Off.
                                                                              Unit is Milliseconds. */
}CNAS_HSM_NVIM_SESSION_KEEP_ALIVE_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucEhrpdSupportFlg;       /* EHRPD is support or not */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}NAS_NVIM_EHRPD_SUPPORT_FLG_STRU;



typedef struct
{
    VOS_UINT8                           ucActiveFlag;                           /* NV项是否激活 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
    VOS_UINT32                          ulTotalTimeLen;                         /* 第一阶段搜网总时长,单位:s */
}NAS_NVIM_PLMN_SEARCH_PHASE_ONE_TOTAL_TIMER_CFG_STRU;



typedef struct
{
    VOS_UINT16                          usRegFailCauseNum;                      /* 支持的注册失败原因值个数,个数为0表示不支持该特性 */
    VOS_UINT16                          usForbLaTimeLen;                        /* 禁止LA时长,单位:s */
    VOS_UINT16                          usPunishTimeLen;                        /* 放乒乓机制中需要惩罚的时长 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT16                          ausRegFailCauseList[NAS_MML_NVIM_MAX_REG_FAIL_CAUSE_NUM];
}NAS_MMC_NVIM_CUSTOMIZED_FORB_LA_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usCsgAutonomousSrchFirstSrchTimeLen;                   /* CSG自主搜网定时器第一次的时长 */
    VOS_UINT8                           ucCsgAutonomousSrchFirstSrchTimeCount;                 /* CSG自主搜网定时器第一次的次数 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT16                          usCsgAutonomousSrchSecondSrchTimeLen;                  /* CSG自主搜网定时器第二次的时长 */
    VOS_UINT8                           ucCsgAutonomousSrchSecondSrchTimeCount;                /* CSG自主搜网定时器第二次的次数 */
    VOS_UINT8                           ucReserved2;
} NAS_NVIM_CSG_AUTONOMOUS_SEARCH_CFG_STRU;


typedef struct
{
    NVIM_PLMN_VALUE_STRU                stPlmnId;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT32                          ulCsgId;
}NAS_NVIM_PLMN_WITH_CSG_ID_STRU;



typedef struct
{
    VOS_UINT8                           ucPlmnWithCsgIdNum;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
    NAS_NVIM_PLMN_WITH_CSG_ID_STRU      astPlmnWithCsgIdList[NAS_NVIM_MAX_PLMN_CSG_ID_NUM];
}NAS_NVIM_ALLOWED_CSG_LIST_STRU;

typedef struct
{
    VOS_UINT8                           ucPlmnWithCsgIdNum;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
    NAS_NVIM_PLMN_WITH_CSG_ID_STRU      astPlmnWithCsgIdList[NAS_NVIM_MAX_PLMN_CSG_ID_NUM];
}NAS_NVIM_UE_BASED_OPERATOR_CSG_LIST_STRU;

typedef struct
{
    VOS_UINT8                           ucNum;
    VOS_UINT8                           ucRat;                                  /* 记录duplicate rplmn的接入技术*/
    VOS_UINT8                           aucReserve[2];                          /*NV项相关的结构体，在4字节方式下，需手动补齐空洞*/
    NVIM_PLMN_VALUE_STRU                astRplmnAndEplmnList[NVIM_MAX_EPLMN_NUM];
}NAS_NVIM_CSG_DUPLICATED_RPLMN_AND_EPLMN_LIST_STRU;


typedef struct
{
    /* CSG禁止网络惩罚时长,单位秒 */
    VOS_UINT16                          usCsgPlmnForbiddenPeriod;

    /* 配置CSG网络被拒哪些原因值时需要将该网络加入禁止csg网络列表 */
    VOS_UINT8                           ucCauseNum;
    VOS_UINT8                           ucReserved;
    VOS_UINT16                          ausCause[NAS_NVIM_MAX_CSG_REJ_CAUSE_NUM];
}NAS_NVIM_CSG_FORBIDDEN_PLMN_CFG_STRU;

typedef struct
{
    VOS_UINT8                                               ucBitOp1:1;
    VOS_UINT8                                               ucBitOp2:1;
    VOS_UINT8                                               ucBitOp3:1;
    VOS_UINT8                                               ucBitOp4:1;
    VOS_UINT8                                               ucBitOp5:1;
    VOS_UINT8                                               ucBitOp6:1;
    VOS_UINT8                                               ucBitOp7:1;
    VOS_UINT8                                               ucBitOp8:1;
    VOS_UINT8                                               ucReserved0;
    VOS_UINT8                                               ucIsSupportCsgFlag;             /* VOS_TRUE:UE支持CSG功能; VOS_FALSE:UE不支持CSG功能 */
    VOS_UINT8                                               ucCsgListOnlyReportOperatorCsgListFlag; /* VOS_TRUE:csg列表搜网结果只上报在operator csg list中网络; VOS_FALSE:CSG列表搜网结果所有CSG ID都上报 */
    NAS_NVIM_CSG_FORBIDDEN_PLMN_CFG_STRU                    stCsgForbiddenPlmnCfg;       /* CSG禁止网络相关配置 */
    NAS_NVIM_ALLOWED_CSG_LIST_STRU                          stAllowedCsgList;               /* 记录Allowed CSG List信息 */
    VOS_UINT32                                              ulCsgPeriodicSearchPeriod;      /* CSG周期性搜网时长,单位分钟 */
    NAS_NVIM_CSG_DUPLICATED_RPLMN_AND_EPLMN_LIST_STRU       stDuplicatedRplmnAndEplmnList;  /* 关机时如果之前进行过CSG指定搜网，且关机时驻留CSG网络，则记录CSG指定搜网前的RPLMN和EPLMN*/
    NAS_NVIM_CSG_AUTONOMOUS_SEARCH_CFG_STRU                 stCsgAutonomousSrchCfg;         /* CSG自主搜网相关配置 */
	NAS_NVIM_UE_BASED_OPERATOR_CSG_LIST_STRU                stUeBasedOperatorCsgList;       /* 记录Based CSG List信息 */
    VOS_UINT8                                               ucRplmnCellType;                /* 上次驻留小区类型，用于判定是否更新duplicated RPLMN */
    VOS_UINT8                                               ucReserved1;
    VOS_UINT8                                               ucReserved2;
    VOS_UINT8                                               ucReserved3;
    VOS_UINT32                                              ulReserved1;
} NAS_NVIM_CSG_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucKeepCsForbInfoFlg;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_KEEP_CS_FORB_INFO_WHEN_PS_REG_SUCC_STRU;


typedef struct
{
    VOS_UINT8                           ucNwIgnoreAuthFailFlg;                  /* 当网络对鉴权失败结果没有响应，释放链路时是否清楚CKSN的配置 */
    VOS_UINT8                           ucUsimGsmAuthActiveFlg;                            /* USIM卡在GSM下做2G cs/ps 鉴权，在3G下是否需要清除CS/ps CKSN值 */
    VOS_UINT8                           ucUsimDoneGsmCsAuthFlg;
    VOS_UINT8                           ucUsimDoneGsmPsAuthFlg;
}NAS_NVIM_CLEAR_CKSN_STRU;


typedef struct
{
    VOS_UINT8                           ucNetSelMenuFlg;                        /* 网络选择菜单控制，VOS_TRUE:激活；VOS_FALSE:未激活 */
    VOS_UINT8                           ucRatBalancingFlg;                      /* 接入技术平衡控制，VOS_TRUE:激活；VOS_FALSE:未激活 */
    VOS_UINT8                           aucReserved[2];                         /* 保留 */
}NAS_NVIM_ATT_ENS_CTRL_STRU;



typedef struct
{
    VOS_UINT8                           ucEnableFlg;                            /* FRAT特性是否生效 */
    VOS_UINT8                           ucIgnitionState;                        /* FRAT特性生效时，当前Ignition状态 */
    VOS_UINT8                           ucImsiPlmnNum;
    VOS_UINT8                           ucFratPlmnNum;

    VOS_UINT32                          ulFratSlowTimerValue;                   /* FRAT特性生效时，Slow Timer时长，单位 秒，默认值180分钟 */
    VOS_UINT32                          ulFratFastTimerValue;                   /* FRAT特性生效时，Fast Timer时长，单位 秒，默认值3分钟 */
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnIdList[NAS_MML_NVIM_FRAT_MAX_IMSI_NUM];          /* FRAT特性生效时，支持的IMSI的PLMN的列表 */
    NAS_SIM_FORMAT_PLMN_ID              astFratPlmnIdList[NAS_MML_NVIM_FRAT_MAX_PLMN_ID_NUM];       /* FRAT特性生效时，驻留PLMN的列表，最少不能少于20个PLMN列表 */
}NAS_NVIM_ATT_FRAT_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucPagingResponseRetrySupportFlg;        /* 标识位，用于标示是否使能寻呼响应重复功能。1:使能 0:不使能 */
    VOS_UINT8                           ucPagingResponseRetryPeriod;            /* 寻呼响应重复周期长度,单位是秒*/
    VOS_UINT8                           ucPagingResoneseRetry2GInterval;        /* 2G下寻呼响应重复的间隔时间,单位是秒 */
    VOS_UINT8                           ucPagingResoneseRetry3GInterval;        /* 3G下寻呼响应重复的间隔时间,单位是秒 */

}NAS_MMC_PAGING_RESPONSE_RETRY_CFG_STRU;

typedef struct
{
    VOS_UINT8                           ucTiNonregularCustom;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
    VOS_UINT8                           ucReserve7;
}NAS_NVIM_NONREGULAR_SERVICE_CUSTOM_STRU;

typedef struct
{
    VOS_UINT8                           ucHoldRetrieveRejOptimize;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
}NAS_NVIM_HOLD_RETRIEVE_REJ_OPTIMIZE_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableFlag;                           /* VOS_TRUE:使能,VOS_FALSE:去使能 */
    VOS_UINT8                           aucReserved1;
    VOS_UINT8                           aucReserved2;
    VOS_UINT8                           aucReserved3;
}NAS_NVIM_CC_STATUS_ENQUIRY_CFG_STRU;


enum CNAS_NVIM_HRPD_SESSION_STATUS_ENUM
{
    CNAS_NVIM_HRPD_SESSION_STATUS_CLOSE,
    CNAS_NVIM_HRPD_SESSION_STATUS_OPEN,
    CNAS_NVIM_HRPD_SESSION_STATUS_BUTT
};
typedef VOS_UINT8 CNAS_NVIM_HRPD_SESSION_STATUS_ENUM_UINT8;


enum CNAS_NVIM_HRPD_SESSION_TYPE_ENUM
{
    CNAS_NVIM_HRPD_SESSION_TYPE_HRPD,
    CNAS_NVIM_HRPD_SESSION_TYPE_EHRPD,
    CNAS_NVIM_HRPD_SESSION_TYPE_BUTT
};
typedef VOS_UINT8 CNAS_NVIM_HRPD_SESSION_TYPE_ENUM_UINT8;


enum CNAS_NVIM_HARDWARE_ID_TYPE_ENUM
{
    CNAS_NVIM_HARDWARE_ID_TYPE_MEID                    = 0x0000FFFF,
    CNAS_NVIM_HARDWARE_ID_TYPE_ESN                     = 0x00010000,
    CNAS_NVIM_HARDWARE_ID_TYPE_NULL                    = 0x00FFFFFF,
    CNAS_NVIM_HARDWARE_ID_TYPE_BUTT
};
typedef VOS_UINT32 CNAS_NVIM_HARDWARE_ID_TYPE_ENUM_UINT32;


enum CNAS_NVIM_HARDWARE_ID_SRC_TYPE_ENUM
{
    CNAS_NVIM_HARDWARE_ID_SRC_TYPE_NVIM,
    CNAS_NVIM_HARDWARE_ID_SRC_TYPE_RAND,
    CNAS_NVIM_HARDWARE_ID_SRC_TYPE_UIM,
    CNAS_NVIM_HARDWARE_ID_SRC_TYPE_BUTT
};
typedef VOS_UINT32 CNAS_NVIM_HARDWARE_ID_SRC_TYPE_ENUM_UINT32;



typedef struct
{
    VOS_UINT8                           aucCurUATI[CNAS_NVIM_UATI_OCTET_LENGTH];
    VOS_UINT8                           ucUATIColorCode;
    VOS_UINT8                           ucUATISubnetMask;
    VOS_UINT8                           ucUATIAssignMsgSeq;
    VOS_UINT8                           ucRsv2;
}CNAS_NVIM_HRPD_UATI_INFO_STRU;


typedef struct
{
    CNAS_NVIM_HARDWARE_ID_TYPE_ENUM_UINT32                  enHwidType;
    CNAS_NVIM_HARDWARE_ID_SRC_TYPE_ENUM_UINT32              enHwidSrcType;
    VOS_UINT32                                              ulEsn;                                /* 32-bit */
    VOS_UINT8                                               aucMeId[CNAS_NVIM_MEID_OCTET_NUM];    /* 56-bit */
    VOS_UINT8                                               ucRsv1;
}CNAS_NVIM_HARDWARE_ID_INFO_STRU;


typedef struct
{
    VOS_INT32                           lLongitude;
    VOS_INT32                           lLatitude;
}CNAS_NVIM_LOC_INFO_STRU;


typedef struct
{
    CNAS_NVIM_HRPD_SESSION_STATUS_ENUM_UINT8                enSessionStatus;
    CNAS_NVIM_HRPD_SESSION_TYPE_ENUM_UINT8                  enSessionType;
    VOS_UINT8                                               aucIccid[CNAS_NVIM_ICCID_OCTET_LEN];
    CNAS_NVIM_HRPD_UATI_INFO_STRU                           stUatiInfo;
    CNAS_NVIM_LOC_INFO_STRU                                 stLocInfo;
    CNAS_NVIM_HARDWARE_ID_INFO_STRU                         stHwid;
}CNAS_NVIM_HRPD_SESSION_INFO_STRU;


typedef struct
{
    PS_BOOL_ENUM_UINT8                  enSuppOnlyDo0;                          /* 是否只支持DO0版本 */
    PS_BOOL_ENUM_UINT8                  enSuppDoaWithMfpa;                      /* 是否支持DOA版本，应用类型只支持MFPA */
    PS_BOOL_ENUM_UINT8                  enSuppDoaWithEmfpa;                     /* 是否支持DOA版本，应用类型支持MFPA和EMPA */
    PS_BOOL_ENUM_UINT8                  enSuppDoaEhrpd;                         /* 是否支持eHRPD */
}CNAS_NVIM_HRPD_UE_REV_INFO_STRU;


typedef struct
{
    VOS_UINT32                          ulSessionSeed;
    CNAS_NVIM_HRPD_UE_REV_INFO_STRU     stUERevInfo;
    VOS_UINT8                           ucLteRegSuccFlg;
    VOS_UINT8                           aucReserve1[3];
    VOS_UINT32                          ulPseudorandomNumber;  /* 用于生成sessionseed的伪随机数 */
    VOS_UINT8                           aucReserve2[4];
}CNAS_NVIM_HRPD_SESSION_INFO_EX_STRU;

/*****************************************************************************
结构名    : NAS_NVIM_TIME_INFO_REPORT_OPTIMIZE_CFG_STRU
结构说明  : en_NV_Item_Time_Info_Report_Cfg(2434) 时间优化上报策略控制
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucRptOptType;                           /* 上报策略控制 0:不控制，1:^TIME时区、夏令时变化上报， */
                                                                                /*  2:保留，同0，3:在1的基础上增加灭屏上报间隔时间控制 */
    VOS_UINT8                           ucInterval;                             /* RptOptType为3时，灭屏上报间隔时间，单位min，取值0-255 */
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_TIME_INFO_REPORT_OPTIMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucAccessAuthAvailFlag;
    VOS_UINT8                           ucAccessAuthUserNameLen;
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           aucAccessAuthUserName[CNAS_NVIM_MAX_AUTHDATA_USERNAME_LEN];
}CNAS_NVIM_HRPD_ACCESS_AUTH_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucUseImsiFlg;                           /* VDF网络锁网状态下发起紧急呼携带IMSI开关 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_EMC_UNDER_NET_PIN_CFG_STRU;


typedef struct
{
    VOS_UINT8                 ucPsRegFailMaxTimesTrigLauOnceFlg;
    VOS_UINT8                 ucReserved1;
    VOS_UINT8                 ucReserved2;
    VOS_UINT8                 ucReserved3;
}NAS_NVIM_PS_REG_FAIL_MAX_TIMES_TRIG_LAU_ONCE_CFG_STRU;


typedef struct
{
    VOS_UINT8                 ucKeepSrchHplmnEvenRejByCause13Flg;
    VOS_UINT8                 ucReserved1;
    VOS_UINT8                 ucReserved2;
    VOS_UINT8                 ucReserved3;
}NAS_NVIM_KEEP_SRCH_HPLMN_EVEN_REJ_BY_CAUSE_13_CFG_STRU;


typedef struct
{
    VOS_UINT8                 ucEpsRejByCause14InVplmnAllowPsRegFlg;
    VOS_UINT8                 ucReserved1;
    VOS_UINT8                 ucReserved2;
    VOS_UINT8                 ucReserved3;
}NAS_NVIM_EPS_REJ_BY_CAUSE_14_IN_VPLMN_ALLOW_PS_REG_CFG_STRU;


enum NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_FLAG_ENUM
{
    NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_OFF                     = 0,           /* 关闭此优化 */
    NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_ON_FOR_ROAM             = 1,           /* 优化打开，但是仅在漫游时生效 */
    NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_ON_FOR_ALL              = 2,           /* 优化打开，漫游与非漫游都生效 */
    NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_BUTT
};
typedef VOS_UINT8 NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_FLAG_ENUM_UINT8;


typedef struct
{
    NAS_MMC_NVIM_CARRY_EPLMN_SWITCH_FLAG_ENUM_UINT8         enSwitchFlag;
    VOS_UINT8                                               ucCarryEplmnSceneSwitchOn;
    VOS_UINT8                                               ucCarryEplmnSceneAreaLost;
    VOS_UINT8                                               ucCarryEplmnSceneAvailableTimerExpired;
    VOS_UINT8                                               ucCarryEplmnSceneSysCfgSet;
    VOS_UINT8                                               ucCarryEplmnSceneDisableLte;
    VOS_UINT8                                               ucCarryEplmnSceneEnableLte;
    VOS_UINT8                                               ucCarryEplmnSceneCSFBServiceRej;
    VOS_UINT8                                               ucReserved1;
    VOS_UINT8                                               ucReserved2;
    VOS_UINT8                                               ucReserved3;
    VOS_UINT8                                               ucReserved4;
}NAS_NVIM_CARRY_EPLMN_WHEN_SRCH_RPLMN_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucActiveFlg;                                                /* 定制是否打开 */
    VOS_UINT8                           ucRetryTimerLenOnLte;                                       /* LTE下PDP retry时长，单位：s */
    VOS_UINT8                           ucRetryTimerLenOnGu;                                        /* GU下PDP retry时长，单位：s */
    VOS_UINT8                           ucLimitTimerLenAfterMaxCnt;                                 /* PDP激活被拒达到最大次数，限制PDP激活的时长，单位：min */
    VOS_UINT8                           ucMaxRetryCnt;                                              /* 最大retry次数 */
    VOS_UINT8                           ucReserve;
    VOS_UINT8                           ucPlmnNum;
    VOS_UINT8                           ucCauseNum;
    NAS_SIM_FORMAT_PLMN_ID              astPlmnIdList[NAS_NVIM_MAX_LIMIT_PDP_ACT_PLMN_NUM];         /* 运营商列表 */
    VOS_UINT8                           aucCauseList[NAS_NVIM_MAX_LIMIT_PDP_ACT_CAUSE_NUM];         /* 原因值列表 */
}NAS_NVIM_TELCEL_PDP_ACT_LIMIT_CFG_STRU;

typedef struct
{
    VOS_UINT8                 ucSwitchOnBorderPlmnSearchFlg;                 /* 开机边境搜网优化开关 VOS_TRUE:开启 VOS_FALSE:关闭 */
    VOS_UINT8                 ucBorderBgSearchFlg;                              /* 边境场景背景搜优化开关 VOS_TRUE:开启 VOS_FALSE:关闭 */
    VOS_UINT8                 ucReserved1;
    VOS_UINT8                 ucReserved2;
}NAS_NVIM_BORDER_PLMN_SEARCH_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucHomeSidNidDependOnPrlFlg;
    VOS_UINT8                           aucReserved[3];
}CNAS_NVIM_HOME_SID_NID_DEPEND_ON_PRL_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usStartSid;
    VOS_UINT16                          usEndSid;
    VOS_UINT32                          ulMcc;
}CNAS_NVIM_SYS_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucEnable;                          /* 白名单是否使能 */
    VOS_UINT8                           ucReserved;
    VOS_UINT16                          usWhiteSysNum;                     /* 支持白名单的个数,个数为0时表示不支持白名单 */
    CNAS_NVIM_SYS_INFO_STRU             astSysInfo[CNAS_NVIM_MAX_WHITE_LOCK_SYS_NUM];
}CNAS_NVIM_OPER_LOCK_SYS_WHITE_LIST_STRU;


typedef struct
{
    VOS_UINT16                          usChannel;
    VOS_UINT8                           aucReserved[2];
}CNAS_NVIM_FREQENCY_CHANNEL_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableFlg;
    VOS_UINT8                           ucReserved;
    VOS_UINT16                          usFreqNum;
    CNAS_NVIM_FREQENCY_CHANNEL_STRU     astFreqList[CNAS_NVIM_MAX_HRPD_CUSTOMIZE_FREQ_NUM];
}CNAS_NVIM_CTCC_CUSTOMIZE_FREQ_LIST_STRU;


typedef struct
{
    VOS_UINT16                          usPrimaryA;
    VOS_UINT16                          usPrimaryB;
    VOS_UINT16                          usSecondaryA;
    VOS_UINT16                          usSecondaryB;
}CNAS_NVIM_CDMA_STANDARD_CHANNELS_STRU;


typedef struct
{
    VOS_UINT32                                              ulEnableFlag;
}CNAS_NVIM_NO_CARD_MODE_CFG_STRU;


typedef struct
{
    VOS_UINT8                                               aucRedialTimes[CNAS_NVIM_1X_MAX_MRU_SYS_NUM];  /* 紧急呼叫失败时当前驻留的频点在mru list的重拨次数，超过次数起搜网*/
}CNAS_NVIM_1X_EMC_REDIAL_SYS_ACQ_CFG_STRU;


typedef struct
{
    VOS_UINT8                           aucAvoidTimerLen[CNAS_NVIM_1X_AVOID_MAX_PHASE_NUM];
}CNAS_NVIM_1X_AVOID_PHASE_NUM;


typedef struct
{
    CNAS_NVIM_1X_AVOID_PHASE_NUM        astAvoidPhaseNum[CNAS_NVIM_1X_AVOID_REASON_MAX];
}CNAS_NVIM_1X_AVOID_SCHEDULE_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucPowerOffCampOnCtrlFlg;
    VOS_UINT8                                               aucReserved[3];
}CNAS_NVIM_1X_POWER_OFF_CAMP_ON_CTRL_STRU;


typedef struct
{
    VOS_UINT8                                               ucIsConsiderRoamIndInPRLFlg;        /* 是否在PRL表最优系统判断中考虑ROAM IND属性的标志 */
    VOS_UINT8                                               ucReserved1;
    VOS_UINT8                                               ucReserved2;
    VOS_UINT8                                               ucReserved3;
} CNAS_NVIM_1X_PRL_ROAM_IND_STRATEGY_CFG_STRU;


typedef struct
{
    VOS_UINT32                                              ul1xSysAcqNoRfProtectTimerLen;
    VOS_UINT32                                              ul1xRedirNoRfProtectTimerLen;
    VOS_UINT32                                              ulHrpdSysAcqNoRfProtectTimerLen;
    VOS_UINT8                                               aucReserved[4];
}CNAS_NVIM_1X_DO_SYS_ACQ_NO_RF_PROTECT_TIMER_CFG_STRU;


typedef struct
{
    VOS_UINT8                                               ucLteDoConnInterrupt1xSysAcqFlg;
    VOS_UINT8                                               ucReserved1;
    VOS_UINT8                                               ucReserved2;
    VOS_UINT8                                               ucReserved3;
}CNAS_NVIM_LTE_DO_CONN_INTERUPT_1X_SYS_ACQ_CFG_STRU;


typedef struct
{
    VOS_UINT32                                              ul1xSysAcqDelayTimerLen;
    VOS_UINT8                                               uc1xSysAcqSyncDelayEnable;
    VOS_UINT8                                               auc1xSysAcqSyncDelayFreqNum[3];             //  仅使用第一个字节表示延迟同步每轮sync的频点个数
}CNAS_NVIM_LTE_OR_DO_CONN_1X_SYS_ACQ_SYNC_DELAY_INFO_STRU;


typedef struct
{
    VOS_UINT8                          ucImsiListNum;                                                  /*功能起效的SIM卡数目(LTE OOS后先搜2G再搜3G)  */
    VOS_UINT8                          ucReserved1;
    VOS_UINT8                          ucReserved2;
    VOS_UINT8                          ucReserved3;
    NAS_SIM_FORMAT_PLMN_ID             astImsiList[NAS_NVIM_LTE_OOS_2G_PREF_PLMN_SEL_MAX_IMSI_LIST_NUM];/* SIM卡列表 (LTE OOS后先搜2G再搜3G) */
}NAS_MMC_NVIM_LTE_OOS_2G_PREF_PLMN_SEL_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usChannel;
    VOS_UINT16                          usBandClass;
} CNAS_NVIM_CUSTOM_FREQUENCY_CHANNEL_STRU;


typedef struct
{
    VOS_UINT8                               ucEnableFlg;
    VOS_UINT8                               ucReserved;
    VOS_UINT16                              usFreqNum;
    CNAS_NVIM_CUSTOM_FREQUENCY_CHANNEL_STRU astFreqList[CNAS_NVIM_MAX_CDMA_1X_CUSTOM_PREF_CHANNELS_NUM];
} CNAS_NVIM_CDMA_1X_CUSTOM_PREF_CHANNELS_STRU;


typedef struct
{
    VOS_UINT8                               ucEnableFlg;
    VOS_UINT8                               ucReserved;
    VOS_UINT16                              usFreqNum;
    CNAS_NVIM_CUSTOM_FREQUENCY_CHANNEL_STRU astFreqList[CNAS_NVIM_MAX_CDMA_1X_CUSTOMIZE_PREF_CHANNELS_NUM];
} CNAS_NVIM_CDMA_1X_CUSTOMIZE_PREF_CHANNELS_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableDynloadTW;
    VOS_UINT8                           ucReseverd1;
    VOS_UINT8                           ucReseverd2;
    VOS_UINT8                           ucReseverd3;
}NAS_NVIM_DYNLOAD_CTRL_STRU;


typedef struct
{
    VOS_UINT8                           ucTWMaxAttemptCount;
    VOS_UINT8                           ucReseverd1;
    VOS_UINT8                           ucReseverd2;
    VOS_UINT8                           ucReseverd3;
}NAS_NVIM_DYNLOAD_EXCEPTION_CTRL_STRU;


typedef struct
{
    VOS_UINT8                           ucNoDataSrvRspSo33;                     /* 没有数据业务时，Paging rsp的回复。0 - Paging Rsp中SO带0； 1 - Paging Rsp中SO带33 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}CNAS_NVIM_1X_PAGING_RSP_SO_CFG_STRU;




typedef struct
{
    VOS_UINT8                           ucIsStartT310AccordWith3GPP;            /* 当proceeding或者progress中，携带progress indicator值为#1，#2，#64时，
                                                                                   是否启动T310, 0 - 不按协议做;  1 -  按照协议做 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_PROGRESS_INDICATOR_START_T310_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucPppDeactTimerLen;                     /* 单位(S), PPP去激活流程时间 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_CDATA_DISCING_PARA_INFO_STRU;

typedef struct
{
    VOS_UINT32                          ulIsPppAuthGetFromCard;
}NAS_NVIM_PPP_AUTH_INFO_FROM_CARD_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableFlag;                           /* VOS_TRUE:Auto attach功能使能；VOS_FALSE:Auto attach功能关闭 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_EHRPD_AUTO_ATTACH_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucEccSrvCap;        /* ECC服务能力: TRUE -- 支持ECC服务，FALSE -- 不支持ECC服务 */
    VOS_UINT8                           ucEccSrvStatus;     /* ECC服务状态: TRUE -- ECC服务打开，FALSE -- ECC服务关闭 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
} CNAS_NVIM_1X_CALL_ENCVOICE_ECC_SRV_CAP_INFO_STRU;



typedef struct
{
    VOS_UINT16                          usTotalTimeLen;         /* 该阶段整体搜索时长,单位为秒 */
    VOS_UINT16                          usSleepTimeLen;         /* 该阶段available定时器的启动时长,单位为秒 */
    VOS_UINT16                          usReserve1;
    VOS_UINT16                          usReserve2;
    VOS_UINT8                           ucLteHistorySrchNum;       /* 该阶段LTE历史频点搜索总次数 */
    VOS_UINT8                           ucLteFullBandSrchNum;      /* 该阶段LTE全频段搜索总次数 */
    VOS_UINT8                           ucLtePrefBandSrchNum;      /* 该阶段LTE pref band搜总次数 */
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
} NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_PATTERN_CFG_STRU;


typedef struct
{
    VOS_UINT8                           uc1xBsrLteActiveFlg;                /* 数据切换切换到1X上BSR LTE的激活标记--CCF用例要求 */
    VOS_UINT8                           uc1xBsrLteTimerLen;                 /* 数据切换切换到1X上BSR LTE的定时器时长-单位秒 */
    VOS_UINT8                           ucSrlte1xBsrLteEnableFlg;           /* SRLTE下1x数据业务态BSR LTE使能标志 */
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
    VOS_UINT8                           ucReserve7;
    VOS_UINT8                           ucReserve8;
} NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_CTRL_STRU;



typedef struct
{
    NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_CTRL_STRU         stCtrlInfo;
    NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_PATTERN_CFG_STRU  stPhaseOnePatternCfg;   /* 首阶段配置信息 */
    NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_PATTERN_CFG_STRU  stPhaseTwoPatternCfg;   /* 二阶段配置信息 */
    NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_PATTERN_CFG_STRU  stPhaseThreePatternCfg; /* 三阶段配置信息:目前未使用,预留 */
} NAS_NVIM_1X_SERVICE_CL_SYSTEM_ACQUIRE_STRATEGY_CFG_STRU;

typedef struct
{
    VOS_UINT8                           ucLteOos1xActDelayTimerLen;   /* VOLTE,lte oos后到1x激活间的延迟定时器,单位(s) */

    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
    VOS_UINT8                           ucReserve7;
} NAS_NVIM_LTE_OOS_DELAY_ACTIVATE_1X_TIMER_INFO_STRU;



typedef struct
{
    VOS_UINT8                           ucClImsSupportFlag;             /* CL模式下，IMS能力支持开关 */

    VOS_UINT8                           ucClEnhanceVolteFlag;           /* cdma normal volte flag, effective only when ucClImsSupportFlag is true
                                                                                  0: normal volte , 电信volte与srlte切换通过ap下发LTEIMSSWITHC来控制;
                                                                                  1: enhance volte,  电信volte与srlte自动切换  */

    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT32                          ulDisableLteTimerLen;           /* 惩罚定时器时长 单位:秒 */
    VOS_UINT8                           ucPingPongCtrlTimerLen;           /* 乒乓切换限定时长 单位:分钟 */
    VOS_UINT8                           ucMaxPingPongNum;                 /* 限定时间内的最大切换次数 */
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
} NAS_NVIM_CL_VOLTE_CFG_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucMru0SearchTimerLen;
    VOS_UINT8                           ucPhaseNum;
    VOS_UINT8                           ucHrpdMru0TimerMaxExpiredTimes; /* 低功耗模式下，HRPD MRU0搜网定时器超时次数，达到此数值后才发起搜网 */
    VOS_UINT8                           ucReserved;
    CNAS_NVIM_OOC_TIMER_INFO_STRU       astOocTimerInfo[CNAS_NVIM_MAX_HRPD_OOC_SCHEDULE_PHASE_NUM];
}CNAS_NVIM_HRPD_OOC_TIMER_SCHEDULE_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucReAcqLteWithNoRfEnable;           /* lte no rf时重新捕获lte是否使能 */
    NAS_NVIM_CL_SYS_ACQ_DSDS_STRATEGY_SCENE_ENUM_UINT8      enReAcqLteWithNoRfScene;            /* lte no rf时重新捕获lte的搜索场景 */
    VOS_UINT8                                               ucReAcqLteWithNoRfDelayTime;        /* lte no rf时重新捕获lte的延迟时间-单位秒 */
    VOS_UINT8                                               ucRsv1;
    VOS_UINT16                                              usRsv1;
    VOS_UINT16                                              usRsv2;
    VOS_UINT8                                               ucRsv2;
    VOS_UINT8                                               ucRsv3;
    VOS_UINT8                                               ucRsv4;
    VOS_UINT8                                               ucRsv5;
    VOS_UINT8                                               ucRsv6;
    VOS_UINT8                                               ucRsv7;
    VOS_UINT8                                               ucRsv8;
    VOS_UINT8                                               ucRsv9;
}NAS_NVIM_CL_SYSTEM_ACQUIRE_DSDS_STRATEGY_CFG_STRU;



typedef struct
{
    VOS_UINT8                                               ucPhaseNum;                 /* 总阶段个数 */
    NAS_NVIM_CHAN_REPEAT_SCAN_ENUM_UINT8                    enChanRepeatScanStrategy;   /* 频点重复搜索策略 */
    VOS_UINT16                                              usReserved;
    VOS_UINT8                                               uc1xOocDoTchPhase1TimerLen; /* Do TCH，前4次尝试 Ooc Timer 最短时长,单位秒 */
    VOS_UINT8                                               uc1xOocDoTchPhase2TimerLen; /* Do TCH，4次以上尝试 Ooc Timer 最短时长,单位秒 */
    VOS_UINT16                                              usRsv1;
    VOS_UINT8                                               uc1xMru0TimerMaxExpiredTimes;   /* 低功耗模式下，MRU0定时器超时最大次数，达到此数值后发起MRU0搜网 */
    VOS_UINT8                                               ucRsv2;
    VOS_UINT8                                               ucRsv3;
    VOS_UINT8                                               ucRsv4;
}CNAS_NVIM_1X_OOS_SYS_ACQ_STRATEGY_CTRL_STRU;


typedef struct
{
    VOS_UINT16                          usTotalTimeLen;         /* 1x一个阶段搜索总时长,单位为秒 */
    VOS_UINT16                          usSleepTimeLen;         /* 1x搜索一轮间隔时长,单位为秒 */
    VOS_UINT8                           ucSrchNum;              /* 1x一个阶段搜索几轮 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT16                          usMru0SearchTimerLen;       /* 搜索MRU0时available定时器的时长,每个阶段的时长,单位为秒 */

    VOS_UINT16                          usModem0MinSleepTimerLen;     /* 1x在主卡上，当前阶段最短SleepTimeLen,如果usSleepTimeLen小于该值，取ucMinSleepTimerLen,用于对不同产品的定制需求 */

    VOS_UINT8                           ucModem1MinSleepTimerLen; /* 1x在副卡上，当前阶段最短SleepTimeLen,如果usSleepTimeLen小于该值，取ucMinSleepTimerLen,用于对不同产品的定制需求 */
    VOS_UINT8                           ucModem2MinSleepTimerLen;  /* 1x在天际通上，当前阶段最短SleepTimeLen,如果usSleepTimeLen小于该值，取ucMinSleepTimerLen,用于对不同产品的定制需求 */
    VOS_UINT8                           ucRsv6;
    VOS_UINT8                           ucRsv7;
}CNAS_NVIM_1X_OOS_SYS_ACQ_STRATEGY_PATTERN_STRU;


typedef struct
{
    CNAS_NVIM_1X_OOS_SYS_ACQ_STRATEGY_CTRL_STRU             stCtrlInfo;
    CNAS_NVIM_1X_OOS_SYS_ACQ_STRATEGY_PATTERN_STRU          astSysAcqPhaseInfo[CNAS_NVIM_MAX_1X_OOC_SCHEDULE_PHASE_NUM];
}CNAS_NVIM_1X_OOS_SYS_ACQ_STRATEGY_CFG_STRU;


typedef struct
{
    VOS_UINT8                           uc1xOosReportEnable;
    VOS_UINT8                           uc1xMtServiceExceptionReportEnable;
    VOS_UINT8                           ucHrpdUatiFailReportEnable;
    VOS_UINT8                           ucHrpdSessionFailReportEnable;
    VOS_UINT8                           ucHrpdSessionExceptionDeactReportEnable;
    VOS_UINT8                           uctHrpdOrLteOosReportEnable;
    VOS_UINT8                           uctXregResltReportEnable;
    VOS_UINT8                           ucXsmsReportEnable;
    VOS_UINT8                           ucVolteIms1xSwitchReportEnable;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;

    VOS_UINT8                           ucReserve7;
    VOS_UINT8                           ucReserve8;
    VOS_UINT8                           ucReserve9;
    VOS_UINT8                           ucReserve10;
    VOS_UINT8                           ucReserve11;
    VOS_UINT8                           ucReserve12;
    VOS_UINT8                           ucReserve13;
    VOS_UINT8                           ucReserve14;
    VOS_UINT8                           ucReserve15;
}NAS_NVIM_CDMA_ERR_LOG_ACTIVE_REPORT_CONTRL_STRU;


typedef struct
{
    VOS_UINT32                          ulMccNum;                               /* 保存国内MCC的个数 */
    VOS_UINT32                          aulMcc[CNAS_NVIM_HOME_MCC_MAX_NUM];     /* 保存国内MCC的值 */
} CNAS_NVIM_HOME_MCC_INFO_STRU;

typedef struct
{
    VOS_UINT8                           ucEnable;
    VOS_UINT8                           ucReserved;
    VOS_UINT16                          usMru0Times;
}CNAS_NVIM_MRU0_SWITCH_ON_OOC_STRATEGY_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucMaxHistorySrchTimes;
    VOS_UINT8                           ucMaxPrefBandSrchTimes;
    VOS_UINT8                           ucBsrTimerMaxExpiredTimes; /* 低功耗模式下，BSR定时器超时次数，达到此数值后发起搜网 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT16                          usBsrTimerLenWithNoMatchedMsplRec;  /* MSPL表中没有匹配的记录，BSR时长 */
    VOS_UINT16                          usBsrTimerLen;                      /* MSPL表中有匹配的记录，BSR时长 */
}NAS_NVIM_SYS_ACQ_BSR_TIMER_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucBsrTimerActivateFlag;
    VOS_UINT8                                               ucBsrPhaseNum;
    VOS_UINT8                                               ucHrpdConnBsrActiveFlg; /* HRPD下存在连接时BSR是否打开标记。VOS_TRUE:打开;VOS_FALSE:关闭 */
    VOS_UINT8                                               ucEhrpdConnBsrActiveFlg;/* EHRPD下存在连接时BSR是否打开标记。VOS_TRUE:打开;VOS_FALSE:关闭 */
    NAS_NVIM_SYS_ACQ_BSR_TIMER_INFO_STRU                    astBsrTimerInfo[NAS_NVIM_MAX_BSR_PHASE_NUM];
}NAS_NVIM_SYS_ACQ_BSR_CTRL_STRU;

typedef struct
{
    VOS_UINT8                           ucImsiPlmnListNum;              /* 定制的IMSI列表个数 */
    VOS_UINT8                           ucOPlmnListNum;                 /* 用户配置的Oplmn的个数 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    NAS_SIM_FORMAT_PLMN_ID              astImsiPlmnList[NAS_MMC_NVIM_MAX_USER_CFG_IMSI_PLMN_NUM];
    NAS_SIM_FORMAT_PLMN_ID              astOPlmnList[NAS_MMC_NVIM_MAX_CUSTOM_SUPPLEMENT_OPLMN_NUM];
}NAS_MMC_NVIM_CUSTOM_SUPPLEMENT_OPLMN_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucOosSrchSleepCnt;
    VOS_UINT8                           ucHighPrioSrchSleepCnt;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
}AI_MODEM_NAS_PLMN_SRCH_CFG_STRU;



typedef struct
{
    VOS_INT16                           sThresholdRsrq;                         /* 服务小区的RSRQ门限，AI MODE测量优化时服务小区RSRQ需高于该门限。取值范围：[-15, -1],单位1dB */
    VOS_INT16                           sThresholdRsrp;                         /* 服务小区的RSRP门限，AI MODE测量优化时服务小区RSRP需高于该门限。取值范围：[-115 , -1],单位1dB */
    VOS_UINT16                          usStillTimeThres;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}AI_MODEM_LTE_MEASURE_CFG_STRU;



typedef struct
{
    VOS_INT16                           sThresholdRssi;                         /* GSM的RSSI的范围[-115，0] */
    VOS_UINT16                          usMeasOptimizeTimerLen;                 /* GSM周期性降功耗检查定时器时长，单位:s */
    VOS_UINT16                          usStillTimeThres;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}AI_MODEM_GSM_MEASURE_CFG_STRU;



typedef struct
{
    VOS_INT16                           sThresholdRscp;                         /* WCDMA的RSCP的范围[-120，0] */
    VOS_INT16                           sThresholdEcn0;                         /* WCDMA的ECN0范围[-24,0] */
    VOS_UINT16                          usStillTimeThres;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}AI_MODEM_WCDMA_MEASURE_CFG_STRU;




typedef struct
{
    VOS_UINT8                           ucTimeStart;
    VOS_UINT8                           ucTimeEnd;
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
}AI_MODEM_TIME_CFG_STRU;




typedef struct
{
    VOS_UINT8                           ucActFlg;                               /*是否激活，0不激活，1激活 */
    VOS_UINT8                           ucApplyArea;                            /*适用区域,默认0表示国内区域 */
    VOS_UINT8                           ucSensorIccFlg;                         /*SensorHub状态通过icc通道更新标志,0不使用ICC，1使用ICC*/
    VOS_UINT8                           ucReserve2;

    AI_MODEM_TIME_CFG_STRU              stTimeCfg;
    AI_MODEM_NAS_PLMN_SRCH_CFG_STRU     stNasSrchCfg;
    AI_MODEM_LTE_MEASURE_CFG_STRU       stLteMeasureCfg;
    AI_MODEM_GSM_MEASURE_CFG_STRU       stGasMeasureCfg;
    AI_MODEM_WCDMA_MEASURE_CFG_STRU     stWcdmaMeasureCfg;


    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;

    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
    VOS_UINT8                           ucReserve7;
    VOS_UINT8                           ucReserve8;

    VOS_UINT8                           ucReserve9;
    VOS_UINT8                           ucReserve10;
    VOS_UINT8                           ucReserve11;
    VOS_UINT8                           ucReserve12;

    VOS_UINT8                           ucReserve13;
    VOS_UINT8                           ucReserve14;
}AI_MODEM_CFG_NVIM_STRU;




typedef struct
{
    VOS_UINT8                           ucSensorIccFlg;            /* ICC通道是否激活，0不激活，1激活 */
    VOS_UINT8                           ucReserve1;          /* 保留位 */
    VOS_UINT8                           ucReserve2;          /* 保留位 */
    VOS_UINT8                           ucReserve3;          /* 保留位 */

}OPEN_ICC_CFG_STRU;



typedef struct
{
    VOS_UINT8                           ucSarSensorFlg;            /* sar sensor hub是否激活，0不激活，1激活 */
    VOS_UINT8                           ucReserve1;          /* 保留位 */
    VOS_UINT8                           ucReserve2;          /* 保留位 */
    VOS_UINT8                           ucReserve3;          /* 保留位 */

}SAR_SENSOR_HUB_CFG_STRU;


typedef struct
{
    VOS_UINT8                               ucEnableFlg;
    VOS_UINT8                               ucReserved;
    VOS_UINT16                              usFreqNum;
    CNAS_NVIM_FREQENCY_CHANNEL_STRU         astFreqList[CNAS_NVIM_MAX_CDMA_HRPD_CUSTOMIZE_PREF_CHANNELS_NUM];
} CNAS_NVIM_CDMA_HRPD_CUSTOMIZE_PREF_CHANNELS_STRU;


typedef struct
{
    VOS_UINT16                                              usImsiFileTimerLen;        /* IMSI读取回复定时器时长 ，单位秒 */
    VOS_UINT16                                              usIccidFileTimerLen;       /* ICCID读取回复定时器时长，单位秒 */
    VOS_UINT16                                              usServiceFileTimerLen;     /* 服务文件读取回复定时器时长，单位秒 */
    VOS_UINT16                                              usReserved1;
    VOS_UINT16                                              usReserved2;
    VOS_UINT16                                              usReserved3;
    VOS_UINT16                                              usReserved4;
    VOS_UINT16                                              usReserved5;
    VOS_UINT16                                              usReserved6;
    VOS_UINT16                                              usReserved7;
}NAS_NVIM_READ_USIM_FILE_TIMER_CTRL_STRU;



typedef struct
{
    VOS_UINT16                          usTimes;
    VOS_UINT16                          usTimerLen;
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
    VOS_UINT8                           ucRsv6;
    VOS_UINT8                           ucRsv7;
    VOS_UINT8                           ucRsv8;
}NAS_NVIM_MODE_SELECTION_RETRY_TIMER_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucRetryPhaseNum;
    VOS_UINT8                                               aucRsved[3];
    NAS_NVIM_MODE_SELECTION_RETRY_TIMER_INFO_STRU           astRetryTimerInfo[NAS_NVIM_MODE_SELECTION_RETRY_TIMER_PHASE_NUM_MAX];
}NAS_NVIM_MODE_SELECTION_RETRY_TIMER_CTRL_STRU;


typedef struct
{
    VOS_UINT8                                               ucNoCardPowerSaveFlg;         /* 无卡情况下是否PowerSave所有模式 */
    VOS_UINT8                                               ucClAcqLteFlg;                /* CL模式下无卡情况下是否搜LTE */
    VOS_UINT8                                               ucReserve1;
    VOS_UINT8                                               ucReserve2;
}NAS_NVIM_NO_CARD_SYS_ACQ_CFG_STRU;



typedef struct
{
    VOS_UINT8                                               ucActiveFlg;

    /* 先搜的控制，默认允许VOS_TRUE */
    VOS_UINT8                                               ucIsAllowFirstSrch;

    /* 后搜的控制，默认不允许VOS_FALSE */
    VOS_UINT8                                               ucIsAllowLastSrch;

    /* CL模式只有边界国外，是否切GUL的控制，默认切GUL，VOS_TRUE*/
    VOS_UINT8                                               ucIsOnlyBoundarySwitchMode;

    /* Retry timer len从en_NV_Item_MODE_SELECTION_RETRY_SYS_ACQ_STRATEGY中的sleep timer中取 */
    NAS_NVIM_MODE_SELECTION_RETRY_TIMER_CTRL_STRU           stRsv;

    /* 模式切换后，是否需要立即通知卡切IMSI */
    VOS_UINT8                                               ucIsNeedFastSwitchImsi;
    VOS_UINT8                                               ucRsv2;
    VOS_UINT8                                               ucRsv3;
    VOS_UINT8                                               ucRsv4;
}NAS_NVIM_MODE_SELECTION_CFG_STRU;



typedef struct
{
    VOS_UINT32                          ulCount;            /* 乒乓次数 */
    VOS_UINT32                          ulTimerLen;         /* 上述乒乓次数范围内的惩罚时间 */
}NAS_NVIM_MODE_SELECTION_PUNISH_TIMER_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucActiveFlg;
    VOS_UINT8                                               ucRsv1;
    VOS_UINT8                                               ucRsv2;
    VOS_UINT8                                               ucPunishPhaseNum;
    NAS_NVIM_MODE_SELECTION_PUNISH_TIMER_INFO_STRU          astPunishTimerInfo[NAS_NVIM_MODE_SELECTION_PUNISH_TIMER_PHASE_NUM_MAX];
}NAS_NVIM_MODE_SELECTION_PUNISH_CTRL_INFO_STRU;


typedef struct
{
    VOS_UINT16                          usSleepTimerLen;
    VOS_UINT16                          usRsved1;
    VOS_UINT8                           ucHistoryNum;                           /* 第几次的历史搜 会变成 PrefBand/FullBand搜 */
    VOS_UINT8                           ucPrefBandNum;                          /* 第几次的PrefBand搜 会变成 FullBand搜 */
    VOS_UINT8                           ucFullBandNum;                          /* 第几次FullBand搜後, 此阶段结束, 进入下一阶段 */
    VOS_UINT8                           ucRsved1;                               /* 预留 */
    VOS_UINT8                           ucRsved2;
    VOS_UINT8                           ucRsved3;
    VOS_UINT8                           ucRsved4;
    VOS_UINT8                           ucRsved5;
}NAS_NVIM_MODE_SELECTION_RETRY_SYS_ACQ_PHASE_STRU;


typedef struct
{
    VOS_UINT8                                               ucActiveFlg;
    VOS_UINT8                                               ucPhaseNum;
    VOS_UINT8                                               ucRsved1;
    VOS_UINT8                                               ucRsved2;
    NAS_NVIM_MODE_SELECTION_RETRY_SYS_ACQ_PHASE_STRU        astSysAcqPhase[NAS_NVIM_MODE_SELECTION_RETRY_SYS_ACQ_PHASE_NUM_MAX];
}NAS_NVIM_MODE_SELECTION_RETRY_SYS_ACQ_STRATEGY_STRU;


typedef struct
{
    VOS_UINT8                           ucHistoryCfgSwitch;                                               /* History搜类型总控开关。0:History搜网类型不可配置。  1:History搜网类型可配置*/
    VOS_UINT8                           ucPrefBandCfgSwitch;                                              /* PrefBand搜类型总控开关。0:PrefBand搜网类型不可配置。1:PrefBand搜网类型可配置*/
    VOS_UINT8                           ucRsv1;                                                           /* RFU */
    VOS_UINT8                           ucRsv2;                                                           /* RFU */
    VOS_UINT8                           aucHistoryActiveFlg[NAS_NVIM_CUSTOMIZE_MAX_CL_ACQ_SCENE_NUM];      /* ucHistoryCfgSwitch为0时该数组无意义。
                                                                                                                        数组下表代表激活History搜类型的搜网场景，每个数组元素:
                                                                                                                        0表示该场景不激活History搜类型;
                                                                                                                        1表示该场景激活History搜类型。
                                                                                                                        数组下表对应的具体搜网场景见结构体NAS_MSCC_NVIM_SYS_ACQ_SCENE_ENUM_UINT32 */
    VOS_UINT8                           aucPrefBandActiveFlg[NAS_NVIM_CUSTOMIZE_MAX_CL_ACQ_SCENE_NUM];    /* ucPrefBandCfgSwitch为0时该数组无意义。
                                                                                                                          数组下表代表激活PrefBand搜类型的搜网场景，每个数组元素:
                                                                                                                          0表示该场景不激活PrefBand搜类型;
                                                                                                                          1表示该场景激活PrefBand搜类型。
                                                                                                                          数组下表对应的具体搜网场景见结构体NAS_MSCC_NVIM_SYS_ACQ_SCENE_ENUM_UINT32 */
    VOS_UINT8                           ucRsv3;                                                           /* RFU */
    VOS_UINT8                           ucRsv4;                                                           /* RFU */
    VOS_UINT8                           ucRsv5;                                                           /* RFU */
    VOS_UINT8                           ucRsv6;                                                           /* RFU */
}NAS_NVIM_CL_SYS_ACQ_TYPE_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT16                                              usOosModeSwitchSrchTimerLen;     /* MMC OOS模式切换搜索时长 */
    VOS_UINT16                                              usPowerOnModeSwitchSrchTimerLen; /* MMC 开机场景下，模式切换搜索时长 */
    VOS_UINT8                                               ucOosModeSwitchSrchTimes;        /* MMC OOS下，模式切换搜索次数 */
    VOS_UINT8                                               ucPowerOnModeSwitchSrchTimes;    /* MMC 开机场景下，模式切换搜索次数 */
    VOS_UINT8                                               aucReserved[2];
}NAS_MMC_NVIM_OOS_MODE_SWITCH_SRCH_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT16                                              usOosModeSwitchSrchTimerLen;     /* MMC OOS模式切换搜索时长 */
    VOS_UINT16                                              usPowerOnModeSwitchSrchTimerLen; /* MMC 开机场景下，模式切换搜索时长 */
    VOS_UINT8                                               ucOosModeSwitchSrchTimes;        /* MMC OOS下，模式切换搜索次数 */
    VOS_UINT8                                               ucPowerOnModeSwitchSrchTimes;    /* MMC 开机场景下，模式切换搜索次数 */
    VOS_UINT8                                               aucReserved[2];
}CNAS_XSD_NVIM_OOS_MODE_SWITCH_SRCH_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT8                                               ucIsPowerupRegAdv;              /* 是否将开机注册提前,VOS_TRUE为提前,VOS_FALSE为不提前 */
    VOS_UINT8                                               aucRsv[3];
}CNAS_NVIM_1X_REG_CFG_INFO_STRU;


typedef struct
{
    VOS_UINT8                               ucEnableFlg;
    VOS_UINT8                               ucReserved1;
    VOS_UINT8                               ucReserved2;
    VOS_UINT8                               ucReserved3;
} NAS_NVIM_SMS_FAIL_LINK_CTRL_CFG_STRU;


typedef struct
{
    VOS_UINT8                               ucEnableFlg;
    VOS_UINT8                               ucReserved1;
    VOS_UINT8                               ucReserved2;
    VOS_UINT8                               ucReserved3;
} NAS_NVIM_HIGH_SPEED_MODE_RPT_CFG_STRU;


typedef struct
{
    VOS_UINT16                         usSidRangeStart;                                      /*SID 范围起始值 */
    VOS_UINT16                         usSidRangeEnd;                                        /*SID 范围结束值 */
    VOS_UINT32                         ulMcc;                                                /*mobile country code */
}NAS_NVIM_SID_AND_MCC_INFO_STRU;


typedef struct
{
    VOS_UINT32                          ulNum;                                                /* 中国边界国家网络数目 */
    NAS_NVIM_SID_AND_MCC_INFO_STRU      astSidMccInfo[NAS_NVIM_CHINA_BOUNDARY_NETWORK_NUM_MAX];
}NAS_NVIM_CHINA_BOUNDARY_SID_AND_MCC_INFO_STRU;



typedef struct
{
    VOS_UINT8                                               ucActiveFlg;
    VOS_UINT8                                               ucHomeNetworkNum;               /* 中国home网络数目 */
    VOS_UINT8                                               ucRsved1;
    VOS_UINT8                                               ucRsved2;
    NAS_NVIM_SID_AND_MCC_INFO_STRU                          astSidRangeMccInfo[NAS_NVIM_CHINA_HOME_NETWORK_NUM_MAX];
}NAS_NVIM_CHINA_HOME_SID_AND_MCC_INFO_STRU;


typedef struct
{
    VOS_UINT8                                               ucReestEnableFlg;                                                               /* 是否支持被叫重建链 */
    VOS_UINT8                                               ucProtectTimerLen;                                                              /* 重试保护定时器时长 */
    VOS_UINT8                                               ucIntervalTimerLen;                                                             /* 重试间隔定时器时长 */
    VOS_UINT8                                               ucMtEstCnfReestCauseNum;                                                        /* est_cnf可重试的原因值个数 */
    VOS_UINT8                                               aucMtEstCnfReestCause[NAS_NVIM_1X_MT_EST_CNF_REEST_CAUSE_MAX_NUM];              /* est_cnf重试原因值 */
    VOS_UINT8                                               ucMtTerminateIndReestCauseNum;                                                  /* terminate_ind可重试的原因值个数 */
    VOS_UINT8                                               aucMtTerminateIndReestCause[NAS_NVIM_1X_MT_TERMINATE_IND_REEST_CAUSE_MAX_NUM];  /* terminate_ind重试原因值 */
}NAS_NVIM_1X_MT_NVIM_REEST_CFG_STRU;


typedef struct
{
    VOS_UINT8                               ucQuickDisplayEnableFlg;
    VOS_UINT8                               ucReserved1;
    VOS_UINT8                               ucReserved2;
    VOS_UINT8                               ucReserved3;
} NAS_NVIM_POWER_ON_QUICK_DISPLAY_NORMAL_SERVICE_OPTIMIZE_INFO_STRU;


typedef struct
{
    VOS_UINT8                           ucZeroPlmnInvalidFlag;                         /* TMSI或P_TMSI重分配的PLMN为0是否无效的标志 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_TMSI_OR_P_TMSI_REALLOC_PLMN_VALID_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucLogPrintMaxCnt;                       /* 最大LOG打印数量 */
    VOS_UINT8                           ucReserve1;
    VOS_UINT8                           ucReserve2;
    VOS_UINT8                           ucReserve3;
    VOS_UINT8                           ucReserve4;
    VOS_UINT8                           ucReserve5;
    VOS_UINT8                           ucReserve6;
    VOS_UINT8                           ucReserve7;
} NAS_NVIM_LOG_PRINT_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usGsmA5;
    VOS_UINT8                           aucReserve[2];
}NAS_NVIM_GSM_A5_STRU;

typedef struct
{
    VOS_UINT8                           ucPowerSaveEnableFlag;             /* 0:NV 未激活; 1:NV 激活 */
    VOS_UINT8                           ucScreenOnEnableFlag;              /* 0:亮屏时直接上报;1:亮屏时也要功率控制 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT32                          ulGutlOosRptTimerInterval;         /* GUTL模式下，相隔多久，单位是秒 */
    VOS_UINT32                          ulGutlOosCsUserSenseTimerInterval; /* 记录CS的掉网多长时间的算是长时间丢网，单位是秒 */
    VOS_UINT32                          ulGutlOosPsUserSenseTimerInterval; /* 记录PS的掉网多长时间的算是长时间丢网，单位是秒 */
    VOS_UINT32                          ul1xOosRptTimerInterval;           /* CL模式下，1x丢网上报时间间隔，单位:秒*/
    VOS_UINT32                          ulDoLteOosRptTimerInterval;        /* CL模式下，DO_LTE丢网上报时间间隔，单位:秒*/
    VOS_UINT32                          ul1xRegFailRptTimerInterval;       /* CL模式下，1x注册失败CHR上报时间间隔,单位:秒 */
    VOS_UINT32                          ulReserved4;                       /* 保留位 */
}NAS_NVIM_OOS_CHR_POWER_SAVE_CFG_STRU;


typedef struct
{
    VOS_UINT8                               ucNvActiveFlg;                             /* NV是否激活 */
    VOS_UINT8                               ucT310CHREnableFlg;                        /* 控制310定时器CHR上报 */
    VOS_UINT8                               ucT310Len;
    VOS_UINT8                               ucReserved1;
} NAS_CC_NVIM_T310_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucEmcCateSupportEcallFlag;
    VOS_UINT8                           ucBit8Is1OtherBitOKFlag;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
}NAS_NVIM_EMC_CATE_SUPPORT_ECALL_CFG_STRU;


typedef struct
{
    VOS_UINT8                               ucNvActiveFlg;                             /* NV是否激活 */
    VOS_UINT8                               ucProtectTimerLen;                         /* 保护定时器时长 */
    VOS_UINT8                               ucReserved1;
    VOS_UINT8                               ucReserved2;
} NAS_NVIM_MO_COLLISION_STRU;


typedef struct
{
    VOS_UINT8                           ucSrvccNoCallNumT3240Flag;              /* 0，默认值，按照原来逻辑，立即释放链接，不修改T3240定时器时长
                                                                                   1，不立即释放链接，修改T3240定时器时长为ulSrvccNoCallNumT3240Len ms */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;
    VOS_UINT32                          ulSrvccNoCallNumT3240Len;               /* 修改T3240定时器时长，单位ms，默认值是1000 ms */
}NAS_NVIM_SRVCC_NO_CALL_NUM_T3240_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucClamk2RptFlag;                        /* GU下RAU ATTACH是否带classmark 2 */
    VOS_UINT8                           ucClamk3RptFlag;                        /* GU下RAU ATTACH是否带classmark 3 */
    VOS_UINT8                           ucSuppCodecRptFlag;                     /* GU下RAU ATTACH是否带supported codecs */
    VOS_UINT8                           ucReserved;                             /* 保留位 */
}NAS_NVIM_CLASSMARK_SUPPCODEC_CAPRPT_CTRL_STRU;


typedef struct
{
    VOS_UINT8                           ucSsConnStateRcvRelIndSetCauseFlg;      /* SS连接状态收到REL IND是否要设置原因值标志:
                                                                                   0--不设置原因值；
                                                                                   1--设置原因值 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    VOS_UINT8                           ucReserved3;

}NAS_NVIM_SS_CONN_STATE_RCV_REL_IND_SET_CAUSE_STRU;



typedef struct
{
    VOS_UINT8                           ucMtS12DiscardSysInfoFlg;                  /* 收到paging发起est req后，如果收到系统消息，是否继续处理被叫 */
    VOS_UINT8                           ucChrEnableFlg;                            /* 这种场景下的收益CHR上报是否使能 */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
}NAS_NVIM_MT_MM_S12_RCV_SYSINFO_CFG_STRU;




/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/


/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
*****************************************************************************/










#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif





#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of NasNvInterface.h */
