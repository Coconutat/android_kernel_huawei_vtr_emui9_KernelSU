/************************************************************
*
* Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
* FileName: switch_fsa9685.c
* Author: lixiuna(00213837)       Version : 0.1      Date:  2013-11-06
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*  Description:    .c file for switch chip
*  Version:
*  Function List:
*  History:
*  <author>  <time>   <version >   <desc>
***********************************************************/

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <huawei_platform/usb/switch/switch_usb.h>
#include "switch_chip.h"
#include <linux/hisi/usb/hisi_usb.h>
#ifdef CONFIG_HDMI_K3
#include <../video/k3/hdmi/k3_hdmi.h>
#endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#include <linux/wakelock.h>
#include <huawei_platform/log/hw_log.h>
#include <chipset_common/hwusb/hw_usb_rwswitch.h>
#include <huawei_platform/usb/switch/switch_fsa9685.h>
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#include <huawei_platform/power/direct_charger_power_supply.h>

extern unsigned int get_boot_into_recovery_flag(void);
static int fsa9685_is_support_scp(void);
#define HWLOG_TAG switch_fsa9685
HWLOG_REGIST();

static struct fsa9685_device_info *g_fsa9685_dev = NULL;

static struct fsa9685_device_ops *g_fsa9685_dev_ops = NULL;

static int vendor_id;
static int gpio = -1;

static u32 scp_error_flag = 0;/*scp error flag*/
static int rt8979_osc_lower_bound = 0; /* lower bound for OSC setting */
static int rt8979_osc_upper_bound = 0; /* upper bound for OSC setting */
static int rt8979_osc_trim_code = 0; /* osc trimming setting (read from IC) */
static int rt8979_osc_trim_adjust = 0;	/* OSC adjustment */
static int rt8979_osc_trim_default = 0;	/* While attaching, reset OSC adjustment*/

static bool rt8979_dcd_timeout_enabled = false;

static inline bool is_rt8979(void)
{
    return (vendor_id == RT8979) ? true : false;
}
static bool rt8979_is_in_fm8(void);
static void rt8979_auto_restart_accp_det(void);
static void rt8979_force_restart_accp_det(bool open);
static int rt8979_sw_open(bool open);
static int rt8979_adjust_osc(int8_t val);

void fsa9685_accp_detect_lock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	mutex_lock(&di->accp_detect_lock);

	hwlog_info("accp_detect_lock lock\n");
}

void fsa9685_accp_detect_unlock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	mutex_unlock(&di->accp_detect_lock);

	hwlog_info("accp_detect_lock unlock\n");
}

void fsa9685_accp_adaptor_reg_lock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	mutex_lock(&di->accp_adaptor_reg_lock);

	hwlog_info("accp_adaptor_reg_lock lock\n");
}

void fsa9685_accp_adaptor_reg_unlock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	mutex_unlock(&di->accp_adaptor_reg_lock);

	hwlog_info("accp_adaptor_reg_lock unlock\n");
}

void fsa9685_usb_switch_wake_lock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	if (!wake_lock_active(&di->usb_switch_lock)) {
		wake_lock(&di->usb_switch_lock);
		hwlog_info("usb_switch_lock lock\n");
	}
}

void fsa9685_usb_switch_wake_unlock(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return;
	}

	if (wake_lock_active(&di->usb_switch_lock)) {
		wake_unlock(&di->usb_switch_lock);
		hwlog_info("usb_switch_lock unlock\n");
	}
}

static void rt8979_regs_dump(void);
int is_support_fcp(void);

static int fsa9685_write_reg(int reg, int val)
{
	int ret = -1;
	int i = 0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return ret;
	}

	for (i = 0; i < I2C_RETRY && ret < 0; i++) {
		ret = i2c_smbus_write_byte_data(di->client, reg, val);

		if (ret < 0) {
			hwlog_err("error: i2c write failed(reg=%02x ret=%d)!\n", reg, ret);
			msleep(1);
		}
	}

	return ret;
}

static int fsa9685_read_reg(int reg)
{
	int ret = -1;
	int i = 0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return ret;
	}

	for (i = 0; i < I2C_RETRY && ret < 0; i++) {
		ret = i2c_smbus_read_byte_data(di->client, reg);
		if (ret < 0) {
			hwlog_err("error: i2c read failed(reg=%02x ret=%d)!\n", reg, ret);
			msleep(1);
		}
	}

	return ret;
}

static int fsa9685_write_reg_mask(int reg, int value, int mask)
{
	int ret = 0;
	int val = 0;

	val = fsa9685_read_reg(reg);
	if (val < 0) {
		return val;
	}

	val &= ~mask;
	val |= value & mask;
	ret = fsa9685_write_reg(reg, val);

	return ret;
}

int fsa9685_common_write_reg(int reg, int val)
{
	return fsa9685_write_reg(reg, val);
}

int fsa9685_common_read_reg(int reg)
{
	return fsa9685_read_reg(reg);
}

int fsa9685_common_write_reg_mask(int reg, int value, int mask)
{
	return fsa9685_write_reg_mask(reg, value, mask);
}

static int fsa9685_get_device_id(void)
{
	int id = 0;
	int vendor_id = 0;
	int version_id = 0;

	id = fsa9685_read_reg(FSA9685_REG_DEVICE_ID);
	if (id < 0) {
		hwlog_err("error: get_device_id read fail!\n");
		return -1;
	}

	vendor_id = (id & FSA9685_REG_DEVICE_ID_VENDOR_ID_MASK) >> FSA9685_REG_DEVICE_ID_VENDOR_ID_SHIFT;
	version_id = (id & FSA9685_REG_DEVICE_ID_VERSION_ID_MASK) >> FSA9685_REG_DEVICE_ID_VERSION_ID_SHIFT;

	hwlog_info("get_device_id [%x]=%x,%d,%d\n", FSA9685_REG_DEVICE_ID, id, vendor_id, version_id);

	if (vendor_id == FSA9685_VENDOR_ID) {
		if (version_id == FSA9683_VERSION_ID) {
			hwlog_info("find fsa9683\n");
			return USBSWITCH_ID_FSA9683;
		}
		else if (version_id == FSA9688_VERSION_ID) {
			hwlog_info("find fsa9688\n");
			return USBSWITCH_ID_FSA9688;
		}
		else if (version_id == FSA9688C_VERSION_ID) {
			hwlog_info("find fsa9688c\n");
			return USBSWITCH_ID_FSA9688C;
		}
		else {
			hwlog_err("error: use default id (fsa9685)!\n");
			return USBSWITCH_ID_FSA9685;
		}
	}
	else if (vendor_id == RT8979_VENDOR_ID) {
		if (version_id == RT8979_1_VERSION_ID) {
			hwlog_info("find rt8979 (first revision)\n");
			return USBSWITCH_ID_RT8979;
		}
		else if (version_id == RT8979_2_VERSION_ID) {
			hwlog_info("find rt8979 (second revision)\n");
			return USBSWITCH_ID_RT8979;
		}
		else {
			hwlog_err("error: use default id (rt8979)!\n");
			return USBSWITCH_ID_RT8979;
		}
	}
	else {
		hwlog_err("error: use default id (fsa9685)!\n");
		return USBSWITCH_ID_FSA9685;
	}
}

static int fsa9685_device_ops_register(struct fsa9685_device_ops* ops)
{
	if (ops != NULL) {
		g_fsa9685_dev_ops = ops;
		hwlog_info("fsa9685_device ops register ok\n");
	}
	else {
		hwlog_info("fsa9685_device ops register fail!\n");
		return -1;
	}

	return 0;
}

static void fsa9685_select_device_ops(int device_id)
{
	switch (device_id) {
		case USBSWITCH_ID_FSA9683:
		case USBSWITCH_ID_FSA9685:
		case USBSWITCH_ID_FSA9688:
		case USBSWITCH_ID_FSA9688C:
			fsa9685_device_ops_register(usbswitch_fsa9685_get_device_ops());
		break;

		case USBSWITCH_ID_RT8979:
			fsa9685_device_ops_register(usbswitch_rt8979_get_device_ops());
		break;

		default:
			fsa9685_device_ops_register(usbswitch_fsa9685_get_device_ops());

			hwlog_err("error: use default ops (fsa9685)!\n");
		break;
	}
}

static int fsa9685_manual_switch(int input_select)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	if ((NULL == ops) || (NULL == ops->manual_switch)) {
		hwlog_err("error: ops is null or manual_switch is null!\n");
		return -1;
	}

	hwlog_info("input_select=%d\n", input_select);

	/* Two switch not support USB2_ID */
	if (di->two_switch_flag && (FSA9685_USB2_ID_TO_IDBYPASS == input_select)) {
		return 0;
	}

	return ops->manual_switch(input_select);
}

static int fsa9685_manual_detach_work(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -ERR_NO_DEV;
	}

	schedule_delayed_work(&di->detach_delayed_work, msecs_to_jiffies(20));

	return 0;
}
static void fsa9685_detach_work(struct work_struct *work)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return;
	}

	if ((NULL == ops) || (NULL == ops->detach_work)) {
		hwlog_err("error: ops is null or detach_work is null!\n");
		return;
	}

	return ops->detach_work();
}

static int fsa9685_dcd_timeout(bool enable_flag)
{
	int reg_val = 0;
	struct fsa9685_device_info *di = g_fsa9685_dev;
	int ret;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return SET_DCDTOUT_FAIL;
	}
	enable_flag |= di->dcd_timeout_force_enable;
	if (!is_rt8979()) {
		reg_val = fsa9685_read_reg(FSA9685_REG_DEVICE_ID);
		/*we need 9688c 9683 except 9688 not support enable dcd time out */
		if(FSA9688_VERSION_ID == ((reg_val & FAS9685_VERSION_ID_BIT_MASK) >> FAS9685_VERSION_ID_BIT_SHIFT)){
			return SET_DCDTOUT_FAIL;
		}
		ret = fsa9685_write_reg_mask(FSA9685_REG_CONTROL2,enable_flag,FSA9685_DCD_TIME_OUT_MASK);
		if(ret < 0){
			hwlog_err("%s:write fsa9688c DCD enable_flag error!!!\n",__func__);
			return SET_DCDTOUT_FAIL;
		}
	} else { /* RT8979 */
		rt8979_dcd_timeout_enabled = enable_flag;
		if (enable_flag){
			ret = fsa9685_write_reg_mask(RT8979_REG_TIMING_SET_2, 0, RT8979_REG_TIMING_SET_2_DCDTIMEOUT);
			if(ret < 0){
				hwlog_err("%s:write fsa9688c DCD enable_flag error!!!\n",__func__);
				return SET_DCDTOUT_FAIL;
			}
		}
		else{
			ret = fsa9685_write_reg_mask(RT8979_REG_TIMING_SET_2, RT8979_REG_TIMING_SET_2_DCDTIMEOUT, RT8979_REG_TIMING_SET_2_DCDTIMEOUT);
			if(ret < 0){
				hwlog_err("%s:write fsa9688c DCD enable_flag error!!!\n",__func__);
				return SET_DCDTOUT_FAIL;
			}
		}
		ret = fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, FSA9685_DCD_TIME_OUT_MASK,FSA9685_DCD_TIME_OUT_MASK);
		if(ret < 0){
			hwlog_err("%s:write fsa9688c DCD enable_flag error!!!\n",__func__);
			return SET_DCDTOUT_FAIL;
		}
	}
	hwlog_info("%s:write fsa9688c DCD enable_flag is:%d!!!\n",__func__,enable_flag);
	return SET_DCDTOUT_SUCC;
}

int fsa9685_dcd_timeout_status(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return SET_DCDTOUT_FAIL;
	}

	return di->dcd_timeout_force_enable;
}

static void fsa9685_intb_work(struct work_struct *work);
static irqreturn_t fsa9685_irq_handler(int irq, void *dev_id)
{
    int gpio_value;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return IRQ_HANDLED;
	}

#ifdef CONFIG_TCPC_CLASS
    if(di->pd_support) {
        //do nothing
    } else {
#endif
        fsa9685_usb_switch_wake_lock();
#ifdef CONFIG_TCPC_CLASS
    }
#endif

    gpio_value = gpio_get_value(gpio);
    if(gpio_value==1)
        hwlog_err("%s: intb high when interrupt occured!!!\n", __func__);

    schedule_work(&di->g_intb_work);

    hwlog_info("%s: ------end. gpio_value=%d\n", __func__, gpio_value);
    return IRQ_HANDLED;
}
/****************************************************************************
  Function:     is_fcp_charger_type
  Description:  after fcp detect ok,it will show it is fcp adapter
  Input:        void
  Output:       NA
  Return:        true:fcp adapter
                false: not fcp adapter
***************************************************************************/
int is_fcp_charger_type(void)
{
    int reg_val = 0;

    if(is_support_fcp())
    {
        return false;
    }

    reg_val = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);

    if(reg_val < 0)
    {
        hwlog_err("%s: read FSA9685_REG_DEVICE_TYPE_4 error!reg:%d\n", __func__,reg_val);
        return false;
    }
    if(reg_val & FSA9685_ACCP_CHARGER_DET)
    {
        return true ;
    }
    return false;
}
static enum hisi_charger_type fsa9685_get_charger_type(void)
{
    enum hisi_charger_type charger_type = CHARGER_TYPE_NONE;
    int val = 0, usb_status;
	int muic_status1;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return charger_type;
	}

    val = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
    if (val < 0)
    {
        hwlog_err("%s: read REG[%d] erro, val = %d, charger_type set to NONE\n",
                  __func__, FSA9685_REG_DEVICE_TYPE_1, val);
        return charger_type;
    }

	if (is_rt8979()) {
		muic_status1 = fsa9685_read_reg(RT8979_REG_MUIC_STATUS1); //Patrick Added, 2017/4/19
		usb_status = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
		if (usb_status != val)
			charger_type = CHARGER_TYPE_NONE;
		else if (muic_status1 & RT8979_DCDT)
			charger_type = CHARGER_TYPE_NONE;
		else if (val & FSA9685_USB_DETECTED)
			charger_type = CHARGER_TYPE_SDP;
		else if(val & FSA9685_CDP_DETECTED)
			charger_type = CHARGER_TYPE_CDP;
		else if(val & FSA9685_DCP_DETECTED)
			charger_type = CHARGER_TYPE_DCP;
		else
			charger_type = CHARGER_TYPE_NONE;
	} else {
		if (val & FSA9685_USB_DETECTED)
			charger_type = CHARGER_TYPE_SDP;
		else if(val & FSA9685_CDP_DETECTED)
			charger_type = CHARGER_TYPE_CDP;
		else if(val & FSA9685_DCP_DETECTED)
			charger_type = CHARGER_TYPE_DCP;
		else
			charger_type = CHARGER_TYPE_NONE;
	}

    if((charger_type == CHARGER_TYPE_NONE) && is_fcp_charger_type())
    {
       charger_type = CHARGER_TYPE_DCP;/*if is fcp ,report dcp,because when we detect fcp last time ,FSA9685_REG_DEVICE_TYPE_4 will be set */
       hwlog_info("%s:update charger type by device type4, charger type is:%d\n",__func__,charger_type);
    }

    return charger_type;
}

