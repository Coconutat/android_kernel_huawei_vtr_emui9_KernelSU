/*
 * hi64xx_mbhc.c -- hi64xx mbhc driver
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/switch.h>
#include <linux/version.h>
#include <dsm_audio/dsm_audio.h>
#include <linux/hisi/hi64xx/hi64xx_utils.h>
#include <linux/hisi/hi64xx/hi64xx_mbhc.h>
#include <linux/hisi/hi64xx/hi64xx_regs.h>
#include "huawei_platform/audio/anc_hs_interface.h"
#include "huawei_platform/audio/invert_hs.h"
#include <sound/soc.h>
#include "huawei_platform/audio/usb_analog_hs_interface.h"

#include "hs_auto_calib.h"

/*lint -e750 -e838 -e732 -e655 -e64*/
#define HI64xx_BTN_MASK	(SND_JACK_BTN_0 | SND_JACK_BTN_1 | SND_JACK_BTN_2 | SND_JACK_BTN_3)

#define HI64xx_HANDLE_DELAY_800_MS	(800)
#define HI64xx_HANDLE_DELAY_30_MS	(30)
#define HI64XX_CLR_IRQ_COMHL_ECO_STATUS  (0x3F)
#define EXTERN_CABLE_MBHC_VREF_DAFULT_VALUE  (0x9E)
#define HI64xx_POWERON_MICBIAS_SLEEP_30_MS (30)
#define INVALID_VOLTAGE (0xFFFFFFFF)


static int hi64xx_btn_bits[] = {
	IRQ_BTNUP_COMP1,
	IRQ_BTNDOWN_COMP1,
	IRQ_BTNUP_COMP2,
	IRQ_BTNDOWN_COMP2,
	IRQ_BTNUP_ECO,
	IRQ_BTNDOWN_ECO,
};

/* defination of private data */
struct hi64xx_mbhc_priv {
	struct hi64xx_mbhc mbhc_pub;
	struct snd_soc_codec *codec;
	struct hi64xx_resmgr *resmgr;
	struct hi64xx_irq *irqmgr;
	struct miscdevice miscdev;
	struct wake_lock wake_lock;
	struct wake_lock micbias_wake_lock;
	struct mutex plug_mutex;
	struct mutex status_mutex;
	struct mutex saradc_mutex;
	struct workqueue_struct *micbias_delay_wq;
	struct delayed_work micbias_delay_work;
	bool mbhc_micbias_work;
	bool hs_plug_status;
	bool anc_hs_plug_status;
	/* headset status */
	enum hisi_jack_states hs_status;
	int btn_report;
	int need_match_micbias;

	struct switch_dev sdev;
	/* board defination */
	struct hi64xx_mbhc_config mbhc_config;
	struct hi64xx_hs_cfg hs_cfg;
};

#define MBHC_TYPE_FAIL_MAX_TIMES             (5)
#define MBHC_TYPE_REPORT_MAX_TIMES           (20)

static unsigned int mbhc_type_fail_times = 0;
static unsigned int mbhc_type_report_times = MBHC_TYPE_REPORT_MAX_TIMES;


static void hi64xx_mbhc_dmd_fail_report(int adc)
{
	mbhc_type_fail_times ++;
	if ((mbhc_type_fail_times >= MBHC_TYPE_FAIL_MAX_TIMES) && (mbhc_type_report_times > 0)) {
		mbhc_type_fail_times = 0;
		mbhc_type_report_times --;

		audio_dsm_report_info(AUDIO_CODEC, DSM_HI6402_MBHC_HS_ERR_TYPE, "abnormal headset type! adc = [%d], total times = [%d]\n", adc,
							(MBHC_TYPE_REPORT_MAX_TIMES - mbhc_type_report_times)*MBHC_TYPE_FAIL_MAX_TIMES);

	}
	return;
}

static struct snd_soc_jack hs_jack;
void hi64xx_soc_jack_report(int report, int mask)
{
	snd_soc_jack_report(&hs_jack, report, mask);
}

void hi64xx_irq_mask_btn_irqs(struct hi64xx_mbhc *mbhc)
{
	int irq_num = sizeof(hi64xx_btn_bits)/sizeof(int);
	int *phy_irqs = hi64xx_btn_bits;

	struct hi64xx_mbhc_priv *priv =
		(struct hi64xx_mbhc_priv*)mbhc;

	if (priv == NULL) {
		pr_err("%s: null pointer \n", __FUNCTION__);
		return;
	}

	hi64xx_irq_disable_irqs(priv->irqmgr, irq_num, phy_irqs);
}

void hi64xx_irq_unmask_btn_irqs(struct hi64xx_mbhc *mbhc)
{
	int irq_num = sizeof(hi64xx_btn_bits)/sizeof(int);
	int *phy_irqs = hi64xx_btn_bits;

	struct hi64xx_mbhc_priv *priv =
		(struct hi64xx_mbhc_priv*)mbhc;

	if (priv == NULL) {
		pr_err("%s: null pointer \n", __FUNCTION__);
		return;
	}

	hi64xx_irq_enable_irqs(priv->irqmgr, irq_num, phy_irqs);
}

void hi64xx_micbias_work_func(struct work_struct *work)
{
	struct hi64xx_mbhc_priv *priv = container_of(work, struct hi64xx_mbhc_priv, micbias_delay_work.work);
	/* hs micbias off */
	hi64xx_resmgr_release_micbias(priv->resmgr);
}

static void hi64xx_hs_micbias_enable(struct hi64xx_mbhc_priv *priv, bool enable)
{
	int ret = 0;
	if (enable) {
		/* hs micbias on */
		hi64xx_resmgr_request_micbias(priv->resmgr);
	} else {
		/* hs micbias pd */
		wake_lock_timeout(&priv->micbias_wake_lock, msecs_to_jiffies(3500));
		ret = mod_delayed_work(priv->micbias_delay_wq,
			&priv->micbias_delay_work,
			msecs_to_jiffies(3000));
		if (ret != 0) {
			hi64xx_resmgr_release_micbias(priv->resmgr);
		}
	}

	return;
}

