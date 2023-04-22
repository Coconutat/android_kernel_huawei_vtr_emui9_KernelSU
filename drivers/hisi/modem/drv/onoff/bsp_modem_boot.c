/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*lint --e{528,537,715} */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <osl_spinlock.h>
#include "mdrv_errno.h"
#include <bsp_om_enum.h>
#include <bsp_reset.h>
#include <bsp_dump.h>
#include <gunas_errno.h>
#include <bsp_slice.h>
#include <linux/ctype.h>



struct balong_power_plat_data {
    u32 modem_state;
};

enum modem_state_index {
    MODEM_NOT_READY = 0,
    MODEM_READY,
    MODEM_INVALID,
};

static struct balong_power_plat_data* balong_driver_plat_data = NULL;
static const char* const modem_state_str[] = {
    "MODEM_STATE_OFF\n",
    "MODEM_STATE_READY\n",
    "MODEM_STATE_INVALID\n",
};
/*To make modem poweroff called only once when there are two rilds.*/
static int modem_power_off_flag = 0;
static int modem_power_on_flag = 0;
spinlock_t modem_power_spinlock;


int mdrv_set_modem_state(unsigned int state)
{
    if (!balong_driver_plat_data){
        printk("Balong_power %s:%d not init.\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }
    if (state >= MODEM_INVALID){
        printk("Balong_power %s:%d invalid state 0x%x.\n", __FUNCTION__, __LINE__, state);
        return -EINVAL;
    }

    balong_driver_plat_data->modem_state = state;

    if (balong_driver_plat_data->modem_state == MODEM_READY) {
        printk(KERN_ERR"Balong_power %s:%d set state %d ,time slice %d\n", __FUNCTION__, __LINE__, state, bsp_get_elapse_ms());
    } else {
        printk(KERN_ERR"Balong_power %s:%d set state %d\n", __FUNCTION__, __LINE__, state);
    }

    return 0;
}

static ssize_t balong_power_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t len;

    if (!balong_driver_plat_data) {
        printk(KERN_ERR"Balong_power %s:%d not init.\n", __FUNCTION__, __LINE__);
        return 0;
    }
    if (balong_driver_plat_data->modem_state >= MODEM_INVALID){
        printk(KERN_ERR"Balong_power : %s:%d Invalid state 0x%x now is set.\n", __FUNCTION__, __LINE__, balong_driver_plat_data->modem_state);
        return 0;
    }

    len = snprintf(buf, strlen(modem_state_str[balong_driver_plat_data->modem_state]) + 1,
        "%s\n", modem_state_str[balong_driver_plat_data->modem_state]);

    return len;
}

ssize_t modem_reset_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    long state;
    char* endp;
    unsigned long lock_flag;
    if(count > 3)
    {
        pr_err("buf len err: %d\n", (int)count);
        return -EINVAL;
    }
    if(count ==2 && !isdigit(buf[0]))
    {
        pr_err("count = 2,buf err: %c\n", buf[0]);
        return -EINVAL;
    }
       if(count ==3 && (!isdigit(buf[0]) || !isdigit(buf[1])))
       {
        pr_err("count = 3,buf err: %c%c\n", buf[0],buf[1]);
        return -EINVAL;
    }

    dev_info(dev, "Power set to %s\n", buf);
    state = simple_strtol(buf, &endp, 10); /*10 means read as dec*/
    pr_err("count = %lu\n", (unsigned long)count);

    /* 整机复位对rild为桩,应该整机复位 */
    if (!bsp_reset_is_connect_ril())
    {
        pr_err("<modem_reset_set>: modem reset not to be connected to ril\n");
        system_error(DRV_ERRNO_RESET_REBOOT_REQ, 0, 0, NULL, 0);
        return (ssize_t)count;
    }

    if (*buf == '\0' || *buf == *endp)/* return 0 means match failed */
    {
        return (ssize_t)count;
    }

    spin_lock_irqsave(&modem_power_spinlock, lock_flag);/*lint !e550*/
    if((modem_power_off_flag)&&(!modem_power_on_flag)&&(state != BALONG_MODEM_ON))
    {
        pr_err("modem has been power off,please power on,don't reset!\n");
        spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
        return (ssize_t)count;
    }

    if (state == BALONG_MODEM_RESET) /* 切卡 */
    {
        pr_err("modem reset %d\n", BALONG_MODEM_RESET);
        system_error(DRV_ERRNO_RESET_SIM_SWITCH, 0, 0, NULL, 0);
        spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
        return (ssize_t)count;
    }
    else if(state == BALONG_MODEM_OFF)
    {
        /*To make modem poweroff called only once when there are two rilds.*/
        if(modem_power_off_flag)
        {
            pr_err("Balong_power: modem power off has been called! \n");
            spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
            return (ssize_t)count;
        }
        bsp_modem_power_off();
        pr_err("modem power off %d\n", BALONG_MODEM_OFF);
        modem_power_off_flag = 1;
        spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
        return (ssize_t)count;
    }
    else if(state == BALONG_MODEM_ON) /* TODO: 是否需要上电，根HIFI什么关系 */
    {
        if((modem_power_off_flag)&&(!modem_power_on_flag))
        {
            bsp_modem_power_on();
            modem_power_on_flag = 1;
            pr_err("modem power on %d\n", BALONG_MODEM_ON);
        }
        else
        {
            pr_err("modem now is power on!\n");
        }
    }
    else if(state == BALONG_MODEM_RILD_SYS_ERR)
    {
        pr_err("modem reset using system_error by rild %d\n", BALONG_MODEM_RILD_SYS_ERR);
        system_error(NAS_REBOOT_MOD_ID_RILD, 0, 0, NULL, 0);
    }
    else if(state == BALONG_MODEM_3RD_SYS_ERR)
    {
        pr_err("modem reset using system_error by 3rd modem %d\n", DRV_ERRNO_RESET_3RD_MODEM);
        system_error(DRV_ERRNO_RESET_3RD_MODEM, 0, 0, NULL, 0);
    }
    else
    {
        pr_err("Balong_power : invalid code to balong power !!!!\n");
        spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
        return (ssize_t)count;
    }
    spin_unlock_irqrestore(&modem_power_spinlock, lock_flag);
    return (ssize_t)count;

}

