/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
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
** File:
**     tas2560-misc.c
**
** Description:
**     misc driver for Texas Instruments TAS2560 High Performance 4W Smart Amplifier
**
** =============================================================================
*/


#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include "tas2560.h"
#include "tas2560_core.h"
#include "tas2560_misc.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

static int g_logEnable = 1;
static struct tas2560_priv *g_tas2560 = NULL;
/*lint -e845 -e747 -e64 -e30 -e785 -e438 -e712 -e550 -e50 -e529*/
/*lint -e774 -e778 -e838 -e753 -e750 -e530 -e1564 -e142 -e613*/
static int tas2560_file_open(struct inode *inode, struct file *file)
{
	struct tas2560_priv *pTAS2560 = g_tas2560;
	UNUSED(inode);

	if (!try_module_get(THIS_MODULE)) return -ENODEV;

	file->private_data = (void*)pTAS2560;

	if(g_logEnable) dev_info(pTAS2560->dev,
				"%s\n", __FUNCTION__);
	return 0;
}

static int tas2560_file_release(struct inode *inode, struct file *file)
{
	struct tas2560_priv *pTAS2560 = (struct tas2560_priv *)file->private_data;
	UNUSED(inode);

	if(g_logEnable) dev_info(pTAS2560->dev,
				"%s\n", __FUNCTION__);

	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);

	return 0;
}

static ssize_t tas2560_file_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned char *p_kBuf = NULL;
	unsigned int nValue = 0;
	unsigned char value = 0;
	UNUSED(ppos);

	if(NULL == file||NULL == file->private_data||NULL == buf){
		return -1;
	}
	pTAS2560 = (struct tas2560_priv *)file->private_data;

	mutex_lock(&pTAS2560->file_lock);

	switch(pTAS2560->mnDBGCmd)
	{
		case TIAUDIO_CMD_REG_READ:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
				"TIAUDIO_CMD_REG_READ: current_reg = 0x%x, count=%d\n", pTAS2560->mnCurrentReg, (int)count);
			if(count == 1){
				ret = pTAS2560->read(pTAS2560, pTAS2560->mnCurrentReg, &nValue, LOG_ENABLE);
				if( 0 > ret) {
					dev_err(pTAS2560->dev, "dev read fail %d\n", ret);
					break;
				}

				value = (u8)nValue;
				if(g_logEnable) dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_READ: nValue=0x%x, value=0x%x\n",
					nValue, value);
				ret = copy_to_user(buf, &value, 1);
				if (0 != ret) {
					/* Failed to copy all the data, exit */
					dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
				}
			}else if(count > 1){
				p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
				if(p_kBuf != NULL){
					ret = pTAS2560->bulk_read(pTAS2560, pTAS2560->mnCurrentReg, p_kBuf, count);
					if( 0 > ret) {
						dev_err(pTAS2560->dev, "dev bulk read fail %d\n", ret);
					}else{
						ret = copy_to_user(buf, p_kBuf, count);
						if (0 != ret) {
							/* Failed to copy all the data, exit */
							dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
						}
					}

					kfree(p_kBuf);
				}else{
					dev_err(pTAS2560->dev, "read no mem\n");
				}
			}
		}
		break;

		case TIAUDIO_CMD_SAMPLERATE:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
						"TIAUDIO_CMD_SAMPLERATE: count = %d\n",
						(int)count);
			if(count == 4){
				p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
				if(p_kBuf != NULL){
					p_kBuf[0] = (pTAS2560->mnSamplingRate&0x000000ff);
					p_kBuf[1] = ((pTAS2560->mnSamplingRate&0x0000ff00)>>8);
					p_kBuf[2] = ((pTAS2560->mnSamplingRate&0x00ff0000)>>16);
					p_kBuf[3] = ((pTAS2560->mnSamplingRate&0xff000000)>>24);

					ret = copy_to_user(buf, p_kBuf, count);
					if (0 != ret) {
						/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}

					kfree(p_kBuf);
				}else{
					dev_err(pTAS2560->dev, "read no mem\n");
				}
			}
		}
		break;

		case TIAUDIO_CMD_BITRATE:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
						"TIAUDIO_CMD_BITRATE: count = %d\n",
						(int)count);

			if(count == 1){
				int bitRate = tas2560_get_bit_rate(pTAS2560);
				if(bitRate >=0){
					ret = copy_to_user(buf, &bitRate, 1);
					if (0 != ret) {
					/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}
				}
			}
		}
		break;

		case TIAUDIO_CMD_DACVOLUME:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
						"TIAUDIO_CMD_DACVOLUME: count = %d\n",
						(int)count);

			if(count == 1){
				int volume = tas2560_get_volume(pTAS2560);
				if(volume >=0){
					ret = copy_to_user(buf, &volume, 1);
					if (0 != ret) {
					/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}
				}
			}
		}
		break;

		default:
			dev_err(pTAS2560->dev,"%s cmd is not supported\n",__func__);
		break;
	}

	 pTAS2560->mnDBGCmd = 0;

	mutex_unlock(&pTAS2560->file_lock);
	return (ssize_t)count;
}

