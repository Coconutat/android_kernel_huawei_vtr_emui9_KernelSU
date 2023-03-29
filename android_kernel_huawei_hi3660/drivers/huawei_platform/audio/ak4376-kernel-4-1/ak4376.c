/*
 * ak4376.c  --  audio driver for AK4376
 *
 * Copyright (C) 2015 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      15/06/12	    1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/version.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#include "ak4376.h"

#define LOG_TAG "AK4376"

#define PRINT_INFO  1
#define PRINT_WARN  1
#define PRINT_DEBUG 0
#define PRINT_ERR   1

#if PRINT_INFO
#define logi(fmt, ...) printk("[" LOG_TAG "][I]" fmt, ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if PRINT_WARN
#define logw(fmt, ...) printk("[" LOG_TAG "][W]" fmt, ##__VA_ARGS__)
#else
#define logw(fmt, ...)
#endif

#if PRINT_DEBUG
#define logd(fmt, ...) printk("[" LOG_TAG "][D]" fmt, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if PRINT_ERR
#define loge(fmt, ...) printk("[" LOG_TAG "][E]" fmt, ##__VA_ARGS__)
#else
#define loge(fmt, ...)
#endif

enum {
	OFF = 0,
	ON,
};

enum {
	SWITCH_OFF = 0,
	CODEC_ON,
	HIFI_ON,
};

enum {
	PLL_OFF = 0,
	BICK_PLL,
	MCKI_PLL,
	XTAL_MODE,
};

#define AK4376_NAME "ak4376-codec"
#define AK4376_SWTICH_VOLTAGE_VALUE   (3300000)
#define AK4376_MAX_REG_NUM            (24)
#define AK4376_DEFALUT_NORMAL_FREQ    (48000)
#define AK4376_FLAG_MAX               (32)

#define SAMPLE_RATE_8KHZ      (8000)
#define SAMPLE_RATE_16KHZ     (16000)
#define SAMPLE_RATE_32KHZ     (32000)
#define SAMPLE_RATE_48KHZ     (48000)
#define SAMPLE_RATE_96KHZ     (96000)
#define SAMPLE_RATE_192KHZ    (192000)
#define SAMPLE_RATE_384KHZ    (384000)

enum {
	PLL_OFF_48kHz    = 0,
	BICK_PLL_8kHz    = 1,
	BICK_PLL_16kHz   = 2,
	BICK_PLL_32kHz   = 3,
	BICK_PLL_48kHz   = 4,
	BICK_PLL_96kHz   = 5,
	BICK_PLL_192kHz  = 6,
	BICK_PLL_384kHz  = 7,
	MCKI_PLL_32kHz   = 8,
	MCKI_PLL_48kHz   = 9,
	MCKI_PLL_96kHz   = 10,
	MCKI_PLL_192kHz  = 11,
	MCKI_PLL_384kHz  = 12,
};
/* AK4376 Codec Private Data */
struct ak4376_priv {
	struct mutex mutex;
	int priv_pdn_en;			//PDN GPIO pin
	int pdn;							//PDN control, 0:Off, 1:On
	int fs1;
	int rclk;							//Master Clock
	int nBickFreq;						//0:32fs, 1:48fs, 2:64fs
	int nPllMCKI;						//0:9.6MHz, 1:11.2896MHz, 2:12.288MHz, 3:19.2MHz
	int nDeviceID;						//0:AK4375, 1:AK4375A, 2:AK4376, 3:Other IC
	int lpmode;							//0:High Performance, 1:Low power mode
	int xtalfreq;						//0:12.288MHz, 1:11.2896MHz
	int nDACOn;
	int nPllMode;
	int hifi_pwren;
	int switch_ac_dc;
	int switch_dir;
	int switch_mute;
	int switch_sel;
	int switch_voltage;
	int switch_power_en;

	int hifi_power_off_flag;
	int switch_ac_dc_flg;
	int switch_dir_flg;
	int switch_open_flg;
	int switch_sel_flg;
	int switch_power_en_flg;
	int swtich_vdd_flg;
	int swtich_existence;
	unsigned int regulator_config;
	int switch_ajust_seq;
	int hifi_mclk_en;
	unsigned int master_mode;
	unsigned int freq;
	unsigned int swtich_delaytime;
	struct clk *mclk;

	struct regulator *swtich_vdd;
	struct i2c_client *i2c;
};

static struct mutex io_lock;
static int PMHLHP_CLOSE = 0;
static int pll_mode_and_sample_rate_pre = 0;

static unsigned int ak4376_reg_read(struct snd_soc_codec *, unsigned int);
static int ak4376_write_register(struct snd_soc_codec *, unsigned int, unsigned int);
static int ak4376_switch_regulator_enable(struct ak4376_priv *pdata);
static int ak4376_switch_regulator_disable(struct ak4376_priv *pdata);

static struct snd_soc_codec *ak4376_codec;

#define I2C_WRITE_READ_RETRY_TIMES         (3)
#define I2C_REG_FAIL_MAX_TIMES             (10)
#define I2C_FAIL_REPORT_MAX_TIMES          (20)
static unsigned int i2c_reg_fail_times = 0;
static unsigned int i2c_fail_report_times = I2C_FAIL_REPORT_MAX_TIMES;

#define HI6555_0X11A_REG                   0x11A
#define HI6555_0X11A_XO_FREQ_CODEC_MASK    0x03
extern unsigned int hisi_pmic_reg_read(int addr);
extern void hisi_pmic_reg_write(int addr, int val);

static void hifi_dsm_i2c_reg_fail_report(void)
{
	i2c_reg_fail_times ++;
	if ((i2c_reg_fail_times > I2C_REG_FAIL_MAX_TIMES) && (i2c_fail_report_times > 0)) {
		i2c_reg_fail_times = 0;
		i2c_fail_report_times --;
#ifdef CONFIG_HUAWEI_DSM
		audio_dsm_report_info(AUDIO_CODEC, DSM_HIFI_AK4376_I2C_ERR, " ak4376_i2c read/write err! total times = [%d]\n",
							(I2C_FAIL_REPORT_MAX_TIMES - i2c_fail_report_times)*10);
#endif
	}
	return;
}

static inline void ak4376_update_register(struct snd_soc_codec *codec)
{
	u8 cur_cache;
	u8 *cache =  NULL;
	int i;
	unsigned int reg = 0;
	unsigned int index = 0;
	u8 cur_register;

	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return;
	}

	cache = codec->reg_cache;
	if(NULL == cache) {
		loge("%s(%d): cache is NULL\n",__FUNCTION__,__LINE__);
		return;
	}

	logd("%s(%d)\n", __FUNCTION__,__LINE__);

	for (i = 0; i < AK4376_16_DUMMY; i++) {
		cur_register = ak4376_reg_read(codec, i);
		cur_cache = cache[i];

		logd("%s(%d) reg:0x%x (I2C, cache)=(0x%x,0x%x)\n", __FUNCTION__,__LINE__,i,cur_register,cur_cache);

		if (cur_register != cur_cache){
			ak4376_write_register(codec, i, cur_cache);
			logd("%s(%d) write cache to register\n", __FUNCTION__,__LINE__);
		}
	}

	for (i = (AK4376_MAX_REG_NUM -2); i <= AK4376_MAX_REG_NUM; i++) {
		if(i == (AK4376_MAX_REG_NUM -2)){
			reg = AK4376_24_MODE_CONTROL;
			index = AK4376_24_MODE_CONTROL;
		} else if(i == (AK4376_MAX_REG_NUM - 1)) {
			reg = AK4376_26_DAC_ADJ1;
			index = AK4376_24_MODE_CONTROL + 1;
		} else if(i == AK4376_MAX_REG_NUM){
			reg = AK4376_2A_DAC_ADJ2;
			index = AK4376_24_MODE_CONTROL + 2;
		}

		cur_register = ak4376_reg_read(codec, reg);
		cur_cache = cache[index];

		logi("%s(%d) (reg:0x%x)cur_register=%x, cur_cache=%x\n", __FUNCTION__,__LINE__,reg,cur_register,cur_cache);

		if (cur_register != cur_cache)
			ak4376_write_register(codec, reg, cur_cache);
	}
}

static int ak4376_mclk_enable(struct ak4376_priv *ak4376, bool enable);
/* GPIO control for PDN */
static int ak4376_pdn_control(struct snd_soc_codec *codec, int pdn)
{
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d) pdn = %d ak4376->pdn = %d\n",__FUNCTION__, __LINE__, pdn, ak4376->pdn);

	if ((OFF == ak4376->pdn) && (ON == pdn)) {
		gpio_direction_output(ak4376->priv_pdn_en, ON);
		logi("%s(%d) Turn on priv_pdn_en\n", __FUNCTION__,__LINE__);
		ak4376->pdn = ON;
		if (OFF == ak4376->hifi_mclk_en) {
			udelay(1000);
			logi("%s(%d) Turn on mclk\n", __FUNCTION__,__LINE__);
			ak4376_mclk_enable(ak4376, true);
			ak4376->hifi_mclk_en = ON;
		}
		udelay(800);

		ak4376_update_register(codec);

	} else if ((ak4376->pdn == ON) && (pdn == OFF)) {
		gpio_direction_output(ak4376->priv_pdn_en, OFF);
		logi("%s(%d) Turn off priv_pdn_en\n", __FUNCTION__,__LINE__);
		ak4376->pdn = OFF;
		if (ak4376->hifi_mclk_en == ON) {
			logi("%s(%d) Turn off mclk\n", __FUNCTION__,__LINE__);
			ak4376_mclk_enable(ak4376, false);
			ak4376->hifi_mclk_en = OFF;
		}

		snd_soc_cache_init(codec);
	}

	return 0;
}
#ifdef AK4376_KERNEL_4_1_DEBUG
static ssize_t ak4376_reg_data_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret = 0, i, fpt;
	int rx[AK4376_MAX_REG_NUM + 1] = {0};

	if ((NULL == ak4376_codec) || (NULL == buf)) {
		loge("%s: ak4376_codec or buf is NULL.\n", __FUNCTION__);
		return -ENOMEM;
	}

	mutex_lock(&io_lock);
	for (i = 0; i < AK4376_16_DUMMY; i++) {
		ret = ak4376_reg_read(ak4376_codec, i);

		if (ret < 0) {
			loge("%s(%d): read register[0x%x] error.\n", __FUNCTION__,__LINE__,i);
			break;
		} else {
			rx[i] = ret;
		}
	}

	rx[AK4376_MAX_REG_NUM-2] = ak4376_reg_read(ak4376_codec, AK4376_24_MODE_CONTROL);
	rx[AK4376_MAX_REG_NUM-1] = ak4376_reg_read(ak4376_codec, AK4376_26_DAC_ADJ1);
	rx[AK4376_MAX_REG_NUM] = ak4376_reg_read(ak4376_codec, AK4376_2A_DAC_ADJ2);

	mutex_unlock(&io_lock);

	if (AK4376_16_DUMMY == i) {
		ret = fpt = 0;
		for (i = 0; i < AK4376_16_DUMMY; i++, fpt += 6) {
			ret += snprintf(buf + fpt, 32, "%02x,%02x\n", i, rx[i]);
		}
		ret += snprintf(buf + i * 6, 32, "24,%02x\n", rx[AK4376_MAX_REG_NUM -2]);
		ret += snprintf(buf + (i + 1) * 6, 32, "26,%02x\n", rx[AK4376_MAX_REG_NUM -1]);
		ret += snprintf(buf + (i + 2) * 6, 32, "2a,%02x\n", rx[AK4376_MAX_REG_NUM]);

		return ret;
	} else {
		return snprintf(buf, 32, "read error");
	}
}

static DEVICE_ATTR(ak4376_reg_data_read, 0440, ak4376_reg_data_show, NULL);

static ssize_t ak4376_reg_data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int reg = 0;
	int val = 0;
	char *pEnd = buf;

	if (NULL == pEnd || NULL == ak4376_codec) {
		loge("%s : null pointer\n", __FUNCTION__);
		return -EIO;
	}

	/* echo reg val > ak4376_reg_data_write
	eg: echo 0x1A 0x22 > ak4376_reg_data_write */
	reg = (int)simple_strtol(buf, &pEnd, 16);
	if (reg < AK4376_00_POWER_MANAGEMENT1 ||
		((reg >= AK4376_16_DUMMY) && (reg <= AK4376_23_DUMMY)) ||
		reg > AK4376_2A_DAC_ADJ2) {
		loge("%s : 0x%x\n", __FUNCTION__, reg);
		return -EINVAL;
	}
	if (NULL == pEnd || ' ' != *pEnd) {
		loge("%s : input error!\n", __FUNCTION__);
		return -EINVAL;
	}

	pEnd++;
	logd("%s  reg : 0x%x", __FUNCTION__, reg);
	val = (int)simple_strtol(pEnd, &pEnd, 16);
	logd("%s  val : 0x%x", __FUNCTION__, val);
	if (val < 0x0 || val > 0xff) {
		loge("%s  invalid register value: 0x%x\n", __FUNCTION__, val);
		return -EINVAL;
	}

	ak4376_write_register(ak4376_codec, reg, val);

	return (ssize_t)count;
}

static DEVICE_ATTR(ak4376_reg_data_write, 0660, NULL, ak4376_reg_data_store);

static ssize_t ak4376_flag_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	char buffer[AK4376_FLAG_MAX] = {0};

	if ((NULL == ak4376_codec) || (NULL == buf)) {
		loge("%s: ak4376_codec or buf is NULL.\n", __FUNCTION__);
		return -EIO;
	}
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(ak4376_codec);

	if (NULL == ak4376) {
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	snprintf(buffer, sizeof(buffer), "pdn : %d\n", ak4376->pdn);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "nDeviceID : %d\n", ak4376->nDeviceID);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "hifi_power_off_flag : %d\n", ak4376->hifi_power_off_flag);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "master_mode : %d\n", ak4376->master_mode);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "nPllMode : %d\n", ak4376->nPllMode);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "freq : %d\n", ak4376->freq);
	strncat(buf, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer), "lpmode : %d\n", ak4376->lpmode);
	strncat(buf, buffer, strlen(buffer));

	return (ssize_t)strlen(buf);
}