void hi64xx_micbias_enable_for_usb_ana_hs(struct hi64xx_mbhc_priv *priv, bool enable)
{
	if (NULL == priv) {
		pr_err("%s: NULL pointer.\n", __FUNCTION__);
		return;
	}

	if (enable) {
		/* hs micbias on */
		hi64xx_resmgr_request_micbias(priv->resmgr);
	} else {
		/* hs micbias pd */
		hi64xx_resmgr_release_micbias(priv->resmgr);
	}

	return;
}

void hi64xx_jack_report(struct hi64xx_mbhc_priv *priv)
{
	enum hisi_jack_states jack_status = priv->hs_status;
	int jack_report = 0;

	switch(priv->hs_status) {
	case HISI_JACK_NONE:
		jack_report = 0;
		pr_info("%s : plug out\n", __FUNCTION__);
		break;
	case HISI_JACK_HEADSET:
		jack_report = SND_JACK_HEADSET;
		pr_info("%s : 4-pole headset plug in\n", __FUNCTION__);
		break;
	case HISI_JACK_INVERT:
		jack_report = SND_JACK_HEADPHONE;
		pr_info("%s : invert headset plug in\n", __FUNCTION__);
		break;
	case HISI_JACK_HEADPHONE:
		jack_report = SND_JACK_HEADPHONE;
		pr_info("%s : 3-pole headphone plug in\n", __FUNCTION__);
		break;
	case HISI_JACK_EXTERN_CABLE:
		jack_report = 0;
		pr_info("%s : extern cable plug in and jack_report(0x%x)\n", __FUNCTION__,jack_report);
		break;
	default:
		pr_err("%s : error hs_status(%d)\n", __FUNCTION__, priv->hs_status);
		break;
	}

	/* clear btn event */
	hi64xx_soc_jack_report(0, HI64xx_BTN_MASK);
	/* btn_report jack status */
	hi64xx_soc_jack_report(jack_report, SND_JACK_HEADSET);

	switch_set_state(&priv->sdev, jack_status);
}

static inline bool check_headset_pluged_in(struct hi64xx_mbhc_priv *priv)
{
	int ret = 0;
	unsigned int irq_source_reg = priv->hs_cfg.mbhc_reg->irq_source_reg;

	if(check_usb_analog_hs_support()) {
		ret = usb_analog_hs_check_headset_pluged_in();
		if(ret == USB_ANA_HS_PLUG_IN) {
			pr_info("usb ananlog hs is PLUG_IN\n");
			return true;
		} else {
			pr_info("usb ananlog hs is PLUG_OUT\n");
			return false;
		}
	} else {
		ret = anc_hs_interface_check_headset_pluged_in();

		if(ret == NO_MAX14744) {
			/*
			* 0 : means headset is pluged out
			* 1 : means headset is pluged in
			*/
			pr_info("max14744 NO_MAX14744");
			return (0 != (snd_soc_read(priv->codec, irq_source_reg) & (1 << HI64xx_PLUGIN_IRQ_BIT)));
		} else if(ret == HANDSET_PLUG_IN) {
			pr_info("max14744 HANDSET_PLUG_IN");
			return true;
		} else{
			pr_info("max14744 HANDSET_PLUG_OUT");
			return false;
		}
	}
}

bool hi64xx_check_saradc_ready_detect(struct snd_soc_codec *codec)
{
	int value = 0;

	if (!codec) {
		return false;
	}

	/* read codec status */
	value = snd_soc_read(codec, HI64xx_REG_IRQ_2 - CODEC_BASE_ADDR) & (1 << HI64xx_SARADC_RD_BIT);

	/*clr irq*/
	hi64xx_update_bits(codec, HI64xx_REG_IRQ_2 - CODEC_BASE_ADDR,
			1 << HI64xx_SARADC_RD_BIT, 1 << HI64xx_SARADC_RD_BIT);

	if (0 == value)
		return false;

	return true;
}

static unsigned int hi64xx_get_voltage_value(struct hi64xx_mbhc_priv *priv)
{
	struct hs_mbhc_func *mbhc_func =  priv->hs_cfg.mbhc_func;
	unsigned int voltage_value = 0;

	if (!mbhc_func->hs_get_voltage) {
		pr_err("%s : cannot get voltage value\n", __FUNCTION__);
		return 0;
	}

	mutex_lock(&priv->saradc_mutex);
	voltage_value = mbhc_func->hs_get_voltage(priv->codec, priv->mbhc_config.coefficient);
	mutex_unlock(&priv->saradc_mutex);

	return voltage_value;
}

void hi64xx_hstype_identify(struct hi64xx_mbhc_priv *priv,
								int *anc_type, unsigned int voltage_value)
{

	if (priv->mbhc_config.hs_3_pole_max_voltage >= voltage_value) {
		/* 3-pole headphone */
		pr_info("%s : 3 pole is pluged in\n", __FUNCTION__);
		priv->hs_status = HISI_JACK_HEADPHONE;
		*anc_type = ANC_HS_NORMAL_3POLE;
	} else if (priv->mbhc_config.hs_4_pole_min_voltage <= voltage_value &&
			priv->mbhc_config.hs_4_pole_max_voltage >= voltage_value) {
		/* 4-pole headset */
		pr_info("%s : 4 pole is pluged in\n", __FUNCTION__);
		priv->hs_status = HISI_JACK_HEADSET;
		*anc_type = ANC_HS_NORMAL_4POLE;
	} else if (priv->mbhc_config.hs_detect_extern_cable &&
			(priv->mbhc_config.hs_extern_cable_min_voltage <= voltage_value &&
			priv->mbhc_config.hs_extern_cable_max_voltage >= voltage_value)) {
		pr_info("%s : set as extern_cable\n", __FUNCTION__);
		priv->hs_status = HISI_JACK_EXTERN_CABLE;
		*anc_type = ANC_HS_REVERT_4POLE;
	} else {
		/* invert 4-pole headset */
		pr_info("%s : need further detect, report as 3-pole headphone,adc_v:%d\n", __FUNCTION__, voltage_value);
		priv->hs_status = HISI_JACK_INVERT;
		*anc_type = ANC_HS_REVERT_4POLE;

		/* real invert headset */
		if(priv->mbhc_config.hs_4_pole_min_voltage > voltage_value) {
			invert_hs_control(INVERT_HS_MIC_GND_CONNECT);
		}
	}
}

