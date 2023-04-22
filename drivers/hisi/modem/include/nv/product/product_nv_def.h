/******************************************************************************

    Copyright(C)2008,Hisilicon Co. LTD.

 ******************************************************************************
  File Name       :   product_nv_def.h
  Description     :   产线NV 结构体定义
  History         :
******************************************************************************/

#ifndef __PRODUCT_NV_DEF_H__
#define __PRODUCT_NV_DEF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define MAX_SINGLE_GUC_BAND_CHECK_NUM       28
#define MAX_SINGLE_TL_BAND_CHECK_NUM        12
#define MAX_NV_GUC_CHECK_ITEM_NUM           32
#define MAX_NV_TL_CHECK_ITEM_NUM            64

#ifndef __u8_defined
#define __u8_defined
typedef signed char          s8;
typedef unsigned char        u8;
typedef signed short         s16;
typedef unsigned short       u16;
typedef signed int           s32;
typedef unsigned int         u32;
typedef signed long long     s64;
typedef unsigned long long   u64;
#endif

/*请参考结构体样例*/
typedef struct
{
    int reserved;  /*note */
}PRODUCT_MODULE_STRU;

typedef struct
{
    u32 uwGucCrcResult;                     		/* GUc主卡NV的校验结果 */
    u32 uwTlCrcResult;                      		/* TL主卡NV的校验结果 */
    u32 uwGucM2CrcResult;                   		/* GUC副卡NV的校验结果 */
}NV_CRC_CHECK_RESULT_STRU;

typedef struct
{
    u16 uhwTransmitMode;                       		/* 指示该组的发送模式 */
    u16 uhwBand;                              		/* 指示该组的BAND号 */
    u16 uhwEnable;                              	/* 指示是否有效 */
    u16 uhwValidCount;                              	/* 指示有效数据的个数 */
    u16 auhwNeedCheckID[MAX_SINGLE_GUC_BAND_CHECK_NUM]; /* 保存需要检查的NV_ID值 */
}SINGLE_GUC_BAND_NV_ID_STRU;

typedef struct
{
    u16 uhwTransmitMode;                       		/* 指示该组的发送模式 */
    u16 uhwBand;                              		/* 指示该组的BAND号 */
    u16 uhwEnable;                              	/* 指示是否有效 */
    u16 uhwValidCount;                              	/* 指示有效数据的个数 */
    u16 auhwNeedCheckID[MAX_SINGLE_TL_BAND_CHECK_NUM]; 	/* 保存需要检查的NV_ID值 */
}SINGLE_TL_BAND_NV_ID_STRU;

typedef struct
{
    SINGLE_GUC_BAND_NV_ID_STRU astNVIDCheckItem[MAX_NV_GUC_CHECK_ITEM_NUM];  	/* 保存需要检查的NV_ID值 */
}NV_GUC_CHECK_ITEM_STRU;

typedef struct
{
    SINGLE_TL_BAND_NV_ID_STRU astNVIDCheckItem[MAX_NV_TL_CHECK_ITEM_NUM];   	/* 保存需要检查的NV_ID值 */
}NV_TL_CHECK_ITEM_STRU;

/*****************************************************************************
 结构名    : CHG_BATTERY_LOW_TEMP_PROTECT_NV
 结构说明  : CHG_BATTERY_LOW_TEMP_PROTECT_NV结构 ID=52005
*****************************************************************************/
typedef struct
{
    u32  ulIsEnable;
    s32  lCloseAdcThreshold;
    u32  ulTempLowCount;
}CHG_BATTERY_LOW_TEMP_PROTECT_NV;


typedef struct
{
    u32                          ulIsEnable;
    s32                           lCloseADCHold;
    u32                          ulTempOverMax;
}OM_BATTREY_TEMP_CFG_STRU;

/*****************************************************************************
 结构名    : nvi_cust_pid_type
 结构说明  : nvi_cust_pid_type结构
*****************************************************************************/
typedef struct
{
    u32  nv_status;
    u16  cust_first_pid;
    u16  cust_rewind_pid;
}nvi_cust_pid_type;

/*****************************************************************************
 结构名    : NV_WEBNAS_SD_WORKMODE_STRU
 结构说明  : NV_WEBNAS_SD_WORKMODE结构
*****************************************************************************/
typedef struct
{
    u32 ulSDWorkMode;
}NV_WEBNAS_SD_WORKMODE_STRU;

/*****************************************************************************
 结构名    : NV_OLED_TEMP_ADC_STRU
 结构说明  : NV_OLED_TEMP_ADC结构
*****************************************************************************/
typedef struct
{
    s16       sTemp;
    s16       sADC;
}NV_OLED_TEMP_ADC_STRU;

/*****************************************************************************
 结构名    : NV_OLED_TEMP_ADC_STRU_ARRAY
 结构说明  : NV_OLED_TEMP_ADC_STRU_ARRAY结构  ID=49
*****************************************************************************/
typedef struct
{
    NV_OLED_TEMP_ADC_STRU          stNV_OLED_TEMP_ADC[28];
}NV_OLED_TEMP_ADC_STRU_ARRAY;

/*****************************************************************************
 结构名    : TEMP_ADC_STRU
 结构说明  : TEMP_ADC结构
*****************************************************************************/
typedef struct
{
    s16    sTemp;
    u16    usADC;
}TEMP_ADC_STRU;

/*****************************************************************************
 结构名    : TEMP_ADC_STRU_ARRAY
 结构说明  : TEMP_ADC_STRU_ARRAY结构
*****************************************************************************/
typedef struct
{
    TEMP_ADC_STRU    stTempAdc[28];
}TEMP_ADC_STRU_ARRAY;

/*****************************************************************************
 结构名    : USB_SN_NV_INFO_STRU
 结构说明  : USB_SN_NV_INFO结构
*****************************************************************************/
typedef struct
{
    u32    usbSnNvStatus;
    u32    usbSnNvSucFlag;
    u32    usbSnReserved1;
    /*u32    usbSnReserved2;*/
}USB_SN_NV_INFO_STRU;

/*****************************************************************************
 结构名    : LIVE_TIME_STRU
 结构说明  : LIVE_TIME结构
*****************************************************************************/
typedef struct
{
    u32    ulLiveTime;
}LIVE_TIME_STRU;

/*****************************************************************************
 结构名    : LIVE_TIME_CONTROL_STRU
 结构说明  : LIVE_TIME_CONTROL结构
*****************************************************************************/
typedef struct
{
    u32    ulEnable;
    u32    ulCycle;
} LIVE_TIME_CONTROL_STRU;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


