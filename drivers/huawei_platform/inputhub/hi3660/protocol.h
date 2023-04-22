

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

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
#define MAX_GYRO_CALIBRATE_DATA_LENGTH     12
#define MAX_CAP_PROX_CALIBRATE_DATA_LENGTH     16
#define MAX_MAG_AKM_CALIBRATE_DATA_LENGTH    28

/*tag----------------------------------------------------*/
typedef enum {
	TAG_FLUSH_META,
	TAG_BEGIN = 0x01,
	TAG_SENSOR_BEGIN = TAG_BEGIN,
	TAG_ACCEL = TAG_SENSOR_BEGIN,
	TAG_GYRO,
	TAG_MAG,
	TAG_ALS,
	TAG_PS,			/*5 */
	TAG_SCREEN_ROTATE,
	TAG_LINEAR_ACCEL,
	TAG_GRAVITY,
	TAG_ORIENTATION,
	TAG_ROTATION_VECTORS,	/*10 */
	TAG_PRESSURE,
	TAG_TEMP,
	TAG_HUMIDITY,
	TAG_AMBIENT_TEMP,
	TAG_LABC,		/*15 */
	TAG_HALL,
	TAG_MAG_UNCALIBRATED,
	TAG_GAME_RV,
	TAG_GYRO_UNCALIBRATED,
	TAG_SIGNIFICANT_MOTION,	/*20 */
	TAG_STEP_DETECTOR,
	TAG_STEP_COUNTER,
	TAG_GEOMAGNETIC_RV,
	TAG_HANDPRESS,
	TAG_CAP_PROX,		/*25 */
	TAG_FINGERSENSE,
	TAG_PHONECALL,
	TAG_GPS_4774_I2C,
	TAG_OIS,                    /*29 */
	TAG_CHARGER,
	TAG_SWITCH,
	TAG_MAGN_BRACKET,
	TAG_TILT_DETECTOR,
	TAG_RPC,            /*34*/
	TAG_AGT,
	TAG_SENSOR_END,
	TAG_TP = 0x80,
	TAG_SPI,
	TAG_I2C,
	TAG_UART,
	TAG_RGBLIGHT,
	TAG_BUTTONLIGHT, /*0x85 = 133*/
	TAG_BACKLIGHT,
	TAG_VIBRATOR,
	TAG_SYS,
	TAG_LOG,
	TAG_MOTION, /*8a = 138*/
	TAG_CURRENT,
	TAG_LOG_BUFF,
	TAG_RAMDUMP,
	TAG_PDR,
	TAG_AR,/*8f  = 143 */
	TAG_FAULT,
	TAG_SHAREMEM,
	TAG_CA,
	TAG_SHELL_DBG,
	TAG_FP,
	TAG_KEY,/*0x95 = 149*/
	TAG_AOD,
	TAG_MODEM,
	TAG_PD,
	TAG_GPS,
	TAG_FLP,
	TAG_ENVIRONMENT,
	TAG_DATA_PLAYBACK,
	TAG_KB,/*0x9D = 157*/
	TAG_END = 0xFF
} obj_tag_t;

