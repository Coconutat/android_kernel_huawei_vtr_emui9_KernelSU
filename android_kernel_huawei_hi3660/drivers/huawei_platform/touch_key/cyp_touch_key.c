

#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <huawei_platform/log/hw_log.h>
#include <media/huawei/hw_extern_pmic.h>
#include <linux/proc_fs.h>
#include "cyp_touch_key.h"
#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>

static struct dsm_dev dsm_touch_key = {
	.name = CYTTSP_DSM_TOUCH_KEY,
	.device_name = CYTTSP_DSM_DEVICE_NAME,
	.ic_name = CYTTSP_DSM_IC_NAME,
	.module_name = CYTTSP_DSM_MODULE_NAME,
	.fops = NULL,
	.buff_size = CYTTSP_DSM_BUFFER_SIZE,
};
struct dsm_client *touchkey_dclient = NULL;
#endif

#define HWLOG_TAG cyp_key

HWLOG_REGIST();

int fw_update_flag = PROC_OFF;
bool in_bootloader = false;
struct i2c_client *this_client = NULL;
struct device *global_dev = NULL;

static int cyttsp_i2c_read_block(struct device *dev, unsigned char addr,
				unsigned char len, void *data)
{
	unsigned char this_addr = addr;
	int ret = 0;
	struct i2c_client *client = to_i2c_client(dev);

	ret = i2c_master_send(client, &this_addr, 1);
	if (ret != 1) {
		hwlog_err("%s-%d: failed to read block!\n", __func__, __LINE__);
		return -EIO;
	}

	ret = i2c_master_recv(client, data, len);

	return (ret < 0) ? ret : ret != len ? -EIO : 0;
}

static int cyttsp_read_reg(struct cyttsp_button_data *data, unsigned char reg)
{
	unsigned char val = 0;
	int ret = 0;

	ret = cyttsp_i2c_read_block(&data->client->dev,
				reg, 1, &val);
	if (ret < 0) {
		hwlog_err("%s-%d: failed to read reg!\n", __func__, __LINE__);
		return ret;
	}

	return val;
}

