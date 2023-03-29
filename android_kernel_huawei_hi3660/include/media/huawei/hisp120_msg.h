/*
 * hisp120_msg.c
 *
 * header file for hisp120.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * lixiuhua <aimee.li@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef HISP120_MSG_H_INCLUDED
#define HISP120_MSG_H_INCLUDED

#define MAX_INPUT_STREAM_NUM (2)
#define MAX_STREAM_NUM       (10)
#define ARSR_REQ_OUT_NUM     (2)
#define NAME_LEN             (32)
#define PARAS_LEN            (400)
#define EXT_ACK_PARAS_LEN    (68)
#define EVENT_PARAMS_LEN     (400)
#define PIPELINE_COUNT       (2)

/* based on msg len is 464 */
#define MAX_SET_ISP_NR  1//50//53//59
#define MAX_GET_ISP_NR  1//100//108//119
#define MAX_SET_I2C_NR  1//35//35//39
#define MAX_GET_I2C_NR  1//50//53//59

typedef enum
{
    PIXEL_FMT_RAW10         = 0,
    PIXEL_FMT_RAW12         = 1,
    PIXEL_FMT_RAW14         = 2,
    PIXEL_FMT_JPEG          = 3,
    PIXEL_FMT_YUV422_UYVY   = 4,  // default jpgenc format
    PIXEL_FMT_YUV420_NV12   = 5,
    PIXEL_FMT_YUV420_NV21   = 6,
    PIXEL_FMT_YUV422_VYUY   = 7,
    PIXEL_FMT_YUV422_YUYV   = 8,
    PIXEL_FMT_YUV422_YVYU   = 9,
    PIXEL_FMT_MONOCHROME    = 10,
    PIXEL_FMT_Y8            = 11,
    PIXEL_FMT_YUV422_SP     = 12,
} pix_format_e;

typedef enum
{
    STREAM_REP_YUV_IN           = 0,
    STREAM_REP_RAW_IN           = 1,
    STREAM_ISP_YUV_OUT_PREVIEW  = 2,
    STREAM_ISP_YUV_OUT_VIDEO    = 3,
    STREAM_REP_YUV_OUT          = 4,
    STREAM_ISP_YUV_OUT_TINY     = 5,
    STREAM_ISP_RAW_OUT          = 6,
    STREAM_RAW_OUT              = 7,
    STREAM_UT_OUT               = 8,
    STREAM_ISP_PD               = 9,
    STREAM_POS_MAX,
} stream_pos_e;

typedef enum
{
    STREAM_TYPE_INPUT,
    STREAM_TYPE_OUTPUT,
} stream_type_e;

typedef enum
{
    REGISTER_TYPE_ISP,
    REGISTER_TYPE_I2C,
} register_type_e;

struct hi_list_head
{
    struct hi_list_head *next, *prev;
};

typedef enum
{
    /* Request items. */
    COMMAND_QUERY_CAPABILITY = 0x1000,
    COMMAND_ACQUIRE_CAMERA,
    COMMAND_RELEASE_CAMERA,
    COMMAND_USECASE_CONFIG,
    COMMAND_GET_OTP,
    COMMAND_REQUEST,
    COMMAND_JPEG_ENCODE,
    COMMAND_MAP_BUFFER,
    COMMAND_UNMAP_BUFFER,
    COMMAND_CALIBRATION_DATA,
    COMMAND_SET_ISP_REGISTERS,
    COMMAND_GET_ISP_REGISTER,
    COMMAND_SET_IIC_REGISTER,
    COMMAND_GET_IIC_REGISTER,
    COMMAND_TEST_CASE_INTERFACE,
    COMMAND_FLUSH,
    COMMAND_EXTEND_SET,
    COMMAND_EXTEND_GET,
    COMMAND_INV_TLB,
    COMMAND_QUERY_OIS_UPDATE,
    COMMAND_OIS_UPDATE,
    COMMAND_QUERY_LASER,
    COMMAND_ACQUIRE_LASER,
    COMMAND_RELEASE_LASER,
    COMMAND_ACQUIRE_DEPTHISP,
    COMMAND_RELEASE_DEPTHISP,
    COMMAND_GET_API_VERSION,
    COMMAND_STREAM_ON,
    COMMAND_STREAM_OFF,
    COMMAND_ARSR_REQUEST,
    COMMAND_MOTION_SENSOR_MAP_REQUEST,
    COMMAND_MEM_POOL_INIT_REQUEST,//FIXME:DIFFER, not in isp, for compilation
    COMMAND_MEM_POOL_DEINIT_REQUEST,//FIXME:DIFFER, not in isp, for compilation
    COMMAND_ISP_CPU_POWER_OFF_REQUEST,

    /* Response items. */
    QUERY_CAPABILITY_RESPONSE = 0x2000,
    ACQUIRE_CAMERA_RESPONSE,
    RELEASE_CAMERA_RESPONSE,
    USECASE_CONFIG_RESPONSE,
    GET_OTP_RESPONSE,
    REQUEST_RESPONSE,
    JPEG_ENCODE_RESPONSE,
    MAP_BUFFER_RESPONSE,
    UNMAP_BUFFER_RESPONSE,
    CALIBRATION_DATA_RESPONSE,
    SET_ISP_REGISTERS_RESPONSE,
    GET_ISP_REGISTER_RESPONSE,
    SET_IIC_REGISTER_RESPONSE,
    GET_IIC_REGISTER_RESPONSE,
    TEST_CASE_RESPONSE,
    FLUSH_RESPONSE,
    EXTEND_SET_RESPONSE,
    EXTEND_GET_RESPONSE,
    INV_TLB_RESPONSE,
    QUERY_OIS_UPDATE_RESPONSE,
    OIS_UPDATE_RESPONSE,
    QUERY_LASER_RESPONSE,
    ACQUIRE_LASER_RESPONSE,
    RELEASE_LASER_RESPONSE,
    ACQUIRE_DEPTHISP_RESPONSE,
    RELEASE_DEPTHISP_RESPONSE,
    GET_ISP_VERSION_RESPONSE,
    STREAM_ON_RESPONSE,
    STREAM_OFF_RESPONSE,
    ARSR_REQUEST_RESPONSE,
    MOTION_SENSOR_MAP_RESPONSE,
    MEM_POOL_INIT_RESPONSE, //FIXME:DIFFER, not in isp, for compilation
    MEM_POOL_DEINIT_RESPONSE,//FIXME:DIFFER, not in isp, for compilation
    ISP_CPU_POWER_OFF_RESPONSE,
    /* Event items sent to AP. */
    MSG_EVENT_SENT           = 0x3000,
} api_id_e;

