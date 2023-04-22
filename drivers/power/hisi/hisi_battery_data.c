/*************************************************************
* Filename:    hisi_battery_data.c
*
* Discription:  driver for battery.
* Copyright:    (C) 2014 huawei.
*
* revision history:
*
*
**************************************************************/
#include <linux/power/hisi/hisi_battery_data.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/string.h>
#include <linux/slab.h>
/*lint -e451*/
#include <asm/bug.h>
/*lint +e451*/
#include <linux/module.h>

#include <huawei_platform/power/power_dsm.h>
#ifdef CONFIG_HUAWEI_BATTERY_INFORMATION
#include <huawei_platform/power/batt_info.h>
#endif


/* #define HISI_BATTERY_DATA_DEBUG */
#include "securec.h"

#define BATTERY_DATA_INFO
#ifndef BATTERY_DATA_INFO
#define bat_data_debug(fmt, args...)do {} while (0)
#define bat_data_info(fmt, args...) do {} while (0)
#define bat_data_warn(fmt, args...) do {} while (0)
#define bat_data_err(fmt, args...)  do {} while (0)
#else
#define bat_data_debug(fmt, args...)do { printk(KERN_DEBUG   "[hisi_battery_data]" fmt, ## args); } while (0)
#define bat_data_info(fmt, args...) do { printk(KERN_INFO    "[hisi_battery_data]" fmt, ## args); } while (0)
#define bat_data_warn(fmt, args...) do { printk(KERN_WARNING"[hisi_battery_data]" fmt, ## args); } while (0)
#define bat_data_err(fmt, args...)  do { printk(KERN_ERR   "[hisi_battery_data]" fmt, ## args); } while (0)
#endif

#ifdef HISI_BATTERY_DATA_DEBUG
#define hisi_bat_info(fmt, args...) do { bat_data_info(fmt, ## args); } while (0)
#else
#define hisi_bat_info(fmt, args...) do {} while (0)
#endif

/***************************static variable definition***********************************/
static struct hisi_coul_battery_data **p_data = NULL;
static unsigned int hisi_bat_data_size = 0;	/* used to store number of bat types defined in DTS */
/* used to judege whether bat_drv works fine or not, 1 means yes,0 means no */
static int bat_param_status = 0;
static int temp_points[] = { -20, -10, 0, 25, 40, 60 };

static int get_battery_data_by_id_volt(unsigned int id_index, unsigned int id_voltage)
{
	if (id_index >= hisi_bat_data_size)
		return -EINVAL;

	if ((id_voltage > p_data[id_index]->id_identify_min) && (id_voltage <= p_data[id_index]->id_identify_max)) {
		if ((id_voltage < p_data[id_index]->id_voltage_min) || (id_voltage > p_data[id_index]->id_voltage_max)) {
			power_dsm_dmd_report_format(POWER_DSM_BATTERY_DETECT, DSM_BATTERY_DETECT_ERROR_NO, \
				"Battery id voltage:%d is out of normal range:[%d~%d],identify range:[%d~%d]!\n", id_voltage,
				p_data[id_index]->id_voltage_min, p_data[id_index]->id_voltage_max,
				p_data[id_index]->id_identify_min, p_data[id_index]->id_identify_max);
		}
		return 0;
	}

	return -EINVAL;
}
static int get_battery_data_by_id_sn(unsigned int id_index)
{
#ifdef CONFIG_HUAWEI_BATTERY_INFORMATION
	int ret;
	unsigned char id_sn[ID_SN_SIZE] = {0};

	if (id_index >= hisi_bat_data_size)
		return -EINVAL;

	ret = get_battery_type(id_sn);
	if (ret == BATTERY_DRIVER_FAIL) {
		bat_data_err("get id_sn from ic fail!\n");
		return -EINVAL;
	}
	bat_data_info("id_sn from ic is %s\n", id_sn);
	if (!strncmp(p_data[id_index]->id_sn, id_sn, strlen(id_sn))) {
		return 0;
	} else {
		return -EINVAL;
	}
#else
	bat_data_err("has no CONFIG_HUAWEI_BATTERY_INFORMATION\n");
	return -EINVAL;
#endif
}
struct hisi_coul_battery_data *get_battery_data(unsigned int id_voltage)
{
	unsigned int i;
	int ret;

	if (!bat_param_status) {
		bat_data_err("battery param is invalid\n");
		return NULL;
	}

