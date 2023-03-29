/*
 *	huawei_nfc.h - Huawei NFC driver
 *
 *	Copyright (C) 2017 Huawei Corporation.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _HUAWEI_NFC_H
#define _HUAWEI_NFC_H

#define MAX_GPIO_PLATFROM_SIZE		32
#define CHIP_NAME_STR_MIN_LEN		6
#define CHIP_NAME_STR_LEN			32
#define MAX_ATTRIBUTE_BUFFER_SIZE	128

#define MIN_SE_NUM			1
#define MAX_SE_NUM			3

#define NFCC_DMD_NUMBER_MIN  923002000
#define NFCC_DMD_NUMBER_MAX  923002016

#define NFC_SIM_CARD1		1
#define NFC_SIM_CARD2		2
#define NFC_ESE_CARD		3

#define DEFAULT_NFCC_CHIP	   "pn551"

#define SET_PMU_REG_BIT(reg_val, bit_pos) ((reg_val) |= 1<<(bit_pos))
#define CLR_PMU_REG_BIT(reg_val, bit_pos) ((reg_val) &= ~(1<<(bit_pos)))

#define CLR_BIT		0
#define SET_BIT		1

#define PMIC_OFF	0
#define PMIC_ON 	1

#define TEL_HUAWEI_NV_NFCCAL_NUMBER   372
#define TEL_HUAWEI_NV_NFCCAL_NAME     "NFCCAL"
#define TEL_HUAWEI_NV_NFCCAL_LEGTH    104

struct huawei_nfc_data {
	char nfcc_chip_type[CHIP_NAME_STR_LEN];			// nfcc chip type string pointer
	unsigned int supported_card_num;	// max supported card num
	unsigned int ven_on_gpio_type;		// type of nfcc ven or Pon gpio connected to SOC
	unsigned int nfc_close_type;		// using in atcmdserver, different from shut down to disable nfc
	unsigned int ese_type;				// connected ese type
	unsigned int activited_se_info[MAX_SE_NUM];		// supported technologies by different SE
	unsigned int activited_se_num;		// current activited SE number
	unsigned int atcmdsrv_cmd_result;	// using in atcmdserver, indicate Nfcservice cmd executed process
	unsigned int hal_dmd_number;		// dmd recording number occurred in HAL
	unsigned int enable_status;			// NFC enable process flag
	unsigned int fw_update;				// Firmware updated and nfc opened succ flag
	unsigned int clk_src;				// NFCC CLK source
	unsigned int nfc_switch;            // NFC switch status 1 -- open, 0 -- close
	unsigned int dmd_num;               // last dmd error recorded number
};

typedef struct pmu_reg_control {
	int addr;  /* reg address */
	int pos;   /* bit position */
} t_pmu_reg_control;

/*
 * NFCC VEN GPIO could be controlled by different gpio type
 * NFC_ON_BY_GPIO: normal gpio in AP
 * NFC_ON_BY_HISI_PMIC: a gpio in PMU(HI6421V500 or before), usually notely PMU0_NFC_ON
 * NFC_ON_BY_HI6421V600_PMIC: a gpio in HI6421V600 PMU Platform, usually notely PMU0_NFC_ON
 * NFC_ON_BY_REGULATOR_BULK: a gpio used when pn544 chip which can
 * keep high when system shutdown
 * */
enum NFCC_ON_TYPE {
	NFCC_ON_BY_GPIO = 0,
	NFCC_ON_BY_HISI_PMIC,
	NFCC_ON_BY_HI6421V600_PMIC,
	NFCC_ON_BY_REGULATOR_BULK,
	NFCC_ON_BY_HI6555V110_PMIC,
	NFCC_ON_BY_HI6421V700_PMIC,
	NFCC_ON_BY_MAX,
};

/*
 * !!Warning!!, add new type at last, need to match HAL defination
 * NFCC CLK SRC could be controlled by different PMU platform
 * NFCC_CLK_SRC_CPU: SOC CLK PIN GPIO
 * NFCC_CLK_SRC_PMU_HI6421: chicago pmu platform, Hi6421V530
 * NFCC_CLK_SRC_PMU_HI6555:
 * NFCC_CLK_SRC_PMU_HI6421V600: Boston pmu platform, Hi6421V600
 * */
enum NFCC_CLK_SRC_TYPE {
	NFCC_CLK_SRC_CPU = 0,
	NFCC_CLK_SRC_PMU_HI6421,
	NFCC_CLK_SRC_PMU_HI6555,
	NFCC_CLK_SRC_PMU_HI6421V600,
	NFCC_CLK_SRC_XTAL,
	NFCC_CLK_SRC_PMU_HI6555V200,
};

/*
 * NFCC can wired different eSEs by SWP/DWP
 * default NFCC don't wired eSE.
 * NFC_WITHOUT_ESE: NFCC don't wired eSE
 * NFC_ESE_P61: use NXP p61 as eSE
 * NFC_ESE_HISEE: use hisi se as eSE
 * NFC_ESE_FELICA: use sony cxd22xx as eSE for felica
 */
enum NFCC_ESE_TYPE {
	NFCC_WITHOUT_ESE = 0,
	NFCC_ESE_P61,
	NFCC_ESE_HISEE,
	NFCC_ESE_FELICA
};

/*
 * When SE is activated by NFCC,
 * we should record technologies SE can be supported.
 * NFC_WITHOUT_SE_ACTIVITED: cann't find activated SE
 * NFC_WITH_SE_TYPE_A: the activated SE support RF technology A
 * NFC_WITH_SE_TYPE_B: the activated SE support RF technology B
 * NFC_WITH_SE_TYPE_AB: the activated SE support RF technology A & B
 * */
enum NFCC_ACTIVATED_SE_INFO_E {
	NFC_WITHOUT_SE_ACTIVITED = 0,
	NFCC_WITH_SE_TYPE_A,
	NFCC_WITH_SE_TYPE_B,
	NFCC_WITH_SE_TYPE_AB,
};

int set_hisi_pmic_onoroff(int stat);
int init_nfc_cfg_data_from_dts(struct huawei_nfc_data *p_nfc_data);
int create_sysfs_interfaces(struct device *dev);
int remove_sysfs_interfaces(struct device *dev);
void register_nfc_dmd_client(void);
void init_nfc_info_data(struct huawei_nfc_data *p_nfc_data);

#endif