static int check_plug_in_detect_para(struct hi64xx_mbhc_priv *priv)
{
	if (!priv) {
		pr_err("%s : priv is not exit\n", __FUNCTION__);
		return -1;
	}
	if (!priv->hs_cfg.mbhc_func) {
		pr_err("%s : mbhc func is not exit\n", __FUNCTION__);
		return -1;
	}
	if (!priv->hs_cfg.mbhc_func->hs_mbhc_on) {
		pr_err("%s : mbhc on func is not exit\n", __FUNCTION__);
		return -1;
	}

	if (!priv->hs_cfg.res_detect_func) {
		pr_err("%s : res detect on func is not exit\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static void hi64xx_hs_res_detect(struct hi64xx_mbhc_priv *priv)
{
	bool hs_res_detect = false;
	struct hs_res_detect_func *res_detect_func = priv->hs_cfg.res_detect_func;

	hs_res_detect = (NULL != res_detect_func->hs_res_detect
		&& !priv->hs_plug_status
		&& HISI_JACK_INVERT != priv->hs_status
		&& !priv->anc_hs_plug_status);
	if (hs_res_detect) {
		res_detect_func->hs_path_enable(priv->codec);
		msleep(100);
		res_detect_func->hs_res_detect(priv->codec);
		res_detect_func->hs_path_disable(priv->codec);
	} else {
		pr_info("%s : no need enable res detect, hs_plug_status:%d, anc_hs_plug_status:%d\n",
				__FUNCTION__, priv->hs_plug_status, priv->anc_hs_plug_status);
	}
}

void hi64xx_plug_in_detect(struct hi64xx_mbhc_priv *priv)
{
	unsigned int voltage_value = 0;
	int anc_type = ANC_HS_REVERT_4POLE;
	struct hs_mbhc_func *mbhc_func = NULL;

	if(check_plug_in_detect_para(priv))
		return;

	mbhc_func = priv->hs_cfg.mbhc_func;

	if (!check_headset_pluged_in(priv))
		return;

	wake_lock(&priv->wake_lock);
	mutex_lock(&priv->plug_mutex);

	pr_debug("%s(%u) : in", __FUNCTION__,__LINE__);

	invert_hs_control(INVERT_HS_MIC_GND_DISCONNECT);

	if(check_anc_hs_interface_support()) {
		//mask btn irqs while control boost
		hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
		anc_hs_interface_start_charge();
	}

	/* micbias on */
	hi64xx_hs_micbias_enable(priv, true);

	/* mbhc on */
	mbhc_func->hs_mbhc_on(priv->codec);

	/* get voltage by read sar in mbhc */
	voltage_value = hi64xx_get_voltage_value(priv);

	mutex_lock(&priv->status_mutex);

	hi64xx_hstype_identify(priv, &anc_type, voltage_value);

	mutex_unlock(&priv->status_mutex);

	if(check_anc_hs_interface_support()  && priv->hs_status == HISI_JACK_HEADSET) {
		//mask btn irqs while control boost
		hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
		priv->anc_hs_plug_status = anc_hs_interface_charge_detect(voltage_value, anc_type);
		hi64xx_irq_unmask_btn_irqs(&priv->mbhc_pub);
	}

	anc_hs_interface_refresh_headset_type(anc_type);
	/* real invert headset */
	if((priv->mbhc_config.hs_4_pole_min_voltage > voltage_value) && (priv->hs_status == HISI_JACK_INVERT)) {
		anc_hs_interface_invert_hs_control(ANC_HS_MIC_GND_CONNECT);
	}

	if(check_usb_analog_hs_support() && (priv->hs_status == HISI_JACK_INVERT ||priv->hs_status == HISI_JACK_HEADPHONE)) {
		hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
		//before change the mic/gnd, power down the MICBIAS, to avoid pop sound in hs.
		hi64xx_micbias_enable_for_usb_ana_hs(priv, false);
		usb_ana_hs_mic_swtich_change_state();
		//after change the mic/gnd, power on the MICBIAS, to identify the hs type.
		hi64xx_micbias_enable_for_usb_ana_hs(priv, true);
		msleep(HI64xx_POWERON_MICBIAS_SLEEP_30_MS);
		hi64xx_irq_unmask_btn_irqs(&priv->mbhc_pub);
		voltage_value = hi64xx_get_voltage_value(priv);
		mutex_lock(&priv->status_mutex);
		hi64xx_hstype_identify(priv, &anc_type, voltage_value);
		mutex_unlock(&priv->status_mutex);
	}

	/* hi6403 & first plugin detect & not invert headphone & not anc headphone
	   then headphone res will be detected */
	hi64xx_hs_res_detect(priv);

	if(priv->need_match_micbias == 1) {
		hi64xx_hs_micbias_enable(priv, false);
		priv->need_match_micbias = 0;
	}

	if (!check_headset_pluged_in(priv)) {
		pr_info("hi64xx_plug_in_detect: headset has been pluged out.\n");
		goto exit;
	}

	if(priv->mbhc_config.hs_4_pole_max_voltage > voltage_value) {
		hi64xx_jack_report(priv);
		priv->hs_plug_status = true;
		if(priv->mbhc_config.hs_detect_extern_cable) {
			pr_info("%s : not turn off mbhc micbias for extern cable\n", __FUNCTION__);
		} else {
			/* micbias off */
			hi64xx_hs_micbias_enable(priv, false);
		}
	} else {
		if(priv->mbhc_config.hs_detect_extern_cable) {
			priv->hs_plug_status = false;
			priv->need_match_micbias = 0;
			hi64xx_jack_report(priv);
		} else {
			priv->need_match_micbias = 1;
			hi64xx_mbhc_dmd_fail_report(voltage_value);
		}
	}

exit:
	mutex_unlock(&priv->plug_mutex);
	wake_unlock(&priv->wake_lock);
	return;
}

void hi64xx_btn_down(struct hi64xx_mbhc_priv *priv)
{
	unsigned int voltage_value = 0;

	if (!check_headset_pluged_in(priv)) {
		pr_info("%s(%u) : hs pluged out \n", __FUNCTION__, __LINE__);
		return;
	}

	wake_lock(&priv->wake_lock);

	if (HISI_JACK_HEADSET == priv->hs_status) {
		/* micbias on */
		hi64xx_hs_micbias_enable(priv, true);

		/* auto read */
		voltage_value = hi64xx_get_voltage_value(priv);

		if(priv->mbhc_config.hs_detect_extern_cable) {
			pr_info("%s : not turn off mbhc micbias for extern cable\n", __FUNCTION__);
		} else {
			/* micbias off */
			hi64xx_hs_micbias_enable(priv, false);
		}

		msleep(30);

		if (!check_headset_pluged_in(priv)) {
			pr_info("%s(%u) : hs pluged out \n", __FUNCTION__, __LINE__);
			goto end;
		}

		if ((voltage_value >= priv->mbhc_config.hs_4_pole_min_voltage) && (voltage_value <= priv->mbhc_config.hs_4_pole_max_voltage)) {
			pr_info("%s(%u) : process as btn up! \n", __FUNCTION__, __LINE__);
			mutex_lock(&priv->status_mutex);
			priv->btn_report = 0;
			mutex_unlock(&priv->status_mutex);
		} else if ((voltage_value >= priv->mbhc_config.btn_play_min_voltage) && (voltage_value <= priv->mbhc_config.btn_play_max_voltage)) {
			mutex_lock(&priv->status_mutex);
			priv->btn_report = SND_JACK_BTN_0;
			mutex_unlock(&priv->status_mutex);
		} else if (priv->mbhc_config.btn_volume_up_min_voltage < voltage_value && voltage_value <= priv->mbhc_config.btn_volume_up_max_voltage) {
			mutex_lock(&priv->status_mutex);
			priv->btn_report = SND_JACK_BTN_1;
			mutex_unlock(&priv->status_mutex);
		} else if (priv->mbhc_config.btn_volume_down_min_voltage < voltage_value && voltage_value <= priv->mbhc_config.btn_volume_down_max_voltage) {
			mutex_lock(&priv->status_mutex);
			priv->btn_report = SND_JACK_BTN_2;
			mutex_unlock(&priv->status_mutex);
		} else if ((voltage_value > priv->mbhc_config.btn_voice_assistant_min_voltage) && (voltage_value < priv->mbhc_config.btn_voice_assistant_max_voltage)) {
			mutex_lock(&priv->status_mutex);
			priv->btn_report = SND_JACK_BTN_3;
			mutex_unlock(&priv->status_mutex);
			pr_info("key voice_assistant , saradc value is %d\n", voltage_value);
			goto VOICE_ASSISTANT_KEY;
		} else {
			msleep(30);
			hi64xx_plug_in_detect(priv);
			goto end;
		}

		if (!check_headset_pluged_in(priv)) {
			pr_info("%s(%u) : hs pluged out \n", __FUNCTION__, __LINE__);
			goto end;
		}
		startup_FSM(REC_JUDGE, voltage_value, &(priv->btn_report));
VOICE_ASSISTANT_KEY:

		/*btn_report key event*/
		pr_info("%s(%u): btn_report type = 0x%x, status=0x%x\n",
				__FUNCTION__, __LINE__, priv->btn_report, priv->hs_status);
		hi64xx_soc_jack_report(priv->btn_report, HI64xx_BTN_MASK);
	}

end:
	wake_unlock(&priv->wake_lock);

	return;
}

static irqreturn_t hi64xx_plugin_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	msleep(800);

	hi64xx_plug_in_detect(priv);

	return IRQ_HANDLED;
}

void hi64xx_plug_out_detect(struct hi64xx_mbhc_priv *priv)
{
	BUG_ON(NULL == priv);

	if (!priv->hs_cfg.mbhc_func->hs_mbhc_off) {
		pr_err("%s : mbhc off func is not exit\n", __FUNCTION__);
		return;
	}

	if (check_headset_pluged_in(priv)) {
		pr_info("%s : hs still plugin \n", __FUNCTION__);
		return;
	}
	pr_debug("%s(%u) : in", __FUNCTION__,__LINE__);

	mutex_lock(&priv->plug_mutex);

	cancel_delayed_work(&priv->micbias_delay_work);
	flush_workqueue(priv->micbias_delay_wq);

	hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);

	invert_hs_control(INVERT_HS_MIC_GND_CONNECT);

	//stop charge first
	anc_hs_interface_stop_charge();

	hi64xx_resmgr_force_release_micbias(priv->resmgr);
	priv->need_match_micbias = 0;
	hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
	priv->hs_cfg.mbhc_func->hs_mbhc_off(priv->codec);

	mutex_lock(&priv->status_mutex);
	priv->hs_status = HISI_JACK_NONE;
	priv->btn_report = 0;
	mutex_unlock(&priv->status_mutex);
	headset_auto_calib_reset_interzone();

	hi64xx_jack_report(priv);
	priv->hs_plug_status = false;

	mutex_unlock(&priv->plug_mutex);
}

static irqreturn_t hi64xx_plugout_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv;

	priv = (struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	hi64xx_plug_out_detect(priv);

	return IRQ_HANDLED;
}

static irqreturn_t hi64xx_btnup_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	if (!check_headset_pluged_in(priv))
		return IRQ_HANDLED;

	if(priv->mbhc_config.hs_detect_extern_cable &&
		HISI_JACK_EXTERN_CABLE == priv->hs_status) {/*lint !e613*/
		pr_info("%s : for extern cable return!\n", __FUNCTION__);
		return IRQ_HANDLED;
	}

	if (HISI_JACK_INVERT == priv->hs_status) {
		pr_info("%s: further detect\n", __FUNCTION__);
		/* further detect */
		hi64xx_plug_in_detect(priv);
	} else if (0 == priv->btn_report) {
		if (HISI_JACK_HEADSET != priv->hs_status) {
			/* further detect */
			hi64xx_plug_in_detect(priv);
		}
		return IRQ_HANDLED;
	} else {
		mutex_lock(&priv->status_mutex);
		priv->btn_report = 0;
		hi64xx_soc_jack_report(priv->btn_report, HI64xx_BTN_MASK);
		mutex_unlock(&priv->status_mutex);
		pr_info("%s(%u) : btn up !\n", __FUNCTION__, __LINE__);
	}

	return IRQ_HANDLED;
}

