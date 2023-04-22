/*
 *	cxd224x-i2c.c - cxd224x NFC i2c driver
 *
 * Copyright (C) 2017- Huawei Corporation.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include "huawei_nfc.h"

#define HWLOG_TAG nfc
HWLOG_REGIST();

#ifdef CONFIG_HUAWEI_DSM
static struct dsm_dev dsm_nfc = {
	.name = "dsm_nfc",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};

static struct dsm_client *nfc_dclient;
#endif

static struct huawei_nfc_data *s_p_nfc_data;

/*lint -e516 -e515 -e717 -e960 -e712 -e747*/
static int nfc_record_dmd_info(unsigned int dmd_no, const char *dmd_info)
{
/*lint -e529 -esym(529,*)*/
#ifdef CONFIG_HUAWEI_DSM
	if (dmd_no < NFCC_DMD_NUMBER_MIN || dmd_no > NFCC_DMD_NUMBER_MAX
		|| dmd_info == NULL || NULL == nfc_dclient) {
		hwlog_info("%s: para error: %d\n", __func__, dmd_no); /*lint !e960*/
		return -1;
	}

	hwlog_info("%s: dmd no: %d - %s", __func__, dmd_no, dmd_info); /*lint !e960*/
	if (!dsm_client_ocuppy(nfc_dclient)) {
		dsm_client_record(nfc_dclient, "DMD info:%s", dmd_info);
		dsm_client_notify(nfc_dclient, dmd_no);
	}
#endif
	return 0;
}
/*lint +e516 +e515 +e717 +e960 +e712 +e747*/
/*lint -e529 +esym(529,*)*/

static int nfc_nv_opera(int opr, int idx, const char *name, int len, void *data)
{
	int ret = -1;
	struct hisi_nve_info_user user_info;

	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = opr;
	user_info.nv_number = idx;
	user_info.valid_size = len;

	strncpy(user_info.nv_name, name, sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';

	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		hwlog_err("%s:nve_direct_access read error(%d)\n", __func__, ret);
		return -1;
	}
	memcpy((char *)data, (char *)user_info.nv_data, len);
	return 0;
}

static int hisi_pmu_reg_operate_by_bit(int reg_addr, int bit_pos, int op)
{
	int ret = 0;
	if (reg_addr <= 0 || bit_pos < 0 || op < 0) {
		hwlog_err("%s: reg_addr[%x], bit_pos[%d], op[%d], error!\n", __func__, reg_addr, bit_pos, op);
		return -1;
	}
	ret = hisi_pmic_reg_read(reg_addr);
	if (op == CLR_BIT) {
		CLR_PMU_REG_BIT(ret, bit_pos);
	} else {
		SET_PMU_REG_BIT(ret, bit_pos);
	}
	hisi_pmic_reg_write(reg_addr, ret);
	hwlog_info("%s: reg: 0x%x, pos: %d, value: 0x%x\n", __func__, reg_addr, bit_pos, ret);
	return 0;
}

static int get_dts_pama_config_string(struct device_node *np, 
				const char *prop_name,
				char *out_string,
				int out_string_len)
{
	char *out_value = NULL;
	int ret = -1;

	ret = of_property_read_string(np, prop_name, (const char **)&out_value);
	if (ret != 0) {
		hwlog_err("%s: can not get prop values with prop_name: %s\n",
						__func__, prop_name);
	} else {
		if (NULL == out_value) {
			hwlog_info("%s: error out_value = NULL\n", __func__);
			ret = -1;
		} else if (strlen(out_value) >= out_string_len) {
			hwlog_info("%s: error out_value len :%d >= out_string_len:%d\n",
				__func__, (int)strlen(out_value), (int)out_string_len);
			ret = -1;
		} else {
			strncpy(out_string, out_value, strlen(out_value));
			hwlog_info("%s: %s = %s\n", __func__, prop_name, out_string);
		}
	}

	return ret;
}

