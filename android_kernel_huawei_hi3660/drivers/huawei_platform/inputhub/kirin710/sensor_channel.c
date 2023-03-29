/*
 *  drivers/misc/inputhub/sensorhub_channel.c
 *  Sensor Hub Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: qindiwen <qindiwen@huawei.com>
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <huawei_platform/inputhub/sensorhub.h>
#include <sensor_detect.h>
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "sensor_config.h"

int first_start_flag;
int ps_first_start_flag;
int txc_ps_flag;
int ams_tmd2620_ps_flag;
int avago_apds9110_ps_flag;
int als_first_start_flag;
int gyro_first_start_flag;
int handpress_first_start_flag;
int rohm_rgb_flag;
int avago_rgb_flag;
int is_cali_supported;
int  ams_tmd3725_rgb_flag;
int  ams_tmd3725_ps_flag;
int  liteon_ltr582_rgb_flag;
int  liteon_ltr582_ps_flag;
int ltr578_flag;
int apds9922_flag;
int rpr531_flag;
int tmd2745_flag;
int apds9999_rgb_flag;
int apds9999_ps_flag;
int ltr2568_ps_flag;
int  ams_tmd3702_rgb_flag;
int  ams_tmd3702_ps_flag;
int apds9253_rgb_flag;
int vishay_vcnl36658_als_flag;
int vishay_vcnl36658_ps_flag;
int ams_tof_flag;
int sharp_tof_flag;
int tsl2591_flag;
int bh1726_flag;
extern char sensor_chip_info[SENSOR_MAX][MAX_CHIP_INFO_LEN];
extern t_ap_sensor_ops_record all_ap_sensor_operations[TAG_SENSOR_END];
extern int flag_for_sensor_test;
extern struct mutex mutex_vibrator;

extern int send_app_config_cmd(int tag, void *app_config, bool use_lock);

static bool sensor_switch_flags[TAG_SENSOR_END] = {false,};
uint8_t tag_to_hal_sensor_type[TAG_SENSOR_END];
static uint8_t hal_sensor_type_to_tag[SENSORHUB_TYPE_END];
static DEFINE_MUTEX(mutex_status);

struct sensors_cmd_map {
	int hal_sensor_type;
	int tag;
};

static const struct sensors_cmd_map sensors_cmd_map_tab[] = {
	{SENSORHUB_TYPE_ACCELEROMETER, TAG_ACCEL},
	{SENSORHUB_TYPE_LIGHT, TAG_ALS},
	{SENSORHUB_TYPE_PROXIMITY, TAG_PS},
	{SENSORHUB_TYPE_GYROSCOPE, TAG_GYRO},
	{SENSORHUB_TYPE_GRAVITY, TAG_GRAVITY},
	{SENSORHUB_TYPE_MAGNETIC, TAG_MAG},
	{SENSORHUB_TYPE_LINEARACCELERATE, TAG_LINEAR_ACCEL},
	{SENSORHUB_TYPE_ORIENTATION, TAG_ORIENTATION},
	{SENSORHUB_TYPE_ROTATEVECTOR, TAG_ROTATION_VECTORS},
	{SENSORHUB_TYPE_PRESSURE, TAG_PRESSURE},
	{SENSORHUB_TYPE_HALL, TAG_HALL},
	{SENSORHUB_TYPE_MAGNETIC_FIELD_UNCALIBRATED, TAG_MAG_UNCALIBRATED},
	{SENSORHUB_TYPE_GAME_ROTATION_VECTOR, TAG_GAME_RV},
	{SENSORHUB_TYPE_GYROSCOPE_UNCALIBRATED, TAG_GYRO_UNCALIBRATED},
	{SENSORHUB_TYPE_SIGNIFICANT_MOTION, TAG_SIGNIFICANT_MOTION},
	{SENSORHUB_TYPE_STEP_DETECTOR, TAG_STEP_DETECTOR},
	{SENSORHUB_TYPE_STEP_COUNTER, TAG_STEP_COUNTER},
	{SENSORHUB_TYPE_GEOMAGNETIC_ROTATION_VECTOR, TAG_GEOMAGNETIC_RV},
	{SENSORHUB_TYPE_HANDPRESS, TAG_HANDPRESS},
	{SENSORHUB_TYPE_CAP_PROX, TAG_CAP_PROX},
	{SENSORHUB_TYPE_PHONECALL, TAG_PHONECALL},
	{SENSORHUB_TYPE_MAGN_BRACKET, TAG_MAGN_BRACKET},
	{SENSORHUB_TYPE_TILT_DETECTOR, TAG_TILT_DETECTOR},
	{SENSORHUB_TYPE_META_DATA, TAG_FLUSH_META},
	{SENSORHUB_TYPE_RPC, TAG_RPC},
	{SENSORHUB_TYPE_AGT, TAG_AGT},
	{SENSORHUB_TYPE_COLOR,TAG_COLOR},
	{SENSORHUB_TYPE_ACCELEROMETER_UNCALIBRATED, TAG_ACCEL_UNCALIBRATED},
};

static void init_hash_tables(void)
{
	int i;
	for (i = 0; i < sizeof(sensors_cmd_map_tab) / sizeof(sensors_cmd_map_tab[0]); ++i) {
		tag_to_hal_sensor_type[sensors_cmd_map_tab[i].tag] = sensors_cmd_map_tab[i].hal_sensor_type;
		hal_sensor_type_to_tag[sensors_cmd_map_tab[i].hal_sensor_type] = sensors_cmd_map_tab[i].tag;
	}
}

static bool switch_sensor(int tag, bool enable)
{
	bool result = false;
	mutex_lock(&mutex_status);
	if((enable && !sensor_switch_flags[tag]) || (!enable))
	{
		sensor_switch_flags[tag] = enable;
		result = true;
	}
	hwlog_info("%s ret is %d\n", __func__, result);
	mutex_unlock(&mutex_status);
	return result;
}

static bool ap_sensor_flush(int tag)
{
	struct sensor_data event;
	bool work_on_ap = all_ap_sensor_operations[tag].work_on_ap;

	if (work_on_ap) {
		if (all_ap_sensor_operations[tag].ops.enable) {/*leave this code for furture use*/
			all_ap_sensor_operations[tag].ops.enable(1);
			msleep(60);
		}
		event.type = tag_to_hal_sensor_type[TAG_FLUSH_META];
		event.length = 4;
		event.value[0] = tag_to_hal_sensor_type[tag];
		return inputhub_route_write_batch(ROUTE_SHB_PORT,
						  (char *)&event, event.length + OFFSET_OF_END_MEM(struct sensor_data, length), 0);
	}

	return work_on_ap;
}