static int cyttsp_write_reg(struct cyttsp_button_data *data,
				unsigned char reg, unsigned char val)
{
	int ret = 0;
	unsigned char buffer[CYTTSP_I2C_BUF_SIZE] = {0};
	struct device *dev = &data->client->dev;
	struct i2c_client *client = to_i2c_client(dev);

	buffer[0] = reg;
	buffer[1] = val;

	ret = i2c_master_send(client, buffer, (int)sizeof(buffer));
	if (ret != sizeof(buffer)) {
		hwlog_err("%s-%d: failed to write reg!\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

static int cyttsp_i2c_recv(struct device *dev, unsigned char len, void *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int count = i2c_master_recv(client, buf, len);

	return count < 0 ? count : 0;
}

static int cyttsp_i2c_send(struct device *dev, unsigned char len, const void *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int count = i2c_master_send(client, buf, len);

	return count < 0 ? count : 0;
}

static int cyttsp_get_single_line(const unsigned char *src, unsigned char *dst)
{
	int i = 0;

	while (src[i++] != '\n');

	strncpy(dst, src, (i - 1));

	return i;
}

static unsigned char cyttsp_convert(unsigned char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}

	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	return 0;
}

static unsigned char cyttsp_convert_to_u8(unsigned char *buf)
{
	unsigned char msb = cyttsp_convert(buf[0]);
	unsigned char lsb = cyttsp_convert(buf[1]);
	unsigned char ret = msb;

	ret = ret * CYTTSP_16BITS_SHIFT + lsb;
	return ret;
}

static unsigned short cyttsp_convert_to_u16(unsigned char *buf)
{
	unsigned char msb = cyttsp_convert_to_u8(buf);
	unsigned char lsb = cyttsp_convert_to_u8(buf + 2);
	unsigned short ret = msb;

	ret = (ret << CYTTSP_16BITS_SHIFT) | lsb;
	return ret;
}

static unsigned char calculate_checksum8(unsigned char *src, int len)
{
	unsigned char total = 0;
	int i;

	for (i = 0; i < len; i++) {
		total += src[i];
	}
	total = 256 - total;

	return total;
}

static unsigned short cyttsp_checksum16(unsigned char *data, int len)
{
	unsigned short total = 0;
	int i;

	for (i = 0; i < len; i++) {
		total += data[i];
	}
	total = CYTTSP_16BITS_FULL_MASK - total;

	return (unsigned short)(total & CYTTSP_16BITS_FULL_MASK);
}

static int cyttsp_handle_response(struct cyttsp_button_data *data,
				unsigned char cmd, unsigned char *buffer, int resp_len, unsigned char other)
{
	if (cmd == CYTTSP_CMD_ENTER_BTLD) {
		if (resp_len < CYTTSP_ENTER_BTLD_RESP_LEN) {
			hwlog_warn("%s: response length mismatch for enter bootloader\n", __func__);
		}
		hwlog_info("%s: silicon id = 0x%02x 0x%02x 0x%02x 0x%02x, "
			"rev = 0x%02x, bootloader ver = 0x%02x 0x%02x 0x%02x\n", __func__,
			buffer[0], buffer[1], buffer[2], buffer[3],
			buffer[4], buffer[5], buffer[6], buffer[7]);
	} else if (cmd == CYTTSP_CMD_GET_FLASH_SIZE) {
		if (resp_len < CYTTSP_GET_FLASHSZ_RESP_LEN) {
			hwlog_warn("%s: response length mismatch for get flash size\n", __func__);
		}
		hwlog_info("%s: first row number of flash: 0x%02x 0x%02x\n", __func__,
			buffer[0], buffer[1]);
		hwlog_info("%s: last row number of flash: 0x%02x 0x%02x\n", __func__,
			buffer[2], buffer[3]);
	} else if (cmd == CYTTSP_CMD_VERIFY_ROW) {
		if (other != buffer[0]) {
			hwlog_err("%s: row_checksum = 0x%02x, received = 0x%02x\n", __func__,
				other, buffer[4]);
			return -EINVAL;
		}
	}

	return 0;
}

static int cyttsp_recv_command(struct cyttsp_button_data *data,
				unsigned char *error_code, unsigned char *payload, int *data_len)
{
	struct device *dev = &data->client->dev;
	unsigned char buffer[CYTTSP_FW_ORG_DATA_LEN];
	int remain_len;
	int len;
	int ret;
	int offset;
	int i = 0, k = 0;
	unsigned char *curr = payload;
	unsigned short cal_checksum;
	unsigned short buffer_u16;

	while (1) {
		ret = cyttsp_i2c_recv(dev, 1, buffer);
		if (ret != 0) {
			hwlog_err("%s: failed to read buffer\n", __func__);
			return ret;
		}
		if (CYTTSP_PACKET_START == buffer[0]) {
			break;
		}
	}

	ret = cyttsp_i2c_recv(dev, CYTTSP_RESP_HEADER_LEN - 1, &buffer[1]);
	if (ret != 0) {
		hwlog_err("%s: failed to read response header\n", __func__);
		return ret;
	}
	if (buffer[1] != CYTTSP_STS_SUCCESS) {
		*error_code = buffer[1];
		return -EINVAL;
	}
	remain_len = buffer[2] | (buffer[3] << CYTTSP_8BITS_SHIFT);
	*data_len = remain_len;
	offset = 4;

	while (remain_len > 0) {
		if (remain_len > CYTTSP_MAX_PAYLOAD_LEN) {
			len = CYTTSP_MAX_PAYLOAD_LEN;
		} else {
			len = remain_len;
		}

		ret = cyttsp_i2c_recv(dev, len, &buffer[offset]);
		if (ret != 0) {
			hwlog_err("failed to receive response at %d\n", i);
			return ret;
		}
		memcpy(curr, &buffer[offset], len);
		offset += len;
		curr += len;
		remain_len -= len;
		i++;
	}
	cal_checksum = cyttsp_checksum16(&buffer[1], 3 + (*data_len));
	ret = cyttsp_i2c_recv(dev, CYTTSP_RESP_TAIL_LEN, &buffer[offset]);
	if (ret != 0) {
		hwlog_err("%s: failed to receive tail\n", __func__);
		return ret;
	}
	buffer_u16 = buffer[offset+1] << CYTTSP_8BITS_SHIFT;
	if (cal_checksum != (buffer_u16 | buffer[offset])) {
		hwlog_err("%s: checksum not equal\n", __func__);
		return ret;
	}
	if (buffer[offset+2] != CYTTSP_PACKET_END) {
		hwlog_err("%s: Invalid packet tail\n", __func__);
		return -EINVAL;
	}
	if (data->dbg_dump) {
		hwlog_info("recv buffer = ");
		for (k = 0; k < *data_len + CYTTSP_RESP_HEADER_LEN + CYTTSP_RESP_TAIL_LEN; k++) {
			hwlog_info("0x%02x ", buffer[k]);
		}
		hwlog_info("\n");
	}
	return 0;
}

static int cyttsp_send_command(struct cyttsp_button_data *data,
				unsigned char cmd, unsigned char *payload, int data_len, unsigned char other)
{
	int i = 0;
	int k = 0;
	int len = 0;
	int ret = 0;
	int resp_len = 0;
	unsigned char buffer[CYTTSP_SENDND_COMMAND_BUF_SIZE] = {0};
	unsigned char resp[CYTTSP_SENDND_COMMAND_RESP_SIZE] = {0};
	unsigned char error_code = 0;
	unsigned short checksum;
	unsigned char *curr_buf = payload;
	struct device *dev = &data->client->dev;

	buffer[0] = CYTTSP_PACKET_START;
	buffer[1] = cmd;

	do {
		i = 0;

		if (data_len > CYTTSP_MAX_PAYLOAD_LEN) {
			len = CYTTSP_MAX_PAYLOAD_LEN;
		} else {
			len = data_len;
		}

		buffer[2] = (unsigned char)(len & CYTTSP_8BITS_MASK);
		buffer[3] = (unsigned char)((len & CYTTSP_16BITS_MASK) >> CYTTSP_8BITS_SHIFT);

		data_len -= len;
		if (len != 0) {
			memcpy(&buffer[4], curr_buf, len);
		}
		i = 4 + len;

		checksum = cyttsp_checksum16(buffer+1, len+3);
		curr_buf += len;

		buffer[i++] = (unsigned char)(checksum & CYTTSP_8BITS_MASK);
		buffer[i++] = (unsigned char)((checksum & CYTTSP_16BITS_MASK) >> CYTTSP_8BITS_SHIFT);
		buffer[i++] = CYTTSP_PACKET_END;

		ret = cyttsp_i2c_send(dev, i, buffer);
		if (ret != 0) {
			hwlog_err("%s: failed to send cmd 0x%02x\n", __func__, cmd);
			return ret;
		}

		mdelay(CYTTSP_WRITE_DELAY_COUNT);

		if (cmd != CYTTSP_CMD_EXIT_BTLD) {
			error_code = 0;
			resp_len = 0;
			ret = cyttsp_recv_command(data, &error_code, resp, &resp_len);
			if (ret) {
				hwlog_err("%s: response error code = 0x%02x\n",
					__func__, error_code);
				return ret;
			}
			ret = cyttsp_handle_response(data, cmd, resp, resp_len, other);
			if (ret) {
				hwlog_err("%s: response error for cmd 0x%x\n", __func__, cmd);
				return ret;
			}
		}

		if (data->dbg_dump) {
			hwlog_info("send buffer = ");
			for (k = 0; k < i; k++) {
				hwlog_info("0x%x ", buffer[k]);
			}
			hwlog_info("\n");
		}
	} while (data_len > 0);

	return 0;
}

static void cyttsp_button_print_dt(struct cyttsp_button_platform_data *pdata)
{
	if(NULL == pdata) {
		hwlog_err("%s: pdata is NULL, cant print!", __func__);
		return;
	}
	hwlog_info("%s: irq gpio = %d\n", __func__, pdata->irq_gpio);
	hwlog_info("%s: irq_flags = %d\n", __func__, (int)pdata->irq_flags);
	hwlog_info("%s: input name = %s\n", __func__, pdata->input_name);
	hwlog_info("%s: button number = %d\n", __func__, pdata->nbuttons);
	hwlog_info("%s: button_status_reg = 0x%02x\n", __func__, pdata->button_status_reg);
	hwlog_info("%s: work_mode_reg = 0x%02x\n", __func__, pdata->work_mode_reg);
	hwlog_info("%s: bootloader_addr = 0x%02x\n", __func__, pdata->bl_addr);
	hwlog_info("%s: idac_limit = %u, %u\n", __func__, pdata->idac_limit[CYTTSP_IDAC_MIN], pdata->idac_limit[CYTTSP_IDAC_MAX]);
	hwlog_info("%s: cmod_limit = %u, %u\n", __func__, pdata->cmod_limit[CYTTSP_CMOD_MIN], pdata->cmod_limit[CYTTSP_CMOD_MAX]);
	hwlog_info("%s: cmod_charging_limit = %u\n", __func__, pdata->cmod_charging_limit);
}

static int cyttsp_button_parse_dt(struct device *dev,
			struct cyttsp_button_platform_data *pdata)
{
	int ret = 0;
	unsigned int temp_val = 0;
	unsigned long long vdd_temp = 0;

	if(NULL == dev) {
		hwlog_err("%s: cyttsp_button_parse_dt err, dev NULL\n", __func__);
		return -CYTTSP_PARAM_NULL;
	}
	struct device_node *np = dev->of_node;

	if(NULL == pdata) {
		hwlog_err("%s: cyttsp_button_parse_dt err, pdata NULL\n", __func__);
		return -CYTTSP_PARAM_NULL;
	}
	if(NULL == np) {
		hwlog_err("%s: cyttsp_button_parse_dt err, np NULL\n", __func__);
		return -CYTTSP_PARAM_NULL;
	}

	/* irq gpio */
	pdata->irq_gpio = of_get_named_gpio_flags(np, "cyttsp,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND;

	/* vdd chanel num */
	ret = of_property_read_u32(np, "cyttsp,extern_vdd_supply", &temp_val);
	if (ret) {
		hwlog_err("%s: unable to read vdd supply\n", __func__);
	} else {
		pdata->vdd = temp_val;
		hwlog_info("%s: vdd chanel is %d\n", __func__, pdata->vdd);
	}

	/* voltage of vdd */
	ret = of_property_read_u64(np, "cyttsp,extern_vdd_vol", &vdd_temp);
	if (ret) {
		pdata->vdd_vol = CYP_DEFAULT_VDD_VOL;
		hwlog_err("%s: unable to read vdd supply, set default value: %ld\n",
			__func__, (unsigned long)pdata->vdd_vol);
	} else {
		pdata->vdd_vol = vdd_temp;
		hwlog_info("%s: pdata->vdd_vol = %ld\n", __func__, (unsigned long)pdata->vdd_vol);
	}

	/* charactors for input_dev */
	ret = of_property_read_string(np, "cyttsp,input-name",
			&pdata->input_name);
	if (ret && (ret != -EINVAL)) {
		hwlog_err("%s: unable to read input name\n", __func__);
		return ret;
	}

	/* addr of button status register */
	ret = of_property_read_u32(np, "cyttsp,button-status-reg",
			&temp_val);
	if (ret) {
		hwlog_err("%s: unable to read fw button-status-reg\n", __func__);
		return ret;
	} else {
		pdata->button_status_reg = (unsigned char)temp_val;
	}

	/* addr of work mode register */
	ret = of_property_read_u32(np, "cyttsp,work-mode-reg",
			&temp_val);
	if (ret) {
		hwlog_err("%s: unable to read work mode reg\n", __func__);
		return ret;
	} else {
		pdata->work_mode_reg = (unsigned char)temp_val;
	}

	/* numbers of key */
	ret = of_property_read_u32(np, "cyttsp,key-num", &temp_val);
	if (ret) {
		hwlog_err("%s: unable to read key num\n", __func__);
		return ret;
	} else {
		pdata->nbuttons = temp_val;
	}

	if (pdata->nbuttons != 0) {
		pdata->key_code = devm_kzalloc(dev, sizeof(int) * pdata->nbuttons, GFP_KERNEL);
		if (!pdata->key_code) {
			return -ENOMEM;
		}
		ret = of_property_read_u32_array(np, "cyttsp,key-codes",
						pdata->key_code, pdata->nbuttons);
		if (ret) {
			hwlog_err("%s: unable to read key codes\n", __func__);
			return ret;
		}
	}

	ret = of_property_read_u32(np, "cyttsp,bootloader-addr", &temp_val);
	if (ret) {
		pdata->bl_addr = CYTTSP_DEFAULT_BL_ADDR;
		hwlog_err("%s: unable to read btld address, set default addr: 0x%02x\n",
			__func__, pdata->bl_addr);
	} else {
		pdata->bl_addr = (unsigned char)temp_val;
	}

	ret = of_property_read_u32_array(np, "cyttsp,cap-limit",
				pdata->cp_limit, CYTTSP_CP_NUM);
	if(ret) {
		hwlog_err("%s: Unable to read cap-limit\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "cyttsp,noise-limit", &temp_val);
	if(ret) {
		pdata->noise_limit = CYTTSP_DEFAULT_NOISE_LIMIT;
		hwlog_err("%s: Unable to read noise-limit, set default noise limit: 0x%02x\n",
			__func__, pdata->noise_limit);
	} else {
		pdata->noise_limit = temp_val;
	}

	ret = of_property_read_u32_array(np, "cyttsp,idac-limit", pdata->idac_limit, CYTTSP_IDAC_NUM);
	if(ret) {
		pdata->idac_limit[CYTTSP_IDAC_MIN] = CYTTSP_DEFAULT_IDAC_LIMIT_MIN;
		pdata->idac_limit[CYTTSP_IDAC_MAX] = CYTTSP_DEFAULT_IDAC_LIMIT_MAX;
		hwlog_err("%s: Unable to read idac-limit, set default idac limit: 0x%02x, 0x%04x\n",
			__func__, pdata->idac_limit[CYTTSP_IDAC_MIN], pdata->idac_limit[CYTTSP_IDAC_MAX]);
	}

	ret = of_property_read_u32_array(np, "cyttsp,cmod-limit", pdata->cmod_limit, CYTTSP_CMOD_NUM);
	if(ret) {
		pdata->cmod_limit[CYTTSP_CMOD_MIN] = CYTTSP_DEFAULT_CMOD_LIMIT_MIN;
		pdata->cmod_limit[CYTTSP_CMOD_MAX] = CYTTSP_DEFAULT_CMOD_LIMIT_MAX;
		hwlog_err("%s: Unable to read cmod-limit, set default cmod limit: 0x%02x, 0x%04x\n",
			__func__, pdata->cmod_limit[CYTTSP_CMOD_MIN], pdata->cmod_limit[CYTTSP_CMOD_MAX]);
	}

	ret = of_property_read_u32(np, "cyttsp,cmod-charging-limit", &temp_val);
	if(ret) {
		pdata->cmod_charging_limit = CYTTSP_DEFAULT_CMOD_CHARGING_LIMIT;
		hwlog_err("%s: Unable to read cmod-charging-limit, set default cmod charging limit: 0x%04x\n",
			__func__, pdata->cmod_charging_limit);
	} else {
		pdata->cmod_charging_limit = temp_val;
	}

	ret = of_property_read_u32(np, "cyttsp,support-mmi", &temp_val);
	if(ret) {
		pdata->if_support_mmi = CYTTSP_DEFAULT_SUPPORT_MMI;
		hwlog_err("%s: Unable to read support-mmi, set default support: %d\n",
			__func__, pdata->if_support_mmi);
	} else {
		pdata->if_support_mmi = temp_val;
	}

	ret = of_property_read_u32(np, "cyttsp,support-running", &temp_val);
	if(ret) {
		pdata->if_support_running = CYTTSP_DEFAULT_SUPPORT_RUNNING;
		hwlog_err("%s: Unable to read support-running, set default support: %d\n",
			__func__, pdata->if_support_running);
	} else {
		pdata->if_support_running = temp_val;
	}

	cyttsp_button_print_dt(pdata);

	return 0;
}

static int cyttsp_button_power_on(struct input_dev *in_dev)
{
	int ret = 0;
	struct cyttsp_button_data *data = input_get_drvdata(in_dev);
	struct cyttsp_button_platform_data *pdata = data->pdata;

	if (pdata->vdd) {
		ret = hw_extern_pmic_config(pdata->vdd,  pdata->vdd_vol, TURN_ON);
		if (ret < 0) {
			hwlog_err("%s: failed to enable regulator vdd\n", __func__);
		}
	}
	return 0;
}

static int cyttsp_button_power_off(struct input_dev *in_dev)
{
	int ret = 0;
	struct cyttsp_button_data *data = input_get_drvdata(in_dev);
	struct cyttsp_button_platform_data *pdata = data->pdata;

	if (pdata->vdd) {
		ret = hw_extern_pmic_config(pdata->vdd,  pdata->vdd_vol, TURN_OFF);
		if (ret < 0) {
			hwlog_err("%s: Failed to disable regulator vdd\n", __func__);
		}
	}
	return 0;
}

static int cyttsp_button_input_enable(struct input_dev *in_dev)
{
	int ret = 0;
	struct cyttsp_button_data *data = input_get_drvdata(in_dev);
	struct cyttsp_button_platform_data *pdata = data->pdata;

	if (true == data->enable) {
		hwlog_err("%s: button power on already\n", __func__);
		return 0;
	}

	if (pdata->work_mode_reg != CYTTSP_REG_INVALID) {
		ret = cyttsp_write_reg(data, pdata->work_mode_reg, CYTTSP_NORMAL_MODE);
		if (ret) {
			hwlog_err("%s: fail to enter normal mode\n",
				__func__);
			return ret;
		} else {
			hwlog_info("%s: enter normal mode\n", __func__);
		}
	}
	data->enable = true;
	enable_irq(pdata->irq_num);

	return 0;
}

static int cyttsp_button_input_disable(struct input_dev *in_dev)
{
	int ret = 0;
	struct cyttsp_button_data *data = input_get_drvdata(in_dev);
	struct cyttsp_button_platform_data *pdata = data->pdata;

	if (false == data->enable) {
		hwlog_err("%s: button power off already\n", __func__);
		return 0;
	}
	disable_irq(pdata->irq_num);

	if (pdata->work_mode_reg != CYTTSP_REG_INVALID) {
		ret = cyttsp_write_reg(data, pdata->work_mode_reg, CYTTSP_DEEPSLEEP_MODE);
		if (ret) {
			hwlog_err("%s: fail to enter deep sleep mode\n", __func__);
			return ret;
		} else {
			hwlog_info("%s: enter deep sleep mode\n", __func__);
		}
	}
	data->enable = false;

	return 0;
}

void cyp_report_dmd(const char* dmd_log, int dmd_num)
{
	if(NULL == dmd_log) {
		hwlog_err("%s: dmd_log NULL\n", __func__);
		return;
	}

	#ifdef CONFIG_HUAWEI_DSM
		if(!touchkey_dclient) {
			hwlog_err("%s: touchkey_dclient is NULL, return\n", __func__);
		} else {
			if (!dsm_client_ocuppy(touchkey_dclient)) {
				hwlog_err("%s\n", dmd_log);
				dsm_client_record(touchkey_dclient, dmd_log);
				dsm_client_notify(touchkey_dclient, dmd_num);
			}
		}
	#endif
}

#ifdef CONFIG_FB
static int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	int *blank;
	int ret = 0;
	struct fb_event *evdata = data;
	struct cyttsp_button_data *cyp_data = NULL;

	if(NULL == evdata || NULL == self) {
		hwlog_err("%s: evdata or self is NULL, return\n", __func__);
		return 0;
	}

	cyp_data = container_of(self, struct cyttsp_button_data, fb_notif);

	if (PROC_ON == fw_update_flag) {
		cyp_data->enable = true;
		hwlog_debug("%s: fw updating, block callback\n", __func__);
		return -EBUSY;
	} else {
		if (evdata && evdata->data &&
				(FB_EVENT_BLANK == event) && cyp_data) {
			blank = evdata->data;
			if (FB_BLANK_UNBLANK == *blank) {
				hwlog_info("%s: --=UNBLANK SCREEN FOR CYTTSP=--\n", __func__);
				ret = cyttsp_button_input_enable(cyp_data->input_dev);
				if (ret) {
					hwlog_err("%s-%d: fail to enable button\n", __func__,
						__LINE__);
					goto err_enable_or_disable_ic;
				}
			} else if (FB_BLANK_POWERDOWN == *blank) {
				hwlog_info("%s: --=BLANK SCREEN FOR CYTTSP=--\n", __func__);
				ret = cyttsp_button_input_disable(cyp_data->input_dev);
				if (ret) {
					hwlog_err("%s-%d: fail to disable button\n", __func__,
						__LINE__);
					goto err_enable_or_disable_ic;
				}
			}
		}
	}
	return 0;

err_enable_or_disable_ic:
	cyp_report_dmd(CYTTSP_DSM_EN_DISABLE_ERR, DSM_TOUCHKEY_WAKE_UP_ERR);
	return 0;
}
#endif

static irqreturn_t cyttsp_button_interrupt_handler(int irq, void *dev_id)
{
	bool curr_state = 0;
	bool new_state = 0;
	bool sync = false;
	int val = 0;
	unsigned char key = 0;
	unsigned long key_states = 0;
	struct cyttsp_button_data *data = dev_id;
	struct cyttsp_button_platform_data *pdata = NULL;;

	if(NULL == data) {
		hwlog_err("%s: data is NULL\n", __func__);
		goto param_null;
	}

	if(NULL == data->pdata) {
		hwlog_err("%s: pdata is NULL\n", __func__);
		goto param_null;
	}
	pdata = data->pdata;

	mutex_lock(&data->input_dev->mutex);
	if (data->enable) {
		val = cyttsp_read_reg(data, CYTTSP_REG_TOUCHMODE);
		if (val < 0) {
			hwlog_err("%s: fail to read touch mode reg\n", __func__);
			goto err_read_i2c;
		} else {
			data->glove_mode = !!((unsigned char)val & (1 << CYTTSP_GLOVE_MODE_SHIFT));
		}

		val = cyttsp_read_reg(data, pdata->button_status_reg);
		if (val < 0) {
			hwlog_err("%s: fail to read status reg\n", __func__);
			goto err_read_i2c;
		}

		key_states = (unsigned long)val;

		for (key = 0; key < pdata->nbuttons; key++) {
			curr_state = test_bit(key, &data->key_status);
			new_state = test_bit(key, &key_states);

			if (curr_state ^ new_state) {
				input_event(data->input_dev, EV_KEY,
					pdata->key_code[key], !!(key_states & (1 << key)));
				sync = true;
			}
		}

		data->key_status = key_states;

		if (sync) {
			input_sync(data->input_dev);
		}
	}
	mutex_unlock(&data->input_dev->mutex);

	return IRQ_HANDLED;

err_read_i2c:
	schedule_work(&(data->touchkey_dsm_work_struct));
	mutex_unlock(&data->input_dev->mutex);
param_null:
	return IRQ_NONE;
}

/* to be continue */
static ssize_t cyttsp_debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char *c = "actived";
	return sprintf(buf, "%s\n", c);
}

/* to be continue */
static ssize_t cyttsp_debug_enable_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	int error = 0;
	int read_write = 0, value = 0, reg = 0, num = 0;
	unsigned char value_uc = 0, reg_uc = 0, len_uc = 0;
	int data_int = 0;
	struct cyttsp_button_data *data = NULL;

	if(NULL == dev) {
		hwlog_err("%s: dev is NULL, just return!\n", __func__);
		goto err_device_null;
	}
	if(NULL == buf) {
		hwlog_err("%s: buf is NULL, just return!\n", __func__);
		goto err_buf_null;
	}
	if(NULL == this_client) {
		hwlog_err("%s: client is NULL, just return!\n", __func__);
		goto err_i2c_client_null;
	}
	data = i2c_get_clientdata(this_client);
	if(NULL == data) {
		hwlog_err("%s: data is NULL, just return!\n", __func__);
		goto err_data_null;
	}

	/*if it is read mode*/
	error = sscanf(buf, "%d 0x%2x %d", &read_write, &reg, &num);
	if(error < 0) {
		/*if it is write mode*/
		error = sscanf(buf, "%d 0x%2x 0x%2x", &read_write, &reg, &value);
		if(error < 0) {
			goto err_param;
		}
	}
	hwlog_warn("parse: mode: %d, reg:%d, value:%d, length:%d\n", read_write, reg, value, num);

	if((CYTTSP_SYS_PARAM_READ == read_write || CYTTSP_SYS_PARAM_WRITE == read_write)
			&& (reg <= CYTTSP_SYS_PARAM_REG_MAX && reg >= CYTTSP_SYS_PARAM_REG_MIN)
			&& (value <= CYTTSP_SYS_PARAM_VALUE_MAX && value >= CYTTSP_SYS_PARAM_VALUE_MIN)
			&& (CYTTSP_SYS_PARAM_LEN_ZERO == num || CYTTSP_SYS_PARAM_LEN_TWO == num || CYTTSP_SYS_PARAM_LEN_ONE == num)) {
		value_uc = (unsigned char)value;
		reg_uc = (unsigned char)reg;
		len_uc = (unsigned char)num;
	} else {
		goto err_param;
	}

	if(CYTTSP_SYS_PARAM_WRITE == read_write) {
		ret = cyttsp_write_reg(data, reg_uc, value_uc);
		if(ret < 0) {
			hwlog_err("%s: write reg faild\n", __func__);
			goto err_write_reg;
		} else {
			hwlog_warn("%s: write reg success\n", __func__);
		}
	} else if(CYTTSP_SYS_PARAM_READ == read_write) {
		ret = cyttsp_i2c_read_block(&data->client->dev, reg_uc, len_uc, &data_int);
		if(ret < 0) {
			hwlog_err("%s: read block faild\n", __func__);
			goto err_read_block;
		}
		hwlog_warn("read reg result is %d\n", data_int);
	}

	return count;

err_param:
	hwlog_err("err param, please input such cmd \"mode(0|1) reg(max:107) value(max:255) [length(1|2)]\"\n");
err_read_block:
err_write_reg:
err_data_null:
err_i2c_client_null:
err_buf_null:
err_device_null:
	return -EIO;
}

static DEVICE_ATTR(debug_enable, S_IWUSR | S_IRUSR, cyttsp_debug_enable_show,
			cyttsp_debug_enable_store);

static struct attribute *cyttsp_attrs[] = {
	&dev_attr_debug_enable.attr,
	NULL
};

static const struct attribute_group cyttsp_attr_group = {
	.attrs = cyttsp_attrs,
};

static bool touchkey_noise_test(struct cyttsp_button_data *data, char (*result_array)[CYTTSP_REASON_LENGTH],
				char (*reason_array)[CYTTSP_REASON_LENGTH], int test_num) {
	int ret = 0;
	unsigned short raw_data = 0;
	unsigned char raw_count_reg[CYTTSP_TOTAL_KEY_NUM] = {CYTTSP_RAW_COUNT_BTN0_L, CYTTSP_RAW_COUNT_BTN1_L};
	/*raw_data_btn_max is to store max data read from RAW_COUNT reg, raw_data_btn_min is to store min data read from RAW_COUNT reg
	**raw_data_btn_max is initialized in 0x00, raw_data_btn_min is initialized in 0xFF
	*/
	unsigned short raw_data_btn_max[CYTTSP_TOTAL_KEY_NUM] = {CYTTSP_RAW_DATA_MIN};
	unsigned short raw_data_btn_min[CYTTSP_TOTAL_KEY_NUM] = {CYTTSP_RAW_DATA_MAX, CYTTSP_RAW_DATA_MAX};
	unsigned short noise_btn[CYTTSP_TOTAL_KEY_NUM] = {0};
	int count = CYTTSP_READ_NOISE_TIMES;
	int which_btn = 0;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == data || NULL == result_array || NULL == reason_array) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	if(NULL == data->client) {
		hwlog_err("%s: data->client NULL!\n", __func__);
		goto param_null;
	}
	if(NULL == &(data->client->dev)) {
		hwlog_err("%s: &data->client->dev NULL!\n", __func__);
		goto param_null;
	}
	pdata = data->pdata;
	if(NULL == pdata) {
		hwlog_err("%s: param pdata NULL!\n", __func__);
		goto param_null;
	}

	/*To get noise data, you should read RAW_COUNT reg 10 times,
	**then select the max one subtract the min one, that is the noise data
	*/
	do {
		for(which_btn = 0; which_btn < CYTTSP_TOTAL_KEY_NUM; which_btn++) {
			ret = cyttsp_i2c_read_block(&data->client->dev, raw_count_reg[which_btn], CYTTSP_I2C_BUF_SIZE, &raw_data);
			if(ret < 0) {
				hwlog_err("%s: read raw data from block err, continue!\n", __func__);
				continue;
			}
			if(raw_data > raw_data_btn_max[which_btn]) {
				raw_data_btn_max[which_btn] = raw_data;
			}
			else {
				if(raw_data < raw_data_btn_min[which_btn])
					raw_data_btn_min[which_btn] = raw_data;
			}
		}
		mdelay(CYTTSP_NOISE_TEST_DELAY);
	} while(--count);

	noise_btn[0] = raw_data_btn_max[0] - raw_data_btn_min[0];
	noise_btn[1] = raw_data_btn_max[1] - raw_data_btn_min[1];

	pdata->cypress_store_noise[CYTTSP_KEY1]= noise_btn[CYTTSP_KEY1];
	pdata->cypress_store_noise[CYTTSP_KEY2]= noise_btn[CYTTSP_KEY2];
	for(which_btn = 0; which_btn < CYTTSP_TOTAL_KEY_NUM; which_btn++) {
		if(noise_btn[which_btn] > pdata->noise_limit) {
			hwlog_err("%s: noise data is not in limit\n", __func__);
			sprintf(*(reason_array + test_num), "reason:noise(%d);", noise_btn[which_btn]);
			goto not_in_limit;
		}
	}
	sprintf(*(reason_array + test_num), "noise:%d,noise:%d;",
		noise_btn[0], noise_btn[1]);
	sprintf(*(result_array + test_num), "-%dP", test_num);
	return true;

not_in_limit:
	sprintf(*(result_array + test_num), "-%dF", test_num);
param_null:
	return false;
}

static bool touchkey_idac_test(struct cyttsp_button_data *data, char (*result_array)[CYTTSP_REASON_LENGTH],
				char (*reason_array)[CYTTSP_REASON_LENGTH], int test_num) {
	int comp_idac0 = 0, comp_idac1 = 0;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == data || NULL == result_array || NULL == reason_array || NULL == (reason_array + test_num) || NULL == (result_array + test_num) ||
			NULL == *(reason_array + test_num) || NULL == *(result_array + test_num) || test_num < CYTTSP_TEST_MIN || test_num > CYTTSP_TEST_MAX) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	pdata = data->pdata;
	if(NULL == pdata) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}

	comp_idac0 = cyttsp_read_reg(data, CYTTSP_REG_COMP_IDAC_0);
	if(comp_idac0 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
		snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, CYTTSP_IDAC_REG_FAILED);
		goto err_read_reg;
	}

	comp_idac1 = cyttsp_read_reg(data, CYTTSP_REG_COMP_IDAC_1);
	if(comp_idac1 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
		snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, CYTTSP_IDAC_REG_FAILED);
		goto err_read_reg;
	}

	if(pdata->idac_limit[CYTTSP_IDAC_MAX] <= CYTTSP_DEFAULT_IDAC_LIMIT_MAX && pdata->idac_limit[CYTTSP_IDAC_MIN] <= CYTTSP_DEFAULT_IDAC_LIMIT_MAX) {
		if(comp_idac0 > (int)pdata->idac_limit[CYTTSP_IDAC_MAX] || comp_idac0 < (int)pdata->idac_limit[CYTTSP_IDAC_MIN] || comp_idac1 > (int)pdata->idac_limit[CYTTSP_IDAC_MAX] || comp_idac1 < (int)pdata->idac_limit[CYTTSP_IDAC_MIN]) {
			hwlog_err("%s: %d idac over bound!\n", __func__, __LINE__);
			snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, "reason:idac over bound!idac(%d),idac:(%d);",
				comp_idac0, comp_idac1);
			goto err_over_bound;
		}
	} else {
		hwlog_err("%s: %d idac limit err!\n", __func__, __LINE__);
		snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, "reason:idac limit err!idac(%d),idac:(%d);", comp_idac0, comp_idac1);
		goto err_limit;
	}
	snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, "idac:%d,idac:%d;",
		comp_idac0, comp_idac1);
	snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dP", test_num);
	hwlog_warn("%s: idac0 is %d, idac1 is %d\n", __func__, comp_idac0, comp_idac1);

	return true;

