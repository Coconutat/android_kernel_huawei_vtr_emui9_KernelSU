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
#include <linux/wakelock.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include "../Platform_Linux/platform_helpers.h"
#include "hw_scp.h"
#include "core.h"
#include "port.h"
#include "../Platform_Linux/fusb3601_global.h"
#include <huawei_platform/power/wired_channel_switch.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#include <linux/hisi/usb/hisi_usb.h>
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
#include <huawei_platform/audio/usb_analog_hs_interface.h>
#endif

#define FUSB3601_PWCTRL   0x1c
#define FUSB3601_AUTO_DISCH_EN 0x10
#define FUSB3601_AUTO_DISCH_DIS 0xef
#define FUSB3601_COMMAND  0x23
#define FUSB3601_DISABLE_SINK_VBUS  0x44
#define FUSB3601_DISABLE_VBUS_DET  0x22
#define FUSB3601_ENABLE_VBUS_DET  0x33
#define FUSB3601_SINK_VBUS  0x55

#define ACCP_CMD_SBRRD  0x0c
#define ACCP_CMD_SBRWR  0x0b
#define SCP_ACK_AND_NACK_MASK 0x28
#define SCP_ACK_MASK 0x20
#define FUSB3601_REG_ACCP_RT_CMD   0xa0
#define FUSB3601_REG_ACCP_RT_ADDR  0xa1
#define FUSB3601_REG_ACCP_RT_TX0 0xa2
#define FUSB3601_REG_ACCP_RT_ACK_RX  0xc0
#define FUSB3601_REG_ACCP_RT_RX0  0xc1
#define FUSB3601_REG_ACCP_RT_RX1  0xc2
#define FUSB3601_SCP_ENABLE1 0x80
#define FUSB3601_CHIP_RESET 0x40
#define FUSB3601_SCP_ENABLE1_INIT 0x80
#define FUSB3601_SCP_ENABLE2 0x81
#define FUSB3601_MST_RESET_MSK 0x02
#define FUSB3601_SCP_ENABLE2_INIT 0xa1
#define FUSB3601_DPD_DISABLE 0x81
#define FUSB3601_SCP_INT1_MASK 0x82
#define FUSB3601_SCP_INT1_MASK_INIT 0x00
#define FUSB3601_SCP_INT2_MASK 0x83
#define FUSB3601_SCP_INT2_MASK_INIT 0x00
#define FUSB3601_TIMER_SET1 0x84
#define FUSB3601_TIMER_SET1_INIT 0x00
#define FUSB3601_TIMER_SET2 0x85
#define FUSB3601_TIMER_SET2_INIT 0x00
#define FUSB3601_MUS_INTERRUPT_MASK 0xd4
#define FUSB3601_MUS_INTERRUPT_MASK_INIT 0x1b
#define FUSB3601_MUS_CONTROL1 0xd1
#define FUSB3601_DCD_TIMEOUT_DISABLE 0xef
#define FUSB3601_DEVICE_TYPE 0xd6
#define FUSB3601_SCP_EVENT_1 0x86
#define FUSB3601_SCP_EVENT1_CC_PLGIN_DPDN_PLGIN 0x60
#define FUSB3601_SCP_EVENT_2 0x87
#define FUSB3601_SCP_EVENT2_ACK 0x20
#define FUSB3601_SCP_EVENT_3 0x88
#define FUSB3601_FM_CONTROL1 0xdc
#define FUSB3601_MSM_EN_HIGH 0xfe
#define FUSB3601_MSM_EN_LOW 0xfd
#define FUSB3601_FM_CONTROL3 0xde
#define FUSB3601_FM_CONTROL4 0xdf
#define FUSB3601_DIS_VBUS_DETECTION_MSK 0xdf
#define FUSB3601_VOUT_ENABLE 0x7b
#define FUSB3601_VOUT_DISABLE 0x73
#define MUS_CONTRAL2 0xd2
#define FUSB3601_DCP_DETECT 0x40
#define FUSB3601_SCP_OR_FCP_DETECT 0x19
#define FUSB3601_SCP_B_DETECT 0x08

static struct mutex FUSB3601_accp_detect_lock;
static struct mutex FUSB3601_accp_adaptor_reg_lock;

static int wired_channel_status = WIRED_CHANNEL_RESTORE;

struct delayed_work m_work;
static int FUSB3601_is_support_scp(void);
#define HWLOG_TAG FUSB3601_scp
HWLOG_REGIST();

static u32 FUSB3601_fcp_support = 0;
static u32 FUSB3601_scp_support = 0;
static u32 FUSB3601_scp_error_flag = 0;/*scp error flag*/