typedef enum
{
    PRIMARY_CAMERA = 0,
    FRONT_CAMERA,
    SECONDARY_CAMERA,
    THIRD_CAMERA,
    IR_CAMERA,
} camera_id_t;
typedef enum _map_pool_usage_e{
    MAP_POOL_USAGE_FW = 0,
    MAP_POOL_USAGE_ISP_FW,
    MAP_POOL_USAGE_ISP,
    MAP_POOL_USAGE_MAX,
} map_pool_usage_e;

typedef struct _msg_ack_get_api_version_t
{
    unsigned int major_version;
    unsigned int minor_version;
} msg_ack_get_api_version_t;

typedef struct _msg_req_query_capability_t
{
    unsigned int cam_id;
    unsigned int csi_index;
    unsigned int i2c_index;
    char         product_name[NAME_LEN];
    char         sensor_name[NAME_LEN];
    unsigned int input_settings_buffer;
} msg_req_query_capability_t;

typedef struct _msg_ack_query_capability_t
{
    unsigned int cam_id;
    char         product_name[NAME_LEN];
    char         sensor_name[NAME_LEN];
    unsigned int output_metadata_buffer;
    int status;
    int          version;
} msg_ack_query_capability_t;

typedef struct _msg_req_query_laser_t
{
    unsigned int i2c_index;
    char         product_name[NAME_LEN];
    char         name[NAME_LEN];
} msg_req_query_laser_t;

typedef struct _laser_spad_t
{
    unsigned int    ref_spad_count;
    unsigned char   is_aperture_spads;
}laser_spad_t;

typedef struct _msg_ack_query_laser_t
{
    char           name[NAME_LEN];
    unsigned char  revision;
    int            status;
    laser_spad_t   spad;
} msg_ack_query_laser_t;

typedef struct _msg_req_acquire_camera_t
{
    unsigned int cam_id;
    unsigned int csi_index;
    unsigned int i2c_index;
    char         sensor_name[NAME_LEN];
    char         product_name[NAME_LEN];
    unsigned int input_otp_buffer;
    unsigned int input_calib_buffer;
    unsigned int buffer_size;
    unsigned int info_buffer;
    unsigned int info_count;
    unsigned int factory_calib_buffer;
} msg_req_acquire_camera_t;

typedef struct _msg_ack_acquire_camera_t
{
    unsigned int cam_id;
    char         sensor_name[NAME_LEN];
} msg_ack_acquire_camera_t;

typedef struct _msg_req_release_camera_t
{
    unsigned int cam_id;
} msg_req_release_camera_t;

typedef struct _msg_ack_release_camera_t
{
    unsigned int cam_id;
} msg_ack_release_camera_t;

typedef struct _fov_info
{
    float          x;
    float          y;
    float          width;
    float          height;
    float          angle;
} fov_info_t;

typedef struct _laser_fov_t
{
    float          x;
    float          y;
    float          width;
    float          height;
    float          angle;
} laser_fov_t;

typedef struct _laser_dmax_t
{
    unsigned int dmax_range;
    unsigned int dmax_rate;
} laser_dmax_t;

typedef struct _laser_ref_t
{
    unsigned char ref_vhvsetting;
    unsigned char ref_phasecal;
} laser_ref_t;

typedef struct _msg_req_acquire_laser_t
{
    unsigned int    i2c_index;
    char            product_name[NAME_LEN];
    char            name[NAME_LEN];
    int             offset;
    int             xtalk;
    laser_fov_t     fov_info;
    laser_spad_t    spad;
    laser_ref_t     ref;
    laser_dmax_t    dmax;
} msg_req_acquire_laser_t;

typedef struct _msg_ack_acquire_laser_t
{
    char           name[NAME_LEN];
    unsigned char  revision;
    int            status;
} msg_ack_acquire_laser_t;

typedef struct _msg_req_release_laser_t
{
    unsigned int i2c_index;
} msg_req_release_laser_t;

typedef struct _msg_ack_release_laser_t
{
    unsigned int i2c_index;
} msg_ack_release_laser_t;

typedef struct _msg_req_acquire_depthisp_t
{
    unsigned int  i2c_index;
    unsigned char chip_type;
    char          product_name[NAME_LEN];
    char          name[NAME_LEN];
} msg_req_acquire_depthisp_t;

typedef struct _msg_ack_acquire_depthisp_t
{
    char         name[NAME_LEN];
    int          status;
} msg_ack_acquire_depthisp_t;

typedef struct _msg_req_release_depthisp_t
{
    unsigned int i2c_index;
} msg_req_release_depthisp_t;