static int send_sensor_batch_flush_cmd(unsigned int cmd, struct ioctl_para *para, int tag)
{
	write_info_t winfo;
	uint32_t subcmd = 0;

	memset(&winfo, 0, sizeof(winfo));

	if (SHB_IOCTL_APP_SENSOR_BATCH == cmd) {
		interval_param_t batch_param;
		batch_param.mode = AUTO_MODE;
		batch_param.period = para->period_ms;

		if(0 != para->period_ms){
			batch_param.batch_count = (para->timeout_ms > para->period_ms) ? (para->timeout_ms /para->period_ms) : 1;
		}else{
			batch_param.batch_count = 1;
			//hwlog_info("%s para->period_ms is zero, set count to 1\n");
		}

		hwlog_info("%s batch period=%d, count=%d\n", __func__, para->period_ms, batch_param.batch_count);
		return inputhub_sensor_setdelay(tag, &batch_param);
	} else if (SHB_IOCTL_APP_SENSOR_FLUSH == cmd) {
		hwlog_info("flush in %s \n", __func__);
		ap_sensor_flush(tag);
		winfo.cmd = CMD_CMN_CONFIG_REQ;
		winfo.tag = tag;
		subcmd = SUB_CMD_FLUSH_REQ;
		winfo.wr_buf = &subcmd;
		winfo.wr_len = SUBCMD_LEN;

		hwlog_info("flush sensor %s (tag:%d)!\n", obj_tag_str[tag] ? obj_tag_str[tag] : "TAG_UNKNOWN", tag);
		write_customize_cmd(&winfo, NULL, true);
	}
	return 0;
}