/****************************************************************************
  Function:     is_usb_id_abnormal
  Description:  check whether is usb id abnormal
  Input:        void
  Output:       NA
  Return:       1 -- abnormal ; 0 -- normal
***************************************************************************/
int fsa9685_is_water_intrused(void)
{
	u8 usb_id;
	int i;

	for (i = 0; i < 5; i++) {
		usb_id = fsa9685_read_reg(FSA9685_REG_ADC);
		usb_id &= FSA9685_REG_ADC_VALUE_MASK;
		hwlog_info("usb id is 0x%x\n", usb_id);
		if (!((usb_id >= WATER_CHECK_MIN_ID) && (usb_id <= WATER_CHECK_MAX_ID) && (usb_id != 0x0B)))
			return 0;
		msleep(100);
	}
	return 1;
}

static int rt8979_sw_open(bool open)
{
	int ret;

	hwlog_info("%s : %s\n", __func__, open ? "open" : "auto");
	return fsa9685_write_reg_mask(FSA9685_REG_CONTROL,
		open ? 0 : FSA9685_SWITCH_OPEN, FSA9685_SWITCH_OPEN);
}

static int rt8979_accp_enable(bool en)
{
	int ret;

	return fsa9685_write_reg_mask(FSA9685_REG_CONTROL2,
            en ? FSA9685_ACCP_AUTO_ENABLE : 0,
            FSA9685_ACCP_AUTO_ENABLE);
}

static void rt8979_auto_restart_accp_det(void)
{
	hwlog_info("%s-%d +++\n", __func__, __LINE__);
	if ((fsa9685_read_reg(FSA9685_REG_CONTROL2) & FSA9685_ACCP_AUTO_ENABLE) == 0) {
		hwlog_info("%s-%d do restart accp det\n", __func__, __LINE__);
		rt8979_sw_open(true);
		rt8979_accp_enable(true);
		msleep(30);
		fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, 0, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
		fsa9685_write_reg_mask(RT8979_REG_MUIC_CTRL, 0, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);
		fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, RT8979_REG_USBCHGEN_ACCPDET_STAGE1, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
		msleep(300);
		rt8979_sw_open(false);
		fsa9685_write_reg_mask(RT8979_REG_MUIC_CTRL, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);
	}
	hwlog_info("%s-%d ---\n", __func__, __LINE__);
}

static void rt8979_force_restart_accp_det(bool open)
{
	hwlog_info("%s-%d +++\n", __func__, __LINE__);
    rt8979_sw_open(true);
	rt8979_accp_enable(true);
    msleep(1);
    fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, 0, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
    fsa9685_write_reg_mask(RT8979_REG_MUIC_CTRL, 0, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);
    fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, RT8979_REG_USBCHGEN_ACCPDET_STAGE1, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
    msleep(300);
    rt8979_sw_open(open);
    fsa9685_write_reg_mask(RT8979_REG_MUIC_CTRL, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);
	hwlog_info("%s-%d ---\n", __func__, __LINE__);
}


static void rt8979_intb_work(struct work_struct *work)
{
    int reg_ctl, reg_intrpt, reg_adc, reg_dev_type1, reg_dev_type2, reg_dev_type3, vbus_status;
    int ret = -1;
    int ret2 = 0;
	int muic_status1, muic_status2;
    int id_valid_status = ID_VALID;
    int usb_switch_wakelock_flag = USB_SWITCH_NEED_WAKE_UNLOCK;
    static int invalid_times = 0;
    static int otg_attach = 0;
    static int pedestal_attach = 0;
	static bool redo_bc12 = false;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return;
	}

	reg_intrpt = fsa9685_read_reg(FSA9685_REG_INTERRUPT);
	vbus_status = fsa9685_read_reg(FSA9685_REG_VBUS_STATUS);
    hwlog_info("%s: read FSA9685_REG_INTERRUPT. reg_intrpt=0x%x\n", __func__, reg_intrpt);
	muic_status1 = fsa9685_read_reg(RT8979_REG_MUIC_STATUS1); //Patrick Added, 2017/4/19
	muic_status2 = fsa9685_read_reg(RT8979_REG_MUIC_STATUS2); //Patrick Added, 2017/4/25
	hwlog_info("%s: read MUIC STATUS1. reg_status=0x%x\n", __func__, muic_status1);
	if (muic_status1 & RT8979_DCDT) {
		hwlog_info("%s: DCD timoeut (DCT enable = %s)\n", __func__, rt8979_dcd_timeout_enabled ? "true" : "false");
		if (!rt8979_dcd_timeout_enabled) {
			hwlog_info("%s: DCD timoeut -> redo USB detection\n", __func__);
			rt8979_accp_enable(false);
			fsa9685_write_reg(FSA9685_REG_INTERRUPT_MASK, 0); // enable all interrupt, 2017/4/25
			fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, 0, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
			fsa9685_write_reg_mask(RT8979_REG_TIMING_SET_2, 0x38, 0x38);
			fsa9685_write_reg_mask(RT8979_REG_USBCHGEN, RT8979_REG_USBCHGEN_ACCPDET_STAGE1, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
			redo_bc12 = true;
			goto OUT;
		}
	} else
		fsa9685_write_reg_mask(FSA9685_REG_INTERRUPT_MASK, 1 << 5, 1 <<5); // Mask VBUS change
	muic_status2 >>= 4;
	muic_status2 &= 0x07;
	if (redo_bc12) {
		switch (muic_status2) {
			case 2: // SDP
				reg_intrpt |= FSA9685_ATTACH;
				break;
			case 5: // CDP
				reg_intrpt |= FSA9685_ATTACH;
				break;
			case 4: // DCP
				reg_intrpt |= FSA9685_ATTACH;
		}
		redo_bc12 = false;
	}
    if(!is_support_fcp()
        &&((RT8979_REG_ALLMASK != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1))
        ||(RT8979_REG_ALLMASK != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2))))
    {
        hwlog_info("disable fcp interrrupt again!!\n");
        ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, 0, FSA9685_ACCP_OSC_ENABLE);
        if (!is_rt8979())
                ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, di->dcd_timeout_force_enable,FSA9685_DCD_TIME_OUT_MASK);
        ret2 |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1, RT8979_REG_ALLMASK);
        ret2 |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2, RT8979_REG_ALLMASK);
        hwlog_info("%s : read ACCP interrupt,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,
            fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1), fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2));
        if(ret2 < 0)
        {
            hwlog_err("accp interrupt mask write failed \n");
        }
    }
    if (unlikely(reg_intrpt < 0)) {
        hwlog_err("%s: read FSA9685_REG_INTERRUPT error!!!\n", __func__);
    } else if (unlikely(reg_intrpt == 0)) {
        hwlog_err("%s: read FSA9685_REG_INTERRUPT, and no intr!!!\n", __func__);
    } else {
        if (reg_intrpt & FSA9685_DEVICE_CHANGE) {
            hwlog_info("%s: Device Change\n", __func__);
        }

        if ((reg_intrpt & FSA9685_ATTACH) && !rt8979_is_in_fm8()){
            hwlog_info("%s: FSA9685_ATTACH\n", __func__);
            rt8979_sw_open(false);
            fsa9685_write_reg(FSA9685_REG_ACCP_CMD, RT8979_REG_ACCP_CMD_STAGE1);
            fsa9685_write_reg(FSA9685_REG_ACCP_ADDR, RT8979_REG_ACCP_ADDR_VAL1);
            rt8979_osc_trim_adjust = rt8979_osc_trim_default;
            rt8979_adjust_osc(0);
            reg_dev_type1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
            reg_dev_type2 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_2);
            reg_dev_type3 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_3);
            hwlog_info("%s: reg_dev_type1=0x%X, reg_dev_type2=0x%X, reg_dev_type3= 0x%X\n", __func__,
                reg_dev_type1, reg_dev_type2, reg_dev_type3);
            if (reg_dev_type1 & FSA9685_FC_USB_DETECTED) {
                hwlog_info("%s: FSA9685_FC_USB_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_USB_DETECTED){
                hwlog_info("%s: FSA9685_USB_DETECTED\n", __func__);
                if (FSA9685_USB2_ID_TO_IDBYPASS == get_swstate_value()){
                    switch_usb2_access_through_ap();
                    hwlog_info("%s: fsa9685 switch to USB2 by setvalue\n", __func__);
                }
            }
            if (reg_dev_type1 & FSA9685_UART_DETECTED) {
                hwlog_info("%s: FSA9685_UART_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_MHL_DETECTED) {
                if (di->mhl_detect_disable == 1) {
                    hwlog_info("%s: mhl detection is not enabled on this platform, regard as an invalid detection\n", __func__);
                    id_valid_status = ID_INVALID;
                } else {
                    hwlog_info("%s: FSA9685_MHL_DETECTED\n", __func__);
                }
            }
            if (reg_dev_type1 & FSA9685_CDP_DETECTED) {
                hwlog_info("%s: FSA9685_CDP_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_DCP_DETECTED) {
                hwlog_info("%s: FSA9685_DCP_DETECTED\n", __func__);
                charge_type_dcp_detected_notify();
            }
            if ((reg_dev_type1 & FSA9685_USBOTG_DETECTED) && di->usbid_enable) {
                hwlog_info("%s: FSA9685_USBOTG_DETECTED\n", __func__);
                otg_attach = 1;
                usb_switch_wakelock_flag = USB_SWITCH_NEED_WAKE_LOCK;
                hisi_usb_id_change(ID_FALL_EVENT);
            }
            if (reg_dev_type1 & FSA9685_DEVICE_TYPE1_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                hwlog_info("%s: FSA9685_DEVICE_TYPE1_UNAVAILABLE_DETECTED\n", __func__);
            }
            if (reg_dev_type2 & FSA9685_JIG_UART) {
                hwlog_info("%s: FSA9685_JIG_UART\n", __func__);
            }
            if (reg_dev_type2 & FSA9685_DEVICE_TYPE2_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                hwlog_info("%s: FSA9685_DEVICE_TYPE2_UNAVAILABLE_DETECTED\n", __func__);
            }
            if (reg_dev_type3 & FSA9685_CUSTOMER_ACCESSORY7) {
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
                hwlog_info("%s:Enter FSA9685_CUSTOMER_ACCESSORY7\n", __func__);
            }
            if (reg_dev_type3 & FSA9685_CUSTOMER_ACCESSORY5) {
                hwlog_info("%s: FSA9685_CUSTOMER_ACCESSORY5, 365K\n", __func__);
                pedestal_attach = 1;
            }
            if (reg_dev_type3 & FSA9685_FM8_ACCESSORY) {
                hwlog_info("%s: FSA9685_FM8_DETECTED\n", __func__);
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
            }
            if (reg_dev_type3 & FSA9685_DEVICE_TYPE3_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                if (reg_intrpt & FSA9685_VBUS_CHANGE) {
                    usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
                }
                hwlog_info("%s: FSA9685_DEVICE_TYPE3_UNAVAILABLE_DETECTED\n", __func__);
            }
        }
        if (reg_intrpt & FSA9685_RESERVED_ATTACH) {
            id_valid_status = ID_INVALID;
            if (reg_intrpt & FSA9685_VBUS_CHANGE) {
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
            }
            hwlog_info("%s: FSA9685_RESERVED_ATTACH\n", __func__);
        }
        if (reg_intrpt & FSA9685_DETACH) {
            hwlog_info("%s: FSA9685_DETACH\n", __func__);
			//rt8979_sw_open(true);
			rt8979_accp_enable(true);
            reg_ctl = fsa9685_read_reg(FSA9685_REG_CONTROL);
            reg_dev_type2 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_2);
            hwlog_info("%s: reg_ctl=0x%x\n", __func__, reg_ctl);
            if (reg_ctl < 0){
                hwlog_err("%s: read FSA9685_REG_CONTROL error!!! reg_ctl=%d\n", __func__, reg_ctl);
                goto OUT;
            }
            if (0 == (reg_ctl & FSA9685_MANUAL_SW) && !rt8979_is_in_fm8())
            {
                reg_ctl |= FSA9685_MANUAL_SW;
                ret = fsa9685_write_reg(FSA9685_REG_CONTROL, reg_ctl);
                if (ret < 0) {
                    hwlog_err("%s: write FSA9685_REG_CONTROL error!!!\n", __func__);
                    goto OUT;
                }
            }
            if ((otg_attach == 1) && di->usbid_enable) {
                hwlog_info("%s: FSA9685_USBOTG_DETACH\n", __func__);
                hisi_usb_id_change(ID_RISE_EVENT);
                otg_attach = 0;
            }
			else {
				//hisi_usb_otg_bc_again(); // Added by Patrick
			}
            if (pedestal_attach ==1) {
                hwlog_info("%s: FSA9685_CUSTOMER_ACCESSORY5_DETACH\n", __func__);
                pedestal_attach = 0;
            }
            if (reg_dev_type2 & FSA9685_JIG_UART) {
                hwlog_info("%s: FSA9685_JIG_UART\n", __func__);
            }
        }
        if (reg_intrpt & FSA9685_VBUS_CHANGE) {
            hwlog_info("%s: FSA9685_VBUS_CHANGE\n", __func__);
        }
        if (reg_intrpt & FSA9685_ADC_CHANGE) {
            reg_adc = fsa9685_read_reg(FSA9685_REG_ADC);
            hwlog_info("%s: FSA9685_ADC_CHANGE. reg_adc=%d\n", __func__, reg_adc);
            if (reg_adc < 0) {
                hwlog_err("%s: read FSA9685_ADC_CHANGE error!!! reg_adc=%d\n", __func__, reg_adc);
            }
        }
    }
    if ((ID_INVALID == id_valid_status) &&
                (reg_intrpt & (FSA9685_ATTACH | FSA9685_RESERVED_ATTACH))) {
        invalid_times++;
        hwlog_info("%s: invalid time:%d reset fsa9685 work\n", __func__, invalid_times);
        if (invalid_times < MAX_DETECTION_TIMES) {
            hwlog_info("%s: start schedule delayed work\n", __func__);
            schedule_delayed_work(&di->detach_delayed_work, msecs_to_jiffies(0));
        } else {
            invalid_times = 0;
        }
    } else if ((ID_VALID == id_valid_status) &&
                (reg_intrpt & (FSA9685_ATTACH | FSA9685_RESERVED_ATTACH))) {
        invalid_times = 0;
    }