/*cmd----------------------------------------------*/
typedef enum {
	/*common command*/
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

	CMD_PRIVATE = 0x1f,
	CMD_DATA_REQ = CMD_PRIVATE,
	CMD_DATA_RESP,

	/*accel send to mcu with no para*/
	CMD_SELFCALI_REQ,
	CMD_SELFCALI_RESP,

	CMD_SET_PARAMET_REQ,
	CMD_SET_PARAMET_RESP,
	CMD_SET_OFFSET_REQ,
	CMD_SET_OFFSET_RESP,
	CMD_SELFTEST_REQ,
	CMD_SELFTEST_RESP,

	/*mag cap_prox read data from nv and send it to mcu*/
	CMD_SET_CALIBRATE_REQ,
	CMD_SET_CALIBRATE_RESP,

	/*calibrate data from mcu, then write to nv*/
	CMD_CALIBRATE_DATA_REQ,
	CMD_CALIBRATE_DATA_RESP,

	/*gyro none use*/
	CMD_CONFIG_REQ,
	CMD_CONFIG_RESP,

	CMD_SET_SLAVE_ADDR_REQ,
	CMD_SET_SLAVE_ADDR_RESP,

	CMD_SET_FAULT_TYPE_REQ,
	CMD_SET_FAULT_TYPE_RESP,
	CMD_SET_FAULT_ADDR_REQ,
	CMD_SET_FAULT_ADDR_RESP,

	CMD_SET_OIS_REQ,
	CMD_SET_OIS_RESP,

	CMD_ADDITIONAL_INFO_REQ,
	CMD_ADDITIONAL_INFO_RESP,

	CMD_FW_DLOAD_REQ,
	CMD_FW_DLOAD_RESP,
	CMD_BACKLIGHT_REQ,
	CMD_BACKLIGHT_RESP,

	CMD_SET_RESET_PARAM_REQ,
	CMD_SET_RESET_PARAM_RESP,

	CMD_RAMDUMP_REQ = CMD_PRIVATE,
	CMD_RAMDUMP_RESP,
	/*accelerometer command*/
	CMD_ACCEL_DATA_REQ = CMD_DATA_REQ,
	CMD_ACCEL_DATA_RESP = CMD_DATA_RESP,
	CMD_ACCEL_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_ACCEL_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_ACCEL_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_ACCEL_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_ACCEL_OFFSET_REQ = CMD_SET_OFFSET_REQ,
	CMD_ACCEL_OFFSET_RESP = CMD_SET_OFFSET_RESP,
	CMD_ACCEL_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_ACCEL_SELFTEST_RESP = CMD_SELFTEST_RESP,

	/*gyroscopy command*/
	CMD_GYRO_DATA_REQ = CMD_DATA_REQ,
	CMD_GYRO_DATA_RESP = CMD_DATA_RESP,
	CMD_GYRO_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_GYRO_SELFTEST_RESP = CMD_SELFTEST_RESP,
	CMD_GYRO_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_GYRO_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_GYRO_CONFIG_REQ = CMD_CONFIG_REQ,
	CMD_GYRO_CONFIG_RESP = CMD_CONFIG_RESP,
	CMD_GYRO_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_GYRO_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_GYRO_OFFSET_REQ = CMD_SET_OFFSET_REQ,
	CMD_GYRO_OFFSET_RESP = CMD_SET_OFFSET_RESP,
	CMD_GYRO_OIS_REQ = CMD_SET_OIS_REQ,
	CMD_GYRO_OIS_RESP = CMD_SET_OIS_RESP,

	/*magnetometer command*/
	CMD_MAG_DATA_REQ = CMD_DATA_REQ,
	CMD_MAG_DATA_RESP = CMD_DATA_RESP,
	CMD_MAG_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_MAG_SELFTEST_RESP = CMD_SELFTEST_RESP,
	CMD_MAG_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_MAG_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_MAG_SET_CALIBRATE_TO_MCU_REQ = CMD_SET_OFFSET_REQ,
	CMD_MAG_SET_CALIBRATE_TO_MCU_RESP = CMD_SET_OFFSET_RESP,
	CMD_MAG_SEND_CALIBRATE_TO_AP_REQ = CMD_CALIBRATE_DATA_REQ,
	CMD_MAG_SEND_CALIBRATE_TO_AP_RESP = CMD_CALIBRATE_DATA_RESP,

	/*ambient light sensor*/
	CMD_ALS_DATA_REQ = CMD_DATA_REQ,
	CMD_ALS_DATA_RESP = CMD_DATA_RESP,
	CMD_ALS_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_ALS_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_ALS_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_ALS_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_ALS_OFFSET_REQ = CMD_SET_OFFSET_REQ,
	CMD_ALS_OFFSET_RESP = CMD_SET_OFFSET_RESP,
	CMD_ALS_RESET_PARA_REQ = CMD_SET_RESET_PARAM_REQ,
	CMD_ALS_RESET_PARA_RESP = CMD_SET_RESET_PARAM_RESP,

	/*proximity sensor*/
	CMD_PS_DATA_REQ = CMD_DATA_REQ,
	CMD_PS_DATA_RESP = CMD_DATA_RESP,
	CMD_PS_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_PS_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_PS_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_PS_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_PS_OFFSET_REQ = CMD_SET_OFFSET_REQ,
	CMD_PS_OFFSET_RESP = CMD_SET_OFFSET_RESP,

	CMD_SCREEN_ROTATE_DATA_REQ = CMD_DATA_REQ,
	CMD_SCREEN_ROTATE_DATA_RESP = CMD_DATA_RESP,

	/*linear acceleration*/
	CMD_LINEAR_ACCEL_DATA_REQ = CMD_DATA_REQ,
	CMD_LINEAR_ACCEL_DATA_RESP = CMD_DATA_RESP,

	/*gravity*/
	CMD_GRAVITY_DATA_REQ = CMD_DATA_REQ,
	CMD_GRAVITY_DATA_RESP = CMD_DATA_RESP,

	/*orientation*/
	CMD_ORIENTATION_DATA_REQ = CMD_DATA_REQ,
	CMD_ORIENTATION_DATA_RESP = CMD_DATA_RESP,

	/*rotation vectors*/
	CMD_ROTATION_VECTORS_DATA_REQ = CMD_DATA_REQ,
	CMD_ROTATION_VECTORS_DATA_RESP = CMD_DATA_RESP,

	/*step counter*/
	CMD_STEP_COUNTER_DATA_REQ = CMD_DATA_REQ,
	CMD_STEP_COUNTER_DATA_RESP = CMD_DATA_RESP,

	//significant motion
	CMD_SIGNIFICANT_MOTION_DATA_REQ = CMD_DATA_REQ,
	CMD_SIGNIFICANT_MOTION_RESP = CMD_DATA_RESP,


	//uncalibrate mag
	CMD_UNCALIBRATED_MAG_DATA_REQ = CMD_DATA_REQ,
	CMD_UNCALIBRATED_MAG_RESP = CMD_DATA_RESP,

	//uncalibrate gyro
	CMD_UNCALIBRATED_GYROSCOPE_DATA_REQ = CMD_DATA_REQ,
	CMD_UNCALIBRATED_GYROSCOPE_RESP = CMD_DATA_RESP,

	/*airpress sensor*/
	CMD_AIRPRESS_DATA_REQ = CMD_DATA_REQ,
	CMD_AIRPRESS_DATA_RESP = CMD_DATA_RESP,
	CMD_AIRPRESS_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_AIRPRESS_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_AIRPRESS_SET_CALIDATA_REQ = CMD_SET_OFFSET_REQ,
	CMD_AIRPRESS_SET_CALIDATA_RESP = CMD_SET_OFFSET_RESP,

	/*handpress sensor*/
	CMD_HANDPRESS_DATA_REQ = CMD_DATA_REQ,
	CMD_HANDPRESS_DATA_RESP = CMD_DATA_RESP,
	CMD_HANDPRESS_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_HANDPRESS_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_HANDPRESS_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_HANDPRESS_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_HANDPRESS_SET_CALIDATA_REQ = CMD_SET_OFFSET_REQ,
	CMD_HANDPRESS_SET_CALIDATA_RESP = CMD_SET_OFFSET_RESP,
	CMD_HANDPRESS_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_HANDPRESS_SELFTEST_RESP = CMD_SELFTEST_RESP,

	/*cap_prox sensor*/
	CMD_CAP_PROX_DATA_REQ = CMD_DATA_REQ,
	CMD_CAP_PROX_DATA_RESP = CMD_DATA_RESP,
	CMD_CAP_PROX_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_CAP_PROX_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_CAP_PROX_SET_CALIDATA_REQ = CMD_SET_OFFSET_REQ,
	CMD_CAP_PROX_SET_CALIDATA_RESP = CMD_SET_OFFSET_RESP,
	CMD_CAP_PROX_CALIBRATE_REQ = CMD_SELFCALI_REQ,
	CMD_CAP_PROX_CALIBRATE_RESP =CMD_SELFCALI_RESP,

	/*pseduo_sar sensor*/
	CMD_PSEUDO_SAR_DATA_REQ = CMD_DATA_REQ,
	CMD_PSEUDO_SAR_DATA_RESP = CMD_DATA_RESP,
	CMD_PSEUDO_SAR_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_PSEUDO_SAR_PARAMET_RESP = CMD_SET_PARAMET_RESP,


	/*GSENSOR GATHER used for GPS*/
	CMD_GPS_4774_I2C_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_GPS_4774_I2C_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_GPS_4774_I2C_CONFIG_REQ = CMD_PRIVATE,
	CMD_GPS_4774_I2C_CONFIG_RESP,
	CMD_GPS_4774_I2C_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_GPS_4774_I2C_SELFTEST_RESP = CMD_SELFTEST_RESP,

        //charger
        CMD_CHARGER_I2C_PARAMET_REQ = CMD_SET_PARAMET_REQ,
        CMD_CHARGER_I2C_PARAMET_RESP = CMD_SET_PARAMET_RESP,
        CMD_CHARGER_I2C_SELFTEST_REQ = CMD_SELFTEST_REQ,
        CMD_CHARGER_I2C_SELFTEST_RESP = CMD_SELFTEST_RESP,
        CMD_CHARGER_DATA_REQ,
        CMD_CHARGER_DATA_REPORT_REQ,

	//switch
        CMD_SWITCH_I2C_PARAMET_REQ = CMD_SET_PARAMET_REQ,
        CMD_SWITCH_I2C_PARAMET_RESP = CMD_SET_PARAMET_RESP,

	/*touch*/
	CMD_TP_POSITION_REQ = CMD_PRIVATE,
	CMD_TP_POSITION_RESP,
	CMD_TP_RELEASE_REQ,
	CMD_TP_RELEASE_RESP,

	/*SPI*/
	CMD_SPI_BAUD_REQ = CMD_PRIVATE,
	CMD_SPI_BAUD_RESP,
	CMD_SPI_TRANS_REQ,
	CMD_SPI_TRANS_RESP,

	/*I2C*/
	CMD_I2C_TRANS_REQ = CMD_PRIVATE,
	CMD_I2C_TRANS_RESP,

	/*RGB light and button light*/
	CMD_LIGHT_BRIGHT_REQ = CMD_PRIVATE,
	CMD_LIGHT_BRIGHT_RESP,
	CMD_LIGHT_FLICKER_REQ,
	CMD_LIGHT_FLICKER_RESP,

	/*backlight*/
	CMD_BACKLIGHT_LEVEL_REQ = CMD_PRIVATE,
	CMD_BACKLIGHT_LEVEL_RESP,
	CMD_BACKLIGHT_OUTPUT_REQ,
	CMD_BACKLIGHT_OUTPUT_RESP,
	CMD_BACKLIGHT_OUTPUTSTOP_REQ,
	CMD_BACKLIGHT_OUTPUTSTOP_RESP,

	/*vibrator*/
	CMD_VIBRATOR_SINGLE_REQ = CMD_PRIVATE,
	CMD_VIBRATOR_SINGLE_RESP,
	CMD_VIBRATOR_REPEAT_REQ,
	CMD_VIBRATOR_REPEAT_RESP,

	/*system status*/
	CMD_SYS_STATUSCHANGE_REQ = CMD_PRIVATE,
	CMD_SYS_STATUSCHANGE_RESP,	/*32 */

	CMD_SYS_DYNLOAD_REQ,
	CMD_SYS_DYNLOAD_RESP,
	CMD_SYS_HEARTBEAT_REQ,
	CMD_SYS_HEARTBEAT_RESP,
	CMD_SYS_LOG_LEVEL_REQ,
	CMD_SYS_LOG_LEVEL_RESP,
	CMD_SYS_CTS_RESTRICT_MODE_REQ,
	CMD_SYS_CTS_RESTRICT_MODE_RESP,

	/*LOG*/
	CMD_LOG_REPORT_REQ = CMD_PRIVATE,
	CMD_LOG_REPORT_RESP,
	CMD_LOG_CONFIG_REQ,
	CMD_LOG_CONFIG_RESP,
	CMD_LOG_POWER_REQ,
	CMD_LOG_POWER_RESP,
	/*log buff*/
	CMD_LOG_SER_REQ = CMD_PRIVATE,
	CMD_LOG_USEBUF_REQ,
	CMD_LOG_BUFF_ALERT,
	CMD_LOG_BUFF_FLUSH,
	CMD_LOG_BUFF_FLUSHP,

	/*motion*/
	CMD_MOTION_OPEN_REQ = CMD_CMN_OPEN_REQ,
	CMD_MOTION_OPEN_RESP,
	CMD_MOTION_CLOSE_REQ,
	CMD_MOTION_CLOSE_RESP,
	CMD_MOTION_INTERVAL_REQ,
	CMD_MOTION_INTERVAL_RESP,
	CMD_MOTION_ATTR_ENABLE_REQ = CMD_PRIVATE,
	CMD_MOTION_ATTR_ENABLE_RESP,
	CMD_MOTION_ATTR_DISABLE_REQ,
	CMD_MOTION_ATTR_DISABLE_RESP,
	CMD_MOTION_REPORT_REQ,
	CMD_MOTION_REPORT_RESP,
	CMD_MOTION_SET_PARA_REQ,
	CMD_MOTION_SET_PARA_RESP,

	//ca
	CMD_CA_OPEN_REQ = CMD_CMN_OPEN_REQ,
	CMD_CA_OPEN_RESP,
	CMD_CA_CLOSE_REQ,
	CMD_CA_CLOSE_RESP,
	CMD_CA_INTERVAL_REQ,
	CMD_CA_INTERVAL_RESP,
	CMD_CA_ATTR_ENABLE_REQ = CMD_PRIVATE,
	CMD_CA_ATTR_ENABLE_RESP,
	CMD_CA_ATTR_DISABLE_REQ,
	CMD_CA_ATTR_DISABLE_RESP,
	CMD_CA_REPORT_REQ,
	CMD_CA_REPORT_RESP,

	CMD_FINGERPRINT_OPEN_REQ = CMD_CMN_OPEN_REQ,
	CMD_FINGERPRINT_OPEN_RESP,
	CMD_FINGERPRINT_CLOSE_REQ,
	CMD_FINGERPRINT_CLOSE_RESP,
	CMD_FINGERPRINT_INTERVAL_REQ,
	CMD_FINGERPRINT_INTERVAL_RESP,
	CMD_FINGERPRINT_REPORT_REQ,
	CMD_FINGERPRINT_REPORT_RESP,
	CMD_FINGERPRINT_SPI_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_FINGERPRINT_SPI_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	//ois
	CMD_OIS_OPEN_REQ = CMD_CMN_OPEN_REQ,
	CMD_OIS_OPEN_RESP,
	CMD_OIS_CLOSE_REQ,
	CMD_OIS_CLOSE_RESP,
	CMD_OIS_INTERVAL_REQ,
	CMD_OIS_INTERVAL_RESP,

	//finger sense
	CMD_ACCEL_FINGERSENSE_ENABLE_REQ = CMD_PRIVATE,
	CMD_ACCEL_FINGERSENSE_ENABLE_RESP,
	CMD_ACCEL_FINGERSENSE_CLOSE_REQ,
	CMD_ACCEL_FINGERSENSE_CLOSE_RESP,
	CMD_ACCEL_FINGERSENSE_REQ_DATA_REQ,
	CMD_ACCEL_FINGERSENSE_DATA_REPORT,

	/*current*/
	CMD_CURRENT_GET_REQ = CMD_PRIVATE,
	CMD_CURRENT_GET_RESP,
	CMD_CURRENT_UNGET_REQ,
	CMD_CURRENT_UNGET_RESP,
	CMD_CURRENT_DATA_REQ,
	CMD_CURRENT_DATA_RESP,
	CMD_CURRENT_CFG_REQ,
	CMD_CURRENT_CFG_RESP,
	/*tag pdr*/
	CMD_FLP_PDR_DATA_REQ = CMD_PRIVATE,
	CMD_FLP_PDR_DATA_RESP,
	CMD_FLP_PDR_START_REQ,
	CMD_FLP_PDR_START_RESP,
	CMD_FLP_PDR_STOP_REQ,
	CMD_FLP_PDR_STOP_RESP,
	CMD_FLP_PDR_WRITE_REQ,
	CMD_FLP_PDR_WRITE_RESP,
	CMD_FLP_PDR_UPDATE_REQ,
	CMD_FLP_PDR_UPDATE_RESP,
	CMD_FLP_PDR_FLUSH_REQ,
	CMD_FLP_PDR_FLUSH_RESP,
	CMD_FLP_PDR_UNRELIABLE_REQ,
	CMD_FLP_PDR_UNRELIABLE_RESP,
	CMD_FLP_PDR_STEPCFG_REQ,
	CMD_FLP_PDR_STEPCFG_RESP,
	/*tag ar*/
	CMD_FLP_AR_DATA_REQ = CMD_PRIVATE,
	CMD_FLP_AR_DATA_RESP,
	CMD_FLP_AR_START_REQ,
	CMD_FLP_AR_START_RESP,
	CMD_FLP_AR_CONFIG_REQ = CMD_FLP_AR_START_REQ,
	CMD_FLP_AR_CONFIG_RESP,
	CMD_FLP_AR_STOP_REQ,
	CMD_FLP_AR_STOP_RESP,
	CMD_FLP_AR_UPDATE_REQ,
	CMD_FLP_AR_UPDATE_RESP,
	CMD_FLP_AR_FLUSH_REQ,
	CMD_FLP_AR_FLUSH_RESP,
	CMD_CELL_INFO_DATA_REQ,
	CMD_CELL_INFO_DATA_RESP,
	CMD_FLP_AR_GET_STATE_REQ,
	CMD_FLP_AR_GET_STATE_RESP,
	/*tag environment*/
	CMD_ENVIRONMENT_DATA_REQ = CMD_PRIVATE,
	CMD_ENVIRONMENT_DATA_RESP,
	CMD_ENVIRONMENT_INIT_REQ,
	CMD_ENVIRONMENT_INIT_RESP,
	CMD_ENVIRONMENT_ENABLE_REQ,
	CMD_ENVIRONMENT_ENABLE_RESP,
	CMD_ENVIRONMENT_DISABLE_REQ,
	CMD_ENVIRONMENT_DISABLE_RESP,
	CMD_ENVIRONMENT_EXIT_REQ,
	CMD_ENVIRONMENT_EXIT_RESP,
	CMD_ENVIRONMENT_DATABASE_REQ,
	CMD_ENVIRONMENT_DATABASE_RESP,
	CMD_ENVIRONMENT_GET_STATE_REQ,
	CMD_ENVIRONMENT_GET_STATE_RESP,

	/*tag flp*/
	CMD_FLP_START_BATCHING_REQ = CMD_PRIVATE,
	CMD_FLP_START_BATCHING_RESP,
	CMD_FLP_STOP_BATCHING_REQ,
	CMD_FLP_STOP_BATCHING_RESP,
	CMD_FLP_UPDATE_BATCHING_REQ,
	CMD_FLP_UPDATE_BATCHING_RESP,
	CMD_FLP_GET_BATCHED_LOCATION_REQ,
	CMD_FLP_GET_BATCHED_LOCATION_RESP,
	CMD_FLP_FLUSH_LOCATION_REQ,
	CMD_FLP_FLUSH_LOCATION_RESP,
	CMD_FLP_INJECT_LOCATION_REQ,
	CMD_FLP_INJECT_LOCATION_RESP,
	CMD_FLP_RESET_REQ,
	CMD_FLP_RESET_RESP,
	CMD_FLP_GET_BATCH_SIZE_REQ,
	CMD_FLP_GET_BATCH_SIZE_RESP,

	CMD_FLP_ADD_GEOF_REQ,
	CMD_FLP_ADD_GEOF_RESP,
	CMD_FLP_REMOVE_GEOF_REQ,
	CMD_FLP_REMOVE_GEOF_RESP,
	CMD_FLP_MODIFY_GEOF_REQ,
	CMD_FLP_MODIFY_GEOF_RESP,

	CMD_FLP_LOCATION_UPDATE_REQ,
	CMD_FLP_LOCATION_UPDATE_RESP,
	CMD_FLP_GEOF_TRANSITION_REQ,
	CMD_FLP_GEOF_TRANSITION_RESP,
	CMD_FLP_GEOF_MONITOR_STATUS_REQ,
	CMD_FLP_GEOF_MONITOR_STATUS_RESP,

	//Always On Display
	CMD_AOD_START_REQ = CMD_PRIVATE,
	CMD_AOD_START_RESP,
	CMD_AOD_STOP_REQ,
	CMD_AOD_STOP_RESP,
	CMD_AOD_START_UPDATING_REQ,
	CMD_AOD_START_UPDATING_RESP,
	CMD_AOD_END_UPDATING_REQ,
	CMD_AOD_END_UPDATING_RESP,
	CMD_AOD_SET_TIME_REQ,
	CMD_AOD_SET_TIME_RESP,
	CMD_AOD_SET_DISPLAY_SPACE_REQ,
	CMD_AOD_SET_DISPLAY_SPACE_RESP,
	CMD_AOD_SETUP_REQ,
	CMD_AOD_SETUP_RESP,

	CMD_MAGN_BRACKET_DATA_REQ = CMD_DATA_REQ,
	CMD_MAGN_BRACKET_DATA_RESP = CMD_DATA_RESP,
	CMD_MAGN_BRACKET_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_MAGN_BRACKET_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_MAGN_BRACKET_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_MAGN_BRACKET_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_MAGN_BRACKET_OFFSET_REQ = CMD_SET_OFFSET_REQ,
	CMD_MAGN_BRACKET_OFFSET_RESP = CMD_SET_OFFSET_RESP,
	CMD_MAGN_BRACKET_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_MAGN_BRACKET_SELFTEST_RESP = CMD_SELFTEST_RESP,

	/*SHAREMEM*/
	CMD_SHMEM_AP_RECV_REQ = CMD_PRIVATE,
	CMD_SHMEM_AP_RECV_RESP,
	CMD_SHMEM_AP_SEND_REQ,
	CMD_SHMEM_AP_SEND_RESP,

	/* SHELL_DBG */
	CMD_SHELL_DBG_REQ = CMD_PRIVATE,
	CMD_SHELL_DBG_RESP,

	/* tag modem for cell info*/
	CMD_MODEM_CELL_INFO_REQ = CMD_PRIVATE,
	CMD_MODEM_CELL_INFO_RESP,

	/* TAG_DATA_PLAYBACK */
	CMD_DATA_PLAYBACK_DATA_READY_REQ = CMD_PRIVATE,         /*BACKPLAY*/
	CMD_DATA_PLAYBACK_DATA_READY_RESP,
	CMD_DATA_PLAYBACK_BUF_READY_REQ,        /*RECORD*/
	CMD_DATA_PLAYBACK_BUF_READY_RESP,

	/* key */
	CMD_KEY_DATA_REQ = CMD_DATA_REQ,
	CMD_KEY_DATA_RESP = CMD_DATA_RESP,
	CMD_KEY_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_KEY_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	CMD_KEY_SELFCALI_REQ = CMD_SELFCALI_REQ,
	CMD_KEY_SELFCALI_RESP = CMD_SELFCALI_RESP,
	CMD_KEY_SET_CALIDATA_REQ = CMD_SET_OFFSET_REQ,
	CMD_KEY_SET_CALIDATA_RESP = CMD_SET_OFFSET_RESP,
	CMD_KEY_SELFTEST_REQ = CMD_SELFTEST_REQ,
	CMD_KEY_SELFTEST_RESP = CMD_SELFTEST_RESP,
	CMD_KEY_FW_DLOAD_REQ = CMD_FW_DLOAD_REQ,
	CMD_KEY_FW_DLOAD_RESP = CMD_FW_DLOAD_RESP,
	CMD_KEY_BACKLIGHT_REQ = CMD_BACKLIGHT_REQ,
	CMD_KEY_BACKLIGHT_RESP = CMD_BACKLIGHT_RESP,

	   /* rpc*/
       CMD_RPC_DATA_REQ = CMD_PRIVATE,
       CMD_RPC_DATA_RESP,
       CMD_RPC_START_REQ,
       CMD_RPC_START_RESP,
       CMD_RPC_STOP_REQ,
       CMD_RPC_STOP_RESP,
       CMD_RPC_UPDATE_REQ,
       CMD_RPC_UPDATE_RESP,

	/* PD */
	CMD_PD_REQ = CMD_PRIVATE,
	CMD_PD_RESP,

	/* KB */
	CMD_KB_OPEN_REQ = CMD_CMN_OPEN_REQ,
	CMD_KB_OPEN_RESP,
	CMD_KB_CLOSE_REQ,
	CMD_KB_CLOSE_RESP,

	CMD_KB_REPORT_REQ,
	CMD_KB_REPORT_RESP,
	CMD_KB_EVENT_REQ = CMD_PRIVATE,
	CMD_KB_EVENT_RESP,
	CMD_KB_PARAMET_REQ = CMD_SET_PARAMET_REQ,
	CMD_KB_PARAMET_RESP = CMD_SET_PARAMET_RESP,
	/*err resp*/
	CMD_ERR_RESP = 0xfe,

} obj_cmd_t;

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

