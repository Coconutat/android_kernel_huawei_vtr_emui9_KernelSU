

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#define SUBCMD_LEN 4
#define MAX_PKT_LENGTH     128
#define MAX_PKT_LENGTH_AP     2560
#define MAX_LOG_LEN     100
#define MAX_PATTERN_SIZE 16
#define MAX_ACCEL_PARAMET_LENGTH    100
#define MAX_MAG_PARAMET_LENGTH    100
#define MAX_GYRO_PARAMET_LENGTH    100
#define MAX_ALS_PARAMET_LENGTH    100
#define MAX_PS_PARAMET_LENGTH    100
#define MAX_I2C_DATA_LENGTH     50
#define MAX_SENSOR_CALIBRATE_DATA_LENGTH  60
#define MAX_MAG_CALIBRATE_DATA_LENGTH     12
#define MAX_GYRO_CALIBRATE_DATA_LENGTH     72
#define MAX_GYRO_TEMP_OFFSET_LENGTH     56
#define MAX_CAP_PROX_CALIBRATE_DATA_LENGTH     16
#define MAX_MAG_AKM_CALIBRATE_DATA_LENGTH    28
//data flag consts
#define DATA_FLAG_FLUSH_OFFSET (0)
#define DATA_FLAG_VALID_TIMESTAMP_OFFSET (1)
#define FLUSH_END (1<<DATA_FLAG_FLUSH_OFFSET)
#define DATA_FLAG_VALID_TIMESTAMP (1<<DATA_FLAG_VALID_TIMESTAMP_OFFSET)
#define ACC_CALIBRATE_DATA_LENGTH  (15)
#define GYRO_CALIBRATE_DATA_LENGTH  (18)
#define PS_CALIBRATE_DATA_LENGTH  (3)
#define ALS_CALIBRATE_DATA_LENGTH  (6)

/*tag----------------------------------------------------*/
typedef enum {
    TAG_FLUSH_META,
    TAG_BEGIN = 0x01,
    TAG_SENSOR_BEGIN = TAG_BEGIN,
    TAG_ACCEL = TAG_SENSOR_BEGIN,
    TAG_GYRO,
    TAG_MAG,
    TAG_ALS,
    TAG_PS,/*5*/
    TAG_LINEAR_ACCEL,
    TAG_GRAVITY,
    TAG_ORIENTATION,
    TAG_ROTATION_VECTORS,
    TAG_PRESSURE,/*0x0a=10*/
    TAG_HALL,
    TAG_MAG_UNCALIBRATED,
    TAG_GAME_RV,
    TAG_GYRO_UNCALIBRATED,
    TAG_SIGNIFICANT_MOTION,/*0x0f=15*/
    TAG_STEP_DETECTOR,
    TAG_STEP_COUNTER,
    TAG_GEOMAGNETIC_RV,
    TAG_HANDPRESS,
    TAG_FINGERSENSE,/*0x14=20*/
    TAG_PHONECALL,
    TAG_GPS_4774_I2C,
    TAG_OIS,
    TAG_TILT_DETECTOR,
    TAG_RPC,/*0x19=25*///should same with modem definition
    TAG_CAP_PROX,
    TAG_MAGN_BRACKET,
    TAG_AGT,
	TAG_COLOR,
	TAG_ACCEL_UNCALIBRATED,/*0x1e = 30*/
    TAG_TOF,
    TAG_DROP,
    TAG_SENSOR_END,//sensor end should < 45
    TAG_HW_PRIVATE_APP_START = 45,/*0x2d=45*/
    TAG_AR = TAG_HW_PRIVATE_APP_START,
    TAG_MOTION,
    TAG_GPS,
    TAG_PDR,
    TAG_CA,
    TAG_FP,/*0x32=50*/
    TAG_KEY,
    TAG_AOD,
    TAG_FLP,
    TAG_ENVIRONMENT,
    TAG_LOADMONITOR,/*0x37=55*/
    TAG_APP_CHRE,
    TAG_FP_UD,
    TAG_HW_PRIVATE_APP_END,//APP_END should < 64, because power log used bitmap
    TAG_MODEM = 128,/*0x80=128*/
    TAG_TP,
    TAG_SPI,
    TAG_I2C,
    TAG_UART,
    TAG_RGBLIGHT,
    TAG_BUTTONLIGHT,/*0x86=135*/
    TAG_BACKLIGHT,
    TAG_VIBRATOR,
    TAG_SYS,
    TAG_LOG,
    TAG_LOG_BUFF,/*0x8b=140*/
    TAG_RAMDUMP,
    TAG_FAULT,
    TAG_SHAREMEM,
    TAG_SHELL_DBG,
    TAG_PD,/*0x90=145*/
    TAG_I3C,
    TAG_DATA_PLAYBACK,
    TAG_CHRE,
    TAG_SENSOR_CALI,
    TAG_CELL,
    TAG_BIG_DATA,
    TAG_END = 0xFF
}obj_tag_t;

