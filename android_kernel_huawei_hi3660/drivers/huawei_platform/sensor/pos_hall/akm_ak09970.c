/*
 * akm_ak09970.c
 *
 * step hall driver
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <huawei_platform/log/hw_log.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG AKM_AK09970
HWLOG_REGIST();

#define AK09970_SYSCLS_NAME "stepmotor_hall"
#define AK09970_SYSDEV_NAME "ak09970_dev"
#define AK09970_FST_CHECK_VAR_UPPER_LIMIT
#define AK09970_RESET_DELAY_MS 1
#define AK09970_DRDY_DELAY_MS 2
#define AK09970_DRDY_PIN_CONFIG 0x0001
#define AK09970_SW_FUNC_CONFIG 0x0000
#define AK09970_ERRXY_CONFIG 0x0080
#define AK09970_ERRADC_CONFIG 0x0100
#define AK09970_INT_CONFIG 0x0000
#define AK09970_ODINT_CONFIG 0x0000
/*
 * Expected standard deviation of AK09970 data in uT unit: 0uTrms ~ 50uTrms,
 * including the environment mag disturbance.
 * Variance of AK09970 data in uT unit : 0(uTrms^2) ~ 2500(uTrms^2)
 * Varianc raw register value (0 / res^2) ~ (2500 /res^2)
 */
#ifdef AK09970_WIDE_RANGE
	#define AK09970_FST_VAR_LIMIT_HIGH 260 // (50^2)/(3.1^2)
	#define AK09970_FST_VAR_LIMIT_LOW 0
#else
	#define AK09970_FST_VAR_LIMIT_HIGH 2066 // (50^2)/(1.1^2)
	#define AK09970_FST_VAR_LIMIT_LOW 0
#endif

#define AK09970_WIA_VALUE 0xC048
#define AK09970_WIA_SIZE 2
#define AK09970_ST1_SIZE 2
#define AK09970_BDATA_SIZE 8
#define AK09970_SOFT_RESET_VALUE 0x01
#define AK09970_ST1_IS_DRDY_VALUE 0xFC01
#define AK09970_ST1_NO_DRDY_VALUE 0xFC00
#define AK09970_FST_DATA_BUF_SIZE 100
#define AK09970_FST_DRDY_RETRY_MAX 10
#define AKM_BASE_NUM 10
/*
 * AK09970_REG_SDR_VALUE (spec 12.3.6)
 * 0x00 : Low noise drive
 * 0x01 : Low power drive
 */
#define AK09970_REG_SDR_VALUE 0x00
/*
 * AK09970_REG_SMR_VALUE (spec 12.3.6)
 * 0x00 : High sensitivity
 * 0x20 : Wide measurement range
 */
#ifdef AK09970_WIDE_RANGE
	#define AK09970_REG_SMR_VALUE 0x20
	#define AK09970_SCALE_AMPLIFY_Q16 203161 // 3.1uT/LSB in Q16
	#define AK09970_SCALE_DATA_RANGE 10570
#else
	#define AK09970_REG_SMR_VALUE 0x00
	#define AK09970_SCALE_AMPLIFY_Q16 72090 // 1.1uT/LSB in Q16
	#define AK09970_SCALE_DATA_RANGE 29788
#endif
#define AK09970_REG_WIA_ADDR 0x00
#define AK09970_REG_CNTL1_ADDR 0x20
#define AK09970_REG_CNTL2_ADDR 0x21
#define AK09970_REG_SRST_ADDR 0x30
#define AK09970_REG_ST1_ADDR 0x10
#define AK09970_REG_ALL_AXES_ADDR 0x17

#define AK09970_MODE_POWERDOWN_MODE 0x00
#define AK09970_MODE_SNG_MEASURE_MODE 0x01
#define AK09970_MODE_CNT_MEASURE_MODE1 0x02 // 0.25Hz
#define AK09970_MODE_CNT_MEASURE_MODE2 0x04 // 0.5Hz
#define AK09970_MODE_CNT_MEASURE_MODE3 0x06 // 1Hz
#define AK09970_MODE_CNT_MEASURE_MODE4 0x08 // 10Hz
#define AK09970_MODE_CNT_MEASURE_MODE5 0x0A // 20Hz
#define AK09970_MODE_CNT_MEASURE_MODE6 0x0C // 50Hz
#define AK09970_MODE_CNT_MEASURE_MODE7 0x0E // 100Hz

#define AK09970_ODR_10HZ 10
#define AK09970_ODR_20HZ 20
#define AK09970_ODR_50HZ 50
#define AK09970_ODR_100HZ 100
#define AK09970_DELAY_025HZ 4000000000LL
#define AK09970_DELAY_05HZ 2000000000LL
#define AK09970_DELAY_1HZ 1000000000LL
#define AK09970_DELAY_10HZ 100000000LL
#define AK09970_DELAY_20HZ 50000000LL
#define AK09970_DELAY_50HZ 20000000LL
#define AK09970_DELAY_100HZ 10000000LL
#define AK09970_MIN_DELAY_NS AK09970_DELAY_100HZ // 10ms, 100Hz
#define AK09970_MAX_DELAY_NS AK09970_DELAY_025HZ // 4s, 0.25Hz
#define AK09970_DEFAULT_DELAY_NS AK09970_DELAY_100HZ // 10ms

#define POS_HALL_NV_NUM 415
#define POS_HALL_NV_SIZE 24
#define POS_HALL_NV_NAME "MOTORPOS"
#define NV_READ_TAG 1
#define NV_WRITE_TAG 0
#define NV_WRITE_TIME_COST_MS 300

#define AXIS_COUNT 3
#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define AK09970_RETCODE_SUCCESS 0
#define AK09970_RETCODE_COMMON_ERR (-1)
#define AK09970_RETCODE_FST_FAIL (-2)
#define AK09970_RETCODE_I2C_OP_FAIL (-3)
#define AK09970_RETCODE_RESET_FAIL (-4)
#define AK09970_RETCODE_ID_CHECK_FAIL (-5)
#define AK09970_RETCODE_FST_SET_MODE_FAIL (-6)
#define AK09970_RETCODE_FST_IS_DRDY_FAIL (-7)
#define AK09970_RETCODE_FST_NO_DRDY_FAILED (-8)
#define AK09970_RETCODE_FST_VAR_L_FAIL (-9)
#define AK09970_RETCODE_FST_VAR_H_FAILED (-10)
#define AK09970_RETCODE_NULL_POINT_ERR (-11)

#define AK09970_CNTL1_CONFIG (AK09970_DRDY_PIN_CONFIG | \
	AK09970_SW_FUNC_CONFIG | AK09970_ERRXY_CONFIG | \
	AK09970_ERRADC_CONFIG | AK09970_INT_CONFIG | AK09970_ODINT_CONFIG)