#define CMN_RESP_LENGTH 4
#define RESP_ERRNO_LENGTH   4
#define PKT_LENGTH(pkt) (sizeof(pkt_header_t) + ((const pkt_header_t *)pkt)->length)

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
	uint8_t sub_cmd;
} pkt_fcp_t;

typedef struct {
	pkt_header_resp_t hd;
	int32_t wr;
} pkt_fcp_resp_t;

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
	uint8_t body[];
} pkt_t;

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

//data flag consts
#define DATA_FLAG_FLUSH_OFFSET (0)
#define DATA_FLAG_VALID_TIMESTAMP_OFFSET (1)
#define FLUSH_END (1<<DATA_FLAG_FLUSH_OFFSET)
#define DATA_FLAG_VALID_TIMESTAMP (1<<DATA_FLAG_VALID_TIMESTAMP_OFFSET)

typedef struct
{
    pkt_header_t hd;
    uint16_t data_flag;
    uint16_t cnt;
    uint16_t len_element;
    uint16_t sample_rate;
    uint64_t timestamp;
    struct sensor_data_xyz xyz[];	/*x,y,z,acc,time*/
} pkt_batch_data_req_t;

typedef struct {
	pkt_header_t hd;
	s16 zaxis_data[];
} pkt_fingersense_data_report_req_t;

