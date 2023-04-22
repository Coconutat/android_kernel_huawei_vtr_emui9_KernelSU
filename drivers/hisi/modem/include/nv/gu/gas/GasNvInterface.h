

#ifndef __GASNVINTERFACE_H__
#define __GASNVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(2)
#else
#pragma pack(push, 2)
#endif

#include "vos.h"

/*****************************************************************************
  2 Macro
*****************************************************************************/
#define NVIM_ULTRA_CLASSMARK_LEN                            (24)
#define NVIM_BAND_PWR_LEN                                   (8)
#define NVIM_CLASSMARK1_LEN                                 (2)
#define NVIM_CLASSMARK2_LEN                                 (4)
#define NVIM_CLASSMARK3_LEN                                 (16)
#define NVIM_CLASSMARK3_R8_LEN                              (36)
#define NVIM_GCF_ITEM_LEN                                   (80)
#define NVIM_GSM_BA_MAX_SIZE                                (33)
#define NVIM_EGPRS_RA_CAPABILITY_DATA_LEN                   (53)
#define NVIM_PREFER_GSM_PLMN_LIST_LEN                       (976)
#define NVIM_GSM_DEC_FAIL_ARFCN_LIST_LEN                    (120)
#define NVIM_EUTRA_MAX_SUPPORT_BANDS_NUM                    (64)
#define NVIM_TDS_MAX_SUPPORT_BANDS_NUM                      (8)
#define NVIM_EUTRA_CAPA_COMM_INFO_SIZE                      (260)
#define NVIM_CBS_MID_LIST_LEN                               (2004)
#define NVIM_CBS_MID_RANGE_LIST_LEN                         (2004)
/* Added by yangsicong for L2G REDIR C1 CUSTUME, 2015-1-26, begin */
#define NVIM_GAS_C1_CALC_OPT_PLMN_WHITE_LIST_CNT_MAX        (20)
/* Added by yangsicong for L2G REDIR C1 CUSTUME, 2015-1-26, end */

#define NVIM_GAS_GSM_BAND_CUSTOMIZE_LIST_MAX_CNT            (80)

#define NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_PLMN_MAX_CNT        (10)
#define NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_RANGE_MAX_CNT       (8)
#define NVIM_GSM_OPERATE_CUSTOMIZE_DESCRETE_FREQ_MAX_CNT    (16)

#if defined( __PS_WIN32_RECUR__ ) || defined (DMT)
#define MAX_CHR_ALARM_ID_NUM (20)
#endif

/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/
enum NVIM_TDS_FREQ_BAND_LIST_ENUM
{
    ID_NVIM_TDS_FREQ_BAND_A         = 0x01,         /* 频点范围: 9504~9596  10054~10121 */
    ID_NVIM_TDS_FREQ_BAND_B         = 0x02,         /* 频点范围: 9254~9546  9654~9946 */
    ID_NVIM_TDS_FREQ_BAND_C         = 0x04,         /* 频点范围: 9554~9646 */
    ID_NVIM_TDS_FREQ_BAND_D         = 0x08,         /* 频点范围: 12854~13096 */
    ID_NVIM_TDS_FREQ_BAND_E         = 0x10,         /* 频点范围: 11504~11996 */
    ID_NVIM_TDS_FREQ_BAND_F         = 0x20,         /* 频点范围: 9404~9596 */
    ID_NVIM_TDS_FREQ_BAND_BUTT
};
typedef VOS_UINT8  NVIM_TDS_FREQ_BAND_LIST_ENUM_UINT8;


enum NVIM_BAND_IND_ENUM
{
    NVIM_BAND_IND_2                 = 0x2,          /* BAND2 */
    NVIM_BAND_IND_3                 = 0x3,          /* BAND3 */
    NVIM_BAND_IND_5                 = 0x5,          /* BAND5 */
    NVIM_BAND_IND_8                 = 0x8,          /* BAND8 */
    NVIM_BAND_IND_BUTT
};
typedef VOS_UINT8 NVIM_BAND_IND_ENUM_UINT8;


/*****************************************************************************
  5 STRUCT
*****************************************************************************/
/*****************************************************************************
*                                                                            *
*                           参数设置消息结构                                 *
*                                                                            *
******************************************************************************/

/*****************************************************************************
 结构名    : NVIM_ULTRA_CLASSMARK_STRU
 结构说明  : en_NV_Item_Ultra_Classmark 结构
 DESCRIPTION: 废弃项
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucUltraClassmark[NVIM_ULTRA_CLASSMARK_LEN];
}NVIM_ULTRA_CLASSMARK_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_MULTIRATE_FLAG_STRU
 结构说明  : en_NV_Item_Gas_MultiRateFlag 结构
 DESCRIPTION: 废弃项
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usMultiRateFlag;
    VOS_UINT8                           aucReserve[2];
}NVIM_GAS_MULTIRATE_FLAG_STRU;

/*****************************************************************************
 结构名    : NVIM_BAND_PWR_STRU
 结构说明  : en_NV_Item_Band_Pwr 结构
 DESCRIPTION: MS最大输出功率等级
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucBandPwr[NVIM_BAND_PWR_LEN];          /* 存储MS最大输出功率等级 */
}NVIM_BAND_PWR_STRU;

/*****************************************************************************
 结构名    : NVIM_VGCS_FLAG_STRU
 结构说明  : en_NV_Item_Vgcs_Flag 结构
 DESCRIPTION: MS是否支持语音组寻呼业务
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usVgcsFlag;                             /* 0x0: 不支持语音组寻呼业务
                                                                                   0x1: 支持语音组寻呼业务 */
    VOS_UINT8                           aucReserve[2];
}NVIM_VGCS_FLAG_STRU;

/*****************************************************************************
 结构名    : NVIM_EGPRS_MULTI_SLOT_CLASS_STRU
 结构说明  : en_NV_Item_Egprs_Multi_Slot_Class 结构
 DESCRIPTION:  标识MS的EGPRS多时隙能力等级
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usEgprsMultiSlotClass;                  /* Range: [0,12]
                                                                                   0x0:MS多时隙能力等级为0xC
                                                                                   0x1~0xC:指示MS多时隙能力等级 */
    VOS_UINT8                           aucReserve[2];
}NVIM_EGPRS_MULTI_SLOT_CLASS_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_CLASSMARK1_STRU
 结构说明  : en_NV_Item_Gsm_Classmark1 结构
 DESCRIPTION:  GSM Classmark1码流，仅供内部使用
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucGsmClassmark1[NVIM_CLASSMARK1_LEN];  /* CLASSMARK1 码流 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_CLASSMARK1_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_CLASSMARK2_STRU
 结构说明  : en_NV_Item_Gsm_Classmark2 结构
 DESCRIPTION:  GSM Classmark2码流，仅供内部使用
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucGsmClassmark2[NVIM_CLASSMARK2_LEN];  /* CLASSMARK2 码流 */
}NVIM_GSM_CLASSMARK2_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_CLASSMARK3_STRU
 结构说明  : en_NV_Item_Gsm_Classmark3 结构
 DESCRIPTION:  GSM Classmark3码流，仅供内部使用
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucGsmClassmark3[NVIM_CLASSMARK3_LEN];  /* CLASSMARK3 码流 */
}NVIM_GSM_CLASSMARK3_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_IND_FREQ_STRU
 结构说明  : en_NV_Item_Gsm_Ind_Freq 结构
 DESCRIPTION:  锁定GSM频点时，存储用户指定的频点。
               开机搜网时，先搜索用户指定的频点，主要是方便测试人员测试使用
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmIndFreq;                           /* 用户锁定频点，频点为0x0000~0xFFFE之一。
                                                                                   用户没有锁定频点，频点为0xFFFF */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_IND_FREQ_STRU;

/*****************************************************************************
 结构名    : NVIM_GCF_ITEM_STRU
 结构说明  : en_NV_Item_GCF_Item 结构
 DESCRIPTION:  GCF用例桩，内部使用。已废弃。
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucGcfItem[NVIM_GCF_ITEM_LEN];
}NVIM_GCF_ITEM_STRU;

/*****************************************************************************
 结构名    : NVIM_G2W_RSSI_RSCP_OFFSET_STRU
 结构说明  : en_NV_Item_G2W_RSSI_RSCP_OFFSET 结构
 DESCRIPTION:  已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usG2WRssiRscpOffset;
    VOS_UINT8                           aucReserve[2];
}NVIM_G2W_RSSI_RSCP_OFFSET_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_BA_COUNT_STRU
 结构说明  : en_NV_Item_GSM_Ba_Count 结构
 DESCRIPTION:  保存BA与服务小区的个数。每次驻留后，MS都会将当前服务小区与邻区的个数更新到该NV项中。
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmBaCount;                           /* 0x0000:保存的BA与服务小区个数和为0。
                                                                                   0x0001~0x0021:GSM BA LIST中保存的BA和服务小区个数和 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_BA_COUNT_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_BA_LIST_STRU
 结构说明  : en_NV_Item_GSM_Ba_List 结构
 DESCRIPTION:  保存服务小区所属的PLMN以及服务小区和邻区的频点信息。
               每次驻留后，都会将当前的服务小区所属的PLMN和当前服务小区及邻区的频点更新到该NV项中。
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulMcc;                                  /* 服务小区的移动国家码 */
    VOS_UINT32                          ulMnc;                                  /* 服务小区的移动网码 */
    VOS_UINT16                          usArfcn[NVIM_GSM_BA_MAX_SIZE];          /* 服务小区及其邻区的频点号，ausArfcn[0]保存服务小区的频点
                                                                                   ausArfcn[1]~ausArfcn[33]保存邻区的频点 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_BA_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_EGPRS_FLAG_STRU
 结构说明  : en_NV_Item_Egprs_Flag 结构
 DESCRIPTION:  MS是否支持EGPRS。该NV可以针对每个Modem单独配置
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usEgprsFlag;                            /* 0x0000:不支持EGPRS
                                                                                   0x0001:支持EGPRS */
    VOS_UINT8                           aucReserve[2];
}NVIM_EGPRS_FLAG_STRU;

/*****************************************************************************
 结构名    : NVIM_EGPRS_RA_CAPABILITY_STRU
 结构说明  : en_NV_Item_EgprsRaCapability 结构
 DESCRIPTION:  EGPRS下RA能力，已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucLength;                               /* 字节长度 */
    VOS_UINT8                           aucEgprsRaCapabilityData[NVIM_EGPRS_RA_CAPABILITY_DATA_LEN];    /* 能力码流 */
    VOS_UINT8                           aucReserve[2];
}NVIM_EGPRS_RA_CAPABILITY_STRU;

