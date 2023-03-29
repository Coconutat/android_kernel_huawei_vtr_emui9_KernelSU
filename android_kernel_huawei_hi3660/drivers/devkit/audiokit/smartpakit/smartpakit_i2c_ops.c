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

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <dsm/dsm_pub.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/list.h>
#include <linux/irq.h>

#define SUPPORT_DEVICE_TREE
#ifdef SUPPORT_DEVICE_TREE
#include <linux/regulator/consumer.h>
#endif

#ifdef CONFIG_HUAWEI_DSM_AUDIO_MODULE
#define CONFIG_HUAWEI_DSM_AUDIO
#endif
#ifdef CONFIG_HUAWEI_DSM_AUDIO
#include <dsm_audio/dsm_audio.h>
#endif

#ifdef CONFIG_HUAWEI_DEVICEBOX_AUDIO_MODULE
#define CONFIG_HUAWEI_DEVICEBOX_AUDIO
#endif

#ifdef CONFIG_HUAWEI_DEVICEBOX_AUDIO
#include <deviceboxID/deviceboxID.h>
#endif

#include "smartpakit.h"

#define HWLOG_TAG smartpakit
HWLOG_REGIST();

#ifdef CONFIG_HUAWEI_DSM_AUDIO
#define SMARTPAKIT_DSM_BUF_SIZE DSM_SMARTPA_BUF_SIZE
#endif

// 0 not init completed, 1 init completed
extern int smartpakit_init_flag;
bool smartpakit_i2c_probe_skip[SMARTPAKIT_PA_ID_MAX] = {0 ,0 ,0 ,0};

#define RETRY_TIMES (3)
static inline int smartpakit_regmap_read(struct regmap *regmap, unsigned int reg_addr, unsigned int * value)
{
	int ret = 0, i = 0;
	for (i = 0; i < RETRY_TIMES; i++) {
		ret = regmap_read(regmap, reg_addr, value);
		if(ret == 0) {
			return ret;
		}
		mdelay(1);
	}
	return ret;
}

static inline int smartpakit_regmap_write(struct regmap *regmap, unsigned int reg_addr, unsigned int value)
{
	int ret = 0, i = 0;
	for (i = 0; i < RETRY_TIMES; i++) {
		ret = regmap_write(regmap, reg_addr, value);
		if(ret == 0) {
			return ret;
		}
		mdelay(1);
	}
	return ret;
}

static inline int smartpakit_regmap_update_bits(struct regmap *regmap, unsigned int reg_addr, unsigned int mask, unsigned int value)
{
	int ret = 0, i = 0;
	for (i = 0; i < RETRY_TIMES; i++) {
		ret = regmap_update_bits(regmap, reg_addr, mask, value);
		if(ret == 0) {
			return ret;
		}
		mdelay(1);
	}
	return ret;
}

/*lint -e438 -e838*/
static void smartpakit_dsm_report_by_i2c_error(const char *model, int id, int flag, int errno, struct i2c_err_info* info)
{
	if (0 == errno) {
		return;
	}

#ifdef CONFIG_HUAWEI_DSM_AUDIO
	if (0 == flag) { // read i2c error
		if (info) {
			hwlog_info("%s: dsm report, %s_%d i2c read errno(%d) %u fail times of %u all times, err_details is 0x%lx.\n", __func__, model, id, errno, info->err_count, info->regs_num, info->err_details);
			audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s_%d i2c read errno(%d) %u fail times of %u all times, err_details is 0x%lx.", model, id, errno, info->err_count, info->regs_num, info->err_details);
		} else {
			hwlog_info("%s: dsm report, %s_%d i2c read errno %d.\n", __func__, model, id, errno);
			audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s_%d i2c read errno %d.", model, id, errno);
		}
	} else { // 1 == flag write i2c error
		if (info) {
			hwlog_info("%s: dsm report, %s_%d i2c write errno(%d) %u fail times of %u all times, err_details is 0x%lx.\n", __func__, model, id, errno, info->err_count, info->regs_num, info->err_details);
			audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s_%d i2c write errno(%d) %u fail times of %u all times, err_details is 0x%lx.", model, id, errno, info->err_count, info->regs_num, info->err_details);
		} else {
			hwlog_info("%s: dsm report, %s_%d i2c write errno %d.\n", __func__, model, id, errno);
			audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s_%d i2c write errno %d.", model, id, errno);
		}
	}
#endif
}

static int smartpakit_hw_reset(void *priv)
{
	smartpakit_priv_t *pakit_priv = (smartpakit_priv_t *)priv;
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_gpio_sequence_t *sequence = NULL;
	int ret = 0;
	int i = 0;
	int j = 0;

	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < (int)pakit_priv->pa_num; i++) {
		i2c_priv = pakit_priv->i2c_priv[i];
		if (NULL == i2c_priv) {
			hwlog_err("%s: i2c_priv[%d] invalid argument!!!\n", __func__, i);
			ret = -EINVAL;
			break;
		}

		// If one gpio control multi-pa, some hw_reset of pa maybe NULL
		if (NULL == i2c_priv->hw_reset) {
			continue;
		}

		// set hw_reset debounce
		if (i2c_priv->reset_debounce_wait_time > 0) {
			cancel_delayed_work_sync(&i2c_priv->irq_debounce_work);

			hwlog_info("%s: pa%d reset debounce, trigger delayed_work!\n", __func__, i2c_priv->chip_id);
			i2c_priv->irq_debounce_jiffies = jiffies + msecs_to_jiffies(i2c_priv->reset_debounce_wait_time);
			schedule_delayed_work(&i2c_priv->irq_debounce_work, msecs_to_jiffies(i2c_priv->reset_debounce_wait_time));
		}

		sequence = &i2c_priv->hw_reset->sequence;
		if (sequence->node != NULL) {
			for (j = 0; j < (int)sequence->num; j++) {
				hwlog_err("%s: hw_reset%d=%d,%d\n", __func__,
					i2c_priv->hw_reset->gpio, sequence->node[j].state, sequence->node[j].delay);
				gpio_direction_output((unsigned)i2c_priv->hw_reset->gpio, (int)sequence->node[j].state);
				if (sequence->node[j].delay > 0) {
					// delay time units: msecs
					// last node use msleep
					if (j == (int)(sequence->num - 1)) {
						msleep(sequence->node[j].delay);
					} else {
						mdelay(sequence->node[j].delay); /*lint !e647 !e747 !e774*/
					}
				}
			}
		}
	}

	return ret;
}

static int smartpakit_do_reg_ctl(smartpakit_i2c_priv_t *i2c_priv, smartpakit_reg_ctl_sequence_t *sequence, char *report)
{
#ifdef CONFIG_HUAWEI_DSM_AUDIO
	char report_tmp[SMARTPAKIT_NAME_MAX] = { 0 };
#endif
	unsigned int reg_addr  = 0;
	unsigned int ctl_value = 0;
	unsigned int ctl_type  = 0;
	unsigned int value     = 0;
	int ret = 0;
	int ret_once = 0;
	int i = 0;
	int j = 0;

	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg)
		|| (NULL == i2c_priv->regmap_cfg->regmap)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if ((sequence != NULL) && (sequence->num > 0) && (sequence->regs != NULL)) {
		for (i = 0; i < (int)sequence->num; i++) {
			reg_addr  = sequence->regs[i].addr;
			ctl_value = sequence->regs[i].value;
			ctl_type  = sequence->regs[i].ctl_type;

			if (SMARTPAKIT_REG_CTL_TYPE_W == ctl_type) {
				ret_once = smartpakit_regmap_write(i2c_priv->regmap_cfg->regmap, reg_addr, ctl_value);
				if (1 == i2c_priv->probe_completed) {
					smartpakit_dsm_report_by_i2c_error(i2c_priv->chip_model, (int)i2c_priv->chip_id, 1, ret_once, NULL);
				}
				ret += ret_once;
				hwlog_info("%s: pa[%d], w reg[0x%x] = 0x%x\n", __func__, i2c_priv->chip_id, reg_addr, ctl_value);
			} else if (SMARTPAKIT_REG_CTL_TYPE_DELAY == ctl_type) {
				if (ctl_value > 0) {
					// delay time units: msecs
					msleep(ctl_value);
				}
			} else { // SMARTPAKIT_REG_CTL_TYPE_R
				for (j = 0; j < (int)ctl_value; j++) {
					ret_once = smartpakit_regmap_read(i2c_priv->regmap_cfg->regmap, reg_addr, &value);
					if (1 == i2c_priv->probe_completed) {
						smartpakit_dsm_report_by_i2c_error(i2c_priv->chip_model, (int)i2c_priv->chip_id, 0, ret_once, NULL);
					}
					if (ret_once < 0) {
						ret += ret_once;
					}
					hwlog_info("%s: pa[%d], r reg[0x%x] = 0x%x\n", __func__, i2c_priv->chip_id, reg_addr, value);

					// dsm report
#ifdef CONFIG_HUAWEI_DSM_AUDIO
					if (report != NULL) {
						snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "reg[0x%x]=0x%x,", reg_addr, value);
						strncat(report, report_tmp, SMARTPAKIT_DSM_BUF_SIZE - strlen(report) - 1);
					}
#endif

					// cal reg_addr
					reg_addr++;
				}
			}
		}
	}

	return ret;
}

