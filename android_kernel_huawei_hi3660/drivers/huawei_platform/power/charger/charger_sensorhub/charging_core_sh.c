/*
 * drivers/power/huawei_charger/charging_core_sh.c
 *
 *huawei charging core sensorhub driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/raid/pq.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#include <huawei_platform/power/charging_core_sh.h>

#define HWLOG_TAG sensorhub
HWLOG_REGIST();

struct charge_core_info_sh *g_core_info_sh;

/**********************************************************
*  Function:       charge_core_battery_data
*  Discription:    get the charge raw data from hisi_battery_data module
*  Parameters:   di:charge_core_info_sh
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_core_battery_data(struct charge_core_info_sh *di)
{
	int i;
	unsigned int j;
	struct chrg_para_lut *p_batt_data = NULL;

	p_batt_data = hisi_battery_charge_params();
	if (NULL == p_batt_data) {
		hwlog_err("get battery params fail!\n");
		return -EINVAL;
	}

	di->data.temp_level = (unsigned int)((p_batt_data->temp_len) / TEMP_PARA_TOTAL);
	for (i = 0; i < (p_batt_data->temp_len) / TEMP_PARA_TOTAL; i++) {
		di->temp_para[i].temp_min =
		    p_batt_data->temp_data[i][TEMP_PARA_TEMP_MIN];
		di->temp_para[i].temp_max =
		    p_batt_data->temp_data[i][TEMP_PARA_TEMP_MAX];
		di->temp_para[i].iin_temp =
		    p_batt_data->temp_data[i][TEMP_PARA_IIN];
		di->temp_para[i].ichg_temp =
		    p_batt_data->temp_data[i][TEMP_PARA_ICHG];
		di->temp_para[i].vterm_temp =
		    p_batt_data->temp_data[i][TEMP_PARA_VTERM];
		di->temp_para[i].temp_back =
		    p_batt_data->temp_data[i][TEMP_PARA_TEMP_BACK];

		if (di->temp_para[i].iin_temp == -1)
			di->temp_para[i].iin_temp = di->data.iin_max;
		if (di->temp_para[i].ichg_temp == -1)
			di->temp_para[i].ichg_temp = di->data.ichg_max;

		hwlog_debug
		    ("temp_min = %d,temp_max = %d,iin_temp = %d,ichg_temp = %d,vterm_temp = %d,temp_back = %d\n",
		     di->temp_para[i].temp_min, di->temp_para[i].temp_max,
		     di->temp_para[i].iin_temp, di->temp_para[i].ichg_temp,
		     di->temp_para[i].vterm_temp, di->temp_para[i].temp_back);

		if ((di->temp_para[i].temp_min < BATTERY_TEMPERATURE_MIN)
		    || (di->temp_para[i].temp_min > BATTERY_TEMPERATURE_MAX)
		    || (di->temp_para[i].temp_max < BATTERY_TEMPERATURE_MIN)
		    || (di->temp_para[i].temp_max > BATTERY_TEMPERATURE_MAX)
		    || (di->temp_para[i].iin_temp < CHARGE_CURRENT_0000_MA)
		    || (di->temp_para[i].iin_temp > CHARGE_CURRENT_4000_MA)
		    || (di->temp_para[i].ichg_temp < CHARGE_CURRENT_0000_MA)
		    || (di->temp_para[i].ichg_temp > CHARGE_CURRENT_4000_MA)
		    || (di->temp_para[i].vterm_temp < BATTERY_VOLTAGE_3200_MV)
		    || (di->temp_para[i].vterm_temp > BATTERY_VOLTAGE_4500_MV)
		    || (di->temp_para[i].temp_back < BATTERY_TEMPERATURE_0_C)
		    || (di->temp_para[i].temp_back > BATTERY_TEMPERATURE_5_C)) {
			hwlog_err("the temp_para value is out of range!!\n");
			return -EINVAL;
		}
	}
	for (i = 0; i < (p_batt_data->volt_len) / VOLT_PARA_TOTAL; i++) {
		di->volt_para[i].vbat_min =
		    p_batt_data->volt_data[i][VOLT_PARA_VOLT_MIN];
		di->volt_para[i].vbat_max =
		    p_batt_data->volt_data[i][VOLT_PARA_VOLT_MAX];
		di->volt_para[i].iin_volt =
		    p_batt_data->volt_data[i][VOLT_PARA_IIN];
		di->volt_para[i].ichg_volt =
		    p_batt_data->volt_data[i][VOLT_PARA_ICHG];
		di->volt_para[i].volt_back =
		    p_batt_data->volt_data[i][VOLT_PARA_VOLT_BACK];

		if (di->volt_para[i].iin_volt == -1)
			di->volt_para[i].iin_volt = di->data.iin_max;
		if (di->volt_para[i].ichg_volt == -1)
			di->volt_para[i].ichg_volt = di->data.ichg_max;

		hwlog_debug
		    ("vbat_min = %d,vbat_max = %d,iin_volt = %d,ichg_volt = %d,volt_back = %d\n",
		     di->volt_para[i].vbat_min, di->volt_para[i].vbat_max,
		     di->volt_para[i].iin_volt, di->volt_para[i].ichg_volt,
		     di->volt_para[i].volt_back);

		if ((di->volt_para[i].vbat_min < BATTERY_VOLTAGE_MIN_MV)
		    || (di->volt_para[i].vbat_min > BATTERY_VOLTAGE_MAX_MV)
		    || (di->volt_para[i].vbat_max < BATTERY_VOLTAGE_MIN_MV)
		    || (di->volt_para[i].vbat_max > BATTERY_VOLTAGE_MAX_MV)
		    || (di->volt_para[i].iin_volt < CHARGE_CURRENT_0000_MA)
		    || (di->volt_para[i].iin_volt > CHARGE_CURRENT_4000_MA)
		    || (di->volt_para[i].ichg_volt < CHARGE_CURRENT_0000_MA)
		    || (di->volt_para[i].ichg_volt > CHARGE_CURRENT_4000_MA)
		    || (di->volt_para[i].volt_back < BATTERY_VOLTAGE_0000_MV)
		    || (di->volt_para[i].volt_back > BATTERY_VOLTAGE_0200_MV)) {
			hwlog_err("the volt_para value is out of range!!\n");
			return -EINVAL;
		}
	}

	/*Initialize segment charging param */

	di->data.segment_level =
	    (p_batt_data->segment_len) / SEGMENT_PARA_TOTAL;
	for (j = 0; j < di->data.segment_level; j++) {
		di->segment_para[j].vbat_min =
		    p_batt_data->segment_data[j][SEGMENT_PARA_VOLT_MIN];
		di->segment_para[j].vbat_max =
		    p_batt_data->segment_data[j][SEGMENT_PARA_VOLT_MAX];
		di->segment_para[j].ichg_segment =
		    p_batt_data->segment_data[j][SEGMENT_PARA_ICHG];
		di->segment_para[j].vterm_segment =
		    p_batt_data->segment_data[j][SEGMENT_PARA_VTERM];
		di->segment_para[j].volt_back =
		    p_batt_data->segment_data[j][SEGMENT_PARA_VOLT_BACK];

		hwlog_info
		    ("segment param: vbat_min = %d,vbat_max = %d,ichg_segment = %d,vterm_segment = %d,volt_back = %d\n",
		     di->segment_para[j].vbat_min, di->segment_para[j].vbat_max,
		     di->segment_para[j].ichg_segment,
		     di->segment_para[j].vterm_segment,
		     di->segment_para[j].volt_back);

		if ((di->segment_para[j].vbat_min < BATTERY_VOLTAGE_MIN_MV)
		    || (di->segment_para[j].vbat_min > BATTERY_VOLTAGE_MAX_MV)
		    || (di->segment_para[j].vbat_max < BATTERY_VOLTAGE_MIN_MV)
		    || (di->segment_para[j].vbat_max > BATTERY_VOLTAGE_MAX_MV)
		    || (di->segment_para[j].ichg_segment < CHARGE_CURRENT_0000_MA)
		    || (di->segment_para[j].ichg_segment > CHARGE_CURRENT_MAX_MA)
		    || (di->segment_para[j].vterm_segment < BATTERY_VOLTAGE_3200_MV)
		    || (di->segment_para[j].vterm_segment > BATTERY_VOLTAGE_4500_MV)
		    || (di->segment_para[j].volt_back < BATTERY_VOLTAGE_0000_MV)
		    || (di->segment_para[j].volt_back > BATTERY_VOLTAGE_0200_MV)) {
			hwlog_err("the segment_para value is out of range!!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void charge_core_parse_high_temp_limit(struct device_node *np,
				 struct charge_core_info_sh *di)
{
	int ret = 0;
	unsigned int high_temp_limit = 0;

	ret = of_property_read_u32(np, "high_temp_limit", &high_temp_limit);
	if(ret){
		hwlog_err("get high_temp_limit failed,use default config.\n");
	}
	di->data.high_temp_limit = high_temp_limit;

	hwlog_info("high_temp_limit = %d\n",di->data.high_temp_limit);
}

/**********************************************************
*  Function:       charge_core_parse_dts
*  Discription:    parse the module dts config value
*  Parameters:   np:device_node
*                      di:charge_core_info_sh
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_core_parse_dts(struct device_node *np,
				 struct charge_core_info_sh *di)
{
	int ret = 0;
	int i = 0;
	int array_len = 0;
	int idata = 0;
	const char *chrg_data_string = NULL;
	unsigned int vdpm_control_type = VDPM_BY_CAPACITY;
	unsigned int vdpm_buf_limit = VDPM_DELTA_LIMIT_5;

	/*wakesource charge current */
	ret = of_property_read_u32(np, "iin_weaksource", &(di->data.iin_weaksource));
	if (ret) {
		hwlog_info("get iin_weaksource failed , sign with an invalid number.\n");
		di->data.iin_weaksource = INVALID_CURRENT_SET;
	}
	hwlog_debug("iin_weaksource = %d\n", di->data.iin_weaksource);
	/*ac charge current */
	ret = of_property_read_u32(np, "iin_ac", &(di->data.iin_ac));
	if (ret) {
		hwlog_err("get iin_ac failed\n");
		return -EINVAL;
	}
	hwlog_debug("iin_ac = %d\n", di->data.iin_ac);
	ret = of_property_read_u32(np, "ichg_ac", &(di->data.ichg_ac));
	if (ret) {
		hwlog_err("get ichg_ac failed\n");
		return -EINVAL;
	}
	hwlog_debug("ichg_ac = %d\n", di->data.ichg_ac);

	/*fcp charge current */
	ret = of_property_read_u32(np, "iin_fcp", &(di->data.iin_fcp));
	if (ret) {
		hwlog_info("get iin_fcp failed ,use iin_ac's value instead \n");
		di->data.iin_fcp = di->data.iin_ac;
	}
	hwlog_debug("iin_fcp = %d\n", di->data.iin_fcp);
	ret = of_property_read_u32(np, "ichg_fcp", &(di->data.ichg_fcp));
	if (ret) {
		hwlog_info("get ichg_fcp failed ,use ichg_ac's value instead \n");
		di->data.ichg_fcp = di->data.ichg_ac;
	}
	hwlog_debug("ichg_fcp = %d\n", di->data.ichg_fcp);

	/*usb charge current */
	ret = of_property_read_u32(np, "iin_usb", &(di->data.iin_usb));
	if (ret) {
		hwlog_err("get iin_usb failed\n");
		return -EINVAL;
	}
	hwlog_debug("iin_usb = %d\n", di->data.iin_usb);
	ret = of_property_read_u32(np, "ichg_usb", &(di->data.ichg_usb));
	if (ret) {
		hwlog_err("get ichg_usb failed\n");
		return -EINVAL;
	}
	hwlog_debug("ichg_usb = %d\n", di->data.ichg_usb);
	/*nonstandard charge current */
	ret = of_property_read_u32(np, "iin_nonstd", &(di->data.iin_nonstd));
	if (ret) {
		hwlog_err("get iin_nonstd failed\n");
		return -EINVAL;
	}
	hwlog_debug("iin_nonstd = %d\n", di->data.iin_nonstd);
	ret = of_property_read_u32(np, "ichg_nonstd", &(di->data.ichg_nonstd));
	if (ret) {
		hwlog_err("get ichg_nonstd failed\n");
		return -EINVAL;
	}
	hwlog_debug("ichg_nonstd = %d\n", di->data.ichg_nonstd);
	/*Charging Downstream Port */
	ret = of_property_read_u32(np, "iin_bc_usb", &(di->data.iin_bc_usb));
	if (ret) {
		hwlog_err("get iin_bc_usb failed\n");
		return -EINVAL;
	}
	hwlog_debug("iin_bc_usb = %d\n", di->data.iin_bc_usb);
	ret = of_property_read_u32(np, "ichg_bc_usb", &(di->data.ichg_bc_usb));
	if (ret) {
		hwlog_err("get ichg_bc_usb failed\n");
		return -EINVAL;
	}
	hwlog_debug("ichg_bc_usb = %d\n", di->data.ichg_bc_usb);
	/*VR Charge current */
	ret = of_property_read_u32(np, "iin_vr", &(di->data.iin_vr));
	if (ret) {
		hwlog_err("get iin_vr failed\n");
		return -EINVAL;
	}
	hwlog_debug("iin_vr = %d\n", di->data.iin_vr);
	ret = of_property_read_u32(np, "ichg_vr", &(di->data.ichg_vr));
	if (ret) {
		hwlog_err("get ichg_vr failed\n");
		return -EINVAL;
	}
	hwlog_debug("ichg_vr = %d\n", di->data.ichg_vr);
	/*terminal current */
	ret = of_property_read_u32(np, "iterm", &(di->data.iterm));
	if (ret) {
		hwlog_err("get iterm failed\n");
		return -EINVAL;
	}
	hwlog_debug("iterm = %d\n", di->data.iterm);
	/*otg current */
	ret = of_property_read_u32(np, "otg_curr", &(di->data.otg_curr));
	if (ret) {
		hwlog_err("get otg_curr failed\n");
		return -EINVAL;
	}
	hwlog_debug("otg_curr = %d\n", di->data.otg_curr);
	/*segment para type */
	ret =
	    of_property_read_u32(np, "segment_type", &(di->data.segment_type));
	if (ret) {
		hwlog_err("get segment_type failed\n");
		return -EINVAL;
	}
	/*TypeC High mode current */
	ret =
	    of_property_read_u32(np, "typec_support",
				 &(di->data.typec_support));
	if (ret) {
		hwlog_err("get typec support flag!\n");
		return -EINVAL;
	}
	hwlog_info("typec support flag = %d\n", di->data.typec_support);

	ret = of_property_read_u32(np, "iin_typech", &(di->data.iin_typech));
	if (ret) {
		hwlog_err("get typec high mode ibus curr failed\n");
		return -EINVAL;
	}
	hwlog_info("typec high mode ibus curr = %d\n", di->data.iin_typech);
	ret = of_property_read_u32(np, "ichg_typech", &(di->data.ichg_typech));
	if (ret) {
		hwlog_err("get typec high mode ibat curr failed\n");
		return -EINVAL;
	}
	hwlog_info("typec high mode ibat curr = %d\n", di->data.ichg_typech);

	charge_core_parse_high_temp_limit(np, di);

	/*vdpm_para*/
	/*vdpm control type : 0 vdpm controlled by cbat; 1 vdpm controlled by vbat*/
	ret = of_property_read_u32(np, "vdpm_control_type", &(vdpm_control_type));
	if(ret){
        hwlog_err("get vdpm_control_type failed, use default config.\n");
	}
	di->data.vdpm_control_type = vdpm_control_type;
	hwlog_info("vdpm_control_type = %d\n",di->data.vdpm_control_type);
	/*vdpm buffer setting*/
	ret = of_property_read_u32(np, "vdpm_buf_limit", &(vdpm_buf_limit));
	if(ret){
        hwlog_err("get vdpm_buf_limit failed,use default config.\n");
	}
	di->data.vdpm_buf_limit = vdpm_buf_limit;
	hwlog_info("vdpm_buf_limit = %d\n",di->data.vdpm_buf_limit);
	array_len = of_property_count_strings(np, "vdpm_para");
	if ((array_len <= 0) || (array_len % VDPM_PARA_TOTAL != 0)) {
		hwlog_err
		    ("vdpm_para is invaild,please check vdpm_para number!!\n");
		return -EINVAL;
	}

	if (array_len > VDPM_PARA_LEVEL * VDPM_PARA_TOTAL) {
		array_len = VDPM_PARA_LEVEL * VDPM_PARA_TOTAL;
		hwlog_err("vdpm_para is too long,use only front %d paras!!\n",
			  array_len);
		return -EINVAL;
	}

	memset(di->vdpm_para, 0, VDPM_PARA_LEVEL * sizeof(struct charge_vdpm_data));	/* data reset to 0*/

	for (i = 0; i < array_len; i++) {
		ret =
		    of_property_read_string_index(np, "vdpm_para", i,
						  &chrg_data_string);
		if (ret) {
			hwlog_err("get vdpm_para failed\n");
			return -EINVAL;
		}

		idata = simple_strtol(chrg_data_string, NULL, 10);
		switch (i % VDPM_PARA_TOTAL) {
		case VDPM_PARA_CAP_MIN:
			if ((idata < VDPM_CBAT_MIN) || (idata > VDPM_CBAT_MAX)) {
				hwlog_err
				    ("the vdpm_para cap_min is out of range!!\n");
				return -EINVAL;
			}
			di->vdpm_para[i / (VDPM_PARA_TOTAL)].cap_min = idata;
			break;
		case VDPM_PARA_CAP_MAX:
			if ((idata < VDPM_CBAT_MIN) || (idata > VDPM_CBAT_MAX)) {
				hwlog_err
				    ("the vdpm_para cap_max is out of range!!\n");
				return -EINVAL;
			}
			di->vdpm_para[i / (VDPM_PARA_TOTAL)].cap_max = idata;
			break;
		case VDPM_PARA_DPM:
			if ((idata < VDPM_VOLT_MIN) || (idata > VDPM_VOLT_MAX)) {
				hwlog_err
				    ("the vdpm_para vin_dpm is out of range!!\n");
				return -EINVAL;
			}
			di->vdpm_para[i / (VDPM_PARA_TOTAL)].vin_dpm = idata;
			break;
		case VDPM_PARA_CAP_BACK:
			if((idata < 0) || (idata > (int)(di->data.vdpm_buf_limit))){
				hwlog_err
				    ("the vdpm_para cap_back is out of range!!\n");
				return -EINVAL;
			}
			di->vdpm_para[i / (VDPM_PARA_TOTAL)].cap_back = idata;
			break;
		default:
			hwlog_err("get vdpm_para failed\n");
		}
		hwlog_debug("di->vdpm_para[%d][%d] = %d\n",
			    i / (VDPM_PARA_TOTAL), i % (VDPM_PARA_TOTAL), idata);
	}

	/* inductance_para */
	memset(di->inductance_para, 0, INDUCTANCE_PARA_LEVEL * sizeof(struct charge_inductance_data));	/*data reset to 0*/

	array_len = of_property_count_strings(np, "inductance_para");
	if ((array_len <= 0) || (array_len % INDUCTANCE_PARA_TOTAL != 0)) {
		hwlog_err
		    ("inductance_para is invaild,please check inductance_para number!!\n");
		return -EINVAL;
	}
	if (array_len > INDUCTANCE_PARA_LEVEL * INDUCTANCE_PARA_TOTAL) {
		array_len = INDUCTANCE_PARA_LEVEL * INDUCTANCE_PARA_TOTAL;
		hwlog_err
		    ("inductance_para is too long,use only front %d paras!!\n",
		     array_len);
		return -EINVAL;
	}

	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "inductance_para", i, &chrg_data_string);
		if (ret) {
			hwlog_err("get inductance_para failed\n");
			return -EINVAL;
		}

		idata = simple_strtol(chrg_data_string, NULL, 10);
		switch (i % INDUCTANCE_PARA_TOTAL) {
		case INDUCTANCE_PARA_CAP_MIN:
			if ((idata < INDUCTANCE_CBAT_MIN)
			    || (idata > INDUCTANCE_CBAT_MAX)) {
				hwlog_err
				    ("the inductance_para cap_min is out of range!!\n");
				return -EINVAL;
			}
			di->inductance_para[i / (INDUCTANCE_PARA_TOTAL)].cap_min = idata;
			break;
		case INDUCTANCE_PARA_CAP_MAX:
			if ((idata < INDUCTANCE_CBAT_MIN)
			    || (idata > INDUCTANCE_CBAT_MAX)) {
				hwlog_err
				    ("the inductance_para cap_max is out of range!!\n");
				return -EINVAL;
			}
			di->inductance_para[i / (INDUCTANCE_PARA_TOTAL)].cap_max = idata;
			break;
		case INDUCTANCE_PARA_IIN:
			if ((idata < INDUCTANCE_IIN_MIN)
			    || (idata > INDUCTANCE_IIN_MAX)) {
				hwlog_err
				    ("the inductance_para iin is out of range!!\n");
				return -EINVAL;
			}
			di->inductance_para[i / (INDUCTANCE_PARA_TOTAL)].iin_inductance = idata;
			break;
		case INDUCTANCE_PARA_CAP_BACK:
			if ((idata < 0) || (idata > INDUCTANCE_CAP_DETA)) {
				hwlog_err
				    ("the inductance_para cap_back is out of range!!\n");
				return -EINVAL;
			}
			di->inductance_para[i / (INDUCTANCE_PARA_TOTAL)].cap_back = idata;
			break;
		default:
			hwlog_err("get vdpm_para failed\n");
		}
		hwlog_info("di->inductance_para[%d][%d] = %d\n",
			   i / (INDUCTANCE_PARA_TOTAL),
			   i % (INDUCTANCE_PARA_TOTAL), idata);
	}

	if (strstr(saved_command_line, "androidboot.swtype=factory")
	    && (!is_hisi_battery_exist())) {
		di->data.iin_ac = CHARGE_CURRENT_2000_MA;
		di->data.ichg_ac = CHARGE_CURRENT_1900_MA;
		di->data.iin_bc_usb = CHARGE_CURRENT_2000_MA;
		di->data.ichg_bc_usb = CHARGE_CURRENT_1900_MA;
		hwlog_info
		    ("factory version,iin_ac = %d mA,ichg_ac %d mA,iin_bc_usb = %d mA,ichg_bc_usb = %d mA\n",
		     di->data.iin_ac, di->data.ichg_ac, di->data.iin_bc_usb,
		     di->data.ichg_bc_usb);
	}
	di->data.iin_max = di->data.iin_ac < di->data.iin_fcp ? di->data.iin_fcp : di->data.iin_ac;
	di->data.ichg_max = di->data.ichg_ac < di->data.ichg_fcp ? di->data.ichg_fcp : di->data.ichg_ac;
	hwlog_info("iin_max = %d mA,ichg_max %d mA\n", di->data.iin_max, di->data.ichg_max);
	return 0;
}