typedef struct
{
    pkt_header_t hd;
    uint32_t data;
}pkt_charging_data_req_t;

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
	pkt_header_t hd;
	uint16_t data[12];
} pkt_key_data_req_t;

typedef struct
{
	pkt_header_t hd;
	uint16_t data_flag;
	uint16_t cnt;
	uint16_t len_element;
	uint16_t sample_rate;
	uint64_t timestamp;
	int32_t status;
} pkt_magn_bracket_data_req_t;

typedef struct open_param {
	uint32_t period;
	uint32_t batch_count;
	uint8_t mode;
	uint8_t reserved[3];
} open_param_t;

typedef struct close_param {
	uint8_t reserved[4];
} close_param_t;

typedef struct {
	pkt_header_t hd;
	union {
		open_param_t param;
		char app_config[16];
	};
} pkt_cmn_interval_req_t;

typedef struct {
	pkt_header_t hd;
	close_param_t close_param;
} pkt_cmn_close_req_t;

typedef struct {
	pkt_header_resp_t hd;
	int32_t offset_x;
	int32_t offset_y;
	int32_t offset_z;
	/*INT8 reserve;*/
} pkt_accel_selfcali_resp_t;

typedef struct {
	pkt_header_t hd;
	uint32_t log_level;
} pkt_log_level_req_t;

