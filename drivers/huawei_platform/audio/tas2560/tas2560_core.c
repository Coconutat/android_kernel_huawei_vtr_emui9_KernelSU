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
**     tas2560-core.c
**
** Description:
**     TAS2560 common functions for Android Linux
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include "tas2560.h"
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif
//#include "tas2560_core.h"
/*lint -e845 -e747 -e64 -e30 -e785 -e438 -e712 -e550 -e50 -e529 -e820*/
/*lint -e774 -e778 -e838 -e753 -e750 -e530 -e1564 -e142 -e679 -e826*/
#define TAS2560_MDELAY 0xFFFFFFFE
#define MAX_SUPPORTED_FADETYPE  9
#define MAX_IRQ_TYPE 	9
#define MAX_CLIENTS 	8
#define REG_CAP_MAX 	102
#define RECOVERY_RETRY_TIME 	2
static int dump_regs 	= 0;
int irq_status_reg1 	= 0;
int irq_status_reg2 	= 0;

#if 1
static unsigned int p_tas2560_startup_data[] =
{
	/* reg address			size	values	*/
	TAS2560_CLK_SEL, 		0x01,	0x01,
	TAS2560_SET_FREQ, 		0x01,	0x10,
	TAS2560_MUTE_REG,		0x01,	0x41,
	TAS2560_MDELAY,			0x01,	0x10,

	0xFFFFFFFF, 0xFFFFFFFF
};
#endif