static int send_sensor_cmd(unsigned int cmd, unsigned long arg)
{
	uint8_t app_config[16] = { 0, };
	void __user *argp = (void __user *)arg;
	int tag = 0;
	int ret = 0;
	struct ioctl_para para;
	interval_param_t delay_param;

	if (flag_for_sensor_test)
		return 0;

	if (copy_from_user(&para, argp, sizeof(para)))
		return -EFAULT;

	delay_param.period = para.delay_ms;
	delay_param.batch_count = 1;
	delay_param.mode = AUTO_MODE;
	delay_param.reserved[0] = TYPE_STANDARD; /*for step counter only*/

	if (!(SENSORHUB_TYPE_BEGIN <= para.shbtype && para.shbtype < SENSORHUB_TYPE_END)) {
		hwlog_err("error shbtype %d in %s\n", para.shbtype, __func__);
		return -EINVAL;
	}

	tag = hal_sensor_type_to_tag[para.shbtype];
	switch (cmd) {
	case SHB_IOCTL_APP_ENABLE_SENSOR:
		break;
	case SHB_IOCTL_APP_DISABLE_SENSOR:
		switch_sensor(tag, false);
		if (tag == TAG_STEP_COUNTER)
			return inputhub_sensor_enable_stepcounter(false, TYPE_STANDARD);
		else
	        	return inputhub_sensor_enable(tag, false);
	case SHB_IOCTL_APP_DELAY_SENSOR:
		if(switch_sensor(tag, true)) {
			if (tag == TAG_STEP_COUNTER)
				ret = inputhub_sensor_enable_stepcounter(false, TYPE_STANDARD);
			else
				ret = inputhub_sensor_enable(tag, true);
			if (ret) {
				hwlog_err("failed to enable sensor tag %d\n", tag);
				return -1;
			}
		}
		if (tag == TAG_STEP_COUNTER) {
			inputhub_sensor_setdelay(tag, &delay_param);
			app_config[0] = 1;
			app_config[1] = TYPE_STANDARD;
			return send_app_config_cmd(TAG_STEP_COUNTER, app_config, true);
		} else {
			return inputhub_sensor_setdelay(tag, &delay_param);
		}
	case SHB_IOCTL_APP_SENSOR_BATCH:
		if(switch_sensor(tag, true) && inputhub_sensor_enable(tag, true))
		{
			hwlog_err("failed to enable sensor tag %d\n", tag);
			return -1;
		}
	case SHB_IOCTL_APP_SENSOR_FLUSH:
		if (tag == TAG_STEP_COUNTER) {
			ret = send_sensor_batch_flush_cmd(cmd, &para, tag);
			if (0 == ret) {
				/*success*/
				app_config[0] = 1;
				app_config[1] = TYPE_STANDARD;
				return send_app_config_cmd(TAG_STEP_COUNTER, app_config, true);
			} else {
				/*fail*/
				hwlog_err("send_sensor_cmd failed %d in %s %d!\n", cmd, __func__, __LINE__);
				return ret;
			}
		} else {
			return send_sensor_batch_flush_cmd(cmd, &para, tag);
		}
	default:
		hwlog_err("unknown shb_hal_cmd %d in %s!\n", cmd, __func__);
		return -EINVAL;
	}

	return 0;
}

/*******************************************************************************************
Function:       shb_read
Description:    定义/dev/sensorhub节点的读函数，从kernel的事件缓冲区中读数据
Data Accessed:  无
Data Updated:   无
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         无
Return:         实际读取数据的长度
*******************************************************************************************/
static ssize_t shb_read(struct file *file, char __user *buf, size_t count,
              loff_t *pos)
{
    return inputhub_route_read(ROUTE_SHB_PORT,buf, count);
}

static ssize_t shb_write(struct file *file, const char __user *data,
                        size_t len, loff_t *ppos)
{
    hwlog_info("%s need to do...\n", __func__);
    return len;
}
static void get_acc_calibrate_data(void)
{
    int ret = 0;
    if (0 == first_start_flag)
    {
        ret=send_gsensor_calibrate_data_to_mcu();
        if(ret) hwlog_err( "get_acc_calibrate_data read from nv fail, ret=%d", ret);
        else
        {
            hwlog_info("read nv success\n");
        }
    }
}