static irqreturn_t hi64xx_btndown_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	pr_err("%s: btn down \n", __FUNCTION__);

	hi64xx_btn_down(priv);

	return IRQ_HANDLED;
}

/*lint -save -e613 -e548 -e730*/
static irqreturn_t hi64xx_btnup_comp2_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;
	struct snd_soc_codec *codec = NULL;

	BUG_ON(NULL == priv);

	codec = priv->codec;
	BUG_ON(NULL == codec);

	pr_info("%s: btn up comp2 \n", __FUNCTION__);

	if(priv->hs_status == HISI_JACK_HEADSET) {
		msleep(HI64xx_HANDLE_DELAY_30_MS);
		hi64xx_plug_in_detect(priv);
	} else if(priv->mbhc_config.hs_detect_extern_cable) {
		msleep(HI64xx_HANDLE_DELAY_800_MS);
		hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
		hi64xx_plug_in_detect(priv);
		hi64xx_irq_unmask_btn_irqs(&priv->mbhc_pub);
		if(priv->mbhc_config.irq_reg0) {
			msleep(HI64xx_HANDLE_DELAY_30_MS);
			snd_soc_write(codec, priv->mbhc_config.irq_reg0, HI64XX_CLR_IRQ_COMHL_ECO_STATUS);
		}
	} else {
		pr_info("%s: need do nothing\n", __FUNCTION__);
	}

	return IRQ_HANDLED;
}