/*------------------------cmd----------------------*/
typedef enum {
	CMD_CMN_OPEN_REQ = 0x01,
	CMD_CMN_OPEN_RESP,
	CMD_CMN_CLOSE_REQ,
	CMD_CMN_CLOSE_RESP,
	CMD_CMN_INTERVAL_REQ,
	CMD_CMN_INTERVAL_RESP,
	CMD_CMN_CONFIG_REQ,
	CMD_CMN_CONFIG_RESP,
	CMD_CMN_FLUSH_REQ,
	CMD_CMN_FLUSH_RESP,

	CMD_DATA_REQ = 0x1f,
	CMD_DATA_RESP,

	CMD_SET_FAULT_TYPE_REQ,//0x21
	CMD_SET_FAULT_TYPE_RESP,
	CMD_SET_FAULT_ADDR_REQ,
	CMD_SET_FAULT_ADDR_RESP,

	/*SPI*/
	CMD_SPI_BAUD_REQ,//0x25
	CMD_SPI_BAUD_RESP,
	CMD_SPI_TRANS_REQ,
	CMD_SPI_TRANS_RESP,

	/*I2C*/
	CMD_I2C_TRANS_REQ,//0x29
	CMD_I2C_TRANS_RESP,

	/*system status*/
	CMD_SYS_STATUSCHANGE_REQ,//0x2b
	CMD_SYS_STATUSCHANGE_RESP,
	CMD_SYS_DYNLOAD_REQ,
	CMD_SYS_DYNLOAD_RESP,
	CMD_SYS_HEARTBEAT_REQ,
	CMD_SYS_HEARTBEAT_RESP,
	CMD_SYS_LOG_LEVEL_REQ,
	CMD_SYS_LOG_LEVEL_RESP,
	CMD_SYS_CTS_RESTRICT_MODE_REQ,
	CMD_SYS_CTS_RESTRICT_MODE_RESP,

	/*LOG*/
	CMD_LOG_REPORT_REQ,//0x35
	CMD_LOG_REPORT_RESP,
	CMD_LOG_CONFIG_REQ,
	CMD_LOG_CONFIG_RESP,
	CMD_LOG_POWER_REQ,
	CMD_LOG_POWER_RESP,

	/*SHAREMEM*/
	CMD_SHMEM_AP_RECV_REQ,//0x3b
	CMD_SHMEM_AP_RECV_RESP,
	CMD_SHMEM_AP_SEND_REQ,
	CMD_SHMEM_AP_SEND_RESP,

	/* tag modem for cell info*/
	CMD_MODEM_CELL_INFO_REQ,//0x3f
	CMD_MODEM_CELL_INFO_RESP,
	CMD_MODEM_REBOOT_NOTIFY_REQ,
	CMD_MODEM_REBOOT_NOTIFY_RESP,

	/* SHELL_DBG */
	CMD_SHELL_DBG_REQ,//0x43
	CMD_SHELL_DBG_RESP,

	/* LoadMonitor */
	CMD_READ_AO_MONITOR_SENSOR,//0x45
	CMD_READ_AO_MONITOR_SENSOR_RESP,

	/* TAG_DATA_PLAYBACK */
	CMD_DATA_PLAYBACK_DATA_READY_REQ, //0x47        /*BACKPLAY*/
	CMD_DATA_PLAYBACK_DATA_READY_RESP,
	CMD_DATA_PLAYBACK_BUF_READY_REQ,        /*RECORD*/
	CMD_DATA_PLAYBACK_BUF_READY_RESP,

	/* CHRE */
	CMD_CHRE_AP_SEND_TO_MCU,//0x4b
	CMD_CHRE_AP_SEND_TO_MCU_RESP,
	CMD_CHRE_MCU_SEND_TO_AP,
	CMD_CHRE_MCU_SEND_TO_AP_RESP,

	/* BIG DATA */
	CMD_BIG_DATA_REQUEST_DATA,//0x4f
	CMD_BIG_DATA_REQUEST_DATA_RESP,
	CMD_BIG_DATA_SEND_TO_AP,   //0x51
	CMD_BIG_DATA_SEND_TO_AP_RESP,  //0x52

	/*log buff*/
	CMD_LOG_SER_REQ = 0xf1,
	CMD_LOG_USEBUF_REQ,
	CMD_LOG_BUFF_ALERT,
	CMD_LOG_BUFF_FLUSH,
	CMD_LOG_BUFF_FLUSHP,
	CMD_EXT_LOG_FLUSH,
	CMD_EXT_LOG_FLUSHP,

	/*max cmd*/
	CMD_ERR_RESP = 0xfe,
} obj_cmd_t;

