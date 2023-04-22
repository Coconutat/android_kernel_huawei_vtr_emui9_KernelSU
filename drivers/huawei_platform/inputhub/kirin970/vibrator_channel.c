/*
 *  drivers/misc/inputhub/vibratorhub_channel.c
 *  FHB Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: @
 *  Date:   2016.3.10
 *
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "protocol.h"
#include <linux/cdev.h>
#include<linux/leds.h>
#include <linux/device.h>
#include "sensor_config.h"
#include "sensor_feima.h"
#include "sensor_sysfs.h"
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/switch.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

struct drv2605_data {
	struct led_classdev cclassdev;
	int gpio_enable;
	int gpio_pwm;
	int max_timeout_ms;
	int reduce_timeout_ms;
	int play_effect_time;
	volatile int should_stop;
	//struct timed_output_dev dev;
	struct hrtimer timer;
	struct mutex lock;
	struct work_struct work;
	struct work_struct work_play_eff;
	unsigned char sequence[8];
	struct class* class;
	struct device* device;
	dev_t version;
	struct cdev cdev;
	struct switch_dev sw_dev;
};

static struct drv2605_data *data;
static int vib_calib_result = 0;
static char reg_add = 0;
static char reg_value = 0;
extern struct vibrator_paltform_data vibrator_data;
extern sys_status_t iom3_sr_status;
struct wake_lock vibwlock;

static int vib_time =0;
#if defined(CONFIG_HISI_VIBRATOR)
extern volatile int vibrator_shake;
#else
volatile int vibrator_shake;
#endif
static char rtp_strength = 0x7F;
#define VIB_REG_WRITE 0
#define VIB_REG_READ 1
#define DEVICE_NAME "drv2605"
#define DRIVER_VERSION "130"
#define CDEVIE_NAME  "haptics"
#define VIBRA_NAME "vibrator"
#define MODE_STANDBY        0x40
#define MODE_DEVICE_READY   0x00
#define MAX_HAP_BUF_SIZE 100
#define MIN_HAP_BUF_SIZE  2
#define DRV2605_REG_ADDR 0x5A
#define DRV2605_BUS_NUM 0
#define DRV2605_RW_LEN 1
#define MAX_BUF_LEGTH    16
#define MODE_REG 0x01
#define VIB_CA_DIAG_RST 0x08
#define HAPTIC_STOP 0
#define HAPTICS_NUM 8
#define CONVERT_TO_16 16
#define CONVERT_TO_10 10
#define VIB_TEST_CMD_LEN 20
/*sw_dev state*/
#define SW_STATE_IDLE				0x00
#define SW_STATE_AUDIO2HAPTIC			0x01
#define SW_STATE_SEQUENCE_PLAYBACK		0x02
#define SW_STATE_RTP_PLAYBACK			0x04
#define YES 1
#define NO  0
#define MAX_TIMEOUT 10000	/* 10s */
#define MIN_REDUCE_TIMEOUT 10 /* 10ms */
#define MAX_REDUCE_TIMEOUT 50 /* 50ms */
#define VIB_OFF 0
#define VIB_WAKELOCK_TIME  1100
enum VIB_TEST_TYPE{
	VIB_MMI_CALI = 1,
	VIB_DBC_CALI = 2,
	VIB_HAPTIC_TEST = 3,
};

