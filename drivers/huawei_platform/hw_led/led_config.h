#ifndef __LED_CONFIG_H__
#define __LED_CONFIG_H__

#define LED_CONFIG    "led_config"

#define LED_CONFIG_COEFF_10 10
#define LED_CONFIG_BUFF_LEN 32

#define TP_MODULE_ID_NUM_00 0
#define TP_MODULE_ID_NUM_01 1
#define TP_MODULE_ID_NUM_02 2
#define TP_MODULE_ID_NUM_03 3
#define TP_MODULE_ID_NUM_04 4
#define TP_MODULE_ID_NUM_05 5
#define TP_MODULE_ID_NUM_06 6
#define TP_MODULE_ID_NUM_07 7
#define TP_MODULE_ID_NUM_08 8
#define TP_MODULE_ID_NUM_09 9
#define TP_MODULE_ID_NUM_10 10
#define TP_MODULE_ID_NUM_11 11
#define TP_MODULE_ID_NUM_12 12
#define TP_MODULE_ID_NUM_13 13
#define TP_MODULE_ID_NUM_14 14
#define TP_MODULE_ID_NUM_15 15
#define TP_MODULE_ID_NUM_16 16
#define TP_MODULE_ID_NUM_17 17
#define TP_MODULE_ID_NUM_18 18
#define TP_MODULE_ID_NUM_19 19
#define TP_MODULE_ID_NUM_20 20
#define TP_MODULE_ID_NUM_21 21
#define TP_MODULE_ID_NUM_22 22
#define TP_MODULE_ID_NUM_23 23
#define TP_MODULE_ID_NUM_24 24
#define TP_MODULE_ID_NUM_25 25
#define TP_MODULE_ID_NUM_26 26

#define TP_IC_ID_NUM_10 10
#define TP_IC_ID_NUM_11 11
#define TP_IC_ID_NUM_12 12
#define TP_IC_ID_NUM_13 13
#define TP_IC_ID_NUM_14 14
#define TP_IC_ID_NUM_15 15
#define TP_IC_ID_NUM_16 16
#define TP_IC_ID_NUM_17 17
#define TP_IC_ID_NUM_18 18
#define TP_IC_ID_NUM_19 19
#define TP_IC_ID_NUM_20 20
#define TP_IC_ID_NUM_21 21
#define TP_IC_ID_NUM_22 22
#define TP_IC_ID_NUM_23 23
#define TP_IC_ID_NUM_24 24
#define TP_IC_ID_NUM_25 25
#define TP_IC_ID_NUM_26 26
#define TP_IC_ID_NUM_27 27
#define TP_IC_ID_NUM_28 28
#define TP_IC_ID_NUM_29 29
#define TP_IC_ID_NUM_30 30
#define TP_IC_ID_NUM_31 31
#define TP_IC_ID_NUM_32 32
#define TP_IC_ID_NUM_33 33
#define TP_IC_ID_NUM_34 34
#define TP_IC_ID_NUM_35 35
#define TP_IC_ID_NUM_36 36
#define TP_IC_ID_NUM_37 37
#define TP_IC_ID_NUM_38 38
#define TP_IC_ID_NUM_39 39
#define TP_IC_ID_NUM_40 40
#define TP_IC_ID_NUM_41 41
#define TP_IC_ID_NUM_42 42
#define TP_IC_ID_NUM_43 43
#define TP_IC_ID_NUM_44 44
#define TP_IC_ID_NUM_45 45
#define TP_IC_ID_NUM_46 46
#define TP_IC_ID_NUM_47 47
#define TP_IC_ID_NUM_48 48
#define TP_IC_ID_NUM_49 49
#define TP_IC_ID_NUM_50 50
#define TP_IC_ID_NUM_51 51

#define GET_U8_FROM_NODE(dn, name, temp_val, received_val, flag) \
	if (of_property_read_u32(dn, name, &temp_val)) \
	{ \
		hwlog_err("%s:read %s fail, using default!!\n", __func__, name); \
		received_val = 0; \
		flag = 0; \
	} else { \
		received_val = (uint8_t)temp_val; \
		hwlog_info("%s:read %s suss, value %d!!\n", __func__, name, received_val); \
		flag = 1; \
	}

struct led_current_config {
	uint8_t red_curr;
	uint8_t green_curr;
	uint8_t blue_curr;
	uint8_t got_flag;
};

extern void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds);
#endif
