#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include "../../huawei_ts_kit_algo.h"
#include "../../huawei_ts_kit_api.h"
#include "atmel.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#define SELFCAPMAX 5000
#define T38COVERCUT 58
#define T38COVERSUPWIDL 59
#define T38COVERSUPWIDH 60
#define T38COVERGLASSFL 61
#define T38GLOVESUPWIDL 62
#define T38GLOVESUPWIDH 63

#define REPORT_RATE_PASS "-4P"
#define REPORT_RATE_FAIL "-4F"
#define SHORT_TEST_PASS "-8P"
#define SHORT_TEST_FAIL "-8F"

#define PMU_GPIO_TYPE 1
#define PMU_LDO_TYPE 1
#define PMU_DEFAULT_TYPE 0

#define ASCII_LF (0x0A)
#define ASCII_CR (0x0D)
#define ASCII_COMMA (0x2C)
#define ASCII_ZERO (0x30)
#define ASCII_NINE (0x39)

#define MAX_CASE_CNT 5
#define RAWDATA_LIMIT 0
#define RAWDATA_TXTX_DELTA_LIMIT 1
#define RAWDATA_RXRX_DELTA_LIMIT 2
#define NOISE_LIMIT 3
#define RAWDATA_MIN_MAX_DELTA_LIMIT 4

#define MXT_SCAN_ONCE_TIME  20
#define MXT_NORMAL_MODE_WAIT_SCAN_ONCE_TIME  100
#define MXT_DOZE_MODE_WAIT_SCAN_ONCE_TIME   150

#define FEATUREFILE_NOT_READ 0     	//feature file not read yet, glove ,hoslter...
#define FEATUREFILE_READ_SUCCESS 1	//feature file read success

#define AUX_DATA_OFFSET  6
#define MAX_FINGER_DATA_SIZE 10
#define NEW_IC_ADDRESS_DELTA 0x24
#define OLD_IC_ADDRESS_DELTA 0x26

#define ATMEL_VENDER_NAME  "atmel"

#ifdef ROI
int atmel_gMxtT6Cmd58Flag = MXT_T6_CMD58_ON;
#endif

#define T100_PROC_FINGER_NUM
#define AVDD_LDO_VALUE (3300000)
#define VDDIO_LDO_VALUE (1850000)
#define ATMEL_MXT_MOISTURE_MODE 0x0a

#define DEFAULT_X_MAX  (1600)
#define DEFAULT_Y_MAX  (2560)

#define T72_STATE_NOT_CHANGE 0
#define T72_STATE_CHANGED 1
#define DEFAULT_NOISE_LEVEL 5

#define STABLE_STATUS 0
#define NOISE_STATUS 1
#define VNOISE_STATUS 2
#define MAX_DUALX_STATUS 3

#define MAX_RAWDATA_DELTA_LIMIT_COUNT 2
#define MAX_NOISE_LIMIT_COUNT 1
#define MAX_RAWDATA_MIN_MAX_DELTA_LIMIT_COUNT 1

#define TP_COLOR_SIZE 15
#define DEFAULT_ACTIVE2IDLE_TIME 50		//the default time of tp active status change to idle status
#define T7_ACT2IDLETO_OFFSET 2			//tp status active to idle

#if defined (CONFIG_TEE_TUI)
static struct tui_mxt_data_t {
	char device_name[10];
	u8 max_reportid;
	u16 T5_address;
	u8 T5_msg_size;
	u16 T44_address;
	u16 T100_address;
	u8 T100_reportid_min;
	u8 T100_reportid_max;
	u16 addr;
} tui_mxt_data;
#endif

extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];

atomic_t atmel_mmi_test_status = ATOMIC_INIT(0);

static volatile bool mxt_t100_int = true;//  need report input

bool atmel_update_sd_mode = false;   //true: is sd update mode , false : auto update mode
static int featurefile_read_success = FEATUREFILE_NOT_READ;


static atomic_t atmel_mxt_moisture_flag = ATOMIC_INIT(0);  //water mode
static atomic_t atmel_mxt_vnoise_flag = ATOMIC_INIT(0); //noise

#ifdef ROI
static unsigned char roi_data[ROI_DATA_READ_LENGTH] = { 0 };
#endif

struct mxt_data *mxt_core_data = NULL;

static void mxt_parse_module_dts(struct ts_kit_device_data *chip_data);
static int atmel_chip_detect(struct ts_kit_platform_data *platform_data);
static int atmel_init_chip(void);
static int atmel_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data);
static int atmel_irq_top_half(struct ts_cmd_node *cmd);
static int atmel_irq_bottom_half(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd);
static int atmel_fw_update_boot(char *file_name);
static int atmel_fw_update_sd(void);
static int atmel_chip_get_info(struct ts_chip_info_param *info);
static int atmel_before_suspend(void);
static int atmel_suspend(void);
static int atmel_resume(void);
static int atmel_after_resume(void *feature_info);
static void atmel_shutdown(void);
static int atmel_input_config(struct input_dev *input_dev);
static int atmel_get_rawdata(struct ts_rawdata_info *info,
			     struct ts_cmd_node *out_cmd);
static unsigned char *atmel_roi_rawdata(void);
static int atmel_chip_get_capacitance_test_type(struct ts_test_type_info *info);
static int atmel_charger_switch(struct ts_charger_info *info);
static int atmel_chip_glove_switch(struct ts_glove_info *info);
static int atmel_chip_holster_switch(struct ts_holster_info *info);

static int atmel_roi_switch(struct ts_roi_info *info);
static int mxt_set_feature(struct mxt_data *data, int featurecode,
			   int enableflag);
static int mxt_enable_feature(struct mxt_data *data, int featurecode);
static int mxt_disable_feature(struct mxt_data *data, int featurecode);
static int atmel_init_feature_file(bool from_sd);
static int atmel_regs_operate(struct ts_regs_info *info);
static int atmel_debug_switch(u8 loglevel);
static void mxt_disable_all_features(void);
static int mxt_initialize_info_block(struct mxt_data *data);

static void atmel_special_hardware_test_switch(unsigned int value);
static int atmel_special_hardware_test_result(char *buf);
static int mxt_get_module_name(void);
static void atmel_dsm_record_chip_status(struct mxt_data *data);
static void atmel_status_resume(void);

static int atmel_parse_threshold_file(void);
static int atmel_parse_threshold_file_method(const char *buf, uint32_t file_size);
static int atmel_get_one_value(const char *buf, uint32_t *offset);
static int atmel_ctoi(char *buf, uint32_t count);
static void atmel_chip_touch_switch(void);
extern void ts_i2c_error_dmd_report(u8* reg_addr);

/*WORKAROUND1 finger down workaround*/
/*WORKAROUND2 glove down workaround*/
/*WORKAROUND3 for T72 noise level check feature*/
/*WORKAROUND4 VN-N workaround*/
/*WORKAROUND5 for charger workaround*/
static struct feature_info mxt_feature_list[] = {
{"GLOVE", MAX_GLOVE_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"COVER", MAX_COVER_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WAKEUP", MAX_WAKEUP_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"GUESTURE", MAX_GUESTURE_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"KNUCKLE", MAX_KNUCKLE_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND1", MAX_WORKAROUND1_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND2", MAX_WORKAROUND2_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND3", MAX_WORKAROUND3_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND4", MAX_WORKAROUND4_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND5", MAX_WORKAROUND5_CONF, 0, MXT_FEATURE_DISABLE, 0},
{"WORKAROUND6", MAX_WORKAROUND6_CONF, 0, MXT_FEATURE_DISABLE, 0}
};

struct ts_device_ops ts_kit_atmel_ops = {
	.chip_detect = atmel_chip_detect,
	.chip_init = atmel_init_chip,
	.chip_parse_config = atmel_parse_dts,
	.chip_input_config = atmel_input_config,
	.chip_irq_top_half = atmel_irq_top_half,
	.chip_irq_bottom_half = atmel_irq_bottom_half,
	.chip_fw_update_boot = atmel_fw_update_boot,
	.chip_fw_update_sd = atmel_fw_update_sd,
	.chip_get_info = atmel_chip_get_info,
	.chip_before_suspend = atmel_before_suspend,
	.chip_suspend = atmel_suspend,
	.chip_resume = atmel_resume,
	.chip_after_resume = atmel_after_resume,
	.chip_get_rawdata = atmel_get_rawdata,
	.chip_roi_rawdata = atmel_roi_rawdata,
	.chip_roi_switch = atmel_roi_switch,
	.chip_shutdown = atmel_shutdown,
	.chip_get_capacitance_test_type = atmel_chip_get_capacitance_test_type,
	.chip_glove_switch = atmel_chip_glove_switch,
	.chip_holster_switch = atmel_chip_holster_switch,
	.chip_regs_operate = atmel_regs_operate,
	.chip_debug_switch = atmel_debug_switch,
	.chip_charger_switch = atmel_charger_switch,
	.chip_special_hardware_test_swtich = atmel_special_hardware_test_switch,
	.chip_special_hardware_test_result = atmel_special_hardware_test_result,
	.chip_touch_switch = atmel_chip_touch_switch,
};

static int __mxt_cache_write(struct  ts_kit_device_data *chip_data, u16 addr,
			     u16 reg, u16 len, const void *val, u8 *w_cache,
			     u8 *w_cache_pa, unsigned long flag)
{
	struct i2c_msg xfer;
	void *buf = NULL;
	u16 transferred= 0, extend = 0;
	int retry = RETRY_TEST_TIME;
	int ret = 0;

	if (NULL == chip_data || NULL == chip_data->ts_platform_data ||
			NULL == chip_data->ts_platform_data->client) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	memset(&xfer, 0 , sizeof(struct i2c_msg));
	if (test_flag(I2C_ACCESS_NO_REG, &flag)) {
		extend = 0;
		if (test_flag(I2C_ACCESS_NO_CACHE, &flag)) {
			w_cache = (u8 *) val;
			w_cache_pa = (u8 *) val;
		}
	} else {
		extend = 2;  //need 2 bytes for reg addr
		if (test_flag(I2C_ACCESS_NO_CACHE, &flag)) {
			buf = kzalloc(len + extend, GFP_KERNEL);
			if (!buf)
				return -ENOMEM;
			w_cache = buf;
			w_cache_pa = buf;
		}
		w_cache[0] = reg & 0xff;
		w_cache[1] = (reg >> 8) & 0xff;
	}

	/* Write register */
	xfer.addr = addr;
	xfer.flags = 0;
	xfer.buf = w_cache_pa;

	transferred = 0;
	while (transferred < len) {
		xfer.len = len - transferred + extend;
		if (xfer.len > MXT_MAX_BLOCK_WRITE)
			xfer.len = MXT_MAX_BLOCK_WRITE;

		if (test_flag(I2C_ACCESS_NO_CACHE, &flag) &&
		    test_flag(I2C_ACCESS_NO_REG, &flag))
			xfer.buf = w_cache_pa + transferred;
		else
			memcpy(w_cache + extend, val + transferred,
			       xfer.len - extend);

		if (extend) {
			w_cache[0] = (reg + transferred) & 0xff;
			w_cache[1] = ((reg + transferred) >> 8) & 0xff;
		}

retry_write:
		/*ret = chip_data->bops->bus_xfer(&xfer, 1) ;*/
		ret = i2c_transfer(chip_data->ts_platform_data->client->adapter, &xfer, 1);
		if (ret != 1) {		/* i2c write success */
			if (retry) {
				TS_LOG_INFO
				    ("%s: i2c transfer(w) retry, reg %d\n",
				     __func__, reg);
				msleep(25);//delay 25ms
				retry--;
				goto retry_write;
			} else {
				TS_LOG_ERR
				    ("%s: i2c transfer(w) failed (%d) reg %d len %d transferred %d\n",
				     __func__, ret, reg, len, transferred);
				if (buf)
					kfree(buf);
#if defined (CONFIG_HUAWEI_DSM)
				ts_i2c_error_dmd_report((u8 *)&addr);
#endif
				return -EIO;
			}
		}

		transferred += xfer.len - extend;

		TS_LOG_DEBUG
		    ("[mxt] i2c transfer(w) reg %d len %d current %d transferred %d\n",
		     reg, len, xfer.len - extend, transferred);
	}

	if (buf)
		kfree(buf);
	return 0;
}

static int __mxt_cache_read(struct ts_kit_device_data *chip_data, u16 addr,
			    u16 reg, u16 len, void *val, u8 *r_cache,
			    u8 *r_cache_pa, u8 *w_cache, u8 *w_cache_pa,
			    unsigned long flag)
{
	struct i2c_msg *msgs = NULL;
	int num = 0;

	struct i2c_msg xfer[MXT_XFER_TWO_MSG];
	char buf[MXT_XFER_TWO_MSG] = {0};
	u16 transferred = 0;
	int retry = RETRY_TEST_TIME;
	int ret = 0;

	if (NULL == chip_data || NULL == chip_data->ts_platform_data ||
			NULL == chip_data->ts_platform_data->client) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	TS_LOG_DEBUG("__mxt_cache_read\n");
	memset(xfer, 0 , sizeof(struct i2c_msg) * MXT_XFER_TWO_MSG);
	if (test_flag(I2C_ACCESS_NO_CACHE, &flag)) {
		w_cache = buf;
		w_cache_pa = buf;
		r_cache = val;
		r_cache_pa = val;
	}

	if (test_flag(I2C_ACCESS_NO_REG, &flag)) {
		msgs = &xfer[1];
		num = 1;
	} else {
		w_cache[0] = reg & 0xff;
		w_cache[1] = (reg >> 8) & 0xff;

		msgs = &xfer[0];
		num = ARRAY_SIZE(xfer);

		/* Write register */
		xfer[0].addr = addr;
		xfer[0].flags = 0;
		xfer[0].len = 2;
		xfer[0].buf = w_cache_pa;
	}

	/* Read data */
	xfer[1].addr = addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].buf = r_cache_pa;

	transferred = 0;
	while (transferred < len) {
		if (!test_flag
		    (I2C_ACCESS_NO_REG | I2C_ACCESS_R_REG_FIXED, &flag)) {
			w_cache[0] = (reg + transferred) & 0xff;
			w_cache[1] = ((reg + transferred) >> 8) & 0xff;
		}

		if (test_flag(I2C_ACCESS_NO_CACHE, &flag))
			xfer[1].buf = r_cache_pa + transferred;
		xfer[1].len = len - transferred;
		if (xfer[1].len > MXT_MAX_BLOCK_READ)
			xfer[1].len = MXT_MAX_BLOCK_READ;
retry_read:
		/*ret = chip_data->bops->bus_xfer(msgs, num) ;*/
		ret = i2c_transfer(chip_data->ts_platform_data->client->adapter, msgs, num);
		if (ret != num) {
			if (retry) {
				TS_LOG_DEBUG
				    ("%s: i2c transfer(r) retry, reg %d\n",
				     __func__, reg);
				msleep(25);//delay 25ms
				retry--;
				goto retry_read;
			} else {
				TS_LOG_ERR
				    ("%s: i2c transfer(r) failed (%d) reg %d len %d transferred %d\n",
				     __func__, ret, reg, len, transferred);
#if defined (CONFIG_HUAWEI_DSM)
				ts_i2c_error_dmd_report((u8 *)&addr);
#endif
				return -EIO;
			}
		}
		if (!test_flag(I2C_ACCESS_NO_CACHE, &flag))
			memcpy(val + transferred, r_cache, xfer[1].len);
		transferred += xfer[1].len;

		TS_LOG_DEBUG
		    ("[mxt] i2c transfer(r) reg %d len %d current %d transferred %d\n",
		     reg, len, xfer[1].len, transferred);
	}
	return 0;
}

static int __atmel_read_reg_ext(struct mxt_data *data, u16 addr, u16 reg, u16 len,
			      void *val, unsigned long flag)
{
	u8 *r_cache = NULL, *r_cache_pa = NULL, *w_cache = NULL, *w_cache_pa= NULL;
	int ret = 0;

	if(!val || !data) {
		TS_LOG_ERR("%s,param invalid\n", __func__);
		return -EINVAL;
	}
	flag |= I2C_ACCESS_NO_CACHE;

	mutex_lock(&data->bus_access_mutex);
	ret =
	    __mxt_cache_read(data->atmel_chip_data, addr, reg, len, val, r_cache,
			     r_cache_pa, w_cache, w_cache_pa, flag);
	mutex_unlock(&data->bus_access_mutex);

	return ret;
}

static int __atmel_write_reg_ext(struct mxt_data *data, u16 addr, u16 reg,
			       u16 len, const void *val, unsigned long flag)
{
	u8 *w_cache = NULL, *w_cache_pa = NULL;
	int ret = 0;

	if(!val || !data) {
		TS_LOG_ERR("%s,param invalid\n", __func__);
		return -EINVAL;
	}

	flag |= I2C_ACCESS_NO_CACHE;

	mutex_lock(&data->bus_access_mutex);
	ret =
	    __mxt_cache_write(data->atmel_chip_data, addr, reg, len, val, w_cache,
			      w_cache_pa, flag);
	mutex_unlock(&data->bus_access_mutex);

	return ret;
}

int __atmel_read_reg(struct mxt_data *data, u16 reg, u16 len, void *val)
{
	return __atmel_read_reg_ext(data, data->addr, reg, len, val, 0);
}

int __atmel_write_reg(struct mxt_data *data, u16 reg, u16 len, const void *val)
{
	return __atmel_write_reg_ext(data, data->addr, reg, len, val, 0);
}

int atmel_write_reg(struct mxt_data *data, u16 reg, u8 val)
{
	return __atmel_write_reg(data, reg, 1, &val);
}

static int atmel_pinctl_get(struct mxt_data *data)
{
	int error = 0;
	if(!data || !data->atmel_dev) {
		TS_LOG_ERR("%s,param invalid\n", __func__);
		return -EINVAL;
	}
	data->pctrl = devm_pinctrl_get(&data->atmel_dev->dev);
	if (IS_ERR(data->pctrl) || !(data->pctrl)) {
		TS_LOG_ERR("failed to devm pinctrl get\n");
		error = -EINVAL;
		data->pctrl = NULL;
		return -EINVAL;
	}

	data->pins_default = pinctrl_lookup_state(data->pctrl, "default");
	if (IS_ERR(data->pins_default)) {
		TS_LOG_ERR("failed to pinctrl lookup state default\n");
		error = -EINVAL;
		data->pins_default = NULL;
		goto err;
	}

	data->pins_idle = pinctrl_lookup_state(data->pctrl, "idle");
	if (IS_ERR(data->pins_idle)) {
		TS_LOG_ERR("failed to pinctrl lookup state idle\n");
		error = -EINVAL;
		data->pins_idle = NULL;
		goto err;
	}

	return 0;

err:
	if(data->pctrl) {
		devm_pinctrl_put(data->pctrl);
		data->pctrl = NULL;
	}
	return error;
}

static int atmel_pinctl_select_normal(struct mxt_data *data)
{
	int error = 0;
	if(!data || !(data->pctrl) || !(data->pins_default)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		error = -EINVAL;
		return error;
	}
	error = pinctrl_select_state(data->pctrl, data->pins_default);
	if(error) {
		TS_LOG_ERR("%s, pinctrl set normal fail\n", __func__);
	}
	return error;
}

static int atmel_pinctl_select_idle(struct mxt_data *data)
{
	int error = 0;
	if(!data || !(data->pctrl) || !(data->pins_idle)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		error = -EINVAL;
		return error;
	}
	error = pinctrl_select_state(data->pctrl, data->pins_idle);
	if(error) {
		TS_LOG_ERR("%s, pinctrl set idle fail\n", __func__);
	}
	return error;
}

static void atmel_power_off(struct mxt_data *data)
{
	int error = 0;

	if(NULL == data || data->atmel_chip_data == NULL || data->atmel_chip_data->ts_platform_data == NULL) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		error = -EINVAL;
		goto err;
	}
	gpio_direction_output(data->atmel_chip_data->ts_platform_data->reset_gpio, 0);
	msleep(2);//sequence request

	error = ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_OFF, 0);	/* 0,no delay */
	if (error) {
		TS_LOG_ERR("%s, power off vcc fail, %d\n", __func__, error);
		goto err;
	}
	error = ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_OFF, 0);	/* 0, no delay */
	if (error) {
		TS_LOG_ERR("%s, power off iovdd fail, %d\n", __func__, error);
		goto err;
	}

err:
	return;
}
static int atmel_power_release(void)
{
	int ret = 0;

	ret = ts_kit_power_supply_put(TS_KIT_IOVDD);
	if (ret) {
		TS_LOG_ERR("%s, put iovdd supply fail, %d\n", __func__, ret);
		goto err;
	}
	ret = ts_kit_power_supply_put(TS_KIT_VCC);
	if (ret) {
		TS_LOG_ERR("%s, put vcc supply fail, %d\n", __func__, ret);
		goto err;
	}
	return 0;
err:
	return ret;
}

static int atmel_power_init(void)
{
	int ret = 0;

	ret = ts_kit_power_supply_get(TS_KIT_IOVDD);
	if (ret) {
		TS_LOG_ERR("%s, get iovdd supply fail, %d\n", __func__, ret);
		return ret;
	}
	ret = ts_kit_power_supply_get(TS_KIT_VCC);
	if (ret) {
		TS_LOG_ERR("%s, get vcc supply fail, %d\n", __func__, ret);
		goto err;
	}
	return 0;

err:
	ts_kit_power_supply_put(TS_KIT_IOVDD);
	return ret;
}

static int atmel_power_on(struct mxt_data *data)
{
	int error = 0;

	if(NULL == data || data->atmel_chip_data == NULL || data->atmel_chip_data->ts_platform_data == NULL) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		error = -EINVAL;
		return error;
	}
	TS_LOG_INFO("atmel_power_on called\n");
	gpio_direction_output(data->atmel_chip_data->ts_platform_data->reset_gpio, 0);

	error = atmel_pinctl_get(data);
	if (error < 0) {
		TS_LOG_ERR("%s,failed to set pinctl\n", __func__);
		goto error;
	}

	error = atmel_pinctl_select_normal(data);
	if (error < 0) {
		TS_LOG_ERR("%s,set iomux normal error, %d\n", __func__,error);
		goto pinctl_select_error;
	}
	error = ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_ON, 1);	/* power on iovdd, delay 1ms */
	if (error) {
		TS_LOG_ERR("%s, power on iovdd fail, %d\n", __func__, error);
		goto pinctl_get_err;
	}
	error = ts_kit_power_supply_ctrl(TS_KIT_VCC, TS_KIT_POWER_ON, 10);  /* power on vcc, delay 10ms */
	if (error) {
		TS_LOG_ERR("%s, power on vcc fail, %d\n", __func__, error);
		goto power_on_err;
	}

	gpio_direction_output(data->atmel_chip_data->ts_platform_data->reset_gpio, 1);
	error = gpio_direction_input(data->atmel_chip_data->ts_platform_data->irq_gpio);
	if (error) {
		TS_LOG_ERR("set gpio to input status error!\n");
		goto error;
	}

	return 0;
power_on_err:
	ts_kit_power_supply_ctrl(TS_KIT_IOVDD, TS_KIT_POWER_OFF, 0);	/* 0,no delay */
pinctl_get_err:
	atmel_pinctl_select_idle(data);
pinctl_select_error:
error:

	return error;
}

int atmel_probe_info_block(struct mxt_data *data)
{
	u8 val = 0;

	return __atmel_read_reg(data, 0, 1, &val);	/* 0,reg_addr; 1,read one byte */
}