static unsigned int fadein_fadeout_table[MAX_SUPPORTED_FADETYPE][5] =
{
	{24, 	 0x00,	0xAA,	0x39,	0x0},
	{48, 	 0x00,	0x55,	0x38,	0x0},
	{120, 	 0x00,	0x22,	0x1D,	0x0},
	{240, 	 0x00,	0x11,	0x0F,	0x0},
	{480, 	 0x00,	0x08,	0x88,	0x0},
	{960,  	 0x00,	0x04,	0x44,	0x0},
	{1000,	 0x00,	0x04,	0x18,	0x0},
	{1500,	 0x00,	0x02,	0xBB,	0x0},
	{2000, 	 0x00,	0x02,	0x0C,	0x0}
};
static unsigned int p_tas2560_boost_headroom_data[] =
{
	TAS2560_BOOST_HEAD,		0x04,	0x06, 0x66, 0x66, 0x00,

	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_thermal_foldback[] =
{
	TAS2560_THERMAL_FOLDBACK,		0x04,	0x39, 0x80, 0x00, 0x00,

	0xFFFFFFFF, 0xFFFFFFFF
};
static unsigned int p_tas2560_PPG_data[] =
{
	TAS2560_PPG,	0x04,	0x65, 0xac, 0x8c, 0x2e,

	0xFFFFFFFF, 0xFFFFFFFF
};
static unsigned int p_tas2560_HPF_data[] =
{
	/* reg address			size	values */
	/*Isense path HPF cut off -> 2Hz*/
	TAS2560_ISENSE_PATH_CTL1,	0x04,	0x7F, 0xFB, 0xB5, 0x00,
	TAS2560_ISENSE_PATH_CTL2,	0x04,	0x80, 0x04, 0x4C, 0x00,
	TAS2560_ISENSE_PATH_CTL3,	0x04,	0x7F, 0xF7, 0x6A, 0x00,
	/*all pass*/
	TAS2560_HPF_CUTOFF_CTL1,	0x04,	0x7F, 0xFF, 0xFF, 0xFF,
	TAS2560_HPF_CUTOFF_CTL2,	0x04,	0x00, 0x00, 0x00, 0x00,
	TAS2560_HPF_CUTOFF_CTL3,	0x04,	0x00, 0x00, 0x00, 0x00,

	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_Vsense_biquad_data[] =
{
	/* reg address			size	values */
	/* vsense delay in biquad = 3/8 sample @48KHz */
	TAS2560_VSENSE_DEL_CTL1,	0x04,	0x3a, 0x46, 0x74, 0x00,
	TAS2560_VSENSE_DEL_CTL2,	0x04,	0x22, 0xf3, 0x07, 0x00,
	TAS2560_VSENSE_DEL_CTL3,	0x04,	0x80, 0x77, 0x61, 0x00,
	TAS2560_VSENSE_DEL_CTL4,	0x04,	0x22, 0xa7, 0xcc, 0x00,
	TAS2560_VSENSE_DEL_CTL5,	0x04,	0x3a, 0x0c, 0x93, 0x00,

	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_48khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x08,
	TAS2560_SR_CTRL3,		0x01,	0x10,
	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_16khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x18,
	TAS2560_SR_CTRL3,		0x01,	0x20,
	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_8khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x30,
	TAS2560_SR_CTRL3,		0x01,	0x20,
	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_4Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x6f, 0x5c, 0x28, 0xf5,
	TAS2560_BOOST_OFF,		0x04,	0x67, 0xae, 0x14, 0x7a,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1c, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x1f, 0x0a, 0x3d, 0x70,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x22, 0x14, 0x7a, 0xe1,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x25, 0x1e, 0xb8, 0x51,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x28, 0x28, 0xf5, 0xc2,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2b, 0x33, 0x33, 0x33,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x2e, 0x3d, 0x70, 0xa3,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x31, 0x47, 0xae, 0x14,
	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_6Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x73, 0x33, 0x33, 0x33,
	TAS2560_BOOST_OFF,		0x04,	0x6b, 0x85, 0x1e, 0xb8,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1d, 0x99, 0x99, 0x99,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x20, 0xcc, 0xcc, 0xcc,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x24, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x27, 0x33, 0x33, 0x33,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x2a, 0x66, 0x66, 0x66,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2d, 0x99, 0x99, 0x99,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x30, 0xcc, 0xcc, 0xcc,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x34, 0x00, 0x00, 0x00,
	0xFFFFFFFF, 0xFFFFFFFF
};

static unsigned int p_tas2560_8Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x75, 0xc2, 0x8e, 0x00,
	TAS2560_BOOST_OFF,		0x04,	0x6e, 0x14, 0x79, 0x00,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1e, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x21, 0x3d, 0x71, 0x00,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x24, 0x7a, 0xe1, 0x00,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x27, 0xb8, 0x52, 0x00,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x2a, 0xf5, 0xc3, 0x00,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2e, 0x33, 0x33, 0x00,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x31, 0x70, 0xa4, 0x00,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x34, 0xae, 0x14, 0x00,
	0xFFFFFFFF, 0xFFFFFFFF
};

irqs irqtable[MAX_IRQ_TYPE] =
{
	{INT_SAR, 	0x00000002, 	0,	0x26,	"INT_SAR"	},
	{INT_CLK2,	0x00000004,	0,	0x26,	"INT_CLK2"	},
	{INT_BRNO,	0x00000008,	0,	0x26,	"INT_BRNO"	},
	{INT_OVRT,	0x00000010,	0,	0x26,	"INT_OVRT"	},
	{INT_CLK1,	0x00000020,	0,	0x26,	"INT_CLK1"	},
	{INT_AUV,	0x00000040,	0,	0x26,	"INT_AUV"	},
	{INT_OVRI,	0x00000080,	0,	0x26,	"INT_OVRI"	},
	{INT_MCHLT,	0x00000040,	0,	0x27,	"INT_MCHLT"	},
	{INT_WCHLT,	0x00000080,	0,	0x27,	"INT_WCHLT"	},
};

regs regtable[REG_CAP_MAX] =
{
	{0,	0},	{1,	0},	{2,	0},	{3,	0},	{4,	0},
	{5,	0},	{6,	0},	{7,	0},	{8,	0},	{9,	0},
	{10,	0},	{11,	0},	{12,	0},	{13,	0},	{14,	0},
	{15,	0},	{16,	0},	{17,	0},	{18,	0},	{19,	0},
	{20,	0},	{21,	0},	{22,	0},	{23,	0},	{24,	0},
	{25,	0},	{26,	0},	{27,	0},	{28,	0},	{29,	0},
	{30,	0},	{31,	0},	{32,	0},	{33,	0},	{34,	0},
	{35,	0},	{36,	0},	{37,	0},	{38,	0},	{39,	0},
	{40,	0},	{41,	0},	{42,	0},	{43,	0},	{44,	0},
	{45,	0},	{46,	0},	{47,	0},	{48,	0},	{49,	0},
	{50,	0},	{51,	0},	{52,	0},	{53,	0},	{54,	0},
	{55,	0},	{56,	0},	{57,	0},	{58,	0},	{59,	0},
	{60,	0},	{61,	0},	{62,	0},	{63,	0},	{64,	0},
	{65,	0},	{66,	0},	{67,	0},	{68,	0},	{69,	0},
	{70,	0},	{71,	0},	{72,	0},	{73,	0},	{74,	0},
	{75,	0},	{76,	0},	{77,	0},	{78,	0},	{79,	0},
	{80,	0},	{81,	0},	{82,	0},	{83,	0},	{84,	0},
	{85,	0},	{86,	0},	{87,	0},	{88,	0},	{89,	0},
	{90,	0},	{91,	0},	{92,	0},	{93,	0},	{94,	0},
	{95,	0},	{96,	0},	{97,	0},	{98,	0},	{99,	0},
	{100,0},	{101,0},
};
static int tas2560_i2c_load_data(struct tas2560_priv *pTAS2560,
				  unsigned int *pData) {
	unsigned int nRegister;
	unsigned int *nData;
	unsigned char Buf[128];
	unsigned long nLength = 0;
	unsigned int nSize = 0;
	unsigned int i =0;
	int ret = 0;

	do{
		nRegister = pData[nLength];
		nSize = pData[nLength + 1];
		nData = &pData[nLength + 2];
		if (nRegister == TAS2560_MDELAY){
			mdelay(nData[0]);/*lint !e647*/

		}
		else{
			if (nRegister != 0xFFFFFFFF){
				if(nSize > 128){
					dev_err(pTAS2560->dev,
						"invalid size, maximum is 128 bytes!\n");
					break;
				}

				if(nSize > 1){
					for(i = 0; i < nSize; i++) Buf[i] = (unsigned char)nData[i];
					ret += pTAS2560->bulk_write(pTAS2560, nRegister, Buf, nSize);
				}else if(nSize == 1){
					ret += pTAS2560->write(pTAS2560,nRegister, nData[0]);
				}else{
					dev_err(pTAS2560->dev,
						"invalid size, minimum is 1 bytes!\n");
				}
			}
		}
		nLength = nLength + 2 + pData[nLength+1] ;
	} while (nRegister != 0xFFFFFFFF);

	return ret;
}

void tas2560_sw_shutdown(struct tas2560_priv *pTAS2560,
			        int sw_shutdown)
{
	if (sw_shutdown)
		pTAS2560->update_bits(pTAS2560, TAS2560_PWR_REG,
				    TAS2560_PWR_BIT_MASK,0);
	else
		pTAS2560->update_bits(pTAS2560, TAS2560_PWR_REG,
				    TAS2560_PWR_BIT_MASK, TAS2560_PWR_BIT_MASK);
}

int tas2560_set_SampleRate(struct tas2560_priv *pTAS2560, unsigned int nSamplingRate)
{
	int ret = 0;

	switch(nSamplingRate){
	case 48000:
		dev_dbg(pTAS2560->dev,"Sampling rate = 48 khz\n");
		tas2560_i2c_load_data(pTAS2560,p_tas2560_48khz_data);
		break;
	case 44100:
		dev_dbg(pTAS2560->dev,"Sampling rate = 44.1 khz\n");
		pTAS2560->write(pTAS2560, TAS2560_SR_CTRL1, 0x11);
		break;
	case 16000:
		dev_dbg(pTAS2560->dev,"Sampling rate = 16 khz\n");
		tas2560_i2c_load_data(pTAS2560,p_tas2560_16khz_data);
		break;
	case 8000:
		dev_dbg(pTAS2560->dev,"Sampling rate = 8 khz\n");
		tas2560_i2c_load_data(pTAS2560,p_tas2560_8khz_data);
		break;
	default:
		dev_err(pTAS2560->dev,"Invalid Sampling rate, %d\n", nSamplingRate);
		ret = -1;
		break;
	}

	if(ret >= 0)
		pTAS2560->mnSamplingRate = nSamplingRate;

	return ret;
}

int tas2560_set_bit_rate(struct tas2560_priv *pTAS2560, unsigned int nBitRate)
{
	unsigned int n = 2;
	int ret = 0;

	dev_dbg(pTAS2560->dev, " nBitRate = %d \n",
		nBitRate);
	switch(nBitRate){
		case 16:
			n = 0;
			break;
		case 20:
			n = 1;
			break;
		case 24:
			n = 2;
			break;
		case 32:
			n = 3;
			break;
		default:
			n = 2;
			break;
	}

	ret = pTAS2560->update_bits(pTAS2560, TAS2560_DAI_FMT, 0x03, n);

	return ret;
}

int tas2560_get_bit_rate(struct tas2560_priv *pTAS2560)
{
	int nBitRate = -1, ret = 0;
	unsigned int value = 0;

	ret = pTAS2560->read(pTAS2560,TAS2560_DAI_FMT,&value,LOG_ENABLE);
	if(ret){
		dev_err(pTAS2560->dev, " tas2560_get_bit_rate read failed \n");
	}
	value &= 0x03;

	switch(value){
		case 0:
			nBitRate = 16;
		break;
		case 1:
			nBitRate = 20;
		break;
		case 2:
			nBitRate = 24;
		break;
		case 3:
			nBitRate = 32;
		break;
		default:
		break;
	}

	return nBitRate;
}

int tas2560_set_ASI_fmt(struct tas2560_priv *pTAS2560, unsigned int fmt)
{
	u8 serial_format = 0, asi_cfg_1=0;
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		asi_cfg_1 = 0x00;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		asi_cfg_1 = TAS2560_WCLKDIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		asi_cfg_1 = TAS2560_BCLKDIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		asi_cfg_1 = (TAS2560_BCLKDIR | TAS2560_WCLKDIR);
		break;
	default:
		dev_err(pTAS2560->dev, "ASI format master is not found\n");
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		asi_cfg_1 |= 0x00;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		asi_cfg_1 |= TAS2560_WCLKINV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		asi_cfg_1 |= TAS2560_BCLKINV;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		asi_cfg_1 = (TAS2560_WCLKINV | TAS2560_BCLKINV);
		break;
	default:
		dev_err(pTAS2560->dev, "ASI format Inverse is not found\n");
		ret = -EINVAL;
	}

	pTAS2560->update_bits(pTAS2560, TAS2560_ASI_CFG_1, TAS2560_DIRINV_MASK,
			    asi_cfg_1);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK ){
	case (SND_SOC_DAIFMT_I2S ):
		serial_format |= TAS2560_DATAFORMAT_I2S;
		break;
	case (SND_SOC_DAIFMT_DSP_A ):
		serial_format |= TAS2560_DATAFORMAT_DSP;
		break;
      case  (SND_SOC_DAIFMT_DSP_B ):
		serial_format |= TAS2560_DATAFORMAT_MONOPCM;
		break;
	case (SND_SOC_DAIFMT_RIGHT_J):
		serial_format |= TAS2560_DATAFORMAT_RIGHT_J;
		break;
	case (SND_SOC_DAIFMT_LEFT_J):
		serial_format |= TAS2560_DATAFORMAT_LEFT_J;
		break;
	default:
		dev_err(pTAS2560->dev, "DAI Format is not found, fmt=0x%x\n", fmt);
		ret = -EINVAL;
		break;
	}

	pTAS2560->update_bits(pTAS2560, TAS2560_DAI_FMT, TAS2560_DAI_FMT_MASK,
			    serial_format);

	return ret;
}