static void get_supported_nfc_card_num(struct device_node *np,
			struct huawei_nfc_data *p_nfc_data)
{
	int ret = -1;
	unsigned int nfc_card_num = 1;

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data is NULL\n", __func__);
		return;
	}
	ret = of_property_read_u32(np, "nfc_card_num", &nfc_card_num);
	if (ret != 0) {
		nfc_card_num = 1;
		hwlog_err("%s: can't get nfc card num config!\n", __func__);
	}
	p_nfc_data->supported_card_num = nfc_card_num;

	hwlog_info("%s: supported_card_num:%d\n", __func__, p_nfc_data->supported_card_num);
	return;
}

static void get_nfcc_on_gpio_type(struct device_node *np, 
			struct huawei_nfc_data *p_nfc_data)
{
	char nfc_on_str[MAX_GPIO_PLATFROM_SIZE] = {0};
	int ret = -1;

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data is NULL\n", __func__);
		return;
	}

	memset(nfc_on_str, 0, MAX_GPIO_PLATFROM_SIZE);
	ret = get_dts_pama_config_string(np, "nfc_on_type",
			nfc_on_str, sizeof(nfc_on_str));

	if (ret != 0) {
		memset(nfc_on_str, 0, MAX_GPIO_PLATFROM_SIZE);
		p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_GPIO;
		hwlog_err("%s: can't find nfc_on_type\n", __func__);
	} else {
		if (!strncasecmp(nfc_on_str, "gpio", strlen("gpio"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_GPIO;
		} else if (!strncasecmp(nfc_on_str, "hisi_pmic", strlen("hisi_pmic"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_HISI_PMIC;
		} else if (!strncasecmp(nfc_on_str, "hi6421v600_pmic", strlen("hi6421v600_pmic"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_HI6421V600_PMIC;
		} else if (!strncasecmp(nfc_on_str, "hi6421v700_pmic", strlen("hi6421v700_pmic"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_HI6421V700_PMIC;
		} else if (!strncasecmp(nfc_on_str, "hi6555v110_pmic", strlen("hi6555v110_pmic"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_HI6555V110_PMIC;
		} else if (!strncasecmp(nfc_on_str, "regulator_bulk", strlen("regulator_bulk"))) {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_REGULATOR_BULK;
		} else {
			p_nfc_data->ven_on_gpio_type = NFCC_ON_BY_GPIO;
		}
	}
	hwlog_info("%s: g_nfc_on_type:%d\n", __func__, p_nfc_data->ven_on_gpio_type);

	return;
}

static void get_nfcc_wired_ese_type(struct device_node *np, 
			struct huawei_nfc_data *p_nfc_data)
{
	char nfc_ese_str[MAX_GPIO_PLATFROM_SIZE] = {0};
	int ret = -1;

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data is NULL\n", __func__);
		return;
	}

	memset(nfc_ese_str, 0, MAX_GPIO_PLATFROM_SIZE);
	ret = get_dts_pama_config_string(np, "nfc_ese_type",
		nfc_ese_str, sizeof(nfc_ese_str));

	if (ret != 0) {
		p_nfc_data->ese_type = NFCC_WITHOUT_ESE;
		hwlog_err("%s: can't find nfc_ese_type node\n", __func__);
		return;
	} else {
		if (!strncasecmp(nfc_ese_str, "hisee", strlen("hisee"))) {
			p_nfc_data->ese_type = NFCC_ESE_HISEE;
		} else if (!strncasecmp(nfc_ese_str, "p61", strlen("p61"))) {
			p_nfc_data->ese_type = NFCC_ESE_P61;
		} else if (!strncasecmp(nfc_ese_str, "felica", strlen("felica"))) {
			p_nfc_data->ese_type = NFCC_ESE_FELICA;
		} else {
			p_nfc_data->ese_type = NFCC_WITHOUT_ESE;
		}
	}
	hwlog_info("%s: g_nfc_ese_type:%d\n", __func__, p_nfc_data->ese_type);
	return;
}

static void get_nfcc_clk_src_type(struct device_node *np,
			struct huawei_nfc_data *p_nfc_data)
{
	int ret = -1;
	char *nfc_clk_status = NULL;
	/* register for different platform number:	  0			   1		  2				3			4			5	   */
	/* register for different platform:			  SOC		HI6421		HI6555		 HI6421		  XTAL		HI6555v200 */
	t_pmu_reg_control nfc_clk_eco_reg[]		= {{0x000, 0}, {0x000, 0}, {0x000, 0}, {0x000, 0}, {0x000, 0}, {0x108, 1}};
	t_pmu_reg_control nfc_clk_xo_reg[]		= {{0x000, 0}, {0x000, 0}, {0x000, 0}, {0x000, 0}, {0x000, 0}, {0x0FD, 0}};
	t_pmu_reg_control nfc_clk_hd_reg[]		= {{0x000, 0}, {0x0C5, 5}, {0x125, 4}, {0x119, 4}, {0x000, 0}, {0x0FD, 4}};
	t_pmu_reg_control nfc_clk_en_reg[]		= {{0x000, 0}, {0x10A, 2}, {0x110, 0}, {0x000, 0}, {0x000, 0}, {0x039, 0}};
	t_pmu_reg_control nfc_clk_dig_reg[]		= {{0x000, 0}, {0x10C, 0}, {0x116, 2}, {0x238, 0}, {0x000, 0}, {0x1C0, 0}};
	t_pmu_reg_control clk_driver_strength[] = {{0x000, 0}, {0x109, 4}, {0x116, 0}, {0x10D, 0}, {0x000, 0}, {0x0F2, 0}};

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data is NULL\n", __func__);
		return;
	}

	ret = of_property_read_string(np, "clk_status", (const char **)&nfc_clk_status);
	if (ret) {
		hwlog_err("[%s,%d]:read clk status fail\n", __func__, __LINE__);
		p_nfc_data->clk_src = NFCC_CLK_SRC_CPU;
	} else if (!strcmp(nfc_clk_status, "xtal")) {
		p_nfc_data->clk_src = NFCC_CLK_SRC_XTAL;
		hwlog_info("%s: config clock using xtal!\n", __func__);
	} else if (!strcmp(nfc_clk_status, "pmu")) {
		p_nfc_data->clk_src = NFCC_CLK_SRC_PMU_HI6421;
		/*use pmu wifibt clk*/
		hwlog_info("%s: config pmu_HI6421 clock in register!\n", __func__);
		/*Register 0x0c5 bit5 = 0 -- not disable wifi_clk_en gpio to control wifi_clk output*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6421].addr,
					nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6421].pos, CLR_BIT);
		/*Register 0x10A bit2 = 0 -- disable internal register to control wifi_clk output*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6421].addr,
					nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6421].pos, CLR_BIT);
		/*Register 0x10C bit0 = 0 -- sine wave(default); 1 --	square wave*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6421].addr,
					nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6421].pos, SET_BIT);
		/*Register 0x109 bit5:bit4 = drive-strength
					  00 --3pF//100K;
					  01 --10pF//100K;
					  10 --16pF//100K;
					  11 --25pF//100K */
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421].pos, SET_BIT);
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421].pos + 1, SET_BIT);
	} else if (!strcmp(nfc_clk_status, "pmu_hi6555")) {
		p_nfc_data->clk_src = NFCC_CLK_SRC_PMU_HI6555;
		/*use pmu wifibt clk*/
		hwlog_info("%s: config pmu_hi6555 clock in register!\n", __func__);
		/*Register 0x125 bit4 = 0 -- not disable wifi_clk_en gpio to control wifi_clk output*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6555].addr,
					nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6555].pos, CLR_BIT);
		/*Register 0x110 bit0 = 0 -- disable internal register to control wifi_clk output*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6555].addr,
					nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6555].pos, CLR_BIT);
		/*Register 0x116 bit2 = sine wave(default):1 --  square wave:0*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6555].addr,
					nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6555].pos, CLR_BIT);
		/*Register 0x116 bit1:bit0 = drive-strength
					00 --3pF//100K;
					01 --10pF//100K;
					10 --16pF//100K;
					11 --30pF//100K */
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555].pos, SET_BIT);
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555].pos + 1, SET_BIT);
		
	} else if (!strcmp(nfc_clk_status, "pmu_hi6421v600")) {
		p_nfc_data->clk_src = NFCC_CLK_SRC_PMU_HI6421V600;

		/*use pmu nfc clk*/
		hwlog_info("%s: config pmu_hi6421v600 clock in register!\n", __func__);
		/*Register 0x119 bit4 = 0 -- not disable nfc_clk_en gpio to control nfc_clk output*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6421V600].addr,
					nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6421V600].pos, CLR_BIT);
		/*Register 0x238 bit0 = sine wave(default):1 --  square wave:0*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6421V600].addr,
					nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6421V600].pos, CLR_BIT);
		/*Register 0x10D bit1:bit0 = drive-strength
					00 --3pF//100K;
					01 --10pF//100K;
					10 --16pF//100K;
					11 --30pF//100K */
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421V600].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421V600].pos, SET_BIT);
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421V600].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6421V600].pos + 1, CLR_BIT);
	} else if (!strcmp(nfc_clk_status, "pmu_hi6555v200")) {
		p_nfc_data->clk_src = NFCC_CLK_SRC_PMU_HI6555V200;

		/*use pmu nfc clk*/
		hwlog_info("%s: config pmu_hi6555v200 clock in register!\n", __func__);
		/* Register 0x039 bit0 = 0 disable */
		hisi_pmu_reg_operate_by_bit(nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					nfc_clk_en_reg[NFCC_CLK_SRC_PMU_HI6555V200].pos, CLR_BIT);
		/*Register 0x0FD bit4 = 1 enable*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					nfc_clk_hd_reg[NFCC_CLK_SRC_PMU_HI6555V200].pos, SET_BIT);
		/*Register 0x0FD bit0 = 1 enable*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_xo_reg[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					nfc_clk_xo_reg[NFCC_CLK_SRC_PMU_HI6555V200].pos, SET_BIT);
		/*Register 0x108 bit1 = 1 enable*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_eco_reg[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					nfc_clk_eco_reg[NFCC_CLK_SRC_PMU_HI6555V200].pos, SET_BIT);
		/*Register 0x1C0 bit0 = sine wave(default):1 --  square wave:0*/
		hisi_pmu_reg_operate_by_bit(nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					nfc_clk_dig_reg[NFCC_CLK_SRC_PMU_HI6555V200].pos, SET_BIT);
		/*Register 0x0F2 bit1:bit0 = drive-strength
					00 --3pF//100K;
					01 --10pF//100K;
					10 --16pF//100K;
					11 --30pF//100K */
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555V200].pos, SET_BIT);
		hisi_pmu_reg_operate_by_bit(clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555V200].addr,
					clk_driver_strength[NFCC_CLK_SRC_PMU_HI6555V200].pos + 1, SET_BIT);
	} else {
		p_nfc_data->clk_src = NFCC_CLK_SRC_XTAL;
		hwlog_info("%s: defualt config clock using xtal!\n", __func__);
	}
	hwlog_info("[%s,%d]:clock source is %d\n", __func__, __LINE__, p_nfc_data->clk_src);

	return;
}
static ssize_t nfcc_chip_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%s\n", DEFAULT_NFCC_CHIP));
	}
	return (ssize_t)(snprintf(buf, strlen(s_p_nfc_data->nfcc_chip_type)+1, "%s",
		s_p_nfc_data->nfcc_chip_type));
}