int atmel_bootloader_read(struct mxt_data *data, u8 *val, unsigned int count)
{
	return __atmel_read_reg_ext(data, data->bootloader_addr, 0, count, val,
				  I2C_ACCESS_NO_REG);
}

int atmel_bootloader_write(struct mxt_data *data, const u8 * const val,
			 unsigned int count)
{
	return __atmel_write_reg_ext(data, data->bootloader_addr, 0, count, val,
				   I2C_ACCESS_NO_REG);
}

/*
 *Function for getting bootloader address
 *
 *Bootloader mode is the mode under which FW updating happens.
 *The address for bootloader is different for different mxt devices.
 *
 */
int atmel_lookup_bootloader_address(struct mxt_data *data, bool retry)
{
	u8 appmode = 0;
	u8 bootloader = 0;
	u8 family_id = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (data->info)
		family_id = data->info->family_id;

	appmode = data->addr & 0x7F;	/*0x7F, bit_0~bit_7 is valid */
	/*
	0x4a, 0x4b,0x4c,0x4d,0x5a,0x5b  atmel tp ic  i2c adress
	*/
	switch (appmode) {
	case 0x4a:  //normal mode i2c address
	case 0x4b:
		/* Chips after 1664S use different scheme */
		if (retry || family_id >= MXT_FAMILY_ID) {
			bootloader = appmode - NEW_IC_ADDRESS_DELTA;
			break;
		}
		/* Fall through for normal case */
	case 0x4c:
	case 0x4d:
	case 0x5a:
	case 0x5b:
		bootloader = appmode - OLD_IC_ADDRESS_DELTA;
		break;
	default:
		TS_LOG_ERR("Appmode i2c address 0x%02x not found\n", appmode);
		return -EINVAL;
	}

	data->bootloader_addr = bootloader;

	TS_LOG_INFO("Appmode i2c address 0x%02x, bootloader 0x%02x\n", appmode,
		    bootloader);

	return 0;
}

/*
 *Function for detecting if the device is under bootloader mode
 *
 *Bootloader mode is the mode under which FW updating happens.
 *
 */
static int mxt_probe_bootloader(struct mxt_data *data, bool retry)
{
	int ret = 0;
	u8 val = 0;
	bool crc_failure = false;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	ret = atmel_lookup_bootloader_address(data, retry);
	if (ret) {
		TS_LOG_ERR("%s, lookup_bootloader_address fail", __func__);
		return ret;
	}
	ret = atmel_bootloader_read(data, &val, 1);
	if (ret) {
		TS_LOG_ERR("%s,  atmel_bootloader_read failed", __func__);
		return ret;
	}

	/* Check app crc fail mode */
	crc_failure = (val & ~MXT_BOOT_STATUS_MASK) == MXT_APP_CRC_FAIL;

	TS_LOG_ERR("Detected bootloader, status:%02X%s\n",
		   val, crc_failure ? ", APP_CRC_FAIL" : "");

	return 0;
}

/*
 *Function for unlocking bootloader mode.
 *
 *Bootloader mode is the mode under which FW updating happens.
 *
 */
int atmel_send_bootloader_cmd(struct mxt_data *data, bool unlock)
{
	int ret = 0;
	u8 buf[MXT_BOOTLOADER_CMD_LEN] = {0};
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	if (unlock) {
		buf[0] = MXT_UNLOCK_CMD_LSB;
		buf[1] = MXT_UNLOCK_CMD_MSB;
	} else {
		buf[0] = 0x01;	// lock cmd lsb
		buf[1] = 0x01;	// lock cmd msb
	}

	ret = atmel_bootloader_write(data, buf, MXT_BOOTLOADER_CMD_LEN);
	if (ret) {
		TS_LOG_ERR("%s,failed to write under bootloader mode\n",
			   __func__);
		return ret;
	}

	return 0;
}

/*
 *Function for initializing information table.
 *
 *Information table holds the overall constructure of mxt device objects.
 *
 */
static int mxt_pre_initialize(struct mxt_data *data)
{
	int error = NO_ERR;
	bool alt_bootloader_addr = false;
	bool retry = false;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
retry_info:
	error = atmel_probe_info_block(data);
	if (error) {
retry_bootloader:
		error = mxt_probe_bootloader(data, alt_bootloader_addr);
		if (error) {
			if (alt_bootloader_addr) {
				/* Chip is not in appmode or bootloader mode */
				TS_LOG_ERR
				    ("%s,failed to probe bootloader,alt_bootloader_addr = %d\n",
				     __func__, alt_bootloader_addr);
				return error;
			}

			TS_LOG_INFO("%s,Trying alternate bootloader address\n",
				    __func__);
			alt_bootloader_addr = true;
			goto retry_bootloader;
		} else {
			if (retry) {
				TS_LOG_INFO("Could not recover device from "
					    "bootloader mode\n");
				/* this is not an error state, we can reflash
				 * from here */
				return 0;
			}

			/* Attempt to exit bootloader into app mode */
			atmel_send_bootloader_cmd(data, false);
			msleep(250);  //ic need delay
			retry = true;
			goto retry_info;
		}
	}

	return error;
}

static int atmel_chip_detect(struct ts_kit_platform_data *platform_data)
{
	struct mxt_data *data = mxt_core_data;
	int error = 0;

	TS_LOG_INFO("atmel chip detect called\n");

	if (!platform_data  || !(platform_data->ts_dev)
		|| !(platform_data->client)
		|| !data || !(data->atmel_chip_data)) {
		TS_LOG_ERR("platform_data is NULL \n");
		return -ENOMEM;
	}
	data->atmel_dev = platform_data->ts_dev;  //data
	data->sleep_mode = POWER_DOWN_MODE;
	data->atmel_dev->dev.of_node = data->atmel_chip_data->cnode;
	data->atmel_chip_data->easy_wakeup_info.sleep_mode = TS_POWER_OFF_MODE;
	data->atmel_chip_data->ts_platform_data = platform_data;
	data->atmel_chip_data->is_in_cell = false;
	data->atmel_chip_data->is_i2c_one_byte = 0;
	data->atmel_chip_data->is_new_oem_structure= 0;
	data->dev = &(platform_data->client->dev);
	memset(data->module_id, '\0', sizeof(data->module_id));

	mutex_init(&data->bus_access_mutex);
	mutex_init(&data->access_mutex);
	mutex_init(&data->debug_msg_lock);
	data->use_regulator = true;
	data->in_bootloader = false;

	error= atmel_parse_dts(data->atmel_dev->dev.of_node, data->atmel_chip_data);
	if (error) {
		TS_LOG_ERR("atmel_parse_dts failed\n");
		goto err_free_object;
	}
	TS_LOG_INFO("atmel_parse_dts success\n");

	error = atmel_power_init();
	if (error) {
		TS_LOG_ERR("atmel_power_init failed\n");
		goto err_free_object;
	}

	error = atmel_power_on(data);
	if (error) {
		TS_LOG_ERR("atmel_power_on failed\n");
		goto err_free_regulator;
	}

	mdelay(100);		/*after hw reset, need  > 88ms.*/
	error = mxt_pre_initialize(data);
	if (error) {
		TS_LOG_ERR("Failed pre initialize chip\n");
		goto err_pre_initialize;
	}

	TS_LOG_INFO("mxt_pre_initialize success\n");

	error = atmel_mem_access_init(data);
	if (error) {
		TS_LOG_ERR("%s,access init failed\n", __func__);
		goto err_pre_initialize;
	}
	return NO_ERR;
err_pre_initialize:
	atmel_pinctl_select_idle(data);
	atmel_power_off(data);
err_free_regulator:
	atmel_power_release();
err_free_object:
	if (data) {
		mutex_destroy(&data->bus_access_mutex);
		mutex_destroy(&data->debug_msg_lock);
		mutex_destroy(&data->access_mutex);
	}
	if(mxt_core_data && mxt_core_data->atmel_chip_data) {
		kfree(mxt_core_data->atmel_chip_data);
		mxt_core_data->atmel_chip_data = NULL;/*Fix the DTS parse error cause panic bug*/
	}

	if (mxt_core_data) {
		kfree(mxt_core_data);
		mxt_core_data = NULL;
	}
	return error;
}

void atmel_free_input_device(struct mxt_data *data)
{
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	atmel_debug_msg_remove(data);
}

void atmel_free_object_table(struct mxt_data *data)
{
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	/*atmel_debug_msg_remove(data);*/
	data->object_table = NULL;
	if(NULL != data->info) {
		kfree(data->info);
		data->info = NULL;
	}

	if (NULL != data->msg_buf) {
		kfree(data->msg_buf);
	}
	data->msg_buf = NULL;

	atmel_free_input_device(data);

	data->enable_reporting = false;
	data->T5_address = 0;
	data->T5_msg_size = 0;
	data->T6_reportid = 0;
	data->T7_address = 0;
	data->T8_address = 0;
	data->T9_address = 0;
	data->T9_reportid_min = 0;
	data->T9_reportid_max = 0;
	data->T15_address = 0;
	data->T15_reportid_min = 0;
	data->T15_reportid_max = 0;
	data->T18_address = 0;
	data->T19_address = 0;
	data->T19_reportid = 0;
	data->T24_address = 0;
	data->T24_reportid = 0;
	data->T25_address = 0;
	data->T25_reportid = 0;
	data->T37_address = 0;
	data->T38_address = 0;
	data->T40_address = 0;
	data->T42_address = 0;
	data->T42_reportid_min = 0;
	data->T42_reportid_max = 0;
	data->T44_address = 0;
	data->T46_address = 0;
	data->T47_address = 0;
	data->T48_reportid = 0;
	data->T55_address = 0;
	data->T56_address = 0;
	data->T61_address = 0;
	data->T61_reportid_min = 0;
	data->T61_reportid_max = 0;
	data->T61_instances = 0;
	data->T63_reportid_min = 0;
	data->T63_reportid_max = 0;
	data->T65_address = 0;
	data->T68_address = 0;
	data->T71_address = 0;
	data->T72_address = 0;
	data->T72_reportid_min = 0;
	data->T72_reportid_max = 0;
	data->T78_address = 0;
	data->T80_address = 0;
	data->T81_address = 0;
	data->T81_reportid_min = 0;
	data->T81_reportid_max = 0;
	data->T92_address = 0;
	data->T92_reportid = 0;
	data->T93_address = 0;
	data->T93_reportid = 0;
	data->T96_address = 0;
	data->T97_address = 0;
	data->T97_reportid_min = 0;
	data->T97_reportid_max = 0;
	data->T99_address = 0;
	data->T99_reportid = 0;
	data->T100_address = 0;
	data->T100_reportid_min = 0;
	data->T100_reportid_max = 0;
	data->T102_address = 0;
	data->T102_reportid = 0;
	data->T104_address = 0;
	data->T113_address = 0;
	data->T115_address = 0;
	data->T115_reportid = 0;
	data->T125_reportid = 0;
	data->max_reportid = 0;
}

int mxt_obj_instances(const struct mxt_object *obj)
{
	if(!obj) {
		TS_LOG_ERR("%s , param invalid\n", __func__);
		return -EINVAL;
	}
	return obj->instances_minus_one + 1;	/* instance read from IC is the actual instance minus 1*/
}

int mxt_obj_size(const struct mxt_object *obj)
{
	if(!obj) {
		TS_LOG_ERR("%s , param invalid\n", __func__);
		return -EINVAL;
	}
	return obj->size_minus_one + 1;		/* instance read from IC is the actual instance minus 1*/
}


/*
 *Function for reading object instance
 *
 *Object is the function unit of maxTouch devices. One object might have many instances.
 *
 */
static int mxt_read_obj_instance(struct mxt_data *data, u8 type, u8 instance,
				 u8 offset, u8 *val)
{
	struct mxt_object *object = NULL;
	u16 reg = 0;
	if(!data || !val){
		TS_LOG_ERR("%s, param invalid\n", __func__);
		goto out;
	}
	object = atmel_get_object(data, type);
	if (!object) {
		TS_LOG_ERR("%s:object=%ld\n", __func__, PTR_ERR(object));
		goto out;
	}

	if (offset >= mxt_obj_size(object)
	    || instance >= mxt_obj_instances(object)) {
		TS_LOG_ERR
		    ("%s, offset: %d, object size: %d, instance: %d, obj instance: %d\n",
		     __func__, offset, mxt_obj_size(object), instance,
		     mxt_obj_instances(object));
		goto out;
	}

	reg = object->start_address + instance * mxt_obj_size(object) + offset;
	return __atmel_read_reg(data, reg, 1, val);

out:
	return -EINVAL;
}

static int mxt_read_object(struct mxt_data *data, u8 type, u8 offset, u8 *val)
{
	return mxt_read_obj_instance(data, type, 0, offset, val);
}

/*
 *Function for write object instance
 *
 *Object is the function unit of maxTouch devices. One object might have many instances.
 *
 */
static int mxt_write_obj_instance(struct mxt_data *data, u8 type, u8 instance,
				  u8 offset, u8 val)
{
	struct mxt_object *object = NULL;
	u16 reg = 0;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	object = atmel_get_object(data, type);
	if (!object) {
		TS_LOG_ERR("offset is null");
		return -EINVAL;
	} else if (offset >= mxt_obj_size(object) ||
	    instance >= mxt_obj_instances(object)) {
		TS_LOG_ERR
		    ("offset=%d mxt_obj_size(object)=%d instance=%d mxt_obj_instances(object)=%d\n",
		     offset, mxt_obj_size(object), instance,
		     mxt_obj_instances(object));
		return -EINVAL;
	}

	reg = object->start_address + instance * mxt_obj_size(object) + offset;
	return __atmel_write_reg(data, reg, 1, &val);
}

static int mxt_write_object(struct mxt_data *data, u8 type, u8 offset, u8 val)
{
	return mxt_write_obj_instance(data, type, 0, offset, val);
}

#if defined (CONFIG_TEE_TUI)
static void mxt_get_tui_data(struct mxt_data *data)
{
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (data->atmel_chip_data) {
		tui_mxt_data.max_reportid = data->max_reportid;
		tui_mxt_data.T100_address = data->T100_address;
		tui_mxt_data.T100_reportid_max = data->T100_reportid_max;
		tui_mxt_data.T100_reportid_min = data->T100_reportid_min;
		tui_mxt_data.T44_address = data->T44_address;
		tui_mxt_data.T5_address = data->T5_address;
		tui_mxt_data.T5_msg_size = data->T5_msg_size;
		tui_mxt_data.addr = data->addr;
		strncpy(tui_mxt_data.device_name, "atmel", strlen("atmel"));
		tui_mxt_data.device_name[strlen("atmel") + 1] = '\0';

		data->atmel_chip_data->tui_data = &tui_mxt_data;
	}
	TS_LOG_INFO
	    ("max_reportid:0x%x,T100_address:0x%x,T100_reportid_max:0x%x,T100_reportid_min:0x%x,T44_address:0x%x,T5_address:0x%x,T5_msg_size:0x%x,addr:0x%x,name:%s\n",
	     tui_mxt_data.max_reportid, tui_mxt_data.T100_address,
	     tui_mxt_data.T100_reportid_max, tui_mxt_data.T100_reportid_min,
	     tui_mxt_data.T44_address, tui_mxt_data.T5_address,
	     tui_mxt_data.T5_msg_size, tui_mxt_data.addr,
	     tui_mxt_data.device_name);
}
#endif
static void mxt_parse_each_object_table(struct mxt_object * object, u8 min_id, u8 max_id)
{
	struct mxt_data *data = mxt_core_data;
	if(!data || !data->info || !object) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	switch (object->type) {
			case MXT_GEN_MESSAGEPROCESSOR_T5:
				if (data->info->family_id == MXT_MXT224_FAMILY_ID) {
					/* On mXT224 read and discard unused CRC byte
					 * otherwise DMA reads are misaligned */
					data->T5_msg_size = mxt_obj_size(object);
				} else {
					/* CRC not enabled, so skip last byte */
					data->T5_msg_size = mxt_obj_size(object) - 1;
				}
				data->T5_address = object->start_address;
				break;
			case MXT_GEN_COMMANDPROCESSOR_T6:
				data->T6_reportid = min_id;
				data->T6_address = object->start_address;
				break;
			case MXT_GEN_POWERCONFIG_T7:
				data->T7_address = object->start_address;
				break;
			case MXT_GEN_ACQUISITIONCONFIG_T8:
				data->T8_address = object->start_address;
				break;
			case MXT_TOUCH_MULTITOUCHSCREEN_T9:
				/* Only handle messages from first T9 instance */
				data->T9_reportid_min = min_id;
				data->T9_reportid_max = min_id +
					object->num_report_ids - 1;
				data->T9_address = object->start_address;
				data->num_touchids = object->num_report_ids;
				break;
			case MXT_TOUCH_KEYARRAY_T15:
				data->T15_reportid_min = min_id;
				data->T15_reportid_max = max_id;
				data->T15_address = object->start_address;
				break;
			case MXT_SPT_COMCONFIG_T18:
				data->T18_address = object->start_address;
				break;
			case MXT_SPT_GPIOPWM_T19:
				data->T19_address = object->start_address;
				data->T19_reportid = min_id;
				break;
			case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
				data->T24_address = object->start_address;
				data->T24_reportid = min_id;
				break;
			case MXT_SPT_SELFTEST_T25:
				data->T25_address = object->start_address;
				data->T25_reportid = min_id;
				break;
			case MXT_DEBUG_DIAGNOSTIC_T37:
				data->T37_address = object->start_address;
				break;
			case MXT_SPT_USERDATA_T38:
				data->T38_address = object->start_address;
				break;
			case MXT_PROCI_GRIPSUPPRESSION_T40:
				data->T40_address = object->start_address;
				break;
			case MXT_PROCI_TOUCHSUPPRESSION_T42:
				data->T42_address = object->start_address;
				data->T42_reportid_min = min_id;
				data->T42_reportid_max = max_id;
				break;
			case MXT_SPARE_T44:
				data->T44_address = object->start_address;
				break;
			case MXT_SPT_CTECONFIG_T46:
				data->T46_address = object->start_address;
				break;
			case MXT_PROCI_STYLUS_T47:
				data->T47_address = object->start_address;
				break;
			case MXT_PROCG_NOISESUPPRESSION_T48:
				data->T48_reportid = min_id;
				break;
			case MXT_ADAPTIVE_T55:
				data->T55_address = object->start_address;
				break;
			case MXT_PROCI_SHIELDLESS_T56:
				data->T56_address = object->start_address;
				break;
			case MXT_SPT_TIMER_T61:
				/* Only handle messages from first T63 instance */
				data->T61_address = object->start_address;
				data->T61_reportid_min = min_id;
				data->T61_reportid_max = max_id;
				data->T61_instances = mxt_obj_instances(object);
				break;
			case MXT_PROCI_ACTIVESTYLUS_T63:
				/* Only handle messages from first T63 instance */
				data->T63_reportid_min = min_id;
				data->T63_reportid_max = min_id;
				data->num_stylusids = 1;
				break;
			case MXT_PROCI_LENSBENDING_T65:
				data->T65_address = object->start_address;
				break;
			case MXT_SPARE_T68:
				data->T68_address = object->start_address;
				data->T68_reportid_min = min_id;
				data->T68_reportid_max = max_id;
				break;
			case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
				data->T70_address = object->start_address;
				data->T70_reportid_min = min_id;
				data->T70_reportid_max = max_id;
				break;
			case MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71:
				data->T71_address = object->start_address;
				break;
			case MXT_PROCG_NOISESUPPRESSION_T72:
				data->T72_address = object->start_address;
				data->T72_reportid_min = min_id;
				data->T72_reportid_max = max_id;
				break;
			case MXT_PROCI_GLOVEDETECTION_T78:
				data->T78_address = object->start_address;
				break;
			case MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80:
				data->T80_address = object->start_address;
				data->T80_reportid = min_id;
				break;
			case MXT_PROCI_UNLOCKGESTURE_T81:
				data->T81_address = object->start_address;
				data->T81_reportid_min = min_id;
				data->T81_reportid_max = max_id;
				break;
			case MXT_PROCI_GESTURE_T92:
				data->T92_address = object->start_address;
				data->T92_reportid = min_id;
				break;
			case MXT_PROCI_TOUCHSEQUENCELOGGER_T93:
				data->T93_address = object->start_address;
				data->T93_reportid = min_id;
				break;
			case MXT_TOUCH_SPT_PTC_TUNINGPARAMS_T96:
				data->T96_address = object->start_address;
				break;
			case MXT_TOUCH_PTC_KEYS_T97:
				data->T97_reportid_min = min_id;
				data->T97_reportid_max = max_id;
				data->T97_address = object->start_address;
				break;
			case MXT_PROCI_KEYGESTUREPROCESSOR_T99:
				data->T99_address = object->start_address;
				data->T99_reportid = min_id;
				break;
			case MXT_TOUCH_MULTITOUCHSCREEN_T100:
				data->T100_reportid_min = min_id;
				data->T100_reportid_max = max_id;
				data->T100_address = object->start_address;
				/* first two report IDs reserved */
				data->num_touchids = object->num_report_ids - 2;
				break;
			case MXT_SPT_SELFCAPHOVERCTECONFIG_T102:
				data->T102_address = object->start_address;
				data->T102_reportid = min_id;
				break;
			case MXT_PROCI_AUXTOUCHCONFIG_T104:
				data->T104_address = object->start_address;
				break;
			case MXT_SPT_SELFCAPMEASURECONFIG_T113:
				data->T113_address = object->start_address;
				break;
			case MXT_PROCI_SYMBOLGESTURE_T115:
				data->T115_address = object->start_address;
				data->T115_reportid = min_id;
				break;
			case MXT_SPT_SMARTSCAN_T124:
				data->T124_address = object->start_address;
				data->T124_reportid = min_id;
				break;
			case MXT_PROCI_PEAKREGIONDATA_T125:
				data->T125_address = object->start_address;
				data->T125_reportid = min_id;
				break;
		}
}

/*
 *Function for saving the object information to the mxt-data
 *
 *object information includes the report id ranges and object address.
 *
 */
static int mxt_parse_object_table(struct mxt_data *data,
				  struct mxt_object *object_table)
{
	int i = 0;
	u8 reportid = 0;
	u16 end_address= 0;
	struct mxt_object *object = NULL;
	u8 min_id = 0, max_id = 0;