int tas2560_set_pll_clkin(struct tas2560_priv *pTAS2560, int clk_id,
				  unsigned int freq)
{
	int ret = 0;
	unsigned char pll_in = 0;
	dev_dbg(pTAS2560->dev, "%s, clkid=%d\n", __func__, clk_id);

	switch (clk_id) {
	case TAS2560_PLL_CLKIN_BCLK:
		pll_in = 0;
		break;
	case TAS2560_PLL_CLKIN_MCLK:
		pll_in = 1;
		break;
	case TAS2560_PLL_CLKIN_PDMCLK:
		pll_in = 2;
		break;
	default:
		dev_err(pTAS2560->dev, "Invalid clk id: %d\n", clk_id);
		ret = -EINVAL;
		break;
	}

	if(ret >= 0){
		pTAS2560->update_bits(pTAS2560,
			TAS2560_CLK_SEL,
			TAS2560_PLL_SRC_MASK,
			pll_in<<6);
		pTAS2560->mnClkid = clk_id;
		pTAS2560->mnClkin = freq;
	}

	return ret;
}

int tas2560_setupPLL(struct tas2560_priv *pTAS2560, unsigned int pll_clkin)
{
	unsigned int pll_clk = pTAS2560->mnSamplingRate * 1024;
	unsigned int power = 0, temp;
	unsigned int d = 1, pll_clkin_divide = 1;
	unsigned char j = 1, p = 1;
	int ret = 0;

	if (!pll_clkin) {
		if (pTAS2560->mnClkid != TAS2560_PLL_CLKIN_BCLK){
			dev_err(pTAS2560->dev,
				"pll_in %d, pll_clkin frequency err:%d\n",
				pTAS2560->mnClkid, pll_clkin);
			return -EINVAL;
		}

		pll_clkin = pTAS2560->mnSamplingRate * pTAS2560->mnFrameSize;
	}

	pTAS2560->read(pTAS2560,TAS2560_PWR_REG,&power,LOG_ENABLE);
	if(power&TAS2560_PWR_BIT_MASK){
		dev_dbg(pTAS2560->dev, "power down to update PLL\n");
		pTAS2560->write(pTAS2560, TAS2560_PWR_REG, TAS2560_PWR_BIT_MASK|TAS2560_MUTE_MASK);
		mdelay(1);
	}

	/* Fill in the PLL control registers for J & D
	 * pll_clk = (pll_clkin * J.D) / P
	 * Need to fill in J and D here based on incoming freq
	 */
	if(pll_clkin <= 40000000)
		p = 1;
	else if(pll_clkin <= 80000000)
		p = 2;
	else if(pll_clkin <= 160000000)
		p = 3;
	else{
		dev_err(pTAS2560->dev, "PLL Clk In %d not covered here\n", pll_clkin);
		ret = -EINVAL;
	}

	if(ret >=0){
		j = (unsigned char)((pll_clk * p) / pll_clkin);
		d = (pll_clk * p) % pll_clkin;
		d /= (pll_clkin / 10000);

		pll_clkin_divide = pll_clkin/((unsigned int)1<<p);

		if((d ==0)
			&&((pll_clkin_divide <512000)|| (pll_clkin_divide >20000000))){
			dev_err(pTAS2560->dev, "PLL cal ERROR!!!, pll_in=%d\n", pll_clkin);
			ret = -EINVAL;
		}

		if((d!=0)
			&&((pll_clkin_divide < 10000000)||(pll_clkin_divide>20000000))){
			dev_err(pTAS2560->dev, "PLL cal ERROR!!!, pll_in=%d\n", pll_clkin);
			ret = -EINVAL;
		}

		if(j == 0){
			dev_err(pTAS2560->dev, "PLL cal ERROR!!!, j ZERO\n");
			ret = -EINVAL;
		}
	}

	if(ret >= 0){
		dev_info(pTAS2560->dev,
			"PLL clk_in = %d, P=%d, J.D=%d.%d\n", pll_clkin, p, j, d);
		//update P
		if(p == 64) temp = 0;
		else temp = p;
		pTAS2560->update_bits(pTAS2560, TAS2560_CLK_SEL, TAS2560_PLL_P_MASK, temp);

		//Update J
		temp = j;
		if(pll_clkin < 1000000) temp |= 0x80;
		pTAS2560->write(pTAS2560, TAS2560_SET_FREQ, temp);

		//Update D
		temp = (d&0x00ff);
		pTAS2560->write(pTAS2560, TAS2560_PLL_D_LSB, temp);
		temp = ((d&0x3f00)>>8);
		pTAS2560->write(pTAS2560, TAS2560_PLL_D_MSB, temp);
	}

	/* Restore PLL status */
	if(power&TAS2560_PWR_BIT_MASK){
		pTAS2560->write(pTAS2560, TAS2560_PWR_REG, power);
		mdelay(1);
	}

	return ret;
}

