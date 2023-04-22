/*
 * Copyright (C) huawei company
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/rtc.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/hisi/usb/hisi_usb.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#ifdef CONFIG_HW_TOUCH_KEY
#include <huawei_platform/sensor/huawei_key.h>
#endif
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "contexthub_recovery.h"
#include "protocol.h"
#include "sensor_config.h"
#include "sensor_detect.h"
#include "tp_color.h"

int hisi_nve_direct_access(struct hisi_nve_info_user *user_info);

uint8_t hp_offset[24];
char sensor_chip_info[SENSOR_MAX][MAX_CHIP_INFO_LEN];
u8 phone_color;
u8 tp_manufacture = TS_PANEL_UNKNOWN;
int als_para_table = 0;
uint8_t ps_sensor_calibrate_data[MAX_SENSOR_CALIBRATE_DATA_LENGTH];
uint8_t tof_sensor_calibrate_data[TOF_CALIDATA_NV_SIZE];
struct hisi_nve_info_user user_info;
uint8_t als_sensor_calibrate_data[MAX_SENSOR_CALIBRATE_DATA_LENGTH];
union sar_calibrate_data sar_calibrate_datas;
extern int is_cali_supported;
extern int tmd2745_flag;
extern int rohm_rpr531_flag;
extern int tsl2591_flag;
extern int bh1726_flag;
s16 minThreshold_als_para;
s16 maxThreshold_als_para;

int gsensor_offset[ACC_CALIBRATE_DATA_LENGTH];	/*g-sensor calibrate data*/
int gyro_sensor_offset[GYRO_CALIBRATE_DATA_LENGTH];
static char gyro_temperature_offset[GYRO_TEMP_CALI_NV_SIZE];
int ps_sensor_offset[PS_CALIBRATE_DATA_LENGTH];
static uint8_t tof_sensor_offset[TOF_CALIDATA_NV_SIZE];
uint16_t als_offset[ALS_CALIBRATE_DATA_LENGTH];
uint16_t als_dark_noise_offset = 0;
static char str_charger[] = "charger_plug_in_out";
static uint8_t gsensor_calibrate_data[MAX_SENSOR_CALIBRATE_DATA_LENGTH];
static uint8_t msensor_calibrate_data[MAX_MAG_CALIBRATE_DATA_LENGTH];
static uint8_t msensor_akm_calibrate_data[MAX_MAG_AKM_CALIBRATE_DATA_LENGTH];
static uint8_t gyro_sensor_calibrate_data[GYRO_CALIDATA_NV_SIZE];
static uint8_t gyro_temperature_calibrate_data[GYRO_TEMP_CALI_NV_SIZE];
static uint8_t handpress_calibrate_data[MAX_SENSOR_CALIBRATE_DATA_LENGTH];
static char vib_calib[VIB_CALIDATA_NV_SIZE] = { 0 };

extern int first_start_flag;
extern int ps_first_start_flag;
extern int txc_ps_flag;
extern int ams_tmd2620_ps_flag;
extern int avago_apds9110_ps_flag;
extern int als_first_start_flag;
extern int gyro_first_start_flag;
extern int handpress_first_start_flag;
extern int rohm_rgb_flag;
extern int avago_rgb_flag;
extern int  ams_tmd3725_rgb_flag;
extern int  ams_tmd3725_ps_flag;
extern int liteon_ltr582_ps_flag;
extern int liteon_ltr582_rgb_flag;
extern int ltr578_flag;
extern int rpr531_flag;
extern int tmd2745_flag;
extern int apds9922_flag;
extern int g_iom3_state;
extern int iom3_power_state;
extern u8 tplcd_manufacture;
extern int akm_cal_algo;
extern int apds9999_rgb_flag;
extern int apds9999_ps_flag;
extern int  ams_tmd3702_rgb_flag;
extern int  ams_tmd3702_ps_flag;
extern int apds9253_rgb_flag;
extern int  vishay_vcnl36658_als_flag;
extern int  vishay_vcnl36658_ps_flag;
extern int ams_tof_flag;
extern int sharp_tof_flag;
extern struct airpress_platform_data airpress_data;
extern struct sar_platform_data sar_pdata;
extern struct als_platform_data als_data;
extern struct compass_platform_data mag_data;
#ifdef CONFIG_HUAWEI_DSM
extern struct dsm_client *shb_dclient;
#endif
extern void resend_als_parameters_to_mcu(void);

void __dmd_log_report(int dmd_mark, const char *err_func, const char *err_msg)
{
#ifdef CONFIG_HUAWEI_DSM
	if (!dsm_client_ocuppy(shb_dclient)) {
		dsm_client_record(shb_dclient, "[%s]%s", err_func, err_msg);
		dsm_client_notify(shb_dclient, dmd_mark);
	}
#endif
}

