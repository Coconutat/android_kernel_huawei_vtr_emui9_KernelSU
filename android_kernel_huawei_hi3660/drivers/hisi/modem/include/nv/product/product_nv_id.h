/******************************************************************************

    Copyright(C)2008,Hisilicon Co. LTD.

 ******************************************************************************
  File Name       :   product_nv_id.h
  Description     :   productNV ID枚举定义
  History         :

******************************************************************************/

#ifndef __PRODUCT_NV_ID_H__
#define __PRODUCT_NV_ID_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*
 *  NV ID 的添加按从小到大排列
 */

typedef enum
{
    /* common nvid */
    NV_ID_CRC_CHECK_RESULT  = 0xc350,
    NV_ID_GUC_CHECK_ITEM    = 0xc351,
    NV_ID_TL_CHECK_ITEM     = 0xc352,
    NV_ID_GUC_M2_CHECK_ITEM = 0xc353,
    NV_ID_PRODUCT_END       = 0xcb1f,

    /* used by mbb product */
    NV_ID_DRV_LIVE_TIME_CONTROL            = 22,
    NV_ID_DRV_LIVE_TIME                    = 23,
    NV_ID_DRV_USBSN_NV_INFO                = 26,
    NV_ID_DRV_BATTERY_TEMP_HKADC           = 35,
    NV_ID_DRV_OLED_TEMP_ADC                = 49,
    NV_ID_DRV_WEBNAS_SD_WORK_MODE          = 51,
    NV_ID_MSP_BATTERY_TEMP_CFG             = 50016,
    NV_ID_DRV_CUST_PID_TYPE                = 50071,
    NV_ID_DRV_BATT_LOW_TEMP_PROTECT        = 52005,
}NV_ID_PRODUCT_ENUM;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


