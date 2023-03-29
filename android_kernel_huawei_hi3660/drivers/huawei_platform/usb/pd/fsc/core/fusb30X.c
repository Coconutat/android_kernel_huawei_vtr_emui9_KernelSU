#include <linux/kernel.h>
#include <linux/printk.h>
#include "fusb30X.h"
#include "platform.h"

FSC_BOOL DeviceWrite(FSC_U8 regAddr, FSC_U8 length, FSC_U8* data)
{
    FSC_BOOL error;
    error = platform_i2c_write(FUSB300SlaveAddr, FUSB300AddrLength, length, length, FUSB300IncSize, regAddr, data);
    if (error == FALSE)
        return TRUE;
    pr_info("FUSB %s - I2C Write NAK!\n", __func__);
    return FALSE;
}

FSC_BOOL DeviceRead(FSC_U8 regAddr, FSC_U8 length, FSC_U8* data)
{
    FSC_BOOL error;
    error = platform_i2c_read(FUSB300SlaveAddr, FUSB300AddrLength, length, length, FUSB300IncSize, regAddr, data);
    if (error == FALSE)
        return TRUE;
    pr_info("FUSB %s - I2C Read NAK!\n", __func__);
    return FALSE;
}