int read_calibrate_data_from_nv(int nv_number, int nv_size, char *nv_name)
{
	int ret = 0;

	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = NV_READ_TAG;
	user_info.nv_number = nv_number;
	user_info.valid_size = nv_size;
	strncpy(user_info.nv_name, nv_name, sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	if ((ret = hisi_nve_direct_access(&user_info))!=0)
	{
		hwlog_err("hisi_nve_direct_access read nv %d error(%d)\n", nv_number, ret);
		return -1;
	}
	return 0;
}

int write_calibrate_data_to_nv(int nv_number, int nv_size, char *nv_name, char *temp)
{
	int ret = 0;
	struct hisi_nve_info_user local_user_info;
	memset(&local_user_info, 0, sizeof(local_user_info));
	local_user_info.nv_operation = NV_WRITE_TAG;
	local_user_info.nv_number = nv_number;
	local_user_info.valid_size = nv_size;
	strncpy(local_user_info.nv_name, nv_name, sizeof(local_user_info.nv_name));
	local_user_info.nv_name[sizeof(local_user_info.nv_name) - 1] = '\0';
	//copy to nv by pass
	memcpy(local_user_info.nv_data, temp, sizeof(local_user_info.nv_data) < local_user_info.valid_size ? sizeof(local_user_info.nv_data) : local_user_info.valid_size);
	if ((ret = hisi_nve_direct_access(&local_user_info))!=0)
	{
		hwlog_err("hisi_nve_direct_access read nv %d error(%d)\n", nv_number, ret);
		return -1;
	}
	return 0;
}

int send_calibrate_data_to_mcu(int tag, uint32_t subcmd, const void *data, int length, bool is_recovery)
{
	int ret = 0;
    	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	pkt_parameter_req_t cpkt;
	pkt_header_t *hd = (pkt_header_t *)&cpkt;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	pkg_ap.tag=tag;
	pkg_ap.cmd=CMD_CMN_CONFIG_REQ;
	cpkt.subcmd = subcmd;
	pkg_ap.wr_buf=&hd[1];
	pkg_ap.wr_len=length+SUBCMD_LEN;
	memcpy(cpkt.para, data, length);

	if(is_recovery)
		return write_customize_cmd(&pkg_ap, NULL, false);

	ret=write_customize_cmd(&pkg_ap,  &pkg_mcu, true);
	if(ret) {
		hwlog_err("send tag %d calibrate data to mcu fail,ret=%d\n", tag, ret);
		return -1;
	}
	if(pkg_mcu.errno!=0) {
		hwlog_err("send tag %d  calibrate data fail,err=%d\n", tag, pkg_mcu.errno);
		return -1;
	} else {
		hwlog_info("send tag %d calibrate data to mcu success\n",tag);
	}
	return 0;
}


/*******************************************************************************
Function:	write_gsensor_offset_to_nv
Description:  将temp数据写入NV 项中
Data Accessed:  无
Data Updated:   无
Input:        g-sensor 校准值
Output:         无
Return:         成功或者失败信息: 0->成功, -1->失败
*******************************************************************************/
int write_gsensor_offset_to_nv(char *temp, int length)
{
	int ret = 0;

	if (temp == NULL) {
		hwlog_err("write_gsensor_offset_to_nv fail, invalid para!\n");
		return -1;
	}

	if (write_calibrate_data_to_nv(ACC_OFFSET_NV_NUM, length, "gsensor", temp))
		return -1;
	memcpy((void *)gsensor_calibrate_data, (void *)temp, MAX_SENSOR_CALIBRATE_DATA_LENGTH);
	memcpy(gsensor_offset, gsensor_calibrate_data,
	       (sizeof(gsensor_offset) < ACC_OFFSET_NV_SIZE) ? sizeof(gsensor_offset) : ACC_OFFSET_NV_SIZE);
	hwlog_info("nve_direct_access write temp (%d %d %d  %d %d %d %d %d %d  %d %d %d %d %d %d)\n",
		   gsensor_offset[0], gsensor_offset[1], gsensor_offset[2],gsensor_offset[3], gsensor_offset[4],
		   gsensor_offset[5],gsensor_offset[6], gsensor_offset[7],gsensor_offset[8],gsensor_offset[9],
		   gsensor_offset[10],gsensor_offset[11],gsensor_offset[12], gsensor_offset[13],gsensor_offset[14]);

	return ret;
}

/*******************************************************************************
Function:	send_gsensor_calibrate_data_to_mcu
Description:   读取NV项中的gsensor 校准数据，并发送给mcu 侧
Data Accessed:  无
Data Updated:   无
Input:         无
Output:         无
Return:         成功或者失败信息: 0->成功, -1->失败
*******************************************************************************/
int send_gsensor_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(ACC_OFFSET_NV_NUM, ACC_OFFSET_NV_SIZE, "gsensor"))
		return -1;

	first_start_flag=1;
	//copy to gsensor_offset by pass
	memcpy(gsensor_offset, user_info.nv_data, sizeof(gsensor_offset));
	hwlog_info("nve_direct_access read gsensor_offset_sen (%d %d %d %d %d %d)\n",
	     gsensor_offset[0], gsensor_offset[1], gsensor_offset[2],
	     gsensor_offset[3], gsensor_offset[4], gsensor_offset[5]);
	hwlog_info("nve_direct_access read gsensor_xis_angle (%d %d %d %d %d %d %d %d %d)\n",
		gsensor_offset[6],gsensor_offset[7],gsensor_offset[8],gsensor_offset[9],gsensor_offset[10],
		gsensor_offset[11],gsensor_offset[12],gsensor_offset[13],gsensor_offset[14]);

	memcpy(gsensor_calibrate_data, gsensor_offset,
	       (sizeof(gsensor_calibrate_data) <	ACC_OFFSET_NV_SIZE) ? sizeof(gsensor_calibrate_data) : ACC_OFFSET_NV_SIZE);
	if (send_calibrate_data_to_mcu(TAG_ACCEL, SUB_CMD_SET_OFFSET_REQ, gsensor_offset, ACC_OFFSET_NV_SIZE, false))
		return -1;

	return 0;
}

int send_gyro_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(GYRO_CALIDATA_NV_NUM, GYRO_CALIDATA_NV_SIZE, "GYRO"))
		return -1;

	gyro_first_start_flag = 1;
	/*copy to gsensor_offset by pass*/
	memcpy(gyro_sensor_offset, user_info.nv_data, sizeof(gyro_sensor_offset));
	hwlog_info( "nve_direct_access read gyro_sensor offset: %d %d %d  sensitity:%d %d %d \n",
		gyro_sensor_offset[0],gyro_sensor_offset[1],gyro_sensor_offset[2], gyro_sensor_offset[3],gyro_sensor_offset[4],gyro_sensor_offset[5]);
	hwlog_info( "nve_direct_access read gyro_sensor xis_angle: %d %d %d  %d %d %d %d %d %d \n",
		gyro_sensor_offset[6], gyro_sensor_offset[7], gyro_sensor_offset[8], gyro_sensor_offset[9], gyro_sensor_offset[10],
		gyro_sensor_offset[11],gyro_sensor_offset[12],gyro_sensor_offset[13],gyro_sensor_offset[14]);
	hwlog_info( "nve_direct_access read gyro_sensor online offset: %d %d %d\n",
		gyro_sensor_offset[15], gyro_sensor_offset[16], gyro_sensor_offset[17]);

	memcpy(&gyro_sensor_calibrate_data, gyro_sensor_offset,
	       (sizeof(gyro_sensor_calibrate_data) < GYRO_CALIDATA_NV_SIZE) ? sizeof(gyro_sensor_calibrate_data) : GYRO_CALIDATA_NV_SIZE);
	if (send_calibrate_data_to_mcu(TAG_GYRO, SUB_CMD_SET_OFFSET_REQ, gyro_sensor_offset, GYRO_CALIDATA_NV_SIZE, false))
		return -1;
	return 0;
}

int send_gyro_temperature_offset_to_mcu(void)
{
	if (read_calibrate_data_from_nv(GYRO_TEMP_CALI_NV_NUM, GYRO_TEMP_CALI_NV_SIZE, "GYTMP"))
		return -1;

	/*copy to gsensor_offset by pass*/
	memcpy(gyro_temperature_offset, user_info.nv_data, sizeof(gyro_temperature_offset));

	if (send_calibrate_data_to_mcu(TAG_GYRO, SUB_CMD_GYRO_TMP_OFFSET_REQ, gyro_temperature_offset, sizeof(gyro_temperature_offset), false))
		return -1;

	memcpy(&gyro_temperature_calibrate_data, gyro_temperature_offset,
	       (sizeof(gyro_temperature_calibrate_data) < sizeof(gyro_temperature_offset)) ?
	       sizeof(gyro_temperature_calibrate_data) : sizeof(gyro_temperature_offset));
	return 0;
}

int write_gyro_sensor_offset_to_nv(char *temp, int length)
{
	int ret = 0;

	if (temp == NULL) {
		hwlog_err("write_gyro_sensor_offset_to_nv fail, invalid para!\n");
		return -1;
	}

	if (write_calibrate_data_to_nv(GYRO_CALIDATA_NV_NUM, length, "GYRO", temp))
		return -1;

	memcpy(gyro_sensor_calibrate_data, temp, sizeof(gyro_sensor_calibrate_data) < length ? sizeof(gyro_sensor_calibrate_data) : length);
	memcpy(gyro_sensor_offset, gyro_sensor_calibrate_data,
	       (sizeof(gyro_sensor_offset) < GYRO_CALIDATA_NV_SIZE) ? sizeof(gyro_sensor_offset) : GYRO_CALIDATA_NV_SIZE);
	hwlog_info("gyro calibrate data (%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d),online offset %d %d %d lens=%d\n",
		gyro_sensor_offset[0], gyro_sensor_offset[1], gyro_sensor_offset[2],gyro_sensor_offset[3], gyro_sensor_offset[4],
		gyro_sensor_offset[5],gyro_sensor_offset[6], gyro_sensor_offset[7],gyro_sensor_offset[8],gyro_sensor_offset[9],
		gyro_sensor_offset[10],gyro_sensor_offset[11],gyro_sensor_offset[12], gyro_sensor_offset[13],gyro_sensor_offset[14],
		gyro_sensor_offset[15],gyro_sensor_offset[16],gyro_sensor_offset[17],length);
	return ret;
}

