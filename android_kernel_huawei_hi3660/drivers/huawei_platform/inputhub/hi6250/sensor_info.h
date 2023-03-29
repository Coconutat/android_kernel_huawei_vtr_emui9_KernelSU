/*
 *  drivers/misc/inputhub/inputhub_route.h
 *  Sensor Hub Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: qindiwen <inputhub@huawei.com>
 *
 */
#ifndef	__SENSORS_H__
#define	__SENSORS_H__

#include "sensor_detect.h"

#define PDC_SIZE		27
#define SENSOR_PLATFORM_EXTEND_DATA_SIZE    50
#define EXTEND_DATA_TYPE_IN_DTS_BYTE        0
#define EXTEND_DATA_TYPE_IN_DTS_HALF_WORD   1
#define EXTEND_DATA_TYPE_IN_DTS_WORD        2
#define MAX_CHIP_INFO_LEN (50)
#define MAX_STR_CHARGE_SIZE  (50)

/* Number of z-axis samples required by FingerSense at 1.6KHz ODR */
#define FINGERSENSE_DATA_NSAMPLES 128
#define HALL_COVERD     (1)
#define SENSOR_LIST_NUM 50

#define NV_READ_TAG	1
#define NV_WRITE_TAG	0
#define PS_CALIDATA_NV_NUM	334
#define PS_CALIDATA_NV_SIZE  12
#define ALS_CALIDATA_NV_NUM	339
#define ALS_CALIDATA_NV_SIZE  12
#define GYRO_CALIDATA_NV_NUM	341
#define GYRO_CALIDATA_NV_SIZE  60
#define HANDPRESS_CALIDATA_NV_NUM  354
#define HANDPRESS_CALIDATA_NV_SIZE  24
#define AIRPRESS_CALIDATA_NV_NUM	332
#define AIRPRESS_CALIDATA_NV_SIZE  4
#define CAP_PROX_CALIDATA_NV_NUM  310
#define CAP_PROX_CALIDATA_NV_SIZE  28
#define TP_COLOR_NV_NUM 16
#define TP_COLOR_NV_SIZE 15
#define pinhole_para_size (10)
#define TMD2745_PARA_SIZE (10)
#define RPR531_PARA_SIZE (16)

#define DEFAULT_TPLCD (0)
#define LG_TPLCD (1)
#define JDI_TPLCD (2)
#define TRULY_TPLCD (2)
#define MUTTO_TPLCD (3)

#define WHITE	0xE1
#define BLACK	0xD2
#define BLACK2   0x4B
#define PINK	0xC3
#define RED		0xB4
#define YELLOW	0xA5
#define BLUE	0x96
#define GOLD  0x87
#define GRAY  0x78
#define BROWN  0x69
#define CAFE_2  0x5A
#define SILVER  0x3C
#define SEND_ERROR  (-1)
#define SEND_SUC       (0)
enum ts_panel_id {
	TS_PANEL_OFILIM 	 	= 0,
	TS_PANEL_EELY		 	= 1,
	TS_PANEL_TRULY	 		= 2,
	TS_PANEL_MUTTO	 		= 3,
	TS_PANEL_GIS		 	= 4,
	TS_PANEL_JUNDA	 		= 5,
	TS_PANEL_LENS 		 	= 6,
	TS_PANEL_YASSY	 		= 7,
	TS_PANEL_JDI 		 	= 6,
	TS_PANEL_SAMSUNG  		= 9,
	TS_PANEL_LG 		 	= 10,
	TS_PANEL_TIANMA 	 	= 11,
	TS_PANEL_CMI 		 	= 12,
	TS_PANEL_BOE  		 	= 13,
	TS_PANEL_CTC 		 	= 14,
	TS_PANEL_EDO 		 	= 15,
	TS_PANEL_SHARP	 		= 16,
	TS_PANEL_AUO 			= 17,
	TS_PANEL_TOPTOUCH 		= 18,
	TS_PANEL_BOE_BAK		= 19,
	TS_PANEL_CTC_BAK 		= 20,
	TS_PANEL_UNKNOWN 		= 0xFF,
};