err_over_bound:
err_limit:
err_read_reg:
	snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dF", test_num);
param_null:
	return false;
}

static bool touchkey_cmod_test(struct cyttsp_button_data *data, char (*result_array)[CYTTSP_REASON_LENGTH],
				char (*reason_array)[CYTTSP_REASON_LENGTH], int test_num)
{
	int ret = 0;
	unsigned int cmod = 0, cmod_charging = 0;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == data || NULL == result_array || NULL == reason_array || NULL == (reason_array + test_num) || NULL == (result_array + test_num) ||
			NULL == *(reason_array + test_num) || NULL == *(result_array + test_num) ||
			NULL == (result_array + test_num + CYTTSP_MMI_NEXT_TEST) || NULL == (reason_array + test_num + CYTTSP_MMI_NEXT_TEST) ||
			NULL == *(result_array + test_num + CYTTSP_MMI_NEXT_TEST) || NULL == *(reason_array + test_num + CYTTSP_MMI_NEXT_TEST) ||
			test_num > CYTTSP_TEST_MAX || test_num < CYTTSP_TEST_MIN) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	pdata = data->pdata;
	if(NULL == pdata) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	if(NULL == data->client) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	if(NULL == &data->client->dev) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}

	ret = cyttsp_write_reg(data, CYTTSP_REG_CAP_EN, CYTTSP_TRIGGER_MEASURE_CP);
	if(ret) {
		hwlog_err("%s: cyttsp_write_reg error\n", __func__);
		snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, CYTTSP_WRITE_REG_ERR);
		snprintf(*(reason_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, CYTTSP_WRITE_REG_ERR);
		goto err_write_reg;
	}
	msleep(CYTTSP_READ_CMOD_DELAY);

	ret = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_CMOD, CYTTSP_I2C_BUF_SIZE, &cmod);
	if(ret < 0) {
		hwlog_err("%s: cmod cyttsp_i2c_read_block error\n", __func__);
		snprintf(*(reason_array + test_num), strlen(CYTTSP_READ_REG_ERR), CYTTSP_READ_REG_ERR);
		snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dF", test_num);
	} else {
		if(cmod > pdata->cmod_limit[CYTTSP_CMOD_MAX] || cmod < pdata->cmod_limit[CYTTSP_CMOD_MIN]) {
			hwlog_err("%s: cmod err, cmod(%u);", __func__, cmod);
			snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, "reason:cmod err, cmod(%u);", cmod);
			snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dF", test_num);
		} else {
			snprintf(*(reason_array + test_num), CYTTSP_REASON_LENGTH, "cmod:%u;", cmod);
			snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dP", test_num);
			hwlog_warn("cmod is %u\n", cmod);
		}
	}

	ret = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_CMOD_CHARGING, CYTTSP_I2C_BUF_SIZE, &cmod_charging);
	if(ret < 0) {
		hwlog_err("%s: cmod charging cyttsp_i2c_read_block error\n", __func__);
		snprintf(*(reason_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, CYTTSP_READ_REG_ERR);
		snprintf(*(result_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "-%dF", test_num + CYTTSP_MMI_NEXT_TEST);
	} else {
		if(cmod_charging > pdata->cmod_charging_limit) {
			hwlog_err("%s: cmod charging err, cmod charging(%u);", __func__, cmod);
			snprintf(*(reason_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "reason:cmod err, cmod_charging(%u);", cmod_charging);
			snprintf(*(result_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "-%dF", test_num + CYTTSP_MMI_NEXT_TEST);
		} else {
			snprintf(*(reason_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "cmod_charging:%u;", cmod_charging);
			snprintf(*(result_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "-%dP", test_num + CYTTSP_MMI_NEXT_TEST);
			hwlog_warn("cmod charging is %d\n", cmod_charging);
		}
	}

	return true;

err_write_reg:
	snprintf(*(result_array + test_num), CYTTSP_REASON_LENGTH, "-%dF", test_num);
	snprintf(*(result_array + test_num + CYTTSP_MMI_NEXT_TEST), CYTTSP_REASON_LENGTH, "-%dF", test_num + CYTTSP_MMI_NEXT_TEST);
param_null:
	return false;
}

static bool cyttsp_dump_all_regs(struct cyttsp_button_data *data)
{
	unsigned short singal_0 = 0, singal_1 = 0;
	unsigned short raw_count_0 = 0, raw_count_1 = 0;
	unsigned short baseline_b0 = 0, baseline_b1 = 0;
	unsigned short raw_button_0 = 0, raw_button_1 = 0;
	unsigned short f_threshold_0 = 0, f_threshold_1 = 0;
	int error = 0;
	int speed_mode = 0;
	int mod_idac0 = 0, mod_idac1 = 0;
	int baseline = 0, bist_enable = 0;
	int comp_idac0 = 0, comp_idac1 = 0;
	int n_threshold_0 = 0, n_threshold_1 = 0;
	int touchmode_val = 0, workmode_val= 0, reset_val = 0;

	if(NULL == data) {
		hwlog_err("%s: data NULL!\n", __func__);
		goto data_null;
	}

	touchmode_val = cyttsp_read_reg(data, CYTTSP_REG_TOUCHMODE);
	if(touchmode_val < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_TOUCHMODE, touchmode_val);

	workmode_val = cyttsp_read_reg(data, CYTTSP_REG_WORKMODE);
	if(workmode_val < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_WORKMODE, workmode_val);

	reset_val = cyttsp_read_reg(data, CYTTSP_REG_RESET);
	if(reset_val < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_RESET, reset_val);

	mod_idac0 = cyttsp_read_reg(data, CYTTSP_REG_MOD_IDAC_0);
	if(mod_idac0 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_MOD_IDAC_0, mod_idac0);

	mod_idac1 = cyttsp_read_reg(data, CYTTSP_REG_MOD_IDAC_1);
	if(mod_idac1 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_MOD_IDAC_1, mod_idac1);

	comp_idac0 = cyttsp_read_reg(data, CYTTSP_REG_COMP_IDAC_0);
	if(comp_idac0 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_COMP_IDAC_0, comp_idac0);

	comp_idac1 = cyttsp_read_reg(data, CYTTSP_REG_COMP_IDAC_1);
	if(comp_idac1 < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_COMP_IDAC_1, comp_idac1);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_F_THRESHOLD_0, CYTTSP_I2C_BUF_SIZE, &f_threshold_0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__,CYTTSP_REG_F_THRESHOLD_0, f_threshold_0);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_F_THRESHOLD_1, CYTTSP_I2C_BUF_SIZE, &f_threshold_1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_F_THRESHOLD_1, f_threshold_1);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_N_THRESHOLD_0, CYTTSP_I2C_BUF_SIZE, &n_threshold_0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_N_THRESHOLD_0, n_threshold_1);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_N_THRESHOLD_1, CYTTSP_I2C_BUF_SIZE, &n_threshold_1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_N_THRESHOLD_1, n_threshold_1);

	baseline = cyttsp_read_reg(data, CYTTSP_REG_BASELINE);
	if(baseline < 0) {
		hwlog_err("%s: %d read reg faild\n",__func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_BASELINE, baseline);

	bist_enable = cyttsp_read_reg(data, CYTTSP_REG_CAP_EN);
	if(bist_enable < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_REG_CAP_EN, bist_enable);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_RAW_COUNT_0, CYTTSP_I2C_BUF_SIZE, &raw_count_0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_RAW_COUNT_0, raw_count_0);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_RAW_COUNT_1, CYTTSP_I2C_BUF_SIZE, &raw_count_1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_RAW_COUNT_1, raw_count_1);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_BASELINE_B0, CYTTSP_I2C_BUF_SIZE, &baseline_b0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_BASELINE_B0, baseline_b0);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_REG_BASELINE_B1, CYTTSP_I2C_BUF_SIZE, &baseline_b1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n",__func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_REG_BASELINE_B1, baseline_b1);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_SIGNAL_BTN0_L, CYTTSP_I2C_BUF_SIZE, &singal_0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n",__func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_SIGNAL_BTN0_L, singal_0);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_SIGNAL_BTN1_L, CYTTSP_I2C_BUF_SIZE, &singal_1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_SIGNAL_BTN1_L, singal_1);

	speed_mode = cyttsp_read_reg(data, CYTTSP_SPEED_MODE);
	if(speed_mode < 0) {
		hwlog_err("%s: %d read reg faild\n",__func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%02x\n", __func__, CYTTSP_SPEED_MODE, speed_mode);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_BTN0_RAW_START, CYTTSP_I2C_BUF_SIZE, &raw_button_0);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_BTN0_RAW_START, raw_button_0);

	error = cyttsp_i2c_read_block(&data->client->dev, CYTTSP_BTN1_RAW_START, CYTTSP_I2C_BUF_SIZE, &raw_button_1);
	if(error < 0) {
		hwlog_err("%s: %d read reg faild\n", __func__, __LINE__);
	}
	hwlog_warn("%s: reg 0x%02x = 0x%04x\n", __func__, CYTTSP_BTN1_RAW_START, raw_button_1);

	return true;

data_null:
	return false;
}

static bool touchkey_cp_test(struct cyttsp_button_data *data, char (*result_array)[CYTTSP_REASON_LENGTH],
				char (*reason_array)[CYTTSP_REASON_LENGTH], int test_num)
{
	char m_reason_array[CYTTSP_CP_NUM][CYTTSP_REASON_LENGTH] = {{0}, {0}};
	int val = 0;
	unsigned char reg_array[CYTTSP_CP_NUM] = {CYTTSP_CP_BTN0, CYTTSP_CP_BTN1};
	int total_reg = CYTTSP_CP_NUM;
	int current_round = 0;
	int ret = 0;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == data || NULL == result_array || NULL == reason_array) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}
	pdata = data->pdata;
	if(NULL == pdata) {
		hwlog_err("%s: param pdata NULL!\n", __func__);
		goto param_null;
	}

	/*If you want to read cp, you should write CYTTSP_TRIGGER_MEASURE_CP in reg CYTTSP_REG_CAP_EN firstly,
	**Then delay CYTTSP_READ_CP_DELAY (ms)
	*/
	ret = cyttsp_write_reg(data, CYTTSP_REG_CAP_EN, CYTTSP_TRIGGER_MEASURE_CP);
	if(ret) {
		hwlog_err("%s: cyttsp_write_reg error\n", __func__);
		strcpy(*(result_array + test_num), CYTTSP_WRITE_REG_ERR);
		goto err_write_reg;
	}
	msleep(CYTTSP_READ_CP_DELAY);

	for(current_round = 0; current_round < total_reg; current_round++) {
		val = cyttsp_read_reg(data, reg_array[current_round]);
		pdata->cypress_store_cp[current_round] = val;
		if(val < 0) {
			strcpy(*(result_array + test_num), CYTTSP_READ_REG_ERR);
			goto err_read_reg;
		} else if(0 == (((val >= pdata->cp_limit[0]) && (val < pdata->cp_limit[1])) ? 1 : 0)) {
			hwlog_err("%s: CYTTSP_CP_BTN not in limit! val is %d\n", __func__, val);
			sprintf(*(m_reason_array + current_round), "reason:cp(%d);", val);
			break;
		}
		sprintf(*(m_reason_array + current_round), "cp:%d", val);
	}

	if(strstr(*m_reason_array, "reason")) {
		strncpy(*(reason_array + test_num), *m_reason_array, strlen(*m_reason_array));
		goto not_in_limit;
	} else {
		if(strstr(*(m_reason_array + 1), "reason")) {
			strcpy(*(reason_array + test_num), *(m_reason_array + 1));
			goto not_in_limit;
		} else {
			strcpy(*(reason_array + test_num), *m_reason_array);
			strcat(*(reason_array + test_num), ",");
			strcat(*(reason_array + test_num), *(m_reason_array + 1));
			strcat(*(reason_array + test_num), ";");
		}
	}
	sprintf(*(result_array + test_num), "-%dP", test_num);

	return true;