typedef struct {
	pkt_header_t hd;
	int32_t offset_x;
	int32_t offset_y;
	int32_t offset_z;
} pkt_accel_offset_req_t;

typedef struct {
	pkt_header_resp_t hd;
	int32_t offset_x;
	int32_t offset_y;
	int32_t offset_z;
} pkt_gyro_selfcali_resp_t;

typedef struct {
	pkt_header_resp_t hd;
	int32_t offset;
} pkt_ps_selfcali_resp_t;

typedef struct {
	pkt_header_resp_t hd;
	uint16_t sensitity_r;
	uint16_t sensitity_g;
	uint16_t sensitity_b;
	uint16_t sensitity_c;
	uint16_t sensitity_lux;
	uint16_t sensitity_cct;
} pkt_als_selfcali_resp_t;

typedef struct {
	pkt_header_resp_t hd;
	uint8_t rawdata[24];
} pkt_handpress_selfcali_resp_t;

typedef struct {
	pkt_header_t hd;
	int32_t a;
	int32_t b;
	int32_t c;
	int32_t d;
} pkt_rotation_vectors_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint8_t para[];
} pkt_parameter_req_t;
typedef struct {
	pkt_header_t hd;
	uint8_t gyro_type;
} pkt_config_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t data;
} pkt_als_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t data;
} pkt_ps_data_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t baudrate;
} pkt_spi_baud_req_t;