int write_gyro_temperature_offset_to_nv(char *temp, int length)
{
	int ret = 0;

	if (temp == NULL) {
		hwlog_err("write_gyro_temp_offset_to_nv fail, invalid para!\n");
		return -1;
	}
	if (write_calibrate_data_to_nv(GYRO_TEMP_CALI_NV_NUM, length, "GYTMP", temp))
		return -1;

	memcpy(gyro_temperature_calibrate_data, temp,
		sizeof(gyro_temperature_calibrate_data) < length ? sizeof(gyro_temperature_calibrate_data) : length);
	hwlog_info("write_gyro_temp_offset_to_nv suc len=%d \n", length);
	return ret;
}

int send_vibrator_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(VIB_CALIDATA_NV_NUM, VIB_CALIDATA_NV_SIZE, VIB_CALIDATA_NV_NAME)){
		return -1;
	}
	vib_calib[0] = (int8_t) user_info.nv_data[0];
	vib_calib[1] = (int8_t) user_info.nv_data[1];
	vib_calib[2] = (int8_t) user_info.nv_data[2];
	hwlog_err("%s read vib_calib (0x%x  0x%x  0x%x )\n", __func__, vib_calib[0], vib_calib[1], vib_calib[2]);
	if (send_calibrate_data_to_mcu(TAG_VIBRATOR, SUB_CMD_SET_OFFSET_REQ, vib_calib, 3, false)){
		hwlog_err("send para fail\n");
		return -1;
	}
	return 0;
}

int send_mag_calibrate_data_to_mcu(void)
{
	int mag_size = 0;
	if (1 == akm_cal_algo) {
		mag_size = MAG_AKM_CALIBRATE_DATA_NV_SIZE;
	} else {
		mag_size = MAG_CALIBRATE_DATA_NV_SIZE;
	}

	if (read_calibrate_data_from_nv(MAG_CALIBRATE_DATA_NV_NUM, mag_size, "msensor"))
		return -1;

	if (1 == akm_cal_algo) {
		memcpy(msensor_akm_calibrate_data, user_info.nv_data, MAG_AKM_CALIBRATE_DATA_NV_SIZE);
		hwlog_info("send mag_sensor data %d, %d, %d to mcu success\n", msensor_akm_calibrate_data[0],
			   msensor_akm_calibrate_data[1], msensor_akm_calibrate_data[2]);
		if (send_calibrate_data_to_mcu(TAG_MAG, SUB_CMD_SET_OFFSET_REQ, msensor_akm_calibrate_data, MAG_AKM_CALIBRATE_DATA_NV_SIZE, false))
			return -1;
	} else {
		memcpy(msensor_calibrate_data, user_info.nv_data, MAG_CALIBRATE_DATA_NV_SIZE);
		hwlog_info("send mag_sensor data %d, %d, %d to mcu success\n",
			msensor_calibrate_data[0], msensor_calibrate_data[1], msensor_calibrate_data[2]);
		if (send_calibrate_data_to_mcu(TAG_MAG, SUB_CMD_SET_OFFSET_REQ, msensor_calibrate_data, MAG_CALIBRATE_DATA_NV_SIZE, false))
			return -1;
	}

	return 0;
}

int write_magsensor_calibrate_data_to_nv(char *src)
{
	int ret = 0;
	int mag_size = 0;

	if (NULL == src) {
		hwlog_err("%s fail, invalid para!\n", __func__);
		return -1;
	}

	if (1 == akm_cal_algo) {
		mag_size = MAG_AKM_CALIBRATE_DATA_NV_SIZE;
		memcpy(&msensor_akm_calibrate_data, src, sizeof(msensor_akm_calibrate_data));
		if (write_calibrate_data_to_nv(MAG_CALIBRATE_DATA_NV_NUM, mag_size, "msensor", src))
			return -1;
	} else {
		mag_size = MAG_CALIBRATE_DATA_NV_SIZE;
		memcpy(&msensor_calibrate_data, src, sizeof(msensor_calibrate_data));
		if (write_calibrate_data_to_nv(MAG_CALIBRATE_DATA_NV_NUM, mag_size, "msensor", src))
			return -1;
	}

	return ret;
}

static int mag_enviroment_change_notify(struct notifier_block *nb,
			unsigned long action, void *data)
{
	if (send_calibrate_data_to_mcu(TAG_MAG, SUB_CMD_SET_OFFSET_REQ, str_charger, sizeof(str_charger), false))
		return -1;

	return 0;
}

int send_ps_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(PS_CALIDATA_NV_NUM, PS_CALIDATA_NV_SIZE, "PSENSOR"))
		return -1;

	ps_first_start_flag = 1;

	memcpy(ps_sensor_offset, user_info.nv_data, sizeof(ps_sensor_offset));
	hwlog_info( "nve_direct_access read ps_offset (%d,%d,%d)\n", ps_sensor_offset[0], ps_sensor_offset[1], ps_sensor_offset[2]);
	memcpy(&ps_sensor_calibrate_data, ps_sensor_offset,
	       (sizeof(ps_sensor_calibrate_data) < PS_CALIDATA_NV_SIZE) ? sizeof(ps_sensor_calibrate_data) : PS_CALIDATA_NV_SIZE);

	if (send_calibrate_data_to_mcu(TAG_PS, SUB_CMD_SET_OFFSET_REQ, ps_sensor_offset, PS_CALIDATA_NV_SIZE, false))
		return -1;

	return 0;
}
int send_tof_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(PS_CALIDATA_NV_NUM, TOF_CALIDATA_NV_SIZE, "PSENSOR"))
		return -1;

	ps_first_start_flag = 1;

	memcpy(tof_sensor_offset, user_info.nv_data, sizeof(tof_sensor_offset));
	hwlog_info("nve_direct_access read tof sensor offset offset[0]=%d offset[9]=%d offset[19]=%d offset[27]=%d)\n",
	     tof_sensor_offset[0], tof_sensor_offset[9], tof_sensor_offset[19], tof_sensor_offset[TOF_CALIDATA_NV_SIZE - 1]);

	memcpy(&tof_sensor_calibrate_data, tof_sensor_offset,
	       (sizeof(tof_sensor_calibrate_data) < TOF_CALIDATA_NV_SIZE) ? sizeof(tof_sensor_calibrate_data) : TOF_CALIDATA_NV_SIZE);

	if (send_calibrate_data_to_mcu(TAG_TOF, SUB_CMD_SET_OFFSET_REQ, tof_sensor_offset, TOF_CALIDATA_NV_SIZE, false))
		return -1;

	return 0;
}