err_write_reg:
err_read_reg:
not_in_limit:
	sprintf(*(result_array + test_num), "-%dF", test_num);
param_null:
	return false;
}

static bool i2c_test(struct cyttsp_button_data *data, char (*result_array)[CYTTSP_REASON_LENGTH],
				char (*reason_array)[CYTTSP_REASON_LENGTH], int test_num)
{
	int ret = 0;

	if(NULL == data || NULL == result_array || NULL == reason_array) {
		hwlog_err("%s: param NULL!\n", __func__);
		goto param_null;
	}

	ret = cyttsp_read_reg(data, CYTTSP_REG_WORKMODE);
	if(ret < 0) {
		hwlog_err("%s: cyttsp_read_reg error, I2C err\n", __func__);
		goto err_test_i2c;
	}
	sprintf(*(result_array + test_num), "-%dP", test_num);
	sprintf(*(reason_array + test_num), CYTTSP_I2C_OK);
	return true;

err_test_i2c:
	sprintf(*(reason_array + test_num), CYTTSP_I2C_ERR);
	sprintf(*(result_array + test_num), "-%dF", test_num);
param_null:
	return false;
}

static int rawdata_proc_show(struct seq_file *m, void *v)
{
	char m_reason_array[CYTTSP_CP_TEST_NUM][CYTTSP_REASON_LENGTH] = {{0}, {0}, {0}, {0}, {0}, {0}};
	char m_result_array[CYTTSP_CP_TEST_NUM][CYTTSP_REASON_LENGTH] = {{0}, {0}, {0}, {0}, {0}, {0}};
	struct i2c_client *client = NULL;
	struct cyttsp_button_data *data = NULL;
	bool ret = 0;

	if(NULL == global_dev) {
		hwlog_err("%s: param global_dev is NULL\n", __func__);
		seq_printf(m, CYTTSP_PROBE_FAILED);
		seq_printf(m, CYTTSP_I2C_MMI_ERR);
		goto err_param;
	}

	client = to_i2c_client(global_dev);
	if(NULL == client) {
		hwlog_err("%s: param client is NULL\n", __func__);
		seq_printf(m, CYTTSP_PROBE_FAILED);
		seq_printf(m, CYTTSP_I2C_MMI_ERR);
		goto err_param;
	}
	data = i2c_get_clientdata(client);
	if(NULL == data) {
		hwlog_err("%s: param data is NULL\n", __func__);
		seq_printf(m, CYTTSP_PROBE_FAILED);
		seq_printf(m, CYTTSP_I2C_MMI_ERR);
		goto err_param;
	}
	if(NULL == data->pdata) {
		hwlog_err("%s: param pdata is NULL\n", __func__);
		seq_printf(m, CYTTSP_PROBE_FAILED);
		seq_printf(m, CYTTSP_I2C_MMI_ERR);
		goto err_param;
	}

	ret = i2c_test(data, m_result_array, m_reason_array, CYTTSP_MMI_TEST_ONE);
	if(false == ret) {
		hwlog_err("%s: i2c test err\n", __func__);
		goto err_i2c;
	}

	ret = touchkey_cp_test(data, m_result_array, m_reason_array, CYTTSP_MMI_TEST_TWO);
	if(false == ret) {
		hwlog_err("%s: cp test err\n", __func__);
	}

	ret = touchkey_noise_test(data, m_result_array, m_reason_array, CYTTSP_MMI_TEST_THREE);
	if(false == ret) {
		hwlog_err("%s: noise test err\n", __func__);
	}

	ret = touchkey_idac_test(data, m_result_array, m_reason_array, CYTTSP_MMI_TEST_FOUR);
	if(false == ret) {
		hwlog_err("%s: idac test err\n", __func__);
	}
	ret = touchkey_cmod_test(data, m_result_array, m_reason_array, CYTTSP_MMI_TEST_FIVE);
	if(false == ret) {
		hwlog_err("%s: cmod test err\n", __func__);
	}

	/*if cp is zero ,it shows is IC error*/
	if((CYTTSP_ERR_CP_RET == data->pdata->cypress_store_cp[CYTTSP_KEY1]) || (CYTTSP_ERR_CP_RET == data->pdata->cypress_store_cp[CYTTSP_KEY2])) {
		hwlog_err("%s: cp is zero, error\n", __func__);
		ret = cyttsp_dump_all_regs(data);
		if(false == ret) {
			hwlog_err("%s: dump all regs error\n", __func__);
		}
	}

	/*seq reason*/
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_ONE));
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_TWO));
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_THREE));
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_FOUR));
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_FIVE));
	seq_printf(m, *(m_reason_array + CYTTSP_REASON_OF_TEST_SIX));

	//seq result
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_SIX));
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_FIVE));
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_FOUR));
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_THREE));
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_TWO));
	seq_printf(m, *(m_result_array + CYTTSP_MMI_TEST_ONE));

	goto end;