static ssize_t nfcc_card_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", MIN_SE_NUM));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n",
		(unsigned char)s_p_nfc_data->supported_card_num));
}

static ssize_t nfc_at_result_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}
	if(buf != NULL && buf[0] >= '0')
	{
		s_p_nfc_data->atcmdsrv_cmd_result = buf[0] - '0';
	} else {
		hwlog_err("%s: para buf error\n", __func__);
		return -EINVAL;
	}
	return (ssize_t)count;
}

static ssize_t nfc_at_result_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d\n", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d",
		s_p_nfc_data->atcmdsrv_cmd_result));
}

static ssize_t nfc_fwupdate_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}
	if ((buf !=NULL) && ('1' == buf[0])) {
		s_p_nfc_data->fw_update = 1;
		hwlog_info("%s:firmware update success\n", __func__);
	}

	return (ssize_t)count;
}

static ssize_t nfc_fwupdate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", s_p_nfc_data->fw_update));
}

static ssize_t nfc_wired_ese_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", NFCC_WITHOUT_ESE));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", s_p_nfc_data->ese_type));
}

static ssize_t nfc_activated_se_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", NFC_WITHOUT_SE_ACTIVITED));
	}

	if (s_p_nfc_data->activited_se_num > MAX_SE_NUM
		|| s_p_nfc_data->activited_se_num < MIN_SE_NUM) {
		hwlog_err("%s: para s_p_nfc_data->activited_se_num is %d\n", __func__,
			s_p_nfc_data->activited_se_num);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", NFC_WITHOUT_SE_ACTIVITED));
	}

	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d",
			s_p_nfc_data->activited_se_info[s_p_nfc_data->activited_se_num - 1]));
}

