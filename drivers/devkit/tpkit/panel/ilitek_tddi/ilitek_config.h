#ifndef __ILITEK_CONFIG_H__
#define __ILITEK_CONFIG_H__

#define ILITEK_ICE_MODE_CMD_LEN                4
#define ILITEK_ICE_MODE_W_MAX_LEN              64

#define ILITEK_DEBUG_REG_PC_CONT               0x44008

#define CONFIG_CHIP_INFO(_chip_id, _addr1, _addr2, _addr3, _addr4,\
    _delay1, _delay2, _delay3) \
    { \
        .chip_id = _chip_id, \
        .chip_pid = 0, \
        .chip_type = 0, \
        .wdt_addr = _addr1, \
        .pid_addr = _addr2, \
        .ice_mode_addr = _addr3, \
        .ic_reset_addr = _addr4, \
        .delay_time_high = _delay1, \
        .delay_time_low = _delay2, \
        .delay_time_edge = _delay3, \
    }

typedef enum cmd_types{
    CMD_DISABLE = 0x00,
    CMD_ENABLE = 0x01,
    CMD_SEAMLESS = 0x02,
    CMD_STATUS = 0x03,
    CMD_PT_MODE = 0x04,
    CMD_ROI_DATA = 0x0E,
}cmd_types;

struct ilitek_chip_info {
    support_ic chip_id;
    u32 chip_pid;
    u32 chip_type;
    u32 wdt_addr;
    u32 pid_addr;
    u32 ice_mode_addr;
    u32 ic_reset_addr;

    int delay_time_high;
    int delay_time_low;
    int delay_time_edge;
};

struct virtual_key {
    int nId;
    u16 nX;
    u16 nY;
    int nStatus;
    int nFlag;
};

struct ilitek_tp_info {
    u16 nMaxX;
    u16 nMaxY;
    u16 nMinX;
    u16 nMinY;

    u8 nMaxTouchNum;
    u8 nMaxKeyButtonNum;

    u8 nXChannelNum;
    u8 nYChannelNum;
    u8 nHandleKeyFlag;
    u8 nKeyCount;

    u16 nKeyAreaXLength;
    u16 nKeyAreaYLength;

    /* added for protocol v5 */
    u8 self_tx_channel_num;
    u8 self_rx_channel_num;
    u8 side_touch_type;

    struct virtual_key vkeys[ILITEK_VIRTUAL_KEYS];
};

struct ilitek_config {
    bool ice_mode_enable;

    struct ilitek_chip_info *chip_info;

    struct ilitek_tp_info tp_info;

    struct ilitek_version pro_ver;
    struct ilitek_version core_ver;
    struct ilitek_version fw_ver;
};

void ilitek_config_disable_report_irq(void);
void ilitek_config_enable_report_irq(void);
int ilitek_config_check_int_status(bool high);
int ilitek_i2c_read(u8 *buf, u16 len);
int ilitek_i2c_write(u8 *buf, u16 len);
int ilitek_i2c_write_read(u8 *cmd, u8 *buf, u16 len);
u32 ilitek_config_ice_mode_read(u32 addr);
int ilitek_config_ice_mode_write(u32 addr, u32 data, u32 len);
u32 ilitek_config_read_write_onebyte(u32 addr);
int ilitek_config_ice_mode_disable(void);
int ilitek_config_ice_mode_enable(void);
int ilitek_config_sense_ctrl(cmd_types cmd);
int ilitek_config_sleep_ctrl(cmd_types cmd);
int ilitek_config_glove_ctrl(cmd_types cmd);
u8 ilitek_config_get_glove_status(void);
int ilitek_config_stylus_ctrl(cmd_types cmd);
int ilitek_config_tp_scan_mode_ctrl(cmd_types cmd);
int ilitek_config_lpwg_ctrl(cmd_types cmd);
int ilitek_config_gesture_ctrl(u8 func);
int ilitek_config_phone_cover_ctrl(cmd_types cmd);
int ilitek_config_finger_sense_ctrl(cmd_types cmd);
int ilitek_config_proximity_ctrl(cmd_types cmd);
int ilitek_config_plug_ctrl(cmd_types cmd);
int ilitek_config_set_phone_cover(u8 *pattern);
int ilitek_config_roi_ctrl(cmd_types cmd);
u8 ilitek_config_get_roi_status(void);
int ilitek_config_load_gesture_code(void);
int ilitek_config_ic_reset(void);
int ilitek_config_set_watch_dog(bool enable);
int ilitek_config_check_cdc_busy(int retrys, int delay);
void ilitek_config_ic_suspend(void);
void ilitek_config_ic_resume(void);
int ilitek_config_into_easy_wakeup(void);
int ilitek_config_outof_easy_wakeup(void);
void ilitek_parse_protocol_verison(u8 *buf, u16 len);
void ilitek_parse_core_version(u8 *buf, u16 len);
void ilitek_parse_firmware_version(u8 *buf, u16 len);
void ilitek_parse_tp_info(u8 *buf, u16 len);
void ilitek_parse_key_info(u8 *buf, u16 len);
int ilitek_config_read_ic_info(pro_ic_infos index);
int ilitek_config_get_project_id(void);
int ilitek_config_get_chip_id (u32 dts_chip_id);
u32 ilitek_config_calc_package_length(pro_fw_modes mode);
int ilitek_config_parse_package(u8 package_id);
int ilitek_config_mode_ctrl(pro_fw_modes mode, u8 *data);
int ilitek_config_init(u32 dts_chip_id);
void ilitek_config_exit(void);
#endif

