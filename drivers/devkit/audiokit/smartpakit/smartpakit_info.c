/*
** =============================================================================
** Copyright (c) 2017 Huawei Device Co.Ltd
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; version 2.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
** Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
**Author: wangping48@huawei.com
** =============================================================================
*/
// for smartpakit infomation

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/delay.h>

#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/of_device.h>
#include <linux/i2c-dev.h>

#define SUPPORT_DEVICE_TREE
#ifdef SUPPORT_DEVICE_TREE
#include <linux/regulator/consumer.h>
#endif

#include "smartpakit.h"

#define HWLOG_TAG smartpakit
HWLOG_REGIST();

#define SMARTPAKIT_INFO_BUF_MAX    (512)
#define SMARTPAKIT_REG_CTL_COUNT   (32)

typedef struct name_to_index {
    char name[SMARTPAKIT_NAME_MAX];
    unsigned int index;
} name_to_index_t;

#define TO_NAME_INDEX(X)   #X, X

extern smartpakit_priv_t *smartpakit_priv;
extern void smartpakit_ctrl_get_model(char *dst, smartpakit_priv_t *pakit_priv);
extern irqreturn_t smartpakit_i2c_thread_irq(int irq, void *data);
char g_buffer_info[SMARTPAKIT_INFO_BUF_MAX] = {0};

static name_to_index_t soc_platform_name_index[SMARTPAKIT_SOC_PLATFORM_MAX] = {
	{TO_NAME_INDEX(SMARTPAKIT_SOC_PLATFORM_HISI)},
	{TO_NAME_INDEX(SMARTPAKIT_SOC_PLATFORM_QCOM)},
};

#define GET_SOC_PLATDORM(index) \
	soc_platform_name_index[index].name + strlen("SMARTPAKIT_SOC_PLATFORM_")

static name_to_index_t algo_in_name_index[SMARTPAKIT_ALGO_IN_MAX] = {
    {TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_CODEC_DSP)},
	{TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_SOC_DSP)},
	{TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_WITH_DSP)},
	{TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_SIMPLE)},
	{TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_SIMPLE_WITH_I2C)},
	{TO_NAME_INDEX(SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)},
};

#define GET_ALGO_IN(index) \
	algo_in_name_index[index].name + strlen("SMARTPAKIT_ALGO_IN_")

static name_to_index_t out_device_name_index[SMARTPAKIT_OUT_DEVICE_MAX] = {
    {TO_NAME_INDEX(SMARTPAKIT_OUT_DEVICE_SPEAKER)},
	{TO_NAME_INDEX(SMARTPAKIT_OUT_DEVICE_RECEIVER)},
};

#define GET_OUT_DEVICE(index) \
	out_device_name_index[index].name + strlen("SMARTPAKIT_OUT_DEVICE_")

static name_to_index_t chip_vendor_name_index[SMARTPAKIT_CHIP_VENDOR_MAX] = {
	{TO_NAME_INDEX(SMARTPAKIT_CHIP_VENDOR_MAXIM)},
	{TO_NAME_INDEX(SMARTPAKIT_CHIP_VENDOR_NXP)},
	{TO_NAME_INDEX(SMARTPAKIT_CHIP_VENDOR_TI)},
	{TO_NAME_INDEX(SMARTPAKIT_CHIP_VENDOR_OTHER)},
	{TO_NAME_INDEX(SMARTPAKIT_CHIP_VENDOR_CUSTOMIZE)},
};

#define GET_CHIP_VENDOR(index) \
	chip_vendor_name_index[index].name + strlen("SMARTPAKIT_CHIP_VENDOR_")

