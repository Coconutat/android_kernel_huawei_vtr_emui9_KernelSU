

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
#include <huawei_platform/usb/switch/switch_ap/switch_chip.h>
#include <linux/hisi/usb/hisi_usb.h>
#ifdef CONFIG_HDMI_K3
#include <../video/k3/hdmi/k3_hdmi.h>
#endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/wakelock.h>
#include <huawei_platform/log/hw_log.h>
#include <chipset_common/hwusb/hw_usb_rwswitch.h>
#include <huawei_platform/power/huawei_charger_sh.h>
#include <huawei_platform/usb/switch/switch_fsa9685.h>
#include <inputhub_route.h>
#include <protocol.h>
#include <sensor_info.h>

static struct mutex accp_detect_lock;
static struct mutex accp_adaptor_reg_lock;
static struct delayed_work   detach_delayed_work;
static struct wake_lock usb_switch_lock;
#ifdef CONFIG_FSA9685_DEBUG_FS
static int reg_locked = 1;
static char chip_regs[0x5c+2] = { 0 };
#endif

struct work_struct   g_intb_work_sh;
extern int switch_irq;
extern int g_iom3_state;
extern struct switch_platform_data switch_fsa9685_data;
extern void charge_wake_unlock(void);
static int is_sh_support_fcp(void);

static void usb_switch_wake_lock(void)
{
    if (!wake_lock_active(&usb_switch_lock)) {
        wake_lock(&usb_switch_lock);
        hwlog_info("usb switch wake lock\n");
    }
}
static void usb_switch_wake_unlock(void)
{
    if (wake_lock_active(&usb_switch_lock)) {
        wake_unlock(&usb_switch_lock);
        hwlog_info("usb switch wake unlock\n");
    }
}

/**********************************************************
*  Function:       bq25892_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fsa9685_write_reg(int reg, int val)
{
	uint8_t tx_buf[2];
	int ret;
	tx_buf[0] = (uint8_t)reg;
	tx_buf[1] = (uint8_t)val;
	ret = mcu_i2c_rw(switch_fsa9685_data.cfg.bus_num, (uint8_t)switch_fsa9685_data.cfg.i2c_address,
		tx_buf, 2, NULL, 0);
	if (ret < 0) {
		hwlog_err("fsa9685 write value %d to register %d fail!\n", val, reg);
	} else {
		hwlog_info("fsa9685 write value %d to register %d success!\n", val, reg);
#ifdef CONFIG_FSA9685_DEBUG_FS
		chip_regs[reg] = val;
#endif
	}
	return ret;
}

/**********************************************************
*  Function:       fsa9685_read_block
*  Discription:    register read block interface
*  Parameters:   reg:register name
*                      value:register value
			len:register number
*  return value:  0-sucess or others-fail
**********************************************************/
static int fsa9685_read_block(int reg, u8*value, unsigned len)
{
	int ret = mcu_i2c_rw(3, (uint8_t)switch_fsa9685_data.cfg.i2c_address,
		(uint8_t*)&reg,	1, value, len);
	if (ret < 0) {
		hwlog_err("fsa9685 read register %d fail!\n", reg);
	} else {
		hwlog_info("fsa9685 read value %d from register %d success!\n", *value, reg);
	}
	return ret;
}