static ssize_t nfc_activated_se_info_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int val = 0;
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}

	if (s_p_nfc_data->activited_se_num > MAX_SE_NUM
		|| s_p_nfc_data->activited_se_num < MIN_SE_NUM) {
		hwlog_err("%s: para s_p_nfc_data->activited_se_num is %d\n", __func__,
			s_p_nfc_data->activited_se_num);
		return -EINVAL;
	}
	if (sscanf(buf, "%1d", &val) == 1) {
		s_p_nfc_data->activited_se_info[s_p_nfc_data->activited_se_num - 1] = val;
	} else {
		hwlog_err("%s: set g_nfc_activated_se_info error\n", __func__);
		s_p_nfc_data->activited_se_info[s_p_nfc_data->activited_se_num - 1] = 0;
	}
	hwlog_info("%s: g_nfc_activated_se_info:%d\n", __func__,
		s_p_nfc_data->activited_se_info[s_p_nfc_data->activited_se_num - 1]);
	return (ssize_t)count;
}

static ssize_t nfc_sim_switch_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int val = 0;
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}
	if (sscanf(buf, "%1d", &val) == 1) {
		if (val >= MIN_SE_NUM && val <= MAX_SE_NUM) {
			s_p_nfc_data->activited_se_num = val;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	hwlog_info("%s: activited_se_num:%d\n", __func__, s_p_nfc_data->activited_se_num);
	return (ssize_t)count;
}

static ssize_t nfc_sim_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", MIN_SE_NUM));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n",
		s_p_nfc_data->activited_se_num));
}