/**********************************************************
*  Function:       charge_core_probe
*  Discription:    charge module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int charge_core_probe(struct platform_device *pdev)
{
	struct charge_core_info_sh *di;
	int ret = 0;
	struct device_node *np = NULL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (NULL == di) {
		hwlog_err("charge_core_info_sh is NULL!\n");
		return -ENOMEM;
	}

	g_core_info_sh = di;
	np = pdev->dev.of_node;
	platform_set_drvdata(pdev, di);

	ret = charge_core_parse_dts(np, di);//share memory to sensorhub
	if (ret < 0) {
		hwlog_err("get sensorhub charge_data from dts fail!\n");
		goto err_batt;
	}

	ret = charge_core_battery_data(di);//share memory to sensorhub
	if (ret < 0) {
		hwlog_err("get sensorhub battery charge data fail!\n");
		goto err_batt;
	}

	hwlog_info("charging core sensorhub init ok!\n");
	return 0;
	
err_batt:
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	g_core_info_sh = NULL;
	return ret;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_core_remove
*  Discription:    charge module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int charge_core_remove(struct platform_device *pdev)
{
	struct charge_core_info_sh *di = platform_get_drvdata(pdev);

	if (di == NULL) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return -ENODEV;
	}

	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	g_core_info_sh = NULL;

	return 0;
}

static struct of_device_id charge_core_match_table[] = {
	{
	 .compatible = "huawei,charging_core_sensorhub",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver charge_core_driver = {
	.probe = charge_core_probe,
	.remove = charge_core_remove,
	.driver = {
		   .name = "huawei,charging_core_sensorhub",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(charge_core_match_table),
		   },
};
/*lint -restore*/

/**********************************************************
*  Function:       charge_core_init
*  Discription:    charge module initialization
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init charge_core_init(void)
{
	return platform_driver_register(&charge_core_driver);
}

/**********************************************************
*  Function:       charge_core_exit
*  Discription:    charge module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit charge_core_exit(void)
{
	platform_driver_unregister(&charge_core_driver);
}

/*lint -save -e* */
module_init(charge_core_init);
module_exit(charge_core_exit);
/*lint -restore*/

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("charging core sensorhub module driver");
MODULE_LICENSE("GPL");