static DEVICE_ATTR(ak4376_flag_read, 0440, ak4376_flag_show, NULL);

static struct attribute *ak4376_attributes[] = {
	&dev_attr_ak4376_reg_data_read.attr,
	&dev_attr_ak4376_reg_data_write.attr,
	&dev_attr_ak4376_flag_read.attr,
	NULL
};

static const struct attribute_group ak4376_attr_group = {
	.attrs = ak4376_attributes,
};
#endif

/* ak4376 register cache & default register settings */
static const u8 ak4376_reg[AK4376_MAX_REGISTERS] = {
	0x00,	/*	0x00	AK4376_00_POWER_MANAGEMENT1		*/
	0x00,	/*	0x01	AK4376_01_POWER_MANAGEMENT2		*/
	0x00,	/*	0x02	AK4376_02_POWER_MANAGEMENT3		*/
	0x00,	/*	0x03	AK4376_03_POWER_MANAGEMENT4		*/
	0x00,	/*	0x04	AK4376_04_OUTPUT_MODE_SETTING	*/
	0x00,	/*	0x05	AK4376_05_CLOCK_MODE_SELECT		*/
	0x00,	/*	0x06	AK4376_06_DIGITAL_FILTER_SELECT	*/
	0x00,	/*	0x07	AK4376_07_DAC_MONO_MIXING		*/
	0x00,	/*	0x08	AK4376_08_RESERVED				*/
	0x00,	/*	0x09	AK4376_09_RESERVED				*/
	0x00,	/*	0x0A	AK4376_0A_RESERVED				*/
	0x11,	/*	0x0B	AK4376_0B_LCH_OUTPUT_VOLUME		*/
	0x11,	/*	0x0C	AK4376_0C_RCH_OUTPUT_VOLUME		*/
	0x0B,	/*	0x0D	AK4376_0D_HP_VOLUME_CONTROL		*/
	0x00,	/*	0x0E	AK4376_0E_PLL_CLK_SOURCE_SELECT	*/
	0x00,	/*	0x0F	AK4376_0F_PLL_REF_CLK_DIVIDER1	*/
	0x00,	/*	0x10	AK4376_10_PLL_REF_CLK_DIVIDER2	*/
	0x00,	/*	0x11	AK4376_11_PLL_FB_CLK_DIVIDER1	*/
	0x00,	/*	0x12	AK4376_12_PLL_FB_CLK_DIVIDER2	*/
	0x00,	/*	0x13	AK4376_13_DAC_CLK_SOURCE		*/
	0x00,	/*	0x14	AK4376_14_DAC_CLK_DIVIDER		*/
	0x42,	/*	0x15	AK4376_15_AUDIO_IF_FORMAT		*/
	0x00,	/*	0x16	AK4376_16_DUMMY					*/
	0x00,	/*	0x17	AK4376_17_DUMMY					*/
	0x00,	/*	0x18	AK4376_18_DUMMY					*/
	0x00,	/*	0x19	AK4376_19_DUMMY					*/
	0x00,	/*	0x1A	AK4376_1A_DUMMY					*/
	0x00,	/*	0x1B	AK4376_1B_DUMMY					*/
	0x00,	/*	0x1C	AK4376_1C_DUMMY					*/
	0x00,	/*	0x1D	AK4376_1D_DUMMY					*/
	0x00,	/*	0x1E	AK4376_1E_DUMMY					*/
	0x00,	/*	0x1F	AK4376_1F_DUMMY					*/
	0x00,	/*	0x20	AK4376_20_DUMMY					*/
	0x00,	/*	0x21	AK4376_21_DUMMY					*/
	0x00,	/*	0x22	AK4376_22_DUMMY					*/
	0x00,	/*	0x23	AK4376_23_DUMMY					*/
	0x00,	/*	0x24	AK4376_24_MODE_CONTROL			*/
	0x40,	/*	0x26	AK4376_26_DAC_ADJ1				*/
	0x00,	/*	0x2A	AK4376_2A_DAC_ADJ2				*/
};


/* Output Digital volume control:
 * from -12.5 to 3 dB in 0.5 dB steps (mute instead of -12.5 dB) */
static DECLARE_TLV_DB_SCALE(ovl_tlv, -1250, 50, 0);
static DECLARE_TLV_DB_SCALE(ovr_tlv, -1250, 50, 0);

/* HP-Amp Analog volume control:
 * from -22 to 6 dB in 2 dB steps (mute instead of -42 dB) */
static DECLARE_TLV_DB_SCALE(hpg_tlv, -2200, 200, 0);

static const char *ak4376_ovolcn_select_texts[] = {"Dependent", "Independent"};
static const char *ak4376_mdacl_select_texts[] = {"x1", "x1/2"};
static const char *ak4376_mdacr_select_texts[] = {"x1", "x1/2"};
static const char *ak4376_invl_select_texts[] = {"Normal", "Inverting"};
static const char *ak4376_invr_select_texts[] = {"Normal", "Inverting"};
static const char *ak4376_cpmod_select_texts[] =
		{"Automatic Switching", "+-VDD Operation", "+-1/2VDD Operation"};
static const char *ak4376_hphl_select_texts[] = {"9ohm", "Hi-Z"};
static const char *ak4376_hphr_select_texts[] = {"9ohm", "Hi-Z"};
static const char *ak4376_dacfil_select_texts[]  =
		{"Sharp Roll-Off", "Slow Roll-Off", "Short Delay Sharp Roll-Off", "Short Delay Slow Roll-Off"};
static const char *ak4376_bcko_select_texts[] = {"64fs", "32fs"};
static const char *ak4376_dfthr_select_texts[] = {"Digital Filter", "Bypass"};
static const char *ak4376_ngate_select_texts[] = {"ON", "OFF"};
static const char *ak4376_ngatet_select_texts[] = {"Short", "Long"};

static const struct soc_enum ak4376_dac_enum[] = {
	SOC_ENUM_SINGLE(AK4376_0B_LCH_OUTPUT_VOLUME, 7,
			ARRAY_SIZE(ak4376_ovolcn_select_texts), ak4376_ovolcn_select_texts),
	SOC_ENUM_SINGLE(AK4376_07_DAC_MONO_MIXING, 2,
			ARRAY_SIZE(ak4376_mdacl_select_texts), ak4376_mdacl_select_texts),
	SOC_ENUM_SINGLE(AK4376_07_DAC_MONO_MIXING, 6,
			ARRAY_SIZE(ak4376_mdacr_select_texts), ak4376_mdacr_select_texts),
	SOC_ENUM_SINGLE(AK4376_07_DAC_MONO_MIXING, 3,
			ARRAY_SIZE(ak4376_invl_select_texts), ak4376_invl_select_texts),
	SOC_ENUM_SINGLE(AK4376_07_DAC_MONO_MIXING, 7,
			ARRAY_SIZE(ak4376_invr_select_texts), ak4376_invr_select_texts),
	SOC_ENUM_SINGLE(AK4376_03_POWER_MANAGEMENT4, 2,
			ARRAY_SIZE(ak4376_cpmod_select_texts), ak4376_cpmod_select_texts),
	SOC_ENUM_SINGLE(AK4376_04_OUTPUT_MODE_SETTING, 0,
			ARRAY_SIZE(ak4376_hphl_select_texts), ak4376_hphl_select_texts),
	SOC_ENUM_SINGLE(AK4376_04_OUTPUT_MODE_SETTING, 1,
			ARRAY_SIZE(ak4376_hphr_select_texts), ak4376_hphr_select_texts),
	SOC_ENUM_SINGLE(AK4376_06_DIGITAL_FILTER_SELECT, 6,
			ARRAY_SIZE(ak4376_dacfil_select_texts), ak4376_dacfil_select_texts),
	SOC_ENUM_SINGLE(AK4376_15_AUDIO_IF_FORMAT, 3,
			ARRAY_SIZE(ak4376_bcko_select_texts), ak4376_bcko_select_texts),
	SOC_ENUM_SINGLE(AK4376_06_DIGITAL_FILTER_SELECT, 3,
			ARRAY_SIZE(ak4376_dfthr_select_texts), ak4376_dfthr_select_texts),
	SOC_ENUM_SINGLE(AK4376_06_DIGITAL_FILTER_SELECT, 0,
			ARRAY_SIZE(ak4376_ngate_select_texts), ak4376_ngate_select_texts),
	SOC_ENUM_SINGLE(AK4376_06_DIGITAL_FILTER_SELECT, 1,
			ARRAY_SIZE(ak4376_ngatet_select_texts), ak4376_ngatet_select_texts),
};

static const char *bickfreq_on_select[] = {"32fs", "48fs", "64fs"};
static const char *pllmcki_on_select[] = {"9.6MHz", "11.2896MHz", "12.288MHz", "19.2MHz"};
static const char *lpmode_on_select[] = {"High Performance", "Low Power"};
static const char *xtalfreq_on_select[] = {"12.288MHz", "11.2896MHz"};
static const char *pdn_on_select[] = {"OFF", "ON"};
static const char *power_vdd_off_select[] = {"OFF", "ON"};
static const char *pllmode_on_select[] = {"PLL_OFF", "PLL_BICK_MODE", "PLL_MCKI_MODE", "XTAL_MODE"};
static const char *msmode_on_select[] = {"SLAVE", "MASTER"};
static const char *pll_mode_sample_rate_select[] = {"pll_off",
			"bick_8kHz","bick_16kHz","bick_32kHz", "bick_48kHz", "bick_96kHz", "bick_192kHz", "bick_384kHz",
			"mcki_32kHz", "mcki_48kHz", "mcki_96kHz", "mcki_192kHz", "mcki_384kHz"};
static const char *mclk_select[] = {"OFF", "ON"};

static const struct soc_enum ak4376_bitset_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(bickfreq_on_select), bickfreq_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pllmcki_on_select), pllmcki_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(lpmode_on_select), lpmode_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(xtalfreq_on_select), xtalfreq_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pdn_on_select), pdn_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(power_vdd_off_select), power_vdd_off_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pllmode_on_select), pllmode_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(msmode_on_select), msmode_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pll_mode_sample_rate_select), pll_mode_sample_rate_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mclk_select), mclk_select),
};

static const char * const hifi_switch_vdd_text[] = {
		"OFF", "ON"};
static const char * const hifi_switch_vdd_en_text[] = {
		"OFF", "ON"};
static const char * const hifi_switch_dir_text[] = {
		"OFF", "ON"};
static const char * const hifi_switch_acdc_text[] = {
		"OFF", "ON"};
static const char * const hifi_switch_sel_text[] = {
		"OFF", "ON"};
static const char * const hifi_switch_open_text[] = {
		"OFF", "CODEC_ON" ,"HIFI_ON"};

static const struct soc_enum hifi_switch_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_vdd_text), hifi_switch_vdd_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_vdd_en_text), hifi_switch_vdd_en_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_dir_text), hifi_switch_dir_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_acdc_text), hifi_switch_acdc_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_sel_text), hifi_switch_sel_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hifi_switch_open_text), hifi_switch_open_text),
};

static int ak4376_mclk_enable(struct ak4376_priv *ak4376, bool enable)
{
	int ret = 0;
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	if (!ak4376->mclk) {
		/* No mclk, it's not an error,  */
		loge("%s(%d): ak4376 is NULL!\n", __FUNCTION__,__LINE__);
		return 0;
	}

	if (enable){
		ret = clk_prepare_enable(ak4376->mclk);
		if (ret) {
			loge("%s(%d): ak4376 mclk prepare enable failed %d !\n", __FUNCTION__,__LINE__,ret);
			return ret;
		} else {
			mdelay(1);
		}
	} else {
		clk_disable_unprepare(ak4376->mclk);
	}

	return 0;
}

static int ak4376_set_mcki(struct snd_soc_codec *codec, int fs, int rclk)
{
	u8 mode;
	u8 mode2;
	int mcki_rate;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s fs=%d rclk=%d\n",__FUNCTION__, fs, rclk);

	if ((fs != 0)&&(rclk != 0)) {
		if (rclk > 28800000)
			return -EINVAL;

		if (PLL_OFF == ak4376->nPllMode) {	//PLL_OFF
			mcki_rate = rclk/fs;
		} else {		//XTAL_MODE
			if ( ak4376->xtalfreq == 0 ) {		//12.288MHz
				mcki_rate = 12288000/fs;
			} else {	//11.2896MHz
				mcki_rate = 11289600/fs;
			}
		}

		mode = snd_soc_read(codec, AK4376_05_CLOCK_MODE_SELECT);
		mode &= ~AK4376_CM;

		if (ak4376->lpmode == 0) {				//High Performance Mode
			switch (mcki_rate) {
				case 32:
					mode |= AK4376_CM_0;
					break;
				case 64:
					mode |= AK4376_CM_1;
					break;
				case 128:
					mode |= AK4376_CM_3;
					break;
				case 256:
					mode |= AK4376_CM_0;
					mode2 = snd_soc_read(codec, AK4376_24_MODE_CONTROL);
					if ( fs <= 12000 ) {
						mode2 |= 0x40;	//DSMLP=1
						snd_soc_write(codec, AK4376_24_MODE_CONTROL, mode2);
					} else {
						mode2 &= ~0x40;	//DSMLP=0
						snd_soc_write(codec, AK4376_24_MODE_CONTROL, mode2);
					}
					break;
				case 512:
					mode |= AK4376_CM_1;
					break;
				case 1024:
					mode |= AK4376_CM_2;
					break;
				default:
					return -EINVAL;
			}
		} else {					//Low Power Mode (LPMODE == DSMLP == 1)
			switch (mcki_rate) {
				case 32:
					mode |= AK4376_CM_0;
					break;
				case 64:
					mode |= AK4376_CM_1;
					break;
				case 128:
					mode |= AK4376_CM_3;
					break;
				case 256:
					mode |= AK4376_CM_0;
					break;
				case 512:
					mode |= AK4376_CM_1;
					break;
				case 1024:
					mode |= AK4376_CM_2;
					break;
				default:
					return -EINVAL;
			}
		}

		snd_soc_write(codec, AK4376_05_CLOCK_MODE_SELECT, mode);
	}

	return 0;
}


