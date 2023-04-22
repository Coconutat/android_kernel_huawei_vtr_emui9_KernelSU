#ifndef __ILITEK_REPORT_H__
#define __ILITEK_REPORT_H__

/* The example for the gesture virtual keys */
typedef enum ilitek_gestures{
    ILITEK_GESTURE_DOUBLECLICK = 0x58,
    ILITEK_GESTURE_UP = 0x60,
    ILITEK_GESTURE_DOWN = 0x61,
    ILITEK_GESTURE_LEFT = 0x62,
    ILITEK_GESTURE_RIGHT = 0x63,
    ILITEK_GESTURE_M = 0x64,
    ILITEK_GESTURE_W = 0x65,
    ILITEK_GESTURE_C = 0x66,
    ILITEK_GESTURE_E = 0x67,
    ILITEK_GESTURE_V = 0x68,
    ILITEK_GESTURE_O = 0x69,
    ILITEK_GESTURE_S = 0x6A,
    ILITEK_GESTURE_Z = 0x6B,
    ILITEK_GESTURE_NUMS,
}ilitek_gestures;

struct ilitek_report_finger_data{
    u16 id;
    u16 x;
    u16 y;
    u16 pressure;
};
struct ilitek_report{
    struct ilitek_protocol *pro;
    struct ilitek_protocol *cfg;

    pro_fw_modes fw_mode;
    u8 package_id;
    u8 *package_data;
    u32 package_len;

    u8 cur_finger_num;
    u8 last_finger_num;
    u8 roi_data[ROI_DATA_READ_LENGTH];
    struct ilitek_report_finger_data fingers[ILITEK_TOUCSCEEN_FINGERS];
};
int ilitek_report_data(struct ts_fingers *p_info);
int ilitek_report_init(void);
void ilitek_report_exit(void);
#endif