OUT:
#ifdef CONFIG_TCPC_CLASS
    if(di->pd_support) {
        //do nothing
    } else {
#endif
        if((USB_SWITCH_NEED_WAKE_UNLOCK == usb_switch_wakelock_flag) &&
            (0 == invalid_times)) {
            fsa9685_usb_switch_wake_unlock();
        }
#ifdef CONFIG_TCPC_CLASS
    }
#endif

    hwlog_info("%s: ------end.\n", __func__);
    return;
}

static void fsa9685_intb_work(struct work_struct *work)
{
    int reg_ctl, reg_intrpt, reg_adc, reg_dev_type1, reg_dev_type2, reg_dev_type3, vbus_status;
    int ret = -1;
    int ret2 = 0;
    int id_valid_status = ID_VALID;
    int usb_switch_wakelock_flag = USB_SWITCH_NEED_WAKE_UNLOCK;
    static int invalid_times = 0;
    static int otg_attach = 0;
    static int pedestal_attach = 0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return;
	}

    reg_intrpt = fsa9685_read_reg(FSA9685_REG_INTERRUPT);
    vbus_status = fsa9685_read_reg(FSA9685_REG_VBUS_STATUS);
    hwlog_info("%s: read FSA9685_REG_INTERRUPT. reg_intrpt=0x%x\n", __func__, reg_intrpt);
    /* if support fcp ,disable fcp interrupt */
    if(!is_support_fcp()
        &&((0xFF != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1))
        ||(0xFF != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2))))
    {
        hwlog_info("disable fcp interrrupt again!!\n");
        ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, FSA9685_ACCP_OSC_ENABLE,FSA9685_ACCP_OSC_ENABLE);
        ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, di->dcd_timeout_force_enable,FSA9685_DCD_TIME_OUT_MASK);
        ret2 |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1, 0xFF);
        ret2 |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2, 0xFF);
        hwlog_info("%s : read ACCP interrupt,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,
            fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1), fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2));
        if(ret2 < 0)
        {
            hwlog_err("accp interrupt mask write failed \n");
        }
    }

    if (unlikely(reg_intrpt < 0)) {
        hwlog_err("%s: read FSA9685_REG_INTERRUPT error!!!\n", __func__);
    } else if (unlikely(reg_intrpt == 0)) {
        hwlog_err("%s: read FSA9685_REG_INTERRUPT, and no intr!!!\n", __func__);
    } else {
        if (reg_intrpt & FSA9685_ATTACH){
            hwlog_info("%s: FSA9685_ATTACH\n", __func__);
            reg_dev_type1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
            reg_dev_type2 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_2);
            reg_dev_type3 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_3);
            hwlog_info("%s: reg_dev_type1=0x%X, reg_dev_type2=0x%X, reg_dev_type3= 0x%X\n", __func__,
                reg_dev_type1, reg_dev_type2, reg_dev_type3);
            if (reg_dev_type1 & FSA9685_FC_USB_DETECTED) {
                hwlog_info("%s: FSA9685_FC_USB_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_USB_DETECTED){
                hwlog_info("%s: FSA9685_USB_DETECTED\n", __func__);
                if (FSA9685_USB2_ID_TO_IDBYPASS == get_swstate_value()){
                    switch_usb2_access_through_ap();
                    hwlog_info("%s: fsa9685 switch to USB2 by setvalue\n", __func__);
                }
            }
            if (reg_dev_type1 & FSA9685_UART_DETECTED) {
                hwlog_info("%s: FSA9685_UART_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_MHL_DETECTED) {
                if (di->mhl_detect_disable == 1) {
                    hwlog_info("%s: mhl detection is not enabled on this platform, regard as an invalid detection\n", __func__);
                    id_valid_status = ID_INVALID;
                } else {
                    hwlog_info("%s: FSA9685_MHL_DETECTED\n", __func__);
                }
            }
            if (reg_dev_type1 & FSA9685_CDP_DETECTED) {
                hwlog_info("%s: FSA9685_CDP_DETECTED\n", __func__);
            }
            if (reg_dev_type1 & FSA9685_DCP_DETECTED) {
                hwlog_info("%s: FSA9685_DCP_DETECTED\n", __func__);
                charge_type_dcp_detected_notify();
            }
            if ((reg_dev_type1 & FSA9685_USBOTG_DETECTED) && di->usbid_enable) {
                hwlog_info("%s: FSA9685_USBOTG_DETECTED\n", __func__);
                otg_attach = 1;
                usb_switch_wakelock_flag = USB_SWITCH_NEED_WAKE_LOCK;
                hisi_usb_id_change(ID_FALL_EVENT);
            }
            if (reg_dev_type1 & FSA9685_DEVICE_TYPE1_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                hwlog_info("%s: FSA9685_DEVICE_TYPE1_UNAVAILABLE_DETECTED\n", __func__);
            }
            if (reg_dev_type2 & FSA9685_JIG_UART) {
                hwlog_info("%s: FSA9685_JIG_UART\n", __func__);
            }
            if (reg_dev_type2 & FSA9685_DEVICE_TYPE2_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                hwlog_info("%s: FSA9685_DEVICE_TYPE2_UNAVAILABLE_DETECTED\n", __func__);
            }
            if (reg_dev_type3 & FSA9685_CUSTOMER_ACCESSORY7) {
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
                hwlog_info("%s:Enter FSA9685_CUSTOMER_ACCESSORY7\n", __func__);
            }
            if (reg_dev_type3 & FSA9685_CUSTOMER_ACCESSORY5) {
                hwlog_info("%s: FSA9685_CUSTOMER_ACCESSORY5, 365K\n", __func__);
                pedestal_attach = 1;
            }
            if (reg_dev_type3 & FSA9685_FM8_ACCESSORY) {
                hwlog_info("%s: FSA9685_FM8_DETECTED\n", __func__);
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
            }
            if (reg_dev_type3 & FSA9685_DEVICE_TYPE3_UNAVAILABLE) {
                id_valid_status = ID_INVALID;
                if (reg_intrpt & FSA9685_VBUS_CHANGE) {
                    usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
                }
                hwlog_info("%s: FSA9685_DEVICE_TYPE3_UNAVAILABLE_DETECTED\n", __func__);
            }
        }

        if (reg_intrpt & FSA9685_RESERVED_ATTACH) {
            id_valid_status = ID_INVALID;
            if (reg_intrpt & FSA9685_VBUS_CHANGE) {
                usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
            }
            hwlog_info("%s: FSA9685_RESERVED_ATTACH\n", __func__);
        }

        if (reg_intrpt & FSA9685_DETACH) {
            hwlog_info("%s: FSA9685_DETACH\n", __func__);
            /* check control register, if manual switch, reset to auto switch */
            reg_ctl = fsa9685_read_reg(FSA9685_REG_CONTROL);
            reg_dev_type2 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_2);
            hwlog_info("%s: reg_ctl=0x%x\n", __func__, reg_ctl);
            if (reg_ctl < 0){
                hwlog_err("%s: read FSA9685_REG_CONTROL error!!! reg_ctl=%d\n", __func__, reg_ctl);
                goto OUT;
            }
            if (0 == (reg_ctl & FSA9685_MANUAL_SW))
            {
                reg_ctl |= FSA9685_MANUAL_SW;
                ret = fsa9685_write_reg(FSA9685_REG_CONTROL, reg_ctl);
                if (ret < 0) {
                    hwlog_err("%s: write FSA9685_REG_CONTROL error!!!\n", __func__);
                    goto OUT;
                }
            }
            if ((otg_attach == 1) && di->usbid_enable) {
                hwlog_info("%s: FSA9685_USBOTG_DETACH\n", __func__);
                hisi_usb_id_change(ID_RISE_EVENT);
                otg_attach = 0;
            }
            if (pedestal_attach ==1) {
                hwlog_info("%s: FSA9685_CUSTOMER_ACCESSORY5_DETACH\n", __func__);
                pedestal_attach = 0;
            }
            if (reg_dev_type2 & FSA9685_JIG_UART) {
                hwlog_info("%s: FSA9685_JIG_UART\n", __func__);
            }
        }
        if (reg_intrpt & FSA9685_VBUS_CHANGE) {
            hwlog_info("%s: FSA9685_VBUS_CHANGE\n", __func__);
        }
        if (reg_intrpt & FSA9685_ADC_CHANGE) {
            reg_adc = fsa9685_read_reg(FSA9685_REG_ADC);
            hwlog_info("%s: FSA9685_ADC_CHANGE. reg_adc=%d\n", __func__, reg_adc);
            if (reg_adc < 0) {
                hwlog_err("%s: read FSA9685_ADC_CHANGE error!!! reg_adc=%d\n", __func__, reg_adc);
            }
            /* do user specific handle */
        }
    }

    if ((ID_INVALID == id_valid_status) &&
                (reg_intrpt & (FSA9685_ATTACH | FSA9685_RESERVED_ATTACH))) {
        invalid_times++;
        hwlog_info("%s: invalid time:%d reset fsa9685 work\n", __func__, invalid_times);

        if (invalid_times < MAX_DETECTION_TIMES) {
            hwlog_info("%s: start schedule delayed work\n", __func__);
            schedule_delayed_work(&di->detach_delayed_work, msecs_to_jiffies(0));
        } else {
            invalid_times = 0;
        }
    } else if ((ID_VALID == id_valid_status) &&
                (reg_intrpt & (FSA9685_ATTACH | FSA9685_RESERVED_ATTACH))) {
        invalid_times = 0;
    }

OUT:
#ifdef CONFIG_TCPC_CLASS
    if(di->pd_support) {
        //do nothing
    } else {
#endif
        if((USB_SWITCH_NEED_WAKE_UNLOCK == usb_switch_wakelock_flag) &&
            (0 == invalid_times)) {
            fsa9685_usb_switch_wake_unlock();
        }
#ifdef CONFIG_TCPC_CLASS
    }
#endif

    hwlog_info("%s: ------end.\n", __func__);
    return;
}

static ssize_t fsa9685_dump_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;

	if ((NULL == ops) || (NULL == ops->dump_regs)) {
		hwlog_err("error: ops is null or dump_regs is null!\n");
		return -1;
	}

	return ops->dump_regs(buf);
}

static DEVICE_ATTR(dump_regs, S_IRUGO, fsa9685_dump_regs_show, NULL);

static ssize_t jigpin_ctrl_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;
	int jig_val = JIG_PULL_DEFAULT_DOWN;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	if ((NULL == ops) || (NULL == ops->jigpin_ctrl_store)) {
		hwlog_err("error: ops is null or jigpin_ctrl_store is null!\n");
		return -1;
	}

	if (sscanf(buf, "%d", &jig_val) != 1) {
		hwlog_err("error: unable to parse input:%s!\n", buf);
		return -1;
	}

	return ops->jigpin_ctrl_store(di->client, jig_val);
}

static ssize_t jigpin_ctrl_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;

	if ((NULL == ops) || (NULL == ops->jigpin_ctrl_show)) {
		hwlog_err("error: ops is null or jigpin_ctrl_show is null!\n");
		return -1;
	}

	return ops->jigpin_ctrl_show(buf);
}

static DEVICE_ATTR(jigpin_ctrl, S_IRUGO | S_IWUSR, jigpin_ctrl_show, jigpin_ctrl_store);

static ssize_t switchctrl_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;
	int action = 0;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	if ((NULL == ops) || (NULL == ops->switchctrl_store)) {
		hwlog_err("error: ops is null or switchctrl_store is null!\n");
		return -1;
	}

	if (sscanf(buf, "%d", &action) != 1) {
		hwlog_err("error: unable to parse input:%s!\n", buf);
		return -1;
	}

	return ops->switchctrl_store(di->client, action);
}

static bool rt8979_is_in_fm8(void)
{
    int id_value, device_type3, status1;
    bool retval;
    device_type3 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_3);
    id_value = fsa9685_read_reg(RT8979_REG_ADC);
    status1 = fsa9685_read_reg(RT8979_REG_MUIC_STATUS1);
    if (status1 & RT8979_REG_MUIC_STATUS1_FMEN)
        retval = true;
    else
        retval = false;
    return retval;
}
static ssize_t switchctrl_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fsa9685_device_ops *ops = g_fsa9685_dev_ops;

	if ((NULL == ops) || (NULL == ops->switchctrl_show)) {
		hwlog_err("error: ops is null or switchctrl_show is null!\n");
		return -1;
	}

	return ops->switchctrl_show(buf);
}

static DEVICE_ATTR(switchctrl, S_IRUGO | S_IWUSR, switchctrl_show, switchctrl_store);

