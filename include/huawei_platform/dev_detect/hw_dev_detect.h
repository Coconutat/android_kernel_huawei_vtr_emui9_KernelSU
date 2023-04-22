#ifndef _HUAWEI_DEV_DETECT_H_
#define _HUAWEI_DEV_DETECT_H_

/**
 * Note: The enums in these two files are same.
 * ${ANDROID_ROOT}/vendor/huawei/extra/kernel/include/dev_detect/
 * hw_dev_detect.h
 *
 * ${ANDROID_ROOT}/vendor/huawei/chipset_common/modules/include/dev_detect/
 * hw_dev_detect.h
 *
 * If you modify one of them, please modify the other one to make sure they
 * are the same.
 */

/* hw device list */
enum hw_device_type {
	DEV_DETECT_TOUCH_PANEL = 0, /* id : 0 order number: 1 */
	DEV_DETECT_COMPASS,         /* 1 */
	DEV_DETECT_G_SENSOR,        /* 2 */
	DEV_DETECT_CAMERA_MAIN,     /* 3 */
	DEV_DETECT_CAMERA_SLAVE,    /* 4 */
	DEV_DETECT_KEYPAD,          /* 5 */
	DEV_DETECT_APS,             /* 6 */
	DEV_DETECT_GYROSCOPE,       /* 7 */
	DEV_DETECT_NFC,             /* 8 */
	DEV_DETECT_DC_DC,           /* 9 */
	DEV_DETECT_SPEAKER,         /* 10 */
	DEV_DETECT_OFN,             /* 11 */
	DEV_DETECT_TPS,             /* 12 */
	DEV_DETECT_L_SENSOR,        /* 13 */
	DEV_DETECT_CHARGER,         /* 14 */
	DEV_DETECT_BATTERY,         /* 15 */
	DEV_DETECT_NCT,             /* 16 */
	DEV_DETECT_MHL,             /* 17 */
	DEV_DETECT_AUDIENCE,        /* 18 */
	DEV_DETECT_IRDA,            /* 19 */
	DEV_DETECT_CS,              /* 20 */
	DEV_DETECT_USB_SWITCH,      /* 21 */
	DEV_DETECT_PMU_DCDC,        /* 22 */
	DEV_DETECT_FPGA,            /* 23 */
	DEV_DETECT_CPU_CHIP,        /* 24 */
	DEV_DETECT_AIRPRESS,        /* 25 */
	DEV_DETECT_HANDPRESS,       /* 26 */
	DEV_DETECT_FFLASH,          /* 27 */
	DEV_DETECT_VIBRATOR_LRA,    /* 28 */
	DEV_DETECT_TYPEC,           /* 29 */
	DEV_DETECT_ANC_MAX14744,    /* 30 */
	DEV_DETECT_LASER,           /* 31 */
	DEV_DETECT_CAMERA_PMIC,     /* 32 */
	DEV_DETECT_LOADSWITCH,      /* 33 */
	DEV_DETECT_BUCKBOOST_MAX77813, /* 34 */
	DEV_DETECT_COUL,            /* 35 */
	DEV_DETECT_WIFI,            /* 36 */
	DEV_DETECT_BT,              /* 37 */
	DEV_DETECT_FM,              /* 38 */
	DEV_DETECT_GPS,             /* 39 */
	DEV_DETECT_GPU,             /* 40 */
	DEV_DETECT_ANTENNABOARD,    /* 41 */
	DEV_DETECT_BATTERY_STATE,   /* 42 */
	DEV_DETECT_PMIC,            /* 43 */
};

struct detect_device {
	const size_t device_id;
	const char *device_name;
};