/*****************************************************************************
 结构名    : NVIM_PREFER_GSM_PLMN_COUNT_STRU
 结构说明  : en_NV_Item_Prefer_GSM_PLMN_Count 结构
 DESCRIPTION:  存储GSM优选PLMN的个数
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usPreferGsmPlmnCount;                   /* 优选PLMN的个数 */
    VOS_UINT8                           aucReserve[2];
}NVIM_PREFER_GSM_PLMN_COUNT_STRU;

/*****************************************************************************
 结构名    : NVIM_PREFER_GSM_PLMN_LIST_STRU
 结构说明  : en_NV_Item_Prefer_GSM_PLMN_List 结构
 DESCRIPTION:  存储GSM优选小区的信息。保存MCC和MNC及其该PLMN下的解码成功频点个数和具体频点号。
               最多可以保存15个PLMN，每个PLMN下最多可以保存60个频点。
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          ausPreferGsmPlmnListData[NVIM_PREFER_GSM_PLMN_LIST_LEN];    /* GSM优选小区的列表码流 */
}NVIM_PREFER_GSM_PLMN_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_DEC_FAIL_ARFCN_COUNT_STRU
 结构说明  : en_NV_Item_GSM_DEC_FAIL_ARFCN_Count 结构
 DESCRIPTION:  存储GSM解码失败的频点个数，已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmDecFailArfcnCount;                 /* 解码失败的频点个数 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_DEC_FAIL_ARFCN_COUNT_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_DEC_FAIL_ARFCN_LIST_STRU
 结构说明  : en_NV_Item_GSM_DEC_FAIL_ARFCN_List 结构
 DESCRIPTION:  存储GSM解码失败的频点列表，已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmDecFailArfcnList[NVIM_GSM_DEC_FAIL_ARFCN_LIST_LEN];    /* 解码失败的列表码流 */
}NVIM_GSM_DEC_FAIL_ARFCN_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_PREFER_GSM_PLMN_SWITCH_STRU
 结构说明  : en_NV_Item_Prefer_GSM_PLMN_Switch 结构
 DESCRIPTION:  优选小区的控制开关。优选小区打开时，GSM_PLMN_Count和GSM_PLMN_List两项NV起作用。
               该NV不可配置。
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usPreferGsmPlmnSwitch;                  /* 0x0000:开关关闭；0x0001；开关打开 */
    VOS_UINT8                           aucReserve[2];
}NVIM_PREFER_GSM_PLMN_SWITCH_STRU;


typedef struct
{
    VOS_UINT8                           ucAgingEnable;                          /* 优选小区老化的NV使能 */
    VOS_UINT8                           ucRsrv1;                                /* 保留位 */
    VOS_UINT8                           ucRsrv2;                                /* 保留位 */
    VOS_UINT8                           ucRsrv3;                                /* 保留位 */
    VOS_INT16                           sDecFailedRssiThreshold;                /* 从优选小区列表中删除解码失败的RSSI门限 */
    VOS_UINT16                          usRsrv1;                                /* 保留位 */
    VOS_UINT16                          usRsrv2;                                /* 保留位 */
    VOS_UINT16                          usRsrv3;                                /* 保留位 */
    VOS_UINT32                          ulRsrv1;                                /* 保留位 */
    VOS_UINT32                          ulRsrv2;                                /* 保留位 */
}NVIM_PREFER_GSM_PLMN_CUSTOMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GERAN_FEATURE_PACKAGE1_STRU
 结构说明  : en_NV_Item_Geran_Feature_Package1 结构
 DESCRIPTION: 保存MS是否支持Geran Feature Package1
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGeranFeaturePackage1;                 /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GERAN_FEATURE_PACKAGE1_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_A5_STRU
 结构说明  : en_NV_Item_Gsm_A5 结构
 DESCRIPTION: 保存单板所支持的A5加密算法。目前支持A5/1、A5/2、A5/3、A5/4。
              Bit位从右向左，依次为bit0、bit1...bit7。每个bit代表的含义如下:
              bit0=0:A5/1不支持，bit0=1:A5/1支持；
              bit1=0:A5/2不支持，bit1=1:A5/2支持；
              bit2=0:A5/3不支持，bit2=1:A5/3支持；
              bit3=0:A5/4不支持，bit3=1:A5/4支持；
              其他bit位暂不使用
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmA5;                                /* Range: [0,15]
                                                                                   0x0:不支持A5算法;
                                                                                   0x1:支持A5/1;
                                                                                   0x2:支持A5/2;
                                                                                   0x3:支持A5/1和A5/2 
                                                                                   0x4:支持A5/3;
                                                                                   0x5:支持A5/3和A5/1;
                                                                                   0x6:支持A5/3和A5/2;
                                                                                   0x7:支持A5/3、A5/2和A5/3 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_A5_STRU;

/*****************************************************************************
 结构名    : NVIM_LOW_COST_EDGE_FLAG_STRU
 结构说明  : en_NV_Item_LowCostEdge_Flag 结构
 DESCRIPTION: 表示MS是否支持lowCostEdge特性。如果MS支持该特性，则上行不支持8PSK编码方式。
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usLowCostEdgeFlag;                      /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                           aucReserve[2];
}NVIM_LOW_COST_EDGE_FLAG_STRU;

/*****************************************************************************
 结构名    : NVIM_GPRS_ACTIVE_TIMER_LENGTH_STRU
 结构说明  : en_NV_Item_GPRS_ActiveTimerLength 结构
 DESCRIPTION: GPRS Active Timer的长度。
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulGprsActiveTimerLength;                /* 0x0000:100ms;
                                                                                   0x0001:100ms;
                                                                                   0x0002:200ms;依次类推 */
}NVIM_GPRS_ACTIVE_TIMER_LENGTH_STRU;

/*****************************************************************************
 结构名    : NVIM_GPRS_MULTI_SLOT_CLASS_STRU
 结构说明  : en_Nv_Item_Gprs_Multi_Slot_Class 结构
 DESCRIPTION: 标识MS的GPRS多时隙能力等级
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGprsMultiSlotClass;                   /* Range: [0,12]
                                                                                   0x0:MS多时隙能力等级为0xC
                                                                                   0x1~0xC:指示MS多时隙能力等级 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GPRS_MULTI_SLOT_CLASS_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_PLMN_SEARCH_ARFCN_MAX_NUM_STRU
 结构说明  : en_NV_Item_GSM_PLMN_SEARCH_ARFCN_MAX_NUM 结构
 DESCRIPTION: GSM搜网时可以搜索的最大频点个数，推荐使用0x003C及以上的数值
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usGsmPlmnSearchArfcmMaxNum;             /* Range: [0x1,0x8C],搜网时最多搜索的频点个数，建议使用默认值以上的值，确认搜网的性能 */
    VOS_UINT8                           aucReserve[2];
}NVIM_GSM_PLMN_SEARCH_ARFCN_MAX_NUM_STRU;

/*****************************************************************************
 结构名    : NVIM_GCBS_CONF_STRU
 结构说明  : en_Nv_Item_GCBS_Conf 结构
 DESCRIPTION: 控制GCBS功能特性，包括ECBCH信道使能开关、GCBS Drx功能使能开关和GCBS激活定时器时长
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usECBCHSwitch;              /* 是否启动ECBCH信道,0x0:不启动,0x1:启动 */
    VOS_UINT16                                      usDrxSwitch;                /* 是否启动GCBS DRX功能,0x0:不启动,0x1:启动 */
    VOS_UINT32                                      ulGCBSActiveTimerLength;    /* GCBS激活定时器时长。单位ms */
}NVIM_GCBS_CONF_STRU;

/*****************************************************************************
 结构名    : NVIM_REPEATED_ACCH_CAPABILITY_STRU
 结构说明  : en_NV_Item_Repeated_Acch_Capability 结构
 DESCRIPTION: 是否支持Repeated FACCH/SACCH功能。该配置R6后使用
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usRepeatedAcchCapability;   /* 0x0:不支持，BSS只能用下行repeated FACCH块发送;
                                                                                   0x1:支持，BSS可以用下行repeated FACCH块来发送。 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_REPEATED_ACCH_CAPABILITY_STRU;


/*****************************************************************************
 结构名    : NVIM_ES_IND_STRU
 结构说明  : en_NV_Item_ES_IND 结构
 DESCRIPTION: 是否支持Controlled Early Classmark Sending
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usEsInd;                    /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_ES_IND_STRU;

/*****************************************************************************
 结构名    : NVIM_GPRS_EXTENDED_DYNAMIC_ALLOCATION_CAPABILITY_STRU
 结构说明  : en_NV_Item_GPRS_Extended_Dynamic_Allocation_Capability 结构
 DESCRIPTION: 是否支持GPRS扩展动态分配能力
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usGprsExtDynAllocCap;       /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_GPRS_EXTENDED_DYNAMIC_ALLOCATION_CAPABILITY_STRU;

/*****************************************************************************
 结构名    : NVIM_EGPRS_EXTENDED_DYNAMIC_ALLOCATION_CAPABILITY_STRU
 结构说明  : en_NV_Item_EGPRS_Extended_Dynamic_Allocation_Capability 结构
 DESCRIPTION: 是否支持EGPRS扩展动态分配能力
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usEGprsExtDynAllocCap;      /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_EGPRS_EXTENDED_DYNAMIC_ALLOCATION_CAPABILITY_STRU;

/*****************************************************************************
 结构名    : NVIM_REVISION_LEVEL_INDICATOR_STRU
 结构说明  : en_NV_Item_Revision_Level_Indicator 结构
 DESCRIPTION: ME发布版本
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usRevLvlInd;                /* Range: [0,1],0x0:R99之前版本,0x1:RR或之后版本 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_REVISION_LEVEL_INDICATOR_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_REVISION_LEVEL_CUSTOMIZE_STRU
 结构说明  : en_NV_Item_GAS_Revision_Level_Customization 结构
 DESCRIPTION: 异系统获取classmark2中的revision level IE能力定制
*****************************************************************************/
typedef struct
{
    VOS_UINT8                            ucInterRatCustomizeFlag;       /* 异系统获取classmark2中的Revison level IE能力定制，
                                                                           0:异系统获取能力不定制；
                                                                           1:表示定制生效，异系统获取能力classmark2中Revison level固定填充R99或以后版本
                                                                        */
    
    VOS_UINT8                            ucRsv1;                        /* 保留位1 */
    
    VOS_UINT8                            ucRsv2;                        /* 保留位2 */
    
    VOS_UINT8                            ucRsv3;                        /* 保留位3 */

}NVIM_GAS_REVISION_LEVEL_CUSTOMIZE_STRU;