struct {
    int haptics_type;
    char haptics_value[HAPTICS_NUM];
    int time;
} haptics_table_hub[] = {
    { 1, {0x04,0,0,0,0,0,0,0},75},
    { 2, {0x18,0x06,0x18,0x06,0x18,0,0,0},200},
    { 3, {0x01,0,0,0,0,0,0,0},231},
    { 4, {0x02,0,0,0,0,0,0,0},102},
    { 5, {0x07,0,0,0,0,0,0,0},379},
    { 6, {0x0A,0,0,0,0,0,0,0},212},
    { 7, {0x0E,0x85,0x0E,0x85,0,0,0,0},808},
    { 8, {0x10,0xE4,0,0,0,0,0,0},1222},
    { 9, {0x67,0,0,0,0,0,0,0},289},
    { 10, {0x67,0x85,0x67,0x85,0,0,0,0},600},
    { 11, {0x05,0,0,0,0,0,0,0},45},
    { 12, {0x15,0,0,0,0,0,0,0},55},
    { 13, {0x16,0,0,0,0,0,0,0},53},
    { 14, {0x1B,0,0,0,0,0,0,0},289},
    { 15, {0x1C,0,0,0,0,0,0,0},291},
    { 16, {0x52,0x15,0,0,0,0,0,0},518},
    { 17, {0x53,0x15,0,0,0,0,0,0},605},
    { 18, {0x6A,0x16,0,0,0,0,0,0},501},
    { 19, {0x04,0,0,0,0,0,0,0},74},
    { 20, {0x06,0,0,0,0,0,0,0},113},
    { 21, {0x06,0,0,0,0,0,0,0},113},
    { 22, {0x05,0,0,0,0,0,0,0},45},
    { 23, {0x04,0,0,0,0,0,0,0},74},
    { 31, {0x2E,0,0,0,0,0,0,0},200},
    { 32, {0x2D,0,0,0,0,0,0,0},200},
    { 33, {0x2C,0,0,0,0,0,0,0},200},
};

static int vibrator_enable(struct led_classdev *cdev, int value);
static void vibrator_set_time(int val);

static void vibrator_operate_reg(char reg, char rw_state, char write_regval, char* read_regval)
{
	char bus_num = 0, i2c_address = 0, reg_add = 0, len = 0;
	char rw = 0, buf_temp[DEBUG_DATA_LENGTH] = { 0 };
	int ret = 0;

	if(read_regval == NULL){
		return;
	}
	bus_num = DRV2605_BUS_NUM;
	i2c_address = DRV2605_REG_ADDR;
	reg_add = reg;
	len = DRV2605_RW_LEN;
	if (len > DEBUG_DATA_LENGTH - 1) {
		hwlog_err("len exceed %d\n", len);
		len = DEBUG_DATA_LENGTH - 1;
	}
	rw = rw_state;
	buf_temp[0] = reg_add;
	buf_temp[1] = write_regval;

	hwlog_info("In %s! bus_num = %d, i2c_address = %d, reg_add = %d, len = %d, rw = %d, buf_temp[1] = %d\n",
	     __func__, bus_num, i2c_address, reg_add, len, rw, buf_temp[1]);
	if (rw) {
		ret = mcu_i2c_rw(bus_num, i2c_address, &buf_temp[0], 1, &buf_temp[1], len);
	} else {
		ret = mcu_i2c_rw(bus_num, i2c_address, buf_temp, 2, NULL, 0);
	}
	if (ret < 0)
		hwlog_err("oper %d(1/32:r 0/31:w) i2c reg fail!\n", rw);
	if (rw) {
		hwlog_err("i2c reg %x value %x %x %x %x\n", reg_add, buf_temp[1], buf_temp[2], buf_temp[3], buf_temp[4]);
	}
	*read_regval = buf_temp[1];
	return;
}

static int write_vibrator_calib_value_to_nv(char *temp)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	if (temp == NULL) {
		hwlog_err("write_vibrator_calib_value_to_nv fail, invalid para!\n");
		return -1;
	}
	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = NV_WRITE_TAG;
	user_info.nv_number = VIB_CALIDATA_NV_NUM;
	user_info.valid_size = VIB_CALIDATA_NV_SIZE;
	strncpy(user_info.nv_name, "VIBCAL", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	user_info.nv_data[0] = temp[0];
	user_info.nv_data[1] = temp[1];
	user_info.nv_data[2] = temp[2];
	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		hwlog_err("vibrator nve_direct_access write error(%d)\n", ret);
		return -1;
	}

	hwlog_info("vibrator nve_direct_access write nv_data (0x%x  0x%x  0x%x )\n",
		 user_info.nv_data[0], user_info.nv_data[1],
		 user_info.nv_data[2]);

	return ret;
}

