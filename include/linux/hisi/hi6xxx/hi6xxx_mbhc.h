#ifndef __HI6XXX_MBHC_H__
#define __HI6XXX_MBHC_H__

#include <linux/wakelock.h>
#include <linux/switch.h>
#include <sound/soc.h>


#define MICBIAS_PD_WAKE_LOCK_MS        3500
#define MICBIAS_PD_DELAY_MS            3000
#define IRQ_HANDLE_WAKE_LOCK_MS        2000
#define LINEOUT_PO_RECHK_WAKE_LOCK_MS  3500
#define LINEOUT_PO_RECHK_DELAY_MS      800
#define HI6XXX_HKADC_CHN	14
#define HI6XXX_INVALID_IRQ	(-1)

#ifdef CLT_AUDIO
#define static_t
#else
#define static_t static
#endif

enum hi6xxx_jack_states {
	HI6XXX_JACK_BIT_NONE           = 0, /* unpluged */
	HI6XXX_JACK_BIT_HEADSET     = 1, /* pluged with mic */
	HI6XXX_JACK_BIT_HEADPHONE = 2, /* pluged with 3 pole */
	HI6XXX_JACK_BIT_INVERT = 3, /* pluged without mic */
	HI6XXX_JACK_BIT_PLUGING      = 4, /* pluging, not fully pluged */
	HI6XXX_JACK_BIT_INVALID       = 5, /* index */
	HI6XXX_JACK_BIT_LINEOUT      = 6, /* pluged with lineout */
};

enum delay_time {
	HS_TIME_PI_DETECT       = 800, /* ms */
	HS_TIME_COMP_IRQ_TRIG   = 30,  /* ms */
	HS_TIME_COMP_IRQ_TRIG_2 = 50, /*ms*/
};

struct hi6xxx_jack_data {
	struct snd_soc_jack jack;
	int report;
	struct switch_dev sdev;
	bool is_dev_registered;
};

struct hi6xxx_mbhc_config {/*mV*/
	/* board config */
	int hs_3_pole_max_voltage;
	int hs_4_pole_min_voltage;
	int hs_4_pole_max_voltage;

	int btn_play_min_voltage;
	int btn_play_max_voltage;
	int btn_volume_up_min_voltage;
	int btn_volume_up_max_voltage;
	int btn_volume_down_min_voltage;
	int btn_volume_down_max_voltage;
	int btn_voice_assistant_min_voltage;
	int btn_voice_assistant_max_voltage;
};

/* defination of public data */
struct hi6xxx_mbhc {
};

/* defination of private data */
struct hi6xxx_mbhc_priv {
	struct hi6xxx_mbhc mbhc_pub;
	struct snd_soc_codec *codec;
	struct hi6xxx_jack_data  hs_jack;
	enum hi6xxx_jack_states  hs_status;
	enum hi6xxx_jack_states  old_hs_status;
	int pressed_btn_type;
	int adc_voltage;
	/* work queue for headset */
	struct workqueue_struct *hs_pi_dwq; /* headset plugin delayed work queue */
	struct delayed_work hs_pi_dw;
	struct workqueue_struct *hs_po_dwq; /* headset plugout delayed work queue */
	struct delayed_work hs_po_dw;
	struct workqueue_struct *hs_comp_l_btn_down_dwq; /* headset button down delayed work queue */
	struct delayed_work hs_comp_l_btn_down_dw;
	struct workqueue_struct *hs_comp_l_btn_up_dwq; /* headset button up delayed work queue */
	struct delayed_work hs_comp_l_btn_up_dw;
	struct workqueue_struct *hs_eco_btn_down_dwq; /* headset eco button down delayed work queue */
	struct delayed_work hs_eco_btn_down_dw;
	struct workqueue_struct *hs_eco_btn_up_dwq; /* headset eco button up delayed work queue */
	struct delayed_work hs_eco_btn_up_dw;
	struct workqueue_struct *hs_comp_h_btn_down_dwq; /* headset button down delayed work queue */
	struct delayed_work hs_comp_h_btn_down_dw;
	struct workqueue_struct *hs_comp_h_btn_up_dwq; /* headset button up delayed work queue */
	struct delayed_work hs_comp_h_btn_up_dw;
	struct workqueue_struct *hs_micbias_delay_pd_dwq; /* hs_micbias_delay_pd work queue */
	struct delayed_work hs_micbias_delay_pd_dw;
	struct workqueue_struct *lineout_po_recheck_dwq; /* lineout_po_recheck work queue */
	struct delayed_work lineout_po_recheck_dw;
	struct mutex io_mutex;
	struct mutex hkadc_mutex;
	struct mutex plug_mutex;
	struct mutex hs_micbias_mutex;
	struct wake_lock wake_lock;
	unsigned int hs_micbias_dapm;
	bool hs_micbias_mbhc;