static int ak4376_set_pllblock(struct snd_soc_codec *codec, int fs)
{
	u8 mode;
	int nMClk, nPLLClk, nRefClk;
	int PLDbit, PLMbit, MDIVbit;
	int PLLMCKI;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	mode = snd_soc_read(codec, AK4376_05_CLOCK_MODE_SELECT);
	mode &= ~AK4376_CM;

		if ( fs <= 24000 ) {
			mode |= AK4376_CM_1;
			nMClk = 512 * fs;
		} else if ( fs <= 96000 ) {
			mode |= AK4376_CM_0;
			nMClk = 256 * fs;
		} else if ( fs <= 192000 ) {
			mode |= AK4376_CM_3;
			nMClk = 128 * fs;
		} else {		//fs > 192kHz
			mode |= AK4376_CM_1;
			nMClk = 64 * fs;
		}

	snd_soc_write(codec, AK4376_05_CLOCK_MODE_SELECT, mode);

	if ( (fs % 8000) == 0 ) {
		nPLLClk = 122880000;
	} else if ( (fs == 11025 ) && ( ak4376->nBickFreq == 1 ) && ( ak4376->nPllMode == BICK_PLL )) {
		nPLLClk = 101606400;
	} else {
		nPLLClk = 112896000;
	}

	if ( BICK_PLL == ak4376->nPllMode ) {		//BICK_PLL (Slave)
		if ( ak4376->nBickFreq == 0 ) {		//32fs
			if ( fs <= 96000 ) PLDbit = 1;
			else if ( fs <= 192000 ) PLDbit = 2;
			else PLDbit = 4;
			nRefClk = 32 * fs / PLDbit;
		}
		else if ( ak4376->nBickFreq == 1 ) {	//48fs
			if ( fs <= 16000 ) PLDbit = 1;
			else if ( fs <= 192000 ) PLDbit = 3;
			else PLDbit = 6;
			nRefClk = 48 * fs / PLDbit;
		} else {  									// 64fs
			if ( fs <= 48000 ) PLDbit = 1;
			else if ( fs <= 96000 ) PLDbit = 2;
			else if ( fs <= 192000 ) PLDbit = 4;
			else PLDbit = 8;
			nRefClk = 64 * fs / PLDbit;
		}
	} else {//MCKI_PLL (Master)
			if ( ak4376->nPllMCKI == 0 ) { //9.6MHz
				PLLMCKI = 9600000;
				if ( (fs % 4000) == 0) nRefClk = 1920000;
				else nRefClk = 384000;
			} else if ( ak4376->nPllMCKI == 1 ) { //11.2896MHz
				PLLMCKI = 11289600;
				if ( (fs % 4000) == 0) return -EINVAL;
				else nRefClk = 2822400;
			} else if ( ak4376->nPllMCKI == 2 ) { //12.288MHz
				PLLMCKI = 12288000;
				if ( (fs % 4000) == 0) nRefClk = 3072000;
				else nRefClk = 768000;
			} else {								//19.2MHz
				PLLMCKI = 19200000;
				if ( (fs % 4000) == 0) nRefClk = 1920000;
				else nRefClk = 384000;
			}
			PLDbit = PLLMCKI / nRefClk;
	}

	PLMbit = nPLLClk / nRefClk;
	MDIVbit = nPLLClk / nMClk;
	logi("ak4376_set_pllblock PLMbit = %d, nPLLClk = %d, nRefClk = %d, MDIVbit = %d, nMClk = %d\n", PLMbit, nPLLClk, nRefClk, MDIVbit, nMClk);

	PLDbit--;
	PLMbit--;
	MDIVbit--;

	//PLD15-0
	snd_soc_write(codec, AK4376_0F_PLL_REF_CLK_DIVIDER1, ((PLDbit & 0xFF00) >> 8));
	snd_soc_write(codec, AK4376_10_PLL_REF_CLK_DIVIDER2, ((PLDbit & 0x00FF) >> 0));
	//PLM15-0
	snd_soc_write(codec, AK4376_11_PLL_FB_CLK_DIVIDER1, ((PLMbit & 0xFF00) >> 8));
	snd_soc_write(codec, AK4376_12_PLL_FB_CLK_DIVIDER2, ((PLMbit & 0x00FF) >> 0));

	if ( BICK_PLL == ak4376->nPllMode ) {	//BICK_PLL (Slave)
		snd_soc_update_bits(codec, AK4376_0E_PLL_CLK_SOURCE_SELECT, 0x03, 0x01);	//PLS=1(BICK)
	} else {										//MCKI PLL (Slave/Master)
		snd_soc_update_bits(codec, AK4376_0E_PLL_CLK_SOURCE_SELECT, 0x03, 0x00);	//PLS=0(MCKI)
	}

	//MDIV7-0
	snd_soc_write(codec, AK4376_14_DAC_CLK_DIVIDER, MDIVbit);

	return 0;
}

static int ak4376_set_timer(struct snd_soc_codec *codec)
{
	int curdata;
	int count, tm, nfs;
	unsigned int ret, lvdtm, vddtm;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	lvdtm = 0;
	vddtm = 0;

	nfs = ak4376->fs1;

	//LVDTM2-0 bits set
	ret = snd_soc_read(codec, AK4376_03_POWER_MANAGEMENT4);
	curdata = (ret & 0x70) >> 4;	//Current data Save
	ret &= ~0x70;
	do {
		count = 1000 * (64 << lvdtm);
		tm = count / nfs;
		if ( tm > LVDTM_HOLD_TIME ) break;
		lvdtm++;
	} while ( lvdtm < 7 );			//LVDTM2-0 = 0~7
	if ( curdata != lvdtm) {
		snd_soc_write(codec, AK4376_03_POWER_MANAGEMENT4, (ret | (lvdtm << 4)));
	}

	//VDDTM3-0 bits set
	ret = snd_soc_read(codec, AK4376_04_OUTPUT_MODE_SETTING);
	curdata = (ret & 0x3C) >> 2;	//Current data Save
	ret &= ~0x3C;
	do {
		count = 1000 * (1024 << vddtm);
		tm = count / nfs;
		if ( tm > VDDTM_HOLD_TIME ) break;
		vddtm++;
	} while ( vddtm < 8 );			//VDDTM3-0 = 0~8
	if ( curdata != vddtm) {
		snd_soc_write(codec, AK4376_04_OUTPUT_MODE_SETTING, (ret | (vddtm<<2)));
	}

	return 0;
}

static int ak4376_set_fmt(struct snd_soc_codec *codec, unsigned int fmt)
{
	u8 format;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d) fmt(0x%x)\n",__FUNCTION__,__LINE__,fmt);

	ak4376_pdn_control(codec, ON);

	/* set master/slave audio interface */
	format = snd_soc_read(codec, AK4376_15_AUDIO_IF_FORMAT);
	format &= ~AK4376_DIF;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBS_CFS:
			format |= AK4376_SLAVE_MODE;
			break;
		case SND_SOC_DAIFMT_CBM_CFM:
			if (ak4376->nDeviceID == 2) {
			format |= AK4376_MASTER_MODE;
			}
			else return -EINVAL;
			break;
		case SND_SOC_DAIFMT_CBS_CFM:
		case SND_SOC_DAIFMT_CBM_CFS:
		default:
			dev_err(codec->dev, "Clock mode unsupported");
			return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			format |= AK4376_DIF_I2S_MODE;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			format |= AK4376_DIF_MSB_MODE;
			break;
		default:
			return -EINVAL;
	}

	/* set format */
	snd_soc_write(codec, AK4376_15_AUDIO_IF_FORMAT, format);

	return 0;
}

static int ak4376_set_mute(struct snd_soc_codec *codec)
{
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,4,0)
        logi("%s(%d) codec->dapm.bias_level=%d\n",__FUNCTION__,__LINE__,snd_soc_codec_get_bias_level(codec));

	if (snd_soc_codec_get_bias_level(codec) <= SND_SOC_BIAS_STANDBY) {
		logd("%s(%d) codec->dapm.bias_level <= SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
		/* for avoiding pop sound , only use ak4376_set_bias_level to turn off pdn */
		if (ak4376->swtich_existence) {
			ak4376_pdn_control(codec, OFF);
		}
	}
#else
	logi("%s(%d) codec->dapm.bias_level=%d\n",__FUNCTION__,__LINE__,codec->dapm.bias_level);

	if (codec->dapm.bias_level <= SND_SOC_BIAS_STANDBY) {
		logd("%s(%d) codec->dapm.bias_level <= SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
		/* for avoiding pop sound , only use ak4376_set_bias_level to turn off pdn */
		if (ak4376->swtich_existence) {
			ak4376_pdn_control(codec, OFF);
		}
	}
#endif
	return 0;
}

static int ak4376_hw_params_set(struct snd_soc_codec *codec)
{
	u8 fs;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): ak4376->freq %d, ak4376->nPllMode %d\n",__FUNCTION__,__LINE__, ak4376->freq, ak4376->nPllMode);

	fs = snd_soc_read(codec, AK4376_05_CLOCK_MODE_SELECT);
	fs &= ~AK4376_FS;

	switch (ak4376->freq) {
		case 8000:
			fs |= AK4376_FS_8KHZ;
			break;
		case 11025:
			fs |= AK4376_FS_11_025KHZ;
			break;
		case 16000:
			fs |= AK4376_FS_16KHZ;
			break;
		case 22050:
			fs |= AK4376_FS_22_05KHZ;
			break;
		case 32000:
			fs |= AK4376_FS_32KHZ;
			break;
		case 44100:
			fs |= AK4376_FS_44_1KHZ;
			break;
		case 48000:
			fs |= AK4376_FS_48KHZ;
			break;
		case 88200:
			fs |= AK4376_FS_88_2KHZ;
			break;
		case 96000:
			fs |= AK4376_FS_96KHZ;
			break;
		case 176400:
			fs |= AK4376_FS_176_4KHZ;
			break;
		case 192000:
			fs |= AK4376_FS_192KHZ;
			break;
		case 352800:
			fs |= AK4376_FS_352_8KHZ;
			break;
		case 384000:
			fs |= AK4376_FS_384KHZ;
			break;
		default:
			return -EINVAL;
	}
	snd_soc_write(codec, AK4376_05_CLOCK_MODE_SELECT, fs);

	if ( PLL_OFF == ak4376->nPllMode ) {		//PLL Off
		snd_soc_update_bits(codec, AK4376_13_DAC_CLK_SOURCE, 0x03, 0x00);	//DACCKS=0
		ak4376_set_mcki(codec, ak4376->freq, ak4376->rclk);
	} else if ( XTAL_MODE == ak4376->nPllMode ) {	//XTAL MODE
		snd_soc_update_bits(codec, AK4376_13_DAC_CLK_SOURCE, 0x03, 0x02);	//DACCKS=2
		ak4376_set_mcki(codec, ak4376->freq, ak4376->rclk);
	} else {											//PLL mode
		snd_soc_update_bits(codec, AK4376_13_DAC_CLK_SOURCE, 0x03, 0x01);	//DACCKS=1
		ak4376_set_pllblock(codec, ak4376->freq);
	}

	ak4376_set_timer(codec);

	return 0;
}

static int get_bickfs(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->nBickFreq;

	return 0;
}

static int ak4376_set_bickfs(struct snd_soc_codec *codec)
{
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);

	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d) nBickFreq=%d\n",__FUNCTION__,__LINE__,ak4376->nBickFreq);

	if ( ak4376->nBickFreq == 0 ) { 	//32fs
		snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x03, 0x01);	//DL1-0=01(16bit, >=32fs)
	} else if( ak4376->nBickFreq == 1 ) {	//48fs
		snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x03, 0x00);	//DL1-0=00(24bit, >=48fs)
	} else {								//64fs
		snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x03, 0x02);	//DL1-0=1x(32bit, >=64fs)
	}

	return 0;
}

static int set_bickfs(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376->nBickFreq = ucontrol->value.enumerated.item[0];

	ak4376_set_bickfs(codec);

	return 0;
}

static int get_pllmcki(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->nPllMCKI;

	return 0;
}

static int set_pllmcki(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376->nPllMCKI = ucontrol->value.enumerated.item[0];
	if (ak4376->nPllMCKI == 0 && (!ak4376->swtich_existence)) {
		logi("set_pllmcki: close PMHLHP\n");
		snd_soc_update_bits(codec, AK4376_03_POWER_MANAGEMENT4, 0x03,0x00);
	}
	return 0;
}

static int get_lpmode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
	return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->lpmode;

	return 0;
}

static int ak4376_set_lpmode(struct snd_soc_codec *codec)
{
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);

	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d) lpmode = %d, ak4376->fs1 = %d, nPllMCKI %d\n",__FUNCTION__,__LINE__, ak4376->lpmode, ak4376->fs1, ak4376->nPllMCKI);
	/* POP noise process: if switch lpmode without complete shutup/down processing, reg0x02 will cause pop
		we need control reg0x03 to avoid */
	if ((ak4376->pdn == ON) && (PMHLHP_CLOSE == 0) && ak4376->nPllMCKI != 0) {
		logi("ak4376_set_lpmode: close PMHLHP\n");
		snd_soc_update_bits(codec, AK4376_03_POWER_MANAGEMENT4, 0x03,0x00);
		PMHLHP_CLOSE = 1;
	} else {
		logi("ak4376_set_lpmode: not close PMHLHP\n");
	}

	if ( ak4376->lpmode == 0 ) { 	//High Performance Mode
		snd_soc_update_bits(codec, AK4376_02_POWER_MANAGEMENT3, 0x10, 0x00);	//LPMODE=0(High Performance Mode)
			if ( ak4376->fs1 <= 12000 ) {
				snd_soc_update_bits(codec, AK4376_24_MODE_CONTROL, 0x40, 0x40);	//DSMLP=1
			} else {
				snd_soc_update_bits(codec, AK4376_24_MODE_CONTROL, 0x40, 0x00);	//DSMLP=0
			}
	} else {							//Low Power Mode
		snd_soc_update_bits(codec, AK4376_02_POWER_MANAGEMENT3, 0x10, 0x10);	//LPMODE=1(Low Power Mode)
		snd_soc_update_bits(codec, AK4376_24_MODE_CONTROL, 0x40, 0x40);			//DSMLP=1
	}
	/* POP noise process: if switch lpmode without complete shutup/down processing, reg0x02 will cause pop
		we need control reg0x03 to avoid */
	if ((ak4376->pdn == ON) && (PMHLHP_CLOSE == 1) && ak4376->nPllMCKI != 0) {
		logi("ak4376_set_lpmode: open PMHLHP\n");
		snd_soc_update_bits(codec, AK4376_03_POWER_MANAGEMENT4, 0x03, 0x03);
		PMHLHP_CLOSE = 0;
	} else {
		logi("ak4376_set_lpmode: not open PMHLHP\n");
	}
	return 0;
}

