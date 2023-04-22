/*
 * File:   fusb3601_driver.c
 */

/* Standard Linux includes */
#include <linux/init.h>       /* __init, __initdata, etc */
#include <linux/module.h>     /* Needed to be a module */
#include <linux/kernel.h>     /* Needed to be a kernel module */
#include <linux/i2c.h>        /* I2C functionality */
#include <linux/slab.h>       /* devm_kzalloc */
#include <linux/types.h>      /* Kernel datatypes */
#include <linux/errno.h>      /* EINVAL, ERANGE, etc */
#include <linux/of_device.h>  /* Device tree functionality */
#include <linux/delay.h>

/* Driver-specific includes */
#include "fusb3601_global.h"  /* Driver-specific structures/types */
#include "platform_helpers.h" /* I2C R/W, GPIO, misc, etc */
#include "../core/policy.h"
#include "../core/port.h"
#include "../core/moisture_detection.h"

#ifdef FSC_DEBUG
#include "../core/core.h"     /* GetDeviceTypeCStatus */
#include "../core/hw_scp.h"
#include "dfs.h"
#endif /* FSC_DEBUG */

#include "fusb3601_driver.h"
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#endif
#include <huawei_platform/usb/hw_pd_dev.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#ifdef CONFIG_CONTEXTHUB_PD
extern bool hisi_dptx_ready(void);
#endif
static int dpd_enable = 1;
static int max_output_current = SOURCE_CURRENT_1500;
static int use_super_switch_cutoff_wired_channel = 0;
static device_role d_role = DEVICE_ROLE_DRP;
static int  fusb3601_mode = 1;
void FUSB3601_core_set_drp(struct Port *port);
void FUSB3601_core_set_sink(struct Port *port);
int get_dpd_enable(void)
{
	return dpd_enable;
}
int get_source_max_output_current(void)
{
	return max_output_current;
}
device_role fusb3601_get_device_role(void)
{
	return d_role;
}
/******************************************************************************
* Driver functions
******************************************************************************/
static void fusb3601_set_cc_mode_work(struct work_struct *work)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return;
	}
	down(&chip->suspend_lock);
	if (!fusb3601_mode){
		pr_info("%s:set CC to UFP\n",__func__);
		d_role = DEVICE_ROLE_UFP_ONLY;
		FUSB3601_core_set_sink(&chip->port);
	} else {
		pr_info("%s:set CC to DRP\n",__func__);
		d_role = DEVICE_ROLE_DRP;
		FUSB3601_core_set_drp(&chip->port);
	}
	up(&chip->suspend_lock);
}
void fusb3601_set_cc_mode(int mode)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return;
	}
	pr_info("%s\n",__func__);
	fusb3601_mode = mode;
	schedule_work(&chip->fusb3601_set_cc_mode_work);
}
int fusb3601_get_cc_mode(void)
{
       return 0;
}

static void fusb3601_hard_reset(void* client)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    pr_debug("%s++\n", __func__);
    if (chip == NULL)
    {
        pr_debug("%s - Chip structure is null!\n", __func__);
        return;
    }
    down(&chip->suspend_lock);
    if((&chip->port)->last_policy_state_ == peSinkReady) {
        pr_debug("%s - fusb3601 hard reset!\n", __func__);
        FUSB3601_set_policy_state(&chip->port, peSinkSendHardReset);
        FUSB3601_PolicySinkSendHardReset(&chip->port);
    }
    up(&chip->suspend_lock);
}

static void fusb3601_set_voltage(void* client, int set_voltage)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    pr_debug("%s++\n", __func__);
    if (chip == NULL)
    {
        pr_debug("%s - Chip structure is null!\n", __func__);
        return;
    }
    down(&chip->suspend_lock);
    if((&chip->port)->last_policy_state_ == peSinkReady) {
        pr_debug("%s - fusb3601 set voltage!\n", __func__);
        FUSB3601_SetPDLimitVoltage(set_voltage);
        FUSB3601_set_policy_state(&chip->port, peSinkGetSourceCap);
        FUSB3601_PolicySinkGetSourceCap(&chip->port);
    }
    up(&chip->suspend_lock);
}