/**********************************************************
*  Function:       fsa9685_read_reg
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fsa9685_read_reg(int reg)
{
	uint8_t value;
	int ret = -1;

	ret = fsa9685_read_block(reg, &value, 1);
	if (ret < 0) {
		return ret;
	} else {
	#ifdef CONFIG_FSA9685_DEBUG_FS
    		chip_regs[reg] = value;
	#endif
		return value;
	}
}

static int fsa9685_write_reg_mask(int reg, int value,int mask)
{
    int val=0,ret=0;

    val= fsa9685_read_reg(reg);
    if(val < 0)
    {
        return val;
    }
    val &= ~mask;
    val |=value & mask;
    ret = fsa9685_write_reg(reg,val);
    return ret;
}

static int fsa9685_manual_switch(int input_select)
{
    int value = 0, ret = 0;

    hwlog_info("%s: input_select = %d", __func__, input_select);
    /* Two switch not support USB2_ID*/
    if(switch_fsa9685_data.two_switch_flag && (FSA9685_USB2_ID_TO_IDBYPASS == input_select))
    {
        return 0;
    }
    switch (input_select){
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

    ret = fsa9685_write_reg(FSA9685_REG_MANUAL_SW_1, value);
    if ( ret < 0 ){
        ret = -ERR_FSA9685_REG_MANUAL_SW_1;
        hwlog_err("%s: write reg FSA9685_REG_MANUAL_SW_1 error!!! ret=%d\n", __func__, ret);
        return ret;
    }
    value = fsa9685_read_reg(FSA9685_REG_CONTROL);
    if (value < 0){
        ret = -ERR_FSA9685_READ_REG_CONTROL;
        hwlog_err("%s: read FSA9685_REG_CONTROL error!!! ret=%d\n", __func__, ret);
        return ret;
    }

    value &= (~FSA9685_MANUAL_SW); // 0: manual switching
    ret = fsa9685_write_reg(FSA9685_REG_CONTROL, value);
    if ( ret < 0 ){
        ret = -ERR_FSA9685_WRITE_REG_CONTROL;
        hwlog_err("%s: write FSA9685_REG_CONTROL error!!! ret=%d\n", __func__, ret);
        return ret;
    }
    return 0;
}

static int fsa9685_manual_detach_work(void)
{
    int ret = 0;

    schedule_delayed_work(&detach_delayed_work, msecs_to_jiffies(20));
    hwlog_info("%s: ------end.\n", __func__);
    return ret;
}
static void fsa9685_detach_work(struct work_struct *work)
{
    int ret;
    hwlog_info("%s: ------entry.\n", __func__);

    ret = fsa9685_read_reg(FSA9685_REG_DETACH_CONTROL);
    if ( ret < 0 ){
        hwlog_err("%s: read FSA9685_REG_DETACH_CONTROL error!!! ret=%d", __func__, ret);
        return;
    }

    ret = fsa9685_write_reg(FSA9685_REG_DETACH_CONTROL, 1);
    if ( ret < 0 ){
        hwlog_err("%s: write FSA9685_REG_DETACH_CONTROL error!!! ret=%d", __func__, ret);
        return;
    }

    hwlog_info("%s: ------end.\n", __func__);
    return;
}

static int fsa9685_dcd_timeout(bool enable_flag)
{
	int reg_val = 0;
	int ret;

	reg_val = fsa9685_read_reg(FSA9685_REG_DEVICE_ID);
	/*we need 9688c 9683 except 9688 not support enable dcd time out */
	if(FSA9688_VERSION_ID == ((reg_val & FAS9685_VERSION_ID_BIT_MASK) >> FAS9685_VERSION_ID_BIT_SHIFT)){
		return -1;
	}
	ret = fsa9685_write_reg_mask(FSA9685_REG_CONTROL2,enable_flag,FSA9685_DCD_TIME_OUT_MASK);
	if(ret < 0){
		hwlog_err("%s:write fsa9688c DCD enable_flag error!!!\n",__func__);
		return -1;
	}
	fsa9685_read_reg(FSA9685_REG_CONTROL2);//temp
	hwlog_info("%s:write fsa9688c DCD enable_flag is:%d!!!\n",__func__,enable_flag);
	return 0;
}