static int FUSB3601_is_support_fcp(void);
void FUSB3601_scp_initialize(void);
static int FUSB3601_scp_get_adapter_vendor_id(void);
extern int state_machine_need_resched;
static int FUSB3601_accp_adapter_reg_write(int val, int reg);
static int FUSB3601_accp_adapter_reg_read(int* val, int reg);
/****************************************************************************
  Function:     FUSB3601_fcp_stop_charge_config
  Description:  fcp stop charge config
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static void FUSB3601_clear_scp_event1(void)
{
	FSC_U8 reg_val1 = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1,&reg_val1);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_1, 1, &reg_val1);
	if (!ret)
	    hwlog_info("%s:i2c error\n", __func__);
}
static int FUSB3601_fcp_stop_charge_config(void)
{
#ifdef CONFIG_DIRECT_CHARGER
	FUSB3601_clear_scp_event1();
#endif
    return 0;
}
int FUSB3601_vout_enable(int enable)
{
	int ret;
	FSC_U8 data = 0;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port *port;

	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		pr_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL3,&data);
	hwlog_info("%s:FM_CONTROL3 befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (enable) {
		data =FUSB3601_VOUT_ENABLE;
	} else {
		data =FUSB3601_VOUT_DISABLE;
	}
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_FM_CONTROL3, 1, &data);
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL3,&data);
	hwlog_info("%s:FM_CONTROL3 after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;

}
int FUSB3601_msw_enable(int enable)
{
	int ret;
	FSC_U8 data;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL1,&data);
	hwlog_info("%s:FM_CONTROL1 befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (enable) {
		data =FUSB3601_MSM_EN_HIGH;
	} else {
		data =FUSB3601_MSM_EN_LOW;
	}
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_FM_CONTROL1, 1, &data);
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_FM_CONTROL1,&data);
	hwlog_info("%s:FM_CONTROL1 after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;
}

static int FUSB3601_chip_reset_nothing(void)
{
	return 0;
}
static int FUSB3601_fcp_adapter_reset(void)
{
	int ret = 0;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_ENABLE2,&data);
	if (!ret) {
		return -1;
	}
	hwlog_info("%s\n", __func__);
	data |= FUSB3601_MST_RESET_MSK;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
	if (!ret) {
		return -1;
	}
	data &= ~FUSB3601_MST_RESET_MSK;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
	if (!ret) {
		return -1;
	}
	FUSB3601_core_redo_bc12(&chip->port);
	return 0;
}
static int FUSB3601_dcd_timout_disable(void)
{
	int ret;
	FSC_U8 data;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL1,&data);
	hwlog_info("%s:reg befor writing is : [0x%x], ret = %d\n", __func__, data,ret);
	data &= FUSB3601_DCD_TIMEOUT_DISABLE;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_MUS_CONTROL1, 1, &data);
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL1,&data);
	hwlog_info("%s:reg after writing is : [0x%x], ret = %d\n", __func__, data,ret);
	if (ret)
		return 0;
	else
		return -1;
}
static int FUSB3601_clear_event2_and_event3(void)
{
	FSC_U8 reg_val2 = 0;
	FSC_U8 reg_val3 = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
	ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_2, 1, &reg_val2);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_3, 1, &reg_val3);
	if (ret)
	    return 0;
	else {
	    hwlog_info("%s:i2c error\n", __func__);
	    return -1;
	}
}
void FUSB3601_dump_register(void)
{
	FSC_U8 i = 0;
	FSC_U8 data = 0;
	int ret;
	for(i = 0x00; i <= 0x1f; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x23; i <= 0x29; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x2e; i <= 0x33; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x50; i <= 0x53; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x70; i <= 0x79; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x80; i <= 0x88; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0x90; i <= 0x9d; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xa0; i <= 0xb2; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xc0; i <= 0xd0; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xd1; i <= 0xe0; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
	for(i = 0xe1; i <= 0xea; ++i) {
		ret = FUSB3601_fusb_I2C_ReadData(i,&data);
		hwlog_info("%s : register[0x%x] = 0x%x\n",__func__, i, data);
	}
}
static void FUSB3601_clear_tx_buffer(void)
{
	FSC_U8 data = 0;
	int ret;

	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_TX0,1, &data);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR,1, &data);
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD,1, &data);
	(void)ret;
}
static int FUSB3601_accp_transfer_check(void)
{
	FSC_U8 reg_val2 = 0;
	FSC_U8 reg_val3 = 0;
	int i =0;
	int ret;

	do{
		usleep_range(30000,31000);
		ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2,&reg_val2);
		hwlog_info("%s:FUSB3601_SCP_EVENT_2 = 0x%x\n", __func__,reg_val2);
		ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_3,&reg_val3);
		hwlog_info("%s:FUSB3601_SCP_EVENT_3 = 0x%x\n", __func__,reg_val3);
		if (!ret) {
		    hwlog_info("%s:read  error\n", __func__);
		    return -1;
		}
		i++;
	}while(i < ACCP_TRANSFER_POLLING_RETRY_TIMES && (reg_val2 & SCP_ACK_AND_NACK_MASK) == 0 && reg_val3 == 0);

	/*W1C for event2 and event3*/
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_2, 1, &reg_val2);
	ret |= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_EVENT_3, 1, &reg_val3);

	if(i >= ACCP_TRANSFER_POLLING_RETRY_TIMES) {
		hwlog_info("%s : read accp interrupt time out,total time is %d ms\n",__func__,i*10);
		FUSB3601_dump_register();
		FUSB3601_clear_tx_buffer();
	}

	/*if something  changed print reg info */
	if((reg_val2 & SCP_ACK_MASK) && (reg_val3 == 0)) {
		/*succeed*/
		hwlog_info("%s:succ!\n", __func__);
		return 0;
	} else {
		hwlog_err("%s : event2=0x%x,event3=0x%x\n",__func__,reg_val2,reg_val3);
		return -1;
	}
}
/****************************************************************************
  Function:     FUSB3601_accp_adapter_reg_read
  Description:  read adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_accp_adapter_reg_read(int* val, int reg)
{
	int ret;
	int i;
	FSC_U8 addr;
	FSC_U8 data;
	FSC_U8 cmd = ACCP_CMD_SBRRD;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	if (reg > MAX_U8 || reg < 0) {
		hwlog_err("%s: reg addr = 0x%x\n", __func__, reg);
		return -1;
	}
	addr = (FSC_U8)reg;

	mutex_lock(&FUSB3601_accp_adaptor_reg_lock);

	hwlog_info("%s: reg_addr = 0x%x\n", __func__, reg);
	for (i = 0; i< FCP_RETRY_MAX_TIMES; i++) {
		/*before send cmd, clear event2 and event3*/
		ret = FUSB3601_clear_event2_and_event3();
		if (ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR, 1, &addr);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD, 1, &cmd);
		if (!ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		if (0 == FUSB3601_accp_transfer_check()) {
			msleep(20);
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_ACK_RX,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX1,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX0,&data);
			if (!ret) {
				mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
				return -1;
			} else {
				*val = data;
				mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
				hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
				return 0;
			}
		} else {
			msleep(20);
		}
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
	if (ret) {
		hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
		if (data & FUSB3601_SCP_B_DETECT) {
			FUSB3601_core_redo_bc12(&chip->port);
		}
	}
	mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
	return -1;
}

