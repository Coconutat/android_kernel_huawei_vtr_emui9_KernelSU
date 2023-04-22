


#ifndef HISP160_MSG_H_INCLUDED
#define HISP160_MSG_H_INCLUDED

#define MAX_INPUT_STREAM_NUM (2)
#define MAX_STREAM_NUM       (14)
#define ARSR_REQ_OUT_NUM     (2)
#define NAME_LEN             (32)
#define PARAS_LEN            (400)
#define EXT_ACK_PARAS_LEN    (64)
#define EVENT_PARAMS_LEN     (400)
#define PIPELINE_COUNT       (2)

/* based on msg len is 464 */
#define MAX_SET_ISP_NR  1//50//53//59
#define MAX_GET_ISP_NR  1//100//108//119
#define MAX_SET_I2C_NR  1//35//35//39
#define MAX_GET_I2C_NR  1//50//53//59

typedef enum
{
    DMAP_DGEN_DISP,
    DMAP_DGEN_CONF,
    DMAP_LEFT_VERT,
    DMAP_DGEN_DEPTH,
    DMAP_DOPT_HORZ_DISP,
    DMAP_DOPT_VERT_DISP,
    DMAP_DOPT_DEPTH,
    DMAP_DOPT_XMAP,
    DMAP_DOPT_YMAP,
    DMAP_MAX_OUTPUT,
} dmap_output_e;

typedef enum
{
    PIXEL_FMT_RAW10,
    PIXEL_FMT_RAW12,
    PIXEL_FMT_RAW14,
    PIXEL_FMT_JPEG,
    PIXEL_FMT_YUV422_UYVY,  // default jpgenc format
    PIXEL_FMT_YUV420_NV12,
    PIXEL_FMT_YUV420_NV21,
    PIXEL_FMT_YUV422_VYUY,
    PIXEL_FMT_YUV422_YUYV,
    PIXEL_FMT_YUV422_YVYU,
    PIXEL_FMT_MONOCHROME,
    PIXEL_FMT_Y8,
    PIXEL_FMT_YUV420_PLANAR,//only warp
    PIXEL_FMT_XYMAP_8,//only warp
    PIXEL_FMT_XYMAP,//only warp
    PIXEL_FMT_XYMAP_XY,//only warp
    PIXEL_FMT_XYMAP_XY_8,//only warp
    PIXEL_FMT_WARP_XYMAP,//only warp
    PIXEL_FMT_DEPTH_MAP_8,//only warp
    PIXEL_FMT_DEPTH_MAP_16,//only warp
    PIXEL_FMT_YUV422_SP,
} pix_format_e;

typedef enum
{
    STREAM_REP_YUV_IN,
    STREAM_REP_RAW_IN,
    STREAM_ISP_YUV_OUT_PREVIEW,
    STREAM_ISP_YUV_OUT_VIDEO,
    STREAM_REP_YUV_OUT,
    STREAM_ISP_YUV_OUT_TINY,
    STREAM_ISP_RAW_OUT,
    STREAM_ISP_YUV_OUT_DMAP_CAP,
    STREAM_ISP_YUV_OUT_DMAP_PRE,
    STREAM_ISP_SENSOR_Y_OUT_FD,
    STREAM_RAW_OUT,
    STREAM_ISP_YUV_OUT_CB,
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
    COMMAND_WARP_REQUEST,
    COMMAND_ARSR_REQUEST,
    COMMAND_DGEN_REQUEST,
    COMMAND_DOPT_REQUEST,
    COMMAND_DMAP_MAP_REQUEST,
    COMMAND_DMAP_UNMAP_REQUEST,
    COMMAND_DGEN_FLUSH_REQUEST,
    COMMAND_DOPT_FLUSH_REQUEST,
    COMMAND_DMAP_FORMAT_REQUEST,
    COMMAND_MOTION_SENSOR_MAP_REQUEST,
    COMMAND_DRBR_REQUEST,
    COMMAND_DMAP_REQUEST,
    COMMAND_DMAP_FLUSH_REQUEST,
    COMMAND_MEM_POOL_INIT_REQUEST,
    COMMAND_MEM_POOL_DEINIT_REQUEST,
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
    WARP_REQUEST_RESPONSE,
    ARSR_REQUEST_RESPONSE,
    DGEN_REQUEST_RESPONSE,
    DOPT_REQUEST_RESPONSE,
    DMAP_MAP_RESPONSE,
    DMAP_UNMAP_RESPONSE,
    DGEN_FLUSH_RESPONSE,
    DOPT_FLUSH_RESPONSE,
    DMAP_FORMAT_RESPONSE,
    MOTION_SENSOR_MAP_RESPONSE,
    DRBR_REQUEST_RESPONSE,
    DMAP_REQUEST_RESPONSE,
    DMAP_FLUSH_RESPONSE,
    MEM_POOL_INIT_RESPONSE,
    MEM_POOL_DEINIT_RESPONSE,
    /* Event items sent to AP. */
    MSG_EVENT_SENT           = 0x3000,
} api_id_e;