typedef enum{
	SUB_CMD_NULL_REQ = 0x0,
	SUB_CMD_SELFCALI_REQ = 0x01,
	SUB_CMD_SET_PARAMET_REQ,
	SUB_CMD_SET_OFFSET_REQ,
	SUB_CMD_SELFTEST_REQ,
	SUB_CMD_CALIBRATE_DATA_REQ,
	SUB_CMD_SET_SLAVE_ADDR_REQ,
	SUB_CMD_SET_RESET_PARAM_REQ,
	SUB_CMD_ADDITIONAL_INFO,
	SUB_CMD_FW_DLOAD_REQ,
	SUB_CMD_FLUSH_REQ,
	SUB_CMD_SET_ADD_DATA_REQ,//11
	SUB_CMD_SET_DATA_TYPE_REQ,//12
	SUB_CMD_SET_DATA_MODE = 0x0d,//13

	/*motion*/
	SUB_CMD_MOTION_ATTR_ENABLE_REQ = 0x20,
	SUB_CMD_MOTION_ATTR_DISABLE_REQ,
	SUB_CMD_MOTION_REPORT_REQ,

	//ca
	SUB_CMD_CA_ATTR_ENABLE_REQ = 0x20,
	SUB_CMD_CA_ATTR_DISABLE_REQ,
	SUB_CMD_CA_REPORT_REQ,

	//fingerprint
	SUB_CMD_FINGERPRINT_OPEN_REQ = 0x20,
	SUB_CMD_FINGERPRINT_CLOSE_REQ,
	SUB_CMD_FINGERPRINT_CONFIG_SENSOR_DATA_REQ,

	//gyro
	SUB_CMD_GYRO_OIS_REQ = 0x20,
	SUB_CMD_GYRO_TMP_OFFSET_REQ,

	//finger sense
	SUB_CMD_ACCEL_FINGERSENSE_ENABLE_REQ = 0x20,
	SUB_CMD_ACCEL_FINGERSENSE_CLOSE_REQ,
	SUB_CMD_ACCEL_FINGERSENSE_REQ_DATA_REQ,
	SUB_CMD_ACCEL_FINGERSENSE_DATA_REPORT,

	/*tag pdr*/
	SUB_CMD_FLP_PDR_START_REQ = 0x20,
	SUB_CMD_FLP_PDR_STOP_REQ,
	SUB_CMD_FLP_PDR_WRITE_REQ,
	SUB_CMD_FLP_PDR_UPDATE_REQ,
	SUB_CMD_FLP_PDR_FLUSH_REQ,
	SUB_CMD_FLP_PDR_UNRELIABLE_REQ,
	SUB_CMD_FLP_PDR_STEPCFG_REQ,

	/*tag ar*/
	SUB_CMD_FLP_AR_START_REQ = 0x20,
	SUB_CMD_FLP_AR_CONFIG_REQ,
	SUB_CMD_FLP_AR_STOP_REQ,
	SUB_CMD_FLP_AR_UPDATE_REQ,
	SUB_CMD_FLP_AR_FLUSH_REQ,
	SUB_CMD_FLP_AR_GET_STATE_REQ,
	SUB_CMD_FLP_AR_SHMEM_STATE_REQ,
	SUB_CMD_CELL_INFO_DATA_REQ = 0x29,

	/*tag environment*/
	SUB_CMD_ENVIRONMENT_INIT_REQ = 0x20,
	SUB_CMD_ENVIRONMENT_ENABLE_REQ,
	SUB_CMD_ENVIRONMENT_DISABLE_REQ,
	SUB_CMD_ENVIRONMENT_EXIT_REQ,
	SUB_CMD_ENVIRONMENT_DATABASE_REQ,
	SUB_CMD_ENVIRONMENT_GET_STATE_REQ,

	/*tag flp*/
	SUB_CMD_FLP_START_BATCHING_REQ = 0x20,
	SUB_CMD_FLP_STOP_BATCHING_REQ,
	SUB_CMD_FLP_UPDATE_BATCHING_REQ,
	SUB_CMD_FLP_GET_BATCHED_LOCATION_REQ,
	SUB_CMD_FLP_FLUSH_LOCATION_REQ,
	SUB_CMD_FLP_INJECT_LOCATION_REQ,
	SUB_CMD_FLP_RESET_REQ,
	SUB_CMD_FLP_RESET_RESP,
	SUB_CMD_FLP_GET_BATCH_SIZE_REQ,
	SUB_CMD_FLP_BATCH_PUSH_GNSS_REQ,

	SUB_CMD_FLP_ADD_GEOF_REQ,
	SUB_CMD_FLP_REMOVE_GEOF_REQ,
	SUB_CMD_FLP_MODIFY_GEOF_REQ,
	SUB_CMD_FLP_LOCATION_UPDATE_REQ,
	SUB_CMD_FLP_GEOF_TRANSITION_REQ,
	SUB_CMD_FLP_GEOF_MONITOR_STATUS_REQ,

	SUB_CMD_FLP_GEOF_GET_TRANSITION_REQ,

	SUB_CMD_FLP_CELLFENCE_ADD_REQ,
	SUB_CMD_FLP_CELLFENCE_OPT_REQ,
	SUB_CMD_FLP_CELLFENCE_TRANSITION_REQ,
	SUB_CMD_FLP_CELLFENCE_INJECT_RESULT_REQ,

	SUB_CMD_FLP_CELLTRAJECTORY_CFG_REQ,
	SUB_CMD_FLP_CELLTRAJECTORY_REQUEST_REQ,
	SUB_CMD_FLP_CELLTRAJECTORY_REPORT_REQ,
	SUB_CMD_FLP_CELLDB_LOCATION_REPORT_REQ,

	SUB_CMD_FLP_COMMON_STOP_SERVICE_REQ,
	SUB_CMD_FLP_COMMON_WIFI_CFG_REQ,

	SUB_CMD_FLP_GEOF_GET_LOCATION_REQ,
	SUB_CMD_FLP_GEOF_GET_LOCATION_REPORT_REQ,
	SUB_CMD_FLP_ADD_WIFENCE_REQ,
	SUB_CMD_FLP_REMOVE_WIFENCE_REQ,
	SUB_CMD_FLP_PAUSE_WIFENCE_REQ,
	SUB_CMD_FLP_RESUME_WIFENCE_REQ,
	SUB_CMD_FLP_GET_WIFENCE_STATUS_REQ,
	SUB_CMD_FLP_WIFENCE_TRANSITION_REQ,
	SUB_CMD_FLP_WIFENCE_STATUS_REQ,
	SUB_CMD_FLP_COMMON_DEBUG_CONFIG_REQ,

	SUB_CMD_FLP_CELL_CELLBATCHING_CFG_REQ,
	SUB_CMD_FLP_CELL_CELLBATCHING_REQ,
	SUB_CMD_FLP_CELL_CELLBATCHING_REPORT_REQ,
	SUB_CMD_FLP_DIAG_SEND_CMD_REQ,
	SUB_CMD_FLP_DIAG_DATA_REPORT_REQ,

	//Always On Display
	SUB_CMD_AOD_START_REQ = 0x20,
	SUB_CMD_AOD_STOP_REQ,
	SUB_CMD_AOD_START_UPDATING_REQ,
	SUB_CMD_AOD_END_UPDATING_REQ,
	SUB_CMD_AOD_SET_TIME_REQ,
	SUB_CMD_AOD_SET_DISPLAY_SPACE_REQ,
	SUB_CMD_AOD_SETUP_REQ,
	SUB_CMD_AOD_DSS_ON_REQ,
	SUB_CMD_AOD_DSS_OFF_REQ,

	//key
	SUB_CMD_BACKLIGHT_REQ = 0x20,
       	// rpc
	SUB_CMD_RPC_START_REQ = 0x20,
	SUB_CMD_RPC_STOP_REQ,
	SUB_CMD_RPC_UPDATE_REQ,
	SUB_CMD_RPC_LIBHAND_REQ,

	SUB_CMD_VIBRATOR_SINGLE_REQ = 0x20,
	SUB_CMD_VIBRATOR_REPEAT_REQ,
	SUB_CMD_VIBRATOR_ON_REQ,
	SUB_CMD_VIBRATOR_SET_AMPLITUDE_REQ,

	SUB_CMD_MAX = 0xff,
}obj_sub_cmd_t;