int tas2560_setLoad(struct tas2560_priv *pTAS2560, int load)
{
	int ret = 0;
	unsigned int value = 0;

	dev_dbg(pTAS2560->dev,"%s:0x%x\n",__func__, load);

	switch(load){
	case LOAD_8OHM:
		value = 0;
		tas2560_i2c_load_data(pTAS2560,p_tas2560_8Ohm_data);
		break;
	case LOAD_6OHM:
		value = 1;
		tas2560_i2c_load_data(pTAS2560,p_tas2560_6Ohm_data);
		break;
	case LOAD_4OHM:
		value = 2;
		tas2560_i2c_load_data(pTAS2560,p_tas2560_4Ohm_data);
		break;
	default:
		break;
	}

	pTAS2560->update_bits(pTAS2560, TAS2560_LOAD, LOAD_MASK, value<<3);

	return ret;
}

int tas2560_getLoad(struct tas2560_priv *pTAS2560)
{
	int ret = -1;
	int value = -1;

	dev_dbg(pTAS2560->dev,"%s\n",__func__);
	pTAS2560->read(pTAS2560, TAS2560_LOAD, &value,LOG_ENABLE);

	value = (value&0x18)>>3;

	switch(value){
	case 0:
		ret = LOAD_8OHM;
		break;
	case 1:
		ret = LOAD_6OHM;
		break;
	case 2:
		ret = LOAD_4OHM;
		break;
	default:
		break;
	}

	return ret;
}

