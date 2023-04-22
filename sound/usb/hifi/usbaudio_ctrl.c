/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#include <linux/slab.h>

#include <sound/control.h>
#include <sound/core.h>
#include <sound/info.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/audio/usb_audio_power.h>
#include <linux/wakelock.h>

#include "usbaudio.h"
#include "card.h"
#include "helper.h"
#include "format.h"
#include "clock.h"
#include "usbaudio_dsp_client.h"
#include "usbaudio_ioctl.h"
#include <huawei_platform/log/imonitor.h>
#include <huawei_platform/log/imonitor_keys.h>


#ifdef CLT_AUDIO
#include "usbaudio.h"
#endif

#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#define DTS_USBAUDIO_DSP_NAME "hisilicon,usbaudiodsp"
#define HUAWEI_USB_HEADSET_PRENAME "HUAWEI USB-C"
#define BBIITT_USB_HEADSET_PRENAME "BBIITT USB-C"
#define USB_AUDIO_TYPEC_INFO_EVENT_ID 931001000

static unsigned int insert_times;
static DEFINE_MUTEX(connect_mutex);
static DEFINE_MUTEX(usbaudio_wakeup_mutex);

struct usbaudio_dsp_client_ops {
	bool (*controller_switch)(struct usb_device *dev, u32 usb_id, struct usb_host_interface *ctrl_intf, int ctrlif, struct usbaudio_pcms *pcms);
	int (*disconnect)(struct snd_usb_audio *chip, unsigned int dsp_reset_flag);
	int (*set_pipeout_interface)(struct snd_usb_audio *chip, struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate);
	int (*set_pipein_interface)(struct snd_usb_audio *chip, struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate);
	bool (*controller_query)(struct snd_usb_audio *chip);
	bool (*send_usbaudioinfo2hifi)(struct snd_usb_audio *chip, struct usbaudio_pcms *pcms);
	void (*setinterface_complete)(unsigned int dir, unsigned int running, int ret, unsigned int rate);
	void (*nv_check_complete)(unsigned int usb_id);
	int (*usb_resume)(void);
};

struct usbaudio_dsp  {
	struct snd_usb_audio *chip;
	struct usbaudio_pcms pcms;
	struct usbaudio_dsp_client_ops *ops;
	unsigned int pipeout_running_flag;
	unsigned int pipein_running_flag;
	unsigned int dsp_reset_flag;
	struct usbaudio_info info;
	unsigned int usb_id;
	bool usbaudio_connected;
	unsigned int dn_rate;
	unsigned int up_rate;
	bool wake_up;
};

static struct usbaudio_dsp_client_ops client_ops = {
	.controller_switch = controller_switch,
	.disconnect = disconnect,
	.set_pipeout_interface = set_pipeout_interface,
	.set_pipein_interface = set_pipein_interface,
	.controller_query = controller_query,
	.setinterface_complete = setinterface_complete,
	.nv_check_complete = usbaudio_nv_check_complete,
	.usb_resume = usb_power_resume,
};

struct completion usbaudio_resume_complete;
#define USBAUDIO_WAKE_UP_TIMEOUT (5 * HZ)
static struct usbaudio_dsp *usbaudio_hifi = NULL;

void usbaudio_resume(void)
{
	unsigned long ret;
	mutex_lock(&usbaudio_wakeup_mutex);
	if (!usbaudio_hifi->wake_up) {
		reinit_completion(&usbaudio_resume_complete);
		mutex_unlock(&usbaudio_wakeup_mutex);
		ret = wait_for_completion_timeout(&usbaudio_resume_complete, USBAUDIO_WAKE_UP_TIMEOUT);
		if (ret == 0) {
			pr_err("usbaudio resume timeout\n");
		}
	} else {
		mutex_unlock(&usbaudio_wakeup_mutex);
	}
}

void usbaudio_ctrl_wake_up(bool wake_up)
{
	mutex_lock(&usbaudio_wakeup_mutex);
	if (usbaudio_hifi) {
		usbaudio_hifi->wake_up = wake_up;
		if (usbaudio_hifi->wake_up)
			complete(&usbaudio_resume_complete);
	}
	mutex_unlock(&usbaudio_wakeup_mutex);
}