static int set_lpmode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376->lpmode = ucontrol->value.enumerated.item[0];

	ak4376_set_lpmode(codec);

	return 0;
}

static int get_xtalfreq(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->xtalfreq;

	return 0;
}

static int set_xtalfreq(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376->xtalfreq = ucontrol->value.enumerated.item[0];

	return 0;
}

static int get_pdn(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->pdn;

	return 0;
}

static int set_pdn(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = NULL;
	struct ak4376_priv *ak4376 = NULL;
	int set_pdn_stauts = ucontrol->value.integer.value[0];

	codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	/* for avoiding pop sound , only use ak4376_set_bias_level() to turn off pdn */
	if (set_pdn_stauts == 0 && !ak4376->swtich_existence)
		return 0;

	logi("%s(%d) value =%d\n",__FUNCTION__,__LINE__,set_pdn_stauts);

	ak4376_pdn_control(codec, set_pdn_stauts);

	return 0;
}


static int get_ak4376_vdd_power_off(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->hifi_power_off_flag;

	return 0;
}

static int set_ak4376_vdd_power_off(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	switch (ucontrol->value.integer.value[0]) {
		case OFF:
			ak4376->hifi_power_off_flag = OFF;
			if (ak4376->hifi_pwren > 0) {
				gpio_direction_output(ak4376->hifi_pwren, 1);
			}
			break;
		case ON:
			ak4376->hifi_power_off_flag = ON;
			if (ak4376->hifi_pwren > 0) {
				gpio_direction_output(ak4376->hifi_pwren, 0);
			}
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int hifi_swtich_dir_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->switch_dir_flg;
	return 0;
}

static int hifi_swtich_dir_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	if(ak4376->switch_dir > 0) {
		switch (ucontrol->value.integer.value[0]) {
			case OFF:
				ak4376->switch_dir_flg = OFF;
				gpio_direction_output(ak4376->switch_dir, 0);
				break;
			case ON:
				ak4376->switch_dir_flg = ON;
				gpio_direction_output(ak4376->switch_dir, 1);
				break;
			default:
				return -EINVAL;
		}
	} else {
		loge("%s(%d): switch_dir gpio num is error\n",__FUNCTION__,__LINE__);
	}
	return 0;
}

static int hifi_swtich_acdc_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->switch_ac_dc_flg;
	return 0;
}

static int hifi_swtich_acdc_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	if(ak4376->switch_ac_dc > 0) {
		switch (ucontrol->value.integer.value[0]) {
			case OFF:
				ak4376->switch_ac_dc_flg = OFF;
				gpio_direction_output(ak4376->switch_ac_dc, 0);
				break;
			case ON:
				ak4376->switch_ac_dc_flg = ON;
				gpio_direction_output(ak4376->switch_ac_dc, 1);
				break;
			default:
				return -EINVAL;
		}
	} else {
		loge("%s(%d): switch_ac_dc gpio num is error\n",__FUNCTION__,__LINE__);
	}
	return 0;
}

static int hifi_swtich_open_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->switch_open_flg;
	return 0;
}

static int hifi_swtich_open_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	if(ak4376->switch_mute > 0) {
		switch (ucontrol->value.integer.value[0]) {
			case SWITCH_OFF:
				ak4376->switch_open_flg = SWITCH_OFF;
				gpio_direction_output(ak4376->switch_mute, 1);
				break;
			case CODEC_ON:
				ak4376->switch_open_flg = CODEC_ON;
				gpio_direction_output(ak4376->switch_mute, 0);
				break;
			case HIFI_ON:
				ak4376->switch_open_flg = HIFI_ON;
				gpio_direction_output(ak4376->switch_mute, 0);
				break;
			default:
				return -EINVAL;
		}
	} else {
		loge("%s(%d): switch_mute gpio num is error\n",__FUNCTION__,__LINE__);
	}
	return 0;
}

static int hifi_swtich_sel_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->switch_sel_flg;
	return 0;
}

static int hifi_swtich_sel_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	if(ak4376->switch_sel > 0) {
		switch (ucontrol->value.integer.value[0]) {
			case OFF:
				ak4376->switch_sel_flg = OFF;
				gpio_direction_output(ak4376->switch_sel, 0);
				break;
			case ON:
				ak4376->switch_sel_flg = ON;
				gpio_direction_output(ak4376->switch_sel, 1);
				break;
			default:
				return -EINVAL;
		}
	} else {
		loge("%s(%d): switch_sel gpio num is error\n",__FUNCTION__,__LINE__);
	}
	return 0;
}

static int hifi_swtich_vdd_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->swtich_vdd_flg;
	return 0;
}

static int hifi_swtich_vdd_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	switch (ucontrol->value.integer.value[0]) {
		case OFF:
			ak4376_switch_regulator_disable(ak4376);
			break;
		case ON:
			ak4376_switch_regulator_enable(ak4376);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int hifi_swtich_vdd_en_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);
	ucontrol->value.integer.value[0] = ak4376->switch_power_en_flg;
	return 0;
}

static int hifi_swtich_vdd_en_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value =%ld\n",__FUNCTION__,__LINE__,ucontrol->value.integer.value[0]);
	if(ak4376->switch_power_en > 0) {
		switch (ucontrol->value.integer.value[0]) {
			case OFF:
				ak4376->switch_power_en_flg = OFF;
				gpio_direction_output(ak4376->switch_power_en, 0);
				break;
			case ON:
				ak4376->switch_power_en_flg = ON;
				gpio_direction_output(ak4376->switch_power_en, 1);
				break;
			default:
				return -EINVAL;
		}
	} else {
		loge("%s(%d): switch_power_en gpio num is error\n",__FUNCTION__,__LINE__);
	}
	return 0;
}

static int get_pllmode(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d): FAIL codec is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->nPllMode;

	return 0;
}

static int set_pllmode(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d): FAIL codec is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376->nPllMode = ucontrol->value.enumerated.item[0];

	logi("%s(%d) nPllMode=%d\n",__FUNCTION__,__LINE__,ak4376->nPllMode);

	return 0;
}

static int get_msmode(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d): FAIL codec is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->master_mode;

	return 0;
}

static int set_msmode(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d): FAIL codec is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376->master_mode = ucontrol->value.enumerated.item[0];
	if (AK4376_SLAVE_MODE == ak4376->master_mode) {
		snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x10, 0x00);	//MS bit = 0 slave mode
	} else {
		snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x10, 0x10);	//MS bit = 1 master mode
	}

	logi("%s(%d) master_mode=%d\n",__FUNCTION__,__LINE__,ak4376->master_mode);

	return 0;
}

static int get_pll_mode_and_sample_rate(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = NULL;
	struct ak4376_priv *ak4376 = NULL;
	int pll_mode_and_sample_rate = PLL_OFF_48kHz;	//default is PLL_OFF_48kHz

	if (NULL == kcontrol) {
		loge("%s(%d): FAIL kcontrol is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}
	codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d) FAIL codec is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}
	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__, __LINE__);

	if ((ak4376->freq == SAMPLE_RATE_8KHZ) && (ak4376->nPllMode == BICK_PLL)) {
		pll_mode_and_sample_rate = BICK_PLL_8kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_16KHZ) && (ak4376->nPllMode == BICK_PLL)) {
		pll_mode_and_sample_rate = BICK_PLL_16kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_32KHZ) && (ak4376->nPllMode == BICK_PLL)) {
		pll_mode_and_sample_rate = BICK_PLL_32kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_48KHZ) && (ak4376->nPllMode == BICK_PLL)) {
		pll_mode_and_sample_rate = BICK_PLL_48kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_32KHZ) && (ak4376->nPllMode == MCKI_PLL)) {
		pll_mode_and_sample_rate = MCKI_PLL_32kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_48KHZ) && (ak4376->nPllMode == MCKI_PLL)) {
		pll_mode_and_sample_rate = MCKI_PLL_48kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_96KHZ) && (ak4376->nPllMode == MCKI_PLL)) {
		pll_mode_and_sample_rate = MCKI_PLL_96kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_192KHZ) && (ak4376->nPllMode == MCKI_PLL)) {
		pll_mode_and_sample_rate = MCKI_PLL_192kHz;
	} else if ((ak4376->freq == SAMPLE_RATE_384KHZ) && (ak4376->nPllMode == MCKI_PLL)) {
		pll_mode_and_sample_rate = MCKI_PLL_384kHz;
	} else {
		logi("%s(%d) default value\n",__FUNCTION__, __LINE__);
	}

	ucontrol->value.enumerated.item[0] = pll_mode_and_sample_rate;

	return 0;
}

