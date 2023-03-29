/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __HI6402_DSP_H__
#define __HI6402_DSP_H__

#include <sound/soc.h>
#include <linux/hisi/hi64xx/hi64xx_resmgr.h>
#include <linux/hisi/hi64xx/hi64xx_irq.h>
#include <linux/hisi/hi64xx/hi_cdc_ctrl.h>

#ifndef OK
#define OK            0
#endif
#define ERROR       (-1)
#define BUSY        (-2)
#define NOMEM       (-3)
#define INVALID     (-4)
#define REDUNDANT   (-5)

#define MSG_SEND_RETRIES 0
#define HIFI_SEC_MAX_NUM 32

#define INT_TO_ADDR(low,high) (void*) (unsigned long)((unsigned long long)(low) | ((unsigned long long)(high)<<32))
#define GET_LOW32(x) (unsigned int)(((unsigned long long)(unsigned long)(x))&0xffffffffULL)
#define GET_HIG32(x) (unsigned int)((((unsigned long long)(unsigned long)(x))>>32)&0xffffffffULL)

enum {
	HI_FREQ_SCENE_PA,
	HI_FREQ_SCENE_HOOK,
	HI_FREQ_SCENE_GET_PARA,
	HI_FREQ_SCENE_SET_PARA,
	HI_FREQ_SCENE_OM,
	HI_FREQ_SCENE_MAD_TEST,
	HI_FREQ_SCENE_DUMP,
	HI_FREQ_SCENE_FAULT_INJECT,
	HI_FREQ_SCENE_PWR_TEST,
	HI_FREQ_SCENE_ANC,
	HI_FREQ_SCENE_ANC_TEST,
	HI_FREQ_SCENE_ANC_DEBUG,
	HI_FREQ_SCENE_OM_HOOK,
	HI_FREQ_SCENE_DSP_DEBUG,
	HI_FREQ_SCENE_MEM_CHECK,
	HI_FREQ_SCENE_FASTTRANS,
	HI_FREQ_SCENE_IR_LEARN,
	HI_FREQ_SCENE_IR_TRANS,
	HI_FREQ_SCENE_BUTT,
};

enum {
	LOW_FREQ_SCENE_WAKE_UP,
	LOW_FREQ_SCENE_SET_PARA,
	LOW_FREQ_SCENE_FAST_TRANS_SET,
	LOW_FREQ_SCENE_PWR_TEST,
	LOW_FREQ_SCENE_MSG_PROC,
	LOW_FREQ_SCENE_MULTI_WAKE_UP,
	LOW_FREQ_SCENE_BUTT,
};

enum {
	HIFI_STATE_UNINIT,
	HIFI_STATE_INIT,
	HIFI_STATE_BUTT,
};

struct om_stop_hook_msg;

struct misc_io_async_param {
	unsigned int para_in_l;
	unsigned int para_in_h;
	unsigned int para_size_in;
};

struct misc_io_sync_param {
	unsigned int para_in_l;
	unsigned int para_in_h;
	unsigned int para_size_in;

	unsigned int para_out_l;
	unsigned int para_out_h;
	unsigned int para_size_out;
};

struct codec_io_dump_buf_param {
	unsigned int user_buf_l;        /*User space allocated memory address*/
	unsigned int user_buf_h;        /*User space allocated memory address*/
	unsigned int clear;             /*clear current log buf*/
	unsigned int buf_size;          /*User space allocated memory length*/
};

#define HI6402_HIFI_MISC_IOCTL_ASYNCMSG        _IOWR('A', 0x90, struct misc_io_async_param)
#define HI6402_HIFI_MISC_IOCTL_SYNCMSG         _IOWR('A', 0x91, struct misc_io_sync_param)
#define HI6402_HIFI_MISC_IOCTL_MLIB_TEST_MSG   _IOWR('A', 0x92, struct misc_io_sync_param)
#define HI64XX_HIFI_MISC_IOCTL_CODEC_DISPLAY_MSG      _IOWR('A', 0x93, struct codec_io_dump_buf_param)

