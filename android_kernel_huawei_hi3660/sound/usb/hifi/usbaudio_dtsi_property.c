/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/string.h>

//#include "usbaudio_dtsi_property.h"

#define DTS_USBAUDIO_DTSI_PROPERTY_NAME "hisilicon,usbaudio_dtsi_property"


struct usbaudio_dtsi_property_data {
	bool need_auto_suspend;
};

static struct usbaudio_dtsi_property_data *g_pdata_dtsi_property = NULL;

bool get_usbaudio_need_auto_suspend(void)
{
	if(NULL == g_pdata_dtsi_property)
		return false;

	return g_pdata_dtsi_property->need_auto_suspend;
}


static void parse_dtsi_autosuspend(struct device_node *node, struct usbaudio_dtsi_property_data  *pdata)
{
	unsigned int val = 0;
	if(NULL == pdata) {
		pr_err("%s: pdata is NULL\n", __FUNCTION__);
		return;
	}

	if(NULL == node) {
		pr_err("%s: device_node is NULL\n",__FUNCTION__);
		pdata->need_auto_suspend = false;
		return;
	}

	if (!of_property_read_u32(node, "auto_suspend", &val)){
		if(val){
			pdata->need_auto_suspend = true;
		} else {
			pdata->need_auto_suspend = false;
		}
		pr_info("%s: need_auto_suspend is %d!\n", __FUNCTION__, pdata->need_auto_suspend);
	} else {
		pdata->need_auto_suspend = false;
		pr_info("%s: need_auto_suspend set deflut is %d!\n", __FUNCTION__, pdata->need_auto_suspend);
	}

}


static int usbaudio_dtsi_property_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node =  dev->of_node;

	pr_info("%s: probe begin\n",__FUNCTION__);

	g_pdata_dtsi_property = kzalloc(sizeof(struct usbaudio_dtsi_property_data), GFP_KERNEL);
	if (NULL == g_pdata_dtsi_property) {
		pr_err("cannot allocate usbaudio_dtsi_property_data\n");
		return -ENOMEM;
	}

	parse_dtsi_autosuspend(node, g_pdata_dtsi_property);

	return 0;
}

static int usbaudio_dtsi_property_remove(struct platform_device *pdev)
{

	kfree(g_pdata_dtsi_property);
	g_pdata_dtsi_property = NULL;

	return 0;
}

static const struct of_device_id usbaudio_dtsi_property_match_table[] = {
	{
		.compatible = DTS_USBAUDIO_DTSI_PROPERTY_NAME,
	},
	{ },
};

MODULE_DEVICE_TABLE(of, usbaudio_dtsi_property_match_table);

static struct platform_driver usbaudio_dtsi_property_driver = {
	.driver =
	{
		.name  = "usbaudio_dtsi_property",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(usbaudio_dtsi_property_match_table),
	},
	.probe  = usbaudio_dtsi_property_probe,
	.remove = usbaudio_dtsi_property_remove,
};

static int __init usbaudio_dtsi_property_init(void)
{
	return platform_driver_register(&usbaudio_dtsi_property_driver);
}

static void __exit usbaudio_dtsi_property_exit(void)
{
	platform_driver_unregister(&usbaudio_dtsi_property_driver);
}


fs_initcall_sync(usbaudio_dtsi_property_init);
module_exit(usbaudio_dtsi_property_exit);

MODULE_DESCRIPTION("hisi usbaudio dtsi property driver");
MODULE_LICENSE("GPL");