	if(!data || !data->info || !object_table) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/* Valid Report IDs start counting from 1 */
	reportid = 1;
	data->mem_size = 0;
	for (i = 0; i < data->info->object_num; i++) {
		object = object_table + i;

		le16_to_cpus(&object->start_address);

		if (object->num_report_ids) {
			min_id = reportid;
			reportid += object->num_report_ids *
			    mxt_obj_instances(object);
			max_id = reportid - 1;	/*min reportid is 1*/
		} else {
			min_id = 0;
			max_id = 0;
		}

		TS_LOG_DEBUG
		    ("T%u Start:%u Size:%u Instances:%u Report IDs:%u-%u\n",
		     object->type, object->start_address, mxt_obj_size(object),
		     mxt_obj_instances(object), min_id, max_id);

		mxt_parse_each_object_table(object, min_id, max_id);

		end_address = object->start_address
		    + mxt_obj_size(object) * mxt_obj_instances(object) - 1;

		if (end_address >= data->mem_size)
			data->mem_size = end_address + 1;
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;

	/* If T44 exists, T5 position has to be directly after, t5_addr = t44_addr + 1 */
	if (data->T44_address && (data->T5_address != data->T44_address + 1)) {
		TS_LOG_ERR("Invalid T44 position\n");
		return -EINVAL;
	}

	data->msg_buf = kcalloc(data->max_reportid,
				data->T5_msg_size, GFP_KERNEL);
	if (!data->msg_buf) {
		TS_LOG_ERR("Failed to allocate message buffer\n");
		return -ENOMEM;
	}

	return 0;
}

static void mxt_calc_crc24(u32 *crc, u8 firstbyte, u8 secondbyte)
{
	static const unsigned int crcpoly = MXT_CRCPOLY;
	u32 result = 0;
	u32 data_word= 0;

	data_word = (secondbyte << 8) | firstbyte;
	result = ((*crc << 1) ^ data_word);

	if (result & MXT_CRC_CHECK)
		result ^= crcpoly;

	*crc = result;
}

u32 atmel_calculate_crc(u8 *base, off_t start_off, off_t end_off)
{
	u32 crc = 0;
	u8 *ptr = base + start_off;
	u8 *last_val = base + end_off - 1;
	if(!base) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (end_off < start_off)
		return -EINVAL;

	while (ptr < last_val) {
		mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
		ptr += 2;
	}

	/* if len is odd, fill the last byte with 0 */
	if (ptr == last_val)
		mxt_calc_crc24(&crc, *ptr, 0);

	/* Mask to 24-bit */
	crc &= 0x00FFFFFF;

	return crc;
}

static int mxt_read_info_block(struct mxt_data *data)
{
	int error = 0;
	size_t size = 0;
	void *id_buf = NULL, *buf = NULL;
	uint8_t num_objects = 0;
	u32 calculated_crc = 0;
	u8 *crc_ptr = NULL;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/* If info block already allocated, free it */
	if (data->info != NULL)
		atmel_free_object_table(data);

	/* Read 7-byte ID information block starting at address 0 */
	size = sizeof(struct mxt_info);
	id_buf = kzalloc(size, GFP_KERNEL);
	if (!id_buf) {
		TS_LOG_ERR("Failed to allocate memory\n");
		return -ENOMEM;
	}

	error = __atmel_read_reg(data, 0, size, id_buf);
	if (error) {
		TS_LOG_ERR("%s, Failed to get id info\n", __func__);
		kfree(id_buf);
		return error;
	}

	/* Resize buffer to give space for rest of info block */
	num_objects = ((struct mxt_info *)id_buf)->object_num;
	size += (num_objects * sizeof(struct mxt_object))
	    + MXT_INFO_CHECKSUM_SIZE;

	buf = krealloc(id_buf, size, GFP_KERNEL);
	if (!buf) {
		TS_LOG_ERR("Failed to allocate memory\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	/* Read rest of info block */
	error = __atmel_read_reg(data, MXT_OBJECT_START,
			       size - MXT_OBJECT_START, buf + MXT_OBJECT_START);
	if (error) {
		TS_LOG_ERR("%s, Failed to read object info\n", __func__);
		goto err_free_mem;
	}
	/* Extract & calculate checksum */
	crc_ptr = buf + size - MXT_INFO_CHECKSUM_SIZE;
	data->info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

	calculated_crc = atmel_calculate_crc(buf, 0,
					   size - MXT_INFO_CHECKSUM_SIZE);

	/* CRC mismatch can be caused by data corruption due to I2C comms
	 * issue or else device is not using Object Based Protocol */
	if ((data->info_crc == 0) || (data->info_crc != calculated_crc)) {
		TS_LOG_INFO
		    ("Info Block CRC error calculated=0x%06X read=0x%06X\n",
		     calculated_crc, data->info_crc);

		TS_LOG_DEBUG("info block size %d\n", (int)size);
		print_hex_dump(KERN_ERR, "[mxt] INFO:", DUMP_PREFIX_NONE, 16, 1,		/* 16,rawsize; 1,groupsize */
			       buf, size, false);

		error = -EIO;
		goto err_free_mem;
	}

	/* Save pointers in device data structure */
	data->info = (struct mxt_info *)buf;

	TS_LOG_INFO
	    ("Family: %u Variant: %u Firmware V%u.%u.%02X Objects: %u. matrix_xsize:%u. matrix_ysize:%u\n",
	     data->info->family_id, data->info->variant_id,
	     data->info->version >> 4, data->info->version & 0xf,		/*print High 4 bit, Low 4 bit separately*/
	     data->info->build, data->info->object_num,
	     data->info->matrix_xsize, data->info->matrix_ysize);

	/* Parse object table information */
	error = mxt_parse_object_table(data, (struct mxt_object *)(buf + MXT_OBJECT_START));
	if (error) {
		TS_LOG_ERR("Error %d parsing object table\n", error);
		atmel_free_object_table(data);
		return error;
	}

	TS_LOG_DEBUG("T5 message size %d\n", data->T5_msg_size);

	data->object_table = (struct mxt_object *)(buf + MXT_OBJECT_START);
	return 0;

err_free_mem:
	kfree(buf);
	return error;
}

static int board_por_reset(struct mxt_data *data)
{
	int error = 0;
	/*write your hw reset here, and return 0*/
	/*if no, returen -EIO*/
	if(data == NULL || data->atmel_chip_data == NULL
		|| data->atmel_chip_data->ts_platform_data == NULL) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EIO;
	}

	atmel_power_off(data);
	msleep(10);//wait 10ms
	error = atmel_power_on(data);
	if(error) {
		TS_LOG_ERR("%s, power on failed\n", __func__);
		goto exit;
	}

	msleep(100);//after power on need wait 100ms

	TS_LOG_INFO("%s, reset success\n", __func__);
	return 0;
exit:
	return error;
}

/*
 *Function for sending t6 command
 *
 *t6 is used to control mxt devices in varies ways such as resetting, backup, and calibrate.
 *
 */
int atmel_t6_command(struct mxt_data *data, u16 cmd_offset, u8 value, bool wait)
{
	u16 reg = 0;
	u8 command_register = 0;
	int timeout_counter = 0;
	int ret = 0;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	reg = data->T6_address + cmd_offset;

	ret = atmel_write_reg(data, reg, value);
	if (ret) {
		TS_LOG_ERR("%s: write %u error\n", __func__, reg);
		return ret;
	}
	TS_LOG_INFO("Set T6  to mode %d!\n", value);

	if (!wait) {
		TS_LOG_INFO("%s:wait = %d\n", __func__, wait);
		return 0;
	}
	do {
		msleep(14);//ic need
		ret = __atmel_read_reg(data, reg, 1, &command_register);
		if (ret) {
			TS_LOG_INFO("%s: timeout_counter=%d\n", __func__,
				    timeout_counter);
			return ret;
		}
	} while ((command_register != 0) && (timeout_counter++ <= MAX_TIMEOUT_TIMES));//timeout times

	if (timeout_counter > MAX_TIMEOUT_TIMES) {
		TS_LOG_ERR("Command failed!\n");
		return -EIO;
	}

	return 0;
}

int atmel_soft_reset(struct mxt_data *data)
{
	int error = 0;
	TS_LOG_INFO("Resetting chip\n");
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	atmel_status_resume();
	error = atmel_t6_command(data, MXT_COMMAND_REPORTALL, 1, false);
	if(error) {
		TS_LOG_ERR("%s, atmel_t6_command failed\n", __func__);
		return error;
	}
	return 0;
}


static int mxt_set_t7_active2ide_timeout_cfg(struct mxt_data *data, u8 timeout) // timeout in step of 200ms , eg, 50 means 10 seconds
{
	int error = NO_ERR;
	if(!data) {
		TS_LOG_ERR("%s, param is invalid\n", __func__);
		return -EINVAL;
	}
	error = __atmel_write_reg(data, data->T7_address+ T7_ACT2IDLETO_OFFSET, 1, &timeout);
	if (error) {
		TS_LOG_ERR("%s, set active to idle time failed,error = %d\n", __func__, error);
		return error;
	}
	TS_LOG_INFO("%s, set active to idle time success\n", __func__);
	return NO_ERR;
}

/*
 *Function for configuring t7
 *
 *t7 is used to config the mxt power consumption via setting the scan speed under active or inactive mode.
 *
 */
static int mxt_set_t7_power_cfg(struct mxt_data *data, u8 sleep)
{
	int error = 0;
	struct t7_config *new_config = NULL;
	struct t7_config deepsleep = {.active = 0, .idle = 0 };

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (sleep == MXT_POWER_CFG_DEEPSLEEP)
		new_config = &deepsleep;
	else if(sleep == MXT_POWER_CFG_DOZE) {
		new_config = &data->t7_cfg_doze;
	} else
		new_config = &data->t7_cfg;

	error = __atmel_write_reg(data, data->T7_address,
				sizeof(data->t7_cfg), new_config);
	if (error) {
		TS_LOG_ERR("%s, set t7_power_cfg fail\n", __func__);
		return error;
	}

	TS_LOG_INFO("Set T7 ACTV:%d IDLE:%d\n", new_config->active,
		     new_config->idle);

	msleep(10);//wait write ic finish

	return 0;
}

/*
 *Function for reading out t7 config when booting up.
 *
 *t7 is used to config the mxt power consumption via setting the scan speed under active or inactive mode.
 *
 */
int atmel_init_t7_power_cfg(struct mxt_data *data)
{
	int error = 0;
	bool retry = false;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

recheck:
	error = __atmel_read_reg(data, data->T7_address,
			       sizeof(data->t7_cfg), &data->t7_cfg);
	if (error) {
		TS_LOG_ERR("%s, __atmel_read_reg failed\n", __func__);
		return error;
	}

	if (data->t7_cfg.active == 0 || data->t7_cfg.idle == 0) {
		if (!retry) {
			TS_LOG_DEBUG("T7 cfg zero, resetting\n");
			error = atmel_soft_reset(data);
			if(error) {
				TS_LOG_ERR("%s, atmel_soft_reset failed\n", __func__);
				return error;
			}
			retry = true;
			goto recheck;
		} else {
			TS_LOG_DEBUG("T7 cfg zero after reset, overriding\n");
			data->t7_cfg.active = MXT_SCAN_ONCE_TIME; //how long to scan once
			data->t7_cfg.idle = MXT_NORMAL_MODE_WAIT_SCAN_ONCE_TIME;//how long to wait for a scan
			data->t7_cfg_doze.active = data->t7_cfg.active;
			data->t7_cfg_doze.idle = MXT_DOZE_MODE_WAIT_SCAN_ONCE_TIME;//max 255
			return mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);
		}
	} else {
		data->t7_cfg_doze.active = data->t7_cfg.active;
		data->t7_cfg_doze.idle = MXT_DOZE_MODE_WAIT_SCAN_ONCE_TIME;
		TS_LOG_DEBUG("Initialised power cfg: ACTV %d, IDLE %d\n",
			     data->t7_cfg.active, data->t7_cfg.idle);
		return 0;
	}
}

struct mxt_object *atmel_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object = NULL;
	int i = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		goto out;
	}
	if (!data->object_table) {
		TS_LOG_ERR("%s:data->object_table=%ld\n", __func__,
			   PTR_ERR(data->object_table));
		goto out;
	}

	for (i = 0; i < data->info->object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

out:
	TS_LOG_ERR("Invalid object type T%u\n", type);
	return NULL;
}

/*
 *Function for reading out t100 config.
 *
 *t100 is the main touch object which configs the touch performance, including the resolution, touth threshold and so on.
 *
 */
static int mxt_read_t100_config(struct mxt_data *data)
{
	int error = 0;
	struct mxt_object *object = NULL;
	u16 range_x = 0, range_y = 0;
	u8 x_size = 0, x_origin = 0, y_size = 0, y_origin = 0;

	if(!data || !data->atmel_chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	object = atmel_get_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T100);
	if (!object) {
		TS_LOG_ERR("get MXT_TOUCH_MULTITOUCHSCREEN_T100 failed\n");
		return -EINVAL;
	}

	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_XRANGE,
			       sizeof(range_x), &range_x);
	if (error) {
		TS_LOG_ERR("%s:read MXT_T100_XRANGE failed\n", __func__);
		return error;
	}

	le16_to_cpus(&range_x);

	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_YRANGE,
			       sizeof(range_y), &range_y);
	if (error) {
		TS_LOG_ERR("read MXT_T100_YRANGE failed\n");
		return error;
	}

	le16_to_cpus(&range_y);

	error = __atmel_read_reg(data,
			       object->start_address,
			       sizeof(data->tchcfg), &data->tchcfg);
	if (error) {
		TS_LOG_ERR("read data->tchcfg failed\n");
		return error;
	}

	/* Handle default values */
	if (range_x == 0)
		range_x = data->atmel_chip_data->x_max - 1;//use dts config

	/* Handle default values */
	if (range_y == 0)
		range_y = data->atmel_chip_data->y_max - 1;//use dts config

	if (test_flag_8bit(MXT_T100_CFG_SWITCHXY, &data->tchcfg[MXT_T100_CFG1])) {
		data->max_x = range_y;
		data->max_y = range_x;
	} else {
		data->max_x = range_x;
		data->max_y = range_y;
	}

/*read x size and y size*/
	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_XSIZE,
			       sizeof(x_size), &x_size);
	if (error) {
		TS_LOG_ERR("read MXT_T100_XSIZE failed\n");
		return error;
	}

	le16_to_cpus(&x_size);

	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_YSIZE,
			       sizeof(y_size), &y_size);
	if (error) {
		TS_LOG_ERR("read MXT_T100_YSIZE failed\n");
		return error;
	}

	le16_to_cpus(&y_size);

/*read x origin and y origin*/
	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_XORIGIN,
			       sizeof(x_origin), &x_origin);
	if (error) {
		TS_LOG_ERR("read MXT_T100_XORIGIN failed\n");
		return error;
	}

	le16_to_cpus(&x_origin);

	error = __atmel_read_reg(data,
			       object->start_address + MXT_T100_YORIGIN,
			       sizeof(y_origin), &y_origin);
	if (error) {
		TS_LOG_ERR("read MXT_T100_YORIGIN failed\n");
		return error;
	}

	le16_to_cpus(&y_origin);
	data->x_size = x_size;
	data->y_size = y_size;
	data->x_origin = x_origin;
	data->y_origin = y_origin;
	/* allocate aux bytes */
	TS_LOG_INFO
	    ("T100 Touchscreen size X%uY%u, X_size%uY_size%u, X_origin%uY_origin%u\n",
	     data->max_x, data->max_y, data->x_size, data->y_size,
	     data->x_origin, data->y_origin);

	return 0;
}

static int mxt_initialize_t100_input_device(struct mxt_data *data)
{
	int error = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	error = mxt_read_t100_config(data);
	if (error)
		TS_LOG_ERR("Failed to initialize T100 resolution\n");

	return 0;
}

int atmel_configure_objects(struct mxt_data *data)
{
	int error = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	error = atmel_debug_msg_init(data);
	if (error) {
		TS_LOG_ERR("Failed to initialize debug msg\n");
		return error;
	}

	error = atmel_init_t7_power_cfg(data);
	if (error) {
		TS_LOG_ERR("Failed to initialize power cfg\n");
		return error;
	}

	if (data->T100_reportid_min) {
		error = mxt_initialize_t100_input_device(data);
		if (error)
			return error;
	} else {
		TS_LOG_ERR("No touch object detected\n");
	}

	data->enable_reporting = true;
	return 0;
}

static int mxt_initialize_info_block(struct mxt_data *data)
{
	int error = 0;
	bool alt_bootloader_addr = false;
	bool retry = false;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
retry_info:
	error = mxt_read_info_block(data);
	if (error) {
retry_bootloader:
		error = mxt_probe_bootloader(data, alt_bootloader_addr);
		if (error) {
			if (alt_bootloader_addr) {
				/* Chip is not in appmode or bootloader mode */
				TS_LOG_ERR
				    ("%s, Chip is not in appmode or bootloader mode",
				     __func__);
				return error;
			}

			TS_LOG_INFO("%s, Trying alternate bootloader address\n",
				    __func__);
			alt_bootloader_addr = true;
			goto retry_bootloader;
		} else {
			if (retry) {
				TS_LOG_ERR("Could not recover device from "
					   "bootloader mode\n");
				/* this is not an error state, we can reflash
				 * from here */
				data->in_bootloader = true;
				return 0;
			}

			/* Attempt to exit bootloader into app mode */
			board_por_reset(data);

			retry = true;
			goto retry_info;
		}
	}

	return error;
}

/*
 *Function for initializing touch device.
 *
 *First, it will initialize the information block, which contains the object information.
 *Second, it will initialize necessary object for the mxt touch device.
 *
 */
static int mxt_initialize(struct mxt_data *data)
{
	int error = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	error = mxt_initialize_info_block(data);
	if (error) {
		TS_LOG_ERR("%s, mxt_initialize_info_block fail", __func__);
		return error;
	}

	error = atmel_configure_objects(data);
	if (error) {
		TS_LOG_ERR("%s, atmel_configure_objects fail", __func__);
		return error;
	}
#if defined(CONFIG_MXT_FORCE_RESET_AT_POWERUP)
	TS_LOG_ERR("board force a reset after gpio init\n");
	error = atmel_soft_reset(data);
	if(error) {
		TS_LOG_ERR("%s, atmel_soft_reset fail", __func__);
		return error;
	}
#endif
#if defined (CONFIG_TEE_TUI)
	mxt_get_tui_data(data);
#endif
	return 0;
}

static void mxt_parse_module_dts(struct ts_kit_device_data *chip_data)
{
	struct device_node *device = NULL;
	int retval = 0, read_val = 0;

	if(!chip_data || !mxt_core_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	//ts_anti_false_touch_param_achieve(chip_data);
	device = of_find_compatible_node(NULL, NULL, mxt_core_data->module_id);
	if (!device) {
		TS_LOG_INFO("%s, No chip specific dts: %s, need to parse\n",
			    __func__, mxt_core_data->module_id);
		return;
	}

	TS_LOG_INFO("%s, parse specific dts: %s\n", __func__,
		    mxt_core_data->module_id);

	ts_parse_panel_specific_config(device, chip_data);	/* parse module config */

	retval =
	    of_property_read_u32(device, "t72_noise_level_threshold",
				 &read_val);
	if (!retval) {
		TS_LOG_INFO
		    ("get chip specific t72_noise_level_threshold = %d\n",
		     read_val);
		mxt_core_data->t72_noise_level_threshold = read_val;
	} else {
		mxt_core_data->t72_noise_level_threshold = DEFAULT_NOISE_LEVEL;
	}

	retval = of_property_read_u32(device, "roi_data_invert", &read_val);
	if (!retval) {
		TS_LOG_INFO("roi_data_invert = %d\n", read_val);
		mxt_core_data->roi_data_invert = (u8) read_val;
	} else {
		TS_LOG_INFO("can not get roi_data_invert value\n");
		mxt_core_data->roi_data_invert = 1;//need invert
	}

}

static void mxt_init_extra_data(void)
{
	if(!mxt_core_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	memset(&mxt_core_data->sta_cnt, 0, sizeof(struct status_count));
	memset(&mxt_core_data->t72_noise_level, 0, sizeof(struct t72_noise_level));
	memset(&mxt_core_data->feature_extra_data, 0,
	       sizeof(struct mxt_feature_extra_data));
	memset(&mxt_core_data->t25_msg, 0, sizeof(mxt_core_data->t25_msg));
	mxt_core_data->noise_state = MXT_T72_NOISE_SUPPRESSION_NO_DEF;
	mxt_core_data->mxt_cur_finger_number = 0;
	mxt_core_data->t72_state_change = T72_STATE_NOT_CHANGE;
}

static int atmel_init_chip(void)
{
	struct mxt_data *data =mxt_core_data;
	int error = 0;

	if (!data) {
		TS_LOG_ERR("atmel init chip invalidmxt_core_data\n");
		return -ENODEV;
	}
	data->do_calibration = true;
	wake_lock_init(&data->ts_flash_wake_lock, WAKE_LOCK_SUSPEND, ATMEL_VENDER_NAME);

	error = mxt_initialize(data);
	if (error) {
		TS_LOG_ERR("atmel init chip mxt_initialize() failed\n");
		return error;
	}

	error = mxt_get_module_name();
	if (error) {
		TS_LOG_ERR("atmel init get lockdown info failed\n");
		return error;
	}

	mxt_parse_module_dts(data->atmel_chip_data);

	mxt_init_extra_data();

	return error;
}

/*parse fw_need_depend_on_lcd flag, 0: not need, 1:need*/
void atmel_parse_fw_need_depend_on_lcd(struct device_node *core_node, struct ts_kit_device_data *chip_data)
{
	int value = 0;
	int error = 0;

	if(NULL == core_node || NULL == chip_data || !mxt_core_data) {
		TS_LOG_ERR("%s, core_node or chip_data is NULL\n", __func__);
		return;
	}

	/*  fw need depend on lcd module */
	error = of_property_read_u32(core_node, "fw_need_depend_on_lcd", &value);
	if (error) {
		TS_LOG_INFO("parse fw_need_depend_on_lcd failed\n");
		value = 0;
	}
	mxt_core_data->fw_need_depend_on_lcd = value;
	TS_LOG_INFO("%s, fw_need_depend_on_lcd = %d \n", __func__, mxt_core_data->fw_need_depend_on_lcd);
}

static int atmel_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data)
{
	struct mxt_data *data = mxt_core_data;
	int error = 0;

	if(!device || !chip_data || !data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		error = -EINVAL;
		goto err;
	}
	error =of_property_read_u32(device, "noise_debug_enable",&data->noise_debug_enable);
	if (error) {
		data->noise_debug_enable = 0;//not support noise debug
		TS_LOG_ERR("Not support noise debug\n");
	}
	TS_LOG_INFO("noise_debug_enbale = %d.\n",data->noise_debug_enable);

	error =of_property_read_u32(device, "support_get_tp_color",&data->support_get_tp_color);
	if (error) {
		data->support_get_tp_color = 0;//not support get tp color
		TS_LOG_ERR("Not support tp color\n");
	}
	TS_LOG_INFO("support_get_tp_color = %d.\n", data->support_get_tp_color);

	error =
	    of_property_read_u32(device, "slave_address", (u32*)&data->addr);
	if (error) {
		TS_LOG_ERR("get slave_address failed\n");
		error = -EINVAL;
		goto err;
	}
	TS_LOG_INFO("slave_address = 0x%x\n", data->addr);
	if (sizeof(chip_data->chip_name) > strlen(ATMEL_VENDER_NAME)+1) {
		strncpy(chip_data->chip_name, ATMEL_VENDER_NAME,strlen(ATMEL_VENDER_NAME)+1);
	}else {
		strncpy(chip_data->chip_name, ATMEL_VENDER_NAME,sizeof(chip_data->chip_name)-1);
		chip_data->chip_name[sizeof(chip_data->chip_name)-1] = '\0';
	}
	TS_LOG_INFO("%s, chip_name %s\n",__func__ , chip_data->chip_name);

	/*parse fw is not need depend on lcd flag*/
	atmel_parse_fw_need_depend_on_lcd(device, chip_data);
err:
	return error;
}

static int atmel_input_config(struct input_dev *input_dev)
{
	struct mxt_data *data = mxt_core_data;
	if(!data || !input_dev || !data->atmel_chip_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(TS_SLIDE_L2R, input_dev->keybit);
	set_bit(TS_SLIDE_R2L, input_dev->keybit);
	set_bit(TS_SLIDE_T2B, input_dev->keybit);
	set_bit(TS_SLIDE_B2T, input_dev->keybit);
	set_bit(TS_CIRCLE_SLIDE, input_dev->keybit);
	set_bit(TS_LETTER_c, input_dev->keybit);
	set_bit(TS_LETTER_e, input_dev->keybit);
	set_bit(TS_LETTER_m, input_dev->keybit);
	set_bit(TS_LETTER_w, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);

	set_bit(TS_TOUCHPLUS_KEY0, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY1, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY2, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY3, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY4, input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
	/*input_set_capability(input_dev, EV_KEY, BTN_TOUCH);*/

	/* For single touch */
	input_set_abs_params(input_dev, ABS_X, 0, data->atmel_chip_data->x_max - 1, 0,
			     0);
	input_set_abs_params(input_dev, ABS_Y, 0, data->atmel_chip_data->y_max - 1, 0,
			     0);
	TS_LOG_DEBUG
	    ("data->atmel_chip_data->x_max-1=%d, data->atmel_chip_data->y_max-1=%d\n",
	     data->atmel_chip_data->x_max - 1, data->atmel_chip_data->y_max - 1);

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);		/* abs_pressure range*/
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);	/* abs_mt_tacking_id range */
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);		/* abs_mt_pressure range */
//#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);	/* abs_mt_width_major range */
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);	/* abs_mt_width_minor range */
//#else
//	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 100, 0, 0);		/* abs_mt_distance range */
//#endif
#if 0
	if ((test_flag_8bit
	     (MXT_T100_TCHAUX_AMPL, &data->tchcfg[MXT_T100_TCHAUX])))
		input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
