/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define DEBUG
#include <linux/regmap.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/printk.h>
#include <huawei_platform/audio/lm48560.h>

static struct lm48560_data *g_pLM48560Data;

static int lm48560_reg_read(struct lm48560_data *pLM48560Data,
    unsigned char reg, unsigned int *pValue)
{
    unsigned int val = 0;
    int nResult = 0;

    mutex_lock(&pLM48560Data->lock);
    nResult = regmap_read(pLM48560Data->mpRegmap, reg, &val);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "%s, Reg = 0x%x, err = %d\n",  __func__, reg, nResult);
    } else {
        *pValue = val;
        dev_dbg(pLM48560Data->dev, "RegRead(0x%x) = 0x%x\n", reg, val);
    }
    mutex_unlock(&pLM48560Data->lock);

    return nResult;
}

static int lm48560_bulk_read(struct lm48560_data *pLM48560Data,
    unsigned char reg, unsigned int count, unsigned char *pBuf)
{
    int nResult = 0, i;
    unsigned int val = 0;

    mutex_lock(&pLM48560Data->lock);
    for (i = 0; i < count; i++) {
        nResult = regmap_read(pLM48560Data->mpRegmap, reg + i, &val);
        if (nResult < 0) {
            dev_err(pLM48560Data->dev, "%s, Reg[i] = 0x%x[%d], err = %d\n", __func__, reg, i, nResult);
            break;
        } else {
            pBuf[i] = (unsigned char)val;
            dev_dbg(pLM48560Data->dev, "%s, Reg[%d] = 0x%x\n", __func__, reg + i, pBuf[i]);
        }
    }
    mutex_unlock(&pLM48560Data->lock);

    return nResult;
}

static int lm48560_reg_write(struct lm48560_data *pLM48560Data,
    unsigned char reg, unsigned char val)
{
    int nResult = 0;

    mutex_lock(&pLM48560Data->lock);
    nResult = regmap_write(pLM48560Data->mpRegmap, reg, val);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "%s, reg = 0x%x, value = 0x%x, error = %d\n", __func__, reg, val, nResult);
    } else {
        dev_dbg(pLM48560Data->dev, "RegWrite(0x%x) = 0x%x\n", reg, val);
    }
    mutex_unlock(&pLM48560Data->lock);
    return nResult;
}

static int lm48560_bulk_write(struct lm48560_data *pLM48560Data,
    unsigned char reg, unsigned int count, unsigned char *pBuf)
{
    int nResult = 0, i;

    mutex_lock(&pLM48560Data->lock);
    for (i = 0; i < count; i++) {
        nResult = regmap_write(pLM48560Data->mpRegmap, reg + i, pBuf[i]);
        if (nResult < 0) {
            dev_err(pLM48560Data->dev, "%s, Reg[i] = 0x%x[%d], error = %d\n", __func__, reg, i, nResult);
            break;
        } else {
            dev_dbg(pLM48560Data->dev, "RegWrite(0x%x) = 0x%x\n", reg + i, pBuf[i]);
        }
    }
    mutex_unlock(&pLM48560Data->lock);
    return nResult;
}

static int lm48560_set_bits(struct lm48560_data *pLM48560Data,
    unsigned char reg, unsigned char mask, unsigned char val)
{
    int nResult = 0;

    mutex_lock(&pLM48560Data->lock);
    nResult = regmap_update_bits(pLM48560Data->mpRegmap, reg, mask, (val & mask));
    if (nResult < 0) {
        dev_err(pLM48560Data->dev,  "%s, reg = 0x%x, mask = 0x%x, value = 0x%x, error = %d\n", __func__, reg, mask, val, nResult);
    }
    mutex_unlock(&pLM48560Data->lock);
    return nResult;
}

int lm48560_power_status (void)
{
    if (NULL == g_pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return LM48560_OFF;
    }
    pr_debug("%s, running status is %d!\n", __func__, g_pLM48560Data->mbPowerUp);
    return g_pLM48560Data->mbPowerUp;
}