typedef struct _BH1745_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_lcd_manufacture;
	uint8_t tp_color;
	s16 bh745_para[25];	/*give to bh1745 rgb sensor use,output lux and cct will use these para*/
} BH1745_ALS_PARA_TABLE;

typedef struct _APDS9251_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_lcd_manufacture;
	uint8_t tp_color;
	s16 apds251_para[23];	/*give to apds251 rgb sensor use,output lux and cct will use these par*/
} APDS9251_ALS_PARA_TABLE;	/*the apds251_para size must small SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef struct _RPR531_ALS_PARA_TABLE{
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_manufacture;
	s16 rpr531_para[RPR531_PARA_SIZE];
}RPR531_ALS_PARA_TABLE;
typedef struct _PINHOLE_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t sens_name;
	uint8_t tp_manufacture;
	s16 pinhole_para[pinhole_para_size];/*modify the size of the array to pass more data */
} PINHOLE_ALS_PARA_TABLE;	/*the ph_para size must small SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef struct _TMD2745_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_manufacture;
	uint8_t tp_color;
	s16 als_para[TMD2745_PARA_SIZE];/*modify the size of the array to pass more data */
} TMD2745_ALS_PARA_TABLE;           /*keep als_para size smaller than SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef enum {
	SUC = 1,
	EXEC_FAIL,
	NV_FAIL,
	COMMU_FAIL,
	POSITION_FAIL,
	RET_TYPE_MAX
} RET_TYPE;

enum detect_state {
	DET_INIT = 0,
	DET_FAIL,
	DET_SUCC
};

enum mag_charger_data{
	NO_CHARGER = 0,                  // not need any charger for mag
	NO_CAL_BY_CHARGER,                // when charger no calibrate but let calibrate order to 0
	CAL_BY_CHARGER                      //when charger calibrate by current
};

struct i2c_data {
	uint8_t bus_num;
	uint8_t i2c_address;
	uint16_t reg_address;
	uint8_t reg_type;
	uint8_t reg_len;
	uint8_t data[MAX_I2C_DATA_LENGTH];
};

struct t_sensor_get_data {
	atomic_t reading;
	struct completion complete;
	struct sensor_data data;
};

struct charge_current_mag
{
	char str_charge[MAX_STR_CHARGE_SIZE];
	int current_offset_x;
	int current_offset_y;
	int current_offset_z;
};
struct sensor_status {
	int status[TAG_SENSOR_END];	/*record whether sensor is in activate status, already opened and setdelay*/
	int delay[TAG_SENSOR_END];	/*record sensor delay time*/
	int opened[TAG_SENSOR_END];	/*record whether sensor was opened*/
	int batch_cnt[TAG_SENSOR_END];
	char gyro_selfTest_result[5];
	char mag_selfTest_result[5];
	char accel_selfTest_result[5];
	char gps_4774_i2c_selfTest_result[5];
	char handpress_selfTest_result[5];
	char selftest_result[TAG_SENSOR_END][5];
	int gyro_ois_status;
	struct t_sensor_get_data get_data[TAG_SENSOR_END];
};
struct sensorlist_info{
	/**
	 * Name of this sensor.
	 * All sensors of the same "type" must have a different "name".
	 */
	char     name[50];

	/** vendor of the hardware part */
	char     vendor[50];
	/**
	* version of the hardware part + driver. The value of this field
	* must increase when the driver is updated in a way that changes the
	* output of this sensor. This is important for fused sensors when the
	* fusion algorithm is updated.
	*/
	int32_t             version;

	/** maximum range of this sensor's value in SI units */
	int32_t           maxRange;

	/** smallest difference between two values reported by this sensor */
	int32_t           resolution;

	/** rough estimate of this sensor's power consumption in mA */
	int32_t           power;

	/**
	* this value depends on the reporting mode:
	*
	* continuous: minimum sample period allowed in microseconds
	* on-change : 0
	* one-shot  :-1
	* special   : 0, unless otherwise noted
	*/
	int32_t         minDelay;