#endif
#ifdef TYPE_B_PROTOCOL
	/* For multi touch */
	error =
	    input_mt_init_slots(input_dev,
				data->num_touchids /*, INPUT_MT_DIRECT */);
	if (error) {
		TS_LOG_ERR("input_mt_init_slots failed, error = %d\n", error);
		return error;
	}
#endif
	/*input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);*/
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->atmel_chip_data->x_max_mt - 1, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->atmel_chip_data->y_max_mt - 1, 0, 0);
	TS_LOG_DEBUG
	    ("MT: data->atmel_chip_data->x_max-1=%d, data->atmel_chip_data->y_max-1=%d\n",
	     data->atmel_chip_data->x_max_mt - 1, data->atmel_chip_data->y_max_mt - 1);
	return NO_ERR;
}

static int atmel_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	TS_LOG_DEBUG("atmel irq top half called\n");
	return NO_ERR;
}

static void mxt_dump_message(struct mxt_data *data, u8 *message)
{
	/*print_hex_dump(KERN_DEBUG, "MXT MSG:", DUMP_PREFIX_NONE, 16, 1,*/
	/*message, data->T5_msg_size, false);*/
	return;
}

static void mxt_reset_slots(struct mxt_data *data)
{
	return;
}

/*
 *The following functions for processing object messages.
 *
 *Some, not all, object can report messages to the host. An example is t100, which report the touch status.
 *
 */
static void mxt_proc_t6_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = 0;
	u32 crc = 0;

	if(!data || !msg) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	status = msg[1];
	crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);		/*crc=msg[2]+msg[3]*255+msg[3]*65535*/

	if (crc != data->config_crc) {
		data->config_crc = crc;
		TS_LOG_DEBUG("T6 Config Checksum: 0x%06X\n", crc);
	}

	/* Output debug if status has changed */
	if (status != data->t6_status) {
		TS_LOG_DEBUG("T6 Status 0x%02X%s%s%s%s%s%s%s\n",
			     status,
			     (status == 0) ? " OK" : "",
			     (status & MXT_T6_STATUS_RESET) ? " RESET" : "",
			     (status & MXT_T6_STATUS_OFL) ? " OFL" : "",
			     (status & MXT_T6_STATUS_SIGERR) ? " SIGERR" : "",
			     (status & MXT_T6_STATUS_CAL) ? " CAL" : "",
			     (status & MXT_T6_STATUS_CFGERR) ? " CFGERR" : "",
			     (status & MXT_T6_STATUS_COMSERR) ? " COMSERR" :
			     "");

		if (status & MXT_T6_STATUS_CAL) {
			mxt_reset_slots(data);	/*release all points in calibration for safe*/
			TS_LOG_INFO("%s, T6 calibrated!\n", __func__);
		}

	}
	/* Save current status */
	data->t6_status = status;
}

void atmel_parse_t100_scr_message(const u8 *message, unsigned long scraux,
			    struct scr_info *in)
{
	u8 aux = 1;//aux data offset
	if(!message || !in) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	memset(in, 0, sizeof(struct scr_info));
	in->status = message[aux++];

	if (test_flag(MXT_T100_SCRAUX_NUMTCH, &scraux))
		in->num_tch = message[aux++];

	if (test_flag(MXT_T100_SCRAUX_TCHAREA, &scraux)) {
		in->area_tch = MAKEWORD(message[aux], message[aux + 1]);
		aux += 2;
	}

	if (test_flag(MXT_T100_SCRAUX_ATCHAREA, &scraux)) {
		in->area_atch = MAKEWORD(message[aux], message[aux + 1]);
		aux += 2;
	}

	if (test_flag(MXT_T100_SCRAUX_INTTCHAREA, &scraux)) {
		in->area_inttch = MAKEWORD(message[aux], message[aux + 1]);
		aux += 2;
	}
}

void atmel_parse_t100_ext_message(const u8 *message, const u8 *tchcfg,
			    struct ext_info *in)
{
	u8 aux = AUX_DATA_OFFSET;
	u8 exp = 0;

	if(!message || !in || !tchcfg) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	memset(in, 0, sizeof(struct ext_info));
	in->status = message[1];

	if (!(in->status & MXT_T100_DETECT))
		return;

	/************************************************
	auxdata parse:
	tchcfg[MXT_T100_TCHAUX] parse:
	bit0: vec support   orientation
	bit1:amp support	pressure
	bit2:area		major, different bit5
	bit3:width, height  useless
	bit4:peak		useless
	bit5:width, height, area
	bit6:xboundary,yboundary   major minor
	************************************************/

	if (test_flag_8bit(MXT_T100_TCHAUX_VECT, &tchcfg[MXT_T100_TCHAUX]))
		in->vec = message[aux++];

	if (aux < MAX_FINGER_DATA_SIZE) {
		/*check amp setting, if amp setting is disabled then set amp to 0xff, in case system do not response touch.*/
		if (test_flag_8bit
		    (MXT_T100_TCHAUX_AMPL, &tchcfg[MXT_T100_TCHAUX])) {
			in->amp = message[aux++];
			if (in->amp < 0xff)
				in->amp++;
		} else {
			in->amp = 0xff;
		}
	}

	if (aux < MAX_FINGER_DATA_SIZE) {
		if (test_flag_8bit
		    (MXT_T100_TCHAUX_AREA, &tchcfg[MXT_T100_TCHAUX]))
			in->area = message[aux++];
	}

	if (aux < MAX_FINGER_DATA_SIZE - 1) {
		if (test_flag_8bit
		    (MXT_T100_TCHAUX_HW, &tchcfg[MXT_T100_TCHAUX])) {
			in->height = message[aux++];
			in->width = message[aux++];
			if (test_flag_8bit
			    (MXT_T100_CFG_SWITCHXY, &tchcfg[MXT_T100_CFG1]))
				swap(in->height, in->width);
		}
	}

	if (aux < MAX_FINGER_DATA_SIZE) {
		if (test_flag_8bit
		    (MXT_T100_TCHAUX_PEAK, &tchcfg[MXT_T100_TCHAUX]))
			in->peak = message[aux++];
	}

	if (aux < MAX_FINGER_DATA_SIZE - 1) {
		if (test_flag_8bit
		    (MXT_T100_TCHAUX_AREAHW, &tchcfg[MXT_T100_TCHAUX])) {
		    /*********************************
		    	message[aux] High 3bits & 0x3 means exp
		    	message[aux] Low 5bits << exp means area
		    	message[aux + 1] Low 4bits << exp means height
		    	message[aux + 1] High 4bits << exp means width
		    	**********************************/
			exp = (message[aux] >> 5) & 0x3;
			in->area = (message[aux] & 0x1f) << exp;
			in->height = (message[aux + 1] & 0xf) << exp;
			in->width = (message[aux + 1] >> 4) << exp;
			if (test_flag_8bit
			    (MXT_T100_CFG_SWITCHXY, &tchcfg[MXT_T100_CFG1]))
				swap(in->height, in->width);
		}
	}
	if (aux < MAX_FINGER_DATA_SIZE - 1) {
		if (test_flag_8bit(MXT_T100_TCHAUX_EDGE, &tchcfg[MXT_T100_TCHAUX])) {
			in->xboundary= message[aux++];
			in->yboundary= message[aux++];
		}
	}
}

static void mxt_t100_touchdown_message(struct mxt_data *data, u8 status)
{
	struct feature_info *feature_glove = &mxt_feature_list[MAX_GLOVE_CONF];

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (MXT_FEATURE_ENABLE == feature_glove->on_flag
	    &&mxt_core_data->noise_state != MXT_T72_NOISE_SUPPRESSION_VNOIS
	    &&mxt_core_data->noise_state != MXT_T72_NOISE_SUPPRESSION_NOIS) {
		switch (status & MXT_T100_TYPE_MASK) {
		case MXT_T100_TYPE_FINGER:
			mxt_enable_feature(data, MAX_WORKAROUND1_CONF);
			break;
		case MXT_T100_TYPE_GLOVE:
			mxt_enable_feature(data, MAX_WORKAROUND2_CONF);
			break;
		default:
			break;
		}
	}
}

static void mxt_t100_touchup_message(struct mxt_data *data, u8 status)
{
	struct feature_info *feature = &mxt_feature_list[MAX_GLOVE_CONF];

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (MXT_FEATURE_ENABLE == feature->on_flag
	    &&mxt_core_data->noise_state != MXT_T72_NOISE_SUPPRESSION_VNOIS
	    &&mxt_core_data->noise_state != MXT_T72_NOISE_SUPPRESSION_NOIS) {
		switch (status & MXT_T100_TYPE_MASK) {
		case MXT_T100_TYPE_FINGER:
			TS_LOG_DEBUG("mxt_cur_finger_number: %d\n",
				    mxt_core_data->mxt_cur_finger_number);
			if (mxt_core_data->mxt_cur_finger_number == 0) {
				mxt_disable_feature(data, MAX_WORKAROUND1_CONF);
			}
			break;
		case MXT_T100_TYPE_GLOVE:
			if (mxt_core_data->mxt_cur_finger_number == 0) {
				mxt_disable_feature(data, MAX_WORKAROUND2_CONF);
			}
			break;
		default:
			break;
		}
	}
}

static bool mxt_touch_suppression(struct mxt_data *data, int x, int y,
				  u8 status)
{
	u8 widHB = 0, widLB = 0;
	int width = 0;
	bool retval = false;
	struct feature_info *feature_glove = &mxt_feature_list[MAX_GLOVE_CONF];
	struct feature_info *feature_cover = &mxt_feature_list[MAX_COVER_CONF];

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return false;
	}
	if (MXT_FEATURE_ENABLE == feature_glove->on_flag
	    && (status & MXT_T100_TYPE_MASK) == MXT_T100_TYPE_GLOVE) {
		widHB =mxt_core_data->feature_extra_data.glove_sup_h;
		widLB =mxt_core_data->feature_extra_data.glove_sup_l;
		width = (widHB << 8) | widLB;
		TS_LOG_DEBUG("touch suppression width for glove is %d\n",
			     width);
		retval = (x < width || x > data->max_x - width);
	}

	if (MXT_FEATURE_ENABLE == feature_cover->on_flag
	    &&mxt_core_data->feature_extra_data.cover_glass) {
		widHB =mxt_core_data->feature_extra_data.cover_sup_h;
		widLB =mxt_core_data->feature_extra_data.cover_sup_l;
		width = (widHB << 8) | widLB;
		TS_LOG_DEBUG("touch suppression width for cover is %d\n",
			     width);
		retval = retval || (x < width || x > data->max_x - width);
	}

	return retval;
}

static void mxt_proc_t100_message(struct mxt_data *data, u8 *message)
{
	struct ts_fingers *cache = NULL;

	int id = 0;
	u8 status = 0;
	int x = 0;
	int y = 0;
	struct ext_info info;
	u16 reg = 0;
	u8 command_register[2] = {0};
	int ret = 0;
	/* do not report events if input device not yet registered */

	if (!data->enable_reporting) {
		TS_LOG_DEBUG("%s, enable_reporting is not set\n", __func__);
		return;
	}
	cache = data->ts_cache;
	/*id frome 43~53.id:43,usered for screen status; id:44,reserved, 45~54 is the touch msg*/
	id = message[0] - data->T100_reportid_min - 2;
	/* ignore SCRSTATUS events */
	if (id < 0 || id >= TS_MAX_FINGER) {
		TS_LOG_DEBUG("T100 [%d] SCRSTATUS : 0x%x\n", id, message[1]);	/*message[1]:touch_status */
		return;
	}
	/***********************************
	message data parse
	0:report id
	1:touch status
	3 << 8 | 4: x
	5 << 8 | 6: y
	6-9: aux data
	***********************************/
	status = message[1];//1: touch_status
	x = (message[3] << 8) | message[2];
	y = (message[5] << 8) | message[4];

	if (MXT_FEATURE_ENABLE == mxt_feature_list[MAX_COVER_CONF].on_flag) {
		if (mxt_core_data->feature_extra_data.cover_cut) {
			x = data->max_x - x;
		}
	}

	if((status & MXT_T100_EVENT_MASK) == MXT_T100_EVENT_SUP) {
		TS_LOG_INFO("Touch have been suppressed!");
	}

	atmel_parse_t100_ext_message(message, data->tchcfg, &info);
	TS_LOG_DEBUG(
		"[%u] status:%02X x:%u y:%u [amp]:%02X [vec]:%02X [area]:%02X [peak]:%02X [width]:%02X [height]:%02X [xboundry]:%d [yboundry]:%d\n",
		id,
		status,
		x, y,
		info.amp,
		info.vec,
		info.area,
		info.peak,
		info.width,
		info.height,
		info.xboundary,
		info.yboundary);

	memset(&cache->fingers[id], 0, sizeof(cache->fingers[id]));

	if (status & MXT_T100_DETECT) {
		switch (status & MXT_T100_EVENT_MASK) {
		case MXT_T100_EVENT_DOWN:
			mxt_t100_touchdown_message(data, status);
			break;
		default:
			break;
		}
	} else {
		/* Touch no longer active, close out slot */
		switch (status & MXT_T100_EVENT_MASK) {
		case MXT_T100_EVENT_UP:
			mxt_t100_touchup_message(data, status);
			break;
		default:
			break;
		}
		cache->cur_finger_number = mxt_core_data->mxt_cur_finger_number;
		goto out;
	}

	if (mxt_touch_suppression(data, x, y, status))
		goto out;	/*if touch is within the suppression area*/

	switch (status & MXT_T100_TYPE_MASK) {
	case MXT_T100_TYPE_FINGER:
		cache->fingers[id].status = TP_FINGER;
		TS_LOG_DEBUG("Touch type is finger, status:%02X\n",
			     status & MXT_T100_TYPE_MASK);
		break;
	case MXT_T100_TYPE_STYLUS:
		cache->fingers[id].status = TP_STYLUS;
		TS_LOG_DEBUG("Touch type is stylus, status:%02X\n",
			     status & MXT_T100_TYPE_MASK);
		break;
	case MXT_T100_TYPE_GLOVE:
		cache->fingers[id].status = TP_GLOVE;
		TS_LOG_DEBUG("Touch type is glove, status:%02X\n",
			     status & MXT_T100_TYPE_MASK);
		break;
	case MXT_T100_TYPE_LARGETOUCH:
		TS_LOG_INFO("Touch type is large touch, status:%02X\n", status & MXT_T100_TYPE_MASK);
		break;
	default:
		cache->fingers[id].status = 0;
		TS_LOG_DEBUG("Touch type is undefined, status:%02X\n",
			     status & MXT_T100_TYPE_MASK);
		break;
	}
	/* Touch active */
	cache->fingers[id].x = x;
	cache->fingers[id].y = y;
	cache->fingers[id].major = info.xboundary;
	cache->fingers[id].minor = info.yboundary;

	cache->fingers[id].pressure = info.amp;
	cache->cur_finger_number = mxt_core_data->mxt_cur_finger_number;

	if (test_flag_8bit
	    (MXT_T100_TCHAUX_AREA | MXT_T100_TCHAUX_AREAHW,
	     &data->tchcfg[MXT_T100_TCHAUX])) {
		if (cache->fingers[id].status == TP_STYLUS)
			cache->fingers[id].major = MXT_TOUCH_MAJOR_T47_STYLUS;
		else
			cache->fingers[id].major = info.area;
	}

	if (test_flag_8bit
	    (MXT_T100_TCHAUX_VECT, &data->tchcfg[MXT_T100_TCHAUX]))
		cache->fingers[id].orientation = info.vec;

	if(data->noise_debug_enable){
		if (test_flag_8bit
			(MXT_T100_TCHAUX_PEAK, &data->tchcfg[MXT_T100_TCHAUX]))
			cache->fingers[id].pressure = info.peak;
	}

	TS_LOG_DEBUG
	    ("finger[%u] status:%02X, x:%u, y:%u, pressure:%u, cur_finger_number:%u\n",
	     id, cache->fingers[id].status, cache->fingers[id].x,
	     cache->fingers[id].y, cache->fingers[id].pressure,
	     cache->cur_finger_number);

out:
#ifdef ROI
	if ((MXT_T6_CMD58_ON == atmel_gMxtT6Cmd58Flag)
	    && (MXT_FEATURE_ENABLE ==
		mxt_feature_list[MAX_KNUCKLE_CONF].on_flag)) {
		reg = data->T37_address;
		ret =
		    __atmel_read_reg(data, reg, sizeof(command_register),
				   &command_register[0]);
		if (ret) {
			TS_LOG_ERR("T37 read offset 0 failed %d!\n", ret);
			return;
		}
		if ((message[1] & MXT_T100_EVENT_MASK) == MXT_T100_EVENT_DOWN
		    && ((u8) command_register[0] != MXT_T6_DEBUG_PEAKREGIONDATA
			|| command_register[1] != 0)) {
			atmel_t6_command(data, MXT_COMMAND_DIAGNOSTIC,
				       MXT_T6_DEBUG_PEAKREGIONDATA, 0);
			TS_LOG_DEBUG("T37 has been set to T125 mode!\n");
		}
	}
#endif
	return;
}

static void mxt_proc_t25_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = 0;
	status = msg[1];

	/* Output debug if status has changed */
	TS_LOG_DEBUG("T25 Status 0x%x Info: %x %x %x %x %x\n",
		     status, msg[2], msg[3], msg[4], msg[5], msg[6]);	/*the 2nd、3rd、4th、5th、6th save t25 info*/

	/* Save current status */
	memcpy(&data->t25_msg[0], &msg[1], sizeof(data->t25_msg));
}