static ssize_t nfc_close_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", s_p_nfc_data->nfc_close_type));
}

static ssize_t nfc_close_type_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int val = 0;
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}

	if (sscanf(buf, "%1d", &val) == 1) {
		s_p_nfc_data->nfc_close_type = val;
	} else {
		hwlog_err("%s: set nfc_close_type error\n", __func__);
		s_p_nfc_data->nfc_close_type = 0;
	}
	hwlog_info("%s: nfc_close_type:%d\n", __func__, s_p_nfc_data->nfc_close_type);
	return (ssize_t)count;
}

static ssize_t nfc_enable_status_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int val = 0;
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}

	if (sscanf(buf, "%1d", &val) == 1) {
		s_p_nfc_data->enable_status = val;
	} else {
		hwlog_err("%s: set enable_status error\n", __func__);
		s_p_nfc_data->enable_status = 0;
	}
	hwlog_info("%s: enable_status:%d\n", __func__, s_p_nfc_data->enable_status);
	return (ssize_t)count;
}
static ssize_t nfc_enable_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", s_p_nfc_data->enable_status));
}

static ssize_t nfcc_clk_src_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d\n", NFCC_CLK_SRC_CPU));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", s_p_nfc_data->clk_src));
}

static ssize_t nfcc_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char cali_info[TEL_HUAWEI_NV_NFCCAL_LEGTH] = {0};
	char cali_str[TEL_HUAWEI_NV_NFCCAL_LEGTH * 2 + 1] = {0};
	int ret = -1, i = 0;
	int cal_len = 0;
	ret = nfc_nv_opera(NV_READ, TEL_HUAWEI_NV_NFCCAL_NUMBER, TEL_HUAWEI_NV_NFCCAL_NAME, TEL_HUAWEI_NV_NFCCAL_LEGTH, cali_info);
	if (ret < 0) {
		hwlog_err("%s: get nv error ret: %d!\n", __func__, ret);
		return -EINVAL;
	}
	cal_len = cali_info[0];
	if (cal_len >= TEL_HUAWEI_NV_NFCCAL_LEGTH){
		cal_len = TEL_HUAWEI_NV_NFCCAL_LEGTH-1;
	}
	for (i = 0; i < cal_len + 1; i ++) {
		snprintf(&cali_str[i*2], 3, "%02X", cali_info[i]);
	}
	hwlog_info("%s: nfc cal info: [%s]!\n", __func__, cali_str);
	return (ssize_t)(snprintf(buf, TEL_HUAWEI_NV_NFCCAL_LEGTH*2, "%s", cali_str));
}

