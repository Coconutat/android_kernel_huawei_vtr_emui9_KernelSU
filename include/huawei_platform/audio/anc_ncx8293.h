/*
 * anc_ncx8293.h -- anc headset driver
 *
 * Copyright (c) 2014 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ANC_NCX8293
#define ANC_NCX8293

#include "huawei_platform/audio/anc_hs_interface.h"

#define ANC_HS_ENABLE_CHARGE    0
#define ANC_HS_DISABLE_CHARGE   1
#define ANC_HS_DISCHARGE_TIME       30
#define CLOSE_MIC_OUT_PATH_TIME     1
#define CHIP_DEFUALT_POWERED_TIME    8 // 2.73(PMIC powered on time) + 5(chip powered on time)

#define ANC_NCX8293_R000_DEVICE_ID                    0x00
#define ANC_NCX8293_R001_DEVICE_SETUP                 0x01
#define ANC_NCX8293_R002_INTERRUPT                    0x02
#define ANC_NCX8293_R003_MIC_PATH_SELECT              0x03
#define ANC_NCX8293_R004_DET_TRIGGER                  0x04
#define ANC_NCX8293_R005_ACCESSORY_STATUS             0x05
#define ANC_NCX8293_R006_SWITCH_STATUS1               0x06
#define ANC_NCX8293_R007_SWITCH_STATUS2               0x07
#define ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1       0x08
#define ANC_NCX8293_R009_MANUAL_SWITCH_CONTROL2       0x09
#define ANC_NCX8293_R00A_KEY_PRESS                    0x0a
#define ANC_NCX8293_R00B_DOUBLE_CLICK_TIME            0x0b
#define ANC_NCX8293_R00C_THRESHOLD1                   0x0c
#define ANC_NCX8293_R00D_THRESHOLD2                   0x0d
#define ANC_NCX8293_R00E_THRESHOLD3                   0x0e
#define ANC_NCX8293_R00F_THRESHOLD4                   0x0f
#define ANC_NCX8293_R010_ANC_UPPER_THRESHOLD          0x10
#define ANC_NCX8293_R032_ANC_KEY_DEBOUNCE_THRESHOLD   0x32

/* ANC_NCX8293_R001_DEVICE_SETUP */
#define ANC_NCX8293_MANAUAL_SWITCH_ENABLE_BIT         0x01
#define ANC_NCX8293_KEY_PRESS_DET_DISABLE_BIT         0x04
#define ANC_NCX8293_CTRL_OUTPUT_LOW_BIT               0x51
#define ANC_NCX8293_CTRL_OUTPUT_HIGH_BIT              0x71

/* ANC_NCX8293_R002_INTERRUPT */
#define ANC_NCX8293_INSERTION_IRQ_BIT                 0x01
#define ANC_NCX8293_REMOVAL_IRQ_BIT                   0x02
#define ANC_NCX8293_KEY_PRESS_IRQ_BIT                 0x20

/* ANC_NCX8293_R005_ACCESSORY_STATUS */
#define ANC_NCX8293_ATTACH_BIT                        0x01
#define ANC_NCX8293_ANC_BIT                           0x10

/* ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1 */
#define ANC_NCX8293_POWER_MODE_BIT                    0x04
#define ANC_NCX8293_MIC_BIAS_MODE_BIT                 0x10
#define ANC_NCX8293_IDLE_MODE_BIT                     0x00
#define ANC_NCX8293_GND_SL_SWITCH_BIT                 0x01

/* ANC_NCX8293_R00A_KEY_PRESS */
#define ANC_NCX8293_KEY1_PRESS                        0x01
#define ANC_NCX8293_KEY1_RELEASE                      0x02
#define ANC_NCX8293_KEY2_PRESS                        0x04
#define ANC_NCX8293_KEY2_RELEASE                      0x08
#define ANC_NCX8293_KEY3_PRESS                        0x10
#define ANC_NCX8293_KEY3_RELEASE                      0x20