static int lm48560_enable(struct lm48560_data *pLM48560Data)
{
    int nResult = 0;

    dev_dbg(pLM48560Data->dev, "%s\n", __func__);
    if (!pLM48560Data->mbPowerUp) {
        if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
            /* SHDN must be high for normal operations */
            gpio_direction_output(pLM48560Data->mnGPIOEN, 1);
            msleep(15);
        }

        nResult = lm48560_reg_write(pLM48560Data,
                    LM48560_GAIN_REG,
                    pLM48560Data->mnGain);
        if (nResult < 0)
            goto err;

        nResult = lm48560_reg_write(pLM48560Data,
                    LM48560_NOCLIP_REG,
                    pLM48560Data->mnReleaseTime
                        | pLM48560Data->mnAttackTime
                        | pLM48560Data->mnVoltageLimiter);
        if (nResult < 0)
            goto err;

        nResult = lm48560_reg_write(pLM48560Data,
                    LM48560_SHUTDOWN_REG,
                    pLM48560Data->mnTurnOn
                        | pLM48560Data->mnInSel
                        | pLM48560Data->mnBstEn
                        | LM48560_DEVICE_ENABLED);
        if (nResult < 0)
            goto err;
        pLM48560Data->mbPowerUp = LM48560_ON;
    }
err:
    return nResult;
}

static int lm48560_disable(struct lm48560_data *pLM48560Data)
{
    int nResult = 0;

    dev_dbg(pLM48560Data->dev, "%s\n", __func__);
    if (pLM48560Data->mbPowerUp) {
        nResult = lm48560_reg_write(pLM48560Data,
                    LM48560_SHUTDOWN_REG,
                    pLM48560Data->mnTurnOn
                        | pLM48560Data->mnInSel
                        | pLM48560Data->mnBstEn
                        | LM48560_DEVICE_SHUTDOWN);

        if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
            gpio_direction_output(pLM48560Data->mnGPIOEN, 0);
            /* the initializatio will be lost if SHDN set to LOW */
        }
        pLM48560Data->mbPowerUp = LM48560_OFF;
    }

    return nResult;
}

void lm48560_opt (unsigned int status)
{
    int ret = 0;
    struct lm48560_data *pLM48560Data = g_pLM48560Data;
    if (NULL == pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return;
    }

    if (LM48560_ON == status) {
        ret = lm48560_enable(pLM48560Data);
        if (ret < 0) {
            pr_err("%s, lm48560 open fail!\n", __func__);
            return;
        }
    }
    else {
        ret = lm48560_disable(pLM48560Data);
        if (ret < 0) {
            pr_err("%s, lm48560 close fail!\n", __func__);
            return;
        }
    }
}

#ifdef CONFIG_OF
static int lm48560_parse_dt(struct device *dev, struct lm48560_data *pLM48560Data)
{
    struct device_node *np = dev->of_node;
    int nResult = 0;
    u32 temp;

    pLM48560Data->mnGPIOEN = of_get_named_gpio(np, "ti,reset-gpio", 0);
    if (!gpio_is_valid(pLM48560Data->mnGPIOEN)) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,reset-gpio", np->full_name, pLM48560Data->mnGPIOEN);
    } else {
        dev_dbg(pLM48560Data->dev, "ti, reset-gpio = %d\n", pLM48560Data->mnGPIOEN);
    }

    nResult = of_property_read_u32(np, "ti,turn-on", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,turn-on", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnTurnOn = ((temp & LM48560_TURNON_MASK) << LM48560_TURNON_SHIFT);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,turn-on", np->full_name, nResult);
    }

    nResult = of_property_read_u32(np, "ti,input-sel", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,input-sel", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnInSel = ((temp & LM48560_INSEL_MASK) << LM48560_INSEL_SHIFT);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,input-sel", np->full_name, nResult);
    }

    nResult = of_property_read_u32(np, "ti,boost-en", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,boost-en", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnBstEn = ((temp & LM48560_BOOST_MASK) << LM48560_BOOST_SHIFT);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,boost-en", np->full_name, nResult);

    }

    nResult = of_property_read_u32(np, "ti,release-time", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,release-time", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnReleaseTime = ((temp & LM48560_RLT_MASK) << LM48560_RLT_SHIFT);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,release-time", np->full_name, nResult);
    }

    nResult = of_property_read_u32(np, "ti,attack-time", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,attack-time", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnAttackTime = ((temp & LM48560_ATK_MASK) << LM48560_ATK_SHIFT);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,attack-time", np->full_name, nResult);
    }

    nResult = of_property_read_u32(np, "ti,voltage-limit", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n", "ti,voltage-limit", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnVoltageLimiter = (temp & LM48560_VLIM_MASK);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,voltage-limit", np->full_name, nResult);
    }

    nResult = of_property_read_u32(np, "ti,amp-gain", &temp);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s failed %d\n","ti,amp-gain", np->full_name, nResult);
    }
    else {
        pLM48560Data->mnGain = (temp & LM48560_GAIN_MASK);
        dev_err(pLM48560Data->dev, "Looking up %s property in node %s %d\n", "ti,amp-gain", np->full_name, nResult);
    }

    return nResult;
}
#endif

