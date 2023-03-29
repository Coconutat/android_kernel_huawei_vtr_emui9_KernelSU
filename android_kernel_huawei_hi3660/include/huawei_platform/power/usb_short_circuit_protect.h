#ifndef _USB_SHORT_CURRENT_PROTECT
#define _USB_SHORT_CURRENT_PROTECT

#define HIZ_MODE 1
#define NOT_HIZ_MODE 0
#define DMD_NOTIFY_HIZ_ENABLE 1
#define DMD_NOTIFY_HIZ_DISABLE 0
#define DMD_HIZ_DISABLE 0
#define USCP_ENABLE_PAR 0644
#define USB_TEMP_PAR 0444
#define USCP_ENFORCE_PAR 0644
#define USB_TEMP_NUM 25
#define GET_TEMP_VAL_NUM 2
#define CHECK_NTC_TEMP_MAX 100
#define CHECK_NTC_TEMP_MIN -30
#define SLEEP_200MS  200
#define SLEEP_10MS   (10)
#define CHECK_INTERVAL_10000 10000
#define CHECK_INTERVAL_300 300
#define CHECK_CNT_INIT 1100
#define CHECK_CNT_LIMIT 1001
#define DEFAULT_TUSB_THRESHOLD 40

int is_in_uscp_mode(void);
int is_uscp_hiz_mode(void);
int is_in_rt_uscp_mode(void);
int get_usb_ntc_temp(void);

#endif
