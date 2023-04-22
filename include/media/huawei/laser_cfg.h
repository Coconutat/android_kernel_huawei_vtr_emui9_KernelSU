


#ifndef __HW_ALAN_KERNEL_HWCAM_LASER_CFG_H__
#define __HW_ALAN_KERNEL_HWCAM_LASER_CFG_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>


typedef enum _tag_hwlaser_config_type
{
    HWCAM_LASER_POWERON,
    HWCAM_LASER_POWEROFF,
    HWCAM_LASER_POWERON_EXT,
    HWCAM_LASER_POWEROFF_EXT,
    HWCAM_LASER_LOADFW,
    HWCAM_LASER_CMD,
    HWCAM_LASER_MATCHID,
    HWCAM_LASER_SET_FLAG,

} hwlaser_config_type_t;

/* 32-bit cmd mask */
enum
{
	LASER_CMD_DIR_FLAG_SHIT = 30,		/* [31:30] CMD dirction type */
	LASER_CMD_RESP_FLAG_SHIT = 28,		/* [   28] CMD response type */
	LASER_CMD_OUT_LEN_SHIT = 22,		/* [27:22] CMD out len 0~63byte */
	LASER_CMD_IN_LEN_SHIT = 16,		/* [21:16] CMD in len 0~63byte */
	LASER_CMD_OPCODE_SHIT = 0,			/* [15: 0] CMD opcode */

	LASER_CMD_DIR_FLAG_MASK = (0x3U << LASER_CMD_DIR_FLAG_SHIT),
	LASER_CMD_RESP_FLAG_MASK = (0x3U << LASER_CMD_RESP_FLAG_SHIT),
	LASER_CMD_OUT_LEN_MASK = (0x3fU << LASER_CMD_OUT_LEN_SHIT),
	LASER_CMD_IN_LEN_MASK = (0x3fU << LASER_CMD_IN_LEN_SHIT),
	LASER_CMD_OPCODE_MASK = (0xffffU << LASER_CMD_OPCODE_SHIT),
};

/* cmd direction type */
enum
{
	LASER_SET_CMD	 = 0x1U,
	LASER_GET_CMD	 = 0x2U,
	LASER_INOUT_CMD = 0x3U,
};

/* cmd response type */
enum
{
    LASER_BLOCK_WRITE_CMD = 2U,
	LASER_BLOCK_RESPONSE_CMD = 1U,
	LASER_NORMAL_RESPONSE_CMD = 0U,
};

enum {
	LASER_NULL = 1<<0,
	LASER_AL6045 = 1<<1,
	LASER_AL6010 = 1<<2,
};

typedef struct _tag_hwlaser_config_data
{
    uint32_t cfgtype;
    uint32_t cmd;
    int laser_type;
    int data;

    union {
        uint8_t  buf[64];
        uint16_t buf16[32];
    } u;
}hwlaser_config_data_t;


enum
{
	HWLASER_NAME_SIZE                          =   32,
	HWLASER_V4L2_EVENT_TYPE                       =   V4L2_EVENT_PRIVATE_START + 0x00080000,

	HWLASER_HIGH_PRIO_EVENT                       =   0x1500,
	HWLASER_MIDDLE_PRIO_EVENT                     =   0x2000,
	HWLASER_LOW_PRIO_EVENT                        =   0x3000,
};

enum
{
    HWLASER_POS_HISP = 0,
    HWLASER_POS_AP,
};

enum
{
    HWLASER_DEFAULT_VERSION = 0,
    HWLASER_L0_VERSION,
    HWLASER_L1_VERSION,
    HWLASER_SHARP_L0_VERSION = 1<<4,
    HWLASER_MAX_VERSION,
};


typedef struct _tag_hwlaser_info
{
    char                                        product_name[HWLASER_NAME_SIZE];
    char                                        name[HWLASER_NAME_SIZE];
    int                                         i2c_idx;
    int                                         valid;
    int                                         version; //defult:HWLASER_L0_VERSION
    int                                         ap_pos;
} hwlaser_info_t;

typedef struct _tag_hwlaser_status_t
{
    // char                                        product_name[HWLASER_NAME_SIZE];
    char                                        name[HWLASER_NAME_SIZE];
    int                                         status; // 0-> ok; other: erro
}hwlaser_status_t;