int tas2560_get_volume(struct tas2560_priv *pTAS2560)
{
	int ret = -1;
	int value = -1;

	dev_dbg(pTAS2560->dev,"%s\n",__func__);
	ret = pTAS2560->read(pTAS2560, TAS2560_SPK_CTRL_REG, &value,LOG_ENABLE);
	if(ret >=0)
		return (value&0x0f);

	return ret;
}

int tas2560_set_volume(struct tas2560_priv *pTAS2560, int volume)
{
	int ret = -1;

	dev_dbg(pTAS2560->dev,"%s\n",__func__);
	ret = pTAS2560->update_bits(pTAS2560, TAS2560_SPK_CTRL_REG, 0x0f, ((unsigned int)volume)&0x0f);

	return ret;
}

int tas2560_mute(struct tas2560_priv *pTAS2560, int mute)
{
	int ret = -1;

	pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG1, 0x0);
	pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG2, 0xAA);
	pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG3, 0x39);
	pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG4, 0x0);

	dev_dbg(pTAS2560->dev,"%s device mute %d\n",__func__,mute);
	ret = pTAS2560->update_bits(pTAS2560, TAS2560_MUTE_REG, 0x01, ((unsigned int)mute)&0x1);

	return ret;
}

int tas2560_fadeIn_fadeout(struct tas2560_priv *pTAS2560, int muteflag, unsigned int time)
{
	int i = 0;
	int ret = -1;
	int find_reg_val = -1;

	for(i=0;i<MAX_SUPPORTED_FADETYPE;i++){
		if(time == fadein_fadeout_table[i][0]){
			find_reg_val = i;
		}
	}

	if((find_reg_val>=0)&&(find_reg_val<MAX_SUPPORTED_FADETYPE)){
		pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG1, fadein_fadeout_table[find_reg_val][1]);
		pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG2, fadein_fadeout_table[find_reg_val][2]);
		pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG3, fadein_fadeout_table[find_reg_val][3]);
		pTAS2560->write(pTAS2560,TAS2560_FADETIME_CTRL_REG4, fadein_fadeout_table[find_reg_val][4]);
		ret = pTAS2560->update_bits(pTAS2560, TAS2560_MUTE_REG, 0x01, ((unsigned int)muteflag)&0x1);
	}else{
		dev_err(pTAS2560->dev,"%s find compared time failed\n",__func__);
	}

	return ret;
}