static read_info_t vibrator_send_cali_test_cmd(char* cmd, int len, RET_TYPE *rtype)
{
	int ret = 0;
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	pkt_parameter_req_t spkt;
	pkt_header_t *shd = (pkt_header_t *)&spkt;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	pkg_ap.tag = TAG_VIBRATOR;
	pkg_ap.cmd=CMD_CMN_CONFIG_REQ;
	spkt.subcmd = SUB_CMD_SELFCALI_REQ;
	pkg_ap.wr_buf=&shd[1];
	pkg_ap.wr_len=len+SUBCMD_LEN;
	memcpy(spkt.para, cmd, len);
	//hwlog_err("tag %d calibrator val is %lu  len is %lu.\n", TAG_VIBRATOR, cmd, sizeof(cmd));
	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);
	if (ret) {
		*rtype = COMMU_FAIL;
		hwlog_err("drv2605 calibrate cmd to mcu fail,ret=%d\n", ret);
		return pkg_mcu;
	}
	if (pkg_mcu.errno != 0) {
		hwlog_err("drv2605 calibrate fail, %d\n", pkg_mcu.errno);
		*rtype = EXEC_FAIL;
	} else {
		hwlog_info("drv2605 calibrate return success\n");
		*rtype = SUC;
	}
	return pkg_mcu;
}
static int vibrator_off(void)
{
	vibrator_set_time(VIB_OFF);
	vibrator_shake = 0;
}
static void vibra_set_work(void)
{
	vibrator_set_time(vib_time);
}
static void haptics_play_effect(void)
{
	unsigned char haptics_val[VIB_TEST_CMD_LEN] = {0};
	RET_TYPE vib_return_calibration = RET_INIT;
	read_info_t read_pkg;
	vibrator_shake = 1;
	switch_set_state(&data->sw_dev, SW_STATE_SEQUENCE_PLAYBACK);
	haptics_val[0] = VIB_HAPTIC_TEST;
	memcpy(&haptics_val[1], data->sequence, sizeof(data->sequence));
	memset(&read_pkg, 0, sizeof(read_pkg));
	read_pkg = vibrator_send_cali_test_cmd(haptics_val, sizeof(haptics_val), &vib_return_calibration);
	if ((vib_return_calibration == COMMU_FAIL)||(vib_return_calibration == EXEC_FAIL)) {
		hwlog_err("%s: commu fail\n", __func__);
	} else if (read_pkg.errno == 0) {
		hwlog_info("%s: commu succ\n", __func__);
	}
	if(data->play_effect_time > 0){
		msleep(data->play_effect_time);
	}
	switch_set_state(&data->sw_dev, SW_STATE_IDLE);
	vibrator_shake = 0;
	return;
}

static ssize_t vibrator_dbc_test_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	uint64_t value = 0;
	char test_case = 0;
	RET_TYPE vib_return_calibration = RET_INIT;
	read_info_t read_pkg;
	char test_cmd[VIB_TEST_CMD_LEN] = {0};
	hwlog_info("%s\n", __func__);
	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_err("vibrator dbc test read value error\n");
		return -EINVAL;
	}

	test_case = ((char)value)&0xff;
	if((test_case!=0)&&(test_case!=1)&&(test_case!=2)){
		hwlog_err("vibrator dbc test read value para not defiend error\n");
		return -EINVAL;
	}

	test_cmd[0] = VIB_DBC_CALI;
	test_cmd[1] = test_case;
	hwlog_info("vibrator dbc test read value test cmd= %d, %d\n", test_cmd[0], test_cmd[1]);
	memset(&read_pkg, 0, sizeof(read_pkg));
	read_pkg = vibrator_send_cali_test_cmd(test_cmd, sizeof(test_cmd), &vib_return_calibration);
	if ((vib_return_calibration == COMMU_FAIL)||(vib_return_calibration == EXEC_FAIL)) {
		hwlog_err("%s: commu fail\n", __func__);
	} else if (read_pkg.errno == 0) {
		hwlog_err("%s: commu succ\n", __func__);
	}
	return count;
}