static void fsa9685_intb_work(struct work_struct *work);
irqreturn_t fsa9685_irq_sh_handler(int irq, void *dev_id)
{
    int gpio_value;

    usb_switch_wake_lock();
    gpio_value = gpio_get_value(switch_fsa9685_data.gpio_intb);
    if(gpio_value==1)
        hwlog_err("%s: intb high when interrupt occured!!!\n", __func__);

    schedule_work(&g_intb_work_sh);

    hwlog_info("%s: ------end. gpio_value=%d\n", __func__, gpio_value);
    return IRQ_HANDLED;
}
/****************************************************************************
  Function:     is_sh_fcp_charger_type
  Description:  after fcp detect ok,it will show it is fcp adapter
  Input:        void
  Output:       NA
  Return:        true:fcp adapter
                false: not fcp adapter
***************************************************************************/
static int is_sh_fcp_charger_type(void)//sysfs
{
    int reg_val = 0;

    if(is_sh_support_fcp())
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
    int val = 0;

    val = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
    if (val < 0)
    {
        hwlog_err("%s: read REG[%d] erro, val = %d, charger_type set to NONE\n",
                  __func__, FSA9685_REG_DEVICE_TYPE_1, val);
        return charger_type;
    }

    if (val & FSA9685_USB_DETECTED)
        charger_type = CHARGER_TYPE_SDP;
    else if(val & FSA9685_CDP_DETECTED)
        charger_type = CHARGER_TYPE_CDP;
    else if(val & FSA9685_DCP_DETECTED)
        charger_type = CHARGER_TYPE_DCP;
    else
        charger_type = CHARGER_TYPE_NONE;

    if((charger_type == CHARGER_TYPE_NONE) && is_sh_fcp_charger_type())
    {
       charger_type = CHARGER_TYPE_DCP;/*if is fcp ,report dcp,because when we detect fcp last time ,FSA9685_REG_DEVICE_TYPE_4 will be set */
       hwlog_info("%s:update charger type by device type4, charger type is:%d\n",__func__,charger_type);
    }

    return charger_type;
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
    reg_intrpt = fsa9685_read_reg(FSA9685_REG_INTERRUPT);
    vbus_status = fsa9685_read_reg(FSA9685_REG_VBUS_STATUS);
    hwlog_info("%s: read FSA9685_REG_INTERRUPT. reg_intrpt=0x%x\n", __func__, reg_intrpt);
    /* if support fcp ,disable fcp interrupt */
    if(!is_sh_support_fcp()
        &&((0xFF != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK1))
        ||(0xFF != fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT_MASK2))))
    {
        hwlog_info("disable fcp interrrupt again!!\n");
        ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, FSA9685_ACCP_OSC_ENABLE,FSA9685_ACCP_OSC_ENABLE);
        ret2 |= fsa9685_write_reg_mask(FSA9685_REG_CONTROL2, 0,FSA9685_DCD_TIME_OUT_MASK);
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
                if (switch_fsa9685_data.fsa9685_mhl_detect_disable == 1) {
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
                charge_type_dcp_detected_notify_sh();
            }
            if ((reg_dev_type1 & FSA9685_USBOTG_DETECTED) && switch_fsa9685_data.fsa9685_usbid_enable) {
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
            if ((otg_attach == 1) && switch_fsa9685_data.fsa9685_usbid_enable) {
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
            schedule_delayed_work(&detach_delayed_work, msecs_to_jiffies(0));
        } else {
            invalid_times = 0;
        }
    } else if ((ID_VALID == id_valid_status) &&
                (reg_intrpt & (FSA9685_ATTACH | FSA9685_RESERVED_ATTACH))) {
        invalid_times = 0;
    }

    if((USB_SWITCH_NEED_WAKE_UNLOCK == usb_switch_wakelock_flag) &&
            (0 == invalid_times)) {
        usb_switch_wake_unlock();
    }

OUT:
    hwlog_info("%s: ------end.\n", __func__);
    return;
}

#ifdef CONFIG_FSA9685_DEBUG_FS
static ssize_t dump_regs_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    const int regaddrs[] = {0x01, 0x02, 0x03, 0x04, 0x5, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d};
    const char str[] = "0123456789abcdef";
    int i = 0, index;
    char val = 0;
    for (i=0; i<0x60; i++) {
        if ((i%3)==2)
            buf[i]=' ';
        else
            buf[i] = 'x';
    }
    buf[0x5d] = '\n';
    buf[0x5e] = 0;
    buf[0x5f] = 0;
    if ( reg_locked != 0 ) {
        for (i=0; i<ARRAY_SIZE(regaddrs); i++) {
            if (regaddrs[i] != 0x03) {
                val = fsa9685_read_reg(regaddrs[i]);
                chip_regs[regaddrs[i]] = val;
            }
        }
    }

    for (i=0; i<ARRAY_SIZE(regaddrs); i++) {
        index = regaddrs[i];
        val = chip_regs[index];
            buf[3*index] = str[(val&0xf0)>>4];
        buf[3*index+1] = str[val&0x0f];
        buf[3*index+2] = ' ';
    }

    return 0x60;
}
static DEVICE_ATTR(dump_regs, S_IRUGO, dump_regs_show, NULL);
#endif