static int fusb3601_get_cc_state(void)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;
	int val = 0;
	if (!chip)
	{
		pr_info("FUSB  %s - Chip structure is NULL!\n", __func__);
		return -1;
	}
	port = &chip->port;
	if (!port) {
		return -1;
	}
	val = FUSB3601_GetStateCC(port);
	pr_info("FUSB  %s - CCStat.CC1 = 0x%x, CCStat.CC2 = 0x%x\n",
		__func__, port->registers_.CCStat.CC1_STAT, port->registers_.CCStat.CC2_STAT);
	val = val & CC_STATUS_MASK;
	return val;
}


static struct pd_dpm_ops tcpc_device_pd_dpm_ops = {
	.pd_dpm_get_hw_dock_svid_exist = NULL,
	.pd_dpm_notify_direct_charge_status = NULL,
	.pd_dpm_set_cc_mode = fusb3601_set_cc_mode,
	.pd_dpm_hard_reset = fusb3601_hard_reset,
	.pd_dpm_set_voltage = fusb3601_set_voltage,
	.pd_dpm_get_cc_state = fusb3601_get_cc_state,
};

static int __init fusb3601_init(void)
{
        pr_debug("FUSB  %s - Start driver initialization...vHW1.0.22\n", __func__);

        return i2c_add_driver(&fusb3601_driver);
}

static void __exit fusb3601_exit(void)
{
        i2c_del_driver(&fusb3601_driver);
        pr_debug("FUSB  %s - Driver deleted...\n", __func__);
}

static int fusb3601_i2c_resume(struct device* dev)
{
        struct fusb3601_chip *chip;
        struct i2c_client *client = to_i2c_client(dev);

        pr_err("FUSB  %s ++\n", __func__);
        if (client) {
                chip = i2c_get_clientdata(client);
                if (chip)
                        up(&chip->suspend_lock);
        }
        pr_err("FUSB  %s --\n", __func__);
        return 0;
}