int send_als_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(ALS_CALIDATA_NV_NUM, ALS_CALIDATA_NV_SIZE, "LSENSOR"))
		return -1;

	als_first_start_flag = 1;
	memcpy(als_offset, user_info.nv_data, sizeof(als_offset));
	memcpy(&als_sensor_calibrate_data, als_offset, ALS_CALIDATA_NV_SIZE);
	hwlog_info("nve_direct_access read lsensor_offset (%d %d %d %d %d %d)\n",
	     als_offset[0], als_offset[1], als_offset[2], als_offset[3], als_offset[4], als_offset[5]);

	if (send_calibrate_data_to_mcu(TAG_ALS, SUB_CMD_SET_OFFSET_REQ, als_sensor_calibrate_data, ALS_CALIDATA_NV_SIZE, false))
		return -1;

	return 0;
}

/*
* handpress nv read write
*/
int send_handpress_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(HANDPRESS_CALIDATA_NV_NUM, HANDPRESS_CALIDATA_NV_SIZE, "HPDATA"))
		return -1;

	handpress_first_start_flag = 1;
	memcpy(hp_offset, user_info.nv_data, HANDPRESS_CALIDATA_NV_SIZE);
	memcpy(&handpress_calibrate_data, hp_offset,
	       (sizeof(handpress_calibrate_data) < sizeof(hp_offset)) ? sizeof(handpress_calibrate_data) : sizeof(hp_offset));

	if (send_calibrate_data_to_mcu(TAG_HANDPRESS, SUB_CMD_SET_OFFSET_REQ, hp_offset, HANDPRESS_CALIDATA_NV_SIZE, false))
		return -1;

	return 0;
}

int send_airpress_calibrate_data_to_mcu(void)
{
	if (read_calibrate_data_from_nv(AIRPRESS_CALIDATA_NV_NUM, AIRPRESS_CALIDATA_NV_SIZE, "AIRDATA"))
		return -1;

	/*send to mcu*/
	memcpy(&airpress_data.offset, user_info.nv_data, AIRPRESS_CALIDATA_NV_SIZE);
	hwlog_info("airpress offset data=%d\n",airpress_data.offset);

	if (send_calibrate_data_to_mcu(TAG_PRESSURE, SUB_CMD_SET_OFFSET_REQ, &airpress_data.offset, AIRPRESS_CALIDATA_NV_SIZE, false))
		return -1;

	return 0;
}

int send_cap_prox_calibrate_data_to_mcu(void)
{
	if (strlen(sensor_chip_info[CAP_PROX]) == 0){
		hwlog_info("is not overseas phone,cap_prox sensor is not in board\n");
		return 0;
	}

	if (read_calibrate_data_from_nv(CAP_PROX_CALIDATA_NV_NUM, CAP_PROX_CALIDATA_NV_SIZE, "Csensor"))
		return -1;

	//send to mcu
	memcpy(&sar_calibrate_datas, user_info.nv_data, sizeof(sar_calibrate_datas));
	if (!strncmp(sensor_chip_info[CAP_PROX], "huawei,cypress_sar_psoc4000", strlen("huawei,cypress_sar_psoc4000"))){
		hwlog_info("idac:%d, rawdata:%d length:%ld %ld\n", sar_calibrate_datas.cypres_cali_data.sar_idac,
				sar_calibrate_datas.cypres_cali_data.raw_data, sizeof(sar_calibrate_datas), sizeof(sar_calibrate_datas));
	} else if(!strncmp(sensor_chip_info[CAP_PROX], "huawei,semtech-sx9323", strlen("huawei,semtech-sx9323"))){
		hwlog_info("sx9323:offset=%d, diff=%d length:%ld %ld\n", sar_calibrate_datas.semtech_cali_data.offset,
				sar_calibrate_datas.semtech_cali_data.diff, sizeof(sar_calibrate_datas), sizeof(sar_calibrate_datas));
	} else {
		hwlog_info("CAP_PROX cal_offset[0],digi_offset[0]:%x,%x\n",
			sar_calibrate_datas.cap_cali_data.cal_offset[0],sar_calibrate_datas.cap_cali_data.digi_offset[0]);
		hwlog_info("CAP_PROX cal_fact_base[0],swap_flag[0]:%x,%x\n",
			sar_calibrate_datas.cap_cali_data.cal_fact_base[0], sar_calibrate_datas.cap_cali_data.swap_flag[0]);
	}
	if (send_calibrate_data_to_mcu(TAG_CAP_PROX, SUB_CMD_SET_OFFSET_REQ, (const void *)&sar_calibrate_datas, sizeof(sar_calibrate_datas), false))
		return -1;

	return 0;
}

static int send_add_data_to_mcu(int tag, uint32_t subcmd, const void *data, int length, bool is_recovery)
{
	int ret = 0;
	write_info_t pkg_ap;
	read_info_t	pkg_mcu;
	pkt_parameter_req_t cpkt;
	pkt_header_t *hd = (pkt_header_t *)&cpkt;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	memset(&cpkt, 0, sizeof(cpkt));

	pkg_ap.tag=tag;
	pkg_ap.cmd=CMD_CMN_CONFIG_REQ;
	cpkt.subcmd = subcmd;
	pkg_ap.wr_buf=&hd[1];
	pkg_ap.wr_len=length+SUBCMD_LEN;
	memcpy(cpkt.para, data, length);

	if(is_recovery)
		return write_customize_cmd(&pkg_ap, NULL, false);

	ret=write_customize_cmd(&pkg_ap, &pkg_mcu, false);
	if(ret) {
		hwlog_err("tag %d send_add_data_to_mcu() fail,ret=%d\n", tag, ret);
		return -1;
	}

	return 0;
}

extern struct adux_sar_add_data_t adux_sar_add_data;

int send_sar_add_data_to_mcu(void)
{
	if (strlen(sensor_chip_info[CAP_PROX]) == 0)
		return -1;

	//send to mcu
	if (!strncmp(sensor_chip_info[CAP_PROX], "huawei,cypress_sar_psoc4000", strlen("huawei,cypress_sar_psoc4000"))){
	} else if(!strncmp(sensor_chip_info[CAP_PROX], "huawei,semtech-sx9323", strlen("huawei,semtech-sx9323"))){
	} else if(!strncmp(sensor_chip_info[CAP_PROX], "huawei,adi-adux1050", strlen("huawei,adi-adux1050"))){
		hwlog_info("CAP_PROX adi1050 send_sar_add_data_to_mcu\n" );
		if (send_add_data_to_mcu(TAG_CAP_PROX, SUB_CMD_SET_ADD_DATA_REQ, (const void *)&adux_sar_add_data,
											sizeof(adux_sar_add_data), false)){
			return -1;
		}
	}
	return 0;
}

void reset_add_data(void)
{
	int ret = 0;

	ret = send_sar_add_data_to_mcu();
	if( ret ){
		hwlog_info( "reset_add_data() failed\n" );
	}else{
		hwlog_info( "reset_add_data() success\n" );
	}
}