static ssize_t nfc_switch_state_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}
	if(buf != NULL && buf[0] >= '0')
	{
		s_p_nfc_data->nfc_switch = buf[0] - '0';
	} else {
		hwlog_err("%s: para buf error\n", __func__);
		return -EINVAL;
	}
	return (ssize_t)count;
}

static ssize_t nfc_switch_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d\n", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d",
		s_p_nfc_data->nfc_switch));
}

static ssize_t nfc_hal_dmd_info_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned int val = 0, cnt_hal_dmd = 0;
	char dmd_info_from_hal[64] = {'\0'};

	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return -EINVAL;
	}

	/* The length of DMD error number is 9. */
	if (sscanf(buf, "%9d", &val) == 1) {
		if (val < NFCC_DMD_NUMBER_MIN || val > NFCC_DMD_NUMBER_MAX) {
			return (ssize_t)count;
		}
		s_p_nfc_data->dmd_num = val;
		/* The max length of content for current dmd description set as 63.
		   Example for DMD Buf: '923002014 CoreReset:600006A000D1A72000'.
		   A space as a separator is between dmd error no and description.*/
		if (sscanf(buf, "%*s%63s", dmd_info_from_hal) == 1) {
			nfc_record_dmd_info(val, dmd_info_from_hal);
		}

	} else {
		hwlog_err("%s: get dmd number error\n", __func__);
	}
	return (ssize_t)count;
}

static ssize_t nfc_hal_dmd_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (s_p_nfc_data == NULL) {
		hwlog_err("%s: para s_p_nfc_data is NULL\n", __func__);
		return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d\n", 0));
	}
	return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", s_p_nfc_data->dmd_num));
}