static bool lm48560_volatile(struct device *pDev, unsigned int nRegister)
{
    return true;
}

static bool lm48560_writeable(struct device *pDev, unsigned int nRegister)
{
    return true;
}

static int lm48560_file_open(struct inode *inode, struct file *file)
{
    struct lm48560_data *pLM48560Data = g_pLM48560Data;

    if (!try_module_get(THIS_MODULE))
        return -ENODEV;

    if (!pLM48560Data) {
        return -ENODEV;
    }

    file->private_data = (void *)pLM48560Data;
    dev_dbg(pLM48560Data->dev, "%s\n", __func__);

    return 0;
}

static int lm48560_file_release(struct inode *inode, struct file *file)
{
    struct lm48560_data *pLM48560Data = (struct lm48560_data *)file->private_data;

    if (NULL == pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return -ENODEV;
    }

    dev_dbg(pLM48560Data->dev, "%s\n", __func__);
    module_put(THIS_MODULE);

    return 0;
}

static ssize_t lm48560_file_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct lm48560_data *pLM48560Data = (struct lm48560_data *)file->private_data;
    if (NULL == pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return -ENODEV;
    }

    int nResult = 0;
    unsigned int nValue = 0;
    unsigned char nBuf[LM48560_REG_MAX_NUM];

    mutex_lock(&pLM48560Data->file_lock);

    switch (pLM48560Data->mnCmd) {
    case LM48560_CMD_REG_READ: {
        if (count == LM48560_REG_MIN_NUM) {
            nResult = lm48560_reg_read(pLM48560Data, pLM48560Data->mnCurrentReg, &nValue);
            if (nResult >= 0) {
                nResult = copy_to_user(buf, &nValue, 1);
                if (nResult != 0) {
                    /* Failed to copy all the data, exit */
                    dev_err(pLM48560Data->dev, "copy to user fail %d\n", nResult);
                }
            }
        } else if ((count > LM48560_REG_MIN_NUM) && (count < LM48560_REG_MAX_NUM)) {
            nResult = lm48560_bulk_read(pLM48560Data, pLM48560Data->mnCurrentReg, count, nBuf);
            if (nResult >= 0) {
                nResult = copy_to_user(buf, nBuf, count);
                if (nResult != 0) {
                    /* Failed to copy all the data, exit */
                    dev_err(pLM48560Data->dev, "copy to user fail %d\n", nResult);
                }
            }
        } else {
            dev_err(pLM48560Data->dev, "%s, count exceeds max register number\n", __func__);
        }
    }
    break;
    default:
        dev_err(pLM48560Data->dev, "%s, unkown command %d\n", __func__, pLM48560Data->mnCmd);
    break;
    }

    mutex_unlock(&pLM48560Data->file_lock);
    return count;
}

