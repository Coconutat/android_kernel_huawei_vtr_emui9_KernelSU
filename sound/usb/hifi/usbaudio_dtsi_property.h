#ifndef __USBAUDIO_DTSI_PROPERTY_H
#define __USBAUDIO_DTSI_PROPERTY_H

#ifdef CONFIG_USB_AUDIO_DTSI_PROPERTY
bool get_usbaudio_need_auto_suspend(void);
#else
static inline bool get_usbaudio_need_auto_suspend(void)
{
    return false;
}
#endif /* CONFIG_USB_AUDIO_DTSI_PROPERTY */

#endif /* __USBAUDIO_DTSI_PROPERTY_H */