static int smartpakit_dump_regs(void *priv)
{
	smartpakit_priv_t *pakit_priv = (smartpakit_priv_t *)priv;
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	int ret = 0;
	int i = 0;

	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < (int)pakit_priv->pa_num; i++) {
		i2c_priv = pakit_priv->i2c_priv[i];
		if (NULL == i2c_priv) {
			hwlog_err("%s: i2c_priv[%d] invalid argument!!!\n", __func__, i);
			ret = -EINVAL;
			break;
		}

		hwlog_info("%s: pa[%d], dump regs ...\n", __func__, i);
		ret += smartpakit_do_reg_ctl(i2c_priv, i2c_priv->dump_regs_sequence, NULL);
	}

	return ret;
}

static int smartpakit_read_regs(void *priv, void __user *arg)
{
	smartpakit_i2c_priv_t *i2c_priv = (smartpakit_i2c_priv_t *)priv;
	smartpakit_get_param_t reg;
	int ret = 0;

	if (NULL == i2c_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&reg, (void *)arg, sizeof(smartpakit_get_param_t))) {
		hwlog_err("%s: read reg copy_from_user failed!!!", __func__);
		return -EFAULT;
	}

	if ((i2c_priv->regmap_cfg != NULL) && (i2c_priv->regmap_cfg->regmap != NULL)) {
		ret = smartpakit_regmap_read(i2c_priv->regmap_cfg->regmap, reg.index, &reg.value);
		smartpakit_dsm_report_by_i2c_error(i2c_priv->chip_model, (int)i2c_priv->chip_id, 0, ret, NULL);
		if (ret < 0) {
			hwlog_debug4("%s: regmap_read failed %d!!!", __func__, ret);
		}
	} else {
		reg.value = 0;
	}

	if (copy_to_user((void *)arg, &reg, sizeof(smartpakit_get_param_t))) {
		hwlog_err("%s: send reg value to user failed!!!", __func__);
		return -EFAULT;
	}

	return ret;
}

static bool smartpakit_need_write_regs(smartpakit_priv_t *pakit_priv,
	int id, unsigned int num, smartpakit_param_node_t *regs)
{
	smartpakit_pa_ctl_sequence_t *old = &pakit_priv->poweron_sequence[id];
	unsigned int regs_size = sizeof(smartpakit_param_node_t) * num;

	if ((old->node != NULL) && (old->param_num == num) &&
		(memcmp(old->node, regs, regs_size) == 0))
		return false;

	return true;
}