static const struct detect_device hw_detect_device_array[] = {
	{ DEV_DETECT_TOUCH_PANEL, "touch_panel" }, /* id : 0 order number 1 */
	{ DEV_DETECT_COMPASS, "compass" },         /* id : 1 order number 2 */
	{ DEV_DETECT_G_SENSOR, "g_sensor" },       /* id : 2 order number 3 */
	{ DEV_DETECT_CAMERA_MAIN, "camera_main" }, /* id : 3 order number 4 */
	{ DEV_DETECT_CAMERA_SLAVE, "camera_slave" },/* id : 4 order number 5 */
	{ DEV_DETECT_KEYPAD, "keypad" },           /* id : 5 order number 6 */
	{ DEV_DETECT_APS, "aps" },                 /* id : 6 order number 7 */
	{ DEV_DETECT_GYROSCOPE, "gyroscope" },     /* id : 7 order number 8 */
	{ DEV_DETECT_NFC, "nfc" },                 /* id : 8 order number 9*/
	{ DEV_DETECT_DC_DC, "dc_dc" },             /* id : 9 order number 10 */
	{ DEV_DETECT_SPEAKER, "speaker" },         /* id : 10 order number 11 */
	{ DEV_DETECT_OFN, "ofn" },                 /* id : 11 order number 12 */
	{ DEV_DETECT_TPS, "tps" },                 /* id : 12 order number 13 */
	{ DEV_DETECT_L_SENSOR, "l_sensor" },       /* id : 13 order number 14 */
	{ DEV_DETECT_CHARGER, "charge" },          /* id : 14 order number 15 */
	{ DEV_DETECT_BATTERY, "battery" },         /* id : 15 order number 16 */
	{ DEV_DETECT_NCT, "nct" },                 /* id : 16 order number 17 */
	{ DEV_DETECT_MHL, "mhl" },                 /* id : 17 order number 18 */
	{ DEV_DETECT_AUDIENCE, "audience" },       /* id : 18 order number 19 */
	{ DEV_DETECT_IRDA, "irda" },               /* id : 19 order number 20 */
	{ DEV_DETECT_CS, "hand_sensor"},           /* id : 20 order number 21 */
	{ DEV_DETECT_USB_SWITCH, "usb_switch"},    /* id : 21 order number 22 */
	{ DEV_DETECT_PMU_DCDC, "pmu_dcdc"},        /* id : 22 order number 23 */
	{ DEV_DETECT_FPGA, "antenna_ctl"},         /* id : 23 order number 24 */
	{ DEV_DETECT_CPU_CHIP, "cpu_chip"},        /* id : 24 order number 25 */
	{ DEV_DETECT_AIRPRESS, "airpress" },       /* id : 25 order number 26 */
	{ DEV_DETECT_HANDPRESS, "handpress" },     /* id : 26 order number 27 */
	{ DEV_DETECT_FFLASH, "fflash"},            /* id : 27 order number 28 */
	{ DEV_DETECT_VIBRATOR_LRA, "vibrator_lra"},/* id : 28 order number 29 */
	{ DEV_DETECT_TYPEC, "typec"},              /* id : 29 order number 30 */
	/* add for anc hs device check */
	{ DEV_DETECT_ANC_MAX14744, "anc_max14744"},/* id : 30 order number 31 */
	{ DEV_DETECT_LASER, "laser"},              /* id : 31 order number 32 */
	{ DEV_DETECT_CAMERA_PMIC, "camera_pmic"},  /* id : 32 order number 33 */
	{ DEV_DETECT_LOADSWITCH, "charge_loadswitch"},/*id: 33 order number 34*/
	{ DEV_DETECT_BUCKBOOST_MAX77813, "max77813"}, /*id: 34 order number 35*/
	{ DEV_DETECT_COUL, "coul"},                   /*id: 35 order number 36*/
	{ DEV_DETECT_WIFI, "wifi" },           /* id : 36 order number 37 */
	{ DEV_DETECT_BT, "bt" },               /* id : 37 order number 38 */
	{ DEV_DETECT_FM, "fm" },               /* id : 38 order number 39 */
	{ DEV_DETECT_GPS, "gps" },             /* id : 39 order number 40 */
	{ DEV_DETECT_GPU, "gpu" },             /* id : 40 order number 41 */
	{ DEV_DETECT_ANTENNABOARD, "antennaboard" }, /*id : 41 order number 42*/
	{ DEV_DETECT_BATTERY_STATE, "battery_state"},/*id : 42 order number 43*/
	{ DEV_DETECT_PMIC, "pmic" },          /* id : 43 order number 44 */
};

/**
 * It is used in the device's probe function, if device's probe is completed,
 * set true flag corresponding to dev_id.
 */
int set_hw_dev_detect_result(const size_t dev_id);

#endif
