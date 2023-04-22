/* 
 * File:   fusb3601_driver.h
 */

#ifndef FUSB3601_DRIVER_H
#define FUSB3601_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Platform-specific configuration data
 ******************************************************************************/
/* Length must be less than I2C_NAME_SIZE
 * (currently 20, see include/linux/mod_devicetable.h) */
#define FUSB3601_I2C_DRIVER_NAME                "fusb3601"
/* Must match device tree .compatible string exactly */
#define FUSB3601_I2C_DEVICETREE_NAME            "fairchild,fusb3601"
/* First try for block reads/writes */
#define FUSB3601_I2C_SMBUS_BLOCK_REQUIRED_FUNC (I2C_FUNC_SMBUS_I2C_BLOCK)
/* If no block reads/writes, try single reads/block writes */
#define FUSB3601_I2C_SMBUS_REQUIRED_FUNC    (I2C_FUNC_SMBUS_WRITE_I2C_BLOCK | \
                                             I2C_FUNC_SMBUS_READ_BYTE_DATA)

#define SOURCE_CURRENT_1500 150
#define SOURCE_CURRENT_1000 100

#define CC_STATUS_MASK 0x0F
/*******************************************************************************
* Driver structs
******************************************************************************/
/* Defined by the build system when configuring "open firmware" (OF)
 * aka device-tree */
#ifdef CONFIG_OF
/* Used by kernel to match device-tree entry to driver
 * String must match device-tree node exactly
 */
static const struct of_device_id fusb3601_dt_match[] = {
        { .compatible = FUSB3601_I2C_DEVICETREE_NAME },
        {},
};
MODULE_DEVICE_TABLE(of, fusb3601_dt_match);
#endif /* CONFIG_OF */

/* This identifies our I2C driver in the kernel's driver module table */
static const struct i2c_device_id fusb3601_i2c_device_id[] = {
        { FUSB3601_I2C_DRIVER_NAME, 0 },
        {}
};
/* Used to generate map files used by depmod for module dependencies */
MODULE_DEVICE_TABLE(i2c, fusb3601_i2c_device_id);

/*******************************************************************************                        
 * Driver module functions
 ******************************************************************************/                        
/* Called when driver is inserted into the kernel */
static int __init fusb3601_init(void);
/* Called when driver is removed from the kernel */
static void __exit fusb3601_exit(void);

static int fusb3601_i2c_resume(struct device* dev);
static int fusb3601_i2c_suspend(struct device* dev);

/* Called when the associated device is added */
static int fusb3601_probe(struct i2c_client* client,
                         const struct i2c_device_id* id);
/* Called when the associated device is removed */
static int fusb3601_remove(struct i2c_client* client);

static void fusb3601_shutdown(struct i2c_client *client);

#ifdef CONFIG_PM
static const struct dev_pm_ops fusb3601_dev_pm_ops = {
        .suspend = fusb3601_i2c_suspend,
        .resume  = fusb3601_i2c_resume,
};
#endif

/* Defines our driver's name, device-tree match, and required callbacks */
static struct i2c_driver fusb3601_driver = {
    .driver = {
        /* Must match our id_table name */
        .name = FUSB3601_I2C_DRIVER_NAME,
        /* Device-tree match structure to pair the DT device with our driver */
        .of_match_table = of_match_ptr(fusb3601_dt_match),
#ifdef CONFIG_PM
        .pm = &fusb3601_dev_pm_ops,
#endif
    },
    /* Called on device add, inits/starts driver */
    .probe = fusb3601_probe,
    /* Called on device remove, cleans up driver */
    .remove = fusb3601_remove,
    .shutdown = fusb3601_shutdown,
    /* I2C id structure to associate with our driver */
    .id_table = fusb3601_i2c_device_id,
};

#ifdef __cplusplus
}
#endif

#endif /* FUSB3601_DRIVER_H */

