/*
 * headset debug driver.
 *
 * Copyright (c) 2018 HuaWei Terminal Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#ifdef CONFIG_HUAWEI_HEADSET_DEBUG_SWITCH
#include <linux/switch.h>
#endif
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include "headset_debug.h"

#ifdef CONFIG_HUAWEI_HEADSET_DEBUG
#define LOG_TAG "headset_debug"
#define INT_HEX_STR_SIZE 5 //16 2+1+1

static struct headset_debug g_headset_debug;

/*lint -e715 */
static ssize_t headset_debug_state_read(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char kn_buf[INT_HEX_STR_SIZE] = {0};
	ssize_t byte_read;

	snprintf(kn_buf, INT_HEX_STR_SIZE, "%d", atomic_read(&g_headset_debug.state)); /*unsafe_function_ignore: snprintf */
	byte_read = simple_read_from_buffer(user_buf, count, ppos, kn_buf, strlen(kn_buf));

	return byte_read;
}

static int switch_to_jack_report(int state)
{
	int jack_report = 0;

	switch (state) {
	case HEADSET_DEBUG_JACK_BIT_NONE:
		jack_report = 0;
		break;
	case HEADSET_DEBUG_JACK_BIT_HEADSET:
		jack_report = SND_JACK_HEADSET;
		break;
	case HEADSET_DEBUG_JACK_BIT_HEADSET_NO_MIC:
		jack_report = SND_JACK_HEADPHONE;
		break;
	case HEADSET_DEBUG_JACK_BIT_HEADPHONE:
		jack_report = SND_JACK_HEADPHONE;
		break;
	case HEADSET_DEBUG_JACK_BIT_LINEOUT:
		jack_report = SND_JACK_LINEOUT;
		break;
	default:
		loge("error hs_status(%d)\n", state);
	}
	return jack_report;
}

void headset_debug_set_state(int state, bool use_input)
{
	if(!use_input) {
		atomic_set(&g_headset_debug.state, state);
		return;
	}

	switch(state) {
		case 0:
			atomic_set(&g_headset_debug.state, HEADSET_DEBUG_JACK_BIT_NONE);
			break;
		case SND_JACK_HEADPHONE:
			atomic_set(&g_headset_debug.state, HEADSET_DEBUG_JACK_BIT_HEADSET_NO_MIC);
			break;
		case SND_JACK_HEADSET:
			atomic_set(&g_headset_debug.state, HEADSET_DEBUG_JACK_BIT_HEADSET);
			break;
		case SND_JACK_LINEOUT:
			atomic_set(&g_headset_debug.state, HEADSET_DEBUG_JACK_BIT_LINEOUT);
			break;
		default:
			loge("state error %d\n", state);
	}
	return;
}
#define HEADSET_DEBUG_ALL_KEY_TYPE \
	SND_JACK_BTN_0|SND_JACK_BTN_1|SND_JACK_BTN_2|SND_JACK_BTN_3|SND_JACK_BTN_4|SND_JACK_BTN_5
#define HEADSET_DEBUG_REPORT_KEY(type) \
	do {\
		snd_soc_jack_report(g_headset_debug.jack, type, HEADSET_DEBUG_ALL_KEY_TYPE);\
		msleep(30);\
		snd_soc_jack_report(g_headset_debug.jack, 0, HEADSET_DEBUG_ALL_KEY_TYPE);\
	}while(0)

static ssize_t headset_debug_state_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char kn_buf[INT_HEX_STR_SIZE] = {0};
	ssize_t byte_writen;
	int status = 0;
	int ret = 0;
	int jack_report = 0;

	byte_writen = simple_write_to_buffer(kn_buf, INT_HEX_STR_SIZE - 1, ppos, user_buf, count); /*lint !e747 */
	if (byte_writen != count) { /*lint !e737 */
		loge("simple_write_to_buffer err:%zd\n", byte_writen);
		return -ENOMEM;
	}

	ret = kstrtouint(kn_buf, 0, &status);
	if(ret) {
		loge("kstrtouint error %d\n", ret);
		return -EINVAL;
	}
	switch(status) {
		case HEADSET_DEBUG_JACK_BIT_NONE:
		case HEADSET_DEBUG_JACK_BIT_HEADSET:
		case HEADSET_DEBUG_JACK_BIT_HEADPHONE:
		case HEADSET_DEBUG_JACK_BIT_HEADSET_NO_MIC:
		case HEADSET_DEBUG_JACK_BIT_PLUGING:
		case HEADSET_DEBUG_JACK_BIT_INVALID:
		case HEADSET_DEBUG_JACK_BIT_LINEOUT:
			atomic_set(&g_headset_debug.state, status);
#ifdef  CONFIG_HUAWEI_HEADSET_DEBUG_SWITCH
			if(g_headset_debug.sdev)
				switch_set_state(g_headset_debug.sdev, status);
#endif
			if(g_headset_debug.jack){
				jack_report = switch_to_jack_report(status);
				snd_soc_jack_report(g_headset_debug.jack, jack_report, SND_JACK_HEADSET);
			}
			break;
		case HEADSET_DEBUG_KEY_0:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_0);
			break;
		case HEADSET_DEBUG_KEY_1:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_1);
			break;
		case HEADSET_DEBUG_KEY_2:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_2);
			break;
		case HEADSET_DEBUG_KEY_3:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_3);
			break;
		case HEADSET_DEBUG_KEY_4:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_4);
			break;
		case HEADSET_DEBUG_KEY_5:
			HEADSET_DEBUG_REPORT_KEY(SND_JACK_BTN_5);
			break;
		default:
			loge("error code %d\n", status);
			break;
	}

	return byte_writen;
}

/*lint -e785 */
static const struct file_operations headset_debug_state_fops = {
	.read  = headset_debug_state_read,
	.write = headset_debug_state_write,
};

void headset_debug_init(struct snd_soc_jack *jack, struct switch_dev *sdev)
{
	g_headset_debug.jack = jack;
	g_headset_debug.sdev = sdev;

	g_headset_debug.df_dir = debugfs_create_dir("headset", NULL);
	if (!g_headset_debug.df_dir) {
		loge("create headset debugfs dir\n");
		return ;
	}

	if (!debugfs_create_file("state", 0640, g_headset_debug.df_dir, NULL, &headset_debug_state_fops)) {
		loge("create headset debugfs rr\n");
		return ;
	}
	atomic_set(&g_headset_debug.state, 0);

	return ;
}

void headset_debug_uninit()
{
	if (g_headset_debug.df_dir) {
		debugfs_remove_recursive(g_headset_debug.df_dir);
		g_headset_debug.df_dir = NULL;
	}
}
#endif