#define FAKE_MSG_MAX_LEN 120
struct fake_dsp2ap_msg {
	unsigned int reserved;
	unsigned int msg_id;
	char msg_buf[FAKE_MSG_MAX_LEN];
};

struct fake_sync_msg {
	unsigned short msg_id;
	unsigned short reserved;
	unsigned int param1;
	unsigned int param2;
	unsigned int param3;
};

#define HI64XX_HIFI_MISC_IOCTL_KCOV_FAKE_DSP2AP_MSG   _IOWR('A', 0x94, struct fake_dsp2ap_msg)
#define HI64XX_HIFI_MISC_IOCTL_KCOV_FAKE_SYNCMSG  _IOWR('A', 0x95, struct fake_sync_msg)
#define HI64XX_HIFI_MISC_IOCTL_KCOV_FAKE_DUMP_LOG  _IOW('A', 0x96, unsigned int)

/*
 *  dsp img download
 */
enum DRV_HIFI_IMAGE_SEC_TYPE_ENUM {
	DRV_HIFI_IMAGE_SEC_TYPE_CODE = 0,                /* code section */
	DRV_HIFI_IMAGE_SEC_TYPE_DATA,                    /* data section */
	DRV_HIFI_IMAGE_SEC_TYPE_BSS,                     /* bss section */
	DRV_HIFI_IMAGE_SEC_TYPE_BUTT,
};
enum DRV_HIFI_IMAGE_SEC_LOAD_ENUM {
	DRV_HIFI_IMAGE_SEC_LOAD_STATIC = 0,             /* before dsp reset  download one time*/
	DRV_HIFI_IMAGE_SEC_LOAD_DYNAMIC,                /* maybe need download dynamic */
	DRV_HIFI_IMAGE_SEC_UNLOAD,                      /* the section do not need download*/
	DRV_HIFI_IMAGE_SEC_LOAD_BUTT,
};

struct drv_hifi_image_sec {
	unsigned short	sn;                            /* section serial number*/
	unsigned char	type;                          /* section type :code, data, bss    */
	unsigned char	load_attib;                    /* download attribute:static,dynmic,unload*/
	unsigned int	src_offset;                    /* offset of down file, bytes */
	unsigned int	des_addr;                      /* des addr , bytes */
	unsigned int	size;                          /* section length, bytes */
};

struct drv_hifi_image_head {
	char				time_stamp[24];            /* image time stamp */
	unsigned int			image_size;            /* image size, bytes */
	unsigned int			sections_num;          /* section number */
	struct drv_hifi_image_sec	sections[HIFI_SEC_MAX_NUM];      /* section head include section infomation */
};

enum pll_state {
	PLL_PD,
	PLL_HIGH_FREQ,
	PLL_LOW_FREQ,
	PLL_RST,
};

enum HI64XX_HIFI_PCM_SAMPLE_RATE {
	HI64XX_HIFI_PCM_SAMPLE_RATE_8K = 0,
	HI64XX_HIFI_PCM_SAMPLE_RATE_16K,
	HI64XX_HIFI_PCM_SAMPLE_RATE_32K,
	HI64XX_HIFI_PCM_SAMPLE_RATE_RESERVED0,
	HI64XX_HIFI_PCM_SAMPLE_RATE_48K,
	HI64XX_HIFI_PCM_SAMPLE_RATE_96K,
	HI64XX_HIFI_PCM_SAMPLE_RATE_192K,
	HI64XX_HIFI_PCM_SAMPLE_RATE_RESERVED1,
};

enum HI64XX_HIFI_PCM_DIRECT {
	HI64XX_HIFI_PCM_IN = 0,
	HI64XX_HIFI_PCM_OUT,
	HI64XX_HIFI_PCM_DIRECT_BUTT,
};