static ssize_t jigpin_ctrl_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
    int jig_val = JIG_PULL_DEFAULT_DOWN;
    int ret = 0;

    if (sscanf(buf, "%d", &jig_val) != 1) {
        hwlog_err("%s:write regs failed, invalid input!\n", __func__);
        return -1;
    }
    ret = fsa9685_write_reg_mask(FSA9685_REG_CONTROL, 0, FSA9685_MANUAL_SW);
    if (ret < 0) {
        hwlog_err("%s:write FSA9685_REG_CONTROL error!\n",__func__);
        return ret;
    }
    if (FSA9683_I2C_ADDR == switch_fsa9685_data.cfg.i2c_address
          || CBTL9689_I2C_ADDR == switch_fsa9685_data.cfg.i2c_address) {
        ret = fsa9685_write_reg_mask(FSA9685_REG_WD_CTRL,
                   FSA9685_WD_CTRL_JIG_MANUAL_EN,FSA9685_WD_CTRL_JIG_MANUAL_EN);
        if (ret < 0) {
            hwlog_err("%s:write FSA9685_REG_WD_CTRL error!!!\n", __func__);
            return ret;
        }
    }
    switch (jig_val) {
        case JIG_PULL_DEFAULT_DOWN:
            hwlog_info("%s:pull down jig pin to default state\n", __func__);
            if (FSA9683_I2C_ADDR == switch_fsa9685_data.cfg.i2c_address) {
                ret = fsa9685_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
                            FSA9683_REG_JIG_DEFAULT_DOWN, FSA9685_REG_JIG_MASK);
                if (ret < 0) {
                    hwlog_err("%s:write FSA9685_REG_MANUAL_SW_2 error!\n",__func__);
                    break;
                }
            } else {
                ret = fsa9685_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
                            FSA9685_REG_JIG_DEFAULT_DOWN, FSA9685_REG_JIG_MASK);
                if (ret < 0) {
                    hwlog_err("%s:write FSA9685_REG_MANUAL_SW_2 error!\n",__func__);
                    break;
                }
            }
            break;
        case JIG_PULL_UP:
            hwlog_info("%s:pull up jig pin to cut battery\n", __func__);
            if(FSA9683_I2C_ADDR == switch_fsa9685_data.cfg.i2c_address){
                ret = fsa9685_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
                            FSA9683_REG_JIG_UP, FSA9685_REG_JIG_MASK);
                if (ret < 0) {
                    hwlog_err("%s:write FSA9685_REG_MANUAL_SW_2 error!\n",__func__);
                }
            }else {
                ret = fsa9685_write_reg_mask(FSA9685_REG_MANUAL_SW_2,
                            FSA9685_REG_JIG_UP, FSA9685_REG_JIG_MASK);
                if (ret < 0) {
                    hwlog_err("%s:write FSA9685_REG_MANUAL_SW_2 error!\n",__func__);
                }
            }
            break;
        default:
            hwlog_err("%s:Wrong input action!\n", __func__);
            return -1;
    }
    return 0x60;
}

static ssize_t jigpin_ctrl_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    int manual_sw2_val;
    manual_sw2_val = fsa9685_read_reg(FSA9685_REG_MANUAL_SW_2);
    if (manual_sw2_val < 0) {
        hwlog_err("%s: read FSA9685_REG_MANUAL_SW_2 error!!! reg=%d.\n", __func__, manual_sw2_val);
    }

    return snprintf(buf, PAGE_SIZE, "%02x\n", manual_sw2_val);
}

static DEVICE_ATTR(jigpin_ctrl, S_IRUGO | S_IWUSR, jigpin_ctrl_show, jigpin_ctrl_store);

static ssize_t switchctrl_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
    int action = 0;
    if (sscanf(buf, "%d", &action) != 1) {
        hwlog_err("%s:write regs failed, invalid input!\n", __func__);
        return -1;
    }
    switch (action) {
        case MANUAL_DETACH:
            hwlog_info("%s:manual_detach\n", __func__);
            usbswitch_common_manual_detach();
            break;
        case MANUAL_SWITCH:
            hwlog_info("%s:manual_switch\n", __func__);
            usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_VBAT);
            break;
        default:
            hwlog_err("%s:Wrong input action!\n", __func__);
            return -1;
    }
    return 0x60;
}

