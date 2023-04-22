/*
 * File:   fusb30x_driver.c
 * Author: Tim Bremm <tim.bremm@fairchildsemi.com>
 * Company: Fairchild Semiconductor
 *
 * Created on September 2, 2015, 10:22 AM
 */

/* Standard Linux includes */
#include <linux/init.h>                                                         // __init, __initdata, etc
#include <linux/module.h>                                                       // Needed to be a module
#include <linux/kernel.h>                                                       // Needed to be a kernel module
#include <linux/i2c.h>                                                          // I2C functionality
#include <linux/slab.h>                                                         // devm_kzalloc
#include <linux/types.h>                                                        // Kernel datatypes
#include <linux/errno.h>                                                        // EINVAL, ERANGE, etc
#include <linux/of_device.h>                                                    // Device tree functionality
#ifdef CONFIG_USE_CAMERA3_ARCH
#include <media/huawei/hw_extern_pmic.h>
#endif
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/of.h>

/* Driver-specific includes */
#include "fusb30x_global.h"                                                     // Driver-specific structures/types
#include "platform_helpers.h"                                                   // I2C R/W, GPIO, misc, etc

//#ifdef FSC_DEBUG
#include "../core/core.h"                                                       // GetDeviceTypeCStatus
//#endif // FSC_DEBUG

#include "fusb30x_driver.h"
#include <huawei_platform/usb/hw_pd_dev.h>

#ifdef CONFIG_POGO_PIN
#include <huawei_platform/usb/huawei_pogopin.h>
#endif
/******************************************************************************
* Driver functions
******************************************************************************/
/* Under Construction */
static int pd_dpm_wake_lock_call(struct notifier_block *fsc_nb, unsigned long event, void *data)
{
	struct fusb30x_chip *chip = container_of(fsc_nb, struct fusb30x_chip, fsc_nb);

	switch(event)
	{
		case PD_WAKE_LOCK:
			FSC_PRINT("FUSB %s - wake lock node called\n", __func__);
			wake_lock(&chip->fusb302_wakelock);
			break;
		case PD_WAKE_UNLOCK:
			FSC_PRINT("FUSB %s - wake unlock node called\n", __func__);
			wake_unlock(&chip->fusb302_wakelock);
			break;
		default:
			FSC_PRINT("FUSB %s - unknown event: %d\n", __func__, event);
			break;
	}

	return NOTIFY_OK;
}

static int __init fusb30x_init(void)
{
    pr_debug("FUSB  %s - Start driver initialization...\n", __func__);

	return i2c_add_driver(&fusb30x_driver);
}

static void __exit fusb30x_exit(void)
{
	i2c_del_driver(&fusb30x_driver);
    pr_debug("FUSB  %s - Driver deleted...\n", __func__);
}

static FSC_BOOL fusb_write_mask(FSC_U8 reg, FSC_U8 MASK, FSC_U8 SHIFT, FSC_U8 value)
{
	FSC_BOOL ret;
	FSC_U8 val;

	ret = fusb_I2C_ReadData(reg, &val);
	if (FALSE == ret)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = fusb_I2C_WriteData(reg, 1, &val);

	return ret;
}

static int is_cable_for_direct_charge(void)
{
	FSC_U8 val;
	FSC_U8 cc2;
	FSC_U8 cc1;
	FSC_BOOL ret;
	FSC_U8 meas_cc2 = 0x02;
	FSC_U8 meas_cc1 = 0x01;
	ret = fusb_write_mask(FSC_CC_SELECT_REG_02H, FSC_CC_SELECT_MASK, FSC_CC_SELECT_SHIFT, meas_cc2);
	msleep(1);
	ret &= fusb_I2C_ReadData( FSC_CC_STATUS_40H,&cc2);
	ret &= fusb_write_mask(FSC_CC_SELECT_REG_02H, FSC_CC_SELECT_MASK, FSC_CC_SELECT_SHIFT, meas_cc1);
	msleep(1);
	ret &= fusb_I2C_ReadData( FSC_CC_STATUS_40H,&cc1);
	if (!ret)
	{
		pr_info("%s:REG R/W FAIL!!!!\n",__func__);
		return -1;
	}
	pr_info("%s:cc2_REG0x40 = 0x%x\n",__func__,cc2);
	pr_info("%s:cc1_REG0x40 = 0x%x\n",__func__,cc1);
	cc2 = cc2 & FSC_CC_STATUS_MASK;
	cc1 = cc1 & FSC_CC_STATUS_MASK;
	if ((FSC_CC_STATUS_FOR_DOUBLE_56K != cc1) || (FSC_CC_STATUS_FOR_DOUBLE_56K != cc2))
		return -1;
	pr_info("%s:cc_check succ\n",__func__);
	return 0;
}

