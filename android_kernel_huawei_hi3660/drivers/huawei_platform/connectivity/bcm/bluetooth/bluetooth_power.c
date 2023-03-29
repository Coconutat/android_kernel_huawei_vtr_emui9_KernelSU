/* Copyright (c) 2009-2013,	Code Aurora	Forum. All rights reserved.
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 and
 * only	version	2 as published by the Free Software	Foundation.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A	PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received	a copy of the GNU General Public License
 * along with this program;	if not,	write to the Free Software
 * Foundation, Inc., 51	Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */


/*
 * Bluetooth Power Switch Module
 * controls	power to external Bluetooth	device
 * with	interface to power management device
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>

#include <linux/clk.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mtd/hisi_nve_interface.h>
#ifdef CONFIG_HWCONNECTIVITY
#include <huawei_platform/connectivity/hw_connectivity.h>
#endif

#define	PROC_CAL_DIR	"bluetooth_power/data"

#define DTS_COMP_BLUETOOTH_POWER_NAME "huawei,bluetooth_power"

#define BLUETOOTH_HUAWEI_NV_BTNVRAM_NAME                             "BTCALNV"
#define BLUETOOTH_HUAWEI_NV_BTNVRAM_NAME_LENGTH               7
#define BLUETOOTH_HUAWEI_NV_BTNVRAM_NUMBER                         352
#define BLUETOOTH_HUAWEI_NV_BTNVRAM_LENGTH                          104

typedef struct bluepower_data {
	struct gpio gpio_enable;	/* gpio BT_enable */
#if defined(CONFIG_BCM4343)
	struct pinctrl *bt_pin;	/* pinctrl */
	struct pinctrl_state *bt_pin_default;	/*Normal mode */
	struct pinctrl_state *bt_pin_idle;	/*Lowpower mode */
#endif
	struct clk *clk;	/* bt 32k clock */
	struct rfkill *rfkill;
	bool previous;
	struct proc_dir_entry *bluetooth_power_dir;
	struct proc_dir_entry *data_dir;
	unsigned pwr_count;	/* only if count equals 0, then open bluetooth */
} btpower_data;

/*************************************************
Function:	bluetooth_power_handle
Description:power on / power off  Bluetooth
Calls:		bluetooth_power_handle()
Input:		platform_device *pdev
			bluepower_data *p_dev_data
Output:
Return:		int ret
Others:		NA
*************************************************/
static int bluetooth_power_handle(struct bluepower_data *p_dev_data, int on)
{
	int ret = 0;

	pr_info("%s,%d.\n", __func__, on);

	/* power on     the     bluetooth */
	if (on) {
		if (p_dev_data->pwr_count == 0) {
			pr_info("%s, bluetooth power on in\n", __func__);
#if defined(CONFIG_BCM4343)
			/*SET NORMAL */
			if (NULL != p_dev_data->bt_pin_default) {
				pinctrl_select_state(p_dev_data->bt_pin,
						     p_dev_data->bt_pin_default);
			} else {
				pr_err
				    ("%s, p_dev_data->bt_pin_default is NULL!\n",
				     __func__);
			}
#endif

			/*enable bt     sleep clk */

			pr_info("enable bluetooth 32 clk======>\n");
			ret = clk_prepare_enable(p_dev_data->clk);
			if (unlikely(ret < 0)) {
				pr_err("%s : enable	clk	failed %d",
				       __func__, ret);
				goto clk_err;
			}
			msleep(5);

			pr_info("power on gpio BT_EN\n");
			gpio_set_value(p_dev_data->gpio_enable.gpio, 1);
			msleep(5);
			p_dev_data->pwr_count = 1;
			printk
			    ("%s, bluetooth power on out. bt enable gpio is %d\n",
			     __func__, p_dev_data->gpio_enable.gpio);

		}		/* end if pwr_count == 0 */
	} else {
		/* power off the bluetooth */
		if (p_dev_data->pwr_count == 1) {
			pr_info("%s, bluetooth power off in\n", __func__);
#if defined(CONFIG_BCM4343)
			/*SET LOWPOWER */
			if (NULL != p_dev_data->bt_pin_idle) {
				pinctrl_select_state(p_dev_data->bt_pin,
						     p_dev_data->bt_pin_idle);
			} else {
				pr_err("%s, p_dev_data->bt_pin_idle is NULL!\n",
				       __func__);
			}
#endif

			gpio_set_value(p_dev_data->gpio_enable.gpio, 0);
			msleep(5);

			/*disable sleep clk */
			clk_disable(p_dev_data->clk);

			p_dev_data->pwr_count = 0;
			pr_info("%s, bluetooth power off out\n", __func__);

		}		/* end if pwr_count == 1 */
	}			/* end else on == 0 */

	return ret;

clk_err:
	/*pinctrl_select_state(bluetooth_dev_data.bt_uart, bluetooth_dev_data.bt_uart_idle); */
	return ret;
}