enum HI64XX_HIFI_DSP_IF_PORT {
	HI64XX_HIFI_DSP_IF_PORT_0 = 0,
	HI64XX_HIFI_DSP_IF_PORT_1,
	HI64XX_HIFI_DSP_IF_PORT_2,
	HI64XX_HIFI_DSP_IF_PORT_3,
	HI64XX_HIFI_DSP_IF_PORT_4,
	HI64XX_HIFI_DSP_IF_PORT_5,
	HI64XX_HIFI_DSP_IF_PORT_6,
	HI64XX_HIFI_DSP_IF_PORT_7,
	HI64XX_HIFI_DSP_IF_PORT_8,
	HI64XX_HIFI_DSP_IF_PORT_9,
	HI64XX_HIFI_DSP_IF_PORT_BUTT,
};

enum HI64xx_HIFI_DSP_MSG_STATE {
	HI64xx_HIFI_MSG_STATE_CLEAR = 0,
	HI64xx_HIFI_AP_SEND_MSG,
	HI64xx_HIFI_DSP_RECEIVE_MSG,
	HI64xx_HIFI_DSP_SEND_MSG_CNF,
	HI64xx_HIFI_AP_RECEIVE_MSG_CNF,
	HI64xx_HIFI_DSP_SEND_PLL_SW_OFF_CNF,
	HI64xx_HIFI_DSP_SEND_PLL_SW_ON_CNF,
	HI64xx_HIFI_AP_RECEIVE_PLL_SW_CNF,
	HI64xx_HIFI_DSP_SEND_PWRON_CNF,
	HI64xx_HIFI_AP_RECEIVE_PWRON_CNF,
	HI64xx_HIFI_MSG_STATE_BUTT
};

struct krn_param_io_buf {
	unsigned char *buf_in;
	unsigned int buf_size_in;
	/*XXX: variables below is for sync cmd only*/
	unsigned char *buf_out;
	unsigned int buf_size_out;
};

struct hi64xx_dsp_ops {
	/* init 64xx dsp regs */
	void (*init)(void);
	/* deinit 64xx dsp regs */
	void (*deinit)(void);
	/* enable 64xx dsp clk */
	void (*clk_enable)(bool);
	/* config 64xx dsp axi */
	void (*ram2axi)(bool);
	/* config runstall */
	void (*runstall)(bool);
	/* config watchdog */
	void (*wtd_enable)(bool);
	/* config uart */
	void (*uart_enable)(bool);
	/* notify 64xx dsp */
	void (*notify_dsp)(void);
	/* suspend proc */
	int (*suspend)(void);
	/* resume proc */
	int (*resume)(void);
	/* power on/off dsp */
	void (*dsp_power_ctrl)(bool);
	void (*set_dsp_div)(enum pll_state state);
	/* soundtrigger fast channel open/close */
	void (*soundtrigger_fasttrans_ctrl)(bool enable, bool fm);
	void (*dsp_if_set_bypass)(unsigned int dsp_if_id, bool enable);
	void (*mad_enable)(void);
	void (*mad_disable)(void);
	/*ir path*/
	void (*ir_path_clean)(void);
	bool (*check_dp_clk)(void);
	bool (*check_i2s2_clk)(void);
	int (*set_sample_rate)(unsigned int dsp_if_id, unsigned int sample_rate_in, unsigned int sample_rate_out);
	void (*config_usb_low_power)(void);
};