err_i2c:
	seq_printf(m, *m_reason_array);
	seq_printf(m, CYTTSP_I2C_MMI_ERR);
err_param:
end:
	return 0;
}

static int mmi_support_show(struct seq_file *m, void *v)
{
	struct i2c_client *client = NULL;
	struct cyttsp_button_data *data = NULL;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == global_dev) {
		hwlog_err("%s: param global_dev is NULL\n", __func__);
		seq_printf(m, CYTTSP_NULL_DATA);
		goto err_param;
	}

	client = to_i2c_client(global_dev);
	data = i2c_get_clientdata(client);
	pdata = data->pdata;

	if(1 == pdata->if_support_mmi) {
		seq_printf(m, CYTTSP_SUPPORT_FUNC);
	} else {
		seq_printf(m, CYTTSP_NOT_SUPPORT_FUNC);
	}

	if(1 == pdata->if_support_running) {
		seq_printf(m, CYTTSP_SUPPORT_FUNC);
	} else {
		seq_printf(m, CYTTSP_NOT_SUPPORT_FUNC);
	}

err_param:
	return 0;
}

static int rawdata_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rawdata_proc_show, NULL);
}

static int mmi_support_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmi_support_show, NULL);
}

const struct file_operations raw_data_fops = {
	.open = rawdata_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations mmi_support_fops = {
	.open = mmi_support_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void procfs_create(struct device *dev)
{
	if(NULL == dev)
		goto goback;

	global_dev = dev;
	if(!proc_mkdir("touchkey", NULL))
		goto goback;

	proc_create("touchkey/touchkey_capacitance_data",
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL, &raw_data_fops);
	proc_create("touchkey/if_support_mmi",
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL, &mmi_support_fops);

goback:
	return;
}

static int cyttsp_need_update_or_not(struct cyttsp_button_data *data,
							const struct firmware *fw)
{
	unsigned char buffer[CYTTSP_FW_ORG_DATA_LEN];
	const unsigned char *curr = fw->data;
	unsigned char id[CYTTSP_NEED_UPDATE_SIZE] = {0};
	int in_chip_id[CYTTSP_NEED_UPDATE_SIZE] = {0};
	int total = fw->size;
	int ret;

	while (total != 0) {
		ret = cyttsp_get_single_line(curr, buffer);
		total -= ret;
		curr += ret;
	}

	/* array element 56 and 58 contain fw version in image */
	id[1] = cyttsp_convert_to_u8(&buffer[57]);
	id[0] = cyttsp_convert_to_u8(&buffer[55]);

	/* get fw version in chip at addr 0x46 */
	ret = cyttsp_read_reg(data, CYTTSP_BUTTON_FW_VER1);
	if (ret < 0) {
		hwlog_err("%s: fail to read fw id0!\n", __func__);
		return false;
	}
	in_chip_id[0] = ret;

	/* get fw version in chip at addr 0x47 */
	ret = cyttsp_read_reg(data, CYTTSP_BUTTON_FW_VER2);
	if (ret < 0) {
		hwlog_err("%s: fail to read fw id1!\n", __func__);
		return false;
	}
	in_chip_id[1] = ret;

	hwlog_info("%s: image version = 0x%02x 0x%02x,"
		" in chip version = 0x%02x 0x%02x\n", __func__,
			id[0], id[1], in_chip_id[0], in_chip_id[1]);

	if (id[0] != (unsigned char)in_chip_id[0] || id[1] != (unsigned char)in_chip_id[1]) {
		return true;
	}

	return false;
}

static int cyttsp_fw_update_proc(struct cyttsp_button_data *data)
{
	int i = 0;
	int ret = 0;
	int total =0;
	unsigned char cmd[CYTTSP_FW_ORG_DATA_LEN];
	unsigned char buffer[CYTTSP_FW_ORG_DATA_LEN];
	bool need_update = false;
	const unsigned char *tmp_buf;
	const char *fw_info = "ts/touch_key/stf_cyp_psoc4000.cyacd";
	unsigned char val = 0;
	int error = 0;

	/* btld entrance: 0x04
	* cmd sequence to switch into btld mode: 0x2B, 0x2B, 0xFE, 0xFA
	*/
	unsigned char switch_to_btld_cmd[CYTTSP_BTLD_CMD_BUFF] = {0x04, 0x2B, 0x2B, 0xFE, 0xFA};

	const struct firmware *fw = NULL;
	struct device *dev = &data->client->dev;
	struct cyttsp_button_platform_data *pdata = data->pdata;

	disable_irq(pdata->irq_num);

	/* request fw image from bank */
	ret = request_firmware(&fw, fw_info, dev);
	if (ret < 0) {
		hwlog_err("%s: fail to request firmware %s\n", __func__, fw_info);
		goto recovery;
	}

	total = fw->size;
	tmp_buf = fw->data;

	if (false == in_bootloader) {
		/* get fw version in ic */
		ret = cyttsp_i2c_read_block(&data->client->dev,
					CYTTSP_BUTTON_FW_VER1, CYTTSP_FW_VER_SIZE,
					data->fw_version);
		if (ret) {
			hwlog_err("%s: unable to get fw version\n", __func__);
			goto free_fw;
		}
		hwlog_info("%s: ic fw version: 0x%02x 0x%02x\n", __func__,
				(unsigned int)data->fw_version[0], (unsigned int)data->fw_version[1]);

		/* check version and if not same, update fw */
		need_update = cyttsp_need_update_or_not(data, fw);
		if (!need_update) {
				hwlog_info("%s: same ver between ic and image, NO need to update\n",
				__func__);
			goto free_fw;
		} else {
				hwlog_info("%s: ver's diff between ic and image, NEED update\n",
				__func__);
		}

		ret = cyttsp_i2c_send(dev, sizeof(switch_to_btld_cmd), switch_to_btld_cmd);
		if (ret) {
			hwlog_err("%s-%d: fail to switch to bootloader\n", __func__, __LINE__);
			goto free_fw;
		} else {
			hwlog_info("%s-%d: switch to btld ops mode already\n", __func__, __LINE__);
		}
	}

	mdelay(CYTTSP_FWUP_DELAY_COUNT);
	data->app_addr = data->client->addr;
	data->bl_addr = pdata->bl_addr;
	data->client->addr = data->bl_addr;

	error = cyttsp_i2c_recv(dev, 1, &val);
	if (error != 0) {
		data->client->addr = CYTTSP_NEW_BL_ADDR;
	}
	hwlog_info("%s: connect to bootloader addr is 0x%02x\n", __func__,data->client->addr);

	/* 1. enter into bootloader */
	ret = cyttsp_send_command(data, CYTTSP_CMD_ENTER_BTLD, NULL, 0, 0);
	if (ret) {
		hwlog_err("%s: fail to enter into bootloader\n", __func__);
		goto exit_bootloader;
	}

	/* 2. get flash size */
	cmd[0] = 0;
	ret = cyttsp_send_command(data, CYTTSP_CMD_GET_FLASH_SIZE, cmd, 1, 0);
	if (ret) {
		hwlog_err("%s: fail to get flash size\n", __func__);
		goto exit_bootloader;
	}

	ret = cyttsp_get_single_line(tmp_buf, buffer);
	total -= ret;
	tmp_buf += ret;

	/* 3. send data and program, verify*/
	while (total > 0) {

		int j;
		unsigned char tmp[CYTTSP_PROGRAM_DATA_LEN];
		unsigned char array_id;
		unsigned char data_buf[CYTTSP_AVAILABLE_DATA_LEN];
		unsigned char row_checksum;
		unsigned short row_num, length;
		unsigned char length_u8;

		ret = cyttsp_get_single_line(tmp_buf, buffer);
		total -= ret;
		tmp_buf += ret;

		array_id = cyttsp_convert_to_u8(&buffer[1]);
		row_num = cyttsp_convert_to_u16(&buffer[3]);
		length = cyttsp_convert_to_u16(&buffer[7]);
		length_u8 = (unsigned char)length;
		hwlog_debug("%s: array_id = 0x%02x, row_num = 0x%02x, len = 0x%02x\n",
				__func__, array_id, row_num, length);

		/* the following step will get ride of the inavailable data
		** in each line of .cyacd file, and put the usefull data into
		** data_buf array.
		** format of those inavailable data is like this:
		** 4 byte silicon id;
		** 1 byte silicon rev;
		** 1 byte checksum data;
		** 1 byte array id;
		** 2 byte row number;
		** 2 byte data length
		*/
		for (j = 0; j < length_u8 * 2; j += 2) {
			data_buf[j/2] = cyttsp_convert_to_u8(&buffer[11+j]);
		}

		/* send data here */
		ret = cyttsp_send_command(data, CYTTSP_CMD_SEND_DATA,
					&data_buf[0], length - 1, 0);
		if (ret) {
			hwlog_err("%s: fail to send data at round %d\n", __func__, i);
			goto exit_bootloader;
		}

		/* program here */
		tmp[0] = 0x00;
		tmp[1] = row_num & CYTTSP_8BITS_MASK;
		tmp[2] = (row_num & CYTTSP_16BITS_MASK) >> CYTTSP_8BITS_SHIFT;
		tmp[3] = data_buf[length - 1];
		ret = cyttsp_send_command(data, CYTTSP_CMD_PROGRAM_ROW, tmp, 4, 0);
		if (ret) {
			hwlog_err("%s: fail to program row at round %d!\n", __func__, i);
			goto exit_bootloader;
		}

		/* verify row */
		row_checksum = calculate_checksum8(&data_buf[0], length);
		tmp[0] = 0x00;
		tmp[1] = row_num & CYTTSP_8BITS_MASK;
		tmp[2] = (row_num & CYTTSP_16BITS_MASK) >> CYTTSP_8BITS_SHIFT;
		ret = cyttsp_send_command(data, CYTTSP_CMD_VERIFY_ROW, tmp, 3, row_checksum);
		if (ret) {
			hwlog_err("%s: fail to verify row at round %d\n", __func__, i);
			goto exit_bootloader;
		}
		i++;
	}
	hwlog_err("%s: touch key update FW success\n", __func__);
exit_bootloader:
	/* 4. Exit bootloader mode */
	cmd[0] = 0;
	ret = cyttsp_send_command(data, CYTTSP_CMD_EXIT_BTLD, cmd, 1, 0);
	if (ret) {
		hwlog_err("%s: fail to exit bootloader mode\n", __func__);
	} else {
		hwlog_info("%s: back into normal mode already\n", __func__);
	}
	data->client->addr = data->app_addr;
free_fw:
	release_firmware(fw);
recovery:
	enable_irq(pdata->irq_num);

	return ret;
}

static int cyttsp_fw_update_thread(void *arg)
{
	int error = 0;
	struct cyttsp_button_data *data = i2c_get_clientdata(this_client);

	if(kthread_should_stop()) {
		hwlog_err("%s: thread had stopped\n", __func__);
		return -EIO;
	}
	hwlog_info("%s: fw update process running\n", __func__);

	fw_update_flag = PROC_ON;
	error = cyttsp_fw_update_proc(data);
	if (error < 0) {
		cyp_report_dmd(CYTTSP_DSM_FW_UPDATE_ERR, DSM_TOUCHKEY_FW_UPDATE_ERR);

		fw_update_flag = PROC_OFF;
		hwlog_info("%s: fw update failed\n", __func__);
		return -EIO;
	}
	fw_update_flag = PROC_OFF;

	return 0;
}

void touchkey_dsm_func(void)
{
	cyp_report_dmd(CYTTSP_DSM_IRQ_I2C_ERR, DSM_TOUCHKEY_HANDLER_I2C_ERR);
}

/* 
 * touch fix implemented by zxz0O0
 * some devices did not properly update touch firmware, therefore did not correctly respond to requests
 */
static int cyttsp_button_probe(struct i2c_client *client)
{
	unsigned char val = 0;
	struct device *dev = NULL;
	int i = 0;
	int error = 0;
	int error2 = 0;
	int pollution  = 0;
	struct cyttsp_button_data *data = NULL;
	struct cyttsp_button_platform_data *pdata = NULL;

	if(NULL == client) {
		hwlog_err("%s: client is NULL\n", __func__);
		return -EIO;
	}
	pdata = client->dev.platform_data;

	in_bootloader = false;
	fw_update_flag = PROC_OFF;
	hwlog_info("%s: cyttsp button's probing\n", __func__);

	/* i2c sm-bus check */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		hwlog_err("%s: SMBus byte data not supported\n", __func__);
		return -EIO;
	}

#ifdef CONFIG_HUAWEI_DSM
	if(!touchkey_dclient) {
		touchkey_dclient = dsm_register_client(&dsm_touch_key);
		if(NULL == touchkey_dclient) {
			hwlog_err("%s: dsm_register_client failed, touchkey_dclient NULL!\n", __func__);
		}
	}
#endif

	/* get and set dts node info */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct cyttsp_button_platform_data), GFP_KERNEL);
		if (!pdata) {
			hwlog_err("%s: fail to allocate memroy for pdata\n", __func__);
			error = -ENOMEM;
			goto err_pdata_request;
		}
		error = cyttsp_button_parse_dt(&client->dev, pdata);
		if (error) {
			hwlog_err("%s: parse dts failed\n", __func__);
			goto parse_dt_error;
		}
	} else {
		pdata = client->dev.platform_data;
	}

	data = kzalloc(sizeof(struct cyttsp_button_data), GFP_KERNEL);
	if (!data) {
		hwlog_err("%s: fail to allocate memory for data!\n", __func__);
		error = -ENOMEM;
		goto err_data_request;
	}

	this_client = client;
	data->client = client;
	data->pdata = pdata;

	/* setup irq gpio */
	error = gpio_request(pdata->irq_gpio, "cyttsp_button_irq_gpio");
	if (error) {
		hwlog_err("%s: unable to request gpio [%d]\n",
				__func__, pdata->irq_gpio);
		goto err_irq_gpio_req;
	}

	error = gpio_direction_input(pdata->irq_gpio);
	if (error) {
		hwlog_err("%s: unable to set direction for gpio [%d]\n",
				__func__, pdata->irq_gpio);
		goto err_irq_gpio_req_direction;
	}

	pdata->irq_num = gpio_to_irq(pdata->irq_gpio);
	if (!pdata->irq_num) {
		hwlog_err("%s: fail to request irq numbers\n", __func__);
		error = pdata->irq_num;
		goto err_request_irq_num;
	}

	i2c_set_clientdata(data->client, data);

	/* Initialize input device */
	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		hwlog_err("%s: fail to allocate input device\n", __func__);
		goto err_allocate_input;
	}

	data->input_dev->name = CYTTSP_INPUT_DEVICE_NAME;

	input_set_drvdata(data->input_dev, data);
	dev_set_drvdata(&data->client->dev, data);

	data->input_dev->id.bustype = BUS_I2C;
	data->input_dev->dev.parent = &data->client->dev;

	/* power on button */
	error = cyttsp_button_power_on(data->input_dev);
	if (error) {
		hwlog_err("%s(%d): fail to power on button\n", __func__,
			__LINE__);
		goto err_request_irq;
	}

	data->enable = false;
