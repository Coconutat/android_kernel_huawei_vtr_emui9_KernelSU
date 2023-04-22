/*
 * usb_audio_power.h -- usb audio power control driver
 *
 * Copyright (c) 2015 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __USB_AUDIO_POWER_H__
#define __USB_AUDIO_POWER_H__

#define IOCTL_USB_AUDIO_POWER_BUCKBOOST_NO_HEADSET_CMD     _IO('Q', 0x01)
#define IOCTL_USB_AUDIO_POWER_SCHARGER_CMD                 _IO('Q', 0x02)

enum VBOOST_CONTROL_SOURCE_TYPE {
    VBOOST_CONTROL_PM = 0,
    VBOOST_CONTROL_AUDIO,
};
enum MLIB_VOICE_PARA_ENUM {
    AUDIO_POWER_GPIO_RESET = 0,
    AUDIO_POWER_GPIO_SET,
};
#ifdef CONFIG_USB_AUDIO_POWER
int bst_ctrl_enable(bool enable, enum VBOOST_CONTROL_SOURCE_TYPE type);
int usb_audio_power_buckboost(void);
int usb_audio_power_scharger(void);
#else
static inline int bst_ctrl_enable(bool enable, enum VBOOST_CONTROL_SOURCE_TYPE type)
{
    return 0;
}

static inline int usb_audio_power_buckboost(void)
{
    return 0;
}

static inline int usb_audio_power_scharger(void)
{
    return 0;
}
#endif

#endif //__USB_AUDIO_POWER_H__