	/**
	* number of events reserved for this sensor in the batch mode FIFO.
	* If there is a dedicated FIFO for this sensor, then this is the
	* size of this FIFO. If the FIFO is shared with other sensors,
	* this is the size reserved for that sensor and it can be zero.
	*/
	uint32_t        fifoReservedEventCount;

	/**
	* maximum number of events of this sensor that could be batched.
	* This is especially relevant when the FIFO is shared between
	* several sensors; this value is then set to the size of that FIFO.
	*/
	uint32_t        fifoMaxEventCount;
	/**
	* This value is defined only for continuous mode and on-change sensors. It is the delay between
	* two sensor events corresponding to the lowest frequency that this sensor supports. When lower
	* frequencies are requested through batch()/setDelay() the events will be generated at this
	* frequency instead. It can be used by the framework or applications to estimate when the batch
	* FIFO may be full.
	*
	* @note
	*   1) period_ns is in nanoseconds where as maxDelay/minDelay are in microseconds.
	*              continuous, on-change: maximum sampling period allowed in microseconds.
	*              one-shot, special : 0
	*   2) maxDelay should always fit within a 32 bit signed integer. It is declared as 64 bit
	*      on 64 bit architectures only for binary compatibility reasons.
	* Availability: SENSORS_DEVICE_API_VERSION_1_3
	*/
	int32_t maxDelay;

	/**
	* Flags for sensor. See SENSOR_FLAG_* above. Only the least significant 32 bits are used here.
	* It is declared as 64 bit on 64 bit architectures only for binary compatibility reasons.
	* Availability: SENSORS_DEVICE_API_VERSION_1_3
	*/
	uint32_t flags;
};
struct g_sensor_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t axis_map_x;
	uint8_t axis_map_y;
	uint8_t axis_map_z;
	uint8_t negate_x;
	uint8_t negate_y;
	uint8_t negate_z;
	uint8_t gpio_int1;
	uint8_t gpio_int2;
	uint8_t gpio_int2_sh;
	uint16_t poll_interval;
	int offset_x;
	int offset_y;
	int offset_z;
	int sensitivity_x;
	int sensitivity_y;
	int sensitivity_z;
	uint8_t calibrate_style;
	uint8_t data_convert;
	uint8_t g_sensor_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};
struct gyro_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t exist;
	uint8_t axis_map_x;
	uint8_t axis_map_y;
	uint8_t axis_map_z;
	uint8_t negate_x;
	uint8_t negate_y;
	uint8_t negate_z;
	uint8_t gpio_int1;
	uint8_t gpio_int2;
	uint8_t gpio_int2_sh;
	uint16_t poll_interval;
	uint8_t gyro_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};
struct compass_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t axis_map_x;
	uint8_t axis_map_y;
	uint8_t axis_map_z;
	uint8_t negate_x;
	uint8_t negate_y;
	uint8_t negate_z;
	uint8_t outbit;
	uint8_t calibrate_method;
	uint8_t gpio_drdy;
	uint8_t gpio_rst;
	uint8_t soft_filter;
	uint8_t charger_trigger;
	uint8_t pdc_data[PDC_SIZE];
	uint16_t poll_interval;
	uint8_t compass_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

struct rgb_lux_cal_raw_data {
	uint16_t cal_average_red;
	uint16_t cal_average_green;
	uint16_t cal_average_blue;
	uint16_t cal_average_clear;
	uint16_t cal_average_als;
	uint16_t cal_average_cct;
};

struct rgb_als_calibrate_result {
	uint16_t cal_result_red;
	uint16_t cal_result_green;
	uint16_t cal_result_blue;
	uint16_t cal_result_clear;
	uint16_t cal_result_als;
	uint16_t cal_result_cct;
};

struct rgb_lux_cal_pare_to_nv {
	struct rgb_als_calibrate_result rgb_cal_ratio_result;
	struct rgb_lux_cal_raw_data rgb_cal_raw_data;
};