typedef enum {
	EN_OK = 0,
	EN_FAIL,
} err_no_t;

typedef enum {
	NO_RESP,
	RESP,
} obj_resp_t;

typedef enum {
	MOTION_TYPE_START,
	MOTION_TYPE_PICKUP,
	MOTION_TYPE_FLIP,
	MOTION_TYPE_PROXIMITY,
	MOTION_TYPE_SHAKE,
	MOTION_TYPE_TAP,
	MOTION_TYPE_TILT_LR,
	MOTION_TYPE_ROTATION,
	MOTION_TYPE_POCKET,
	MOTION_TYPE_ACTIVITY,
	MOTION_TYPE_TAKE_OFF,
	MOTION_TYPE_EXTEND_STEP_COUNTER,
	MOTION_TYPE_EXT_LOG, //type 0xc
	MOTION_TYPE_HEAD_DOWN,
	MOTION_TYPE_PUT_DOWN,
	//
	MOTION_TYPE_SIDEGRIP, //sensorhub internal use, must at bottom;
	/*!!!NOTE:add string in motion_type_str when add type*/
	MOTION_TYPE_END,
} motion_type_t;

typedef enum
{
	FINGERPRINT_TYPE_START = 0x0,
	FINGERPRINT_TYPE_HUB,
	FINGERPRINT_TYPE_END,
}fingerprint_type_t;

