/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/syscalls.h>
#include <linux/firmware.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/sched/rt.h>
#include <linux/hisi/rdr_pub.h>
#include <asm/uaccess.h>

#include <linux/hisi/hi64xx/hi64xx_dsp_regs.h>
#include <linux/hisi/hi64xx/hi64xx_vad.h>
#include <linux/hisi/hi64xx/hi64xx_utils.h>
#include <linux/hisi/hi64xx_hifi_misc.h>
#include <dsm_audio/dsm_audio.h>
#include "../mntn/blackbox/platform_hifi/rdr_hisi_audio_codec.h"
#include "../mntn/blackbox/platform_hifi/rdr_hisi_audio_adapter.h"
#include "hi64xx_algo_interface.h"
#include "hi64xx_hifi_interface.h"

#include "hi64xx_hifi_debug.h"
#include "hi64xx_hifi_img_dl.h"
#include "../slimbus/slimbus.h"
#include "hi64xx_hifi_om.h"
#include "hi64xx_hifi_anc_beta.h"
#include "../soundtrigger/soundtrigger_dma_drv.h"

/*lint -e655 -e838 -e730 -e747*/

#define UNUSED_PARAMETER(x) (void)(x)

#define MAX_MSG_SIZE 128
#define MAX_PARA_SIZE 4096
#define RESULT_SIZE 4
#define MAX_OUT_PARA_SIZE ((RESULT_SIZE) + (MAX_PARA_SIZE))
#define MAX_USR_INPUT_SIZE (MAX_MSG_SIZE + MAX_PARA_SIZE)

#define HI64XX_RELOAD_RETRY_MAX 3
#define HI64XX_EXCEPTION_RETRY 3
#define HI64XX_GET_STATE_MUTEX_RETRY 20

#define HI64XX_VERSION_CS      0x11
#define HI64XX_VERSION_ES      0x10
#define MAX_STR_LEN 64
#define HI64XX_IFOPEN_WAIT4DPCLK       1000    //wait 100ms.(1000 cycles * 100us every cycle)
#define MAX_PARAM_SIZE 2

#define HI64XX_CMD_VLD_BITS                0xf

#define MEM_CHECK_MAGIC_NUM1 0x5a5a5a5a
#define MEM_CHECK_MAGIC_NUM2 0xa5a5a5a5

#define HI64XX_DSP_TO_AP_MSG_ACTIVE   (0x5a5a5a5a)
#define HI64XX_DSP_TO_AP_MSG_DEACTIVE (0xa5a5a5a5)

#define SOC_GPIO_ADDR				   0xfff0e3fc
#define SOC_GPIO_DIR_ADDR			   0xfff0e400
#define HI64XX_HIFI_FPGA_OM_TEST_ADDR  0xE86120D4

#define HI64XXDEBUG_LEVEL_PROC_FILE  "hi64xxdebuglevel"
#define HI64XXDEBUG_PROC_FILE        "hi64xxdebug"
#define HI64XXDEBUG_PATH             "hi64xxdbg"

#define ROOT_UID     0
#define SYSTEM_GID   1000

#define INTERVAL_TIMEOUT_MS (1000)
#define INTERRUPTED_SIGNAL_DELAY_MS (10)

#define REG_ROW_LEN         60
#define REG_ROW_NUM         200
#define MAX_DUMP_REG_SIZE   12000

#define WAKEUP_SCENE_DSP_CLOSE 1
#define WAKEUP_SCENE_PLL_CLOSE 2
#define WAKEUP_SCENE_PLL_OPEN 3

void hi64xx_watchdog_send_event(void);

/*XXX: change to 4 to enbale debug print*/
unsigned long hi64xx_dsp_debug_level = 3;


struct reg_rw_struct {
	unsigned int	reg;
	unsigned int	val;
};

struct hi64xx_dsp_priv {
	struct hi64xx_irq *p_irq;
	struct hi64xx_resmgr *resmgr;
	struct snd_soc_codec *codec;
	struct notifier_block resmgr_nb;
	struct hi64xx_dsp_config dsp_config;
	struct mutex msg_mutex;
	struct mutex peri_mutex;

	unsigned int sync_msg_ret;
	wait_queue_head_t sync_msg_wq;

	unsigned int dsp_pllswitch_done;
	wait_queue_head_t dsp_pllswitch_wq;

	unsigned int dsp_pwron_done;
	wait_queue_head_t dsp_pwron_wq;

	struct workqueue_struct *msg_proc_wq;
	struct delayed_work msg_proc_work;

	/* we lock the two below to avoid a change after config */
	enum pll_state pll_state;
	bool dsp_is_running;
	struct mutex state_mutex;

	unsigned int low_freq_scene_status;
	unsigned int high_freq_scene_status;
	unsigned int uart_mode;

	bool is_dspif_hooking;
	bool is_watchdog_coming;
	bool is_sync_write_timeout;
};

static struct hi64xx_dsp_priv *dsp_priv = NULL;

static struct notifier_block hi64xx_sr_nb;
static struct notifier_block hi64xx_reboot_nb;
atomic_t volatile hi64xx_in_suspend = ATOMIC_INIT(0);
atomic_t volatile hi64xx_in_saving = ATOMIC_INIT(0);

struct reg_dump {
	unsigned int addr;
	unsigned int len;
	const char* name;
};

static const struct reg_dump s_reg_dump[] = {
	{HI64xx_DUMP_CFG_SUB_ADDR1,       HI64xx_DUMP_CFG_SUB_SIZE1,      "page_cfg_subsys:"},
	{HI64xx_DUMP_CFG_SUB_ADDR2,       HI64xx_DUMP_CFG_SUB_SIZE2,      "page_cfg_subsys:"},
	{HI64xx_DUMP_AUDIO_SUB_ADDR,      HI64xx_DUMP_AUDIO_SUB_SIZE,     "aud_reg:"},
	{HI64xx_DUMP_DSP_EDMA_ADDR1,      HI64xx_DUMP_DSP_EDMA_SIZE1,     "DMA:"},
	{HI64xx_DUMP_DSP_EDMA_ADDR2,      HI64xx_DUMP_DSP_EDMA_SIZE2,     "DMA:"},
	{HI64xx_DUMP_DSP_EDMA_ADDR3,      HI64xx_DUMP_DSP_EDMA_SIZE3,     "DMA:"},
	{HI64xx_DUMP_DSP_WATCHDOG_ADDR1,  HI64xx_DUMP_DSP_WATCHDOG_SIZE1, "WTD:"},
	{HI64xx_DUMP_DSP_WATCHDOG_ADDR2,  HI64xx_DUMP_DSP_WATCHDOG_SIZE2, "WTD:"},
	{HI64xx_DUMP_DSP_SCTRL_ADDR1,     HI64xx_DUMP_DSP_SCTRL_SIZE1,    "SCTRL:"},
	{HI64xx_DUMP_DSP_SCTRL_ADDR2,     HI64xx_DUMP_DSP_SCTRL_SIZE2,    "SCTRL:"},
	/* XXX: 0x20007038, 0x20007039 should always read in the end */
	{HI64xx_DUMP_CFG_SUB_ADDR3,       HI64xx_DUMP_CFG_SUB_SIZE3,      "page_cfg_subsys:"},
};

static inline long long timeval_to_ms(struct timeval tv)
{
	return 1000 * tv.tv_sec + tv.tv_usec / 1000;
}

static void hi64xx_stop_dspif_hook(void)
{
	if (dsp_priv->is_dspif_hooking) {
		HI64XX_DSP_WARNING("dsp scene will start, stop hooking dspif data.\n");
		hi64xx_stop_hook();
	}
}

void hi64xx_hifi_write_reg(unsigned int reg, unsigned int val)
{
	(void)snd_soc_write(dsp_priv->codec, reg, val);
}

unsigned int hi64xx_hifi_read_reg(unsigned int reg)
{
	return snd_soc_read(dsp_priv->codec, reg);
}

void hi64xx_hifi_reg_set_bit(unsigned int reg, unsigned int offset)
{

	(void)hi64xx_update_bits(dsp_priv->codec, reg, 1 << offset, 1 << offset);
}

void hi64xx_hifi_reg_clr_bit(unsigned int reg, unsigned int offset)
{
	(void)hi64xx_update_bits(dsp_priv->codec, reg, 1 << offset, 0);
}

void hi64xx_hifi_reg_write_bits(unsigned int reg,
				unsigned int value,
				unsigned int mask)
{
	(void)hi64xx_update_bits(dsp_priv->codec, reg, mask, value);
}

void hi64xx_memset(uint32_t dest, size_t n)
{
	size_t i = 0;

	if (n & 0x3) {
		HI64XX_DSP_ERROR("memset size: 0x%zu is not 4 byte aligned\n", n);
		return ;
	}

	for (i = 0; i < n; i = i + 4) {
		hi64xx_hifi_write_reg(dest, 0x0);
		dest += 4;
	}
}

void hi64xx_memcpy(uint32_t dest, uint32_t *src, size_t n)
{
	size_t i = 0;

	if (n & 0x3) {
		HI64XX_DSP_ERROR("memcpy size: 0x%zu is not 4 byte aligned\n", n);
		return ;
	}

	for (i = 0; i < n; i = i + 4) {
		hi64xx_hifi_write_reg(dest, *src);
		src++;
		dest += 4;
	}
}

void hi64xx_write(const unsigned int start_addr,
			 const unsigned int *buf,
			 const unsigned int len)
{
	unsigned int i = 0;

	if (!buf || 0 == len) {
		HI64XX_DSP_ERROR("input buf or len error, len:%u\n", len);
		return;
	}

	if (len & 0x3) {
		HI64XX_DSP_ERROR("write size:0x%x is not 4 byte aligned\n", len);
		return ;
	}

	for (i = 0; i < len; i += 4) {
		hi64xx_hifi_write_reg(start_addr + i, *buf++);
	}
}


static void hi64xx_read_per_4byte(const unsigned int start_addr,
			unsigned int *buf,
			const unsigned int len)
{
	unsigned int i = 0;

	if (!buf || 0 == len) {
		HI64XX_DSP_ERROR("input buf or len error, len:%u\n", len);
		return;
	}

	if (len & 0x3) {
		HI64XX_DSP_ERROR("read size:0x%x  is not 4 byte aligned\n", len);
		return ;
	}

	for (i = 0; i < len; i += 4) {
		*buf++ = hi64xx_hifi_read_reg(start_addr + i);
	}
}

static void hi64xx_read_per_1byte(const unsigned int start_addr,
			      unsigned char *buf,
			      const unsigned int len)
{
	unsigned int i = 0;

	if (!buf || 0 == len) {
		HI64XX_DSP_ERROR("input buf or len error, len:%u\n", len);
		return;
	}

	for (i = 0; i < len; i++) {
		*buf++ = hi64xx_hifi_read_reg(start_addr + i);
	}
}

void hi64xx_read(const unsigned int start_addr,
			unsigned char *arg,
			const unsigned int len)
{
	/* XXX:dont read a block from within 20007xxx to outside, vice versa */
	/* start_addr NOT in 0x20007000~0x20007xxx */
	if (start_addr < HI64XX_1BYTE_SUB_START
	    || start_addr > HI64XX_1BYTE_SUB_END) {
		if ((start_addr < HI64XX_1BYTE_SUB_START)
			&& (start_addr + len >= HI64XX_1BYTE_SUB_START)) {
			HI64XX_DSP_ERROR("range error: start:0x%pK, len:0x%x\n",
				(void *)(unsigned long)start_addr, len);
			return;
		}

		if ((len & 0x3) || (0 == len)) {
			HI64XX_DSP_ERROR("input len error, len:%u\n", len);
			return;
		}

		hi64xx_read_per_4byte(start_addr, (unsigned int *)arg, len);/*lint !e826*/
	/* start_addr in 0x20007000~0x20007xxx */
	} else {
		if (start_addr + len > HI64XX_1BYTE_SUB_END) {
			HI64XX_DSP_ERROR("range error: start:0x%pK, len:0x%x\n",
				(void *)(unsigned long)start_addr, len);
			return;
		}
		hi64xx_read_per_1byte(start_addr, arg, len);
	}
}


static void hi64xx_hifi_set_pll(bool enable)
{
	/* set pll */
	if (enable)
		hi64xx_resmgr_request_pll(dsp_priv->resmgr, PLL_HIGH);
	else
		hi64xx_resmgr_release_pll(dsp_priv->resmgr, PLL_HIGH);
}

static void hi64xx_hifi_set_low_pll(bool enable)
{
	/* set mad_pll */
	if (enable)
		hi64xx_resmgr_request_pll(dsp_priv->resmgr, PLL_LOW);
	else
		hi64xx_resmgr_release_pll(dsp_priv->resmgr, PLL_LOW);
}

static void hi64xx_save_log(void)
{

}

static void hi64xx_update_cmd_status(void)
{
	int count = HI64XX_EXCEPTION_RETRY;
	uint32_t cmd_status;

	cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS);

	while (count) {
		count --;
		hi64xx_hifi_write_reg(HI64xx_DSP_SUB_CMD_STATUS, cmd_status & HI64XX_CMD_VLD_BITS);
		cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS);
		HI64XX_DSP_WARNING("cmd status:%d, retry %d\n", cmd_status, HI64XX_EXCEPTION_RETRY - count);

		if (0 == (cmd_status & (~(uint32_t)HI64XX_CMD_VLD_BITS))) {
			hi64xx_save_log();
			return;
		}
	}

	hi64xx_save_log();
	hi64xx_watchdog_send_event();
	return;
}

static void hi64xx_cmd_status_clr_bit(uint32_t cmd_status, uint8_t cmd_bit)
{
	int count = HI64XX_EXCEPTION_RETRY;

	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SUB_CMD_STATUS, cmd_bit);
	cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS);

	while (count) {
		if (0 == ((cmd_status >> cmd_bit) & 1)) {
			if (count != HI64XX_EXCEPTION_RETRY)
				hi64xx_save_log();
			return;
		} else {
			count --;
			hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SUB_CMD_STATUS, cmd_bit);
			cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS);
			HI64XX_DSP_WARNING("cmd status:%d, retry %d\n", cmd_status, HI64XX_EXCEPTION_RETRY - count);
		}
	}

	hi64xx_save_log();
	hi64xx_watchdog_send_event();
	return;
}