int mxt_process_messages_t25(struct mxt_data *data)
{
	int ret = NO_ERR;
	u8 report_id = 0;

	if(!data || !data->msg_buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/* Read T5 message */
	ret = __atmel_read_reg(data, data->T5_address,
	                data->T5_msg_size, data->msg_buf);  //T5_msg_size
	if (ret) {
		TS_LOG_ERR("Failed to read T5 (%d)\n", ret);
		return ret;
	}

	report_id = data->msg_buf[0];
	if (report_id == data->T25_reportid) {
		mxt_proc_t25_messages(data, data->msg_buf);
		TS_LOG_INFO("%s, T25_msg:%x %x %x %x %x %x\n",__func__,
		data->t25_msg[0], data->t25_msg[1], data->t25_msg[2],
		data->t25_msg[3], data->t25_msg[4], data->t25_msg[5]);
	}
	return 0;
}

static void mxt_proc_t68_messages(struct mxt_data *data, u8 *msg)
{
	TS_LOG_DEBUG("T68 state = 0x%x\n", msg[1]);
}

/*
 *Function for processing t70 messages.
 *
 *T70 is an object which dynamicly configs the maxTouch devices.
 *
 */
static void mxt_proc_t70_messages(struct mxt_data *data, u8 *msg)
{
	if (data->T70_reportid_min == msg[0]) {
		if (msg[1] & MAX_HSYN_MASK) {//1:hsync status  byte
			data->hsyncstatus = MXT_SYNC_LOST;
			TS_LOG_INFO("Hsync lost");
		} else {
			data->hsyncstatus = MXT_SYNC_OK;
		}
	}
	data->t70_msg_processed = 1;//1: t70 msg already process
	TS_LOG_DEBUG("T70 Dynamic Controller ReportID= 0x%x state = 0x%x\n",
		     msg[0], msg[1]);
	TS_LOG_DEBUG("T70 min report id= %d, data->hsyncstatus = %d\n",
		     data->T70_reportid_min, data->hsyncstatus);

	if(data->noise_debug_enable){
		if ((data->T70_reportid_min + 1) == msg[0]) {  //T70 report ID 1 is for moisture mode reporting
			if(msg[1] == 0x01){  //going to moisture mode
				TS_LOG_INFO("mXT moisture mode\n");
			}
			else if(msg[1] == 0x03){ //return to normal mode
				TS_LOG_INFO("mXT non-moisture mode\n");
			}
		}
	}
}

static void increase_sta_cnt(u32 *pnum)
{
	u32 num = 0;

	if(!pnum) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	num = *pnum;
	if (num + 1 < num) {
		memset(&mxt_core_data->sta_cnt, 0, sizeof(struct status_count));
	} else {
		*pnum = num + 1;
	}
}

static void mxt_status_count(u8 *msg)
{
	if(!msg) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	switch (msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) {//T72_NOISE_SUPPRESSION_STATUS2STATE byte
	case MXT_T72_NOISE_SUPPRESSION_STAB:
		increase_sta_cnt(&mxt_core_data->sta_cnt.stable_state);
		break;
	case MXT_T72_NOISE_SUPPRESSION_NOIS:
		increase_sta_cnt(&mxt_core_data->sta_cnt.noise_state);
		break;
	case MXT_T72_NOISE_SUPPRESSION_VNOIS:
		increase_sta_cnt(&mxt_core_data->sta_cnt.vnoise_state);
		atomic_set(&atmel_mxt_vnoise_flag, 1);
		break;
	default:
		break;
	}
}

/*detect charger noise message*/
static void mxt_proc_t72_messages(struct mxt_data *data, u8 *msg)
{
	int noise_level = 0;
	struct feature_info *feature = &mxt_feature_list[MAX_GLOVE_CONF];

	TS_LOG_DEBUG("T72 noise state1 = 0x%x state2 = 0x%x\n", msg[1], msg[2]);	/*msg[1], noise state1; msg[2], noise state2 */

	if(data->noise_debug_enable){
		if (msg[1] & MXT_T72_NOISE_SUPPRESSION_STATECHG) {	/* check noise state1*/
			if ((msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==	/* check noise state2*/
			MXT_T72_NOISE_SUPPRESSION_VNOIS)
			{
				TS_LOG_INFO("mXT in Very Noise mode!\n");
			}
			else if ((msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==	/* check noise state2*/
			MXT_T72_NOISE_SUPPRESSION_NOIS)
			{
				TS_LOG_INFO("mXT in Noise mode!\n");
			}
			else if ((msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==	/* check noise state2*/
			MXT_T72_NOISE_SUPPRESSION_STAB)
			{
			TS_LOG_INFO("mXT in Stable mode!\n");
			}
			return;
		}
	}

	if (msg[1] & MXT_T72_NOISE_SUPPRESSION_STATECHG) {		/* check noise state1*/
		mxt_status_count(msg);
		if ((mxt_core_data->noise_state == MXT_T72_NOISE_SUPPRESSION_VNOIS)
		    && (msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==		/* check noise state2*/
		    MXT_T72_NOISE_SUPPRESSION_NOIS) {
			TS_LOG_DEBUG
			    ("T72 state change from state = %d to state = %d\n",
			    mxt_core_data->noise_state, msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK);		/*noise state2 bit0~bit2 is valid*/
			if (mxt_core_data->mxt_cur_finger_number == 0) {
				mxt_enable_feature(data, MAX_WORKAROUND4_CONF);
				mxt_core_data->t72_state_change = T72_STATE_NOT_CHANGE;
			} else {
				mxt_core_data->t72_state_change = T72_STATE_CHANGED;
			}
		}

		if ((msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==	/* check noise state2*/
		    MXT_T72_NOISE_SUPPRESSION_VNOIS) {
			mxt_core_data->t72_state_change = T72_STATE_NOT_CHANGE;
		}

		if (((msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK) ==	/* check noise state2*/
		     MXT_T72_NOISE_SUPPRESSION_STAB)
		    && (MXT_FEATURE_ENABLE == feature->on_flag)
		    && (mxt_core_data->mxt_cur_finger_number == 0)) {
			mxt_disable_feature(data, MAX_WORKAROUND1_CONF);
			mxt_disable_feature(data, MAX_WORKAROUND2_CONF);
		}
	}
	mxt_core_data->noise_state =
	    msg[2] & MXT_T72_NOISE_SUPPRESSION_STATUS2STATE_MASK;	/* check noise state2*/
	TS_LOG_DEBUG("pre state = %d\n",mxt_core_data->noise_state);

	if (MXT_FEATURE_ENABLE ==
	    mxt_feature_list[MAX_WORKAROUND3_CONF].on_flag) {
		noise_level = msg[4];			/* msg[4], noise_level*/
		mxt_core_data->t72_noise_level.total_count++;
		if (noise_level <mxt_core_data->t72_noise_level_threshold) {
			mxt_core_data->t72_noise_level.success_count++;
		}
		if (mxt_core_data->t72_noise_level.noise_level_max < noise_level) {
			mxt_core_data->t72_noise_level.noise_level_max =
			    noise_level;
		}
		if (mxt_core_data->t72_noise_level.noise_level_min > noise_level) {
			mxt_core_data->t72_noise_level.noise_level_min =
			    noise_level;
		}
		TS_LOG_INFO("%s, noise peak/level msg[4] is %d", __func__,
			    noise_level);
	}
}

static void mxt_proc_T80_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("%s, T80 status = 0x%x\n", __func__, status);
	if (status & ATMEL_MXT_MOISTURE_MODE) {
		increase_sta_cnt(&data->sta_cnt.moisture_state);
		atomic_set(&atmel_mxt_moisture_flag, 1);
	}
}

static void mxt_proc_T81_messages(struct mxt_data *data, u8 *msg)
{
	u8 reportid = msg[0];
	u8 status = msg[1];

	TS_LOG_DEBUG("T81 Status 0x%x Info: %x %x %x %x\n",
		     status, msg[2], msg[3], msg[4], msg[5]);

	msg[0] = reportid;
}

static void mxt_proc_T92_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("T92 %s 0x%x Info: %x %x %x %x %x %x\n",
		     (status & 0x80) ? "stroke" : "symbol",
		     status, msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);
}

static void mxt_proc_T93_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("T93 Status 0x%x Info: %x\n", status, msg[1]);
}

static void mxt_proc_t15_messages(struct mxt_data *data, u8 *msg)
{
	struct ts_fingers *cache = data->ts_cache;
	int key= 0;
	bool curr_state = false, new_state = false;
	bool sync = false;
	unsigned long keystates = le32_to_cpu(msg[2]);
	u8 num_keys = 0;
	const unsigned int *keymap = NULL;

	/* do not report events if input device not yet registered */
	if (!data->enable_reporting)
		return;

	if (!data->pdata)
		return;

	if (!data->pdata->keymap || !data->pdata->num_keys)
		return;

	num_keys = data->pdata->num_keys[T15_T97_KEY];
	keymap = data->pdata->keymap[T15_T97_KEY];
	for (key = 0; key < num_keys; key++) {
		curr_state = test_bit(key, &data->t15_keystatus);
		new_state = test_bit(key, &keystates);

		if (!curr_state && new_state) {
			TS_LOG_DEBUG("T15 key press: %u\n", key);
			__set_bit(key, &data->t15_keystatus);
			cache->special_button_flag = 1;
			cache->special_button_key = keymap[key];
			sync = true;
		} else if (curr_state && !new_state) {
			TS_LOG_DEBUG("T15 key release: %u\n", key);
			__clear_bit(key, &data->t15_keystatus);
			cache->special_button_flag = 0;
			cache->special_button_key = keymap[key];

			sync = true;
		}
	}
}

static void mxt_proc_t97_messages(struct mxt_data *data, u8 *msg)
{
	mxt_proc_t15_messages(data, msg);
}

static void mxt_proc_T99_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("T99 Status 0x%x Event: %d Index %d\n",
		     status, msg[1] & 0xF, msg[2]);
}

static void mxt_proc_t102_messages(struct mxt_data *data, u8 *msg)
{
	TS_LOG_DEBUG("msg for t102 = 0x%x 0x%x 0x%x 0x%x\n",
		     msg[2], msg[3], msg[4], msg[5]);
}

static void mxt_proc_T115_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("T115 Status 0x%x Info: %x\n", status, msg[1]);
}

/*
 *Read T37 data for Knuckle Gesture
 *
 *t37 is used to store diagnostic data which can be references, delta and so on. Which kind of data is stored is controled by byte 5 of t6.
 *this function fist set t6 byte 5 and then read t37 for the data wanted.
 *
 */
 #ifdef ROI
static int atmel_diagnostic_command_roi(struct mxt_data *data, u8 cmd, u8 page,
				      u8 index, u8 num, char *buf, int interval,
				      int interval_c)
{
	s8 command_register[2] = {0};
	int ret = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (buf) {
		/*check the mode and page*/
		ret =
		    __atmel_read_reg(data, data->T37_address,
				   sizeof(command_register),
				   &command_register[0]);
		if (ret) {
			TS_LOG_ERR("T37 read offset 0 failed %d!\n", ret);
			return -EIO;
		}

		if ((u8) command_register[0] != cmd
		    || command_register[1] != page) {
			atmel_t6_command(data, MXT_COMMAND_DIAGNOSTIC,
				       MXT_T6_DEBUG_PEAKREGIONDATA, 0);
			TS_LOG_ERR
			    ("%s, T37 not in Peak Data Mode, wait for next round. Expected (%d,%d). Actual (%d,%d). It is OK on booting\n",
			     __func__, cmd, page, (u8) command_register[0],
			     command_register[1]);
			return -EIO;
		}
		/*read data*/
		ret =
		    __atmel_read_reg(data,
				   data->T37_address +
				   sizeof(command_register) + index, num, buf);
		if (ret) {
			TS_LOG_ERR
			    ("Failed to read T37 val at page %d.%d (%d)\n",
			     page, index, ret);
			return -EIO;
		}

		TS_LOG_DEBUG
		    ("T37 cmd %d page %d, T37 buf(%03d %03d %03d %03d)\n",
		     command_register[0], command_register[1], buf[0], buf[1],
		     buf[2], buf[3]);
	}

	return 0;
}
#endif
static void atmel_read_roi_data(struct mxt_data *data)
{
#ifdef ROI
	int ret = -EINVAL;
	int retry = 0;
	int i = 0, j = 0;
	u16 temp = 0;
	u16 buf[MXT_ROI_ROW][MXT_ROI_ROW];
	u16(*target)[MXT_ROI_ROW] = (u16 (*)[MXT_ROI_ROW])(&roi_data[0] + 4);

	if (atmel_gMxtT6Cmd58Flag != MXT_T6_CMD58_ON) {
		return;
	}
	memset(buf,0, sizeof(u16)* MXT_ROI_ROW * MXT_ROI_ROW);
	do {
		TS_LOG_DEBUG("%s \n", __func__);
		ret =
		    atmel_diagnostic_command_roi(data,
					       MXT_T6_DEBUG_PEAKREGIONDATA, 0,
					       0, MISC_PEAKDATA_LEN_U8,
					       (char *)buf, 10, 5);
		if (ret == 0) {
			break;
		}
		msleep(10);//ic need
	} while (++retry < MXT_RETRY_ROICMD_TIME);//retry 5 times

	if (ret != 0) {
		TS_LOG_ERR("Read Peak Data failed %d\n", ret);
		memset(roi_data, 0, ROI_DATA_READ_LENGTH);
		return;
	}

	TS_LOG_DEBUG("%s, before inert: %d, %d, %d, %d\n", __func__, buf[0][0],
		     buf[0][MXT_ROI_ROW - 1], buf[MXT_ROI_ROW - 1][0],
		     buf[MXT_ROI_ROW - 1][MXT_ROI_ROW - 1]);

	if (mxt_core_data->roi_data_invert) {
		for (i = 0; i < MXT_ROI_ROW; i++) {
			for (j = 0; j < MXT_ROI_ROW / 2; j++) {
				temp = buf[i][j];
				buf[i][j] = buf[i][MXT_ROI_ROW - 1 - j];
				buf[i][MXT_ROI_ROW - 1 - j] = temp;
			}
		}
		TS_LOG_DEBUG("%s, roi data invert.\n", __func__);
	}

	TS_LOG_DEBUG("%s, after inert: %d, %d, %d, %d\n", __func__, buf[0][0],
		     buf[0][MXT_ROI_ROW - 1], buf[MXT_ROI_ROW - 1][0],
		     buf[MXT_ROI_ROW - 1][MXT_ROI_ROW - 1]);

	for (i = 0; i < MXT_ROI_ROW; i++) {
		for (j = 0; j < MXT_ROI_ROW; j++) {
			target[i][j] = buf[i][j];
		}
	}
#endif
}

/*
 *Function for processing t125 messages.
 *
 *T125 is used to detect whether the knuckle gesture data is ready in t37. When it is ready, t125 would generate the a message.
 *
 */
static void mxt_proc_T125_messages(struct mxt_data *data, u8 *msg)
{
#ifdef ROI
	atmel_read_roi_data(data);
#endif
}

static void mxt_proc_T124_messages(struct mxt_data *data, u8 *msg)
{
	if (MXT_T124_VSYNC_MASK & msg[1]) {
		data->vsyncstatus = MXT_SYNC_OK;
	} else {
		data->vsyncstatus = MXT_SYNC_LOST;
	}

	TS_LOG_DEBUG("%s, msg = 0x%x\n", __func__, msg[1]);
}

static void mxt_proc_t42_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	if (status & MXT_T42_MSG_TCHSUP)
		TS_LOG_DEBUG("T42 suppress\n");
	else
		TS_LOG_DEBUG("T42 normal\n");
}

static void mxt_proc_t61_messages(struct mxt_data *data, u8 *msg)
{

}

static int mxt_proc_t48_messages(struct mxt_data *data, u8 *msg)
{

	u8 status = 0, state = 0;

	status = msg[1];
	state = msg[4];

	TS_LOG_DEBUG("T48 state %d status %02X %s%s%s%s%s\n",
		     state,
		     status,
		     (status & 0x01) ? "FREQCHG " : "",
		     (status & 0x02) ? "APXCHG " : "",
		     (status & 0x04) ? "ALGOERR " : "",
		     (status & 0x10) ? "STATCHG " : "",
		     (status & 0x20) ? "NLVLCHG " : "");

	return 0;
}

static void mxt_proc_t24_messages(struct mxt_data *data, u8 *msg)
{
	u8 status = msg[1];

	TS_LOG_DEBUG("T24 Status 0x%x Info: %x %x %x %x %x %x\n",
		     status, msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);
}

static void mxt_proc_t100_message_number(struct mxt_data *data, u8 *message)
{
	int id= 0;
	u8 status = 0;
	u8 val[1] = {0};
	u16 addr = 0;
	int ret = 0;
	struct scr_info info;
	struct ts_feature_info *x_info = NULL;

	if(!data || !message) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	x_info = &data->atmel_chip_data->ts_platform_data->feature_info;

	/* do not report events if input device not yet registered */

	if (!data->enable_reporting) {
		TS_LOG_DEBUG("%s, enable_reporting is not se\n", __func__);
		return;
	}

	id = message[0] - data->T100_reportid_min - 2;

	if (id != -2) { //make sure currunt msg reportid is 43
		TS_LOG_DEBUG("T100 [%d] msg : 0x%x\n", id, message[1]);
		return;
	}

	status = message[1];//1: save status

	atmel_parse_t100_scr_message(message, data->tchcfg[MXT_T100_SCRAUX], &info);
	TS_LOG_DEBUG("[scr] status:%02X  [num]:%d [area]:%d %d %d\n",
		     status,
		     info.num_tch,
		     info.area_tch, info.area_atch, info.area_inttch);

	mxt_core_data->mxt_cur_finger_number = info.num_tch;

	addr = data->T100_address + 1;
	ret = __atmel_read_reg(data, addr, sizeof(val), val);
	if (ret)
		return;

	if (mxt_core_data->mxt_cur_finger_number == 0
	    &&mxt_core_data->t72_state_change == T72_STATE_CHANGED) {
		mxt_enable_feature(data, MAX_WORKAROUND4_CONF);
		mxt_core_data->t72_state_change = T72_STATE_NOT_CHANGE;
	}
	if (mxt_core_data->mxt_cur_finger_number == 0) {
		if (MXT_FEATURE_ENABLE == x_info->glove_info.glove_switch
		    &&mxt_core_data->noise_state ==
		    MXT_T72_NOISE_SUPPRESSION_STAB) {
			mxt_disable_feature(data, MAX_WORKAROUND1_CONF);
			mxt_disable_feature(data, MAX_WORKAROUND2_CONF);
		}
	}
	return;
}

static int mxt_proc_message(struct mxt_data *data, u8 *message)
{
	u8 report_id = 0;
	bool dump = false;

	if(!data || !message) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return 0;
	}
	dump = data->debug_enabled;
	report_id = message[0];//0: save report id

	if (report_id == MXT_RPTID_NOMSG)
		return 0;

	if (report_id == data->T6_reportid) {
		mxt_proc_t6_messages(data, message);
	} else if (report_id >= data->T9_reportid_min
		   && report_id <= data->T9_reportid_max) {
	} else if (report_id >= data->T100_reportid_min
		   && report_id <= data->T100_reportid_max) {
#ifndef T100_PROC_FINGER_NUM
		mxt_proc_t100_message(data, message);
#else
		if (report_id < data->T100_reportid_min + 2) // 43、44 reserved
			mxt_proc_t100_message_number(data, message);
		else
			mxt_proc_t100_message(data, message);
#endif
		mxt_t100_int = true;
	} else if (report_id == data->T19_reportid) {

	} else if (report_id == data->T25_reportid) {
		mxt_proc_t25_messages(data, message);
	} else if (report_id >= data->T63_reportid_min
		   && report_id <= data->T63_reportid_max) {
	} else if (report_id >= data->T42_reportid_min
		   && report_id <= data->T42_reportid_max) {
		mxt_proc_t42_messages(data, message);
	} else if (report_id == data->T48_reportid) {
		mxt_proc_t48_messages(data, message);
	} else if (report_id >= data->T15_reportid_min
		   && report_id <= data->T15_reportid_max) {
		mxt_proc_t15_messages(data, message);
	} else if (report_id == data->T24_reportid) {
		mxt_proc_t24_messages(data, message);
	} else if (report_id >= data->T68_reportid_min
		   && report_id <= data->T68_reportid_max) {
		mxt_proc_t68_messages(data, message);
	} else if (report_id >= data->T61_reportid_min
		   && report_id <= data->T61_reportid_max) {
		mxt_proc_t61_messages(data, message);
	} else if (report_id >= data->T70_reportid_min
		   && report_id <= data->T70_reportid_max) {
		mxt_proc_t70_messages(data, message);
	} else if (report_id >= data->T72_reportid_min
		   && report_id <= data->T72_reportid_max) {
		mxt_proc_t72_messages(data, message);
	} else if (report_id == data->T80_reportid) {
		mxt_proc_T80_messages(data, message);
	} else if (report_id >= data->T81_reportid_min
		   && report_id <= data->T81_reportid_max) {
		mxt_proc_T81_messages(data, message);
	} else if (report_id == data->T92_reportid) {
		mxt_proc_T92_messages(data, message);
	} else if (report_id == data->T93_reportid) {
		mxt_proc_T93_messages(data, message);
	} else if (report_id >= data->T97_reportid_min
		   && report_id <= data->T97_reportid_max) {
		mxt_proc_t97_messages(data, message);
	} else if (report_id == data->T99_reportid) {
		mxt_proc_T99_messages(data, message);
	} else if (report_id == data->T102_reportid) {
		mxt_proc_t102_messages(data, message);
	} else if (report_id == data->T115_reportid) {
		mxt_proc_T115_messages(data, message);
	} else if (report_id == data->T124_reportid) {
		mxt_proc_T124_messages(data, message);
	} else if (report_id == data->T125_reportid) {
		mxt_proc_T125_messages(data, message);
	} else {
		dump = true;
	}

	if (dump || report_id > data->max_reportid)
		mxt_dump_message(data, message);

	if (data->debug_v2_enabled && report_id <= data->max_reportid)
		atmel_debug_msg_add(data, message);

	return 1;
}

static int mxt_read_and_atmel_process_messages(struct mxt_data *data, u8 count)
{
	int ret = 0;
	int i = 0;
	u8 num_valid = 0;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/* Safety check for msg_buf */
	if (count > data->max_reportid)
		return -EINVAL;

	/* Process remaining messages if necessary */
	ret = __atmel_read_reg_ext(data, data->addr, data->T5_address,
				 data->T5_msg_size * count, data->msg_buf,
				 I2C_ACCESS_R_REG_FIXED);
	if (ret) {
		TS_LOG_ERR("Failed to read %u messages (%d)\n", count, ret);
		return ret;
	}

	for (i = 0; i < count; i++) {
		ret = mxt_proc_message(data,
				       data->msg_buf + data->T5_msg_size * i);

		if (ret == 1)	/* do mxt_proc_message success */
			num_valid++;
	}

	/* return number of messages read */
	return num_valid;
}

static int mxt_atmel_process_messages_t44(struct mxt_data *data)
{
	int ret = 0;
	u8 count = 0, num_left = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/* Read T44 and T5 together */
	ret = __atmel_read_reg(data, data->T44_address,
			     data->T5_msg_size + 1, data->msg_buf);
	if (ret) {
		TS_LOG_ERR("Failed to read T44 and T5 (%d)\n", ret);
		return ret;
	}

	count = data->msg_buf[0];//0 :save message num

	if (count == 0) {
		TS_LOG_INFO("Interrupt triggered but zero messages\n");
		return 0;
	} else if (count > data->max_reportid) {
		TS_LOG_ERR("T44 count %d exceeded max report id\n", count);
		count = data->max_reportid;
	}
	/* Process first message, T5 */
	ret = mxt_proc_message(data, data->msg_buf + 1);
	if (ret < 0) {
		TS_LOG_ERR("Unexpected invalid message\n");
		return ret;
	}

	num_left = count - 1;

	/* Process remaining messages if necessary */
	if (num_left) {
		ret = mxt_read_and_atmel_process_messages(data, num_left);
		if (ret < 0) {
			TS_LOG_ERR("mxt_read_and_atmel_process_messages failed\n");
			goto end;
		} else if (ret != num_left)
			TS_LOG_ERR("Unexpected invalid message\n");
	}

end:
	return 0;
}

static int mxt_interrupt(struct mxt_data *data)
{
	int ret = 0;

	ret = mxt_atmel_process_messages_t44(data);

	return ret;
}

int atmel_process_messages(struct mxt_data *data, int timeout)
{
	if(!data || timeout <= 0) {
		TS_LOG_ERR("%s, param invalid\n",__func__);
		return -1;
	}
	do {
		msleep(2);//ic need
		mxt_atmel_process_messages_t44(data);
		timeout--;
		if (!timeout) {
			TS_LOG_INFO("atmel_process_messages timeout\n");
			return -1;
		}
	} while (data->msg_buf[0]);

	return 0;
}

static int atmel_irq_bottom_half(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd)
{
	struct mxt_data *data =mxt_core_data;

	int ret = NO_ERR;
	struct ts_fingers *cache = NULL;

	if(!data ||!data->atmel_chip_data
		|| !out_cmd) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	cache = &out_cmd->cmd_param.pub_params.algo_param.info;
	data->ts_cache = cache;
	mxt_t100_int = false;
	ret = mxt_interrupt(data);
	if (ret) {
		TS_LOG_ERR("mxt_interrupt failed\n");
		return -EBUSY;
	}
	if (mxt_t100_int) {
		out_cmd->command = TS_INPUT_ALGO;
		out_cmd->cmd_param.pub_params.algo_param.algo_order =
		    data->atmel_chip_data->algo_id;
		TS_LOG_DEBUG("order: %d\n",
			     out_cmd->cmd_param.pub_params.algo_param.algo_order);
	}
	return NO_ERR;
}

static int atmel_print_chip_info(void)
{
	struct mxt_data *data = mxt_core_data;
	struct mxt_object *t38 = NULL;
	u8 version_byte[8] = {0};		/* version len 8:*/
	u8 i = 0;
	int ret = 0;

	if (!data || !data->atmel_chip_data) {
		TS_LOG_ERR("%s return due to null data\n", __func__);
		return -ENODEV;
	}

	t38 = atmel_get_object(data, MXT_SPT_USERDATA_T38);
	if (!t38) {
		TS_LOG_ERR("%s return due to null t38\n", __func__);
		return -EINVAL;
	}

	ret =
	    __atmel_read_reg(data, t38->start_address, sizeof(version_byte),
			   version_byte);
	if (ret) {
		TS_LOG_ERR("%s failed to read t38\n", __func__);
		return ret;
	}

	/* version_byte[1]~version_byte[2]:firmware version
	   version_byte[3]~version_byte[5]:cfg version */
	TS_LOG_INFO("FW%02d.%02x_CFG%02d.%02d_%02d%02d%02d\n",
		    version_byte[1], version_byte[2], version_byte[3],
		    version_byte[4], version_byte[5], version_byte[6],
		    version_byte[7]);
	for(i = 1; i < VERSION_NAME_LEN; i++) {
		data->ver_bytes[i] = version_byte[i];
	}
	snprintf(data->atmel_chip_data->version_name, MAX_STR_LEN,
		 "FW%02d.%02x_CFG%02d.%02d_%02d%02d%02d", version_byte[1],
		 version_byte[2], version_byte[3], version_byte[4],
		 version_byte[5], version_byte[6], version_byte[7]);
	TS_LOG_INFO("%s, version_name = %s\n", __func__, data->atmel_chip_data->version_name);
	return ret;
}

static int atmel_fw_update_boot(char *file_name)
{
	struct mxt_data *data = mxt_core_data;
	int error = NO_ERR;
	int retry_count = 0;
	bool update_fw_again_flag = false;

	if (!data || !file_name) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -ENODEV;
	}

	TS_LOG_INFO("fw/cfg update starting!\n");
	atmel_print_chip_info();