typedef enum
{
	CA_TYPE_START,
	CA_TYPE_PICKUP,
	CA_TYPE_PUTDOWN,
	CA_TYPE_ACTIVITY,
	CA_TYPE_HOLDING,
	CA_TYPE_MOTION,
	CA_TYPE_PLACEMENT,
/*!!!NOTE:add string in ca_type_str when add type*/
	CA_TYPE_END,
}ca_type_t;

typedef enum
{
	AUTO_MODE = 0,
	FIFO_MODE,
	INTEGRATE_MODE,
	REALTIME_MODE,
	MODE_END
} obj_report_mode_t;

typedef enum {
	/*system status*/
	ST_NULL = 0,
	ST_BEGIN,
	ST_POWERON = ST_BEGIN,
	ST_MINSYSREADY,
	ST_DYNLOAD,
	ST_MCUREADY,
	ST_TIMEOUTSCREENOFF,
	ST_SCREENON,/*6*/
	ST_SCREENOFF,/*7*/
	ST_SLEEP,/*8*/
	ST_WAKEUP,/*9*/
	ST_POWEROFF,
	ST_RECOVERY_BEGIN,//for ar notify modem when iom3 recovery
	ST_RECOVERY_FINISH,//for ar notify modem when iom3 recovery
	ST_END
} sys_status_t;

typedef enum {
	DUBAI_EVENT_NULL = 0,
	DUBAI_EVENT_AOD_PICKUP = 3,
	DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN =4,
	DUBAI_EVENT_AOD_TIME_STATISTICS = 6,
	DUBAI_EVENT_END
} dubai_event_type_t;

typedef enum {
	BIG_DATA_EVENT_MOTION_TYPE = 936005001,
	BIG_DATA_EVENT_DDR_INFO,
	BIG_DATA_EVENT_TOF_PHONECALL
} big_data_event_id_t;

typedef enum {
	TYPE_STANDARD,
	TYPE_EXTEND
} type_step_counter_t;

typedef struct {
	uint8_t tag;
	uint8_t partial_order;
} pkt_part_header_t;
typedef struct {
	uint8_t tag;
	uint8_t cmd;
	uint8_t resp;	/*value CMD_RESP means need resp, CMD_NO_RESP means need not resp*/
	uint8_t partial_order;
	uint16_t tranid;
	uint16_t length;
} pkt_header_t;

typedef struct {
	uint8_t tag;
	uint8_t cmd;
	uint8_t resp;
	uint8_t partial_order;
	uint16_t tranid;
	uint16_t length;
	uint32_t errno; /*In win32, errno is function name and conflict*/
} pkt_header_resp_t;

typedef struct {
	pkt_header_t hd;
	uint8_t wr;
	uint32_t fault_addr;
} __packed pkt_fault_addr_req_t;

typedef struct aod_display_pos {
	uint16_t x_start;
	uint16_t y_start;
} aod_display_pos_t;

typedef struct aod_start_config {
	aod_display_pos_t aod_pos;
	int32_t intelli_switching;
} aod_start_config_t;

typedef struct aod_time_config {
	uint64_t curr_time;
	int32_t time_zone;
	int32_t sec_time_zone;
	int32_t time_format;
} aod_time_config_t;

typedef struct aod_display_space {
	uint16_t x_start;
	uint16_t y_start;
	uint16_t x_size;
	uint16_t y_size;
} aod_display_space_t;

typedef struct aod_display_spaces {
	int32_t dual_clocks;
	int32_t display_type;
	int32_t display_space_count;
	aod_display_space_t display_spaces[5];
} aod_display_spaces_t;

typedef struct aod_screen_info {
	uint16_t xres;
	uint16_t yres;
	uint16_t pixel_format;
} aod_screen_info_t;

typedef struct aod_bitmap_size {
	uint16_t xres;
	uint16_t yres;
} aod_bitmap_size_t;

typedef struct aod_bitmaps_size {
	int32_t bitmap_type_count;//2, dual clock
	aod_bitmap_size_t bitmap_size[2];
} aod_bitmaps_size_t;

typedef struct aod_config_info {
	uint32_t aod_fb;
	uint32_t aod_digits_addr;
	aod_screen_info_t screen_info;
	aod_bitmaps_size_t bitmap_size;
} aod_config_info_t;

typedef struct {
	pkt_header_t hd;
	uint32_t sub_cmd;
        union {
        	aod_start_config_t start_param;
        	aod_time_config_t time_param;
		aod_display_spaces_t display_param;
		aod_config_info_t config_param;
		aod_display_pos_t display_pos;
    	};
} aod_req_t;

typedef struct {
	pkt_header_t hd;
	int32_t x;
	int32_t y;
	int32_t z;
	uint32_t accracy;
} pkt_xyz_data_req_t;

struct sensor_data_xyz {
	int32_t x;
	int32_t y;
	int32_t z;
	uint32_t accracy;
};

typedef struct
{
    pkt_header_t hd;
    uint16_t data_flag;
    uint16_t cnt;
    uint16_t len_element;
    uint16_t sample_rate;
    uint64_t timestamp;
}  pkt_common_data_t;