/*****************************************************************************
 结构名    : NVIM_DOWNLINK_ADVANCED_RECEIVER_PERFORMANCE_STRU
 结构说明  : en_NV_Item_Downlink_Advanced_Receiver_Performance 结构
 DESCRIPTION: 是否支持下行高级接收能力
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usDlAdvRcvPer;              /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_DOWNLINK_ADVANCED_RECEIVER_PERFORMANCE_STRU;

/*****************************************************************************
 结构名    : NVIM_EXT_RLC_MAC_CTRL_MSG_SEGMENT_CAPABILITY_STRU
 结构说明  : en_NV_Item_Ext_RLC_MAC_Ctrl_Msg_Segment_Capability 结构
 DESCRIPTION: MS是否支持扩展RLC/MAC控制消息分段
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usExtRlcMacCtrlMsgSegCap;   /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_EXT_RLC_MAC_CTRL_MSG_SEGMENT_CAPABILITY_STRU;

/*****************************************************************************
 结构名    : NVIM_PS_HANDOVER_CAPABILITY_STRU
 结构说明  : en_NV_Item_PS_Handover_Capability 结构
 DESCRIPTION: MS是否支持PS域切换
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usPsHandoverCapability;     /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_PS_HANDOVER_CAPABILITY_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_WEAK_SIGNAL_THREHOLD_STRU
 结构说明  : en_NV_Item_GAS_Errorlog_Energy_Threshold 结构
 DESCRIPTION: 设置GSM ErrorLog特性弱信号的RSSI门限值 
*****************************************************************************/
typedef struct
{
    VOS_INT16                                       sWeakSignalThreshold;       /* GSM ErrorLog特性弱信号的RSSI门限值。Range:[-120,20] */
    VOS_UINT8                                       aucReserve[2];
}NVIM_GAS_WEAK_SIGNAL_THREHOLD_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_MULTIRATE_CAP_STRU
 结构说明  : en_NV_Item_GSM_Multirate_Capability 结构
 DESCRIPTION: UE支持的GSM多速率能力
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       ucGsmMultirateCap;          /* GSM多速率能力.0x0:仅支持全速率,0x1:支持DUAL RATE,0x2:仅支持SDCCH */
    VOS_UINT8                                       aucRsv[3];
}NVIM_GSM_MULTIRATE_CAP_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_CLASSMARK3_R8_STRU
 结构说明  : en_NV_Item_Gsm_Classmark3_R8 结构
 DESCRIPTION: GSM Classmark3码流，仅供内部使用
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       aucGsmClassMark3Data[NVIM_CLASSMARK3_R8_LEN];   /* CLASSMARK3 码流 */
}NVIM_GSM_CLASSMARK3_R8_STRU;

/*****************************************************************************
 结构名    : NVIM_EUTRA_MEAS_AND_REPORTING_SUPPORT_FLG_STRU
 结构说明  : en_NV_Item_EUTRA_MEAS_AND_REPORTING_SUPPORT_FLG 结构
 DESCRIPTION: MS是否支持在专用态下进行EUTRAN的邻区测量和上报
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usLteMeasSupportedFlg;      /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_EUTRA_MEAS_AND_REPORTING_SUPPORT_FLG_STRU;

/*****************************************************************************
 结构名    : NVIM_PRI_BASED_RESEL_SUPPORT_FLG_STRU
 结构说明  : en_NV_Item_PRI_BASED_RESEL_SUPPORT_FLG 结构
 DESCRIPTION: MS是否支持基于优先级的重选
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usPriBasedReselSupportFlg;  /* Range: [0,1],0x0:不支持,0x1:支持 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_PRI_BASED_RESEL_SUPPORT_FLG_STRU;

/*****************************************************************************
 结构名    : NVIM_GERAN_TO_EUTRA_SUPPORT_IN_TRANSFER_MODE_STRU
 结构说明  : en_NV_Item_GERAN_TO_EUTRA_SUPPORT_IN_TRANSFER_MODE 结构
 DESCRIPTION: MS在数传态支持的LTE相关的异系统互操作特性
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                      usLteSupportInTransferMode; /* Range: [0,3]
                                                                                   0x0:不支持GSM数传态下的E-UTRAN邻区测量和自主重选;
                                                                                   0x1:支持GSM数传态下的E-UTRAN邻区测量和自主重选,不支持GSM数传态下EUTRAN的邻区测量和CCN/CCO到EUTRAN
                                                                                   0x2:支持GSM数传态下的E-UTRAN邻区测量和自主重选,支持GSM数传态下EUTRAN的邻区测量和CCN/CCO到EUTRAN
                                                                                   0x3:除了1、2以外，还支持PS HANDOVER到EUTRAN,暂时平台不支持该配置 */
    VOS_UINT8                                       aucReserve[2];
}NVIM_GERAN_TO_EUTRA_SUPPORT_IN_TRANSFER_MODE_STRU;

/*****************************************************************************
 结构名    : NVIM_UE_EUTRA_FREQ_BAND_INFO_STRU
 结构说明  : 用于描述一个 LTE 频段
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucBandNo;
    VOS_UINT8                           ucDuplexModeFlg;
    VOS_UINT8                           aucReserve[2];
}NVIM_UE_EUTRA_FREQ_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : NVIM_UE_EUTRA_SUPPORT_FREQ_BAND_LIST_STRU
 结构说明  : en_NV_Item_EUTRA_CAPA_COMM_INFO 结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usBandCnt;
    VOS_UINT8                               aucReserved1[2];
    NVIM_UE_EUTRA_FREQ_BAND_INFO_STRU       astCandBands[NVIM_EUTRA_MAX_SUPPORT_BANDS_NUM];
}NVIM_UE_EUTRA_SUPPORT_FREQ_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_HIGH_MULTISLOT_CLASS_STRU
 结构说明  : en_NV_Item_GAS_High_Multislot_Class 结构
 DESCRIPTION: MS是否支持Multi class33
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usHighMultislotClassFlg;            /* Range: [0,1],0x0:无效,0x1:有效 */
    VOS_UINT16                              usHighMultislotClass;               /* 等级类型，详细参数NV说明书 */
}NVIM_GAS_HIGH_MULTISLOT_CLASS_STRU;

/*****************************************************************************
 结构名    : NVIM_GPRS_NON_DRX_TIMER_LENGTH_STRU
 结构说明  : en_NV_Item_GPRS_Non_Drx_Timer_Length 结构
 DESCRIPTION: 终端的NON-DRX时长的能力，需要在RAU和Attach消息中带给网侧
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usNonDrxTimerLen;                   /* Range: [0,7]
                                                                                   0:NON-DRX时间为0秒
                                                                                   1:NON-DRX时间为1秒
                                                                                   2:NON-DRX时间为2秒
                                                                                   3:NON-DRX时间为4秒
                                                                                   4:NON-DRX时间为8秒
                                                                                   5:NON-DRX时间为16秒
                                                                                   6:NON-DRX时间为32秒
                                                                                   7:NON-DRX时间为64秒
                                                                                   */
    VOS_UINT8                               aucReserve[2];
}NVIM_GPRS_NON_DRX_TIMER_LENGTH_STRU;

/*****************************************************************************
 结构名    : NVIM_UE_TDS_SUPPORT_FREQ_BAND_LIST_STRU
 结构说明  : en_NV_Item_UTRAN_TDD_FREQ_BAND 结构
 DESCRIPTION: TDS支持的频段个数及频段号
*****************************************************************************/
typedef struct
{
    VOS_UINT8                               ucBandCnt;                          /* 支持的TDS频段个数,目前最大支持3个频段(A/E/F),Range: [0,6] */
    VOS_UINT8                               aucReserved[3];
    VOS_UINT8                               aucBandNo[NVIM_TDS_MAX_SUPPORT_BANDS_NUM];  /* aucBandNo[x]标示支持的TDS频段号 */
}NVIM_UE_TDS_SUPPORT_FREQ_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_QSEARCH_CUSTOMIZATION_STRU
 结构说明  : en_NV_Item_QSearch_Customization 结构
 DESCRIPTION: 异系统测量控制参数QSearch取值表的定制模式
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usQSearchCustomMode;                /* 0x0:3GPP协议模式,0x1:中移动定制模式 */
    VOS_UINT8                               aucReserve[2];
}NVIM_QSEARCH_CUSTOMIZATION_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_POOR_RXQUAL_THRESHOLD_STRU
 结构说明  : en_NV_Item_Gsm_Poor_RxQual_ThresHold 结构
 DESCRIPTION: 已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usRrPoorRxQualThresHold;
    VOS_UINT8                               aucReserve[2];
}NVIM_GSM_POOR_RXQUAL_THRESHOLD_STRU;

/*****************************************************************************
 结构名    : NVIM_CSFB_CUSTOMIZATION_STRU
 结构说明  : en_NV_Item_Csfb_Customization 结构
 DESCRIPTION: L2G CSFB呼叫时延优化功能的定制模式
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usCsfbCustomization;                /* 0x0:定制不生效,0x1:定制生效 */
    VOS_UINT8                               aucReserve[2];
}NVIM_CSFB_CUSTOMIZATION_STRU;

/*****************************************************************************
 结构名    : NVIM_CBS_MID_LIST_STRU
 结构说明  : en_NV_Item_CBS_MID_List 结构
 DESCRIPTION: 已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT8                               aucCbsMidList[NVIM_CBS_MID_LIST_LEN];
}NVIM_CBS_MID_LIST_STRU;

/*****************************************************************************
 结构名    : NVIM_CBS_MID_RANGE_LIST_STRU
 结构说明  : en_NV_Item_CBS_MID_Range_List 结构
 DESCRIPTION: 已废弃
*****************************************************************************/
typedef struct
{
    VOS_UINT8                               aucCbsMidRangeList[NVIM_CBS_MID_RANGE_LIST_LEN];
}NVIM_CBS_MID_RANGE_LIST_STRU;


typedef struct
{
   VOS_UINT8                            ucFrLteMeasFlag;                        /* 是否开启GSM自主重定向到LTE的LTE频点测量，0: 关闭, 1: 开启，默认值为1 */
   VOS_UINT8                            ucMaxSavedMeasTimes;                    /* 最大保存的测量次数，默认值为8，范围:[0,8] */

   VOS_UINT8                            ucFrInvalidMeasFlag;                    /* 是否开启用测量结果判断GSM自主重定向到LTE是否有效，0: 关闭, 1: 开启，默认值为1 */
   VOS_UINT8                            ucMaxInvalidMeasTimes;                  /* 最大无效测量次数，用来判定频点是否有效，不大于最大保存的测量次数，默认值为8，范围:[0,8] */
}NVIM_GSM_AUTO_FR_LTE_MEAS_CONFIG_STRU;