typedef struct _tag_hwlaser_customer_nvm_managed {
    uint8_t   global_config__spad_enables_ref_0;
    uint8_t   global_config__spad_enables_ref_1;
    uint8_t   global_config__spad_enables_ref_2;
    uint8_t   global_config__spad_enables_ref_3;
    uint8_t   global_config__spad_enables_ref_4;
    uint8_t   global_config__spad_enables_ref_5;
    uint8_t   global_config__ref_en_start_select;
    uint8_t   ref_spad_man__num_requested_ref_spads;
    uint8_t   ref_spad_man__ref_location;
    uint32_t  algo__crosstalk_compensation_plane_offset_kcps;
    int16_t   algo__crosstalk_compensation_x_plane_gradient_kcps;
    int16_t   algo__crosstalk_compensation_y_plane_gradient_kcps;
    uint16_t  ref_spad_char__total_rate_target_mcps;
    int16_t   algo__part_to_part_range_offset_mm;
    int16_t   mm_config__inner_offset_mm;
    int16_t   mm_config__outer_offset_mm;
} hwlaser_customer_nvm_managed_t;

typedef struct _tag_hwlaser_dmax_calibration_data {
    uint16_t  ref__actual_effective_spads;
    uint16_t  ref__peak_signal_count_rate_mcps;
    uint16_t  ref__distance_mm;
    uint16_t   ref_reflectance;
    uint16_t   coverglass_transmission;
} hwlaser_dmax_calibration_data_t;


/****************************************************************/
/**               hwlaser_xtalk_histogram_data_t                                      *********/
/****************************************************************/
#define HWLASER_MAX_BIN_SEQUENCE_LENGTH  6
#define HWLASER_HISTOGRAM_BUFFER_SIZE   24
#define HWLASER_XTALK_HISTO_BINS 12
typedef struct _tag_hwlaser_xtalk_histogram_shape{
    uint8_t  zone_id;
    uint32_t time_stamp;
    uint8_t  PRM_00019;
    uint8_t  PRM_00020;
    uint8_t  PRM_00021;
    uint32_t bin_data[HWLASER_XTALK_HISTO_BINS];
    uint16_t phasecal_result__reference_phase;
    uint8_t  phasecal_result__vcsel_start;
    uint8_t  cal_config__vcsel_start;
    uint16_t vcsel_width;
    uint16_t PRM_00022;
    uint16_t zero_distance_phase;
} hwlaser_xtalk_histogram_shape_t;

typedef struct _tag_hwlaser_histogram_bin_data{
    uint8_t     cfg_device_state;
    uint8_t     rd_device_state;
    uint8_t  zone_id;
    uint32_t time_stamp;
    uint8_t  PRM_00019;
    uint8_t  PRM_00020;
    uint8_t  PRM_00021;
    uint8_t  number_of_ambient_bins;
    uint8_t  bin_seq[HWLASER_MAX_BIN_SEQUENCE_LENGTH];
    uint8_t  bin_rep[HWLASER_MAX_BIN_SEQUENCE_LENGTH];
    int32_t  bin_data[HWLASER_HISTOGRAM_BUFFER_SIZE];
    uint8_t  result__interrupt_status;
    uint8_t  result__range_status;
    uint8_t  result__report_status;
    uint8_t  result__stream_count;
    uint16_t result__dss_actual_effective_spads;
    uint16_t phasecal_result__reference_phase;
    uint8_t  phasecal_result__vcsel_start;
    uint8_t  cal_config__vcsel_start;
    uint16_t vcsel_width;
    uint8_t  PRM_00008;
    uint16_t PRM_00022;
    uint32_t total_periods_elapsed;
    uint32_t peak_duration_us;
    uint32_t woi_duration_us;
    int32_t  min_bin_value;
    int32_t  max_bin_value;
    uint16_t zero_distance_phase;
    uint8_t  number_of_ambient_samples;
    int32_t  ambient_events_sum;
    int32_t  PRM_00028;
    uint8_t  roi_config__user_roi_centre_spad;
    uint8_t  roi_config__user_roi_requested_global_xy_size;
} hwlaser_histogram_bin_data_t;

