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
#include <linux/wakelock.h>

#include "switch_chip.h"
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/switch/switch_fsa9685.h>

#define HWLOG_TAG usbswitch_rt8979
HWLOG_REGIST();

const static int usbswitch_rt8979_regaddrs[] = {
	FSA9685_REG_DEVICE_ID,
	FSA9685_REG_CONTROL,
	//FSA9685_REG_INTERRUPT,
	FSA9685_REG_INTERRUPT_MASK,
	FSA9685_REG_ADC,
	FSA9685_REG_TIMING_SET_1,
	FSA9685_REG_DETACH_CONTROL,
	FSA9685_REG_DEVICE_TYPE_1,
	FSA9685_REG_DEVICE_TYPE_2,
	FSA9685_REG_DEVICE_TYPE_3,
	FSA9685_REG_MANUAL_SW_1,
	FSA9685_REG_MANUAL_SW_2,
	FSA9685_REG_DCD,

	RT8979_REG_TIMING_SET_2,
	RT8979_REG_MUIC_CTRL,
	RT8979_REG_MUIC_CTRL_3,
	RT8979_REG_MUIC_STATUS1,
	RT8979_REG_MUIC_STATUS2,
	RT8979_REG_ADC,
};

static bool usbswitch_rt8979_is_in_fm8(void)
{
	int id_value = 0, device_type3 = 0, status1 = 0;
	bool retval = false;

	device_type3 = fsa9685_common_read_reg(FSA9685_REG_DEVICE_TYPE_3);
	id_value = fsa9685_common_read_reg(RT8979_REG_ADC);
	status1 = fsa9685_common_read_reg(RT8979_REG_MUIC_STATUS1);

	if (status1 & RT8979_REG_MUIC_STATUS1_FMEN) {
		retval = true;
	}
	else {
		retval = false;
	}

	hwlog_info("is_in_fm8=%d\n", retval);

	return retval;
}

static int usbswitch_rt8979_sw_open(bool open)
{
	hwlog_info("sw_open=%s\n", open ? "open" : "auto");

	return fsa9685_common_write_reg_mask(FSA9685_REG_CONTROL,
		(open ? 0 : FSA9685_SWITCH_OPEN), FSA9685_SWITCH_OPEN);
}

static int usbswitch_rt8979_accp_enable(bool en)
{
	hwlog_info("accp_enable=%s\n", en ? "enable" : "disable");

	return fsa9685_common_write_reg_mask(FSA9685_REG_CONTROL2,
		(en ? FSA9685_ACCP_AUTO_ENABLE : 0), FSA9685_ACCP_AUTO_ENABLE);
}

static void usbswitch_rt8979_force_restart_accp_det(bool open)
{
	hwlog_info("force_restart_accp_det entry\n");

	usbswitch_rt8979_sw_open(true);
	usbswitch_rt8979_accp_enable(true);

	msleep(1); /* sleep 1ms */

	fsa9685_common_write_reg_mask(RT8979_REG_USBCHGEN, 0, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);
	fsa9685_common_write_reg_mask(RT8979_REG_MUIC_CTRL,0, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);
	fsa9685_common_write_reg_mask(RT8979_REG_USBCHGEN,
		RT8979_REG_USBCHGEN_ACCPDET_STAGE1, RT8979_REG_USBCHGEN_ACCPDET_STAGE1);

	msleep(300); /* sleep 300ms */

	usbswitch_rt8979_sw_open(open);
	fsa9685_common_write_reg_mask(RT8979_REG_MUIC_CTRL,
		RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT, RT8979_REG_MUIC_CTRL_DISABLE_DCDTIMEOUT);

	hwlog_info("force_restart_accp_det end\n");
}