int fusb30x_pd_dpm_get_cc_state(void)
{
	FSC_U8 val;
	FSC_U8 cc2;
	FSC_U8 cc1;
	FSC_BOOL ret;
	FSC_U8 meas_cc2 = 0x02;
	FSC_U8 meas_cc1 = 0x01;
	ret = fusb_write_mask(FSC_CC_SELECT_REG_02H, FSC_CC_SELECT_MASK, FSC_CC_SELECT_SHIFT, meas_cc2);
	msleep(1);
	ret &= fusb_I2C_ReadData( FSC_CC_STATUS_40H,&cc2);
	ret &= fusb_write_mask(FSC_CC_SELECT_REG_02H, FSC_CC_SELECT_MASK, FSC_CC_SELECT_SHIFT, meas_cc1);
	msleep(1);
	ret &= fusb_I2C_ReadData( FSC_CC_STATUS_40H,&cc1);
	if (!ret)
	{
		pr_info("%s:REG R/W FAIL!!!!\n",__func__);
		return -1;
	}
	pr_info("%s:cc2_REG0x40 = 0x%x\n",__func__,cc2);
	pr_info("%s:cc1_REG0x40 = 0x%x\n",__func__,cc1);
	cc2 = cc2 & FSC_CC_STATUS_MASK;
	cc1 = cc1 & FSC_CC_STATUS_MASK;
	val = (cc2 << 2) | cc1;
	return val;
}

void fusb30x_set_cc_mode(int mode)
{
       if (!mode){
               pr_info("%s:set CC to UFP\n",__func__);
               core_set_sink();
       } else {
               pr_info("%s:set CC to DRP\n",__func__);
               core_set_drp();
       }
}
int fusb30x_get_cc_mode(void)
{
       return 0;
}

#ifdef CONFIG_POGO_PIN
static int fusb30x_typec_detect_disable(FSC_BOOL disable)
{
	return core_cc_disable(disable);
}

struct cc_detect_ops fusb30x_cc_detect_ops = {
	.typec_detect_disable = fusb30x_typec_detect_disable,
};
#endif

static struct cc_check_ops cc_check_ops = {
	.is_cable_for_direct_charge = is_cable_for_direct_charge,
};

