/*
 * Copyright (C) huawei company
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.

  * Filename:  sensor_detect.h
 *
 * Discription: some functions of sensorhub power
 *
 * Owner:DIVS_SENSORHUB
 */
#ifndef __SENSOR_DETECT_H
#define __SENSOR_DETECT_H
#include "protocol.h"

typedef enum {
	ACC,
	MAG,
	GYRO,
	ALS,
	PS,
	AIRPRESS,
	HANDPRESS,
	CAP_PROX,
	GPS_4774_I2C,
	FINGERPRINT,
	KEY,
	CHARGER,
	SWITCH,
	MAGN_BRACKET,
        RPC,
	MOTION,
	SENSOR_MAX
}SENSOR_DETECT_LIST;

typedef enum{
	BOOT_DETECT,
	REDETECT_LATER
}DETECT_MODE;

struct sensor_combo_cfg {
	uint8_t bus_type;
	uint8_t bus_num;
	uint8_t disable_sample_thread;
	union {
		uint32_t data;
		uint32_t i2c_address;
		union SPI_CTRL ctrl;
	};
};

#define max_tx_rx_len 32
struct detect_word {
	struct sensor_combo_cfg cfg;
	uint32_t tx_len;
	uint8_t tx[max_tx_rx_len];
	uint32_t rx_len;
	uint8_t rx_msk[max_tx_rx_len];
	uint32_t exp_n;
	uint8_t rx_exp[max_tx_rx_len];
};

#define MAX_SENSOR_NAME_LENGTH 20
struct sensor_detect_manager{
	char sensor_name_str[MAX_SENSOR_NAME_LENGTH];
	uint8_t name_str_length;
	SENSOR_DETECT_LIST sensor_id;
	uint8_t detect_result;
	int tag;
	const void *spara;
    	int cfg_data_length;
};

#define MAX_REDETECT_NUM 100
struct sensor_redetect_state{
	uint8_t need_redetect_sensor;
	uint8_t need_recovery;
	uint8_t detect_fail_num;
	uint8_t redetect_num;
};

int init_sensors_cfg_data_from_dts(DETECT_MODE mode);
SENSOR_DETECT_LIST get_id_by_sensor_tag(int tag);
int sensor_set_cfg_data(void);
void sensor_redetect_enter(void);
void sensor_redetect_init(void);
int sensor_set_fw_load(void);
int motion_set_cfg_data(void);
#endif