struct als_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t gpio_int1;
	uint8_t atime;
	uint8_t again;
	uint16_t poll_interval;
	uint16_t init_time;
	int threshold_value;
	int GA1;
	int GA2;
	int GA3;
	int COE_B;
	int COE_C;
	int COE_D;
	uint8_t als_phone_type;
	uint8_t als_phone_version;
	uint8_t als_phone_tp_colour;
	uint8_t als_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

struct sleeve_detect_pare
{
	unsigned int tp_color;
	unsigned int sleeve_detect_threshhold;
};
#define MAX_PHONE_COLOR_NUM  15
struct ps_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t ps_pulse_count;
	uint8_t gpio_int1;
	uint8_t persistent;
	uint8_t ptime;
	uint8_t p_on;		/*need to close oscillator*/
	uint8_t ps_oily_threshold;
	uint16_t poll_interval;
	uint16_t init_time;
	uint16_t use_oily_judge;
	int min_proximity_value;
	int pwindows_value;
	int pwave_value;
	int threshold_value;
	int rdata_under_sun;
	uint8_t ps_flag;
	uint8_t ps_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

struct ps_extend_platform_data {
	uint8_t external_ir_mode_flag;
	uint8_t external_ir_avg_algo;
	int external_ir_calibrate_noise_max;
	int external_ir_calibrate_noise_min;
	int external_ir_calibrate_far_threshold_max;
	int external_ir_calibrate_far_threshold_min;
	int external_ir_calibrate_near_threshold_max;
	int external_ir_calibrate_near_threshold_min;
	int external_ir_calibrate_pwindows_max;
	int external_ir_calibrate_pwindows_min;
	int external_ir_calibrate_pwave_max;
	int external_ir_calibrate_pwave_min;
	int min_proximity_value;
	int pwindows_value;
	int pwave_value;
	int threshold_value;
};

struct ps_external_ir_param {
	int external_ir;
	int internal_ir_min_proximity_value;
	int external_ir_min_proximity_value;
	int internal_ir_pwindows_value;
	int external_ir_pwindows_value;
	int internal_ir_pwave_value;
	int external_ir_pwave_value;
	int internal_ir_threshold_value;
	int external_ir_threshold_value;
	int external_ir_enable_gpio;
};

struct airpress_platform_data {
	struct sensor_combo_cfg cfg;
	int offset;
	uint16_t poll_interval;
	uint8_t airpress_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

#define CYPRESS_CHIPS		2
struct handpress_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t bootloader_type;
	uint8_t id[CYPRESS_CHIPS];
	uint8_t i2c_address[CYPRESS_CHIPS];
	uint8_t t_pionts[CYPRESS_CHIPS];
	uint16_t poll_interval;
	uint32_t irq[CYPRESS_CHIPS];
	uint8_t handpress_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

/*
* sar platform data
*/
struct adux_sar_data {
	uint32_t init_reg_val[17];
	uint16_t high_threshold;
	uint16_t low_threshold;
	uint16_t threshold_to_modem[8];
	uint16_t swap_flag[3];
	uint16_t cal_fact_base[3];
	uint16_t cal_offset[3];
	uint16_t digi_offset[3];
	uint16_t cap_prox_extend_data[2];//3
};

struct cypress_sar_data {
	uint16_t threshold_to_ap;
	uint16_t threshold_to_modem[8];
};

struct semteck_sar_data {
	uint16_t threshold_to_ap;
	uint16_t threshold_to_modem[8];
	uint32_t init_reg_val[17];
	uint8_t ph;
	uint16_t calibrate_thred[4];
	uint16_t offset_check;
};
union sar_data {
	struct cypress_sar_data cypress_data;
	struct adux_sar_data	adux_data;
	struct semteck_sar_data	semteck_data;
	//add the others here
};

/*
*
* calibrate_type: config by bit(0~7): 0-free 1-near 2-far other-reserve;
* sar_datas: data for diffrent devices.
*/
struct sar_platform_data {
	struct sensor_combo_cfg cfg;
	uint16_t poll_interval;
	uint16_t flag_for_threshold_config;
	int  calibrate_type;
	int  calibrate_delay;
	union sar_data	sar_datas;
	uint8_t  stage_num;
};

struct sar_cap_proc_calibrate_data {
	uint16_t swap_flag[3];
	uint16_t cal_fact_base[3];
	uint16_t cal_offset[3];
	uint16_t digi_offset[3];
	uint16_t cap_prox_extend_data[2];
};

struct sar_cypress_calibrate_data {
	uint16_t sar_idac;
	uint16_t raw_data;
	uint16_t near_signaldata;
	uint16_t far_signaldata;
};

struct sar_semtech_calibrate_data {
	uint16_t offset;
	uint16_t diff;
};
union sar_calibrate_data {
	struct sar_cap_proc_calibrate_data cap_cali_data;
	struct sar_cypress_calibrate_data cypres_cali_data;
	struct sar_semtech_calibrate_data semtech_cali_data;
};

struct cap_prox_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t gpio_int;
	uint16_t poll_interval;
	int  calibrate_type;
	uint32_t init_reg_val[17];	/* init value */
	uint16_t high_threshold;
	uint16_t low_threshold;
	uint16_t swap_flag[3];   //0x06
	uint16_t cal_fact_base[3];  //read:0x71  write:0x79
	uint16_t cal_offset[3];  //0x09
	uint16_t digi_offset[3]; //0x0a
	uint16_t cap_prox_extend_data[2];//3 //3mm and 8mm threshold
};

