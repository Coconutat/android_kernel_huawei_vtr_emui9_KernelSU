#ifndef __ILITEK_PROTOCOL_H__
#define __ILITEK_PROTOCOL_H__

#define NA                                     0
#define ILITEK_COMMAND_LEN                     12
#define ILITEK_PACKAGE_HEAD_LEN                9

/* V5.X */
#define P5_X_READ_DATA_CTRL                    0xF6
#define P5_X_GET_TP_INFORMATION                0x20
#define P5_X_GET_KEY_INFORMATION               0x27
#define P5_X_GET_FIRMWARE_VERSION              0x21
#define P5_X_GET_PROTOCOL_VERSION              0x22
#define P5_X_GET_CORE_VERSION                  0x23
#define P5_X_MODE_CONTROL                      0xF0
#define P5_X_SET_CDC_INIT                      0xF1
#define P5_X_GET_CDC_DATA                      0xF2
#define P5_X_CDC_BUSY_STATE                    0xF3
#define P5_X_I2C_UART                          0x40

#define P5_X_FIRMWARE_UNKNOWN_MODE             0xFF
#define P5_X_FIRMWARE_DEMO_MODE                0x00
#define P5_X_FIRMWARE_TEST_MODE                0x01
#define P5_X_FIRMWARE_DEBUG_MODE               0x02
#define P5_X_FIRMWARE_I2CUART_MODE             0x03
#define P5_X_FIRMWARE_GESTURE_MODE             0x04/* add */

#define P5_X_DEMO_PACKET_ID                    0x5A
#define P5_X_DEBUG_PACKET_ID                   0xA7
#define P5_X_TEST_PACKET_ID                    0xF2
#define P5_X_GESTURE_PACKET_ID                 0xAA
#define P5_X_I2CUART_PACKET_ID                 0x7A

#define P5_X_DEMO_MODE_PACKET_LENGTH           43
#define P5_X_DEBUG_MODE_PACKET_LENGTH          1280
#define P5_X_TEST_MODE_PACKET_LENGTH           1180

/* ts kit adapter */
#define P5_X_PROJECT_ID_ADDR                   0x1D000


#define PROTOCOL_VERSION(_major, _mid, _minor) \
    { \
        .str = {_major + '0', '.', _mid + '0', '.',  _minor + '0',}, \
        .data = {_major, _mid, _minor,}, \
    }

#define PROTOCOL_CONTROL(_name, _cmd1, _cmd2, _cmd3, _cmd4, _len)\
    { \
        .name = _name, \
        .cmd = {_cmd1, _cmd2, _cmd3, _cmd4,}, \
        .len = _len, \
    }

#define PROTOCOL_IC_INFO(_name, _parse_info, _cmd, _len)\
    { \
        .name = _name, \
        .parse_info = _parse_info,\
        .cmd = _cmd, \
        .len = _len, \
    }

#define PROTOCOL_FIRMWARE_MODE(_name, _mode, _id, _len)\
    { \
        .name = _name, \
        .mode = _mode, \
        .package_id = _id, \
        .len = _len, \
    }

#define PROTOCOL_REPORT_PACKAGE(_func1, _func2, _func3)\
    {\
        .calc_packget_length = _func1,\
        .parse_packget = _func2,\
        .mode_control = _func3,\
    }

typedef enum pro_vers{
    ILITEK_VER_5_0_0,
    ILITEK_VER_5_1_0,
    ILITEK_VER_5_2_0,
    ILITEK_VER_5_3_0,
    ILITEK_VER_5_4_0,
    ILITEK_VER_NUMS,
    ILITEK_VER_UNKNOWN = 0xFF,
}pro_vers;