static ssize_t tas2560_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned char *p_kBuf = NULL;
	unsigned int reg = 0;
	unsigned int len = 0;
	UNUSED(ppos);

	if(NULL == file||NULL == file->private_data){
		return -1;
	}
	pTAS2560 = (struct tas2560_priv *)file->private_data;

	mutex_lock(&pTAS2560->file_lock);

	if ((0 == count) || (NULL == buf)) {
		ret = -1;
		goto err;
	}
	p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
	if(p_kBuf == NULL) {
		dev_err(pTAS2560->dev, "write no mem\n");
		ret = -1;
		goto err;
	}

	ret = copy_from_user(p_kBuf, buf, count);
	if (0 != ret) {
		dev_err(pTAS2560->dev,"copy_from_user failed.\n");
		goto err;
	}

	pTAS2560->mnDBGCmd = p_kBuf[0];
	switch(pTAS2560->mnDBGCmd)
	{
		case TIAUDIO_CMD_REG_WITE:
		if(count > 5){
			reg = ((unsigned int)p_kBuf[1] << 24) +
				((unsigned int)p_kBuf[2] << 16) +
				((unsigned int)p_kBuf[3] << 8) +
				(unsigned int)p_kBuf[4];
			len = count - 5;
			if(len == 1){
				ret = pTAS2560->write(pTAS2560, reg, p_kBuf[5]);
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_WITE, Reg=0x%x, Val=0x%x\n",
					reg, p_kBuf[5]);
			}else{
				ret = pTAS2560->bulk_write(pTAS2560, reg, &p_kBuf[5], len);
			}
		}else{
			dev_err(pTAS2560->dev,"%s, write len fail, count=%d.\n",
				__FUNCTION__, (int)count);
		}
		pTAS2560->mnDBGCmd = 0;
		break;

		case TIAUDIO_CMD_REG_READ:
		if(count == 5){
			pTAS2560->mnCurrentReg = ((unsigned int)p_kBuf[1] << 24) +
				((unsigned int)p_kBuf[2] << 16) +
				((unsigned int)p_kBuf[3] << 8) +
				(unsigned int)p_kBuf[4];
			if(g_logEnable){
				dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_READ, whole=0x%x\n",
					pTAS2560->mnCurrentReg);
			}
		}else{
			dev_err(pTAS2560->dev,"read len fail.\n");
		}
		break;

		case TIAUDIO_CMD_DEBUG_ON:
		{
			if(count == 2){
				g_logEnable = p_kBuf[1];
			}
			pTAS2560->mnDBGCmd = 0;
		}
		break;

		case TIAUDIO_CMD_SAMPLERATE:
		{
			if(count == 5){
				unsigned int nSampleRate = ((unsigned int)p_kBuf[1] << 24) +
					((unsigned int)p_kBuf[2] << 16) +
					((unsigned int)p_kBuf[3] << 8)  +
					(unsigned int)p_kBuf[4];
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_SAMPLERATE, set to %d\n",
					nSampleRate);

				tas2560_set_SampleRate(pTAS2560, nSampleRate);
			}
		}
		break;

		case TIAUDIO_CMD_BITRATE:
		{
			if(count == 2){
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_BITRATE, set to %d\n",
					p_kBuf[1]);

				tas2560_set_bit_rate(pTAS2560, p_kBuf[1]);
			}
		}
		break;

		case TIAUDIO_CMD_DACVOLUME:
		{
			if(count == 2){
				unsigned char volume = (p_kBuf[1] & 0x0f);
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_DACVOLUME, set to %d\n",
					volume);

				tas2560_set_volume(pTAS2560, volume);
			}
		}
		break;

		case TIAUDIO_CMD_SPEAKER:
		{
			if(count == 2){
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_SPEAKER, set to %d\n",
					p_kBuf[1]);
				tas2560_enable(pTAS2560, (p_kBuf[1]>0));
			}
		}
		break;

		default:
			pTAS2560->mnDBGCmd = 0;
		break;
	}

err:
	if(p_kBuf != NULL)
		kfree(p_kBuf);

	mutex_unlock(&pTAS2560->file_lock);

	return ret;
}