typedef struct
{
	pkt_header_resp_t hd;

	uint16_t swap_flag[3];   //0x06
	uint16_t cal_fact_base[3];  //read:0x71  write:0x79
	uint16_t cal_offset[3];  //0x09
	uint16_t digi_offset[3]; //0x0a
	uint16_t cap_prox_extend_data[2];//3 //3mm and 8mm threshold
}pkt_cap_prox_cali_resp_t;

typedef struct {
	pkt_header_t hd;
	uint8_t busid;
	uint8_t addr;
	uint8_t reg_type;
	uint16_t reg;
	uint16_t length;
} pkt_i2c_read_req_t;

typedef struct {
	pkt_header_resp_t hd;
	uint8_t data[];
} pkt_i2c_read_resp_t;

typedef struct {
	pkt_header_t hd;
	uint8_t busid;
	uint8_t addr;
	uint8_t reg_type;
	uint16_t reg;
	uint16_t length;
	uint8_t data[];
} pkt_i2c_write_req_t;

#define GPIO_CS_NO_MASK           0x1FF
#define GPIO_CS_NO_START          0
#define GPIO_CS_IOMG_MASK         0xE00
#define GPIO_CS_IOMG_START        9
#define SPI_DEFAULT_BAUDRATE      5 /* MHz */
#define SPI_DEFAULT_BITS_P_WD     8
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
	uint32_t duration;
} pkt_vibrator_single_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t index;
	uint32_t size;
	uint32_t pattern[MAX_PATTERN_SIZE];
} pkt_vibrator_repeat_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t rgb;
} pkt_light_bright_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t rgb;
	uint32_t ontime;
	uint32_t offtime;
} pkt_light_flicker_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t level;
} pkt_baklight_level_req_t;