/*************************************************
Function:	bluetooth_toggle_radio
Description:be called by system when rfkill is triggered
Calls:		bluetooth_power_handle()
Input:		platform_device *pdev
			bluepower_data *p_dev_data
Output:
Return:		int ret
Others:		NA
*************************************************/
static int bluetooth_toggle_radio(void *data, bool blocked)
{
	int ret = 0;
	struct bluepower_data *bt_data = NULL;

	pr_info("bluetooth_toggle_radio in\n");

	if (NULL == data) {
		ret = -1;
		return ret;
	}

	bt_data = platform_get_drvdata(data);
	if (NULL == bt_data) {
		pr_err("%s:	bt_data is NULL\n", __func__);
		return ret;
	}

	pr_info("bluetooth_toggle_radio blocked: %d , previous: %d\n",
	       blocked, bt_data->previous);

	if (bt_data->previous != blocked) {
		ret = bluetooth_power_handle(bt_data, (!blocked));
		bt_data->previous = blocked;
	}

	pr_info("bluetooth_toggle_radio out\n");

	return ret;
}

static const struct rfkill_ops bluetooth_power_rfkill_ops = {
	.set_block = bluetooth_toggle_radio,
};

/*************************************************
Function:	bluetooth_power_rfkill_probe
Description:rfkill init function
Calls:		rfkill_alloc()
			rfkill_init_sw_state()
			rfkill_register()
			rfkill_destroy()
Input:		platform_device *pdev
			bluepower_data *p_dev_data
Output:
Return:		int ret
Others:		NA
*************************************************/
static int bluetooth_power_rfkill_probe(struct platform_device *pdev,
					struct bluepower_data *p_dev_data)
{
	int ret = 0;

	pr_info("bluetooth_power_rfkill_probe in\n");

	/* alloc memery for rfkill */
	p_dev_data->rfkill = rfkill_alloc("bt_power",
					  &pdev->dev, RFKILL_TYPE_BLUETOOTH,
					  &bluetooth_power_rfkill_ops, pdev);
	if (!p_dev_data->rfkill) {
		dev_err(&pdev->dev, "bluetooth rfkill allocate failed\n");
		return -ENOMEM;
	}

	/* force Bluetooth off during init to allow     for     user control */
	rfkill_init_sw_state(p_dev_data->rfkill, 1);
	p_dev_data->previous = 1;

	ret = rfkill_register(p_dev_data->rfkill);
	if (ret) {
		dev_err(&pdev->dev, "rfkill	register failed=%d\n", ret);
		goto rfkill_failed;
	}

	pr_info("bluetooth_power_rfkill_probe out\n");

	return ret;

rfkill_failed:
	rfkill_destroy(p_dev_data->rfkill);
	return ret;
}

/*************************************************
Function:       bluetooth_power_load_calibrate_data
Description:    bluetooth_power_load_calibrate_data
Calls:          bluetooth_power_load_calibrate_data
Input:          platform_device *pdev
Output:
Return:         int ret
Others:         NA
*************************************************/
static int bluetooth_power_load_calibrate_data(char * calbufp, uint size)
{
	int ret = 0;
	struct hisi_nve_info_user  info;

	pr_info("bluetooth_power_load_calibrate_data start \n");
	memset(calbufp, 0x00, size);
	memset(&info, 0, sizeof(info));

	info.nv_number  = BLUETOOTH_HUAWEI_NV_BTNVRAM_NUMBER;   //nve item
	info.valid_size = BLUETOOTH_HUAWEI_NV_BTNVRAM_LENGTH;
	info.nv_operation = NV_READ;
	strncpy(info.nv_name, BLUETOOTH_HUAWEI_NV_BTNVRAM_NAME, BLUETOOTH_HUAWEI_NV_BTNVRAM_NAME_LENGTH);

	ret = hisi_nve_direct_access( &info );
	if (!ret)
	{
		strncpy(calbufp, info.nv_data, size);
		pr_info("bluetooth_power_load_calibrate_data is: %s \n", calbufp);
		return strlen(calbufp);
	}

	pr_err("%s: failed \n",  __func__);

	return -1;
}

