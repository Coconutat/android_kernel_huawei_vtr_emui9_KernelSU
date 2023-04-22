#ifndef __LINUX_SENSORHUB_PM_H__
#define __LINUX_SENSORHUB_PM_H__

#define RESUME_INIT 0
#define RESUME_MINI 1
#define RESUME_SKIP 2
#define SENSOR_VBUS "sensor-io"
#define SENSOR_VBUS_LDO12 "lsensor-io"

struct ipc_debug {
	int event_cnt[TAG_END];
	int pack_error_cnt;
};

typedef enum{
	SUB_POWER_ON,
	SUB_POWER_OFF
}sub_power_status;

extern int sensorhub_io_driver_init(void);
extern void set_pm_notifier(void);
extern void enable_sensors_when_resume(void);
extern void disable_sensors_when_suspend(void);
extern int tell_ap_status_to_mcu(int ap_st);
extern void update_current_app_status(uint8_t tag,uint8_t cmd);

#endif //__LINUX_SENSORHUB_PM_H__