typedef struct _msg_ack_release_depthisp_t
{
    unsigned int i2c_index;
} msg_ack_release_depthisp_t;

typedef struct _stream_config_t
{
    unsigned int type;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int format;
    unsigned int secure;
} stream_config_t;

typedef struct _msg_req_usecase_config_t
{
    unsigned int cam_id;
    unsigned int extension;
    unsigned int stream_nr;
    unsigned int scene;
    stream_config_t stream_cfg[MAX_STREAM_NUM];
    char            time[32];
} msg_req_usecase_config_t;

typedef struct _msg_ack_usecase_config_t
{
    unsigned int cam_id;
    int          status;
    unsigned int sensor_width;
    unsigned int sensor_height;
} msg_ack_usecase_config_t;

typedef struct _msg_req_stream_on_t
{
    unsigned int cam_id;
} msg_req_stream_on_t;

typedef struct _msg_req_stream_off_t
{
    unsigned int cam_id;
} msg_req_stream_off_t;

typedef struct _msg_ack_stream_on_t
{
    unsigned int cam_id;
    int          status;
} msg_ack_stream_on_t;

typedef struct _msg_ack_stream_off_t
{
    unsigned int cam_id;
    int          status;
} msg_ack_stream_off_t;

typedef struct _msg_req_get_otp_t
{
    unsigned int cam_id;
    unsigned int csi_index;
    unsigned int i2c_index;
    char         sensor_name[NAME_LEN];
    unsigned int input_otp_buffer;
    unsigned int buffer_size;
} msg_req_get_otp_t;

typedef struct _msg_ack_get_otp_t
{
    unsigned int cam_id;
    char         sensor_name[NAME_LEN];
    unsigned int output_otp_buffer;
    unsigned int buffer_size;
    int          status;
} msg_ack_get_otp_t;

typedef struct _stream_info_t
{
    unsigned int buffer;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int format;
    unsigned int valid;
    unsigned int frame_num;
} stream_info_t;

typedef struct _msg_req_request_t
{
    unsigned int cam_id;
    unsigned int num_targets;
    unsigned int target_map;
    unsigned int frame_number;
    unsigned int buf[MAX_STREAM_NUM];
    unsigned int input_setting_buffer;
    unsigned int output_metadata_buffer;
    unsigned int output_cap_info_buffer;
} msg_req_request_t;

typedef struct _msg_ack_request_t
{
    unsigned int cam_id;
    unsigned int num_targets;
    unsigned int target_map;
    unsigned int frame_number;
    stream_info_t stream_info[MAX_STREAM_NUM];
    unsigned int input_setting_buffer;
    unsigned int output_metadata_buffer;
    unsigned int timestampL;
    unsigned int timestampH;
    unsigned int status;
    unsigned int output_cap_info_buffer;
} msg_ack_request_t;

typedef struct _msg_req_arsr_request_t
{
    unsigned int  cam_id;
    unsigned int  frame_number;
    stream_info_t input_stream_info;
    stream_info_t output_stream_info;
} msg_req_arsr_request_t;

typedef struct _msg_ack_arsr_request_t
{
    unsigned int  cam_id;
    unsigned int  frame_number;
    stream_info_t input_stream_info;
    stream_info_t output_stream_info;
    unsigned int  status;
} msg_ack_arsr_request_t;