static int fusb30x_probe (struct i2c_client* client,
                          const struct i2c_device_id* id)
{
    int ret = 0;
    struct fusb30x_chip* chip;
    struct i2c_adapter* adapter;
    int need_not_config_extra_pmic=0;
    if (of_property_read_u32(of_find_compatible_node(NULL,NULL, "huawei,pd_dpm"),"need_not_config_extra_pmic", &need_not_config_extra_pmic)) {
	pr_err("get need_not_config_extra_pmic fail!\n");
    }
    pr_info("need_not_config_extra_pmic = %d!\n", need_not_config_extra_pmic);
    if(!need_not_config_extra_pmic){
#ifdef CONFIG_USE_CAMERA3_ARCH
        hw_extern_pmic_config(FSC_PMIC_LDO_3,FUSB_VIN_3V3, 1);
#endif
        pr_info("%s:PD PMIC ENABLE IS CALLED\n", __func__);
    }

    if (!client)
    {
        pr_err("FUSB  %s - Error: Client structure is NULL!\n", __func__);
        return -EINVAL;
    }
    dev_info(&client->dev, "%s\n", __func__);

    /* Make sure probe was called on a compatible device */
    if (!of_match_device(fusb30x_dt_match, &client->dev))
    {
        dev_err(&client->dev, "FUSB  %s - Error: Device tree mismatch!\n", __func__);
        return -EINVAL;
    }
    pr_debug("FUSB  %s - Device tree matched!\n", __func__);

    /* Allocate space for our chip structure (devm_* is managed by the device) */
    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
    {
        dev_err(&client->dev, "FUSB  %s - Error: Unable to allocate memory for g_chip!\n", __func__);
        return -ENOMEM;
    }
    chip->client = client;                                                      // Assign our client handle to our chip
    fusb30x_SetChip(chip);                                                      // Set our global chip's address to the newly allocated memory
    pr_debug("FUSB  %s - Chip structure is set! Chip: %p ... g_chip: %p\n", __func__, chip, fusb30x_GetChip());

    /* Initialize the chip lock */
    mutex_init(&chip->lock);
    mutex_init(&chip->thread_lock);

    /* Initialize the chip's data members */
    fusb_InitChipData();
    pr_debug("FUSB  %s - Chip struct data initialized!\n", __func__);

    /* Verify that the system has our required I2C/SMBUS functionality (see <linux/i2c.h> for definitions) */
    adapter = to_i2c_adapter(client->dev.parent);
    if (i2c_check_functionality(adapter, FUSB30X_I2C_SMBUS_BLOCK_REQUIRED_FUNC))
    {
        chip->use_i2c_blocks = true;
    }
    else
    {
        // If the platform doesn't support block reads, try with block writes and single reads (works with eg. RPi)
        // NOTE: It is likely that this may result in non-standard behavior, but will often be 'close enough' to work for most things
        dev_warn(&client->dev, "FUSB  %s - Warning: I2C/SMBus block read/write functionality not supported, checking single-read mode...\n", __func__);
        if (!i2c_check_functionality(adapter, FUSB30X_I2C_SMBUS_REQUIRED_FUNC))
        {
            dev_err(&client->dev, "FUSB  %s - Error: Required I2C/SMBus functionality not supported!\n", __func__);
            dev_err(&client->dev, "FUSB  %s - I2C Supported Functionality Mask: 0x%x\n", __func__, i2c_get_functionality(adapter));
            return -EIO;
        }
    }
    pr_debug("FUSB  %s - I2C Functionality check passed! Block reads: %s\n", __func__, chip->use_i2c_blocks ? "YES" : "NO");

    /* Assign our struct as the client's driverdata */
    i2c_set_clientdata(client, chip);
    pr_debug("FUSB  %s - I2C client data set!\n", __func__);

    /* Verify that our device exists and that it's what we expect */
    if (!fusb_IsDeviceValid())
    {
        dev_err(&client->dev, "FUSB  %s - Error: Unable to communicate with device!\n", __func__);
        return -EIO;
    }
    pr_debug("FUSB  %s - Device check passed!\n", __func__);

    /* Initialize the platform's GPIO pins and IRQ */
    ret = fusb_InitializeGPIO();
    if (ret)
    {
        dev_err(&client->dev, "FUSB  %s - Error: Unable to initialize GPIO!\n", __func__);
        return ret;
    }
    pr_debug("FUSB  %s - GPIO initialized!\n", __func__);

    /* Initialize our timer */
    fusb_InitializeTimer();
    pr_debug("FUSB  %s - Timers initialized!\n", __func__);

#ifdef CONFIG_POGO_PIN
	cc_detect_register_ops(&fusb30x_cc_detect_ops);
#endif
#ifdef FSC_DEBUG
    /* Initialize debug sysfs file accessors */
    fusb_Sysfs_Init();
    pr_debug("FUSB  %s - Sysfs device file created!\n", __func__);
#endif // FSC_DEBUG

#ifdef FSC_INTERRUPT_TRIGGERED
    /* Enable interrupts after successful core/GPIO initialization */
    ret = fusb_EnableInterrupts();
    if (ret)
    {
        dev_err(&client->dev, "FUSB  %s - Error: Unable to enable interrupts! Error code: %d\n", __func__, ret);
        return -EIO;
    }

	/* Init wake_lock node callback */
	chip->fsc_nb.notifier_call = pd_dpm_wake_lock_call;
	ret = register_pd_wake_unlock_notifier(&chip->fsc_nb);
	if (ret < 0)
	{
		FSC_PRINT("FUSB %s - register_pd_wake_unlock_notifier failed\n", __func__);
	}
	else
	{
		FSC_PRINT("FUSB %s - register_pd_wake_unlock_notifier OK\n", __func__);
	}

    fusb_InitializeCore();
    pr_debug("FUSB  %s - Core is initialized!\n", __func__);
#else
    fusb_InitializeCore();
    pr_debug("FUSB  %s - Core is initialized!\n", __func__);
    /* Init our workers, but don't start them yet */
    fusb_InitializeWorkers();
    /* Start worker threads after successful initialization */
    fusb_ScheduleWork();
    pr_debug("FUSB  %s - Workers initialized and scheduled!\n", __func__);
#endif  // ifdef FSC_POLLING elif FSC_INTERRUPT_TRIGGERED

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	fusb_dual_role_phy_init();
#endif
    /* Initialize the core and enable the state machine (NOTE: timer and GPIO must be initialized by now) */

    dev_info(&client->dev, "FUSB  %s - FUSB30X Driver loaded successfully!\n", __func__);
	ret = cc_check_ops_register(&cc_check_ops);
	if (ret)
	{
		pr_info("cc_check_ops register failed!\n");
		return -1;
	}
	pr_info("%s cc_check register OK!\n", __func__);
	pr_info("%s probe OK!\n", __func__);
	return ret;
}

static int fusb30x_remove(struct i2c_client* client)
{
    pr_debug("FUSB  %s - Removing fusb30x device!\n", __func__);

#ifndef FSC_INTERRUPT_TRIGGERED // Polling mode by default
    fusb_StopThreads();
#endif  // !FSC_INTERRUPT_TRIGGERED

    fusb_GPIO_Cleanup();
    pr_debug("FUSB  %s - FUSB30x device removed from driver...\n", __func__);
    return 0;
}

/*******************************************************************************
 * Driver macros
 ******************************************************************************/
module_init(fusb30x_init);                                                      // Defines the module's entrance function
module_exit(fusb30x_exit);                                                      // Defines the module's exit function

MODULE_LICENSE("GPL");                                                          // Exposed on call to modinfo
MODULE_DESCRIPTION("Fairchild FUSB30x Driver");                                 // Exposed on call to modinfo
MODULE_AUTHOR("Tim Bremm<tim.bremm@fairchildsemi.com>");                        // Exposed on call to modinfo