typedef struct _tag_hwlaser_xtalk_histogram_data {
	hwlaser_xtalk_histogram_shape_t  xtalk_shape;
	hwlaser_histogram_bin_data_t     xtalk_hist_removed;
} hwlaser_xtalk_histogram_data_t;


/****************************************************************/
/**               hwlaser_additional_offset_cal_data_t                               *********/
/****************************************************************/
typedef struct _tag_hwlaser_additional_offset_cal_data{
    uint16_t  result__mm_inner_actual_effective_spads;
    uint16_t  result__mm_outer_actual_effective_spads;
    uint16_t  result__mm_inner_peak_signal_count_rtn_mcps;
    uint16_t  result__mm_outer_peak_signal_count_rtn_mcps;
}hwlaser_additional_offset_cal_data_t;


/*****************************************************************/
/**               hwlaser_optical_centre_t                                                 *********/
/*****************************************************************/
/*L3 does not have lens, need to delete or comment out*/
typedef struct _tag_hwlaser_optical_centre{
    uint8_t   x_centre;
    uint8_t   y_centre;
} hwlaser_optical_centre_t;


/*****************************************************************/
/**               hwlaser_gain_calibration_data_t                                        *********/
/*****************************************************************/
typedef struct _tag_hwlaser_gain_calibration_data{
    uint16_t   standard_ranging_gain_factor;
    uint16_t   histogram_ranging_gain_factor;
} hwlaser_gain_calibration_data_t;


/*****************************************************************/
/**               hwlaser_cal_peak_rate_map_t                                          *********/
/*****************************************************************/
/*maybe need to delete or comment out*/
#define HWLASER_NVM_PEAK_RATE_MAP_SAMPLES  25
typedef struct _tag_hwlaser_cal_peak_rate_map{
    int16_t     cal_distance_mm;
    uint16_t    cal_reflectance_pc;
    uint16_t    max_samples;
    uint16_t    width;
    uint16_t    height;
    uint16_t    peak_rate_mcps[HWLASER_NVM_PEAK_RATE_MAP_SAMPLES];
} hwlaser_cal_peak_rate_map_t;


/*****************************************************************/
/**                                                                                                 *********/
/**               NEW CALIBRATION datastruct::                                        *********/
/**                                                                                                 *********/
/**               hwlaser_calibration_data                                                 *********/
/**                                                                                                 *********/
/*****************************************************************/
typedef struct _tag_hwlaser_calibration_data {
    uint32_t                              struct_version;//maybe need to delete or comment out
    hwlaser_customer_nvm_managed_t        customer;
    hwlaser_dmax_calibration_data_t       fmt_dmax_cal;//maybe need to delete or comment out
    hwlaser_dmax_calibration_data_t       cust_dmax_cal;
    hwlaser_additional_offset_cal_data_t  add_off_cal_data;
    hwlaser_optical_centre_t              optical_centre;//maybe need to delete or comment out
    hwlaser_xtalk_histogram_data_t        xtalkhisto;
    hwlaser_gain_calibration_data_t       gain_cal;
    hwlaser_cal_peak_rate_map_t           cal_peak_rate_map;//maybe need to delete or comment out
} hwlaser_calibration_data;

typedef struct _tag_hwlaser_calibration_FOV {
    float x;
    float y;
    float width;
    float height;
    float angle;
}hwlaser_calibration_FOV;

typedef struct _tag_hwlaser_calibration_data_L1 {
    hwlaser_calibration_FOV               HW_FOV;
    hwlaser_calibration_data              RAW_CalibData;
} hwlaser_calibration_data_L1;

typedef uint32_t FixPoint1616_t;

typedef struct _tag_hwlaser_calibration_data_L0 {
    int magic;
    int32_t offset;
    FixPoint1616_t xtalk;
    float x;
    float y;
    float width;
    float height;
    float angle;
    int dmaxrange;
    int dmaxrate;
    uint32_t spadcount;
    uint8_t spadmod;
    uint8_t vhvset;
    uint8_t phascal;
} hwlaser_calibration_data_L0;

typedef enum {
	HW_CALIB_OFFSET = 1,
	HW_CALIB_XTALK = 2,
	HW_CALIB_DMAX = 3,
} hw_cal_mode_e;