static ssize_t lm48560_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct lm48560_data *pLM48560Data = g_pLM48560Data;
    if (NULL == pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return -ENODEV;
    }

    int nResult = 0;
    unsigned char nBuf[LM48560_REG_MAX_NUM];
    unsigned int reg = 0, mask = 0, value = 0;
    unsigned int len = 0;

    mutex_lock(&pLM48560Data->file_lock);

    if (count > LM48560_REG_MAX_NUM) {
        dev_err(pLM48560Data->dev, "%s, exceeds count %d\n", __func__, count);
        goto end;
    }

    nResult = copy_from_user(nBuf, buf, count);
    if (nResult != 0) {
        dev_err(pLM48560Data->dev, "copy_from_user failed.\n");
        goto end;
    }

    pLM48560Data->mnCmd = nBuf[LM48560_CMD_TYPE_IND];

    switch (pLM48560Data->mnCmd) {
    case LM48560_CMD_REG_WITE:
        if (count > LM48560_CMD_LEN) {
            reg = nBuf[LM48560_CMD_REG_ADDR_IND];
            len = count - LM48560_CMD_REG_VAL1_IND;
            if (len == LM48560_REG_MIN_NUM) {
                nResult = lm48560_reg_write(pLM48560Data, reg, nBuf[LM48560_CMD_REG_VAL1_IND]);
            } else {
                nResult = lm48560_bulk_write(pLM48560Data, reg, len, &nBuf[LM48560_CMD_REG_VAL1_IND]);
            }
        } else {
            dev_err(pLM48560Data->dev, "%s, write len fail, count=%d.\n", __func__, (int)count);
        }
    break;

    case LM48560_CMD_REG_READ:
        if (count == LM48560_CMD_LEN) {
            pLM48560Data->mnCurrentReg = nBuf[LM48560_CMD_REG_ADDR_IND];
        } else {
            dev_err(pLM48560Data->dev, "read len fail.\n");
        }
    break;

    case LM48560_CMD_REG_SETBIT:
        if (count == LM48560_CMD_SETBIT_LEN) {
            reg = nBuf[LM48560_CMD_REG_ADDR_IND];
            mask = nBuf[LM48560_CMD_REG_MASK_IND];
            value = nBuf[LM48560_CMD_REG_SETBIT_VAL1_IND];
            nResult = lm48560_set_bits(pLM48560Data, reg, mask, value);
        } else {
            dev_err(pLM48560Data->dev, "read len fail.\n");
        }
    break;

    case LM48560_CMD_SHDN: {
        if (count == LM48560_CMD_LEN) {
            if (nBuf[LM48560_CMD_SHDN_STATUS_IND] > LM48560_OFF) {
                if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
                    gpio_direction_output(pLM48560Data->mnGPIOEN, LM48560_ON);
                    msleep(15);
                }
            } else if (nBuf[LM48560_CMD_SHDN_STATUS_IND] == LM48560_OFF) {
                if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
                    gpio_direction_output(pLM48560Data->mnGPIOEN, LM48560_OFF);
                    msleep(15);
                }
            } else {
                dev_err(pLM48560Data->dev, "data[%d] fail.\n", nBuf[LM48560_CMD_SHDN_STATUS_IND]);
            }
        } else {
            dev_err(pLM48560Data->dev, "LM48560_CMD_SHDN len fail.\n");
        }
    }
    break;

    case LM48560_CMD_START:
        lm48560_enable(pLM48560Data);
    break;

    case LM48560_CMD_STOP:
        lm48560_disable(pLM48560Data);
    break;

    default:
        dev_err(pLM48560Data->dev, "%s, unkown command %d\n", __func__, pLM48560Data->mnCmd);
    break;
    }

end:

    mutex_unlock(&pLM48560Data->file_lock);

    return count;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = lm48560_file_read,
    .write = lm48560_file_write,
    .open = lm48560_file_open,
    .release = lm48560_file_release,
};

#define MODULE_NAME "lm48560"
static struct miscdevice lm48560_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MODULE_NAME,
    .fops = &fops,
};

static const struct regmap_config lm48560_i2c_regmap = {
    .reg_bits = 8,
    .val_bits = 8,
    .writeable_reg = lm48560_writeable,
    .volatile_reg = lm48560_volatile,
    .cache_type = REGCACHE_NONE,
    .max_register = 3,
};

