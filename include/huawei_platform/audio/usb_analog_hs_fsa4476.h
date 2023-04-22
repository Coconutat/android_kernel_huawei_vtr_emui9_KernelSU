#ifndef __USB_ANALOG_HS_FSA4476__
#define __USB_ANALOG_HS_FSA4476__

#include "huawei_platform/audio/usb_analog_hs_interface.h"
#include "huawei_platform/usb/hw_pd_dev.h"

extern void pd_dpm_send_event(enum pd_dpm_cable_event_type event);

#ifdef CONFIG_USB_ANALOG_HS_FSA4476
int usb_ana_hs_fsa4476_check_hs_pluged_in(void);
int usb_ana_hs_fsa4476_dev_register(struct usb_analog_hs_dev *dev, void *codec_data);
bool check_usb_analog_hs_fsa4476_support(void);
void usb_ana_hs_fsa4476_plug_in_out_handle(int hs_state);
void usb_ana_hs_fsa4476_mic_swtich_change_state(void);
#else
int usb_ana_hs_fsa4476_check_hs_pluged_in(void)
{
    return 0;
}

int usb_ana_hs_fsa4476_dev_register(struct usb_analog_hs_dev *dev, void *codec_data)
{
	return 0;
}

bool check_usb_analog_hs_fsa4476_support(void)
{
	return false;
}

void usb_ana_hs_fsa4476_plug_in_out_handle(int hs_state)
{
	return;
}

void usb_ana_hs_fsa4476_mic_swtich_change_state(void)
{
	return;
}
#endif

#endif //USB_ANALOG_HS_FSA4476