static ssize_t vibrator_calib_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	read_info_t read_pkg;
	char status = 0;
	RET_TYPE vib_return_calibration = RET_INIT;
	char calib_value[VIB_CALIDATA_NV_SIZE] = {0};
	int  i=0, ret=0;
	char test_case[VIB_TEST_CMD_LEN] = {0};

	hwlog_info("%s\n", __func__);
	test_case[0] = VIB_MMI_CALI;
	memset(&read_pkg, 0, sizeof(read_pkg));
	read_pkg = vibrator_send_cali_test_cmd(test_case,sizeof(test_case), &vib_return_calibration);
	if ((vib_return_calibration == COMMU_FAIL)||(vib_return_calibration == EXEC_FAIL)) {
		hwlog_err("%s: cali fail\n", __func__);
		vib_calib_result = 0;
		return count;
	} else if (read_pkg.errno == 0) {
		hwlog_info("%s: commu succ\n", __func__);
	}
	hwlog_info("calib result= 0x%x, 0x%x, 0x%x, 0x%x\n", read_pkg.data[0], read_pkg.data[1], read_pkg.data[2], read_pkg.data[3]);
	status = VIB_CA_DIAG_RST & read_pkg.data[0];
	if(status != 0){
		vib_calib_result = 0;
		hwlog_err("drv2605 vibrator calibration fail!\n");
		return count;
	} else {
		hwlog_info("drv2605 vibrator calibration success!\n");
		vib_calib_result = 1;
	}
	for(i = 0;i<VIB_CALIDATA_NV_SIZE; i++){
		calib_value[i] = read_pkg.data[i+1];
	}
	ret = write_vibrator_calib_value_to_nv(calib_value);
	if (ret) {
		vib_calib_result = 0;
		hwlog_err("vibrator calibration result write to nv fail!\n");
	}
	return count;
}

static ssize_t vibrator_calib_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	hwlog_info("vibrator check vib calibrate result\n");
	int val = vib_calib_result;
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t vibrator_get_reg_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	uint64_t value = 0;
	char reg_address = 0;
	char read_regval = 0, write_regval = 0;
	if(buf == NULL){
		hwlog_err("%s bad parameter error\n", __func__);
		return count;
	}

	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_info("%s read value error\n", __func__);
		goto out;
	}
	reg_address = (char)value;
	vibrator_operate_reg(reg_address, VIB_REG_READ, write_regval,&read_regval);
	reg_value = read_regval;
	hwlog_info("%s, reg_address is 0x%x, reg_value is 0x%x.\n",__func__, reg_address, reg_value);

out:
	return count;
}

static ssize_t vibrator_get_reg_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int val = reg_value;
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t vibrator_set_reg_address_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	uint64_t value = 0;

	if(buf == NULL){
		hwlog_err("%s bad parameter error\n", __func__);
		return count;
	}
	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_info("vibrator_set_reg_address_store read value error\n");
		goto out;
	}
	reg_add = (char)value;
	hwlog_info("%s, reg_addr is 0x%x.\n", __func__, reg_add);

out:
	return count;
}

static ssize_t vibrator_set_reg_value_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	char val = 0;
	uint64_t value = 0;
	char write_regval = 0, read_regval = 0;
	if(buf == NULL){
		hwlog_err("%s bad parameter error\n", __func__);
		return count;
	}
	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_err("vibrator_set_reg_value_store read value error\n");
		return count;
	}
	val = (char)value;
	hwlog_info("%s, reg_add is 0x%x, reg_value is 0x%x.\n", __func__, reg_add, val);
	vibrator_operate_reg(MODE_REG, VIB_REG_WRITE, MODE_DEVICE_READY, &read_regval);
	msleep(1);
	vibrator_operate_reg(reg_add, VIB_REG_WRITE, val, &read_regval);
	msleep(1);
	vibrator_operate_reg(MODE_REG, VIB_REG_WRITE, MODE_STANDBY, &read_regval);

	return count;
}