	for (i = 0; i < hisi_bat_data_size; i++) {
		if (!strncmp(p_data[i]->identify_type, BATT_IDENTIFY_BY_VOLT, strlen(BATT_IDENTIFY_BY_VOLT))) {
			ret = get_battery_data_by_id_volt(i, id_voltage);
			if (!ret)
				break;
		} else if (!strncmp(p_data[i]->identify_type, BATT_IDENTIFY_BY_SN, strlen(BATT_IDENTIFY_BY_SN))) {
			ret = get_battery_data_by_id_sn(i);
			if (!ret)
				break;
		} else {
			bat_data_err("batt_identify_type error\n");
		}
	}
	if (i == hisi_bat_data_size) {
		i = 0;
		if (!strstr(saved_command_line, "androidboot.swtype=factory")) {
			power_dsm_dmd_report_format(POWER_DSM_BATTERY_DETECT, DSM_BATTERY_DETECT_ERROR_NO, \
				"Battery id is invalid. Use the default battery params!\n");
		}
	}
	bat_data_info("current battery name is %s\n", p_data[i]->batt_brand);

	return p_data[i];
}
static int get_age_para (struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int ret = 0;
	int i = 0, j = 0;
	if(NULL == np || NULL == pdat || NULL ==  pdat->pc_temp_ocv_lut1 || NULL == pdat->pc_temp_ocv_lut2
	 || NULL == pdat->pc_temp_ocv_lut3 || NULL == pdat->pc_temp_ocv_lut0)
	{
		return -1;
	}
	pdat->pc_temp_ocv_lut1->rows = pdat->pc_temp_ocv_lut0->rows;
	pdat->pc_temp_ocv_lut2->rows = pdat->pc_temp_ocv_lut0->rows;
	pdat->pc_temp_ocv_lut3->rows = pdat->pc_temp_ocv_lut0->rows;

	pdat->pc_temp_ocv_lut1->cols = pdat->pc_temp_ocv_lut0->cols;
	pdat->pc_temp_ocv_lut2->cols = pdat->pc_temp_ocv_lut0->cols;
	pdat->pc_temp_ocv_lut3->cols = pdat->pc_temp_ocv_lut0->cols;

	for (i = 0; i < pdat->pc_temp_ocv_lut0->cols; i++) {
		pdat->pc_temp_ocv_lut1->temp[i] = temp_points[i];
		pdat->pc_temp_ocv_lut2->temp[i] = temp_points[i];
		pdat->pc_temp_ocv_lut3->temp[i] = temp_points[i];
	}

	for (i = 0; i < pdat->pc_temp_ocv_lut0->rows - 1; i++) {
		pdat->pc_temp_ocv_lut1->percent[i] = pdat->pc_temp_ocv_lut0->percent[i];
		pdat->pc_temp_ocv_lut2->percent[i] = pdat->pc_temp_ocv_lut0->percent[i];
		pdat->pc_temp_ocv_lut3->percent[i] = pdat->pc_temp_ocv_lut0->percent[i];
		hisi_bat_info("pc_temp_ocv_percent[%d] is %d\n", i, pdat->pc_temp_ocv_lut0->percent[i]);
	}

	ret = of_property_read_u32(np, "vol_dec1", (unsigned int *)(&(pdat->vol_dec1)));
	if (ret) {
		bat_data_err("there is no vol_dec1 config!\n");
		pdat->vol_dec1 = 0;
	}
	ret = of_property_read_u32(np, "vol_dec2", (unsigned int *)(&(pdat->vol_dec2)));
	if (ret) {
		bat_data_err("there is no vol_dec2 config!\n");
		pdat->vol_dec2 = 0;
	}
	ret = of_property_read_u32(np, "vol_dec3", (unsigned int *)(&(pdat->vol_dec3)));
	if (ret) {
		bat_data_err("there is no vol_dec3 config!\n");
		pdat->vol_dec3 = 0;
	}

	for (i = 0; i < pdat->pc_temp_ocv_lut0->cols; i++) {	/* 6 */
		for (j = 0; j < pdat->pc_temp_ocv_lut0->rows; j++) {	/* 29 */
			if(pdat->vol_dec1 > 0)
			{
				ret = of_property_read_u32_index(np, "pc_temp_ocv_ocv1", j * TEMP_SAMPLING_POINTS + i, (unsigned int *)(&(pdat->pc_temp_ocv_lut1->ocv[j][i])));
				if (ret) {
					bat_data_err("get pc_temp_ocv_ocv1[%d] failed\n", j * TEMP_SAMPLING_POINTS + i);
				}
			}
			if(pdat->vol_dec2 > 0)
			{
				ret = of_property_read_u32_index(np, "pc_temp_ocv_ocv2", j * TEMP_SAMPLING_POINTS + i, (unsigned int *)(&(pdat->pc_temp_ocv_lut2->ocv[j][i])));
				if (ret) {
					bat_data_err("get pc_temp_ocv_ocv2[%d] failed\n", j * TEMP_SAMPLING_POINTS + i);
				}
			}
			if(pdat->vol_dec3 > 0)
			{
				ret = of_property_read_u32_index(np, "pc_temp_ocv_ocv3", j * TEMP_SAMPLING_POINTS + i, (unsigned int *)(&(pdat->pc_temp_ocv_lut3->ocv[j][i])));
				if (ret) {
					bat_data_err("get pc_temp_ocv_ocv3[%d] failed\n", j * TEMP_SAMPLING_POINTS + i);
				}
			}
		}
	}
	return 0;
}

