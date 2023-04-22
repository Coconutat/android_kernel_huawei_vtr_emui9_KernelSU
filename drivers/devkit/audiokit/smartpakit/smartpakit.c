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

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#ifdef CONFIG_HUAWEI_DSM_AUDIO_MODULE
#define CONFIG_HUAWEI_DSM_AUDIO
#endif
#ifdef CONFIG_HUAWEI_DSM_AUDIO
#include <dsm/dsm_pub.h>
#endif

#include "smartpakit.h"

#define HWLOG_TAG smartpakit
#define REG_PRINT_NUM 2
HWLOG_REGIST();

#define SMARTPAKIT_IOCTL_OPS(pakit, func) \
do {\
	if ((pakit != NULL) && (pakit->ioctl_ops != NULL) && (pakit->ioctl_ops->func != NULL)) { \
		ret = pakit->ioctl_ops->func(pakit); \
	} \
} while(0)

smartpakit_priv_t *smartpakit_priv = NULL;
EXPORT_SYMBOL_GPL(smartpakit_priv);

// 0 not init completed, 1 init completed
int smartpakit_init_flag = 0;
EXPORT_SYMBOL_GPL(smartpakit_init_flag);

static void smartpakit_reset_i2c_addr_to_pa_index(smartpakit_i2c_priv_t *i2c_priv, unsigned int id)
{
	if (smartpakit_priv->misc_rw_permission_enable && (i2c_priv->i2c != NULL)) {
		if (!smartpakit_priv->misc_i2c_use_pseudo_addr) {
			if (i2c_priv->i2c->addr < SMARTPAKIT_I2C_ADDR_ARRAY_MAX) {
				smartpakit_priv->i2c_addr_to_pa_index[i2c_priv->i2c->addr] = (unsigned char)id;
			}
		} else {
			// use pseudo address to rw i2c
			// for example:
			// pa index:    0    1    2    3
			// i2c addr:    0x35 0x36 0x35 0x36
			// pseudo addr: 0xAA 0xBB 0xCC 0xDD
			if (i2c_priv->i2c_pseudo_addr < SMARTPAKIT_I2C_ADDR_ARRAY_MAX) {
				smartpakit_priv->i2c_addr_to_pa_index[i2c_priv->i2c_pseudo_addr] = (unsigned char)id;
			}
		}
	}
}

static void smartpakit_check_and_set_hw_dev_flag(void)
{
	bool diff_flag = false;
	int i = 0;

	if (NULL == smartpakit_priv) {
		hwlog_info("%s: smartpakit_priv is NULL.\n", __func__);
		return;
	}

	if (smartpakit_priv->i2c_num < smartpakit_priv->pa_num) {
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: i2c_num < pa_num.\n", __func__);
#endif
		return;
	}

	if (smartpakit_priv->pa_num > 1) {
		for (i = 1; i < smartpakit_priv->pa_num; i++) {
			if (strcmp(smartpakit_priv->chip_model_list[0], smartpakit_priv->chip_model_list[i])) {
				diff_flag = true;
				break;
			}
		}
	}

	if (diff_flag) {
		hwlog_info("%s: PLEASE NOTE: CHIP_MODEL NOT SAME!!!\n", __func__);
	} else {
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: check chip_model success.\n", __func__);
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
		set_hw_dev_flag(DEV_I2C_SPEAKER);
#endif
	}

	return;
}

static void smartpakit_set_gpio_for_check_request(smartpakit_i2c_priv_t *i2c_priv)
{
	if (i2c_priv != NULL) {
		if (i2c_priv->hw_reset != NULL) {
			smartpakit_priv->gpio_of_hw_reset[i2c_priv->chip_id] = i2c_priv->hw_reset->gpio;
		}

		if (i2c_priv->irq_handler != NULL) {
			smartpakit_priv->gpio_of_irq[i2c_priv->chip_id] = i2c_priv->irq_handler->gpio;
		}
	}
}