/* ANC_NCX8293_R032_ANC_KEY_DEBOUNCE_THRESHOLD */
#define ANC_NCX8293_2ms_KEY_DEBOUNCE_TIME             0x00
#define ANC_NCX8293_30ms_KEY_DEBOUNCE_TIME            0x11
#define ANC_NCX8293_60ms_KEY_DEBOUNCE_TIME            0x22 //default
#define ANC_NCX8293_90ms_KEY_DEBOUNCE_TIME            0x33

struct MLIBSetParaInfo {
    unsigned   short   msgID;
    short   reserve;
    int     uwPathID;
    int     uwModuleID;
    int     uwSize;
    char    aucData[];
};
#define MLIB_PARA_LENGTH_MAX 200

#define ERROR_RET 1
#define OK_RET    0

#define MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC 262148

enum AUDIO_MSG_ENUM {
    ID_AP_AUDIO_ENHANCE_SET_DEVICE_IND  = 0xDD91,
    ID_AP_AUDIO_MLIB_SET_PARA_IND       = 0xDD92,
};
enum MLIB_PATH_ENUM {
    MLIB_PATH_CS_VOICE_CALL_MICIN = 0,
    MLIB_PATH_CS_VOICE_CALL_SPKOUT,
    MLIB_PATH_VOIP_CALL_MICIN,
    MLIB_PATH_VOIP_CALL_SPKOUT,
    MLIB_PATH_AUDIO_PLAY,
    MLIB_PATH_AUDIO_RECORD,
    MLIB_PATH_SIRI_MICIN,
    MLIB_PATH_SIRI_SPKOUT,
    MLIB_PATH_BUTT,
};
enum MLIB_MODULE_ENUM {
    MLIB_MODULE_DEFAULT = 0,
    MLIB_MODULE_BALONG_PP,
    MLIB_MODULE_FORTE_VOICE,
    MLIB_MODULE_MBDRC,
    MLIB_MODULE_KARAOKE,
    MLIB_MODULE_MCPS_TEST,
    MLIB_MODULE_WNR_VOICE,
    MLIB_MODULE_3A_VOICE,
    MLIB_MODULE_BUTT,
};
enum MLIB_VOICE_PARA_ENUM
{
    MLIB_PARA_INVALID = 0,
    MLIB_BWE_PARA_ENABLE,
    MLIB_LVM_PARA_ENABLE,
    MLIB_BT_PARA_NREC,
    MLIB_RESET_PARA_ENABLE,
    MLIB_PRINT_MCPS_ENABLE,
    MLIB_VOICE_ALGO_BYPASS,
    MLIB_WNR_PARA_ENABLE,
    MLIB_DHF_PARA_ENABLE,
    MLIB_ANC_HS_PARA_ENABLE,
};
struct MlibParameterVoice
{
    int32_t  key;
    int32_t  value;
};

#ifdef CONFIG_ANC_NCX8293
int anc_ncx8293_dev_register(struct anc_hs_dev *dev, void * codec_data);
bool anc_ncx8293_check_headset_pluged_in(void);
void anc_ncx8293_start_charge(void);
bool anc_ncx8293_charge_detect(int saradc_value, int headset_type);
void anc_ncx8293_stop_charge(void);
void anc_ncx8293_force_charge(int disable);
bool check_anc_ncx8293_support(void);
bool anc_ncx8293_plug_enable(void);
void anc_ncx8293_invert_headset_control(int connect);
void anc_ncx8293_refresh_headset_type(int headset_type);
#else
int anc_ncx8293_dev_register(struct anc_hs_dev *dev, void * codec_data)
{
    return 0;
}
bool anc_ncx8293_check_headset_pluged_in(void)
{
    return false;
}
static inline void anc_ncx8293_start_charge(void)
{
    return;
}
static inline bool anc_ncx8293_charge_detect(int saradc_value, int headset_type)
{
    return false;
}
static inline void anc_ncx8293_stop_charge(void)
{
    return;
}
static inline void anc_ncx8293_force_charge(int disable)
{
    return;
}
static inline bool check_anc_ncx8293_support(void)
{
    return false;
}
static inline bool anc_ncx8293_plug_enable(void)
{
    return false;
}
void anc_ncx8293_invert_headset_control(int connect)
{
    return;
}
void anc_ncx8293_refresh_headset_type(int headset_type)
{
    return;
}
#endif

#endif //ANC_HS_H