typedef struct
{
   VOS_UINT8                            ucC1CustomizeSwitchFlag;          /* 是否开启路损C1定制，0: 关闭, 1: 开启，默认值为0 */
   VOS_UINT8                            ucRxlevAccessMin;                 /* 开启路损C1定制后，定制的最小接入电平等级，默认等级为8，范围:[0,63] */
   VOS_UINT8                            ucC1ValueThreshold;               /* C1阈值 */
   VOS_UINT8                            aucRsv[1];
}NVIM_GSM_C1_CUSTOMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_NFREQ_THRESHOLD_STRU
 结构说明  : en_NV_Item_Gsm_NFreq_Threshold 结构
 DESCRIPTION: 邻频干扰过滤门限
*****************************************************************************/
typedef struct
{
    VOS_UINT8                            ucFirstNFreqThreshold;                 /* 邻频干扰过滤门限 */
    VOS_UINT8                            aucRsv[3];
}NVIM_GSM_NFREQ_THRESHOLD_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_W_NON_NCELL_MEAS_CTRL_STRU
 结构说明  : en_NV_Item_Gas_W_Non_NCell_Meas_Ctrl 结构
 DESCRIPTION: 该NV用来定制GSM下，WCDMA非邻区测量的参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                            ucIdleNonNCellMeasEnable;              /* 空闲态下是否启动非邻区测量，0: 关闭, 1: 开启 */
    VOS_UINT8                            ucTransferNonNCellMeasEnable;          /* 传输态下是否启动非邻区测量，0: 关闭, 1: 开启 */
    VOS_INT16                            sRscpThreshold;                        /* 启动非邻区测量的 RSCP 门限，Range:[-119,0) */
    VOS_INT16                            sEcn0Threshold;                        /* 启动非邻区测量的 ECN0 门限，Range:[-20,0) */
    VOS_INT16                            sRssiThreshold;                        /* 启动非邻区测量的 RSSI 门限，Range:[-101,0) */
}NVIM_GAS_W_NON_NCELL_MEAS_CTRL_STRU;
/* Added by l67237 for Gas Dsds Feature, 2014-3-4, begin */
/*****************************************************************************
 结构名    : NV_GSM_RF_UNAVAILABLE_CFG_STRU
 结构说明  : en_NV_Item_GSM_RF_UNAVAILABLE_CFG 结构
 DESCRIPTION: 是否启动搜网的开关和启动搜网的时间
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucStartSearchFlag;                      /* 是否启动搜网:,1:是; 0:否 */
    VOS_UINT8                           ucSearchTimerLen;                       /* 多长时间启动搜网，单位:s */
    VOS_UINT8                           ucEnableScellSbFbFlag;                  /* 是否生效服务小区同步，1:是；0:否 */
    VOS_UINT8                           ucRsv;
}NV_GSM_RF_UNAVAILABLE_CFG_STRU;
/* Added by l67237 for Gas Dsds Feature, 2014-3-4, end */

/*****************************************************************************
 结构名    : NVIM_GSM_CELL_INFO_RPT_CFG_STRU
 结构说明  : en_NV_Item_Gsm_Cell_Info_Rpt_Cfg 结构
 DESCRIPTION: GAS向MTC上报当前小区信息的配置信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                            ucCellInfoRptFlg;                      /* 是否启动上报: 0:不启动; 1:启动 */
    VOS_UINT8                            aucRsv[1];                             /* 保留位 */
    VOS_UINT16                           usRptInterval;                         /* 上报周期长度, 单位: ms */
}NVIM_GSM_CELL_INFO_RPT_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_INDIVIDUAL_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_Gas_Individual_Customize_Cfg 结构
 DESCRIPTION: 用于设置非标的特殊定制相关的配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucAccFailNoPunishCfg;                   /* 该字段已被废弃,替换使用NV3036 */
    VOS_INT8                            cFreqRxlevThreshold;                    /* 有效频点能量门限,主要用于搜网流程
                                                                                   被动重选 目标小区选择时也会用到 */

    VOS_UINT8                           ucAutoFrNonChanRelCase;                 /* 没有收到 Channel Release 的异常场景下是否启动 FR:
                                                                                   bit0: 1: 主动挂断时启动 FR;
                                                                                         0: 主动挂断时不启动FR
                                                                                   bit1: 1: 语音链路失败启动FR;
                                                                                         0: 语音链路失败不启动FR
                                                                                   bit2: 1: 无线链路失效自主返回前需要先尝试呼叫重建;
                                                                                         0: 无线链路失效自主返回前不需要先尝试呼叫重建 */

    VOS_UINT8                           ucScellPrioFastEnableFlg;               /* 在SI2Q未收齐前，是否快速生效服务小区优先级，0:不生效,1:生效 */

    VOS_UINT8                           aucRsv[36];
}NVIM_GAS_INDIVIDUAL_CUSTOMIZE_CFG_STRU;


typedef struct
{
   VOS_UINT16                           usCsHoTimeAlarmThreshold;              /* CS切换时间长度门限，超过该切换时间，GAS主动上报告警信息 */

   VOS_UINT16                           usRptPseudBtsMinTimeInterval;          /* 两次伪基站CHR上报的最短时间间隔，单位:分钟；默认值为30分钟 */   

   VOS_INT16                            sRptRaFailRssiThreshold;               /* 随机接入失败的能量门限，超过该能量，GAS主动上报随机接入失败事件, 默认值为-85dBm */

   VOS_UINT16                           usRptRaNoRspMinTimeInterval;           /* 两次随机接入无响应CHR上报的最短时间间隔，单位:分钟；默认值为30分钟 */   
   
   VOS_INT16                            sRaNoRspRssiThreshold;                 /* 一次有效的随机接入无响应的能量门限，超过该能量，GAS记录一次有效的随机接入无响应,默认值为-85dBm */
   
   VOS_UINT8                            usRaNoRspAlarmTimesThreshold;          /* 连续随机接入无响应次数门限，超过该次数，GAS主动上报告警信息，默认值是10次 */   

   VOS_UINT8                            ucRptAbnormalAirMsgMinTimeInterval;    /* 两次空口异常CHR上报的最短时间间隔，单位:分钟；默认值为30分钟 */      

   VOS_UINT32                           ulRptRaFailMinTimeInterval;            /* 两次随机接入失败主动上报的最短时间间隔，单位:分钟；默认值为30分钟 */

   VOS_UINT32                           ulRsv7;                                /* 预留位7，为以后KWKC预留 */
}NVIM_GAS_MNTN_CONFIG_STRU;


typedef struct
{
   VOS_UINT8                            ucNotSuppBssPagingCoorFlag;             /* 不支持寻呼协调事件上报开关 */
   
   VOS_UINT8                            ucNotSuppBssPagingCoorMaxNum;           /* 不支持寻呼协调事件上报小区数 */

   VOS_UINT8                            ucRsrcCheckExcpFlag;                    /* 资源核查异常事件上报开关 */

   VOS_UINT8                            ucRsrcCheckExcpMaxNum;                  /* 资源核查异常事件上报记录个数 */

   VOS_UINT8                            ucCustomC1CellFlag;                     /* C1优化小区事件上报开关 */

   VOS_UINT8                            ucCustomC1CellMaxNum;                   /* C1优化小区事件上报个数 */

   VOS_UINT8                            ucRsv5;                                 /* 预留位，为以后KWKC预留 */

   VOS_UINT8                            ucRsv6;                                 /* 预留位，为以后KWKC预留 */

   VOS_UINT16                           usRsv1;                                 /* 预留位，为以后KWKC预留 */   

   VOS_UINT16                           usRsv2;                                 /* 预留位，为以后KWKC预留 */

   VOS_UINT16                           usRsv3;                                 /* 预留位，为以后KWKC预留 */
   
   VOS_UINT16                           usRsv4;                                 /* 预留位，为以后KWKC预留 */

   VOS_UINT32                           ulNotSuppBssPagingCoorInterval;         /* 不支持寻呼协调事件上报时间间隔，默认 30min, 单位: 10ms */  

   VOS_UINT32                           ulRsrcCheckExcpInterval;                /* 资源核查异常事件上报时间间隔，默认 60min, 单位: min */

   VOS_UINT32                           ulCustomC1CellInterval;                 /* C1优化小区事件上报时间间隔，默认 30min, 单位: min */

   VOS_UINT32                           ulRsv4;                                 /* 预留位，为以后KWKC预留 */
}NVIM_GAS_MNTN_CHR_DIRECT_RPT_CONFIG_STRU;



typedef struct
{
   VOS_UINT8                            ucTdsDefaultQRxlMin;            /* TDS 默认重选门限, 单位: -1dB */

   VOS_UINT8                            aucRsv[3];                      /* 保留位 */
}NVIM_GAS_UTRAN_TDD_DEFAULT_Q_RXLMIN;

/* Added by yangsicong for L2G REDIR C1 CUSTUME, 2015-1-26, begin */
typedef struct
{
    VOS_UINT32                          ulMcc;
    VOS_UINT32                          ulMnc;
}NVIM_GAS_PLMN_ID_STRU;


typedef struct
{
   VOS_UINT16                           usPlmnCnt;                          /* 白名单中PLMN个数 */
   VOS_UINT16                           usRxlevAccessMin;                   /* 白名单中小区定制的最小接入电平等级, 默认等级为8，范围:[0,63]
                                                                               0值表示这个域不生效, 定制门限使用 NV9248 中 ucRxlevAccessMin 域的值 */

   NVIM_GAS_PLMN_ID_STRU                astPlmn[NVIM_GAS_C1_CALC_OPT_PLMN_WHITE_LIST_CNT_MAX];       /* 白名单中的PLMN ID个数，最多20个 */
}NVIM_GAS_C1_CALC_OPT_PLMN_WHITE_LIST_STRU;
/* Added by yangsicong for L2G REDIR C1 CUSTUME, 2015-1-26, end */

/*****************************************************************************
 结构名    : NVIM_GSM_RAPID_HO_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_GSM_RAPID_HO_CUSTOMIZE_CFG 结构,GSM快速切换定制特性相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucSwitchFlag;                   /* 特性开关 */
    VOS_UINT8                           ucBadQualityThreshold;          /* 信号质量判定门限.误码率.单位: 百分之一 */
    VOS_UINT16                          usAlpha;                        /* Alpha因子. 单位: 0.001 */
    VOS_UINT8                           ucBadQualityCntThreshold;       /* 信号质量差统计门限 */
    VOS_UINT8                           ucNCellRptAddValue;             /* 邻区上报增加值 */
    VOS_UINT8                           aucRsv[2];                      /* 保留位 */
}NVIM_GSM_RAPID_HO_CUSTOMIZE_CFG_STRU;