typedef struct {
	pkt_header_t hd;
	uint32_t freq;
	uint32_t duty;
} pkt_backlight_output_req_t;

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
	/*(MAX_PKT_LENGTH-sizeof(PKT_HEADER)-sizeof(End))/sizeof(uint16_t)*/
	uint8_t end;
	uint8_t file_count;
	uint16_t file_list[];
} pkt_sys_dynload_req_t;

typedef struct {
	pkt_header_t hd;
	uint8_t type;
	uint8_t value;
	uint16_t reserve;
} pkt_log_config_req_t;

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
	motion_type_t motion_type;
	uint16_t reserve;
} pkt_motion_open_req_t;

typedef struct {
	pkt_header_t hd;
	motion_type_t motion_type;
	uint16_t reserve;
} pkt_motion_close_req_t;

typedef struct {
	pkt_header_t hd;
	motion_type_t motion_type;
	uint16_t interval;
} pkt_motion_interval_req_t;

typedef struct {
	pkt_header_t hd;
	motion_type_t motion_type;
	uint8_t motion_result;
	int8_t motion_status;
	uint8_t data_len;
	int32_t data[];
} pkt_motion_report_req_t;

typedef struct {
	pkt_header_t hd;
	motion_type_t motion_type;
	uint16_t motion_attribute;
} pkt_motion_attr_enable_req_t;