#define ak09970_adc_overflow(st1) ((0x0100 & (st1)) >> 8)
#define ak09970_set_sdr(ctrl2) (AK09970_REG_SDR_VALUE | (ctrl2))
#define ak09970_set_smr(ctrl2) (AK09970_REG_SMR_VALUE | (ctrl2))
#define make_u16(u8h, u8l) \
	((uint16_t)(((uint16_t)(u8h) << 8) | (uint16_t)(u8l)))
#define make_s16(u8h, u8l) \
	((int16_t)(((uint16_t)(u8h) << 8) | (uint16_t)(u8l)))

#define AK09970_FST_STEP_1_1 0x0101
#define AK09970_FST_STEP_1_2 0x0102
#define AK09970_FST_STEP_1_3 0x0103
#define AK09970_FST_STEP_1_4 0x0104
#define AK09970_FST_STEP_1_5 0x0105
#define AK09970_FST_STEP_2_1 0x0201
#define AK09970_FST_STEP_2_2 0x0202
#define AK09970_FST_STEP_2_3 0x0203
#define AK09970_FST_STEP_2_4 0x0204
#define AK09970_FST_STEP_2_5 0x0205
#define AK09970_FST_STEP_2_6 0x0206
#define AK09970_FST_STEP_2_7 0x0207
#define AK09970_FST_STEP_2_8 0x0208
#define AK09970_FST_STEP_2_9 0x0209
#define AK09970_FST_STEP_2_A 0x020A

struct type_nv_data {
	int close_x;
	int close_y;
	int close_z;
	int open_x;
	int open_y;
	int open_z;
};

struct type_device_data {
	struct i2c_client *i2c;
	struct class *dev_class;
	struct device *class_dev;
	struct mutex op_mutex;
	struct mutex sensor_mutex;
	struct mutex val_mutex;
	struct mutex nv_mutex;
	uint32_t odr; // Hz
	bool enable_flag;
	int64_t ns_delay; // ns
	bool data_refresh_flag;
	bool nvwrite_success_flag;
	bool nvwrite_finish_flag;
	struct type_nv_data written_calibrate_data;
	int32_t sensor_data[AXIS_COUNT]; // 0,1,2 refer to x,y,z
	struct hrtimer poll_timer;
	struct workqueue_struct *work_queue;
	struct workqueue_struct *nvwrite_queue;
	struct delayed_work dwork;
	struct delayed_work nvwrite_work;
	wait_queue_head_t read_waitq;
	wait_queue_head_t nvwrite_waitq;
};

struct type_mag_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

#ifdef CONFIG_HUAWEI_DSM
static struct dsm_dev dsm_vibrator = {
	.name = "dsm_vibrator",
	.device_name = "step_motor",
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};
struct dsm_client *vibrator_dclient = NULL;
#endif

static struct type_device_data *ak09970_dev_data = NULL;
static int ak09970_gpio_rst;

void dmd_log_report(int dmd_mark, const char *err_func, const char *err_msg)
{
#ifdef CONFIG_HUAWEI_DSM
	if (!dsm_client_ocuppy(vibrator_dclient)) {
		dsm_client_record(vibrator_dclient, "%s, %s", err_func, err_msg);
		dsm_client_notify(vibrator_dclient, dmd_mark);
	}
#endif
}