/* HI64xx_DSP_CMD_STATUS :
 * bit 0 sync_msg
 * bit 1 dsp_pllswitch
 * bit 2 dsp_pwron
 */
static irqreturn_t hi64xx_msg_irq_handler(int irq, void *data)
{
	uint32_t cmd_status = 0;

	IN_FUNCTION;

	if(!hi64xx_hifi_read_reg(HI64xx_DSP_CMD_STATUS_VLD)){
		HI64XX_DSP_ERROR("CMD invalid\n");
		return IRQ_HANDLED;
	}

	/*
	 * todo : 2.use else if ? cmd should not come together ?
	 * todo : 3.freq high -> freq low and stop mad cmd maybe come together
	 *			(eg.headset play and mad on,then stop play and stop mad)
	 */
	cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_CMD_STATUS);
	HI64XX_DSP_INFO("cmd status:%d\n", cmd_status);
	if (0 != (cmd_status & (~(uint32_t)HI64XX_CMD_VLD_BITS)))
		hi64xx_update_cmd_status();

	if (dsp_priv->dsp_config.cmd4_addr)
		HI64XX_DSP_INFO("[codechifi timestamp:%u] update cnf\n", hi64xx_hifi_read_reg(dsp_priv->dsp_config.cmd4_addr));

	if (cmd_status & (0x1 << HI64xx_DSP_MSG_BIT)){

		HI64XX_DSP_INFO("codec hifi msg cnf\n");
		if (dsp_priv->dsp_config.msg_state_addr != 0)
			hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
				HI64xx_HIFI_AP_RECEIVE_MSG_CNF);

		hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_MSG_BIT);
		dsp_priv->sync_msg_ret = 1;
		wake_up_interruptible_all(&dsp_priv->sync_msg_wq);
	}
	if (cmd_status & (0x1 << HI64xx_DSP_PLLSWITCH_BIT)){

		HI64XX_DSP_INFO("codec hifi pll switch done cnf\n");
		if (dsp_priv->dsp_config.msg_state_addr != 0)
			hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
				HI64xx_HIFI_AP_RECEIVE_PLL_SW_CNF);

		hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_PLLSWITCH_BIT);
		dsp_priv->dsp_pllswitch_done = 1;
		wake_up_interruptible_all(&dsp_priv->dsp_pllswitch_wq);
	}
	if (cmd_status & (0x1 << HI64xx_DSP_POWERON_BIT)){

		HI64XX_DSP_INFO("codec hifi pwron done cnf\n");
		if (dsp_priv->dsp_config.msg_state_addr != 0)
			hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
				HI64xx_HIFI_AP_RECEIVE_PWRON_CNF);

		hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_POWERON_BIT);
		dsp_priv->dsp_pwron_done = HIFI_STATE_INIT;
		wake_up_interruptible_all(&dsp_priv->dsp_pwron_wq);
	}
	if (cmd_status & (0x1 << HI64xx_DSP_MSG_WITH_CONTENT_BIT)){

		HI64XX_DSP_INFO("codec hifi msg come\n");
		if (!dsp_priv->msg_proc_wq) {
			HI64XX_DSP_ERROR("message workqueue doesn't init\n");
			hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_MSG_WITH_CONTENT_BIT);
			return IRQ_HANDLED;
		}

		if (!queue_delayed_work(dsp_priv->msg_proc_wq,
				&dsp_priv->msg_proc_work, msecs_to_jiffies(0)))
			HI64XX_DSP_WARNING("lost dsp msg\n");

		hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_MSG_WITH_CONTENT_BIT);
	}

	OUT_FUNCTION;

	return IRQ_HANDLED;
}

static int hi64xx_write_mlib_para(const unsigned char *arg,
				  const unsigned int len)
{
	if ((0 == len) || (len > MAX_PARA_SIZE)) {
		HI64XX_DSP_ERROR("msg length:%u exceed limit!\n", len);
		return -EINVAL;
	}

	if ((len & 0x3) != 0) {
		HI64XX_DSP_ERROR("msg length:%u is not 4 byte aligned\n", len);
		return -EINVAL;
	}

	hi64xx_write(dsp_priv->dsp_config.para_addr, (unsigned int *)arg, len);

	return OK;
}

static int hi64xx_read_mlib_para(unsigned char *arg, const unsigned int len)
{
	if (!arg || 0 == len) {
		HI64XX_DSP_ERROR("input buf or len error, len:%u\n", len);
		return -EINVAL;
	}

	if (len > MAX_PARA_SIZE) {
		HI64XX_DSP_ERROR("msg length:%u exceed limit!\n", len);
		return -EINVAL;
	}

	if ((len & 0x3) != 0) {
		HI64XX_DSP_ERROR("msg length:%u is not 4 byte aligned\n", len);
		return ERROR;
	}

	hi64xx_read(dsp_priv->dsp_config.para_addr, arg, len);

	return OK;
}

unsigned int hi64xx_read_mlib_test_para(unsigned char *arg, unsigned int len)
{
	unsigned int addr = 0;
	unsigned int msg_size = 0;
	unsigned int count;
	unsigned int value;
	IN_FUNCTION;

	if (!dsp_priv) {
		HI64XX_DSP_ERROR("dsp_priv is null\n");
		return 0;
	}

	addr = dsp_priv->dsp_config.mlib_to_ap_msg_addr;
	msg_size = dsp_priv->dsp_config.mlib_to_ap_msg_size;

	if (!addr || (!msg_size)) {
		HI64XX_DSP_ERROR("cannot find msg addr or size!\n");
		return 0;
	}

	if (hi64xx_hifi_read_reg(addr) != UCOM_PROTECT_WORD) {
		HI64XX_DSP_ERROR("mlib test cannot find parameters!\n");
		return 0;
	}

	for (count = 1; count < (msg_size / sizeof(unsigned int)); count++) {
		value = hi64xx_hifi_read_reg(addr + count * sizeof(unsigned int));
		if(count * sizeof(unsigned int) >= len) {
			HI64XX_DSP_ERROR("input not enough space!\n");
			return 0;
		}

		if(value == UCOM_PROTECT_WORD && count > 0) {
			break;
		}

		memcpy(arg + (count - 1) * sizeof(unsigned int), &value, sizeof(value));/* unsafe_function_ignore: memcpy */
		HI64XX_DSP_INFO("mlib test para[0x%x]\n", value);
	}

	OUT_FUNCTION;
	return (count - 1) * sizeof(unsigned int);
}

static int hi64xx_write_msg(const void *arg, const unsigned int len)
{
	int ret = OK;

	IN_FUNCTION;

	if ((0 == len) || (len > MAX_MSG_SIZE)) {
		HI64XX_DSP_ERROR("msg length exceed limit!\n");
		return -EINVAL;
	}

	if ((len & 0x3) != 0) {
		HI64XX_DSP_ERROR("msg length:%u is not 4 byte aligned\n", len);
		return -EINVAL;
	}

	hi64xx_write(dsp_priv->dsp_config.msg_addr, (unsigned int *)arg, len);

	dsp_priv->sync_msg_ret = 0;

	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);

	if (dsp_priv->dsp_config.msg_state_addr != 0)
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
			HI64xx_HIFI_AP_SEND_MSG);

	if(dsp_priv->dsp_config.dsp_ops.notify_dsp)
		dsp_priv->dsp_config.dsp_ops.notify_dsp();

	OUT_FUNCTION;

	return ret;
}

static int hi64xx_get_input_param(unsigned int usr_para_size,
				  void __user *usr_para_addr,
				  unsigned int *krn_para_size,
				  void **krn_para_addr)
{
	void *para_in = NULL;
	unsigned int para_size_in = 0;

	IN_FUNCTION;

	if (NULL == usr_para_addr) {
		HI64XX_DSP_ERROR("usr_para_addr is null no user data\n");
		goto ERR;
	}

	if ((0 == usr_para_size) || (usr_para_size > MAX_USR_INPUT_SIZE)) {
		HI64XX_DSP_ERROR("usr buffer size:%u out of range\n", usr_para_size);
		goto ERR;
	}

	para_size_in = roundup(usr_para_size, 4);

	para_in = kzalloc(para_size_in, GFP_KERNEL);
	if (para_in == NULL) {
		HI64XX_DSP_ERROR("kzalloc fail\n");
		goto ERR;
	}

	if (copy_from_user(para_in, usr_para_addr, usr_para_size)) {
		HI64XX_DSP_ERROR("copy_from_user fail\n");
		goto ERR;
	}

	*krn_para_size = para_size_in;
	*krn_para_addr = para_in;

	OUT_FUNCTION;

	return OK;

ERR:
	if (para_in != NULL) {
		kfree(para_in);
		para_in = NULL;
	}

	OUT_FUNCTION;

	return -EINVAL;
}

static void hi64xx_param_free(void **krn_para_addr)
{
	IN_FUNCTION;

	if (*krn_para_addr != NULL) {
		kfree(*krn_para_addr);
		*krn_para_addr = NULL;
	} else {
		HI64XX_DSP_ERROR("krn_para_addr to free is NULL\n");
	}

	OUT_FUNCTION;

	return;
}

static int hi64xx_alloc_output_param_buffer(unsigned int usr_para_size,
					    void __user *usr_para_addr,
						unsigned int *krn_para_size,
						void **krn_para_addr)
{
	if (*krn_para_addr != NULL) {
		HI64XX_DSP_ERROR("*krn_para_addr has already alloc memory\n");
		return -EINVAL;
	}

	HI64XX_DSP_DEBUG("malloc size: %u\n", usr_para_size);
	if (usr_para_size == 0 || usr_para_size > MAX_OUT_PARA_SIZE) {
		HI64XX_DSP_ERROR("usr space size invalid\n");
		return -EINVAL;
	}

	if (usr_para_addr == NULL) {
		HI64XX_DSP_ERROR("usr_para_addr is NULL\n");
		return -EINVAL;
	}

	*krn_para_addr = kzalloc(usr_para_size, GFP_KERNEL);
	if (*krn_para_addr == NULL) {
		HI64XX_DSP_ERROR("kzalloc fail\n");
		return -EINVAL;
	}

	*krn_para_size = usr_para_size;

	return OK;
}

static int hi64xx_put_output_param(unsigned int usr_para_size,
				   void __user *usr_para_addr,
				   unsigned int krn_para_size,
				   void *krn_para_addr)
{
	int ret = OK;

	if (0 == usr_para_size || !usr_para_addr) {
		HI64XX_DSP_ERROR("input usr_para_size[%d] or usr_para_addr error\n", usr_para_size);
		return -EINVAL;
	}

	if (0 == krn_para_size || !krn_para_addr) {
		HI64XX_DSP_ERROR("input error krn_para_size:%u\n", krn_para_size);
		return -EINVAL;
	}

	IN_FUNCTION;

	if (krn_para_size != usr_para_size) {
		HI64XX_DSP_ERROR("krn para size:%d != usr para size%d\n",
				 krn_para_size, usr_para_size);
		return -EINVAL;
	}

	HI64XX_DSP_DEBUG("user_para_size:%d\n", usr_para_size);
	ret = copy_to_user(usr_para_addr, krn_para_addr, usr_para_size);
	if (ret != OK) {
		HI64XX_DSP_ERROR("copy_to_user fail, ret is %d\n", ret);
		return -EPERM;
	}

	OUT_FUNCTION;

	return OK;
}

static bool hi64xx_error_detect(void)
{
	unsigned int version = hi64xx_hifi_read_reg(HI64xx_VERSION);

	if (HI64XX_VERSION_CS != version
		&& HI64XX_VERSION_ES != version) {
		HI64XX_DSP_ERROR("Codec err,ver 0x%x\n", version);
		audio_dsm_report_info(AUDIO_CODEC, DSM_CODEC_HIFI_RESET, "DSM_HI6402_CRASH, version: 0x%x\n", version);
		return true;
	}

	return false;
}

static bool hi64xx_retry_max_detect(int reload_retry_count)
{
	if (reload_retry_count >= HI64XX_RELOAD_RETRY_MAX) {
		HI64XX_DSP_ERROR("Codec hifi reset, reload retry count reaches the upper limit\n");
		audio_dsm_report_info(AUDIO_CODEC, DSM_CODEC_HIFI_RESET, "Codec err, reload retry count reaches upper limit\n");
		return true;
	}

	return false;
}

static bool hi64xx_request_state_mutex(void)
{
	int count = HI64XX_GET_STATE_MUTEX_RETRY;
	int sleep_time = 60;

	while (count) {
		if (mutex_trylock(&dsp_priv->state_mutex)) {
			HI64XX_DSP_INFO("state_mutex lock\n");
			return true;
		}
		msleep(sleep_time);
		count--;
	}

	HI64XX_DSP_ERROR("request state_mutex timeout %dms\n", sleep_time * (HI64XX_GET_STATE_MUTEX_RETRY - count));
	return false;
}

static void hi64xx_release_state_mutex(void)
{
	mutex_unlock(&dsp_priv->state_mutex);/*lint !e455*/
	HI64XX_DSP_INFO("state_mutex unlock\n");
}

static void hi64xx_watchdog_process(void)
{
	/* stop codec om asp dma */
	if (HI64XX_CODEC_TYPE_6402 != dsp_priv->dsp_config.codec_type)
		hi64xx_hifi_om_hook_stop();

	/* stop soundtrigger asp dma */
	hi64xx_soundtrigger_dma_close();

	rdr_codec_hifi_watchdog_process();
}

static int hi64xx_check_sync_write_status(void)
{
	uint32_t cmd_status = 0;

	if(!hi64xx_hifi_read_reg(HI64xx_DSP_CMD_STATUS_VLD)){
		HI64XX_DSP_ERROR("CMD invalid\n");
		goto out;
	}

	cmd_status = hi64xx_hifi_read_reg(HI64xx_DSP_CMD_STATUS);
	HI64XX_DSP_INFO("cmd status:%d, sync msg ret:%d\n",
		cmd_status, dsp_priv->sync_msg_ret);

	if (cmd_status & (0x1 << HI64xx_DSP_MSG_BIT)) {
		HI64XX_DSP_WARNING("codec hifi msg cnf, but ap donot handle this irq\n");
		if (dsp_priv->dsp_config.msg_state_addr != 0)
			hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
				HI64xx_HIFI_AP_RECEIVE_MSG_CNF);

		hi64xx_cmd_status_clr_bit(cmd_status, HI64xx_DSP_MSG_BIT);

		return OK;
	}