updata_firmware_again:
	error = atmel_update_fw_file_name(data, file_name);
	if(error) {
		TS_LOG_ERR("atmel_update_fw_file_name failed\n");
		return error;
	}

	error = atmel_check_firmware_version(data);
	if (error) {
		TS_LOG_INFO("no need to update fw\n");
	} else {
		/*lock it for disable outside access*/
		mutex_lock(&data->access_mutex);
		error = atmel_load_fw(data);
		mutex_unlock(&data->access_mutex);
		if (error) {
			TS_LOG_ERR("The firmware update failed(%d)\n", error);
			goto update_firmware_err;
		} else {
			error = mxt_initialize_info_block(data);
			if (error) {
				TS_LOG_ERR
				    ("mxt_initialize_info_block failed(%d)\n",
				     error);
				goto update_firmware_err;
			}
			TS_LOG_INFO("The firmware update succeeded\n");
		}
	}
	error = atmel_update_cfg(data);
	if (error) {
		TS_LOG_ERR("atmel_update_cfg failed(%d)\n", error);
		goto update_firmware_err;
	}

	if (featurefile_read_success == FEATUREFILE_NOT_READ) {//get glove, holster e.g.  init status
		atmel_init_feature_file(false);
	}

	error = atmel_soft_reset(data);
	if(error) {
		TS_LOG_ERR("%s, atmel_soft_reset fail\n", __func__);
		goto update_firmware_err;
	}
	TS_LOG_INFO("fw/cfg update ending!\n");
	atmel_print_chip_info();

	if (!strncmp("NULL", data->module_id, strlen("NULL"))) {
		TS_LOG_INFO("again get module name and parse module\n");
		error = mxt_get_module_name();
		if (error) {
			TS_LOG_ERR("atmel init get lockdown info failed\n");
		}

		mxt_parse_module_dts(data->atmel_chip_data);
		mxt_init_extra_data();
	}

	if(data->is_firmware_broken && (retry_count == 0) ){
		TS_LOG_INFO("%s, need update firmware again\n", __func__);
		retry_count++;
		data->is_firmware_broken = false;
		goto updata_firmware_again;
	}
	return error;
update_firmware_err:
	if(!update_fw_again_flag) {
		TS_LOG_INFO("update fw|cfg failed, now try again\n");
		update_fw_again_flag = true;
		atmel_power_off(data);
		atmel_power_on(data);
		msleep(100);//for sequence
		goto updata_firmware_again;
	}
	return error;
}

static int atmel_fw_update_sd(void)
{
	char file_name[MAX_FILE_NAME_LENGTH] = {0};
	atmel_update_sd_mode = true;
	mxt_disable_all_features();
	atmel_fw_update_boot(file_name);
	atmel_update_sd_mode = false;
	return 0;
}

static void atmel_get_tp_color(struct mxt_data *data, struct pds_code *code) {
	if(!data || !code) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	cypress_ts_kit_color[0] = code->tp_color;
	TS_LOG_INFO("%s, get tp color :0x%02X\n", __func__, cypress_ts_kit_color[0]);
}
static int mxt_get_module_name(void)
{
	int ret = 0;
	struct mxt_data *data = mxt_core_data;
	struct pds_code *code = NULL;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	code = kzalloc(sizeof(struct pds_code), GFP_KERNEL);
	if (!code) {
		TS_LOG_ERR("malloc failed\n");
		ret = -ENOMEM;
		goto out;
	}

	code->module_name = NULL;
	ret = atmel_pds_get_data(data, code);
	if (ret) {
		TS_LOG_ERR("%s failed to read pds\n", __func__);
	}

	if(data->support_get_tp_color) {
		atmel_get_tp_color(data, code);
	}
	memcpy(data->module_id, code->id, MISC_PDS_DATA_LEN);//get 9bytes
out:
	if (code != NULL) {
		kfree(code);
	}

	return ret;
}

static int atmel_chip_get_info(struct ts_chip_info_param *info)
{
	struct mxt_data *data = mxt_core_data;
	struct mxt_object *t38 = NULL;
	u8 version_byte[8] = {0};	/* version len:8*/
	u8 i = 1;
	int ret = 0;

	if (!data || !info || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s return due to null data\n", __func__);
		return -ENODEV;
	}

	t38 = atmel_get_object(data, MXT_SPT_USERDATA_T38);
	if (!t38) {
		TS_LOG_ERR("%s return due to null t38\n", __func__);
		return -EINVAL;
	}

	if (unlikely(
		(atomic_read(&data->atmel_chip_data->ts_platform_data->state) == TS_SLEEP)
		|| (atomic_read(&data->atmel_chip_data->ts_platform_data->state) == TS_WORK_IN_SLEEP))) {
		for(i = 1; i < VERSION_NAME_LEN; i++) {	// the first byte not used
			version_byte[i] = data->ver_bytes[i];
		}
	} else {
		ret =
			__atmel_read_reg(data, t38->start_address, sizeof(version_byte),
				   version_byte);
		if (ret) {
			TS_LOG_ERR("%s failed to read t38\n", __func__);
			return ret;
		}

		ret = mxt_get_module_name();
		if (ret) {
			TS_LOG_ERR("%s failed to get module name\n", __func__);
			return ret;
		}
	}
	/*snprintf(info->chip_name, sizeof(info->chip_name), "%s", "Touch");*/
	snprintf(info->ic_vendor, sizeof(info->ic_vendor), "Atmel-%s",
		data->module_id);
	snprintf(info->fw_vendor, sizeof(info->fw_vendor),
		 "FW%02d.%02x_CFG%02d.%02d_%02d%02d%02d", version_byte[1],
		 version_byte[2], version_byte[3], version_byte[4],
		 version_byte[5], version_byte[6], version_byte[7]);
	snprintf(info->mod_vendor, sizeof(info->mod_vendor), "%s",
		data->module_name);
	TS_LOG_INFO("ic_vendor: %s, fw_vendor: %s, mod_vendor: %s\n",
		    info->ic_vendor, info->fw_vendor, info->mod_vendor);

	return ret;
}

static int atmel_before_suspend(void)
{
	TS_LOG_INFO("%s \n", __func__);

	return 0;
}

static int mxt_atmel_process_messages_until_invalid(struct mxt_data *data)
{
	int count = 0, read = 0;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	count = data->max_reportid;

	/* Read messages until we force an invalid */
	do {
		read = mxt_read_and_atmel_process_messages(data, 1);
		if (read < 1)
			return 0;
	} while (--count);

	TS_LOG_ERR("CHG pin isn't cleared\n");
	return -EBUSY;
}

/*
 *Function for checking t70 messages when the interrupt is disabled.
 *
 *T70 is an object which dynamicly configs the maxTouch devices.
 *
 */
static int mxt_get_t70_msg(struct mxt_data *data)
{
	int i = 0;
	int error = 0;
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	error = atmel_soft_reset(data);
	if(error) {
		TS_LOG_ERR("%s, atmel_soft_reset fail\n", __func__);
		return error;
	}
	error = atmel_t6_command(data, MXT_COMMAND_REPORTALL, 1, false);
	if(error) {
		TS_LOG_ERR("%s, atmel_t6_command fail\n", __func__);
		return error;
	}
	data->t70_msg_processed = 0;//0:t70_msg need process
	mdelay(100);//ic need
	for (i = 0; i < MXT_T70_PROCESS_RETRY; i++) {
		atmel_process_messages(data, MXT_T70_PROCESS_TIMEOUT);
		if (data->t70_msg_processed == 1) {//1:t70_msg process finished
			break;
		}
	}

	if (i == MXT_T70_PROCESS_RETRY) {
		TS_LOG_INFO("mxt_get_t70_msg fail\n");
		return 0;
	}
	return 1;
}

static void mxt_start(struct mxt_data *data, bool resume)
{
	int ret = 0;
	if (!data) {
		TS_LOG_DEBUG("mxt_start exit, param error!\n");
		return;
	}
	if(!data->suspended || data->in_bootloader) {
		TS_LOG_DEBUG("mxt_start exit, suspend:%d, in_bootloader:%d\n",
			     data->suspended, data->in_bootloader);
		return;
	}

	TS_LOG_INFO("mxt_start +\n");

	if (data->sleep_mode == POWER_DOWN_MODE) {
		TS_LOG_DEBUG("atmel regulators enable\n");
		ret = atmel_pinctl_select_normal(data);
		if (ret < 0) {
			TS_LOG_ERR("set iomux normal error, %d\n", ret);
		}
		atmel_power_on(data);
	} else {
		mxt_atmel_process_messages_until_invalid(data);
		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);
		ret = atmel_soft_reset(data);
		if (ret) {
			TS_LOG_ERR("%s, atmel_soft_reset failed!\n", __func__);
		}
		atmel_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);
	}

	data->enable_reporting = true;
	data->suspended = false;
}

static void mxt_stop(struct mxt_data *data, bool suspend)
{
	int ret = 0;

	if (!data) {
		TS_LOG_DEBUG("mxt_stop exit, param error!\n");
		return;
	}
	if (data->suspended || data->in_bootloader) {
		TS_LOG_DEBUG("mxt_stop exit, suspend:%d, in_bootloader:%d\n",
			     data->suspended, data->in_bootloader);
		return;
	}

	TS_LOG_INFO("mxt_stop, called in suspend\n");

	data->enable_reporting = false;

	if (data->sleep_mode == POWER_DOWN_MODE) { // delete g_tp_power_ctrl ?
		TS_LOG_DEBUG("atmel regulators disable\n");
		ret = atmel_pinctl_select_idle(data);
		if (ret < 0) {
			TS_LOG_ERR("set iomux normal error, %d\n", ret);
		}
		atmel_power_off(data);
	} else {
		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);
	}

	if (suspend) {
		if (ret == -EBUSY) {
			TS_LOG_INFO("mxt_stop: set wakeup enable\n");
			/*msleep(200);*/
			mxt_atmel_process_messages_until_invalid(data);
		}
	}

	data->suspended = true;
}

static int atmel_suspend(void)
{
	int error = NO_ERR;
	struct mxt_data *data = mxt_core_data;
	TS_LOG_INFO("atmel_suspend +\n");
	if(!data || !(data->atmel_chip_data)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return error;
	}

	switch (data->atmel_chip_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		/*for in_cell, tp will power off in suspend. */
		if (data->atmel_chip_data->is_in_cell) {
			/*power on and off in lcd */
			mxt_stop(data, true);
		} else {
			mxt_stop(data, true);
		}
		break;
		/*for gesture wake up mode suspend. */
	case TS_GESTURE_MODE:
		break;
	default:
		TS_LOG_ERR("no suspend mode\n");
		return -EINVAL;
	}

	TS_LOG_INFO("atmel_suspend -\n");
	return error;
}

static int atmel_resume(void)
{
	int error = NO_ERR;
	struct mxt_data *data = mxt_core_data;

	TS_LOG_INFO("atmel_resume +\n");
	if(!data || !(data->atmel_chip_data)) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return error;
	}

	switch (data->atmel_chip_data->easy_wakeup_info.sleep_mode) {
	case TS_POWER_OFF_MODE:
		mxt_start(data, true);
		break;
	case TS_GESTURE_MODE:
		break;
	default:
		TS_LOG_ERR("no resume mode\n");
		return -EINVAL;
	}

	TS_LOG_INFO("atmel_resume -\n");
	return error;
}

void atmel_status_resume(void)
{
	struct mxt_data *data = mxt_core_data;
	struct ts_feature_info *info = NULL;

	if(!data|| !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	info = &data->atmel_chip_data->ts_platform_data->feature_info;

	mxt_disable_all_features();
	atmel_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
	msleep(105);// send t6 cmd need wait

	if (MXT_FEATURE_ENABLE == info->glove_info.glove_switch) {
		mxt_enable_feature(data, MAX_GLOVE_CONF);
	}

	if (MXT_FEATURE_ENABLE == info->holster_info.holster_switch) {
		if (MXT_FEATURE_ENABLE == info->glove_info.glove_switch) {
			mxt_disable_feature(data, MAX_GLOVE_CONF);
		}
		mxt_enable_feature(data, MAX_COVER_CONF);
	}

	if (info->roi_info.roi_supported) {
		if (MXT_FEATURE_ENABLE == info->roi_info.roi_switch) {
			mxt_enable_feature(data, MAX_KNUCKLE_CONF);
		}
	}
#if defined(HUAWEI_CHARGER_FB)
	if (info->charger_info.charger_supported) {
		if (MXT_FEATURE_ENABLE == info->charger_info.charger_switch) {
			mxt_enable_feature(data, MAX_WORKAROUND5_CONF);
		}
	}
#endif
}

static int atmel_fw_check_after_resume(void)
{
	int retval = 0;
	struct mxt_data *data = mxt_core_data;
	struct mxt_info *info = NULL;

	if(!data){
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	/*read the info block from the chip again*/
	info = kzalloc(sizeof(struct mxt_info), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("Failed to allocate memory\n");
		return -ENOMEM;
	}
	retval = __atmel_read_reg(data, 0, sizeof(struct mxt_info), info);
	if (retval) {
		TS_LOG_ERR("%s, Failed to get fw info\n", __func__);
		kfree(info);
		return retval;
	}
	/*get low-4bit version and high-4bit versoin */
	TS_LOG_INFO
	    ("Current firmware V%u.%u.%02X . Former firmware V%u.%u.%02X\n",
	     info->version >> 4, info->version & 0xf, info->build,
	     data->info->version >> 4, data->info->version & 0xf,
	     data->info->build);

	if (info->version != data->info->version
	    || info->build != data->info->build) {
		retval = MXT_DRIVER_NEED_UPDATE_MESSAGE;
	}

	kfree(info);
	return retval;
}

static void atmel_dsm_record_chip_status(struct mxt_data *data)
{
	if(!data){
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (atomic_read(&atmel_mxt_moisture_flag)) {
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_IN_MOISTURE_ERROR_NO, "TP IC works in moisture mode,moisture count = %d.\n", data->sta_cnt.moisture_state);
#endif
		atomic_set(&atmel_mxt_moisture_flag, 0);
	}
	if (atomic_read(&atmel_mxt_vnoise_flag)) {
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_IN_VNOISE_ERROR_NO, "TP IC works in very noise mode,vnoise count = %d.\n", data->sta_cnt.vnoise_state);
#endif
		atomic_set(&atmel_mxt_vnoise_flag, 0);
	}
}

static int atmel_after_resume(void *feature_info)
{
	struct mxt_data *data = mxt_core_data;
	int retval = NO_ERR;

	if(!data || !feature_info) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	msleep(100);//after resume need wait > 90ms
	retval = atmel_fw_check_after_resume();
	TS_LOG_INFO("%s + atmel_fw_check_after_resume retval=%d\n", __func__,
		    retval);
	if (MXT_DRIVER_NEED_UPDATE_MESSAGE == retval) {
		retval = mxt_initialize(data);
		if (retval) {
			TS_LOG_ERR("%s, Failed to initialize touch device\n",
				   __func__);
			return retval;
		}
	}
	atmel_status_resume();
	mdelay(10);//ic need
#if defined (CONFIG_HUAWEI_DSM)
	atmel_dsm_record_chip_status(data);
#endif
	TS_LOG_INFO
	    ("%s - status count is: stable: %d, noise: %d, very noise: %d\n",
	     __func__,mxt_core_data->sta_cnt.stable_state,
	    mxt_core_data->sta_cnt.noise_state,mxt_core_data->sta_cnt.vnoise_state);
	return retval;
}

static int mxt_dualX_resume(struct mxt_data *data, u8 *dualX_status)
{
	int retval = NO_ERR;
	if(!data || !dualX_status) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_STABCTRL,
			     dualX_status[STABLE_STATUS]);
	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_NOISCTRL,
			     dualX_status[NOISE_STATUS]);
	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_VNOICTRL,
			     dualX_status[VNOISE_STATUS]);

	TS_LOG_DEBUG
	    ("T72 recover dualX setting: STABLE: %d, NOISE:  %d, VNOISE:  %d\n",
	     dualX_status[STABLE_STATUS], dualX_status[NOISE_STATUS], dualX_status[VNOISE_STATUS]);
	return retval;
}

static int mxt_dualX_disable(struct mxt_data *data, u8 *dualX_status)
{
	int retval = NO_ERR;
	u8 tmp_dualX_status[MAX_DUALX_STATUS] = { 0 };
	if( !data || !dualX_status) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	retval +=
	    mxt_read_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			    MXT_T72_NOISE_SUPPRESSION_STABCTRL, dualX_status);
	retval +=
	    mxt_read_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			    MXT_T72_NOISE_SUPPRESSION_NOISCTRL,
			    dualX_status + NOISE_STATUS);
	retval +=
	    mxt_read_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			    MXT_T72_NOISE_SUPPRESSION_VNOICTRL,
			    dualX_status + VNOISE_STATUS);

	tmp_dualX_status[STABLE_STATUS] =
	    dualX_status[STABLE_STATUS] & (~MXT_T72_NOISE_SUPPRESSION_DUALXMODE);
	tmp_dualX_status[NOISE_STATUS] =
	    dualX_status[NOISE_STATUS] & (~MXT_T72_NOISE_SUPPRESSION_DUALXMODE);
	tmp_dualX_status[VNOISE_STATUS] =
	    dualX_status[VNOISE_STATUS] & (~MXT_T72_NOISE_SUPPRESSION_DUALXMODE);

	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_STABCTRL,
			     tmp_dualX_status[STABLE_STATUS]);
	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_NOISCTRL,
			     tmp_dualX_status[NOISE_STATUS]);
	retval +=
	    mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T72,
			     MXT_T72_NOISE_SUPPRESSION_VNOICTRL,
			     tmp_dualX_status[VNOISE_STATUS]);

	TS_LOG_DEBUG
	    ("T72 original dualX setting is: STABLE: %d, NOISE:  %d, VNOISE:  %d\n",
	     dualX_status[STABLE_STATUS], dualX_status[NOISE_STATUS], dualX_status[VNOISE_STATUS]);
	TS_LOG_DEBUG
	    ("T72 set dualX setting to: STABLE: %d, NOISE:  %d, VNOISE:  %d\n",
	     tmp_dualX_status[STABLE_STATUS], tmp_dualX_status[NOISE_STATUS], tmp_dualX_status[VNOISE_STATUS]);
	return retval;
}

static void atmel_get_threshold_file_name(struct mxt_data *data, char *file_name)
{
	int offset = 0;
	if(!data || !file_name || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	offset = snprintf(file_name, MAX_FILE_NAME_LENGTH, "ts/");
	offset +=
	    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		     data->atmel_chip_data->ts_platform_data->product_name);
	offset +=
	    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		     "_");
	offset +=
	    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		     data->atmel_chip_data->chip_name);
	offset +=
	    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		     "_");
	offset +=
	    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		     data->module_id);
	if(data->fw_need_depend_on_lcd) {
		offset +=
			snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
				"_");
		offset +=
			snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
				data->lcd_module_name);
	}

	snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - offset,
		 "_threshold.csv");
}