/****************************************************************************
  Function:     fcp_adapter_reset
  Description:  reset adapter
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
int fcp_adapter_reset(void)
{
	int ret = 0,val = 0;
	if (is_rt8979()) {
		fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, 0,FAS9685_ACCP_CNTL_MASK);
		val = FSA9685_ACCP_MSTR_RST;
	}
	else
		val = FSA9685_ACCP_MSTR_RST | FAS9685_ACCP_SENDCMD |FSA9685_ACCP_IS_ENABLE;
	ret =  fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, val,FAS9685_ACCP_CNTL_MASK);
	hwlog_info("%s : send fcp adapter reset %s \n",__func__,ret < 0 ? "fail": "sucess");
	return ret;
}
/****************************************************************************
  Function:     fcp_stop_charge_config
  Description:  fcp stop charge config
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int fcp_stop_charge_config(void)
{
    int ret = 0;
#ifdef CONFIG_DIRECT_CHARGER
    if (!direct_charge_get_cutoff_normal_flag())
    {
#endif
#ifdef CONFIG_BOOST_5V
        ret = boost_5v_enable(DISABLE, BOOST_CTRL_FCP);
	ret |= direct_charge_set_bst_ctrl(DISABLE);
        if (ret)
        {
            hwlog_err("[%s]:5v boost close fail!\n", __func__);
            return BOOST_5V_CLOSE_FAIL;
        }
        hwlog_info("%s:5v boost close!\n", __func__);
#endif
#ifdef CONFIG_DIRECT_CHARGER
    }
#endif
    return 0;
}
/****************************************************************************
  Function:     switch_chip_reset
  Description:  reset fsa9688
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
int switch_chip_reset(void)
{
	//const int rt8979_reset_delay = 300;
    int ret = 0,reg_ctl = 0,gpio_value = 0;
    int slave_good, accp_status_mask;
	int reg_val1, reg_val2;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	hwlog_info("%s++", __func__);
	printk("caller is %pf/%pf\n", __builtin_return_address(1), __builtin_return_address(0)); // Only for debug
	if (is_rt8979())
		disable_irq(di->client->irq);
    ret = fsa9685_write_reg(0x19, 0x89);
    if(ret < 0)
    {
        hwlog_err("reset fsa9688 failed \n");
    }
    if (is_rt8979()) {
        msleep(1);
		rt8979_accp_enable(true);
        fsa9685_write_reg_mask(RT8979_REG_TIMING_SET_2, RT8979_REG_TIMING_SET_2_DCDTIMEOUT, RT8979_REG_TIMING_SET_2_DCDTIMEOUT);
        rt8979_dcd_timeout_enabled = false;
    }
    ret = fsa9685_write_reg(FSA9685_REG_DETACH_CONTROL, 1);
    if ( ret < 0 ){
        hwlog_err("%s: write FSA9685_REG_DETACH_CONTROL error!!! ret=%d", __func__, ret);
    }

    /* disable accp interrupt */
    ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, FSA9685_ACCP_OSC_ENABLE,FSA9685_ACCP_OSC_ENABLE);
    if (!is_rt8979())
        ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, di->dcd_timeout_force_enable,FSA9685_DCD_TIME_OUT_MASK);
    ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1, 0xFF);
    ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2, 0xFF);
    if(ret < 0)
    {
        hwlog_err("accp interrupt mask write failed \n");
    }
    /* clear INT MASK */
    reg_ctl = fsa9685_read_reg(FSA9685_REG_CONTROL);
    if ( reg_ctl < 0 ) {
        hwlog_err("%s: read FSA9685_REG_CONTROL error!!! reg_ctl=%d.\n", __func__, reg_ctl);
		if (is_rt8979())
			enable_irq(di->client->irq);
        return -1;
    }
    hwlog_info("%s: read FSA9685_REG_CONTROL. reg_ctl=0x%x.\n", __func__, reg_ctl);

    reg_ctl &= (~FSA9685_INT_MASK);
    ret = fsa9685_write_reg(FSA9685_REG_CONTROL, reg_ctl);
    if ( ret < 0 ) {
        hwlog_err("%s: write FSA9685_REG_CONTROL error!!! reg_ctl=%d.\n", __func__, reg_ctl);
		if (is_rt8979())
			enable_irq(di->client->irq);
        return -1;
    }
    hwlog_info("%s: write FSA9685_REG_CONTROL. reg_ctl=0x%x.\n", __func__, reg_ctl);

    ret = fsa9685_write_reg(FSA9685_REG_DCD, 0x0c);
    if ( ret < 0 ) {
        hwlog_err("%s: write FSA9685_REG_DCD error!!! reg_DCD=0x%x.\n", __func__, 0x08);
		if (is_rt8979())
			enable_irq(di->client->irq);
        return -1;
    }
    hwlog_info("%s: write FSA9685_REG_DCD. reg_DCD=0x%x.\n", __func__, 0x0c);

    gpio_value = gpio_get_value(gpio);
    hwlog_info("%s: intb=%d after clear MASK.\n", __func__, gpio_value);

    if (gpio_value == 0) {
        schedule_work(&di->g_intb_work);
    }
	if (is_rt8979())
		enable_irq(di->client->irq);
	hwlog_info("%s--", __func__);
    return 0;
}
/****************************************************************************
  Function:     fsa9685_fcp_cmd_transfer_check
  Description:  check cmd transfer success or fail
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
int fsa9685_fcp_cmd_transfer_check(void)
{
    int reg_val1 = 0,reg_val2 =0,i =0;
    /*read accp interrupt registers until value is not zero */
    do{
        usleep_range(30000, 31000);
        reg_val1 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2);
        i++;
    } while(i < MUIC_ACCP_INTREG_MAXREADCOUNT && reg_val1== 0 && reg_val2 == 0);

    if(reg_val1== 0 && reg_val2 == 0)
    {
        hwlog_info("%s : read accp interrupt time out,total time is %d ms\n",__func__,i*10);
    }
    if(reg_val1 < 0 || reg_val2 < 0 )
    {
        hwlog_err("%s: read  error!!! reg_val1=%d,reg_val2=%d \n", __func__, reg_val1,reg_val2);
        return -1;
    }

    /*if something  changed print reg info */
    if(reg_val2 & (FAS9685_PROSTAT | FAS9685_DSDVCSTAT) )
    {
        hwlog_info("%s : ACCP state changed  ,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,reg_val1,reg_val2);
    }

    /* judge if cmd transfer success */
    if((reg_val1 & FAS9685_ACK) &&(reg_val1 & FAS9685_CMDCPL)
        && !(reg_val1 & FAS9685_CRCPAR)
        && !(reg_val2 & (FAS9685_CRCRX | FAS9685_PARRX)))
    {
        return 0;
    }


    hwlog_err("%s : reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,reg_val1,reg_val2);
    return -1;
}

/****************************************************************************
  Function:     rt8979_fcp_cmd_transfer_check
  Description:  check cmd transfer success or fail
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
int rt8979_fcp_cmd_transfer_check(void)
{
    const int wait_time = RT8979_FCP_DM_TRANSFER_CHECK_WAIT_TIME;
    int reg_val1 = 0,reg_val2 =0,i = -1;
    int scp_status;
    /*read accp interrupt registers until value is not zero */

    scp_status = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if (scp_status == 0)
        return -1;
    do{
        usleep_range(30000,31000);
        reg_val1 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2);
        i++;
    } while(i < MUIC_ACCP_INTREG_MAXREADCOUNT && reg_val1== 0 && reg_val2 == 0);

    hwlog_info("%s : ACCP_INT = 0x%02x, 0x%02x\n", __func__, reg_val1, reg_val2);
    if(reg_val1== 0 && reg_val2 == 0)
    {
        hwlog_info("%s : read accp interrupt time out,total time is %d ms\n",__func__, wait_time);
    }
    if(reg_val1 < 0 || reg_val2 < 0 )
    {
        hwlog_err("%s: read  error!!! reg_val1=%d,reg_val2=%d \n", __func__, reg_val1,reg_val2);
        return -1;
    }

    /*if something  changed print reg info */
    if(reg_val2 & (FAS9685_PROSTAT | FAS9685_DSDVCSTAT) )
    {
        hwlog_info("%s : ACCP state changed  ,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,reg_val1,reg_val2);
    }

    if (reg_val2 == 0) {
        if (reg_val1 == FAS9685_CMDCPL) {
            // increase OSC
            hwlog_info("%s: increase OSC\n", __func__);
            if (rt8979_adjust_osc(-1) == 0) {
                return -RT8979_RETRY;
            }
        } else if (reg_val1	== (FAS9685_CMDCPL | FAS9685_CRCPAR)) {
            // decrease OSC
            hwlog_info("%s: decrease OSC\n", __func__);
            if (rt8979_adjust_osc(1) == 0) {
                return -RT8979_RETRY;
            }
        } else if (reg_val1	== (FAS9685_CMDCPL | FAS9685_ACK | FAS9685_CRCPAR)) {
            hwlog_info("%s: decrease OSC\n", __func__);
            if (rt8979_adjust_osc(1) == 0) {
                return -RT8979_FAIL;
            }
        }
    }

    hwlog_info("%s : RT8979 accp int1,2 = 0x%x, 0x%x\n", __func__, reg_val1, reg_val2);
    if ((reg_val1 & FAS9685_CMDCPL) && (reg_val1 & FAS9685_ACK)
        && !(reg_val1 & FAS9685_CRCPAR)
        && !(reg_val2 & (FAS9685_CRCRX | FAS9685_PARRX)))
        return 0;

    hwlog_err("%s : reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,reg_val1,reg_val2);
    return -1;
}


/****************************************************************************
  Function:     fcp_cmd_transfer_check
  Description:  check cmd transfer success or fail
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/

int fcp_cmd_transfer_check(void)
{
	return is_rt8979() ? 
		rt8979_fcp_cmd_transfer_check() : 
		fsa9685_fcp_cmd_transfer_check();
}

/****************************************************************************
  Function:     fcp_protocol_restart
  Description:  disable accp protocol and enable again
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
void fcp_protocol_restart(void)
{
    int reg_val =0;
    int ret = 0;
    int slave_good, accp_status_mask;
	
	
	
    if (is_rt8979()) {
        slave_good = RT8979_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask =  RT8979_ACCP_STATUS_MASK;
		rt8979_sw_open(true);
		rt8979_regs_dump();
		/* RT8979 didn't support fcp_protocol_restart */
		return;
    } else {
        slave_good = FSA9688_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask = FSA9688_ACCP_STATUS_MASK;
    }
    /* disable accp protocol */
    fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, 0,FAS9685_ACCP_CNTL_MASK);
	usleep_range(100000,101000);
    reg_val = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);

    if(slave_good == (reg_val & accp_status_mask))
    {
        hwlog_err("%s : disable accp enable bit failed ,accp status [0x40]=0x%x  \n",__func__,reg_val);
    }

    /* enable accp protocol */
    fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, FSA9685_ACCP_IS_ENABLE,FAS9685_ACCP_CNTL_MASK);
	usleep_range(100000,101000);
    reg_val = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if(slave_good != (reg_val & accp_status_mask))
    {
        hwlog_err("%s : enable accp enable bit failed, accp status [0x40]=0x%x  \n",__func__,reg_val);
    }
    /* disable accp interrupt */
    ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, FSA9685_ACCP_OSC_ENABLE,FSA9685_ACCP_OSC_ENABLE);
    if (is_rt8979()) {
        ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2,
            FSA9685_DCD_TIME_OUT_MASK,
            FSA9685_ACCP_ENABLE | FSA9685_DCD_TIME_OUT_MASK);
    }
    ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1, 0xFF);
    ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2, 0xFF);
    if(ret < 0)
    {
        hwlog_err("accp interrupt mask write failed \n");
    }
    hwlog_info("%s-%d :disable and enable accp protocol accp status  is 0x%x \n",__func__, __LINE__,reg_val);
}
/****************************************************************************
  Function:     accp_adapter_reg_read
  Description:  read adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
int accp_adapter_reg_read(int* val, int reg)
{
    int reg_val1 = 0,reg_val2 =0;
    int i=0,ret =0, adjust_osc_count = 0;
    int fcp_check_ret;

    hwlog_info("%s : reg = 0x%x\n", __func__, reg);
    fsa9685_accp_adaptor_reg_lock();
    for(i=0;i< FCP_RETRY_MAX_TIMES && adjust_osc_count < RT8979_ADJ_OSC_MAX_COUNT;i++)
    {
        /*before send cmd, read and clear accp interrupt registers */
        reg_val1 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2);

        ret |= fsa9685_write_reg(FSA9685_REG_ACCP_CMD, FSA9685_ACCP_CMD_SBRRD);
        ret |= fsa9685_write_reg(FSA9685_REG_ACCP_ADDR, reg);
        ret |= fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, FSA9685_ACCP_IS_ENABLE | FAS9685_ACCP_SENDCMD,FAS9685_ACCP_CNTL_MASK);
        if(ret)
        {
            hwlog_err("%s: write error ret is %d \n",__func__,ret);
            fsa9685_accp_adaptor_reg_unlock();
            return -1;
        }

        /* check cmd transfer success or fail */
        fcp_check_ret = fcp_cmd_transfer_check();
        if(0 == fcp_check_ret)
        {
            /* recived data from adapter */
            *val = fsa9685_read_reg(FSA9685_REG_ACCP_DATA);
            hwlog_info("%s-%d: reg = 0x%x, val = 0x%x\n", __func__, __LINE__, reg, *val);
            break;
        } else if (-RT8979_RETRY == fcp_check_ret) {
			adjust_osc_count++;
			i--; /* do retry, so decrease the retry count */
		}
        /* if transfer failed, restart accp protocol */
        fcp_protocol_restart();
        hwlog_err("%s : adapter register read fail times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,*val,reg_val1,reg_val2);
    }

    hwlog_debug("%s : adapter register retry times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,*val,reg_val1,reg_val2);
    if(FCP_RETRY_MAX_TIMES == i)
    {
        hwlog_err("%s : ack error,retry %d times \n",__func__,i);
        ret = -1;
    }
    else
    {
        ret = 0;
    }
    fsa9685_accp_adaptor_reg_unlock();
    return ret;
}

/****************************************************************************
  Function:     accp_adapter_reg_write
  Description:  write value into the adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
int accp_adapter_reg_write(int val, int reg)
{
    int reg_val1 = 0,reg_val2 =0;
    int i = 0,ret = 0, adjust_osc_count = 0;
    int fcp_check_ret;

    hwlog_info("%s : reg = 0x%x, val = 0x%x\n", __func__, reg, val);
    fsa9685_accp_adaptor_reg_lock();
    for(i=0;i< FCP_RETRY_MAX_TIMES && adjust_osc_count < RT8979_ADJ_OSC_MAX_COUNT;i++)
    {
        /*before send cmd, clear accp interrupt registers */
        reg_val1 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2);

        ret |=fsa9685_write_reg(FSA9685_REG_ACCP_CMD, FSA9685_ACCP_CMD_SBRWR);
        ret |=fsa9685_write_reg(FSA9685_REG_ACCP_ADDR, reg);
        ret |=fsa9685_write_reg(FSA9685_REG_ACCP_DATA, val);
        ret |=fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, FSA9685_ACCP_IS_ENABLE | FAS9685_ACCP_SENDCMD,FAS9685_ACCP_CNTL_MASK);
        hwlog_info("accp_adapter_reg_write: 0x%x=0x%x\r\n", reg, val);
        if(ret < 0)
        {
            hwlog_err("%s: write error ret is %d \n",__func__,ret);
            fsa9685_accp_adaptor_reg_unlock();
            return -1;
        }

        /* check cmd transfer success or fail */
	fcp_check_ret = fcp_cmd_transfer_check();
        if(0 == fcp_check_ret)
        {
            break;
        } else if (-RT8979_RETRY == fcp_check_ret) {
            adjust_osc_count++;
            i--; /* do retry, so decrease the retry count */
	}
        /* if transfer failed, restart accp protocol */
        fcp_protocol_restart();
        hwlog_err("%s : adapter register write fail times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,val,reg_val1,reg_val2);
    }
    hwlog_debug("%s : adapter register retry times=%d ,register=0x%x,data=0x%x,reg[0x59]=0x%x,reg[0x5A]=0x%x \n",__func__,i,reg,val,reg_val1,reg_val2);

    if(FCP_RETRY_MAX_TIMES == i)
    {
        hwlog_err("%s : ack error,retry %d times \n",__func__,i);
        ret = -1;
    }
    else
    {
        ret = 0;
    }
    fsa9685_accp_adaptor_reg_unlock();
    return ret;
}