static long tas2560_file_do_ioctl(struct file *file, unsigned int cmd, void __user *p, unsigned long compat_mode)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int value = 0;
	unsigned char volume = 0;
	unsigned int __user *arg = (unsigned int __user *) p;
	char *dmdreport_content = NULL;
	int ret = 0;
	UNUSED(compat_mode);

	if(NULL == file||NULL == file->private_data){
		return -1;
	}
	pTAS2560 = (struct tas2560_priv *)file->private_data;

	dev_info(pTAS2560->dev,"%s cmd id: 0x%x\n",__func__,cmd);

	mutex_lock(&pTAS2560->file_lock);

	switch (cmd) {
		case SMARTPA_SPK_DAC_VOLUME:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_DAC_VOLUME called\n",__func__);
			ret = get_user(value,arg);
			volume = (unsigned char)(value & 0x0f);
			tas2560_set_volume(pTAS2560, volume);
		}
		break;

		case SMARTPA_SPK_MUTE:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_MUTE called\n",__func__);
			tas2560_mute(pTAS2560,0x1);
		}
		break;

		case SMARTPA_SPK_UNMUTE:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_UNMUTE called\n",__func__);
			tas2560_mute(pTAS2560,0x0);
		}
		break;

		case SMARTPA_SPK_FADEIN:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_FADEIN called\n",__func__);
			ret = get_user(value,arg);
			tas2560_fadeIn_fadeout(pTAS2560, 0, (int)value);
		}
		break;

		case SMARTPA_SPK_FADEOUT:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_FADEOUT called\n",__func__);
			ret = get_user(value,arg);
			tas2560_fadeIn_fadeout(pTAS2560, 1, (int)value);
		}
		break;

		case SMARTPA_SPK_POWER_ON:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_POWER_ON called\n",__func__);
			ret = tas2560_enable(pTAS2560, true);
			if(ret){
				dmdreport_content = "tas2560 ioctl power on failed\n";
			}
		}
		break;

		case SMARTPA_SPK_POWER_OFF:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_POWER_OFF called\n",__func__);
			ret = tas2560_enable(pTAS2560, false);
			if(ret){
				dmdreport_content = "tas2560 ioctl power down failed\n";
			}
		}
		break;

		case SMARTPA_SPK_SET_SAMPLERATE:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_SET_SAMPLERATE called\n",__func__);
			ret = get_user(value,arg);
			tas2560_set_SampleRate(pTAS2560, value);
		}
		break;

		case SMARTPA_SPK_SET_BITRATE:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_SPK_SET_BITRATE called\n",__func__);
			ret = get_user(value,arg);
			tas2560_set_bit_rate(pTAS2560, value);
		}
		break;

		case SMARTPA_DUMP_REGS:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_RESET_DUMP_REGS called\n",__func__);
			tas2560_dump_regs(pTAS2560);
		}
		break;

		case SMARTPA_RESET_CHIP:
		{
			dev_info(pTAS2560->dev,"%s SMARTPA_RESET_CHIP called\n",__func__);
			tas2560_force_reprogram_chip(pTAS2560);
		}
		break;

		default:
			dev_err(pTAS2560->dev,"%s call nothing\n",__func__);
			break;
	}
	mutex_unlock(&pTAS2560->file_lock);
#ifdef CONFIG_HUAWEI_DSM
	if ((0 != ret)&&(NULL != dmdreport_content)) {
		dev_info(pTAS2560->dev,dmdreport_content);
		audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR,dmdreport_content);
	}
#endif
	return ret;
}

static long tas2560_file_unlocked_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	return tas2560_file_do_ioctl(file, command, (void __user *)arg, 0);
}

#ifdef CONFIG_COMPAT
static long tas2560_file_compact_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	return tas2560_file_do_ioctl(file, command, compat_ptr(arg), 1);
}
#else
#define tas2560_file_compact_ioctl NULL
#endif

static struct file_operations fops =
{
	.owner = THIS_MODULE,
#ifdef TAS2560_DEBUG
	.read = tas2560_file_read,
	.write = tas2560_file_write,
#endif
	.unlocked_ioctl = tas2560_file_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl    = tas2560_file_compact_ioctl,
#endif
	.open = tas2560_file_open,
	.release = tas2560_file_release,
};

#define MODULE_NAME	"tas2560"
static struct miscdevice tas2560_misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODULE_NAME,
	.fops = &fops,
};

int tas2560_register_misc(struct tas2560_priv * pTAS2560)
{
	int ret = 0;

	g_tas2560 = pTAS2560;

	ret = misc_register(&tas2560_misc);
	if (ret) {
		dev_err(pTAS2560->dev, "TAS2560 misc fail: %d\n", ret);
	}

    dev_info(pTAS2560->dev, "%s, leave\n", __FUNCTION__);

	return ret;
}

int tas2560_deregister_misc(struct tas2560_priv * pTAS2560)
{
	misc_deregister(&tas2560_misc);
	UNUSED(pTAS2560);
	return 0;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 Misc Smart Amplifier driver");
MODULE_LICENSE("GPLv2");