extern int send_vibrator_calibrate_data_to_mcu(void);
static void get_vibrator_calibrate_data(void)
{
    int ret = 0;
    static int vib_first_flag = 0;
    if(0 == strlen(sensor_chip_info[VIBRATOR])){
        return;
    }

    if (0 == vib_first_flag)
    {
        ret = send_vibrator_calibrate_data_to_mcu();
        if(ret) {
		hwlog_err( "%s read from nv fail, ret=%d", __func__, ret);
	}
        else
        {
		hwlog_info("%s read nv success\n", __func__);
        }
	vib_first_flag = 1;
    }
}

extern int send_mag_calibrate_data_to_mcu(void);
static void get_mag_calibrate_data(void)
{
    int ret = 0;
    static int mag_first_start_flag = 0;
    if (0 == mag_first_start_flag)
    {
        mag_current_notify();
        ret = send_mag_calibrate_data_to_mcu();
        if (ret) {
            hwlog_err("%s read mag data from nv or send to iom3 failed, ret=%d\n", __func__, ret);
        } else {
            hwlog_info("%s read mag data from nv and send to iom3 success\n", __func__);
        }
        mag_first_start_flag = 1;
    }
}

extern int send_cap_prox_calibrate_data_to_mcu(void);
static void get_cap_prox_calibrate_data(void)
{
    int ret = 0;
    static int cap_prox_first_start_flag = 0;
    if (0 == cap_prox_first_start_flag)
    {
        ret = send_cap_prox_calibrate_data_to_mcu();
        if (ret) {
            hwlog_err("%s read cap_prox data from nv or send to iom3 failed, ret=%d\n", __func__, ret);
        } else {
            hwlog_info("%s read cap_prox data from nv and send to iom3 success\n", __func__);
        }
        cap_prox_first_start_flag = 1;
    }
}

static void get_airpress_calibrate_data(void)
{
     int ret = 0;
    static int airpress_first_start_flag = 0;
    if(0 == strlen(sensor_chip_info[AIRPRESS])){
        return;
    }
    if (0 == airpress_first_start_flag)
    {
        ret = send_airpress_calibrate_data_to_mcu();
        if (ret) {
            hwlog_err("%s read airpress data from nv or send to iom3 failed, ret=%d\n", __func__, ret);
        } else {
            hwlog_info("%s read airpress data from nv and send to iom3 success\n", __func__);
            airpress_first_start_flag = 1;
        }
    }
}

extern int send_ps_calibrate_data_to_mcu(void);
extern int send_tof_calibrate_data_to_mcu(void);
static void get_psensor_calibrate_data(void)
{
    int ret = 0;
    if (0 == ps_first_start_flag &&  (txc_ps_flag == 1 || ams_tmd2620_ps_flag == 1 || avago_apds9110_ps_flag == 1 
		|| ams_tmd3725_ps_flag == 1 || liteon_ltr582_ps_flag == 1 || apds9999_ps_flag == 1 || ams_tmd3702_ps_flag == 1 || vishay_vcnl36658_ps_flag ==1 || ltr2568_ps_flag == 1))
    {
        ret=send_ps_calibrate_data_to_mcu();
        if(ret) hwlog_err( "get_ps_calibrate_data read from nv fail, ret=%d", ret);
        else
        {
            hwlog_info("read nv success\n");
        }
    }
    if(0 == ps_first_start_flag && (ams_tof_flag == 1 || sharp_tof_flag == 1)){
        ret=send_tof_calibrate_data_to_mcu();
        if(ret){
            hwlog_err( "get_tof_calibrate_data read from nv fail, ret=%d", ret);
        }
        else
        {
            hwlog_info("tof read nv success\n");
        }
    }
}

