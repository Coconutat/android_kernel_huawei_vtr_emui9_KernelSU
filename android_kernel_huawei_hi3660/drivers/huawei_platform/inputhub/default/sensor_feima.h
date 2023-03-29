#ifndef __SENSOR_FEIMA_H__
#define __SENSOR_FEIMA_H__

#define ALS_DBG_PARA_SIZE (8)
#define BUF_SIZE (128)
#define ALS_MCU_HAL_CONVER (10)
#define ACC_CONVERT_COFF 1000

struct sensor_cookie {
	int tag;
	const char *name;
	const struct attribute_group *attrs_group;
	struct device *dev;
};
typedef struct {
    uint16_t sub_cmd;
    uint16_t sar_info;
} rpc_ioctl_t;
#endif //__SENSOR_FEIMA_H__