#ifdef CONFIG_FB
	data->fb_notif.notifier_call = fb_notifier_callback;
	error = fb_register_client(&data->fb_notif);
	if (error) {
		hwlog_err("%s: unable to register fb_notifier: %d", __func__, error);
		goto err_register_fb;
	}
#endif

	for (i = 0; i < pdata->nbuttons; i++) {
		input_set_capability(data->input_dev, EV_KEY, pdata->key_code[i]);
		hwlog_info("%s: key_code[%d] = %d\n", __func__, i + 1, pdata->key_code[i]);
	}

	__set_bit(EV_SYN, data->input_dev->evbit);
	__set_bit(EV_KEY, data->input_dev->evbit);

	error = input_register_device(data->input_dev);
	if (error) {
		hwlog_err("%s: unable to register input device, error: %d\n", __func__, error);
		goto err_register_device;
	}

	/* request irq and install interrupt handler */
	error = request_threaded_irq(pdata->irq_num, NULL, cyttsp_button_interrupt_handler,
					pdata->irq_flags, client->dev.driver->name, data);
	if (error) {
		hwlog_err("%s: error %d registering irq\n", __func__, error);
		goto err_request_irq;
	}
	disable_irq(pdata->irq_num);
	INIT_WORK(&(data->touchkey_dsm_work_struct), touchkey_dsm_func);

	/* make debug sysfs nodes */
	error = sysfs_create_group(&client->dev.kobj, &cyttsp_attr_group);
	if (error) {
		hwlog_err("%s: failure %d creating sysfs group\n", __func__, error);
		goto err_create_sys_group;
	}

	procfs_create(&client->dev);

	dev = &data->client->dev;
	mdelay(CYTTSP_WAKE_DELAY_COUNT);
	error = cyttsp_i2c_read_block(dev, CYTTSP_REG_TOUCHMODE, 1, &val);
	if (error < 0) {
		pollution = data->client->addr;
		hwlog_info("%s-%d: restore new app client addr = 0x%02x\n", __func__, __LINE__,pollution);
		data->app_addr = pollution;
		data->bl_addr = pdata->bl_addr;
		data->client->addr = data->bl_addr;
		hwlog_info("%s-%d: client addr = 0x%02x\n", __func__, __LINE__,
			data->client->addr);
		/*to read old*/
		error = cyttsp_i2c_recv(dev, 1, &val);
		if (error != 0) {
			data->client->addr = CYTTSP_POLLUTION_SLAVE_ADDR;
			error2 = cyttsp_i2c_read_block(dev, CYTTSP_REG_WORKMODE, 1, &val);
			hwlog_info("%s-%d: try with slave addr = %d\n", __func__, __LINE__, error2);
		}
		if (error != 0 && error2 != 0) {
			data->client->addr = CYTTSP_NEW_BL_ADDR;
			/*if old version bootloader addr is not connect ,then to read new*/
			error = cyttsp_i2c_recv(dev, 1, &val);
			if (error != 0) {
				cyp_report_dmd(CYTTSP_DSM_PROBE_I2C_ERR_1, DSM_TOUCHKEY_PROBE_I2C_ERR);
				hwlog_err("%s: fail to read new btld slave addr\n", __func__);
				hwlog_err("%s: button's not on site\n", __func__);
				goto err_write_no_sleep_mode;
			}else {
				pdata->bl_addr = CYTTSP_NEW_BL_ADDR;
				data->client->addr = pollution;
				in_bootloader = true;
				hwlog_info("%s-%d: succ to read from new btld addr, in btld mode\n", __func__, __LINE__);
			}
		} else {
			/*check it is old version bootloader addr or new version app addr.*/
			error = cyttsp_i2c_read_block(dev, CYTTSP_REG_LOCKDOWN_INFO, 1, &val);
			hwlog_info("%s-%d: reg_lockdown error = %d, val = %d\n", __func__, __LINE__, error, val);
			/*it is new version app addr,need go back bootloader mode*/
			if(CYTTSP_NEW_VERSION_APP == val){
				unsigned char switch_to_new_btld_cmd[CYTTSP_BTLD_CMD_BUFF] = {0x06, 0x2B, 0x2B, 0xFE, 0xFA};
				error = cyttsp_i2c_send(dev, sizeof(switch_to_new_btld_cmd), switch_to_new_btld_cmd);
				if (error) {
					cyp_report_dmd(CYTTSP_DSM_PROBE_I2C_ERR_2, DSM_TOUCHKEY_PROBE_I2C_ERR);
					hwlog_err("%s-%d: fail to switch to new bootloader\n", __func__, __LINE__);
				} else {
					hwlog_info("%s-%d: switch to new btld ops mode already\n", __func__, __LINE__);
					pdata->bl_addr = CYTTSP_NEW_BL_ADDR;
					data->client->addr = pollution;
					in_bootloader = true;
				}
			}else{
				if (data->client->addr == CYTTSP_POLLUTION_SLAVE_ADDR) {
					/* cmd sequence to switch into btld mode: 0x2B, 0x2B, 0xFE, 0xFA */
					unsigned char switch_to_btld_cmd[5] = {0x04, 0x2B, 0x2B, 0xFE, 0xFA};
					hwlog_info("%s-%d: trying to switch slave in bl mode\n", __func__, __LINE__);
					error = cyttsp_i2c_send(dev, sizeof(switch_to_btld_cmd), switch_to_btld_cmd);
					if (error) {
						cyp_report_dmd(CYTTSP_DSM_PROBE_I2C_ERR_2, DSM_TOUCHKEY_PROBE_I2C_ERR);
						hwlog_err("%s-%d: fail to switch to bootloader\n", __func__, __LINE__);
						goto err_write_no_sleep_mode;
					} else {
						hwlog_info("%s-%d: switch to btld ops mode already\n", __func__, __LINE__);
					}
					data->client->addr = pollution;
					in_bootloader = true;
                 	 		hwlog_info("%s-%d: data->client->addr = 0x%02x\n", __func__, __LINE__,
						data->client->addr);
				} else {
					in_bootloader = true;
					data->client->addr = data->app_addr;
					hwlog_info("%s-%d: get back client addr = 0x%02x\n", __func__, __LINE__,
						data->client->addr);
					hwlog_info("%s-%d: succ to read from btld addr, in btld mode\n", __func__, __LINE__);
				}
			}
		}
	}else {
		hwlog_info("%s: succ to read from app addr, in app mode\n", __func__);
		hwlog_info("%s-%d: data->client->addr = 0x%02x\n", __func__, __LINE__,
			data->client->addr);

		/* wake ic up from deep sleep mode */
		mdelay(CYTTSP_WAKE_DELAY_COUNT);
		error = cyttsp_write_reg(data, pdata->work_mode_reg, CYTTSP_NORMAL_MODE);
		if (!error) {
			hwlog_info("%s: ic awaken from sleep mode\n", __func__);
		} else {
			cyp_report_dmd(CYTTSP_DSM_PROBE_I2C_ERR_3, DSM_TOUCHKEY_PROBE_I2C_ERR);

			hwlog_err("%s: wake ic from sleep mode failed\n", __func__);
			goto err_write_no_sleep_mode;
		}
	}

	kthread_run(cyttsp_fw_update_thread, NULL, "cyp_key_updatethread");

	enable_irq(pdata->irq_num);
	hwlog_info("%s: button probe done\n", __func__);

	return 0;