extern int send_als_calibrate_data_to_mcu(void);
static void get_als_calibrate_data(void)
{
    int ret = 0;
    if ((0 == als_first_start_flag) && ((rohm_rgb_flag == 1) || (avago_rgb_flag == 1) || (ams_tmd3725_rgb_flag == 1)
		|| (liteon_ltr582_rgb_flag == 1) || ( 1==is_cali_supported) || (apds9999_rgb_flag == 1) || (ams_tmd3702_rgb_flag == 1)
		|| (apds9253_rgb_flag == 1) || (vishay_vcnl36658_als_flag ==1) ))
    {
        ret=send_als_calibrate_data_to_mcu();
        if(ret) hwlog_err( "get_als_calibrate_data read from nv fail, ret=%d", ret);
        else
        {
            hwlog_info("read nv success\n");
        }
    }
}

extern int read_gyro_offset_from_nv(void);
extern int send_gyro_temperature_offset_to_mcu(void);
static void get_gyro_calibrate_data(void)
{
	int ret = 0;

	if(0 == strlen(sensor_chip_info[GYRO])) {
		return;
	}
	if(0 == gyro_first_start_flag)
	{
		ret=send_gyro_calibrate_data_to_mcu();
		if(ret)
			hwlog_err( "get_gyro_calibrate_data read from nv fail, ret=%d", ret);
		else
			hwlog_info("gyro_sensor read nv success\n");
		ret=send_gyro_temperature_offset_to_mcu();
		if(ret)
			hwlog_err( "get_gyro_temperature calibrate_data read from nv fail, ret=%d", ret);
		else
			hwlog_info("gyro read temperature offset from nv success\n");
	}
}

static void get_handpress_calibrate_data(void)
{
	int ret = 0;

	if(0 == strlen(sensor_chip_info[HANDPRESS])) {
		return;
	}
	if (0 == handpress_first_start_flag) {
		ret = send_handpress_calibrate_data_to_mcu();
		if(ret)
			hwlog_err( "handpress read from nv fail, ret=%d", ret);
		else
			hwlog_info("handpress read nv success\n");
    }
}

static void send_sensor_calib_data(void)
{
        get_acc_calibrate_data();
        get_mag_calibrate_data();
        get_cap_prox_calibrate_data();
        get_airpress_calibrate_data();
        get_psensor_calibrate_data();
        get_als_calibrate_data();
        get_gyro_calibrate_data();
	get_handpress_calibrate_data();
	get_vibrator_calibrate_data();
}

extern int send_sar_add_data_to_mcu(void);

static void send_sensor_add_data(void)//additional data
{
	int ret = 0;
	static int sar_first_start_flag = 0;
	if(0==sar_first_start_flag){
		ret = send_sar_add_data_to_mcu();
		if(ret){
			hwlog_err("%s send_sar_add_data_to_mcu failed, ret=%d\n", __func__, ret);
		} else {
			hwlog_info("%s send_sar_add_data_to_mcu success\n", __func__);
		}
		sar_first_start_flag = 1;
	}
}

/*******************************************************************************************
Function:       shb_ioctl
Description:    定义/dev/sensorhub节点的ioctl函数，主要用于设置传感器命令和获取MCU是否存在
Data Accessed:  无
Data Updated:   无
Input:          struct file *file, unsigned int cmd, unsigned long arg，cmd是命令码，arg是命令跟的参数
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static long shb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int sensorMcuMode;

	/* add sensors of android4.4(MAGNETIC_FIELD_UNCALIBRATED~GEOMAGNETIC_ROTATION_VECTOR) */
	switch (cmd) {
		case SHB_IOCTL_APP_ENABLE_SENSOR:
		case SHB_IOCTL_APP_DISABLE_SENSOR:
		case SHB_IOCTL_APP_DELAY_SENSOR:
			break;
		case SHB_IOCTL_APP_GET_SENSOR_MCU_MODE:
			sensorMcuMode = getSensorMcuMode();
			hwlog_info( "isSensorMcuMode [%d]\n", sensorMcuMode );
			if (copy_to_user(argp, &sensorMcuMode, sizeof(sensorMcuMode)))
	    			return -EFAULT;
			return 0;
		case SHB_IOCTL_APP_SENSOR_BATCH:
			hwlog_info("shb_ioctl  cmd : batch flush SHB_IOCTL_APP_SENSOR_BATCH\n");
			break;
		case SHB_IOCTL_APP_SENSOR_FLUSH:
			hwlog_info("shb_ioctl  cmd : batch flush SHB_IOCTL_APP_SENSOR_FLUSH\n");
			break;
		default:
			hwlog_err("shb_ioctl unknown cmd : %d\n", cmd);
			return -ENOTTY;
	}
	send_sensor_calib_data();
	send_sensor_add_data();//addinitial data
	return send_sensor_cmd(cmd, arg);
}