static ssize_t haptic_test_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	char a[2] = {0};
	char haptic_value[100] = {0};
	uint64_t value = 0;
	char type = 0;
	int i = 0, j = 0;
	int  m , n ;
	int time = 0, table_num = 0;
	char rtp_value = 0;

	if(count < MIN_HAP_BUF_SIZE || count > MAX_HAP_BUF_SIZE || buf == NULL){
		hwlog_info("-----> haptic_test bad value\n");
		return -1;
	}

	//get haptic value, the buf max length is count -2
	for (i = 0, j = 0;i < 100 && i < count-MIN_HAP_BUF_SIZE;) {
		memcpy(&a[0], &buf[i], 2);
		i = i + 2;
		hwlog_info("-----> haptic_test1 is %d %d\n", a[0],a[1]);

		if ((a[0] == 57) && (a[1] == 57)) {
			haptic_value[j] = 0;
			break;
		} else {
			m = ((a[0] > 57)?(a[0]-97+10):(a[0]-48))*16;
			n =  ((a[1] > 57)?(a[1]-97+10):(a[1]-48));
			haptic_value[j] = m + n;
		}
		hwlog_info("-----> haptic_test2 is 0x%x, m = %d, n=%d\n",
						haptic_value[j],m,n);
		j++;
	}

	vibrator_off();
	memcpy(&data->sequence, &haptic_value,8);
	data->play_effect_time = 0;
	haptics_play_effect();
	hwlog_info("%s\n", __func__);
	return count;
}

static ssize_t vibrator_set_rtp_value_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	uint64_t value = 0;

	if(buf == NULL){
		hwlog_err("drv2605 vibrator_set_rtp_value_store error buf\n");
		goto out;
	}

	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_info("vibrator_set_rtp_value_store read value error\n");
		goto out;
	}

	rtp_strength = (char)value;
	hwlog_info("vibrator_set_rtp_value_store is 0x%x\n", rtp_strength);

out:
	return count;
}

static ssize_t vibrator_reg_value_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	char reg_val = 0;
	char write_regval = 0, read_regval = 0;
	char reg_address = 0;

	reg_address =  (char)reg_add;
	vibrator_operate_reg(reg_add, VIB_REG_READ, write_regval, &read_regval);
	hwlog_info("reg_address is 0x%x, reg_value is 0x%x.\n", reg_address, read_regval);
	return snprintf(buf, PAGE_SIZE, "%u\n", (unsigned int)read_regval);
}

static ssize_t vibrator_reg_value_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	char val = 0;
	char read_regval = 0;
	uint64_t value = 0;
	if(buf == NULL){
		hwlog_err("drv2605 vibrator_reg_value_store error buf\n");
		goto out;
	}

	if (strict_strtoull(buf, CONVERT_TO_16, &value)) {
		hwlog_info("vibrator_reg_value_store read value error\n");
		goto out;
	}

	val = (char)value;
	hwlog_info("reg_add is 0x%x, reg_value is 0x%x.\n",reg_add, val);
	vibrator_operate_reg(reg_add, VIB_REG_WRITE, val, &read_regval);

out:
	return (ssize_t)count;
}

static ssize_t set_amplitude_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	char val = 0;
	uint64_t value = 0;
	int ret = 0;
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	pkt_parameter_req_t spkt;
	pkt_header_t *shd = (pkt_header_t *)&spkt;

	if(buf == NULL){
		hwlog_err("drv2605 set_amplitude_store error buf\n");
		goto out;
	}

	if (strict_strtoull(buf, CONVERT_TO_10, &value)) {
		hwlog_err("drv2605 set_amplitude_store read value error\n");
		goto out;
	}

	val = (char)value;
	hwlog_info("drv2605 set_amplitude_store: reg_values = 0x%x.\n",val);

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	pkg_ap.tag = TAG_VIBRATOR;
	pkg_ap.cmd=CMD_CMN_CONFIG_REQ;
	spkt.subcmd = SUB_CMD_VIBRATOR_SET_AMPLITUDE_REQ;
	pkg_ap.wr_buf=&shd[1];
	pkg_ap.wr_len=sizeof(val)+SUBCMD_LEN;
	memcpy(spkt.para, &val, sizeof(val));
	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);
	if (ret) {
		hwlog_err("drv2605 set_amplitude_store cmd to mcu fail,ret=%d\n", ret);
	}
	if (pkg_mcu.errno != 0) {
		hwlog_err("drv2605 set_amplitude_store fail, %d\n", pkg_mcu.errno);
	} else {
		hwlog_info("drv2605 set_amplitude_store  success\n");
	}