static ssize_t nfcc_svdd_sw_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
       int val = 0;

       if (sscanf(buf, "%1d", &val) == 1) {
               if (val == CLR_BIT || val == SET_BIT) {
                       hisi_pmic_reg_write(0x240, val);
               } else {
            hwlog_err("%s: val [%d] error\n", __func__, val);
            return (ssize_t)count;
               }
       } else {
               hwlog_err("%s: val len error\n", __func__);
               return (ssize_t)count;
       }
       hwlog_info("%s: nfc svdd switch to %d\n", __func__, val);
       return (ssize_t)count;
}

static ssize_t nfcc_svdd_sw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
       int sw_status = 0;

       sw_status = hisi_pmic_reg_read(0x240);
       return (ssize_t)(snprintf(buf, MAX_ATTRIBUTE_BUFFER_SIZE-1, "%d", sw_status));
}

static struct device_attribute huawei_nfc_attr[] = {
	__ATTR(nfc_chip_type, 0444, nfcc_chip_type_show, NULL),
	__ATTR(nfc_card_num, 0444, nfcc_card_num_show, NULL),
	__ATTR(nfc_fwupdate, 0664, nfc_fwupdate_show, nfc_fwupdate_store),
	__ATTR(nfc_at_result, 0664, nfc_at_result_show, nfc_at_result_store),
	__ATTR(nfc_sim_switch, 0664, nfc_sim_switch_show, nfc_sim_switch_store),
	__ATTR(nfc_activated_se_info, 0664, nfc_activated_se_info_show, nfc_activated_se_info_store),
	__ATTR(nfc_close_type, 0664, nfc_close_type_show, nfc_close_type_store),
	__ATTR(nfc_enable_status, 0664, nfc_enable_status_show, nfc_enable_status_store),
	__ATTR(nfc_clk_src, 0444, nfcc_clk_src_show, NULL),
	__ATTR(nfc_calibration, 0444, nfcc_calibration_show, NULL),
	__ATTR(nfc_switch_state, 0664, nfc_switch_state_show, nfc_switch_state_store),
	__ATTR(nfc_hal_dmd, 0664, nfc_hal_dmd_info_show, nfc_hal_dmd_info_store),
	__ATTR(nfc_svdd_sw, 0664, nfcc_svdd_sw_show, nfcc_svdd_sw_store),
	__ATTR(nfc_wired_ese_type, 0444, nfc_wired_ese_info_show, NULL),
};


int set_hisi_pmic_onoroff(int stat)
{
	static t_pmu_reg_control nfc_on_hisiXX[NFCC_ON_BY_MAX] =
	{
		{0, 	0},/*NFCC_ON_BY_GPIO*/
		{0x0C3, 4},/*NFCC_ON_BY_HISI_PMIC*/
		{0x240, 0},/*NFCC_ON_BY_HI6421V600_PMIC */
		{0, 	0},/*NFCC_ON_BY_REGULATOR_BULK*/
		{0x158, 0},/*NFCC_ON_BY_HI6555V110_PMIC*/
		{0x2E6, 0},/*NFCC_ON_BY_HI6421V700_PMIC*/
	};
	int ret = 0;
	int nfc_on_type = 0;
	if (s_p_nfc_data == NULL)
	{
		hwlog_err("%s: s_p_nfc_data is null !\n", __func__);
		return -1;
	}
	nfc_on_type = s_p_nfc_data->ven_on_gpio_type;
	if (nfc_on_type >= NFCC_ON_BY_MAX)
	{
		hwlog_err("%s: nfc_on_type = %d is not exist !\n", __func__, nfc_on_type);
		return -1;
	}
	hwlog_info("%s: g_nfc_on_type: %d\n", __func__, nfc_on_type);
	if(stat == PMIC_OFF)
	{
		ret = hisi_pmu_reg_operate_by_bit(nfc_on_hisiXX[nfc_on_type].addr, nfc_on_hisiXX[nfc_on_type].pos, CLR_BIT);
		if(ret != 0)
		{
		  hwlog_err("%s:PMIC_OFF hisi_pmu_reg_operate_by_bit failed %d\n", __func__, ret);
		}
	}
	else if(stat == PMIC_ON)
	{
		ret = hisi_pmu_reg_operate_by_bit(nfc_on_hisiXX[nfc_on_type].addr, nfc_on_hisiXX[nfc_on_type].pos, SET_BIT);
		if(ret != 0)
		{
		  hwlog_err("%s:PMIC_ON hisi_pmu_reg_operate_by_bit failed %d\n", __func__, ret);
		}
	}
	else
	{
	   hwlog_err("%s:stat error, stat = %d\n", __func__, stat);
	}
	return ret;
}
int create_sysfs_interfaces(struct device *dev)
{
	int i= 0;
	for (i = 0; i < ARRAY_SIZE(huawei_nfc_attr); i++) {
		if (device_create_file(dev, huawei_nfc_attr + i)) {
			goto error;
		}
	}
	return 0;
error:
	for ( ; i >= 0; i--) {
		device_remove_file(dev, huawei_nfc_attr + i);
	}
	hwlog_err("%s:pn547 unable to create sysfs interface.\n", __func__);
	return -1;
}

