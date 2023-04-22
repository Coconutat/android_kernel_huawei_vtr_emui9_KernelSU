#ifndef __SENSOR_DEBUG_H__
#define __SENSOR_DEBUG_H__


struct led_alwayson_data {
	struct device* dev;
	struct class*  led_alwayson_class;
	struct platform_device *pdev;
};

#define LED_ALWAYSON_NAME    "led_alwayson"


#endif
