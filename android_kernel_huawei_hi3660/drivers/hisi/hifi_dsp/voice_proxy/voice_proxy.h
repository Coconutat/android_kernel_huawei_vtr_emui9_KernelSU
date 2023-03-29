/*
 * Hifi_voice_proxy.h - HW voice proxy in kernel, it is used for pass through voice
 * data between AP and hifi.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __VOICE_PROXY_H__
#define __VOICE_PROXY_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/io.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/of.h>

#include <asm/memory.h>
#include <asm/types.h>

#include "drv_mailbox_cfg.h"
#include "audio_hifi.h"
#include "hifi_om.h"

/* The size limit for the in and out parameters of read/write/mailbox*/
#define VOICE_PROXY_LIMIT_PARAM_SIZE (300)
/* QUEUE_SIZE_MUST_GREATER_THAN_SECRET_KEY_NEGOTIATION_SIZE(500) */
#define VOICE_PROXY_QUEUE_SIZE_MAX 600

enum send_tfagent_data_type {
	VOLTE_NONE,
	PROXY_TO_TF_UNDECRYPT_DATA,
	PROXY_TO_TF_UNENCRYPT_DATA,
	TF_TO_PROXY_DECRYPTED_DATA,
	TF_TO_PROXY_ENCRYPTED_DATA,
	VOLTE_MAX,
};

enum voice_modem_no {
    VOICE_MC_MODEM0 = 0,
    VOICE_MC_MODEM1,
    VOICE_MC_MODEM_NUM_BUTT
};

struct send_tfagent_data {
	int32_t data_type;
	uint32_t id;
	int32_t buf_size;
	int8_t data[PROXY_VOICE_CODEC_MAX_DATA_LEN];
};

struct proxy_voice_cnf_cmd_code {
	struct list_head list_node;
	uint16_t msg_id;
	uint16_t modem_no;
	uint32_t channel_id;
};

struct voice_proxy_cmd_node {
	struct list_head list_node;
	uint16_t msg_id;
	uint16_t modem_no;
};

struct voice_proxy_data_buf {
	uint32_t id;
	int32_t size;
	int8_t data[4];
};

struct voice_proxy_data_node {
	struct list_head list_node;
	struct voice_proxy_data_buf list_data;
};

typedef void (*register_mailbox_cb)(mb_msg_cb mail_cb);
typedef int32_t (*read_mailbox_msg_cb)(void *mail_handle, int8_t *buf, int32_t *size);
typedef int32_t (*mailbox_send_msg_cb)(uint32_t mailcode, uint16_t msg_id, void *buf, uint32_t size);

typedef void (*voice_proxy_sign_init_cb)(void);
typedef void (*voice_proxy_msg_cb)(int8_t *rev_buf, uint32_t buf_size);
typedef void (*voice_proxy_cmd_cb)(int8_t *data, uint32_t *size, uint16_t *msg_id);

typedef int32_t (*voice_proxy_add_data_cb)(int8_t *data, uint32_t size);

struct voice_proxy_sign_init {
	voice_proxy_sign_init_cb callback;
};

struct voice_proxy_msg_handle {
	uint16_t msg_id;
	voice_proxy_msg_cb callback;
};

struct voice_proxy_cmd_handle {
	uint16_t msg_id;
	voice_proxy_cmd_cb callback;
};

int64_t voice_proxy_get_timems(void);
int32_t voice_proxy_add_work_queue_cmd(uint16_t msg_id, uint16_t modem_no, uint32_t channel_id);
int32_t voice_proxy_create_data_node(struct voice_proxy_data_node **node, int8_t *data, int32_t size);
void voice_proxy_register_msg_callback(uint16_t msg_id, voice_proxy_msg_cb callback);
void voice_proxy_deregister_msg_callback(uint16_t msg_id);
void voice_proxy_register_cmd_callback(uint16_t msg_id, voice_proxy_cmd_cb callback);
void voice_proxy_deregister_cmd_callback(uint16_t msg_id);
void voice_proxy_register_sign_init_callback(voice_proxy_sign_init_cb cb);
void voice_proxy_deregister_sign_init_callback(voice_proxy_sign_init_cb cb);
void voice_proxy_set_send_sign(bool first, bool *cnf, int64_t *timestamp);
int32_t voice_proxy_add_cmd(uint16_t msg_id);
int32_t voice_proxy_add_data(voice_proxy_add_data_cb callback, int8_t *data, uint32_t size, uint16_t msg_id);
int64_t voice_proxy_get_time_ms(void);
#endif /* end of voice_proxy.h */

