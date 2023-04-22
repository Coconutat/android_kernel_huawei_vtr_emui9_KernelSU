/**********************************************************
 * Filename:  iomcu_power.h
 *
 * Discription: some functions of sensorhub power
 *
 * Owner:DIVS_SENSORHUB
 *
**********************************************************/
#ifndef __IOMCU_POWER_H
#define __IOMCU_POWER_H

typedef struct
{
    uint8_t power_status;
    uint8_t app_status[TAG_END];
    uint32_t idle_time;
    uint64_t active_app_during_suspend;
} iomcu_power_status;

struct power_dbg {
	const char *name;
	const struct attribute_group *attrs_group;
	struct device *dev;
};

typedef enum{
	SUB_POWER_ON,
	SUB_POWER_OFF
}sub_power_status;

enum {
	APP_PEDOMETER,//0
	APP_SIGNIFICANT_MOTION,
	APP_STEP_DETECTOR,
	APP_ACTIVITY,
	APP_ORIENTATION,
	APP_LINEAR_ACCEL,
	APP_GRAVITY,
	APP_ROTATION_VECTOR,
	APP_GAME_ROTATION_VECTOR,
	APP_GEOMAGNETIC_ROTATION_VECTOR,
	APP_MOTION,//10
	APP_ACCEL,
	APP_GYRO,
	APP_MAG,
	APP_ALS,
	APP_PS,//15
	APP_AIRPRESS,
	APP_PDR,
	APP_AR,
	APP_FINGERSENSE,
	APP_PHONECALL,//20
	APP_GSENSOR_GATHER,
	APP_UNCALIBRATE_MAG,
	APP_UNCALIBRATE_GYRO,
	APP_HANDPRESS,
	APP_CA,//25
	APP_OIS,
	APP_FINGERPRINT,
	APP_CAP_PROX,
	APP_KEY,
	APP_AOD,//30
	APP_CHARGING,
	APP_SWITCH,
	APP_MAGN_BRACKET,
	APP_GPS,
	APP_FLP,
	APP_TILT_DETECTOR,
        APP_RPC,
	APP_MAX = 64,
};

void update_current_app_status(uint8_t tag,uint8_t cmd);
void check_current_app(void);
 
#endif