typedef enum pro_ctrls{
    ILITEK_SENSE_CTRL,
    ILITEK_SLEEP_CTRL,
    ILITEK_GLOVE_CTRL,
    ILITEK_STYLUS_CTRL,
    ILITEK_SCAN_MODE_CTRL,
    ILITEK_LPWG_CTRL,
    ILITEK_GESTURE_CTRL,
    ILITEK_PHONE_COVER_CTRL,
    ILITEK_FINGER_SENSE_CTRL,
    ILITEK_PROXIMITY_CTRL,
    ILITEK_PLUG_CTRL,
    ILITEK_PHONE_COVER_WINDOW_CTRL,
    ILITEK_ROI_CTRL,
    ILITEK_CTRL_NUMS,
    ILITEK_CTRL_UNKNOWN = 0xFF,
}pro_ctrls;

struct protocol_control {
    const char *name;
    u8 cmd[ILITEK_COMMAND_LEN];
    u32 len;
};

typedef enum pro_ic_infos{
    ILITEK_PRO_INFO,
    ILITEK_FW_INFO,
    ILITEK_CORE_INFO,
    ILITEK_TP_INFO,
    ILITEK_KEY_INFO,
    ILITEK_INFO_NUMS,
    ILITEK_INFO_UNKNOWN = 0xFF,
}pro_ic_infos;

struct protocol_ic_info {
    const char *name;
    u8 cmd;
    u32 len;
    void (*parse_info)(u8 *, u32);
};

typedef enum pro_fw_modes{
    ILITEK_DEMO_MODE,
    ILITEK_TEST_MODE,
    ILITEK_DEBUG_MODE,
    ILITEK_I2C_UART_MODE,
    ILITEK_GESTURE_MODE,
    ILITEK_MODE_NUMS,
    ILITEK_UNKNOWN_MODE = 0xFF,
}pro_fw_modes;

struct protocol_firmware_mode {
    const char *name;
    u8 mode;
    u8 package_id;
    u32 len;
};

struct protocol_funcs {
    u32 (*calc_packget_length)(pro_fw_modes);
    int (*parse_packget)(u8);
    int (*mode_control)(pro_fw_modes, u8 *);
};

struct protocol_test {
    /* MP Test with cdc commands */
    u8 cmd_cdc;
    u8 cmd_get_cdc;
    u32 cdc_len;

    u8 cm_data;
    u8 cs_data;

    u8 rx_short;
    u8 rx_open;
    u8 tx_short;

    u8 mutual_dac;
    u8 mutual_bg;
    u8 mutual_signal;
    u8 mutual_no_bk;
    u8 mutual_bk_dac;
    u8 mutual_has_bk;
    u16 mutual_has_bk_16;

    u8 self_dac;
    u8 self_bk_dac;
    u8 self_has_bk;
    u8 self_no_bk;
    u8 self_signal;
    u8 self_bg;

    u8 key_dac;
    u8 key_has_bk;
    u8 key_bg;
    u8 key_no_bk;
    u8 key_open;
    u8 key_short;

    u8 st_no_bk;
    u8 st_open;
    u8 st_dac;
    u8 st_has_bk;
    u8 st_bg;

    u8 tx_rx_delta;

    u8 trcrq_pin;
    u8 resx2_pin;
    u8 mutual_integra_time;
    u8 self_integra_time;
    u8 key_integra_time;
    u8 st_integra_time;
    u8 peak_to_peak;

    u8 get_timing;
    u8 doze_p2p;
    u8 doze_raw;
};

struct ilitek_protocol {
    struct ilitek_config *cfg;

    struct ilitek_version *ver;

    /* tp control */
    struct protocol_control *ctrls;

    /* get ic information */
    u8 cmd_read_ctrl;
    struct protocol_ic_info *infos;

    /* tp firmware mode*/
    u8 cmd_mode_ctrl;
    u8 cmd_i2cuart;
    struct protocol_firmware_mode *modes;

    u8 cmd_cdc_busy;
    u32 addr_project_id;

    /* tp report data*/
    struct protocol_funcs *funcs;

    /* tp test */
    struct protocol_test *test;
};

extern struct ilitek_version pro_versions[ILITEK_VER_NUMS];
int ilitek_protocol_update(struct ilitek_version *p_target_ver);
int ilitek_protocol_init(pro_vers index);
void ilitek_protocol_exit(void);
#endif