typedef struct
{
    NVIM_GAS_PLMN_ID_STRU               stPlmn;                                 /* 定制频段的PLMN */
    VOS_UINT32                          ulBand;                                 /* 频段 */
}NVIM_GSM_BAND_CUSTOMIZE_STRU;


typedef struct
{
   VOS_UINT16                           usItemCnt;                  /* 定制项个数，Range:[0-80] */
   VOS_UINT16                           usRestoreEnableMask;        /* 启用被丢弃的超出协议范围的频点生效场景,
                                                                       bit0:L2G重定向流程，0x0:不生效,0x1:生效 
                                                                       bit1:GOOS搜流程, 0x0:不生效,0x1:生效 */
   VOS_UINT32                           ulCustomizeBandEnableMask;  /* 定制 Band 生效场景
                                                                       bit0:L2G重定向流程，0x0:不生效,0x1:生效 
                                                                       bit1:GOOS搜流程, 0x0:不生效,0x1:生效 */
   NVIM_GSM_BAND_CUSTOMIZE_STRU         astCustomizeBand[NVIM_GAS_GSM_BAND_CUSTOMIZE_LIST_MAX_CNT];     /* PLMN列表 */
}NVIM_GAS_GSM_BAND_CUSTOMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_SEARCH_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_GSM_SEARCH_CUSTOMIZE_CFG 结构，GSM搜网流程相关配置
 DESCRIPTION: GSM搜网流程相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucRsv1;                                 /* 废弃，高质量指定搜流程是否使用历史频点 */
    VOS_UINT8                           ucRsv2;                                 /* 废弃，GOos搜流程是否使用历史频点 */

    VOS_UINT8                           ucGeoHighUseStoreFlag;                  /* 高质量获取地理信息流程是否使用历史频点 */
    VOS_UINT8                           ucGeoHighRemoveStoreFlag;               /* 高质量获取地理信息流程是否在高质量扫频阶段删除历史频点 */
    VOS_UINT8                           ucGeoNormalUseStoreFlag;                /* 非高质量获取地理信息流程是否使用历史频点 */

    VOS_UINT8                           ucRmvDecFailSamePlmnFlag;               /* 频点解码失败后是否删除所有相同PLMN的优选小区 */

    VOS_UINT8                           ucUpdateRssiBySi;                       /* 根据系统消息更新信号强度 */

    VOS_UINT8                           ucHistorySrchOperatorCustomizeCellCnt;  /* HISTORY搜索运营商定制频点的个数范围 */

}NVIM_GSM_SEARCH_CUSTOMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucEnableFlg;                    /* 是否生效,0x0:不生效,0x1:生效 */
    VOS_UINT8                           ucRefreshTimeLen;               /* 使用历史系统消息驻留后多久强制刷新系统消息.
                                                                           单位:秒. 0表示不强制刷新 */

    VOS_UINT16                          usValidTimeLen;                 /* 有效时长, 单位:分钟 */
    VOS_UINT32                          ulUsableMask;                   /* 用于指定可以使用历史系统消息驻留的流程 */
    VOS_UINT8                           ucNcellEnableFlg;               /* 邻区系统消息是否生效 */
    VOS_UINT8                           ucNcellValidTimeLen;            /* 有效时长, 单位:秒 */
    VOS_UINT8                           ucRsv3;                         /* 保留位 */
    VOS_UINT8                           ucRsv4;                         /* 保留位 */
}NVIM_GAS_GSM_CELL_HISTORY_BCCH_SI_CFG_STRU;


typedef struct
{
    VOS_UINT8                            ucHPrioCustomizeFlag;   /* H_PRIO使用场景定制配置, 0表示使用H_PRIO参数不考虑异系统小区的优先级，
                                                                    1表示使用H_PRIO参数的前提条件是异系统小区的优先级比服务小区优先级低 */
    
    VOS_UINT8                            ucRsv1;                 /* 保留位1 */
    
    VOS_UINT8                            ucRsv2;                 /* 保留位2 */
    
    VOS_UINT8                            ucRsv3;                 /* 保留位3 */
}NVIM_GAS_INTER_RAT_RESEL_H_PRIO_CUSTOMIZE_STRU;


typedef struct
{
   VOS_UINT8                            ucRrRaFailEnableFlg;        /* 生效标志,0:不生效;1:生效 */

   VOS_UINT8                            ucRsv1;                     /* 保留位 */
   VOS_UINT8                            ucRsv2;                     /* 保留位 */
   VOS_UINT8                            ucRsv3;                     /* 保留位 */
}NVIM_GAS_FAST_AREA_LOST_CFG_STRU;


typedef struct
{
    VOS_UINT8                            ucAutoFrInvalidMask;   /* 生效标志。各bit值 0 表示可以返回, 1 表示无效不可以返回 */
                                                                /* bit0: CSFB 主叫, NAS仍未反馈业务建立结果的情况下是否可以返回 */
                                                                /* bit1: CSFB 被叫, NAS仍未反馈业务建立结果的情况下是否可以返回 */
                                                                /* bit2~bit8: 保留位 */

    VOS_UINT8                            ucAutoFrNonCsfbFlg;       /* 非CSFB场景下, 业务结束时是否自主返回 */
    VOS_UINT8                            ucRsv2;                /* 保留位 */
    VOS_UINT8                            ucRsv3;                /* 保留位 */
}NVIM_GAS_AUTO_FR_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucPrioReselDisableMask; /* 禁用基于优先级测量和重选准则的场景.以下各 bit 值1表示禁用.0表示不禁用 */
                                                                /* 注: 配置为0时是否启用基于优先级的算法仍受协议和 NV 9003的约束 */
                                                                /* bit0: LTE 模式不支持, 且没有3G优先级参数的场景 */
                                                                /* bit1: LTE 模式被 Disable, 且没有3G优先级参数的场景 */
                                                                /* bit2~bit8: 保留位 */

    VOS_UINT8                           ucWaitAllNCellDecodedByThreshGsmLow;    /* 低优先级判决中，等待所有邻区解码完成才进行裁决 */
    VOS_UINT8                           ucRsv2;                 /* 保留位 */
    VOS_UINT8                           ucRsv3;                 /* 保留位 */
    VOS_UINT16                          usRsv4;                 /* 保留位 */
    VOS_UINT16                          usRsv5;                 /* 保留位 */
}NVIM_GAS_INTER_RAT_RESEL_CFG_STRU;


typedef struct
{
    VOS_UINT16      usEnableBitmap;             /* 识别算法是否使能 ：0=是；1=否 */
                                                    /* bit0: X2G REDIR流程是否使能识别算法; */
                                                    /* bit1: X2G reselect流程是否使能识别算法; */
                                                    /* bit2: X2G CCO流程是否使能识别算法; */
                                                    /* bit3: GSM  SPEC搜/GOOS搜流程是否使能识别算法; */
                                                    /* bit4: GSM HISTORY搜/GSM FAST搜流程是否使能识别算法; */
                                                    /* bit5~bit15: 预留 */
    VOS_UINT16      ucReserve1;                 /* 备用字段 */
    VOS_UINT16      usFeatureSetEnableBitmap;   /* 使能的伪基站特征集BITMAP: bit (n)=1表示第n个特征集是有效的; */
    VOS_UINT16      usReserve2;                 /* 备用字段 */
    VOS_UINT16      usPseudBTSFeatureSet0;      /* 特征集0,每个bit代表一个特征,该bit为1时表示该特征属于这个特征集合:
                                                    bit0代表 RX_ACCESS_MIN=0; 
                                                    bit1 代表 CCCH-CONF='001'时,BS-AG-BLKS-RES=2; 
                                                    bit2 代表 不支持GPRS; 
                                                    bit3 代码CRO超过 60;
                                                    bit4 代码MCC为460;
                                                    bit5~bit15预留 */
    VOS_UINT16      usPseudBTSFeatureSet1;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet2;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet3;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet4;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet5;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet6;      /* 备用字段*/
    VOS_UINT16      usPseudBTSFeatureSet7;      /* 备用字段*/
} NVIM_GAS_PSEUD_BTS_IDENT_CUSTOMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT32              ulHistoryFreqEnableBitmap;                          /* 历史频点搜索流程使能bit位 */
    VOS_UINT32              ulOperateFreqEnableBitmap;                          /* 运营商定制频点搜索流程使能bit位 */
    VOS_UINT32              ulFullListEnableBitmap;                             /* 全频段搜索流程使能bit位 */
    VOS_UINT32              ulHistoryFilterSrchedFreqEnableBitmap;              /* 历史频点搜索流程过滤已搜索频点功能使能 bit位 */
    VOS_UINT32              ulOperateFilterSrchedFreqEnableBitmap;              /* 运营商定制频点搜索流程过滤已搜索频点功能使能 bit位 */
    VOS_UINT32              ulFullListFilterSrchedFreqEnableBitmap;             /* 全频段频点搜索流程过滤已搜索频点功能使能 bit位 */
    VOS_UINT32              ulIgnoreLowPrioJudgeEnableBitmap;                   /* 不区分低优先级功能使能bit位 */
    VOS_UINT32              ulCloudFreqEnableBitmap;                            /* 云端定制频点搜索流程使能bit位 */
    VOS_UINT32              ulHistoryFreqBandSrchEnableBitmap;                  /* 历史频点所在频段搜索流程使能位 */
    VOS_UINT32              ulPresetFreqBandSrchEnableBitmap;                   /* 预置频点搜做频段搜索流程使能位 */
    VOS_UINT32              ulCloudBandEnableBitmap;                            /* 云端定制频段搜索流程使能bit位 */
    VOS_UINT32              ulCloudFreqFilterSrchedFreqEnableBitmap;            /* 云端频点搜索流程过滤已搜索频点功能使能 bit位 */
    VOS_UINT32              ulRescueCellByPresetBandEnableBitmap;               /* 预置频段挽救的功能控制使能bit位 */
    VOS_UINT32              ulCssShareFreqSrchEnableBitmap;                     /* CSS各Modem共享历史频点搜索流程使能bit位 */
    VOS_UINT32              ulReserved3;                                        /* 保留位 */
    VOS_UINT32              ulReserved4;                                        /* 保留位 */
    VOS_UINT32              ulReserved5;                                        /* 保留位 */
}NVIM_GSM_NETWORK_SEARCH_CUSTOMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT16                          usFreqBegin;                            /* 起始频点 */
    VOS_UINT16                          usFreqEnd;                              /* 终止频点 */
    NVIM_BAND_IND_ENUM_UINT8            enBand;                                 /* BAND指示 */
    VOS_UINT8                           ucRsv1;                                 /* 保留位 */
    VOS_UINT8                           ucRsv2;                                 /* 保留位 */
    VOS_UINT8                           ucRsv3;                                 /* 保留位 */
}NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_RANGE_STRU;