out:

	return (ssize_t)count;
}

static ssize_t support_amplitude_control_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", vibrator_data.support_amplitude_control);//not support
}

static DEVICE_ATTR(vibrator_dbc_test, S_IRUSR | S_IWUSR, NULL,
		   vibrator_dbc_test_store);
static DEVICE_ATTR(vibrator_calib, S_IRUSR | S_IWUSR, vibrator_calib_show,
		   vibrator_calib_store);
static DEVICE_ATTR(vibrator_get_reg, S_IRUSR | S_IWUSR, vibrator_get_reg_show,
		   vibrator_get_reg_store);
static DEVICE_ATTR(vibrator_set_reg_address, S_IRUSR | S_IWUSR, NULL,
		   vibrator_set_reg_address_store);
static DEVICE_ATTR(vibrator_set_reg_value, S_IRUSR | S_IWUSR, NULL,
		   vibrator_set_reg_value_store);
static DEVICE_ATTR(haptic_test, S_IRUSR|S_IWUSR, NULL, haptic_test_store);
static DEVICE_ATTR(vibrator_set_rtp_value, S_IRUSR|S_IWUSR, NULL,
		   vibrator_set_rtp_value_store);
static DEVICE_ATTR(vibrator_reg_value, S_IRUSR|S_IWUSR, vibrator_reg_value_show,
		   vibrator_reg_value_store);
static DEVICE_ATTR(set_amplitude, S_IRUSR|S_IWUSR, NULL,
		   set_amplitude_store);
static DEVICE_ATTR(support_amplitude_control, S_IRUSR|S_IWUSR, support_amplitude_control_show,
		   NULL);

static struct attribute *vb_attributes[] = {
	&dev_attr_vibrator_dbc_test.attr,
	&dev_attr_vibrator_calib.attr,
	&dev_attr_vibrator_get_reg.attr,
	&dev_attr_vibrator_set_reg_address.attr,
	&dev_attr_vibrator_set_reg_value.attr,
	&dev_attr_haptic_test.attr,
	&dev_attr_vibrator_set_rtp_value.attr,
	&dev_attr_vibrator_reg_value.attr,
	&dev_attr_set_amplitude.attr,
	&dev_attr_support_amplitude_control.attr,
	NULL
};

static const struct attribute_group vb_attr_group = {
	.attrs = vb_attributes,
};
static void vibrator_set_time(int val){
	int ret = 0;
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	pkt_parameter_req_t spkt;
	pkt_header_t *shd = (pkt_header_t *)&spkt;
	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	pkg_ap.tag = TAG_VIBRATOR;
	pkg_ap.cmd=CMD_CMN_CONFIG_REQ;
	spkt.subcmd = SUB_CMD_VIBRATOR_ON_REQ;
	pkg_ap.wr_buf=&shd[1];
	pkg_ap.wr_len=sizeof(val)+SUBCMD_LEN;
	memcpy(spkt.para, &val, sizeof(val));
	if (iom3_sr_status == ST_SLEEP) {
		hwlog_err("mcu is sleep vibrator_set_time return\n");
		return;
	}
	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);
	if (ret) {
		hwlog_err("send tag %d vibrator_set_time cmd to mcu fail,ret=%d\n", TAG_VIBRATOR, ret);
	}
	if (pkg_mcu.errno != 0) {
		hwlog_err("send tag %d vibrator_set_time fail, %d\n", TAG_VIBRATOR, pkg_mcu.errno);
	}
}
static int vibrator_enable(struct led_classdev *cdev, int value)
{
	int val = value;

	if (val > 0) {
		vibrator_shake = 1;
		if (vibrator_data.reduce_timeout_ms) {
			if ((val > MIN_REDUCE_TIMEOUT) && (val <= MAX_REDUCE_TIMEOUT)) {
				val = vibrator_data.reduce_timeout_ms;
			}
		}
		if (val > vibrator_data.max_timeout_ms) {
			val = vibrator_data.max_timeout_ms;
		}
		vib_time = val;
		//vibrator_set_time(val);
		wake_lock_timeout(&vibwlock,VIB_WAKELOCK_TIME);
		hwlog_err("vibrator_enable, time = %d end\n", val);
	}else{
		//vibrator_set_time(VIB_OFF);
		hwlog_err("vibrator_enable, time = %d end\n", val);
		vib_time = 0;
		vibrator_shake = 0;
	}
	schedule_work(&data->work);
	return 0;
}