bool usbaudio_ctrl_controller_switch(struct usb_device *dev, u32 usb_id, struct usb_host_interface *ctrl_intf, int ctrlif)
{
	bool ret;
	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi) {
		ret = false;
	} else {
		usbaudio_hifi->pipeout_running_flag = STOP_STREAM;
		usbaudio_hifi->pipein_running_flag = STOP_STREAM;
		usbaudio_hifi->usb_id = usb_id;
		usbaudio_hifi->usbaudio_connected = true;
		ret = usbaudio_hifi->ops->controller_switch(dev, usb_id, ctrl_intf, ctrlif, &usbaudio_hifi->pcms);
	}
	mutex_unlock(&connect_mutex);

	return ret;
}

void usbaudio_ctrl_hifi_reset_inform(void)
{
	if (usbaudio_hifi && usbaudio_hifi->chip) {
		usbaudio_hifi->dsp_reset_flag = USBAUDIO_DSP_ABNORMAL;
	}
}

int usbaudio_ctrl_disconnect(void)
{
	int ret;

	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi || !usbaudio_hifi->chip) {
		ret = 0;
	} else {
		usbaudio_hifi->usbaudio_connected = false;
		ret = usbaudio_hifi->ops->disconnect(usbaudio_hifi->chip, usbaudio_hifi->dsp_reset_flag);
		usbaudio_hifi->chip = NULL;
	}
	mutex_unlock(&connect_mutex);

	return ret;
}

void usbaudio_ctrl_nv_check(void)
{
	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi || !usbaudio_hifi->usbaudio_connected) {
		pr_err("usbaudio has been disconnected\n");
	} else {
		usbaudio_hifi->ops->nv_check_complete(usbaudio_hifi->usb_id);
	}
	mutex_unlock(&connect_mutex);
}

int usbaudio_ctrl_usb_resume(void)
{
	int ret;
	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi || !usbaudio_hifi->chip) {
		ret = 0;
	} else {
		ret = usbaudio_hifi->ops->usb_resume();
	}
	mutex_unlock(&connect_mutex);

	return ret;
}

int usbaudio_ctrl_set_pipeout_interface(unsigned int running, unsigned int rate)
{
	int ret;
	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi || !usbaudio_hifi->chip) {
		ret = 0;
	} else {
		if (running == usbaudio_hifi->pipeout_running_flag && usbaudio_hifi->dn_rate == rate) {
			pr_err("usbaudio_ctrl_set_pipeout_interface RETURN. running:%d rate %d \n", running, rate);
			ret = 0;
		} else {
			usbaudio_hifi->pipeout_running_flag = running;
			if (running == STOP_STREAM)
				usbaudio_resume();
			ret = usbaudio_hifi->ops->set_pipeout_interface(usbaudio_hifi->chip, &usbaudio_hifi->pcms, running, rate);
			usbaudio_hifi->dn_rate = rate;
		}
		usbaudio_hifi->ops->setinterface_complete(SNDRV_PCM_STREAM_PLAYBACK, running, ret, rate);
	}
	mutex_unlock(&connect_mutex);

	return ret;
}

int usbaudio_ctrl_set_pipein_interface(unsigned int running, unsigned int rate)
{
	int ret;
	mutex_lock(&connect_mutex);
	if (!usbaudio_hifi || !usbaudio_hifi->chip) {
		ret = 0;
	} else {
		if (running == usbaudio_hifi->pipein_running_flag && usbaudio_hifi->up_rate == rate) {
			pr_err("usbaudio_ctrl_set_pipein_interface RETURN. running:%d rate %d \n", running, rate);
			ret = 0;
		} else {
			usbaudio_hifi->pipein_running_flag = running;
			if (running == STOP_STREAM)
				usbaudio_resume();
			ret = usbaudio_hifi->ops->set_pipein_interface(usbaudio_hifi->chip, &usbaudio_hifi->pcms, running, rate);
			usbaudio_hifi->up_rate = rate;
		}
		usbaudio_hifi->ops->setinterface_complete(SNDRV_PCM_STREAM_CAPTURE, running, ret, rate);
	}
	mutex_unlock(&connect_mutex);

	return ret;
}