/****************************************************************************
  Function:     FUSB3601_accp_adapter_reg_write
  Description:  write value into the adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_accp_adapter_reg_write(int val, int reg)
{
	int ret;
	int i;
	FSC_U8 addr;
	FSC_U8 data;
	FSC_U8 cmd = ACCP_CMD_SBRWR;

	hwlog_info("%s: reg_addr = 0x%x\n", __func__, reg);
	if (reg > MAX_U8 || reg < 0 || val > MAX_U8 || val < 0) {
		hwlog_err("%s: reg addr = 0x%x data = 0x%x\n", __func__, reg, val);
		return -1;
	}
	addr = (FSC_U8)reg;
	data = (FSC_U8)val;

	mutex_lock(&FUSB3601_accp_adaptor_reg_lock);
	for (i = 0; i< FCP_RETRY_MAX_TIMES; i++) {
		/*before send cmd, clear event2 and event3*/
		ret = FUSB3601_clear_event2_and_event3();
		if (ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_TX0, 1, &data);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_ADDR, 1, &addr);
		ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_REG_ACCP_RT_CMD, 1, &cmd);
		if (!ret) {
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return -1;
		}
		if (0 == FUSB3601_accp_transfer_check()) {
			msleep(20);
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_ACK_RX,&data);
			ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_REG_ACCP_RT_RX0,&data);
			mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
			return 0;
		}
		else {
			msleep(20);
		}
	}
	mutex_unlock(&FUSB3601_accp_adaptor_reg_lock);
	return -1;
}