static int read_calibrate_data_from_nv(int nv_number,
	int nv_size, const char *nv_name, struct type_nv_data *nv_info)
{
	struct hisi_nve_info_user nv_user_info;
	int ret;

	if ((nv_name == NULL) || (nv_info == NULL)) {
		hwlog_err("%s: read_calibrate_data_from_nv INPUT PARA NULL\n",
			__func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	hwlog_info("%s: read_calibrate_data_from_nv %d in!!\n",
		__func__, nv_number);
	memset(&nv_user_info, 0, sizeof(nv_user_info));
	nv_user_info.nv_operation = NV_READ_TAG;
	nv_user_info.nv_number = nv_number;
	nv_user_info.valid_size = nv_size;
	strncpy(nv_user_info.nv_name, nv_name,
		sizeof(nv_user_info.nv_name) - 1);
	nv_user_info.nv_name[sizeof(nv_user_info.nv_name) - 1] = '\0';
	ret = hisi_nve_direct_access(&nv_user_info);
	if (ret != 0) {
		hwlog_err("%s: hisi_nve_direct_access read nv %d error = %d\n",
			__func__, nv_number, ret);
		return AK09970_RETCODE_COMMON_ERR;
	}
	memcpy(nv_info, nv_user_info.nv_data,
		(sizeof(nv_user_info.nv_data) < nv_user_info.valid_size) ?
		sizeof(nv_user_info.nv_data) : nv_user_info.valid_size);
	return ret;
}

static int write_calibrate_data_to_nv(int nv_number,
	int nv_size, const char *nv_name, const struct type_nv_data *nv_info)
{
	struct hisi_nve_info_user nv_user_info;
	int ret;

	if ((nv_name == NULL) || (nv_info == NULL)) {
		hwlog_err("%s: write_calibrate_data_to_nv INPUT PARA NULL\n",
			__func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	hwlog_info("%s: write_calibrate_data_to_nv %d in!!\n",
		__func__, nv_number);
	memset(&nv_user_info, 0, sizeof(nv_user_info));
	nv_user_info.nv_operation = NV_WRITE_TAG;
	nv_user_info.nv_number = nv_number;
	nv_user_info.valid_size = nv_size;
	strncpy(nv_user_info.nv_name, nv_name,
		sizeof(nv_user_info.nv_name) - 1);
	nv_user_info.nv_name[sizeof(nv_user_info.nv_name) - 1] = '\0';
	/* copy to nv by pass */
	memcpy(nv_user_info.nv_data, nv_info,
		(sizeof(nv_user_info.nv_data) < nv_user_info.valid_size) ?
		sizeof(nv_user_info.nv_data) : nv_user_info.valid_size);
	ret = hisi_nve_direct_access(&nv_user_info);
	if (ret != 0) {
		hwlog_err("%s: hisi_nve_direct_access read nv %d error = %d\n",
			__func__, nv_number, ret);
		return AK09970_RETCODE_COMMON_ERR;
	}
	return AK09970_RETCODE_SUCCESS;
}

static void nvwrite_process(struct work_struct *work)
{
	struct type_device_data *akm_ak09970 = NULL;
	int ret;

	akm_ak09970 = container_of((struct delayed_work *)work,
		struct type_device_data, nvwrite_work);

	mutex_lock(&akm_ak09970->nv_mutex);
	ret = write_calibrate_data_to_nv(POS_HALL_NV_NUM, POS_HALL_NV_SIZE,
		POS_HALL_NV_NAME, &akm_ak09970->written_calibrate_data);
	if (ret == AK09970_RETCODE_SUCCESS)
		akm_ak09970->nvwrite_success_flag = true;
	akm_ak09970->nvwrite_finish_flag = true;
	if (waitqueue_active(&akm_ak09970->nvwrite_waitq))
		wake_up_interruptible(&akm_ak09970->nvwrite_waitq);
	mutex_unlock(&akm_ak09970->nv_mutex);
}

static int ak09970_hard_reset(int gpio_rst)
{
	int ret;

	ret = gpio_direction_output(gpio_rst, 1); // set high
	if (ret != 0) {
		hwlog_err("%s: reset pre pull up failed. ret = %d\n",
			__func__, ret);
		goto reset_failed;
	}
	mdelay(AK09970_RESET_DELAY_MS); // Keep reset pin at high over 5us

	ret = gpio_direction_output(gpio_rst, 0); // set low
	if (ret != 0) {
		hwlog_err("%s: reset pull down failed. ret = %d\n",
			__func__, ret);
		goto reset_failed;
	}
	mdelay(AK09970_RESET_DELAY_MS); // Keep reset pin at low over 5us

	ret = gpio_direction_output(gpio_rst, 1); // set high again
	if (ret != 0) {
		hwlog_err("%s: reset end pull up failed. ret = %d\n",
			__func__, ret);
		goto reset_failed;
	}
	mdelay(AK09970_RESET_DELAY_MS);

	hwlog_info("%s: success\n", __func__);
	return AK09970_RETCODE_SUCCESS;

reset_failed:
	hwlog_info("%s: failed\n", __func__);
	return AK09970_RETCODE_COMMON_ERR;
}

static int ak09970_soft_reset(const struct type_device_data *akm_ak09970)
{
	uint8_t i2c_data;
	struct i2c_client *client = NULL;
	int ret;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	client = akm_ak09970->i2c;
	if (client == NULL) {
		hwlog_err("%s: client is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	hwlog_info("%s: ak09970 soft reset\n", __func__);

	i2c_data = AK09970_SOFT_RESET_VALUE;
	ret = i2c_smbus_write_i2c_block_data(client,
		AK09970_REG_SRST_ADDR, 1, &i2c_data);
	if (ret < 0) {
		hwlog_info("%s: i2c write fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c write fail\n");
		return ret;
	}

	return AK09970_RETCODE_SUCCESS;
}

/* select mode by delay */
static uint8_t ak09970_select_mode(int64_t delay)
{
	uint8_t mode;

	if (delay >= AK09970_DELAY_025HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE1;
	else if (delay >= AK09970_DELAY_05HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE2;
	else if (delay >= AK09970_DELAY_1HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE3;
	else if (delay >= AK09970_DELAY_10HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE4;
	else if (delay >= AK09970_DELAY_20HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE5;
	else if (delay >= AK09970_DELAY_50HZ)
		mode = AK09970_MODE_CNT_MEASURE_MODE6;
	else
		mode = AK09970_MODE_CNT_MEASURE_MODE7;

	return mode;
}

static int16_t ak09970_set_mode(const struct type_device_data *akm_ak09970,
	const uint8_t mode)
{
	uint8_t i2c_data;
	uint16_t ctrl1_data = AK09970_CNTL1_CONFIG;
	struct i2c_client *client = NULL;
	int ret;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	hwlog_info("%s: mode = 0x%x\n", __func__, mode);

	client = akm_ak09970->i2c;
	if (client == NULL) {
		hwlog_err("%s: client is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	ret = i2c_smbus_write_i2c_block_data(client,
		AK09970_REG_CNTL1_ADDR, 2, (uint8_t *)&ctrl1_data);
	if (ret < 0) {
		hwlog_info("%s: i2c ctrl1 write fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c write fail\n");
		return ret;
	}

	i2c_data = mode;
	if (mode != AK09970_MODE_POWERDOWN_MODE) {
		i2c_data = ak09970_set_sdr(i2c_data);
		i2c_data = ak09970_set_smr(i2c_data);
	}

	ret = i2c_smbus_write_i2c_block_data(client, AK09970_REG_CNTL2_ADDR,
		1, &i2c_data);
	if (ret < 0) {
		hwlog_info("%s: i2c cntrl2 write fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c write fail\n");
		return ret;
	}

	return AK09970_RETCODE_SUCCESS;
}

static int64_t ak09970_get_poll_delay(struct type_device_data *akm_ak09970)
{
	int64_t delay;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	delay = akm_ak09970->ns_delay;
	if (delay < AK09970_MIN_DELAY_NS)
		delay = AK09970_MIN_DELAY_NS;
	else if (delay > AK09970_MAX_DELAY_NS)
		delay = AK09970_MAX_DELAY_NS;

	return delay;
}

static void ak09970_enable_set(struct type_device_data *akm_ak09970)
{
	bool enable = false;
	uint8_t mode;
	int64_t delay;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return;
	}

	enable = akm_ak09970->enable_flag;
	delay = ak09970_get_poll_delay(akm_ak09970);
	mutex_lock(&akm_ak09970->op_mutex);
	if (enable) {
		mutex_lock(&akm_ak09970->sensor_mutex);
		akm_ak09970->data_refresh_flag = false;
		mutex_unlock(&akm_ak09970->sensor_mutex);

		mode = ak09970_select_mode(delay);
		ak09970_set_mode(akm_ak09970, mode);
		hrtimer_start(&akm_ak09970->poll_timer,
				ns_to_ktime(delay),
				HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&akm_ak09970->poll_timer);
		cancel_work_sync(&akm_ak09970->dwork.work);

		ak09970_set_mode(akm_ak09970, AK09970_MODE_POWERDOWN_MODE);

		mutex_lock(&akm_ak09970->sensor_mutex);
		akm_ak09970->data_refresh_flag = false;
		mutex_unlock(&akm_ak09970->sensor_mutex);
	}
	mutex_unlock(&akm_ak09970->op_mutex);
}

static void ak09970_poll_delay_set(struct type_device_data *akm_ak09970)
{
	bool enable = false;
	uint32_t freq;
	int64_t delay;
	int16_t delay_update = 0;
	uint8_t mode;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return;
	}

	freq = akm_ak09970->odr;

	// 1000000000LL is 1s
	if (freq == 0)
		freq = AK09970_ODR_100HZ;
	delay = 1000000000LL / freq;

	if (delay < AK09970_MIN_DELAY_NS)
		delay = AK09970_MIN_DELAY_NS;
	else if (delay > AK09970_MAX_DELAY_NS)
		delay = AK09970_MAX_DELAY_NS;

	mutex_lock(&akm_ak09970->val_mutex);
	if (akm_ak09970->ns_delay != delay) {
		akm_ak09970->ns_delay = delay;
		delay_update = 1;
	}
	mutex_unlock(&akm_ak09970->val_mutex);
	enable = akm_ak09970->enable_flag;
	if ((delay_update == 1) && (enable)) {
		ak09970_set_mode(akm_ak09970, AK09970_MODE_POWERDOWN_MODE);
		mode = ak09970_select_mode(delay);
		ak09970_set_mode(akm_ak09970, mode);
	}
}

/*
 * ak09970_fst_check_var
 * Input:  Int array with AK09970_FST_DATA_BUF_SIZE length
 * Output: Variance of buf
 */
static int64_t ak09970_fst_check_var(const int16_t buf[], int size)
{
	int i;
	int64_t sum = 0;
	int64_t var_sum = 0;
	int64_t ave;
	int64_t var;

	if (size == 0) {
		hwlog_err("%s: size is 0 during checking variance\n",
			__func__);
		return 0;
	}

	for (i = 0; i < size; i++)
		sum = sum + buf[i];

	ave = (int64_t)(sum / size);

	for (i = 0; i < size; i++)
		var_sum = var_sum + (buf[i] - ave) * (buf[i] - ave);
	var = (int64_t)(var_sum / size);
	return var;
}

static int64_t ak09970_fst_check_diff(const int16_t buf[], int size)
{
	int i;
	int64_t val;
	int ret = AK09970_RETCODE_FST_FAIL;

	if (size == 0) {
		hwlog_err("%s: size is 0 during checking diff\n",
			__func__);
		return 0;
	}
	val = buf[0];
	for (i = 0; i < size; i++) {
		if (val != buf[i]) {
			ret = AK09970_RETCODE_SUCCESS;
			break;
		}
		val = buf[i];
	}

	return ret;
}

static int ak09970_fst_reset(const struct type_device_data *akm_ak09970)
{
	int ret;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

#ifdef AK09970_FST_RESET_HARD_CONFIG
	if (ak09970_gpio_rst == 0) {
		hwlog_err("%s: ak09970_gpio_rst = 0\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}
	ret = ak09970_hard_reset(ak09970_gpio_rst);
#else
	ret = ak09970_soft_reset(akm_ak09970);
#endif
	if (ret != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: reset failed. result = %d\n", __func__, ret);
		return AK09970_RETCODE_RESET_FAIL;
	}
	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_check_chip_id(const struct i2c_client *client)
{
	uint8_t reg_wia[AK09970_WIA_SIZE];
	uint16_t chip_id;
	int ret;

	if (client == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	ret = i2c_smbus_read_i2c_block_data(client,
		AK09970_REG_WIA_ADDR, AK09970_WIA_SIZE, reg_wia);
	if (ret < 0) {
		hwlog_info("%s: read chip_id fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c read fail\n");
		return AK09970_RETCODE_I2C_OP_FAIL;
	}

	chip_id = make_u16(reg_wia[1], reg_wia[0]);
	if (chip_id != AK09970_WIA_VALUE) {
		hwlog_err("%s: invalid device id, chip_id = 0x%x\n",
			__func__, chip_id);
		return AK09970_RETCODE_ID_CHECK_FAIL;
	}
	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_get_onedata_config(
	const struct type_device_data *akm_ak09970)
{
	struct i2c_client *client = NULL;
	int ret;
	int st1_retry_count = AK09970_FST_DRDY_RETRY_MAX;
	uint8_t st1_reg[AK09970_ST1_SIZE];
	uint16_t st1;
	int result;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	client = akm_ak09970->i2c;
	if (client == NULL) {
		hwlog_err("%s: client is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	if (ak09970_set_mode(akm_ak09970, AK09970_MODE_SNG_MEASURE_MODE) !=
		AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: set mode failed\n", __func__);
		return AK09970_RETCODE_FST_SET_MODE_FAIL;
	}

	// Spec 8.4, Max measurement time is 0.872ms;
	// Double wait time (2ms), to make sure DRDY bit can be set.
	mdelay(AK09970_DRDY_DELAY_MS);

	while (st1_retry_count > 0) {
		ret = i2c_smbus_read_i2c_block_data(client,
			AK09970_REG_ST1_ADDR, AK09970_ST1_SIZE, st1_reg);
		if (ret < 0) {
			hwlog_err("%s: read reg data fail\n", __func__);
			dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
				"i2c read fail\n");
			return AK09970_RETCODE_I2C_OP_FAIL;
		}

		st1 = make_u16(st1_reg[0], st1_reg[1]);
		if (st1 == AK09970_ST1_IS_DRDY_VALUE) {
			result = AK09970_RETCODE_SUCCESS;
			break;
		} else {
			mdelay(AK09970_DRDY_DELAY_MS);
			result = AK09970_RETCODE_FST_IS_DRDY_FAIL;
		}
		hwlog_err("%s: ST1 not ready retry, st1 = 0x%x\n",
			__func__, st1);
		st1_retry_count--;
	}

	if (result != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ST1 DRDY check failed\n", __func__);
		return AK09970_RETCODE_FST_IS_DRDY_FAIL;
	}

	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_get_onedata_read(
	const struct type_device_data *akm_ak09970,
	struct type_mag_data *mag_data)
{
	struct i2c_client *client = NULL;
	int ret;
	uint8_t reg_data[AK09970_BDATA_SIZE];
	uint8_t st1_reg[AK09970_ST1_SIZE]; // 2 is st1_reg size
	uint16_t st1;

	if ((akm_ak09970 == NULL) || (mag_data == NULL)) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	client = akm_ak09970->i2c;
	if (client == NULL) {
		hwlog_err("%s: client is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	ret = i2c_smbus_read_i2c_block_data(client, AK09970_REG_ALL_AXES_ADDR,
		AK09970_BDATA_SIZE, reg_data);
	if (ret < 0) {
		hwlog_err("%s: read reg data fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c read fail\n");
		return AK09970_RETCODE_I2C_OP_FAIL;
	}

	mag_data->x = make_s16(reg_data[6], reg_data[7]);
	mag_data->y = make_s16(reg_data[4], reg_data[5]);
	mag_data->z = make_s16(reg_data[2], reg_data[3]);

	ret = i2c_smbus_read_i2c_block_data(client, AK09970_REG_ST1_ADDR,
		AK09970_ST1_SIZE, st1_reg);
	if (ret < 0) {
		hwlog_err("%s: read reg data fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c read fail\n");
		return AK09970_RETCODE_I2C_OP_FAIL;
	}

	st1 = make_u16(st1_reg[0], st1_reg[1]);
	if (st1 != AK09970_ST1_NO_DRDY_VALUE) {
		hwlog_err("%s: ST1 NO_DRDY check failed\n", __func__);
		return AK09970_RETCODE_FST_NO_DRDY_FAILED;
	}

	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_get_onedata(const struct type_device_data *akm_ak09970,
	struct type_mag_data *mag_data)
{
	int ret;

	if ((akm_ak09970 == NULL) || (mag_data == NULL)) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	ret = ak09970_fst_get_onedata_config(akm_ak09970);
	if (ret != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_fst_get_onedata_config fail\n",
			__func__);
		return ret;
	}
	ret = ak09970_fst_get_onedata_read(akm_ak09970, mag_data);
	if (ret != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_fst_get_onedata_read fail\n",
			__func__);
		return ret;
	}
	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_check_axis_data(const int16_t buf_data[], int size)
{
	int64_t buf_var;
	int ret;

	if (buf_data == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	buf_var = ak09970_fst_check_var(buf_data, size);
	hwlog_info("%s: variance[%d, %d] = %lld\n", __func__,
		AK09970_FST_VAR_LIMIT_HIGH,
		AK09970_FST_VAR_LIMIT_LOW, buf_var);
	if (buf_var <= AK09970_FST_VAR_LIMIT_LOW) {
		hwlog_err("%s: variance exceeds low limitation\n", __func__);
		ret = ak09970_fst_check_diff(buf_data, size);
		if (ret != AK09970_RETCODE_SUCCESS)
			return AK09970_RETCODE_FST_VAR_L_FAIL;
	}
#ifdef AK09970_FST_CHECK_VAR_UPPER_LIMIT
	if (buf_var >= AK09970_FST_VAR_LIMIT_HIGH) {
		hwlog_err("%s: variance exceeds high limitation\n", __func__);
		return AK09970_RETCODE_FST_VAR_H_FAILED;
#endif
	}

	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_check_data(const struct type_mag_data buf_data[],
	int size)
{
	int16_t buf_tmp[AK09970_FST_DATA_BUF_SIZE];
	int i;
	int ret;

	if (buf_data == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}
	if (size != AK09970_FST_DATA_BUF_SIZE) {
		hwlog_err("%s: buf_data size error\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}

	hwlog_info("%s: check axis_x variance\n", __func__);
	for (i = 0; i < AK09970_FST_DATA_BUF_SIZE; i++)
		buf_tmp[i] = buf_data[i].x;
	ret = ak09970_fst_check_axis_data(buf_tmp, size);
	if (ret != AK09970_RETCODE_SUCCESS)
		return ret;
	hwlog_info("%s: check axis_y variance\n", __func__);
	for (i = 0; i < AK09970_FST_DATA_BUF_SIZE; i++)
		buf_tmp[i] = buf_data[i].y;
	ret = ak09970_fst_check_axis_data(buf_tmp, size);
	if (ret != AK09970_RETCODE_SUCCESS)
		return ret;
	hwlog_info("%s: check axis_z variance\n", __func__);
	for (i = 0; i < AK09970_FST_DATA_BUF_SIZE; i++)
		buf_tmp[i] = buf_data[i].z;
	ret = ak09970_fst_check_axis_data(buf_tmp, size);
	if (ret != AK09970_RETCODE_SUCCESS)
		return ret;

	return AK09970_RETCODE_SUCCESS;
}

static int ak09970_fst_run(const struct type_device_data *akm_ak09970)
{
	int result;
	struct type_mag_data buf_data[AK09970_FST_DATA_BUF_SIZE];
	int i;

	if (akm_ak09970 == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	result = ak09970_fst_reset(akm_ak09970);
	if (result != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_fst_reset result = %d\n",
			__func__, result);
		return result;
	}
	result = ak09970_check_chip_id(akm_ak09970->i2c);
	if (result != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_check_chip_id result = %d\n",
			__func__, result);
		return result;
	}
	if (ak09970_set_mode(akm_ak09970, AK09970_MODE_POWERDOWN_MODE) !=
		AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_set_mode result = %d\n",
			__func__, result);
		return AK09970_RETCODE_FST_SET_MODE_FAIL;
	}
	for (i = 0; i < AK09970_FST_DATA_BUF_SIZE; i++) {
		result = ak09970_fst_get_onedata(akm_ak09970, &buf_data[i]);
		if (result != AK09970_RETCODE_SUCCESS) {
			hwlog_err("%s: ak09970_fst_get %d data result = %d\n",
				__func__, i, result);
			return result;
		}
	}

	result = ak09970_fst_check_data(buf_data, AK09970_FST_DATA_BUF_SIZE);
	if (result != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: ak09970_fst_check_data result = %d\n",
			__func__, result);
		return result;
	}

	return AK09970_RETCODE_SUCCESS;
}

static ssize_t ak09970_enable_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct type_device_data *akm_ak09970 = NULL;
	uint8_t val;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}

	val = akm_ak09970->enable_flag ? 1 : 0;
	hwlog_info("%s: enable_flag = %d\n", __func__, val);

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t ak09970_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t size)
{
	unsigned long val;
	struct type_device_data *akm_ak09970 = NULL;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}

	if (size == 0) {
		hwlog_err("%s: input data size is 0\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}

	if (strict_strtol(buf, AKM_BASE_NUM, &val))
		return -EINVAL;

	mutex_lock(&akm_ak09970->val_mutex);
	akm_ak09970->enable_flag = (val == 0) ? false : true;
	hwlog_info("%s: enable_flag(val) = %ld\n", __func__, val);
	mutex_unlock(&akm_ak09970->val_mutex);

	ak09970_enable_set(akm_ak09970);

	return size;
}

static ssize_t ak09970_odr_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct type_device_data *akm_ak09970 = NULL;
	uint32_t val;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}

	val = akm_ak09970->odr;
	hwlog_info("%s: odr_val = %d\n", __func__, val);

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t ak09970_odr_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t size)
{
	unsigned long val;
	struct type_device_data *akm_ak09970 = NULL;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	if (size == 0) {
		hwlog_err("%s: input data size is 0\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}

	if (strict_strtol(buf, AKM_BASE_NUM, &val))
		return -EINVAL;

	hwlog_info("%s: val = %ld \n", __func__, val);

	if (val <= AK09970_ODR_10HZ)
		val = AK09970_ODR_10HZ;
	else if (val <= AK09970_ODR_20HZ)
		val = AK09970_ODR_20HZ;
	else if (val <= AK09970_ODR_50HZ)
		val = AK09970_ODR_50HZ;
	else
		val = AK09970_ODR_100HZ;

	mutex_lock(&akm_ak09970->val_mutex);
	akm_ak09970->odr = (uint32_t)val;
	mutex_unlock(&akm_ak09970->val_mutex);
	hwlog_info("%s: odr_val = %ld\n", __func__, val);

	ak09970_poll_delay_set(akm_ak09970);

	return size;
}

static ssize_t ak09970_mag_data_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	const int NS_2_MS = 1000000;
	const int TIMEOUT_SCALE = 2;
	struct type_device_data *akm_ak09970 = NULL;
	int timeout_ms;
	int32_t data[AXIS_COUNT];
	int ret;
	int error_flag = 0;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}
	// convert ns to ms, then double it as time-out parameter
	timeout_ms = ak09970_get_poll_delay(akm_ak09970) / NS_2_MS *
		TIMEOUT_SCALE;
	ret = wait_event_interruptible_timeout(akm_ak09970->read_waitq,
		akm_ak09970->data_refresh_flag, msecs_to_jiffies(timeout_ms));
	if (ret == 0) { // timeout
		error_flag = 1;
		hwlog_err("%s: mag_data wait timeout\n", __func__);
	}

	mutex_lock(&akm_ak09970->sensor_mutex);
	data[AXIS_X] = akm_ak09970->sensor_data[AXIS_X];
	data[AXIS_Y] = akm_ak09970->sensor_data[AXIS_Y];
	data[AXIS_Z] = akm_ak09970->sensor_data[AXIS_Z];
	akm_ak09970->data_refresh_flag = false;
	mutex_unlock(&akm_ak09970->sensor_mutex);

	return scnprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n",
		data[AXIS_X], data[AXIS_Y], data[AXIS_Z], error_flag);
}

static ssize_t ak09970_self_test_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct type_device_data *akm_ak09970 = NULL;
	bool enable = false;
	int ret;
	int result = 0;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s: called\n", __func__);

	enable = akm_ak09970->enable_flag;
	if (enable) {
		hwlog_info("%s: stop sensor running for self-test\n", __func__);
		mutex_lock(&akm_ak09970->val_mutex);
		akm_ak09970->enable_flag = false;
		mutex_unlock(&akm_ak09970->val_mutex);

		ak09970_enable_set(akm_ak09970);
	}

	ret = ak09970_fst_run(akm_ak09970);
	if (ret != AK09970_RETCODE_SUCCESS)
		result = AK09970_RETCODE_FST_FAIL;

	return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ak09970_nv_data_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct type_nv_data nv_para;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}

	memset(&nv_para, 0, sizeof(nv_para));
	if (read_calibrate_data_from_nv(POS_HALL_NV_NUM,
		POS_HALL_NV_SIZE, POS_HALL_NV_NAME, &nv_para) !=
		AK09970_RETCODE_SUCCESS)
		return AK09970_RETCODE_COMMON_ERR;

	return scnprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d\n",
		nv_para.close_x,nv_para.close_y,nv_para.close_z,
		nv_para.open_x,nv_para.open_y,nv_para.open_z);
}

static ssize_t ak09970_nv_data_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t size)
{
	const int SCANF_DATA_COUNT = 6;
	struct type_device_data *akm_ak09970 = NULL;
	int count;
	int ret;

	if ((dev == NULL) || (attr == NULL) || (buf == NULL)) {
		hwlog_err("%s: input NULL!! \n", __func__);
		return -EINVAL;
	}
	if (size == 0) {
		hwlog_err("%s: input data size is 0\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}

	akm_ak09970 = dev_get_drvdata(dev);
	if (akm_ak09970 == NULL) {
		hwlog_err("%s: akm_ak09970 NULL!! \n", __func__);
		return -EINVAL;
	}
	mutex_lock(&akm_ak09970->nv_mutex);
	count = sscanf(buf,"%d,%d,%d,%d,%d,%d",
		&(akm_ak09970->written_calibrate_data.close_x),
		&(akm_ak09970->written_calibrate_data.close_y),
		&(akm_ak09970->written_calibrate_data.close_z),
		&(akm_ak09970->written_calibrate_data.open_x),
		&(akm_ak09970->written_calibrate_data.open_y),
		&(akm_ak09970->written_calibrate_data.open_z));
	akm_ak09970->nvwrite_finish_flag = false;
	akm_ak09970->nvwrite_success_flag = false;
	mutex_unlock(&akm_ak09970->nv_mutex);
	if (count != SCANF_DATA_COUNT) {
		hwlog_err("%s: sscanf fail\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}
	hwlog_info("%s: nv_write buf = %s\n", __func__, buf);
	queue_work(akm_ak09970->nvwrite_queue,
		&akm_ak09970->nvwrite_work.work);
	ret = wait_event_interruptible_timeout(akm_ak09970->nvwrite_waitq,
		akm_ak09970->nvwrite_finish_flag,
		msecs_to_jiffies(NV_WRITE_TIME_COST_MS));
	if (ret == 0) { // timeout
		hwlog_err("nv_write wait timeout\n");
		return AK09970_RETCODE_COMMON_ERR;
	}
	if (akm_ak09970->nvwrite_success_flag == false) {
		hwlog_err("nv_write fail\n");
		return AK09970_RETCODE_COMMON_ERR;
	}
	return size;
}

DEVICE_ATTR(ak09970_enable, 0660,
	ak09970_enable_show, ak09970_enable_store);
DEVICE_ATTR(ak09970_odr, 0660,
	ak09970_odr_show, ak09970_odr_store);
DEVICE_ATTR(ak09970_mag_data, 0440,
	ak09970_mag_data_show, NULL);
DEVICE_ATTR(ak09970_self_test, 0440,
	ak09970_self_test_show, NULL);
DEVICE_ATTR(ak09970_nv_data, 0660,
	ak09970_nv_data_show, ak09970_nv_data_store);

static struct attribute *ak09970_attributes[] = {
	&dev_attr_ak09970_enable.attr,
	&dev_attr_ak09970_odr.attr,
	&dev_attr_ak09970_mag_data.attr,
	&dev_attr_ak09970_self_test.attr,
	&dev_attr_ak09970_nv_data.attr,
	NULL,
};

static const struct attribute_group ak09970_attr_group = {
	.attrs = ak09970_attributes,
};

static const struct attribute_group *ak09970_attr_groups[] = {
	&ak09970_attr_group,
	NULL,
};

static int create_sysfs_interfaces(struct type_device_data *akm_ak09970)
{
	int err = 0;

	if (akm_ak09970 == NULL)
		return -EINVAL;

	// class_destroy in remove_sysfs_interfaces
	akm_ak09970->dev_class = class_create(THIS_MODULE,
		AK09970_SYSCLS_NAME);
	if (IS_ERR(akm_ak09970->dev_class)) {
		err = PTR_ERR(akm_ak09970->dev_class);
		goto class_create_failed;
	}

	akm_ak09970->dev_class->dev_groups = ak09970_attr_groups;

	// device_destroy in create_sysfs_interfaces
	akm_ak09970->class_dev = device_create(akm_ak09970->dev_class,
		NULL, 0, akm_ak09970, AK09970_SYSDEV_NAME);
	if (IS_ERR(akm_ak09970->class_dev)) {
		err = PTR_ERR(akm_ak09970->class_dev);
		goto device_create_failed;
	}

	return err;
device_create_failed:
	akm_ak09970->class_dev = NULL;
	class_destroy(akm_ak09970->dev_class);
class_create_failed:
	akm_ak09970->dev_class = NULL;
	return err;
}

static void remove_sysfs_interfaces(struct type_device_data *akm_ak09970)
{
	if (akm_ak09970 == NULL)
		return;

	if (akm_ak09970->dev_class != NULL) {
		// device_create in create_sysfs_interfaces
		device_destroy(akm_ak09970->dev_class, 0);
		// class_create in create_sysfs_interfaces
		class_destroy(akm_ak09970->dev_class);
		akm_ak09970->dev_class = NULL;
	}

	return;
}

static int16_t check_data_overflow_in_q16(int16_t data_val)
{
	if (data_val >= AK09970_SCALE_DATA_RANGE)
		return AK09970_SCALE_DATA_RANGE;
	else if (data_val <= -AK09970_SCALE_DATA_RANGE)
		return -AK09970_SCALE_DATA_RANGE;
	else
		return data_val;
}

static void ak09970_dev_poll(struct work_struct *work)
{
	struct type_device_data *akm_ak09970 = NULL;
	struct i2c_client *client = NULL;
	uint8_t reg_data[AK09970_BDATA_SIZE];
	uint16_t st1;
	int16_t data[AXIS_COUNT];
	int ret;

	if (work == NULL) {
		hwlog_err("%s: NULL pointer input\n", __func__);
		return;
	}

	akm_ak09970 = container_of((struct delayed_work *)work,
		struct type_device_data, dwork);
	client = akm_ak09970->i2c;
	if (client == NULL) {
		hwlog_err("%s: client is NULL\n", __func__);
		return;
	}

	ret = i2c_smbus_read_i2c_block_data(client,
		AK09970_REG_ALL_AXES_ADDR, AK09970_BDATA_SIZE, reg_data);
	if (ret < 0) {
		hwlog_err("%s: read reg data fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GS_I2C_ERROR, __func__,
			"i2c read fail\n");
		return;
	}

	st1 = make_u16(reg_data[0], reg_data[1]); // 0 - ST1_H, 1 - ST1-L
	data[AXIS_X] = make_s16(reg_data[6], reg_data[7]); // 6 - X_H, 7 - X_L
	data[AXIS_X] = check_data_overflow_in_q16(data[AXIS_X]);
	data[AXIS_Y] = make_s16(reg_data[4], reg_data[5]); // 4 - Y_H, 5 - Y_L
	data[AXIS_Y] = check_data_overflow_in_q16(data[AXIS_Y]);
	data[AXIS_Z] = make_s16(reg_data[2], reg_data[3]); // 2 - Z_H, 3 - Z_L
	data[AXIS_Z] = check_data_overflow_in_q16(data[AXIS_Z]);

	if (ak09970_adc_overflow(st1))
		hwlog_err("%s: sensor data overflow\n", __func__);

	mutex_lock(&akm_ak09970->sensor_mutex);
	akm_ak09970->sensor_data[AXIS_X] =
		(int32_t)(data[AXIS_X] * AK09970_SCALE_AMPLIFY_Q16);
	akm_ak09970->sensor_data[AXIS_Y] =
		(int32_t)(data[AXIS_Y] * AK09970_SCALE_AMPLIFY_Q16);
	akm_ak09970->sensor_data[AXIS_Z] =
		(int32_t)(data[AXIS_Z] * AK09970_SCALE_AMPLIFY_Q16);
	akm_ak09970->data_refresh_flag = true;
	if (waitqueue_active(&akm_ak09970->read_waitq))
		wake_up_interruptible(&akm_ak09970->read_waitq);
	mutex_unlock(&akm_ak09970->sensor_mutex);
}

static enum hrtimer_restart ak09970_timer_func(struct hrtimer *timer)
{
	struct type_device_data *akm_ak09970 = NULL;
	int64_t delay;

	akm_ak09970 = container_of(timer, struct type_device_data, poll_timer);
	delay = ak09970_get_poll_delay(akm_ak09970);
	queue_work(akm_ak09970->work_queue, &akm_ak09970->dwork.work);
	hrtimer_forward_now(&akm_ak09970->poll_timer, ns_to_ktime(delay));
	return HRTIMER_RESTART;
}

static int init_ak09970_device(struct i2c_client *client)
{
	int ret;
	struct device *dev = &client->dev;

	ak09970_gpio_rst = of_get_named_gpio(dev->of_node, "gpio_rst", 0);
	hwlog_info("%s: ak09970_gpio_rst = %d\n", __func__, ak09970_gpio_rst);
	if (ak09970_gpio_rst == 0) {
		hwlog_err("%s: ak09970_gpio_rst = 0\n", __func__);
		return AK09970_RETCODE_COMMON_ERR;
	}
	ret = devm_gpio_request(dev, ak09970_gpio_rst, "gpio_rst");
	if (ret != 0) {
		hwlog_err("%s: reset pin request failed. ret = %d\n",
			__func__, ret);
		return ret;
	}
	ret = ak09970_hard_reset(ak09970_gpio_rst);
	if (ret != AK09970_RETCODE_SUCCESS) {
		hwlog_err("%s: hard reset failed. ret = %d\n", __func__, ret);
		goto gpio_free;
	}
	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA)) {
		hwlog_err("%s: i2c smbus byte data unsupported\n", __func__);
		ret = -EOPNOTSUPP;
		goto gpio_free;
	}
	ret = ak09970_check_chip_id(client);
	if (ret < 0) {
		hwlog_err("%s: ak09970_check_chip_id fail\n", __func__);
		goto gpio_free;
	}
	return AK09970_RETCODE_SUCCESS;

gpio_free:
	devm_gpio_free(dev, ak09970_gpio_rst);
	return ret;
}

static int init_ak09970_struct(struct i2c_client *client)
{
	ak09970_dev_data->i2c = client;
	i2c_set_clientdata(client, ak09970_dev_data);

	// Init variables
	mutex_init(&ak09970_dev_data->op_mutex);
	mutex_init(&ak09970_dev_data->sensor_mutex);
	mutex_init(&ak09970_dev_data->val_mutex);
	mutex_init(&ak09970_dev_data->nv_mutex);

	ak09970_dev_data->odr = 0;
	ak09970_dev_data->enable_flag = false;
	ak09970_dev_data->ns_delay = AK09970_DEFAULT_DELAY_NS;
	ak09970_dev_data->sensor_data[AXIS_X] = 0;
	ak09970_dev_data->sensor_data[AXIS_Y] = 0;
	ak09970_dev_data->sensor_data[AXIS_Z] = 0;
	ak09970_dev_data->data_refresh_flag = false;
	ak09970_dev_data->nvwrite_finish_flag = false;
	ak09970_dev_data->nvwrite_success_flag = false;

	// stop poll_timer in ak09970_remove
	hrtimer_init(&ak09970_dev_data->poll_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ak09970_dev_data->poll_timer.function = ak09970_timer_func;

	// destroy_workqueue in ak09970_remove
	init_waitqueue_head(&ak09970_dev_data->read_waitq);
	ak09970_dev_data->work_queue = alloc_workqueue("ak09970_poll_work",
		WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (ak09970_dev_data->work_queue == NULL) {
		hwlog_err("%s: unable to creat new work queue\n",__func__);
		return AK09970_RETCODE_COMMON_ERR;
	}
	INIT_WORK(&ak09970_dev_data->dwork.work, ak09970_dev_poll);
	init_waitqueue_head(&ak09970_dev_data->nvwrite_waitq);
	ak09970_dev_data->nvwrite_queue = alloc_workqueue("ak09970_nvwrite_work",
		WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (ak09970_dev_data->nvwrite_queue == NULL) {
		hwlog_err("%s: unable to creat nvwrite work queue\n",
			__func__);
		return AK09970_RETCODE_COMMON_ERR;
	}
	INIT_WORK(&ak09970_dev_data->nvwrite_work.work, nvwrite_process);
	return AK09970_RETCODE_SUCCESS;
}

static void deinit_ak09970_struct(struct i2c_client *client)
{
	struct type_device_data *akm_ak09970 = NULL;

	hwlog_info("%s: called\n", __func__);
	akm_ak09970 = i2c_get_clientdata(client);
	if (akm_ak09970->work_queue != NULL) {
		destroy_workqueue(akm_ak09970->work_queue);
		akm_ak09970->work_queue = NULL;
	}
	if (akm_ak09970->nvwrite_queue != NULL) {
		destroy_workqueue(akm_ak09970->nvwrite_queue);
		akm_ak09970->nvwrite_queue = NULL;
	}
	if (hrtimer_active(&akm_ak09970->poll_timer))
		hrtimer_cancel(&akm_ak09970->poll_timer);
	i2c_set_clientdata(client, NULL);
}

static void deinit_ak09970_device(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	if (ak09970_gpio_rst != 0) {
		devm_gpio_free(dev, ak09970_gpio_rst);
		ak09970_gpio_rst = 0;
	}
}

int ak09970_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int ret;

	hwlog_info("%s: called\n", __func__);
	(void)idp;
	if (client == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	ret = init_ak09970_device(client);
	if (ret < 0) {
		hwlog_err("%s: init_ak09970_device fail\n", __func__);
		dmd_log_report(DSM_VIBRATOR_GC_FAILURE, __func__,
			"init_ak09970_device fail\n");
		return ret;
	}

	ak09970_dev_data = kzalloc(sizeof(*ak09970_dev_data), GFP_KERNEL);
	if (ak09970_dev_data == NULL) {
		hwlog_err("%s: memory allocation failed\n", __func__);
		ret = -ENOMEM;
		goto ak09970_malloc_fail;
	}

	ret = init_ak09970_struct(client);
	if (ret < 0) {
		hwlog_err("%s: init_ak09970_struct fail\n", __func__);
		goto ak09970_struct_init_fail;
	}

	ret = create_sysfs_interfaces(ak09970_dev_data);
	if (ret < 0) {
		hwlog_err("%s: create_sysfs_interfaces fail\n", __func__);
		goto ak09970_struct_init_fail;
	}

	hwlog_info("%s: success\n", __func__);
	return AK09970_RETCODE_SUCCESS;
ak09970_struct_init_fail:
	kfree(ak09970_dev_data);
ak09970_malloc_fail:
	deinit_ak09970_device(client);
	return ret;
}

int ak09970_remove(struct i2c_client *client)
{
	struct type_device_data *akm_ak09970 = NULL;

	hwlog_info("%s: called\n", __func__);
	if (client == NULL) {
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return AK09970_RETCODE_NULL_POINT_ERR;
	}

	akm_ak09970 = i2c_get_clientdata(client);
	remove_sysfs_interfaces(akm_ak09970);
	deinit_ak09970_struct(client);
	kfree(ak09970_dev_data);
	deinit_ak09970_device(client);

	return AK09970_RETCODE_SUCCESS;
}

static struct i2c_device_id ak09970_idtable[] = {
	{ "ak09970", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ak09970_idtable);

static const struct of_device_id ak09970_of_id_table[] = {
	{ .compatible = "akm,ak09970" },
	{},
};

static struct i2c_driver ak09970_driver = {
	.driver = {
		.name = "ak09970",
		.owner = THIS_MODULE,
		.of_match_table = ak09970_of_id_table,
	},
	.id_table = ak09970_idtable,
	.probe = ak09970_probe,
	.remove = ak09970_remove,
};

static int __init ak09970_init(void)
{
	int rc;

	hwlog_info("%s: called\n", __func__);
	rc = i2c_add_driver(&ak09970_driver);
#ifdef CONFIG_HUAWEI_DSM
	if (!vibrator_dclient)
		vibrator_dclient = dsm_find_client("dsm_vibrator");
	if (!vibrator_dclient)
		vibrator_dclient = dsm_register_client(&dsm_vibrator);
	if (!vibrator_dclient)
		hwlog_err("dsm register client failed\n");
	else
		hwlog_info("dsm register client successfully\n");
#endif
	hwlog_info("%s: done, rc = %d\n", __func__, rc);
	return rc;
}

static void __exit ak09970_exit(void)
{
	hwlog_info("%s: called\n", __func__);
	i2c_del_driver(&ak09970_driver);
#ifdef CONFIG_HUAWEI_DSM
	if (vibrator_dclient) {
		dsm_unregister_client(vibrator_dclient, &dsm_vibrator);
		vibrator_dclient = NULL;
	}
	hwlog_info("dsm unregister client successfully\n");
#endif
}

module_init(ak09970_init);
module_exit(ak09970_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("AKM09970 driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