void reset_calibrate_data(void)
{
	send_calibrate_data_to_mcu(TAG_ACCEL, SUB_CMD_SET_OFFSET_REQ, gsensor_calibrate_data, ACC_OFFSET_NV_SIZE, true);
	if (1 == akm_cal_algo) {
		send_calibrate_data_to_mcu(TAG_MAG, SUB_CMD_SET_OFFSET_REQ, msensor_akm_calibrate_data, MAG_AKM_CALIBRATE_DATA_NV_SIZE, true);
	} else {
		send_calibrate_data_to_mcu(TAG_MAG, SUB_CMD_SET_OFFSET_REQ, msensor_calibrate_data, MAG_CALIBRATE_DATA_NV_SIZE, true);
	}
	if (txc_ps_flag == 1 || ams_tmd2620_ps_flag == 1 || avago_apds9110_ps_flag == 1 || ams_tmd3725_ps_flag == 1
		|| liteon_ltr582_ps_flag == 1 || apds9999_ps_flag == 1 || ams_tmd3702_ps_flag == 1 || vishay_vcnl36658_ps_flag == 1) {
		send_calibrate_data_to_mcu(TAG_PS, SUB_CMD_SET_OFFSET_REQ, ps_sensor_calibrate_data, PS_CALIDATA_NV_SIZE, true);
	}
	if (ams_tof_flag == 1 || sharp_tof_flag == 1) {
		send_calibrate_data_to_mcu(TAG_TOF, SUB_CMD_SET_OFFSET_REQ, tof_sensor_calibrate_data, TOF_CALIDATA_NV_SIZE, true);
	}
	if (rohm_rgb_flag == 1 || avago_rgb_flag == 1 || ams_tmd3725_rgb_flag == 1 || liteon_ltr582_rgb_flag == 1 || is_cali_supported == 1
		|| apds9999_rgb_flag == 1 || ams_tmd3702_rgb_flag == 1 || apds9253_rgb_flag == 1|| vishay_vcnl36658_als_flag ==1
		|| tsl2591_flag == 1) {
		send_calibrate_data_to_mcu(TAG_ALS, SUB_CMD_SET_OFFSET_REQ, als_sensor_calibrate_data, ALS_CALIDATA_NV_SIZE, true);
	}
	if (strlen(sensor_chip_info[GYRO])) {
		send_calibrate_data_to_mcu(TAG_GYRO, SUB_CMD_SET_OFFSET_REQ, gyro_sensor_calibrate_data, GYRO_CALIDATA_NV_SIZE, true);
		send_calibrate_data_to_mcu(TAG_GYRO, SUB_CMD_GYRO_TMP_OFFSET_REQ, gyro_temperature_calibrate_data, GYRO_TEMP_CALI_NV_SIZE, true);
	}
	if (strlen(sensor_chip_info[AIRPRESS])) {
		hwlog_info("airpress offset reset data=%d\n",airpress_data.offset);
		send_calibrate_data_to_mcu(TAG_PRESSURE, SUB_CMD_SET_OFFSET_REQ, &airpress_data.offset, AIRPRESS_CALIDATA_NV_SIZE, true);
	}
	if (strlen(sensor_chip_info[HANDPRESS])) {
		send_calibrate_data_to_mcu(TAG_HANDPRESS, SUB_CMD_SET_OFFSET_REQ, handpress_calibrate_data, AIRPRESS_CALIDATA_NV_SIZE, true);
	}
	if (strlen(sensor_chip_info[CAP_PROX])) {
		send_calibrate_data_to_mcu(TAG_CAP_PROX, SUB_CMD_SET_OFFSET_REQ, &sar_calibrate_datas, sizeof(sar_calibrate_datas), true);
	}
}

int mcu_i2c_rw(uint8_t bus_num, uint8_t i2c_add, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct sensor_combo_cfg cfg;
	cfg.bus_type = TAG_I2C;
	cfg.bus_num = bus_num;
	cfg.i2c_address = i2c_add;

	ret = combo_bus_trans(&cfg, tx, tx_len, rx_out, rx_len);
	return ret < 0 ? -1 : 0;
}

int mcu_spi_rw(uint8_t bus_num, union SPI_CTRL ctrl, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	struct sensor_combo_cfg cfg;
	cfg.bus_type = TAG_SPI;
	cfg.bus_num = bus_num;
	cfg.ctrl = ctrl;

	ret = combo_bus_trans(&cfg, tx, tx_len, rx_out, rx_len);
	return ret < 0 ? -1 : 0;
}

int combo_bus_trans(struct sensor_combo_cfg *p_cfg, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len)
{
	int ret;
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	pkt_combo_bus_trans_req_t *pkt_combo_trans;
	uint32_t cmd_wd_len;

	if (!p_cfg) {
		hwlog_err("%s: p_cfg null\n", __func__);
		return -1;
	}

	memset((void *)&pkg_ap, 0, sizeof(pkg_ap));
	memset((void *)&pkg_mcu, 0, sizeof(pkg_mcu));

	pkg_ap.tag = p_cfg->bus_type;

	/* check and get bus type */
	if (pkg_ap.tag == TAG_I2C)
		pkg_ap.cmd            = CMD_I2C_TRANS_REQ;
	else if (pkg_ap.tag == TAG_SPI)
		pkg_ap.cmd            = CMD_SPI_TRANS_REQ;
	else {
		hwlog_err("%s: bus_type %d unknown\n", __func__, p_cfg->bus_type);
		return -1;
	}

	if (tx_len >= (uint32_t)0xFFFF - sizeof(*pkt_combo_trans)){
		hwlog_err("%s: tx_len %x too big\n", __func__, tx_len);
		return -1;
	}
	cmd_wd_len = tx_len + sizeof(*pkt_combo_trans);
	pkt_combo_trans = kzalloc((size_t)cmd_wd_len, GFP_KERNEL);
	if (!pkt_combo_trans) {
		hwlog_err("alloc failed in %s\n", __func__);
		return -1;
	}

	pkt_combo_trans->busid   = p_cfg->bus_num;
	pkt_combo_trans->ctrl    = p_cfg->ctrl;
	pkt_combo_trans->rx_len  = (uint16_t)rx_len;
	pkt_combo_trans->tx_len  = (uint16_t)tx_len;
	if (tx_len && tx) {
		memcpy((void *)pkt_combo_trans->tx, (void *)tx, (unsigned long)tx_len);
	}
	pkg_ap.wr_buf            = ((pkt_header_t*)pkt_combo_trans + 1);
	pkg_ap.wr_len            = (int)(cmd_wd_len - sizeof(pkt_combo_trans->hd));

	hwlog_info("%s: tag %d cmd %d data=%d, tx_len=%d,rx_len=%d\n",
		__func__, pkg_ap.tag, pkg_ap.cmd, p_cfg->data, tx_len, rx_len);

	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);
	if (ret) {
		hwlog_err("send cmd to mcu fail, data=%d, tx_len=%d,rx_len=%d\n", p_cfg->data, tx_len, rx_len);
		ret = -1;
	} else {
		if (pkg_mcu.errno != 0) {
			hwlog_err("mcu_rw fail, data=%d, tx_len=%d,rx_len=%d\n", p_cfg->data, tx_len, rx_len);
			ret = -1;
		} else {
			if (rx_out && rx_len) {
				memcpy((void *)rx_out, (void *)pkg_mcu.data, (unsigned long)rx_len);
			}
			ret = pkg_mcu.data_length;
		}
	}
	kfree(pkt_combo_trans);
	return ret;
}