/*******************************************************************************************
Function:       shb_open
Description:    定义/dev/sensorhub节点的open函数，暂未实现实质功能
Data Accessed:  无
Data Updated:   无
Input:          struct inode *inode, struct file *file
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int shb_open(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok\n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       shb_release
Description:    定义/dev/sensorhub节点的release函数，暂未实现实质功能
Data Accessed:  无
Data Updated:   无
Input:          struct inode *inode, struct file *file
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int shb_release(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok\n", __func__);
	return 0;
}

static void enable_sensors_when_recovery_iom3(void)
{
	int tag;
	interval_param_t interval_param;

	for (tag = TAG_SENSOR_BEGIN; tag < TAG_SENSOR_END; ++tag) {
		if (sensor_status.status[tag]) {
			interval_param.period = sensor_status.delay[tag];	/*default delay_ms*/
			interval_param.batch_count = sensor_status.batch_cnt[tag];
			interval_param.mode = AUTO_MODE;
			interval_param.reserved[0] = TYPE_STANDARD;
			inputhub_sensor_enable_nolock(tag, true);
			inputhub_sensor_setdelay_nolock(tag,	&interval_param);
		}
	}
}

static int sensor_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
{
	hwlog_info("%s (%lu) +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		reset_calibrate_data();
		reset_add_data();
		enable_sensors_when_recovery_iom3();
		break;
	default:
		hwlog_err("%s -unknow state %ld\n", __func__, foo);
		break;
	}
	hwlog_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block sensor_recovery_notify = {
	.notifier_call = sensor_recovery_notifier,
	.priority = -1,
};

//--------------------------------------
static const struct file_operations shb_fops = {
    .owner      =   THIS_MODULE,
    .llseek     =   no_llseek,
    .read = shb_read,
    .write      =   shb_write,
    .unlocked_ioctl =   shb_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl =   shb_ioctl,
#endif
    .open       =   shb_open,
    .release    =   shb_release,
};

static struct miscdevice senorhub_miscdev = {
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "sensorhub",
    .fops =     &shb_fops,
};

/*******************************************************************************************
Function:       sensorhub_init
Description:    申请kernel缓冲区并初始化读写指针，注册msic设备
Data Accessed:  无
Data Updated:   无
Input:          无
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static int __init sensorhub_init(void)
{
	int ret;

	if(is_sensorhub_disabled())
		return -1;

	init_hash_tables();
	ret = inputhub_route_open(ROUTE_SHB_PORT);
	if (ret != 0) {
	    hwlog_err( "cannot open inputhub route err=%d\n", ret);
	    return ret;
	}

	ret = misc_register(&senorhub_miscdev);
	if (ret != 0) {
	    inputhub_route_close(ROUTE_SHB_PORT);
	    hwlog_err( "cannot register miscdev err=%d\n", ret);
	    return ret;
	}
	register_iom3_recovery_notifier(&sensor_recovery_notify);
	//hwlog_info("%s ok\n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       sensorhub_exit
Description:    释放kernel缓冲区并初始化读写指针，注销msic设备
Data Accessed:  无
Data Updated:   无
Input:          无
Output:         无
Return:         成功或者失败信息
*******************************************************************************************/
static void __exit sensorhub_exit(void)
{
    inputhub_route_close(ROUTE_SHB_PORT);
    misc_deregister(&senorhub_miscdev);
    hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(sensorhub_init);
module_exit(sensorhub_exit);

MODULE_AUTHOR("SensorHub <smartphone@huawei.com>");
MODULE_DESCRIPTION("SensorHub driver");
MODULE_LICENSE("GPL");