typedef enum _ucfg_ext_e
{
    NO_USE               = 0 << 0,
    H_VIDEO_720P_120     = 1 << 1,
    H_VIDEO_1080P_60     = 1 << 2,
    MIRROR_MODE          = 1 << 3,
    LONG_EXPOSURE_MODE   = 1 << 4,
    HDR_MOVIE            = 1 << 5,
    DARK_RAIDER_MODE     = 1 << 6,
    H_VIDEO_720P_60      = 1 << 7,
    H_VIDEO_VGA_120      = 1 << 8,
    TUNING_PRE_MODE      = 1 << 9,
    H_VIDEO_720P_240     = 1 << 10,
    H_VIDEO_1080P_120    = 1 << 11,
    H_VIDEO_HIGH_RES     = 1 << 12,
    SEAMLESS_MODE        = 1 << 13,
    RESERVED             = 1 << 14,
} ucfg_ext_e;

typedef enum _ucfg_scene_e
{
    CAMERA_SCENE_NORMAL = 0,
    CAMERA_SCENE_VIDEO,
    CAMERA_SCENE_DEBUG,
}ucfg_scene_e;

typedef enum
{
    PRIMARY_CAMERA = 0,
    FRONT_CAMERA,
    SECONDARY_CAMERA,
    IR_CAMERA,
} camera_id_t;
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

typedef struct _laser_dmax_t
{
    unsigned int dmax_range;
    unsigned int dmax_rate;
}laser_dmax_t;

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
} msg_req_acquire_camera_t;

typedef struct _msg_ack_acquire_camera_t
{
    unsigned int cam_id;
    char         sensor_name[NAME_LEN];
} msg_ack_acquire_camera_t;
typedef struct _laser_fov_t
{
    float          x;
    float          y;
    float          width;
    float          height;
    float          angle;
}laser_fov_t;

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
}fov_info_t;