static int FUSB3601_scp_adapter_reg_read(int* val, int reg)
{
	int ret;

	if (FUSB3601_scp_error_flag) {
		hwlog_err("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
		return -1;
	}
	ret = FUSB3601_accp_adapter_reg_read(val, reg);
	if (ret) {
		hwlog_err("%s : error reg = %d \n",__func__,reg);
		FUSB3601_scp_error_flag = 1;
		return -1;
	}
	return 0;
}

static int FUSB3601_scp_adapter_reg_write(int val, int reg)
{
	int ret;

	if (FUSB3601_scp_error_flag) {
		hwlog_err("%s : scp timeout happened ,do not write reg = %d \n",__func__,reg);
		return -1;
	}
	ret = FUSB3601_accp_adapter_reg_write(val, reg);
	if (ret) {
		hwlog_err("%s : error reg = %d \n",__func__,reg);
		FUSB3601_scp_error_flag = 1;
		return -1;
	}
	return 0;
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
static int FUSB3601_accp_adapter_detect(void)
{
	int ret;
	int i;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_DEVICE_TYPE,&data);
	if (ret) {
		if (data & FUSB3601_DCP_DETECT) {
			/*if DCP is detected by superswitch, no need to redo bc1.2
			 go to scp detection directly*/
			hwlog_info("%s, DEVICE_TYPE = 0x%x, DCP detected by superswitch\n", __func__,data);
		} else {
			/*if no dcp is detected, then redo bc1.2 and return fail ,
			 for the next loop 30 seconds later, we will read DEVICE_TYPE
			 register again* */
			//redo_bc12();
			FUSB3601_core_redo_bc12(&chip->port);
			hwlog_info("%s,DEVICE_TYPE = 0x%x\n", __func__,data);
			return 1;
		}
	} else {
		hwlog_info("%s,%d\n", __func__, __LINE__);
		return -1;
	}
	for (i = 0; i < ACCP_DETECT_MAX_COUT; i++)
	{
		ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
		if (!ret) {
			return -1;
		}
		hwlog_info("%s,%d,data = 0x%x\n", __func__, __LINE__,data);
		if (data & FUSB3601_SCP_OR_FCP_DETECT) {
			msleep(150);
			return 0;
		}
		msleep(ACCP_POLL_TIME);
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_1, &data);
	if (ret) {
		if (data == FUSB3601_SCP_EVENT1_CC_PLGIN_DPDN_PLGIN) {
			ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_SCP_EVENT_2, &data);
			if (ret) {
				if (data & FUSB3601_SCP_EVENT2_ACK) {
					data = 0x00;
					ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE1, 1, &data);
					if (ret) {
						return -1;
					}
				}
			}
		}
	}
	FUSB3601_core_redo_bc12_limited(&chip->port);
	return -1;
}
static int FUSB3601_fcp_adapter_detect(void)
{
	int ret;
#ifdef CONFIG_DIRECT_CHARGER
	int val;
#endif
	ret = FUSB3601_accp_adapter_detect();
	if (ACCP_ADAPTOR_DETECT_OTHER == ret) {
		hwlog_info("fcp adapter other detect\n");
		return FCP_ADAPTER_DETECT_OTHER;
	}
	if (ACCP_ADAPTOR_DETECT_FAIL == ret) {
		hwlog_info("fcp adapter detect fail\n");
		return FCP_ADAPTER_DETECT_FAIL;
	}
#ifdef CONFIG_DIRECT_CHARGER
	if (FUSB3601_is_support_scp()) {
		return FCP_ADAPTER_DETECT_SUCC;
	}
	ret = FUSB3601_accp_adapter_reg_read(&val, SCP_ADP_TYPE);
	if(ret) {
		hwlog_err("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
		return FCP_ADAPTER_DETECT_SUCC;
	}
	return FCP_ADAPTER_DETECT_OTHER;
#else
	return FCP_ADAPTER_DETECT_SUCC;
#endif
}
/****************************************************************************
  Function:     FUSB3601_fcp_get_adapter_output_vol
  Description:  get fcp output vol
  Input:        NA.
  Output:       fcp output vol(5V/9V/12V)
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_fcp_get_adapter_output_vol(int *vol)
{
    int num = 0;
    int output_vol = 0;
    int ret =0;

    /*get adapter vol list number,exclude 5V*/
    ret = FUSB3601_accp_adapter_reg_read(&num, FCP_SLAVE_REG_DISCRETE_CAPABILITIES);
    /*currently,fcp only support three out vol config(5v/9v/12v)*/
    if (ret || num > FCP_MAX_OUTPUT_VOL_NUM)
    {
        hwlog_err("%s: vout list support err, reg[0x21] = %d.\n", __func__, num);
        return -1;
    }

    /*get max out vol value*/
   ret = FUSB3601_accp_adapter_reg_read(&output_vol, FCP_SLAVE_REG_DISCRETE_OUT_V(num));
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
  Function:     FUSB3601_fcp_set_adapter_output_vol
  Description:  set fcp adapter output vol
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int FUSB3601_fcp_set_adapter_output_vol(int *output_vol)
{
    int val = 0;
    int vol = 0;
    int ret = 0;
    int reg_val2;
    FSC_U8 data;

    /*read ID OUTI , for identify huawei adapter*/
    ret = FUSB3601_accp_adapter_reg_read(&val, FCP_SLAVE_REG_ID_OUT0);
    if(ret < 0)
    {
        hwlog_err("%s: adapter ID OUTI read failed, ret is %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("%s: id out reg[0x4] = %d.\n", __func__, val);

    /*get adapter max output vol value*/
    ret = FUSB3601_fcp_get_adapter_output_vol(&vol);
    if(ret < 0)
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
    ret |= FUSB3601_accp_adapter_reg_write(vol, FCP_SLAVE_REG_VOUT_CONFIG);
    ret |= FUSB3601_accp_adapter_reg_read(&val, FCP_SLAVE_REG_VOUT_CONFIG);
    hwlog_info("%s: vout config reg[0x2c] = %d.\n", __func__, val);
    if(ret < 0 ||val != vol )
    {
        hwlog_err("%s:out vol config err, reg[0x2c] = %d.\n", __func__, val);
        return -1;
    }

    ret = FUSB3601_accp_adapter_reg_write(FCP_SLAVE_SET_VOUT, FCP_SLAVE_REG_OUTPUT_CONTROL);
    if(ret < 0)
    {
        hwlog_err("%s : enable adapter output voltage failed \n ",__func__);
        return -1;
    }
    hwlog_info("fcp adapter output vol set ok.\n");
    return 0;
}

/****************************************************************************
  Function:     FUSB3601_fcp_get_adapter_max_power
  Description:  get fcp adpter max power
  Input:        NA.
  Output:       NA
  Return:       MAX POWER(W)
***************************************************************************/
static int FUSB3601_fcp_get_adapter_max_power(int *max_power)
{
    int reg_val = 0;
    int ret =0;

    /*read max power*/
    ret = FUSB3601_accp_adapter_reg_read(&reg_val, FCP_SLAVE_REG_MAX_PWR);
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
*  Function:       FUSB3601_fcp_get_adapter_output_current
*  Discription:    fcp get the output current from adapter max power and output vol
*  Parameters:     NA
*  return value:  input_current(MA)
**********************************************************/
static int FUSB3601_fcp_get_adapter_output_current(void)
{
    int output_current = 0;
    int output_vol = 0;
    int max_power = 0;
    int ret =0;

    ret |= FUSB3601_fcp_get_adapter_output_vol(&output_vol);
    ret |= FUSB3601_fcp_get_adapter_max_power(&max_power);
    if (ret != 0 || 0 == output_vol)
    {
        hwlog_err("%s : output current read failed \n",__func__);
        return -1;
    }
    output_current = max_power*1000/output_vol;
    hwlog_info("%s: output current = %d.\n", __func__, output_current);
    return output_current;
}

/**********************************************************
*  Function:       FUSB3601_is_support_fcp
*  Discription:    check whether support fcp
*  Parameters:     NA
*  return value:   0:support
                  -1:not support
**********************************************************/
static int FUSB3601_is_support_fcp(void)
{
	return 0;
}
/**********************************************************
*  Function:       fcp_adapter_status_check
*  Discription:    when in fcp status ,it will check adapter reg status
*  Parameters:     NA
*  return value: 0:status ok ;FCP_ADAPTER_OTEMP:over temp;FCP_ADAPTER_OCURRENT: over current;FCP_ADAPTER_OVLT: over ovl;
**********************************************************/
static int FUSB3601_fcp_read_adapter_status (void)
{
    int val = 0,ret =0;
    ret = FUSB3601_accp_adapter_reg_read(&val, FCP_ADAPTER_STATUS);
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
static int FUSB3601_scp_adaptor_detect(void)
{
    int ret;
    int val;

    FUSB3601_scp_error_flag = 0;
    ret = FUSB3601_accp_adapter_detect();
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
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_ADP_TYPE);
    if(ret)
    {
        hwlog_err("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
        return SCP_ADAPTOR_DETECT_OTHER;
    }
    hwlog_info("%s : read SCP_ADP_TYPE val = %d \n",__func__,val);
    if ((val & SCP_ADP_TYPE_B_MASK) == SCP_ADP_TYPE_B)
    {
        hwlog_info("scp type B adapter detect\n ");
        ret = FUSB3601_scp_adapter_reg_read(&val, SCP_B_ADP_TYPE);
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
static int FUSB3601_scp_output_mode_enable(int enable)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_OUTPUT_MODE_MASK);
    val |= enable ? SCP_OUTPUT_MODE_ENABLE:SCP_OUTPUT_MODE_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}

static int FUSB3601_scp_adaptor_output_enable(int enable)
{
    int val;
    int ret;

    ret = FUSB3601_scp_output_mode_enable(1);
    if(ret)
    {
        hwlog_err("%s : scp output mode enable failed ,ret = %d \n",__func__,ret);
        return -1;
    }

    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_OUTPUT_MASK);
    val |= enable ? SCP_OUTPUT_ENABLE:SCP_OUTPUT_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int FUSB3601_scp_adaptor_reg_reset(int enable)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_ADAPTOR_RESET_MASK);
    val |= enable ? SCP_ADAPTOR_RESET_ENABLE:SCP_ADAPTOR_RESET_DISABLE;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}


static int FUSB3601_is_support_scp(void)
{
    return 0;
}
static int FUSB3601_scp_config_iset_boundary(int iboundary)
{
    int val;
    int ret;

    /*high byte store in low address*/
    val = (iboundary >> BITS_PER_BYTE) & 0xff;
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_L);
    if (ret)
        return ret;
    /*low byte store in high address*/
    val = iboundary & 0xff;
    ret |= FUSB3601_scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_H);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
    }
    return ret;

}
static int FUSB3601_scp_config_vset_boundary(int vboundary)
{
    int val;
    int ret;

    /*high byte store in low address*/
    hwlog_info("[%s], %d\n", __func__, __LINE__);
    val = (vboundary >> BITS_PER_BYTE) & 0xff;
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_L);
    if (ret)
        return ret;
    /*low byte store in high address*/
    val = vboundary & 0xff;
    ret |= FUSB3601_scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_H);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
    }
    hwlog_info("[%s], %d\n", __func__, __LINE__);
    return ret;

}
static int FUSB3601_scp_set_adaptor_voltage(int vol)
{
    int val;
    int ret;

    val = vol - VSSET_OFFSET;
    val = val / VSSET_STEP;
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_VSSET);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int FUSB3601_scp_set_watchdog_timer(int second)
{
    int val;
    int ret;

    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_WATCHDOG_MASK);
    val |= (second * SCP_WATCHDOG_BITS_PER_SECOND) & SCP_WATCHDOG_MASK; /*1 bit means 0.5 second*/
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_CTRL_BYTE1);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int FUSB3601_scp_set_dp_delitch(void)
{
    int val;
    int ret;

    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val befor is %d \n", __func__, val);
    val &= ~(SCP_DP_DELITCH_MASK);
    val |= SCP_DP_DELITCH_5_MS;
    hwlog_info("[%s]val after is %d \n", __func__, val);
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_CTRL_BYTE1);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}

