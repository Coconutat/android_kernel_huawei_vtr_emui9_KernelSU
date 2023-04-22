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
#include "../Platform_Linux/platform_helpers.h"
#include "moisture_detection.h"
#include "core.h"
#include "../Platform_Linux/fusb3601_global.h"
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#include <huawei_platform/power/huawei_charger.h>

#define FUSB3601_MUS_INTERRUPT_MASK 0xd4
static int detect_finish_flag = 1;
static struct delayed_work m_work;
#define HWLOG_TAG moisture_detection
HWLOG_REGIST();
/****************************************************************************
  Function:     moisture_detection_enable
  Description:  enable detect moisture on typec port
  Input:         NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
#if 0
static int moisture_detection_enable(void)
{
	int ret;
	int vbus_val = 0;
	int cc1_stat = 0;
	int cc2_stat = 0;
	int cc_unattached = 0;
	FSC_U8 data;

	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_PWRSTAT,&data);
	if (ret) {
		hwlog_info("%s:PWRSTAT is: [0x%x]\n", __func__, data);
		vbus_val = data & FUSB3601_VBUS_VAL_MASK;
	} else {
		return -1;
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_CCSTAT,&data);
	if (ret) {
		hwlog_info("%s:CCSTAT is : [0x%x]\n", __func__, data);
		cc1_stat = data & FUSB3601_CC1_STAT_MASK;
		cc2_stat = data & FUSB3601_CC2_STAT_MASK;
		if ((FUSB3601_CC1_OPEN == cc1_stat) && (FUSB3601_CC2_OPEN == cc2_stat)) {
			cc_unattached = 1;
		} else {
			cc_unattached = 0;
		}
	} else {
		return -1;
	}
	if(vbus_val == FUSB3601_VBUS_NOT_CONNECTED && cc_unattached) {
		ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL2,&data);
		if (ret) {
			hwlog_info("%s:MUS_CONTROL2 before write is : [0x%x]\n", __func__, data);
			data |= FUSB3601_MUS_H2O_DET_EN;
			ret = FUSB3601_fusb_I2C_WriteData(FUSB3601_MUS_CONTROL2, 1, &data);
			if (ret) {
				ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_CONTROL2,&data);
				detect_finish_flag = 0;
				hwlog_info("%s:H2O_DET enabled, MUS_CONTROL2 after = 0x%x wait for interrupt\n", __func__,data);
				return 0;
			} else {
				return -1;
			}
		} else {
			return -1;
		}
	}
	hwlog_info("vbus and cc_state not ready for moisture_detection\n", __func__);
	return -1;
}
#endif
void moisture_detection_complete(void)
{
	int ret;
	FSC_U8 data;
	int vbus_val = 0;
	static int moisture_detected_already_reported = 0;
	static int moisture_detected_counter = 0;
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_VBUS_VOLTAGEL,&data);
	if (ret) {
		hwlog_info("%s:VBUS_VOLTAGEL is: [0x%x]\n", __func__, data);
		vbus_val += data;
	} else {
		return;
	}
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_VBUS_VOLTAGEH,&data);
	if (ret) {
		hwlog_info("%s:VBUS_VOLTAGEH is: [0x%x]\n", __func__, data);
		vbus_val += ((data & FUSB3601_VBUS_VOLTAGEH_VAL_MASK) << BITS_PER_BYTE);
	} else {
		return;
	}
	/*adc value is a 9 bits integer, so there is no overflow risk for multiplication*/
	vbus_val *= FUSB3601_VBUS_VOLTAGE_LSB_IN_ONE_TENTH_MV;
	vbus_val /= TEN;
	detect_finish_flag = 1;
	hwlog_info("%s:vbus voltage is: %d mv\n", __func__, vbus_val);
	if (vbus_val < MOISTURE_DETECTED_THRESHOLD) {
		moisture_detected_counter ++;
	} else {
		moisture_detected_counter = 0;
	}
	if (moisture_detected_counter >= MOISTURE_DETECTED_CNT_THRESHOLD) {
		hwlog_info("%s:moisture detected\n", __func__);
		if (!moisture_detected_already_reported) {
			send_water_intrused_event(true);
			moisture_detected_already_reported = 1;
		}
	} else {
		hwlog_info("%s:moisture not detected\n", __func__);
		if (moisture_detected_already_reported) {
			send_water_intrused_event(false);
			moisture_detected_already_reported = 0;
		}
	}
}

#if 0
static void monitor_work(struct work_struct *work)
{
	int ret;
	FSC_U8 data;
	ret = FUSB3601_fusb_I2C_ReadData(FUSB3601_MUS_INTERRUPT_MASK,&data);
	hwlog_info("%s:MUS_INTERRUPT_MASK : 0x%x mv\n", __func__, data);
	if (detect_finish_flag) {
		moisture_detection_enable();
	} else {
		hwlog_info(" %s: moisture_detection not finish!\n",__func__);
	}
	/*schedule_delayed_work(&m_work, msecs_to_jiffies(1000));*/
}
#endif
void moisture_detection_init(void)
{
    /*hwlog_info(" %s++!\n", __func__);
    INIT_DELAYED_WORK(&m_work, monitor_work);
    schedule_delayed_work(&m_work, msecs_to_jiffies(0));
    hwlog_info(" %s--!\n", __func__);*/
}