static int smartpakit_do_write_regs(smartpakit_i2c_priv_t *i2c_priv, unsigned int num, smartpakit_param_node_t *regs)
{
	smartpakit_priv_t *pakit_priv = NULL;
	smartpakit_regmap_cfg_t *cfg = NULL;
	int ret = 0;
	int i = 0;
	int errno = 0;
	struct i2c_err_info info = {
		.regs_num = 0,
		.err_count = 0,
		.err_details = 0,
	};

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: num=%d...\n", __func__, num);
#endif
	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg) || (NULL == i2c_priv->regmap_cfg->regmap) || (NULL == regs)) {
		hwlog_err("%s: Invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	cfg = i2c_priv->regmap_cfg;
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: chip model %s, chip_id %d.\n", __func__, i2c_priv->chip_model, i2c_priv->chip_id);
#endif

	// for smartpa, node type is:
	// 1. write reg node
	// 2. gpio node
	// 3. delay time node
	// 4. skip node
	// 5. rxorw node
	// 6. read reg node
	for (i = 0; i < (int)num; i++) {
		if (SMARTPAKIT_PARAM_NODE_TYPE_DELAY == regs[i].node_type) {
			if (regs[i].delay > 0) {
				// delay time units: msecs
				msleep(regs[i].delay);
			}
			continue;
		} else if (SMARTPAKIT_PARAM_NODE_TYPE_SKIP == regs[i].node_type) {
			//hwlog_info("%s: this param node skip(%d)!!!\n", __func__, i);
			continue;
		} else if (SMARTPAKIT_PARAM_NODE_TYPE_REG_RXORW == regs[i].node_type) {
			int value = 0;

			ret = smartpakit_regmap_read(cfg->regmap, regs[i].value, &value);
			if (ret < 0) {
				hwlog_info("%s: rxorw node(%d/%d), read reg[0x%x] = 0x%x failed ret = %d.\n", __func__, i + 1, num, regs[i].value, value, ret);
			} else {
#ifndef CONFIG_FINAL_RELEASE
				hwlog_info("%s: rxorw node(%d/%d), read reg[0x%x] = 0x%x\n", __func__, i + 1, num, regs[i].value, value);
#endif
			}
			value ^= regs[i].mask;
#ifndef CONFIG_FINAL_RELEASE
			hwlog_info("%s: rxorw node(%d/%d), write reg[0x%x] = 0x%x(after xor 0x%x)\n", __func__,
				i + 1, num, regs[i].index, value, regs[i].mask);
#endif

			ret += smartpakit_regmap_write(cfg->regmap, regs[i].index, value);
		} else if (SMARTPAKIT_PARAM_NODE_TYPE_REG_READ == regs[i].node_type) {
			int value = 0;

			ret = smartpakit_regmap_read(cfg->regmap, regs[i].index, &value);
			if (ret < 0) {
				hwlog_info("%s: read node(%d/%d), read reg[0x%x] = 0x%x failed ret = %d.\n", __func__, i + 1, num, regs[i].index, value, ret);
			} else {
#ifndef CONFIG_FINAL_RELEASE
				hwlog_info("%s: read node(%d/%d), read reg[0x%x] = 0x%x\n", __func__, i + 1, num, regs[i].index, value);
#endif
			}
		}else if (SMARTPAKIT_PARAM_NODE_TYPE_GPIO == regs[i].node_type) { // gpio
			pakit_priv = (smartpakit_priv_t *)i2c_priv->priv_data;
			if (NULL == pakit_priv) {
				hwlog_info("%s: pakit_priv == NULL!!!\n", __func__);
				continue;
			}

			if (regs[i].index >= pakit_priv->switch_num) {
				hwlog_err("%s: Invalid argument, regs[%d].index %d>=%d!!!\n", __func__,
					i, regs[i].index, pakit_priv->switch_num);
				return -EINVAL;
			}

			if (pakit_priv->switch_ctl != NULL) {
				gpio_direction_output((unsigned)pakit_priv->switch_ctl[regs[i].index].gpio, (int)regs[i].value);
				if (regs[i].delay > 0) {
					// delay time units: msecs
					msleep(regs[i].delay);
				}
				continue;
			}
		} else if (0 == (regs[i].mask ^ cfg->value_mask)) {
			ret = smartpakit_regmap_write(cfg->regmap, regs[i].index, regs[i].value);
		} else {
			ret = smartpakit_regmap_update_bits(cfg->regmap, regs[i].index, regs[i].mask, regs[i].value);
		}
		if (ret) {
			hwlog_err("%s: regs[%d]:0x%x, value:0x%x write error(errno:%d).\n", __func__,i, regs[i].index, regs[i].value, ret);
			if (i < I2C_STATUS_B64) {
				info.err_details |= (unsigned long int)1 << i;
			}
			info.err_count++;
			errno = ret;
		}
		if (regs[i].delay > 0) {
			// delay time units: msecs
			// last node use msleep
			if (i == (int)(num - 1)) {
				msleep(regs[i].delay);
			} else {
				mdelay(regs[i].delay); /*lint !e647 !e747 !e774*/
			}
		}
	}
	info.regs_num = num;
	smartpakit_dsm_report_by_i2c_error(i2c_priv->chip_model, (int)i2c_priv->chip_id, 1, errno, &info);
	return ret;
}

static int smartpakit_do_write_regs_all(smartpakit_priv_t *pakit_priv, smartpakit_pa_ctl_sequence_t *sequence)
{
	int pa_num_need_ops  = 0;
	int reg_num_need_ops = 0;
	int index = 0;
	int ret = 0;
	int i = 0;

	// if pa_ctl_num == 0, need init pa_ctl_num and pa_ctl_index
	if (0 == sequence->pa_ctl_num) {
		sequence->pa_ctl_num = pakit_priv->pa_num;

		for (i = 0; i < (int)pakit_priv->pa_num; i++) {
			sequence->pa_ctl_index[i] = (unsigned int)i;
		}
	}

	pa_num_need_ops  = (int)sequence->pa_ctl_num;
	reg_num_need_ops = (int)sequence->param_num;
	if (0 == pa_num_need_ops) {
		hwlog_err("%s: invalid argument, pa_num=%d!!!\n", __func__, pa_num_need_ops);
		return -EINVAL;
	}

	// check reg_num need write
	if ((reg_num_need_ops % pa_num_need_ops) != 0) {
		hwlog_err("%s: invalid argument, pa_num=%d, reg_num=%d!!!\n", __func__, pa_num_need_ops, reg_num_need_ops);
		return -EINVAL;
	}
	reg_num_need_ops /= pa_num_need_ops;

	if ((0 == reg_num_need_ops) || (NULL == sequence->node)) {
		hwlog_err("%s: invalid argument, reg_num=%d, sequence->node=%pK!!!\n", __func__, reg_num_need_ops, sequence->node);
		return -EINVAL;
	}

	hwlog_debug("%s: pa_num=%d, reg_num=%d!!!\n", __func__, pa_num_need_ops, reg_num_need_ops);
	for (i = 0; i < pa_num_need_ops; i++) {
		index = (int)sequence->pa_ctl_index[i];
		if (!pakit_priv->force_refresh_chip &&
			!smartpakit_need_write_regs(pakit_priv,
			index, (unsigned int)reg_num_need_ops,
			sequence->node + (i * reg_num_need_ops))) {
			hwlog_info("%s: pa%d not need re-write the same regs\n",
				__func__, index);
			continue;
		}
		ret += smartpakit_do_write_regs(pakit_priv->i2c_priv[index], (unsigned int)reg_num_need_ops,
			sequence->node + (i * reg_num_need_ops)); /*lint !e679*/
	}
	sequence->param_num_of_one_pa = reg_num_need_ops;

	return ret;
}

static int smartpakit_write_regs(void *priv, void __user *arg, int compat_mode)
{
	smartpakit_i2c_priv_t *i2c_priv = (smartpakit_i2c_priv_t *)priv;
	smartpakit_pa_ctl_sequence_t sequence;
	int ret = 0;

	hwlog_debug("%s: enter ...\n", __func__);
	if ((NULL == i2c_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	memset(&sequence, 0, sizeof(smartpakit_pa_ctl_sequence_t));
	ret = smartpakit_parse_params(&sequence, arg, compat_mode);
	if (ret < 0) {
		hwlog_err("%s: parse w_regs failed!!!\n", __func__);
		goto err_out;
	}

	if (!smartpakit_need_write_regs(i2c_priv->priv_data,
		i2c_priv->chip_id, sequence.param_num, sequence.node)) {
		hwlog_info("%s: pa%d not need re-write the same regs\n",
			__func__, i2c_priv->chip_id);
		goto err_out;
	}

	ret = smartpakit_do_write_regs(i2c_priv, sequence.param_num, sequence.node);
	if (ret < 0) {
		hwlog_err("%s: do_write_regs failed!!!\n", __func__);
		goto err_out;
	}
	sequence.param_num_of_one_pa = sequence.param_num;

	ret = smartpakit_set_poweron_regs(i2c_priv->priv_data, &sequence);
	if (ret < 0) {
		hwlog_err("%s: set_poweron_regs failed!!!\n", __func__);
		goto err_out;
	}

err_out:
	if (sequence.node != NULL) {
		kfree(sequence.node);
		sequence.node = NULL;
	}

	return ret;
}

static int smartpakit_write_regs_all(void *priv, void __user *arg, int compat_mode)
{
	smartpakit_priv_t *pakit_priv = (smartpakit_priv_t *)priv;
	smartpakit_pa_ctl_sequence_t sequence;
	int ret = 0;
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter ...\n", __func__);
#endif

	if ((NULL == pakit_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	memset(&sequence, 0, sizeof(smartpakit_pa_ctl_sequence_t));
	ret = smartpakit_parse_params(&sequence, arg, compat_mode);
	if (ret < 0) {
		hwlog_err("%s: parse w_regs failed!!!\n", __func__);
		goto err_out;
	}

	// check pa_ctl_num
	if ((sequence.pa_ctl_num > pakit_priv->pa_num)
		|| (sequence.pa_ctl_index_max >= pakit_priv->pa_num)) {
		hwlog_err("%s: invalid regs_sequence, pa_ctl_num %d>%d, pa_ctl_index_max %d>=%d!!!\n", __func__,
			sequence.pa_ctl_num, pakit_priv->pa_num, sequence.pa_ctl_index_max, pakit_priv->pa_num);
		ret = -EINVAL;
		goto err_out;
	}

	ret = smartpakit_do_write_regs_all(pakit_priv, &sequence);
	if (ret < 0) {
		hwlog_err("%s: do_write_regs_all failed!!!\n", __func__);
		goto err_out;
	}

	ret = smartpakit_set_poweron_regs(pakit_priv, &sequence);
	if (ret < 0) {
		hwlog_err("%s: set_poweron_regs failed!!!\n", __func__);
		goto err_out;
	}

err_out:
	if (sequence.node != NULL) {
		kfree(sequence.node);
		sequence.node = NULL;
	}

	return ret;
}

static int smartpakit_i2c_read(struct i2c_client *i2c, char *rx_data, int length)
{
	int ret = 0;
	struct i2c_msg msg[] = {
		{
			.addr  = i2c->addr,
			.flags = I2C_M_RD,
			.len   = (unsigned short)length,
			.buf   = (unsigned char *)rx_data,
		},
	};

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (0 > ret) {
		smartpakit_dsm_report_by_i2c_error("none", 0xff, 0, ret, NULL);
		hwlog_err("%s: transfer error %d", __func__, ret);
		return ret;
	}

	return ret;
}

static int smartpakit_i2c_write(struct i2c_client *i2c, char *rx_data, int length)
{
	int ret = 0;
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.len = (unsigned short)length,
			.buf = (unsigned char *)rx_data,
		},
	};

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (0 > ret) {
		smartpakit_dsm_report_by_i2c_error("none", 0xff, 1, ret, NULL);
		hwlog_err("%s: transfer error %d", __func__, ret);
		return ret;
	}

	return ret;
}

smartpakit_i2c_ioctl_ops_t smartpakit_ioctl_ops = {
	.hw_reset          = smartpakit_hw_reset,
	.dump_regs         = smartpakit_dump_regs,
	.read_regs         = smartpakit_read_regs,
	.write_regs        = smartpakit_write_regs,
	.write_regs_all    = smartpakit_write_regs_all,
	.do_write_regs_all = smartpakit_do_write_regs_all,
	.i2c_read          = smartpakit_i2c_read,
	.i2c_write         = smartpakit_i2c_write,
};

static int smartpakit_resume_regs(void *priv)
{
	smartpakit_priv_t *pakit_priv = (smartpakit_priv_t *)priv;
	int ret = 0;
	int i = 0;

	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	pakit_priv->force_refresh_chip = true;

	// init chip
	hwlog_info("%s: init chips...\n", __func__);
	ret = smartpakit_do_write_regs_all(pakit_priv, &pakit_priv->resume_sequence);
	if (ret < 0) {
		hwlog_err("%s: write init chip regs failed!!!\n", __func__);
		goto err_out;
	}

	// poweron chip
	hwlog_info("%s: poweron chips...\n", __func__);
	for (i = 0; i < SMARTPAKIT_PA_ID_MAX; i++) {
		if (1 == pakit_priv->poweron_sequence[i].pa_poweron_flag) {
			ret = smartpakit_do_write_regs_all(pakit_priv, &pakit_priv->poweron_sequence[i]);
			if (ret < 0) {
				hwlog_err("%s: write poweron %d regs failed!!!\n", __func__, i);
				goto err_out;
			}
		}
	}

err_out:
	pakit_priv->force_refresh_chip = false;
	return ret;
}

static void smartpakit_sync_irq_debounce_time(smartpakit_i2c_priv_t *i2c_priv)
{
	smartpakit_priv_t *pakit_priv = NULL;
	smartpakit_i2c_priv_t *i2c_priv_p = NULL;
	int i = 0;

	if (!i2c_priv || !i2c_priv->probe_completed || !i2c_priv->sync_irq_debounce_time)
		return;

	pakit_priv = (smartpakit_priv_t *)i2c_priv->priv_data;
	if (!pakit_priv)
		return;

	for (i = 0; i < (int)pakit_priv->pa_num; i++) {
		i2c_priv_p = pakit_priv->i2c_priv[i];
		if (!i2c_priv_p) {
			hwlog_err("%s: i2c_priv[%d] is null!!!\n", __func__, i);
			break;
		}

		if (i2c_priv_p == i2c_priv)
			continue;

		if (i2c_priv_p->reset_debounce_wait_time > 0) {
			cancel_delayed_work_sync(&i2c_priv_p->irq_debounce_work);

			i2c_priv_p->irq_debounce_jiffies = jiffies + msecs_to_jiffies(i2c_priv_p->reset_debounce_wait_time);
			schedule_delayed_work(&i2c_priv_p->irq_debounce_work, msecs_to_jiffies(i2c_priv_p->reset_debounce_wait_time));
		}
	}
	hwlog_info("%s: pa%u irq trigger, sync debounce time\n", __func__, i2c_priv->chip_id);
}

static bool smartpakit_i2c_check_rw_sequence_existed(smartpakit_priv_t *pakit_priv, smartpakit_reg_ctl_sequence_t **rw_sequence)
{
	bool check = false;
	int i = 0;

	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return check;
	}

	for (i = 0; i < (int)pakit_priv->pa_num; i++) {
		if ((NULL == pakit_priv->i2c_priv[i]) || (NULL == pakit_priv->i2c_priv[i]->irq_handler)) {
			hwlog_err("%s: check failed!!!\n", __func__);
			break;
		}

		if (pakit_priv->i2c_priv[i]->irq_handler->rw_sequence != NULL) {
			*rw_sequence = pakit_priv->i2c_priv[i]->irq_handler->rw_sequence;
			check = true;
			break;
		}
	}

	return check;
}

static bool smartpakit_i2c_check_need_reset_existed(smartpakit_priv_t *pakit_priv)
{
	bool check = false;
	int i = 0;

	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return check;
	}

	for (i = 0; i < (int)pakit_priv->pa_num; i++) {
		if ((NULL == pakit_priv->i2c_priv[i]) || (NULL == pakit_priv->i2c_priv[i]->irq_handler)) {
			hwlog_err("%s: check failed!!!\n", __func__);
			break;
		}

		if (pakit_priv->i2c_priv[i]->irq_handler->need_reset) {
			check = true;
			break;
		}
	}

	return check;
}

void smartpakit_i2c_handler_irq(struct work_struct *work)
{
	smartpakit_i2c_priv_t *i2c_priv = container_of(work, smartpakit_i2c_priv_t, irq_handle_work);
	smartpakit_priv_t *pakit_priv   = NULL;
	smartpakit_reg_ctl_sequence_t *rw_sequence = NULL;
	char *report = NULL;
#ifdef CONFIG_HUAWEI_DSM_AUDIO
	char report_tmp[SMARTPAKIT_NAME_MAX] = { 0 };
#endif
	int ret = 0;
	int i = 0;

	hwlog_info("%s: enter ...\n", __func__);
	if (/*(NULL == i2c_priv) || */(NULL == i2c_priv->priv_data) || (NULL == i2c_priv->irq_handler)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}
	pakit_priv = (smartpakit_priv_t *)i2c_priv->priv_data;

	mutex_lock(&pakit_priv->irq_handler_lock);
	mutex_lock(&pakit_priv->hw_reset_lock);
	mutex_lock(&pakit_priv->dump_regs_lock);
	mutex_lock(&pakit_priv->i2c_ops_lock);

	// read or write regs
	if (smartpakit_i2c_check_rw_sequence_existed(pakit_priv, &rw_sequence)) {
#ifdef CONFIG_HUAWEI_DSM_AUDIO
		report = kzalloc(sizeof(char) * SMARTPAKIT_DSM_BUF_SIZE, GFP_KERNEL);
		if (NULL == report) {
			hwlog_err("%s: kzalloc dsm report buffer failed!!!\n", __func__);
			goto err_out;
		}
#endif

		// traverse all pa to print regs and report
		for (i = 0; i < (int)pakit_priv->pa_num; i++) {
			if (NULL == pakit_priv->i2c_priv[i]) {
				continue;
			}

			hwlog_info("%s: pa[%d], rw_sequence ...\n", __func__, pakit_priv->i2c_priv[i]->chip_id);
#ifdef CONFIG_HUAWEI_DSM_AUDIO
			if (pakit_priv->i2c_priv[i]->chip_id == i2c_priv->chip_id) {
				// current pa, irq trigger from this pa
				snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "*pa[%s_%d]:", pakit_priv->i2c_priv[i]->chip_model, pakit_priv->i2c_priv[i]->chip_id);
			} else {
				snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "pa[%s_%d]:", pakit_priv->i2c_priv[i]->chip_model, pakit_priv->i2c_priv[i]->chip_id);
			}
			strncat(report, report_tmp, SMARTPAKIT_DSM_BUF_SIZE - strlen(report) - 1);

#ifdef CONFIG_HUAWEI_DEVICEBOX_AUDIO
			// int deviceboxID_read(int out_id)
			// param out_id: from 0 ~ 3
			// chip_id: from 0 ~ 3
			snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "boxid=%d,", deviceboxID_read(pakit_priv->i2c_priv[i]->chip_id));
			strncat(report, report_tmp, SMARTPAKIT_DSM_BUF_SIZE - strlen(report) - 1);
#endif
#endif // CONFIG_HUAWEI_DSM_AUDIO

			ret = smartpakit_do_reg_ctl(pakit_priv->i2c_priv[i], rw_sequence, report);
			if (ret < 0) {
				hwlog_err("%s: pa[%d], rw_sequence failed!!!\n", __func__, pakit_priv->i2c_priv[i]->chip_id);
				goto err_out;
			}
		}
	} else {
		hwlog_info("%s: pa[%d], not need rw_sequence, skip!!!\n", __func__, i2c_priv->chip_id);
	}

	// dsm report ...