static ssize_t switchctrl_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    int device_type1 = 0, device_type2 = 0, device_type3 = 0, mode = -1, tmp = 0;
    device_type1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_1);
    if (device_type1 < 0) {
        hwlog_err("%s: read FSA9685_REG_DEVICE_TYPE_1 error!!! reg=%d.\n", __func__, device_type1);
        goto read_reg_failed;
    }
    device_type2 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_2);
    if (device_type2 < 0) {
        hwlog_err("%s: read FSA9685_REG_DEVICE_TYPE_2 error!!! reg=%d.\n", __func__, device_type2);
        goto read_reg_failed;
    }
    device_type3 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_3);
    if (device_type3 < 0) {
        hwlog_err("%s: read FSA9685_REG_DEVICE_TYPE_3 error!!! reg=%d.\n", __func__, device_type3);
        goto read_reg_failed;
    }
    hwlog_info("%s:device_type1=0x%x,device_type2=0x%x,device_type3=0x%x\n", __func__,device_type1,device_type2,device_type3);
    tmp = device_type3 << 16 | device_type2 << 8 | device_type1;
    mode = 0;
    while (tmp >> mode)
        mode++;
read_reg_failed:
    return sprintf(buf, "%d\n", mode);
}

static DEVICE_ATTR(switchctrl, S_IRUGO | S_IWUSR, switchctrl_show, switchctrl_store);

/****************************************************************************
  Function:     acp_adapter_detect
  Description:  detect accp adapter
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
                1:fcp adapter but detect fail
***************************************************************************/
static int accp_adapter_detect(void)
{
    int reg_val1 = 0;
    int reg_val2 = 0;
    int i = 0;

    mutex_lock(&accp_detect_lock);
    /*check accp status*/
    reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
    reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
    if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
        && (FSA9688_ACCP_STATUS_SLAVE_GOOD == (reg_val2 & FSA9688_ACCP_STATUS_MASK )))
    {
        hwlog_info("accp adapter detect ok.\n");
        mutex_unlock(&accp_detect_lock);
        return ACCP_ADAPTOR_DETECT_SUCC;
    }

    /* enable accp */
    reg_val1 = fsa9685_read_reg(FSA9685_REG_CONTROL2);
    reg_val1 |= FSA9685_ACCP_ENABLE;
    fsa9685_write_reg(FSA9685_REG_CONTROL2, reg_val1);

    /*detect hisi acp charger*/
    for(i=0; i < ACCP_DETECT_MAX_COUT; i++)
    {
        reg_val1 = fsa9685_read_reg(FSA9685_REG_DEVICE_TYPE_4);
        reg_val2 = fsa9685_read_reg(FSA9685_REG_ACCP_STATUS);
        if((reg_val1 & FSA9685_ACCP_CHARGER_DET)
            && (FSA9688_ACCP_STATUS_SLAVE_GOOD == (reg_val2 & FSA9688_ACCP_STATUS_MASK )))
        {
            break;
        }
        msleep(ACCP_POLL_TIME);
    }
    /*clear accp interrupt*/
    hwlog_info("%s : read ACCP interrupt,reg[0x59]=0x%x,reg[0x5A]=0x%x\n",__func__,
        fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT1), fsa9685_read_reg(FSA9685_REG_ACCP_INTERRUPT2));
    if(ACCP_DETECT_MAX_COUT == i )
    {
        mutex_unlock(&accp_detect_lock);
        hwlog_info("not accp adapter ,reg[0x%x]=0x%x,reg[0x%x]=0x%x \n",FSA9685_REG_DEVICE_TYPE_4,reg_val1,FSA9685_REG_ACCP_STATUS,reg_val2);
        if(reg_val1 & FSA9685_ACCP_CHARGER_DET)
        {
            return ACCP_ADAPTOR_DETECT_FAIL;/*accp adapter but detect fail */
        }
        return ACCP_ADAPTOR_DETECT_OTHER;/*not fcp adapter*/

    }
    hwlog_info("accp adapter detect ok,take %d ms \n",i*ACCP_POLL_TIME);
    mutex_unlock(&accp_detect_lock);
    return ACCP_ADAPTOR_DETECT_SUCC;
}
static int fcp_adapter_detect(void)
{
    int ret;

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
}