struct hi64xx_dsp_config {
	struct hi64xx_dsp_ops dsp_ops;
	bool slimbus_load;
	unsigned int codec_type;
	unsigned int msg_addr;
	unsigned int rev_msg_addr;
	unsigned int para_addr;
	unsigned int cmd0_addr;
	unsigned int cmd1_addr;
	unsigned int cmd2_addr;
	unsigned int cmd3_addr;
	unsigned int cmd4_addr;
	unsigned int wtd_irq_num;
	unsigned int vld_irq_num;
	unsigned int dump_ocram_addr;
	unsigned int dump_ocram_size;
	unsigned int dump_log_addr;
	unsigned int dump_log_size;
	unsigned int msg_state_addr;
	unsigned int wfi_state_addr;
	unsigned int ocram_start_addr;
	unsigned int ocram_size;
	unsigned int itcm_start_addr;
	unsigned int itcm_size;
	unsigned int dtcm_start_addr;
	unsigned int dtcm_size;
	unsigned int mlib_to_ap_msg_addr;
	unsigned int mlib_to_ap_msg_size;
	enum bustype_select bus_sel;
};

typedef int (*cmd_process_func)(struct krn_param_io_buf *);

struct cmd_func_pair {
	int cmd_id;
	cmd_process_func func;
	char cmd_name[50];
};


void hi64xx_memset(uint32_t dest, size_t n);
void hi64xx_memcpy(uint32_t dest, uint32_t *src, size_t n);
void hi64xx_write(const unsigned int start_addr,
			 const unsigned int *buf,
			 const unsigned int len);
void hi64xx_read(const unsigned int start_addr,
			unsigned char *arg,
			const unsigned int len);
int hi64xx_request_pll_resource(unsigned int scene_id);
void hi64xx_release_pll_resource(unsigned int scene_id);
void hi64xx_hifi_pwr_off(void);
void hi64xx_hifi_write_reg(unsigned int reg, unsigned int val);
unsigned int hi64xx_hifi_read_reg(unsigned int reg);
void hi64xx_hifi_reg_set_bit(unsigned int reg, unsigned int offset);
void hi64xx_hifi_reg_clr_bit(unsigned int reg, unsigned int offset);
void hi64xx_hifi_reg_write_bits(unsigned int reg,
				unsigned int value,
				unsigned int mask);
bool hi64xx_hifi_is_running(void);
void hi64xx_watchdog_send_event(void);
size_t hi64xx_get_dump_reg_size(void);
size_t hi64xx_append_comment(char *buf, const size_t size);
unsigned int hi64xx_misc_get_ocram_dump_addr(void);
unsigned int hi64xx_misc_get_ocram_dump_size(void);
unsigned int hi64xx_misc_get_log_dump_addr(void);
unsigned int hi64xx_misc_get_log_dump_size(void);
void hi64xx_hifi_dump_with_path(char *path);
void hi64xx_misc_dump_reg(char *buf, const size_t size);
void hi64xx_misc_dump_bin(const unsigned int addr, char *buf, const size_t len);
int hi64xx_hifi_misc_suspend(void);
int hi64xx_hifi_misc_resume(void);
int hi64xx_hifi_misc_init(struct snd_soc_codec *codec,
				struct hi64xx_resmgr *resmgr,
				struct hi64xx_irq *irqmgr,
				struct hi64xx_dsp_config *dsp_config);
void hi64xx_hifi_misc_deinit(void);
void hi64xx_hifi_misc_peri_lock(void);
void hi64xx_hifi_misc_peri_unlock(void);
int hi64xx_func_start_hook(struct krn_param_io_buf *param);
int hi64xx_func_stop_hook(struct krn_param_io_buf *param);
void hi64xx_stop_hook(void);
unsigned int hi64xx_misc_get_ocram_start_addr(void);
unsigned int hi64xx_misc_get_ocram_size(void);
unsigned int hi64xx_misc_get_itcm_start_addr(void);
unsigned int hi64xx_misc_get_itcm_size(void);
unsigned int hi64xx_misc_get_dtcm_start_addr(void);
unsigned int hi64xx_misc_get_dtcm_size(void);
void hi64xx_soundtrigger_close_codec_dma(void);
bool hi64xx_check_i2s2_clk(void);
bool hi64xx_get_sample_rate_index(unsigned int sample_rate, unsigned char *sel);
#endif/*__HI6402_DSP_H__*/