int scp_adapter_reg_read(int* val, int reg)
{
    int ret;

    if (scp_error_flag)
    {
        hwlog_err("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
        return -1;
    }
    ret = accp_adapter_reg_read(val, reg);
    if (ret)
    {
        hwlog_err("%s : error reg = %d \n",__func__,reg);
        scp_error_flag = 1;
        return -1;
    }
    return 0;
}

int scp_adapter_reg_write(int val, int reg)
{
    int ret;

    if (scp_error_flag)
    {
        hwlog_err("%s : scp timeout happened ,do not write reg = %d \n",__func__,reg);
        return -1;
    }
    ret = accp_adapter_reg_write(val, reg);
    if (ret)
    {
        hwlog_err("%s : error reg = %d \n",__func__,reg);
        scp_error_flag = 1;
        return -1;
    }
    return 0;
}

static void rt8979_regs_dump(void)
{
    const int reg_addr[] = {
		FSA9685_REG_CONTROL,
        RT8979_REG_MUIC_EXT1,
        RT8979_REG_MUIC_STATUS1,
        RT8979_REG_MUIC_STATUS2,
        FSA9685_REG_ACCP_STATUS,
        RT8979_REG_MUIC_EXT2,
        FSA9685_REG_CONTROL2,	//0x0e
	FSA9685_REG_DEVICE_TYPE_1, //0x08
	FSA9685_REG_DEVICE_TYPE_2,	//0x09
	FSA9685_REG_DEVICE_TYPE_3,	//0x0a
	FSA9685_REG_DEVICE_TYPE_4,	//0x0f
    };
    int regval;
    int i;
    for (i = 0; i < ARRAY_SIZE(reg_addr); i++) {
        regval = fsa9685_read_reg(reg_addr[i]);
        hwlog_info("reg[0x%02x] = 0x%02x\n", reg_addr[i], regval);
    }
}
static int check_accp_ic_status(void)
{
	int check_times = 0;
	int reg_dev_type1 = 0;
	int ret = 0;
#ifdef CONFIG_BOOST_5V
	ret = boost_5v_enable(ENABLE, BOOST_CTRL_FCP);
	ret |= direct_charge_set_bst_ctrl(ENABLE);
	if (ret)
	{
		hwlog_err("[%s]:5v boost open fail!\n", __func__);
		return ACCP_NOT_PREPARE_OK;
	}
	hwlog_info("[%s]:5v boost open!\n", __func__);
#endif
	for (check_times = 0; check_times < ADAPTOR_BC12_TYPE_MAX_CHECK_TIME; check_times++)
	{
		reg_dev_type1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
		if (reg_dev_type1 & FSA9685_DCP_DETECTED)
		{
			hwlog_info("%s: FSA9685_DCP_DETECTED\n", __func__);
			break;
		}
		hwlog_info("[%s]reg_dev_type1 = 0x%x,check_times = %d!\n", __func__,reg_dev_type1,check_times);
		usleep_range(WAIT_FOR_BC12_DELAY*1000, (WAIT_FOR_BC12_DELAY+1)*1000);
	}
	hwlog_info("[%s]:accp is ok,check_times = %d!\n", __func__,check_times);
	return ACCP_PREPARE_OK;
}
/****************************************************************************
  Function:     acp_adapter_detect
  Description:  detect accp adapter
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
                1:fcp adapter but detect fail
***************************************************************************/
static int fsa9865_accp_adapter_detect(void)
{
    int reg_val1 = 0;
    int reg_val2 = 0;
    int i = 0;
    int slave_good, accp_status_mask;
    bool is_dcp = false;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return ACCP_ADAPTOR_DETECT_OTHER;
	}

    if (is_rt8979()) {
        slave_good = RT8979_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask =  RT8979_ACCP_STATUS_MASK;
        rt8979_regs_dump();
    } else {
        slave_good = FSA9688_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask = FSA9688_ACCP_STATUS_MASK;
    }

    if (ACCP_NOT_PREPARE_OK == check_accp_ic_status())
    {
        hwlog_err("check_accp_ic_status not prepare ok!\n");
        return ACCP_ADAPTOR_DETECT_OTHER;
    }
    fsa9685_accp_detect_lock();
    /*check accp status*/
    reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
    reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
        && (slave_good == (reg_val2 & accp_status_mask )))
    {
        hwlog_info("accp adapter detect ok.\n");
        fsa9685_accp_detect_unlock();
        return ACCP_ADAPTOR_DETECT_SUCC;
    }

    /* enable accp */
    reg_val1 = fsa9685_read_reg(FSA9685_REG_CONTROL2);
    reg_val1 |= FSA9685_ACCP_ENABLE;
    fsa9685_write_reg(FSA9685_REG_CONTROL2, reg_val1);

    hwlog_info("accp_adapter_detect, 0x0e=0x%x\r\n", reg_val1);

    /*detect hisi acp charger*/
    for(i=0; i < ACCP_DETECT_MAX_COUT; i++)
    {
        reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
        if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
            && (slave_good == (reg_val2 & accp_status_mask )))
        {
            break;
        }
        usleep_range(ACCP_POLL_TIME*1000, (ACCP_POLL_TIME+1)*1000);
    }
    /*clear accp interrupt*/
    hwlog_info("%s : read ACCP interrupt,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,
        fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1), fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2));
    if(ACCP_DETECT_MAX_COUT == i )
    {
        fsa9685_accp_detect_unlock();
        hwlog_info("not accp adapter ,reg[0x%x]=0x%x,reg[0x%x]=0x%x \n",FSA9685_REG_DEVICE_TYPE_4,reg_val1,FSA9685_REG_ACCP_STATUS,reg_val2);
        if(reg_val1 & FSA9685_ACCP_CHARGER_DET)
        {
            return ACCP_ADAPTOR_DETECT_FAIL;/*accp adapter but detect fail */
        }
        return ACCP_ADAPTOR_DETECT_OTHER;/*not fcp adapter*/

    }
    hwlog_info("accp adapter detect ok,take %d ms \n",i*ACCP_POLL_TIME);
    fsa9685_accp_detect_unlock();
    return ACCP_ADAPTOR_DETECT_SUCC;
}
static int rt8979_accp_adapter_detect(void)
{
    int reg_val1 = 0;
    int reg_val2 = 0;
    int i = 0, j = 0;
    int slave_good, accp_status_mask;
    bool is_dcp = false;
    bool vbus_present;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return ACCP_ADAPTOR_DETECT_OTHER;
	}

    if (ACCP_NOT_PREPARE_OK == check_accp_ic_status())
    {
        hwlog_err("check_accp_ic_status not prepare ok!\n");
        return ACCP_ADAPTOR_DETECT_OTHER;
    }
    slave_good = RT8979_ACCP_STATUS_SLAVE_GOOD;
    accp_status_mask =  RT8979_ACCP_STATUS_MASK;
    rt8979_regs_dump();
    fsa9685_accp_detect_lock();
    reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
    reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
        && (slave_good == (reg_val2 & accp_status_mask )))
    {
        hwlog_info("accp adapter detect ok.\n");
		rt8979_sw_open(true);
        fsa9685_accp_detect_unlock();
        return ACCP_ADAPTOR_DETECT_SUCC;
    }
	rt8979_auto_restart_accp_det();
	/*
    fsa9685_write_reg_mask(FSA9685_REG_ACCP_CNTL, 0,FAS9685_ACCP_CNTL_MASK);
    fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, 0,
        FSA9685_ACCP_AUTO_ENABLE | FSA9685_ACCP_ENABLE);
    fsa9685_write_reg_mask(FSA9685_REG_CONTROL2,
        FSA9685_ACCP_AUTO_ENABLE | FSA9685_ACCP_ENABLE,
        FSA9685_ACCP_AUTO_ENABLE | FSA9685_ACCP_ENABLE);
    reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
    hwlog_info("accp_adapter_detect, DEV_TYPE1=0x%x\r\n", reg_val1);
    if (reg_val1 > 0 && reg_val1 & FSA9685_DCP_DETECTED) {
        is_dcp = true;
        fsa9685_write_reg_mask(FSA9685_REG_CONTROL, FSA9685_SWITCH_OPEN, FSA9685_SWITCH_OPEN);
    }
    reg_val1 = fsa9685_read_reg(FSA9685_REG_CONTROL2);*/
    rt8979_regs_dump();
    vbus_present = (fsa9685_read_reg(RT8979_REG_MUIC_STATUS1) & RT8979_REG_MUIC_STATUS1_DCDT) ? false : true;
    for (j = 0; j < ACCP_MAX_TRYCOUNT && vbus_present; j++)
    {
        for(i=0; i < ACCP_DETECT_MAX_COUT; i++)
        {
            reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
            reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
            if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
                && (slave_good == (reg_val2 & accp_status_mask )))
            {
                 break;
            }
            usleep_range(ACCP_POLL_TIME*1000, (ACCP_POLL_TIME + 1)*1000);
        }
        hwlog_info("%s : read ACCP interrupt,reg[0x59]=0x%x,reg[0x5A]=0x%x, reg[0xA7]=0x%x\n",__func__,
        fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1), fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2),
        fsa9685_read_reg(RT8979_REG_MUIC_EXT2));
        if(ACCP_DETECT_MAX_COUT == i )
        {
            fsa9685_accp_detect_unlock();
            hwlog_info("not accp adapter ,reg[0x%x]=0x%x,reg[0x%x]=0x%x \n",FSA9685_REG_DEVICE_TYPE_4,reg_val1,FSA9685_REG_ACCP_STATUS,reg_val2);
            if(reg_val1 & FSA9685_ACCP_CHARGER_DET)
            {
                return ACCP_ADAPTOR_DETECT_FAIL;/*accp adapter but detect fail */
            }
            rt8979_regs_dump();
            vbus_present = (fsa9685_read_reg(RT8979_REG_MUIC_STATUS1) & RT8979_REG_MUIC_STATUS1_DCDT) ? false : true;
            if (vbus_present) {
		rt8979_force_restart_accp_det(true);
            }
        } else {
            hwlog_info("accp adapter detect ok,take %d ms \n",i*ACCP_POLL_TIME);
			rt8979_sw_open(true);
            fsa9685_accp_detect_unlock();
            return ACCP_ADAPTOR_DETECT_SUCC;
        }
    }
    return ACCP_ADAPTOR_DETECT_OTHER;/*not fcp adapter*/
}
int accp_adapter_detect(void)
{
    return is_rt8979() ? rt8979_accp_adapter_detect() :
        fsa9865_accp_adapter_detect();
}
static int fcp_adapter_detect(void)
{
    int ret;
#ifdef CONFIG_DIRECT_CHARGER
    int val;
#endif
    ret = accp_adapter_detect();
    if (ACCP_ADAPTOR_DETECT_OTHER == ret)
    {
        hwlog_info("fcp adapter other detect\n");
        return FCP_ADAPTER_DETECT_OTHER;
    }
    if (ACCP_ADAPTOR_DETECT_FAIL == ret)
    {
        hwlog_info("fcp adapter detect fail\n");
        return FCP_ADAPTER_DETECT_FAIL;
    }
#ifdef CONFIG_DIRECT_CHARGER
    if (fsa9685_is_support_scp())
    {
        return FCP_ADAPTER_DETECT_SUCC;
    }
    ret = accp_adapter_reg_read(&val, SCP_ADP_TYPE);
    if(ret)
    {
        hwlog_err("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
        return FCP_ADAPTER_DETECT_SUCC;
    }
    return FCP_ADAPTER_DETECT_OTHER;
#else
    return FCP_ADAPTER_DETECT_SUCC;
#endif
}
/**********************************************************
*  Function:        fsa9685_reg_dump
*  Discription:     dump register for charger dsm
*  Parameters:    ptr
*  return value:   void
**********************************************************/
#define DUMP_REG_NUM 21
#define DUMP_STR_LENTH 32

struct fsa9885_reg_dump_type
{
    unsigned char reg_add[DUMP_REG_NUM];
    unsigned char reg_val[DUMP_REG_NUM];
};

void fsa9685_reg_dump(char* ptr)
{
	const unsigned char reg_dump[DUMP_REG_NUM] = {0x01, 0x02, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x40, 0x41, 0x44, 0x47, 0x48, 0x5b, 0x5c};
	int i = 0;
	int val = 0;
	char buf[DUMP_STR_LENTH] = {0};

	struct fsa9885_reg_dump_type fsa9885_reg_dump;

	for(i = 0;i < sizeof(reg_dump)/sizeof(unsigned char);i++)
	{
		val = fsa9685_read_reg(reg_dump[i]);
		if (val < 0) {
			hwlog_err("%s: fsa9685_read_reg error!!!", __func__);
			return ;
		}
		fsa9885_reg_dump.reg_val[i] = val;
		fsa9885_reg_dump.reg_add[i] = reg_dump[i];
		val = 0;
	}

	snprintf(buf,sizeof(buf),"\n");
	strncat(ptr,buf,strlen(buf));
	memset(buf, 0, DUMP_STR_LENTH);

	for(i = 0;i < DUMP_REG_NUM;i++)
	{
		snprintf(buf,sizeof(buf),"reg[0x%2x]=0x%2x\n",fsa9885_reg_dump.reg_add[i],fsa9885_reg_dump.reg_val[i]);
		strncat(ptr,buf,strlen(buf));
		memset(buf, 0, DUMP_STR_LENTH);
	}
}
/****************************************************************************
  Function:     fcp_get_adapter_output_vol
  Description:  get fcp output vol
  Input:        NA.
  Output:       fcp output vol(5V/9V/12V)
  Return:        0: success
                -1: fail
***************************************************************************/
int fcp_get_adapter_output_vol(int *vol)
{
    int num = 0;
    int output_vol = 0;
    int ret =0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    /*get adapter vol list number,exclude 5V*/
    ret = accp_adapter_reg_read(&num, FCP_SLAVE_REG_DISCRETE_CAPABILITIES);
    /*currently,fcp only support three out vol config(5v/9v/12v)*/
    if (ret || num > 2 )
    {
        hwlog_err("%s: vout list support err, reg[0x21] = %d.\n", __func__, num);
        return -1;
    }

    /*get max out vol value*/
   ret = accp_adapter_reg_read(&output_vol, FCP_SLAVE_REG_DISCRETE_OUT_V(num));
    if(ret )
    {
        hwlog_err("%s: get max out vol value failed ,ouputvol=%d,num=%d.\n",__func__,output_vol,num);
        return -1;
    }
    *vol = output_vol;
    hwlog_info("%s: get adapter max out vol = %d,num= %d.\n", __func__, output_vol,num);
    return 0;
}


/****************************************************************************
  Function:     fcp_set_adapter_output_vol
  Description:  set fcp adapter output vol
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
int fcp_set_adapter_output_vol(int *output_vol)
{
    int val = 0;
    int vol = 0;
    int ret = 0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    /*read ID OUTI , for identify huawei adapter*/
    ret = accp_adapter_reg_read(&val, FCP_SLAVE_REG_ID_OUT0);
    if(ret < 0)
    {
        hwlog_err("%s: adapter ID OUTI read failed, ret is %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("%s: id out reg[0x4] = %d.\n", __func__, val);

    /*get adapter max output vol value*/
    ret = fcp_get_adapter_output_vol(&vol);
    if(ret <0)
    {
        hwlog_err("%s: fcp get adapter output vol err.\n", __func__);
        return -1;
    }

    /* PLK only support 5V/9V */
    if(vol > FCP_OUTPUT_VOL_9V * FCP_VOL_STEP)
    {
        vol = FCP_OUTPUT_VOL_9V * FCP_VOL_STEP;
    }
    *output_vol = vol/FCP_VOL_STEP;

    /*retry if write fail */
    ret |= accp_adapter_reg_write(vol, FCP_SLAVE_REG_VOUT_CONFIG);
    ret |= accp_adapter_reg_read(&val, FCP_SLAVE_REG_VOUT_CONFIG);
    hwlog_info("%s: vout config reg[0x2c] = %d.\n", __func__, val);
    if(ret <0 ||val != vol )
    {
        hwlog_err("%s:out vol config err, reg[0x2c] = %d.\n", __func__, val);
        return -1;
    }

    ret = accp_adapter_reg_write(FCP_SLAVE_SET_VOUT, FCP_SLAVE_REG_OUTPUT_CONTROL);
    if(ret < 0)
    {
        hwlog_err("%s : enable adapter output voltage failed \n ",__func__);
        return -1;
    }
    hwlog_info("fcp adapter output vol set ok.\n");
    return 0;
}

/****************************************************************************
  Function:     fcp_get_adapter_max_power
  Description:  get fcp adpter max power
  Input:        NA.
  Output:       NA
  Return:       MAX POWER(W)
***************************************************************************/
int fcp_get_adapter_max_power(int *max_power)
{
    int reg_val = 0;
    int ret =0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    /*read max power*/
    ret = accp_adapter_reg_read(&reg_val, FCP_SLAVE_REG_MAX_PWR);
    if(ret != 0)
    {
        hwlog_err("%s: read max power failed \n",__func__);
        return -1;
    }

    hwlog_info("%s: max power reg[0x22] = %d.\n", __func__, reg_val);
    *max_power = (reg_val >> 1);
    return 0;
}

/**********************************************************
*  Function:       fcp_get_adapter_output_current
*  Discription:    fcp get the output current from adapter max power and output vol
*  Parameters:     NA
*  return value:  input_current(MA)
**********************************************************/
int fcp_get_adapter_output_current(void)
{
    int output_current = 0;
    int output_vol = 0;
    int max_power = 0;
    int ret =0;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    ret |= fcp_get_adapter_output_vol(&output_vol);
    ret |= fcp_get_adapter_max_power(&max_power);
    if (ret != 0)
    {
        hwlog_err("%s : output current read failed \n",__func__);
        return -1;
    }
    output_current = max_power*1000/output_vol;
    hwlog_info("%s: output current = %d.\n", __func__, output_current);
    return output_current;
}

/**********************************************************
*  Function:       is_support_fcp
*  Discription:    check whether support fcp
*  Parameters:     NA
*  return value:   0:support
                  -1:not support
**********************************************************/
int is_support_fcp(void)
{
    int reg_val = 0;
    static int flag_result = -EINVAL;
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    if(!di->fcp_support)
    {
        return -1;
    }

    if(flag_result != -EINVAL)
    {
        return flag_result;
    }
    if (is_rt8979())
    {
        return 0;
    }
    reg_val = fsa9685_read_reg(FSA9685_REG_DEVICE_ID);
    if (FSA9688_VERSION_ID != ((reg_val & FAS9685_VERSION_ID_BIT_MASK) >> FAS9685_VERSION_ID_BIT_SHIFT)
        && FSA9688C_VERSION_ID != ((reg_val & FAS9685_VERSION_ID_BIT_MASK) >> FAS9685_VERSION_ID_BIT_SHIFT)){
        hwlog_err("%s:no fsa9688,no support fcp, reg[0x1]=%d.\n", __func__, reg_val);
        flag_result = -1;
    }
    else
    {
        flag_result = 0;
    }

    return flag_result;
}
/**********************************************************
*  Function:       fcp_switch_status_check
*  Discription:    when in fcp status ,it will check the reg status of 9685
*  Parameters:     NA
*  return value:   -1:status error 0:status ok
**********************************************************/
int fcp_read_switch_status(void)
{
    int reg_val1 = 0;
    int slave_good, accp_status_mask;
    /*check accp status*/
    if (is_rt8979()) {
        slave_good = RT8979_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask = RT8979_ACCP_STATUS_MASK;
    } else {
        slave_good = FSA9688_ACCP_STATUS_SLAVE_GOOD;
        accp_status_mask = FSA9688_ACCP_STATUS_MASK;
    }
    reg_val1 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if((slave_good != (reg_val1 & accp_status_mask )))
    {
         return -1;
    }
    return 0;
}
/**********************************************************
*  Function:       fcp_adapter_status_check
*  Discription:    when in fcp status ,it will check adapter reg status
*  Parameters:     NA
*  return value: 0:status ok ;FCP_ADAPTER_OTEMP:over temp;FCP_ADAPTER_OCURRENT: over current;FCP_ADAPTER_OVLT: over ovl;
**********************************************************/
int fcp_read_adapter_status (void)
{
    int val = 0,ret =0;
    ret = accp_adapter_reg_read(&val, FCP_ADAPTER_STATUS);
    if(ret !=0)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return 0;
    }
    hwlog_info("val is %d \n",val);

    if( FCP_ADAPTER_OVLT == (val & FCP_ADAPTER_OVLT))
    {
       return FCP_ADAPTER_OVLT;
    }

    if( FCP_ADAPTER_OCURRENT == (val & FCP_ADAPTER_OCURRENT))
    {
        return FCP_ADAPTER_OCURRENT;
    }

    if( FCP_ADAPTER_OTEMP == (val & FCP_ADAPTER_OTEMP))
    {
        return FCP_ADAPTER_OTEMP;
    }
    return 0;
}
#ifdef CONFIG_DIRECT_CHARGER
static int fsa9685_scp_adaptor_detect(void)
{
    int ret;
    int val;

    scp_error_flag = 0;
    ret = accp_adapter_detect();
    if (ACCP_ADAPTOR_DETECT_OTHER == ret)
    {
        hwlog_info("scp adapter other detect\n");
        return SCP_ADAPTOR_DETECT_OTHER;
    }
    if (ACCP_ADAPTOR_DETECT_FAIL == ret)
    {
        hwlog_info("scp adapter detect fail\n");
        return SCP_ADAPTOR_DETECT_FAIL;
    }
    ret = scp_adapter_reg_read(&val, SCP_ADP_TYPE);
    if(ret)
    {
        hwlog_err("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
        return SCP_ADAPTOR_DETECT_OTHER;
    }
    hwlog_info("%s : read SCP_ADP_TYPE val = %d \n",__func__,val);
    if ((val & SCP_ADP_TYPE_B_MASK) == SCP_ADP_TYPE_B)
    {
        hwlog_info("scp type B adapter detect\n ");
        ret = scp_adapter_reg_read(&val, SCP_B_ADP_TYPE);
        if (ret)
        {
            hwlog_err("%s : read SCP_B_ADP_TYPE fail ,ret = %d \n",__func__,ret);
            return SCP_ADAPTOR_DETECT_OTHER;/*not scp adapter*/
        }
        hwlog_info("%s : read SCP_B_ADP_TYPE val = %d \n",__func__,val);
        if (SCP_B_DIRECT_ADP == val)
        {
                hwlog_info("scp type B direct charge adapter detect\n ");
                return SCP_ADAPTOR_DETECT_SUCC;
        }
    }

    return SCP_ADAPTOR_DETECT_OTHER;

}
static int fsa9685_scp_output_mode_enable(int enable)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_OUTPUT_MODE_MASK);
    val |= enable ? SCP_OUTPUT_MODE_ENABLE:SCP_OUTPUT_MODE_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}