int tas2560_parse_dt(struct device *dev,
			struct tas2560_priv *pTAS2560)
{
	struct device_node *np = dev->of_node;
	int rc= 0, ret = 0;

	rc = of_property_read_u32(np, "pa_type", &pTAS2560->pa_type);
	if (rc) {
		dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
			"pa_type", rc);
		dev_info(pTAS2560->dev, "config pa_type as patype_default\n");
		pTAS2560->pa_type = patype_default;
		ret = -1;
	}else{
		dev_dbg(pTAS2560->dev, "dts: pa_type=%d", pTAS2560->pa_type);
	}

	rc = of_property_read_u32(np, "iv_type", &pTAS2560->iv_type);
	if (rc) {
		dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
			"iv_type", rc);
		dev_info(pTAS2560->dev, "config iv_type as iv_default\n");
		pTAS2560->iv_type = iv_default;
		ret = -1;
	}else{
		dev_dbg(pTAS2560->dev, "dts: iv_type=%d", pTAS2560->iv_type);
	}

	rc = of_property_read_u32(np, "gain", &pTAS2560->gain);
	if(rc){
	        dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
	                "gain", rc);
		dev_info(pTAS2560->dev, "config gain as default\n");
	        pTAS2560->gain = 15;
	        ret = -1;
	}else{
	        dev_dbg(pTAS2560->dev, "dts: gain=%d", pTAS2560->gain);
	}

	rc = of_property_read_u32(np, "gain_incall", &pTAS2560->gain_incall);
	if(rc){
	        dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
	                "gain_incall", rc);
		dev_info(pTAS2560->dev, "config gain_incall as default\n");
	        pTAS2560->gain_incall = 15;
	        ret = -1;
	}else{
	        dev_dbg(pTAS2560->dev, "dts: gain_incall=%d", pTAS2560->gain_incall);
	}

	rc = of_property_read_u32(np, "load_type", &pTAS2560->load_type);
	if (rc) {
		dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
			"load_type", rc);
		dev_info(pTAS2560->dev, "config load_type as load_default\n");
		pTAS2560->load_type = load_default;
		ret = -1;
	}else{
		dev_dbg(pTAS2560->dev, "dts: load_type=%d", pTAS2560->load_type);
	}

	rc = of_property_read_u32(np, "gpio_reset", &pTAS2560->mnResetGPIO);
	if (rc) {
		dev_err(pTAS2560->dev, "no specified node %s in dts, seek ret%d\n",
			"gpio_reset", rc);
		ret = -1;
	}else{
		dev_dbg(pTAS2560->dev, "pin gpio_reset=%d", pTAS2560->mnResetGPIO);
	}

	rc = of_property_read_u32(np, "ti,ppg", &pTAS2560->mnPPG);
	if (rc) {
		dev_err(pTAS2560->dev, "Looking up %s property in node %s failed %d\n",
			"ti,ppg", np->full_name, rc);
		dev_info(pTAS2560->dev, "no need to enable PPG ,use default setting ,mnPPG %d\n",pTAS2560->mnPPG);
		ret = -1;
	} else {
		dev_dbg(pTAS2560->dev, "dts: ti,ppg=%d", pTAS2560->mnPPG);
	}

	return ret;
}