typedef struct
{
    pkt_common_data_t data_hd;
    struct sensor_data_xyz xyz[];	/*x,y,z,acc,time*/
} pkt_batch_data_req_t;

typedef struct {
	pkt_header_t hd;
	s16 zaxis_data[];
} pkt_fingersense_data_report_req_t;

typedef struct {
	pkt_header_t hd;
	uint16_t data_flag;
	uint32_t step_count;
	uint32_t begin_time;
	uint16_t record_count;
	uint16_t capacity;
	uint32_t total_step_count;
	uint32_t total_floor_ascend;
	uint32_t total_calorie;
	uint32_t total_distance;
	uint16_t step_pace;
	uint16_t step_length;
	uint16_t speed;
	uint16_t touchdown_ratio;
	uint16_t reserved1;
	uint16_t reserved2;
	uint16_t action_record[];
} pkt_step_counter_data_req_t;

typedef struct
{
    pkt_header_t hd;
    int32_t type;
    int16_t serial;
    int16_t end;  // 0: more additional info to be send  1:this pkt is last one
    union {
        // for each frame, a single data type, either int32_t or float, should be used.
        int32_t data_int32[14];
        //float   data_float[14];
    };
}pkt_additional_info_req_t;

typedef struct
{
    pkt_common_data_t data_hd;
    int32_t status;
} pkt_magn_bracket_data_req_t;

typedef struct
{
    unsigned int type;
    unsigned int initial_speed;
    unsigned int height;
    int angle_pitch;
    int angle_roll;
    unsigned int material;
} drop_info_t;

typedef struct
{
    pkt_common_data_t data_hd;
    drop_info_t   data;
} pkt_drop_data_req_t;

typedef struct interval_param{
	uint32_t  period;
	//每batch_count组数据上报一次，in & out，
	//输入为期望值，输出为器件实际支持的最接近的值
	uint32_t	batch_count;
	//0：自动模式，由MCU根据业务特点及系统状态等条件来判断是否上报；
	//1：FIFO(Batch)，可能会有多条记录；
	//2：Integrate,将最新数据更新/累加，但不增加记录，择机上报；
	//3：实时模式，不管AP处于何种状态，实时上报
	uint8_t	mode;
	uint8_t	reserved[3]; //reserved[0]目前motion与计步器使用
}__packed interval_param_t;

typedef struct {
	pkt_header_t hd;
	interval_param_t param;
}__packed pkt_cmn_interval_req_t;

typedef struct {
	pkt_header_t hd;
	char app_config[16];
}__packed pkt_cmn_motion_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t subcmd;
	uint8_t para[128];
}__packed pkt_parameter_req_t;

typedef struct
{
    pkt_header_t hd;
    uint32_t subcmd;
}__packed pkt_subcmd_req_t;

typedef struct
{
    pkt_header_resp_t hd;
    uint32_t subcmd;
}__packed pkt_subcmd_resp_t;

union SPI_CTRL {
	uint32_t data;
	struct {
		uint32_t gpio_cs   : 16; /* bit0~8 is gpio NO., bit9~11 is gpio iomg set */
		uint32_t baudrate  : 5; /* unit: MHz; 0 means default 5MHz */
		uint32_t mode      : 2; /* low-bit: clk phase , high-bit: clk polarity convert, normally select:0 */
		uint32_t bits_per_word : 5; /* 0 means default: 8 */
		uint32_t rsv_28_31 : 4;
	} b;
};

typedef struct {
	pkt_header_t hd;
	uint8_t busid;
	union {
		uint32_t i2c_address;
		union SPI_CTRL ctrl;
	};
	uint16_t rx_len;
	uint16_t tx_len;
	uint8_t tx[];
} pkt_combo_bus_trans_req_t;

typedef struct {
	pkt_header_resp_t hd;
	uint8_t data[];
} pkt_combo_bus_trans_resp_t;

typedef struct {
	pkt_header_t hd;
	uint16_t status;
	uint16_t version;
} pkt_sys_statuschange_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t idle_time;
	uint64_t current_app_mask;
} pkt_power_log_report_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t event_id;
} pkt_big_data_report_t;

typedef struct {
	pkt_header_t hd;
	/*(MAX_PKT_LENGTH-sizeof(PKT_HEADER)-sizeof(End))/sizeof(uint16_t)*/
	uint8_t end;
	uint8_t file_count;
	uint16_t file_list[];
} pkt_sys_dynload_req_t;

typedef struct {
	pkt_header_t hd;
	uint8_t level;
	uint8_t dmd_case;
	uint8_t resv1;
	uint8_t resv2;
	uint32_t dmd_id;
	uint32_t info[5];
} pkt_dmd_log_report_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t subcmd;
	char calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
} pkt_mag_calibrate_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t subcmd;
	char calibrate_data[MAX_MAG_AKM_CALIBRATE_DATA_LENGTH];
} pkt_akm_mag_calibrate_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t subcmd;
	char calibrate_data[MAX_GYRO_CALIBRATE_DATA_LENGTH];
} pkt_gyro_calibrate_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t subcmd;
	char calibrate_data[MAX_GYRO_TEMP_OFFSET_LENGTH];
} pkt_gyro_temp_offset_req_t;