static int set_pll_mode_and_sample_rate(struct snd_kcontrol *kcontrol,
							struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = NULL;
	struct ak4376_priv *ak4376 = NULL;
	int pll_mode_and_sample_rate = PLL_OFF_48kHz;	//default is PLL_OFF_48kHz

	if (NULL == kcontrol) {
		loge("%s(%d): FAIL kcontrol is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}
	codec = snd_soc_kcontrol_codec(kcontrol);
	if (NULL == codec) {
		loge("%s(%d): FAIL codec is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}
	ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): FAIL ak4376 is null pointer\n",__FUNCTION__, __LINE__);
		return -ENXIO;
	}

	pll_mode_and_sample_rate = ucontrol->value.enumerated.item[0];

	logi("%s(%d): pll_mode_and_sample_rate = %d pll_mode_and_sample_rate_pre = %d\n",__FUNCTION__, __LINE__, pll_mode_and_sample_rate, pll_mode_and_sample_rate_pre);
	if ((pll_mode_and_sample_rate != PLL_OFF_48kHz) && (pll_mode_and_sample_rate_pre != PLL_OFF_48kHz)) {
		if ((ak4376->pdn == ON) && (PMHLHP_CLOSE == 0)) {
			logi("%s(%d): close PMHLHP\n",__FUNCTION__, __LINE__);
			snd_soc_update_bits(codec, AK4376_03_POWER_MANAGEMENT4, 0x03,0x00);
			PMHLHP_CLOSE = 1;
		} else {
			logi("%s(%d): not close PMHLHP ak4376->pdn %d , PMHLHP_CLOSE %d\n",__FUNCTION__, __LINE__, ak4376->pdn, PMHLHP_CLOSE);
		}
	} else {
		logi("%s(%d): not close PMHLHP\n",__FUNCTION__, __LINE__);
	}

	//now rclk only for PLL_OFF mode, and should not be 0
	switch (pll_mode_and_sample_rate) {
		case PLL_OFF_48kHz:
			ak4376->freq = SAMPLE_RATE_48KHZ;
			ak4376->rclk = SAMPLE_RATE_48KHZ;
			ak4376->nPllMode = PLL_OFF;
			break;
		case BICK_PLL_8kHz:
			ak4376->freq = SAMPLE_RATE_8KHZ;
			ak4376->rclk = SAMPLE_RATE_8KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_16kHz:
			ak4376->freq = SAMPLE_RATE_16KHZ;
			ak4376->rclk = SAMPLE_RATE_16KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_32kHz:
			ak4376->freq = SAMPLE_RATE_32KHZ;
			ak4376->rclk = SAMPLE_RATE_32KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_48kHz:
			ak4376->freq = SAMPLE_RATE_48KHZ;
			ak4376->rclk = SAMPLE_RATE_48KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_96kHz:
			ak4376->freq = SAMPLE_RATE_96KHZ;
			ak4376->rclk = SAMPLE_RATE_96KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_192kHz:
			ak4376->freq = SAMPLE_RATE_192KHZ;
			ak4376->rclk = SAMPLE_RATE_192KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case BICK_PLL_384kHz:
			ak4376->freq = SAMPLE_RATE_384KHZ;
			ak4376->rclk = SAMPLE_RATE_384KHZ;
			ak4376->nPllMode = BICK_PLL;
			break;
		case MCKI_PLL_32kHz:
			ak4376->freq = SAMPLE_RATE_32KHZ;
			ak4376->rclk = SAMPLE_RATE_32KHZ;
			ak4376->nPllMode = MCKI_PLL;
			break;
		case MCKI_PLL_48kHz:
			ak4376->freq = SAMPLE_RATE_48KHZ;
			ak4376->rclk = SAMPLE_RATE_48KHZ;
			ak4376->nPllMode = MCKI_PLL;
			break;
		case MCKI_PLL_96kHz:
			ak4376->freq = SAMPLE_RATE_96KHZ;
			ak4376->rclk = SAMPLE_RATE_96KHZ;
			ak4376->nPllMode = MCKI_PLL;
			break;
		case MCKI_PLL_192kHz:
			ak4376->freq = SAMPLE_RATE_192KHZ;
			ak4376->rclk = SAMPLE_RATE_192KHZ;
			ak4376->nPllMode = MCKI_PLL;
			break;
		case MCKI_PLL_384kHz:
			ak4376->freq = SAMPLE_RATE_384KHZ;
			ak4376->rclk = SAMPLE_RATE_384KHZ;
			ak4376->nPllMode = MCKI_PLL;
			break;
		default:
			ak4376->freq = SAMPLE_RATE_48KHZ;
			ak4376->rclk = SAMPLE_RATE_48KHZ;
			ak4376->nPllMode = PLL_OFF;
			break;
	}

	ak4376_hw_params_set(codec);

	if ((pll_mode_and_sample_rate != PLL_OFF_48kHz) && (pll_mode_and_sample_rate_pre != PLL_OFF_48kHz)) {
		if ((ak4376->pdn == ON) && (PMHLHP_CLOSE == 1)) {
			logi("%s(%d): open PMHLHP\n",__FUNCTION__, __LINE__);
			snd_soc_update_bits(codec, AK4376_03_POWER_MANAGEMENT4, 0x03, 0x03);
			PMHLHP_CLOSE = 0;
		} else {
			logi("%s(%d): not open PMHLHP ak4376->pdn %d , PMHLHP_CLOSE %d\n",__FUNCTION__, __LINE__, ak4376->pdn, PMHLHP_CLOSE);
		}
	} else {
		logi("%s(%d): not open PMHLHP\n",__FUNCTION__, __LINE__);
	}

	logi("%s(%d) ak4376->switch_ajust_seq=%d \n",__FUNCTION__, __LINE__, ak4376->switch_ajust_seq);
	if(ON == ak4376->switch_ajust_seq){
		if ((pll_mode_and_sample_rate == PLL_OFF_48kHz) && (pll_mode_and_sample_rate_pre != PLL_OFF_48kHz)) {
			logi("%s(%d): ak4376_pdn_control enter \n",__FUNCTION__, __LINE__);
			if (ON == ak4376->pdn){
				logi("%s(%d): ak4376_pdn_control do turn off pdn\n",__FUNCTION__, __LINE__);
				ak4376_pdn_control(codec, OFF);
			}
		} else {
			logi("%s(%d): ak4376_pdn_control do nothing and leave \n",__FUNCTION__, __LINE__);
		}
	}

	pll_mode_and_sample_rate_pre = pll_mode_and_sample_rate;
	logi("%s(%d) pll_mode_and_sample_rate=%d pll_mode_and_sample_rate_pre=%d\n",__FUNCTION__, __LINE__, pll_mode_and_sample_rate,pll_mode_and_sample_rate_pre);
	return 0;
}

static int get_mclk(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ucontrol->value.enumerated.item[0] = ak4376->hifi_mclk_en;

	return 0;
}

static int set_mclk(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct ak4376_priv *ak4376 = NULL;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logi("%s(%d): value = %ld\n", __FUNCTION__, __LINE__, ucontrol->value.integer.value[0]);
	switch (ucontrol->value.integer.value[0]) {
		case OFF:
			ak4376->hifi_mclk_en = OFF;
			ak4376_mclk_enable(ak4376, false);
			break;
		case ON:
			ak4376->hifi_mclk_en = ON;
			ak4376_mclk_enable(ak4376, true);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

#ifdef AK4376_DEBUG
static const char *test_reg_select[]   =
{
	"read AK4376 Reg 00:24",
};

static const struct soc_enum ak4376_enum[] =
{
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};

static int nTestRegNo = 0;

static int get_test_reg(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* Get the current output routing */
	ucontrol->value.enumerated.item[0] = nTestRegNo;

	return 0;
}

static int set_test_reg(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	u32 currMode = ucontrol->value.enumerated.item[0];
	int i, value;
	int regs, rege;
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);

	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	nTestRegNo = currMode;

	regs = 0x00;
	rege = 0x15;

	for ( i = regs ; i <= rege ; i++ ){
		value = snd_soc_read(codec, i);
		logi("***AK4376 Addr,Reg=(%x, %x)\n", i, value);
	}
	value = snd_soc_read(codec, 0x24);
	logi("***AK4376 Addr,Reg=(%x, %x)\n", 0x24, value);

	return 0;
}
#endif

static const struct snd_kcontrol_new ak4376_snd_controls[] = {
	SOC_SINGLE_TLV("AK4376 Digital Output VolumeL",
			AK4376_0B_LCH_OUTPUT_VOLUME, 0, 0x1F, 0, ovl_tlv),
	SOC_SINGLE_TLV("AK4376 Digital Output VolumeR",
			AK4376_0C_RCH_OUTPUT_VOLUME, 0, 0x1F, 0, ovr_tlv),
	SOC_SINGLE_TLV("AK4376 HP-Amp Analog Volume",
			AK4376_0D_HP_VOLUME_CONTROL, 0, 0x0F, 0, hpg_tlv),
	SOC_SINGLE("AK4376 DAC-ADJ1 Gain",
			AK4376_26_DAC_ADJ1, 0, 0x7F, 0),
	SOC_SINGLE("AK4376 DAC-ADJ2 Gain",
			AK4376_2A_DAC_ADJ2, 0, 0xFF, 0),

	SOC_ENUM("AK4376 Digital Volume Control", ak4376_dac_enum[0]),
	SOC_ENUM("AK4376 DACL Signal Level", ak4376_dac_enum[1]),
	SOC_ENUM("AK4376 DACR Signal Level", ak4376_dac_enum[2]),
	SOC_ENUM("AK4376 DACL Signal Invert", ak4376_dac_enum[3]),
	SOC_ENUM("AK4376 DACR Signal Invert", ak4376_dac_enum[4]),
	SOC_ENUM("AK4376 Charge Pump Mode", ak4376_dac_enum[5]),
	SOC_ENUM("AK4376 HPL Power-down Resistor", ak4376_dac_enum[6]),
	SOC_ENUM("AK4376 HPR Power-down Resistor", ak4376_dac_enum[7]),
	SOC_ENUM("AK4376 DAC Digital Filter Mode", ak4376_dac_enum[8]),
	SOC_ENUM("AK4376 BICK Output Frequency", ak4376_dac_enum[9]),
	SOC_ENUM("AK4376 Digital Filter Mode", ak4376_dac_enum[10]),
	SOC_ENUM("AK4376 Noise Gate", ak4376_dac_enum[11]),
	SOC_ENUM("AK4376 Noise Gate Time", ak4376_dac_enum[12]),

	SOC_ENUM_EXT("AK4376 BICK Frequency Select", ak4376_bitset_enum[0], get_bickfs, set_bickfs),
	SOC_ENUM_EXT("AK4376 PLL MCKI Frequency", ak4376_bitset_enum[1], get_pllmcki, set_pllmcki),
	SOC_ENUM_EXT("AK4376 Low Power Mode", ak4376_bitset_enum[2], get_lpmode, set_lpmode),
	SOC_ENUM_EXT("AK4376 Xtal Frequency", ak4376_bitset_enum[3], get_xtalfreq, set_xtalfreq),
	SOC_ENUM_EXT("AK4376 PDN Control", ak4376_bitset_enum[4], get_pdn, set_pdn),
	SOC_ENUM_EXT("AK4376 VDD Power_Off Control", ak4376_bitset_enum[5], get_ak4376_vdd_power_off, set_ak4376_vdd_power_off),
	SOC_ENUM_EXT("AK4376 PLL Mode", ak4376_bitset_enum[6], get_pllmode, set_pllmode),
	SOC_ENUM_EXT("AK4376 MS Mode", ak4376_bitset_enum[7], get_msmode, set_msmode),
	SOC_ENUM_EXT("AK4376 PLL Mode and Sample Rate", ak4376_bitset_enum[8], get_pll_mode_and_sample_rate, set_pll_mode_and_sample_rate),
	SOC_ENUM_EXT("AK4376 MCLK Control", ak4376_bitset_enum[9], get_mclk, set_mclk),

	SOC_ENUM_EXT("HIFI SWTICH VDD", hifi_switch_enum[0], hifi_swtich_vdd_get, hifi_swtich_vdd_set),
	SOC_ENUM_EXT("HIFI SWTICH VDD EN", hifi_switch_enum[1], hifi_swtich_vdd_en_get, hifi_swtich_vdd_en_set),
	SOC_ENUM_EXT("HIFI SWTICH DIR", hifi_switch_enum[2], hifi_swtich_dir_get, hifi_swtich_dir_set),
	SOC_ENUM_EXT("HIFI SWTICH ACDC", hifi_switch_enum[3], hifi_swtich_acdc_get, hifi_swtich_acdc_set),
	SOC_ENUM_EXT("HIFI SWTICH SEL", hifi_switch_enum[4], hifi_swtich_sel_get, hifi_swtich_sel_set),
	SOC_ENUM_EXT("HIFI SWTICH OPEN", hifi_switch_enum[5], hifi_swtich_open_get, hifi_swtich_open_set),


#ifdef AK4376_DEBUG
	SOC_ENUM_EXT("Reg Read", ak4376_enum[0], get_test_reg, set_test_reg),
#endif

};



/* DAC control */
static int ak4376_dac_event2(struct snd_soc_codec *codec, int event)
{
	u8 MSmode;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	logi("ak4376_dac_event2 enter event %d\n", event);
	MSmode = snd_soc_read(codec, AK4376_15_AUDIO_IF_FORMAT);

	switch (event) {
		case SND_SOC_DAPM_PRE_PMU:	/* before widget power up */
			ak4376->nDACOn = ON;
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x01,0x01);		//PMCP1=1
			mdelay(6);																//wait 6ms
			udelay(500);															//wait 0.5ms
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x30,0x30);		//PMLDO1P/N=1
			mdelay(1);																//wait 1ms
			break;
		case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x02,0x02);		//PMCP2=1
			mdelay(4);																//wait 4ms
			udelay(500);															//wait 0.5ms
			break;
		case SND_SOC_DAPM_PRE_PMD:	/* before widget power down */
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x02,0x00);		//PMCP2=0
			break;
		case SND_SOC_DAPM_POST_PMD:	/* after widget power down */
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x30,0x00);		//PMLDO1P/N=0
			snd_soc_update_bits(codec, AK4376_01_POWER_MANAGEMENT2, 0x01,0x00);		//PMCP1=0

			if (PLL_OFF == ak4376->nPllMode) {
				if (MSmode & 0x10) {	//Master mode
					snd_soc_update_bits(codec, AK4376_15_AUDIO_IF_FORMAT, 0x10,0x00);	//MS bit = 0
				}
			}
			ak4376->nDACOn = OFF;
			break;
	}
	return 0;
}

static int ak4376_dac_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) //CONFIG_LINF
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d),event = %d\n",__FUNCTION__,__LINE__, event);

	ak4376_dac_event2(codec, event);

	return 0;
}

/* PLL control */
static int ak4376_pll_event2(struct snd_soc_codec *codec, int event)
{
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	logi("%s(%d),event = %d\n",__FUNCTION__,__LINE__, event);
	switch (event) {
		case SND_SOC_DAPM_PRE_PMU:	/* before widget power up */
		case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
			/* if hardware without switch , ak4376 maybe be used incall mode , and support changed sample-rate
			so ak4376_hw_params_set should be controled by kcontrol in set_pll_mode_and_sample_rate() */
			if (ak4376->swtich_existence) {
				ak4376_hw_params_set(codec);
			}
			if(!ak4376->master_mode) {
				ak4376_set_fmt(codec, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS);
			} else {
				ak4376_set_fmt(codec, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM);
			}

			if ((BICK_PLL == ak4376->nPllMode) || (MCKI_PLL == ak4376->nPllMode)) {
				snd_soc_update_bits(codec, AK4376_00_POWER_MANAGEMENT1, 0x01,0x01);	//PMPLL=1
			} else if (XTAL_MODE == ak4376->nPllMode) {
				snd_soc_update_bits(codec, AK4376_00_POWER_MANAGEMENT1, 0x10,0x10);	//PMOSC=1
			}
			break;
		case SND_SOC_DAPM_PRE_PMD:	/* before widget power down */
		case SND_SOC_DAPM_POST_PMD:	/* after widget power down */
			if ((BICK_PLL == ak4376->nPllMode) || (MCKI_PLL == ak4376->nPllMode)) {
				snd_soc_update_bits(codec, AK4376_00_POWER_MANAGEMENT1, 0x01,0x00);	//PMPLL=0
			} else if (XTAL_MODE == ak4376->nPllMode) {
				snd_soc_update_bits(codec, AK4376_00_POWER_MANAGEMENT1, 0x10,0x00);	//PMOSC=0
			}
			ak4376_set_mute(codec);
			break;
	}

	return 0;
}

static int ak4376_pll_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) //CONFIG_LINF
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	if(NULL == codec){
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	logd("%s(%d),event = %d\n",__FUNCTION__,__LINE__, event);

	ak4376_pll_event2(codec, event);

	return 0;
}

/* HPL Mixer */
static const struct snd_kcontrol_new ak4376_hpl_mixer_controls[] = {
	SOC_DAPM_SINGLE("LDACL", AK4376_07_DAC_MONO_MIXING, 0, 1, 0),
	SOC_DAPM_SINGLE("RDACL", AK4376_07_DAC_MONO_MIXING, 1, 1, 0),
};

/* HPR Mixer */
static const struct snd_kcontrol_new ak4376_hpr_mixer_controls[] = {
	SOC_DAPM_SINGLE("LDACR", AK4376_07_DAC_MONO_MIXING, 4, 1, 0),
	SOC_DAPM_SINGLE("RDACR", AK4376_07_DAC_MONO_MIXING, 5, 1, 0),
};