typedef struct __tag_hwlaser_calibration_data {
    int32_t is_read;
    hw_cal_mode_e mode;
    union laser_calibration_data{
        hwlaser_calibration_data_L0  dataL0;
        hwlaser_calibration_data_L1  dataL1;
    }u;
} hwlaser_calibration_data_t;



typedef uint8_t HWLASER_PresetModes;

#define HWLASER_PRESETMODE_SINGLE_MODE                   ((HWLASER_PresetModes)  0)
#define HWLASER_PRESETMODE_CONTINUOUS_MODE       ((HWLASER_PresetModes)  1)
#define HWLASER_PRESETMODE_RANGING                ((HWLASER_PresetModes)  2)
#define HWLASER_PRESETMODE_MULTIZONES_SCANNING              ((HWLASER_PresetModes)  3)
#define HWLASER_PRESETMODE_AUTONOMOUS               ((HWLASER_PresetModes)  4)
#define HWLASER_PRESETMODE_LITE_RANGING           ((HWLASER_PresetModes)  5)
#define HWLASER_PRESETMODE_CONTINUOUS_TIMED_MODE     ((HWLASER_PresetModes)  6)
#define HWLASER_PRESETMODE_OLT                       ((HWLASER_PresetModes)  7)








typedef enum _hwlaser_parameter_name_e
{
	OFFSET_PAR = 0,
	XTALKRATE_PAR = 1,
	XTALKENABLE_PAR = 2,
	GPIOFUNC_PAR = 3,
	LOWTHRESH_PAR = 4,
	HIGHTHRESH_PAR = 5,
	DEVICEMODE_PAR = 6,
	INTERMEASUREMENT_PAR = 7,
	REFERENCESPADS_PAR = 8,
	REFCALIBRATION_PAR = 9,
    POLLDELAY_PAR = 10,
    /*!< TIMINGBUDGET_PAR
     ** @ref parameter.value field is timing budget in micro second
     ** type : int32_t
     ** @note the value cannot be set while ranging will set
     ** ebusy errno,
     ** value set is absorbed at next range start @ref
     *IOCTL_INIT
     **/
    TIMINGBUDGET_PAR = 11,
    /*!< DISTANCEMODE_PAR
     * * valid distance mode value :
     * * @li 1 @a DISTANCEMODE_SHORT
     * * @li 2 @a DISTANCEMODE_MEDIUM
     * * @li 3 @a DISTANCEMODE_LONG
     * * @li 4 @a DISTANCEMODE_AUTO_LITE
     * * @li 5 @a DISTANCEMODE_AUTO
     * * type : int32_t
     * * @warning distance mode can only
     * * be set while not ranging
     *  */
    DISTANCEMODE_PAR = 12,
    ///HW Custom param
    /*!< NVM_PAR
     *  * @ref value : principal x
     *  * type : float
     *  * @ref valu2 : principal y
     *  * type : float
     *  * @note get para from NVM
     *  */
     SIGMALIMIT_PAR = 13,
     SIGNALLIMIT_PAR = 14,
     RIT_PAR = 15,
    NVM_PAR = 0x100,

}hwlaser_parameter_name_e;

typedef hwlaser_parameter_name_e parameter_name_e;


typedef struct _hwlaser_parameter {
    uint32_t is_read;   /*!< [in]1: Get 0: Set*/
    hwlaser_parameter_name_e name;  /*!< [in]parameter to set/get */
    int32_t value;        /*!< [in/out] value in int32_t to set /get */
    int32_t value2;     /*!< [in/out] optional 2nd value int int32_t*/
    int32_t status;     /*!< [out]status of the operation */
} hwlaser_parameter_t;

#define HWLASER_MAX_USER_ZONES                169

typedef struct _tag_user_roi{
    uint8_t   TopLeftX;   /*!< Top Left x coordinate:  0-15 range */
    uint8_t   TopLeftY;   /*!< Top Left y coordinate:  0-15 range */
    uint8_t   BotRightX;  /*!< Bot Right x coordinate: 0-15 range */
    uint8_t   BotRightY;  /*!< Bot Right x coordinate:0-15 range  */
} UserRoi_t;