static void usbswitch_rt8979_detach_work(void)
{
	int ret = 0;

	hwlog_info("detach_work entry\n");

	ret = fsa9685_common_read_reg(RT8979_REG_USBCHGEN);
	if (ret < 0) {
		hwlog_err("error: detach_work read fail!\n");
		return;
	}

	ret = fsa9685_common_write_reg(RT8979_REG_USBCHGEN, RT8979_REG_USBCHGEN_DETACH_STAGE1);
	if (ret < 0) {
		hwlog_err("error: detach_work write fail!\n");
		return;
	}

	ret = fsa9685_common_write_reg(RT8979_REG_USBCHGEN, RT8979_REG_USBCHGEN_DETACH_STAGE2);
	if (ret < 0) {
		hwlog_err("error: detach_work write fail!\n");
		return;
	}

	msleep(50); /* sleep 50ms */

	usbswitch_rt8979_force_restart_accp_det(false);

	hwlog_info("detach_work end\n");

	return;
}

static int usbswitch_rt8979_manual_switch(int input_select)
{
	int value = 0, ret = 0;

	switch (input_select) {
		case FSA9685_USB1_ID_TO_IDBYPASS:
			value = REG_VAL_FSA9685_USB1_ID_TO_IDBYPASS;
		break;

		case FSA9685_USB2_ID_TO_IDBYPASS:
			value = REG_VAL_FSA9685_USB2_ID_TO_IDBYPASS;
		break;

		case FSA9685_UART_ID_TO_IDBYPASS:
			value = REG_VAL_FSA9685_UART_ID_TO_IDBYPASS;
		break;

		case FSA9685_MHL_ID_TO_CBUS:
			value = REG_VAL_FSA9685_MHL_ID_TO_CBUS;
		break;

		case FSA9685_USB1_ID_TO_VBAT:
			value = REG_VAL_FSA9685_USB1_ID_TO_VBAT;
		break;

		case FSA9685_OPEN:
		default:
			value = REG_VAL_FSA9685_OPEN;
		break;
	}

	ret = fsa9685_common_write_reg(FSA9685_REG_MANUAL_SW_1, value);
	if (ret < 0) {
		ret = -ERR_FSA9685_REG_MANUAL_SW_1;
		hwlog_err("error: manual_switch write fail!\n");
		return ret;
	}

	value = fsa9685_common_read_reg(FSA9685_REG_CONTROL);
	if (value < 0) {
		ret = -ERR_FSA9685_READ_REG_CONTROL;
		hwlog_err("error: manual_switch read fail!\n");
		return ret;
	}

	value &= (~FSA9685_MANUAL_SW); /* 0: manual switching */
	ret = fsa9685_common_write_reg(FSA9685_REG_CONTROL, value);
	if (ret < 0) {
		ret = -ERR_FSA9685_WRITE_REG_CONTROL;
		hwlog_err("error: manual_switch write fail!\n");
		return ret;
	}

	usbswitch_rt8979_sw_open(false);

	return 0;
}

static int usbswitch_rt8979_switchctrl_store(struct i2c_client *client, int action)
{
	switch (action) {
		case MANUAL_DETACH:
			hwlog_info("manual_detach\n");
			usbswitch_common_manual_detach();
		break;

		case MANUAL_SWITCH:
		hwlog_info("manual_switch(usb1_id_to_vbat)\n");
			usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_VBAT);
		break;

		default:
			hwlog_err("error: Wrong input action!\n");
			return -1;
		break;
	}

	return 0x60;
}

static int usbswitch_rt8979_switchctrl_show(char *buf)
{
	int device_type1 = 0, device_type2 = 0, device_type3 = 0, mode = -1, tmp = 0;

	device_type1 = fsa9685_common_read_reg(FSA9685_REG_DEVICE_TYPE_1);
	if (device_type1 < 0) {
		hwlog_err("error: switchctrl_show read fail!\n");
		goto read_reg_failed;
	}

	device_type2 = fsa9685_common_read_reg(FSA9685_REG_DEVICE_TYPE_2);
	if (device_type2 < 0) {
		hwlog_err("error: switchctrl_show read fail!\n");
		goto read_reg_failed;
	}

	device_type3 = fsa9685_common_read_reg(FSA9685_REG_DEVICE_TYPE_3);
	if (device_type3 < 0) {
		hwlog_err("error: switchctrl_show read fail!\n");
		goto read_reg_failed;
	}

	hwlog_info("type1=0x%x type2=0x%x type3=0x%x\n",device_type1, device_type2, device_type3);

	tmp = device_type3 << 16 | device_type2 << 8 | device_type1;
	mode = 0;
	while (tmp >> mode) {
		mode++;
	}

	if (usbswitch_rt8979_is_in_fm8()) {
		mode = RT8979_FM8_MODE;
	}

read_reg_failed:
	return scnprintf(buf, PAGE_SIZE, "%d\n", mode);
}