void smartpakit_append_info(char *fmt, ...)
{
	va_list args;
	char tmp_str[SMARTPAKIT_INFO_BUF_MAX] = {0};

	if(NULL == fmt) {
		hwlog_err("%s, src string is null.", __func__);
		return;
	}

	va_start(args, fmt);
	vscnprintf(tmp_str, SMARTPAKIT_INFO_BUF_MAX, (const char *)fmt, args);
	va_end(args);

	strncat(g_buffer_info, tmp_str, SMARTPAKIT_INFO_BUF_MAX - strlen(g_buffer_info) - 1);
	return;
}
// pa info
static int smartpakit_get_pa_info(char *buffer, const struct kernel_param *kp)
{
	smartpakit_priv_t *kit = smartpakit_priv;
	char chip_model[SMARTPAKIT_NAME_MAX] = {0};
	unsigned int chip_vendor = 0;
	unsigned int device = 0;
	int i = 0;
	int len = 0;

	memset(g_buffer_info, 0, SMARTPAKIT_INFO_BUF_MAX);

	UNUSED(kp);
	if ((NULL == buffer) || (NULL == kit)) {
		hwlog_err("%s: buffer or kit is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// simple pa
	if ((SMARTPAKIT_ALGO_IN_SIMPLE == kit->algo_in) || (SMARTPAKIT_ALGO_IN_SIMPLE_WITH_I2C == kit->algo_in)) {
		hwlog_info("%s: simple pa info:\n", __func__);
		smartpakit_append_info("simple pa info:\n", strlen("simple pa info:\n"));
	} else { // smartpa
		hwlog_info("%s: smartpa info:\n", __func__);
		smartpakit_append_info("simple info:\n", strlen("simple info:\n"));
	}

	// soc_platform
	if (kit->soc_platform < SMARTPAKIT_SOC_PLATFORM_MAX) {
		hwlog_info("%s: soc_platform: %d, %s\n", __func__, kit->soc_platform, GET_SOC_PLATDORM(kit->soc_platform));
		smartpakit_append_info("soc_platform: %d, %s\n", kit->soc_platform, GET_SOC_PLATDORM(kit->soc_platform));
	} else {
		hwlog_info("%s: soc_platform: %d, invalid!!!\n", __func__, kit->soc_platform);
		smartpakit_append_info("soc_platform: %d, invalid!!!\n", kit->soc_platform);
	}

	// algo_in
	if (kit->algo_in < SMARTPAKIT_ALGO_IN_MAX) {
		hwlog_info("%s: algo_in: %d, %s\n", __func__, kit->algo_in, GET_ALGO_IN(kit->algo_in));
		smartpakit_append_info("algo_in: %d, %s\n", kit->algo_in, GET_ALGO_IN(kit->algo_in));
	} else {
		hwlog_info("%s: algo_in: %d, invalid!!!\n", __func__, kit->algo_in);
		smartpakit_append_info("algo_in: %d, invalid!!!\n", kit->algo_in);
	}

	// out_device
	hwlog_info("%s: out_device: 0x%04x\n", __func__, kit->out_device);
	smartpakit_append_info("out_device: 0x%04x\n", kit->out_device);

	// out_device: spk or rcv
	for (i = 0; i < (int)kit->pa_num; i++) { /*lint !e838*/
		device 	= kit->out_device >> (i * SMARTPAKIT_PA_OUT_DEVICE_SHIFT);
		device &= SMARTPAKIT_PA_OUT_DEVICE_MASK;

		switch (device) {
			case SMARTPAKIT_OUT_DEVICE_SPEAKER:  // speaker
			case SMARTPAKIT_OUT_DEVICE_RECEIVER: // receiver
				hwlog_info("%s: out_device: pa%d is %s\n", __func__, i, GET_OUT_DEVICE(device));
				smartpakit_append_info("out_device: pa%d is %s\n", i, GET_OUT_DEVICE(device));
				break;
			default:
				break;
		}
	}

	// pa_num
	hwlog_info("%s: pa_num: %d\n", __func__, kit->pa_num);

	// algo_delay_time
	hwlog_info("%s: algo_delay_time: %d\n", __func__, kit->algo_delay_time);
	smartpakit_append_info("pa_num: %d\nalgo_delay_time: %d\n", kit->pa_num, kit->algo_delay_time);

	// chip_vendor
	if (SMARTPAKIT_ALGO_IN_SIMPLE == kit->algo_in) {
		chip_vendor = SMARTPAKIT_CHIP_VENDOR_OTHER;
		strncpy(chip_model, kit->chip_model, (strlen(kit->chip_model) < SMARTPAKIT_NAME_MAX) ? strlen(kit->chip_model) : (SMARTPAKIT_NAME_MAX - 1));
	} else if (SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN == kit->algo_in) {
		chip_vendor = kit->chip_vendor;
		strncpy(chip_model, kit->chip_model, (strlen(kit->chip_model) < SMARTPAKIT_NAME_MAX) ? strlen(kit->chip_model) : (SMARTPAKIT_NAME_MAX - 1));
	} else if (SMARTPAKIT_ALGO_IN_WITH_DSP == kit->algo_in) {
		if (kit->i2c_priv[0] != NULL) {
			chip_vendor = kit->i2c_priv[0]->chip_vendor;
			strncpy(chip_model, kit->i2c_priv[0]->chip_model,
			(strlen(kit->i2c_priv[0]->chip_model) < SMARTPAKIT_NAME_MAX) ? strlen(kit->i2c_priv[0]->chip_model) : (SMARTPAKIT_NAME_MAX - 1));
		} else {
			chip_vendor = 0xFF;
		}
	} else {
		if (kit->i2c_priv[0] != NULL) {
			chip_vendor = kit->i2c_priv[0]->chip_vendor;
		} else {
			chip_vendor = 0xFF;
		}
		smartpakit_ctrl_get_model(chip_model, kit);
	}

	if (chip_vendor != 0xFF) {
		hwlog_info("%s: chip_vendor: %d, %s\n", __func__, chip_vendor, GET_CHIP_VENDOR(chip_vendor));
		hwlog_info("%s: chip_model: %s\n", __func__, chip_model);
		smartpakit_append_info("chip_vendor: %d, %s\nchip_model: %s\n", chip_vendor, GET_CHIP_VENDOR(chip_vendor), chip_model);
	} else {
		hwlog_info("%s: chip_vendor: invalid!!!\nchip_model: invalid!!!\n", __func__);
		smartpakit_append_info("chip_vendor: invalid!!!\nchip_model: invalid!!!\n");
	}

	len = snprintf(buffer, (unsigned long)SMARTPAKIT_INFO_BUF_MAX, g_buffer_info);
	memset(g_buffer_info, 0, SMARTPAKIT_INFO_BUF_MAX);

	return len;
}

#define SMARTPAKIT_INFO_HELP \
	"Usage:\n" \
	"report remap info:	echo m > reg_ctl\n"	\
	"hw_reset:		echo h > reg_ctl\n" \
	"irq_trigger:		echo i,pa_index > reg_ctl\n" \
	"dump_regs:		echo d > reg_ctl\n" \
	"read_regs:		echo \"r,pa_index,reg_addr,[bulk_count_once]\" > reg_ctl\n" \
	"write_regs:		echo \"w,pa_index,reg_addr,reg_value,[reg_value2...]\" > reg_ctl\n"

typedef struct smartpakit_reg_ctl_params {
	char cmd;
	int params_num;
	union {
		int params[SMARTPAKIT_REG_CTL_COUNT];
		int index_i;
		struct {
			int index_d;
			int bulk_d;
		};
		struct {
			int index_r;
			int addr_r;
			int bulk_r;
		};
		struct {
			int index_w;
			int addr_w;
			int value[0];
		};
	};
} smartpakit_reg_ctl_params_t;

static smartpakit_reg_ctl_params_t reg_ctl_params;
static int reg_ctl_flag = 0;

static int smartpakit_get_reg_ctl(char *buffer, const struct kernel_param *kp)
{
	int ret = 0;

	UNUSED(kp);
	if (NULL == buffer) {
		hwlog_err("%s: buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (0 == reg_ctl_flag) {
		ret = snprintf(buffer, (unsigned long)SMARTPAKIT_INFO_BUF_MAX, SMARTPAKIT_INFO_HELP);
	} else {
		if (strlen(g_buffer_info) > 0) {
			ret = snprintf(buffer, (unsigned long)SMARTPAKIT_INFO_BUF_MAX, g_buffer_info);
			memset(g_buffer_info, 0, SMARTPAKIT_INFO_BUF_MAX);
		} else {
			ret = snprintf(buffer, (unsigned long)SMARTPAKIT_INFO_BUF_MAX,
				"smartpa reg_ctl success!(dmesg -c | grep smartpakit)");
		}
	}

	return ret;
}

static int smartpakit_parse_reg_ctl(const char *val)
{
	char buf[SMARTPAKIT_INFO_BUF_MAX] = { 0 };
	char *tokens = NULL;
	char *pbuf = NULL;
	int index = 0;

	if (NULL == val) {
		hwlog_err("%s: val is NULL!!!\n", __func__);
		return -EINVAL;
	}
	memset(&reg_ctl_params, 0, sizeof(smartpakit_reg_ctl_params_t));

	// ops cmd
	hwlog_info("%s: val = %s\n", __func__, val);
	strncpy(buf, val, (unsigned long)(SMARTPAKIT_INFO_BUF_MAX - 1));
	reg_ctl_params.cmd = buf[0];
	pbuf = &buf[2];

	// parse dump/read/write ops params
	do {
		tokens = strsep(&pbuf, ",");
		if (NULL == tokens) {
			break;
		}
		(void)kstrtoint(tokens, 16, &reg_ctl_params.params[index]);
		hwlog_info("%s: tokens[%d] = %s,%d\n", __func__, index, tokens, reg_ctl_params.params[index]);

		index++;
		if (SMARTPAKIT_REG_CTL_COUNT == index) {
			hwlog_info("%s: params count max is %d!!!\n", __func__, SMARTPAKIT_REG_CTL_COUNT);
			break;
		}
	} while(true);

	reg_ctl_params.params_num = index;
	return 0;
}

static int smartpakit_bulk_read_regs(void)
{
	smartpakit_priv_t *kit = smartpakit_priv;
	void *bulk_value = NULL;
	int bulk_value_bytes = 0;
	int bulk_count_once = reg_ctl_params.bulk_r;
	int index = reg_ctl_params.index_r;
	int addr = reg_ctl_params.addr_r;
	int i = 0;

	if ((NULL == kit) || (NULL == kit->i2c_priv[index]) || (NULL == kit->i2c_priv[index]->regmap_cfg)
		|| (NULL == kit->i2c_priv[index]->regmap_cfg->regmap)) {
		hwlog_err("%s: kit or i2c_priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// bulk read regs: index, addr, bulk_count_once
	bulk_value_bytes = kit->i2c_priv[index]->regmap_cfg->cfg.val_bits / SMARTPAKIT_REG_VALUE_B8;
	if (bulk_value_bytes > 2) {
		hwlog_err("%s: bulk read reg, val_bits %d not supported!!!\n", __func__,
			kit->i2c_priv[index]->regmap_cfg->cfg.val_bits);
		return -EINVAL;
	}

	bulk_value = (unsigned char *)kzalloc(bulk_count_once * bulk_value_bytes, GFP_KERNEL);
	if (NULL == bulk_value) {
		hwlog_err("%s: bulk read reg, malloc failed!!!\n", __func__);
		return -ENOMEM;
	}

	(void)regmap_bulk_read(kit->i2c_priv[index]->regmap_cfg->regmap, addr, bulk_value, bulk_count_once * bulk_value_bytes);
	for (i = 0; i < bulk_count_once; i++) {
		if (1 == bulk_value_bytes) {
			hwlog_info("%s: bulk read reg[0x%x]=0x%x\n", __func__, addr + i, ((unsigned char *)bulk_value)[i]);
		} else { // 2 == bulk_value_bytes
			hwlog_info("%s: bulk read reg[0x%x]=0x%x\n", __func__, addr + i, ((unsigned short *)bulk_value)[i]);
		}
	}

	kfree(bulk_value);
	return 0;
}

static int smartpakit_bulk_write_regs(void)
{
	smartpakit_priv_t *kit = smartpakit_priv;
	void *bulk_value = NULL;
	int bulk_value_bytes = 0;
	int bulk_count_once = reg_ctl_params.params_num - 2;
	int index = reg_ctl_params.index_w;
	int addr = reg_ctl_params.addr_w;
	int i = 0;

	if ((NULL == kit) || (NULL == kit->i2c_priv[index]) || (NULL == kit->i2c_priv[index]->regmap_cfg)
		|| (NULL == kit->i2c_priv[index]->regmap_cfg->regmap)) {
		hwlog_err("%s: kit or i2c_priv is NULL!!!\n", __func__);
		return -EINVAL;
	}

	// bulk write regs: index, addr, value1, value2...
	bulk_value_bytes = kit->i2c_priv[index]->regmap_cfg->cfg.val_bits / SMARTPAKIT_REG_VALUE_B8;
	if (bulk_value_bytes > 2) {
		hwlog_err("%s: bulk write reg, val_bits %d not supported!!!\n", __func__,
			kit->i2c_priv[index]->regmap_cfg->cfg.val_bits);
		return -EINVAL;
	}

	bulk_value = (unsigned char *)kzalloc(bulk_count_once * bulk_value_bytes, GFP_KERNEL);
	if (NULL == bulk_value) {
		hwlog_err("%s: bulk write reg, malloc failed!!!\n", __func__);
		return -ENOMEM;
	}

	// get regs value for bulk write
	for (i = 0; i < bulk_count_once; i++) {
		hwlog_info("%s: pa%d bulk write reg[0x%x]=0x%x\n", __func__, index, addr + i, reg_ctl_params.value[i]);

		if (1 == bulk_value_bytes) {
			((unsigned char *)bulk_value)[i] = (unsigned char)reg_ctl_params.value[i];
		} else { // 2 == bulk_value_bytes
			((unsigned short *)bulk_value)[i] = (unsigned short)reg_ctl_params.value[i];
		}
	}
	regmap_bulk_write(kit->i2c_priv[index]->regmap_cfg->regmap, addr, bulk_value, bulk_count_once * bulk_value_bytes);

	kfree(bulk_value);
	return 0;
}

static int smartpakit_set_reg_ctl(const char *val, const struct kernel_param *kp)
{
	smartpakit_priv_t *kit = smartpakit_priv;
	unsigned int value = 0;
	int index = 0;
	int ret = 0;

	UNUSED(kp);
	if (NULL == kit) {
		hwlog_err("%s: kit is NULL!!!\n", __func__);
		return -EINVAL;
	}
	reg_ctl_flag = 0;

	// simple pa: not support i2c r/w ops
	if ((SMARTPAKIT_ALGO_IN_SIMPLE == kit->algo_in) || (SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN == kit->algo_in)) {
		hwlog_err("%s: not support i2c ops(algo_in=%d)!!!\n", __func__, kit->algo_in);
		return -EFAULT;
	}

	// parse cmd and dump/read/write ops params
	ret = smartpakit_parse_reg_ctl(val);
	if (ret < 0) {
		return ret;
	}

	// dump/read/write ops
	switch (reg_ctl_params.cmd) {
		case 'm': // report remap info
			if ((NULL == kit->i2c_priv[0]) || (NULL == kit->i2c_priv[0]->regmap_cfg)) {
				hwlog_err("%s: i2c_priv is null.\n", __func__);
				return -EINVAL;
			}
			memset(g_buffer_info, 0, SMARTPAKIT_INFO_BUF_MAX);
			hwlog_info("%s: reg_bits=%d,val_bits=%d,max_register=%d.\n", __func__, kit->i2c_priv[0]->regmap_cfg->cfg.reg_bits,
						kit->i2c_priv[0]->regmap_cfg->cfg.val_bits,
						kit->i2c_priv[0]->regmap_cfg->cfg.max_register);

			smartpakit_append_info("reg_bits=%d,val_bits=%d,max_register=%d.\n",
						kit->i2c_priv[0]->regmap_cfg->cfg.reg_bits,
						kit->i2c_priv[0]->regmap_cfg->cfg.val_bits,
						kit->i2c_priv[0]->regmap_cfg->cfg.max_register);
			break;
		case 'h': // hw_reset
			if ((kit->ioctl_ops != NULL) && (kit->ioctl_ops->hw_reset != NULL)) {
				hwlog_info("%s: chip hw_reset by test!!!\n", __func__);
				kit->ioctl_ops->hw_reset(kit);
			}
			break;
		case 'i': // irq trigger
			index = reg_ctl_params.index_i;
			if ((reg_ctl_params.params_num >= 1) && (index < kit->pa_num)) {
				if ((kit->i2c_priv[index] != NULL) && (kit->i2c_priv[index]->irq_handler != NULL)) {
					hwlog_info("%s: pa[%d] trigger irq by test!!!\n", __func__, index);
					smartpakit_i2c_thread_irq(kit->i2c_priv[index]->irq_handler->irq, kit->i2c_priv[index]);
				}
			} else {
				hwlog_info("%s: params_num %d need >=1, index %d need <pa_num %d!\n", __func__,
					reg_ctl_params.params_num, index, kit->pa_num);
				return -EINVAL;
			}
			break;
		case 'd': // dump regs
			if ((kit->ioctl_ops != NULL) && (kit->ioctl_ops->dump_regs != NULL)) {
				hwlog_info("%s: dump regs by test!!!\n", __func__);
				kit->ioctl_ops->dump_regs(kit);
			}
			break;
		case 'r':
			index = reg_ctl_params.index_r;
			if ((reg_ctl_params.params_num >= 2) && (index < kit->pa_num)) {
				if (reg_ctl_params.bulk_r > 0) {
					smartpakit_bulk_read_regs();
					break;
				}

				if ((kit->i2c_priv[index] != NULL) && (kit->i2c_priv[index]->regmap_cfg != NULL)
					&& (kit->i2c_priv[index]->regmap_cfg->regmap != NULL)) {
					(void)regmap_read(kit->i2c_priv[index]->regmap_cfg->regmap, reg_ctl_params.addr_r, &value);
					hwlog_info("%s: pa%d read reg[0x%x]=0x%x\n", __func__, index, reg_ctl_params.addr_r, value);
					memset(g_buffer_info, 0, SMARTPAKIT_INFO_BUF_MAX);
					snprintf(g_buffer_info, (unsigned long)SMARTPAKIT_INFO_BUF_MAX, "reg[0x%x]=0x%04x\n", reg_ctl_params.addr_r, value);
				}
			} else {
				hwlog_info("%s: params_num %d need >=2, index %d need <pa_num %d!\n", __func__,
					reg_ctl_params.params_num, index, kit->pa_num);
				return -EINVAL;
			}
			break;
		case 'w':
			index = reg_ctl_params.index_w;
			if ((reg_ctl_params.params_num >= 3) && (index < kit->pa_num)) {
				if (reg_ctl_params.params_num > 3) {
					smartpakit_bulk_write_regs();
					break;
				}

				if ((kit->i2c_priv[index] != NULL) && (kit->i2c_priv[index]->regmap_cfg != NULL)
					&& (kit->i2c_priv[index]->regmap_cfg->regmap != NULL)) {
					hwlog_info("%s: pa%d write reg[0x%x]=0x%x\n", __func__, index, reg_ctl_params.addr_w, reg_ctl_params.value[0]);
					regmap_write(kit->i2c_priv[index]->regmap_cfg->regmap, reg_ctl_params.addr_w, reg_ctl_params.value[0]);
				}
			} else {
				hwlog_info("%s: params_num %d need >=3, index %d need <pa_num %d!\n", __func__,
					reg_ctl_params.params_num, index, kit->pa_num);
				return -EINVAL;
			}
			break;
		default:
			hwlog_info("%s: not support cmd(%c/0x%x)!!!\n", __func__, reg_ctl_params.cmd, reg_ctl_params.cmd);
			return -EFAULT;
	}

	// reg_ctl success
	reg_ctl_flag = 1;
	return 0;
}

static struct kernel_param_ops param_ops_pa_info = {
	.get = smartpakit_get_pa_info,
};

static struct kernel_param_ops param_ops_reg_ctl = {
	.get = smartpakit_get_reg_ctl,
	.set = smartpakit_set_reg_ctl,
};

module_param_cb(pa_info, &param_ops_pa_info, NULL, 0444);
module_param_cb(reg_ctl, &param_ops_reg_ctl, NULL, 0644);

MODULE_DESCRIPTION("smartpakit info driver");
MODULE_AUTHOR("wangping<wangping48@huawei.com>");
MODULE_LICENSE("GPL");