static irqreturn_t hi64xx_btndown_comp2_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;
	struct snd_soc_codec *codec = NULL;

	BUG_ON(NULL == priv);

	codec = priv->codec;
	BUG_ON(NULL == codec);

	pr_info("%s: btn down comp2 \n", __FUNCTION__);

	if(priv->hs_status == HISI_JACK_INVERT) {
		msleep(HI64xx_HANDLE_DELAY_30_MS);
		hi64xx_plug_in_detect(priv);
	} else if(priv->mbhc_config.hs_detect_extern_cable) {
		msleep(HI64xx_HANDLE_DELAY_800_MS);
		hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);
		hi64xx_plug_in_detect(priv);
		hi64xx_irq_unmask_btn_irqs(&priv->mbhc_pub);
		if(priv->mbhc_config.irq_reg0) {
			msleep(HI64xx_HANDLE_DELAY_30_MS);
			snd_soc_write(codec, priv->mbhc_config.irq_reg0, HI64XX_CLR_IRQ_COMHL_ECO_STATUS);
		}
	} else {
		pr_info("%s: need do nothing\n", __FUNCTION__);
	}

	return IRQ_HANDLED;
}
/*lint -restore*/

static irqreturn_t hi64xx_btnup_eco_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	if (!check_headset_pluged_in(priv))
		return IRQ_HANDLED;

	if(priv->mbhc_config.hs_detect_extern_cable &&
		HISI_JACK_EXTERN_CABLE == priv->hs_status) {/*lint !e613*/
		pr_info("%s : for extern cable return!\n", __FUNCTION__);
		return IRQ_HANDLED;
	}

	wake_lock_timeout(&priv->wake_lock, 100);

	if (HISI_JACK_INVERT == priv->hs_status) {
		pr_err("%s: further detect\n", __FUNCTION__);
		/* further detect */
		hi64xx_plug_in_detect(priv);
	} else if (0 == priv->btn_report){
		if (HISI_JACK_HEADSET != priv->hs_status) {
			/* further detect */
			hi64xx_plug_in_detect(priv);
		}
		return IRQ_HANDLED;
	} else {
		mutex_lock(&priv->status_mutex);
		priv->btn_report = 0;
		hi64xx_soc_jack_report(priv->btn_report, HI64xx_BTN_MASK);
		mutex_unlock(&priv->status_mutex);
		pr_info("%s(%u) : btn up !\n", __FUNCTION__, __LINE__);
	}

	return IRQ_HANDLED;
}

static irqreturn_t hi64xx_btndown_eco_handler(int irq, void *data)
{
	struct hi64xx_mbhc_priv *priv =
			(struct hi64xx_mbhc_priv *)data;

	BUG_ON(NULL == priv);

	pr_err("%s: btn down \n", __FUNCTION__);

	hi64xx_btn_down(priv);

	return IRQ_HANDLED;
}

void hi64xx_check_axi_bus_reg_value(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int mask, unsigned int except_val)
{
	unsigned int reg_value = 0;

	reg_value = snd_soc_read(codec, reg);
	if((reg_value&mask) != except_val)
	{
		pr_err("%s : AXI bus error, reg_value(%pK): %d \n",
			__FUNCTION__, (void *)(unsigned long)reg, reg_value);
	}
}