struct gps_4774_platform_data {
	struct sensor_combo_cfg cfg;
	uint16_t poll_interval;
	uint8_t gpio1_gps_cmd_ap;
	uint8_t gpio1_gps_cmd_sh;
	uint8_t gpio2_gps_ready_ap;
	uint8_t gpio2_gps_ready_sh;
	uint8_t gpio3_wakeup_gps_ap;
	uint8_t gpio3_wakeup_gps_sh;
};

struct fingerprint_platform_data {
	struct sensor_combo_cfg cfg;
	uint16_t reg;
	uint16_t chip_id;
	uint16_t gpio_irq;
	uint16_t gpio_irq_sh;
	uint16_t gpio_cs;
	uint16_t gpio_reset;
	uint16_t poll_interval;
};

static struct key_platform_data {
	struct sensor_combo_cfg cfg;
	uint8_t i2c_address_bootloader;
	uint8_t gpio_key_int;
	uint8_t gpio_key_int_sh;
	uint16_t poll_interval;
	uint8_t reserve[16];
};

struct charger_platform_data {
	struct sensor_combo_cfg cfg;
	int bat_comp;
	int vclamp;
	int ico_current_mode;
	int gpio_cd;
	int gpio_cd_sh;
	int gpio_int;
	int gpio_int_sh;
	int rilim;  //this should be configured in dts file based on the real value of the Iin limit resistance
	int adc_channel_iin;  //this should be configured in dts file based on the real adc channel number
	int adc_channel_vbat_sys;
	uint32_t is_board_type; /*0:sft 1:udp 2:asic*/
	uint32_t fcp_support;
	uint32_t fcp_no_switch;
};

struct switch_platform_data {
	struct sensor_combo_cfg cfg;
	int fsa9685_usbid_enable;
	int fsa9685_fcp_support;
	int fsa9685_scp_support;
	int fsa9685_mhl_detect_disable;
	int two_switch_flag;
	int gpio_intb;
};

struct magn_bracket_platform_data {
	struct sensor_combo_cfg cfg;
	int mag_x_change_lower;
	int mag_x_change_upper;
	int mag_y_change_lower;
	int mag_y_change_upper;
	int mag_z_change_lower;
	int mag_z_change_upper;
};

struct motion_platform_data {
	uint8_t pickup_data_flag;
	int angle_gap;
};

typedef struct {
	unsigned int sub_cmd;
} rpc_ioctl_t;