void usbaudio_ctrl_query_info(struct usbaudio_info *usbinfo)
{
	mutex_lock(&connect_mutex);
#ifdef CLT_AUDIO
	usbinfo->controller_location = usbaudio_test_get_controller_state();
	pr_err("controller state %d \n", usbinfo->controller_location);
#else
	if (!usbaudio_hifi || !usbaudio_hifi->chip) {
		usbinfo->controller_location = ACPU_CONTROL;
	} else {
		if(usbaudio_hifi->ops->controller_query(usbaudio_hifi->chip)) {
			usbinfo->controller_location = DSP_CONTROL;
		} else {
			usbinfo->controller_location = ACPU_CONTROL;
		}
	}
#endif

	if (usbaudio_hifi && usbaudio_hifi->chip) {
		usbinfo->usbid = usbaudio_hifi->info.usbid;
		usbinfo->dnlink_channels = usbaudio_hifi->info.dnlink_channels;
		usbinfo->uplink_channels = usbaudio_hifi->info.uplink_channels;
		memcpy(usbinfo->name, usbaudio_hifi->info.name, sizeof(usbinfo->name)); /* unsafe_function_ignore: memcpy */
		memcpy(usbinfo->dnlink_rate_table, usbaudio_hifi->info.dnlink_rate_table, sizeof(usbinfo->dnlink_rate_table)); /* unsafe_function_ignore: memcpy */
		pr_info("usbid 0x%x, dnlink_channels %d, uplink_channels %d \n",
			usbinfo->usbid,
			usbinfo->dnlink_channels,
			usbinfo->uplink_channels);
		pr_info("usbname %s \n", usbinfo->name);
	}
#ifdef CLT_AUDIO
	memcpy(usbinfo->dnlink_rate_table, usbaudio_test_get_rate_table(), sizeof(usbinfo->dnlink_rate_table));// unsafe_function_ignore: memcpy
	usbinfo->usbid = usbaudio_test_get_usb_id();
	pr_info("usb id is :0x%x, dlink_rate_table: %d,%d,%d,%d,%d\n", usbinfo->usbid, usbinfo->dnlink_rate_table[0],
		usbinfo->dnlink_rate_table[1], usbinfo->dnlink_rate_table[2], usbinfo->dnlink_rate_table[3], usbinfo->dnlink_rate_table[4]);
#endif
	mutex_unlock(&connect_mutex);
}