static int fsa9685_scp_adaptor_output_enable(int enable)
{
    int val;
    int ret;

    ret = fsa9685_scp_output_mode_enable(1);
    if(ret)
    {
        hwlog_err("%s : scp output mode enable failed ,ret = %d \n",__func__,ret);
        return -1;
    }

    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_OUTPUT_MASK);
    val |= enable ? SCP_OUTPUT_ENABLE:SCP_OUTPUT_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int fsa9685_adaptor_reset(int enable)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_ADAPTOR_RESET_MASK);
    val |= enable ? SCP_ADAPTOR_RESET_ENABLE:SCP_ADAPTOR_RESET_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}


static int  fsa9685_is_support_scp(void)
{
	struct fsa9685_device_info *di = g_fsa9685_dev;

	if ((NULL == di) || (NULL == di->client)) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

    if(!di->scp_support)
    {
        return -1;
    }
    return 0;
}
static int fsa9685_scp_config_iset_boundary(int iboundary)
{
    int val;
    int ret;

    /*high byte store in low address*/
    val = (iboundary >> 8) & 0xff;
    ret = scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_L);
    if (ret)
        return ret;
    /*low byte store in high address*/
    val = iboundary & 0xff;
    ret |= scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_H);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
    }
    return ret;

}
static int fsa9685_scp_config_vset_boundary(int vboundary)
{
    int val;
    int ret;

    /*high byte store in low address*/
    val = (vboundary >> 8) & 0xff;
    ret = scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_L);
    if (ret)
        return ret;
    /*low byte store in high address*/
    val = vboundary & 0xff;
    ret |= scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_H);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
    }
    return ret;

}
static int fsa9685_scp_set_adaptor_voltage(int vol)
{
    int val;
    int ret;

    val = vol - VSSET_OFFSET;
    val = val / VSSET_STEP;
    ret = scp_adapter_reg_write(val, SCP_VSSET);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int fsa9685_scp_set_watchdog_timer(int second)
{
    int val;
    int ret;

    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_WATCHDOG_MASK);
    val |= (second * 2) & SCP_WATCHDOG_MASK; /*1 bit means 0.5 second*/
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE1);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int fsa9685_scp_init(struct scp_init_data * sid)
{
    /*open 5v boost*/
    int ret;
    int val;

    scp_error_flag = 0;
    ret = fsa9685_scp_output_mode_enable(sid->scp_mode_enable);
    if(ret)
        return ret;
    ret = fsa9685_scp_config_vset_boundary(sid->vset_boundary);
    if(ret)
        return ret;
    ret = fsa9685_scp_config_iset_boundary(sid->iset_boundary);
    if(ret)
        return ret;
    ret = fsa9685_scp_set_adaptor_voltage(sid->init_adaptor_voltage);
    if(ret)
        return ret;
    ret = fsa9685_scp_set_watchdog_timer(sid->watchdog_timer);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
        return ret;
    hwlog_info("%s : CTRL_BYTE0 = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
    if(ret)
        return ret;
    hwlog_info("%s : CTRL_BYTE1 = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
    if(ret)
        return ret;
    hwlog_info("%s : STATUS_BYTE0 = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE1);
    if(ret)
        return ret;
    hwlog_info("%s : STATUS_BYTE1 = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_H);
    if(ret)
        return ret;
    hwlog_info("%s : VSET_BOUNDARY_H = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_L);
    if(ret)
        return ret;
    hwlog_info("%s : VSET_BOUNDARY_L = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_H);
    if(ret)
        return ret;
    hwlog_info("%s : ISET_BOUNDARY_H = 0x%x \n ",__func__, val);
    ret = scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_L);
    if(ret)
        return ret;
    hwlog_info("%s : ISET_BOUNDARY_L = 0x%x \n ",__func__, val);
    return ret;
}
static int fsa9685_scp_chip_reset(void)
{
    hwlog_err("%s\n",__func__);
    return switch_chip_reset();
}
static int fsa9685_scp_exit(struct direct_charge_device* di)
{
	int ret;
	ret = fsa9685_scp_output_mode_enable(0);
	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			ret  |= fsa9685_adaptor_reset(1);
			break;
		default:
			hwlog_info("%s:not iWatt\n",__func__);
	}
	usleep_range(50*1000, 51*1000);
	hwlog_err("%s\n",__func__);
#ifdef CONFIG_BOOST_5V
	boost_5v_enable(DISABLE, BOOST_CTRL_FCP);
	direct_charge_set_bst_ctrl(DISABLE);
#endif
	hwlog_info("%s:5v boost close!\n", __func__);
	scp_error_flag = 0;
	return ret;
}
static int fsa9685_scp_get_adaptor_voltage(void)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_SREAD_VOUT);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val*SCP_SREAD_VOUT_STEP + SCP_SREAD_VOUT_OFFSET;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}
static int fsa9685_scp_set_adaptor_current(int cur)
{
    int val;
    int ret;

    val = cur / ISSET_STEP;
    ret = scp_adapter_reg_write(val, SCP_ISSET);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int fsa9685_scp_get_adaptor_current(void)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_SREAD_IOUT);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val*SCP_SREAD_IOUT_STEP;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}
static int fsa9685_scp_get_adaptor_current_set(void)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_ISSET);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val*ISSET_STEP;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}