/* ak4376 dapm widgets */
static const struct snd_soc_dapm_widget ak4376_dapm_widgets[] = {
// DAC
	SND_SOC_DAPM_DAC_E("AK4376 DAC", "NULL", AK4376_02_POWER_MANAGEMENT3, 0, 0,
			ak4376_dac_event, (SND_SOC_DAPM_POST_PMU |SND_SOC_DAPM_PRE_PMD
							|SND_SOC_DAPM_PRE_PMU |SND_SOC_DAPM_POST_PMD)),

// PLL, OSC
	SND_SOC_DAPM_SUPPLY_S("AK4376 PLL", 0, SND_SOC_NOPM, 0, 0,
			ak4376_pll_event, (SND_SOC_DAPM_POST_PMU |SND_SOC_DAPM_PRE_PMD
							|SND_SOC_DAPM_PRE_PMU |SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_INPUT("AK4376 SDTI"),


// Analog Output
	SND_SOC_DAPM_OUTPUT("AK4376 HPL"),
	SND_SOC_DAPM_OUTPUT("AK4376 HPR"),

	SND_SOC_DAPM_MIXER("AK4376 HPR Mixer", AK4376_03_POWER_MANAGEMENT4, 1, 0,
			&ak4376_hpr_mixer_controls[0], ARRAY_SIZE(ak4376_hpr_mixer_controls)),

	SND_SOC_DAPM_MIXER("AK4376 HPL Mixer", AK4376_03_POWER_MANAGEMENT4, 0, 0,
			&ak4376_hpl_mixer_controls[0], ARRAY_SIZE(ak4376_hpl_mixer_controls)),

};

static const struct snd_soc_dapm_route ak4376_intercon[] =
{

	{"AK4376 DAC", NULL, "AK4376 PLL"},
	{"AK4376 DAC", NULL, "AK4376 SDTI"},

	{"AK4376 HPL Mixer", "LDACL", "AK4376 DAC"},
	{"AK4376 HPL Mixer", "RDACL", "AK4376 DAC"},
	{"AK4376 HPR Mixer", "LDACR", "AK4376 DAC"},
	{"AK4376 HPR Mixer", "RDACR", "AK4376 DAC"},

	{"AK4376 HPL", NULL, "AK4376 HPL Mixer"},
	{"AK4376 HPR", NULL, "AK4376 HPR Mixer"},

};

/* Read ak4376 register cache */
static inline u32 ak4376_read_reg_cache(struct snd_soc_codec *codec, u16 reg)
{
	u8 *cache = codec->reg_cache;
	if(NULL == cache){
		loge("%s(%d): cache is NULL\n",__FUNCTION__,__LINE__);
		return 0;
	}

	if(reg <= AK4376_24_MODE_CONTROL){
		return (u32)cache[reg];
	} else if(AK4376_26_DAC_ADJ1 == reg) {
		return (u32)cache[AK4376_24_MODE_CONTROL+1];
	} else if(AK4376_2A_DAC_ADJ2 == reg) {
		return (u32)cache[AK4376_24_MODE_CONTROL+2];
	}

	loge("%s(%d): reg is invalid value,return 0\n",__FUNCTION__,__LINE__);
	return 0;
}

/* Read ak4376 IC register */
static unsigned int ak4376_i2c_read(u8 *reg, int reglen, u8 *data, int datalen)
{
	struct i2c_msg xfer[2];
	int ret;
	struct ak4376_priv *ak4376;
	struct i2c_client *client;

	if(NULL == ak4376_codec){
		loge("%s: ak4376_codec is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	ak4376 = snd_soc_codec_get_drvdata(ak4376_codec);
	if(NULL == ak4376){
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	client = ak4376->i2c;
	if(NULL == client){
		loge("%s: client is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	// Write register
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = reglen;
	xfer[0].buf = reg;

	// Read data
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = datalen;
	xfer[1].buf = data;

	ret = i2c_transfer(client->adapter, xfer, 2);

	if (ret == 2)
		return 0;
	else if (ret < 0)
		return -ret;
	else
		return -EIO;
}

unsigned int ak4376_reg_read(struct snd_soc_codec *codec, unsigned int reg)
{
	unsigned char tx[1], rx[1];
	int wlen, rlen;
	int ret = 0;
	int retry = 0;
	unsigned int rdata = 0;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);

	if(NULL == ak4376){
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	logd("%s(%d) pdn =%d, hifi_power_off_flag =%d\n",__FUNCTION__,
		__LINE__, ak4376->pdn, ak4376->hifi_power_off_flag);

	if (OFF == ak4376->pdn || ON == ak4376->hifi_power_off_flag) {
		rdata = ak4376_read_reg_cache(codec, reg);
		logd("%s Read reg[0x%x]cache, rdata =%u\n",__FUNCTION__,reg,rdata);
	} else if (ON == ak4376->pdn && OFF == ak4376->hifi_power_off_flag) {
		wlen = 1;
		rlen = 1;
		tx[0] = reg;

		while (retry < I2C_WRITE_READ_RETRY_TIMES) {
			ret = ak4376_i2c_read(tx, wlen, rx, rlen);
			retry++;
			if (ret < 0) {
				loge("%s read register[0x%x] [error = %d retry %d]\n",__FUNCTION__, reg, ret, retry);
#ifdef CONFIG_HUAWEI_DSM
				hifi_dsm_i2c_reg_fail_report();
#endif
				rdata = -EIO;
			} else {
				rdata = (unsigned int)rx[0];
				logd("%s Read IC register[0x%x], rdata =%u\n",__FUNCTION__, reg, rdata);
				break;
			}
		}
	}

	return rdata;
}

/* Write AK4376 register cache */
static inline void ak4376_write_reg_cache(struct snd_soc_codec *codec, u16 reg, u16 value)
{
	u8 *cache = codec->reg_cache;
	if(NULL == cache){
		loge("%s(%d): cache is NULL\n",__FUNCTION__,__LINE__);
		return;
	}

	if(reg <= AK4376_24_MODE_CONTROL){
		cache[reg] = (u8)value;
	} else if(reg == AK4376_26_DAC_ADJ1) {
		cache[AK4376_24_MODE_CONTROL+1] = (u8)value;
	} else if(reg == AK4376_2A_DAC_ADJ2) {
		cache[AK4376_24_MODE_CONTROL+2] = (u8)value;
	}
}

static int ak4376_write_register(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	int wlen;
	int rc = 0;
	int retry = 0;
	unsigned char tx[2];
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376){
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	logd("%s(%d) (%x,%x)\n",__FUNCTION__,__LINE__,reg,value);

	wlen = 2;
	tx[0] = reg;
	tx[1] = value;

	ak4376_write_reg_cache(codec, reg, value);

	if (ON == ak4376->pdn && OFF == ak4376->hifi_power_off_flag) {
		while (retry < I2C_WRITE_READ_RETRY_TIMES) {
			retry++;
			rc = i2c_master_send(ak4376->i2c, tx, wlen);
			if (rc != wlen) {
				loge("%s write register[0x%x] value[0x%x] [error = %d, retry %d]\n",__FUNCTION__, reg, value, rc, retry);
#ifdef CONFIG_HUAWEI_DSM
				hifi_dsm_i2c_reg_fail_report();
#endif
			} else {
				break;
			}
		}
	}

	return rc;
}

// * for AK4376
static int ak4376_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *codec_dai)
{

	return 0;
}

static int ak4376_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{

	return 0;
}

static int ak4376_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{

	return 0;
}

static int ak4376_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{

	return 0;
}

static int ak4376_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int ak4376_show_reg_debug(struct snd_soc_codec *codec)
{
	int i = 0;
	int ret = 0;
	int reg[AK4376_MAX_REG_NUM + 1] = {0};

	if (NULL == codec) {
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}

	for (i = 0; i < AK4376_16_DUMMY; i++) {
		ret = ak4376_reg_read(ak4376_codec, i);

		if (ret < 0) {
			loge("%s(%d): read register[0x%x] error.\n", __FUNCTION__,__LINE__,i);
			break;
		} else {
			reg[i] = ret;
		}
	}
	reg[AK4376_MAX_REG_NUM-2] = ak4376_reg_read(ak4376_codec, AK4376_24_MODE_CONTROL);
	reg[AK4376_MAX_REG_NUM-1] = ak4376_reg_read(ak4376_codec, AK4376_26_DAC_ADJ1);
	reg[AK4376_MAX_REG_NUM] = ak4376_reg_read(ak4376_codec, AK4376_2A_DAC_ADJ2);

	for (i = 0; i < AK4376_16_DUMMY; i++) {
		logi("%s : read register[0x%02x, %02x]\n", __FUNCTION__, i ,reg[i]);
	}
	logi("%s : read register[0x24, %02x]\n", __FUNCTION__, reg[AK4376_MAX_REG_NUM -2]);
	logi("%s : read register[0x26, %02x]\n", __FUNCTION__, reg[AK4376_MAX_REG_NUM -1]);
	logi("%s : read register[0x2a, %02x]\n", __FUNCTION__, reg[AK4376_MAX_REG_NUM]);

	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,4,0)
static int ak4376_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	if (NULL == codec) {
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	if (NULL == ak4376 || NULL == dapm) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	logi("%s(%d) level=%d, codec->dapm.bias_level=%d\n",__FUNCTION__, __LINE__, level, snd_soc_dapm_get_bias_level(dapm));

	switch (level) {
		case SND_SOC_BIAS_ON:
			break;
		case SND_SOC_BIAS_PREPARE:
			if (SND_SOC_BIAS_STANDBY == snd_soc_dapm_get_bias_level(dapm))
				logi("%s(%d) bias_level = SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
			if (SND_SOC_BIAS_ON == snd_soc_dapm_get_bias_level(dapm))
				logi("%s(%d) bias_level = SND_SOC_BIAS_ON\n",__FUNCTION__,__LINE__);
			break;
		case SND_SOC_BIAS_STANDBY:
			if (SND_SOC_BIAS_PREPARE == snd_soc_dapm_get_bias_level(dapm)) {
				logi("%s(%d) bias_level = SND_SOC_BIAS_PREPARE\n",__FUNCTION__,__LINE__);
				if (ak4376->swtich_existence) {
					ak4376_pdn_control(codec, OFF);
				}
			} else if (SND_SOC_BIAS_OFF == snd_soc_dapm_get_bias_level(dapm))
				logi("%s(%d) bias_level = SND_SOC_BIAS_OFF\n",__FUNCTION__,__LINE__);
			break;
		case SND_SOC_BIAS_OFF:
			if (SND_SOC_BIAS_STANDBY == snd_soc_dapm_get_bias_level(dapm)) {
				logi("%s(%d) bias_level = SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
				logi("%s(%d) ak4376->switch_ajust_seq=%d \n",__FUNCTION__, __LINE__, ak4376->switch_ajust_seq);
				if (ak4376->switch_ajust_seq == OFF){
					ak4376_pdn_control(codec, OFF);
				}
			}
			break;
	}
	snd_soc_dapm_init_bias_level(dapm, level);

	/* show chip regs for debug, after enable ak4376 */
	if (SND_SOC_BIAS_ON == level) {
		ak4376_show_reg_debug(codec);
	}

	return 0;
}
#else
static int ak4376_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	if (NULL == codec) {
		loge("%s(%d): codec is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if (NULL == ak4376) {
		loge("%s(%d): ak4376 is NULL\n",__FUNCTION__,__LINE__);
		return -ENXIO;
	}
	logi("%s(%d) level=%d, codec->dapm.bias_level=%d\n",__FUNCTION__, __LINE__, level, codec->dapm.bias_level);

	switch (level) {
		case SND_SOC_BIAS_ON:
			break;
		case SND_SOC_BIAS_PREPARE:
			if (SND_SOC_BIAS_STANDBY == codec->dapm.bias_level)
				logi("%s(%d) bias_level = SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
			if (SND_SOC_BIAS_ON == codec->dapm.bias_level)
				logi("%s(%d) bias_level = SND_SOC_BIAS_ON\n",__FUNCTION__,__LINE__);
			break;
		case SND_SOC_BIAS_STANDBY:
			if (SND_SOC_BIAS_PREPARE == codec->dapm.bias_level) {
				logi("%s(%d) bias_level = SND_SOC_BIAS_PREPARE\n",__FUNCTION__,__LINE__);
				if (ak4376->swtich_existence) {
					ak4376_pdn_control(codec, OFF);
				}
			} else if (SND_SOC_BIAS_OFF == codec->dapm.bias_level)
				logi("%s(%d) bias_level = SND_SOC_BIAS_OFF\n",__FUNCTION__,__LINE__);
			break;
		case SND_SOC_BIAS_OFF:
			if (SND_SOC_BIAS_STANDBY == codec->dapm.bias_level) {
				logi("%s(%d) bias_level = SND_SOC_BIAS_STANDBY\n",__FUNCTION__,__LINE__);
				logi("%s(%d) ak4376->switch_ajust_seq=%d \n",__FUNCTION__, __LINE__, ak4376->switch_ajust_seq);
				if (OFF == ak4376->switch_ajust_seq){
					ak4376_pdn_control(codec, OFF);
				}
			}
			break;
	}
	codec->dapm.bias_level = level;

	/* show chip regs for debug, after enable ak4376 */
	if (SND_SOC_BIAS_ON == level) {
		ak4376_show_reg_debug(codec);
	}

	return 0;
}
#endif

#define AK4376_RATES		(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
				SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
				SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
				SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
				SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |\
				SNDRV_PCM_RATE_192000)

#define AK4376_FORMATS		(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)


static struct snd_soc_dai_ops ak4376_dai_ops = {
	.hw_params	= ak4376_hw_params,
	.set_sysclk	= ak4376_set_dai_sysclk,
	.set_fmt	= ak4376_set_dai_fmt,
	.trigger = ak4376_trigger,
	.digital_mute = ak4376_set_dai_mute,
};

struct snd_soc_dai_driver ak4376_dai[] = {
	{
		.name = "ak4376-AIF1",
		.playback = {
				.stream_name = "Playback",
				.channels_min = 1,
				.channels_max = 2,
				.rates = AK4376_RATES,
				.formats = AK4376_FORMATS,
		},
		.ops = &ak4376_dai_ops,
	},
};

static int ak4376_init_reg(struct snd_soc_codec *codec)
{
	u8 DeviceID;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);
	if(NULL == ak4376){
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376_pdn_control(codec, ON);
	ak4376_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	DeviceID = ak4376_reg_read(codec, AK4376_15_AUDIO_IF_FORMAT);

	switch (DeviceID >> 5) {
		case 0:
			ak4376->nDeviceID = 0;		//0:AK4375
			logi("%s(%d) AK4375 is connecting.\n",__FUNCTION__,__LINE__);
			break;
		case 1:
			ak4376->nDeviceID = 1;		//1:AK4375A
			logi("%s(%d) AK4375A is connecting.\n",__FUNCTION__,__LINE__);
			break;
		case 2:
			ak4376->nDeviceID = 2;		//2:AK4376
			logi("%s(%d) AK4376 is connecting.\n",__FUNCTION__,__LINE__);
			break;
		default:
			ak4376->nDeviceID = 3;		//3:Other IC
			logi("%s(%d) This device are neither AK4375/A nor AK4376.\n",__FUNCTION__,__LINE__);
	}

	#if LINUX_VERSION_CODE > KERNEL_VERSION(4,4,0)
		if (SND_SOC_BIAS_STANDBY == snd_soc_codec_get_bias_level(codec)) {
			if(ak4376->switch_ajust_seq == ON){
				logi("%s(%d) ak4376->switch_ajust_seq=%d, turn off the pdn after init reg \n",__FUNCTION__, __LINE__, ak4376->switch_ajust_seq);
				ak4376_pdn_control(codec, OFF);
			}
		}
	#else
		if (SND_SOC_BIAS_STANDBY == codec->dapm.bias_level) {
			if(ak4376->switch_ajust_seq == ON){
				logi("%s(%d) ak4376->switch_ajust_seq=%d, turn off the pdn after init reg \n",__FUNCTION__, __LINE__, ak4376->switch_ajust_seq);
				ak4376_pdn_control(codec, OFF);
			}
		}
	#endif

	ak4376_set_bias_level(codec, SND_SOC_BIAS_OFF);

	logd("%s(%d)\n",__FUNCTION__,__LINE__);

	return 0;
}

static int ak4376_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct ak4376_priv *ak4376 = snd_soc_codec_get_drvdata(codec);

	if(NULL == ak4376){
		loge("%s: ak4376 is NULL.\n", __FUNCTION__);
		return -EIO;
	}

	logi("%s(%d)\n",__FUNCTION__,__LINE__);

	mutex_init(&io_lock);

	ak4376_codec = codec;

	if(!ak4376->master_mode) {
		ak4376->nPllMode = BICK_PLL;
	} else {
		ak4376->nPllMode = MCKI_PLL;
	}
	ak4376_init_reg(codec);

	ak4376->rclk = 0;
	ak4376->nBickFreq = 1;		//0:32fs, 1:48fs, 2:64fs
	ak4376->nPllMCKI = 3;		//0:9.6MHz, 1:11.2896MHz, 2:12.288MHz, 3:19.2MHz
	ak4376->lpmode = 0;			//0:High Performance mode, 1:Low Power Mode
	ak4376->xtalfreq = 0;		//0:12.288MHz, 1:11.2896MHz
	ak4376->nDACOn = OFF;
	ak4376->fs1 = AK4376_DEFALUT_NORMAL_FREQ;
	return ret;
}

static int ak4376_remove(struct snd_soc_codec *codec)
{

	logi("%s(%d)\n",__FUNCTION__,__LINE__);

	ak4376_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int ak4376_suspend(struct snd_soc_codec *codec)
{

	return 0;
}

static int ak4376_resume(struct snd_soc_codec *codec)
{

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_ak4376 = {
	.probe = ak4376_probe,
	.remove = ak4376_remove,
	.suspend = ak4376_suspend,
	.resume = ak4376_resume,
	.read = ak4376_reg_read,
	.write = ak4376_write_register,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	.component_driver = {
		.controls = ak4376_snd_controls,
		.num_controls = ARRAY_SIZE(ak4376_snd_controls),
		.dapm_widgets = ak4376_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ak4376_dapm_widgets),
		.dapm_routes = ak4376_intercon,
		.num_dapm_routes = ARRAY_SIZE(ak4376_intercon),
	},
#else
	.controls = ak4376_snd_controls,
	.num_controls = ARRAY_SIZE(ak4376_snd_controls),
	.dapm_widgets = ak4376_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak4376_dapm_widgets),
	.dapm_routes = ak4376_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak4376_intercon),
#endif
	.idle_bias_off = true,
	.set_bias_level = ak4376_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(ak4376_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = ak4376_reg,
};

static int ak4376_populate_get_gpio_pdata(struct device *dev, struct ak4376_priv *pdata)
{
	int ret = 0;
	bool isExistGpio = false;
	const char *hifi_pdn = "ak4376,hifi_pdn";
	const char *hifi_pwren = "ak4376,hifi_pwren";

	pdata->priv_pdn_en = of_get_named_gpio(dev->of_node, hifi_pdn, 0);
	if (pdata->priv_pdn_en < 0) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,dev->of_node->full_name, hifi_pdn,
				pdata->priv_pdn_en);
		ret = -EIO;
		goto priv_pdn_en_gpio_request_error;
	}

	if (!gpio_is_valid(pdata->priv_pdn_en)) {
		loge("%s: ak4376_hifi_pdn gpio is invalid!\n",__FUNCTION__);
		ret = -EIO;
		goto priv_pdn_en_gpio_request_error;
	}

	ret = gpio_request(pdata->priv_pdn_en, "ak4376_hifi_pdn");
	if (ret < 0) {
		loge( "%s(): ak4376_hifi_pdn gpio request failed %d",
				__func__, ret);
		goto priv_pdn_en_gpio_request_error;
	}

	isExistGpio = of_property_read_bool(dev->of_node, hifi_pwren);
	if (isExistGpio) {
		pdata->hifi_pwren = of_get_named_gpio(dev->of_node, hifi_pwren, 0);
		if (pdata->hifi_pwren < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
				, __FUNCTION__,dev->of_node->full_name, hifi_pwren,
					pdata->hifi_pwren);
			ret = -EIO;
			goto hifi_pwren_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->hifi_pwren)) {
			loge("%s: ak4376_hifi_pwren gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto hifi_pwren_gpio_request_error;
		}

		ret = gpio_request(pdata->hifi_pwren, "ak4376_hifi_pwren");
		if (ret < 0) {
			loge( "%s(): ak4376_hifi_pwren gpio request failed %d",
				__func__, ret);
			goto hifi_pwren_gpio_request_error;
		}

		gpio_direction_output(pdata->priv_pdn_en, 0);
		pdata->pdn = OFF;

		gpio_direction_output(pdata->hifi_pwren, 1);
		pdata->hifi_power_off_flag = OFF;
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		gpio_direction_output(pdata->priv_pdn_en, 0);
		pdata->pdn = OFF;
		logw("%s:ak4376_hifi_pwren gpio is not exist!\n",__FUNCTION__);
		pdata->hifi_pwren = -1; //invalid gpio
	}

	logd("%s:request ak4376 gpios sucess!\n",__FUNCTION__);
	return 0;

hifi_pwren_gpio_request_error:
	gpio_free(pdata->priv_pdn_en);
priv_pdn_en_gpio_request_error:
	return ret;
}

static int ak4376_populate_get_switch_gpio_pdata(struct device *dev, struct ak4376_priv *pdata)
{
	int ret = 0;
	bool isExistGpio = false;
	const char *switch_acdc = "switch,ac_dc";
	const char *switch_dir = "switch,dir";
	const char *switch_mute = "switch,mute";
	const char *switch_sel = "switch,sel";
	const char *switch_power_en = "switch,power_en";

	/* BTV use analog-switch for switch ak4376 and codec, 
	other device without analog-switch no need to init these resources */
	if (!pdata->swtich_existence) {
		logd("%s(%d):device without analog switch for ak4376!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	isExistGpio = of_property_read_bool(dev->of_node, switch_acdc);
	if (isExistGpio) {
		pdata->switch_ac_dc = of_get_named_gpio(dev->of_node, switch_acdc, 0);
		if (pdata->switch_ac_dc < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
				, __FUNCTION__,dev->of_node->full_name, switch_acdc,
					pdata->switch_ac_dc);
			ret = -EIO;
			goto switch_ac_dc_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->switch_ac_dc)) {
			loge("%s: switch_ac_dc gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto switch_ac_dc_gpio_request_error;
		}
		ret = gpio_request(pdata->switch_ac_dc, "switch_ac_dc");
		if (ret < 0) {
			loge( "%s(): switch_ac_dc gpio request failed %d",
				__func__, ret);
			goto switch_ac_dc_gpio_request_error;
		}
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		logw("%s:switch_ac_dc gpio is not exist!\n",__FUNCTION__);
		pdata->switch_ac_dc = -1; //invalid gpio
	}

	isExistGpio = false;
	isExistGpio = of_property_read_bool(dev->of_node, switch_dir);
	if (isExistGpio) {
		pdata->switch_dir = of_get_named_gpio(dev->of_node, switch_dir, 0);
		if (pdata->switch_dir < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
				, __FUNCTION__,dev->of_node->full_name, switch_dir,
				pdata->switch_dir);
			ret = -EIO;
			goto switch_dir_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->switch_dir)) {
			loge("%s: switch_dir gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto switch_dir_gpio_request_error;
		}
		ret = gpio_request(pdata->switch_dir, "switch_dir");
		if (ret < 0) {
			loge( "%s(): switch_dir gpio request failed %d",
					__func__, ret);
			goto switch_dir_gpio_request_error;
		}
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		logw("%s:switch_dir gpio is not exist!\n",__FUNCTION__);
		pdata->switch_dir = -1; //invalid gpio
	}

	isExistGpio = false;
	isExistGpio = of_property_read_bool(dev->of_node, switch_mute);
	if (isExistGpio) {
		pdata->switch_mute = of_get_named_gpio(dev->of_node, switch_mute, 0);
		if (pdata->switch_mute < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
				, __FUNCTION__,dev->of_node->full_name, switch_mute,
					pdata->switch_mute);
			ret = -EIO;
			goto switch_mute_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->switch_mute)) {
			loge("%s: switch_mute gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto switch_mute_gpio_request_error;
		}
		ret = gpio_request(pdata->switch_mute, "switch_mute");
		if (ret < 0) {
			loge( "%s(): switch_mute gpio request failed %d",
				__func__, ret);
			goto switch_mute_gpio_request_error;
		}
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		logw("%s:switch_mute gpio is not exist!\n",__FUNCTION__);
		pdata->switch_mute = -1; //invalid gpio
	}

	isExistGpio = false;
	isExistGpio = of_property_read_bool(dev->of_node, switch_sel);
	if (isExistGpio) {
		pdata->switch_sel = of_get_named_gpio(dev->of_node, switch_sel, 0);
		if (pdata->switch_sel < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
				, __FUNCTION__,dev->of_node->full_name, switch_sel,
				pdata->switch_sel);
			ret = -EIO;
			goto switch_sel_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->switch_sel)) {
			loge("%s: switch_sel gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto switch_sel_gpio_request_error;
		}

		ret = gpio_request(pdata->switch_sel, "switch_sel");
		if (ret < 0) {
			loge("%s(%d): switch_sel gpio request failed %d",
				__FUNCTION__,__LINE__, ret);
			goto switch_sel_gpio_request_error;
		}
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		logw("%s:switch_sel gpio is not exist!\n",__FUNCTION__);
		pdata->switch_sel = -1; //invalid gpio
	}

	isExistGpio = false;
	isExistGpio = of_property_read_bool(dev->of_node, switch_power_en);
	if (isExistGpio) {
		pdata->switch_power_en = of_get_named_gpio(dev->of_node, switch_power_en, 0);
		if (pdata->switch_power_en < 0) {
			logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,dev->of_node->full_name, switch_power_en,
					pdata->switch_power_en);
			ret = -EIO;
			goto switch_power_en_gpio_request_error;
		}

		if (!gpio_is_valid(pdata->switch_power_en)) {
			loge("%s: switch_power_en gpio is invalid!\n",__FUNCTION__);
			ret = -EIO;
			goto switch_power_en_gpio_request_error;
		}

		ret = gpio_request(pdata->switch_power_en, "swtich_power_en");
		if (ret < 0) {
			loge( "%s(): switch_power_en gpio request failed %d",
					__func__, ret);
			goto switch_power_en_gpio_request_error;
		}
	} else {
		/*The gpio does not use in some product, for it is always enable in the product.*/
		logw("%s:switch_power_en gpio is not exist!\n",__FUNCTION__);
		pdata->switch_power_en = -1; //invalid gpio
	}

	if (pdata->switch_mute > 0) {
		gpio_direction_output(pdata->switch_mute, 1);
		pdata->switch_open_flg = SWITCH_OFF;
	}

	if (pdata->switch_sel > 0) {
		gpio_direction_output(pdata->switch_sel, 0);
		pdata->switch_sel_flg = OFF;
	}

	if (pdata->switch_dir > 0) {
		gpio_direction_output(pdata->switch_dir, 1);
		pdata->switch_dir_flg = ON;
	}

	if (pdata->switch_ac_dc > 0) {
		gpio_direction_output(pdata->switch_ac_dc, 0);
		pdata->switch_ac_dc_flg = OFF;
	}

	if (pdata->switch_power_en > 0) {
		gpio_direction_output(pdata->switch_power_en, 1);
		pdata->switch_power_en_flg = ON;
	}

	return 0;

switch_power_en_gpio_request_error:
	if (pdata->switch_sel > 0) {
		gpio_free(pdata->switch_sel);
	}
switch_sel_gpio_request_error:
	if (pdata->switch_mute > 0) {
		gpio_free(pdata->switch_mute);
	}
switch_mute_gpio_request_error:
	if (pdata->switch_dir > 0) {
		gpio_free(pdata->switch_dir);
	}
switch_dir_gpio_request_error:
	if (pdata->switch_ac_dc > 0) {
		gpio_free(pdata->switch_ac_dc);
	}
switch_ac_dc_gpio_request_error:
	return ret;
}

static void ak4376_free_gpio_pdata(struct ak4376_priv *pdata)
{
	logd("%s(%d):free gpios !\n",__FUNCTION__,__LINE__);

	if(pdata->switch_sel > 0)
		gpio_free(pdata->switch_sel);

	if(pdata->switch_mute > 0)
		gpio_free(pdata->switch_mute);

	if(pdata->switch_dir > 0)
		gpio_free(pdata->switch_dir);

	if(pdata->switch_ac_dc > 0)
		gpio_free(pdata->switch_ac_dc);

	if(pdata->hifi_pwren > 0)
		gpio_free(pdata->hifi_pwren);

	if(pdata->priv_pdn_en > 0)
		gpio_free(pdata->priv_pdn_en);

}

static int ak4376_switch_get_regulator(struct device *dev, struct ak4376_priv *pdata)
{
	int ret = 0;

	const char *regulator_config = "ak4376,regulator_config";
	const char *switch_voltage = "switch,voltage";
	const char *switch_vdd = "switch-vdd";

	/* BTV use analog-switch for switch ak4376 and codec, 
	other device without analog-switch no need to init these resources*/
	if (!pdata->swtich_existence) {
		logd("%s(%d):device without analog switch for ak4376!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	ret = of_property_read_u32(dev->of_node, regulator_config, &pdata->regulator_config);
	if (ret) {
		logi("%s(%d): missing %s in dt node and set dafault value\n", __FUNCTION__,__LINE__, regulator_config);
		pdata->regulator_config = 0;
	} else {
		logi("%s(%d): read %s value is %d \n", __FUNCTION__,__LINE__, regulator_config, pdata->regulator_config);
	}

	if (!pdata->regulator_config) {
		logi("%s(%d): device no need to configure regulator\n", __FUNCTION__,__LINE__);
		return 0;
	}

	logd("%s(%d):regulator configure!\n",__FUNCTION__,__LINE__);

	ret = of_property_read_u32(dev->of_node, switch_voltage, &pdata->switch_voltage);
	if (ret) {
		logi("%s(%d): missing %s in dt node and set dafault value\n", __FUNCTION__,__LINE__, switch_voltage);
		pdata->switch_voltage = AK4376_SWTICH_VOLTAGE_VALUE;
	}
	logd("%s(%d): switch_voltage =%d\n", __FUNCTION__,__LINE__, pdata->switch_voltage);

	pdata->swtich_vdd = regulator_get(dev, switch_vdd);
	if (IS_ERR(pdata->swtich_vdd) || !pdata->swtich_vdd)
	{
		ret = PTR_ERR(pdata->swtich_vdd);
		loge("%s(%d): Regulator get failed swtich_vdd rc = %d\n" ,__FUNCTION__,__LINE__, ret);
		return -EINVAL;
	}

	return 0;
}

static int ak4376_put_switch_regulator(struct ak4376_priv *pdata)
{
	/* BTV use analog-switch for switch ak4376 and codec, 
	other device without analog-switch no need to init these resources*/
	if (!pdata->swtich_existence) {
		logd("%s(%d):device without analog switch for ak4376!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	if (!pdata->regulator_config) {
		logd("%s(%d): device no need to configure regulator\n", __FUNCTION__,__LINE__);
		return 0;
	}

	if (IS_ERR(pdata->swtich_vdd)) {
		loge("%s(%d): regulator swtich_vdd is err\n" ,__FUNCTION__,__LINE__);
		return -EINVAL;
	}

	regulator_put(pdata->swtich_vdd);

	return 0;
}

static int ak4376_switch_regulator_enable(struct ak4376_priv *pdata)
{
	int ret = 0;

	/* BTV use analog-switch for switch ak4376 and codec, 
	other device without analog-switch no need to init these resources*/
	if (!pdata->swtich_existence) {
		logd("%s(%d):device without analog switch for ak4376!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	if (!pdata->regulator_config) {
		logd("%s(%d): device no need to configure regulator\n", __FUNCTION__,__LINE__);
		return 0;
	}

	if (IS_ERR(pdata->swtich_vdd)) {
		loge("%s(%d): regulator swtich_vdd is err\n" ,__FUNCTION__,__LINE__);
		return -EINVAL;
	}

	ret = regulator_set_voltage(pdata->swtich_vdd, pdata->switch_voltage,
							pdata->switch_voltage);
	if (ret) {
		loge("%s(%d): regulator set_vtg failed_vdd rc=%d\n", __FUNCTION__, __LINE__,ret);
		return -EINVAL;
	}

	ret = regulator_enable(pdata->swtich_vdd);
	if (ret) {
		loge("%s(%d): regulator swtich_vdd failed rc=%d\n", __FUNCTION__, __LINE__, ret);
		return -EINVAL;
	}

	pdata->swtich_vdd_flg = ON;

	return 0;
}

static int ak4376_switch_regulator_disable(struct ak4376_priv *pdata)
{
	int ret = 0;

	/* BTV use analog-switch for switch ak4376 and codec, 
	other device without analog-switch no need to init these resources*/
	if (!pdata->swtich_existence) {
		logd("%s(%d):device without analog switch for ak4376!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	if (!pdata->regulator_config) {
		logd("%s(%d): device no need to configure regulator\n", __FUNCTION__,__LINE__);
		return 0;
	}

	if (IS_ERR(pdata->swtich_vdd)) {
		loge("%s(%d): regulator swtich_vdd is err\n" ,__FUNCTION__,__LINE__);
		return -EINVAL;
	}

	ret = regulator_disable(pdata->swtich_vdd);
	if (ret) {
		loge("%s(%d): regulator swtich_vdd disable err\n" ,__FUNCTION__,__LINE__);
		return -EINVAL;
	}

	pdata->swtich_vdd_flg = OFF;

	return 0;
}

/* if device have extern codec use pmu clk together with hi6xxx, need enhance pmu xo_freq_codec drv
   k3Vx platform handle this in audio_codec.c with fastboot 
   0X11A bit[1:0]: 00->3pf//100k, 11->30pf//100k */
static void set_PMU_xo_freq_codec_drv_config(struct device_node *node)
{
	if (NULL == node) {
		pr_err("%s: intput parameters is null pointer ! \n", __FUNCTION__);
		return;
	}
	unsigned int val = 0;
	unsigned int reg_val_new = 0;
	unsigned int reg_val_old = 0;

	if (!of_property_read_u32(node, "hi6555_PMU_CODEC_CLK_DRV", &val)) {
		if(val <= 0x3) {
			reg_val_old = hisi_pmic_reg_read(HI6555_0X11A_REG);
			reg_val_new = (reg_val_old & ~HI6555_0X11A_XO_FREQ_CODEC_MASK) | (val & HI6555_0X11A_XO_FREQ_CODEC_MASK);
			hisi_pmic_reg_write(HI6555_0X11A_REG, reg_val_new);
			pr_info("%s: update PMU 0X11A from 0x%x to 0x%x , val = %x\n", __FUNCTION__, reg_val_old, reg_val_new, val);
		} else {
			pr_err("%s: invalid value for reg %d\n", __FUNCTION__, val);
		}
	} else {
		pr_info("%s: no need to change hi6555 config\n", __FUNCTION__);
	}
}

static int ak4376_i2c_probe(struct i2c_client *i2c,
							const struct i2c_device_id *id)
{
	struct ak4376_priv *ak4376;
	int ret=0;
	const char *master_mode = "ak4376,master_mode";
	const char *work_freq = "ak4376,work_freq";
	const char *swtich_delaytime = "ak4376,swtich_delaytime";
	const char *swtich_existence = "ak4376,swtich_existence";
	const char *switch_ajust_seq = "ak4376,switch_ajust_seq";

	logi("%s(%d)\n",__FUNCTION__,__LINE__);
	ak4376 = kzalloc(sizeof(struct ak4376_priv), GFP_KERNEL);

	if (NULL == ak4376){
		loge("%s(%d): ak4376 priv data is NULL\n",__FUNCTION__,__LINE__);
		return -ENOMEM;
	}
#ifdef AK4376_KERNEL_4_1_DEBUG
	if ((ret = sysfs_create_group(&i2c->dev.kobj, &ak4376_attr_group)) < 0) {
		loge("%s: failed to register ak4376 i2c sysfs, ret =%d\n", __func__, ret);
	}
#endif

	i2c_set_clientdata(i2c, ak4376);

	ak4376->i2c = i2c;
	ak4376->pdn = OFF;
	ak4376->priv_pdn_en = OFF;
	ak4376->switch_ajust_seq = OFF;
	logd("%s(%d) pre register codec\n", __FUNCTION__,__LINE__);

	if (NULL == i2c->dev.of_node) {
		loge("%s: i2c->dev.of_node is NULL\n", __FUNCTION__);
		goto get_gpio_err;
	}

	dev_set_name(&i2c->dev, "%s", "ak4376-codec");

	ret = of_property_read_u32(i2c->dev.of_node, master_mode, &ak4376->master_mode);
	if (ret) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,i2c->dev.of_node->full_name, master_mode, ret);
		ak4376->master_mode = 0;
	}

	ret = of_property_read_u32(i2c->dev.of_node, swtich_delaytime, &ak4376->swtich_delaytime);
	if (ret) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,i2c->dev.of_node->full_name, swtich_delaytime, ret);
		ak4376->swtich_delaytime = 0;
	}

	ret = of_property_read_u32(i2c->dev.of_node, swtich_existence, &ak4376->swtich_existence);
	if (ret) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,i2c->dev.of_node->full_name, swtich_existence, ret);
		ak4376->swtich_existence = 1;
	}

	ret = of_property_read_u32(i2c->dev.of_node, switch_ajust_seq, &ak4376->switch_ajust_seq);
	if (ret) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,i2c->dev.of_node->full_name, switch_ajust_seq, ret);
		ak4376->switch_ajust_seq = OFF;
	}

	ret = of_property_read_u32(i2c->dev.of_node, work_freq, &ak4376->freq);
	if (ret) {
		logw("%s:Looking up %s property in node %s failed %d\n"
			, __FUNCTION__,i2c->dev.of_node->full_name, work_freq, ret);
		ak4376->freq = AK4376_DEFALUT_NORMAL_FREQ;
	}

	ret = ak4376_populate_get_gpio_pdata(&i2c->dev, ak4376);
	if (ret < 0){
		loge("%s(%d): get gpios failed\n",__FUNCTION__,__LINE__);
		goto get_gpio_err;
	}

	ret = ak4376_populate_get_switch_gpio_pdata(&i2c->dev, ak4376);
	if (ret < 0) {
		loge("%s(%d): get switch gpios failed\n",__FUNCTION__,__LINE__);
		goto get_gpio_err;
	}

	ret = ak4376_switch_get_regulator(&i2c->dev, ak4376);
	if (ret < 0){
		loge("%s(%d): get regulator failed\n",__FUNCTION__,__LINE__);
		goto get_regulator_err;
	}

	ret = ak4376_switch_regulator_enable(ak4376);
	if (ret < 0){
		loge("%s(%d): enable switch regulator failed\n",__FUNCTION__,__LINE__);
		goto get_regulator_err;
	}

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak4376, &ak4376_dai[0], ARRAY_SIZE(ak4376_dai));
	if (ret < 0){
		loge("%s(%d): register codec failed\n",__FUNCTION__,__LINE__);
#ifdef CONFIG_HUAWEI_DSM
		audio_dsm_report_info(AUDIO_CODEC, DSM_HIFI_AK4376_CODEC_PROBE_ERR, "ak4376 snd_soc_register_codec fail! %d.\n",ret);
#endif
		goto register_codec_regulator_err;
	}

	set_PMU_xo_freq_codec_drv_config(i2c->dev.of_node);

	ak4376->mclk = devm_clk_get(&i2c->dev, "clk_pmuaudioclk");
	if (IS_ERR(ak4376->mclk)) {
		loge("%s(%d): ak4376 mclk not found\n", __FUNCTION__,__LINE__);
		ret = PTR_ERR(ak4376->mclk);
		goto register_codec_regulator_err;
	}

	ret = clk_prepare_enable(ak4376->mclk);
        if (ret) {
		loge("%s(%d): ak4376 mclk enable failed, ret = %d\n", __FUNCTION__,__LINE__, ret);
        }
	ak4376->hifi_mclk_en = ON;
	logi("%s(%d) register codec ok, pdn=%d, freq =%d\n", __FUNCTION__, __LINE__, 
		ak4376->pdn, ak4376->freq);

	return ret;