static int FUSB3601_scp_init(struct scp_init_data * sid)
{
	int ret;
	int val;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;

	FUSB3601_scp_error_flag = 0;
	if (!chip) {
		hwlog_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		hwlog_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}
	FUSB3601_PDDisable(port);
	port->registers_.AlertMskH.M_VBUS_ALRM_LO= 0;
	FUSB3601_WriteRegisters(port, regALERTMSKH, 1);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
	if (get_dpd_enable()) {
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
		ret = FUSB3601_scp_get_adapter_vendor_id();
		if (IWATT_ADAPTER == ret || WELTREND_ADAPTER == ret || ID0X32_ADAPTER == ret) {
			usb_analog_hs_plug_in_out_handle(DIRECT_CHARGE_IN);
			hwlog_info("%s :  config rd on Dm for IWATT\n ",__func__);
		}
#endif
		data = FUSB3601_SCP_ENABLE2_INIT;
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
		FUSB3601_set_vbus_detach(port, VBUS_DETACH_DISABLE);
		FUSB3601_ReadRegister(port, regFM_CONTROL4);
		hwlog_info("%s:FM_CONTROL4 after writing is : [0x%x]\n", __func__, port->registers_.FMControl4);
	}
	ret = FUSB3601_scp_output_mode_enable(sid->scp_mode_enable);
	if(ret)
		return ret;
	if (RICHTEK_ADAPTER == FUSB3601_scp_get_adapter_vendor_id()) {
		FUSB3601_scp_set_dp_delitch();
	}
	ret = FUSB3601_scp_set_watchdog_timer(sid->watchdog_timer);
	if(ret)
		return ret;
	ret = FUSB3601_scp_config_vset_boundary(sid->vset_boundary);
	if(ret)
		return ret;
	ret = FUSB3601_scp_config_iset_boundary(sid->iset_boundary);
	if(ret)
		return ret;
	ret = FUSB3601_scp_set_adaptor_voltage(sid->init_adaptor_voltage);
	if(ret)
		return ret;
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
	if(ret)
		return ret;
	hwlog_info("%s : CTRL_BYTE0 = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
	if(ret)
		return ret;
	hwlog_info("%s : CTRL_BYTE1 = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
	if(ret)
		return ret;
	hwlog_info("%s : STATUS_BYTE0 = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_STATUS_BYTE1);
	if(ret)
		return ret;
	hwlog_info("%s : STATUS_BYTE1 = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_H);
	if(ret)
		return ret;
	hwlog_info("%s : VSET_BOUNDARY_H = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_L);
	if(ret)
		return ret;
	hwlog_info("%s : VSET_BOUNDARY_L = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_H);
	if(ret)
		return ret;
	hwlog_info("%s : ISET_BOUNDARY_H = 0x%x \n ",__func__, val);
	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_L);
	if(ret)
		return ret;
	hwlog_info("%s : ISET_BOUNDARY_L = 0x%x \n ",__func__, val);
	return ret;
}
static int FUSB3601_scp_exit(struct direct_charge_device* di)
{
	int ret;
	FSC_U8 data;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;
	if (!chip) {
		hwlog_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		hwlog_err("FUSB  %s - port structure is NULL!\n", __func__);
		return -1;
	}
	ret = FUSB3601_scp_output_mode_enable(0);
	FUSB3601_vout_enable(1);
	if (get_dpd_enable()) {
#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
		hwlog_info("%s :  disable rd on Dm\n ",__func__);
		usb_analog_hs_plug_in_out_handle(DIRECT_CHARGE_OUT);
#endif
		data = FUSB3601_DPD_DISABLE;
		ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE2, 1, &data);
		FUSB3601_set_vbus_detach(port, VBUS_DETACH_ENABLE);
		FUSB3601_ReadRegister(port, regFM_CONTROL4);
		hwlog_info("%s:FM_CONTROL4 after writing is : [0x%x]\n", __func__, port->registers_.FMControl4);
		state_machine_need_resched = 1;
		queue_work(chip->highpri_wq,&chip->sm_worker);
	}
	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			ret  |= FUSB3601_scp_adaptor_reg_reset(1);
			break;
		default:
			hwlog_info("%s:not iWatt\n",__func__);
	}
	msleep(50);
	hwlog_err("%s\n",__func__);
	FUSB3601_scp_error_flag = 0;
	return 0;
}
static int FUSB3601_scp_get_adaptor_voltage(void)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_SREAD_VOUT);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val * SCP_SREAD_VOUT_STEP + SCP_SREAD_VOUT_OFFSET;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}
static int FUSB3601_scp_set_adaptor_current(int cur)
{
    int val;
    int ret;

    val = cur / ISSET_STEP;
    ret = FUSB3601_scp_adapter_reg_write(val, SCP_ISSET);
    if(ret < 0)
    {
        hwlog_err("%s : failed \n ",__func__);
        return -1;
    }
    return 0;
}
static int FUSB3601_scp_get_adaptor_current(void)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_SREAD_IOUT);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val*SCP_SREAD_IOUT_STEP;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}
static int FUSB3601_scp_get_adaptor_current_set(void)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_ISSET);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    val = val*ISSET_STEP;
    hwlog_info("[%s]val is %d \n", __func__, val);
    return val;
}