int fill_extend_data_in_dts(struct device_node *dn, const char *name, unsigned char *dest, size_t max_size, int flag)
{
	int ret = 0;
	int buf[SENSOR_PLATFORM_EXTEND_DATA_SIZE] = { 0 };
	struct property *prop = NULL;
	unsigned int len = 0;
	unsigned int i = 0;
	int *pbuf = buf;
	unsigned char *pdest = dest;
	if (!dn || !name || !dest)
		return -1;

	if (0 == max_size)
		return -1;

	prop = of_find_property(dn, name, NULL);
	if (!prop)
		return -EINVAL;

	if (!prop->value)
		return -ENODATA;

	len = prop->length / 4;	/*len: word*/
	if (0 == len || len > max_size) {
		hwlog_err("In %s: len err! len = %d\n", __func__, len);
		return -1;
	}
	ret = of_property_read_u32_array(dn, name, buf, len);
	if (ret) {
		hwlog_err("In %s: read %s failed!\n", __func__, name);
		return -1;
	}
	if (EXTEND_DATA_TYPE_IN_DTS_BYTE == flag) {
		for (i = 0; i < len; i++) {
			*(dest + i) = (uint8_t) buf[i];
		}
	}
	if (EXTEND_DATA_TYPE_IN_DTS_HALF_WORD == flag) {
		for (i = 0; i < len; i++) {
			memcpy(pdest, pbuf, 2);
			pdest += 2;
			pbuf++;
		}
	}
	if (EXTEND_DATA_TYPE_IN_DTS_WORD == flag)
		memcpy(dest, buf, len * sizeof(int));

	return 0;
}

int send_para_flag = 0;
int set_pinhole_para_flag = 0;
int set_rpr531_para_flag = 0;
int set_tmd2745_para_flag = 0;
uint8_t sens_name = APDS9922;
static DEFINE_MUTEX(mutex_set_para);

static BLOCKING_NOTIFIER_HEAD(tp_notifier_list);
int tpmodule_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&tp_notifier_list, nb);
}
EXPORT_SYMBOL(tpmodule_register_client);

int tpmodule_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&tp_notifier_list, nb);
}
EXPORT_SYMBOL(tpmodule_unregister_client);

int tpmodule_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&tp_notifier_list, val, v);
}
EXPORT_SYMBOL(tpmodule_notifier_call_chain);

