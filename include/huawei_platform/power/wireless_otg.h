#ifndef _WIRELESS_OTG
#define _WIRELESS_OTG

#define OTG_SWITCH_ENABLE 1
#define OTG_SWITCH_DISABLE 0

#define NOT_IN_OTG_MODE 0
#define IN_OTG_MODE 1

extern void wireless_otg_detach_handler(int);
extern void wireless_otg_attach_handler(int);
extern int wireless_otg_get_mode(void);

#endif