static int atmel_parse_threshold_file(void)
{
	int retval = 0;
	const struct firmware *thresholdfile = NULL;
	char file_name[128] = {0};		/*128, file name len */

	struct mxt_data *data = mxt_core_data;
	if(!data || !data->atmel_dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	atmel_get_threshold_file_name(data, file_name);
	TS_LOG_INFO("%s, file name: %s\n", __func__, file_name);
	retval = request_firmware(&thresholdfile, file_name, &data->atmel_dev->dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request %s\n", __func__, file_name);
		return -EINVAL;
	}
	if (thresholdfile == NULL) {
		TS_LOG_ERR("%s: thresholdfile == NULL\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	if (thresholdfile->data == NULL || thresholdfile->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	TS_LOG_INFO("%s: thresholdfile->size = %zu\n", __func__, thresholdfile->size);

	TS_LOG_INFO("%s: Found threshold file.\n", __func__);

	retval = atmel_parse_threshold_file_method(&thresholdfile->data[0], thresholdfile->size);
	if (retval < 0) {
		TS_LOG_ERR("%s: Parse Cmcp file\n", __func__);
		retval = -EINVAL;
		goto exit;
	}

exit:
	release_firmware(thresholdfile);
	return retval;
}

static int atmel_parse_threshold_file_method(const char *buf, uint32_t file_size)
{
	int index_count = 0, index_count1 = 0, index_count2 = 0;
	struct mxt_data *data = mxt_core_data;
	int retval = 0;
	int case_num = 0;
	int get_even_num = 0;
	int rx = 0;
	int tx = 0;
	int rawdata_size = 0, tx2tx_size = 0, rx2rx_size = 0, noise_size = 0;
	int buf_offset = 0;
	int m=(int)file_size;

	if(!data || !buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	rx = (int)(data->y_size - data->y_origin);
	tx = (int)(data->x_size - data->x_origin);
	rawdata_size = rx * tx * 2; //up & low limit
	tx2tx_size = (tx - 1) * rx; //calculate diff need tx -1
	rx2rx_size = (rx - 1) * tx; //calculate diff need rx -1
	noise_size = tx * rx;
	TS_LOG_INFO("%s: rawdata:array_size = %d rx =%d tx =%d \n", __func__, rawdata_size,rx,tx);
	TS_LOG_INFO("%s: tx2tx_size:%d rx2rx_size:%d noise_size:%d\n", __func__,
		tx2tx_size, rx2rx_size, noise_size);
	if(!rawdata_size || !tx2tx_size || !rx2rx_size || !noise_size){
		TS_LOG_ERR("%s, limit size invalid\n", __func__);
		return -1;
	}
	for(case_num = 0;case_num < MAX_CASE_CNT; case_num++) {

		switch(case_num){
			case RAWDATA_LIMIT:  //data format :  ABABABAB    A:low, B: up
				index_count1 = 0;
				index_count2 = 0;
				for(index_count = 0; index_count < rawdata_size; ){
					retval = atmel_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						if(0 == get_even_num) {  //get low limit
							data->rawdata_low_limit_buf[index_count1++] = retval;
							get_even_num = 1;
						} else {       //get up limit
							data->rawdata_up_limit_buf[index_count2++] = retval;
							get_even_num = 0;
						}
						TS_LOG_DEBUG("%s: rawdata = %d, index_count=%d\n", __func__, retval, index_count);
						index_count++;
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;
			case RAWDATA_TXTX_DELTA_LIMIT:
				index_count1 = 0;
				index_count2 = 0;
				for(index_count = 0; index_count < tx2tx_size; )
				{
					retval = atmel_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						data->rawdata_tx2tx_low_limit_buf[index_count1++] = 0 - retval;
						data->rawdata_tx2tx_up_limit_buf[index_count2++] = retval;
						index_count++;
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;//break;
			case RAWDATA_RXRX_DELTA_LIMIT:
				index_count1 = 0;
				index_count2 = 0;
				for(index_count = 0; index_count < rx2rx_size; )
				{
					retval = atmel_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						data->rawdata_rx2rx_low_limit_buf[index_count1++] = 0 - retval;
						data->rawdata_rx2rx_up_limit_buf[index_count2++] = retval;
						index_count++;
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;//break;
			case NOISE_LIMIT:
				index_count1 = 0;
				index_count2 = 0;
				for(index_count = 0; index_count < noise_size; )
				{
					retval = atmel_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						data->noise_low_limit_buf[index_count1++] = 0 - retval;
						data->noise_up_limit_buf[index_count2++] = retval;
						index_count++;
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;//break;
			case RAWDATA_MIN_MAX_DELTA_LIMIT:
				for(index_count = 0; index_count < MAX_RAWDATA_MIN_MAX_DELTA_LIMIT_COUNT; )
				{
					retval = atmel_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						data->max_minus_min_limit= retval;
						index_count++;
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;
		}
	}
	return retval;
}

static int atmel_get_one_value(const char *buf, uint32_t *offset)
{
	int value = -1;
	char tmp_buffer[10] = {0};		/*10,buffer len*/
	uint32_t count = 0;
	uint32_t tmp_offset = 0;
	int m=0,n=0;

	if(!buf || !offset) {
		TS_LOG_ERR("%s,param invalid\n", __func__);
		return -EINVAL;
	}
	tmp_offset = *offset;
	/* Bypass extra commas */
	m=tmp_offset + 1;
	while (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_COMMA)
	{
		tmp_offset++;
		m=tmp_offset + 1;
	}
	/* Windows and Linux difference at the end of one line */
	m=tmp_offset + 1;
	n=tmp_offset + 2;
	if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_CR
			&& buf[n] == ASCII_LF)
		tmp_offset += 2;
	else if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_LF)
		tmp_offset += 1;

	/* New line for multiple lines start*/
	m=tmp_offset + 1;
	if (buf[tmp_offset] == ASCII_LF && buf[m] == ASCII_COMMA) {
		tmp_offset++;
		m=tmp_offset + 1;
//		line_count++;
		/*dev_vdbg(dev, "\n");*/
	}
	if (buf[tmp_offset] == ASCII_LF && buf[m]!= ASCII_COMMA) {
		for(;;){
			tmp_offset++;
			m=tmp_offset + 1;
			if (buf[m] == ASCII_COMMA) {
				break;
			}
		}
	}
	/* New line for multiple lines end*/
	/* Beginning */
	if (buf[tmp_offset] == ASCII_COMMA) {
		tmp_offset++;
		for (;;) {
			if ((buf[tmp_offset] >= ASCII_ZERO)
			&& (buf[tmp_offset] <= ASCII_NINE)) {
				tmp_buffer[count++] = buf[tmp_offset] - ASCII_ZERO;
				tmp_offset++;
			} else {
				if (count != 0) {
					value = atmel_ctoi(tmp_buffer,count);
					/*dev_vdbg(dev, ",%d", value);*/
				} else {
					/* 0 indicates no data available */
					value = -1;
				}
				break;
			}
		}
	} else {
	/*do plus outside tmp_offset++;*/
	}

	*offset = tmp_offset;

	return value;
}

static int atmel_ctoi(char *buf, uint32_t count)
{
	int value = 0;
	uint32_t index = 0;
	u32 base_array[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};

	if(!buf) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	for (index = 0; index < count; index++)
		value += buf[index] * base_array[count - 1 - index];
	return value;
}

static int atmel_malloc_limit_buf(struct mxt_data *data)
{
	int retval = NO_ERR;
	int tx_num  = 0, rx_num = 0;
	int rawdata_size = 0, tx2tx_size =0, rx2rx_size = 0, noise_size = 0;

	if(!data) {
		TS_LOG_INFO("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	tx_num = (int)(data->x_size - data->x_origin);
	rx_num = (int)(data->y_size - data->y_origin);
	rawdata_size = tx_num * rx_num;
	tx2tx_size = (tx_num - 1) * rx_num;
	rx2rx_size = (rx_num - 1) * tx_num;
	noise_size = tx_num * rx_num;
	TS_LOG_INFO("%s, tx2tx_size = %d, rx2rx_size = %d\n", __func__, tx2tx_size, rx2rx_size);

	data->rawdata_low_limit_buf = kzalloc(rawdata_size * sizeof(int), GFP_KERNEL);
	data->rawdata_up_limit_buf = kzalloc(rawdata_size * sizeof(int), GFP_KERNEL);

	data->rawdata_rx2rx_low_limit_buf= kzalloc(rx2rx_size * sizeof(int), GFP_KERNEL);
	data->rawdata_rx2rx_up_limit_buf = kzalloc(rx2rx_size * sizeof(int), GFP_KERNEL);

	data->rawdata_tx2tx_up_limit_buf = kzalloc(tx2tx_size * sizeof(int), GFP_KERNEL);
	data->rawdata_tx2tx_low_limit_buf = kzalloc(tx2tx_size * sizeof(int), GFP_KERNEL);

	data->noise_low_limit_buf = kzalloc(noise_size * sizeof(int), GFP_KERNEL);
	data->noise_up_limit_buf = kzalloc(noise_size * sizeof(int), GFP_KERNEL);
	if( !data->rawdata_low_limit_buf || !data->rawdata_up_limit_buf
		|| !data->rawdata_rx2rx_low_limit_buf || !data->rawdata_rx2rx_up_limit_buf
		|| !data->rawdata_tx2tx_up_limit_buf || !data->rawdata_tx2tx_low_limit_buf
		|| !data->noise_low_limit_buf || !data->noise_up_limit_buf) {
		TS_LOG_ERR("%s, request memory failed\n", __func__);
		return -ENOMEM;
	}

	TS_LOG_INFO("%s, requset memory success\n",__func__);
	return retval;
}


static int atmel_get_rawdata(struct ts_rawdata_info *info,
			     struct ts_cmd_node *out_cmd)
{
	struct mxt_data *data = mxt_core_data;
	int retval = NO_ERR;
	int ret1 = 0, ret2 = 0, ret3= 0;
	int cmd = 0;
	int cmd_T25 = 0;
	size_t offset = 0;
	size_t T37_buf_size = 0;
	int max_minus_min_limit = 0;
	char max_min_aver_buf[MAX_BUF_SIZE] = { 0 };
	int retry_count = RETRY_TEST_TIME;
	u8 dualX_status[MAX_DUALX_STATUS] = { 0 };

#ifdef ROI
 /**ATTENTION : Should be set to MXT_T6_CMD58_ON in each return branch**/
	atmel_gMxtT6Cmd58Flag = MXT_T6_CMD58_OFF;
#endif
	if(!data || !info || !out_cmd) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		retval = -EINVAL;
		goto exit;
	}
	if(atomic_read(&atmel_mmi_test_status)){
		TS_LOG_ERR("%s factory test already has been called.\n",__func__);
		retval = -EINVAL;
		goto exit;
	}

	atomic_set(&atmel_mmi_test_status, 1);
	wake_lock(&data->ts_flash_wake_lock);
	retval = mxt_dualX_disable(data, dualX_status);
	if (retval) {
		TS_LOG_ERR("mxt_dualX_disable error\n");
		memcpy(info->result, "0F", strlen("0F"));
		goto out;
	}
	memcpy(info->result, "0P", strlen("0P"));

	msleep(50);//ic need
	if(data->do_calibration) {
		atmel_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);
		TS_LOG_INFO("%s, do_calibration\n", __func__);
	}
	msleep(50);//ic need

	retval = mxt_read_t100_config(data);
	if (retval) {
		TS_LOG_ERR("mxt_read_t100_config error\n");
		goto out;
	}
	retval = atmel_malloc_limit_buf(data);
	if (retval) {
		TS_LOG_ERR("atmel_malloc_limit_buf error\n");
		goto out;
	}
	T37_buf_size =
	    (size_t)(data->x_size - data->x_origin) * (data->y_size -
					       data->y_origin) * sizeof(int);

	TS_LOG_DEBUG
	    ("%s: x = %d , y = %d\n Read ref*********************************\n",
	     __func__, (data->info->matrix_xsize), (data->info->matrix_ysize));
	data->T37_buf_size = T37_buf_size;
	data->T37_buf = kzalloc(data->T37_buf_size, GFP_KERNEL);
	if (!data->T37_buf) {
		TS_LOG_ERR("alloc T37 buf failed\n");
		retval = -ENOMEM;
		goto out;
	}

	retval = atmel_parse_threshold_file();
	if (retval < 0) {
		TS_LOG_ERR("atmel_parse_threshold_file error\n");
		goto out;
	}
	TS_LOG_INFO("get atmel_parse_threshold_file success\n");
	max_minus_min_limit = data->max_minus_min_limit;

	info->buff[0] = (int)(data->y_size - data->y_origin);	/*rx*/
	info->buff[1] = (int)(data->x_size - data->x_origin);	/*tx*/
	cmd = MXT_T6_DEBUG_REF;

	TS_LOG_DEBUG
	    ("\n\n\n%s: x = %d , y = %d\n Read del*********************************\n",
	     __func__, (data->info->matrix_xsize), (data->info->matrix_ysize));

	do {
		retval = atmel_T37_fetch(data, cmd);
		if (!retval) {
			TS_LOG_INFO("atmel_T37_fetch read success\n");
			break;
		}
		mdelay(5);//ic need
		TS_LOG_INFO("try remainning %d times\n", retry_count - 1);
	} while (retry_count--);
	if (retval) {
		TS_LOG_ERR("atmel_T37_fetch read failed,already try %d times\n",
			   RETRY_TEST_TIME - retry_count);
		goto out;
	}

	ret1 = atmel_get_rawdata_test(data);
	if (ret1) {
		strncat(info->result, "-1P", strlen("-1P"));
	} else {
		TS_LOG_ERR("raw data test failed\n");
		strncat(info->result, "-1F", strlen("-1F"));
	}
	TS_LOG_INFO("rawdata test finish\n");
	data->refs_delta_data.refs_data_Average = data->refs_delta_data.Average;
	data->refs_delta_data.refs_data_MaxNum = data->refs_delta_data.MaxNum;
	data->refs_delta_data.refs_data_MinNum = data->refs_delta_data.MinNum;

	memcpy(&info->buff[2], data->T37_buf, data->T37_buf_size);

	ret1 =
		atmel_get_refs_rx2rx_delta_test(data);
	ret2 =
		atmel_get_refs_tx2tx_delta_test(data);
	ret3 = atmel_get_refs_max_minus_min_test(data, max_minus_min_limit);
	if (ret1 && ret2 && ret3) {
		strncat(info->result, "-2P", strlen("-2P"));
	} else {
		TS_LOG_ERR("delta test failed\n");
		strncat(info->result, "-2F", strlen("-2F"));
	}
	TS_LOG_INFO("rawdata delta test finish\n");
	retval = mxt_dualX_resume(data, dualX_status);
	if (retval) {
		TS_LOG_ERR("mxt_dualX_resume error\n");
		goto out;
	}
	msleep(150);//delay 150ms

	offset = (data->T37_buf_size) / (sizeof(int));
	cmd = MXT_T6_DEBUG_DELTA;
	memset(data->T37_buf, 0, data->T37_buf_size);

	do {
		retval = atmel_T37_fetch(data, cmd);
		if (!retval) {
			TS_LOG_INFO("atmel_T37_fetch read success\n");
			break;
		}
		mdelay(5);//ic need
		TS_LOG_INFO("try remainning %d times\n", retry_count - 1);
	} while (retry_count--);

	retval =
	    atmel_get_refs_or_deltas_data_test(data);
	if (retval) {
		strncat(info->result, "-3P", strlen("-3P"));
	} else {
		TS_LOG_ERR("noise test failed\n");
		strncat(info->result, "-3F", strlen("-3F"));
	}
	TS_LOG_INFO("noise test finish\n");
	if(0 != data->T124_address) {
		mxt_get_t70_msg(data); //incell TP need wait lcd
		if ((MXT_SYNC_OK != data->vsyncstatus)
			|| (MXT_SYNC_OK != data->hsyncstatus)) {
			TS_LOG_ERR("Sync lost, V%d, H%d\n", data->vsyncstatus,
			data->hsyncstatus);
			memset(&info->buff[2], -1, data->T37_buf_size);
		}
	}
	memcpy(&info->buff[2 + offset], data->T37_buf, data->T37_buf_size);

	cmd_T25 = MXT_SELF_TEST_PIN_FAULT;
	retval = atmel_t25_selftest_polling(data, cmd_T25);
	if(retval) {
		strncat(info->result, "-4P", strlen("-4P"));
	} else {
		strncat(info->result, "-4F", strlen("-4F"));
	}

	retval = NO_ERR;
	data->refs_delta_data.deltas_data_Average =
	    data->refs_delta_data.Average;
	data->refs_delta_data.deltas_data_MaxNum = data->refs_delta_data.MaxNum;
	data->refs_delta_data.deltas_data_MinNum = data->refs_delta_data.MinNum;

	if(0 != data->T124_address) {
		mxt_get_t70_msg(data);
		if ((MXT_SYNC_OK != data->vsyncstatus)
			|| (MXT_SYNC_OK != data->hsyncstatus)) {
			TS_LOG_ERR("Sync lost, V%d, H%d\n", data->vsyncstatus,
			data->hsyncstatus);
			memset(&info->buff[2], -1, data->T37_buf_size);
		}
	}

	info->used_size = (info->buff[0]) * (info->buff[1]) * 2 + 2;

	info->used_sharp_selcap_single_ended_delta_size = 0;
	info->used_sharp_selcap_touch_delta_size = 0;

	atmel_get_average_max_min_data(data, max_min_aver_buf);
	strncat(info->result, max_min_aver_buf, strlen(max_min_aver_buf));

	strncat(info->result, ";", strlen(";"));
	if (0 == strlen(info->result) || strstr(info->result, "F")) {
		strncat(info->result, "panel_reason", strlen("panel_reason"));
	}
	if(mxt_core_data->description[0] != '\0') {
		strncat(info->result, "-", strlen("-"));
		strncat(info->result, mxt_core_data->description,
			strlen(mxt_core_data->description));
	} else
		strncat(info->result, "-atmel_touch", strlen("-atmel_touch"));

out:
	wake_unlock(&data->ts_flash_wake_lock);
	atomic_set(&atmel_mmi_test_status, 0);

	if (data->T37_buf) {
		kfree(data->T37_buf);
		data->T37_buf = NULL;
		data->T37_buf_size = 0;
	}
	/*free rawdata limit buf*/
	if(data->rawdata_low_limit_buf) {
		kfree(data->rawdata_low_limit_buf);
		data->rawdata_low_limit_buf = NULL;
	}
	if(data->rawdata_up_limit_buf) {
		kfree(data->rawdata_up_limit_buf);
		data->rawdata_up_limit_buf = NULL;
	}

	/*free rawdata_rx2rx limit buf*/
	if(data->rawdata_rx2rx_low_limit_buf) {
		kfree(data->rawdata_rx2rx_low_limit_buf);
		data->rawdata_rx2rx_low_limit_buf = NULL;
	}
	if(data->rawdata_rx2rx_up_limit_buf) {
		kfree(data->rawdata_rx2rx_up_limit_buf);
		data->rawdata_rx2rx_up_limit_buf = NULL;
	}

	/*free rawdata_tx2tx limit buf*/
	if(data->rawdata_tx2tx_low_limit_buf) {
		kfree(data->rawdata_tx2tx_low_limit_buf);
		data->rawdata_tx2tx_low_limit_buf = NULL;
	}
	if(data->rawdata_tx2tx_up_limit_buf) {
		kfree(data->rawdata_tx2tx_up_limit_buf);
		data->rawdata_tx2tx_up_limit_buf = NULL;
	}

	/*free noise limit buf*/
	if(data->noise_low_limit_buf) {
		kfree(data->noise_low_limit_buf);
		data->noise_low_limit_buf = NULL;
	}
	if(data->noise_up_limit_buf) {
		kfree(data->noise_up_limit_buf);
		data->noise_up_limit_buf = NULL;
	}
	if (retval) {
		TS_LOG_ERR("all three tests failed\n");
		strncat(info->result, "-1F-2F-3F", strlen("-1F-2F-3F"));
	}
exit:
#ifdef ROI
	atmel_gMxtT6Cmd58Flag = MXT_T6_CMD58_ON;
#endif
	TS_LOG_INFO("%s, %s\n", __func__, info->result);
	return retval;
}

static void atmel_shutdown(void)
{
	int error = NO_ERR;
	struct mxt_data *data = mxt_core_data;

	error = atmel_pinctl_select_idle(data);
	if (error < 0) {
		TS_LOG_ERR("set iomux normal error, %d\n", error);
	}

	atmel_power_off(data);
}

static int atmel_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int error = NO_ERR;
	struct mxt_data *data = mxt_core_data;

	if (!info || !data || !(data->atmel_chip_data)) {
		TS_LOG_ERR("%s:info=%ld\n", __func__, PTR_ERR(info));
		error = -ENOMEM;
		return error;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type, data->atmel_chip_data->tp_test_type,
		       TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("read_chip_get_test_type=%s, \n",
			    info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid status: %s", info->tp_test_type);
		error = -EINVAL;
		break;
	}
	return error;
}

/*
 *Function for parsing feature file.
 *
 *Feature file is used to store feature configuration for the maxtouch devices.
 *
 */
static int mxt_search_string(const char *orignalstring, int len, char c)
{
	int count = 0;
	int i = 0;

	if(!orignalstring) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++) {
		if (orignalstring[i] == c)
			count++;
	}
	return count;
}

static void mxt_print_feature_info(const char* func_name,
						struct feature_info* feature)
{
	int i = 0;
	if(!feature || !func_name) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	for (i = 0; i < feature->reg_num; i++) {
		TS_LOG_DEBUG
		    ("%s, objectId: %d, instance: %d, offset: %d, newvalue: %d, oldvalue: %d\n",
		     func_name, feature->reg_value[i].objectId,
		     feature->reg_value[i].instance,
		     feature->reg_value[i].offset,
		     feature->reg_value[i].newvalue,
		     feature->reg_value[i].oldvalue);
	}
}

static void mxt_get_feature_file_name(char *feature_file_name, bool from_sd)
{
	int offset = 0;
	struct mxt_data *data = mxt_core_data;

	if(!data || !feature_file_name || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	if (!from_sd) {
		offset = snprintf(feature_file_name, PAGE_SIZE, "ts/atmel/");
		if (data->description[0] != '\0') {
			offset +=
			    snprintf(feature_file_name + offset,
				     PAGE_SIZE - offset, data->description);
		} else {
			offset +=
			    snprintf(feature_file_name + offset,
				     PAGE_SIZE - offset,
				     data->atmel_chip_data->ts_platform_data->product_name);
		}
		snprintf(feature_file_name + offset, PAGE_SIZE - offset,
			 "_config_feature.raw");
	} else {
		offset =
		    snprintf(feature_file_name, PAGE_SIZE,
			     "ts/atmel_config_feature.raw");
	}
	TS_LOG_INFO("%s: feature_file_name is %s\n", __func__,
		    feature_file_name);
}

static int mxt_write_file_config_for_feature(struct feature_info *feature)
{
	struct mxt_data *data = mxt_core_data;
	int i = 0;
	int ret = NO_ERR;

	if(!data || !feature) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < feature->reg_num; i++) {
		ret = mxt_write_obj_instance(data,
					     feature->reg_value[i].objectId,
					     feature->reg_value[i].instance,
					     feature->reg_value[i].offset,
					     (feature->reg_value[i].newvalue));
		if (ret) {
			TS_LOG_ERR("%s, enable function failed\n", __func__);
			feature->on_flag = MXT_FEATURE_SETERROR;
			goto out;
		}
	}
	mxt_print_feature_info(__func__, feature);

out:
	return ret;
}

static int mxt_write_dev_config_for_feature(struct feature_info *feature)
{
	struct mxt_data *data = mxt_core_data;
	int i = 0;
	int ret = NO_ERR;

	if(!data || !feature) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < feature->reg_num; i++) {

		ret = mxt_write_obj_instance(data,
					     feature->reg_value[i].objectId,
					     feature->reg_value[i].instance,
					     feature->reg_value[i].offset,
					     (feature->reg_value[i].oldvalue));
		if (ret) {
			TS_LOG_ERR("%s, disable function failed\n", __func__);
			feature->on_flag = MXT_FEATURE_SETERROR;
			goto out;
		}
	}
	mxt_print_feature_info(__func__, feature);

out:
	return ret;
}

static int mxt_store_dev_config_for_feature(struct feature_info *feature)
{
	struct mxt_data *data = mxt_core_data;
	int ret = NO_ERR;
	int i = 0;

	if(!data || !feature) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < feature->reg_num; i++) {
		ret = mxt_read_obj_instance(data,
					    feature->reg_value[i].objectId,
					    feature->reg_value[i].instance,
					    feature->reg_value[i].offset,
					    &(feature->reg_value[i].oldvalue));
		if (ret) {
			TS_LOG_ERR("%s, read tp register error!", __func__);
			goto out;
		}
	}
	mxt_print_feature_info(__func__, feature);

out:
	return ret;
}

static void parse_extra_feature_cover(struct feature_info *feature)
{
	int i = 0;

	if(!feature) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	for (i = 0; i < feature->reg_num; i++) {
		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38COVERGLASSFL
		    && feature->reg_value[i].newvalue == 1) {
			mxt_core_data->feature_extra_data.cover_glass = true;
		}

		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38COVERCUT
		    && feature->reg_value[i].newvalue != 0) {
			mxt_core_data->feature_extra_data.cover_cut = true;
		}

		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38COVERSUPWIDL) {
			mxt_core_data->feature_extra_data.cover_sup_l =
			    feature->reg_value[i].newvalue;
		}

		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38COVERSUPWIDH) {
			mxt_core_data->feature_extra_data.cover_sup_h =
			    feature->reg_value[i].newvalue;
		}
	}

	TS_LOG_INFO
	    ("%s: cover_glass is %d, cover_cut, and h, l is %d(%03d,%03d)\n",
	     __func__, mxt_core_data->feature_extra_data.cover_glass,
	     mxt_core_data->feature_extra_data.cover_cut,
	     mxt_core_data->feature_extra_data.cover_sup_h,
	     mxt_core_data->feature_extra_data.cover_sup_l);
}

static void parse_extra_feature_glove(struct feature_info *feature)
{
	int i = 0;
	if(!feature) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	for (i = 0; i < feature->reg_num; i++) {
		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38GLOVESUPWIDL) {
			mxt_core_data->feature_extra_data.glove_sup_l =
			    feature->reg_value[i].newvalue;
		}

		if (feature->reg_value[i].objectId == MXT_SPT_USERDATA_T38
		    && feature->reg_value[i].offset == T38GLOVESUPWIDH) {
			mxt_core_data->feature_extra_data.glove_sup_h =
			    feature->reg_value[i].newvalue;
		}
	}

	TS_LOG_INFO("%s, glove_sup h, l is (%03d,%03d)\n", __func__,
		    mxt_core_data->feature_extra_data.glove_sup_h,
		    mxt_core_data->feature_extra_data.glove_sup_l);
}