static int fsa9685_scp_get_adaptor_max_current(void)
{
    int val;
    int ret;
    int A;
    int B;
    int rs;

    ret = scp_adapter_reg_read(&val, SCP_MAX_IOUT);
    if(ret)
    {
        hwlog_err("%s : read MAX_IOUT failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]max_iout reg is %d \n", __func__, val);
    A = (SCP_MAX_IOUT_A_MASK & val) >> SCP_MAX_IOUT_A_SHIFT;
    B = SCP_MAX_IOUT_B_MASK & val;
    switch (A){
        case 0:
            A = 1;
	    break;
	case 1:
            A = 10;
	    break;
	case 2:
            A = 100;
	    break;
	case 3:
            A = 1000;
	    break;
	default:
	    return -1;
    }
    rs = B*A;
    hwlog_info("[%s]MAX IOUT initial is %d \n", __func__, rs);
    ret = scp_adapter_reg_read(&val, SCP_SSTS);
    if(ret)
    {
        hwlog_err("%s : read SSTS failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]ssts reg is %d \n", __func__, val);
    B = (SCP_SSTS_B_MASK & val) >> SCP_SSTS_B_SHIFT;
    A = SCP_SSTS_A_MASK & val;
    if (1 == B)
    {
        rs = rs * A / 8;
    }
    hwlog_info("[%s]MAX IOUT final is %d \n", __func__, rs);
    return rs;
}

static int fsa9685_scp_get_adaptor_temp(int* temp)
{
    int val = 0;
    int ret;

    ret = scp_adapter_reg_read(&val, SCP_INSIDE_TMP);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val is %d \n", __func__, val);
    *temp = val;

    return 0;
}
static int fsa9685_scp_cable_detect(void)
{
    int val;
    int ret;
    ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return SCP_CABLE_DETECT_ERROR;
    }
    hwlog_info("[%s]val is %d \n", __func__, val);
    if (val & SCP_CABLE_STS_MASK)
    {
        return SCP_CABLE_DETECT_SUCC;
    }
    return SCP_CABLE_DETECT_FAIL;
}
static int fsa9685_scp_adaptor_reset(void)
{
    return fcp_adapter_reset();
}
static int fsa9685_stop_charge_config(void)
{
    return 0;
}
static int fsa9685_is_scp_charger_type(void)
{
    return is_fcp_charger_type();
}
static int fsa9685_scp_get_adaptor_status(void)
{
    return 0;
}
static int fsa9685_scp_get_adaptor_info(void* info)
{
    int ret;
    struct adaptor_info* p = (struct adaptor_info*)info;

    ret = scp_adapter_reg_read(&(p->b_adp_type), SCP_B_ADP_TYPE);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->vendor_id_h), SCP_VENDOR_ID_H);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->vendor_id_l), SCP_VENDOR_ID_L);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->module_id_h), SCP_MODULE_ID_H);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->module_id_l), SCP_MODULE_ID_L);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->serrial_no_h), SCP_SERRIAL_NO_H);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->serrial_no_l), SCP_SERRIAL_NO_L);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->pchip_id), SCP_PCHIP_ID);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->hwver), SCP_HWVER);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->fwver_h), SCP_FWVER_H);
    if(ret)
        return ret;
    ret = scp_adapter_reg_read(&(p->fwver_l), SCP_FWVER_L);

    return ret;
}
static int fsa9685_get_adapter_vendor_id(void)
{
	int val = 0;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_PCHIP_ID);
	if(ret)
	{
		hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
		return -1;
	}
	hwlog_info("[%s]val is 0x%x \n", __func__, val);
	switch (val)
	{
		case VENDOR_ID_RICHTEK:
			hwlog_info("[%s]adapter is richtek \n", __func__);
			return RICHTEK_ADAPTER;
		case VENDOR_ID_IWATT:
			hwlog_info("[%s]adapter is iwatt \n", __func__);
			return IWATT_ADAPTER;
		default:
			hwlog_info("[%s]this adaptor vendor id is not found!\n", __func__);
			return val;
	}
}
static int fsa9685_get_usb_port_leakage_current_info(void)
{
	int val = 0;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
	if(ret)
	{
		hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
		return -1;
	}
	hwlog_info("[%s]val is 0x%x \n", __func__, val);
	val &= SCP_PORT_LEAKAGE_INFO;
	val = val>>SCP_PORT_LEAKAGE_SHIFT;
	hwlog_info("[%s]val is 0x%x \n", __func__, val);
	return val;
}
static int fsa9685_scp_get_chip_status(void)
{
    return 0;
}
static void fsa9685_scp_set_direct_charge_mode(int mode)
{
	hwlog_info("[%s]mode is 0x%x \n", __func__, mode);
	return;
}

static int fsa9685_scp_get_adaptor_type(void)
{
	return LVC_MODE;
}
struct smart_charge_ops scp_fsa9685_ops = {
    .is_support_scp = fsa9685_is_support_scp,
    .scp_init = fsa9685_scp_init,
    .scp_exit = fsa9685_scp_exit,
    .scp_adaptor_detect = fsa9685_scp_adaptor_detect,
    .scp_set_adaptor_voltage = fsa9685_scp_set_adaptor_voltage,
    .scp_get_adaptor_voltage = fsa9685_scp_get_adaptor_voltage,
    .scp_set_adaptor_current = fsa9685_scp_set_adaptor_current,
    .scp_get_adaptor_current = fsa9685_scp_get_adaptor_current,
    .scp_get_adaptor_current_set = fsa9685_scp_get_adaptor_current_set,
    .scp_get_adaptor_max_current = fsa9685_scp_get_adaptor_max_current,
    .scp_adaptor_reset = fsa9685_scp_adaptor_reset,
    .scp_adaptor_output_enable = fsa9685_scp_adaptor_output_enable,
    .scp_chip_reset = fsa9685_scp_chip_reset,
    .scp_stop_charge_config = fsa9685_stop_charge_config,
    .is_scp_charger_type = fsa9685_is_scp_charger_type,
    .scp_get_adaptor_status = fsa9685_scp_get_adaptor_status,
    .scp_get_adaptor_info = fsa9685_scp_get_adaptor_info,
    .scp_get_chip_status = fsa9685_scp_get_chip_status,
    .scp_cable_detect = fsa9685_scp_cable_detect,
    .scp_get_adaptor_temp = fsa9685_scp_get_adaptor_temp,
    .scp_get_adapter_vendor_id = fsa9685_get_adapter_vendor_id,
    .scp_get_usb_port_leakage_current_info = fsa9685_get_usb_port_leakage_current_info,
    .scp_set_direct_charge_mode = fsa9685_scp_set_direct_charge_mode,
    .scp_get_adaptor_type = fsa9685_scp_get_adaptor_type,
};
#endif
struct fcp_adapter_device_ops fcp_fsa9688_ops = {
    .get_adapter_output_current = fcp_get_adapter_output_current,
    .set_adapter_output_vol     = fcp_set_adapter_output_vol,
    .detect_adapter             = fcp_adapter_detect,
    .is_support_fcp             = is_support_fcp,
    .switch_chip_reset          = switch_chip_reset,
    .fcp_adapter_reset          = fcp_adapter_reset,
    .stop_charge_config        = fcp_stop_charge_config,
    .is_fcp_charger_type    = is_fcp_charger_type,
    .fcp_read_adapter_status = fcp_read_adapter_status,
    .fcp_read_switch_status = fcp_read_switch_status,
    .reg_dump = fsa9685_reg_dump,
};
struct charge_switch_ops chrg_fsa9685_ops = {
	.get_charger_type = fsa9685_get_charger_type,
	.is_water_intrused = fsa9685_is_water_intrused,
};

/**********************************************************
*  Function:       fcp_mmi_show
*  Discription:    file node for mmi testing fast charge protocol
*  Parameters:     NA
*  return value:   0:success
*                  1:fail
*                  2:not support
**********************************************************/
static ssize_t fcp_mmi_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int result = FCP_TEST_FAIL;
    enum hisi_charger_type type = hisi_get_charger_type();

    /* judge whether support fcp */
    if(fcp_test_is_support())
    {
        result = FCP_NOT_SUPPORT;
        return snprintf(buf,PAGE_SIZE,"%d\n",result);
    }
    /*to avoid the usb port cutoff when CTS test*/
    if ((type == CHARGER_TYPE_SDP) || (type == CHARGER_TYPE_CDP))
    {
        result = FCP_TEST_FAIL;
        hwlog_err("fcp detect fail 1,charge type is %d \n",type);
        return snprintf(buf,PAGE_SIZE,"%d\n",result);
    }
    /* fcp adapter detect */
   if( fcp_test_detect_adapter())
    {
        /* 0:sdp 1:cdp 2:dcp 3:unknow 4:none*/
        hwlog_err("fcp detect fail 2,charge type is %d \n",fsa9685_get_charger_type());
        result = FCP_TEST_FAIL;
    }
    else
    {
        result = FCP_TEST_SUCC;
    }
    hwlog_info("%s: fcp mmi result  %d\n",__func__,result);
    return snprintf(buf,PAGE_SIZE,"%d\n",result);
}

static DEVICE_ATTR(fcp_mmi, S_IRUGO, fcp_mmi_show, NULL);

#ifdef CONFIG_OF
static const struct of_device_id switch_fsa9685_ids[] = {
	{ .compatible = "huawei,fairchild_fsa9685" },
	{ .compatible = "huawei,fairchild_fsa9683"},
	{ .compatible = "huawei,nxp_cbtl9689" },
	{ },
};
MODULE_DEVICE_TABLE(of, switch_fsa9685_ids);
#endif

static struct usbswitch_common_ops huawei_switch_extra_ops = {
	.manual_switch = fsa9685_manual_switch,
	.dcd_timeout_enable = fsa9685_dcd_timeout,
	.dcd_timeout_status = fsa9685_dcd_timeout_status,
	.manual_detach = fsa9685_manual_detach_work,
};

static int fsa9685_parse_dts(struct device_node* np, struct fsa9685_device_info *di)
{
	int ret = 0;

	ret = of_property_read_u32(np, "usbid-enable", &(di->usbid_enable));
	if (ret) {
		di->usbid_enable = 1;
		hwlog_err("error: usbid-enable dts read failed!\n");
	}
	hwlog_info("usbid-enable=%d\n", di->usbid_enable);

	ret = of_property_read_u32(np, "fcp_support", &(di->fcp_support));
	if (ret) {
		di->fcp_support = 0;
		hwlog_err("error: fcp_support dts read failed!\n");
	}
	hwlog_info("fcp_support=%d\n", di->fcp_support);

	ret = of_property_read_u32(np, "scp_support", &(di->scp_support));
	if (ret) {
		di->scp_support = 0;
		hwlog_err("error: scp_support dts read failed!\n");
	}
	hwlog_info("scp_support=%d\n", di->scp_support);

	ret = of_property_read_u32(np, "mhl_detect_disable", &(di->mhl_detect_disable));
	if (ret) {
		di->mhl_detect_disable = 0;
		hwlog_err("error: mhl_detect_disable dts read failed!\n");
	}
	hwlog_info("mhl_detect_disable=%d\n", di->mhl_detect_disable);

	ret = of_property_read_u32(np, "two-switch-flag", &(di->two_switch_flag));
	if (ret) {
		di->two_switch_flag = 0;
		hwlog_err("error: two-switch-flag dts read failed!\n");
	}
	hwlog_info("two-switch-flag=%d\n", di->two_switch_flag);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger"),
		"pd_support", &(di->pd_support));
	if (ret) {
		di->pd_support = 0;
		hwlog_err("error: pd_support dts read failed!\n");
	}
	hwlog_info("pd_support=%d\n", di->pd_support);

	ret = of_property_read_u32(np, "dcd_timeout_force_enable", &(di->dcd_timeout_force_enable));
	if (ret) {
		di->dcd_timeout_force_enable = 0;
		hwlog_err("error: dcd_timeout_force_enable dts read failed!\n");
	}
	hwlog_info("dcd_timeout_force_enable=%d\n",di->dcd_timeout_force_enable);

	return 0;
}

static int rt8979_write_osc_pretrim(void)
{
	int retval = 0;

	retval = fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_VAL1);
	if (retval < 0)
		return retval;
	retval = fsa9685_write_reg(RT8979_REG_EFUSE_CTRL, RT8979_REG_EFUSE_CTRL_VAL);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}
	retval = fsa9685_write_reg(RT8979_REG_EFUSE_PRETRIM_DATA,
		(rt8979_osc_trim_code + rt8979_osc_trim_adjust) ^ RT8979_REG_EFUSE_PRETRIM_DATA_VAL);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}
	retval = fsa9685_write_reg(RT8979_REG_EFUSE_PRETRIM_ENABLE, RT8979_REG_EFUSE_PRETRIM_ENABLE_VAL);
	usleep_range(WRITE_OSC_PRETRIM_DELAY_MIN, WRITE_OSC_PRETRIM_DELAY_MAX);
	retval = fsa9685_read_reg(RT8979_REG_EFUSE_READ_DATA);
	hwlog_info("%s : trim code read = %d\n", __func__, retval);

	retval = fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
	return retval;
}

static int rt8979_adjust_osc(int8_t val) {
	int8_t temp;

	temp = rt8979_osc_trim_code + rt8979_osc_trim_adjust + val;
	if (temp > rt8979_osc_upper_bound || temp < rt8979_osc_lower_bound) {
		hwlog_err("%s : reach to upper/lower bound %d / %d\n", __func__,
			rt8979_osc_trim_code, rt8979_osc_trim_adjust);
		return -RT8979_FAIL;
	}

	rt8979_osc_trim_adjust += val;
	hwlog_info("%s : adjust osc trim code %d / %d\n", __func__,
		rt8979_osc_trim_code, rt8979_osc_trim_adjust);

	return rt8979_write_osc_pretrim();
}

static int rt8979_init_osc_params(void)
{
	int retval = 0;

	hwlog_info("%s : entry\n", __func__);
	retval = fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_VAL1);
	if (retval < 0)
		return retval;

	retval = fsa9685_write_reg(RT8979_REG_EFUSE_CTRL, RT8979_REG_EFUSE_CTRL_VAL);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}

	retval = fsa9685_write_reg(RT8979_REG_EFUSE_PRETRIM_ENABLE, RT8979_REG_EFUSE_PRETRIM_ENABLE_VAL1);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}
	usleep_range(WRITE_OSC_PRETRIM_DELAY_MIN_DEFAULT, WRITE_OSC_PRETRIM_DELAY_MIN);
	retval = fsa9685_read_reg(RT8979_REG_EFUSE_READ_DATA);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}
	hwlog_info("%s : trim code read = %d\n", __func__, retval);
	rt8979_osc_trim_code = retval;
	rt8979_osc_lower_bound = RT8979_OSC_BOUND_MIN;
	rt8979_osc_upper_bound = RT8979_OSC_BOUND_MAX;
	rt8979_osc_trim_adjust = RT8979_OSC_TRIM_ADJUST_DEFAULT;

	retval = rt8979_osc_trim_code + rt8979_osc_trim_adjust;
	if (retval < rt8979_osc_lower_bound)
		retval = rt8979_osc_lower_bound;
	if (retval > rt8979_osc_upper_bound)
		retval = rt8979_osc_upper_bound;
	rt8979_osc_trim_default = retval - rt8979_osc_trim_code;
	rt8979_osc_trim_adjust = rt8979_osc_trim_default;

	retval = fsa9685_write_reg(RT8979_REG_EFUSE_PRETRIM_DATA,
		(retval) ^ RT8979_REG_EFUSE_PRETRIM_DATA_VAL);
	hwlog_info("%s : trim code = %d, rt8979_osc_trim_adjust = %d, %d\n",
			__func__, rt8979_osc_trim_code, rt8979_osc_trim_adjust,
			(rt8979_osc_trim_code + rt8979_osc_trim_adjust) ^ RT8979_REG_EFUSE_PRETRIM_DATA_VAL);
	if (retval < 0) {
		fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
		return retval;
	}
	fsa9685_write_reg(RT8979_REG_EFUSE_PRETRIM_ENABLE, RT8979_REG_EFUSE_PRETRIM_ENABLE_VAL);
	usleep_range(WRITE_OSC_PRETRIM_DELAY_MIN, WRITE_OSC_PRETRIM_DELAY_MAX);
	retval = fsa9685_read_reg(RT8979_REG_EFUSE_READ_DATA);
	hwlog_info("%s : trim code read = %d\n", __func__, retval);

	retval = fsa9685_write_reg(RT8979_REG_TEST_MODE, RT8979_REG_TEST_MODE_DEFAULT_VAL);
	return retval;
}
static int fsa9685_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fsa9685_device_info *di = NULL;
	struct device_node *node = NULL;

    int ret = 0, reg_ctl, gpio_value, reg_vendor = -1;
    bool is_dcp = false;