typedef struct {
	pkt_common_data_t fhd;
	int32_t data;
} fingerprint_upload_pkt_t;

typedef struct {
	uint32_t sub_cmd;
	uint8_t buf[7]; //byte alignment
	uint8_t len;
} fingerprint_req_t;

typedef struct {
	pkt_header_t hd;
	uint64_t app_id;
	uint16_t msg_type;
	uint8_t res[2];
	uint8_t data[];
} chre_req_t;

typedef enum additional_info_type {
    //
    AINFO_BEGIN = 0x0,                      // Marks the beginning of additional information frames
    AINFO_END   = 0x1,                      // Marks the end of additional information frames
    // Basic information
    AINFO_UNTRACKED_DELAY =  0x10000,       // Estimation of the delay that is not tracked by sensor
                                            // timestamps. This includes delay introduced by
                                            // sensor front-end filtering, data transport, etc.
                                            // float[2]: delay in seconds
                                            //           standard deviation of estimated value
                                            //
    AINFO_INTERNAL_TEMPERATURE,             // float: Celsius temperature.
                                            //
    AINFO_VEC3_CALIBRATION,                 // First three rows of a homogeneous matrix, which
                                            // represents calibration to a three-element vector
                                            // raw sensor reading.
                                            // float[12]: 3x4 matrix in row major order
                                            //
    AINFO_SENSOR_PLACEMENT,                 // Location and orientation of sensor element in the
                                            // device frame: origin is the geometric center of the
                                            // mobile device screen surface; the axis definition
                                            // corresponds to Android sensor definitions.
                                            // float[12]: 3x4 matrix in row major order
                                            //
    AINFO_SAMPLING,                         // float[2]: raw sample period in seconds,
                                            //           standard deviation of sampling period

    // Sampling channel modeling information
    AINFO_CHANNEL_NOISE = 0x20000,          // int32_t: noise type
                                            // float[n]: parameters
                                            //
    AINFO_CHANNEL_SAMPLER,                  // float[3]: sample period
                                            //           standard deviation of sample period,
                                            //           quantization unit
                                            //
    AINFO_CHANNEL_FILTER,                   // Represents a filter:
                                            //      \sum_j a_j y[n-j] == \sum_i b_i x[n-i]
                                            //
                                            // int32_t[3]: number of feedforward coefficients, M,
                                            //             number of feedback coefficients, N, for
                                            //               FIR filter, N=1.
                                            //             bit mask that represents which element to
                                            //               which the filter is applied, bit 0 == 1
                                            //               means this filter applies to vector
                                            //               element 0.
                                            // float[M+N]: filter coefficients (b0, b1, ..., BM-1),
                                            //             then (a0, a1, ..., aN-1), a0 is always 1.
                                            //             Multiple frames may be needed for higher
                                            //             number of taps.
                                            //
    AINFO_CHANNEL_LINEAR_TRANSFORM,         // int32_t[2]: size in (row, column) ... 1st frame
                                            // float[n]: matrix element values in row major order.
                                            //
    AINFO_CHANNEL_NONLINEAR_MAP,            // int32_t[2]: extrapolate method
                                            //             interpolate method
                                            // float[n]: mapping key points in pairs, (in, out)...
                                            //           (may be used to model saturation)
                                            //
    AINFO_CHANNEL_RESAMPLER,                // int32_t:  resample method (0-th order, 1st order...)
                                            // float[1]: resample ratio (upsampling if < 1.0;
                                            //           downsampling if > 1.0).
                                            //

    // Custom information
    AINFO_CUSTOM_START =    0x10000000,     //
    // Debugging
    AINFO_DEBUGGING_START = 0x40000000,     //
} additional_info_type_t;

enum {
	FILE_BEGIN,
	FILE_BASIC_SENSOR_APP = FILE_BEGIN,                /* 0 */
	FILE_FUSION,                              /* 1 */
	FILE_FUSION_GAME,                               /* 2 */
	FILE_FUSION_GEOMAGNETIC,                               /* 3 */
	FILE_MOTION,                                /* 4 */
	FILE_PEDOMETER,                          /* 5 */
	FILE_PDR,                            /* 6 */
	FILE_AR,                       /* 7 */
	FILE_GSENSOR_GATHER_FOR_GPS,                /* 8 */
	FILE_PHONECALL,                            /* 9 */
	FILE_FINGERSENSE,                         /* 10 */
	FILE_SIX_FUSION,                               /* 11 */
	FILE_HANDPRESS,                         /* 12 */
	FILE_CA,                       /* 13 */
	FILE_OIS,                        /* 14 */
	FILE_FINGERPRINT,              /*fingerprint_app*/
	FILE_KEY,			 	// 16
	FILE_GSENSOR_GATHER_SINGLE_FOR_GPS,  //17 Single line protocol for austin
	FILE_AOD,                                     //18
	FILE_MODEM,                                //19
	FILE_CHARGING,                          /* 20 */
    	FILE_MAGN_BRACKET,  			//21
    	FILE_FLP,                               // 22
    	FILE_TILT_DETECTOR,     //23
    	FILE_RPC,
	FILE_FINGERPRINT_UD = 28,
	FILE_DROP,           //29
	FILE_APP_ID_MAX = 31,                   /* MAX VALID FILE ID FOR APPs */