static int haptics_open(struct inode * i_node, struct file * filp)
{
	if (data == NULL) {
		return -ENODEV;
	}
	hwlog_info("haptics_open");
	filp->private_data = data;
	return 0;
}

static ssize_t haptics_write(struct file* filp, const char* buff, size_t len, loff_t* off)
{
	int i = 0, type_flag = 0, table_num = 0;
	uint64_t type = 0;
	char write_buf[MAX_BUF_LEGTH] = {0};

	if(len>MAX_BUF_LEGTH || buff == NULL || filp == NULL || off == NULL){
		hwlog_err("[haptics_write] bad value\n");
		return len;
	}

	struct drv2605_data *data = (struct drv2605_data *)filp->private_data;

	mutex_lock(&data->lock);

	if(copy_from_user(write_buf, buff, len)){
		hwlog_err("[haptics_write] copy_from_user failed\n");
		goto out;
	}

	if (strict_strtoull(write_buf, CONVERT_TO_10, &type)) {
		hwlog_err("[haptics_write] read value error\n");
		goto out;
	}

	if (type == HAPTIC_STOP) {
		data->should_stop = YES;
		//hrtimer_cancel(&data->timer);
		//cancel_work_sync(&data->work);
		vibrator_off();
		goto out;
	}

	for (i=0;i<ARRAY_SIZE(haptics_table_hub);i++) {
		if (type == haptics_table_hub[i].haptics_type) {
			table_num = i;
			type_flag = 1;
			hwlog_info("[haptics write] type:%d,table_num:%d.\n", type,table_num);
			break;
		}
	}
	if (!type_flag) {
		hwlog_info("[haptics write] undefined type:%d.\n", type);
		goto out;
	} else {
		data->should_stop = YES;
		vibrator_off();
		memcpy(&data->sequence, &haptics_table_hub[table_num].haptics_value,HAPTICS_NUM);
		hwlog_info("[haptics write] sequence1-4: 0x%x,0x%x,0x%x,0x%x.\n", data->sequence[0],
						data->sequence[1],data->sequence[2],data->sequence[3]);
		hwlog_info("[haptics write] sequence5-8: 0x%x,0x%x,0x%x,0x%x.\n", data->sequence[4],
						data->sequence[5],data->sequence[6],data->sequence[7]);
		data->play_effect_time = haptics_table_hub[table_num].time;
		data->should_stop = NO;
		schedule_work(&data->work_play_eff);
	}
out:
	mutex_unlock(&data->lock);
	return len;
}

static struct file_operations fops =
{
	.open = haptics_open,
	.write = haptics_write,
};
static int haptics_probe(struct drv2605_data *data)
{
	int ret = -ENOMEM;

	data->version = MKDEV(0,0);
	ret = alloc_chrdev_region(&data->version, 0, 1, CDEVIE_NAME);
	if (ret < 0) {
		hwlog_info("drv2605: error getting major number %d\n", ret);
		return ret;
	}

	data->class = class_create(THIS_MODULE, CDEVIE_NAME);
	if (!data->class) {
		hwlog_info("drv2605: error creating class\n");
		goto unregister_cdev_region;
	}

	data->device = device_create(data->class, NULL, data->version, NULL, CDEVIE_NAME);
	if (!data->device) {
		hwlog_info("drv2605: error creating device 2605\n");
		goto destory_class;
	}

	cdev_init(&data->cdev, &fops);
	data->cdev.owner = THIS_MODULE;
	data->cdev.ops = &fops;
	ret = cdev_add(&data->cdev, data->version, 1);
	if (ret) {
		hwlog_info("drv2605: fail to add cdev\n");
		goto destory_device;
	}

	data->sw_dev.name = "haptics";
	ret = switch_dev_register(&data->sw_dev);
	if (ret < 0) {
		hwlog_info("drv2605: fail to register switch\n");
		goto unregister_switch_dev;
	}

	return 0;

unregister_switch_dev:
		switch_dev_unregister(&data->sw_dev);
destory_device:
	device_destroy(data->class, data->version);
destory_class:
	class_destroy(data->class);
unregister_cdev_region:
	unregister_chrdev_region(data->version, 1);

	return ret;
}