typedef struct _msg_req_acquire_laser_t
{
    unsigned int    i2c_index;
    char            product_name[NAME_LEN];
    char            name[NAME_LEN];
    int             offset;
    int             xtalk;
    laser_fov_t     fov_info;
    laser_spad_t    spad;
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

typedef struct _msg_ack_stream_on_t
{
    unsigned int cam_id;
    int          status;
} msg_ack_stream_on_t;

typedef struct _msg_req_get_otp_t
{
    unsigned int cam_id;
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
} msg_ack_request_t;

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

typedef enum{
    MODE_EIS_VIDHANCE = 0,
    MODE_DMAP = 1,
} warp_request_mode_e;

typedef struct grid_displacement
{
    unsigned int x_offset;
    unsigned int y_offset;
} grid_displacement_t;

typedef struct cgrid_info
{
    unsigned int cgrid_offset_h;
    unsigned int cgrid_offset_v;
    unsigned int cgrid_size_exp_h;
    unsigned int cgrid_size_exp_v;
    unsigned int cgrid_sector_h;
    unsigned int cgrid_sector_v;
    grid_displacement_t cgrid_xy_location[567];
} cgrid_info_t;

typedef struct warp_image_info
{
    unsigned int inputwidth;
    unsigned int inputheight;
    unsigned int outputwidth;
    unsigned int outputheight;
    unsigned int i_offset_h;
    unsigned int i_offset_v;
} warp_image_info_t;

typedef struct warp_info
{
    cgrid_info_t grid_info;
    warp_image_info_t image_info;
} warp_info_t;
typedef struct _msg_req_warp_request_t
{
    unsigned int  cam_id;
    unsigned int  frame_number;
    stream_info_t input_stream_info;
    stream_info_t output_stream_info;
    stream_info_t warp_output_stream_info;
    unsigned int  grid_enable;
    unsigned int  grid_order;
    warp_request_mode_e mode;
    unsigned int cgrid_info_buffer;
} msg_req_warp_request_t;

typedef struct _msg_ack_warp_request_t
{
    unsigned int  cam_id;
    unsigned int  frame_number;
    stream_info_t input_stream_info;
    stream_info_t output_stream_info;
    unsigned int  status;
} msg_ack_warp_request_t;

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

typedef struct _dmap_output_info_t
{
    unsigned int       width;
    unsigned int       height;
    unsigned int       stride;
}dmap_output_info_t;

typedef enum
{
    DISP_HORIZONTAL = 0,
    DISP_VERTICAL_AND_FILP,
    DISP_HORIZONTAL_AND_FILP,
    DISP_VERTICAL,
} disp_direction_t;

typedef struct _msg_req_dmap_format_request_t
{
    unsigned int            dgen_output_bit;
    unsigned int            dopt_output_bit;
    dmap_output_info_t      output_info[DMAP_MAX_OUTPUT];
    unsigned int            direction;
    unsigned int            expansion;
    unsigned int            dgen_input_format;        //0:yuv422+yuv422, 1:y+yuv422, 2:yuv422+y, 3:y+y
    unsigned int            dopt_input_format;        //0:yuv422 1:y
    unsigned int            req_switch_mask;          //output 0x1:sparse disp;0x2:sparse depth;0x4:dense disp;0x8:dense depth;0x10:both warp;0x20:XYMAP
}msg_req_dmap_format_t;

typedef struct _msg_ack_dmap_format_request_t
{
    unsigned int            status;
}msg_ack_dmap_format_t;

typedef struct _dgen_algo_cfg_t
{
    /* dmap func cfg & algo switch */
    int                      first_shift;
    unsigned char      max_disp;

    unsigned char     scanline_en;
    unsigned char     scanline_var_en;
    unsigned char     scanline_right_avail;
    unsigned char     lrcheck_en;
    unsigned char     median_filter_en;
    unsigned char     median_conf_en;
    unsigned char     var_gray_en;
    unsigned char     var_thres_en;
    unsigned char     rm_inval_disp_en;

    /* dgen algo param */
    unsigned char multipass_cost_shift;
    unsigned char inval_disp_conf_threshold;
    unsigned short multipass_disp_threshold;

    unsigned short var_threshold;
    unsigned short var_threshold_low;
    unsigned short var_threshold_mid;
    unsigned char var_gray_threshold_mid;
    unsigned char var_gray_threshold_low;
    unsigned short var_threshold_high;
    unsigned short var_gray_weight;
    unsigned char var_gray_threshold_high;
    unsigned char var_gray_shift;

    unsigned char ad_y_shift;
    unsigned char ad_u_shift;
    unsigned char ad_v_shift;
    unsigned char census_shift;

    unsigned char robust_x_a;
    unsigned char robust_k_a;
    unsigned char robust_a;
    unsigned char robust_x_b;
    unsigned char robust_k_b;
    unsigned char robust_k_c;
    unsigned short robust_b;
    unsigned short robust_c;

    unsigned short ad_lambda;
    unsigned short census_lambda;

    unsigned char v_mask_max;
    unsigned char h_mask_max;
    unsigned char v_mask_mid;
    unsigned char h_mask_mid;
    unsigned char yuv_threshold_high;
    unsigned char yuv_threshold_edge_high;
    unsigned char yuv_threshold_low;

    unsigned short scanline_penalty_2;
    unsigned short scanline_penalty_1;
    unsigned short scanline_penalty_mid_2;
    unsigned short scanline_penalty_mid_1;
    unsigned short scanline_penalty_low_2;
    unsigned short scanline_penalty_low_1;
    unsigned char scanline_yuv_threshold;
    unsigned char weight_dir7;
    unsigned char weight_dir2;
    unsigned char weight_dir4;
    unsigned char weight_dir0;

    unsigned char disp_gap;
    unsigned char disp_gap_ratio;
    unsigned char disp_gap_ratio_shift;
    unsigned char lrcheck_threshold;
    unsigned short conf_cost_threshold;

    unsigned short conf_low_slop;
    unsigned short conf_high_slop;
    unsigned int conf_low_intercept;
    unsigned int conf_high_intercept;

} dgen_algo_cfg_t;

typedef struct _dopt_algo_cfg_t
{
    unsigned char  output_dopt_format;
    unsigned char  dfil_horz_en;
    unsigned char  dfil_vert_en;
    unsigned char  double_chech_invalidate;
    unsigned char  dsmth_ref_en;
    unsigned char  dref_loop;

    /* dopt algo param */
    unsigned char y_enable;
    unsigned char u_enable;
    unsigned char v_enable;
    unsigned char y_threshold;
    unsigned char u_threshold;
    unsigned char v_threshold;

    unsigned char search_radius;
    unsigned char smooth_window_size;
    unsigned char edge_pixel_number;
    unsigned char ignore_border_window;
    unsigned char edge_gray_difference;

    unsigned short color_weight_lut[64];
    unsigned short horz_search_range;
    unsigned short vert_search_range;
    unsigned short smooth_weight_lut[9];
    unsigned int sigma_square;
    unsigned int guass_fix;
}dopt_algo_cfg_t;

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

typedef enum {
    WARP_BASE_PRIMARY = 0,
    WARP_BASE_SECOND,
}warp_base_t;

typedef struct _warp_info_params_t
{
    unsigned int fw_map_addr;
    OISInfo nvinfo;
} warp_info_params_t;

typedef struct _msg_req_dgen_request_t
{
    unsigned int      base_img;
    unsigned int      frame_number;
    unsigned int      req_type;
    unsigned int      disp_direction;
    unsigned int      dmap_crop_x;
    unsigned int      dmap_crop_y;
    unsigned int      dmap_crop_width;
    unsigned int      dmap_crop_height;
    unsigned int      input_left_buffer;
    unsigned int      input_right_buffer;
    unsigned int      output_left_raster;
    unsigned int      output_disp_raster;
    unsigned int      output_conf_raster;
    unsigned int      output_disp_fw_addr;
    unsigned int      output_conf_fw_addr;
} msg_req_dgen_request_t;

typedef struct _msg_ack_dgen_request_t
{
    unsigned int  output_disp_buffer;
    unsigned int  output_conf_buffer;
    unsigned int  output_left_raster;
    unsigned int  input_left_buffer;
    unsigned int  input_right_buffer;
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  out_stride;
    unsigned int  timestampL;
    unsigned int  timestampH;
    unsigned int  status;
} msg_ack_dgen_request_t;

typedef struct _msg_req_dopt_request_t
{
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  disp_direction;
    unsigned int  dmap_crop_x;
    unsigned int  dmap_crop_y;
    unsigned int  dmap_crop_width;
    unsigned int  dmap_crop_height;
    unsigned int  input_horz_left_image;
    unsigned int  input_vert_left_image;
    unsigned int  input_raster_disp_buffer;
    unsigned int  input_last_left_buffer;
    unsigned int  input_last_disp_buffer;
    unsigned int  output_disp_buffer;
} msg_req_dopt_request_t;

typedef struct _msg_ack_dopt_request_t
{
    unsigned int  output_disp_buffer;
    unsigned int  input_horz_left_image;
    unsigned int  input_vert_left_image;
    unsigned int  input_raster_disp_buffer;
    unsigned int  input_last_left_buffer;
    unsigned int  input_last_disp_buffer;
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  out_stride;
    unsigned int  timestampL;
    unsigned int  timestampH;
    unsigned int  status;
} msg_ack_dopt_request_t;

typedef struct _msg_req_drbr_request_t
{
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  dmap_crop_x;
    unsigned int  dmap_crop_y;
    unsigned int  dmap_crop_width;
    unsigned int  dmap_crop_height;
    unsigned int  input_buffer;
    unsigned int  output_buffer;
    unsigned int  image_width;
    unsigned int  image_height;
    unsigned int  data_type;
    unsigned int  mode;
    unsigned int  read_flip;
    unsigned int  write_flip;
    unsigned int  rotation;
    unsigned int  rub_dist;
    unsigned int  b2r_expansion;
} msg_req_drbr_request_t;
typedef struct _msg_ack_drbr_request_t
{
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  status;
    unsigned int  timestampL;
    unsigned int  timestampH;
    unsigned int  image_width;
    unsigned int  image_height;
    unsigned int  output_buffer;
    unsigned int  bit_num;
    unsigned int  rotation;
    unsigned int  read_flip;
    unsigned int  write_flip;
    unsigned int  mode;
} msg_ack_drbr_request_t;
typedef struct _msg_req_dmap_request_t
{
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  disp_direction;
    unsigned int  sparse_enable;
    unsigned int  dense_enable;
    unsigned int  warp_enable;
    unsigned int  dmap_crop_x;
    unsigned int  dmap_crop_y;
    unsigned int  dmap_crop_width;
    unsigned int  dmap_crop_height;
    unsigned int  input_dgen_left;
    unsigned int  input_dgen_right;
    unsigned int  output_dgen_left;
    unsigned int  output_sparse_disp;
    unsigned int  output_conf_raster;
    unsigned int  output_sparse_depth;
    unsigned int  output_disp_fw_addr;
    unsigned int  output_conf_fw_addr;
    unsigned int  input_dopt_horz_left;
    unsigned int  input_dopt_vert_left;
    unsigned int  input_dopt_disp_buffer;
    unsigned int  input_last_left_buffer;
    unsigned int  input_last_disp_buffer;
    unsigned int  output_dense_horz_disp;
    unsigned int  output_dense_vert_disp;
    unsigned int  output_dense_depth;
    unsigned int  output_dense_xmap;
    unsigned int  output_dense_ymap;
    unsigned int  input_warp_sparse_setting_buffer;
    unsigned int  input_warp_dense_setting_buffer;
} msg_req_dmap_request_t;
typedef struct _msg_ack_dmap_request_t
{
    unsigned int  output_sparse_disp;
    unsigned int  output_conf_raster;
    unsigned int  output_dgen_left;
    unsigned int  output_sparse_depth;
    unsigned int  input_dgen_left;
    unsigned int  input_dgen_right;
    unsigned int  output_dense_horz_disp;
    unsigned int  output_dense_vert_disp;
    unsigned int  output_dense_depth;
    unsigned int  output_dense_xmap;
    unsigned int  output_dense_ymap;
    unsigned int  input_dopt_horz_left;
    unsigned int  input_dopt_vert_left;
    unsigned int  input_dopt_disp_buffer;
    unsigned int  input_last_left_buffer;
    unsigned int  input_last_disp_buffer;
    unsigned int  input_warp_sparse_setting_buffer;
    unsigned int  input_warp_dense_setting_buffer;
    unsigned int  base_img;
    unsigned int  frame_number;
    unsigned int  req_type;
    unsigned int  out_stride;
    unsigned int  timestampL;
    unsigned int  timestampH;
    unsigned int  warp_flag;
    unsigned int  status;
} msg_ack_dmap_request_t;
typedef struct _msg_req_dmap_map_t
{
    unsigned int fw_cfg_addr;
    unsigned int isp_buf_addr;
    unsigned int cfg_mem_size;
    unsigned int buf_mem_size;
    unsigned int mode;
}msg_req_dmap_map_t;

typedef struct _msg_ack_dmap_map_t
{
    unsigned int status;
}msg_ack_dmap_map_t;

typedef struct _msg_req_dmap_unmap_t
{
    unsigned int unmap_cfg_addr;
    unsigned int unmap_buf_addr;
}msg_req_dmap_unmap_t;

typedef struct _msg_ack_dmap_unmap_t
{
    unsigned int status;
}msg_ack_dmap_unmap_t;

typedef struct _msg_req_dgen_flush_t
{
    int flag;
}msg_req_dgen_flush_t;

typedef struct _msg_ack_dgen_flush_t
{
    unsigned int status;
}msg_ack_dgen_flush_t;

typedef struct _msg_req_dopt_flush_t
{
    int flag;
}msg_req_dopt_flush_t;

typedef struct _msg_ack_dopt_flush_t
{
    unsigned int status;
}msg_ack_dopt_flush_t;

typedef struct _map_pool_desc_t{
    unsigned int start_addr;
    unsigned int ion_iova;
    unsigned int size;
    unsigned int usage;
} map_pool_desc_t;

typedef struct _msg_req_dmap_flush_t
{
    int flag;
}msg_req_dmap_flush_t;

typedef struct _msg_ack_dmap_flush_t
{
    unsigned int status;
}msg_ack_dmap_flush_t;
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

typedef enum{
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
    SCALER_CROP_REGION = 23,
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
    SUBCMD_ENABLE_WARP = 130,
    SUBCMD_SET_STREAM_MODE = 131,
    SUBCMD_SET_DGEN_ALGO = 132,
    SUBCMD_SET_WARP_INFO = 133,
    SUBCMD_SET_DOPT_ALGO = 134,
    SUBCMD_GET_LCD_STATE = 135,
    SUBCMD_SET_LOG_MODULE = 136,
    SUBCMD_SET_DMAP_CAP_SIZE = 137,
    SUBCMD_SET_PLATFORM_ID = 138,
    //tmp
    SUBCMD_SET_FLASH_MODE = 139,
    SUBCMD_SET_LASER_DIRTY = 140,
    SUBCMD_SET_FACE_LANDMARKS = 141,
    SUBCMD_SET_AE_ALWAYS_CONVERGE = 142,
    SUBCMD_SET_DMAP_INFO = 143,
    SUBCMD_SET_PDAF_RESULT = 144,
    SUBCMD_SET_SELF_LEARN_DATA = 145,
    SUBCMD_SET_SFR_TEST_DATA = 146,
    SUBCMD_SET_SCE_HUE_GAIN = 147,
    SUBCMD_SET_MOTION_INFO = 148,
    SUBCMD_ENABLE_EIS = 149,
    SUBCMD_SET_REDUCE_EXPOSURE = 150,
    SUBCMD_SET_APERTURE_MONO = 151,
    SUBCMD_SET_LSC_MODE = 152,
    SUBCMD_SET_SEAMLESS_INFO = 153,
    SUBCMD_SET_RAW2YUV_INFO = 154,
    SUBCMD_SET_ISP_ALGO_STATUS = 155,
    SUBCMD_SET_RAWNF_DS = 156,
    SUBCMD_SET_YUVNF_DS = 157,
    SUBCMD_SET_AF_OTPSTART_MODE = 158,
    SUBCMD_FOV_SCALE_RATIO_STATUS = 159,
    SUBCMD_SET_HFBC_ALIGMENT = 160,
    SUBCMD_SET_PD_OFFSET_CALIB_MMI_ENABLE = 161,
    SUBCMD_SET_SOFTLIGHT_MODE  = 167,
    SUBCMD_MAX,
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
    ALGO_ID_NULL = 0,
    /* special algo */
    ALGO_ID_FD,
    ALGO_ID_AE,
    ALGO_ID_AF,
    ALGO_ID_FLASH,
    /* FE algo */
    ALGO_ID_BLC,
    ALGO_ID_DGAMMA,
    ALGO_ID_PDAF,
    ALGO_ID_SCLRGB,
    ALGO_ID_MINILSC,
    ALGO_ID_STAT3A,
    ALGO_ID_AWB,
    ALGO_ID_DIS,
    ALGO_ID_AUTOCLS,
    ALGO_ID_LASER,
    /* BE algo */
    ALGO_ID_RAWNF,
    ALGO_ID_LSC,
    ALGO_ID_DPCC,
    ALGO_ID_AWBGAIN,
    ALGO_ID_GCD,
    ALGO_ID_LBC,
    ALGO_ID_CC,
    ALGO_ID_DRC,
    ALGO_ID_CGT,
    ALGO_ID_GAMMA,
    ALGO_ID_MINIPIPE,
    ALGO_ID_RGB2YUV,
    ALGO_ID_UVDEC,
    ALGO_ID_CE,
    ALGO_ID_YUVNFIIR,
    ALGO_ID_SHARPEN,
    ALGO_ID_DE,
    ALGO_ID_TNR,
    /*PE algo*/
    ALGO_ID_YUVNF,
    ALGO_ID_LUT3D,
    ALGO_ID_SCALER,
    ALGO_ID_ARSR,
    ALGO_ID_WARP,
    ALGO_ID_DMAP,
    /* others */
    ALGO_ID_OIS,
    ALGO_ID_SD,
    ALGO_ID_MONITOR,
    ALGO_ID_MAX,
} algo_id_e;

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

typedef struct _cap_dmap_yuv_info_t
{
    unsigned int width;
    unsigned int height;
    unsigned int stride;
} cap_dmap_yuv_info_t;

struct msg_base;

typedef void (*msg_looper_handler)(struct msg_base*, void*);

typedef struct msg_base
{
    struct hi_list_head link;
    msg_looper_handler handler;
    void* user;
} msg_base_t;

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
        msg_req_get_otp_t               req_get_otp;
        msg_req_request_t               req_request;
        msg_req_warp_request_t          req_warp_request;
        msg_req_arsr_request_t          req_arsr_request;
        msg_req_dmap_format_t           req_dmap_format;
        msg_req_dmap_request_t          req_dmap_request;
        msg_req_dgen_request_t          req_dgen_request;
        msg_req_dopt_request_t          req_dopt_request;
        msg_req_drbr_request_t          req_drbr_request;
        msg_req_map_buffer_t            req_map_buffer;
        msg_req_unmap_buffer_t          req_unmap_buffer;
        msg_req_dmap_map_t              req_dmap_map;
        msg_req_dmap_unmap_t            req_dmap_unmap;
        msg_req_cal_data_t              req_cal_data;
        msg_req_set_isp_regs_t          req_set_isp_regs;
        msg_req_get_isp_regs_t          req_get_isp_regs;
        msg_req_set_i2c_regs_t          req_set_i2c_regs;
        msg_req_get_i2c_regs_t          req_get_i2c_regs;
        msg_req_test_case_interface_t   req_test_case_interface;
        msg_req_flush_t                 req_flush;
        msg_req_dgen_flush_t            req_dgen_flush;
        msg_req_dopt_flush_t            req_dopt_flush;
        msg_req_dmap_flush_t            req_dmap_flush;
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
        msg_ack_get_otp_t               ack_get_otp;
        msg_ack_request_t               ack_request;
        msg_ack_warp_request_t          ack_warp_request;
        msg_ack_arsr_request_t          ack_arsr_request;
        msg_ack_dmap_format_t           ack_dmap_format;
        msg_ack_dmap_request_t          ack_dmap_request;
        msg_ack_dgen_request_t          ack_dgen_request;
        msg_ack_dopt_request_t          ack_dopt_request;
        msg_ack_drbr_request_t          ack_drbr_request;
        msg_ack_map_buffer_t            ack_map_buffer;
        msg_ack_unmap_buffer_t          ack_unmap_buffer;
        msg_ack_dmap_map_t              ack_dmap_map_buffer;
        msg_ack_dmap_unmap_t            ack_dmap_unmap_buffer;
        msg_ack_cal_data_t              ack_cal_data;
        msg_ack_set_isp_regs_t          ack_set_isp_regs;
        msg_ack_get_isp_regs_t          ack_get_isp_regs;
        msg_ack_set_i2c_regs_t          ack_set_i2c_regs;
        msg_ack_get_i2c_regs_t          ack_get_i2c_regs;
        msg_ack_test_case_interface_t   ack_test_case_interface;
        msg_ack_flush_t                 ack_flush;
        msg_ack_dgen_flush_t            ack_dgen_flush;
        msg_ack_dopt_flush_t            ack_dopt_flush;
        msg_ack_dmap_flush_t            ack_dmap_flush;
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

void msg_init_req(hisp_msg_t* req, unsigned int api_name, unsigned int msg_id);
void msg_init_ack(hisp_msg_t* req, hisp_msg_t* ack);

#endif /* HISP160_MSG_H_INCLUDED */