err_write_no_sleep_mode:
	sysfs_remove_group(&client->dev.kobj, &cyttsp_attr_group);
err_create_sys_group:
	free_irq(pdata->irq_num, data);
err_request_irq:
	global_dev = NULL;
	input_unregister_device(data->input_dev);
err_register_device:
err_register_fb:
#ifdef CONFIG_FB
	fb_unregister_client(&data->fb_notif);
#endif
	input_free_device(data->input_dev);
err_allocate_input:
err_request_irq_num:
err_irq_gpio_req_direction:
	if (gpio_is_valid(pdata->irq_gpio)) {
		gpio_free(pdata->irq_gpio);
	}
err_irq_gpio_req:
	kfree(data);
err_data_request:
parse_dt_error:
err_pdata_request:
	hwlog_err("%s: Button probe failed\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
	if(touchkey_dclient) {
		dsm_unregister_client(touchkey_dclient, &dsm_touch_key);
		touchkey_dclient = NULL;
	}
#endif
	return error;
}

static int cyttsp_button_remove(struct i2c_client *client)
{
	int error = 0;
	struct cyttsp_button_data *data = i2c_get_clientdata(client);
	const struct cyttsp_button_platform_data *pdata = data->pdata;

	sysfs_remove_group(&client->dev.kobj, &cyttsp_attr_group);

	error = cyttsp_button_power_off(data->input_dev);
	if (error) {
		hwlog_err("%s(%d): fail to power off button\n", __func__,
			__LINE__);
	}
	disable_irq(client->irq);
	free_irq(client->irq, data);

	input_unregister_device(data->input_dev);
	if (gpio_is_valid(pdata->irq_gpio)) {
		gpio_free(pdata->irq_gpio);
	}

#ifdef CONFIG_FB
	fb_unregister_client(&data->fb_notif);
#endif

#ifdef CONFIG_HUAWEI_DSM
	if(touchkey_dclient) {
		dsm_unregister_client(touchkey_dclient, &dsm_touch_key);
		touchkey_dclient = NULL;
	}
#endif

	kfree(data);
	data = NULL;

	return 0;
}