static void parse_extra_feature(void)
{
	struct mxt_data *data = mxt_core_data;
	struct ts_feature_info *info = NULL;

	if(!data || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	info = &data->atmel_chip_data->ts_platform_data->feature_info;
	if(info->glove_info.glove_supported)
		parse_extra_feature_glove(&mxt_feature_list[MAX_GLOVE_CONF]);
	if(info->holster_info.holster_supported)
		parse_extra_feature_cover(&mxt_feature_list[MAX_COVER_CONF]);
}

static int mxt_parse_feature_file(const struct firmware *cfg)
{
	int featurecode = 0, feature_list_count = 0;
	char feature_begin[32] = {0};
	char feature_end[32] = {0};
	int begin_pos = 0, end_pos = 0, read_pos = 0, regnum = 0;
	int readobjectId = 0, readinstance = 0, readoffset = 0, readnewvalue = 0;
	int i = 0, ret = NO_ERR;
	struct feature_info *feature = NULL;

	char *tmpbuf = NULL;
	if(!cfg) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	tmpbuf = vmalloc(cfg->size+1);
	if (!tmpbuf) {
		TS_LOG_ERR("%s mxt_parse_feature_file  tmpbuf fail\n",__func__);
		return -ENOMEM;
	}
	memcpy(tmpbuf, cfg->data, cfg->size);
	tmpbuf[cfg->size] = '\0';
	feature_list_count = sizeof(mxt_feature_list)/sizeof(struct feature_info);
	for( featurecode = 0 ; featurecode <  feature_list_count; featurecode++){
		feature = &mxt_feature_list[featurecode];
		//get begin/end strings
		snprintf(feature_begin, sizeof(feature_begin), "[%s BEGIN]", feature->name);
		snprintf(feature_end, sizeof(feature_end), "[%s END]", feature->name);
		TS_LOG_DEBUG("feature_name: %s, feature_begin: %s,  feature_end: %s\n", feature->name, feature_begin, feature_end);

		//find begin/end postions
		begin_pos =strstr(tmpbuf, feature_begin) -  (char *)(tmpbuf);
		end_pos = strstr(tmpbuf , feature_end) - (char *)(tmpbuf);
		if (-1 == begin_pos || -1 == end_pos){
		    TS_LOG_ERR("Failure find %s/%s in file\n", feature_begin, feature_end);
			ret = -EINVAL;
			goto out;
		}

		read_pos = begin_pos + strlen(feature_begin) + 2;
		regnum = mxt_search_string(tmpbuf + read_pos, end_pos - read_pos, ';');
		TS_LOG_DEBUG("%s, begin_pos: %d, end_pos: %d, read_pos: %d, regnum: %d, old regnum :%d \n", __func__, begin_pos, end_pos, read_pos, regnum, feature->reg_num);

		if (NULL == feature->reg_value) {
			feature->reg_value =
			    kzalloc(regnum * sizeof(struct feature_reg_value),
				    GFP_KERNEL);
			if (NULL == feature->reg_value) {
				ret = -EINVAL;
				goto out;
			}
			TS_LOG_DEBUG("kzalloc for feature_value_point %d\n",
				     featurecode);
		} else {
			TS_LOG_DEBUG
			    ("feature_value_point %d had been malloced\n",
			     featurecode);
		}
		feature->reg_num = regnum;

		/*read feature setting*/
		for (i = 0; i < regnum; i++) {
			if (sscanf(tmpbuf + read_pos, "OBJECT=T%d,INSTANCE=%d,OFFSIZE=%d,VALUE=%d;\n",
				&readobjectId, &readinstance, &readoffset, &readnewvalue) != 4) {		/* get 4 para */
				ret = -EINVAL;
				goto out;
			}
			feature->reg_value[i].objectId = (u8)readobjectId;
			feature->reg_value[i].instance = (u8)readinstance;
			feature->reg_value[i].offset = (u8)readoffset;
			feature->reg_value[i].newvalue = (u8)readnewvalue;
			/*skip a line*/
			while( tmpbuf[++read_pos] != '\n') {
				if (tmpbuf[read_pos] == '\0') {
					ret = -EINVAL;
					goto out;
				}
			}

			read_pos++;
		}
	}

out:
	vfree(tmpbuf);
	return ret;
}

static void mxt_init_mxt_feature_list(void)
{
	int i = 0, num = 0;
	struct feature_info *feature = NULL;

	num = sizeof(mxt_feature_list) / sizeof(struct feature_info);
	for (i = 0; i < num; i++) {
		feature = &mxt_feature_list[i];
		if (NULL != feature->reg_value) {
			kfree(feature->reg_value);
			feature->reg_value = NULL;
		}
	}
}

static int mxt_read_feature_file(bool from_sd)
{
	struct mxt_data *data = mxt_core_data;
	char feature_file_name[128] = {0};
	const struct firmware *cfg = NULL;
	int ret = NO_ERR;

	if(!data || !data->atmel_dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	mxt_get_feature_file_name(feature_file_name, from_sd);
	ret = request_firmware(&cfg, feature_file_name, &data->atmel_dev->dev);
	if (ret < 0) {
		TS_LOG_ERR("Failure to request file %s\n", feature_file_name);
		featurefile_read_success = FEATUREFILE_NOT_READ;
		return ret;
	}

	mxt_init_mxt_feature_list();

	ret = mxt_parse_feature_file(cfg);
	if (ret) {
		TS_LOG_ERR("Failure to mxt_parse_feature_file\n");
		goto release;
	}
	featurefile_read_success = FEATUREFILE_READ_SUCCESS;
	TS_LOG_INFO("%s return OK\n", __func__);
release:
	release_firmware(cfg);
	return ret;
}

static int atmel_init_feature_file(bool from_sd)
{
	int ret = 0;
	ret = mxt_read_feature_file(from_sd);
	if (ret) {
		TS_LOG_ERR("%s mxt_read_feature_file fail\n", __func__);
		goto out;
	}
	parse_extra_feature();

out:
	return ret;
}

/*
 *Function for setting up extra features, such as glove, gesture, and kuckle knock.
 *
 *Different features have different configs, so the configs need updating to the mxt device when switching them on/off.
 *The config are stored in a file named XXX_config_feature.raw
 *
 */
static int mxt_set_feature(struct mxt_data *data, int featurecode,
			   int enableflag)
{
	int ret = NO_ERR;
	struct feature_info *feature = &mxt_feature_list[featurecode];

	if (featurefile_read_success == 0) {
		ret = atmel_init_feature_file(false);
		if (ret) {
			TS_LOG_ERR("%s mxt_read_feature_file fail from sd\n",
				   __func__);
			goto out;
		}
	}

	if (enableflag == MXT_FEATURE_ENABLE) {
		if (MXT_FEATURE_DISABLE == feature->on_flag) {
			ret = mxt_store_dev_config_for_feature(feature);
			if (ret) {
				TS_LOG_ERR
				    ("%s mxt_store_dev_config_for_feature fail\n",
				     __func__);
				goto out;
			}
		}
		ret = mxt_write_file_config_for_feature(feature);
		if (ret) {
			TS_LOG_ERR
			    ("%s mxt_write_file_config_for_feature fail\n",
			     __func__);
			goto out;
		}
	} else {
		if (MXT_FEATURE_ENABLE == feature->on_flag) {
			ret = mxt_write_dev_config_for_feature(feature);
			if (ret) {
				TS_LOG_ERR
				    ("%s mxt_write_dev_config_for_feature fail\n",
				     __func__);
				goto out;
			}
		}
	}

out:
	TS_LOG_INFO
	    ("%s set func %s return %s, switch flag: %d, previouse flag: %d\n",
	     __func__, feature->name, ret ? "NG" : "OK", enableflag,
	     feature->on_flag);

	if (!ret) {
		feature->on_flag = enableflag;
	}

	return ret;
}

static int mxt_enable_feature(struct mxt_data *data, int featurecode)
{
	return mxt_set_feature(data, featurecode, MXT_FEATURE_ENABLE);
}

static int mxt_disable_feature(struct mxt_data *data, int featurecode)
{
	return mxt_set_feature(data, featurecode, MXT_FEATURE_DISABLE);
}

static void mxt_disable_all_features(void)
{
	int i = 0;
	int feature_list_count = 0;

	feature_list_count =
	    sizeof(mxt_feature_list) / sizeof(struct feature_info);
	for (i = 0; i < feature_list_count; i++) {
		if (MXT_FEATURE_ENABLE == mxt_feature_list[i].on_flag) {
			mxt_disable_feature(mxt_core_data, i);
		}
	}
}
#if 0
static void mxt_t72noise_switch(u8 t72noise_switch)
{
	struct mxt_data *data = mxt_core_data;

	switch (t72noise_switch) {
	case MXT_FEATURE_ENABLE:
		memset(&mxt_core_data->t72_noise_level, 0,
		       sizeof(struct t72_noise_level));
		mxt_enable_feature(data, MAX_WORKAROUND3_CONF);
		break;
	case MXT_FEATURE_DISABLE:
		mxt_disable_feature(data, MAX_WORKAROUND3_CONF);
		break;
	default:
		break;
	}
	return;
}
#endif
static void mxt_charger_switch(u8 charger_switch)
{
	struct mxt_data *data = mxt_core_data;

	switch (charger_switch) {
	case MXT_FEATURE_ENABLE:
		mxt_enable_feature(data, MAX_WORKAROUND5_CONF);
		break;
	case MXT_FEATURE_DISABLE:
		mxt_disable_feature(data, MAX_WORKAROUND5_CONF);
		break;
	default:
		break;
	}
	return;
}

static int atmel_charger_switch(struct ts_charger_info *info)
{
	int retval = NO_ERR;

#if defined(HUAWEI_CHARGER_FB)
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	if(!info->charger_supported) {
		TS_LOG_ERR("%s , not support charger_switch\n", __func__);
		return NO_ERR;
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		mxt_charger_switch(info->charger_switch);
		break;
	default:
		TS_LOG_INFO("%s, invalid cmd\n", __func__);
		retval = -EINVAL;
		break;
	}
#endif

	return retval;
}

static void mxt_glove_switch(u8 glove_switch)
{
	struct mxt_data *data = mxt_core_data;
	struct ts_feature_info *info = NULL;

	if(!data || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	info = &data->atmel_chip_data->ts_platform_data->feature_info;
	switch (glove_switch) {
	case MXT_FEATURE_ENABLE:
		mxt_enable_feature(data, MAX_GLOVE_CONF);
		break;
	case MXT_FEATURE_DISABLE:
		mxt_disable_all_features();
		atmel_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
		msleep(105);//wait ic process t6 cmd

		if (info->roi_info.roi_supported) {
			if (MXT_FEATURE_ENABLE == info->roi_info.roi_switch) {
				mxt_enable_feature(data, MAX_KNUCKLE_CONF);
			}
		}
#if defined(HUAWEI_CHARGER_FB)
		if (info->charger_info.charger_supported) {
			if (MXT_FEATURE_ENABLE ==
			    info->charger_info.charger_switch) {
				mxt_enable_feature(data, MAX_WORKAROUND5_CONF);
			}
		}
#endif
		break;
	default:
		break;
	}
	atmel_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);
}

static int atmel_chip_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	if(!info->glove_supported) {
		TS_LOG_ERR("%s, not support glove\n", __func__);
		return NO_ERR;
	}
	TS_LOG_INFO("%s: operation is %d,glove_switch is %d,status is %d/n",
		    __func__, info->op_action, info->glove_switch,
		    info->status);
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		mxt_glove_switch(info->glove_switch);
		break;
	case TS_ACTION_READ:
		info->glove_switch = mxt_feature_list[MAX_GLOVE_CONF].on_flag;
		break;
	default:
		TS_LOG_INFO("%s invalid action: %d\n", __func__,
			    info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;

}

static void mxt_holster_switch(u8 holster_switch)
{
	struct mxt_data *data = mxt_core_data;
	struct ts_feature_info *info = NULL;

	if(!data || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	info = &data->atmel_chip_data->ts_platform_data->feature_info;

	switch (holster_switch) {
	case MXT_FEATURE_ENABLE:
		mxt_enable_feature(data, MAX_COVER_CONF);
		break;
	case MXT_FEATURE_DISABLE:
		mxt_disable_all_features();
		atmel_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
		msleep(105);//wait ic process t6 cmd

		if (MXT_FEATURE_ENABLE == info->glove_info.glove_switch) {
			mxt_enable_feature(data, MAX_GLOVE_CONF);
		}

		if (info->roi_info.roi_supported) {
			if (MXT_FEATURE_ENABLE == info->roi_info.roi_switch) {
				mxt_enable_feature(data, MAX_KNUCKLE_CONF);
			}
		}
#if defined(HUAWEI_CHARGER_FB)
		if (info->charger_info.charger_supported) {
			if (MXT_FEATURE_ENABLE ==
			    info->charger_info.charger_switch) {
				mxt_enable_feature(data, MAX_WORKAROUND5_CONF);
			}
		}
#endif
		break;
	default:
		break;
	}
}

static int atmel_chip_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	if(!info->holster_supported) {
		TS_LOG_ERR("%s, not support holster\n", __func__);
		return NO_ERR;
	}
	TS_LOG_INFO("%s: operation is %d, holster_switch is %d, status is %d\n",
		    __func__, info->op_action, info->holster_switch,
		    info->status);
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		mxt_holster_switch(info->holster_switch);
		break;
	case TS_ACTION_READ:
		info->holster_switch = mxt_feature_list[MAX_COVER_CONF].on_flag;
		break;
	default:
		TS_LOG_INFO("%s invalid action: %d\n", __func__,
			    info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;

}

static unsigned char *atmel_roi_rawdata(void)
{
#ifdef ROI
	return roi_data;
#else
	return NULL;
#endif
}

static int atmel_read_roi_switch(struct ts_roi_info *info)
{
	info->roi_switch = mxt_feature_list[MAX_KNUCKLE_CONF].on_flag;
	return NO_ERR;
}

static int atmel_roi_switch(struct ts_roi_info *info)
{
	struct mxt_data *data = mxt_core_data;
	int retval = NO_ERR;

#ifdef ROI
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}
	if(!info->roi_supported) {
		TS_LOG_ERR("%s, not support RIO\n", __func__);
		return NO_ERR;
	}
	TS_LOG_INFO("%s: operation is %d\n", __func__, info->op_action);
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		mxt_set_feature(data, MAX_KNUCKLE_CONF, info->roi_switch);
		break;
	case TS_ACTION_READ:
		retval = atmel_read_roi_switch(info);
		break;
	default:
		TS_LOG_DEBUG("invalid roi switch(%d) action: %d\n",
			     info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
#endif
	return retval;
}

static int atmel_regs_operate(struct ts_regs_info *info)
{
	int retval = NO_ERR;
	u8 value[TS_MAX_REG_VALUE_NUM] = { 0 };
	u8 value_tmp = 0;
	int i = 0;
	struct mxt_data *data = mxt_core_data;

	if (!info || !data || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s: param is Null\n", __func__);
		retval = -ENOMEM;
		return retval;
	}

	value_tmp = info->values[0];

	TS_LOG_INFO("addr(%d),op_action(%d),bit(%d),num(%d)\n", info->addr,
		    info->op_action, info->bit, info->num);

	for (i = 0; i < info->num; i++) {
		TS_LOG_INFO("value[%d]=%d\n", i, info->values[i]);
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		for (i = 0; i < info->num; i++) {
			value[i] = info->values[i];
		}

		if ((1 == info->num) && (MXT_WRITE_BIT_LEN > info->bit)) {
			retval =
			    __atmel_read_reg(data, info->addr, info->num, value);
			if (retval < 0) {
				TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",
					   info->addr);
				retval = -EINVAL;
				goto out;
			}

			if (0 == value_tmp) {
				value[0] &= ~(1 << (uint32_t)(info->bit));
			} else if (1 == value_tmp) {
				value[0] |= (1 << (uint32_t)(info->bit));
			}
		}

		retval = __atmel_write_reg(data, info->addr, info->num, value);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_WRITE error, addr(%d)\n",
				   info->addr);
			retval = -EINVAL;
			goto out;
		}
		break;
	case TS_ACTION_READ:
		retval = __atmel_read_reg(data, info->addr, info->num, value);
		if (retval < 0) {
			TS_LOG_ERR("TS_ACTION_READ error, addr(%d)\n",
				   info->addr);
			retval = -EINVAL;
			goto out;
		}

		if ((1 == info->num) && (MXT_WRITE_BIT_LEN > info->bit)) {
			data->atmel_chip_data->reg_values[0] =
			    (value[0] >> (uint32_t)(info->bit)) & 0x01;
		} else {
			for (i = 0; i < info->num; i++) {
				data->atmel_chip_data->reg_values[i] = value[i];
			}
		}
		break;
	default:
		TS_LOG_ERR("%s, reg operate default invalid action %d\n",
			   __func__, info->op_action);
		retval = -EINVAL;
		break;
	}
out:
	return retval;
}

static int atmel_debug_switch(u8 loglevel)
{
	int retval = NO_ERR;
	if (loglevel) {
		atmel_init_feature_file(true);
	} else {
		atmel_init_feature_file(false);
	}
	return retval;
}

static void atmel_special_hardware_test_switch(unsigned int value)
{
	struct mxt_data *data = mxt_core_data;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}
	switch (value) {
	case MXT_FEATURE_ENABLE:
		memset(&data->t72_noise_level, 0,
		       sizeof(struct t72_noise_level));
		data->t72_noise_level.noise_level_min =
		    data->t72_noise_level_threshold;
		mxt_enable_feature(data, MAX_WORKAROUND3_CONF);
		break;
	case MXT_FEATURE_DISABLE:
		mxt_disable_feature(data, MAX_WORKAROUND3_CONF);
		break;
	default:
		break;
	}
	return;
}

static int atmel_special_hardware_test_result(char *buf)
{
	int ret = 0;
	int t72_test = 2; //time_limit
	char test_status[][20] = { "pass", "fail", "time_limit" };

	int s_count = mxt_core_data->t72_noise_level.success_count;
	int t_count = mxt_core_data->t72_noise_level.total_count;
	int n_max = mxt_core_data->t72_noise_level.noise_level_max;
	int n_min = mxt_core_data->t72_noise_level.noise_level_min;

	/*t72 noise total_count limit > 5*/
	if (t_count >= 5) {
		if (s_count * 2 >= t_count) {	/*success rate is greater than 50%*/
			t72_test = 0;	/*0,test pass */
		} else {
			t72_test = 1;	/*1,test fial */
		}
	}

	ret =
	    snprintf(buf, MAX_HARDWARE_TEST_RESULT_LEN,
		     "%s(s/toal %d/%d)(min:%d, max:%d)_t72_test\n",
		     test_status[t72_test], s_count, t_count, n_min, n_max);
	TS_LOG_INFO("%s, resule:%s", __func__, buf);

	return ret;
}

static void atmel_chip_touch_switch(void)
{
	struct mxt_data *data = mxt_core_data;
	char in_data[MAX_STR_LEN] = {0};
	int error = 0;
	unsigned char stype = 0, soper = 0, param = 0, sdatatowrite = 0;
	char *cur= NULL;
	char *token = NULL;

	u8 touch_mode = MXT_POWER_CFG_RUN;
	TS_LOG_INFO("%s enter\n", __func__);

	if ((NULL == data) ||
		(NULL == data->atmel_chip_data)){
		TS_LOG_ERR("%s, error chip data\n",__func__);
		goto out;
	}

	if (TS_SWITCH_TYPE_DOZE !=	(data->atmel_chip_data->touch_switch_flag & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s, doze mode does not suppored by this chip\n",__func__);
		goto out;
	}
	/* SWITCH_OPER,ENABLE_DISABLE,PARAM */
	/* get switch operate */

	memcpy(in_data, data->atmel_chip_data->touch_switch_info, MAX_STR_LEN - 1);

	TS_LOG_INFO("%s, in_data:%s\n",__func__, in_data);

	/* get stype */
	cur = (char*)in_data;
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s stype get error\n",__func__);
		goto out;
	}
	stype = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get stype=%d\n",__func__,stype);
	/* get switch operate */
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s soper get error\n",__func__);
		goto out;
	}
	soper = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get soper=%d\n",__func__,soper);
	/* get param */
	token = strsep(&cur, ",");
	if(token==NULL){
		TS_LOG_ERR("%s param get error\n",__func__);
		goto out;
	}
	param = (unsigned char)simple_strtol(token, NULL, 0);
	TS_LOG_INFO("%s get param=%d\n",__func__,param);

	if (TS_SWITCH_TYPE_DOZE != (stype & TS_SWITCH_TYPE_DOZE)){
		TS_LOG_ERR("%s stype not  TS_SWITCH_TYPE_DOZE:%d, invalid\n",__func__, stype);
		goto out;
	}
	/* Value"1"->On; Value"2"->Off */
	switch (soper){
		case TS_SWITCH_DOZE_ENABLE:
			sdatatowrite = param;
			touch_mode = MXT_POWER_CFG_DOZE;
			break;
		case TS_SWITCH_DOZE_DISABLE:
			sdatatowrite = DEFAULT_ACTIVE2IDLE_TIME;/*Disable operation*//*50 means if no touch, then tp will change status from active to idle after 50/2*200ms(5s)  */
			touch_mode = MXT_POWER_CFG_RUN;
			break;
		default:
			TS_LOG_ERR("%s soper unknown:%d, invalid\n", __func__ ,soper);
			goto out;
	}
	/*for app, active2idle time is sdatatowrite*100ms, for IC, active2idle time is sdatatowrite*200ms, so sdatatowrite need mul 2 at here*/
	error = mxt_set_t7_active2ide_timeout_cfg(data, sdatatowrite/2);
	if(error) {
		TS_LOG_ERR("%s, %s, set doze mode to value %d,[fail]\n", __func__,(soper == TS_SWITCH_DOZE_ENABLE)? "enable doze":"disable doze", sdatatowrite);
	} else {
		TS_LOG_INFO("%s ,%s, set doze mode to value %d,[OK]\n",__func__, (soper == TS_SWITCH_DOZE_ENABLE)? "enable doze":"disable doze", sdatatowrite);
	}
out:
	return;
}

static int __init atmel_module_init(void)
{
    bool found = false;
    struct device_node* child = NULL;
    struct device_node* root = NULL;
    int error = NO_ERR;

    TS_LOG_INFO(" atmel_module_init called here\n");
    mxt_core_data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
    if (!mxt_core_data) {
    	TS_LOG_ERR("Failed to alloc mem for struct mxt_core_data\n");
       error =  -ENOMEM;
       goto malloc_core_data_err;
    }
    mxt_core_data->atmel_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!mxt_core_data->atmel_chip_data ) {
        TS_LOG_ERR("Failed to alloc mem for struct atmel_chip_data\n");
        error =  -ENOMEM;
        goto malloc_chip_data_err;
    }
    root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
    if (!root)
    {
        TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
        error = -EINVAL;
        goto out;
    }

    for_each_child_of_node(root, child)  //find the chip node
    {
        if (of_device_is_compatible(child, ATMEL_VENDER_NAME))
        {
            found = true;
            break;
        }
    }
    if (!found){
        TS_LOG_ERR(" not found chip atmel child node!\n");
        error = -EINVAL;
        goto out;
    }

    mxt_core_data->atmel_chip_data->cnode = child;
    mxt_core_data->atmel_chip_data->ops = &ts_kit_atmel_ops;

    error = huawei_ts_chip_register(mxt_core_data->atmel_chip_data);
    if(error)
    {
		TS_LOG_ERR(" atmel chip register fail !\n");
		goto out;
    }
    TS_LOG_INFO("atmel chip_register! err=%d\n", error);
    return error;
out:
    if(mxt_core_data && mxt_core_data->atmel_chip_data) {
		kfree(mxt_core_data->atmel_chip_data);
		mxt_core_data->atmel_chip_data = NULL;/*Fix the DTS parse error cause panic bug*/
	}
malloc_chip_data_err:
    if (mxt_core_data) {
		kfree(mxt_core_data);
		mxt_core_data = NULL;
    }
malloc_core_data_err:

    return error;
}

static void __exit atmel_module_exit(void)
{
   TS_LOG_INFO("atmel_module_exit called here\n");

    return;
}

late_initcall(atmel_module_init);
module_exit(atmel_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");