typedef struct
{
    VOS_UINT16                          usFreq;                                 /* 频点号 */
    NVIM_BAND_IND_ENUM_UINT8            enBand;                                 /* BAND指示 */
    VOS_UINT8                           ucRsv1;                                 /* 保留位 */
}NVIM_GSM_OPERATE_CUSTOMIZE_DISCRETE_STRU;


typedef struct
{
    NVIM_GAS_PLMN_ID_STRU                       stPlmn;                         /* PLMN结构 */
    VOS_UINT8                                   ucRangeCnt;                     /* 频点范围的个数，最大个数:NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_RANGE_MAX_CNT */
    VOS_UINT8                                   ucDiscreteCnt;                  /* 离散频点的个数，最大个数:NVIM_GSM_OPERATE_CUSTOMIZE_DESCRETE_FREQ_MAX_CNT */
    VOS_UINT8                                   ucRsv1;                         /* 保留位 */
    VOS_UINT8                                   ucRsv2;                         /* 保留位 */
    NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_RANGE_STRU  astGsmOperateCustomFreqRange[NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_RANGE_MAX_CNT];
                                                                                /* 频点范围结构列表 */
    NVIM_GSM_OPERATE_CUSTOMIZE_DISCRETE_STRU    astGsmOperateCustomDiscrete[NVIM_GSM_OPERATE_CUSTOMIZE_DESCRETE_FREQ_MAX_CNT];
                                                                                /* 离散频点结构列表 */
    VOS_UINT8                                   ucRsv3;                         /* 保留位 */
    VOS_UINT8                                   ucRsv4;                         /* 保留位 */
    VOS_UINT16                                  usRsv1;                         /* 保留位 */
    VOS_UINT32                                  ulRsv1;                         /* 保留位 */
}NVIM_GSM_OPERATOR_CUSTOMIZE_PLMN_ITEM_STRU;


typedef struct
{
    VOS_UINT8                                   ucPlmnCnt;                      /* 运营商定制的PLMN列表个数，最大个数:NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_PLMN_MAX_CNT */
    VOS_UINT8                                   ucRsv1;                         /* 保留位 */
    VOS_UINT8                                   ucRsv2;                         /* 保留位 */
    VOS_UINT8                                   ucRsv3;                         /* 保留位 */
    NVIM_GSM_OPERATOR_CUSTOMIZE_PLMN_ITEM_STRU  astGsmOperatorCustomPlmnItem[NVIM_GSM_OPERATE_CUSTOMIZE_FREQ_PLMN_MAX_CNT];   
                                                                                /* 定制的PLMN列表 */
}NVIM_GSM_OPERATOR_CUSTOMIZE_FREQ_CFG_STRU;


typedef struct
{
    VOS_UINT32                          ulWaitFirstUsableMask;          /* 用于指定等待第一小区的流程 */
    VOS_UINT16                          usWaitFirstTimerLen;            /* 等待第一小区定时器长度，单位:ms */

    VOS_INT16                           sSameLaiThreshold;              /* 优先选择同一个LAI频点的RSSI门限 */
    VOS_UINT32                          ulSameLaiUsableMask;            /* 用于指定优先选择同一个LAI的流程 */
    VOS_UINT16                          usSameLaiTimerLen;              /* 等待同一个LAI小区定时器长度，单位:ms */
    VOS_UINT8                           ucStoreSameLaiFlag;             /* 历史频点阶段是否优先选择同一个LAI频点 */
    VOS_UINT8                           ucFullSameLaiFlag;              /* 全频段阶段是否优先选择同一个LAI频点 */

    VOS_UINT32                          ulWithSiUsableMask;             /* 用于指定优先选择带系统消息的流程 */
    VOS_UINT8                           ucStoreWithSiFlag;              /* 历史频点阶段是否优先选择带系统消息频点 */
    VOS_UINT8                           ucFullWithSiFlag;               /* 全频段阶段是否优先选择带系统消息频点 */
    VOS_INT16                           sWithSiThreshold;               /* 优先选择带系统消息频点的RSSI门限 */

    VOS_UINT8                           ucRmvUtranImpactFlg;            /* 扫频结果中是否删除Utran干扰频点 */
    VOS_UINT8                           ucRmvPseudoNCell;               /* 扫频结果中是否删除邻频干扰频点 */
    VOS_UINT8                           ucAutoAnycellCamp;              /* 指定搜SUITABLE驻留失败后是否主动尝试ANYCELL驻留 */
    VOS_UINT8                           ucMaxArfcnNum;                  /* 并行搜搜索最大频点个数 */

    VOS_UINT8                           ucFirstTcIgnoreSi2Q;            /* 第一个TC周期前是否忽略SI2Quater */
    VOS_UINT8                           ucRmvNoNeedCellFlg;             /* 阶段搜索过程中是否删除已经确认无用的小区 */
    VOS_UINT8                           ucRmvCellBetterThreshold;       /* 阶段搜索过程中删除无用小区要求信号变好的门限 */
    VOS_UINT8                           ucAnyCellSrchBaFlag;            /* Any Cell搜网是否需要搜索BA */

    VOS_UINT16                          usRsv1;                         /* 保留位 */
    VOS_UINT16                          usRsv2;                         /* 保留位 */
    VOS_UINT16                          usRsv3;                         /* 保留位 */
    VOS_UINT16                          usRsv4;                         /* 保留位 */

    VOS_INT16                           sRmvCellLastRxlevThreshold;     /* 阶段搜索过程中删除无用小区要求频点列表搜信号的门限 */
    VOS_INT16                           sRsv2;                          /* 保留位 */
    VOS_INT16                           sRsv3;                          /* 保留位 */
    VOS_INT16                           sRsv4;                          /* 保留位 */

    VOS_UINT32                          ulRsv1;                         /* 保留位 */
    VOS_UINT32                          ulRsv2;                         /* 保留位 */
    VOS_UINT32                          ulRsv3;                         /* 保留位 */
    VOS_UINT32                          ulRsv4;                         /* 保留位 */

}NVIM_GSM_PARALLEL_SEARCH_CUSTOMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucCsfbEnableFlg;                        /* L2G重定向方式的CSFB场景下是否启动该功能,0x0:不生效,0x1:生效 */
    VOS_UINT8                           ucRedirEnableFlg;                       /* L2G重定向方式的非CSFB场景下是否启动该功能,0x0:不生效,0x1:生效 */
    VOS_INT8                            cSpecArfcnPreferThreshold;              /* 网络指定频点的优先门限。 */
    VOS_UINT8                           ucHistoryArfcnRssiReduceValue;          /* 历史频点RSSI减去ucHistoryArfcnRssiReduceValue后参与排序。 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
}NVIM_GSM_ENABLE_HISTORY_ARFCN_WITH_SPEC_ARFCN_LST_STRU;


typedef struct
{
    /* ===============惩罚机制总体相关的NV配置项===================== */
    VOS_UINT8           ucCsAccFailPunishSwitchFlag;        /* 是否开启CS随机接入失败惩罚机制，0: 关闭, 1: 开启，默认值为1 */

    VOS_UINT8           ucPsAccFailPunishSwitchFlag;        /* 是否开启PS随机接入失败惩罚机制，0: 关闭, 1: 开启，默认值为0 */

    VOS_UINT8           ucRrConnFailOptimizeSwtichFlag;     /* 是否开启接入失败优化机制开关，
                                                               0: 关闭, 1: 开启，默认值为1
                                                               打开该NV项后，
                                                               1.
                                                               随机接入失败一次后该小区为FORBIND优先级小区，
                                                               随机接入失败超过一定次数启动惩罚机制，惩罚周期内将认为该小区被BAR；惩罚时间超时则认为是低优先级小区；

                                                               2.
                                                               N200失败次数达到一定次数(NV可配置)，执行被动重选；
                                                               N200失败达到一定次数(NV可配置), 启动惩罚机制，惩罚周期内将认为该小区被BAR；惩罚时间超时则认为是低优先级小区；

                                                               3.
                                                               重新随机接入成功，如果该小区是因为随机接入失败加入列表，从列表中删除该小区；
                                                               重新层二建链成功，如果该小区是因为N200失败加入列表，从列表中删除该小区；                                                               

                                                               关闭该NV项，则FORBIND优先级没有迭代惩罚策略；也没有N200惩罚机制
                                                               随机接入失败一次后该小区为FORBIND优先级小区，超过3次不能驻留；
                                                               */

    /* ===============随机接入失败相关的NV配置项===================== */
    VOS_UINT8           ucStartPunishRaFailTimesThreshold;     /* 在该GSM小区上随机接入失败次数门限, 达到该次数对该小区启动惩罚机制 */

    VOS_UINT8           ucRssiOffset;                         /* 历史信号强度低于门限，小区信号强度增加门限达到该门限将该小区从Bar列表中删除 */


    /* ===============N200相关的NV配置项============================== */
    VOS_UINT8           ucN200FailPunishSwitchFlag;             /* 是否开启N200失败惩罚机制，0: 关闭, 1: 开启，默认值为1 */

    VOS_UINT8           ucStartReselN200FailTimesThreshold;     /* 在该GSM小区上N200失败次数门限, 达到该次数对启动被动重选 */

    VOS_UINT8           ucStartPunishN200FailTimesThreshold;    /* 在该GSM小区上N200失败次数门限, 达到该次数对该小区启动惩罚机制 */
    
    VOS_INT16           sN200FailRssiThreshold;                 /* 在该GSM小区上N200失败RSSI门限，单位:dBm  */

    /* ===============惩罚时间相关的NV配置项============================== */
    VOS_UINT16          usInitialForbidTimeLen;                /* 建链失败第一次的惩罚时间，单位:秒 */

    VOS_UINT16          usMaxForbidTimeLen;                    /* 建链失败的小区的最大惩罚时长，单位:秒 */

    VOS_INT16           sRssiHighThresh;                      /* 单位:dBm, 加入惩罚列表的小区如果信号强度超过该值，该小区从惩罚列表中移除将不考虑信号强度变化 */

    VOS_UINT32          ulForbidCellMaxReServeTimeLen;        /* Forbid小区在惩罚列表中的最大保留时长，单位:分 */

    VOS_UINT32          ulRssiThresholdSet;                    /* 各个段信号强度的上下限集合，各段信号强度的单位:dBm。
                                                                  从低到高，第一个字节表示好信号强度的下限,默认值为-85dBm，即好信号是指信号强度大于-85dBm，不包括-85dBm;
                                                                  第二个字节表示中等信号强度的下限,默认值为-95dBm, 中等信号是指信号强度介于-85dBm和-95dBm, 包括-85dBm和-95dBm;*/

    VOS_UINT32          ulRssiOffsetSet;                       /* 各个段信号强度随机接入失败记录老化的偏移值。
                                                                  从低到高，第一个字节表示好信号强度随机接入失败记录老化的偏移值,默认值为20;
                                                                  第二个字节表示中等信号强度随机接入失败记录老化的偏移值,默认值为8;
                                                                  第三个字节表示弱信号强度随机接入失败记录老化的偏移值,默认值为16;
                                                                  */
    VOS_UINT32          ulRsv6;                              /* 保留位 */
    VOS_UINT32          ulRsv7;                              /* 保留位 */

}NVIM_GAS_RR_CONNECT_FAIL_PUNISH_CFG_STRU;