	FILE_AKM09911_DOE_MAG,                  /* 32 */
	FILE_BMI160_ACC,                        /* 33 */
	FILE_LSM6DS3_ACC,                       /* 34 */
	FILE_BMI160_GYRO,                       /* 35 */
	FILE_LSM6DS3_GYRO,                      /* 36 */
	FILE_AKM09911_MAG,                      /* 37 */
	FILE_BH1745_ALS,                        /* 38 */
	FILE_PA224_PS,                          /* 39 */
	FILE_ROHM_BM1383,                       /* 40 */
	FILE_APDS9251_ALS,                      /* 41 */
	FILE_LIS3DH_ACC,                        /* 42 */
	FILE_KIONIX_ACC,                        /* 43 */
	FILE_APDS993X_ALS,                      /* 44 */
	FILE_APDS993X_PS,                       /* 45 */
	FILE_TMD2620_PS,                        /* 46 */
	FILE_GPS_4774,                          /* 47 */
	FILE_ST_LPS22BH,                        /* 48 */
	FILE_APDS9110_PS,                       /* 49 */
	FILE_CYPRESS_HANDPRESS,                 //50
	FILE_LSM6DSM_ACC,                       //51
	FILE_LSM6DSM_GYRO,                       //52
	FILE_ICM20690_ACC,				//53
	FILE_ICM20690_GYRO,				//54
	FILE_LTR578_ALS,                //55
	FILE_LTR578_PS,                 //56
	FILE_FPC1021_FP,                           //57
	FILE_CAP_PROX,                             //58
        FILE_CYPRESS_KEY,		          //59
        FILE_CYPRESS_SAR,			  //60
        FILE_GPS_4774_SINGLE,		    //61
        FILE_SX9323_CAP_PROX,	           //62
	FILE_BQ25892_CHARGER,             /* 63 */
	FILE_FSA9685_SWITCH,               /* 64 */
	FILE_SCHARGER_V300,                /* 65 */
	FILE_YAS537_MAG,                /* 66 */
	FILE_AKM09918_MAG,           /*67*/
	FILE_TMD2745_ALS,                       /* 68 */
	FILE_TMD2745_PS,                        /* 69 */
	FILE_YAS537_DOE_MAG = 73,      /* 73 */
	FILE_FPC1268_FP,                         /* 74 */
	FILE_GOODIX8206_FP,                  /* 75 */
	FILE_FPC1075_FP,                     /* 76 */
	FILE_FPC1262_FP,                     /* 77 */
	FILE_GOODIX5296_FP,                  /* 78 */
	FILE_GOODIX3288_FP,                  /* 79 */
	FILE_SILEAD6185_FP,                  /* 80 */
	FILE_SILEAD6275_FP,                  /* 81 */
	FILE_BOSCH_BMP380,			/* 82 */
	FILE_TMD3725_ALS,                       // 83
	FILE_TMD3725_PS,                        // 84
	FILE_DRV2605_DRV,                       //85
	FILE_LTR582_ALS, //86
	FILE_LTR582_PS,  //87
	FILE_GOODIX_BAIKAL_FP, //88
	FILE_GOODIX5288_FP,
	FILE_QFP1500_FP, //90
	FILE_FPC1291_FP,  //91
	FILE_FPC1028_FP,  //92
	FILE_GOODIX3258_FP,  //93
	FILE_SILEAD6152_FP,  //94
	FILE_APDS9999_ALS,//95
	FILE_APDS9999_PS,//96
	FILE_TMD3702_ALS,//97
	FILE_TMD3702_PS,//98
	FILE_AMS8701_TOF,    //99
	FILE_VCNL36658_ALS,    //100
	FILE_VCNL36658_PS,    //101
	FILE_SILEAD6165_FP, //102
	FILE_SYNA155A_FP, //103
	FILE_TSL2591_ALS, //104
	FILE_BH1726_ALS, //105
	FILE_GOODIX3658_FP,//106
	FILE_FPC1511_FP,//107
	FILE_PA224_PS_VER2,//108
	FILE_ID_MAX,                       /* MAX VALID FILE ID */
};

#endif