typedef struct _map_pool_desc_t{
    unsigned int start_addr;
    unsigned int ion_iova;
    unsigned int size;
    unsigned int usage;
} map_pool_desc_t;
typedef struct _msg_req_map_buffer_t
{
    unsigned int cam_id;
    unsigned int pool_count;
    map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_map_buffer_t;

typedef struct _msg_ack_map_buffer_t
{
    unsigned int cam_id;
    int status;
} msg_ack_map_buffer_t;

typedef struct _msg_req_unmap_buffer_t
{
    unsigned int cam_id;
    unsigned int buffer;
} msg_req_unmap_buffer_t;

typedef struct _msg_ack_unmap_buffer_t
{
    unsigned int cam_id;
    int status;
} msg_ack_unmap_buffer_t;

typedef struct _msg_req_cal_data_t
{
    unsigned int cam_id;
    unsigned int buffer_size;
    unsigned int cal_data_buffer;
} msg_req_cal_data_t;

typedef struct _msg_ack_cal_data_t
{
    unsigned int cam_id;
    int status;
} msg_ack_cal_data_t;

typedef struct _isp_reg_info_t
{
    unsigned int register_address;
    unsigned int register_value;
} isp_reg_info_t;

typedef struct _msg_req_set_isp_regs_t
{
    unsigned int register_type;
    unsigned int register_count;
    isp_reg_info_t reg_info[MAX_SET_ISP_NR];
} msg_req_set_isp_regs_t;

typedef struct _msg_ack_set_isp_regs_t
{
    int status;
} msg_ack_set_isp_regs_t;

typedef struct _msg_req_get_isp_regs_t
{
    unsigned int register_count;
    unsigned int register_address[MAX_GET_ISP_NR];
} msg_req_get_isp_regs_t;

typedef struct _msg_ack_get_isp_regs_t
{
    int status;
    unsigned int register_count;
    unsigned int register_value[MAX_GET_ISP_NR];
} msg_ack_get_isp_regs_t;

typedef struct _i2c_reg_set_info_t
{
    unsigned int  register_address;
    unsigned int  register_value;
    unsigned char length;                   /**< value length */
} i2c_reg_set_info_t;

typedef struct _msg_req_set_i2c_regs_t
{
    unsigned int register_type;
    unsigned int slave_address;
    unsigned int register_count;
    i2c_reg_set_info_t reg_info[MAX_SET_I2C_NR];
} msg_req_set_i2c_regs_t;

typedef struct _msg_ack_set_i2c_regs_t
{
    int status;
} msg_ack_set_i2c_regs_t;

typedef struct _i2c_reg_get_info_t
{
    unsigned int register_address;
    unsigned char length;                   /**< request getting the len of the register value */
} i2c_reg_get_info_t;

typedef struct _msg_req_get_i2c_regs_t
{
    unsigned int register_type;
    unsigned int slave_address;
    unsigned int register_count;
    i2c_reg_get_info_t reg_info[MAX_GET_I2C_NR];
} msg_req_get_i2c_regs_t;

typedef struct _msg_ack_get_i2c_regs_t
{
    int status;
    unsigned int register_count;
    unsigned int register_value[MAX_GET_I2C_NR];
} msg_ack_get_i2c_regs_t;

typedef struct _msg_req_test_case_interface_t
{
    unsigned int case_handle;
    char         case_name[64];
} msg_req_test_case_interface_t;

typedef struct _msg_ack_test_case_interface_t
{
    int   status;
    int   flag;
} msg_ack_test_case_interface_t;

typedef struct _msg_req_flush_t
{
    unsigned int cam_id;
    unsigned int is_hotplug;
} msg_req_flush_t;

typedef struct _msg_ack_flush_t
{
    int status;
} msg_ack_flush_t;

typedef struct _msg_req_inv_tlb_t
{
    int flag;
} msg_req_inv_tlb_t;

typedef struct _msg_ack_inv_tlb_t
{
    int status;
} msg_ack_inv_tlb_t;

typedef struct _msg_req_query_ois_update_t
{
    unsigned int cam_id;
    char sensor_name[32];
} msg_req_query_ois_update_t;

typedef struct _msg_ack_query_ois_update_t
{
    unsigned int cam_id;
    int status;
} msg_ack_query_ois_update_t;

typedef struct _msg_req_ois_update_t
{
    unsigned int cam_id;
    char sensor_name[32];
    unsigned int input_ois_buffer;
    unsigned int input_ois_buffer_size;
} msg_req_ois_update_t;

typedef struct _msg_ack_ois_update_t
{
    unsigned int cam_id;
    int status;
} msg_ack_ois_update_t;

typedef enum
{
    MOTION_SENSOR_ACCEL = 1,
    MOTION_SENSOR_GYRO = 4,
    MOTION_SENSOR_LINEAR_ACCEL = 10,
}motion_sensor_type_t;

typedef struct _msg_req_motion_sensor_map_t
{
    motion_sensor_type_t motion_sensor_type;
    unsigned int input_motion_sensor_mem_buffer;
    unsigned int input_motion_sensor_mem_buffer_size;
} msg_req_motion_sensor_map_t;

typedef struct _msg_ack_motion_sensor_map_t
{
    motion_sensor_type_t motion_sensor_type;
    int status;
} msg_ack_motion_sensor_map_t;

typedef enum
{
    SUBCMD_ES_WATCHDOG = 0,
    SUBCMD_ES_SYNCLOG = 1,
    SUBCMD_SET_M_DF_FLAG = 2,
    SUBCMD_SET_DF_TUNING = 3,
    SUBCMD_SET_COLOR_BAR = 4,
    SUBCMD_ENABLE_TNR = 5,
    SUBCMD_ENABLE_DIS = 6,
    SUBCMD_ENABLE_FD = 7,
    SUBCMD_SET_FACE = 8,
    SUBCMD_AE_ANTIBANDING_MODE = 9,
    SUBCMD_AE_EXPOSURE_COMPENSATION = 10, //10
    SUBCMD_AE_LOCK = 11,
    SUBCMD_AE_MODE = 12,
    SUBCMD_AE_REGIONS = 13,
    SUBCMD_AE_TARGET_FPS_RANGE = 14,
    SUBCMD_AE_PRECAPTURE_TRIGGER = 15,
    SUBCMD_AF_MODE = 16,
    SUBCMD_AF_REGIONS = 17,
    SUBCMD_AF_TRIGGER = 18,
    SUBCMD_FLASH_MODE = 19,
    SUBCMD_AWB_LOCK = 20, //20
    SUBCMD_AWB_MODE = 21,
    SUBCMD_AWB_REGIONS = 22,
    SUBCMD_SCALER_CROP_REGION = 23,
    SUBCMD_START_CAPTURE = 24,
    SUBCMD_STOP_CAPTURE = 25,
    SUBCMD_SET_DEBUG_OPEN = 26,
    SUBCMD_SET_FLASH_RATIO = 27,
    SUBCMD_SET_MANUAL_FOCUS_MODE = 28,
    SUBCMD_SET_VCM_CODE = 29,
    SUBCMD_SET_BANDING_MSG = 30, //30
    SUBCMD_SET_EXPO_TIME = 31,
    SUBCMD_SET_ISO = 32,
    SUBCMD_SET_ADGAIN = 33,
    SUBCMD_SET_MANUAL_AWB = 34,
    SUBCMD_SET_SCENE_MODE = 35,
    SUBCMD_SET_OVER_EXPOSURE = 36,
    SUBCMD_SET_DEBUG_SHADING = 37,
    SUBCMD_RESUME_3A = 38,
    SUBCMD_SET_CAPTURE_SHARPNESS = 39,
    SUBCMD_SET_CAPTURE_RAWNF = 40, //40
    SUBCMD_SET_CAPTURE_YUVNF = 41,
    SUBCMD_SET_CAPTURE_GCD = 42,
    SUBCMD_SET_SALIENCY_RESULT = 43,
    SUBCMD_SET_PANORAMA_MODE = 44,
    SUBCMD_SET_PANORAMA_LOCK = 45,
    SUBCMD_SET_FAST_SNAPSHOT = 46,
    SUBCMD_SET_SATURATION = 47,
    SUBCMD_SET_CONTRAST = 48,
    SUBCMD_SET_BRIGHTNESS = 49,
    SUBCMD_SET_GSENSOR_INFO = 50, //50
    SUBCMD_SET_7CM_FOCUS_MODE = 51,
    SUBCMD_SET_7CM_FOCUS_REGIONS = 52,
    SUBCMD_SET_BURST_COUNT = 53,
    SUBCMD_SET_TARGET_TRACKING = 54,
    SUBCMD_SET_OIS_MODE = 55,
    SUBCMD_ENABLE_FBCD = 56,
    SUBCMD_TRY_AE = 57,
    SUBCMD_SET_CAPTURE_AE = 58,
    SUBCMD_SET_TARGET_LUMINANCE = 59,
    SUBCMD_GET_CAPTURE_VALID_INFO = 60, //60
    SUBCMD_CAMERA_MODE = 61,
    SUBCMD_SET_OTP_CALIBRATION = 62,
    SUBCMD_SET_MOTIONSENSOR_INFO = 63,
    SUBCMD_PDAF_MMI_TEST_ENABLE = 64,
    SUBCMD_SET_PDAF_MMI_PARAM = 65,
    SUBCMD_SET_EQUIP_MMI_MODE = 66,
    SUBCMD_SET_FLASH_MMI_MODE = 67,
    SUBCMD_SET_MMI_TEST_GAMMA = 68,
    SUBCMD_SET_PROFESSION_CAMERA = 69,
    SUBCMD_SET_METERING_MODE = 70, //70
    SUBCMD_SET_WB_VALUE = 71,
    SUBCMD_LPD_ENABLE = 72,
    SUBCMD_SET_AE_GAIN = 73,
    SUBCMD_AWB_ILLUMINANT = 74,
    SUBCMD_AWB_DAMPING_PARAM = 75,
    SUBCMD_AWB_CURRENT_WP = 76,
    SUBCMD_GAMMA_LOCK = 77,
    SUBCMD_GAMMA_MODE = 78,
    SUBCMD_GAMMA_CURVE = 79,
    SUBCMD_GAMMA_DAMPING_PARAM = 80,//80
    SUBCMD_LSC_ENABLE = 81,
    SUBCMD_DPCC_ENABLE = 82,
    SUBCMD_YUVNF_ENABLE = 83,
    SUBCMD_SHARPNESS_ENABLE = 84,
    SUBCMD_RAWNF_ENABLE = 85,
    SUBCMD_DRC_ENABLE = 86,
    SUBCMD_DRC_MODE = 87,
    SUBCMD_DRC_DAMPING_PARAM = 88,
    SUBCMD_CC_ENABLE = 89,
    SUBCMD_CC_MODE = 90,//90
    SUBCMD_CC_PARAM = 91,
    SUBCMD_CC_MATRIX = 92,
    SUBCMD_SMART_AE_SET_EXPO_COMPENSATION = 93,
    SUBCMD_SET_OIS_MMI_MODE = 94,
    SUBCMD_RESUME_VCM_CODE = 95,
    SUBCMD_PROF_FOCUS_ASSIST_MODE = 96,
    SUBCMD_YUV_CONTRAST_RESULT = 97,
    SUBCMD_SET_FACE_INFO = 98,
    SUBCMD_LASER_CONTROL = 99,
    SUBCMD_USECASE_INITIAL = 100,//100
    SUBCMD_SET_DC_MMI_ENABLE = 101,
    SUBCMD_SET_AF_MESSAGE = 102,
    SUBCMD_CAMERA_FIRMWARE_PROPERTY = 103,
    SUBCMD_SET_DUAL_CAM_SHELTERED = 104,
    SUBCMD_DEPTH_INFO = 105,
    SUBCMD_SET_MMI_7CM_PARAM = 106,
    SUBCMD_AE_YUV_INFO = 107,
    SUBCMD_SET_FILL_RAW = 108,
    SUBCMD_SET_ANDROID_ISO = 109,
    SUBCMD_SET_ANDROID_EXPO_TIME = 110, //110
    SUBCMD_SET_FOCUS_DISTANCE = 111,
    SUBCMD_SET_TONEMAP_MODE = 112,
    SUBCMD_SET_TONEMAP_CURVE = 113,
    SUBCMD_SET_APERTURE_MODE = 114,
    SUBCMD_SET_STD_RAW = 115,
    SUBCMD_SET_CAPFLASH_ON = 116,
    SUBCMD_SET_AFC_DATA = 117,
    SUBCMD_SET_CC_MODE = 118,
    SUBCMD_SET_CC_TRANSFORM = 119,
    SUBCMD_SET_CC_GAINS = 120,         //120
    SUBCMD_SET_CONTROL_MODE = 121,
    SUBCMD_SET_AF_DIRECT_TRANS_BASE = 122,
    SUBCMD_SET_CURVE_MODE = 123,
    SUBCMD_SET_RGB2YUV_MODE = 124,
    SUBCMD_SET_RGB2YUV_PARAM = 125,
    SUBCMD_SET_AF_OTP_CALIB_DATA = 126,
    SUBCMD_SET_SATURATION_COMPENSATION = 127,
    SUBCMD_SET_LOG_LEVEL = 128,
    SUBCMD_SET_AFC_MMI_ENABLE = 129,
    SUBCMD_SET_STREAM_MODE = 130,
    SUBCMD_GET_LCD_STATE = 131,
    SUBCMD_SET_LOG_MODULE = 132,
    SUBCMD_SET_RAW_DS = 133,
    SUBCMD_SET_YUV_DS = 134,
    SUBCMD_SET_ISP_ALGO_STATUS = 135,
    SUBCMD_SET_LSC_MODE  = 136,
    SUBCMD_SET_PLATFORM_ID = 137,
    SUBCMD_SET_FLASH_MODE = 138,
    SUBCMD_SET_LASER_DIRTY = 139,
    SUBCMD_SET_FACE_LANDMARKS  = 140,
    SUBCMD_SET_AE_ALWAYS_CONVERGE = 141,
    SUBCMD_SET_PDAF_RESULT = 142,
    SUBCMD_SET_MOTION_INFO = 143,
    SUBCMD_SET_REDUCE_EXPOSURE = 144,
    SUBCMD_SET_APERTURE_MONO = 145,
    SUBCMD_SET_SELF_LEARN_DATA =146,
    SUBCMD_SET_SFR_TEST_DATA = 147,
    SUBCMD_SET_SCE_HUE_GAIN = 148,
    SUBCMD_SET_RAW2YUV_INFO = 149,
    SUBCMD_SET_DMAP_INFO = 150,//FIXME:DIFFER, not in isp, for compilation

    SUBCMD_SET_OPTICAL_ZOOM_SWITCH = 151,
    SUBCMD_SET_SOFTLIGHT_MODE  = 156,

    SUBCMD_SET_PDALGO_ENABLE = 157,
    SUBCMD_SET_PD_INFO = 158,
    SUBCMD_SET_SWPD_KEY = 159,
    SUBCMD_GET_SWPD_KEY = 160,
    SUBCMD_GET_SENSOR_COORD = 161,
    SUBCMD_SET_AE_SENSOR_VERIFY_MODE = 162,
    SUBCMD_STREAM_REF_VALUE = 163,
    SUBCMD_SET_AF_OTPSTART_MODE = 164,
    SUBCMD_FOV_SCALE_RATIO_STATUS = 165,
    SUBCMD_SET_PD_OFFSET_CALIB_MMI_ENABLE = 166,
    SUBCMD_SET_PD_OFFSET_CALIB_RESULT = 167,
    SUBCMD_SET_FORCE_CAF = 168,
    SUBCMD_SET_RAW_READBACK_ADDR = 169,
    //front camera awb
    SUBCMD_SET_AP_AWB_GAIN = 170,
    SUBCMD_SET_AP_AWB_WP = 171,
    SUBCMD_SET_AP_AWB_COLOR_ZONE = 172,
    SUBCMD_SET_AP_AWB_INIT_PARAM = 173,
    SUBCMD_SD_RESULTS = 174,
    SUBCMD_MANUAL_MAX_EXPO_TIME = 175,
    SUBCMD_SET_AFSTAT_ALGO_RESULT = 176,
    SUBCMD_SET_LCD_FLASH_MODE = 177,
    SUBCMD_SET_AE_FULLSIZE_BINNING_CHANGE = 181,
    SUBCMD_MAX,
    SUBCMD_SET_HFBC_ALIGMENT, // not support in hisp120, for common code compilation
} extendset_info_e;

typedef enum
{
    SUBCMD_EG_LLT,
    SUBCMD_EG_ST,
    SUBCMD_GET_M_DF_FLAG,
    SUBCMD_GET_DF_TUNING,
} extendget_info_e;

typedef enum
{
    EVENT_ERR_CODE = 0,
    EVENT_SHUTTER,
    EVENT_INTERRUPT,
    EVENT_FLASH,
    EVENT_AF,
    EVENT_AF_MMI_DEBUG,
    EVENT_AF_DIRECT_TRANS_BASE,
    EVENT_AF_OTP_CALIB_DATA,
    EVENT_AF_SELF_LEARN_DATA,
    EVENT_AF_STAT_INFO,
} event_info_e;

typedef struct _msg_subreq_es_watchdog_t
{
    unsigned int enable_watchdog;
    unsigned int timeout;
} msg_subreq_es_watchdog_t;

typedef struct _msg_subreq_laser_calib_t
{
    unsigned int i2c_index;
    unsigned int offset;
    unsigned int crosstalk;
} msg_subreq_laser_calib_t;

typedef struct _msg_subreq_es_synclog_t
{
    unsigned int reserved;
} msg_subreq_es_synclog_t;

typedef struct _msg_subreq_eg_llt_t
{
    /*TODO*/
} msg_subreq_eg_llt_t;

typedef struct _msg_subreq_eg_st_t
{
    /*TODO*/
} msg_subreq_eg_st_t;

typedef struct _msg_subreq_optical_zoom_st_t
{
    unsigned int status; // 0: none 1:optical zoom primary to secondary 2:secondary to primary
} msg_subreq_optical_zoom_st_t;

typedef struct _msg_req_extend_set_t
{
    unsigned int cam_id;
    unsigned int extend_cmd;
    char         paras[PARAS_LEN];
} msg_req_extend_set_t;

typedef struct _msg_ack_extend_set_t
{
    unsigned int cam_id;
    unsigned int extend_cmd;
    int          status;
    char         ack_info[EXT_ACK_PARAS_LEN];
} msg_ack_extend_set_t;

/* first expo and gain ack AP to select picture */
typedef struct capture_ack_t
{
    unsigned int flow;//0 for single; 1 for dual
    unsigned int expo[2];
    unsigned int gain[2];
} capture_ack_t;

typedef struct _msg_req_extend_get_t
{
    unsigned int cam_id;
    unsigned int extend_cmd;
    char         *paras;
} msg_req_extend_get_t;

typedef struct _msg_ack_extend_get_t
{
    unsigned int cam_id;
    unsigned int extend_cmd;
    char         *paras;
    int          status;
} msg_ack_extend_get_t;

typedef struct _msg_event_sent_t
{
    unsigned int cam_id;
    event_info_e event_id;
    unsigned int frame_number;
    unsigned int stream_id;
    unsigned int timestampL;
    unsigned int timestampH;
    //unsigned int timestamp;
    char         event_params[EVENT_PARAMS_LEN];
} msg_event_sent_t;

struct msg_base;

typedef void (*msg_looper_handler)(struct msg_base*, void*);

typedef struct msg_base
{
    struct hi_list_head link;
    msg_looper_handler handler;
    void* user;
} msg_base_t;

typedef struct _msg_req_jpeg_encode_t
{
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int input_buffer_y;
    unsigned int input_buffer_uv;
    unsigned int output_buffer;
    unsigned int quality;
    unsigned int format;
} msg_req_jpeg_encode_t;

typedef struct _msg_ack_jpeg_encode_t
{
    unsigned int output_buffer;
    unsigned int filesize;
    int          status;
} msg_ack_jpeg_encode_t;

typedef struct _msg_req_mem_pool_init_t
{
    unsigned int mempool_addr;
    unsigned int mempool_size;
    unsigned int mempool_prot;
}msg_req_mem_pool_init_t;

typedef struct _msg_ack_mem_pool_init_t
{
    unsigned int status;
}msg_ack_mem_pool_init_t;

typedef struct _msg_req_mem_pool_deinit_t
{
    unsigned int mempool_addr;
    unsigned int mempool_size;
    unsigned int mempool_prot;
}msg_req_mem_pool_deinit_t;

typedef struct _msg_ack_mem_pool_deinit_t
{
    unsigned int status;
}msg_ack_mem_pool_deinit_t;

typedef struct _isp_msg_t
{
    unsigned int message_size;
    unsigned int api_name;
    unsigned int message_id;
    unsigned int message_hash;
    union
    {
        /* Request items. */
        msg_req_query_capability_t      req_query_capability;
        msg_req_acquire_camera_t        req_acquire_camera;
        msg_req_release_camera_t        req_release_camera;
        msg_req_usecase_config_t        req_usecase_config;
        msg_req_stream_on_t             req_stream_on;
        msg_req_stream_off_t            req_stream_off;
        msg_req_get_otp_t               req_get_otp;
        msg_req_request_t               req_request;
        msg_req_arsr_request_t          req_arsr_request;
        msg_req_map_buffer_t            req_map_buffer; // DIFFER, type
        msg_req_unmap_buffer_t          req_unmap_buffer;
        msg_req_cal_data_t              req_cal_data;
        msg_req_set_isp_regs_t          req_set_isp_regs;
        msg_req_get_isp_regs_t          req_get_isp_regs;
        msg_req_set_i2c_regs_t          req_set_i2c_regs;
        msg_req_get_i2c_regs_t          req_get_i2c_regs;
        msg_req_test_case_interface_t   req_test_case_interface;
        msg_req_flush_t                 req_flush;
        msg_req_extend_set_t            req_extend_set;
        msg_req_extend_get_t            req_extend_get;
        msg_req_jpeg_encode_t       req_jpeg_encode;
        msg_req_inv_tlb_t               req_inv_tlb;
        msg_req_query_ois_update_t      req_query_ois_update;
        msg_req_ois_update_t            req_ois_update;
        msg_req_query_laser_t           req_query_laser;
        msg_req_acquire_laser_t         req_acquire_laser;
        msg_req_release_laser_t         req_release_laser;
        msg_req_acquire_depthisp_t      req_acquire_depthisp;
        msg_req_release_depthisp_t      req_release_depthisp;
        msg_req_motion_sensor_map_t     req_motion_sensor_map;
        msg_req_mem_pool_init_t         req_mem_pool_init;
        msg_req_mem_pool_deinit_t       req_mem_pool_deinit;
        /* Response items. */
        msg_ack_query_capability_t      ack_query_capability;
        msg_ack_acquire_camera_t        ack_require_camera;
        msg_ack_release_camera_t        ack_release_camera;
        msg_ack_usecase_config_t        ack_usecase_config;
        msg_ack_stream_on_t             ack_stream_on;
        msg_ack_stream_off_t            ack_stream_off;
        msg_ack_get_otp_t               ack_get_otp;
        msg_ack_request_t               ack_request;
        msg_ack_arsr_request_t          ack_arsr_request;
        msg_ack_map_buffer_t            ack_map_buffer;
        msg_ack_unmap_buffer_t          ack_unmap_buffer;
        msg_ack_cal_data_t              ack_cal_data;
        msg_ack_set_isp_regs_t          ack_set_isp_regs;
        msg_ack_get_isp_regs_t          ack_get_isp_regs;
        msg_ack_set_i2c_regs_t          ack_set_i2c_regs;
        msg_ack_get_i2c_regs_t          ack_get_i2c_regs;
        msg_ack_test_case_interface_t   ack_test_case_interface;
        msg_ack_flush_t                 ack_flush;
        msg_ack_extend_set_t            ack_extend_set;
        msg_ack_extend_get_t            ack_extend_get;
        msg_ack_jpeg_encode_t       ack_jpeg_encode;
        msg_ack_inv_tlb_t               ack_inv_tlb;
        msg_ack_query_ois_update_t      ack_query_ois_update;
        msg_ack_ois_update_t            ack_ois_update;
        msg_ack_query_laser_t           ack_query_laser;
        msg_ack_acquire_laser_t         ack_require_laser;
        msg_ack_release_laser_t         ack_release_laser;
        msg_ack_acquire_depthisp_t      ack_require_depthisp;
        msg_ack_release_depthisp_t      ack_release_depthisp;
        msg_ack_get_api_version_t       ack_get_api_version;
        msg_ack_motion_sensor_map_t     ack_motion_sensor_map;
        msg_ack_mem_pool_init_t         ack_mem_pool_init;
        msg_ack_mem_pool_deinit_t       ack_mem_pool_deinit;
        /* Event items sent to AP. */
        msg_event_sent_t                event_sent;
    }u;
    msg_base_t base;
    short token;
    struct rpmsg_ept *ept;
} hisp_msg_t;

enum
{
    OPTICAL_SWITCH_NONE = 0, // zoom without switch
    OPTICAL_SWITCH_PRIMARY_TO_SECONDARY, // optical zoom primary to secondary
    OPTICAL_SWITCH_SECONDARY_TO_PRIMARY, // optical zoom secondary to primary
};

void msg_init_req(hisp_msg_t* req, unsigned int api_name, unsigned int msg_id);
void msg_init_ack(hisp_msg_t* req, hisp_msg_t* ack);
/**********************************************************************
*             following typedefs are not currently used              *
**********************************************************************/

typedef enum
{
    HW_FW_CLOSED = 0,
    HW_CLOSED_ALONE,
    HW_FW_OPEN,
} algo_status_e;

typedef enum
{
    ALGO_ID_NULL = 0,
    /* special algo */
    ALGO_ID_FD,
    ALGO_ID_OIS,
    ALGO_ID_LASER,
    ALGO_ID_FLASH,
    ALGO_ID_AE,
    ALGO_ID_AF,

    /* FE algo */
    ALGO_ID_BLC,
    ALGO_ID_DGAMMA,
    ALGO_ID_SCLRGB,
    ALGO_ID_PDAF,
    ALGO_ID_LSCTOP,
    ALGO_ID_AUTOCLS,
    ALGO_ID_MINILSC,
    ALGO_ID_MINIPIPE,
    ALGO_ID_STAT3A,
    ALGO_ID_AWB,
    ALGO_ID_SD,

    /* BE algo */
    ALGO_ID_RAWNF,
    ALGO_ID_LSC,
    ALGO_ID_DPCC,
    ALGO_ID_AWBGAIN,
    ALGO_ID_GCD,
    ALGO_ID_CC,
    ALGO_ID_GAMMA,
    ALGO_ID_DRC,

    ALGO_ID_RGB2YUV,
    ALGO_ID_CE,
    ALGO_ID_STATYUV,

    /* PE algo */
    ALGO_ID_YUVNF,
    ALGO_ID_LUT3D,
    ALGO_ID_SCALER,
    ALGO_ID_ARSR,

    /* others */
    ALGO_ID_MONITOR,
    ALGO_ID_MAX,
} algo_id_e;

typedef enum _ucfg_ext_e
{
    NO_USE                       = 0 << 0,
    H_VIDEO_720P_120             = 1 << 1,
    H_VIDEO_1080P_60             = 1 << 2,
    MIRROR_MODE                  = 1 << 3,
    LONG_EXPOSURE_MODE           = 1 << 4,
    HDR_MOVIE                    = 1 << 5,
    DARK_RAIDER_MODE             = 1 << 6,
    H_VIDEO_720P_60              = 1 << 7,
    H_VIDEO_VGA_120              = 1 << 8,
    TUNING_PRE_MODE              = 1 << 9,
    H_VIDEO_720P_240             = 1 << 10,
    H_VIDEO_1080P_120            = 1 << 11,
    H_VIDEO_HIGH_RES             = 1 << 12,
    SEAMLESS_MODE                = 1 << 13,
    RESERVED                     = 1 << 14,
} ucfg_ext_e;

typedef enum _ucfg_scene_e
{
    CAMERA_SCENE_NORMAL = 0,
    CAMERA_SCENE_VIDEO,
    CAMERA_SCENE_DEBUG,
}ucfg_scene_e;




typedef enum
{
    DISP_HORIZONTAL = 0,
    DISP_VERTICAL_AND_FILP,
    DISP_HORIZONTAL_AND_FILP,
    DISP_VERTICAL,
} disp_direction_t;

typedef struct OIS2DCurve
{
    float aa;
    float ab;
    float bb;
    float a;
    float b;
    float c;
}OIS2DCurve;

typedef struct OIS2DCurveXY
{
    OIS2DCurve xHall;
    OIS2DCurve yHall;
}OIS2DCurveXY;

typedef struct OISInfo
{
    OIS2DCurveXY hallCalibParas;
    float        hallAccuracy;
    float        normalize;
    int          version;
    char moduleId[64];
}OISInfo;


typedef struct _isp_crop_region_info_t
{
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
} isp_crop_region_info_t;

typedef struct _subcmd_crop_region_info_t
{
    unsigned int cam_count;
    unsigned int cam_id[PIPELINE_COUNT];
    isp_crop_region_info_t crop_region[PIPELINE_COUNT];
} subcmd_crop_region_info_t;



#endif /* HISP_MSG_H_INCLUDED */