/*lint -e438 -e838*/
int smartpakit_register_i2c_device(smartpakit_i2c_priv_t *i2c_priv)
{
	int ret = 0;

	if ((NULL == smartpakit_priv) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (i2c_priv->chip_id >= smartpakit_priv->pa_num) {
		hwlog_err("%s: invalid argument, chip_id %d>=%d!!!\n", __func__, i2c_priv->chip_id, smartpakit_priv->pa_num);
		return -EINVAL;
	}

	if (smartpakit_priv->i2c_priv[i2c_priv->chip_id] != NULL) {
		hwlog_err("%s: chip_id reduplicated error!!!\n", __func__);
		ret = -EINVAL;
	} else {
		i2c_priv->priv_data = (void *)smartpakit_priv;

		smartpakit_priv->i2c_num++;
		smartpakit_priv->i2c_priv[i2c_priv->chip_id] = i2c_priv;
		strncpy(smartpakit_priv->chip_model_list[i2c_priv->chip_id], i2c_priv->chip_model,
			(strlen(i2c_priv->chip_model) < SMARTPAKIT_NAME_MAX) ? strlen(i2c_priv->chip_model) : (SMARTPAKIT_NAME_MAX - 1));
		smartpakit_reset_i2c_addr_to_pa_index(i2c_priv, i2c_priv->chip_id);
		smartpakit_check_and_set_hw_dev_flag();

		smartpakit_set_gpio_for_check_request(i2c_priv);

#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: i2c_priv registered, success!!!\n", __func__);
#endif
	}

	return ret;
}
EXPORT_SYMBOL_GPL(smartpakit_register_i2c_device);

int smartpakit_deregister_i2c_device(smartpakit_i2c_priv_t *i2c_priv)
{
	int i = 0;

	if ((NULL == smartpakit_priv) || (NULL == i2c_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < (int)smartpakit_priv->pa_num; i++) {
		if (NULL == smartpakit_priv->i2c_priv[i]) {
			continue;
		}

		if (i2c_priv->chip_id == smartpakit_priv->i2c_priv[i]->chip_id) {
			smartpakit_reset_i2c_addr_to_pa_index(i2c_priv, SMARTPAKIT_INVALID_PA_INDEX);
			i2c_priv->priv_data = NULL;

			smartpakit_priv->i2c_num--;
			smartpakit_priv->i2c_priv[i] = NULL;
			hwlog_info("%s: i2c_priv deregistered, success!!!\n", __func__);

			if (0 == smartpakit_priv->i2c_num) {
				smartpakit_priv->ioctl_ops = NULL;
				hwlog_info("%s: ioctl_ops deregistered, success!!!\n", __func__);
			} else {
				hwlog_info("%s: ioctl_ops deregistered, skip!!!\n", __func__);
			}
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(smartpakit_deregister_i2c_device);

void smartpakit_register_ioctl_ops(smartpakit_i2c_ioctl_ops_t *ops)
{
	if (smartpakit_priv != NULL) {
		if (NULL == smartpakit_priv->ioctl_ops) {
			smartpakit_priv->ioctl_ops = ops;
#ifndef CONFIG_FINAL_RELEASE
			hwlog_info("%s: ioctl_ops registered, success!!!\n", __func__);
#endif
		} else {
#ifndef CONFIG_FINAL_RELEASE
			hwlog_info("%s: ioctl_ops registered, skip!!!\n", __func__);
#endif
		}
	} else {
		hwlog_err("%s: ioctl_ops register failed!!!\n", __func__);
	}
}
EXPORT_SYMBOL_GPL(smartpakit_register_ioctl_ops);

int smartpakit_parse_params(smartpakit_pa_ctl_sequence_t *sequence, void __user *arg, int compat_mode)
{
	smartpakit_set_param_t param;
	smartpakit_param_node_t *node = NULL;
	unsigned int pa_ctl_write = 0;
	int param_num_need_ops = 0;
	int val_num = 0;
	int ret = 0;
	int i = 0;
	unsigned int need_print_node_num = 0;

	hwlog_debug("%s: enter ...\n", __func__);
	if ((NULL == smartpakit_priv) || (NULL == sequence) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	memset(&param, 0, sizeof(smartpakit_set_param_t));
	// for support arm64, here sizeof int* is 8 bytes
	// But for support arm32 only, here sizeof int * is 4 bytes
#ifdef CONFIG_COMPAT
	if (0 == compat_mode) {
#endif
		smartpakit_set_param_t s;

		// for system/lib64/*.so(64 bits)
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: copy_from_user b64...\n", __func__);
#endif
		if (copy_from_user(&s, arg, sizeof(smartpakit_set_param_t))) {
			hwlog_err("%s: get set_param copy_from_user fail!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}

		param.pa_ctl_mask = s.pa_ctl_mask;
		param.param_num   = s.param_num;
		param.params      = (void __user *)s.params;
#ifdef CONFIG_COMPAT
	} else { // 1 == compat_mode
		smartpakit_set_param_compat_t s_compat;

		// for system/lib/*.so(32 bits)
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: copy_from_user b32...\n", __func__);
#endif
		if (copy_from_user(&s_compat, arg, sizeof(smartpakit_set_param_compat_t))) {
			hwlog_err("%s: get set_param_compat copy_from_user fail!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}

		param.pa_ctl_mask = s_compat.pa_ctl_mask;
		param.param_num   = s_compat.param_num;
		param.params      = compat_ptr(s_compat.params_ptr);
	}

#endif // CONFIG_COMPAT

	hwlog_debug("%s: regs_num=%d.\n", __func__, param.param_num);
	if ((0 == param.param_num) || (param.param_num > SMARTPAKIT_IO_PARAMS_NUM_MAX) || (NULL == (void *)param.params)) {
		hwlog_err("%s: reg_w_sequence invalid argument(%d,%pK)!!!\n", __func__, param.param_num, (void *)param.params);
		ret = -EINVAL;
		goto err_out;
	}

	param_num_need_ops = (int)param.param_num;
	val_num = sizeof(smartpakit_param_node_t) / sizeof(unsigned int);

	if ((param_num_need_ops % val_num) != 0) {
		hwlog_err("%s: invalid argument, regs_num(%d) %% %d != 0, please check XML settings!!!\n", __func__,
			param_num_need_ops, val_num);
		ret = -EINVAL;
		goto err_out;
	}
	param_num_need_ops /= val_num;

	node = (smartpakit_param_node_t *)kzalloc(sizeof(smartpakit_param_node_t) * param_num_need_ops, GFP_KERNEL); /*lint !e737*/
	if (NULL == node) {
		hwlog_err("%s: kzalloc regs failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	if (copy_from_user(node, (void __user *)param.params, sizeof(smartpakit_param_node_t) * param_num_need_ops)) { /*lint !e737*/
		hwlog_err("%s: set regs copy_from_user failed!!!\n", __func__);
		ret = -EIO;
		goto err_out;
	}

	// check whether poweron or not
	if (param.pa_ctl_mask & SMARTPAKIT_NEED_RESUME_FLAG) {
		// clear poweron flag
		param.pa_ctl_mask &= (~SMARTPAKIT_NEED_RESUME_FLAG);

		// set poweron flag
		sequence->pa_poweron_flag = 1;
	} else {
		sequence->pa_poweron_flag = 0;
	}
	sequence->pa_ctl_mask = param.pa_ctl_mask;

	// pa ctl mask
	if (param.pa_ctl_mask > 0) {
		sequence->pa_ctl_num = 0;
		sequence->pa_ctl_index_max = 0;

		for (i = 0; i < SMARTPAKIT_PA_ID_MAX; i++) {
			pa_ctl_write  = param.pa_ctl_mask >> (i * SMARTPAKIT_PA_CTL_OFFSET);
			pa_ctl_write &= SMARTPAKIT_PA_CTL_MASK;

			if (0 == pa_ctl_write) {
				continue;
			}
			if (i >= smartpakit_priv->i2c_num) {
				ret = -EINVAL;
				hwlog_info("%s: pa_ctl_mask param invalid.\n", __func__);
				goto err_out;
			}

			sequence->pa_ctl_index_max = (unsigned int)i;
			sequence->pa_ctl_index[sequence->pa_ctl_num] = (unsigned int)i;
			sequence->pa_ctl_num++;
		}
	}

	sequence->param_num = (unsigned int)param_num_need_ops;
	sequence->node = node;

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: pa_ctl_mask=0x%04x, pa_ctl[%d]=%d, %d, %d, %d.\n", __func__, param.pa_ctl_mask, sequence->pa_ctl_num,
		sequence->pa_ctl_index[0], sequence->pa_ctl_index[1], sequence->pa_ctl_index[2], sequence->pa_ctl_index[3]);
#endif

	need_print_node_num = (sequence->param_num < REG_PRINT_NUM) ? sequence->param_num : REG_PRINT_NUM;

	for (i = 0; i < need_print_node_num; i++) {
		hwlog_info("%s: reg[%d]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x(only print two registers).\n", __func__, i,
			node[i].index, node[i].mask, node[i].value, node[i].delay, node[i].reserved);
	}
	hwlog_debug("%s: enter end, success.\n", __func__);
	return 0;

err_out:
	if (node != NULL) {
		kfree(node);
		node = NULL;
	}

	hwlog_debug("%s: error end, ret=%d.\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(smartpakit_parse_params);

static void smartpakit_reset_poweron_regs(smartpakit_pa_ctl_sequence_t *poweron)
{
	poweron->pa_ctl_mask	  = 0;
	poweron->pa_poweron_flag  = 0;
	poweron->pa_ctl_num 	  = 0;
	poweron->pa_ctl_index_max = 0;
	poweron->pa_ctl_index[0]  = 0;

	poweron->param_num = 0;
	if (poweron->node != NULL) {
		kfree(poweron->node);
		poweron->node = NULL;
	}
}

int smartpakit_set_poweron_regs(smartpakit_priv_t *pakit_priv, smartpakit_pa_ctl_sequence_t *sequence)
{
	smartpakit_pa_ctl_sequence_t *poweron = NULL;
	int i = 0;

	if ((NULL == pakit_priv) || (NULL == sequence)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s: pa_ctl_mask=0x%x, pa_poweron_flag=%d,pa_ctl_num=%d, param_num_of_one_pa=%d\n", __func__,
		sequence->pa_ctl_mask, sequence->pa_poweron_flag, sequence->pa_ctl_num, sequence->param_num_of_one_pa);
	for (i = 0; i < (int)sequence->pa_ctl_num; i++) {
		poweron = &pakit_priv->poweron_sequence[sequence->pa_ctl_index[i]];

		if (1 == sequence->pa_poweron_flag) {
			smartpakit_reset_poweron_regs(poweron);

			poweron->pa_ctl_mask	  = 1 << (sequence->pa_ctl_index[i] * SMARTPAKIT_PA_CTL_OFFSET);
			poweron->pa_poweron_flag  = 1;
			poweron->pa_ctl_num 	  = 1;
			poweron->pa_ctl_index_max = sequence->pa_ctl_index[i];
			poweron->pa_ctl_index[0]  = sequence->pa_ctl_index[i];
#ifndef CONFIG_FINAL_RELEASE
			hwlog_info("%s: on 0x%x, %d, %d\n", __func__, poweron->pa_ctl_mask,
				poweron->pa_ctl_index_max, poweron->pa_ctl_index[0]);
#endif

			poweron->param_num = sequence->param_num_of_one_pa;
			poweron->node = (smartpakit_param_node_t *)kzalloc(sizeof(smartpakit_param_node_t) * poweron->param_num, GFP_KERNEL);
			if (NULL == poweron->node) {
				hwlog_err("%s: poweron->node kzalloc failed!!!\n", __func__);
				return -ENOMEM;
			}

			memcpy(poweron->node, sequence->node + (i * poweron->param_num),
				sizeof(smartpakit_param_node_t) * poweron->param_num);
		} else { // 0 == sequence->pa_poweron_flag
			smartpakit_reset_poweron_regs(poweron);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(smartpakit_set_poweron_regs);

bool smartpakit_is_gpio_request(int gpio, bool is_irq)
{
	bool request = false;
	int i = 0;

	if (smartpakit_priv == NULL) {
		hwlog_err("%s: smartpakit_priv is NULL!!!\n", __func__);
		return request;
	}

	for (i = 0; i < smartpakit_priv->pa_num; i++) {
		if (is_irq) { // for irq
			if (smartpakit_priv->gpio_of_irq[i] == gpio) {
				request = true;
				break;
			}
		} else { // for hw_reset
			if (smartpakit_priv->gpio_of_hw_reset[i] == gpio) {
				request = true;
				break;
			}
		}
	}

	return request;
}

EXPORT_SYMBOL_GPL(smartpakit_is_gpio_request);

static int smartpakit_ctrl_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	if ((NULL == inode) || (NULL == filp)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	ret = nonseekable_open(inode, filp);
	if (ret < 0) {
		hwlog_err("%s: nonseekable_open failed!!!\n", __func__);
		return ret;
	}

	filp->private_data = (void *)smartpakit_priv;
	return ret;
}

static int smartpakit_ctrl_release(struct inode *inode, struct file *filp)
{
	if ((NULL == inode) || (NULL == filp)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	filp->private_data = NULL;
	return 0;
}

static int smartpakit_ctrl_get_current_i2c_client(smartpakit_priv_t *pakit_priv, unsigned short addr)
{
	if (NULL == pakit_priv) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&pakit_priv->i2c_ops_lock);

	pakit_priv->current_i2c_client = NULL;
	if (addr >= SMARTPAKIT_I2C_ADDR_ARRAY_MAX) {
		hwlog_err("%s: invalid i2c slave addr 0x%x!!!\n", __func__, addr);
		goto ERR;
	}

	if (pakit_priv->i2c_addr_to_pa_index[addr] == SMARTPAKIT_INVALID_PA_INDEX) {
		hwlog_err("%s: i2c slave addr 0x%x not registered!!!\n", __func__, addr);
		goto ERR;
	}

	pakit_priv->current_i2c_client = pakit_priv->i2c_priv[smartpakit_priv->i2c_addr_to_pa_index[addr]]->i2c;
	mutex_unlock(&pakit_priv->i2c_ops_lock);
	return 0;
ERR:
	mutex_unlock(&pakit_priv->i2c_ops_lock);
	return -EINVAL;
}

static ssize_t smartpakit_ctrl_read(struct file *file, char __user *buf,
					size_t nbytes, loff_t *pos)
{
	smartpakit_priv_t *pakit_priv = NULL;
	void *kern_buf = NULL;
	int ret = 0;

	UNUSED(pos);
	if ((NULL == file) || (NULL == buf)) {
		hwlog_err("%s: invalid argument, file or buf is NULL!!!\n", __func__);
		return -EINVAL;
	}
	pakit_priv = (smartpakit_priv_t *)file->private_data;

	if ((NULL == pakit_priv) || (NULL == pakit_priv->current_i2c_client)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&pakit_priv->i2c_ops_lock);

	if ((0 == nbytes) || (nbytes > SMARTPAKIT_RW_PARAMS_NUM_MAX)) {
		hwlog_err("%s: nbytes %d is 0 or >%d bytes, error!\n", __func__, (int)nbytes, SMARTPAKIT_RW_PARAMS_NUM_MAX);
		ret = -EINVAL;
		goto ERR;
	}

	kern_buf = kmalloc(nbytes, GFP_KERNEL);
	if (!kern_buf) {
		hwlog_err("Failed to allocate buffer\n");
		ret = -ENOMEM;
		goto ERR;
	}

	if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->i2c_read != NULL)) {
		ret = pakit_priv->ioctl_ops->i2c_read(pakit_priv->current_i2c_client, (char *)kern_buf, (int)nbytes);
	}

	if (0 > ret) {
		hwlog_err("%s: i2c read error %d", __func__, ret);
		goto ERR;
	}

	if (copy_to_user((void  __user *)buf, kern_buf,  nbytes)) {
		ret = -EFAULT;
		goto ERR;
	}

	mutex_unlock(&pakit_priv->i2c_ops_lock);
	kfree(kern_buf);
	return (ssize_t)nbytes;
ERR:
	mutex_unlock(&pakit_priv->i2c_ops_lock);
	if (kern_buf) {
		kfree(kern_buf);
		kern_buf = NULL;
	}
	return ret;
}

static ssize_t smartpakit_ctrl_write(struct file *file,
			 const char __user *buf, size_t nbytes, loff_t *ppos)
{
	smartpakit_priv_t *pakit_priv = NULL;
	void *kern_buf = NULL;
	int ret = 0;

	UNUSED(ppos);
	if ((NULL == file) || (NULL == buf)) {
		hwlog_err("%s: invalid argument, file or buf is NULL!!!\n", __func__);
		return -EINVAL;
	}
	pakit_priv = (smartpakit_priv_t *)file->private_data;

	if ((NULL == pakit_priv) || (NULL == pakit_priv->current_i2c_client)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&pakit_priv->i2c_ops_lock);

	if ((0 == nbytes) || (nbytes > SMARTPAKIT_RW_PARAMS_NUM_MAX)) {
		hwlog_err("%s: nbytes %d is 0 or > %d bytes, error!\n", __func__, (int)nbytes, SMARTPAKIT_RW_PARAMS_NUM_MAX);
		ret = -EINVAL;
		goto ERR;
	}

	kern_buf = kmalloc(nbytes, GFP_KERNEL);
	if (!kern_buf) {
		hwlog_err("Failed to allocate buffer\n");
		ret = -ENOMEM;
		goto ERR;
	}

	if (copy_from_user(kern_buf, (void  __user *)buf, nbytes)) {
		ret = -EFAULT;
		goto ERR;
	}

	if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->i2c_write != NULL)) {
		ret = pakit_priv->ioctl_ops->i2c_write(pakit_priv->current_i2c_client, (char *)kern_buf, (int)nbytes);
	}

	if (0 > ret) {
		hwlog_err("%s: i2c write error %d", __func__, ret);
		goto ERR;
	}
	mutex_unlock(&pakit_priv->i2c_ops_lock);

	kfree(kern_buf);
	return (ssize_t)nbytes;
ERR:
	mutex_unlock(&pakit_priv->i2c_ops_lock);
	if (kern_buf) {
		kfree(kern_buf);
		kern_buf = NULL;
	}
	return ret;
}

void smartpakit_ctrl_get_model(char *dst, smartpakit_priv_t *pakit_priv)
{
	char report_tmp[SMARTPAKIT_NAME_MAX] = { 0 };
	size_t model_len = 0;
	int i = 0;

	if ((NULL == dst) || (NULL == pakit_priv)) {
		hwlog_info("%s: invalid argument!!!\n", __func__);
		return;
	}

	for (i = 0; i < pakit_priv->pa_num; i++) {
		if (i < (pakit_priv->pa_num - 1)) {
			snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "%s_", pakit_priv->chip_model_list[i]);
		} else {
			snprintf(report_tmp, (unsigned long)SMARTPAKIT_NAME_MAX, "%s", pakit_priv->chip_model_list[i]);
		}
		model_len = SMARTPAKIT_NAME_MAX - (strlen(dst) + 1);
		model_len = (strlen(report_tmp) < model_len) ? strlen(report_tmp) : model_len;
		strncat(dst, report_tmp, model_len);
	}
}

EXPORT_SYMBOL_GPL(smartpakit_ctrl_get_model);

static int smartpakit_ctrl_get_info(smartpakit_priv_t *pakit_priv, void __user *arg)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	smartpakit_info_t info;
	size_t model_len = 0;
	char report_tmp[SMARTPAKIT_NAME_MAX] = { 0 };

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter ...\n", __func__);
#endif
	if ((NULL == pakit_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	memset(&info, 0, sizeof(smartpakit_info_t));
	// common info
	info.soc_platform    = pakit_priv->soc_platform;
	info.algo_in	     = pakit_priv->algo_in;
	info.out_device      = pakit_priv->out_device;
	info.algo_delay_time = pakit_priv->algo_delay_time;
	info.two_in_one      = pakit_priv->two_in_one;

	if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
		&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
		// check whether init i2c devices completely or not
		// maybe i2c_num < pa_num for smartpa with dsp
		if (0 == pakit_priv->i2c_num) {
			hwlog_info("%s: init not completed!!!\n", __func__);
			return -EAGAIN;
		}
		info.pa_num = pakit_priv->i2c_num;

		// set info
		i2c_priv = pakit_priv->i2c_priv[0];
		if ((0 == pakit_priv->pa_num) || (NULL == i2c_priv)) {
			hwlog_err("%s: pa_num or i2c_priv invalid argument!!!\n", __func__);
			return -EINVAL;
		}

		// smartpa chip info
		info.chip_vendor = i2c_priv->chip_vendor;
		if (SMARTPAKIT_ALGO_IN_WITH_DSP == pakit_priv->algo_in) {
			if (i2c_priv->chip_model != NULL) {
				model_len = strlen(i2c_priv->chip_model);
				model_len = (model_len < SMARTPAKIT_NAME_MAX) ? model_len : (SMARTPAKIT_NAME_MAX - 1);
				strncpy(info.chip_model, i2c_priv->chip_model, model_len);
			}
		} else {
			smartpakit_ctrl_get_model(report_tmp, pakit_priv);
			snprintf(info.chip_model, (unsigned long)SMARTPAKIT_NAME_MAX, "%s", report_tmp);
		}
	} else { // simple pa(not smartpa) or smartpa with dsp + plugin
		info.pa_num = pakit_priv->pa_num;
		if (SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN == pakit_priv->algo_in) {
			info.chip_vendor = pakit_priv->chip_vendor;
		} else {
			if (NULL == pakit_priv->switch_ctl) {
				hwlog_info("%s: pakit_priv->switch_ctl == NULL!!!\n", __func__);
				return -EAGAIN;
			}
			info.chip_vendor = SMARTPAKIT_CHIP_VENDOR_OTHER;
		}

		if (pakit_priv->chip_model != NULL) {
			model_len = strlen(pakit_priv->chip_model);
			model_len = (model_len < SMARTPAKIT_NAME_MAX) ? model_len : (SMARTPAKIT_NAME_MAX - 1);
			strncpy(info.chip_model, pakit_priv->chip_model, model_len);
		}
	}

	hwlog_info("%s: soc_platform=%d, algo_in=%d, out_device=0x%08x, pa_num=%d, algo_delay_time=%d, chip_vendor=%d, chip_model=%s, two_in_one=%d.\n", __func__,
					info.soc_platform, info.algo_in, info.out_device, info.pa_num, info.algo_delay_time, info.chip_vendor, info.chip_model, info.two_in_one );
	if (info.pa_num != pakit_priv->pa_num) {
		hwlog_info("%s: <***** NOTICE: I2C Number not equal PA Number,*******>, i2c_num = %d, pa_num = %d.\n", __func__, info.pa_num, pakit_priv->pa_num);
	}

	if (copy_to_user((void *)arg, &info, sizeof(smartpakit_info_t))) {
		hwlog_err("%s: send smartpakit info to user failed!!!", __func__);
		return -EIO;
	}

	pakit_priv->resume_sequence_permission_enable = true;
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter end.\n", __func__);
#endif
	return 0;
}

static int smartpakit_ctrl_prepare(smartpakit_priv_t *pakit_priv)
{
	UNUSED(pakit_priv);
	return 0;
}

static int smartpakit_ctrl_unprepare(smartpakit_priv_t *pakit_priv)
{
	UNUSED(pakit_priv);
	return 0;
}

static int smartpakit_ctrl_set_resume_regs(smartpakit_priv_t *pakit_priv, void __user *arg, int compat_mode)
{
	int ret = 0;
	int i;
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter ...\n", __func__);
#endif
	if ((NULL == pakit_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	if (!pakit_priv->resume_sequence_permission_enable) {
		hwlog_err("%s: no permission of resume_sequence, skip!!!\n", __func__);
		return -EINVAL;
	}
	pakit_priv->resume_sequence_permission_enable = false;

	// clear poweron sequence to avoid audio server died
	for (i = 0; i < SMARTPAKIT_PA_ID_MAX; i++)
		smartpakit_reset_poweron_regs(&pakit_priv->poweron_sequence[i]);

	memset(&pakit_priv->resume_sequence, 0, sizeof(smartpakit_pa_ctl_sequence_t));
	ret = smartpakit_parse_params(&pakit_priv->resume_sequence, arg, compat_mode);
	if (ret < 0) {
		hwlog_err("%s: parse w_regs failed!!!\n", __func__);
		return -EINVAL;
	}

	// check pa_ctl_num
	if ((pakit_priv->resume_sequence.pa_ctl_num > pakit_priv->pa_num)
		|| (pakit_priv->resume_sequence.pa_ctl_index_max >= pakit_priv->pa_num)) {
		hwlog_err("%s: invalid regs_sequence, pa_ctl_num %d>%d, pa_ctl_index_max %d>=%d!!!\n", __func__,
			pakit_priv->resume_sequence.pa_ctl_num, pakit_priv->pa_num,
			pakit_priv->resume_sequence.pa_ctl_index_max, pakit_priv->pa_num);
		return -EINVAL;
	}

	// write init chip regs
	mutex_lock(&pakit_priv->i2c_ops_lock);
	if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->do_write_regs_all != NULL)) {
		ret = pakit_priv->ioctl_ops->do_write_regs_all(pakit_priv, &pakit_priv->resume_sequence);
	} else {
		hwlog_err("%s: ioctl_ops or do_write_regs_all is NULL!!!\n", __func__);
		ret = -ECHILD;
	}
	mutex_unlock(&pakit_priv->i2c_ops_lock);

	hwlog_info("%s: enter end, ret=%d.\n", __func__, ret);
	return ret;
}

static int smartpakit_ctrl_read_regs(smartpakit_priv_t *pakit_priv, void __user *arg, unsigned int id)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	int ret = 0;

	if ((NULL == pakit_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&pakit_priv->i2c_ops_lock);
	if (pakit_priv->pa_num >= (id + 1)) {
		i2c_priv = pakit_priv->i2c_priv[id];
	} else {
		i2c_priv = pakit_priv->i2c_priv[0];
	}

	if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->read_regs != NULL)) {
		ret = pakit_priv->ioctl_ops->read_regs(i2c_priv, arg);
	} else {
		hwlog_err("%s: ioctl_ops or read_regs is NULL!!!\n", __func__);
		ret = -ECHILD;
	}
	mutex_unlock(&pakit_priv->i2c_ops_lock);

	return ret;
}

static int smartpakit_ctrl_write_regs(smartpakit_priv_t *pakit_priv, void __user *arg, int compat_mode, unsigned int id)
{
	smartpakit_i2c_priv_t *i2c_priv = NULL;
	int ret = 0;

	hwlog_debug("%s: enter id=0x%x...\n", __func__, id);
	if ((NULL == pakit_priv) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&pakit_priv->i2c_ops_lock);
	if (SMARTPAKIT_PA_ID_ALL == id) {
		if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->write_regs_all != NULL)) {
			ret = pakit_priv->ioctl_ops->write_regs_all(pakit_priv, arg, compat_mode);
		} else {
			hwlog_err("%s: ioctl_ops or write_regs_all is NULL!!!\n", __func__);
			ret = -ECHILD;
		}
	} else {
		if (pakit_priv->pa_num >= (id + 1)) {
			i2c_priv = pakit_priv->i2c_priv[id];
		} else {
			i2c_priv = pakit_priv->i2c_priv[0];
		}

		if ((pakit_priv->ioctl_ops != NULL) && (pakit_priv->ioctl_ops->write_regs != NULL)) {
			ret = pakit_priv->ioctl_ops->write_regs(i2c_priv, arg, compat_mode);
		} else {
			hwlog_err("%s: ioctl_ops or write_regs is NULL!!!\n", __func__);
			ret = -ECHILD;
		}
	}
	mutex_unlock(&pakit_priv->i2c_ops_lock);

	hwlog_debug("%s: enter end, ret=%d.\n", __func__, ret);
	return ret;
}

static int smartpakit_ctrl_simple_pa(smartpakit_priv_t *pakit_priv, void __user *arg, int compat_mode)
{
	smartpakit_pa_ctl_sequence_t sequence;
	int index = 0;
	int ret = 0;
	int i = 0;

	hwlog_info("%s: enter ...\n", __func__);
	if ((NULL == pakit_priv) || (NULL == pakit_priv->switch_ctl) || (NULL == arg)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	memset(&sequence, 0, sizeof(smartpakit_pa_ctl_sequence_t));
	ret = smartpakit_parse_params(&sequence, arg, compat_mode);
	if (ret < 0) {
		hwlog_err("%s: parse gpio_state failed!!!\n", __func__);
		goto err_out;
	}

	// for simple pa, node type is:
	// 1. gpio node
	// 2. delay time node
	for (i = 0; i < (int)sequence.param_num; i++) {
		if (SMARTPAKIT_PARAM_NODE_TYPE_DELAY == sequence.node[i].node_type) {
			if (sequence.node[i].delay > 0) {
				if (sequence.node[i].delay > SMARTPAKIT_DELAY_US_TO_MS) {
					msleep(sequence.node[i].delay / SMARTPAKIT_DELAY_US_TO_MS);
				} else {
					udelay(sequence.node[i].delay); /*lint !e747*/
				}
			}
		} else { // other node type
			index = (int)sequence.node[i].index;
			if (index >= (int)pakit_priv->switch_num) {
				hwlog_err("%s: Invalid argument, node[%d].index %d>=%d!!!\n", __func__,
					i, index, pakit_priv->switch_num);
				ret = -EINVAL;
				goto err_out;
			}

			hwlog_info("%s: %d gpio[%d]=%d, %d.\n", __func__, i,
				pakit_priv->switch_ctl[index].gpio, sequence.node[i].value, sequence.node[i].delay);
			gpio_direction_output((unsigned)pakit_priv->switch_ctl[index].gpio, (int)sequence.node[i].value);
			if (sequence.node[i].delay > 0) {
				// delay time units: usecs
				if (sequence.node[i].delay > SMARTPAKIT_DELAY_US_TO_MS) {
					mdelay(sequence.node[i].delay / SMARTPAKIT_DELAY_US_TO_MS);
				} else {
					udelay(sequence.node[i].delay); /*lint !e747*/
				}
			}
		}
	}

err_out:
	if (sequence.node!= NULL) {
		kfree(sequence.node);
		sequence.node = NULL;
	}

	return ret;
}

static int smartpakit_ctrl_do_ioctl(struct file *file, unsigned int cmd, void __user *arg, int compat_mode)
{
	smartpakit_priv_t *pakit_priv = NULL;
	int ret = 0;

	if ((cmd != I2C_SLAVE) && (cmd != I2C_SLAVE_FORCE)) {
		hwlog_debug("%s: enter, cmd:0x%x compat_mode=%d...\n", __func__, cmd, compat_mode);
	}
	if ((NULL == file) || (NULL == file->private_data)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	pakit_priv = (smartpakit_priv_t *)file->private_data;

	switch (cmd) {
		// NOTE:
		// 1. smartpa with dsp, ex. tfa9895
		// 2. climax tool for NXP
		case I2C_SLAVE:
		case I2C_SLAVE_FORCE:
		ret = smartpakit_ctrl_get_current_i2c_client(pakit_priv, (unsigned short)(unsigned long)arg);
		break;

		// IO controls for smart pa
		case SMARTPAKIT_GET_INFO:
			ret = smartpakit_ctrl_get_info(pakit_priv, arg);
			break;
		/*lint -save -e845*/
		case SMARTPAKIT_HW_RESET:
			mutex_lock(&pakit_priv->hw_reset_lock);
			SMARTPAKIT_IOCTL_OPS(pakit_priv, hw_reset);
			mutex_unlock(&pakit_priv->hw_reset_lock);
			break;
		case SMARTPAKIT_HW_PREPARE:
			ret = smartpakit_ctrl_prepare(pakit_priv);
			break;
		case SMARTPAKIT_HW_UNPREPARE:
			ret = smartpakit_ctrl_unprepare(pakit_priv);
			break;
		case SMARTPAKIT_REGS_DUMP:
			mutex_lock(&pakit_priv->dump_regs_lock);
			SMARTPAKIT_IOCTL_OPS(pakit_priv, dump_regs);
			mutex_unlock(&pakit_priv->dump_regs_lock);
			break;
		/*lint -restore*/

		// read reg cmd
		case SMARTPAKIT_R_PRIL:
			ret = smartpakit_ctrl_read_regs(pakit_priv, arg, SMARTPAKIT_PA_ID_PRIL);
			break;
		case SMARTPAKIT_R_PRIR:
			ret = smartpakit_ctrl_read_regs(pakit_priv, arg, SMARTPAKIT_PA_ID_PRIR);
			break;
		case SMARTPAKIT_R_SECL:
			ret = smartpakit_ctrl_read_regs(pakit_priv, arg, SMARTPAKIT_PA_ID_SECL);
			break;
		case SMARTPAKIT_R_SECR:
			ret = smartpakit_ctrl_read_regs(pakit_priv, arg, SMARTPAKIT_PA_ID_SECR);
			break;

		// write regs cmd
		case SMARTPAKIT_INIT:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_set_resume_regs(pakit_priv, arg, compat_mode);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;
		case SMARTPAKIT_W_ALL:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_write_regs(pakit_priv, arg, compat_mode, SMARTPAKIT_PA_ID_ALL);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;
		case SMARTPAKIT_W_PRIL:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_write_regs(pakit_priv, arg, compat_mode, SMARTPAKIT_PA_ID_PRIL);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;
		case SMARTPAKIT_W_PRIR:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_write_regs(pakit_priv, arg, compat_mode, SMARTPAKIT_PA_ID_PRIR);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;
		case SMARTPAKIT_W_SECL:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_write_regs(pakit_priv, arg, compat_mode, SMARTPAKIT_PA_ID_SECL);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;
		case SMARTPAKIT_W_SECR:
			if ((pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)
				&& (pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN)) {
				ret = smartpakit_ctrl_write_regs(pakit_priv, arg, compat_mode, SMARTPAKIT_PA_ID_SECR);
			} else {
				ret = smartpakit_ctrl_simple_pa(pakit_priv, arg, compat_mode);
			}
			break;

		default:
			hwlog_err("%s: not support cmd(0x%x)!!!\n", __func__, cmd);
			ret = -EIO;
			break;
	}

	if ((cmd != I2C_SLAVE) && (cmd != I2C_SLAVE_FORCE)) {
		hwlog_info("%s: enter end, cmd:0x%x ret=%d.\n", __func__, cmd, ret);
	}
	return ret;
}

static long smartpakit_ctrl_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	return smartpakit_ctrl_do_ioctl(file, command, (void __user *)arg, 0);
}

#ifdef CONFIG_COMPAT
static long smartpakit_ctrl_ioctl_compat(struct file *file, unsigned int command, unsigned long arg)
{
	switch (command) {
		case SMARTPAKIT_INIT_COMPAT:
			command = SMARTPAKIT_INIT;
			break;
		case SMARTPAKIT_W_ALL_COMPAT:
			command = SMARTPAKIT_W_ALL;
			break;
		case SMARTPAKIT_W_PRIL_COMPAT:
			command = SMARTPAKIT_W_PRIL;
			break;
		case SMARTPAKIT_W_PRIR_COMPAT:
			command = SMARTPAKIT_W_PRIR;
			break;
		case SMARTPAKIT_W_SECL_COMPAT:
			command = SMARTPAKIT_W_SECL;
			break;
		case SMARTPAKIT_W_SECR_COMPAT:
			command = SMARTPAKIT_W_SECR;
			break;
		default:
			break;
	}
	return smartpakit_ctrl_do_ioctl(file, command, compat_ptr((unsigned int)arg), 1);
}
#else
#define smartpakit_ctrl_ioctl_compat NULL
#endif

static const struct file_operations smartpakit_ctrl_fops = {
	.owner          = THIS_MODULE,
	.open           = smartpakit_ctrl_open,
	.release        = smartpakit_ctrl_release,
	.read           = smartpakit_ctrl_read,
	.write          = smartpakit_ctrl_write,
	.unlocked_ioctl = smartpakit_ctrl_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = smartpakit_ctrl_ioctl_compat,
#endif
};

static struct miscdevice smartpakit_ctrl_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "smartpakit",
	.fops  = &smartpakit_ctrl_fops,
};

static int smartpakit_parse_dt_switch_ctl(struct platform_device *pdev, smartpakit_priv_t *pakit_priv)
{
	const char *switch_ctl_str = "switch_ctl";
	const char *gpio_reset_str = "gpio_reset";
	const char *gpio_state_default_str = "gpio_state_default";
	smartpakit_switch_node_t *ctl = NULL;
	struct device *dev = NULL;
	struct device_node *node = NULL;
	struct property *pp = NULL;
	int *gpio_state = NULL;
	int count = 0;
	int ret = 0;
	int i = 0;

	if ((NULL == pdev) || (NULL == pakit_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	dev = &pdev->dev;

	node = of_get_child_by_name(dev->of_node, switch_ctl_str);
	if (NULL == node) {
		hwlog_debug("%s: switch_ctl device_node not existed, skip!!!\n", __func__);
		return 0;
	}

	i = 0;
	/*lint -save -e108 -e413 -e613*/
	for_each_property_of_node(node, pp) {
		if (strncmp(pp->name, gpio_reset_str, strlen(gpio_reset_str)) != 0) {
			continue;
		}

		i++;
	}
	pakit_priv->switch_num = (unsigned int)i;

	ctl = kzalloc(sizeof(smartpakit_switch_node_t) * pakit_priv->switch_num, GFP_KERNEL);
	if (NULL == ctl) {
		hwlog_err("%s: kzalloc switch_ctl failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	i = 0;
	for_each_property_of_node(node, pp) {
		if (strncmp(pp->name, gpio_reset_str, strlen(gpio_reset_str)) != 0) {
			continue;
		}

		ret = snprintf(ctl[i].name, (unsigned long)SMARTPAKIT_NAME_MAX, "smartpakit_gpio_reset_%d", i);
		if (ret < 0) {
			hwlog_err("%s: switch_ctl set gpio_name failed!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}

		ctl[i].gpio = of_get_named_gpio(node, pp->name, 0);
		if (ctl[i].gpio < 0) {
			hwlog_info("%s: switch_ctl %s of_get_named_gpio failed(%d)!!!\n", __func__, pp->name, ctl[i].gpio);
			ret = of_property_read_u32(node, pp->name, (u32 *)&ctl[i].gpio);
			if (ret < 0) {
				hwlog_err("%s: switch_ctl %s of_property_read_u32 failed(%d)!!!\n", __func__, pp->name, ret);
				ret = -EFAULT;
				goto err_out;
			}
		}

		ctl[i].state = -1;
		i++;
	}
	/*lint -restore*/

	if (of_property_read_bool(node, gpio_state_default_str)) {
		count = of_property_count_elems_of_size(node, gpio_state_default_str, (int)sizeof(u32));
		if (count != (int)pakit_priv->switch_num) {
			hwlog_err("%s: gpio_state_default count %d!=%d!!!\n", __func__, count, pakit_priv->switch_num);
			ret = -EFAULT;
			goto err_out;
		}

		gpio_state = (int *)kzalloc(sizeof(int) * count, GFP_KERNEL); /*lint !e737*/
		if (NULL == gpio_state) {
			hwlog_err("%s: kzalloc gpio_state_default failed!!!\n", __func__);
			ret = -ENOMEM;
			goto err_out;
		}

		ret = of_property_read_u32_array(node, gpio_state_default_str, (u32 *)gpio_state, (size_t)(long)count);
		if (ret < 0) {
			hwlog_err("%s: switch_ctl get gpio_state_default failed!!!\n", __func__);
			ret = -EFAULT;
			goto err_out;
		}

		for (i = 0; i < (int)pakit_priv->switch_num; i++) {
			ctl[i].state = gpio_state[i];
		}

		kfree(gpio_state);
		gpio_state = NULL;
	} else {
		hwlog_info("%s: gpio_state_default prop not existed, skip!!!\n", __func__);
	}

	for (i = 0; i < (int)pakit_priv->switch_num; i++) {
		hwlog_info("%s: gpio(%d/%d), state(%d)\n", __func__, i, ctl[i].gpio, ctl[i].state);
		if (gpio_request((unsigned)ctl[i].gpio, ctl[i].name) < 0) {
			hwlog_err("%s: gpio_request switch_ctl[%d].gpio=%d failed!!!\n", __func__, i, ctl[i].gpio);
			ctl[i].gpio = 0;
			ret = -EFAULT;
			goto err_out;
		}

		// set gpio default state
		if (ctl[i].state >= 0) {
			gpio_direction_output((unsigned)ctl[i].gpio, ctl[i].state);
		}
	}

	pakit_priv->switch_ctl = ctl;
	return 0;

err_out:
	if (gpio_state != NULL) {
		kfree(gpio_state);
		gpio_state = NULL;
	}

	if (ctl != NULL) {
		for (i = 0; i < (int)pakit_priv->switch_num; i++) {
			if (ctl[i].gpio > 0) {
				gpio_free((unsigned)ctl[i].gpio);
				ctl[i].gpio = 0;
			}
		}

		kfree(ctl);
		ctl = NULL;
	}

	return ret;
}

static void smartpakit_simple_pa_power_off(struct platform_device *pdev)
{
	smartpakit_priv_t *pakit_priv = NULL;
	smartpakit_switch_node_t *ctl = NULL;
	int i = 0;

	if (NULL == pdev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	pakit_priv = (smartpakit_priv_t *)platform_get_drvdata(pdev);
	if ((NULL == pakit_priv)||(NULL == pakit_priv->switch_ctl)||(pakit_priv->algo_in != SMARTPAKIT_ALGO_IN_SIMPLE)) {
		hwlog_err("%s: pakit_priv invalid argument!!!\n", __func__);
		return;
	}
	ctl = pakit_priv->switch_ctl;

	for (i = 0; i < (int)pakit_priv->switch_num; i++) {
		// set gpio default state
		gpio_direction_output((unsigned)ctl[i].gpio, ctl[i].state);
		mdelay(1);// For AW8737 shutdown sequence
	}
	return;
}

static int smartpakit_parse_dt_info(struct platform_device *pdev, smartpakit_priv_t *pakit_priv)
{
	const char *soc_platform_str = "soc_platform";
	const char *algo_in_str      = "algo_in";
	const char *algo_delay_str   = "algo_delay";
	const char *out_device_str   = "out_device";
	const char *chip_vendor_str  = "chip_vendor";
	const char *chip_model_str   = "chip_model";
	const char *two_in_one_str   = "two_in_one";

	struct device *dev = NULL;
	u32 out_device[SMARTPAKIT_PA_ID_MAX] = { 0 };
	int count = 0;
	int ret = 0;
	int i = 0;

	if ((NULL == pdev) || (NULL == pakit_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	dev = &pdev->dev;

	ret = of_property_read_u32(dev->of_node, soc_platform_str, &pakit_priv->soc_platform);
	if ((ret < 0) || (pakit_priv->soc_platform >= SMARTPAKIT_SOC_PLATFORM_MAX)) {
		hwlog_err("%s: get soc_platform failed(%d) or soc_platform %d>=%d!!!\n", __func__,
			ret, pakit_priv->soc_platform, SMARTPAKIT_SOC_PLATFORM_MAX);
		ret = -EFAULT;
		goto err_out;
	}

	ret = of_property_read_u32(dev->of_node, algo_in_str, &pakit_priv->algo_in);
	if ((ret < 0) || (pakit_priv->algo_in >= SMARTPAKIT_ALGO_IN_MAX)) {
		hwlog_err("%s: get pakit_priv->algo_in from dts failed %d,%d!!!\n", __func__, ret, pakit_priv->algo_in);
		ret = -EFAULT;
		goto err_out;
	}

	if (of_property_read_bool(dev->of_node, algo_delay_str)) {
		ret = of_property_read_u32(dev->of_node, algo_delay_str, &pakit_priv->algo_delay_time);
		if (ret < 0) {
			hwlog_err("%s: get pakit_priv->algo_delay from dts failed %d!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
	} else {
		hwlog_debug("%s: algo_delay prop not existed, skip!!!\n", __func__);
		pakit_priv->algo_delay_time = 0;
	}

	// vendor: for smartpa with dsp + plugin
	if (of_property_read_bool(dev->of_node, chip_vendor_str)) {
		ret = of_property_read_u32(dev->of_node, chip_vendor_str, &pakit_priv->chip_vendor);
		if ((ret < 0) || (pakit_priv->chip_vendor >= SMARTPAKIT_CHIP_VENDOR_MAX)) {
			hwlog_err("%s: get chip_vendor from dts failed %d,%d!!!\n", __func__, ret, pakit_priv->chip_vendor);
			ret = -EFAULT;
			goto err_out;
		}
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: chip_vendor=%d\n", __func__, pakit_priv->chip_vendor);
#endif
	} else {
		hwlog_debug("%s: chip_vendor prop not existed, skip!!!\n", __func__);
	}

	if (of_property_read_bool(dev->of_node, two_in_one_str)) {
		ret = of_property_read_u32(dev->of_node, two_in_one_str, &pakit_priv->two_in_one);
		if (ret < 0) {
			hwlog_err("%s: get pakit_priv->two_in_one from dts failed %d!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
	} else {
		hwlog_debug("%s: two_in_one prop not existed, skip!!!\n", __func__);
		pakit_priv->two_in_one = 0;
	}

	// model: for simple pa or smartpa with dsp + plugin
	if (of_property_read_bool(dev->of_node, chip_model_str)) {
		ret = of_property_read_string(dev->of_node, chip_model_str, &pakit_priv->chip_model);
		if (ret < 0) {
			hwlog_err("%s: get chip_model from dts failed %d!!!\n", __func__, ret);
			ret = -EFAULT;
			goto err_out;
		}
#ifndef CONFIG_FINAL_RELEASE
		hwlog_info("%s: chip_model=%s\n", __func__, pakit_priv->chip_model);
#endif
	} else {
		hwlog_debug("%s: chip_model prop not existed, skip!!!\n", __func__);
	}

	// rec or spk device
	count = of_property_count_elems_of_size(dev->of_node, out_device_str, (int)sizeof(u32));
	if ((count <= 0) || (count > SMARTPAKIT_PA_ID_MAX)) {
		hwlog_err("%s: get pa_num failed(%d) or pa_num %d>%d!!!\n", __func__, count, count, SMARTPAKIT_PA_ID_MAX);
		ret = -EFAULT;
		goto err_out;
	}
	pakit_priv->pa_num = (unsigned int)count;

	ret = of_property_read_u32_array(dev->of_node, out_device_str, out_device, (size_t)(long)count);
	if (ret < 0) {
		hwlog_err("%s: get out_device from dts failed %d!!!\n", __func__, ret);
		ret = -EFAULT;
		goto err_out;
	}

	pakit_priv->out_device = 0;
	for (i = 0; i < count; i++) {
		if (out_device[i] >= SMARTPAKIT_OUT_DEVICE_MAX) {
			hwlog_err("%s: out_device error %d>%d!!!\n", __func__, out_device[i], SMARTPAKIT_OUT_DEVICE_MAX);
			ret = -EFAULT;
			goto err_out;
		}

		pakit_priv->out_device |= out_device[i] << (i * SMARTPAKIT_PA_OUT_DEVICE_SHIFT);
	}
#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: pa_num(%d), out_device(0x%04x).\n", __func__, pakit_priv->pa_num, pakit_priv->out_device);
#endif
	return 0;

err_out:
	return ret;
}

static int smartpakit_parse_dt_misc_rw_permission(struct platform_device *pdev, smartpakit_priv_t *pakit_priv)
{
	const char *misc_rw_permission_str  = "misc_rw_permission_enable";
	const char *i2c_use_pseudo_addr_str = "misc_i2c_use_pseudo_addr";
	struct device *dev = NULL;

	if ((NULL == pdev) || (NULL == pakit_priv)) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	dev = &pdev->dev;

	if (of_property_read_bool(dev->of_node, misc_rw_permission_str)) {
		pakit_priv->misc_rw_permission_enable = true;
		hwlog_info("%s: misc_rw_permission_enable prop existed!\n", __func__);
	} else {
		hwlog_debug("%s: misc_rw_permission_enable prop not existed!\n", __func__);
	}

#ifdef SMARTPAKIT_MISC_RW_PERMISSION_ENABLE
	pakit_priv->misc_rw_permission_enable = true;
	hwlog_info("%s: config SMARTPAKIT_MISC_RW_PERMISSION_ENABLE in eng build!\n", __func__);
#endif

	if (of_property_read_bool(dev->of_node, i2c_use_pseudo_addr_str)) {
		pakit_priv->misc_i2c_use_pseudo_addr = true;
		hwlog_info("%s: misc_i2c_use_pseudo_addr prop existed!\n", __func__);
	} else {
		hwlog_debug("%s: misc_i2c_use_pseudo_addr prop not existed!\n", __func__);
	}

	return 0;
}

static void smartpakit_free(smartpakit_priv_t *pakit_priv)
{
	int i = 0;

	if (pakit_priv == NULL) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return;
	}

	if (pakit_priv->switch_ctl != NULL) {
		for (i = 0; i < (int)pakit_priv->switch_num; i++) {
			if (pakit_priv->switch_ctl[i].gpio > 0) {
				gpio_free((unsigned)pakit_priv->switch_ctl[i].gpio);
				pakit_priv->switch_ctl[i].gpio = 0;
			}
		}

		kfree(pakit_priv->switch_ctl);
		pakit_priv->switch_ctl = NULL;
	}

	kfree(pakit_priv);
}

static int smartpakit_probe(struct platform_device *pdev)
{
	smartpakit_priv_t *pakit_priv = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;

#ifndef CONFIG_FINAL_RELEASE
	hwlog_info("%s: enter ...\n", __func__);
#endif

	if (NULL == pdev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	pakit_priv = kzalloc(sizeof(smartpakit_priv_t), GFP_KERNEL);
	if (NULL == pakit_priv) {
		hwlog_err("%s: kzalloc smartpakit_priv failed!!!\n", __func__);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, pakit_priv);

	pakit_priv->chip_vendor = 0;
	pakit_priv->resume_sequence_permission_enable = false;
	pakit_priv->misc_rw_permission_enable = false;
	pakit_priv->misc_i2c_use_pseudo_addr = false;
	pakit_priv->current_i2c_client = NULL;
	pakit_priv->force_refresh_chip = false;

	memset(pakit_priv->i2c_addr_to_pa_index, SMARTPAKIT_INVALID_PA_INDEX,
		sizeof(unsigned char) * SMARTPAKIT_I2C_ADDR_ARRAY_MAX);
	for (i = 0; i < SMARTPAKIT_PA_ID_MAX; i++) {
		len = (strlen(SMARTPAKIT_NAME_INVALID) < SMARTPAKIT_NAME_MAX) ? strlen(SMARTPAKIT_NAME_INVALID) : (SMARTPAKIT_NAME_MAX - 1);
		(void)strncpy(pakit_priv->chip_model_list[i], SMARTPAKIT_NAME_INVALID, len);
		pakit_priv->chip_model_list[i][len] = '\0';
	}

	ret  = smartpakit_parse_dt_info(pdev, pakit_priv);
	ret += smartpakit_parse_dt_switch_ctl(pdev, pakit_priv);
	ret += smartpakit_parse_dt_misc_rw_permission(pdev, pakit_priv);
	if (ret < 0) {
		hwlog_err("%s: parse dt_info or switch_ctl failed(%d)!!!\n", __func__, ret);
		goto err_out;
	}

	// init ops lock
	mutex_init(&pakit_priv->irq_handler_lock);
	mutex_init(&pakit_priv->hw_reset_lock);
	mutex_init(&pakit_priv->dump_regs_lock);
	mutex_init(&pakit_priv->i2c_ops_lock);

	ret = misc_register(&smartpakit_ctrl_miscdev);
	if (0 != ret) {
		hwlog_err("%s: register miscdev failed(%d)!!!\n", __func__, ret);
		goto err_out;
	}

	smartpakit_priv = pakit_priv;
	smartpakit_init_flag = 1;
	hwlog_info("%s: end sucess!!!\n", __func__);
	return 0;

err_out:
	if (pakit_priv != NULL) {
		smartpakit_free(pakit_priv);
		platform_set_drvdata(pdev, NULL);
	}

	return ret;
}

static int smartpakit_remove(struct platform_device *pdev)
{
	smartpakit_priv_t *pakit_priv = NULL;
	int i = 0;

	if (NULL == pdev) {
		hwlog_err("%s: invalid argument!!!\n", __func__);
		return -EINVAL;
	}

	pakit_priv = (smartpakit_priv_t *)platform_get_drvdata(pdev);
	if (NULL == pakit_priv) {
		hwlog_err("%s: pakit_priv invalid argument!!!\n", __func__);
		return -EINVAL;
	}
	platform_set_drvdata(pdev, NULL);

	SMARTPAKIT_KFREE_OPS(pakit_priv->resume_sequence.node);
	for (i = 0; i < SMARTPAKIT_PA_ID_MAX; i++) {
		smartpakit_reset_poweron_regs(&pakit_priv->poweron_sequence[i]);
	}

	smartpakit_free(pakit_priv);
	smartpakit_priv = NULL;

	misc_deregister(&smartpakit_ctrl_miscdev);
	return 0;
}
static void smartpakit_shutdown(struct platform_device *pdev)
{
	smartpakit_simple_pa_power_off(pdev);
}
/*lint -e528*/
static const struct of_device_id smartpakit_match[] = {
	{ .compatible = "huawei,smartpakit", },
	{},
};
MODULE_DEVICE_TABLE(of, smartpakit_match);

static struct platform_driver smartpakit_driver = {
	.driver = {
		.name = "smartpakit",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(smartpakit_match),
	},
	.probe  = smartpakit_probe,
	.remove = smartpakit_remove,
	.shutdown = smartpakit_shutdown,
};

static int __init smartpakit_init(void)
{
	int ret = 0;

#ifdef CONFIG_HUAWEI_SMARTPAKIT_AUDIO_MODULE
	hwlog_info("%s enter from modprobe.\n", __func__);
#else
	hwlog_info("%s: platform_driver_register ...\n", __func__);
#endif
	ret = platform_driver_register(&smartpakit_driver);
	if (ret) {
		hwlog_err("%s: platform_driver_register failed(%d)!!!\n", __func__, ret);
	}

	return ret;
}

static void __exit smartpakit_exit(void)
{
	platform_driver_unregister(&smartpakit_driver);
}
/*lint +e438 +e838*/

late_initcall(smartpakit_init);
module_exit(smartpakit_exit);

/*lint -e753*/
MODULE_DESCRIPTION("smartpakit driver");
MODULE_AUTHOR("wangping<wangping48@huawei.com>");
MODULE_LICENSE("GPL");