/*****************************************************************************
 结构名    : NVIM_GSM_PING_PONG_HO_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_GSM_PING_PONG_HO_CUSTOMIZE_CFG 结构,GSM 乒乓切换定制特性相关配置
 DESCRIPTION: GSM 乒乓切换定制特性相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucSwitchFlag;                   /* 特性开关 */
    VOS_UINT8                           ucHoTimeThreshold;              /* 统计范围内的小区平均驻留时间门限, 初步推荐30s */
    VOS_UINT8                           ucBadCellTimeScale;             /* 坏小区平均时间评估因子, 单位: 0.1, 初步推荐 20 */
    VOS_UINT8                           ucGoodQualityThreshold;         /* 信号质量好判定门限.误码率. 单位: 千分之一, 初步推荐值为5 */
    VOS_UINT8                           ucGoodQualityCntThreshold;      /* 信号质量好的统计次数门限, 初步推荐值 3 */
    VOS_UINT8                           ucNCellRptReduceValue;          /* 优化生效时,邻区上报调整量, 单位 dB, 初步推荐值为20 */
    VOS_UINT8                           ucWatchHoTimes;                 /* 切换次数, 初步推荐值为 4 */
    VOS_UINT8                           aucRsv1;                        /* 保留位 */
    VOS_UINT16                          ausRsv1;
    VOS_UINT16                          ausRsv2;
    VOS_UINT8                           aucRsv3;
    VOS_UINT8                           aucRsv4;
    VOS_UINT8                           aucRsv5;
    VOS_UINT8                           aucRsv6;
}NVIM_GSM_PING_PONG_HO_CUSTOMIZE_CFG_STRU;


typedef struct
{
    VOS_UINT8                           ucDistLteAcsTypeEnable;         /* 使能区分LTE制式进行测量的开关 */
    VOS_UINT8                           aucRsv1;                        /* 保留位 */
    VOS_UINT8                           aucRsv2;
    VOS_UINT8                           aucRsv3;
    VOS_UINT16                          ausRsv1;
    VOS_UINT16                          ausRsv2;
}NVIM_GSM_LTE_MEASURE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_SEC_RXQUAL_SUB_ALPHA_FILTERING_CFG_STRU
 结构说明  : en_NV_Item_GSM_SEC_RXQUAL_SUB_ALPHA_FILTERING_CFG 结构,对GSM SUB集的误码率进行ALPHA滤波, 以判断服务小区是否可以继续驻留的相关配置
 DESCRIPTION: 对GSM SUB集的误码率进行ALPHA滤波, 以判断服务小区是否可以继续驻留的相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usSecRrPoorRxQualSubThresHold;  /* 信号质量误码率判定门限, 精度:千分之一, 有效值为0~1000, 推荐值: 80, 即误码率为8% */
    VOS_UINT16                          usSecRxQualSubAlpha;            /* Alpha因子, 精度:千分之一, 有效值为0~1000, 推荐值: 250, 即Alpha为0.25 */
    VOS_UINT16                          usSecConSubBerCntThresHold;     /* 连续usSecConSubBerCntThresHold个数样点大于阈值, 再开启Poor机制, 推荐值: 2, 即连续三次 */
    VOS_UINT8                           ucRsv1;                         /* 保留位 */
    VOS_UINT8                           ucRsv2;
    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
}NVIM_GSM_SEC_RXQUAL_SUB_ALPHA_FILTERING_CFG_STRU;

#if defined( __PS_WIN32_RECUR__ ) || defined (DMT)

typedef struct  
{
    VOS_UINT32  ulPermitedChrAlarmIdCount;

    VOS_UINT16  aucAlarmIds[MAX_CHR_ALARM_ID_NUM];
    
}NVIM_GAS_CHR_PC_CFG_STRU;
#endif

/*****************************************************************************
 结构名    : NVIM_Item_GAS_SAME_LAI_PREFER_CFG_STRU
 结构说明  : en_NV_Item_GAS_SAME_LAI_PREFER_CFG 结构, 搜网过程中优先选择相同 LAI 小区
             的相关配置.
 DESCRIPTION: 搜网过程中优先选择相同 LAI 小区的相关配置.
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucMtSwitchFlag;                 /* 特性开关 */
    VOS_UINT8                           ucMoSwitchFlag;                 /* 特性开关 */

    VOS_INT8                            cFirstLevelAddValue;           /* 第一能量等级中相同 LAI 小区排序时的能量增加值 */
    VOS_INT8                            cFirstLevelThreshold;          /* 第一能量等级门限 */

    VOS_INT8                            cSecLevelAddValue;             /* 第二能量等级中相同 LAI 小区排序时的能量增加值 */
    VOS_INT8                            cSecLevelThreshold;            /* 第二能量等级门限 */
    VOS_UINT16                          usValidTimeLen;                 /* 有效时长, 单位:分钟 */
    VOS_UINT8                           aucRsv1;                        /* 保留位 */
    VOS_UINT8                           aucRsv2;                        /* 保留位 */
    VOS_UINT8                           aucRsv3;                        /* 保留位 */
    VOS_UINT8                           aucRsv4;                        /* 保留位 */
}NVIM_Item_GAS_SAME_LAI_PREFER_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_HO_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_GSM_HO_CUSTOMIZE_CFG 结构,GSM 切换定制特性相关配置
 DESCRIPTION: GSM 切换定制特性相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucChanModeRptIndication;        /* 语音信道切换切换过程,中给 MM 上报 GAS_RR_CHAN_IND 的时机 */
    VOS_UINT8                           aucRsv1;
    VOS_UINT16                          ausRsv1;
    VOS_UINT8                           aucRsv2;
    VOS_UINT8                           aucRsv3;
    VOS_UINT8                           aucRsv4;
    VOS_UINT8                           aucRsv5;
}NVIM_GSM_HO_CUSTOMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_PMR_CFG_STRU
 结构说明  : en_NV_Item_GSM_PMR_CFG 结构,GSM PMR相关定制
 DESCRIPTION: GSM PMR相关定制
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucNcReportPeriodIMin;           /* 空闲态 PMR 周期最小值 */
    VOS_UINT8                           aucRsv1;
    VOS_UINT16                          ausRsv1;
    VOS_UINT8                           aucRsv2;
    VOS_UINT8                           aucRsv3;
    VOS_UINT8                           aucRsv4;
    VOS_UINT8                           aucRsv5;
}NVIM_GSM_PMR_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GSM_GCBS_MESSAGE_CUSTOMIZE_CFG_STRU
 结构说明  : en_NV_Item_GCbs_Message_Customize_CFG 结构, GCBS Message相关定制
 DESCRIPTION: GCBS Message相关定制
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucIsFixedPageDataLengthFlag;    /* 特性开关,用于控制是否固定88字节Page上报TAF.如果为VOS_TRUE,则是按固定88字节上报;为VOS_FALSE,则根据(有效BLOCK数x22字节)上报;默认为VOS_TRUE */
    VOS_UINT8                           ucIsOptionalMessageReadFlag;    /* 特性开关,用于控制是否读Optional reading CB Message.如果为VOS_TRUE,则读;为VOS_FALSE,则不读;默认为VOS_TRUE */
    VOS_UINT8                           ucIsAdvisedMessageReadFlag;     /* 特性开关,用于控制是否读Reading advised CB Message.如果为VOS_TRUE,则读;为VOS_FALSE,则不读;默认为VOS_TRUE */
    VOS_UINT8                           ucRsv1;                         /* 保留位 */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
    VOS_UINT16                          usRsv3;
}NVIM_GSM_GCBS_MESSAGE_CUSTOMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_GSM_ACTIVE_CELL_RESELECT_CFG_STRU
 结构说明  : en_NV_Item_GAS_GSM_CELL_RESELECT_CFG 结构, GAS 层G2G重选流程相关配置
 DESCRIPTION: GAS 层G2G重选流程相关配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucC2ReselPartialReadFlg;        /* 主动重选是否需要部分读
                                                                           1: 需要;
                                                                           0: 不需要 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT16                          usHistorySi3ValidTimeLen;       /* 驻留阶段历史SI3有效时长. 单位: s */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;

    VOS_UINT8                           ucC2ReselPunishSwitchFlg;       /* 主动重选邻区惩罚特性是否开启
                                                                           1: 开启;
                                                                           0: 关闭 */
    VOS_UINT8                           ucRsv6;
    VOS_INT16                           sScellRssiThreshold;            /* 服务小区能量波动阈值，单位: dBm */
    VOS_UINT16                          usScellPchBerThreshold;         /* 服务小区PCH误码率Alpha滤波阈值，单位: %，千分之一精度 */
    VOS_UINT16                          usScellPchBerAlpha;             /* 服务小区PCH误码率Alpha滤波，Alpha因子，单位: %，千分之一精度 */
    VOS_UINT16                          usForbiddenCellPunishTimeLen;   /* Forbidden 小区重选惩罚时长，当超过时长，将小区清除，单位: s */
    VOS_UINT16                          usForbiddenCellPunishStep;      /* 当服务小区为好小区时，对邻区进行惩罚步进值，单位: dBm */
    VOS_UINT8                           ucScellRaSuccStep;              /* 服务小区RA成功一次步进值 */
    VOS_UINT8                           ucScellRaFailStep;              /* 服务小区RA失败一次步进值 */
    VOS_UINT8                           ucRsv9;
    VOS_UINT8                           ucRsv10;  
    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
}NVIM_GAS_GSM_ACTIVE_CELL_RESELECT_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_GSM_PASSIVE_RESELECT_OPTIMIZE_CFG_STRU
 结构说明  : en_NV_Item_GAS_GSM_PASSIVE_RESELECT_OPTIMIZE_CFG 结构，立即指派被拒优化和T3166/T3168超时优化
 DESCRIPTION: 立即指派被拒优化和T3166/T3168超时优化配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucImmAssRejOptSwitchFlg;            /* 特性开关 */
    VOS_UINT8                           ucImmAssRejCntThreshold;            /* 最大立即指派被拒次数 */
    VOS_UINT8                           ucImmAssRejWaitIndThreshold;        /* 最大立即指派被拒时长，单位: s */

    VOS_UINT8                           ucCcAbnormalRelCnt;                 /* 连续失败超过此门限时发起被动重选, 0xFF 表示关闭对应功能 */

    VOS_UINT8                           ucT3166T3168ExpiredOptSwitchFlg;    /* 特性开关 */
    VOS_UINT8                           ucT3166T3168ExpiredCntThreshold;    /* T3166/T3168超时允许最大超时次数 */
    VOS_UINT16                          usT3166T3168ExpiredPunishTimeLen;   /* T3166/T3168超时惩罚定时器时长，单位: ms */

    VOS_UINT8                           ucRsv1;                             /* 保留位 */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
    VOS_UINT8                           ucRsv6;
    VOS_UINT8                           ucRsv7;
    VOS_UINT8                           ucRsv8;

    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
}NVIM_GAS_GSM_PASSIVE_RESELECT_OPTIMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_GSM_SACCH_BA_INHERIT_OPTIMIZE_CFG_STRU
 结构说明  : en_NV_Item_GAS_GSM_SACCH_BA_INHERIT_OPTIMIZE_CFG 结构, SACCH Ba 继承优化配置结构
 DESCRIPTION: SACCH Ba 继承优化配置结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucInheritBcchFlg;
    VOS_UINT8                           ucInheritLastCellFlg;

    VOS_UINT8                           ucSi5terNotExistJugeTimes;

    VOS_UINT8                           ucRsv1;                             /* 保留位 */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
}NVIM_GAS_GSM_SACCH_BA_INHERIT_OPTIMIZE_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_GSM_PAGE_RCV_CFG_STRU
 结构说明  : en_NV_Item_GAS_GSM_PAGE_RCV_CFG 结构, 寻呼接收相关参数配置
 DESCRIPTION: 寻呼接收相关参数配置
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucRcvPchInPsFlg;            /* 特性总开关. 0:关闭; 1:打开 */
    VOS_UINT8                           ucRcvPchInPsSpecialFlg;     /* 特殊流程下的开关. 0:关闭; 1:打开
                                                                       bit0: 网络模式I的小区上. 是否打开特性
                                                                       bit1: 支持 BSS 的小区上. 是否打开特性
                                                                       bit2: RAU 过程中. 是否打开特性
                                                                       bit3: Attach 过程中. 是否打开特性
                                                                       bit4: PDP 相关流程. 是否打开特性 */

    VOS_UINT8                           ucRsv1;                             /* 保留位 */
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
    VOS_UINT8                           ucRsv6;
}NVIM_GAS_GSM_PAGE_RCV_CFG_STRU;