static int get_fcc_sf_dts(struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int i, ret;

	ret = of_property_read_u32(np, "fcc_sf_cols", (unsigned int *)(&(pdat->fcc_sf_lut->cols)));
	if (ret) {
		bat_data_err("get fcc_sf_cols failed\n");
		return -EINVAL;
	}
	hisi_bat_info("fcc_sf_cols is %d\n", pdat->fcc_sf_lut->cols);
	for (i = 0; i < pdat->fcc_sf_lut->cols; i++) {
		ret = of_property_read_u32_index(np, "fcc_sf_x", i, (unsigned int *)(&(pdat->fcc_sf_lut->x[i])));
		if (ret) {
			bat_data_err("get fcc_sf_x[%d] failed\n", i);
			return -EINVAL;
		}
		hisi_bat_info("fcc_sf_x[%d] is %d\n", i, pdat->fcc_sf_lut->x[i]);
		ret = of_property_read_u32_index(np, "fcc_sf_y", i, (unsigned int *)(&(pdat->fcc_sf_lut->y[i])));
		if (ret) {
			bat_data_err("get fcc_sf_y[%d] failed\n", i);
			return -EINVAL;
		}
		hisi_bat_info("fcc_sf_y[%d] is %d\n", i, pdat->fcc_sf_lut->y[i]);
	}
	/* pc_sf_lut */
	ret = of_property_read_u32(np, "pc_sf_rows", (unsigned int *)(&(pdat->pc_sf_lut->rows)));
	if (ret) {
		bat_data_err("get pc_sf_rows failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_sf_rows is %d\n", pdat->pc_sf_lut->rows);
	ret = of_property_read_u32(np, "pc_sf_cols", (unsigned int *)(&(pdat->pc_sf_lut->cols)));
	if (ret) {
		bat_data_err("get pc_sf_cols failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_sf_cols is %d\n", pdat->pc_sf_lut->cols);
	ret = of_property_read_u32(np, "pc_sf_row_entries", (unsigned int *)(&(pdat->pc_sf_lut->row_entries[0])));
	if (ret) {
		bat_data_err("get pc_sf_row_entries failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_sf_row_entries is %d\n", pdat->pc_sf_lut->row_entries[0]);
	ret = of_property_read_u32(np, "pc_sf_percent", (unsigned int *)(&(pdat->pc_sf_lut->percent[0])));
	if (ret) {
		bat_data_err("get pc_sf_percent failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_sf_percent is %d\n", pdat->pc_sf_lut->percent[0]);
	ret = of_property_read_u32(np, "pc_sf_sf", (unsigned int *)(&(pdat->pc_sf_lut->sf[0][0])));
	if (ret) {
		bat_data_err("get pc_sf_sf failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_sf_sf is %d\n", pdat->pc_sf_lut->sf[0][0]);
	return 0;
}

static int get_id_volt_dat(struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int ret;
	ret = of_property_read_u32(np, "id_identify_min", (unsigned int *)(&(pdat->id_identify_min)));
	if (ret) {
		bat_data_err("get id_identify_min failed\n");
		return -EINVAL;
	}
	hisi_bat_info("id_identify_min is %d\n", pdat->id_identify_min);

	ret = of_property_read_u32(np, "id_identify_max", (unsigned int *)(&(pdat->id_identify_max)));
	if (ret) {
		bat_data_err("get id_identify_max failed\n");
		return -EINVAL;
	}
	hisi_bat_info("id_identify_max is %d\n", pdat->id_identify_max);

	ret = of_property_read_u32(np, "id_voltage_min", (unsigned int *)(&(pdat->id_voltage_min)));
	if (ret) {
		bat_data_err("get id_voltage_min failed\n");
		return -EINVAL;
	}
	hisi_bat_info("id_voltage_min is %d\n", pdat->id_voltage_min);

	ret = of_property_read_u32(np, "id_voltage_max", (unsigned int *)(&(pdat->id_voltage_max)));
	if (ret) {
		bat_data_err("get id_voltage_max failed\n");
		return -EINVAL;
	}
	hisi_bat_info("id_voltage_max is %d\n", pdat->id_voltage_max);
	return 0;
}

static int get_id_sn_dat(struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int ret;
	ret = of_property_read_string(np, "id_sn", (const char **)(&pdat->id_sn));
	if (ret) {
		bat_data_err("get id_sn failed\n");
		return -EINVAL;
	}
	hisi_bat_info("id_sn is %s\n", pdat->id_sn);
	return 0;
}
static int get_dat_decress_para(struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int ret = 0;
	if((NULL == np)||(NULL == pdat))
		return -EINVAL;
	if(strstr(saved_command_line, "batt_decress_flag=true")){
		ret = of_property_read_u32(np, "fcc_decress", &(pdat->fcc));
	if (ret) {
		bat_data_err("get fcc decress failed\n");
		return -EINVAL;
		}
		ret = of_property_read_u32(np, "vbat_max_decress", (unsigned int *)(&(pdat->vbatt_max)));
	if (ret) {
		bat_data_err("get vbat_max decress failed\n");
		return -EINVAL;
		}
	}
	return 0;
}
static int get_decress_fcc_temp(struct device_node *np, struct hisi_coul_battery_data *pdat,int index)
{
	int ret = 0;
	if((NULL == np)||(NULL == pdat))
		return -EINVAL;
	if(strstr(saved_command_line, "batt_decress_flag=true")){
		ret = of_property_read_u32_index(np, "fcc_temp_decress", index, (unsigned int *)(&(pdat->fcc_temp_lut->y[index])));
	if (ret) {
		bat_data_err("get fcc_temp decress[%d] failed\n", index);
		return -EINVAL;
		}
	}
	return 0;
}
static int get_decress_ocv_tbl(struct device_node *np, struct hisi_coul_battery_data *pdat,int row,int col)
{
	int ret = 0;
	if((NULL == np)||(NULL == pdat))
		return -EINVAL;
	if(strstr(saved_command_line, "batt_decress_flag=true")){//battery need decreas vol
		ret = of_property_read_u32_index(np, "pc_temp_ocv_ocv0", row * 6 + col, (unsigned int *)(&(pdat->pc_temp_ocv_lut0->ocv[row][col])));
	if (ret) {
		bat_data_err("get pc_temp_ocv_ocv drcress[%d] failed\n", row * 6 + col);
		return -EINVAL;
		}
	}
	return 0;
}
static int get_decress_segment_para(struct device_node *np, int index,const char **data_string)
{
	int ret = 0;
	if((NULL == np)||(NULL == data_string))
		return -EINVAL;
	if(strstr(saved_command_line, "batt_decress_flag=true")){
		ret = of_property_read_string_index(np, "segment_para_decress", index, data_string);
	if (ret) {
		bat_data_err("get segment_para_decress failed\n");
		return -EINVAL;
		}
	}
	return 0;
}
static int get_dat(struct device_node *np, struct hisi_coul_battery_data *pdat)
{
	int ret = 0;
	int i, j;
	const char *chrg_data_string = NULL;
	int array_len = 0;

	ret = of_property_read_u32(np, "fcc", &(pdat->fcc));
	if (ret) {
		bat_data_err("get fcc failed\n");
		return -EINVAL;
	}
	hisi_bat_info("fcc is %u\n", pdat->fcc);

	ret = of_property_read_string(np, "identify_type", (const char **)(&pdat->identify_type));
	if (ret) {
	    bat_data_err("get batt_identify_type failed!\n");
		return -EINVAL;
	}
	hisi_bat_info("identify_type is %s\n", pdat->identify_type);
	if (!strncmp(pdat->identify_type, BATT_IDENTIFY_BY_VOLT, strlen(BATT_IDENTIFY_BY_VOLT))) {
		ret = get_id_volt_dat(np, pdat);
	} else if (!strncmp(pdat->identify_type, BATT_IDENTIFY_BY_SN, strlen(BATT_IDENTIFY_BY_SN))) {
		ret = get_id_sn_dat(np, pdat);
	} else {
		bat_data_err("batt_identify_type error\n");
		return -EINVAL;
	}
	if (ret) {
		bat_data_err("get batt_identify_para failed!\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "default_rbatt_mohm", (unsigned int *)(&(pdat->default_rbatt_mohm)));
	if (ret) {
		bat_data_err("get default_rbatt_mohm failed\n");
		return -EINVAL;
	}
	hisi_bat_info("default_rbatt_mohm is %d\n", pdat->default_rbatt_mohm);
	/* vbat_max */
	ret = of_property_read_u32(np, "vbat_max", (unsigned int *)(&(pdat->vbatt_max)));
	if (ret) {
		bat_data_err("get vbat_max failed\n");
		return -EINVAL;
	}
	hisi_bat_info("vbat_max is %d\n", pdat->vbatt_max);
	/* ifull*/
	ret = of_property_read_u32(np, "ifull", (unsigned int *)(&(pdat->ifull)));
	if (ret) {
		pdat->ifull = DEFAULT_IFULL_SET;
		bat_data_err("get ifull failed,using DEFAULT_IFULL_SET\n");
	}
	hisi_bat_info("ifull is %d\n", pdat->ifull);
	/* temp_para */
	array_len = of_property_count_strings(np, "temp_para");
	if ((array_len <= 0) || (array_len % TEMP_PARA_TOTAL != 0)
	    || (array_len > TEMP_PARA_LEVEL * TEMP_PARA_TOTAL)) {
		bat_data_err("temp_para is invaild,please check temp_para number!!\n");
		return -EINVAL;
	}
	pdat->chrg_para->temp_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "temp_para", i, &chrg_data_string);
		if (ret) {
			bat_data_err("get temp_para failed\n");
			return -EINVAL;
		}
		pdat->chrg_para->temp_data[i / (TEMP_PARA_TOTAL)][i % (TEMP_PARA_TOTAL)] = simple_strtol(chrg_data_string, NULL, 10);
		hisi_bat_info("chrg_para->temp_data[%d][%d] = %d\n",
			i / (TEMP_PARA_TOTAL),
			i % (TEMP_PARA_TOTAL),
			pdat->chrg_para->temp_data[i / (TEMP_PARA_TOTAL)][i % (TEMP_PARA_TOTAL)]);
	}
	/* vbat_para */
	array_len = of_property_count_strings(np, "vbat_para");
	if ((array_len <= 0) || (array_len % VOLT_PARA_TOTAL != 0)
	    || (array_len > VOLT_PARA_LEVEL * VOLT_PARA_TOTAL)) {
		bat_data_err("vbat_para is invaild,please check vbat_para number!!\n");
		return -EINVAL;
	}
	pdat->chrg_para->volt_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "vbat_para", i, &chrg_data_string);
		if (ret) {
			bat_data_err("get vbat_para failed\n");
			return -EINVAL;
		}
		pdat->chrg_para->volt_data[i / (VOLT_PARA_TOTAL)][i % (VOLT_PARA_TOTAL)] = simple_strtol(chrg_data_string, NULL, 10);
		hisi_bat_info("chrg_para->volt_data[%d][%d] = %d\n",
			i / (VOLT_PARA_TOTAL),
			i % (VOLT_PARA_TOTAL),
			pdat->chrg_para->volt_data[i / (VOLT_PARA_TOTAL)][i % (VOLT_PARA_TOTAL)]);
	}

	/* segment_para */
	array_len = of_property_count_strings(np, "segment_para");
	if ((array_len <= 0) || (array_len % SEGMENT_PARA_TOTAL != 0)
	    || (array_len > SEGMENT_PARA_LEVEL * SEGMENT_PARA_TOTAL)) {
		bat_data_err("segment_para is invaild,please check segment_para number!!\n");
		return -EINVAL;
	}
	pdat->chrg_para->segment_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "segment_para", i, &chrg_data_string);
		if (ret) {
			bat_data_err("get segment_para failed\n");
			return -EINVAL;
		}
		ret = get_decress_segment_para(np,i,&chrg_data_string);
		bat_data_info("get_decress_segment_para res %d",ret);
		pdat->chrg_para->segment_data[i / (SEGMENT_PARA_TOTAL)][i % (SEGMENT_PARA_TOTAL)] = simple_strtol(chrg_data_string, NULL, 10);
		hisi_bat_info("chrg_para->segment_data[%d][%d] = %d\n",
			i / (SEGMENT_PARA_TOTAL),
			i % (SEGMENT_PARA_TOTAL),
			pdat->chrg_para->segment_data[i / (SEGMENT_PARA_TOTAL)][i % (SEGMENT_PARA_TOTAL)]);
	}

	/* batt_brand */
	ret = of_property_read_string(np, "batt_brand", (const char **)(&(pdat->batt_brand)));
	if (ret) {
		bat_data_err("get batt_brand failed\n");
		return -EINVAL;
	}
	hisi_bat_info("batt_brand is %s\n", pdat->batt_brand);
	/* fcc_temp */
	pdat->fcc_temp_lut->cols = TEMP_SAMPLING_POINTS;
	for (i = 0; i < pdat->fcc_temp_lut->cols; i++) {
		pdat->fcc_temp_lut->x[i] = temp_points[i];
		ret = of_property_read_u32_index(np, "fcc_temp", i, (unsigned int *)(&(pdat->fcc_temp_lut->y[i])));
		if (ret) {
			bat_data_err("get fcc_temp[%d] failed\n", i);
			return -EINVAL;
		}
		ret = get_decress_fcc_temp(np,pdat,i);
		hisi_bat_info("get_decress_fcc_temp res %d",ret);
		hisi_bat_info("fcc_temp[%d] is %d\n", i, pdat->fcc_temp_lut->y[i]);
	}
	/*decress para*/
	ret = get_dat_decress_para(np, pdat);
	bat_data_info("get decress para from dts res %d\n",ret);
	/* fcc_sf */
	ret = get_fcc_sf_dts(np, pdat);
	if (ret) {
		bat_data_err("get_fcc_sf_dts failed\n");
		return -EINVAL;
	}

	/* rbat_sf */
	ret = of_property_read_u32(np, "rbatt_sf_rows", (unsigned int *)(&(pdat->rbatt_sf_lut->rows)));
	if (ret) {
		bat_data_err("get rbatt_sf_rows failed\n");
		return -EINVAL;
	}
	hisi_bat_info("rbatt_sf_rows is %d\n", pdat->rbatt_sf_lut->rows);
	ret = of_property_read_u32(np, "rbatt_sf_cols", (unsigned int *)(&(pdat->rbatt_sf_lut->cols)));
	if (ret) {
		bat_data_err("get rbatt_sf_cols failed\n");
		return -EINVAL;
	}
	hisi_bat_info("rbatt_sf_cols is %d\n", pdat->rbatt_sf_lut->cols);
	for (i = 0; i < pdat->rbatt_sf_lut->rows; i++) {
		ret = of_property_read_u32_index(np, "rbatt_sf_percent", i, (unsigned int *)(&(pdat->rbatt_sf_lut->percent[i])));
		if (ret) {
			bat_data_err("get rbatt_sf_percent[%d] failed\n", i);
			return -EINVAL;
		}
		hisi_bat_info("rbatt_sf_percent[%d] is %d\n", i, pdat->rbatt_sf_lut->percent[i]);
	}
	for (i = 0; i < pdat->rbatt_sf_lut->cols; i++) {
		pdat->rbatt_sf_lut->row_entries[i] = temp_points[i];
	}
	for (i = 0; i < pdat->rbatt_sf_lut->cols; i++) {	/* 6 */
		for (j = 0; j < pdat->rbatt_sf_lut->rows; j++) {    /* 28 */
			ret = of_property_read_u32_index(np, "rbatt_sf_sf", j * 6 + i, (unsigned int *)(&(pdat->rbatt_sf_lut->sf[j][i])));
			if (ret) {
				bat_data_err("get rbatt_sf_sf[%d] failed\n", j * 6 + i);
				return -EINVAL;
			}
			hisi_bat_info("rbatt_sf_sf[%d] is %d\n", j * 6 + i, pdat->rbatt_sf_lut->sf[j][i]);
		}
	}
	/* pc_temp_ocv */
	ret = of_property_read_u32(np, "pc_temp_ocv_rows", (unsigned int *)(&(pdat->pc_temp_ocv_lut0->rows)));
	if (ret) {
		bat_data_err("get pc_temp_ocv_rows failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_temp_ocv_rows is %d\n", pdat->pc_temp_ocv_lut->rows);
	ret = of_property_read_u32(np, "pc_temp_ocv_cols", (unsigned int *)(&(pdat->pc_temp_ocv_lut0->cols)));
	if (ret) {
		bat_data_err("get pc_temp_ocv_cols failed\n");
		return -EINVAL;
	}
	hisi_bat_info("pc_temp_ocv_cols is %d\n", pdat->pc_temp_ocv_lut0->cols);
	for (i = 0; i < pdat->pc_temp_ocv_lut0->rows - 1; i++) {
		ret = of_property_read_u32_index(np, "pc_temp_ocv_percent", i, (unsigned int *)(&(pdat->pc_temp_ocv_lut0->percent[i])));
		if (ret) {
			bat_data_err("get pc_temp_ocv_percent[%d] failed\n", i);
			return -EINVAL;
		}
		hisi_bat_info("pc_temp_ocv_percent[%d] is %d\n", i, pdat->pc_temp_ocv_lut0->percent[i]);
	}
	for (i = 0; i < pdat->pc_temp_ocv_lut0->cols; i++) {
		pdat->pc_temp_ocv_lut0->temp[i] = temp_points[i];
	}
	for (i = 0; i < pdat->pc_temp_ocv_lut0->cols; i++) {	/* 6 */
		for (j = 0; j < pdat->pc_temp_ocv_lut0->rows; j++) {	/* 29 */
			ret = of_property_read_u32_index(np, "pc_temp_ocv_ocv", j * 6 + i, (unsigned int *)(&(pdat->pc_temp_ocv_lut0->ocv[j][i])));
			if (ret) {
				bat_data_err("get pc_temp_ocv_ocv[%d] failed\n", j * 6 + i);
				return -EINVAL;
			}
			ret = get_decress_ocv_tbl(np,pdat,j,i);
			hisi_bat_info("get_decress_ocv_tbl res %d",ret);
			hisi_bat_info("rbatt_sf_sf[%d] is %d\n", j * 6 + i, pdat->pc_temp_ocv_lut0->ocv[j][i]);
		}
	}
	get_age_para (np,pdat);

	return ret;
}
static void get_mem_extra (struct hisi_coul_battery_data *pdat)
{
	if(NULL == pdat)
	{
		return;
	}
	pdat->pc_temp_ocv_lut1 = (struct pc_temp_ocv_lut *)kzalloc(sizeof(struct pc_temp_ocv_lut), GFP_KERNEL);
	if (NULL == pdat->pc_temp_ocv_lut1) {
		bat_data_err("alloc pdat->pc_temp_ocv_lut failed\n");
		return;
	}
	pdat->pc_temp_ocv_lut2 = (struct pc_temp_ocv_lut *)kzalloc(sizeof(struct pc_temp_ocv_lut), GFP_KERNEL);
	if (NULL == pdat->pc_temp_ocv_lut2) {
		bat_data_err("alloc pdat->pc_temp_ocv_lut failed\n");
		kfree(pdat->pc_temp_ocv_lut1);
		pdat->pc_temp_ocv_lut1 = NULL;
		return;
	}
	pdat->pc_temp_ocv_lut3 = (struct pc_temp_ocv_lut *)kzalloc(sizeof(struct pc_temp_ocv_lut), GFP_KERNEL);
	if (NULL == pdat->pc_temp_ocv_lut3) {
		bat_data_err("alloc pdat->pc_temp_ocv_lut failed\n");
		kfree(pdat->pc_temp_ocv_lut1);
		kfree(pdat->pc_temp_ocv_lut2);
		pdat->pc_temp_ocv_lut1 = NULL;
		pdat->pc_temp_ocv_lut2 = NULL;
		return;
	}
}
static int get_mem(struct hisi_coul_battery_data **p)
{
	struct hisi_coul_battery_data *pdat;

	pdat = (struct hisi_coul_battery_data *)kzalloc(sizeof(struct hisi_coul_battery_data), GFP_KERNEL);
	if (NULL == pdat) {
		bat_data_err("alloc pdat failed\n");
		return -ENOMEM;
	}

	pdat->fcc_temp_lut = (struct single_row_lut *)kzalloc(sizeof(struct single_row_lut), GFP_KERNEL);
	if (NULL == pdat->fcc_temp_lut) {
		bat_data_err("alloc pdat->fcc_temp_lut failed\n");
		goto get_mem_fail_0;
	}

	pdat->fcc_sf_lut = (struct single_row_lut *)kzalloc(sizeof(struct single_row_lut), GFP_KERNEL);
	if (NULL == pdat->fcc_sf_lut) {
		bat_data_err("alloc pdat->fcc_sf_lut failed\n");
		goto get_mem_fail_1;
	}

	pdat->pc_temp_ocv_lut0 = (struct pc_temp_ocv_lut *)kzalloc(sizeof(struct pc_temp_ocv_lut), GFP_KERNEL);
	if (NULL == pdat->pc_temp_ocv_lut0) {
		bat_data_err("alloc pdat->pc_temp_ocv_lut failed\n");
		goto get_mem_fail_2;
	}
	pdat->pc_temp_ocv_lut = pdat->pc_temp_ocv_lut0;
	pdat->pc_sf_lut = (struct sf_lut *)kzalloc(sizeof(struct sf_lut), GFP_KERNEL);
	if (NULL == pdat->pc_sf_lut) {
		bat_data_err("alloc pdat->pc_sf_lut failed\n");
		goto get_mem_fail_3;
	}

	pdat->rbatt_sf_lut = (struct sf_lut *)kzalloc(sizeof(struct sf_lut), GFP_KERNEL);
	if (NULL == pdat->rbatt_sf_lut) {
		bat_data_err("alloc pdat->rbatt_sf_lut failed\n");
		goto get_mem_fail_4;
	}

	pdat->chrg_para = (struct chrg_para_lut *)kzalloc(sizeof(struct chrg_para_lut), GFP_KERNEL);
	if (NULL == pdat->chrg_para) {
		bat_data_err("alloc pdat->chrg_para failed\n");
		goto get_mem_fail_5;
	}

	get_mem_extra(pdat);
	*p = pdat;
	return 0;
get_mem_fail_5:
	kfree(pdat->rbatt_sf_lut);
get_mem_fail_4:
	kfree(pdat->pc_sf_lut);
get_mem_fail_3:
	pdat->pc_temp_ocv_lut = NULL;
	kfree(pdat->pc_temp_ocv_lut0);
get_mem_fail_2:
	kfree(pdat->fcc_sf_lut);
get_mem_fail_1:
	kfree(pdat->fcc_temp_lut);
get_mem_fail_0:
	kfree(pdat);
	return -ENOMEM;
}

static void free_mem(struct hisi_coul_battery_data **p)
{
	struct hisi_coul_battery_data *pdat = *p;

	if (NULL == pdat) {
		bat_data_err("pointer pd is already NULL\n");
		return;
	}
	if (NULL != pdat->fcc_temp_lut) {
		kfree(pdat->fcc_temp_lut);
		pdat->fcc_temp_lut = NULL;
	}
	if (NULL != pdat->fcc_sf_lut) {
		kfree(pdat->fcc_sf_lut);
		pdat->fcc_sf_lut = NULL;
	}
	if (NULL != pdat->pc_temp_ocv_lut0) {
		pdat->pc_temp_ocv_lut = NULL;
		kfree(pdat->pc_temp_ocv_lut0);
		pdat->pc_temp_ocv_lut0 = NULL;
	}
	if (NULL != pdat->pc_sf_lut) {
		kfree(pdat->pc_sf_lut);
		pdat->pc_sf_lut = NULL;
	}
	if (NULL != pdat->rbatt_sf_lut) {
		kfree(pdat->rbatt_sf_lut);
		pdat->rbatt_sf_lut = NULL;
	}
	if (NULL != pdat->chrg_para) {
		kfree(pdat->chrg_para);
		pdat->chrg_para = NULL;
	}
	if (NULL != pdat->pc_temp_ocv_lut1) {
		kfree(pdat->pc_temp_ocv_lut1);
		pdat->pc_temp_ocv_lut1 = NULL;
	}
	if (NULL != pdat->pc_temp_ocv_lut2) {
		kfree(pdat->pc_temp_ocv_lut2);
		pdat->pc_temp_ocv_lut2 = NULL;
	}
	if (NULL != pdat->pc_temp_ocv_lut3) {
		kfree(pdat->pc_temp_ocv_lut3);
		pdat->pc_temp_ocv_lut3 = NULL;
	}

	kfree(pdat);
	*p = NULL;
}

static int hisi_battery_data_probe(struct platform_device *pdev)
{
	int retval;
	unsigned int i;
	struct device_node *np;
	struct device_node *bat_node;
	/* get device node for battery module */

	bat_param_status = 0;
	np = pdev->dev.of_node;
	if (NULL == np) {
		bat_data_err("get device node failed\n");
		goto fatal_err;
	}
	/* get numeber of types */
	for (i = 0;; ++i) {
		if (!of_parse_phandle(np, "batt_name", i))
			break;

	}
	if (0 == i) {
		bat_data_err("hisi_bat_data_size is zero\n");
		goto fatal_err;
	}
	hisi_bat_data_size = i;
	bat_data_info("hisi_bat_data_size = %u\n", hisi_bat_data_size);

	/* alloc memory to store pointers(point to battery data) */
	p_data = (struct hisi_coul_battery_data **)kzalloc(hisi_bat_data_size * sizeof(struct hisi_coul_battery_data *), GFP_KERNEL);
	if (!p_data) {
		bat_data_err("alloc memory for p_data failed\n");
		goto fatal_err;
	}

	for (i = 0; i < hisi_bat_data_size; ++i) {
		retval = get_mem(&(p_data[i]));
		if (retval) {
			bat_data_err("get_mem[%d] failed\n", i);
			goto fatal_err;
		}
		bat_node = of_parse_phandle(np, "batt_name", i);
		if (NULL == bat_node) {
			bat_data_err("get bat_node failed\n");
			goto fatal_err;
		}
		retval = get_dat(bat_node, p_data[i]);
		if (retval) {
			bat_data_err("get_dat[%d] failed\n", i);
			goto fatal_err;
		}
	}
	platform_set_drvdata(pdev, p_data);
	bat_param_status = 1;

	bat_data_info("probe ok\n");
	return 0;
fatal_err:
	if (NULL != p_data) {
		for (i = 0; i < hisi_bat_data_size; ++i)
			free_mem(&(p_data[i]));

		kfree(p_data);
	}
	p_data = NULL;
	BUG();
	return -EINVAL;
}

static int hisi_battery_data_remove(struct platform_device *pdev)
{
	int i;
	struct hisi_coul_battery_data **di = platform_get_drvdata(pdev);

	if (di) {
		for (i = 0; i < hisi_bat_data_size; ++i)/*lint !e574*/
			free_mem(&(di[i]));

		kfree(di);
	}
	p_data = NULL;
	return 0;
}

static struct of_device_id hisi_bat_match_table[] = {
	{
	 .compatible = "hisi_battery",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver hisi_bat_driver = {
	.probe = hisi_battery_data_probe,
	.remove = hisi_battery_data_remove,
	.driver = {
		   .name = "hisi_battery",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hisi_bat_match_table),
		   },
};

int __init hisi_battery_init(void)
{
	return platform_driver_register(&hisi_bat_driver);
}

void __exit hisi_battery_exit(void)
{
	platform_driver_unregister(&hisi_bat_driver);
}

fs_initcall(hisi_battery_init);
module_exit(hisi_battery_exit);
MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("hisi battery module driver");
MODULE_LICENSE("GPL");