out:

	/* can't get codec version,reset system */
	BUG_ON(hi64xx_error_detect());

	HI64XX_DSP_ERROR("cmd timeout\n");

	audio_dsm_report_info(AUDIO_CODEC, DSM_CODEC_HIFI_SYNC_TIMEOUT,
			"codec hifi sync message timeout,CMD_STAT:0x%x, CMD_STAT_VLD:0x%x\n",
			hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS),
			hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS_VLD));
	if (!(dsp_priv->is_watchdog_coming) && !(dsp_priv->is_sync_write_timeout)) {
		HI64XX_DSP_ERROR("save log and reset media \n");
		dsp_priv->is_sync_write_timeout = true;
		hi64xx_watchdog_process();
	}
	return -EBUSY;
}

static int hi64xx_sync_write(const void *arg, const unsigned int len)
{
	int ret = OK;
	long ret_l = 0;
	int count = 0;
	long long begin, update;
	struct timeval time_begin, time_update;
	unsigned int interrupt_count = 0;
	IN_FUNCTION;

	/* can't get codec version,reset system */
	BUG_ON(hi64xx_error_detect());

	if (!hi64xx_request_state_mutex()) {
		HI64XX_DSP_ERROR("state_mutex not release\n");
		ret = -EBUSY;
		goto out1;
	}

	dsp_priv->sync_msg_ret = 0;

	hi64xx_resmgr_pm_get_clk();

	count = HI64XX_EXCEPTION_RETRY;
	while (count) {
		if (dsp_priv->is_watchdog_coming) {
			HI64XX_DSP_ERROR("watchdog have already come, can't send msg\n");
			ret = -EBUSY;
			goto out;
		}

		ret = hi64xx_write_msg(arg, len);
		if (OK != ret) {
			HI64XX_DSP_ERROR("send msg failed\n");
			goto out;
		}

		do_gettimeofday(&time_begin);

wait_interrupt:
		ret_l = wait_event_interruptible_timeout(dsp_priv->sync_msg_wq,
			(dsp_priv->sync_msg_ret == 1), HZ);/*lint !e665*/

		if (ret_l > 0) {
			if (interrupt_count > 0)
				HI64XX_DSP_WARNING("sync cmd is interrupted %u times\n", interrupt_count);
			goto out;
		} else if (ret_l == -ERESTARTSYS) {
			interrupt_count++;
			do_gettimeofday(&time_update);
			begin = 1000 * time_begin.tv_sec + time_begin.tv_usec/1000;
			update = 1000 * time_update.tv_sec + time_update.tv_usec/1000;
			if (update - begin > INTERVAL_TIMEOUT_MS) {
				count--;
				HI64XX_DSP_WARNING("cmd is interrupted %u times, retry %d times\n",
					interrupt_count, HI64XX_EXCEPTION_RETRY - count);
				interrupt_count = 0;
			} else {
				if (update < begin)
					do_gettimeofday(&time_begin);
				if (dsp_priv->sync_msg_ret == 0)
					goto wait_interrupt;
			}
		} else {
			count--;
			HI64XX_DSP_ERROR("cmd is interrupted %u times, retry %d times, ret = %ld\n",
				interrupt_count, HI64XX_EXCEPTION_RETRY - count, ret_l);
			interrupt_count = 0;
		}

		HI64XX_DSP_ERROR("CMD_STAT:0x%x, CMD_STAT_VLD:0x%x\n",
				hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS),
				hi64xx_hifi_read_reg(HI64xx_DSP_SUB_CMD_STATUS_VLD));
	}

	if (0 == count) {
		if (OK == hi64xx_check_sync_write_status())
			goto out;
		else
			ret = -EBUSY;
	}

out:
	hi64xx_resmgr_pm_put_clk();
	hi64xx_release_state_mutex();
out1:
	OUT_FUNCTION;

	return ret;
}

static void hi64xx_hifi_pause(void)
{
	int wait_cycle = 50;

	IN_FUNCTION;

	while (wait_cycle) {
		if (1 == (hi64xx_hifi_read_reg(HI64xx_DSP_DSP_STATUS0) & 0x1)
			|| (!dsp_priv->dsp_is_running))
			break;

		/* wait 100 to 110us every cycle */
		usleep_range(100, 110);
		wait_cycle--;
	}

	if (0 == wait_cycle) {
		HI64XX_DSP_ERROR("wait dsp wfi timeout, dsp_status0:0x%x, msg state:0x%x(0xff:not hi64xx)\n",
			hi64xx_hifi_read_reg(HI64xx_DSP_DSP_STATUS0),
			(dsp_priv->dsp_config.msg_state_addr != 0) ? hi64xx_hifi_read_reg(dsp_priv->dsp_config.msg_state_addr) : 0xff);
	}

	if(dsp_priv->dsp_config.dsp_ops.wtd_enable)
		dsp_priv->dsp_config.dsp_ops.wtd_enable(false);

	if (dsp_priv->dsp_config.dsp_ops.clk_enable)
		dsp_priv->dsp_config.dsp_ops.clk_enable(false);

	OUT_FUNCTION;
}

static void hi64xx_hifi_resume(enum pll_state state)
{
	IN_FUNCTION;

	if (dsp_priv->dsp_config.dsp_ops.clk_enable)
		dsp_priv->dsp_config.dsp_ops.clk_enable(true);

	if(dsp_priv->dsp_config.dsp_ops.wtd_enable)
		dsp_priv->dsp_config.dsp_ops.wtd_enable(true);

	OUT_FUNCTION;
	return;
}

static void hi64xx_hifi_notify_dsp_pllswitch(unsigned char state)
{
	long ret_l = 0;
	unsigned int interrupt_count = 0;
	unsigned long long begin, update;
	struct timeval time_begin, time_update;

	/* notify dsp stop dma and close dspif */
	dsp_priv->dsp_pllswitch_done = 0;
	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd0_addr, state);

	HI64XX_DSP_INFO("cmd[0x%x]reg[0x%x]pll state[0x%x]\n", state,
			hi64xx_hifi_read_reg(dsp_priv->dsp_config.cmd0_addr),
			hi64xx_hifi_read_reg(dsp_priv->dsp_config.cmd1_addr));

	if(dsp_priv->dsp_config.dsp_ops.notify_dsp)
		dsp_priv->dsp_config.dsp_ops.notify_dsp();

	do_gettimeofday(&time_begin);
	begin = timeval_to_ms(time_begin);

	do {
		if (dsp_priv->is_watchdog_coming) {
			HI64XX_DSP_ERROR("watchdog have already come, can't send msg\n");
			break;
		}

		ret_l = wait_event_interruptible_timeout(dsp_priv->dsp_pllswitch_wq,
				(dsp_priv->dsp_pllswitch_done == 1), HZ);/*lint !e665*/

		if (dsp_priv->dsp_pllswitch_done) {
			HI64XX_DSP_INFO("pll switch done, interrupt count %u\n", interrupt_count);
			break;
		}

		if (ret_l == -ERESTARTSYS) {
			interrupt_count++;
			msleep(INTERRUPTED_SIGNAL_DELAY_MS);
		}

		do_gettimeofday(&time_update);
		update = timeval_to_ms(time_update);

	} while (update - begin < 3 * INTERVAL_TIMEOUT_MS);

	HI64XX_DSP_INFO("pll switch is interrupted %u times, return %ld\n",
					interrupt_count, ret_l);
}

static void hi64xx_hifi_cfg_before_pll_switch(void)
{
	IN_FUNCTION;

	HI64XX_DSP_INFO("state_mutex lock\n");
	mutex_lock(&dsp_priv->state_mutex);

	if (dsp_priv->dsp_is_running
		&& !(dsp_priv->is_sync_write_timeout)
		&& !(dsp_priv->is_watchdog_coming)) {
		/* todo : put below code in hi64xx_hifi_pause() */
		HI64XX_DSP_INFO("notify dsp close dma\n");

		hi64xx_hifi_notify_dsp_pllswitch(HIFI_POWER_CLK_OFF);
		hi64xx_hifi_pause();
	}

	/* unlock in after pll switch */

	OUT_FUNCTION;

	return;/*lint !e454*/
}

bool hi64xx_hifi_is_running(void)
{
	return dsp_priv->dsp_is_running;
}

static const char *state_name[] = {
	"PLL_DOWN",
	"PLL_HIGH_FREQ",
	"PLL_LOW_FREQ",
};

static void hi64xx_hifi_cfg_after_pll_switch(enum pll_state state)
{
	unsigned int regval[5];

	IN_FUNCTION;

	regval[0] = hi64xx_hifi_read_reg(HI64xx_VERSION_REG);
	regval[1] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG0);
	regval[2] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG1);
	regval[3] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG2);
	regval[4] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG3);

	HI64XX_DSP_INFO("%s->%s, dsp_is_runing:%d, version:%#x, chipid0:0x%x, chipid1:0x%x, chipid2:0x%x, chipid3:0x%x\n",
		state_name[dsp_priv->pll_state], state_name[state], dsp_priv->dsp_is_running, regval[0], regval[1], regval[2], regval[3], regval[4]);

	dsp_priv->pll_state = state;

	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);
	if(dsp_priv->dsp_config.dsp_ops.set_dsp_div) {
		dsp_priv->dsp_config.dsp_ops.set_dsp_div(dsp_priv->pll_state);
		HI64XX_DSP_INFO("switch dsp pll source\n");
	}

	if (dsp_priv->dsp_is_running
		&& !(dsp_priv->is_sync_write_timeout)
		&& !(dsp_priv->is_watchdog_coming)) {
		hi64xx_hifi_resume(dsp_priv->pll_state);
		/* todo : put below code in hi64xx_hifi_resume() */

		hi64xx_hifi_notify_dsp_pllswitch(HIFI_POWER_CLK_ON);
	}
	mutex_unlock(&dsp_priv->state_mutex);/*lint !e455*/
	HI64XX_DSP_INFO("state_mutex unlock\n");

	OUT_FUNCTION;
	return;
}

/* check for parameters used by misc, only for if_open/if_close */
static int hi64xx_dsp_if_para_check(const struct krn_param_io_buf *param)
{
	unsigned int i = 0;
	unsigned int message_size = 0;
	DSP_IF_OPEN_REQ_STRU *dsp_if_open_req = NULL;
	PCM_PROCESS_DMA_MSG_STRU *dma_msg_stru = NULL;
	unsigned int max_if_id;

	if (!param) {
		HI64XX_DSP_ERROR("input param is null\n");
		return -EINVAL;
	}

	if (!param->buf_in || 0 == param->buf_size_in) {
		HI64XX_DSP_ERROR("input buf_in or buf_size_in[%u] error\n", param->buf_size_in);
		return -EINVAL;
	}

	if (param->buf_size_in < sizeof(DSP_IF_OPEN_REQ_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}
		max_if_id = HI64XX_HIFI_DSP_IF_PORT_8;

	dsp_if_open_req = (DSP_IF_OPEN_REQ_STRU *)(param->buf_in);
	dma_msg_stru = &dsp_if_open_req->stProcessDMA;

	if (dma_msg_stru->uwIFCount > max_if_id) {
		HI64XX_DSP_ERROR("try to open too many ifs\n");
		return -EINVAL;
	}

	message_size = sizeof(PCM_IF_MSG_STRU) * (dma_msg_stru->uwIFCount)
			+ sizeof(DSP_IF_OPEN_REQ_STRU);
	if (param->buf_size_in < message_size) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	for (i = 0; i < dma_msg_stru->uwIFCount; i++) {
		PCM_IF_MSG_STRU *pcm_if_msg = &dma_msg_stru->stIFCfgList[i];

		if (pcm_if_msg->uwIFId > max_if_id) {
			HI64XX_DSP_ERROR("dsp if ID %d is out of range\n",
					pcm_if_msg->uwIFId);
			return -EINVAL;
		}

		switch (pcm_if_msg->uwSampleRateIn) {
		case 0:
			HI64XX_DSP_INFO("DATA_HOOK_PROCESS, sample_rate=0\n");
			break;
		case 8000:
		case 16000:
		case 32000:
		case 48000:
		case 96000:
		case 192000:
			break;
		default:
			HI64XX_DSP_ERROR("unsupport sample_rate %d \n",
					pcm_if_msg->uwSampleRateIn);
			return -EINVAL;
		}
	}

	return OK;
}

bool hi64xx_get_sample_rate_index(unsigned int sample_rate, unsigned char *index)
{
	switch (sample_rate) {
	case 0:
		HI64XX_DSP_INFO("DATA_HOOK_PROCESS, sample_rate=0\n");
		break;
	case 8000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_8K;
		break;
	case 16000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_16K;
		break;
	case 32000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_32K;
		break;
	case 48000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_48K;
		break;
	case 96000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_96K;
		break;
	case 192000:
		*index = HI64XX_HIFI_PCM_SAMPLE_RATE_192K;
		break;
	default:
		/* shouldn't be here */
		HI64XX_DSP_ERROR("unsupport sample_rate %d!! \n", sample_rate);
		return false;
	}

	return true;
}

/* now we'v alread check the para, so don't do it again */
static void hi64xx_dsp_if_sample_rate_set(const char *arg)
{
	unsigned int i = 0;
	int ret = 0;

	DSP_IF_OPEN_REQ_STRU *dsp_if_open_req = (DSP_IF_OPEN_REQ_STRU *)arg;
	PCM_PROCESS_DMA_MSG_STRU *dma_msg_stru = &dsp_if_open_req->stProcessDMA;

	IN_FUNCTION;

	for (i = 0; i < dma_msg_stru->uwIFCount; i++) {
		PCM_IF_MSG_STRU *pcm_if_msg = &dma_msg_stru->stIFCfgList[i];

		if (dsp_priv->dsp_config.dsp_ops.set_sample_rate) {
			ret = dsp_priv->dsp_config.dsp_ops.set_sample_rate(pcm_if_msg->uwIFId,
				pcm_if_msg->uwSampleRateIn, pcm_if_msg->uwSampleRateOut);
			if (ret) {
				HI64XX_DSP_ERROR("set sample rate error, fid = %u, sample rate in = %u, sample rate out = %u\n",
					pcm_if_msg->uwIFId, pcm_if_msg->uwSampleRateIn, pcm_if_msg->uwSampleRateOut);
			}
		}
	}

	OUT_FUNCTION;
}
/*lint -e454 -e455 -e456*/
void hi64xx_hifi_misc_peri_lock(void)
{
	if (dsp_priv != NULL)
		mutex_lock(&dsp_priv->peri_mutex);
}

void hi64xx_hifi_misc_peri_unlock(void)
{
	if (dsp_priv != NULL)
		mutex_unlock(&dsp_priv->peri_mutex);
}
/*lint +e454 +e455 +e456*/
static void hi64xx_dsp_run(void)
{
	IN_FUNCTION;

	HI64XX_DSP_INFO("state_mutex lock\n");
	mutex_lock(&dsp_priv->state_mutex);
	hi64xx_hifi_misc_peri_lock();

	if (!dsp_priv->dsp_is_running) {
		if (dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl)
			dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl(true);

		if (dsp_priv->dsp_config.dsp_ops.ram2axi)
			dsp_priv->dsp_config.dsp_ops.ram2axi(true);

		if (dsp_priv->dsp_config.dsp_ops.clk_enable)
			dsp_priv->dsp_config.dsp_ops.clk_enable(true);

		if(dsp_priv->dsp_config.dsp_ops.wtd_enable)
			dsp_priv->dsp_config.dsp_ops.wtd_enable(true);

		dsp_priv->dsp_is_running = true;
	}

	hi64xx_hifi_misc_peri_unlock();
	mutex_unlock(&dsp_priv->state_mutex);
	HI64XX_DSP_INFO("state_mutex unlock\n");

	OUT_FUNCTION;
}

static void hi64xx_dsp_stop(void)
{
	int wait_cycle = 50;

	IN_FUNCTION;

	while (wait_cycle) {
		if (1 == (hi64xx_hifi_read_reg(HI64xx_DSP_DSP_STATUS0) & 0x1)
			|| (!dsp_priv->dsp_is_running))
			break;

		/* wait 100 to 110us every cycle */
		usleep_range((unsigned long)100, (unsigned long)110);
		wait_cycle--;
	}

	if (0 == wait_cycle) {
		HI64XX_DSP_ERROR("wait dsp wfi timeout, dsp_status0:0x%x, msg state:0x%x(0xff:not hi64xx)\n",
			hi64xx_hifi_read_reg(HI64xx_DSP_DSP_STATUS0),
			(dsp_priv->dsp_config.msg_state_addr != 0) ? hi64xx_hifi_read_reg(dsp_priv->dsp_config.msg_state_addr) : 0xff);
	}

	HI64XX_DSP_INFO("state_mutex lock\n");
	mutex_lock(&dsp_priv->state_mutex);

	if (dsp_priv->dsp_is_running) {
		if(dsp_priv->dsp_config.dsp_ops.wtd_enable)
			dsp_priv->dsp_config.dsp_ops.wtd_enable(false);

		if (dsp_priv->dsp_config.dsp_ops.clk_enable)
			dsp_priv->dsp_config.dsp_ops.clk_enable(false);

		if (dsp_priv->dsp_config.dsp_ops.ram2axi)
			dsp_priv->dsp_config.dsp_ops.ram2axi(false);
		if (dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl)
			dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl(false);

		dsp_priv->dsp_is_running = false;
	}

	mutex_unlock(&dsp_priv->state_mutex);
	HI64XX_DSP_INFO("state_mutex unlock\n");

	OUT_FUNCTION;
}

int hi64xx_request_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (!dsp_priv) {
		HI64XX_DSP_ERROR("dsp_priv is null\n");
		return -EPERM;
	}

	HI64XX_DSP_INFO("sid[0x%x]hifreq_status[0x%x]", scene_id,
		dsp_priv->high_freq_scene_status);

	if (scene_id >= HI_FREQ_SCENE_BUTT) {
		HI64XX_DSP_ERROR("unknow scene for pll: %u\n", scene_id);
		return -EPERM;
	}

	if ((dsp_priv->high_freq_scene_status & (1 << scene_id)) != 0) {
		HI64XX_DSP_WARNING("scene: %u is alread started.\n", scene_id);
		return REDUNDANT;
	}

	if (dsp_priv->high_freq_scene_status == 0) {
		hi64xx_hifi_set_pll(true);
		hi64xx_dsp_run();
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);
	}

	dsp_priv->high_freq_scene_status |= (1 << scene_id);

	OUT_FUNCTION;

	return OK;
}