int tas2560_enable(struct tas2560_priv *pTAS2560, bool bEnable)
{
	int ret = 0;
	if (bEnable) {
		if (!pTAS2560->mbPowerUp) {
			dev_dbg(pTAS2560->dev,"%s power up\n",__func__);
			/* set clk, power up device mute class d*/
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_startup_data);
			if (pTAS2560->mnPPG)
				tas2560_i2c_load_data(pTAS2560, p_tas2560_PPG_data);
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_HPF_data);
			if(_8OHM_LOAD == pTAS2560->load_type){
				ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_8Ohm_data);
			}else{
				ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_6Ohm_data);
			}
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_boost_headroom_data);
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_thermal_foldback);
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_Vsense_biquad_data);
			ret += tas2560_i2c_load_data(pTAS2560,p_tas2560_48khz_data);

			/*unmute device */
			ret += pTAS2560->write(pTAS2560,TAS2560_MUTE_REG, 0x40);

			pTAS2560->mbPowerUp = true;
		}
	} else {
		if (pTAS2560->mbPowerUp) {
			dev_dbg(pTAS2560->dev,"%s power down\n",__func__);

			/*mute the device */
			ret += pTAS2560->write(pTAS2560,TAS2560_MUTE_REG, 0x41);
			/* power down the device */
			ret += pTAS2560->write(pTAS2560,TAS2560_MUTE_REG, 0x01);
			mdelay(30);
			pTAS2560->mbPowerUp = false;
		}
	}

	return ret;
}

void tas2560_irq_enable(struct tas2560_priv *pTAS2560)
{
	if(NULL == pTAS2560){
		return;
	}
	dev_info(pTAS2560->dev,"now enable tas2560 irq\n");
	pTAS2560->write(pTAS2560,TAS2560_IRQ_PIN_CFG,0x21);
	pTAS2560->write(pTAS2560,TAS2560_INT_CFG_1,0x0);
	pTAS2560->write(pTAS2560,TAS2560_INT_CFG_2,0xFF);

	return;
}
void tas2560_dump_regs(struct tas2560_priv *pTAS2560)
{
	int i = 0;
	dev_info(pTAS2560->dev,">>>dump tas2560 regs..<<<\n");
	for(i=0;i<REG_CAP_MAX;i++){
		pTAS2560->read(pTAS2560,(regtable[i].reg_index),&(regtable[i].reg_val),LOG_DISABLE);
	}
	dev_info(pTAS2560->dev,"dump tas2560 regs finished:\n");
	for(i=0;i<REG_CAP_MAX/4;i++){
		dev_info(pTAS2560->dev,"0x%x=0x%x	0x%x=0x%x	0x%x=0x%x	0x%x=0x%x\n",
				regtable[4*i].reg_index, regtable[4*i].reg_val, regtable[4*i+1].reg_index, regtable[4*i+1].reg_val,
				regtable[4*i+2].reg_index,regtable[4*i+2].reg_val,regtable[4*i+3].reg_index,regtable[4*i+3].reg_val);
	}
	if(REG_CAP_MAX%4){
		for(i=4*(REG_CAP_MAX/4);i<REG_CAP_MAX;i++){
			dev_info(pTAS2560->dev,"0x%x=0x%x	",regtable[i].reg_index,regtable[i].reg_val);
		}
	}
	dev_info(pTAS2560->dev,"\n");

	return;
}

void tas2560_reprogram_chip(struct tas2560_priv *pTAS2560)
{
	int ret = 0;
	/*hardware reset*/
	if(pTAS2560->mnResetGPIO > 0){
		if (!gpio_is_valid(pTAS2560->mnResetGPIO)) {
			pr_err("%s: mnResetGPIO = %d is invalid\n", __func__, pTAS2560->mnResetGPIO);
		} else {
			gpio_direction_output(pTAS2560->mnResetGPIO, 0);
			mdelay(3);
			gpio_direction_output(pTAS2560->mnResetGPIO, 1);
			msleep(1);
		}
	}

	/*soft ware reset*/
	ret = pTAS2560->write(pTAS2560, TAS2560_SW_RESET_REG, 0x01);
	if(ret < 0){
		dev_err(pTAS2560->dev, "ERROR I2C comm, %d\n", ret);
		return;
	}

	mdelay(1);

	/*basic config*/
	pTAS2560->write(pTAS2560,TAS2560_DR_BOOST_REG_1, 0x0c);
	pTAS2560->write(pTAS2560,TAS2560_DR_BOOST_REG_2, 0x33);
	if(_8OHM_LOAD == pTAS2560->load_type){
		pTAS2560->write(pTAS2560,TAS2560_LOAD, 0x83);
	}else{
		pTAS2560->write(pTAS2560,TAS2560_LOAD, 0x8b);
	}
	pTAS2560->write(pTAS2560,TAS2560_DEV_MODE_REG, 0x02);
	if(hi6402_24bit == pTAS2560->iv_type){
		tas2560_set_ASI_fmt(pTAS2560, SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_NB_NF|SND_SOC_DAIFMT_I2S);
		pTAS2560->write(pTAS2560, ASI_OFFSET_1, 0x00);
		tas2560_set_bit_rate(pTAS2560, 24);
	}else{
		tas2560_set_ASI_fmt(pTAS2560, SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_IB_IF|SND_SOC_DAIFMT_DSP_A);
		pTAS2560->write(pTAS2560, ASI_OFFSET_1, 0x01);
		tas2560_set_bit_rate(pTAS2560, 16);
	}

	/*disable*/
	tas2560_enable(pTAS2560, false);
	/*enable*/
	tas2560_enable(pTAS2560, true);

	mdelay(1);
}