static int lm48560_probe(struct i2c_client *client,
                const struct i2c_device_id *id)
{
    struct lm48560_data *pLM48560Data;
    int nResult = 0, nValue;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "i2c is not supported\n");
        return -EIO;
    }

    pLM48560Data = devm_kzalloc(&client->dev, sizeof(struct lm48560_data),
                    GFP_KERNEL);
    if (!pLM48560Data) {
        dev_err(&client->dev, "unable to allocate memory\n");
        return -ENOMEM;
    }

    pLM48560Data->dev = &client->dev;
    i2c_set_clientdata(client, pLM48560Data);

    if (client->dev.of_node) {
#ifdef CONFIG_OF
        nResult = lm48560_parse_dt(pLM48560Data->dev, pLM48560Data);
        if (nResult < 0) {
            dev_err(pLM48560Data->dev, "parse dt fail %d\n", nResult);
            return nResult;
        }
#endif
    } else {
        dev_err(pLM48560Data->dev, "without dt, not support!\n");
        return -ENOTSUPP;
    }

    pLM48560Data->mpRegmap = devm_regmap_init_i2c(client, &lm48560_i2c_regmap);
    if (IS_ERR(pLM48560Data->mpRegmap)) {
        nResult = PTR_ERR(pLM48560Data->mpRegmap);
        dev_err(pLM48560Data->dev, "Failed to allocate register map: %d\n", nResult);
        return nResult;
    }

    mutex_init(&pLM48560Data->lock);
    if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
        nResult = gpio_request(pLM48560Data->mnGPIOEN, "LM48560-SHDN");
        if (nResult < 0) {
            dev_err(pLM48560Data->dev, "%s: GPIO %d request error\n", __func__, pLM48560Data->mnGPIOEN);
            goto err;
        }
        /* perform hardware reset */
        gpio_direction_output(pLM48560Data->mnGPIOEN, LM48560_OFF);
        msleep(10);
        gpio_direction_output(pLM48560Data->mnGPIOEN, LM48560_ON);
        msleep(15);
    }

    nResult = lm48560_reg_read(pLM48560Data, LM48560_SHUTDOWN_REG, &nValue);
    if (nResult < 0) {
        dev_err(pLM48560Data->dev, "%s, I2C fail, %d\n", __func__, nResult);
        goto err;
    }

    if (gpio_is_valid(pLM48560Data->mnGPIOEN)) {
        gpio_direction_output(pLM48560Data->mnGPIOEN, LM48560_OFF);
    }

    mutex_init(&pLM48560Data->file_lock);
    nResult = misc_register(&lm48560_misc);
    if (nResult) {
        mutex_destroy(&pLM48560Data->file_lock);
        dev_err(pLM48560Data->dev, "misc fail: %d\n", nResult);
        nResult = -1;
        goto err;
    }

    dev_set_drvdata(pLM48560Data->dev, pLM48560Data);
    g_pLM48560Data = pLM48560Data;

err:
    if (nResult < 0) {
        mutex_destroy(&pLM48560Data->lock);
        dev_err(pLM48560Data->dev, "%s, fail, %d\n", __func__, nResult);
    } else {
        dev_err(pLM48560Data->dev, "%s, succeed!\n", __func__);
    }

    return nResult;
}

static int lm48560_remove(struct i2c_client *client)
{
    struct lm48560_data *pLM48560Data = i2c_get_clientdata(client);

    if (NULL == pLM48560Data) {
        pr_err("%s, private data is NULL!\n", __func__);
        return -ENOMEM;
    }
    dev_dbg(pLM48560Data->dev, "%s\n", __func__);
    mutex_destroy(&pLM48560Data->lock);
    mutex_destroy(&pLM48560Data->file_lock);

    return 0;
}

static const struct i2c_device_id lm48560_id_table[] = {
    {"lm48560", 0},
    { },
};
MODULE_DEVICE_TABLE(i2c, lm48560_id_table);

#ifdef CONFIG_OF
static const struct of_device_id lm48560_of_id_table[] = {
    {.compatible = "ti,lm48560"},
    { },
};
#endif

static struct i2c_driver lm48560_i2c_driver = {
    .driver = {
        .name = "lm48560",
        .owner = THIS_MODULE,
        .of_match_table = lm48560_of_id_table,
    },
    .probe = lm48560_probe,
    .remove = lm48560_remove,
    .id_table = lm48560_id_table,
};

module_i2c_driver(lm48560_i2c_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("TI LM48560 chip driver");