struct pdr_start_config {
	unsigned int report_interval;
	unsigned int report_precise;
	unsigned int report_count;
	unsigned int report_times;
};

typedef struct {
	unsigned int sub_cmd;
	union {
		struct pdr_start_config start_param;
		unsigned int stop_param;
	};
} pdr_ioctl_t;

typedef struct pdr_result_data {
	unsigned int time_stamp[2];
	unsigned int step_count;
	int relative_position_x;
	int relative_position_y;
	int velocity_x;
	int velocity_y;
	unsigned int migration_distance;
	unsigned int absolute_altitude;
	unsigned int absolute_bearing;
	unsigned int reliability_flag;
} pdr_result_data_t;

extern void read_als_data_from_dts(struct device_node *dn);
extern int write_magsensor_calibrate_data_to_nv(const char *src);
extern int detect_i2c_device(struct device_node *dn, char *device_name);
extern void read_chip_info(struct device_node *dn, SENSOR_DETECT_LIST sname);
extern int fill_extend_data_in_dts(struct device_node *dn,
			const char *name, unsigned char *dest,
			size_t max_size, int flag);
extern int mcu_i2c_rw(uint8_t bus_num, uint8_t i2c_add,
	uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern int mcu_spi_rw(uint8_t bus_num, union SPI_CTRL ctrl,
	uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern int combo_bus_trans(struct sensor_combo_cfg *p_cfg, uint8_t *tx,
	uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern int sensor_set_current_info(void);
extern void __dmd_log_report(int dmd_mark, const char *err_func,
			const char *err_msg);
extern int send_fileid_to_mcu(void);
extern int write_customize_cmd_noresp(int tag, int cmd, const void *data,
			int length);
extern int write_gyro_sensor_offset_to_nv(char *temp, int length);

extern struct sensor_status sensor_status;
extern bool sensor_info_isensor_version;
/* Indicates whether or not FingerSense mode is enabled*/
extern bool fingersense_enabled;
/* Indicates if the data requested by the AP was already received*/
extern bool fingersense_data_ready;
/* z-axis data received from the Sensor hub*/
extern s16 fingersense_data[FINGERSENSE_DATA_NSAMPLES];
extern bool rpc_motion_request;
extern void update_fingersense_zaxis_data(s16 *buffer, int nsamples);
extern int fingersense_commu(unsigned int cmd, unsigned int pare,
			     unsigned int responsed);
extern int fingersense_enable(unsigned int enable);
extern int rpc_motion(unsigned int motion);

extern void update_sensor_info(const pkt_header_t *pkt);
extern void disable_sensors_when_suspend(void);
extern void enable_sensors_when_resume(void);
extern void disable_sensors_when_reboot(void);
extern void enable_key_when_recovery_iom3(void);
extern void disable_key_when_reboot(void);
extern void enable_sensors_when_recovery_iom3(void);
extern void reset_calibrate_when_recovery_iom3(void);
extern sys_status_t iom3_sr_status;
extern const char *get_sensor_info_by_tag(int tag);
extern ssize_t show_sensor_read_airpress_common(struct device *dev,
						struct device_attribute *attr,
						char *buf);
extern ssize_t show_airpress_set_calidata_common(struct device *dev,
						 struct device_attribute *attr,
						 char *buf);
extern ssize_t show_mag_calibrate_method(struct device *dev,
					 struct device_attribute *attr,
					 char *buf);
extern ssize_t sensors_calibrate_show(int tag, struct device *dev,
				      struct device_attribute *attr, char *buf);
extern ssize_t sensors_calibrate_store(int tag, struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count);
extern int ois_commu(int tag, unsigned int cmd, unsigned int pare,
		      unsigned int responsed);
extern ssize_t show_cap_prox_calibrate_method(struct device *dev,
				  struct device_attribute *attr, char *buf);
extern ssize_t show_cap_prox_calibrate_orders(struct device *dev,
				  struct device_attribute *attr, char *buf);
#endif /* __SENSORS_H__ */