/**********************************************************
*  Function:       is_sh_support_fcp
*  Discription:    check whether support fcp
*  Parameters:     NA
*  return value:   0:support
                  -1:not support
**********************************************************/
static int is_sh_support_fcp(void)
{
    int reg_val = 0;
    static int flag_result = -EINVAL;

    if(!switch_fsa9685_data.fsa9685_fcp_support)
    {
        return -1;
    }

    if(flag_result != -EINVAL)
    {
        return flag_result;
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
*  Function:       fcp_mmi_show
*  Discription:    file node for mmi testing fast charge protocol
*  Parameters:     NA
*  return value:   0:success
*                  1:fail
*                  2:not support
**********************************************************/
static ssize_t fcp_mmi_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	pkt_fcp_t pkt;
	pkt_fcp_resp_t resp_pkt;
	int result = FCP_TEST_FAIL;

	memset(&pkt, 0, sizeof(pkt));
	memset(&resp_pkt, 0, sizeof(resp_pkt));

	pkt.hd.tag = TAG_CHARGER;
	pkt.hd.cmd = CMD_CMN_CONFIG_REQ;
	pkt.hd.resp = RESP;
	pkt.hd.length = sizeof(pkt.sub_cmd);
	pkt.sub_cmd = CHARGE_SYSFS_FCP_SUPPORT;
	if (0 == WAIT_FOR_MCU_RESP_DATA_AFTER_SEND(&pkt,
						   inputhub_mcu_write_cmd
						   (&pkt, sizeof(pkt)),
						   5000, &resp_pkt,
						   sizeof(resp_pkt))) {
		hwlog_err("fcp_mmi_show wait for notify sensorhub fcp timeout\n");
		return -1;
	} else {
		if (resp_pkt.hd.errno != 0) {
			hwlog_err("get fcp mmi show from sensorhub fail, errno is %d\n", resp_pkt.hd.errno);
			return -1;
		} else {
			memcpy(&result, &resp_pkt.wr, sizeof(int));
			hwlog_info("%s: fcp mmi result  %d\n",__func__,result);
		}
	}
	return snprintf(buf,PAGE_SIZE,"%d\n",result);
}

static DEVICE_ATTR(fcp_mmi, S_IRUGO, fcp_mmi_show, NULL);

#ifdef CONFIG_OF
static const struct of_device_id switch_fsa9685_ids[] = {
    { .compatible = "huawei,fairchild_fsa9685_sensorhub" },
    { .compatible = "huawei,fairchild_fsa9683_sensorhub"},
    { .compatible = "huawei,nxp_cbtl9689_sensorhub" },
    { },
};
#endif
MODULE_DEVICE_TABLE(of, switch_fsa9685_ids);

void fsa9685_get_gpio_int(void)
{
    int gpio_value = 0;

    gpio_value = gpio_get_value(switch_fsa9685_data.gpio_intb);
    hwlog_info("%s: intb=%d after clear MASK.\n", __func__, gpio_value);

    if (gpio_value == 0) {
	if (g_iom3_state == IOM3_ST_NORMAL)
        	schedule_work(&g_intb_work_sh);
	else
		fsa9685_intb_work(&g_intb_work_sh);
    }
}

/**********************************************************
*  Function:        fsa9685_reg_dump
*  Discription:     dump register for charger dsm
*  Parameters:    ptr
*  return value:   void
**********************************************************/
#define DUMP_REG_NUM 26
#define DUMP_STR_LENTH 14

static void fsa9685_reg_dump(char* ptr)
{
	u8 reg[DUMP_REG_NUM] = { 0 };
	u8 i =0;
	char buff[DUMP_STR_LENTH] = {0};

	fsa9685_read_block(1, &reg[0], 15);
	fsa9685_read_block(64, &reg[15], 9);
	fsa9685_read_block(91, &reg[24], 2);

	for (i = 0; i < 15; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+1, reg[i]);
		strncat(ptr, buff, strlen(buff));
	}
	snprintf(buff, sizeof(buff), "\n");
	strncat(ptr, buff, strlen(buff));
	for (i = 15; i < 24; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+49, reg[i]);
		strncat(ptr, buff, strlen(buff));
	}
	snprintf(buff, sizeof(buff), "\n");
	strncat(ptr, buff, strlen(buff));
	for (i = 24; i < 26; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+67, reg[i]);
		strncat(ptr, buff, strlen(buff));
	}
}

struct fcp_adapter_device_ops sh_fcp_fsa9688_ops = {
	.reg_dump = fsa9685_reg_dump,
	.is_support_fcp = is_sh_support_fcp,
};