typedef struct {
	pkt_header_t hd;
	motion_type_t motion_type;
	uint16_t motion_attribute;
} pkt_motion_attr_disable_req_t;

typedef struct {
	pkt_header_t hd;
	int32_t current_now;
} pkt_current_data_req_t;

typedef struct {
	pkt_header_t hd;
	char calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
} pkt_mag_calibrate_data_req_t;

typedef struct {
	pkt_header_t hd;
	char calibrate_data[MAX_GYRO_CALIBRATE_DATA_LENGTH];
} pkt_gyro_calibrate_data_req_t;

typedef struct {
	pkt_header_resp_t hd;
} pkt_airpress_selfcali_resp_t; /*TBD, what content need?*/

#define REPORT_DATA_LEN  (sizeof(pkt_xyz_data_req_t) - sizeof(pkt_header_t))

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
	FILE_ENVIRONMENT,
       FILE_RPC_NEW,               //26
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
	FILE_PSEUDO_SAR,               /* 68 */
	FILE_ADUX1050_DOUBLE_STAGE,     /* 69 */
    FILE_FPC1268_FP = 74,
    FILE_GOODIX8206_FP = 75,
    FILE_FPC1266Z120_FP = 77,
    FILE_GOODIX5296_FP = 78,
    FILE_GOODIX3288_FP = 79,
    FILE_SILEAD6185_FP = 80,
    FILE_SILEAD6275_FP = 81,
    FILE_FPC1265_FP = 83,
    FILE_FPC1022_FP = 84,
    FILE_GOODIX5266_FP = 85,
	FILE_ID_MAX = 89,                       /* MAX VALID FILE ID */
};

#endif