static int FUSB3601_scp_get_adaptor_max_current(void)
{
    int val;
    int ret;
    int A;
    int B;
    int rs;

    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_MAX_IOUT);
    if(ret)
    {
        hwlog_err("%s : read MAX_IOUT failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]max_iout reg is %d \n", __func__, val);
    A = (SCP_MAX_IOUT_A_MASK & val) >> SCP_MAX_IOUT_A_SHIFT;
    B = SCP_MAX_IOUT_B_MASK & val;
    switch (A){
	case MAX_IOUT_EXP_0:
		A = TEN_EXP_0;
		break;
	case MAX_IOUT_EXP_1:
		A = TEN_EXP_1;
		break;
	case MAX_IOUT_EXP_2:
		A = TEN_EXP_2;
		break;
	case MAX_IOUT_EXP_3:
		A = TEN_EXP_3;
		break;
	default:
	    return -1;
    }
    rs = B*A;
    hwlog_info("[%s]MAX IOUT initial is %d \n", __func__, rs);
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_SSTS);
    if(ret)
    {
        hwlog_err("%s : read SSTS failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]ssts reg is %d \n", __func__, val);
    B = (SCP_SSTS_B_MASK & val) >> SCP_SSTS_B_SHIFT;
    A = SCP_SSTS_A_MASK & val;
    if (DROP_POWER_FLAG == B)
    {
	rs = rs * A / DROP_POWER_FACTOR;
    }
    hwlog_info("[%s]MAX IOUT final is %d \n", __func__, rs);
    return rs;
}