static int fusb3601_i2c_suspend(struct device* dev)
{
        struct fusb3601_chip* chip;
        struct i2c_client* client =  to_i2c_client(dev);

        pr_err("FUSB  %s ++\n", __func__);
        if (client) {
          chip = i2c_get_clientdata(client);
          if (chip)
                  down(&chip->suspend_lock);
        }
        pr_err("FUSB  %s --\n", __func__);
        return 0;
}
static int is_cable_for_direct_charge(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    struct Port* port;
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return -1;
    }

    port = &chip->port;
    if (!port) {
	    return -1;
    }
    if (port->double56k) {
	    return 0;
    } else {
	    return -1;
    }
}
static struct cc_check_ops cc_check_ops = {
	.is_cable_for_direct_charge = is_cable_for_direct_charge,
};
static int fusb3601_probe_work(struct work_struct *work)
{
	struct fusb3601_chip *chip = container_of(work, struct fusb3601_chip, fusb3601_probe_work);
	struct i2c_client *client;
	int ret;
#ifdef CONFIG_CONTEXTHUB_PD
	int count = 50;/*wait until dp phy is ready, timeout is 50*100ms*/
#endif
	if (!chip || NULL == chip->client) {
                pr_err("FUSB  %s - Error: Client structure is NULL!\n",
                       __func__);
                return -EINVAL;
	}
	client = chip->client;
#ifdef CONFIG_CONTEXTHUB_PD
	do {
		msleep(100);
		count--;
		if(true == hisi_dptx_ready())
			break;
	} while(count);
#endif
	/* reset fusb3601*/
        FUSB3601_fusb_reset_with_adc_reset();

        /* Initialize the platform's GPIO pins and IRQ */
        ret = FUSB3601_fusb_InitializeGPIO();
        if (ret)
        {
                dev_err(&client->dev,
                        "FUSB  %s - Error: Unable to initialize GPIO!\n",
                        __func__);
                return ret;
        }
        pr_debug("FUSB  %s - GPIO initialized!\n", __func__);

#ifdef FSC_DEBUG
        /* Initialize debug sysfs file accessors */
        FUSB3601_fusb_Sysfs_Init();
        pr_debug("FUSB  %s - Sysfs device file created!\n", __func__);

        fusb_InitializeDFS();
        pr_debug("FUSB  %s - DebugFS entry created!\n", __func__);
#endif /* FSC_DEBUG */

        /* Initialize the core and enable the state machine
         * (NOTE: timer and GPIO must be initialized by now)
         * Interrupt must be enabled before starting 3601 initialization */
        FUSB3601_fusb_InitializeCore();
	FUSB3601_scp_initialize();
	ret = cc_check_ops_register(&cc_check_ops);
	if (ret)
	{
		pr_info("cc_check_ops register failed!\n");
		return -1;
	}
	pr_info("shanshan1. *%s* pd_dpm_ops_register\n", __func__);
	pd_dpm_ops_register(&tcpc_device_pd_dpm_ops, NULL);
	moisture_detection_init();
        pr_debug("FUSB  %s - Core is initialized!\n", __func__);

        /* Enable interrupts after successful core/GPIO initialization */
        ret = FUSB3601_fusb_EnableInterrupts();
        if (ret)
        {
                dev_err(&client->dev,
            "FUSB  %s - Error: Unable to enable interrupts! Error code: %d\n",
                        __func__, ret);
                return -EIO;
        }

        dev_info(&client->dev,
                 "FUSB  %s - FUSB3601 Driver loaded successfully!\n",
                 __func__);
        return ret;

}
static int fusb3601_probe (struct i2c_client* client,
						const struct i2c_device_id* id)
{
	int ret = 0;
	struct fusb3601_chip* chip;
	struct i2c_adapter* adapter;
	struct device_node* node;

	if (!client)
	{
		pr_err("FUSB  %s - Error: Client structure is NULL!\n", __func__);
		return -EINVAL;
	}
	node = client->dev.of_node;

	ret = of_property_read_u32(node, "dpd_enable", &dpd_enable);
	if (ret)
	{
		dpd_enable = 0;
		dev_err(&client->dev, "failed to get dpd_enable\n");
		return -EINVAL;
	}
	pr_info("dpd_enable = %d\n", dpd_enable);
	ret = of_property_read_u32(node, "max_output_current", &max_output_current);
	if (ret)
	{
		max_output_current = SOURCE_CURRENT_1500;/*used for PDO*/
		dev_err(&client->dev, "failed to get max_output_current\n");
	}
	pr_info("max_output_current = %d\n", max_output_current);
	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,wired_channel_switch"),
			"use_super_switch_cutoff_wired_channel", &use_super_switch_cutoff_wired_channel);
	if (ret) {
		use_super_switch_cutoff_wired_channel = 0;
		pr_info("%s get use_super_switch_cutoff_wired_channel failed\n", __func__);
	}
	pr_info("use_super_switch_cutoff_wired_channel = %d\n", use_super_switch_cutoff_wired_channel);

	dev_info(&client->dev, "%s\n", __func__);

	/* Make sure probe was called on a compatible device */
	if (!of_match_device(fusb3601_dt_match, &client->dev))
	{
		dev_err(&client->dev, "FUSB  %s - Error: Device tree mismatch!\n", __func__);
		return -EINVAL;
	}

	pr_debug("FUSB  %s - Device tree matched!\n", __func__);

	/* Alloc space for our chip struct (devm_* is managed by the device) */
	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
	{
		dev_err(&client->dev, "FUSB  %s - Error: Unable to allocate memory for g_chip!\n", __func__);
		return -ENOMEM;
	}
	chip->use_super_switch_cutoff_wired_channel = use_super_switch_cutoff_wired_channel;

	/* Assign our client handle to our chip */
	chip->client = client;

	/* Set our global chip's address to the newly allocated memory */
	fusb3601_SetChip(chip);

	pr_debug("FUSB  %s - Chip structure is set! Chip: %p ... g_chip: %p\n", __func__, chip, fusb3601_GetChip());

	/* Initialize semaphore*/
	sema_init(&chip->suspend_lock, 1);

	/* Initialize the chip lock */
	mutex_init(&chip->lock);

	/* Initialize the chip's data members */
	FUSB3601_fusb_InitChipData();
	pr_debug("FUSB  %s - Chip struct data initialized!\n", __func__);

	/* Verify that the system has our required I2C/SMBUS functionality
	* (see <linux/i2c.h> for definitions)
	*/
	adapter = to_i2c_adapter(client->dev.parent);
	if (i2c_check_functionality(adapter, FUSB3601_I2C_SMBUS_BLOCK_REQUIRED_FUNC))
	{
		chip->use_i2c_blocks = true;
	}
	else
	{
		/* If the platform doesn't support block reads, try with block
		* writes and single reads (works with eg. RPi)
		* It is likely that this may result in non-standard behavior,
		* but will often be 'close enough' to work for most things
		*/
		dev_warn(&client->dev, "FUSB %s - Warning: I2C/SMBus block rd/wr not supported,\n", __func__);
		dev_warn(&client->dev, "FUSB %s -     checking single-read mode...\n", __func__);

		if (!i2c_check_functionality(adapter, FUSB3601_I2C_SMBUS_REQUIRED_FUNC))
		{
			dev_err(&client->dev, "FUSB  %s - Error: Required I2C/SMBus functionality not supported!\n", __func__);
			dev_err(&client->dev, "FUSB  %s - I2C Supported Functionality Mask: 0x%x\n", __func__, i2c_get_functionality(adapter));
			return -EIO;
		}
	}

	pr_err("FUSB  %s - I2C Functionality check passed! Block reads: %s\n", __func__, chip->use_i2c_blocks ? "YES" : "NO");

	/* Assign our struct as the client's driverdata */
	i2c_set_clientdata(client, chip);
	pr_debug("FUSB  %s - I2C client data set!\n", __func__);

	/* Verify that our device exists and that it's what we expect */
	if (!FUSB3601_fusb_IsDeviceValid())
	{
		dev_err(&client->dev, "FUSB  %s - Error: Unable to communicate with device!\n", __func__);
		return -EIO;
	}
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_USB_SWITCH);
#endif
	pr_debug("FUSB  %s - Device check passed!\n", __func__);
	FUSB3601_charge_register_callback();

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	FUSB3601_dual_role_phy_init();
#endif

	INIT_WORK(&chip->fusb3601_probe_work, fusb3601_probe_work);
	INIT_WORK(&chip->fusb3601_set_cc_mode_work, fusb3601_set_cc_mode_work);
	schedule_work(&chip->fusb3601_probe_work);
	return 0;
}

static int fusb3601_remove(struct i2c_client* client)
{
        pr_debug("FUSB  %s - Removing fusb3601 device!\n", __func__);

        FUSB3601_fusb_GPIO_Cleanup();

#ifdef FSC_DEBUG
        fusb_DFS_Cleanup();
#endif /* FSC_DEBUG */

        pr_debug("FUSB  %s - FUSB3601 device removed from driver.\n", __func__);
        return 0;
}

static void fusb3601_shutdown(struct i2c_client *client)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (!chip)
	{
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return;
	}

	FUSB3601_set_driver_shutdown_flag(1);
	FUSB3601_core_enable_typec(&chip->port, FALSE);
	FUSB3601_core_send_hard_reset(&chip->port);
	FUSB3601_core_set_state_unattached(&chip->port);
	msleep(10);
	FUSB3601_fusb_reset();
	pr_debug("FUSB  %s - fusb3601 shutdown\n", __func__);
}


/*******************************************************************************
 * Driver macros
 ******************************************************************************/
fs_initcall_sync(fusb3601_init);    /* Defines the module's entrance function */
module_exit(fusb3601_exit);    /* Defines the module's exit function */

/* Exposed on call to modinfo */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fairchild FUSB3601 Driver");