typedef uint8_t RoiStatus_t;
#define HWLASER_ROISTATUS_NOT_VALID                 ((RoiStatus_t)  0)
#define HWLASER_ROISTATUS_VALID_NOT_LAST            ((RoiStatus_t)  1)
#define HWLASER_ROISTATUS_VALID_LAST                ((RoiStatus_t)  2)

#define HWLASER_MAX_RANGE_RESULTS              4


typedef struct _tag_hwlaser_roi {
    uint8_t      NumberOfRoi;   /*!< Number of Rois defined*/
    UserRoi_t    UserRois[HWLASER_MAX_USER_ZONES];
    /*!< List of Rois */
} RoiConfig_t;


typedef struct _tag_hwlaser_roi_full_t {
    int32_t     is_read;
    /*!<  specify roi transfer direction \n
     ** @li 0 to get roi
     ** @li !0 to set roi
     **/
    RoiConfig_t roi_cfg;
    /*!< roi data array of max length but only requested copy to/from user
     ** space effectively used
     **/
} hwlaser_roi_full_t;


typedef uint8_t RoiStatus_t;
#define HWLASER_ROISTATUS_NOT_VALID                 ((RoiStatus_t)  0)
#define HWLASER_ROISTATUS_VALID_NOT_LAST            ((RoiStatus_t)  1)
#define HWLASER_ROISTATUS_VALID_LAST                ((RoiStatus_t)  2)

#define HWLASER_MAX_RANGE_RESULTS              4

typedef uint32_t FixPoint1616_t;






typedef struct _tag_hwlaser_ranging_measurement_data {

    uint8_t RangeQualityLevel;
        /*!< indicate a quality level in percentage from 0 to 100
         * @warning Not yet implemented
         */

    int16_t RangeMaxMilliMeter;
        /*!< Tells what is the maximum detection distance of the object
         * in current setup and environment conditions (Filled when
         *  applicable)
         */

    int16_t RangeMinMilliMeter;
        /*!< Tells what is the minimum detection distance of the object
         * in current setup and environment conditions (Filled when
         *  applicable)
         */

    FixPoint1616_t SignalRateRtnMegaCps;
        /*!< Return signal rate (MCPS)\n these is a 16.16 fix point
         *  value, which is effectively a measure of target
         *   reflectance.
         */

    FixPoint1616_t AmbientRateRtnMegaCps;
        /*!< Return ambient rate (MCPS)\n these is a 16.16 fix point
         *  value, which is effectively a measure of the ambien
         *  t light.
         */

    FixPoint1616_t SigmaMilliMeter;
        /*!< Return the Sigma value in millimeter */

    int16_t RangeMilliMeter;
        /*!< range distance in millimeter. This should be between
         *  RangeMinMilliMeter and RangeMaxMilliMeter
         */

    uint8_t RangeFractionalPart;
        /*!< Fractional part of range distance. Final value is a
         **  RangeMilliMeter + RangeFractionalPart/256.
         **  @warning Not yet implemented  */

    uint8_t RangeStatus;
        /*!< Range Status for the current measurement. This is device
         **  dependent. Value = 0 means value is valid.
         **/
} hwlaser_RangingMeasurementData_t;












typedef struct _tag_hwlaser_multi_ranging_data{

    uint32_t TimeStamp;
        /*!< 32-bit time stamp.
         * @warning Not yet implemented
         */

    uint8_t StreamCount;
        /*!< 8-bit Stream Count. */

    uint8_t RoiNumber;
        /*!< Denotes on which ROI the range data is related to. */
    uint8_t NumberOfObjectsFound;
        /*!< Indicate the number of objects found in the current ROI.
         ** This is used to know how many ranging data should be get.
         ** NumberOfObjectsFound is in the range 0 to
         ** HWLASER_MAX_RANGE_RESULTS.
         **/
    RoiStatus_t RoiStatus;
        /*!< Indicate if the data read is valid or not or if this is
         ** the last valid data in the ROI.
         **/
    hwlaser_RangingMeasurementData_t RangeData[HWLASER_MAX_RANGE_RESULTS];
        /*!< Range data each target distance */

    uint8_t HasXtalkValueChanged;
        /*!< set to 1 if a new Xtalk value has been computed whilst
         * smudge correction mode enable by with
         * hwlaser_SmudgeCorrectionEnable() function is either
         * hwlaser_SMUDGE_CORRECTION_CONTINUOUS or
         * hwlaser_SMUDGE_CORRECTION_SINGLE.
         */

    uint16_t EffectiveSpadRtnCount;
        /*!< Return the effective SPAD count for the return signal.
         *  To obtain Real value it should be divided by 256
         */

    int16_t DmaxMilliMeter;
        /*!< range Dmax distance in millimeter.
         */

    hwlaser_RangingMeasurementData_t RecommendedDistanceMode;
        /*!< suggestion for a better distance mode choice to improve
         *  range accuracy.
         */
} hwlaser_multiRangingData_t;