static int usbswitch_rt8979_jigpin_ctrl_store(struct i2c_client *client, int jig_val)
{
	int ret = 0;

	ret = fsa9685_common_write_reg_mask(FSA9685_REG_CONTROL, 0, FSA9685_MANUAL_SW);
	if (ret < 0) {
		hwlog_err("error: jigpin_ctrl_store write fail!\n");
		return ret;
	}

	switch (jig_val) {
		case JIG_PULL_DEFAULT_DOWN:
			hwlog_info("pull down jig pin to default state\n");

			ret = fsa9685_common_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
							FSA9685_REG_JIG_DEFAULT_DOWN, FSA9685_REG_JIG_MASK);
			if (ret < 0) {
				hwlog_err("error: jigpin_ctrl_store write fail!\n");
			}
		break;

		case JIG_PULL_UP:
			hwlog_info("pull up jig pin to cut battery\n");

			ret = fsa9685_common_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
							FSA9685_REG_JIG_UP, FSA9685_REG_JIG_MASK);
			if (ret < 0) {
				hwlog_err("error: jigpin_ctrl_store write fail!\n");
			}
		break;

		default:
			hwlog_err("error: Wrong input action!\n");
			return -1;
		break;
	}

	return 0x60;
}

static int usbswitch_rt8979_jigpin_ctrl_show(char *buf)
{
	int manual_sw2_val = 0;

	manual_sw2_val = fsa9685_common_read_reg(FSA9685_REG_MANUAL_SW_2);
	if (manual_sw2_val < 0) {
		hwlog_err("error: jigpin_ctrl_show read fail!\n");
	}

	return scnprintf(buf, PAGE_SIZE, "%02x\n", manual_sw2_val);
}

static int usbswitch_rt8979_dump_regs(char *buf)
{
	int i = 0;
	int val = 0;
	char rd_buf[FSA9685_RD_BUF_SIZE] = {0};
	int size = ARRAY_SIZE(usbswitch_rt8979_regaddrs);

	scnprintf(rd_buf, FSA9685_RD_BUF_SIZE, "dump_regs: rt8979\n");
	strncat(buf, rd_buf, strlen(rd_buf));

	for (i = 0; i < size; i++) {
		val = fsa9685_common_read_reg(usbswitch_rt8979_regaddrs[i]);

		memset(rd_buf, 0, FSA9685_RD_BUF_SIZE);
		scnprintf(rd_buf, FSA9685_RD_BUF_SIZE, "[0x%02x]: 0x%02x\n", usbswitch_rt8979_regaddrs[i], val);
		strncat(buf, rd_buf, strlen(rd_buf));
	}

	return strlen(buf);
}

static struct fsa9685_device_ops usbswitch_rt8979_ops = {
	.dump_regs = usbswitch_rt8979_dump_regs,
	.jigpin_ctrl_show = usbswitch_rt8979_jigpin_ctrl_show,
	.jigpin_ctrl_store = usbswitch_rt8979_jigpin_ctrl_store,
	.switchctrl_store = usbswitch_rt8979_switchctrl_store,
	.switchctrl_show = usbswitch_rt8979_switchctrl_show,

	.manual_switch = usbswitch_rt8979_manual_switch,

	.detach_work = usbswitch_rt8979_detach_work,
};

struct fsa9685_device_ops* usbswitch_rt8979_get_device_ops(void)
{
	return &usbswitch_rt8979_ops;
}