#ifdef CONFIG_HUAWEI_DSM_AUDIO
	if (report != NULL) {
		hwlog_info("%s: dsm report, %s\n", __func__, report);
		audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_INT_ERR, "smartpakit,%s", report);
	}
#endif

	if (smartpakit_i2c_check_need_reset_existed(pakit_priv)) {
		// dump regs
		smartpakit_dump_regs(pakit_priv);

		// reset chip: how to controls chip by gpio_reset
		// multi chips: one gpio_reset pin
		// multi chips: multi gpio_reset pin
		smartpakit_hw_reset(pakit_priv);

		// re-init regs
		smartpakit_resume_regs(pakit_priv);
	} else {
		hwlog_info("%s: pa[%d], not need reset_chip, skip!!!\n", __func__, i2c_priv->chip_id);
	}

err_out:
#ifdef CONFIG_HUAWEI_DSM_AUDIO
	if (report != NULL) {
		kfree(report);
		report = NULL;
	}
#endif

	mutex_unlock(&pakit_priv->i2c_ops_lock);
	mutex_unlock(&pakit_priv->dump_regs_lock);
	mutex_unlock(&pakit_priv->hw_reset_lock);
	mutex_unlock(&pakit_priv->irq_handler_lock);
	hwlog_info("%s: enter end.\n", __func__);
}

irqreturn_t smartpakit_i2c_thread_irq(int irq, void *data)
{
	smartpakit_i2c_priv_t *i2c_priv = (smartpakit_i2c_priv_t *)data;

	smartpakit_sync_irq_debounce_time(i2c_priv);
	hwlog_info("%s: enter(%d) ...\n", __func__, irq);
	UNUSED(irq);
	if (NULL == i2c_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return IRQ_HANDLED;
	}

	if (i2c_priv->irq_debounce_jiffies > 0) {
		if (time_is_after_jiffies(i2c_priv->irq_debounce_jiffies)) { /*lint !e550 !e774*/
			hwlog_info("%s: debounce wait, skip this irq!!!\n", __func__);
			return IRQ_HANDLED;
		}
	}

	if (0 == i2c_priv->probe_completed) {
		hwlog_err("%s: probe not completed, skip!!!\n", __func__);
		return IRQ_HANDLED;
	}

	if (!work_busy(&i2c_priv->irq_handle_work)) {
		hwlog_info("%s: schedule_work ...\n", __func__);
		schedule_work(&i2c_priv->irq_handle_work);
	} else {
		hwlog_info("%s: work busy, skip!!!\n", __func__);
	}

	hwlog_info("%s: enter end.\n", __func__);
	return IRQ_HANDLED;
}
EXPORT_SYMBOL_GPL(smartpakit_i2c_thread_irq);

static void smartpakit_i2c_irq_debounce_work(struct work_struct *work)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;

	hwlog_info("%s: reset debounce, delayed_work enter...\n", __func__);
	if (work == NULL) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	i2c_priv = container_of(work, smartpakit_i2c_priv_t, irq_debounce_work.work);
	i2c_priv->irq_debounce_jiffies = 0;
	hwlog_info("%s: pa%d reset debounce, delayed_work stop.\n", __func__, i2c_priv->chip_id);
}

static bool smartpakit_i2c_writeable_reg(struct device *dev, unsigned int reg)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_regmap_cfg_t *cfg = NULL;
	int i = 0;

	if (NULL == dev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return false;
	}
	i2c_priv = dev_get_drvdata(dev);

	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg)) {
		hwlog_err("%s: regmap_cfg invalid argument!!!\n", __func__);
		return false;
	}
	cfg = i2c_priv->regmap_cfg;

	if (cfg->num_writeable > 0) {
		if (NULL == cfg->reg_writeable) {
			hwlog_err("%s: cfg->reg_writeable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_writeable; i++) {
			if (cfg->reg_writeable[i] == reg) {
				return true;
			}
		}

		return false;
	} else if (cfg->num_unwriteable > 0) {
		if (NULL == cfg->reg_unwriteable) {
			hwlog_err("%s: cfg->reg_unwriteable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_unwriteable; i++) {
			if (cfg->reg_unwriteable[i] == reg) {
				return false;
			}
		}

		return true;
	} else {
		//hwlog_info("%s: num_writeable and num_unwriteable not setting!!!\n", __func__);
	}

	return true;
}

static bool smartpakit_i2c_readable_reg(struct device *dev, unsigned int reg)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_regmap_cfg_t *cfg = NULL;
	int i = 0;

	if (NULL == dev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return false;
	}
	i2c_priv = dev_get_drvdata(dev);

	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg)) {
		hwlog_err("%s: regmap_cfg invalid argument!!!\n", __func__);
		return false;
	}
	cfg = i2c_priv->regmap_cfg;

	if (cfg->num_readable > 0) {
		if (NULL == cfg->reg_readable) {
			hwlog_err("%s: cfg->reg_writeable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_readable; i++) {
			if (cfg->reg_readable[i] == reg) {
				return true;
			}
		}

		return false;
	} else if (cfg->num_unreadable > 0) {
		if (NULL == cfg->reg_unreadable) {
			hwlog_err("%s: cfg->reg_unwriteable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_unreadable; i++) {
			if (cfg->reg_unreadable[i] == reg) {
				return false;
			}
		}

		return true;
	} else {
		//hwlog_info("%s: num_readable and num_unreadable not setting!!!\n", __func__);
	}

	return true;
}

static bool smartpakit_i2c_volatile_reg(struct device *dev, unsigned int reg)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_regmap_cfg_t *cfg = NULL;
	int i = 0;

	if (NULL == dev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return false;
	}
	i2c_priv = dev_get_drvdata(dev);

	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg)) {
		hwlog_err("%s: regmap_cfg invalid argument!!!\n", __func__);
		return false;
	}
	cfg = i2c_priv->regmap_cfg;

	if (cfg->num_volatile > 0) {
		if (NULL == cfg->reg_volatile) {
			hwlog_err("%s: cfg->reg_writeable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_volatile; i++) {
			if (cfg->reg_volatile[i] == reg) {
				return true;
			}
		}

		return false;
	} else if (cfg->num_unvolatile > 0) {
		if (NULL == cfg->reg_unvolatile) {
			hwlog_err("%s: cfg->reg_unwriteable == NULL!!!\n", __func__);
			return false;
		}

		for (i = 0; i < cfg->num_unvolatile; i++) {
			if (cfg->reg_unvolatile[i] == reg) {
				return false;
			}
		}

		return true;
	} else {
		//hwlog_info("%s: num_volatile and num_unvolatile not setting!!!\n", __func__);
	}

	return true;
}