typedef struct _tag_hwlaser_ranging_data_L0{
	uint32_t TimeStamp;			  /*!< 32-bit time stamp. */
	uint32_t MeasurementTimeUsec;
	uint16_t RangeMilliMeter;	  /*!< range distance in millimeter. */
	uint16_t RangeDMaxMilliMeter;
	FixPoint1616_t SignalRateRtnMegaCps;
	FixPoint1616_t AmbientRateRtnMegaCps;
	uint16_t EffectiveSpadRtnCount;
	uint8_t ZoneId;
	uint8_t RangeFractionalPart;
	uint8_t RangeStatus;
} hwlaser_RangingData_L0_t;

typedef struct _tag_hwlaser_ranging_data_ext_L0{
    uint32_t SignalRtnCounts20ns;
} hwlaser_RangingData_Ext_L0_t;

typedef struct _tag_hwlaser_ranging_data
{
    union hwlaser_measurement_data
    {
        hwlaser_RangingData_L0_t     dataL0;
        hwlaser_multiRangingData_t   data;
    }u;
    union hwlaser_measurement_data_ext
    {
        hwlaser_RangingData_Ext_L0_t dataExtL0;
    }v;
}hwlaser_RangingData_t;

typedef struct _tag_hwlaser_legacy_ranging_data_L0 {
    unsigned int   status;
    unsigned int   Xtalk;
    unsigned int   SigmaLimitValue;
    unsigned int   SignalLimitValue;
    unsigned int   TimingBudget;
    unsigned int   RangeIgonreThreshold;
} hwlaser_LegacyRangingData_L0_t;

typedef struct _tag_hwlaser_fusion_data {
        hwlaser_RangingData_t rData;
        hwlaser_LegacyRangingData_L0_t legacyData;
} hwlaser_FusionData_t;

/** Select reference spad calibration in @ref HWLASER_IOCTL_PERFORM_CALIBRATION.
 *
 * param1, param2 and param3 not use
 */
#define HWLASER_CALIBRATION_REF_SPAD        0

/** Select crosstalk calibration in @ref HWLASER_IOCTL_PERFORM_CALIBRATION.
 **
 ** param1 is calibration method. param2 and param3 not use.
 **/
#define HWLASER_CALIBRATION_CROSSTALK       1

/** Select offset calibration  @ref HWLASER_IOCTL_PERFORM_CALIBRATION.
 ** param1 is target distance in mm. param2 and
 ** param3 not use.
 **/
#define HWLASER_CALIBRATION_OFFSET          2

/** Select reference calibration in @ref HWLASER_IOCTL_PERFORM_CALIBRATION.
 *
 * param1, param2 and param3 not use
 */
#define HWLASER_CALIBRATION_REFERENCE       3

/** Select spad calibration in @ref HWLASER_IOCTL_PERFORM_CALIBRATION.
 *
 * param1, param2 and param3 not use
 */
#define HWLASER_CALIBRATION_SPAD            4

/** @defgroup HWLASER_define_OffsetCal_group Defines Offset Calibration modes
*  Defines all possible Offset Calibration modes for the device
*  @{
*/
typedef uint8_t HWLASER_OffsetCalibrationModes;

#define HWLASER_OFFSETCALIBRATIONMODE_STANDARD \
	((HWLASER_OffsetCalibrationModes)  1)
#define HWLASER_OFFSETCALIBRATIONMODE_PRERANGE_ONLY  \
	((HWLASER_OffsetCalibrationModes)  2)