void hi64xx_release_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (!dsp_priv) {
		HI64XX_DSP_ERROR("dsp_priv is null\n");
		return;
	}

	if (scene_id >= HI_FREQ_SCENE_BUTT) {
		HI64XX_DSP_ERROR("unknow scene for pll: %u\n", scene_id);
		return;
	}

	if ((dsp_priv->high_freq_scene_status & (1 << scene_id)) == 0) {
		HI64XX_DSP_WARNING("scene: %u is NOT started\n", scene_id);
		return;
	}

	dsp_priv->high_freq_scene_status &= ~(1ul << scene_id);

	if (dsp_priv->high_freq_scene_status == 0) {
		if (dsp_priv->low_freq_scene_status == 0) {
			hi64xx_dsp_stop();
		}
		hi64xx_hifi_set_pll(false);
	}

	OUT_FUNCTION;
}

static int hi64xx_request_low_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (scene_id >= LOW_FREQ_SCENE_BUTT) {
		HI64XX_DSP_ERROR("unknow scene for mad pll: %u\n", scene_id);
		return -EINVAL;
	}

	if ((dsp_priv->low_freq_scene_status & (1 << scene_id)) != 0) {
		HI64XX_DSP_WARNING("scene: %u is alread started.\n", scene_id);
		return REDUNDANT;
	}

	if (dsp_priv->low_freq_scene_status == 0) {
		hi64xx_hifi_set_low_pll(true);
		hi64xx_dsp_run();
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);
	}

	dsp_priv->low_freq_scene_status |= (1 << scene_id);

	HI64XX_DSP_INFO("low scene: 0x%x\n", dsp_priv->low_freq_scene_status);

	OUT_FUNCTION;

	return OK;
}

static int hi64xx_release_low_pll_resource(unsigned int scene_id)
{
	IN_FUNCTION;

	if (scene_id >= LOW_FREQ_SCENE_BUTT) {
		HI64XX_DSP_ERROR("unknow scene for low pll: %u\n", scene_id);
		return -EINVAL;
	}

	if ((dsp_priv->low_freq_scene_status & (1 << scene_id)) == 0) {
		HI64XX_DSP_WARNING("scene: %u is NOT started\n", scene_id);
		return REDUNDANT;
	}

	dsp_priv->low_freq_scene_status &= ~(1ul << scene_id);

	if (dsp_priv->low_freq_scene_status == 0) {
		if (dsp_priv->high_freq_scene_status == 0) {
			hi64xx_dsp_stop();
		}
		hi64xx_hifi_set_low_pll(false);
	}

	OUT_FUNCTION;

	return OK;
}

void hi64xx_hifi_pwr_off(void)
{
	hi64xx_dsp_stop();
	if (dsp_priv->dsp_config.dsp_ops.runstall)
		dsp_priv->dsp_config.dsp_ops.runstall(false);
}

static int check_dp_clk(void)
{
	if (!dsp_priv->dsp_config.dsp_ops.check_dp_clk) {
		HI64XX_DSP_WARNING("cannot check dp clk\n");
		return OK;
	}

	if (dsp_priv->dsp_config.dsp_ops.check_dp_clk()) {
		return OK;
	}

	return -EPERM;
}

static void dsp_to_ap_msg_proc(unsigned char * msg_buff)
{
	struct pa_buffer_reverse_msg *reverse_msg = NULL;

	unsigned int msg_id;
	void *msg_body;

	msg_id = *(unsigned int *)(msg_buff + 4); /*lint !e826*/
	msg_body = (void *)(msg_buff + 8);

	HI64XX_DSP_INFO("msg id:0x%x\n", msg_id);
	switch(msg_id) {
	case ANC_MSG_START_HOOK:
		anc_beta_start_hook();
		break;
	case ANC_MSG_STOP_HOOK:
		anc_beta_stop_hook();
		break;
	case ANC_MSG_TRIGGER_DFT:
		anc_beta_log_upload(msg_body);
		break;
	case DSM_MSG_PARAM:
	case DSM_MSG_MONO_STATIC0:
		dsm_beta_dump_file(msg_body, true); /*lint !e747*/
		dsm_beta_log_upload(msg_body);
		break;
	case DSM_MSG_DUAL_STATIC0:
		dsm_beta_dump_file(msg_body, true); /*lint !e747*/
		break;
	case DSM_MSG_DUAL_STATIC1:
		dsm_beta_dump_file(msg_body, false); /*lint !e747*/
		dsm_beta_log_upload(msg_body);
		break;
	case PA_MSG_BUFFER_REVERSE:
		reverse_msg = (struct pa_buffer_reverse_msg *)(msg_buff + 4); /*lint !e826*/
		HI64XX_DSP_ERROR("pa count:%u, proc time:%u00us\n",
			reverse_msg->pa_count, reverse_msg->proc_interval);
		audio_dsm_report_info(AUDIO_CODEC, DSM_CODEC_HIFI_TIMEOUT, "pa count:%u, proc time:%u00us\n",
				reverse_msg->pa_count, reverse_msg->proc_interval);
		break;
	default:
		break;
	}
}

static void hi64xx_msg_proc_work(struct work_struct *work)
{
	unsigned char rev_msg[MAX_MSG_SIZE] = {0};

	BUG_ON(NULL == dsp_priv);
	UNUSED_PARAMETER(work);

	/* |~active flag~|~msg body~| */
	/* |~~~~4 byte~~~|~124 byte~| */

	if (dsp_priv->dsp_config.rev_msg_addr) {
		hi64xx_request_low_pll_resource(LOW_FREQ_SCENE_MSG_PROC);

		hi64xx_read(dsp_priv->dsp_config.rev_msg_addr, rev_msg, MAX_MSG_SIZE);

		if (HI64XX_DSP_TO_AP_MSG_ACTIVE != *(unsigned int *)rev_msg) {
			HI64XX_DSP_ERROR("msg proc status err:0x%x\n", *(unsigned int *)rev_msg);
			hi64xx_release_low_pll_resource(LOW_FREQ_SCENE_MSG_PROC);
			return;
		}

		dsp_to_ap_msg_proc(rev_msg);
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.rev_msg_addr, HI64XX_DSP_TO_AP_MSG_DEACTIVE);

		hi64xx_release_low_pll_resource(LOW_FREQ_SCENE_MSG_PROC);
	}

	return;
}

static void hi64xx_wakeup_dsp_res_handle(void)
{
	if (dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass)
		dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass(HI64XX_HIFI_DSP_IF_PORT_1, false);

	if (dsp_priv->dsp_config.dsp_ops.mad_enable)
		dsp_priv->dsp_config.dsp_ops.mad_enable();

	if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_PA))
		&& (dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_OM_HOOK))) {
		HI64XX_DSP_WARNING("pa & wake up exist, can not hook.\n");
		hi64xx_stop_hook();
	}
}

static int hi64xx_wakeup_res_handle(int scene)
{
	int ret = OK;

	switch(scene) {
	case WAKEUP_SCENE_PLL_CLOSE:
		if (dsp_priv->low_freq_scene_status & (1 << LOW_FREQ_SCENE_MULTI_WAKE_UP)) {
			dsp_priv->low_freq_scene_status &= ~(1ul << LOW_FREQ_SCENE_MULTI_WAKE_UP);
		} else {
			hi64xx_release_low_pll_resource(LOW_FREQ_SCENE_WAKE_UP);
		}
		break;
	case WAKEUP_SCENE_DSP_CLOSE:
		if (!(dsp_priv->low_freq_scene_status & (1 << LOW_FREQ_SCENE_MULTI_WAKE_UP))) {
			if (!(dsp_priv->low_freq_scene_status & (1 << LOW_FREQ_SCENE_WAKE_UP))) {
				HI64XX_DSP_WARNING("scene wakeup is NOT opened.\n");
				ret = REDUNDANT;
				return ret;
			}
			if (dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass)
				dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass(HI64XX_HIFI_DSP_IF_PORT_1, true);
			if (dsp_priv->dsp_config.dsp_ops.mad_disable)
				dsp_priv->dsp_config.dsp_ops.mad_disable();
		}
		break;
	case WAKEUP_SCENE_PLL_OPEN:
		ret = hi64xx_request_low_pll_resource(LOW_FREQ_SCENE_WAKE_UP);
		if (ret == REDUNDANT) {
			if (dsp_priv->low_freq_scene_status & (1 << LOW_FREQ_SCENE_MULTI_WAKE_UP)) {
				ret = ERROR;
				break;
			}
			ret = OK;
			dsp_priv->low_freq_scene_status |= (1 << LOW_FREQ_SCENE_MULTI_WAKE_UP);
			break;
		}

		hi64xx_wakeup_dsp_res_handle();

		break;
	default:
		HI64XX_DSP_ERROR("wake up wrong sceneid:%d\n", scene);
		ret = INVALID;
		break;
	}
	return ret;
}


/*
 * cmd_process_functions
 * */