	bool pre_status_is_lineout; /* is pre_hs_status lineout or not */
	/* gpio intr pin */
	int gpio_intr_pin;
	int gpio_irq;

	/* board config */
	struct hi6xxx_mbhc_config mbhc_config;
};

/* irq */
#define IRQ_PLUG_OUT (0x1 << (ANA_IRQ_MASK_OFFSET + 7))
#define IRQ_PLUG_IN (0x1 << (ANA_IRQ_MASK_OFFSET + 6))
#define IRQ_ECO_BTN_DOWN (0x1 << (ANA_IRQ_MASK_OFFSET + 5))
#define IRQ_ECO_BTN_UP (0x1 << (ANA_IRQ_MASK_OFFSET + 4))
#define IRQ_COMP_L_BTN_DOWN (0x1 << (ANA_IRQ_MASK_OFFSET + 3))
#define IRQ_COMP_L_BTN_UP (0x1 << (ANA_IRQ_MASK_OFFSET + 2))
#define IRQ_COMP_H_BTN_DOWN (0x1 << (ANA_IRQ_MASK_OFFSET + 1))
#define IRQ_COMP_H_BTN_UP (0x1 << (ANA_IRQ_MASK_OFFSET + 0))

#define IRQ_MSK_COMP_H_BTN (IRQ_COMP_H_BTN_UP | IRQ_COMP_H_BTN_DOWN)
#define IRQ_MSK_BTN_ECO (IRQ_ECO_BTN_UP | IRQ_ECO_BTN_DOWN)
#define IRQ_MSK_BTN_NOR (IRQ_COMP_L_BTN_UP | IRQ_COMP_L_BTN_DOWN | IRQ_MSK_COMP_H_BTN)
#define IRQ_MSK_COMP (IRQ_MSK_BTN_NOR | IRQ_MSK_BTN_ECO)
#define IRQ_MSK_HS_ALL (IRQ_PLUG_OUT | IRQ_PLUG_IN | IRQ_MSK_COMP)

/* 0-earphone not pluged, 1-earphone pluged */
#define IRQ_STAT_PLUG_IN (0x1 <<  HS_DET_IRQ_OFFSET)
/* 0-normal btn event    1- no normal btn event */
#define IRQ_STAT_KEY_EVENT (0x1 << HS_MIC_NOR2_IRQ_OFFSET)
/* 0-eco btn event     1- no eco btn event */
#define IRQ_STAT_ECO_KEY_EVENT (0x1 << HS_MIC_ECO_IRQ_OFFSET)

#define JACK_RPT_MSK_BTN (SND_JACK_BTN_0 | SND_JACK_BTN_1 | SND_JACK_BTN_2 | SND_JACK_BTN_3)

extern int hi6xxx_mbhc_init(struct snd_soc_codec *codec, struct hi6xxx_mbhc **mbhc);
extern void hi6xxx_mbhc_deinit(struct hi6xxx_mbhc *mbhc);
extern void hi6xxx_hs_micbias_dapm_enable(struct hi6xxx_mbhc *mbhc, bool enable);

#endif