static int usbaudio_ctrl_typeC_log_upload(void *data)
{
	struct snd_usb_audio *info = NULL;
	struct imonitor_eventobj *obj = NULL;
	int ret = -1;
	if(!data){
		pr_err("headset info is null!\n");
		return ret;
	}
	info = (struct snd_usb_audio *)data;
	insert_times++;
	pr_info("typeC headset bcdDevice %0x, times %d\n",info->dev->descriptor.bcdDevice, insert_times);

	obj = imonitor_create_eventobj(USB_AUDIO_TYPEC_INFO_EVENT_ID);
	if(!obj){
		pr_err("imonitor create eventobj error\n");
		return ret;
	}
	imonitor_set_param(obj, E931001000_USBID_INT, (long)info->usb_id);
	imonitor_set_param(obj, E931001000_IC_VARCHAR, (long)info->dev->manufacturer);
	imonitor_set_param(obj, E931001000_MODULE_VARCHAR, (long)info->card->shortname);
	imonitor_set_param(obj, E931001000_ISN_VARCHAR, (long)info->dev->serial);
	imonitor_set_param(obj, E931001000_TIMES_INT, (long)insert_times);
	imonitor_set_param(obj, E931001000_VER_INT, (long)info->dev->descriptor.bcdDevice);
	ret = imonitor_send_event(obj);
	if(obj){
		imonitor_destroy_eventobj(obj);
	}
	return ret;
}
void usbaudio_ctrl_set_chip(struct snd_usb_audio *chip)
{
	mutex_lock(&connect_mutex);
	if (usbaudio_hifi && chip) {
		usbaudio_hifi->wake_up = true;
		if (chip->card && chip->dev->serial && chip->card->shortname && (sizeof(chip->card->shortname) <= 256)){
			#ifdef CONFIG_HUAWEI_DSM
			if (hisi_usb_using_hifi_usb(chip->dev))
				audio_dsm_report_info(AUDIO_CODEC, DSM_USBAUDIO_INFO, "usbid %x usbphy %s \n", chip->usb_id, "hifi");
			else
				audio_dsm_report_info(AUDIO_CODEC, DSM_USBAUDIO_INFO, "usbid %x usbphy %s \n", chip->usb_id, "arm");
			#endif
			memcpy(usbaudio_hifi->info.name, chip->card->shortname, sizeof(chip->card->shortname)); /* unsafe_function_ignore: memcpy */
			usbaudio_hifi->info.name[USBAUDIO_INFONAME_LEN - 1] = '\0';
		}

		usbaudio_hifi->chip = chip;
		usbaudio_hifi->dsp_reset_flag = USBAUDIO_DSP_NORMAL;
		usbaudio_hifi->info.dnlink_channels = usbaudio_hifi->pcms.fmts[0].channels;
		usbaudio_hifi->info.uplink_channels = usbaudio_hifi->pcms.fmts[1].channels;
		memcpy(usbaudio_hifi->info.dnlink_rate_table, usbaudio_hifi->pcms.fmts[0].rate_table, sizeof(usbaudio_hifi->info.dnlink_rate_table)); /* unsafe_function_ignore: memcpy */
		usbaudio_hifi->info.usbid = chip->usb_id;
		send_usbaudioinfo2hifi(usbaudio_hifi->chip, &usbaudio_hifi->pcms);
	}
	if(usbaudio_ctrl_typeC_log_upload(chip) < 0){
		 pr_err("imonitor send eventobj error\n");
	}
	if (chip) {
		pr_info("usbaudio_ctrl_set_chip: usb id is 0x%x , name is %s\n", chip->usb_id, chip->card->shortname);
		if(hisi_usb_using_hifi_usb(chip->dev)
			&& ((!strncmp(chip->card->shortname, HUAWEI_USB_HEADSET_PRENAME, strlen(HUAWEI_USB_HEADSET_PRENAME)))
				|| (!strncmp(chip->card->shortname, BBIITT_USB_HEADSET_PRENAME, strlen(BBIITT_USB_HEADSET_PRENAME))))) {
			usb_audio_power_buckboost();
		}
	}

	mutex_unlock(&connect_mutex);
}

static int usbaudio_dsp_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	dev_info(dev, "probe begin");
	usbaudio_hifi = devm_kzalloc(dev, sizeof(*usbaudio_hifi), GFP_KERNEL);
	if (!usbaudio_hifi) {
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, usbaudio_hifi);
	usbaudio_hifi->ops = &client_ops;
	usbaudio_hifi->usbaudio_connected = false;
	ret = usbaudio_mailbox_init();
	init_completion(&usbaudio_resume_complete);

	return ret;
}

static int usbaudio_dsp_remove(struct platform_device *pdev)
{
	usbaudio_hifi = NULL;
	usbaudio_mailbox_deinit();

	return 0;
}

static const struct of_device_id usbaudio_dsp_match_table[] = {
	{
		.compatible = DTS_USBAUDIO_DSP_NAME,
		.data = NULL,
	},
	{ /* end */ }
};

static struct platform_driver usbaudio_dsp_driver = {
	.driver =
	{
		.name  = "usbaudio_dsp",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(usbaudio_dsp_match_table),
	},
	.probe	= usbaudio_dsp_probe,
	.remove = usbaudio_dsp_remove,
};

static int __init usbaudio_dsp_init(void)
{
	return platform_driver_register(&usbaudio_dsp_driver);
}

static void __exit usbaudio_dsp_exit(void)
{
	platform_driver_unregister(&usbaudio_dsp_driver);
}

fs_initcall_sync(usbaudio_dsp_init);
module_exit(usbaudio_dsp_exit);

MODULE_DESCRIPTION("hisi usbaudio dsp driver");
MODULE_AUTHOR("guzhengming <guzhengming@hisilicon.com>");
MODULE_LICENSE("GPL");