static int hi64xx_func_if_open(struct krn_param_io_buf *param)
{
	int ret = 0;

	DSP_IF_OPEN_REQ_STRU *dsp_if_open_req = NULL;
	PCM_PROCESS_DMA_MSG_STRU *dma_msg_stru = NULL;

	IN_FUNCTION;

	ret = hi64xx_dsp_if_para_check(param);
	if (ret != OK) {
		HI64XX_DSP_ERROR("dsp if parameter invalid\n");
		goto end;
	}

	dsp_if_open_req = (DSP_IF_OPEN_REQ_STRU *)(param->buf_in);
	dma_msg_stru = &dsp_if_open_req->stProcessDMA;

	hi64xx_stop_dspif_hook();

	switch (dma_msg_stru->uwProcessId) {
// current not support HOOK
	case MLIB_PATH_WAKEUP:
		ret = hi64xx_wakeup_res_handle(WAKEUP_SCENE_PLL_OPEN);
		if (ret != OK) {
			goto end;
		}

		break;
	case MLIB_PATH_ANC:
		HI64XX_DSP_INFO("start anc\n");
		anc_beta_set_voice_hook_switch(dsp_if_open_req->uhwPerms);
		ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_ANC);
		if (ret != OK) {
			goto end;
		}
		break;
	case MLIB_PATH_ANC_DEBUG:
		HI64XX_DSP_INFO("start anc debug\n");
		ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_ANC_DEBUG);
		if (ret != OK) {
			goto end;
		}
		break;
	case MLIB_PATH_SMARTPA:
		ret = check_dp_clk();
		if (ret != OK) {
			HI64XX_DSP_ERROR("DP clk is disable, it's dangerous to send if_open\n");
			audio_dsm_report_info(AUDIO_CODEC,DSM_CODEC_HIFI_IF_OPEN_ERR,"DSM_CODEC_HIFI_IF_OPEN_WITHOUT_DPCLK\n");
			goto end;
		}
		ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_PA);
		if (ret != OK) {
			goto end;
		}

		if ((dsp_priv->low_freq_scene_status & (1 << LOW_FREQ_SCENE_WAKE_UP))
			&& (dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_OM_HOOK))) {
			HI64XX_DSP_WARNING("pa & wake up exist, can not hook.\n");
			hi64xx_stop_hook();
		}

		break;
	case MLIB_PATH_IR_LEARN:
		HI64XX_DSP_INFO("start hi64xx ir learn\n");

		ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_IR_LEARN);
		if (ret != OK) {
			goto end;
		}

		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_OM_HOOK))) {
			HI64XX_DSP_WARNING("in ir learn, can not hook.\n");
			hi64xx_stop_hook();
		}
		break;
	case MLIB_PATH_IR_TRANS:
		HI64XX_DSP_INFO("start hi64xx ir transmit\n");

		ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_IR_TRANS);
		if (ret != OK) {
			goto end;
		}

		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_OM_HOOK))) {
			HI64XX_DSP_WARNING("in ir transmit, can not hook.\n");
			hi64xx_stop_hook();
		}
		break;
	default:
		HI64XX_DSP_ERROR("ProcessId:%u unsupport\n", dma_msg_stru->uwProcessId);
		ret = -EPERM;
		goto end;
	}

	hi64xx_dsp_if_sample_rate_set((char *)param->buf_in);
	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);
	ret = hi64xx_sync_write(param->buf_in, param->buf_size_in);
	if (ret != OK) {
		goto end;
	}

end:
	OUT_FUNCTION;
	return ret;
}

static int hi64xx_func_if_close(struct krn_param_io_buf *param)
{
	int ret = 0;
	DSP_IF_OPEN_REQ_STRU *dsp_if_open_req = NULL;
	PCM_PROCESS_DMA_MSG_STRU *dma_msg_stru = NULL;

	IN_FUNCTION;

	ret = hi64xx_dsp_if_para_check(param);
	if (ret != OK) {
		HI64XX_DSP_ERROR("dsp if parameter invalid\n");
		goto end;
	}

	dsp_if_open_req = (DSP_IF_OPEN_REQ_STRU *)(param->buf_in);
	dma_msg_stru = &dsp_if_open_req->stProcessDMA;

	if (dma_msg_stru->uwProcessId == MLIB_PATH_WAKEUP) {
		if (hi64xx_wakeup_res_handle(WAKEUP_SCENE_DSP_CLOSE) != 0)
			goto end;
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_SMARTPA) {
		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_PA)) == 0) {
			HI64XX_DSP_WARNING("scene smartpa is NOT opened.\n");
			ret = REDUNDANT;
			goto end;
		}
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_ANC) {
		HI64XX_DSP_INFO("stop anc\n");
		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_ANC)) == 0) {
			HI64XX_DSP_WARNING("scene ANC is NOT opened.\n");
			ret = REDUNDANT;
			goto end;
		}
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_ANC_DEBUG) {
		HI64XX_DSP_INFO("stop anc debug\n");
		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_ANC_DEBUG)) == 0) {
			HI64XX_DSP_WARNING("scene anc debug is NOT opened.\n");
			ret = REDUNDANT;
			goto end;
		}
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_IR_LEARN) {
		HI64XX_DSP_INFO("stop hi64xx ir learn\n");
		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_IR_LEARN)) == 0) {
			HI64XX_DSP_WARNING("scene ir is NOT opened.\n");
			ret = REDUNDANT;
			goto end;
		}
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_IR_TRANS) {
		HI64XX_DSP_INFO("stop hi64xx ir transmit\n");
		if ((dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_IR_TRANS)) == 0) {
			HI64XX_DSP_WARNING("scene ir is NOT opened.\n");
			ret = REDUNDANT;
			goto end;
		}
	} else {
		HI64XX_DSP_ERROR("ProcessId:%u unsupport\n", dma_msg_stru->uwProcessId);
		ret = -EINVAL;
		goto end;
	}

	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd1_addr, dsp_priv->pll_state);
	ret = hi64xx_sync_write(param->buf_in, param->buf_size_in);
	if (ret != OK) {
		HI64XX_DSP_ERROR("sync_write ret=%d\n", ret);
		goto end;
	}

	if (dma_msg_stru->uwProcessId == MLIB_PATH_WAKEUP) {
		(void)hi64xx_wakeup_res_handle(WAKEUP_SCENE_PLL_CLOSE);
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_SMARTPA) {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_PA);
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_ANC) {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_ANC);
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_ANC_DEBUG) {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_ANC_DEBUG);
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_IR_LEARN) {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_IR_LEARN);
	} else if (dma_msg_stru->uwProcessId == MLIB_PATH_IR_TRANS) {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_IR_TRANS);
	}
end:
	OUT_FUNCTION;

	return ret;
}

