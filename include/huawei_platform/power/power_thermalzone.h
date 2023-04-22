#ifndef _POWER_THERMALZONE_H_
#define _POWER_THERMALZONE_H_

#define POWER_THERMALZONE_SENSORS              (10)
#define POWER_THERMALZONE_STR_MAX_LEN          (16)

#define POWER_THERMALZONE_DEFAULT_TEMP         (25000)
#define POWER_THERMALZONE_MIN_TEMP             (-40000)
#define POWER_THERMALZONE_MAX_TEMP             (125000)

#define PRORATE_OF_INIT	                       (1000) /*in order to resolve divisor less than zero*/

#define POWER_THERMALZONE_DEFAULT_OPS          "hisi_adc" /*get temp data through hisi adc */
#define POWER_THERMALZONE_ADC_RETRY_TIMES      (3)

enum ntc_index {
	NTC_07050125 = 0, /* Thermitor 07050125 */
	NTC_07050124_PULLUP_10K = 1, /* Thermitor 07050124, pullup 10K */
	NTC_07050124_PULLUP_12K = 2, /* Thermitor 07050124, pullup 12K */
};

enum power_thermalzone_sensors_para {
	THERMAL_SENSOR_NAME,
	THERMAL_DEVICE_NAME,
	THERMAL_NTC_INDEX,
	THERMAL_ZONE_PARA_TOTAL,
};

struct power_thermalzone_sensor {
	char sensor_name[POWER_THERMALZONE_STR_MAX_LEN];
	char ops_name[POWER_THERMALZONE_STR_MAX_LEN];
	int adc_channel;
	int ntc_index;
	struct thermal_zone_device *tz_dev;
	int (*get_raw_data)(int adc_channel, long *data);
};

struct power_thermalzone_info {
	struct platform_device *pdev;
	struct device *dev;
	int total_sensor;
	struct power_thermalzone_sensor sensor[POWER_THERMALZONE_SENSORS];
};

struct power_thermalzone_ops {
	int (*get_raw_data)(int adc_channel, long *data);
};

int power_thermalzone_ops_register(struct power_thermalzone_ops* ops, char *ops_name);

#endif /* end of _POWER_THERMALZONE_H_ */