/*************************************************
Function:       bluetooth_power_read_data_calibrate
Description:    bluetooth_power_read_data_calibrate
Calls:          bluetooth_power_read_data_calibrate
Input:          platform_device *pdev
Output:
Return:         int ret
Others:         NA
*************************************************/
 static ssize_t bluetooth_power_read_data_calibrate(struct file *filp,
					  char __user *buffer,
					  size_t len, loff_t *off)
{
       char BtCalDateBuf[BLUETOOTH_HUAWEI_NV_BTNVRAM_LENGTH + 1];
	int CalDataLen = 0;

	pr_info("bluetooth_power_read_data_calibrate \n");

	CalDataLen = bluetooth_power_load_calibrate_data(BtCalDateBuf, sizeof(BtCalDateBuf));
	if (CalDataLen <= 0) {
		pr_err("%s: calibrate data is empty, no need to calibrate nvram\n",  __func__);
		return -EINVAL;

	}

	if( 0 != copy_to_user(buffer, BtCalDateBuf, CalDataLen))
	{
		pr_err("bluetooth_power_read_data_calibrate copy to user failed \n");
		return -EFAULT;
	}

	pr_info("bluetooth_power_read_data_calibrate read success !! len =%d \n", CalDataLen);
	return 0;
}

static const struct file_operations bt_power_proc_fops_data_calibrate = {
	.owner = THIS_MODULE,
	.read = bluetooth_power_read_data_calibrate,
};

/*************************************************
Function:       bluetooth_power_create_proc_sys
Description:    bluetooth power create proc system
Calls:          bluetooth_power_create_proc_sys
Input:          platform_device *pdev
Output:
Return:         int ret
Others:         NA
*************************************************/
static int bluetooth_power_create_proc_sys(struct bluepower_data *bs_data)
{
	int retval = 0;

       pr_info("bluetooth_power_create_proc_sys in\n");
	bs_data->bluetooth_power_dir = proc_mkdir("bluetooth_power", NULL);
	if (bs_data->bluetooth_power_dir == NULL) {
		pr_err("%s: unable to create /proc/bluetooth_power directory",
		       __func__);
		retval = -ENOMEM;
		return retval;
	}

	bs_data->data_dir = proc_mkdir("data", bs_data->bluetooth_power_dir);
	if (bs_data->data_dir == NULL) {
		pr_err("%s: unable to create /proc/%s directory", __func__,
		       PROC_CAL_DIR);
		retval = -ENOMEM;
		goto err_proc_data;
	}

	/* write proc entries calibrate */
	proc_create("calibrate", S_IRUGO | S_IFREG, bs_data->data_dir,
		    &bt_power_proc_fops_data_calibrate);

       pr_info("bluetooth_power_create_proc_sys out\n");
	return retval;

err_proc_data:
	remove_proc_entry("bluetooth_power", NULL);

	return retval;
}