static int FUSB3601_scp_get_adaptor_temp(int* temp)
{
    int val = 0;
    int ret;

    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_INSIDE_TMP);
    if(ret)
    {
        hwlog_err("%s : read failed ,ret = %d \n",__func__,ret);
        return -1;
    }
    hwlog_info("[%s]val is %d \n", __func__, val);
    *temp = val;

    return 0;
}
static int FUSB3601_scp_cable_detect(void)
{
    int val;
    int ret;
    ret = FUSB3601_scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
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
static int FUSB3601_scp_stop_charge_config(void)
{
    return 0;
}
static int FUSB3601_scp_get_adaptor_status(void)
{
    return 0;
}
static int FUSB3601_scp_get_adaptor_info(void* info)
{
    int ret;
    struct adaptor_info* p = (struct adaptor_info*)info;

    ret = FUSB3601_scp_adapter_reg_read(&(p->b_adp_type), SCP_B_ADP_TYPE);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->vendor_id_h), SCP_VENDOR_ID_H);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->vendor_id_l), SCP_VENDOR_ID_L);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->module_id_h), SCP_MODULE_ID_H);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->module_id_l), SCP_MODULE_ID_L);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->serrial_no_h), SCP_SERRIAL_NO_H);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->serrial_no_l), SCP_SERRIAL_NO_L);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->pchip_id), SCP_PCHIP_ID);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->hwver), SCP_HWVER);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->fwver_h), SCP_FWVER_H);
    if(ret)
        return ret;
    ret = FUSB3601_scp_adapter_reg_read(&(p->fwver_l), SCP_FWVER_L);

    return ret;
}
static int FUSB3601_scp_get_adapter_vendor_id(void)
{
	int val = 0;
	int ret;

	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_PCHIP_ID);
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
		case VENDOR_ID_WELTREND:
			hwlog_info("[%s]adapter is weltrend \n", __func__);
			return WELTREND_ADAPTER;
		case VENDOR_ID_0X32:
			hwlog_info("[%s]adapter id is 0x32 \n", __func__);
			return ID0X32_ADAPTER;
		default:
			hwlog_info("[%s]this adaptor vendor id is not found!\n", __func__);
			return val;
	}
}
static int FUSB3601_scp_get_usb_port_leakage_current_info(void)
{
	int val = 0;
	int ret;

	ret = FUSB3601_scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
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
static int FUSB3601_scp_get_chip_status(void)
{
    return 0;
}
static int FUSB3601_scp_adaptor_reset(void)
{
	return 0;
}
static enum hisi_charger_type FUSB3601_get_charger_type(void)
{
	enum hisi_charger_type charger_type = CHARGER_TYPE_NONE;
	FSC_U8 data;
	int ret;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_DEVICE_TYPE,&data);
	if (ret) {
		if (data & FUSB3601_DCP_DETECT) {
			hwlog_info("%s, DEVICE_TYPE = 0x%x, DCP detected by superswitch\n", __func__,data);
			charger_type = CHARGER_TYPE_DCP;
		} else {
			hwlog_info("%s,DEVICE_TYPE = 0x%x\n", __func__,data);
			FUSB3601_core_redo_bc12(&chip->port);
		}
	} else {
		hwlog_info("%s,error!\n", __func__);
	}

    return charger_type;
}

static void FUSB3601_scp_set_direct_charge_mode(int mode)
{
	hwlog_info("[%s]mode is 0x%x \n", __func__, mode);
	return;
}

static int FUSB3601_scp_get_adaptor_type(void)
{
	return LVC_MODE;
}