static const struct i2c_device_id cyttsp_key_id[] = {
	{CYP_TOUCH_KEY_COMPATIBLE_ID, 0},
	{ },
};

static struct of_device_id cyttsp_match_table[] = {
	{
		.compatible = CYP_TOUCH_KEY_COMPATIBLE_ID,
		.data = NULL,
	},
	{ },
};

static struct i2c_driver cyttsp_button_driver = {
	.driver = {
		.name	= CYP_TOUCH_KEY_COMPATIBLE_ID,
		.owner	= THIS_MODULE,
		.of_match_table = cyttsp_match_table,
	},
	.probe		= cyttsp_button_probe,
	.remove		= cyttsp_button_remove,
	.id_table		= cyttsp_key_id,
};

static int __init cyttsp_button_init(void)
{
	hwlog_info("%s: button module init\n", __func__);
	return i2c_add_driver(&cyttsp_button_driver);
}

static void __exit cyttsp_button_exit(void)
{
	hwlog_info("%s: cyttsp touch key exit\n", __func__);
	i2c_del_driver(&cyttsp_button_driver);
}

module_init(cyttsp_button_init);
module_exit(cyttsp_button_exit);

MODULE_AUTHOR("hwx370038, huzheng3@huawei.com, fixed by zxz0O0, ported to EMUI9 by maimaiguanfan");
MODULE_DESCRIPTION("huawei cypress touch key driver");
MODULE_LICENSE("GPL");