/*************************************************
Function:       bluetooth_power_probe
Description:    bluetooth power driver probe function
Calls:          bluetooth_power_rfkill_probe
Input:          platform_device *pdev
Output:
Return:         int ret
Others:         NA
*************************************************/
static int bluetooth_power_probe(struct platform_device *pdev)
{
	struct device *bluetooth_power_dev = &pdev->dev;
	struct device_node *np = bluetooth_power_dev->of_node;
	struct bluepower_data *bt_data = NULL;
#if defined(CONFIG_BCM4343)
	struct pinctrl *p = NULL;
	enum of_gpio_flags gpio_flags;
	struct pinctrl_state *s_default = NULL;
	struct pinctrl_state *s_idle = NULL;
#endif
	const char *clk_name = NULL;
	int ret = 0;
#if !defined(CONFIG_BCM4343)
	/* udp board doesn't have vio */
	int no_vio_switch = 0;
	int vio_enable = 0;
#endif
	bt_data =
	    devm_kzalloc(bluetooth_power_dev, sizeof(struct bluepower_data),
			 GFP_KERNEL);
	if (bt_data == NULL) {
		dev_err(bluetooth_power_dev,
			"failed to allocate bluepower_data\n");
		return -ENOMEM;
	}

	pr_info("bluetooth_power probe in\n");

#if defined(CONFIG_BCM4343)
	p = pinctrl_get(bluetooth_power_dev);
	if (IS_ERR(p)) {
		pr_err("%s: pinctrl_get failed! p=%p.\n", __func__, p);
		goto err_free_bluepower_data_ret;
	}

	bt_data->bt_pin = p;

	s_default =
	    pinctrl_lookup_state(bt_data->bt_pin, PINCTRL_STATE_DEFAULT);
	s_idle = pinctrl_lookup_state(bt_data->bt_pin, PINCTRL_STATE_IDLE);
	if (IS_ERR(s_default) || IS_ERR(s_idle)) {
		pr_err
		    ("%s: pinctrl_lookup_state failed! default=%p, idle=%p.\n",
		     __func__, s_default, s_idle);
		bt_data->bt_pin_default = NULL;
		bt_data->bt_pin_idle = NULL;
	} else {
		bt_data->bt_pin_default = s_default;
		bt_data->bt_pin_idle = s_idle;
	}
#endif

#if defined(CONFIG_BCM4343)
	bt_data->gpio_enable.gpio =
	    of_get_gpio_by_prop(np, "huawei,bt_en", 0, 0, &gpio_flags);
#else
	of_property_read_u32(np, "huawei,bt_en", &(bt_data->gpio_enable.gpio));
#endif
	if (!gpio_is_valid(bt_data->gpio_enable.gpio)) {
		pr_err("%s: get bt_en gpio failed. gpio=%d\n", __func__,
		       bt_data->gpio_enable.gpio);
#if defined(CONFIG_BCM4343)
		goto err_free_pinctrl_ret;
#else
		goto err_free_bluepower_data_ret;
#endif
	}
#if defined(CONFIG_BCM4343)
	ret = of_property_read_string(np, "huawei,ck32b", &clk_name);
	if (ret || IS_ERR(clk_name)) {
		pr_err("%s: read ck32b failed, ret=%d, clk_name=%p\n", __func__,
		       ret, clk_name);
		goto err_free_pinctrl_ret;
	}
#endif

	bt_data->clk = devm_clk_get(bluetooth_power_dev, clk_name);
	if (IS_ERR(bt_data->clk)) {
		pr_err("%s: Get bt 32k clk failed, clk=%p\n", __func__,
		       bt_data->clk);
		ret = PTR_ERR(bt_data->clk);
#if defined(CONFIG_BCM4343)
		goto err_free_pinctrl_ret;
#else
		goto err_free_bluepower_data_ret;
#endif
	}

	ret = gpio_request(bt_data->gpio_enable.gpio, "bt_gpio_enable");
	if (ret < 0) {
		pr_err("%s: gpio_request %d  failed, ret:%d .\n", __func__,
		       bt_data->gpio_enable.gpio, ret);
		goto err_free_clk_ret;
	}

	ret = gpio_direction_output(bt_data->gpio_enable.gpio, 0);
	if (ret < 0) {
		pr_err("%s: gpio_direction_output %d  failed, ret:%d .\n",
		       __func__, bt_data->gpio_enable.gpio, ret);
		goto err_free_bt_en;
	}
#if !defined(CONFIG_BCM4343)

	of_property_read_u32(np, "huawei,no_vio_switch", &no_vio_switch);
	pr_info("%s: no_vio_switch value is %d\n", __func__, no_vio_switch);
	if (no_vio_switch != 1) {
		pr_err("%s: not udp board, set vio swtich.\n", __func__);
		of_property_read_u32(np, "huawei,vio_en", &vio_enable);
		pr_info("%s: vio_enable gpio is %d\n", __func__, vio_enable);
		if (!gpio_is_valid(vio_enable)) {
			pr_err("%s: get vio_enable (%d) failed.\n", __func__,
			       vio_enable);
			ret = -1;
			goto err_free_bt_en;
		}

		ret = gpio_request(vio_enable, NULL);
		if (ret < 0) {
			pr_err("%s: vio gpio_request failed, ret:%d.\n",
			       __func__, ret);
			goto err_free_bt_en;
		}

		ret = gpio_direction_output(vio_enable, 1);
		gpio_free(vio_enable);
		if (ret < 0) {
			pr_err("%s: gpio_direction_output %d failed, ret:%d.\n",
			       __func__, vio_enable, ret);
			goto err_free_bt_en;
		}
	} else {
		pr_err("%s: this board doesn't have vio switch, do nothing.\n",
		       __func__);
	}
#endif /*CONFIG_BCM4343 */

	platform_set_drvdata(pdev, bt_data);

	ret = bluetooth_power_rfkill_probe(pdev, bt_data);
	if (ret) {
		dev_err(&pdev->dev,
			"bluetooth_power_rfkill_probe failed, ret:%d\n", ret);
		goto err_free_bt_en;
	}

	ret = bluetooth_power_create_proc_sys(bt_data);
	if (ret) {
		dev_err(&pdev->dev,
			"bluetooth_power_create_proc_sys failed, ret:%d\n", ret);
		goto err_free_bt_en;
	}

	pr_info("bluetooth_power probe out\n");

	return ret;

err_free_bt_en:
	gpio_free(bt_data->gpio_enable.gpio);
err_free_clk_ret:
	devm_clk_put(&pdev->dev, bt_data->clk);
#if defined(CONFIG_BCM4343)
err_free_pinctrl_ret:
	pinctrl_put(bt_data->bt_pin);
#endif
err_free_bluepower_data_ret:
	devm_kfree(bluetooth_power_dev, bt_data);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static void bluetooth_power_rfkill_remove(struct platform_device *pdev)
{
	struct bluepower_data *bt_data = NULL;

	if (NULL == pdev)
		return;

	bt_data = platform_get_drvdata(pdev);

	if (NULL == bt_data) {
		dev_err(&pdev->dev, "%s, bt_info is null\n", __func__);
		return;
	}

	if (bt_data->rfkill) {
		rfkill_unregister(bt_data->rfkill);
		rfkill_destroy(bt_data->rfkill);
	}

}

static int bluetooth_power_remove(struct platform_device *pdev)
{
	struct bluepower_data *bt_data = NULL;
	struct device *bluetooth_power_dev = NULL;

	if (NULL == pdev)
		return 0;

	bluetooth_power_dev = &pdev->dev;

	bt_data = platform_get_drvdata(pdev);
	if (NULL == bt_data) {
		pr_err("%s:	bt_data is NULL\n", __func__);
		return 0;
	}

	bluetooth_power_handle(bt_data, 0);
	gpio_free(bt_data->gpio_enable.gpio);

	if (bt_data->clk)
		clk_put(bt_data->clk);
#if defined(CONFIG_BCM4343)
	if (NULL != bt_data->bt_pin_idle)
		pinctrl_select_state(bt_data->bt_pin, bt_data->bt_pin_idle);

	if (NULL != bt_data->bt_pin)
		pinctrl_put(bt_data->bt_pin);
#endif

	bluetooth_power_rfkill_remove(pdev);

	devm_kfree(bluetooth_power_dev, bt_data);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static int bluetooth_power_suspend(struct platform_device *pdev,
				   pm_message_t state)
{
	pr_info("%s+.\n", __func__);
	pr_info("%s-.\n", __func__);
	return 0;
}

static int bluetooth_power_resume(struct platform_device *pdev)
{
	pr_info("%s+.\n", __func__);
	pr_info("%s-.\n", __func__);
	return 0;
}

static const struct of_device_id bluetooth_power_match_table[] = {
	{
	 .compatible = DTS_COMP_BLUETOOTH_POWER_NAME,	/* compatible must match with which defined in dts */
	 .data = NULL,
	 },
	{
	 },
};

MODULE_DEVICE_TABLE(of, bluetooth_power_match_table);

static struct platform_driver bluetooth_power_driver = {
	.probe = bluetooth_power_probe,
	.remove = bluetooth_power_remove,
	.suspend = bluetooth_power_suspend,
	.resume = bluetooth_power_resume,	/* TODO no shutdown ? */

	.driver = {
		   .name = "huawei,bluetooth_power",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(bluetooth_power_match_table),	/* dts required code */
		   },

};

static int __init bluetooth_power_init(void)
{
	int ret;

#ifdef CONFIG_HWCONNECTIVITY
	/*For OneTrack, we need check it's the right chip type or not.
	   If it's not the right chip type, don't init the driver */
	if (!isMyConnectivityChip(CHIP_TYPE_BCM)) {
		pr_err("BT chip type is not match, skip driver init");
		return -EINVAL;
	}
	pr_info("BT chip type is matched with Broadcom, continue");
#endif

	ret = platform_driver_register(&bluetooth_power_driver);
	return ret;
}

static void __exit bluetooth_power_exit(void)
{

	platform_driver_unregister(&bluetooth_power_driver);
}

module_init(bluetooth_power_init);
module_exit(bluetooth_power_exit);

MODULE_LICENSE("GPL v2");