void set_tsl2591_als_extend_prameters(void)
{
	int tsl2591_als_para_table = 0;
	unsigned int i = 0;
	for(i=0; i<ARRAY_SIZE(tsl2591_als_para_diff_tp_color_table);i++)
	{
		if((tsl2591_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
			&& (tsl2591_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
			&&(( tsl2591_als_para_diff_tp_color_table[i].tp_manufacture == tp_manufacture)
				||(tsl2591_als_para_diff_tp_color_table[i].tp_manufacture == TS_PANEL_UNKNOWN))
			&&(tsl2591_als_para_diff_tp_color_table[i].tp_color == phone_color))
		{
			tsl2591_als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data,tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para,
		sizeof(tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para) >
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE?
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE:
		sizeof(tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para));

	minThreshold_als_para = tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para[TSL2591_MAX_ThRESHOLD_NUM];
	maxThreshold_als_para = tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para[TSL2591_MIN_ThRESHOLD_NUM];

	hwlog_info("A_als_para_tabel= %d\n",tsl2591_als_para_table);
	for( i=0; i<TSL2591_PARA_SIZE; i++){
		hwlog_info( "the A %d als paramater is the %d", i, tsl2591_als_para_diff_tp_color_table[tsl2591_als_para_table].tsl2591_para[i] );
	}
	hwlog_info("\n");
}

void set_bh1726_als_extend_prameters(void)
{
	int bh1726_als_para_table = 0;
	unsigned int i = 0;
	for(i=0; i<ARRAY_SIZE(bh1726_als_para_diff_tp_color_table);i++)
	{
		if((bh1726_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
			&& (bh1726_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
			&&(( bh1726_als_para_diff_tp_color_table[i].tp_manufacture == tp_manufacture)
				||(bh1726_als_para_diff_tp_color_table[i].tp_manufacture == TS_PANEL_UNKNOWN))
			&&(bh1726_als_para_diff_tp_color_table[i].tp_color == phone_color))
		{
			bh1726_als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data,bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para,
		sizeof(bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para) >
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE?
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE:
		sizeof(bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para));

	minThreshold_als_para = bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para[BH1726_MAX_ThRESHOLD_NUM];
	maxThreshold_als_para = bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para[BH1726_MIN_ThRESHOLD_NUM];

	hwlog_info("B_als_para_tabel= %d\n",bh1726_als_para_table);
	for( i=0; i<BH1726_PARA_SIZE; i++){
		hwlog_info( "the B %d als paramater is the %d", i, bh1726_als_para_diff_tp_color_table[bh1726_als_para_table].bh1726_para[i] );
	}
	hwlog_info("\n");
}

void set_rpr531_als_extend_prameters(void)
{
	int rpr531_als_para_table = 0;
	unsigned int i = 0;
	for(i=0; i<ARRAY_SIZE(rpr531_als_para_diff_tp_color_table);i++)
	{
		if((rpr531_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
			&& (rpr531_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
			&&(( rpr531_als_para_diff_tp_color_table[i].tp_manufacture == tp_manufacture)
				||(rpr531_als_para_diff_tp_color_table[i].tp_manufacture == TS_PANEL_UNKNOWN)))
		{
			rpr531_als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data,rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para,
		sizeof(rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para) >
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE?
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE:
		sizeof(rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para));

	minThreshold_als_para = rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para[RPR531_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para[RPR531_MAX_ThRESHOLD_NUM];

	hwlog_info ("rpr531_als_para_tabel= %d\n",rpr531_als_para_table);
    for( i=0; i<RPR531_PARA_SIZE; i++){
		hwlog_info( "the rpr531 %d als paramater is the %d", i, rpr531_als_para_diff_tp_color_table[rpr531_als_para_table].rpr531_para[i] );
	}
	hwlog_info("\n");
}

static void set_pinhole_als_extend_parameters(void)
{
	int i =0;
	for (i = 0; i < ARRAY_SIZE(pinhole_als_para_diff_tp_color_table); i++) {
		if ((pinhole_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (pinhole_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (pinhole_als_para_diff_tp_color_table[i].sens_name == sens_name)
		    && ((pinhole_als_para_diff_tp_color_table[i].tp_manufacture == tp_manufacture)
		    		||(pinhole_als_para_diff_tp_color_table[i].tp_manufacture == TS_PANEL_UNKNOWN))) {
			als_para_table = i;
			break;
		}
	}

	memcpy(als_data.als_extend_data, pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para,
	       sizeof(pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para) >
	       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
	       sizeof(pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para));
	minThreshold_als_para = pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para[LTR578_APDS9922_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para[LTR578_APDS9922_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d phone_type=%d, sens_name=%d, phone_version=%d\n",
		als_para_table, als_data.als_phone_type, sens_name, als_data.als_phone_version);
		for( i=0; i<pinhole_para_size; i++){
		hwlog_info( "the %d als paramater is the %d", i, pinhole_als_para_diff_tp_color_table[als_para_table].pinhole_para[i] );
	}
	hwlog_info("\n");
}

static void set_tmd2745_als_extend_parameters(void)
{
	unsigned int i;
	for (i = 0;	i < ARRAY_SIZE(tmd2745_als_para_diff_tp_color_table);i++) {
		if ((tmd2745_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
			&& (tmd2745_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
			&& (tmd2745_als_para_diff_tp_color_table[i].tp_manufacture == tp_manufacture)) {
			als_para_table = i;
			break;
		}
	}

	memcpy(als_data.als_extend_data, tmd2745_als_para_diff_tp_color_table[als_para_table].als_para,
		sizeof(tmd2745_als_para_diff_tp_color_table[als_para_table].als_para) > SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ?
		SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE : sizeof(tmd2745_als_para_diff_tp_color_table[als_para_table].als_para));

	minThreshold_als_para = tmd2745_als_para_diff_tp_color_table[als_para_table].als_para[TMD2745_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = tmd2745_als_para_diff_tp_color_table[als_para_table].als_para[TMD2745_MAX_ThRESHOLD_NUM];

	hwlog_err("als_para_table=%d, phone_type=%d, phone_version=%d\n",
		als_para_table, als_data.als_phone_type, als_data.als_phone_version);
        for( i=0; i<TMD2745_PARA_SIZE; i++){
		hwlog_info( "the ams %d als paramater is the %d", i, tmd2745_als_para_diff_tp_color_table[als_para_table].als_para[i] );
	}
	hwlog_info("\n");
}

static int read_tp_module_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	hwlog_info("%s, start!\n", __func__);
	if(action == TS_PANEL_UNKNOWN){
		return NOTIFY_OK;
	}
	mutex_lock(&mutex_set_para);
	tp_manufacture = action;
	if(1 == set_pinhole_para_flag){
		set_pinhole_als_extend_parameters();
	}else if(true == set_rpr531_para_flag){
		set_rpr531_als_extend_prameters();
	}else if(true == set_tmd2745_para_flag){
		set_tmd2745_als_extend_parameters();
	}else{
		hwlog_info("%s, not get sensor yet!\n", __func__);
		mutex_unlock(&mutex_set_para);
		return NOTIFY_OK;
	}
	mutex_unlock(&mutex_set_para);
	hwlog_info("%s, get tp module type = %d\n", __func__, tp_manufacture);
	if(1 == send_para_flag){
		resend_als_parameters_to_mcu();
	}
	return NOTIFY_OK;
}

static void select_rohm_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(als_para_diff_tp_color_table); i++) {
		if ((als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, als_para_diff_tp_color_table[als_para_table].bh745_para,
		       sizeof(als_para_diff_tp_color_table[als_para_table].bh745_para) > SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ?
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE : sizeof(als_para_diff_tp_color_table[als_para_table].bh745_para));
	minThreshold_als_para = als_para_diff_tp_color_table[als_para_table].bh745_para[BH1745_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = als_para_diff_tp_color_table[als_para_table].bh745_para[BH1745_MAX_ThRESHOLD_NUM];

	hwlog_info("als_para_tabel=%d ,bh1745 phone_color=0x%x  tplcd_manufacture=%d, phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, tplcd_manufacture, als_data.als_phone_type, als_data.als_phone_version);
}

static void select_avago_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(apds_als_para_diff_tp_color_table); i++) {
		if ((apds_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (apds_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (apds_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| apds_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (apds_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, apds_als_para_diff_tp_color_table[als_para_table].apds251_para,
		       sizeof(apds_als_para_diff_tp_color_table[als_para_table].apds251_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(apds_als_para_diff_tp_color_table[als_para_table].apds251_para));
	minThreshold_als_para = apds_als_para_diff_tp_color_table[als_para_table].apds251_para[APDS9251_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = apds_als_para_diff_tp_color_table[als_para_table].apds251_para[APDS9251_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d apds9251 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}

static void select_apds9999_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(apds9999_als_para_diff_tp_color_table); i++) {
		if ((apds9999_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (apds9999_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (apds9999_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| apds9999_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (apds9999_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, apds9999_als_para_diff_tp_color_table[als_para_table].apds9999_para,
		       sizeof(apds9999_als_para_diff_tp_color_table[als_para_table].apds9999_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(apds9999_als_para_diff_tp_color_table[als_para_table].apds9999_para));
	minThreshold_als_para = apds9999_als_para_diff_tp_color_table[als_para_table].apds9999_para[APDS9251_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = apds9999_als_para_diff_tp_color_table[als_para_table].apds9999_para[APDS9251_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d apds9251 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}


static void select_ams_tmd3725_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(tmd3725_als_para_diff_tp_color_table); i++) {
		if ((tmd3725_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (tmd3725_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (tmd3725_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| tmd3725_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (tmd3725_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, tmd3725_als_para_diff_tp_color_table[als_para_table].tmd3725_para,
		       sizeof(tmd3725_als_para_diff_tp_color_table[als_para_table].tmd3725_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(tmd3725_als_para_diff_tp_color_table[als_para_table].tmd3725_para));
	minThreshold_als_para = tmd3725_als_para_diff_tp_color_table[als_para_table].tmd3725_para[TMD3725_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = tmd3725_als_para_diff_tp_color_table[als_para_table].tmd3725_para[TMD3725_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d tmd3725 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}

static void select_ams_tmd3702_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(tmd3702_als_para_diff_tp_color_table); i++) {
		if ((tmd3702_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (tmd3702_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (tmd3702_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| tmd3702_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (tmd3702_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, tmd3702_als_para_diff_tp_color_table[als_para_table].tmd3702_para,
		       sizeof(tmd3702_als_para_diff_tp_color_table[als_para_table].tmd3702_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(tmd3702_als_para_diff_tp_color_table[als_para_table].tmd3702_para));
	minThreshold_als_para = tmd3702_als_para_diff_tp_color_table[als_para_table].tmd3702_para[TMD3702_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = tmd3702_als_para_diff_tp_color_table[als_para_table].tmd3702_para[TMD3702_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d tmd3702 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}

static void select_vishay_vcnl36658_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(vcnl36658_als_para_diff_tp_color_table); i++) {
		if ((vcnl36658_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (vcnl36658_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (vcnl36658_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| vcnl36658_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (vcnl36658_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, vcnl36658_als_para_diff_tp_color_table[als_para_table].vcnl36658_para,
		       sizeof(vcnl36658_als_para_diff_tp_color_table[als_para_table].vcnl36658_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(vcnl36658_als_para_diff_tp_color_table[als_para_table].vcnl36658_para));
	minThreshold_als_para = vcnl36658_als_para_diff_tp_color_table[als_para_table].vcnl36658_para[VCNL36658_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = vcnl36658_als_para_diff_tp_color_table[als_para_table].vcnl36658_para[VCNL36658_MAX_ThRESHOLD_NUM];
	hwlog_info("als_para_tabel=%d vcnl36658 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}

static void select_liteon_ltr582_als_data(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(ltr582_als_para_diff_tp_color_table); i++) {
		if ((ltr582_als_para_diff_tp_color_table[i].phone_type == als_data.als_phone_type)
		    && (ltr582_als_para_diff_tp_color_table[i].phone_version == als_data.als_phone_version)
		    && (ltr582_als_para_diff_tp_color_table[i].tp_lcd_manufacture == tplcd_manufacture
			|| ltr582_als_para_diff_tp_color_table[i].tp_lcd_manufacture == DEFAULT_TPLCD)
		    && (ltr582_als_para_diff_tp_color_table[i].tp_color == phone_color)) {
			als_para_table = i;
			break;
		}
	}
	memcpy(als_data.als_extend_data, ltr582_als_para_diff_tp_color_table[als_para_table].ltr582_para,
		       sizeof(ltr582_als_para_diff_tp_color_table[als_para_table].ltr582_para) >
		       SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE ? SENSOR_PLATFORM_EXTEND_ALS_DATA_SIZE :
		       sizeof(ltr582_als_para_diff_tp_color_table[als_para_table].ltr582_para));

	minThreshold_als_para = ltr582_als_para_diff_tp_color_table[als_para_table].ltr582_para[LTR582_MIN_ThRESHOLD_NUM];
	maxThreshold_als_para = ltr582_als_para_diff_tp_color_table[als_para_table].ltr582_para[LTR582_MAX_ThRESHOLD_NUM];

	hwlog_info("als_para_tabel=%d ltr582 phone_color=0x%x phone_type=%d,phone_version=%d\n",
		     als_para_table, phone_color, als_data.als_phone_type, als_data.als_phone_version);
}


void select_als_para(struct device_node *dn)
{
	int ret = 0;

	if (rohm_rgb_flag == 1) {
		select_rohm_als_data();
	} else if (avago_rgb_flag == 1 || apds9253_rgb_flag == 1) {
		select_avago_als_data();
	} else if (apds9999_rgb_flag == 1) {
		select_apds9999_als_data();
	}else if (ams_tmd3725_rgb_flag == 1) {
		select_ams_tmd3725_als_data();
	}else if (ams_tmd3702_rgb_flag == 1) {
		select_ams_tmd3702_als_data();
	}else if (liteon_ltr582_rgb_flag == 1) {
		select_liteon_ltr582_als_data();
	}else if (vishay_vcnl36658_als_flag ==1){
		select_vishay_vcnl36658_als_data();
	}  else if((apds9922_flag == 1)||(ltr578_flag == 1)){
		if(WAS == als_data.als_phone_type){
			tp_manufacture = tplcd_manufacture;
		}/*when the product is Was,the tp_manufacture is the same as tplcd_manufacture*/
		mutex_lock(&mutex_set_para);
		set_pinhole_als_extend_parameters();
		set_pinhole_para_flag = 1;
		mutex_unlock(&mutex_set_para);
	} else if(true == rpr531_flag){
		mutex_lock(&mutex_set_para);
		set_rpr531_als_extend_prameters();
		set_rpr531_para_flag = true;
		mutex_unlock(&mutex_set_para);
	} else if (tmd2745_flag == 1) {
		mutex_lock(&mutex_set_para);
		set_tmd2745_als_extend_parameters();
		set_tmd2745_para_flag = true;
		mutex_unlock(&mutex_set_para);
	} else if (tsl2591_flag == 1) {
		set_tsl2591_als_extend_prameters();
	} else if (bh1726_flag == 1) {
		set_bh1726_als_extend_prameters();
	}
	else {
		ret = fill_extend_data_in_dts(dn, "als_extend_data", als_data.als_extend_data, 12, EXTEND_DATA_TYPE_IN_DTS_HALF_WORD);
		if (ret)
			hwlog_err("als_extend_data:fill_extend_data_in_dts failed!\n");
	}
}

#ifdef CONFIG_HW_TOUCH_KEY
int huawei_set_key_backlight(void *param_t)
{
	int ret = 0;
	int key_brightness = 0;
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	pkt_parameter_req_t cpkt;
	pkt_header_t *hd = (pkt_header_t *)&cpkt;
	struct key_param_t *param = (struct key_param_t *)param_t;

	if (NULL == param || is_sensorhub_disabled()) {
		hwlog_err("param null or sensorhub is disabled.\n");
		return 0;
	}

	if (strlen(sensor_chip_info[KEY]) == 0) {
		hwlog_err("no key\n");
		return 0;
	}
	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));
	key_brightness = ((param->test_mode << 16) | param->brightness1 | (param->brightness2<<8));

	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.tag = TAG_KEY;
	cpkt.subcmd = SUB_CMD_BACKLIGHT_REQ;
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = sizeof(key_brightness)+SUBCMD_LEN;
	memcpy(cpkt.para, &key_brightness, sizeof(key_brightness));
	if (g_iom3_state == IOM3_ST_RECOVERY || iom3_power_state == ST_SLEEP) {
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	} else
		ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret < 0) {
		hwlog_err("err. write cmd\n");
		return -1;
	}

	if (0 != pkg_mcu.errno) {
		hwlog_info("mcu err \n");
		return -1;
	}

	return 0;

}
#else
int huawei_set_key_backlight(void *param_t)
{
	return 0;
}
#endif

static int light_sensor_update_fastboot_info(void)
{
	char *pstr, *dstr;
	char tmp[20];
	long tmp_code;
	int err;

	pstr = strstr(saved_command_line, "TP_COLOR=");
	if (!pstr) {
		pr_err("No fastboot TP_COLOR info\n");
		return -EINVAL;
	}
	pstr += strlen("TP_COLOR=");
	dstr = strstr(pstr, " ");
	if (!dstr) {
		pr_err("No find the TP_COLOR end\n");
		return -EINVAL;
	}
	memcpy(tmp, pstr, (unsigned long)(dstr - pstr));
	tmp[dstr - pstr] = '\0';
	err = kstrtol(tmp, 10, &tmp_code);
	if (err) {
		hwlog_info("light_sensor_update_fastboot_info: kstrtol failed, err: %d\n", err);
		return err;
	}
	return (int)tmp_code;
}

static int judge_tp_color_correct(u8 color)
{
	/*if the tp color is correct, after the 4~7 bit inversion shoud be same with 0~3 bit;*/
	return ((color & 0x0f) == ((~(color >> 4)) & 0x0f));
}

static struct notifier_block readtp_notify = {
	.notifier_call = read_tp_module_notify,
};

static struct notifier_block charger_notify = {
	.notifier_call = NULL,
};

void read_tp_color_cmdline(void)
{
	int tp_color = 0;
	tpmodule_register_client(&readtp_notify);
	tp_color = light_sensor_update_fastboot_info();
	if (judge_tp_color_correct(tp_color)) {
		phone_color = tp_color;
	} else {
		hwlog_err("light sensor LCD/TP ID error!\n");
		phone_color = WHITE;	/*WHITE is the default tp color*/
	}
	hwlog_err("light sensor read tp color is %d, 0x%x\n", tp_color, phone_color);
}

int mag_current_notify(void)
{
	int ret = 0;
	if(1 == mag_data.charger_trigger) {
		charger_notify.notifier_call = mag_enviroment_change_notify;
		ret = hisi_charger_type_notifier_register(&charger_notify);
		if (ret < 0)
			hwlog_err("mag_charger_type_notifier_register failed\n");
	}
	return ret;
}