register_codec_regulator_err:
	ak4376_put_switch_regulator(ak4376);
get_regulator_err:
	ak4376_free_gpio_pdata(ak4376);
get_gpio_err:
	kfree(ak4376);
	return ret;
}

static int ak4376_i2c_remove(struct i2c_client *client)
{
	struct ak4376_priv *ak4376 = i2c_get_clientdata(client);
	logi("%s(%d)\n",__FUNCTION__,__LINE__);

	if(NULL == ak4376) {
		loge("%s(%d): ak4376 priv data is NULL\n",__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

#ifdef AK4376_KERNEL_4_1_DEBUG
	sysfs_remove_group(&client->dev.kobj, &ak4376_attr_group);
#endif

	ak4376_put_switch_regulator(ak4376);
	ak4376_free_gpio_pdata(ak4376);
	kfree(ak4376);

	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static void ak4376_i2c_shutdown(struct i2c_client *client)
{
	struct ak4376_priv *ak4376 = i2c_get_clientdata(client);
	logi("%s(%d)\n", __FUNCTION__, __LINE__);

	if(NULL == ak4376) {
		loge("%s(%d): ak4376 priv data is NULL\n",__FUNCTION__,__LINE__);
		return ;
	}

	if(SWITCH_OFF != ak4376->switch_open_flg)
		gpio_direction_output(ak4376->switch_mute, 1);

}

static int ak4376_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	logi("%s(%d)\n", __FUNCTION__, __LINE__);
	strncpy(info->type, AK4376_NAME, sizeof(AK4376_NAME));
	return 0;
}

static struct of_device_id ak4376_match_tbl[] = {
	{ .compatible = "huawei,AK4376" },
	{ },
};
MODULE_DEVICE_TABLE(of, ak4376_match_tbl);

static const struct i2c_device_id ak4376_i2c_id[] = {
	{ AK4376_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ak4376_i2c_id);

const unsigned short ak4376_i2c_addr[] = {0x10, 0x00};
static struct i2c_driver ak4376_i2c_driver = {
	.driver = {
		.name = AK4376_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ak4376_match_tbl),
	},
	.probe = ak4376_i2c_probe,
	.shutdown = ak4376_i2c_shutdown,
	.remove = ak4376_i2c_remove,
	.detect = ak4376_i2c_detect,
	.id_table = ak4376_i2c_id,
	.address_list = ak4376_i2c_addr,
};

static int __init ak4376_init(void)
{
	logi("%s(%d)\n", __FUNCTION__,__LINE__);

	return i2c_add_driver(&ak4376_i2c_driver);
}

fs_initcall(ak4376_init);

static void __exit ak4376_exit(void)
{
	logi("%s(%d)\n", __FUNCTION__,__LINE__);

	i2c_del_driver(&ak4376_i2c_driver);
}
module_exit(ak4376_exit);

MODULE_DESCRIPTION("ASoC ak4376 codec driver");
MODULE_LICENSE("GPL");