void hi64xx_check_bus_status(struct hi64xx_mbhc_priv *priv)
{
	BUG_ON(NULL == priv);

	/* check the write register's status */
	hi64xx_check_axi_bus_reg_value(priv->codec, HI64XX_REG_WRITE_DSP_STATUS,HI64xx_WRITE_DSP_STATUS_BIT_MUSK, 0);
	/* check the read register's status */
	hi64xx_check_axi_bus_reg_value(priv->codec, HI64XX_REG_READ_DSP_STATUS,HI64xx_READ_DSP_STATUS_BIT_MUSK, 0);
	/* check AXI dlock irq status */
	hi64xx_check_axi_bus_reg_value(priv->codec, HI64xx_REG_AXI_DLOCK_IRQ_1,HI64xx_AXI_DLOCK_IRQ_BIT_MUSK_1, 0);
	hi64xx_check_axi_bus_reg_value(priv->codec, HI64xx_REG_AXI_DLOCK_IRQ_2,HI64xx_AXI_DLOCK_IRQ_BIT_MUSK_2, 0);
}


static bool hi64xx_check_headset_in(void *priv)
{
	return check_headset_pluged_in((struct hi64xx_mbhc_priv *)priv);
}

/* ToDo: this interface will be omitted */
static void hi64xx_resume_lock(void *priv, bool lock)
{

}

static void plug_in_detect(void *priv)
{
	struct hi64xx_mbhc_priv * di = (struct hi64xx_mbhc_priv *)priv;

	hi64xx_irq_resume_wait(di->irqmgr);
	hi64xx_plug_in_detect(di);
}

static void plug_out_detect(void *priv)
{
	struct hi64xx_mbhc_priv * di = (struct hi64xx_mbhc_priv *)priv;

	hi64xx_irq_resume_wait(di->irqmgr);
	hi64xx_plug_out_detect(di);
}

static void check_bus_status(void *priv)
{
	hi64xx_check_bus_status((struct hi64xx_mbhc_priv *)priv);
}

static int get_hi64xx_headset_type(void *priv)
{
	struct hi64xx_mbhc_priv * di = (struct hi64xx_mbhc_priv *)priv;
	return (int)(di->hs_status);
}

static void hi64xx_hs_high_resistence_enable(void *priv, bool enable)
{
	struct hi64xx_mbhc_priv * di = (struct hi64xx_mbhc_priv *)priv;
	if (NULL == priv) {
		pr_err("%s: NULL pointer.\n", __FUNCTION__);
		return;
	}

	hi64xx_resmgr_hs_high_resistence_enable(di->resmgr, enable);
	return;
}

static struct anc_hs_dev anc_dev = {
	.name = "anc_hs",
	.ops = {
		.check_headset_in = hi64xx_check_headset_in,
		.btn_report = hi64xx_soc_jack_report,
		.codec_resume_lock = hi64xx_resume_lock,
		.plug_in_detect = plug_in_detect,
		.plug_out_detect = plug_out_detect,
		.check_bus_status = check_bus_status,
	},
};

static struct usb_analog_hs_dev usb_analog_dev = {
	.name = "usb_analog_hs",
	.ops = {
		.check_headset_in = hi64xx_check_headset_in,
		.plug_in_detect = plug_in_detect,
		.plug_out_detect = plug_out_detect,
		.get_headset_type = get_hi64xx_headset_type,
		.hs_high_resistence_enable = hi64xx_hs_high_resistence_enable,
	},
};

void hi64xx_mbhc_3_pole_voltage_config(struct device_node *node,
										struct hi64xx_mbhc_config *mbhc_config)
{
	unsigned int temp = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_3_pole_min_voltage", &temp))
		mbhc_config->hs_3_pole_min_voltage = temp;
	else
		mbhc_config->hs_3_pole_min_voltage = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_3_pole_max_voltage", &temp))
		mbhc_config->hs_3_pole_max_voltage = temp;
	else
		mbhc_config->hs_3_pole_max_voltage = 8;
}

void hi64xx_mbhc_4_pole_voltage_config(struct device_node *node,
										struct hi64xx_mbhc_config *mbhc_config)
{
	unsigned int temp = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_4_pole_min_voltage", &temp))
		mbhc_config->hs_4_pole_min_voltage = temp;
	else
		mbhc_config->hs_4_pole_min_voltage = 1150;

	if (!of_property_read_u32(node, "hisilicon,hs_4_pole_max_voltage", &temp))
		mbhc_config->hs_4_pole_max_voltage = temp;
	else
		mbhc_config->hs_4_pole_max_voltage = 2600;
}

void hi64xx_mbhc_btn_voltage_config(struct device_node *node,
										struct hi64xx_mbhc_config *mbhc_config)
{
	unsigned int temp = 0;

	if (!of_property_read_u32(node, "hisilicon,btn_play_min_voltage", &temp))
		mbhc_config->btn_play_min_voltage = temp;
	else
		mbhc_config->btn_play_min_voltage = 0;

	if (!of_property_read_u32(node, "hisilicon,btn_play_max_voltage", &temp))
		mbhc_config->btn_play_max_voltage = temp;
	else
		mbhc_config->btn_play_max_voltage = 100;

	if (!of_property_read_u32(node, "hisilicon,btn_volume_up_min_voltage", &temp))
		mbhc_config->btn_volume_up_min_voltage = temp;
	else
		mbhc_config->btn_volume_up_min_voltage = 130;

	if (!of_property_read_u32(node, "hisilicon,btn_volume_up_max_voltage", &temp))
		mbhc_config->btn_volume_up_max_voltage = temp;
	else
		mbhc_config->btn_volume_up_max_voltage = 320;

	if (!of_property_read_u32(node, "hisilicon,btn_volume_down_min_voltage", &temp))
		mbhc_config->btn_volume_down_min_voltage = temp;
	else
		mbhc_config->btn_volume_down_min_voltage = 350;