static int fsa9685_dcd_timeout_status(void)
{
	return 0;
}

static struct usbswitch_common_ops huawei_switch_extra_ops = {
	.manual_switch = fsa9685_manual_switch,
	.dcd_timeout_enable = fsa9685_dcd_timeout,
	.dcd_timeout_status = fsa9685_dcd_timeout_status,
	.manual_detach = fsa9685_manual_detach_work,
};

static int fsa9685_probe(struct platform_device *pdev)
{
    int ret = 0;
#ifdef CONFIG_FSA9685_DEBUG_FS
    struct class *switch_class = NULL;
    struct device * new_dev = NULL;
#endif

    hwlog_info("%s: ------sensorhub entry.\n", __func__);

#ifdef CONFIG_FSA9685_DEBUG_FS
    ret = device_create_file(&pdev->dev, &dev_attr_dump_regs);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_i2c_check_functionality;
    }

/*create a node for phone-off current drain test*/
    ret = device_create_file(&pdev->dev, &dev_attr_switchctrl);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_get_named_gpio;
    }

    ret = device_create_file(&pdev->dev, &dev_attr_jigpin_ctrl);
    if (ret < 0) {
        hwlog_err("%s: device_create_file error!!! ret=%d.\n", __func__, ret);
        ret = -ERR_SWITCH_USB_DEV_REGISTER;
        goto err_create_jigpin_ctrl_failed;
    }

    mutex_init(&accp_detect_lock);
    mutex_init(&accp_adaptor_reg_lock);
    ret = device_create_file(&pdev->dev, &dev_attr_fcp_mmi);
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
    ret = sysfs_create_link(&new_dev->kobj, &pdev->dev.kobj, "manual_ctrl");
    if (ret < 0) {
        hwlog_err("%s:create link to switch failed!\n", __func__);
        goto err_create_link_failed;
    }
#endif
    wake_lock_init(&usb_switch_lock, WAKE_LOCK_SUSPEND, "usb_switch_wakelock");

    INIT_DELAYED_WORK(&detach_delayed_work, fsa9685_detach_work);
    /* interrupt register */
    INIT_WORK(&g_intb_work_sh, fsa9685_intb_work);

    ret = usbswitch_common_ops_register(&huawei_switch_extra_ops);
    if (ret) {
    	hwlog_err("register extra switch ops failed!\n");
    }

    hwlog_info("%s: ------sensorhub end. ret = %d.\n", __func__, ret);
    return ret;

err_create_link_failed:
    device_remove_file(&pdev->dev, &dev_attr_fcp_mmi);
err_create_fcp_mmi_failed:
    device_remove_file(&pdev->dev, &dev_attr_jigpin_ctrl);
err_create_jigpin_ctrl_failed:
    device_remove_file(&pdev->dev, &dev_attr_switchctrl);
err_get_named_gpio:
    device_remove_file(&pdev->dev, &dev_attr_dump_regs);
err_i2c_check_functionality:
    hwlog_err("%s: ------ sensorhub FAIL!!! end. ret = %d.\n", __func__, ret);
    return ret;
}

static int fsa9685_remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev, &dev_attr_dump_regs);
    device_remove_file(&pdev->dev, &dev_attr_switchctrl);
    device_remove_file(&pdev->dev, &dev_attr_jigpin_ctrl);
    free_irq(switch_irq, NULL);
    gpio_free(switch_fsa9685_data.gpio_intb);
    return 0;
}

static struct platform_driver fsa9685_driver = {
    .driver = {
        .name = "fsa9685_sensorhub",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(switch_fsa9685_ids),
    },
    .probe    = fsa9685_probe,
    .remove   = fsa9685_remove,
};

static __init int fsa9685_init(void)
{
    return platform_driver_register(&fsa9685_driver);
}

static __exit void fsa9685_exit(void)
{
    platform_driver_unregister(&fsa9685_driver);
}

module_init(fsa9685_init);
module_exit(fsa9685_exit);

MODULE_AUTHOR("Lixiuna<lixiuna@huawei.com>");
MODULE_DESCRIPTION("I2C bus driver for sensorhub FSA9685 USB Accesory Detection Switch");
MODULE_LICENSE("GPL v2");