#ifdef CONFIG_FSA9685_DEBUG_FS
    struct class *switch_class = NULL;
    struct device * new_dev = NULL;
#endif

    hwlog_info("%s: ------entry.\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		hwlog_err("error: i2c_check failed!\n");
		ret = -ERR_NO_DEV;
		g_fsa9685_dev = NULL;
		goto err_i2c_check_functionality;
	}

	if (g_fsa9685_dev) {
		hwlog_err("error: chip is already detected!\n");
		ret = -ERR_NO_DEV;
		return ret;
	}
	else {
		di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
		if (!di) {
			hwlog_err("error: kzalloc failed!\n");
			return -ENOMEM;
		}
		g_fsa9685_dev = di;

		di->dev = &client->dev;
		node = di->dev->of_node;
		di->client = client;
		i2c_set_clientdata(client, di);
	}

	/* device idendify */
	di->device_id = fsa9685_get_device_id();
	if (di->device_id < 0) {
		goto err_i2c_check_functionality;
	}

	fsa9685_select_device_ops(di->device_id);

    /* distingush the chip with different address */
    reg_vendor = fsa9685_read_reg(FSA9685_REG_DEVICE_ID);
    if ( reg_vendor < 0 ) {
        hwlog_err("%s: read FSA9685_REG_DEVICE_ID error!!! reg_vendor=%d.\n", __func__, reg_vendor);
        goto err_i2c_check_functionality;
    }
    vendor_id = reg_vendor & FAS9685_VENDOR_ID_BIT_MASK;
    if (is_rt8979()) {
		rt8979_accp_enable(true);
        reg_ctl = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
        hwlog_info("%s : DEV_TYPE1 = 0x%x\n", __func__, reg_ctl);
        is_dcp = (reg_ctl & FSA9685_DCP_DETECTED) ? true : false;

	if (is_dcp) {
            hwlog_info("%s: reset rt8979\n", __func__);
            fsa9685_write_reg(FSA9685_REG_RESET, FSA9685_REG_RESET_ENTIRE_IC);
            msleep(1);
            fsa9685_write_reg_mask(FSA9685_REG_CONTROL, 0, FSA9685_SWITCH_OPEN);
			rt8979_accp_enable(true);
        }
        fsa9685_write_reg_mask(RT8979_REG_TIMING_SET_2, RT8979_REG_TIMING_SET_2_DCDTIMEOUT, RT8979_REG_TIMING_SET_2_DCDTIMEOUT);
		rt8979_init_osc_params();
    }

	ret = device_create_file(&client->dev, &dev_attr_dump_regs);
	if (ret < 0) {
		hwlog_err("error: sysfs device_file create failed(dump_regs)!\n");
		ret = -ERR_SWITCH_USB_DEV_REGISTER;
		goto err_i2c_check_functionality;
	}

/*create a node for phone-off current drain test*/
    ret = device_create_file(&client->dev, &dev_attr_switchctrl);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_get_named_gpio;
    }

    ret = device_create_file(&client->dev, &dev_attr_jigpin_ctrl);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_create_jigpin_ctrl_failed;
    }

    ret = device_create_file(&client->dev, &dev_attr_fcp_mmi);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_create_fcp_mmi_failed;
    }

    switch_class = class_create(THIS_MODULE, "usb_switch");
    if (IS_ERR(switch_class)) {
        hwlog_err("%s:create switch class failed!\n", __func__);
        goto err_create_link_failed;
    }
    new_dev = device_create(switch_class, NULL, 0, NULL, "switch_ctrl");
    if (NULL == new_dev) {
        hwlog_err("%s:device create failed!\n", __func__);
        goto err_create_link_failed;
    }
    ret = sysfs_create_link(&new_dev->kobj, &client->dev.kobj, "manual_ctrl");
    if (ret < 0) {
        hwlog_err("%s:create link to switch failed!\n", __func__);
        goto err_create_link_failed;
    }

	ret = fsa9685_parse_dts(node, di);
	if (ret) {
		hwlog_err("error: parse dts failed!\n");
		goto err_create_link_failed;
	}

	/* init lock */
	mutex_init(&di->accp_detect_lock);
	mutex_init(&di->accp_adaptor_reg_lock);
	wake_lock_init(&di->usb_switch_lock, WAKE_LOCK_SUSPEND, "usb_switch_wakelock");

	/* init work */
	INIT_DELAYED_WORK(&di->detach_delayed_work, fsa9685_detach_work);
	INIT_WORK(&di->g_intb_work, is_rt8979() ? rt8979_intb_work : fsa9685_intb_work);

/*create link end*/
    gpio = of_get_named_gpio(node, "fairchild_fsa9685,gpio-intb", 0);
    if (gpio < 0) {
        hwlog_err("%s: of_get_named_gpio error!!! ret=%d, gpio=%d.\n", __func__, ret, gpio);
        ret = -ERR_OF_GET_NAME_GPIO;
        goto fail_free_wakelock;
    }

    client->irq = gpio_to_irq(gpio);

    if (client->irq < 0) {
        hwlog_err("%s: gpio_to_irq error!!! ret=%d, gpio=%d, client->irq=%d.\n", __func__, ret, gpio, client->irq);
        ret = -ERR_GPIO_TO_IRQ;
        goto fail_free_wakelock;
    }

    ret = gpio_request(gpio, "fsa9685_int");
    if (ret < 0) {
        hwlog_err("%s: gpio_request error!!! ret=%d. gpio=%d.\n", __func__, ret, gpio);
        ret = -ERR_GPIO_REQUEST;
        goto fail_free_wakelock;
    }

    ret = gpio_direction_input(gpio);
    if (ret < 0) {
        hwlog_err("%s: gpio_direction_input error!!! ret=%d. gpio=%d.\n", __func__, ret, gpio);
        ret = -ERR_GPIO_DIRECTION_INPUT;
        goto fail_free_int_gpio;
    }

    if (!is_rt8979()) {
        ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, 0,FSA9685_DCD_TIME_OUT_MASK);
        if ( ret < 0 ){
            hwlog_err("%s: write FSA9685_REG_CONTROL2 FSA9685_DCD_TIME_OUT_MASK error!!! ret=%d", __func__, ret);
        }
        ret |= fsa9685_write_reg_mask(FSA9685_REG_INTERRUPT_MASK,FSA9685_DEVICE_CHANGE,FSA9685_DEVICE_CHANGE);
        if ( ret < 0 ){
            hwlog_err("%s: write Mask  Device change intterrupt error!!! ret=%d", __func__, ret);
        }
    }
    /* if support fcp ,disable fcp interrupt */
    if( 0 == is_support_fcp())
    {
        ret |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, 0, FSA9685_ACCP_OSC_ENABLE);
        ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1, 0xFF);
        ret |= fsa9685_write_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2, 0xFF);
        if(ret < 0)
        {
            hwlog_err("accp interrupt mask write failed \n");
        }
        reg_ctl = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
        hwlog_info("%s : DEV_TYPE1 = 0x%x\n", __func__, reg_ctl);
        if (is_rt8979()) {
            hwlog_info("ChipID = RT8979 (reg_vendor = 0x%02x)\n", reg_vendor);
            fsa9685_write_reg(RT8979_REG_EXT3, RT8979_REG_EXT3_VAL);
            fsa9685_write_reg(RT8979_REG_MUIC_CTRL_3, RT8979_REG_MUIC_CTRL_3_DISABLEID_FUNCTION);
            reg_ctl = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
            hwlog_info("%s : DEV_TYPE1 = 0x%x\n", __func__, reg_ctl);
            reg_ctl = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
            hwlog_info("%s : DEV_TYPE4 = 0x%x\n", __func__, reg_ctl);
            hwlog_info("fsa9685_read_reg, 0xa4=0x%x\r\n", fsa9685_read_reg(RT8979_REG_USBCHGEN));
            hwlog_info("fsa9685_read_reg, 0xA0=0x%x\r\n", fsa9685_read_reg(RT8979_REG_EXT3));
            hwlog_info("fsa9685_read_reg, 0x10=0x%x\r\n", fsa9685_read_reg(RT8979_REG_MUIC_CTRL_3));
            hwlog_info("fsa9685_read_reg, 0x0e=0x%x\r\n", fsa9685_read_reg(RT8979_REG_MUIC_CTRL));
        }
    }
    /* interrupt register */

    ret = request_irq(client->irq,
               fsa9685_irq_handler,
               IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING,
               "fsa9685_int", client);
    if (ret < 0) {
        hwlog_err("%s: request_irq error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_REQUEST_THREADED_IRQ;
        goto fail_free_int_gpio;
    }
    /* clear INT MASK */
    reg_ctl = fsa9685_read_reg(FSA9685_REG_CONTROL);
    if ( reg_ctl < 0 ) {
        hwlog_err("%s: read FSA9685_REG_CONTROL error!!! reg_ctl=%d.\n", __func__, reg_ctl);
        goto fail_free_int_irq;
    }
    hwlog_info("%s: read FSA9685_REG_CONTROL. reg_ctl=0x%x.\n", __func__, reg_ctl);

    reg_ctl &= (~FSA9685_INT_MASK);
    ret = fsa9685_write_reg(FSA9685_REG_CONTROL, reg_ctl);
    if ( ret < 0 ) {
        hwlog_err("%s: write FSA9685_REG_CONTROL error!!! reg_ctl=%d.\n", __func__, reg_ctl);
        goto fail_free_int_irq;
    }
    hwlog_info("%s: write FSA9685_REG_CONTROL. reg_ctl=0x%x.\n", __func__, reg_ctl);

    ret = fsa9685_write_reg(FSA9685_REG_DCD, 0x0c);
    if ( ret < 0 ) {
        hwlog_err("%s: write FSA9685_REG_DCD error!!! reg_DCD=0x%x.\n", __func__, 0x08);
        goto fail_free_int_irq;
    }
    hwlog_info("%s: write FSA9685_REG_DCD. reg_DCD=0x%x.\n", __func__, 0x0c);

    gpio_value = gpio_get_value(gpio);
    hwlog_info("%s: intb=%d after clear MASK.\n", __func__, gpio_value);

    if (gpio_value == 0) {
        hwlog_info("%s: GPIO == 0\n", __func__);
        schedule_work(&di->g_intb_work);
    }

    /* if chip support fcp ,register fcp adapter ops */
    if( 0 == is_support_fcp() && 0 ==fcp_adapter_ops_register(&fcp_fsa9688_ops))
    {
        hwlog_info(" fcp adapter ops register success!\n");
    }

#ifdef CONFIG_HUAWEI_CHARGER
    if(0 == charge_switch_ops_register(&chrg_fsa9685_ops))
    {
        hwlog_info(" charge switch ops register success!\n");
    }
#endif

#ifdef CONFIG_DIRECT_CHARGER
    /* if chip support scp ,register scp adapter ops */
    if( 0 == fsa9685_is_support_scp() && 0 ==scp_ops_register(&scp_fsa9685_ops))
    {
        hwlog_info(" scp adapter ops register success!\n");
    }
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_USB_SWITCH);
#endif

    ret = usbswitch_common_ops_register(&huawei_switch_extra_ops);
    if (ret) {
    	hwlog_err("register extra switch ops failed!\n");
    }

    if (is_rt8979() && is_dcp)
    {
        charge_type_dcp_detected_notify();
    }
    hwlog_info("%s: ------end. ret = %d.\n", __func__, ret);
    return ret;
fail_free_int_irq:
	free_irq(client->irq, client);
fail_free_int_gpio:
	gpio_free(gpio);
fail_free_wakelock:
	wake_lock_destroy(&di->usb_switch_lock);
err_create_link_failed:
    device_remove_file(&client->dev, &dev_attr_fcp_mmi);
err_create_fcp_mmi_failed:
    device_remove_file(&client->dev, &dev_attr_jigpin_ctrl);
err_create_jigpin_ctrl_failed:
    device_remove_file(&client->dev, &dev_attr_switchctrl);
err_get_named_gpio:
    device_remove_file(&client->dev, &dev_attr_dump_regs);
err_i2c_check_functionality:
    g_fsa9685_dev = NULL;

    hwlog_err("%s: ------FAIL!!! end. ret = %d.\n", __func__, ret);
    return ret;
}

static int fsa9685_remove(struct i2c_client *client)
{
	struct fsa9685_device_info *di = i2c_get_clientdata(client);

	hwlog_info("remove begin\n");

	device_remove_file(&client->dev, &dev_attr_dump_regs);
	device_remove_file(&client->dev, &dev_attr_switchctrl);
	device_remove_file(&client->dev, &dev_attr_jigpin_ctrl);
	free_irq(client->irq, client);
	gpio_free(gpio);
	if (di)
		wake_lock_destroy(&di->usb_switch_lock);

	hwlog_info("remove end\n");
	return 0;
}

static void fsa9685_shutdown(struct i2c_client *client)
{

    int ret = 0;
#ifdef CONFIG_BOOST_5V
    boost_5v_enable(ENABLE, BOOST_CTRL_FCP);
    direct_charge_set_bst_ctrl(ENABLE);
#endif
    if (is_rt8979()) {
        ret = fsa9685_read_reg(RT8979_REG_MUIC_CTRL_4);
        ret = fsa9685_write_reg(RT8979_REG_MUIC_CTRL_4, ret&RT8979_REG_MUIC_CTRL_4_ENABLEID2_FUNCTION);
        if(ret < 0)
            hwlog_info("%s: error !!! ret=%d\n", __func__, ret);
    }
}

static const struct i2c_device_id fsa9685_i2c_id[] = {
    { "fsa9685", 0 },
    { }
};

static struct i2c_driver fsa9685_i2c_driver = {
	.probe = fsa9685_probe,
	.remove = fsa9685_remove,
	.shutdown = fsa9685_shutdown,
	.id_table = fsa9685_i2c_id,
	.driver = {
		.name = "fsa9685",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(switch_fsa9685_ids),
	},
};

static __init int fsa9685_i2c_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&fsa9685_i2c_driver);
	if (ret) {
		hwlog_err("error: fsa9685 i2c_add_driver error!\n");
	}

	return ret;
}

static __exit void fsa9685_i2c_exit(void)
{
	i2c_del_driver(&fsa9685_i2c_driver);
}

module_init(fsa9685_i2c_init);
module_exit(fsa9685_i2c_exit);

MODULE_AUTHOR("Lixiuna<lixiuna@huawei.com>");
MODULE_DESCRIPTION("I2C bus driver for FSA9685 USB Accesory Detection Switch");
MODULE_LICENSE("GPL v2");