	if (!of_property_read_u32(node, "hisilicon,btn_volume_down_max_voltage", &temp))
		mbhc_config->btn_volume_down_max_voltage = temp;
	else
		mbhc_config->btn_volume_down_max_voltage = 700;

	if (!of_property_read_u32(node, "hisilicon,btn_voice_assistant_min_voltage", &temp))
		mbhc_config->btn_voice_assistant_min_voltage = temp;
	else
		mbhc_config->btn_voice_assistant_min_voltage = INVALID_VOLTAGE;

	if (!of_property_read_u32(node, "hisilicon,btn_voice_assistant_max_voltage", &temp))
		mbhc_config->btn_voice_assistant_max_voltage = temp;
	else
		mbhc_config->btn_voice_assistant_max_voltage = INVALID_VOLTAGE;
}

void hi64xx_mbhc_hs_extern_cable_config(struct device_node *node,
										struct hi64xx_mbhc_config *mbhc_config)
{
	unsigned int temp = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_extern_cable_min_voltage", &temp))/*lint !e64*/
		mbhc_config->hs_extern_cable_min_voltage = temp;
	else
		mbhc_config->hs_extern_cable_min_voltage = 2651;

	if (!of_property_read_u32(node, "hisilicon,hs_extern_cable_max_voltage", &temp))/*lint !e64*/
		mbhc_config->hs_extern_cable_max_voltage = temp;
	else
		mbhc_config->hs_extern_cable_max_voltage = 2700;

	if (!of_property_read_u32(node, "hisilicon,hs_mbhc_vref_reg_value", &temp))/*lint !e64*/
		mbhc_config->hs_mbhc_vref_reg_value = temp;
	else
		mbhc_config->hs_mbhc_vref_reg_value = EXTERN_CABLE_MBHC_VREF_DAFULT_VALUE;

	if (!of_property_read_u32(node, "hisilicon,hi64xx_irq_reg0", &temp))/*lint !e64*/
		mbhc_config->irq_reg0 = temp;
	else
		mbhc_config->irq_reg0 = 0;

	if (!of_property_read_u32(node, "hs_detect_extern_cable", &temp)) {/*lint !e64*/
		if(temp) {
			mbhc_config->hs_detect_extern_cable = true;
		} else {
			mbhc_config->hs_detect_extern_cable = false;
		}
	} else {
		mbhc_config->hs_detect_extern_cable = false;
	}

}

static void hi64xx_mbhc_config_set(struct device_node *node, struct hi64xx_mbhc_config *mbhc_config)
{
	unsigned int temp = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_det", &temp))
		mbhc_config->hs_det_inv = temp;
	else
		mbhc_config->hs_det_inv = 0;

	if (!of_property_read_u32(node, "hisilicon,hs_ctrl", &temp))
		mbhc_config->hs_ctrl = temp;
	else
		mbhc_config->hs_ctrl = 0x19;

	if (!of_property_read_u32(node, "hisilicon,coefficient", &temp))
		mbhc_config->coefficient = temp;
	else
		mbhc_config->coefficient = 2800; /* saradc range 0 ~ 2800mV */

	hi64xx_mbhc_3_pole_voltage_config(node, mbhc_config);
	hi64xx_mbhc_4_pole_voltage_config(node, mbhc_config);
	hi64xx_mbhc_btn_voltage_config(node, mbhc_config);
	headset_auto_calib_init(node);
	hi64xx_mbhc_hs_extern_cable_config(node, mbhc_config);
}


int hi64xx_register_hs_jack_btn(struct snd_soc_codec *codec)
{
	int ret = 0;

	/* register headset jack */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
	ret = snd_soc_card_jack_new(codec->component.card, "Headset Jack", SND_JACK_HEADSET, &hs_jack, NULL, 0);
#else
	ret = snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &hs_jack);
#endif
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	/* register headset button */
	ret = snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__,  __LINE__, ret);
		return ret;
	}
	ret = snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__,  __LINE__, ret);
		return ret;
	}
	ret = snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__,  __LINE__, ret);
		return ret;
	}

	ret = snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__,  __LINE__, ret);
		return ret;
	}

	/* for sound triger */
	ret = snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_5, KEY_F14);
	if (ret) {
		pr_err("%s %d: jack error, errornum = %d\n", __FUNCTION__,  __LINE__, ret);
		return ret;
	}

	return ret;
}


static void hi64xx_mbhc_first_detect(struct hi64xx_mbhc_priv *priv)
{
	if (check_headset_pluged_in(priv)) {
		if(check_usb_analog_hs_support()) {
			usb_analog_hs_plug_in_out_handle(USB_ANA_HS_PLUG_IN);
		} else {
			hi64xx_plug_in_detect(priv);
		}
	}
}

static int hi64xx_mbhc_init_para_check(struct hi64xx_hs_cfg *hs_cfg)
{
	if (!hs_cfg) {
		pr_err("%s : headset cfg is not exit\n", __FUNCTION__);
		return -EINVAL;
	}
	if (!hs_cfg->mbhc_func) {
		pr_err("%s : mbhc func is not exit\n", __FUNCTION__);
		return -EINVAL;
	}
	if (!hs_cfg->mbhc_func->hs_enable_hsdet) {
		pr_err("%s : hsdet func is not exit\n", __FUNCTION__);
		return -EINVAL;
	}
	return 0;
}

int hi64xx_mbhc_init(struct snd_soc_codec *codec,
		struct device_node *node,
		struct hi64xx_hs_cfg *hs_cfg,
		struct hi64xx_resmgr *resmgr,
		struct hi64xx_irq *irqmgr,
		struct hi64xx_mbhc **mbhc)
{
	int ret = 0;
	struct hs_mbhc_func* mbhc_func;
	struct hi64xx_mbhc_priv *priv = NULL;

	if (hi64xx_mbhc_init_para_check(hs_cfg)) {
		return -EINVAL;
	}


