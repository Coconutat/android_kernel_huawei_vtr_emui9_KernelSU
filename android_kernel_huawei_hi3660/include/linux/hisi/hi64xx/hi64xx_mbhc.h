#ifndef __HI64xx_mbhc_H__
#define __HI64xx_mbhc_H__
#include <sound/jack.h>
#include <linux/switch.h>
#include <linux/wakelock.h>
#include <linux/hisi/hi64xx/hi64xx_irq.h>
#include <linux/hisi/hi64xx/hi64xx_utils.h>
#include <linux/hisi/hi64xx/hi64xx_resmgr.h>


enum hisi_jack_states {
	HISI_JACK_NONE = 0,	/* unpluged */
	HISI_JACK_HEADSET,	/* pluged 4-pole headset */
	HISI_JACK_HEADPHONE,	/* pluged 3-pole headphone */
	HISI_JACK_INVERT,	/* pluged invert 4-pole headset */
	HISI_JACK_EXTERN_CABLE,	/* pluged extern cable,such as antenna cable*/
};

struct hi64xx_mbhc_config {
	/* board defination */
	int hs_det_inv;
	int hs_btn_num;
	unsigned int hs_3_pole_min_voltage;
	unsigned int hs_3_pole_max_voltage;
	unsigned int hs_4_pole_min_voltage;
	unsigned int hs_4_pole_max_voltage;
	unsigned int btn_play_min_voltage;
	unsigned int btn_play_max_voltage;
	unsigned int btn_volume_up_min_voltage;
	unsigned int btn_volume_up_max_voltage;
	unsigned int btn_volume_down_min_voltage;
	unsigned int btn_volume_down_max_voltage;
	unsigned int btn_voice_assistant_min_voltage;
	unsigned int btn_voice_assistant_max_voltage;
	unsigned int hs_extern_cable_min_voltage;
	unsigned int hs_extern_cable_max_voltage;
	unsigned int hs_mbhc_vref_reg_value;
	unsigned int hs_ctrl;
	unsigned int coefficient; /* voltage coefficient*/
	unsigned int irq_reg0;
	bool hs_detect_extern_cable;
};

struct hs_mbhc_reg {
	unsigned int irq_source_reg;
	unsigned int irq_mbhc_2_reg;
};

struct hs_mbhc_func {
	void (*hs_mbhc_on)(struct snd_soc_codec *);
	unsigned int (*hs_get_voltage)(struct snd_soc_codec *, unsigned int);
	void (*hs_enable_hsdet)(struct snd_soc_codec *, struct hi64xx_mbhc_config);
	void (*hs_mbhc_off)(struct snd_soc_codec *);
};

struct hs_res_detect_func {
	void (*hs_res_detect)(struct snd_soc_codec *);
	void (*hs_path_enable)(struct snd_soc_codec *);
	void (*hs_path_disable)(struct snd_soc_codec *);
};

/* defination of public data */
struct hi64xx_mbhc {
};

struct hi64xx_hs_cfg{
	struct hs_mbhc_reg* mbhc_reg;
	struct hs_mbhc_func* mbhc_func;
	struct hs_res_detect_func* res_detect_func;
};

extern int hi64xx_mbhc_init(struct snd_soc_codec *codec,
			struct device_node *node,
			struct hi64xx_hs_cfg *hi64xx_hs_cfg,
			struct hi64xx_resmgr *resmgr,
			struct hi64xx_irq *irqmgr,
			struct hi64xx_mbhc **mbhc);

extern void hi64xx_mbhc_deinit(struct hi64xx_mbhc *mbhc);

extern void hi64xx_irq_mask_btn_irqs(struct hi64xx_mbhc *mbhc);
extern void hi64xx_irq_unmask_btn_irqs(struct hi64xx_mbhc *mbhc);
extern bool hi64xx_check_saradc_ready_detect(struct snd_soc_codec *codec);
#endif