struct smart_charge_ops FUSB3601_scp_ops = {
    .is_support_scp = FUSB3601_is_support_scp,
    .scp_init = FUSB3601_scp_init,
    .scp_exit = FUSB3601_scp_exit,
    .scp_adaptor_detect = FUSB3601_scp_adaptor_detect,
    .scp_set_adaptor_voltage = FUSB3601_scp_set_adaptor_voltage,
    .scp_get_adaptor_voltage = FUSB3601_scp_get_adaptor_voltage,
    .scp_set_adaptor_current = FUSB3601_scp_set_adaptor_current,
    .scp_get_adaptor_current = FUSB3601_scp_get_adaptor_current,
    .scp_get_adaptor_current_set = FUSB3601_scp_get_adaptor_current_set,
    .scp_get_adaptor_max_current = FUSB3601_scp_get_adaptor_max_current,
    .scp_adaptor_reset = FUSB3601_scp_adaptor_reset,
    .scp_adaptor_output_enable = FUSB3601_scp_adaptor_output_enable,
    .scp_chip_reset = FUSB3601_chip_reset_nothing,
    .scp_stop_charge_config = FUSB3601_scp_stop_charge_config,
    .is_scp_charger_type = NULL,
    .scp_get_adaptor_status = FUSB3601_scp_get_adaptor_status,
    .scp_get_adaptor_info = FUSB3601_scp_get_adaptor_info,
    .scp_get_chip_status = FUSB3601_scp_get_chip_status,
    .scp_cable_detect = FUSB3601_scp_cable_detect,
    .scp_get_adaptor_temp = FUSB3601_scp_get_adaptor_temp,
    .scp_get_adapter_vendor_id = FUSB3601_scp_get_adapter_vendor_id,
    .scp_get_usb_port_leakage_current_info = FUSB3601_scp_get_usb_port_leakage_current_info,
    .scp_set_direct_charge_mode = FUSB3601_scp_set_direct_charge_mode,
    .scp_get_adaptor_type = FUSB3601_scp_get_adaptor_type,
};
#endif
struct fcp_adapter_device_ops FUSB3601_fcp_ops = {
    .get_adapter_output_current = FUSB3601_fcp_get_adapter_output_current,
    .set_adapter_output_vol     = FUSB3601_fcp_set_adapter_output_vol,
    .detect_adapter             = FUSB3601_fcp_adapter_detect,
    .is_support_fcp             = FUSB3601_is_support_fcp,
    .switch_chip_reset          = FUSB3601_chip_reset_nothing,
    .fcp_adapter_reset          = FUSB3601_fcp_adapter_reset,
    .stop_charge_config        = FUSB3601_fcp_stop_charge_config,
    .is_fcp_charger_type    = NULL,
    .fcp_read_adapter_status = FUSB3601_fcp_read_adapter_status,
    .fcp_read_switch_status = NULL,
    .reg_dump = NULL,
};
struct charge_switch_ops FUSB3601_switch_ops = {
	.get_charger_type = FUSB3601_get_charger_type,
	.is_water_intrused = NULL,
};

#ifdef CONFIG_SUPERSWITCH_FSC
static int FUSB3601_chsw_set_wired_channel(int flag)
{
	int ret = 0;
	if (WIRED_CHANNEL_CUTOFF == flag) {
		hwlog_info("%s set fusb3601 en disable\n", __func__);
		ret = FUSB3601_vout_enable(0);
	} else {
		hwlog_info("%s set fusb3601 en enable\n", __func__);
		ret = FUSB3601_vout_enable(1);
	}
	if (!ret)
		wired_channel_status = flag;
	return ret;
}
static FUSB3601_chsw_get_wired_channel(void)
{
	return wired_channel_status;
}
static struct wired_chsw_device_ops chsw_ops = {
	.get_wired_channel = FUSB3601_chsw_get_wired_channel,
	.set_wired_channel = FUSB3601_chsw_set_wired_channel,
};
static int FUSB3601_wired_chsw_ops_register(void)
{
	int ret = 0;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (chip->use_super_switch_cutoff_wired_channel) {
		ret = wired_chsw_ops_register(&chsw_ops);
		if (ret) {
			hwlog_err("%s register fusb3601 switch ops failed!\n", __func__);
			return -1;
		}
		hwlog_info("%s fusb3601 switch ops register success\n", __func__);
	}
	hwlog_info("%s++\n", __func__);

	return ret;
}
#endif

void FUSB3601_scp_initialize(void)
{
	int ret;
	FSC_U8 data;

	data = FUSB3601_SCP_ENABLE1_INIT;
	ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_ENABLE1, 1, &data);
	data = FUSB3601_SCP_INT1_MASK_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_INT1_MASK, 1, &data);
	data = FUSB3601_SCP_INT2_MASK_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_SCP_INT2_MASK, 1, &data);
	data = FUSB3601_TIMER_SET1_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_TIMER_SET1, 1, &data);
	data = FUSB3601_TIMER_SET2_INIT;
	ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_TIMER_SET2, 1, &data);

	ret &= FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_INTERRUPT_MASK,&data);
	if (ret) {
	    data &= FUSB3601_MUS_INTERRUPT_MASK_INIT;
	    ret &= FUSB3601_fusb_I2C_WriteData(FUSB3601_MUS_INTERRUPT_MASK, 1, &data);
	}
	if (!ret) {
	    hwlog_err(" %s:%d error!\n", __func__,__LINE__);
	}
	ret = FUSB3601_dcd_timout_disable();
	if (ret) {
	    hwlog_err(" %s:%d error!\n", __func__,__LINE__);
	}
}

void FUSB3601_charge_register_callback(void)
{
    hwlog_info(" %s++!\n", __func__);
    mutex_init(&FUSB3601_accp_detect_lock);
    mutex_init(&FUSB3601_accp_adaptor_reg_lock);

    if( 0 == FUSB3601_is_support_fcp() && 0 ==fcp_adapter_ops_register(&FUSB3601_fcp_ops))
    {
        hwlog_info(" fcp adapter ops register success!\n");
    }
#ifdef CONFIG_DIRECT_CHARGER
    if( 0 == FUSB3601_is_support_scp() && 0 ==scp_ops_register(&FUSB3601_scp_ops))
    {
        hwlog_info(" scp adapter ops register success!\n");
    }
#endif
    if(0 == charge_switch_ops_register(&FUSB3601_switch_ops))
    {
        hwlog_info(" charge switch ops register success!\n");
    }
#ifdef CONFIG_SUPERSWITCH_FSC
	FUSB3601_wired_chsw_ops_register();
#endif
	hwlog_info(" %s--!\n", __func__);
}