static int support_vibratorhub(void)
{
	struct device_node *dn = NULL;
	char *sensor_ty = NULL;
	char *sensor_st = NULL;
	const char *st = "ok";
	int ret = -1;
	for_each_node_with_property(dn, "sensor_type") {
		//sensor type
		ret = of_property_read_string(dn, "sensor_type", (const char **)&sensor_ty);
		if (ret) {
			hwlog_err("get sensor type fail ret=%d\n", ret);
			continue;
		}
		ret = strncmp(sensor_ty, VIBRA_NAME, sizeof(VIBRA_NAME));
		if(!ret){
			ret = of_property_read_string(dn, "status", (const char **)&sensor_st);
			if (ret) {
				hwlog_err("get sensor status fail ret=%d\n", ret);
				return -1;
			}
			ret = strcmp(st,sensor_st);
			if( !ret){
				hwlog_info("%s : vibrator status is %s \n",__func__,sensor_st);
				return 0;
			}
		}
	}
	return -1;
}

/*******************************************************************************************
Function:       vibratorhub_init
Description:    apply kernel buffer, register vibratorhub timeout_dev 
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         result of function, 0 successed, else false
*******************************************************************************************/
static int __init vibratorhub_init(void)
{
	int ret, rc;
	char vb_name[50]= "vibrator";
	if (is_sensorhub_disabled()){
		return -1;
	}
	if(support_vibratorhub()){
		return - 1;
	}
  	data = kzalloc(sizeof(struct drv2605_data), GFP_KERNEL);
	if (!data) {
		hwlog_err("unable to allocate memory\n");
		return -ENOMEM;
	}
	mutex_init(&data->lock);
	data->cclassdev.name = vb_name;
	data->cclassdev.flags = LED_CORE_SUSPENDRESUME;
	data->cclassdev.brightness_set = vibrator_enable;
	data->cclassdev.default_trigger = "transient";
	rc = led_classdev_register(NULL, &data->cclassdev);
	if (rc) {
		hwlog_err("%s, unable to register with timed_output\n", __func__);
		kfree(data);
		return -ENOMEM;
	}
	ret = sysfs_create_group(&data->cclassdev.dev->kobj, &vb_attr_group);
	if (ret) {
		hwlog_err("%s, unable create vibrator's sysfs,DBC check IC fail\n", __func__);
		led_classdev_unregister(&data->cclassdev);
		kfree(data);
		return -ENOMEM;
	}

	wake_lock_init(&vibwlock, WAKE_LOCK_SUSPEND, "vib_sensorhub");
	INIT_WORK(&data->work_play_eff, haptics_play_effect);
	INIT_WORK(&data->work, vibra_set_work);

	haptics_probe(data);
	hwlog_info("%s success!", __func__);
	return rc;

}

/*******************************************************************************************
Function:       fingerprinthub_exit
Description:    release kernel buffer, deregister fingerprinthub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         void
*******************************************************************************************/
static void __exit vibratorhub_exit(void)
{
	mutex_destroy(&data->lock);
	sysfs_remove_group(&data->cclassdev.dev->kobj, &vb_attr_group);
	led_classdev_unregister(&data->cclassdev);
	cancel_work_sync(&data->work_play_eff);
	cancel_work_sync(&data->work);
	wake_lock_destroy(&vibwlock);
	hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(vibratorhub_init);
module_exit(vibratorhub_exit);

MODULE_AUTHOR("VBHub <hujianglei@huawei.com>");
MODULE_DESCRIPTION("VBHub driver");
MODULE_LICENSE("GPL");