	priv = kzalloc(sizeof(struct hi64xx_mbhc_priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		return ret;
	}

	memcpy(&priv->hs_cfg, hs_cfg, sizeof(struct hi64xx_hs_cfg));/* unsafe_function_ignore: memcpy */
	mbhc_func = priv->hs_cfg.mbhc_func;

	hi64xx_mbhc_config_set(node, &priv->mbhc_config);
	priv->codec = codec;
	priv->resmgr = resmgr;
	priv->irqmgr = irqmgr;
	*mbhc = &priv->mbhc_pub;

	priv->need_match_micbias = 0;


	priv->sdev.name = "h2w";
	ret = switch_dev_register(&priv->sdev);
	if (ret) {
		pr_err("%s : error registering switch device %d\n", __FUNCTION__, ret);
		goto err_exit;
	}
	wake_lock_init(&priv->wake_lock, WAKE_LOCK_SUSPEND, "hisi-64xx-mbhc");
	wake_lock_init(&priv->micbias_wake_lock, WAKE_LOCK_SUSPEND, "hisi-64xx-mbhc-micbias");
	mutex_init(&priv->plug_mutex);
	mutex_init(&priv->status_mutex);
	mutex_init(&priv->saradc_mutex);

	priv->micbias_delay_wq = create_singlethread_workqueue("hi64xx_micbias_delay_wq");
	if (!(priv->micbias_delay_wq)) {
		pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
		goto mic_delay_wq_exit;
	}
	INIT_DELAYED_WORK(&priv->micbias_delay_work, hi64xx_micbias_work_func);
	priv->mbhc_micbias_work = false;

	/* register anc hs first */
	anc_hs_interface_dev_register(&anc_dev, priv);

	usb_analog_hs_dev_register(&usb_analog_dev, priv);

	ret = hi64xx_register_hs_jack_btn(codec);
	if (ret) {
		goto jack_exit;
	}

	/* irq request : plugout */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_PLUGOUT, hi64xx_plugout_handler, "plugout", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_plugout_exit;
	}

	/* irq request : plugin */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_PLUGIN, hi64xx_plugin_handler, "plugin", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_plugin_exit;
	}

	/* irq request : button up(eco mode) */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNUP_ECO, hi64xx_btnup_eco_handler, "btnup_eco", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btnupeco_exit;
	}

	/* irq request : button down(eco mode) */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNDOWN_ECO, hi64xx_btndown_eco_handler, "btndown_eco", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btndowneco_exit;
	}

	/* irq request : button down */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNDOWN_COMP1, hi64xx_btndown_handler, "btndown_comp1", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btndowncomp1_exit;
	}

	/* irq request : button up */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNUP_COMP1, hi64xx_btnup_handler, "btnup_comp1", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btnupcomp1_exit;
	}

	/* irq request : comp2 button down */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNDOWN_COMP2, hi64xx_btndown_comp2_handler, "btndown_comp2", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btndowncomp2_exit;
	}

	/* irq request : comp2 button up */
	ret = hi64xx_irq_request_irq(irqmgr, IRQ_BTNUP_COMP2, hi64xx_btnup_comp2_handler, "btnup_comp2", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_btnupcomp2_exit;
	}

	/* mask btn irqs */
	hi64xx_irq_mask_btn_irqs(&priv->mbhc_pub);

	/* enable hsdet */
	mbhc_func->hs_enable_hsdet(codec, priv->mbhc_config);

	/* check jack at first time */
	hi64xx_mbhc_first_detect(priv);



	return ret;

irq_btnupcomp2_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNDOWN_COMP2, priv);
irq_btndowncomp2_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNUP_COMP1, priv);
irq_btnupcomp1_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNDOWN_COMP1, priv);
irq_btndowncomp1_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNDOWN_ECO, priv);
irq_btndowneco_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNUP_ECO, priv);
irq_btnupeco_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLUGIN, priv);
irq_plugin_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLUGOUT, priv);
irq_plugout_exit:
jack_exit:
	if (NULL != priv->micbias_delay_wq) {
		cancel_delayed_work(&priv->micbias_delay_work);
		flush_workqueue(priv->micbias_delay_wq);
		destroy_workqueue(priv->micbias_delay_wq);
		priv->micbias_delay_wq = NULL;
	}
mic_delay_wq_exit:
	wake_lock_destroy(&priv->wake_lock);
	wake_lock_destroy(&priv->micbias_wake_lock);
	mutex_destroy(&priv->plug_mutex);
	mutex_destroy(&priv->status_mutex);
	mutex_destroy(&priv->saradc_mutex);
err_exit:
	kfree(priv);
	return ret;
}

void hi64xx_mbhc_deinit(struct hi64xx_mbhc *mbhc)
{
	struct hi64xx_mbhc_priv* priv =
		(struct hi64xx_mbhc_priv*)mbhc;

	if (priv == NULL)
		return;

	if (priv->irqmgr) {
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLUGOUT, priv);
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLUGIN, priv);
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNUP_ECO, priv);
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNDOWN_ECO, priv);
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNDOWN_COMP1, priv);
		hi64xx_irq_free_irq(priv->irqmgr, IRQ_BTNUP_COMP1, priv);
	}

	wake_lock_destroy(&priv->wake_lock);
	wake_lock_destroy(&priv->micbias_wake_lock);
	mutex_destroy(&priv->plug_mutex);
	mutex_destroy(&priv->status_mutex);
	mutex_destroy(&priv->saradc_mutex);

	if (NULL != priv->micbias_delay_wq) {
		cancel_delayed_work(&priv->micbias_delay_work);
		flush_workqueue(priv->micbias_delay_wq);
		destroy_workqueue(priv->micbias_delay_wq);
		priv->micbias_delay_wq = NULL;
	}



	kfree(priv);
}

MODULE_DESCRIPTION("hi64xx_mbhc");
MODULE_AUTHOR("guzhengming <guzhengming@hisilicon.com>");
MODULE_LICENSE("GPL");
