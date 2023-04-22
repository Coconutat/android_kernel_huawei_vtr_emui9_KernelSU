#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>

/**
* regist maxim power config driver
*/
int irda_maxim_power_config_regist(void);
/**
* unregist maxim power config driver
*/
void irda_maxim_power_config_unregist(void);

/**
* regist irda chip type driver
*/
int irda_chip_type_regist(void);

/**
* unregist irda chip type driver
*/
void irda_chip_type_unregist(void);