static int smartpakit_i2c_parse_dt_reset(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *hw_reset_str     = "hw_reset";
	const char *gpio_reset_str   = "gpio_reset";
	const char *ctl_sequence_str = "ctl_sequence";
	const char *not_need_shutdown_str = "not_need_shutdown";
	smartpakit_gpio_reset_t *reset = NULL;
	struct device_node *node = NULL;
	int count = 0;
	int val_num = 0;
	int ret = 0;
	int i = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	node = of_get_child_by_name(i2c->dev.of_node, hw_reset_str);
	if (NULL == node) {
		hwlog_debug("%s: hw_reset device_node not existed, skip!!!\n", __func__);
		return 0;
	}

	reset = kzalloc(sizeof(smartpakit_gpio_reset_t), GFP_KERNEL);
	if (NULL == reset) {
		hwlog_err("%s: kzalloc hw_reset failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	reset->not_need_shutdown = of_property_read_bool(node, not_need_shutdown_str);
	hwlog_debug("%s: hw_reset not_need_shutdown = %d!!!\n", __func__, (int)reset->not_need_shutdown);

	reset->gpio = of_get_named_gpio(node, gpio_reset_str, 0);
	if (reset->gpio < 0) {
		hwlog_debug("%s: hw_reset of_get_named_gpio gpio_reset failed(%d)!!!\n", __func__, reset->gpio);
		ret = of_property_read_u32(node, gpio_reset_str, (u32 *)&reset->gpio);
		if (ret < 0) {
			hwlog_err("%s: hw_reset of_property_read_u32 gpio_reset failed(%d)!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
	}
	hwlog_debug("%s: hw_reset get gpio %d!!!\n", __func__, reset->gpio);

	ret = snprintf(reset->gpio_name, (unsigned long)SMARTPAKIT_NAME_MAX, "%s_gpio_reset_%d", i2c_priv->chip_model, i2c_priv->chip_id);
	if (ret < 0) {
		hwlog_err("%s: hw_reset set gpio_name failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	val_num = sizeof(smartpakit_gpio_state_t) / sizeof(unsigned int);
	count = of_property_count_elems_of_size(node, ctl_sequence_str, (int)sizeof(u32));
	if ((count <= 0) || ((count % val_num) != 0)) {
		hwlog_err("%s: get node_num failed(%d) or node_num %d%%%d != 0!!!\n", __func__, count, count, val_num);
		ret = -EFAULT;
		goto err_out;
	}
	reset->sequence.num = (unsigned int)(count / val_num);
	hwlog_debug("%s: sequence.num=%d\n", __func__, reset->sequence.num);

	reset->sequence.node = kzalloc(sizeof(smartpakit_gpio_state_t) * reset->sequence.num, GFP_KERNEL);
	if (NULL == reset->sequence.node) {
		hwlog_err("%s: kzalloc sequence.node failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	ret = of_property_read_u32_array(node, ctl_sequence_str, (u32 *)reset->sequence.node, (size_t)(long)count);
	if (ret < 0) {
		hwlog_err("%s: hw_reset get sequence node failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	if (!smartpakit_is_gpio_request(reset->gpio, false)) {
		if (gpio_request((unsigned)reset->gpio, reset->gpio_name) < 0) {
			hwlog_err("%s: gpio_request reset->gpio[%d] failed!!!\n", __func__, reset->gpio);
			reset->gpio = 0;
			ret = -EFAULT;
			goto err_out;
		}
	} else {
		hwlog_debug("%s: this gpio(%d) is requested, skip!!!\n", __func__, reset->gpio);
	}

	// set hw_reset debounce
	if (i2c_priv->reset_debounce_wait_time > 0) {
		hwlog_debug("%s: pa%d reset debounce, trigger delayed_work!\n", __func__, i2c_priv->chip_id);
		i2c_priv->irq_debounce_jiffies = jiffies + msecs_to_jiffies(i2c_priv->reset_debounce_wait_time);
		schedule_delayed_work(&i2c_priv->irq_debounce_work, msecs_to_jiffies(i2c_priv->reset_debounce_wait_time));
	}

	// reset chip
	for (i = 0; i< (int)reset->sequence.num; i++) {

#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: pa is resetting... gpio[%d], pull state[%d](1:up,0:down), delay[%d](ms)\n", __func__,
			reset->gpio, reset->sequence.node[i].state, reset->sequence.node[i].delay);
#endif
		gpio_direction_output((unsigned)reset->gpio, (int)reset->sequence.node[i].state);
		if (reset->sequence.node[i].delay > 0) {
			// delay time units: msecs
			// last node use msleep
			if (i == (int)(reset->sequence.num - 1)) {
				msleep(reset->sequence.node[i].delay);
			} else {
				mdelay(reset->sequence.node[i].delay); /*lint !e647 !e747 !e774*/
			}
		}
	}

	i2c_priv->hw_reset = reset;
	return 0;

err_out:
	if (reset != NULL) {
		SMARTPAKIT_KFREE_OPS(reset->sequence.node);

		kfree(reset);
		reset = NULL;
	}

	return ret;
}

static int smartpakit_i2c_parse_dt_reg_ctl(smartpakit_reg_ctl_sequence_t **reg_ctl, struct device_node *node, const char *ctl_str)
{
	smartpakit_reg_ctl_sequence_t *ctl = NULL;
	int count = 0;
	int val_num = 0;
	int ret = 0;
	int i = 0;

	if ((NULL == node) || (NULL == ctl_str)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	ctl = kzalloc(sizeof(smartpakit_reg_ctl_sequence_t), GFP_KERNEL);
	if (NULL == ctl) {
		hwlog_err("%s: kzalloc reg_ctl_sequence failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	val_num = sizeof(smartpakit_reg_ctl_t) / sizeof(unsigned int);
	count = of_property_count_elems_of_size(node, ctl_str, (int)sizeof(u32));
	if ((count <= 0) || ((count % val_num) != 0)) {
		hwlog_err("%s: get reg_num failed(%d) or reg_num %d%%%d != 0!!!\n", __func__, count, count, val_num);
		ret = -EFAULT;
		goto err_out;
	}
	ctl->num = (unsigned int)(count / val_num);

	ctl->regs = kzalloc(sizeof(smartpakit_reg_ctl_t) * ctl->num, GFP_KERNEL);
	if (NULL == ctl->regs) {
		hwlog_err("%s: kzalloc reg_ctl_sequence regs failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	ret = of_property_read_u32_array(node, ctl_str, (u32 *)ctl->regs, (size_t)(long)count);
	if (ret < 0) {
		hwlog_err("%s: reg_ctl get regs failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

#ifndef CONFIG_FINAL_RELEASE
	// dump regs
	hwlog_info("%s: %s reg_ctl ...\n", __func__, ctl_str);
	for (i = 0; i < (int)ctl->num; i++) {
		hwlog_info("%s: reg_ctl[%d]=0x%x, 0x%x, %d!!!\n", __func__, i,
			ctl->regs[i].addr, ctl->regs[i].value, ctl->regs[i].ctl_type);
	}
#endif

	*reg_ctl = ctl;
	return 0;

err_out:
	if (ctl != NULL) {
		SMARTPAKIT_KFREE_OPS(ctl->regs);

		kfree(ctl);
		ctl = NULL;
	}

	return ret;
}

static int smartpakit_i2c_parse_dt_irq(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *irq_handler_str	   = "irq_handler";
	const char *gpio_irq_str       = "gpio_irq";
	const char *irq_flags_str      = "irq_flags";
	const char *need_reset_str     = "need_reset";
	const char *rw_sequence_str    = "rw_sequence";
	smartpakit_gpio_irq_t *irq_handler = NULL;
	struct device_node *node = NULL;
	int ret = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	node = of_get_child_by_name(i2c->dev.of_node, irq_handler_str);
	if (NULL == node) {
		hwlog_debug("%s: irq_handler device_node not existed, skip!!!\n", __func__);
		return 0;
	}

	irq_handler = kzalloc(sizeof(smartpakit_gpio_irq_t), GFP_KERNEL);
	if (NULL == irq_handler) {
		hwlog_err("%s: kzalloc irq_handler failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	irq_handler->gpio = of_get_named_gpio(node, gpio_irq_str, 0);
	if (irq_handler->gpio < 0) {
		hwlog_debug("%s: irq_handler of_get_named_gpio gpio_irq failed(%d)!!!\n", __func__, irq_handler->gpio);
		ret = of_property_read_u32(node, gpio_irq_str, (u32 *)&irq_handler->gpio);
		if (ret < 0) {
			hwlog_err("%s: irq_handler of_property_read_u32 gpio_irq failed(%d)!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
	}
	hwlog_debug("%s: irq_handler get gpio %d!!!\n", __func__, irq_handler->gpio);

	ret = of_property_read_u32(node, irq_flags_str, &irq_handler->irqflags);
	if (ret < 0) {
		hwlog_err("%s: irq_handler get irq_flags failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}
	irq_handler->need_reset = of_property_read_bool(node, need_reset_str);
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: irq_handler need_reset = %d!!!\n", __func__, (int)irq_handler->need_reset);
#endif

	ret += snprintf(irq_handler->gpio_name, (unsigned long)SMARTPAKIT_NAME_MAX, "%s_gpio_irq_%d", i2c_priv->chip_model, i2c_priv->chip_id);
	ret += snprintf(irq_handler->irq_name, (unsigned long)SMARTPAKIT_NAME_MAX, "%s_irq_%d", i2c_priv->chip_model, i2c_priv->chip_id);
	if (ret < 0) {
		hwlog_err("%s: irq_handler set gpio_name/irq_name failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	// read or write reg sequence
	if (of_property_read_bool(node, rw_sequence_str)) {
		ret = smartpakit_i2c_parse_dt_reg_ctl(&irq_handler->rw_sequence, node, rw_sequence_str);
		if (ret < 0) {
			hwlog_err("%s: parse irq_handler->rw_sequence failed!!!\n", __func__);
			goto err_out;
		}
	} else {
		hwlog_debug("%s: rw_sequence prop not existed, skip!!!\n", __func__);
	}

	// irq handler
	if (!gpio_is_valid((int)irq_handler->gpio)) {
		hwlog_err("%s: irq_handler gpio invalid(%d)!!!\n", __func__, irq_handler->gpio);
		ret = -EFAULT;
		goto err_out;
	}

	if (!smartpakit_is_gpio_request(irq_handler->gpio, true)) {
		ret = devm_gpio_request_one(&i2c->dev, (unsigned)irq_handler->gpio, (unsigned long)GPIOF_DIR_IN, irq_handler->gpio_name);
		if (ret < 0) {
			hwlog_err("%s: irq_handler gpio_request set GPIOF_DIR_IN failed!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}

		irq_handler->irq = gpio_to_irq((unsigned int)irq_handler->gpio);
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: irq_handler get irq %d, irqflags=%d!!!\n", __func__, irq_handler->irq, irq_handler->irqflags);
#endif
		ret = devm_request_threaded_irq(&i2c->dev, (unsigned int)irq_handler->irq, NULL, smartpakit_i2c_thread_irq,
			(unsigned long)(irq_handler->irqflags | IRQF_ONESHOT), irq_handler->irq_name, (void *)i2c_priv);
		if (ret < 0) {
			hwlog_err("%s: irq_handler devm_request_threaded_irq failed!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}
	} else {
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: this gpio(%d) of irq has been requested, skip!!!\n", __func__, irq_handler->gpio);
#endif
	}

	i2c_priv->irq_handler = irq_handler;
	return 0;

err_out:
	if (irq_handler != NULL) {
		if (irq_handler->rw_sequence != NULL) {
			SMARTPAKIT_KFREE_OPS(irq_handler->rw_sequence->regs);

			kfree(irq_handler->rw_sequence);
			irq_handler->rw_sequence = NULL;
		}

		kfree(irq_handler);
		irq_handler = NULL;
	}

	return ret;
}

static int smartpakit_i2c_parse_dt_version_regs(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *version_regs_str = "version_regs";
	int ret = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_bool(i2c->dev.of_node, version_regs_str)) {
		ret = smartpakit_i2c_parse_dt_reg_ctl(&i2c_priv->version_regs_sequence, i2c->dev.of_node, version_regs_str);
		if (ret < 0) {
			hwlog_err("%s: parse version_regs failed!!!\n", __func__);
			goto err_out;
		}
	} else {
		hwlog_debug("%s: version_regs prop not existed, skip!!!\n", __func__);
	}

	return 0;

err_out:
	return ret;
}

static int smartpakit_i2c_parse_dt_dump_regs(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *dump_regs_str = "dump_regs";
	int ret = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_bool(i2c->dev.of_node, dump_regs_str)) {
		ret = smartpakit_i2c_parse_dt_reg_ctl(&i2c_priv->dump_regs_sequence, i2c->dev.of_node, dump_regs_str);
		if (ret < 0) {
			hwlog_err("%s: parse dump_regs failed!!!\n", __func__);
			goto err_out;
		}
	} else {
		hwlog_debug("%s: version_regs prop not existed, skip!!!\n", __func__);
	}

	return 0;

err_out:
	return ret;
}

static unsigned int smartpakit_i2c_get_reg_value_mask(int val_bits)
{
	unsigned int mask = 0;

	if (SMARTPAKIT_REG_VALUE_B16 == val_bits) {
		mask = SMARTPAKIT_REG_VALUE_M16;
	} else if (SMARTPAKIT_REG_VALUE_B24 == val_bits) {
		mask = SMARTPAKIT_REG_VALUE_M24;
	} else if (SMARTPAKIT_REG_VALUE_B32 == val_bits) {
		mask = SMARTPAKIT_REG_VALUE_M32;
	} else{ // SMARTPAKIT_REG_VALUE_B8 or other
		mask = SMARTPAKIT_REG_VALUE_M8;
	}

	return mask;
}

static int smartpakit_i2c_regmap_read_reg_array(struct device_node *node, const char *propname, u32 **regs, int num)
{
	u32 *reg_array = NULL;
	int ret = 0;

	if ((NULL == node) || (NULL == propname)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (0 == num) {
		hwlog_debug("%s: %s prop not existed, skip!!!\n", __func__, propname);
		return 0;
	}

	reg_array = (u32 *)kzalloc(sizeof(u32) * num, GFP_KERNEL); /*lint !e737*/
	if (NULL == reg_array) {
		hwlog_err("%s: kzalloc %s reg_array failed!!!\n", __func__, propname);
		return -EFAULT;
	}

	ret = of_property_read_u32_array(node, propname, reg_array, (size_t)(long)num);
	if (ret < 0) {
		hwlog_err("%s: get reg_array failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	*regs = reg_array;
	return 0;

err_out:
	if (reg_array != NULL) { /*lint !e774*/
		kfree(reg_array);
		reg_array = NULL;
	}
	return ret;
}

/*lint -save -e115*/
static int smartpakit_i2c_regmap_init(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *regmap_cfg_str       = "regmap_cfg";
	const char *reg_bits_str         = "reg_bits";
	const char *val_bits_str         = "val_bits";
	const char *cache_type_str       = "cache_type";
	const char *max_register_str     = "max_register";
	const char *reg_writeable_str    = "reg_writeable";
	const char *reg_unwriteable_str  = "reg_unwriteable";
	const char *reg_readable_str     = "reg_readable";
	const char *reg_unreadable_str   = "reg_unreadable";
	const char *reg_volatile_str     = "reg_volatile";
	const char *reg_unvolatile_str   = "reg_unvolatile";
	const char *reg_defaults_str     = "reg_defaults";
	smartpakit_regmap_cfg_t *cfg = NULL;
	struct device_node *node = NULL;
	int val_num = 0;
	int ret = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	node = of_get_child_by_name(i2c->dev.of_node, regmap_cfg_str);
	if (NULL == node) {
		hwlog_debug("%s: regmap_cfg device_node not existed, skip!!!\n", __func__);
		return 0;
	}

	cfg = kzalloc(sizeof(smartpakit_regmap_cfg_t), GFP_KERNEL);
	if (NULL == cfg) {
		hwlog_err("%s: kzalloc regmap_cfg failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	ret = of_property_read_u32(node, reg_bits_str, (u32 *)&cfg->cfg.reg_bits);
	if (ret < 0) {
		hwlog_err("%s: regmap_cfg get reg_bits failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	ret = of_property_read_u32(node, val_bits_str, (u32 *)&cfg->cfg.val_bits);
	if (ret < 0) {
		hwlog_err("%s: regmap_cfg get val_bits failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}
	cfg->value_mask = smartpakit_i2c_get_reg_value_mask(cfg->cfg.val_bits);

	ret = of_property_read_u32(node, cache_type_str, (u32 *)&cfg->cfg.cache_type);
	if ((ret < 0) || (cfg->cfg.cache_type > REGCACHE_FLAT)) { /*lint !e685*/
		hwlog_err("%s: regmap_cfg get cache_type failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	ret = of_property_read_u32(node, max_register_str, &cfg->cfg.max_register);
	if (ret < 0) {
		hwlog_err("%s: regmap_cfg get max_register failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	cfg->num_writeable   = 0;
	cfg->num_unwriteable = 0;
	cfg->num_readable    = 0;
	cfg->num_unreadable  = 0;
	cfg->num_volatile    = 0;
	cfg->num_unvolatile  = 0;
	cfg->num_defaults    = 0;

	if (of_property_read_bool(node, reg_writeable_str)) {
		cfg->num_writeable	 = of_property_count_elems_of_size(node, reg_writeable_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_unwriteable_str)) {
		cfg->num_unwriteable = of_property_count_elems_of_size(node, reg_unwriteable_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_readable_str)) {
		cfg->num_readable	 = of_property_count_elems_of_size(node, reg_readable_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_unreadable_str)) {
		cfg->num_unreadable  = of_property_count_elems_of_size(node, reg_unreadable_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_volatile_str)) {
		cfg->num_volatile	 = of_property_count_elems_of_size(node, reg_volatile_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_unvolatile_str)) {
		cfg->num_unvolatile  = of_property_count_elems_of_size(node, reg_unvolatile_str, (int)sizeof(u32));
	}
	if (of_property_read_bool(node, reg_defaults_str)) {
		cfg->num_defaults	 = of_property_count_elems_of_size(node, reg_defaults_str, (int)sizeof(u32));
	}

	val_num = sizeof(struct reg_default) / sizeof(unsigned int);
	if (cfg->num_defaults > 0) {
		if ((cfg->num_defaults % val_num) != 0) {
			hwlog_err("%s: get reg_defaults %d%%%d != 0!!!\n", __func__, cfg->num_defaults, val_num);
			ret = -EFAULT;
			goto err_out;
		}

	}
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: regmap_cfg get number(w%d,%d,r%d,%d,v%d,%d,default%d)\n", __func__,
		cfg->num_writeable, cfg->num_unwriteable, cfg->num_readable, cfg->num_unreadable,
		cfg->num_volatile, cfg->num_unvolatile, cfg->num_defaults / val_num);
#endif

	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_writeable_str, &cfg->reg_writeable, cfg->num_writeable);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_unwriteable_str, &cfg->reg_unwriteable, cfg->num_unwriteable);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_readable_str, &cfg->reg_readable, cfg->num_readable);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_unreadable_str, &cfg->reg_unreadable, cfg->num_unreadable);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_volatile_str, &cfg->reg_volatile, cfg->num_volatile);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_unvolatile_str, &cfg->reg_unvolatile, cfg->num_unvolatile);
	ret += smartpakit_i2c_regmap_read_reg_array(node, reg_defaults_str, (u32 **)&cfg->reg_defaults, cfg->num_defaults);
	if (ret < 0) {
		hwlog_err("%s: regmap_cfg get reg_array failed!!!\n", __func__);
		ret = -EFAULT;
		goto err_out;
	}

	/*lint -save -e63*/
	// set num_reg_defaults
	if (cfg->num_defaults > 0) {
		cfg->num_defaults /= val_num;

		cfg->cfg.reg_defaults	  = cfg->reg_defaults;
		cfg->cfg.num_reg_defaults = (unsigned int)cfg->num_defaults;
	}

	//cfg->cfg.name = "smartpakit";
	cfg->cfg.writeable_reg = smartpakit_i2c_writeable_reg;
	cfg->cfg.readable_reg  = smartpakit_i2c_readable_reg;
	cfg->cfg.volatile_reg  = smartpakit_i2c_volatile_reg;
	/*lint -restore*/

	cfg->regmap = regmap_init_i2c(i2c, &cfg->cfg);
	if (IS_ERR(cfg->regmap)) {
		hwlog_err("%s: regmap_init_i2c regmap failed(%pK)!!!\n", __func__, cfg->regmap);
		ret = -EFAULT;
		goto err_out;
	}

	i2c_priv->regmap_cfg = cfg;
	return 0;

err_out:
	if (cfg != NULL) {
		SMARTPAKIT_KFREE_OPS(cfg->reg_writeable);
		SMARTPAKIT_KFREE_OPS(cfg->reg_unwriteable);
		SMARTPAKIT_KFREE_OPS(cfg->reg_readable);
		SMARTPAKIT_KFREE_OPS(cfg->reg_unreadable);
		SMARTPAKIT_KFREE_OPS(cfg->reg_volatile);
		SMARTPAKIT_KFREE_OPS(cfg->reg_unvolatile);
		SMARTPAKIT_KFREE_OPS(cfg->reg_defaults);

		kfree(cfg);
		cfg = NULL;
	}

	return ret;
}
/*lint -restore*/

static int smartpakit_i2c_regmap_deinit(smartpakit_i2c_priv_t *i2c_priv)
{
	smartpakit_regmap_cfg_t *cfg = NULL;

	if ((NULL == i2c_priv) || (NULL == i2c_priv->regmap_cfg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	cfg = i2c_priv->regmap_cfg;

	SMARTPAKIT_KFREE_OPS(cfg->reg_writeable);
	SMARTPAKIT_KFREE_OPS(cfg->reg_unwriteable);
	SMARTPAKIT_KFREE_OPS(cfg->reg_readable);
	SMARTPAKIT_KFREE_OPS(cfg->reg_unreadable);
	SMARTPAKIT_KFREE_OPS(cfg->reg_volatile);
	SMARTPAKIT_KFREE_OPS(cfg->reg_unvolatile);
	SMARTPAKIT_KFREE_OPS(cfg->reg_defaults);

	regmap_exit(cfg->regmap);
	cfg->regmap = NULL;

	kfree(cfg);
	cfg = NULL;
	i2c_priv->regmap_cfg = NULL;
	return 0;
}

static int smartpakit_i2c_parse_dt_chip(struct i2c_client *i2c, smartpakit_i2c_priv_t *i2c_priv)
{
	const char *chip_vendor_str   = "chip_vendor";
	const char *chip_id_str       = "chip_id";
	const char *chip_model_str    = "chip_model";
	const char *debounce_wait_str = "reset_debounce_wait_time";
	const char *i2c_pseudo_addr_str = "i2c_pseudo_addr";
	const char *sync_irq_debounce_time_str = "sync_irq_debounce_time";
	struct device *dev = NULL;
	int ret = 0;

	if ((NULL == i2c) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	dev = &i2c->dev;

	ret = of_property_read_u32(dev->of_node, chip_vendor_str, &i2c_priv->chip_vendor);
	if ((ret < 0) || (i2c_priv->chip_vendor >= SMARTPAKIT_CHIP_VENDOR_MAX)) {
		hwlog_err("%s: get i2c_priv->chip_vendor from dts failed %d,%d!!!\n", __func__, ret, i2c_priv->chip_vendor);
		ret = -EFAULT;
		goto err_out;
	}

	ret = of_property_read_u32(dev->of_node, chip_id_str, &i2c_priv->chip_id);
	if ((ret < 0) || (i2c_priv->chip_id >= SMARTPAKIT_PA_ID_MAX)) {
		hwlog_err("%s: get i2c_priv->chip_id from dts failed %d,%d!!!\n", __func__, ret, i2c_priv->chip_id);
		ret = -EFAULT;
		goto err_out;
	}

	ret = of_property_read_string(dev->of_node, chip_model_str, &i2c_priv->chip_model);
	if (ret < 0) {
		hwlog_err("%s: get i2c_priv->chip_model from dts failed %d!!!\n", __func__, ret);
		ret = -EFAULT;
		goto err_out;
	}

	if (of_property_read_bool(dev->of_node, debounce_wait_str)) {
		ret = of_property_read_u32(dev->of_node, debounce_wait_str, &i2c_priv->reset_debounce_wait_time);
		if (ret < 0) {
			hwlog_err("%s: get reset_debounce_wait_time from dts failed %d!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: reset_debounce_wait_time prop get success(%d)!\n", __func__, i2c_priv->reset_debounce_wait_time);
#endif
	} else {
		hwlog_debug("%s: reset_debounce_wait_time prop not existed, skip!!!\n", __func__);
		i2c_priv->reset_debounce_wait_time = 0;
	}

	if (of_property_read_bool(dev->of_node, i2c_pseudo_addr_str)) {
		ret = of_property_read_u32(dev->of_node, i2c_pseudo_addr_str, &i2c_priv->i2c_pseudo_addr);
		if (ret < 0) {
			hwlog_err("%s: get i2c_pseudo_addr from dts failed %d!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
	} else {
		hwlog_debug("%s: i2c_pseudo_addr prop not existed, skip!!!\n", __func__);
		i2c_priv->i2c_pseudo_addr = 0;
	}

	i2c_priv->sync_irq_debounce_time = false;
	if (of_property_read_bool(dev->of_node, sync_irq_debounce_time_str)) {
		i2c_priv->sync_irq_debounce_time = true;
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: sync irq debounce time enabled\n", __func__);
#endif
	}

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: get chip info(chip_vendor:%d,chip_id:%d,chip_model:%s)\n", __func__, i2c_priv->chip_vendor, i2c_priv->chip_id, i2c_priv->chip_model);
#endif
	return 0;

err_out:
	return ret;
}

static void smartpakit_i2c_free(smartpakit_i2c_priv_t *i2c_priv)
{
	if (NULL == i2c_priv) {
		hwlog_err("%s: i2c_priv invalid argument!!!\n", __func__);
		return;
	}

	cancel_delayed_work_sync(&i2c_priv->irq_debounce_work);

	if (i2c_priv->hw_reset != NULL) {
		SMARTPAKIT_KFREE_OPS(i2c_priv->hw_reset->sequence.node);

		gpio_free((unsigned)i2c_priv->hw_reset->gpio);
		i2c_priv->hw_reset->gpio = 0;

		kfree(i2c_priv->hw_reset);
		i2c_priv->hw_reset = NULL;
	}

	if (i2c_priv->irq_handler != NULL) {
		if (i2c_priv->irq_handler->rw_sequence != NULL) {
			SMARTPAKIT_KFREE_OPS(i2c_priv->irq_handler->rw_sequence->regs);

			kfree(i2c_priv->irq_handler->rw_sequence);
			i2c_priv->irq_handler->rw_sequence = NULL;
		}

		kfree(i2c_priv->irq_handler);
		i2c_priv->irq_handler = NULL;
	}

	if (i2c_priv->version_regs_sequence != NULL) {
		SMARTPAKIT_KFREE_OPS(i2c_priv->version_regs_sequence->regs);

		kfree(i2c_priv->version_regs_sequence);
		i2c_priv->version_regs_sequence = NULL;
	}

	if (i2c_priv->dump_regs_sequence != NULL) {
		SMARTPAKIT_KFREE_OPS(i2c_priv->dump_regs_sequence->regs);

		kfree(i2c_priv->dump_regs_sequence);
		i2c_priv->dump_regs_sequence = NULL;
	}

	smartpakit_i2c_regmap_deinit(i2c_priv);
	kfree(i2c_priv);
}

static int smartpakit_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	int ret = 0;

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter ...\n", __func__);
#endif

	if (0 == smartpakit_init_flag) {
		hwlog_info("%s: this driver need probe_defer!!!\n", __func__);
		return -EPROBE_DEFER;
	}

	if (NULL == i2c) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s: device %s, addr=0x%x, flags=0x%x\n", __func__, id->name, i2c->addr, i2c->flags);
	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		hwlog_err("%s: i2c check functionality error!!!", __func__);
		return -ENODEV;
	}

	i2c_priv = kzalloc(sizeof(smartpakit_i2c_priv_t), GFP_KERNEL);
	if (NULL == i2c_priv) {
		hwlog_err("%s: kzalloc i2c_priv failed!!!\n", __func__);
		goto err_out;
	}
	i2c_priv->probe_completed = 0;
	i2c_priv->reset_debounce_wait_time = 0;
	i2c_priv->irq_debounce_jiffies = 0;

	ret = smartpakit_i2c_parse_dt_chip(i2c, i2c_priv);
	if (ret < 0) {
		goto err_out;
	}

	if (smartpakit_i2c_probe_skip[i2c_priv->chip_id]) {
		hwlog_info("%s: chip_id = %d is probe success, skip!\n", __func__, i2c_priv->chip_id);
		goto err_out;
	}

	i2c_priv->dev = &i2c->dev;
	i2c_priv->i2c = i2c;

	i2c_set_clientdata(i2c, i2c_priv);
	dev_set_drvdata(&i2c->dev, i2c_priv);

	INIT_WORK(&i2c_priv->irq_handle_work, smartpakit_i2c_handler_irq);
	INIT_DELAYED_WORK(&i2c_priv->irq_debounce_work, smartpakit_i2c_irq_debounce_work); /*lint !e747*/

	ret  = smartpakit_i2c_parse_dt_reset(i2c, i2c_priv);
	ret += smartpakit_i2c_parse_dt_irq(i2c, i2c_priv);
	ret += smartpakit_i2c_parse_dt_version_regs(i2c, i2c_priv);
	ret += smartpakit_i2c_parse_dt_dump_regs(i2c, i2c_priv);
	ret += smartpakit_i2c_regmap_init(i2c, i2c_priv);
	if (ret < 0) {
		goto err_out;
	}

	// read chip version
	if (i2c_priv->version_regs_sequence != NULL) {

#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: pa[%d], read version ...\n", __func__, i2c_priv->chip_id);
#endif
		ret = smartpakit_do_reg_ctl(i2c_priv, i2c_priv->version_regs_sequence, NULL);
		if (ret < 0) {
			hwlog_err("%s: %s read or write regs by i2c failed(%d)!!!\n", __func__, i2c_priv->chip_model, ret);
			goto err_out;
		}
	}

	// register this i2c device to i2c_list in smartpakit device
	smartpakit_register_i2c_device(i2c_priv);
	smartpakit_register_ioctl_ops(&smartpakit_ioctl_ops);

	i2c_priv->probe_completed = 1;
	smartpakit_i2c_probe_skip[i2c_priv->chip_id] = true;
	hwlog_info("%s: end sucess!!!\n", __func__);
	return 0;

err_out:
	if (i2c_priv != NULL) {
		smartpakit_i2c_free(i2c_priv);
	}
	hwlog_err("%s: end fail !!!\n", __func__);
	return ret;
}

static int smartpakit_i2c_remove(struct i2c_client *i2c)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;

	hwlog_info("%s: remove\n", __func__);
	if (NULL == i2c) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	i2c_priv = i2c_get_clientdata(i2c);
	if (NULL == i2c_priv) {
		hwlog_err("%s: i2c_priv invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	i2c_set_clientdata(i2c, NULL);
	dev_set_drvdata(&i2c->dev, NULL);

	// deregister this i2c device
	smartpakit_deregister_i2c_device(i2c_priv);

	smartpakit_i2c_free(i2c_priv);
	return 0;
}

static void smartpakit_i2c_shutdown(struct i2c_client *i2c)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_gpio_sequence_t *sequence = NULL;
	int ret = 0;
	int i = 0;

	hwlog_info("%s: shutdown for smartpakit_i2c.\n", __func__);
	if (NULL == i2c) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	i2c_priv = i2c_get_clientdata(i2c);
	if (NULL == i2c_priv) {
		hwlog_err("%s: i2c_priv invalid argument!!!\n", __func__);
		return;
	}

	// for shutdown in music playing
	if ((i2c_priv->hw_reset != NULL) && (i2c_priv->hw_reset->sequence.node != NULL)) {
		if (i2c_priv->hw_reset->not_need_shutdown) {
			hwlog_info("%s: not need shutdown, skip!!!\n", __func__);
			return;
		}

		sequence = &i2c_priv->hw_reset->sequence;
		for (i = (int)(sequence->num - 1); i >=0; i--) {
			ret = gpio_direction_output((unsigned)i2c_priv->hw_reset->gpio, (int)sequence->node[i].state);
			hwlog_err("%s: hw_reset%d=%d,%d, ret=%d\n", __func__,
				i2c_priv->hw_reset->gpio, sequence->node[i].state, sequence->node[i].delay, ret);
			if (sequence->node[i].delay > 0) {
				// delay time units: msecs
				// last node use msleep
				if (i == 0) {
					msleep(sequence->node[i].delay);
				} else {
					mdelay(sequence->node[i].delay); /*lint !e647 !e747 !e774*/
				}
			}
		}
	} else {
		hwlog_info("%s: hw_reset is invalid argument!!!\n", __func__);
	}

	return;
}

#ifdef CONFIG_PM
static int smartpakit_i2c_suspend(struct device *dev)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;

	if (NULL == dev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	i2c_priv = dev_get_drvdata(dev);
	if (NULL == i2c_priv) {
		//hwlog_err("%s: i2c_priv invalid argument!!!\n", __func__);
		return 0;
	}

	if ((i2c_priv->regmap_cfg != NULL) && (i2c_priv->regmap_cfg->regmap != NULL)
		&& (REGCACHE_RBTREE == i2c_priv->regmap_cfg->cfg.cache_type)) { /*lint !e115*/
		regcache_cache_only(i2c_priv->regmap_cfg->regmap, (bool)true);
	}

	return 0;
}

static int smartpakit_i2c_resume(struct device *dev)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;

	if (NULL == dev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	i2c_priv = dev_get_drvdata(dev);
	if (NULL == i2c_priv) {
		//hwlog_err("%s: i2c_priv invalid argument!!!\n", __func__);
		return 0;
	}

	if ((i2c_priv->regmap_cfg != NULL) && (i2c_priv->regmap_cfg->regmap != NULL)
		&& (REGCACHE_RBTREE == i2c_priv->regmap_cfg->cfg.cache_type)) { /*lint !e115*/
		regcache_cache_only(i2c_priv->regmap_cfg->regmap, (bool)false);
		regcache_sync(i2c_priv->regmap_cfg->regmap);
	}

	return 0;
}
#else
#define smartpakit_i2c_suspend NULL
#define smartpakit_i2c_resume  NULL
#endif // !CONFIG_PM

static const struct dev_pm_ops smartpakit_i2c_pm_ops = {
	.suspend = smartpakit_i2c_suspend,
	.resume  = smartpakit_i2c_resume,
};

static const struct i2c_device_id smartpakit_i2c_id[] = {
	{ "smartpakit_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smartpakit_i2c_id);

/*lint -e528*/
static const struct of_device_id smartpakit_i2c_match[] = {
	{ .compatible = "huawei,smartpakit_i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, smartpakit_i2c_match);

static struct i2c_driver smartpakit_i2c_driver = {
	.driver = {
		.name  = "smartpakit_i2c",
		.owner = THIS_MODULE,
		.pm    = &smartpakit_i2c_pm_ops,
		.of_match_table = of_match_ptr(smartpakit_i2c_match),
	},
	.probe    = smartpakit_i2c_probe,
	.remove   = smartpakit_i2c_remove,
	.shutdown = smartpakit_i2c_shutdown,
	.id_table = smartpakit_i2c_id,
};

static int __init smartpakit_i2c_init(void)
{
#ifdef CONFIG_HUAWEI_SMARTPAKIT_AUDIO_MODULE
	hwlog_info("%s enter from modprobe.\n", __func__);
#endif
	return i2c_add_driver(&smartpakit_i2c_driver);
}

static void __exit smartpakit_i2c_exit(void)
{
	i2c_del_driver(&smartpakit_i2c_driver);
}
/*lint +e438 +e838*/

late_initcall(smartpakit_i2c_init);
module_exit(smartpakit_i2c_exit);

/*lint -e753*/
MODULE_DESCRIPTION("smartpakit i2c driver");
MODULE_AUTHOR("wangping<wangping48@huawei.com>");
MODULE_LICENSE("GPL");