/*****************************************************************************
 结构名    : NVIM_GAS_NET_SRCH_RMV_INTER_RAT_FREQ_CFG_STRU
 结构说明  : en_NV_Item_GAS_Net_Srch_Rmv_Inter_Rat_Freq_Cfg 结构, 搜网扣异系统重叠频率参数配置 
 DESCRIPTION: 搜网扣异系统重叠频率参数配置 
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucPreferBandSrchRmvLteFreqSwitch;       /* PREFER Band搜索过程扣除LTE频点带宽范围内的G模频点特性开关；0: 关闭, 1: 开启，默认值为1 */

    VOS_UINT8                           ucPreferBandSrchRmvWcdmaFreqSwitch;     /* PREFER Band搜索过程扣除WCDMA频点带宽范围内的G模频点特性开关；0: 关闭, 1: 开启，默认值为1 */

    VOS_UINT8                           ucPreferBandSrchRmvCdmaFreqSwitch;      /* PREFER Band搜索过程扣除CDMA频点的带宽范围内的G模频点特性开关；0: 关闭, 1: 开启，默认值为1 */

    VOS_UINT8                           ucRsv1;                                 /* 保留位 */

    VOS_UINT8                           ucRsv2;

    VOS_UINT8                           ucRsv3;

    VOS_UINT16                          usRsv1;

    VOS_UINT16                          usRsv2;

    VOS_UINT16                          usRsv3;

    VOS_UINT32                          ulRsv1;

    VOS_UINT32                          ulRsv2;
}NVIM_GAS_NET_SRCH_RMV_INTER_RAT_FREQ_CFG_STRU;


typedef struct
{
    VOS_INT16                   sHighThresh;            /* 上报高质量的信号门限，单位dbm */
    VOS_INT16                   sLowThresh;             /* 上报低质量的信号门限，单位dbm */
    VOS_UINT16                  usOffset;               /* 迟滞参数，单位dbm */
    VOS_UINT16                  usRsv;                  /* 保留位 */
    VOS_UINT32                  ulTEvaluation;          /* 评估时长，单位ms */
    VOS_UINT32                  ulRsv;                  /* 保留位 */
}NVIM_GUAS_CELLULAR_PREFER_REPORT_PARA_CFG_STRU;


typedef struct
{
    NVIM_GUAS_CELLULAR_PREFER_REPORT_PARA_CFG_STRU     stGsmCellPreferCfg;      /* g模相关配置 */
    NVIM_GUAS_CELLULAR_PREFER_REPORT_PARA_CFG_STRU     stWcdmaCellPreferCfg;    /* w模相关配置 */
}NVIM_GUAS_CELLULAR_PREFER_PARA_CFG_STRU;


typedef struct
{
    VOS_UINT8                       ucEnableFlag;                               /* 功能使能开关 */
    VOS_UINT8                       ucReserved1;                                /* 保留位 */
    VOS_UINT8                       ucReserved2;                                /* 保留位 */
    VOS_UINT8                       ucReserved3;                                /* 保留位 */
    VOS_INT16                       sGsmRxlevThresh;                            /* GSM scell Rxlev Great门限，单位db */
    VOS_INT16                       sUtranFddRscpThreshOnRanking;               /* 非GSM Great信号场景，RANKING重选评估中，WCDMA RSCP本地门限，单位db */
    VOS_INT16                       sUtranFddEcNoThreshOnRanking;               /* 非GSM Great信号场景，RANKING重选评估中，WCDMA EcNo本地门限，单位db */
    VOS_INT16                       sUtranFddRscpThreshOnPrio;                  /* 非GSM Great信号场景，Prio重选评估中，WCDMA RSCP本地门限，单位db */
    VOS_INT16                       sUtranFddEcNoThreshOnPrio;                  /* 非GSM Great信号场景，Prio重选评估中，WCDMA EcNo本地门限，单位db */
    VOS_UINT16                      usReserved1;                                /* 保留位 */
    VOS_UINT16                      usReserved2;                                /* 保留位 */
    VOS_UINT16                      usReserved3;                                /* 保留位 */
    VOS_UINT16                      usReserved4;                                /* 保留位 */
    VOS_UINT16                      usReserved5;                                /* 保留位 */
    VOS_UINT16                      usReserved6;                                /* 保留位 */
    VOS_UINT16                      usReserved7;                                /* 保留位 */
}NVIM_GSM_IRAT_ACTIVE_RESEL_CUSTOM_CFG_STRU;




typedef struct
{
    VOS_UINT8                   ucCsfbMoPenaltyFlg;             /* 是否对CSFB MO场景进行惩罚,0: 不惩罚, 1: 惩罚，默认值为0 */                        
    VOS_UINT8                   ucCsfbMtPenaltyFlg;             /* 是否对CSFB MT场景进行惩罚,0: 不惩罚, 1: 惩罚，默认值为0 */
    VOS_UINT16                  usCsfbPenaltyTime;              /* 惩罚时间,单位:分钟，默认值为2160分钟 */
    VOS_UINT8                   ucCsfbFailCountForPenalty;      /* 进行惩罚需要CSFB失败的次数:默认3次 */     
    VOS_UINT8                   ucCsfbDifLaiFailCount;          /* CSFB进行同LAI优化需要CSFB跨LAI失败的次数:默认3次 */      
    VOS_INT16                   sCsfbSameLaiThreshold;          /* CSFB优先选择同一个LAI频点的RSSI门限，默认值为-90dbm */
    VOS_UINT16                  usCsfbSameLaiTimerLen;          /* CSFB等待同一个LAI小区定时器长度，单位:ms，默认值为5000ms */    
    VOS_UINT16                  usRsv1;                         /* 保留位 */
    VOS_UINT16                  usRsv2;                         /* 保留位 */
    VOS_UINT16                  usRsv3;                         /* 保留位 */
}NVIM_GAS_CSFB_FAIL_LAI_PENALTY_CFG_STRU;


typedef struct
{
    VOS_UINT8                   ucAllocUtranPriFlg;             /* 特性总开关. 0:关闭; 1:打开 */    
    VOS_UINT8                   ucDefaultThreshUtran;           /* 如果网络没有配置重选相关的参数，则NV配置 DEFAULT_THRESH_UTRAN 默认值 */   
    VOS_UINT8                   ucDefaultUtranQRxlMin;          /* 如果网络没有配置重选相关的参数，则NV配置 DEFAULT_UTRAN_QRXLEVMIN 默认值 */   
    VOS_UINT8                   ucSetUtranPriMinValueFlg;       /* 设置优化的默认3G优先级固定为最低优先级，即设定为1优先级。0: 不固定为1优先级; 1: 固定为1优先级 */
    VOS_UINT8                   ucRsv1;                         /* 保留位 */
    VOS_UINT8                   ucRsv2;                         /* 保留位 */
    VOS_UINT16                  usRsv3;                         /* 保留位 */
    VOS_UINT16                  usRsv4;                         /* 保留位 */
    VOS_UINT16                  usRsv5;                         /* 保留位 */
    VOS_UINT32                  ulRsv6;                         /* 保留位 */
}NVIM_GAS_NETWORK_NOT_ALLOC_UTRAN_PRIORITY_CFG_STRU;

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