#define HWLASER_OFFSETCALIBRATIONMODE_MULTI_ZONE    \
	((HWLASER_OffsetCalibrationModes)  3)

typedef struct _tag_hwlaser_ioctl_perform_calibration {
    uint32_t calibration_type;
    /*!< [in] select which calibration to do :
     ** @li @ref HWLASER_CALIBRATION_REF_SPAD
     ** @li @ref HWLASER_CALIBRATION_CROSSTALK
     ** @li @ref HWLASER_CALIBRATION_OFFSET
     **/
    uint32_t param1;
    /*!< [in] first param. Usage depends on calibration_type */
    uint32_t param2;
    /*!< [in] second param. Usage depends on calibration_type */
    uint32_t param3;
    /*!< [in] third param. Usage depends on calibration_type */
} hwlaser_ioctl_perform_calibration_t;

typedef enum _tag_hwlaser_event_kind
{
    HWLASER_INFO_CMD_FINISH,
    HWLASER_INFO_ERROR,
    HWLASER_INFO_DUMP,
    HWLASER_INFO_OIS_DONE,
} hwlaser_event_kind_t;

typedef struct _tag_hwlaser_event
{
    hwlaser_event_kind_t                          kind;
    union // can ONLY place 10 int fields here.
    {
        struct
        {
            uint32_t                            cmd;
            uint32_t                            result;
        }                                       cmd_finish;
        struct
        {
            uint32_t                            id;
        }                                       error;
        struct
        {
            uint32_t                            type;
        }                                       dump;
    }data;
}hwlaser_event_t;

typedef struct _tag_hw_laser_fn_t {
    long (*laser_ioctl)(void *laser_data, unsigned int cmd, void  *p);
}hw_laser_fn_t;

typedef struct _tag_hw_laser_ctrl {
    hw_laser_fn_t *func_tbl;
    void *data;
    struct v4l2_fh *fh;
}hw_laser_ctrl_t;

//extern int laser_probe(struct i2c_client *client, const struct i2c_device_id *id);

//extern int laser_remove(struct i2c_client *client);

extern long stmvl53l0_laser_ioctl(void *hw_data, unsigned int cmd, void  *p); 


#define HWLASER_IOCTL_GET_INFO                 _IOR('S', BASE_VIDIOC_PRIVATE + 1, hwlaser_info_t)

#define HWLASER_IOCTL_CONFIG               	_IOWR('A', BASE_VIDIOC_PRIVATE + 2, hwlaser_config_data_t)



#define HWLASER_IOCTL_POWERON  _IO('B', BASE_VIDIOC_PRIVATE + 3)
#define HWLASER_IOCTL_MATCHID                  _IOWR('C', BASE_VIDIOC_PRIVATE + 4, hwlaser_status_t)
#define HWLASER_IOCTL_POWEROFF                 _IO('D', BASE_VIDIOC_PRIVATE + 5)
#define HWLASER_IOCTL_INIT                     _IO('E', BASE_VIDIOC_PRIVATE + 6)
#define HWLASER_IOCTL_CALIBRATION_DATA         _IOWR('F', BASE_VIDIOC_PRIVATE + 7, hwlaser_calibration_data_t)
#define HWLASER_IOCTL_PARAMETER                _IOWR('G', BASE_VIDIOC_PRIVATE + 8, hwlaser_parameter_t)
#define HWLASER_IOCTL_ROI                      _IOWR('H', BASE_VIDIOC_PRIVATE + 9, hwlaser_roi_full_t)
#define HWLASER_IOCTL_START                    _IO('I', BASE_VIDIOC_PRIVATE + 10)
#define HWLASER_IOCTL_STOP                     _IO('J', BASE_VIDIOC_PRIVATE + 11)
#define HWLASER_IOCTL_MZ_DATA                  _IOWR('K', BASE_VIDIOC_PRIVATE + 12, hwlaser_RangingData_t)
#define HWLASER_IOCTL_PERFORM_CALIBRATION      _IOWR('L', BASE_VIDIOC_PRIVATE + 13, hwlaser_ioctl_perform_calibration_t)











#endif // __HW_ALAN_KERNEL_HWCAM_LASER_CFG_H__