ssize_t modem_state_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return (ssize_t)count;
}

/*lint -save -e* */
static DEVICE_ATTR(state, 0660, balong_power_get, modem_reset_set);
static DEVICE_ATTR(modem_state, 0660, balong_power_get, modem_state_write);
/*lint -restore */

static int __init bsp_power_probe(struct platform_device *pdev)
{
    int ret = 0;

    balong_driver_plat_data = pdev->dev.platform_data;

    ret |= device_create_file(&(pdev->dev), &dev_attr_state);
    ret |= device_create_file(&(pdev->dev), &dev_attr_modem_state);
    if (ret)
    {
        printk("fail to creat modem boot sysfs\n");
        return ret;
    }

    return ret;
}

static struct balong_power_plat_data  balong_power_plat_data = {
    .modem_state = MODEM_NOT_READY,
};

static struct platform_device balong_power_device = {
    .name = "balong_power",
    .id = -1,
    .dev = {
        .platform_data = &balong_power_plat_data,
    },/*lint !e785*/
};/*lint !e785*/

static struct platform_driver balong_power_drv = {
    .probe      = bsp_power_probe,
    .driver     = {
        .name     = "balong_power",
        .owner    = THIS_MODULE,/*lint !e64*/
    },/*lint !e785*/
};/*lint !e785*/

static int bsp_modem_boot_init(void);

static int __init bsp_modem_boot_init(void)
{
    int ret;

    ret = platform_device_register(&balong_power_device);
    if(ret)
    {
        printk("register his_modem boot device failed.\n");
        return ret;
    }
    spin_lock_init(&modem_power_spinlock);

    ret = platform_driver_register(&balong_power_drv);  /*lint !e64*/
    if(ret)
    {
        printk("register his_modem boot driver failed.\n");
        platform_device_unregister(&balong_power_device);
    }

    return ret;
}

module_init(bsp_modem_boot_init);