static int hi64xx_func_fastmode(struct krn_param_io_buf *param)
{
	int ret = OK;
	unsigned short fast_mode_enable = 0;
	FAST_TRANS_MSG_STRU *fast_mode_msg = NULL;

	IN_FUNCTION;

	if (param->buf_size_in < sizeof(FAST_TRANS_MSG_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	fast_mode_msg = (FAST_TRANS_MSG_STRU *)param->buf_in;
	fast_mode_enable = fast_mode_msg->fastTransEnable;
	ret = hi64xx_fast_mode_set(fast_mode_enable);

	hi64xx_request_low_pll_resource(LOW_FREQ_SCENE_FAST_TRANS_SET);

	ret += hi64xx_sync_write(param->buf_in, param->buf_size_in);

	hi64xx_release_low_pll_resource(LOW_FREQ_SCENE_FAST_TRANS_SET);

	return ret;
}

static int hi64xx_func_para_set(struct krn_param_io_buf *param)
{
	int ret = OK;
	MLIB_PARA_SET_REQ_STRU *mlib_para = NULL;
	struct MlibParameterST *mlib_para_content = NULL;

	IN_FUNCTION;

	if (!param) {
		HI64XX_DSP_ERROR("param is null\n");
		return -EINVAL;
	}

	if (!param->buf_in || 0 == param->buf_size_in) {
		HI64XX_DSP_ERROR("input buf_in or buf_size_in[%u] error\n", param->buf_size_in);
		return -EINVAL;
	}

	if (param->buf_size_in < sizeof(MLIB_PARA_SET_REQ_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	mlib_para = (MLIB_PARA_SET_REQ_STRU *)param->buf_in;

	mlib_para_content= (struct MlibParameterST *)mlib_para->aucData;

	if (0 == mlib_para->uwSize
		|| mlib_para->uwSize > (param->buf_size_in - sizeof(MLIB_PARA_SET_REQ_STRU))) {
		HI64XX_DSP_ERROR("mlib_para size is out of range.\n");
		return -EINVAL;
	}

	if (MLIB_ST_PARA_TRANSACTION == mlib_para_content->key) {
		hi64xx_request_low_pll_resource(LOW_FREQ_SCENE_SET_PARA);
	} else {
		(void)hi64xx_request_pll_resource(HI_FREQ_SCENE_SET_PARA);
	}

	ret = hi64xx_write_mlib_para(mlib_para->aucData, mlib_para->uwSize);
	if (ret != OK) {
		HI64XX_DSP_ERROR("write mlib para failed\n");
		goto end;
	}

	ret = hi64xx_sync_write(param->buf_in, sizeof(MLIB_PARA_SET_REQ_STRU));
	if (ret != OK) {
		HI64XX_DSP_ERROR("sync write failed\n");
		goto end;
	}

end:
	if (MLIB_ST_PARA_TRANSACTION == mlib_para_content->key) {
		hi64xx_release_low_pll_resource(LOW_FREQ_SCENE_SET_PARA);
	} else {
		hi64xx_release_pll_resource(HI_FREQ_SCENE_SET_PARA);
	}
	OUT_FUNCTION;
	return ret;
}

static int hi64xx_func_para_get(struct krn_param_io_buf *param)
{
	int ret = OK;
	MLIB_PARA_GET_REQ_STRU *mlib_para = NULL;

	IN_FUNCTION;

	if (!param) {
		HI64XX_DSP_ERROR("input param is null\n");
		return -EINVAL;
	}

	if (!param->buf_in || 0 == param->buf_size_in) {
		HI64XX_DSP_ERROR("input buf_in or buf_size_in[%u] error\n", param->buf_size_in);
		return -EINVAL;
	}

	if (!param->buf_out || 0 == param->buf_size_out) {
		HI64XX_DSP_ERROR("input buf_out or buf_size_out[%u] error\n", param->buf_size_out);
		return -EINVAL;
	}

	if (param->buf_size_in < sizeof(MLIB_PARA_GET_REQ_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	(void)hi64xx_request_pll_resource(HI_FREQ_SCENE_GET_PARA);

	if (param->buf_size_in > sizeof(MLIB_PARA_GET_REQ_STRU)) {
		mlib_para = (MLIB_PARA_GET_REQ_STRU *)param->buf_in;/*lint !e826*/
		ret = hi64xx_write_mlib_para(mlib_para->aucData,
				param->buf_size_in - (unsigned int)sizeof(MLIB_PARA_GET_REQ_STRU));
		if (ret != OK) {
			HI64XX_DSP_ERROR("write mlib para failed\n");
			goto end;
		}
	}

	ret = hi64xx_sync_write(param->buf_in, (unsigned int)sizeof(MLIB_PARA_GET_REQ_STRU));
	if (ret != OK) {
		HI64XX_DSP_ERROR("sync write failed\n");
		goto end;
	}

	if (param->buf_size_out <= RESULT_SIZE) {
		HI64XX_DSP_ERROR("not enough space for para get\n");
		goto end;
	}

	/* skip buffer that record result */
	ret = hi64xx_read_mlib_para(param->buf_out + RESULT_SIZE,
				param->buf_size_out - RESULT_SIZE);
	if (ret != OK) {
		HI64XX_DSP_ERROR("read para failed\n");
		goto end;
	}

end:
	hi64xx_release_pll_resource(HI_FREQ_SCENE_GET_PARA);

	OUT_FUNCTION;

	return ret;
}

static int hi64xx_func_fasttrans_config(struct krn_param_io_buf *param)
{
	int ret = OK;
	unsigned int status = 0;
	bool fm_status = false;
	FAST_TRANS_CFG_REQ_STRU* pFastCfg = NULL;

	IN_FUNCTION;

	if (!param) {
		HI64XX_DSP_ERROR("input param is null\n");
		return -EINVAL;
	}

	if (!param->buf_in || 0 == param->buf_size_in) {
		HI64XX_DSP_ERROR("input buf_in or buf_size_in[%u] error\n", param->buf_size_in);
		return -EINVAL;
	}

	if (param->buf_size_in < sizeof(FAST_TRANS_CFG_REQ_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	pFastCfg = (FAST_TRANS_CFG_REQ_STRU*)(param->buf_in);
	status = (unsigned int)(pFastCfg->swStatus);
	if ((status & 0x1u) == 0x1) {
		fm_status = true;
	}

	HI64XX_DSP_INFO("hi64xx_func_fasttrans_config [%d]\n", pFastCfg->uhwMsgId);

	if (ID_AP_DSP_FASTTRANS_OPEN == pFastCfg->uhwMsgId) {
		dsp_priv->dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl(true, fm_status);
	} else {
		dsp_priv->dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl(false, fm_status);
	}

	ret = hi64xx_sync_write(param->buf_in, param->buf_size_in);
	if (ret != OK) {
		HI64XX_DSP_ERROR("sync write failed\n");
		goto end;
	}

end:
	OUT_FUNCTION;
	return ret;
}

void hi64xx_soundtrigger_close_codec_dma(void)
{
	FAST_TRANS_CFG_REQ_STRU FastCfg = {0};

	IN_FUNCTION;
	HI64XX_DSP_INFO("soundtrigger exception,release codec dma\n");
	FastCfg.uhwMsgId = ID_AP_DSP_FASTTRANS_CLOSE;
	if (dsp_priv && dsp_priv->dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl)
		dsp_priv->dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl(false, 0);
	if (hi64xx_sync_write(&FastCfg, sizeof(FastCfg)) != OK) {
		HI64XX_DSP_ERROR("sync write failed\n");
	}
	OUT_FUNCTION;
}

static void release_requested_pll(void)
{
	int i = 0;

	for(i = 0; i < HI_FREQ_SCENE_BUTT; i++) {
		hi64xx_release_pll_resource(i);
	}

	for(i = 0; i < LOW_FREQ_SCENE_BUTT; i++) {
		hi64xx_release_low_pll_resource(i);
	}

}

static void hi64xx_reset_ir_path(void)
{
	/* reset ir path */
	if (dsp_priv->dsp_config.dsp_ops.ir_path_clean)
		dsp_priv->dsp_config.dsp_ops.ir_path_clean();

	if (dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl)
		dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl(true);

	if (dsp_priv->dsp_config.dsp_ops.runstall)
		dsp_priv->dsp_config.dsp_ops.runstall(false);

	if (dsp_priv->dsp_config.dsp_ops.deinit)
		dsp_priv->dsp_config.dsp_ops.deinit();

	if (dsp_priv->dsp_config.dsp_ops.init)
		dsp_priv->dsp_config.dsp_ops.init();
}

static void hi64xx_clear_log_region(void)
{
	unsigned int codec_log_addr;
	unsigned int codec_log_size;

	codec_log_addr = hi64xx_misc_get_log_dump_addr();
	if (0 == codec_log_addr) {
		HI64XX_DSP_ERROR("get codec log dump addr failed\n");
		return;
	}

	codec_log_size = hi64xx_misc_get_log_dump_size();
	if (0 == codec_log_size) {
		HI64XX_DSP_ERROR("get codec log dump size fialed\n");
		return;
	}

	hi64xx_memset(codec_log_addr, (size_t)codec_log_size);
}

static void hi64xx_config_usb(void)
{
	if (dsp_priv->dsp_config.dsp_ops.config_usb_low_power)
		dsp_priv->dsp_config.dsp_ops.config_usb_low_power();
}

static int hi64xx_func_fw_download(struct krn_param_io_buf *param)
{
	char *fw_name = NULL;
	const struct firmware *fw = NULL;
	FW_DOWNLOAD_STRU *dsp_fw_download = NULL;

	int ret = 0;
	long ret_l = 0;
	int i = 0;
	static int reload_retry_count = 0;

	IN_FUNCTION;

	if (!param) {
		HI64XX_DSP_ERROR("input param is null\n");
		return -EINVAL;
	}

	if (!param->buf_in) {
		HI64XX_DSP_ERROR("input buf_in is null\n");
		return -EINVAL;
	}

	if (param->buf_size_in != sizeof(FW_DOWNLOAD_STRU)) {
		HI64XX_DSP_ERROR("input size:%u invalid\n", param->buf_size_in);
		return -EINVAL;
	}

	/* request dsp firmware */
	dsp_fw_download = (FW_DOWNLOAD_STRU *)(param->buf_in);

	fw_name = dsp_fw_download->chwname;
	if (dsp_fw_download->chwname[CODECDSP_FW_NAME_MAX_LENGTH - 1] != '\0') {
		HI64XX_DSP_ERROR("firmware name error\n");
		return -EINVAL;
	}

	ret = request_firmware(&fw, fw_name, dsp_priv->p_irq->dev);
	if (ret != 0) {
		dev_err(dsp_priv->p_irq->dev, "Failed to request dsp image(%s): %d\n", fw_name, ret);
		return ret;
	}
	if (!fw) {
		HI64XX_DSP_ERROR("fw is null\n");
		return -ENOENT;
	}

	/* release all requeseted PLL first beacuse codec dsp maybe request PLL but didn't release when exception */
	release_requested_pll();

	if (HI64XX_CODEC_TYPE_6402 != dsp_priv->dsp_config.codec_type)
		hi64xx_hifi_om_hook_stop();

	hi64xx_hifi_set_pll(true);

	/* restore dsp_if work status */
	for(i = 0; i < HI64XX_HIFI_DSP_IF_PORT_BUTT;i++) {
		if (dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass) {
			dsp_priv->dsp_config.dsp_ops.dsp_if_set_bypass(i, true);
		}
	}

	hi64xx_reset_ir_path();

	dsp_priv->is_watchdog_coming = false;
	dsp_priv->is_sync_write_timeout = false;

	if (dsp_priv->dsp_config.dsp_ops.clk_enable)
		dsp_priv->dsp_config.dsp_ops.clk_enable(true);
	if (dsp_priv->dsp_config.dsp_ops.ram2axi)
		dsp_priv->dsp_config.dsp_ops.ram2axi(true);

	hi64xx_clear_log_region();
	/* fixme: can't use dma mode on NEXT, but it work good on UDP */
	if (dsp_priv->dsp_config.slimbus_load) {
		HI64XX_DSP_INFO("slimbus down load\n");
		hi64xx_release_all_dma();
		hi64xx_hifi_download_slimbus(fw);
	} else {
		HI64XX_DSP_INFO("reg write down load\n");
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd2_addr, dsp_priv->uart_mode);
		hi64xx_hifi_download(fw, dsp_priv->dsp_config.bus_sel);
	}

	hi64xx_config_usb();

	release_firmware(fw);

	if (dsp_priv->dsp_config.msg_state_addr != 0)
		hi64xx_hifi_write_reg(dsp_priv->dsp_config.msg_state_addr,
			HI64xx_HIFI_MSG_STATE_CLEAR);

	/* notify dsp pwr on */
	hi64xx_hifi_write_reg(dsp_priv->dsp_config.cmd0_addr, HIFI_POWER_ON);
	dsp_priv->dsp_pwron_done = HIFI_STATE_UNINIT;

	/* irq clr,unmask*/
	if (hi64xx_hifi_read_reg(HI64xx_REG_IRQ_2)  & 0x1) {
		hi64xx_hifi_write_reg(HI64xx_REG_IRQ_2, 0x1);
	}
	hi64xx_hifi_reg_clr_bit(HI64xx_REG_IRQM_2, 0x0);

	if (dsp_priv->dsp_config.dsp_ops.runstall)
		dsp_priv->dsp_config.dsp_ops.runstall(true);

	/*wait 3s for dsp power on */
	/* todo : add a new wq */
	ret_l = wait_event_interruptible_timeout(dsp_priv->dsp_pwron_wq,
			(dsp_priv->dsp_pwron_done == HIFI_STATE_INIT), (3*HZ));/*lint !e665*/
	if (ret_l <= 0) {
		unsigned int read_res[6];

		HI64XX_DSP_ERROR("wait for dsp pwron error, ret:%ld\n", ret_l);

		read_res[0] = hi64xx_hifi_read_reg(HI64xx_REG_IRQ_0);
		read_res[1] = hi64xx_hifi_read_reg(HI64xx_REG_IRQ_1);
		read_res[2] = hi64xx_hifi_read_reg(HI64xx_REG_IRQ_2);
		read_res[3] = hi64xx_hifi_read_reg(HI64xx_REG_IRQM_0);
		read_res[4] = hi64xx_hifi_read_reg(HI64xx_REG_IRQM_1);
		read_res[5] = hi64xx_hifi_read_reg(HI64xx_REG_IRQM_2);
		HI64XX_DSP_ERROR("14:%#x, 15:%#x, 16:%#x, 17:%#x, 18:%#x, 19:%#x\n",read_res[0],read_res[1],read_res[2],read_res[3],read_res[4],read_res[5]);

		if (dsp_priv->dsp_config.msg_state_addr != 0)
			HI64XX_DSP_ERROR("dsp msg process state:0x%x\n",
				hi64xx_hifi_read_reg(dsp_priv->dsp_config.msg_state_addr));


		/* can't get codec version,reset system */
		BUG_ON(hi64xx_error_detect());
		/* after retry 3 times, reset system */
		BUG_ON(hi64xx_retry_max_detect(reload_retry_count));

		if (!(dsp_priv->is_sync_write_timeout) && (reload_retry_count <= HI64XX_RELOAD_RETRY_MAX)) {
			HI64XX_DSP_ERROR("do reset codecdsp,retry %d\n", reload_retry_count);
			if (0 == ret_l) {
				HI64XX_DSP_ERROR("wait for dsp pwron timeout, dump log and reset dsp\n");
				dsp_priv->is_sync_write_timeout = true;
				reload_retry_count++;
				audio_dsm_report_info(AUDIO_CODEC, DSM_CODEC_HIFI_LOAD_FAIL, "codec hifi load failed, retry times: %d\n", reload_retry_count);
				hi64xx_watchdog_process();
			} else {
				HI64XX_DSP_ERROR("wait event is interrupted, just reset dsp\n");
				hi64xx_watchdog_send_event();
			}
		}
	} else {
		reload_retry_count = 0;
	}

	msleep(1);
	if (dsp_priv->dsp_config.dsp_ops.ram2axi)
		dsp_priv->dsp_config.dsp_ops.ram2axi(false);
	if (dsp_priv->dsp_config.dsp_ops.clk_enable)
		dsp_priv->dsp_config.dsp_ops.clk_enable(false);
	if (dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl)
		dsp_priv->dsp_config.dsp_ops.dsp_power_ctrl(false);
	hi64xx_hifi_set_pll(false);

	OUT_FUNCTION;

	return ret;
}


bool hi64xx_check_i2s2_clk(void)
{
	if (dsp_priv->dsp_config.dsp_ops.check_i2s2_clk)
		return dsp_priv->dsp_config.dsp_ops.check_i2s2_clk();

	return false;
}

int hi64xx_func_start_hook(struct krn_param_io_buf *param)
{
	int ret = 0;

	return ret;
}

void hi64xx_stop_hook(void)
{
	int ret = 0;
	struct om_stop_hook_msg stop_hook_msg;

	if (HI64XX_CODEC_TYPE_6402 == dsp_priv->dsp_config.codec_type) {
		HI64XX_DSP_ERROR("6402 codec do not support hook data\n");
		return;
	}

	if (0 == (dsp_priv->high_freq_scene_status & (1 << HI_FREQ_SCENE_OM_HOOK))) {
		HI64XX_DSP_WARNING("om hook is not opened.\n");
		return;
	}

	stop_hook_msg.msg_id = ID_AP_DSP_HOOK_STOP;

	hi64xx_hifi_om_hook_stop();

	ret = hi64xx_sync_write(&stop_hook_msg, (unsigned int)sizeof(struct om_stop_hook_msg));
	if (ret != OK) {
		HI64XX_DSP_ERROR("sync write failed\n");
		goto end;
	}

	dsp_priv->is_dspif_hooking = false;

	HI64XX_DSP_INFO("hook stopped.\n");

end:
	hi64xx_release_pll_resource(HI_FREQ_SCENE_OM_HOOK);

}

int hi64xx_func_stop_hook(struct krn_param_io_buf *param)
{
	int ret = 0;

	IN_FUNCTION;


	OUT_FUNCTION;

	return ret;
}


static cmd_process_func hi64xx_select_func(const unsigned char *arg,
					   const struct cmd_func_pair *func_map,
					   const unsigned int func_map_size)
{
	unsigned int i = 0;
	unsigned short msg_id;

	if (!arg) {
		HI64XX_DSP_ERROR("arg is null\n");
		return (cmd_process_func)NULL;/*lint !e611*/
	}

	msg_id = *(unsigned short*)arg;

	IN_FUNCTION;
	for (i = 0; i < func_map_size; i++) {
		if (func_map[i].cmd_id == msg_id) {
			HI64XX_DSP_INFO("cmd:%s\n", func_map[i].cmd_name);
			return func_map[i].func;
		}
	}

	HI64XX_DSP_ERROR("cmd_process_func for id:%d not found!\n", msg_id);

	OUT_FUNCTION;
	return (cmd_process_func)NULL;/*lint !e611*/
}

static struct cmd_func_pair sync_cmd_func_map[] = {
	{ ID_AP_DSP_IF_OPEN,         hi64xx_func_if_open,        "ID_AP_DSP_IF_OPEN"},
	{ ID_AP_DSP_IF_CLOSE,        hi64xx_func_if_close,       "ID_AP_DSP_IF_CLOSE"},
	{ ID_AP_DSP_PARAMETER_SET,   hi64xx_func_para_set,       "ID_AP_DSP_PARAMETER_SET"},
	{ ID_AP_DSP_PARAMETER_GET,   hi64xx_func_para_get,       "ID_AP_DSP_PARAMETER_GET"},
	{ ID_AP_DSP_FASTMODE,        hi64xx_func_fastmode,       "ID_AP_DSP_FASTMODE"},
	{ ID_AP_IMGAE_DOWNLOAD,      hi64xx_func_fw_download,    "ID_AP_IMGAE_DOWNLOAD"},
	{ ID_AP_DSP_FASTTRANS_OPEN,  hi64xx_func_fasttrans_config, "ID_AP_DSP_FASTTRANS_OPEN"},
	{ ID_AP_DSP_FASTTRANS_CLOSE, hi64xx_func_fasttrans_config, "ID_AP_DSP_FASTTRANS_CLOSE"},
	{ ID_AP_DSP_HOOK_START,      hi64xx_func_start_hook,     "ID_AP_DSP_HOOK_START"},
	{ ID_AP_DSP_HOOK_STOP,       hi64xx_func_stop_hook,      "ID_AP_DSP_HOOK_STOP"},
};


static int hi64xx_hifi_sync_cmd(unsigned long arg)
{
	int ret = OK;
	cmd_process_func func = NULL;
	struct misc_io_sync_param param;
	struct krn_param_io_buf krn_param;
	unsigned short msg_id = 0;

	IN_FUNCTION;

	memset(&param, 0, sizeof(param));/* unsafe_function_ignore: memset */
	memset(&krn_param, 0, sizeof(krn_param));/* unsafe_function_ignore: memset */

	if (copy_from_user(&param, (void __user *)arg,
			   sizeof(struct misc_io_sync_param))) {
		HI64XX_DSP_ERROR("copy_from_user fail.\n");
		ret = -EFAULT;
		goto end;
	}

	ret = hi64xx_alloc_output_param_buffer(param.para_size_out,
			INT_TO_ADDR(param.para_out_l, param.para_out_h),
			&krn_param.buf_size_out,
			(void **)&krn_param.buf_out);
	if (ret != OK) {
		HI64XX_DSP_ERROR("alloc output buffer failed.\n");
		goto end;
	}

	ret = hi64xx_get_input_param(param.para_size_in,
			INT_TO_ADDR(param.para_in_l, param.para_in_h),
			&krn_param.buf_size_in,
			(void **)&krn_param.buf_in);
	if (ret != OK) {
		HI64XX_DSP_ERROR("get_input_param ret=%d\n", ret);
		goto end;
	}

	msg_id = *(unsigned short*)krn_param.buf_in;
	if ((msg_id != ID_AP_IMGAE_DOWNLOAD && msg_id != ID_AP_DSP_UARTMODE)
		&& dsp_priv->dsp_pwron_done == HIFI_STATE_UNINIT) {
		HI64XX_DSP_ERROR("codec dsp firmware not load,cmd:%d not send\n", msg_id);
		goto end;
	}

	func = hi64xx_select_func(krn_param.buf_in, sync_cmd_func_map,
				  ARRAY_SIZE(sync_cmd_func_map));
	if (func == NULL) {
		HI64XX_DSP_ERROR("select_func error.\n");
		ret = -EINVAL;
		goto end;
	}

	ret = func(&krn_param);
	if (ret != OK) {
		/* don't print err if redundant cmd was received */
		if (ret != REDUNDANT) {
			HI64XX_DSP_ERROR("func process error.\n");
		}
		goto end;
	}

	/* write result to out buf */
	BUG_ON(krn_param.buf_out == NULL);
	if (krn_param.buf_size_out >= sizeof(int)) {
		*(int *)krn_param.buf_out = ret;
	} else {
		HI64XX_DSP_ERROR("not enough space to save result\n");
		goto end;
	}

	/* copy result to user space */
	ret = hi64xx_put_output_param(param.para_size_out,
			INT_TO_ADDR(param.para_out_l, param.para_out_h),
			krn_param.buf_size_out,
			(void *)krn_param.buf_out);
	if (ret != OK) {
		HI64XX_DSP_ERROR("copy result to user failed\n");
		goto end;
	}

end:
	hi64xx_param_free((void **)&krn_param.buf_in);
	hi64xx_param_free((void **)&krn_param.buf_out);

	OUT_FUNCTION;

	return ret;
}


/* dump 64xxdsp manually, debug only */
#define DUMP_DIR_LEN  128
#define DUMP_DIR_ROOT "/data/hisi_logs/hi64xxdump/" /* length:28 */

#define OM_HI64XX_DUMP_RAM_LOG_PATH "codechifi_ram_logs/"
#define OM_HI64XX_DUMP_OCRAM_NAME   "ocram.bin"
#define OM_HI64XX_DUMP_IRAM_NAME    "iram.bin"
#define OM_HI64XX_DUMP_DRAM_NAME    "dram.bin"

#define OM_HI64XX_LOG_PATH          "codechifi_logs/"
#define DUMP_OCRAM_FILE_NAME        "codec_ocram.bin"
#define DUMP_LOG_FILE_NAME          "codec_log.bin"
#define DUMP_REG_FILE_NAME          "codec_reg.bin"

enum {
	DUMP_TYPE_WHOLE_OCRAM,
	DUMP_TYPE_WHOLE_IRAM,
	DUMP_TYPE_WHOLE_DRAM,
	DUMP_TYPE_PRINT_LOG,
	DUMP_TYPE_PANIC_LOG,
	DUMP_TYPE_REG,
};

/*****************************************************************************
 * misc driver
 * */
static int hi64xx_hifi_misc_open(struct inode *finode, struct file *fd)
{
	return 0;
}

static int hi64xx_hifi_misc_release(struct inode *finode, struct file *fd)
{
	return 0;
}



static long hi64xx_hifi_misc_ioctl(struct file *fd,
                                   unsigned int cmd,
                                   unsigned long arg)
{
	int ret = 0;

	IN_FUNCTION;

	if (NULL == (void __user *)arg) {
		HI64XX_DSP_ERROR("input error: arg is NULL\n");
		return -EINVAL;
	}

	mutex_lock(&dsp_priv->msg_mutex);
	switch(cmd) {
		case HI6402_HIFI_MISC_IOCTL_SYNCMSG:
			HI64XX_DSP_DEBUG("ioctl: HIFI_MISC_IOCTL_SYNCMSG\n");
			ret = hi64xx_hifi_sync_cmd(arg);
			break;
		default:
			HI64XX_DSP_ERROR("ioctl: Invalid CMD =0x%x\n", cmd);
			//TODO: should return a meaningful value
			ret = -1;
			break;
	}
	mutex_unlock(&dsp_priv->msg_mutex);

	HI64XX_DSP_INFO("ioctl: ret %d\n",ret);
	OUT_FUNCTION;

	return (long)ret;
}

static long hi64xx_hifi_misc_ioctl32(struct file *fd,
                                   unsigned int cmd,
                                   unsigned long arg)
{
	void __user *user_ptr = (void __user *)compat_ptr(arg);

	return hi64xx_hifi_misc_ioctl(fd, cmd, (unsigned long)user_ptr);
}


static const struct file_operations hi64xx_hifi_misc_fops = {
	.owner			= THIS_MODULE,
	.open			= hi64xx_hifi_misc_open,
	.release		= hi64xx_hifi_misc_release,
	.unlocked_ioctl 	= hi64xx_hifi_misc_ioctl,
	.compat_ioctl 		= hi64xx_hifi_misc_ioctl32,
};

static struct miscdevice hi64xx_hifi_misc_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "hi6402_hifi_misc",
	.fops	= &hi64xx_hifi_misc_fops,
};


extern int rdr_audio_write_file(char *name, char *data, u32 size);

static void hi64xx_parser_codec_reg(char* codec_reg_addr, char* dump_reg_addr, const size_t dump_reg_size)
{
	unsigned int dump_row_num = 4;
	unsigned int dump_data_bytes = 4;
	unsigned int reg_offset = 0;
	unsigned int cur_offset = 0;
	unsigned int i, j;

	if (NULL == codec_reg_addr || NULL == dump_reg_addr || dump_reg_size > MAX_DUMP_REG_SIZE) {
		HI64XX_DSP_ERROR("%s param error:codec_reg_addr:%s,dump_reg_addr:%s,dump_reg_size:%d\n",
			__func__, codec_reg_addr, dump_reg_addr, (unsigned int)dump_reg_size);
		return;
	}

	memset(dump_reg_addr, 0, dump_reg_size);/* unsafe_function_ignore: memset */

	for (i = 0; i < ARRAY_SIZE(s_reg_dump); i++) {
		snprintf(dump_reg_addr + strlen(dump_reg_addr),
			dump_reg_size - strlen(dump_reg_addr),
			"\n%s 0x%08x (0x%08x) ",
			s_reg_dump[i].name,
			s_reg_dump[i].addr,
			s_reg_dump[i].len);

		for (j = 0; j < (s_reg_dump[i].len / dump_data_bytes); j++) {
			if (0 == j % dump_row_num)
				snprintf(dump_reg_addr + strlen(dump_reg_addr),
					dump_reg_size - strlen(dump_reg_addr),
					"\n0x%08x:",
					s_reg_dump[i].addr + j * dump_data_bytes);

			snprintf(dump_reg_addr + strlen(dump_reg_addr),
				dump_reg_size - strlen(dump_reg_addr),
				"0x%08x ",
				*((unsigned int *)(codec_reg_addr + reg_offset) + j));
		}

		if (0 != s_reg_dump[i].len % dump_data_bytes) {
			cur_offset = s_reg_dump[i].len / dump_data_bytes * dump_data_bytes;

			switch (s_reg_dump[i].len % dump_data_bytes) {
			case 1:
				snprintf(dump_reg_addr + strlen(dump_reg_addr),
					dump_reg_size - strlen(dump_reg_addr),
					"0x%02x",
					*(codec_reg_addr + reg_offset + cur_offset));
				break;
			case 2:
				snprintf(dump_reg_addr + strlen(dump_reg_addr),
					dump_reg_size - strlen(dump_reg_addr),
					"0x%02x%02x",
					*(codec_reg_addr + reg_offset + cur_offset + 1),
					*(codec_reg_addr + reg_offset + cur_offset));
				break;
			case 3:
				snprintf(dump_reg_addr + strlen(dump_reg_addr),
					dump_reg_size - strlen(dump_reg_addr),
					"0x%02x%02x%02x",
					*(codec_reg_addr + reg_offset + cur_offset + 2),
					*(codec_reg_addr + reg_offset + cur_offset + 1),
					*(codec_reg_addr + reg_offset + cur_offset));
				break;
			default:
				return;
			}
		}
		reg_offset += s_reg_dump[i].len;
	}

	return;
}


static int hi64xx_save_reg_file(char *path)
{
	int ret = 0;
	size_t reg_size = 0;
	char* reg_buf = NULL;
	char* dump_reg_buf = NULL;
	size_t dump_reg_size = REG_ROW_LEN * REG_ROW_NUM;

	if (NULL == path) {
		HI64XX_DSP_ERROR("path is null, can not dump reg\n");
		return -EINVAL;
	}
	reg_size = hi64xx_get_dump_reg_size();

	reg_buf = vzalloc(reg_size);
	if (NULL == reg_buf) {
		HI64XX_DSP_ERROR("alloc reg_buf failed\n");
		return -ENOMEM;
	}
	hi64xx_misc_dump_reg(reg_buf, reg_size);

	dump_reg_buf = vzalloc(dump_reg_size);
	if (NULL == dump_reg_buf) {
		HI64XX_DSP_ERROR("alloc dump_reg_buf failed\n");
		vfree(reg_buf);
		return -ENOMEM;
	}
	hi64xx_parser_codec_reg(reg_buf, dump_reg_buf, dump_reg_size);

	ret = rdr_audio_write_file(path, dump_reg_buf, (unsigned int)strlen(dump_reg_buf) + 1);
	if (ret)
		HI64XX_DSP_ERROR("write codec reg file fail\n");
	vfree(reg_buf);
	vfree(dump_reg_buf);
	return ret;
}

struct parse_log parse_codec_log[] = {
	{0, RDR_CODECDSP_STACK_TO_MEM_SIZE, PARSER_CODEC_TRACE_SIZE, parse_hifi_trace},
	{RDR_CODECDSP_STACK_TO_MEM_SIZE, RDR_CODECDSP_CPUVIEW_TO_MEM_SIZE, PARSER_CODEC_CPUVIEW_LOG_SIZE, parse_hifi_cpuview}
};

static int hi64xx_parse_ocram_log(char *path, char* buf)
{
	unsigned int i;
	int ret = 0;
	unsigned int total_log_size = 0;
	char *full_text;
	char *parse_buff;

	if (0 == ARRAY_SIZE(parse_codec_log)) {
	    HI64XX_DSP_ERROR("no log to parse\n");
	    return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(parse_codec_log); i++) {
		total_log_size += parse_codec_log[i].parse_log_size;
	}

	full_text = vzalloc(total_log_size);
	if (NULL == full_text) {
		HI64XX_DSP_ERROR("alloc buf failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(parse_codec_log); i++) {
		parse_buff = (char*)vzalloc(parse_codec_log[i].parse_log_size);
		if (NULL == parse_buff) {
			HI64XX_DSP_ERROR("%s error: alloc parse_buff failed\n", __func__);
			vfree(full_text);
			return -ENOMEM;
		}

		ret = parse_codec_log[i].parse_func(buf + parse_codec_log[i].original_offset,
			parse_codec_log[i].original_log_size,
			parse_buff,
			parse_codec_log[i].parse_log_size,
			CODECDSP);
		if (0 == ret) {
			snprintf(full_text + strlen(full_text), total_log_size - strlen(full_text), "\n%s\n", parse_buff);/* unsafe_function_ignore: snprintf */
		} else {
			HI64XX_DSP_ERROR("%s error: parse failed, i = %d\n", __func__, i);
		}
		vfree(parse_buff);
	}

	ret = rdr_audio_write_file(path, full_text, strlen(full_text));
	if (ret)
		HI64XX_DSP_ERROR("write file fail\n");
	vfree(full_text);

	return ret;
}

static int hi64xx_dump_ram_by_type(char *filepath, unsigned int type)
{
	char dump_ram_path[DUMP_DIR_LEN] = {0};
	char *buf;
	int ret;
	unsigned int dump_addr;
	unsigned int dump_size;

	switch (type) {
		case DUMP_TYPE_WHOLE_OCRAM:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_DUMP_RAM_LOG_PATH, OM_HI64XX_DUMP_OCRAM_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			dump_addr = dsp_priv->dsp_config.ocram_start_addr;
			dump_size = dsp_priv->dsp_config.ocram_size;
			break;
		case DUMP_TYPE_WHOLE_IRAM:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_DUMP_RAM_LOG_PATH, OM_HI64XX_DUMP_IRAM_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			dump_addr = dsp_priv->dsp_config.itcm_start_addr;
			dump_size = dsp_priv->dsp_config.itcm_size;
			break;
		case DUMP_TYPE_WHOLE_DRAM:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_DUMP_RAM_LOG_PATH, OM_HI64XX_DUMP_DRAM_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			dump_addr = dsp_priv->dsp_config.dtcm_start_addr;
			dump_size = dsp_priv->dsp_config.dtcm_size;
			break;
		case DUMP_TYPE_PRINT_LOG:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_LOG_PATH, DUMP_LOG_FILE_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			dump_addr = dsp_priv->dsp_config.dump_log_addr;
			dump_size = dsp_priv->dsp_config.dump_log_size;
			break;
		case DUMP_TYPE_PANIC_LOG:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_LOG_PATH, DUMP_OCRAM_FILE_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			dump_addr = dsp_priv->dsp_config.dump_ocram_addr;
			dump_size = dsp_priv->dsp_config.dump_ocram_size;
			break;
		case DUMP_TYPE_REG:
			snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s%s", filepath, OM_HI64XX_LOG_PATH, DUMP_REG_FILE_NAME);/* [false alarm]:snprintf is safe  */ /* unsafe_function_ignore: snprintf */
			ret = hi64xx_save_reg_file(dump_ram_path);
			if (ret != 0)
				HI64XX_DSP_ERROR("save reg dump file fail,type:%u, ret:%d\n", type, ret);

			return ret;
		default:
			HI64XX_DSP_ERROR("dump type: %u err\n", type);
			return -EINVAL;
	}

	buf = vzalloc((unsigned long)dump_size);
	if (!buf) {
		HI64XX_DSP_ERROR("alloc buf failed\n");
		return -ENOMEM;
	}
	hi64xx_misc_dump_bin(dump_addr, buf, dump_size);

	if (type == DUMP_TYPE_PANIC_LOG) {
		ret = hi64xx_parse_ocram_log(dump_ram_path, buf);
		if (ret !=0) {
			HI64XX_DSP_ERROR("save ocram dump file fail,type:%u, ret:%d\n", type, ret);
			vfree(buf);
			return ret;
		}
	} else {
		ret = rdr_audio_write_file(dump_ram_path, buf, dump_size);
		if (ret)
			HI64XX_DSP_ERROR("write file fail\n");
	}

	vfree(buf);

	return 0;
}

void hi64xx_hifi_dump_with_path(char *path)
{
	if (!path) {
		HI64XX_DSP_ERROR("path is null, can not dump\n");
		return;
	}

	if (hi64xx_dump_ram_by_type(path, DUMP_TYPE_PRINT_LOG)) {
		HI64XX_DSP_ERROR("dump ocram log failed\n");
	}

	if (hi64xx_dump_ram_by_type(path, DUMP_TYPE_PANIC_LOG)) {
		HI64XX_DSP_ERROR("dump panic stack log failed\n");
	}

	if (hi64xx_dump_ram_by_type(path, DUMP_TYPE_REG)) {
		HI64XX_DSP_ERROR("dump reg file failed\n");
	}
}


static int hi64xx_sr_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		HI64XX_DSP_INFO("resume +\n");
		atomic_set(&hi64xx_in_suspend, 0);/*lint !e446 !e1058*/
		HI64XX_DSP_INFO("resume -\n");
		break;

	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		HI64XX_DSP_INFO("suspend +\n");
		atomic_set(&hi64xx_in_suspend, 1);/*lint !e446 !e1058*/
		while (1) {
			if (atomic_read(&hi64xx_in_saving))
				msleep(100);
			else
				break;
		}
		HI64XX_DSP_INFO("suspend -\n");
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int hi64xx_reboot_notifier(struct notifier_block *nb,
		unsigned long foo, void *bar)
{
	HI64XX_DSP_INFO("reboot +\n");
	atomic_set(&hi64xx_in_suspend, 1);/*lint !e446 !e1058*/
	while (1) {
		if (atomic_read(&hi64xx_in_saving))
			msleep(100);
		else
			break;
	}
	HI64XX_DSP_INFO("reboot -\n");

	return 0;
}

static int hi64xx_resmgr_notifier(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	struct pll_switch_event *switch_event = (struct pll_switch_event *)ptr;
	enum pll_state pll_state = PLL_RST;
	unsigned int regval[5];

	regval[0] = hi64xx_hifi_read_reg(HI64xx_VERSION_REG);
	regval[1] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG0);
	regval[2] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG1);
	regval[3] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG2);
	regval[4] = hi64xx_hifi_read_reg(HI64XX_CHIP_ID_REG3);

	HI64XX_DSP_INFO("event:%lu, from:%d, to:%d, version:0x%x, chipid0:0x%x, chipid1:0x%x, chipid2:0x%x, chipid3:0x%x\n",
		event, switch_event->from, switch_event->to, regval[0], regval[1], regval[2], regval[3], regval[4]);

	switch (switch_event->to) {
	case PLL_HIGH:
		pll_state = PLL_HIGH_FREQ;
		break;
	case PLL_LOW:
		pll_state = PLL_LOW_FREQ;
		break;
	case PLL_NONE:
		pll_state = PLL_PD;
		break;
	default:
		HI64XX_DSP_ERROR("unsupport pll state:%d\n", switch_event->to);
		return 0;
	}

	switch (event) {
	case PRE_PLL_SWITCH:
		hi64xx_hifi_cfg_before_pll_switch();
		break;
	case POST_PLL_SWITCH:
		hi64xx_hifi_cfg_after_pll_switch(pll_state);
		break;
	default:
		HI64XX_DSP_ERROR("err pll swtich event:%lu\n", event);
		break;
	}

	return 0;
}

void hi64xx_watchdog_send_event(void)
{
	char *envp[2] = {"codechifi_watchdog", NULL};
	HI64XX_DSP_ERROR("now reset mediaserver!\n");
	kobject_uevent_env(&dsp_priv->p_irq->dev->kobj, KOBJ_CHANGE, envp);
}

static irqreturn_t hi64xx_wtd_irq_handler(int irq, void *data)
{
	hi64xx_hifi_write_reg(HI64xx_DSP_WATCHDOG_LOCK, HI64xx_DSP_WATCHDOG_UNLOCK_WORD);
	hi64xx_hifi_write_reg(HI64xx_DSP_WATCHDOG_INTCLR, HI64xx_DSP_WATCHDOG_INTCLR_WORD);
	hi64xx_hifi_write_reg(HI64xx_DSP_WATCHDOG_CONTROL, HI64xx_DSP_WATCHDOG_CONTROL_DISABLE);
	hi64xx_hifi_write_reg(HI64xx_DSP_WATCHDOG_LOCK, HI64xx_DSP_WATCHDOG_LOCK_WORD);

	if(dsp_priv->dsp_config.dsp_ops.wtd_enable)
		dsp_priv->dsp_config.dsp_ops.wtd_enable(false);

	dsp_priv->is_watchdog_coming = true;


	hi64xx_watchdog_process();

	return IRQ_HANDLED;
}

size_t hi64xx_get_dump_reg_size(void)
{
	unsigned int i = 0;
	size_t size = 0;

	for (i = 0; i < ARRAY_SIZE(s_reg_dump); i++) {
		size += s_reg_dump[i].len;
	}

	HI64XX_DSP_INFO("dump size of 64xx regs is 0x%lx\n", size);

	return size;
}

size_t hi64xx_append_comment(char *buf, const size_t size)
{
	unsigned int i = 0;
	size_t offset = 0;
	size_t buffer_used = 0;

	for (i = 0; i < ARRAY_SIZE(s_reg_dump); i++) {
		BUG_ON(buffer_used >= size);
		snprintf(buf + buffer_used, size - buffer_used,/* unsafe_function_ignore: snprintf */
			"%s 0x%08x->0x%08lx ",
			s_reg_dump[i].name, s_reg_dump[i].addr, offset);
		offset += s_reg_dump[i].len;
		buffer_used = strlen(buf);
	}

	HI64XX_DSP_INFO("comment for reg dump size is 0x%lx\n", buffer_used);

	return buffer_used;
}

unsigned int hi64xx_misc_get_ocram_dump_addr(void)
{
	return dsp_priv->dsp_config.dump_ocram_addr;
}

unsigned int hi64xx_misc_get_ocram_dump_size(void)
{
	return dsp_priv->dsp_config.dump_ocram_size;
}

unsigned int hi64xx_misc_get_log_dump_addr(void)
{
	return dsp_priv->dsp_config.dump_log_addr;
}

unsigned int hi64xx_misc_get_log_dump_size(void)
{
	return dsp_priv->dsp_config.dump_log_size;
}

unsigned int hi64xx_misc_get_ocram_start_addr(void)
{
	return dsp_priv->dsp_config.ocram_start_addr;
}

unsigned int hi64xx_misc_get_ocram_size(void)
{
	return dsp_priv->dsp_config.ocram_size;
}

unsigned int hi64xx_misc_get_itcm_start_addr(void)
{
	return dsp_priv->dsp_config.itcm_start_addr;
}

unsigned int hi64xx_misc_get_itcm_size(void)
{
	return dsp_priv->dsp_config.itcm_size;
}

unsigned int hi64xx_misc_get_dtcm_start_addr(void)
{
	return dsp_priv->dsp_config.dtcm_start_addr;
}

unsigned int hi64xx_misc_get_dtcm_size(void)
{
	return dsp_priv->dsp_config.dtcm_size;
}

/* caller should guarantee input para valid */
/*lint -e429*/
void hi64xx_misc_dump_reg(char *buf, const size_t size)
{
	size_t i = 0;
	size_t buffer_used = 0;
	int ret = 0;

	if (!buf) {
		HI64XX_DSP_ERROR("input buf is null\n");
		return;
	}

	ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_DUMP);
	if (ret != 0) {
		HI64XX_DSP_ERROR("dump reg request pll failed\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(s_reg_dump); i++) {
		BUG_ON(buffer_used + s_reg_dump[i].len > size);
		hi64xx_read(s_reg_dump[i].addr, (unsigned char *)(buf + buffer_used), s_reg_dump[i].len);
		buffer_used += s_reg_dump[i].len;
	}

	hi64xx_release_pll_resource(HI_FREQ_SCENE_DUMP);
}

void hi64xx_misc_dump_bin(const unsigned int addr, char *buf, const size_t len)
{
	int ret = 0;

	ret = hi64xx_request_pll_resource(HI_FREQ_SCENE_DUMP);
	if (ret != 0) {
		HI64XX_DSP_ERROR("dump bin request pll failed\n");
		return;
	}

	hi64xx_read(addr, (unsigned char *)buf, len);

	hi64xx_release_pll_resource(HI_FREQ_SCENE_DUMP);
}
/*lint +e429*/

int hi64xx_hifi_misc_suspend(void)
{
	int ret = 0;

	HI64XX_DSP_INFO("suspend+\n");

	/* mad */
	if ((NULL != dsp_priv) && (PLL_LOW_FREQ == dsp_priv->pll_state) && dsp_priv->dsp_is_running) {
		if (dsp_priv->dsp_config.dsp_ops.suspend)
			ret = dsp_priv->dsp_config.dsp_ops.suspend();
	}

	HI64XX_DSP_INFO("suspend-\n");

	return ret;
}

int hi64xx_hifi_misc_resume(void)
{
	int ret = 0;

	HI64XX_DSP_INFO("resume+\n");

	/* mad */
	if ((NULL != dsp_priv) && (PLL_LOW_FREQ == dsp_priv->pll_state) && dsp_priv->dsp_is_running) {
		if (dsp_priv->dsp_config.dsp_ops.resume)
			ret = dsp_priv->dsp_config.dsp_ops.resume();
	}

	HI64XX_DSP_INFO("resume-\n");

	return ret;
}

int hi64xx_hifi_misc_init(struct snd_soc_codec *codec,
				struct hi64xx_resmgr *resmgr,
				struct hi64xx_irq *irqmgr,
				struct hi64xx_dsp_config *dsp_config)
{
	int ret = 0;

	IN_FUNCTION;

	dsp_priv = kzalloc(sizeof(*dsp_priv), GFP_KERNEL);
	if(!dsp_priv){
		pr_err("%s : kzalloc error!\n", __FUNCTION__);
		return -ENOMEM;
	}

	ret = misc_register(&hi64xx_hifi_misc_device);
	if (ret) {
		HI64XX_DSP_ERROR(" misc_register failed, ret %d\n", ret);
		kfree(dsp_priv);
		dsp_priv = NULL;
		return -EBUSY;
	}

	dsp_priv->p_irq = irqmgr;
	dsp_priv->resmgr = resmgr;
	dsp_priv->codec = codec;
	dsp_priv->is_watchdog_coming = false;
	dsp_priv->is_sync_write_timeout = false;
	dsp_priv->dsp_pwron_done = HIFI_STATE_UNINIT;
	dsp_priv->dsp_pllswitch_done = 0;
	dsp_priv->sync_msg_ret = 0;
	dsp_priv->uart_mode = UART_MODE_OFF;

	memcpy(&dsp_priv->dsp_config, dsp_config, sizeof(*dsp_config));/* unsafe_function_ignore: memcpy */

	mutex_init(&dsp_priv->peri_mutex);
	mutex_init(&dsp_priv->msg_mutex);
	mutex_init(&dsp_priv->state_mutex);

	init_waitqueue_head(&dsp_priv->dsp_pwron_wq);
	init_waitqueue_head(&dsp_priv->dsp_pllswitch_wq);
	init_waitqueue_head(&dsp_priv->sync_msg_wq);

	hi64xx_sr_nb.notifier_call = hi64xx_sr_event;
	hi64xx_sr_nb.priority = -1;
	if (register_pm_notifier(&hi64xx_sr_nb)) {
		HI64XX_DSP_ERROR(" Failed to register for PM events\n");
		goto err_exit;
	}

	hi64xx_reboot_nb.notifier_call = hi64xx_reboot_notifier;
	hi64xx_reboot_nb.priority = -1;
	if (register_reboot_notifier(&hi64xx_reboot_nb)) {
		HI64XX_DSP_ERROR(" Failed to register for reboot notifier\n");
		goto err_exit;
	}

	dsp_priv->resmgr_nb.notifier_call = hi64xx_resmgr_notifier;
	dsp_priv->resmgr_nb.priority = -1;
	if (hi64xx_resmgr_register_notifier(dsp_priv->resmgr, &dsp_priv->resmgr_nb)) {
		HI64XX_DSP_ERROR(" Failed to register for resmgr notifier\n");
		goto err_exit;
	}

	ret = hi64xx_irq_request_irq(dsp_priv->p_irq, dsp_priv->dsp_config.vld_irq_num,
				hi64xx_msg_irq_handler,
				"cmd_valid", dsp_priv);
	if (0 > ret) {
		HI64XX_DSP_ERROR("request_irq failed! \n");
		goto err_exit;
	}

	ret = hi64xx_irq_request_irq(dsp_priv->p_irq, dsp_priv->dsp_config.wtd_irq_num,
				hi64xx_wtd_irq_handler,
				"wd_irq", dsp_priv);
	if (0 > ret) {
		HI64XX_DSP_ERROR("request_irq failed! \n");
		goto err_exit;
	}

	if (HI64XX_CODEC_TYPE_6402 != dsp_priv->dsp_config.codec_type) {
		dsp_priv->msg_proc_wq = create_singlethread_workqueue("msg_proc_wq");
		if (!dsp_priv->msg_proc_wq) {
			HI64XX_DSP_ERROR("workqueue create failed\n");
			goto err_exit;
		}
		INIT_DELAYED_WORK(&dsp_priv->msg_proc_work, hi64xx_msg_proc_work);
	}


	OUT_FUNCTION;

	return 0;

err_exit:
	hi64xx_hifi_misc_deinit();
	return -ENOENT;
}
EXPORT_SYMBOL(hi64xx_hifi_misc_init);

void hi64xx_hifi_misc_deinit(void)
{
	IN_FUNCTION;

	if (!dsp_priv)
		return;

	if (dsp_priv->dsp_config.dsp_ops.deinit)
		dsp_priv->dsp_config.dsp_ops.deinit();


	if(dsp_priv->msg_proc_wq) {
		cancel_delayed_work(&dsp_priv->msg_proc_work);
		flush_workqueue(dsp_priv->msg_proc_wq);
		destroy_workqueue(dsp_priv->msg_proc_wq);
	}

	if (dsp_priv->p_irq) {
		hi64xx_irq_free_irq(dsp_priv->p_irq, dsp_priv->dsp_config.vld_irq_num, dsp_priv);
		hi64xx_irq_free_irq(dsp_priv->p_irq, dsp_priv->dsp_config.wtd_irq_num, dsp_priv);
	}

	hi64xx_resmgr_unregister_notifier(dsp_priv->resmgr, &dsp_priv->resmgr_nb);
	unregister_reboot_notifier(&hi64xx_reboot_nb);

	unregister_pm_notifier(&hi64xx_sr_nb);

	mutex_destroy(&dsp_priv->peri_mutex);
	mutex_destroy(&dsp_priv->msg_mutex);
	mutex_destroy(&dsp_priv->state_mutex);

	(void)misc_deregister(&hi64xx_hifi_misc_device);

	kfree(dsp_priv);
	dsp_priv = NULL;

	OUT_FUNCTION;
}
EXPORT_SYMBOL(hi64xx_hifi_misc_deinit);

MODULE_DESCRIPTION("hi64xx hifi misc driver");
MODULE_LICENSE("GPL");