void tas2560_force_reprogram_chip(struct tas2560_priv *pTAS2560)
{
    if (pTAS2560 == NULL)
    {
        printk(KERN_ERR "%s: input null pointer\n", __func__);
        return;
    }
    tas2560_reprogram_chip(pTAS2560);
    tas2560_irq_enable(pTAS2560);
}

void tas2560_handle_irq(struct work_struct *work)
{
	int i = 0;
	int config_status = 0xFF;
	int power_reg = 0x40;
	int power_status_reg = 0xFC;
	struct tas2560_priv *pTAS2560 =
		container_of(work, struct tas2560_priv, tas_irq_handle_work);

	dev_info(pTAS2560->dev,"now comes a tas2560 irq\n");

	mdelay(20);

	pTAS2560->read(pTAS2560,TAS2560_INT_CFG_2,&config_status,LOG_ENABLE);
	if((0xFF != config_status) && (pTAS2560->mbPowerUp)){
		dev_info(pTAS2560->dev,"config_reg corrupt unexceptly 0x%x,should be 0xFF,reset chip\n",config_status);
		for(i=0;i<RECOVERY_RETRY_TIME;i++){
			tas2560_reprogram_chip(pTAS2560);
		}
	}else if(0xFF == config_status){
		pTAS2560->write(pTAS2560,TAS2560_INT_CFG_2,0x0);
		pTAS2560->read(pTAS2560,TAS2560_INT_DET_1,&irq_status_reg1,LOG_ENABLE);
		pTAS2560->read(pTAS2560,TAS2560_INT_DET_2,&irq_status_reg2,LOG_ENABLE);
		pTAS2560->read(pTAS2560,TAS2560_PWR_REG,&power_reg,LOG_ENABLE);
		pTAS2560->read(pTAS2560,TAS2560_PWR_STATUS_REG,&power_status_reg,LOG_ENABLE);
		dev_info(pTAS2560->dev,"read 0x07:0x%x		0x26:0x%x	0x27:0x%x	0x2A:0x%x\n",
			      power_reg,irq_status_reg1,irq_status_reg2,power_status_reg);
		for(i=0;i<MAX_IRQ_TYPE;i++){
			if(((irqtable[i].pendreg == 0x26)&&(irq_status_reg1&irqtable[i].reg_mask))
				||((irqtable[i].pendreg == 0x27)&&(irq_status_reg2&irqtable[i].reg_mask) )){
				dev_info(pTAS2560->dev,"chip detected %s \n",irqtable[i].descript);
				irqtable[i].last_status = 1;
			}
		}
		if((pTAS2560->mbPowerUp == true)&&(((power_reg&0x40) == 0)||(power_status_reg!=0xFC))){
			dev_info(pTAS2560->dev,"power status error, reprogram chip\n");
			for(i=0;i<RECOVERY_RETRY_TIME;i++){
				tas2560_reprogram_chip(pTAS2560);
			}
		}else if((!(irq_status_reg1&0xFC))&&dump_regs){
			tas2560_dump_regs(pTAS2560);
		}

#ifdef CONFIG_HUAWEI_DSM
		if((irq_status_reg1&0xFC)||(irq_status_reg2&0xC0)
		||((pTAS2560->mbPowerUp)&&(!(power_reg&0x40)||(power_status_reg!=0xFC)))){
			audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_INT_ERR,
								"tas2560 exception:0x07=0x%x,0x26=0x%x,0x27=0x%x,0x2A=0x%x",
								power_reg,irq_status_reg1,irq_status_reg2,power_status_reg);
		}
#endif

		irq_status_reg1 = 0;
		irq_status_reg2 = 0;
	}

	pTAS2560->write(pTAS2560,TAS2560_INT_CFG_2,0xFF);

}

irqreturn_t tas2560_thread_irq(int irq, void *_data)
{
	struct tas2560_priv *pTAS2560 = (struct tas2560_priv *)_data;
	UNUSED(irq);
	if (!work_busy(&pTAS2560->tas_irq_handle_work)){
		schedule_work(&pTAS2560->tas_irq_handle_work);
	}

	return IRQ_HANDLED;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 common functions for Android Linux");
MODULE_LICENSE("GPLv2");