int remove_sysfs_interfaces(struct device *dev)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(huawei_nfc_attr); i++) {
		device_remove_file(dev, huawei_nfc_attr + i);
	}
	return 0;
}

int init_nfc_cfg_data_from_dts(struct huawei_nfc_data *p_nfc_data)
{
	struct device_node *np = NULL;
	const char st[] = "disabled";
	char *nfc_st = NULL;
	char *nfc_cy = NULL;
	int ret = -1;

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data or dev is null\n", __func__);
		return -1;
	}

	for_each_node_with_property(np, "nfc_chip_type") {
		/* NFC chip type */
		ret = of_property_read_string(np, "nfc_chip_type",
					 (const char **)&nfc_cy);
		if (ret) {
			hwlog_err("get nfc nfc_chip_type fail ret=%d\n", ret);
			continue;
		}

		/* check status: ok or disable */
		ret = of_property_read_string(np, "status",
					 (const char **)&nfc_st);
		if (ret) {
			hwlog_err("get nfc status fail ret=%d\n", ret);
			continue;
		}
		ret = strncmp(st, nfc_st, sizeof(st));
		if( !ret){
			hwlog_info("%s : nfc chip: %s is %s \n",__func__, nfc_cy, nfc_st);
			continue;
		}
		/* current chip is ready for probe */
		hwlog_info("%s : nfc chip: %s is %s \n",__func__, nfc_cy, nfc_st);
		/* copy chip type */
		memset(p_nfc_data->nfcc_chip_type, 0, CHIP_NAME_STR_LEN);
		strncpy(p_nfc_data->nfcc_chip_type, nfc_cy, strlen(nfc_cy));

		get_supported_nfc_card_num(np, p_nfc_data);
		get_nfcc_on_gpio_type(np, p_nfc_data);
		get_nfcc_wired_ese_type(np, p_nfc_data);
		get_nfcc_clk_src_type(np, p_nfc_data);
	}
	return ret;
}

void init_nfc_info_data(struct huawei_nfc_data *p_nfc_data) {

	if (p_nfc_data == NULL) {
		hwlog_err("%s: para p_nfc_data or dev is null\n", __func__);
		return;
	}
	memset(p_nfc_data, 0, sizeof(struct huawei_nfc_data));

	p_nfc_data->activited_se_num = NFC_SIM_CARD1;

	s_p_nfc_data = p_nfc_data;
}

void register_nfc_dmd_client(void) {
#ifdef CONFIG_HUAWEI_DSM
	if (!nfc_dclient) {
		nfc_dclient = dsm_register_client(&dsm_nfc);
	}
#else
	hwlog_info("%s : CONFIG_HUAWEI_DSM is not enabled \n",__func__);
#endif
}

