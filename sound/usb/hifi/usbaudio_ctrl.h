#ifndef __USBAUDIO_CTRL_H
#define __USBAUDIO_CTRL_H

bool usbaudio_ctrl_controller_switch(struct usb_device *dev, u32 usb_id, struct usb_host_interface *ctrl_intf, int ctrlif);
int usbaudio_ctrl_disconnect(void);
void usbaudio_ctrl_set_chip(struct snd_usb_audio *chip);
void usbaudio_ctrl_wake_up(bool wake_up);
#endif /* __CONTROLLER_SWITCH_H */
