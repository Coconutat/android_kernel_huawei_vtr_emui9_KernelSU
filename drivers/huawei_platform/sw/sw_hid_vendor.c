#include <linux/kref.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/hidraw.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/tty.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <huawei_platform/log/log_exception.h>
#include "sw_core.h"
#include "sw_debug.h"

#include <linux/device.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/delay.h>

#ifdef CONFIG_HUAWEI_PLATFORM
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG sw_core
HWLOG_REGIST();
#else
#define hwlog_debug(fmt, args...)do { printk(KERN_DEBUG   "[hw_kb]" fmt, ## args); } while (0)
#define hwlog_info(fmt, args...) do { printk(KERN_INFO    "[hw_kb]" fmt, ## args); } while (0)
#define hwlog_warn(fmt, args...) do { printk(KERN_WARNING"[hw_kb]" fmt, ## args); } while (0)
#define hwlog_err(fmt, args...)  do { printk(KERN_ERR   "[hw_kb]" fmt, ## args); } while (0)
#endif



static u8 KEYBOARD_HID_ReportDesc[] = {
	//===============================================================
	//  Standard Keycode
	//===============================================================

		0x05, 0x01,                //Usage Page (Generic Desktop)
		0x09, 0x06,                //Usage (Keyboard)
		0xA1, 0x01,                //Collection (Application)
		0x05, 0x07,                //  Usage Page (Keyboard/Keypad)
		0x19, 0xE0,                //  Usage Minimum (Keyboard Left Control)
		0x29, 0xE7,                //  Usage Maximum (Keyboard Right GUI)
		0x15, 0x00,                //  Logical Minimum (0)
		0x25, 0x01,                //  Logical Maximum (1)
		0x75, 0x01,                //  Report Size (1)
		0x95, 0x08,                //  Report Count (8)
		0x81, 0x02,                //  Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
		0x95, 0x01,                //  Report Count (1)
		0x75, 0x08,                //  Report Size (8)
		0x81, 0x01,                //  Input (Cnst,Ary,Abs)
		0x95, 0x05,                //  Report Count (5)
		0x75, 0x01,                //  Report Size (1)
		0x05, 0x08,                //  Usage Page (LEDs)
		0x19, 0x01,                //  Usage Minimum (Num Lock)
		0x29, 0x05,                //  Usage Maximum (Kana)
		0x91, 0x02,                //  Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
		0x95, 0x01,                //  Report Count (1)
		0x75, 0x03,                //  Report Size (3)
		0x91, 0x01,                //  Output (Cnst,Ary,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
		0x95, 0x06,                //  Report Count (6)
		0x75, 0x08,                //  Report Size (8)
		0x15, 0x00,                //  Logical Minimum (0)
		0x26, 0xFF, 0x00,          //  Logical Maximum (255)
		0x05, 0x07,                //  Usage Page (Keyboard/Keypad)
		0x19, 0x00,                //  Usage Minimum (Undefined)
		0x2A, 0xFF, 0x00,          //  Usage Maximum (Keyboard ExSel)
		0x81, 0x00,                //  Input (Data,Ary,Abs)
		0x09, 0x05,                         //     Usage (Vendor Defined)
		0x15, 0x00,                         //     Logical Minimum (0)
		0x26, 0xFF, 0x00,                   //     Logical Maximum (255)
		0x75, 0x08,                         //     Report Size (8 bit)
		0x95, 0x02,                         //     Report Count (2)
		0xB1, 0x02,                         //     Feature (Data, Variable, Absolute)
		0xC0                       //End Collection


};




static u8 CustomKey_HID_ReportDesc[] = {

	//===============================================================
	// Report ID 4:  Advanced buttons
	//===============================================================
		0x05, 0x0C,             //Usage Page
		0x09, 0x01,             //Usage
		0xA1, 0x01,             //Collection
		0x19, 0x00,             //Usage Minimum
		0x2A, 0x9C, 0x02,       //Usage Maximum
		0x15, 0x00,             //Logical Minimum
		0x26, 0x9C, 0x02,       //Logical Maximum
		0x75, 0x10,             //Report Size
		0x95, 0x01,             //Report Count
		0x81, 0x00,             //Input
		0xC0,                   //End Collection
};

#define MOUSE_V1_MAINVER        (0)
#define MOUSE_V1_SUBVER_LIMITE  (3)

#define MOUSE_V2_MAINVER        (0)
#define MOUSE_V2_SUBVER_LIMITE  (5)
//===============================================================
// Report ID : Mouse buttons + X + Y + Z
// Used by keyboard firmware version < 0.3
//===============================================================
static u8 Mouse_HID_ReportDesc[] = {
		0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
		0x09, 0x02,                    // USAGE (Mouse)
		0xa1, 0x01,                    // COLLECTION (Application)
		0x09, 0x01,                    //   USAGE (Pointer)

		0xa1, 0x00,                    //   COLLECTION (Physical)
		0x05, 0x09,                    //     USAGE_PAGE (Button)
		0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
		0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)

		0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
		0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
		0x75, 0x01,                    //     REPORT_SIZE (1)
		0x95, 0x03,                    //     REPORT_COUNT (3)
		0x81, 0x02,                    //     INPUT (Data, Var, Abs)

		0x95, 0x01,                    //     REPORT_COUNT (1)
		0x75, 0x05,                    //     REPORT_SIZE (5)
		0x81, 0x01,                    //     INPUT (Cnst, Var, Abs)

		0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
		0x09, 0x30,                    //     USAGE (X)
		0x09, 0x31,                    //     USAGE (Y)
		0x16, 0x01,0x80,                    //     LOGICAL_MINIMUM (-32767)
		0x26, 0xff,0x7f,                    //     LOGICAL_MAXIMUM (32767)
		0x75, 0x10,                    //     REPORT_SIZE (16)
		0x95, 0x02,                    //     REPORT_COUNT (2)
		0x81, 0x06,                    //     INPUT (Data, Var, Abs)
		0xc0,                          //   END_COLLECTION
		0xc0                           // END_COLLECTION
};

//===============================================================
// Report ID New: Mouse buttons + X + Y + Z + Wheel
// Used by keyboard firmware version >=0.3  <=0.4
// only support Up-down Wheel Feature
//===============================================================

static u8 Mouse_HID_ReportDesc_v2[] = {
		0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
		0x09, 0x02,                    // USAGE (Mouse)
		0xa1, 0x01,                    // COLLECTION (Application)
		0x09, 0x01,                    //   USAGE (Pointer)

		0xa1, 0x00,                    //   COLLECTION (Physical)
		0x05, 0x09,                    //     USAGE_PAGE (Button)
		0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
		0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)

		0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
		0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
		0x75, 0x01,                    //     REPORT_SIZE (1)
		0x95, 0x03,                    //     REPORT_COUNT (3)
		0x81, 0x02,                    //     INPUT (Data, Var, Abs)

		0x95, 0x01,                    //     REPORT_COUNT (1)
		0x75, 0x05,                    //     REPORT_SIZE (5)
		0x81, 0x01,                    //     INPUT (Cnst, Var, Abs)

		0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
		0x09, 0x30,                    //     USAGE (X)
		0x09, 0x31,                    //     USAGE (Y)
		0x16, 0x01,0x80,               //     LOGICAL_MINIMUM (-32767)
		0x26, 0xff,0x7f,               //     LOGICAL_MAXIMUM (32767)
		0x75, 0x10,                    //     REPORT_SIZE (16)
		0x95, 0x02,                    //    Report Count (2)
		0x81, 0x06,                    //     INPUT (Data, Var, Abs)

		0x09, 0x38,                    //     Usage (Wheel)
		0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
		0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
		0x75, 0x08,                    //     REPORT_SIZE (8)
		0x95, 0x01,                    //     REPORT_COUNT (1)
		0x81, 0x06,                    //     INPUT (Data, Var, Abs)

		0xc0,                          //   END_COLLECTION
		0xc0                           // END_COLLECTION
};

//===============================================================
// Report ID New: Mouse buttons + X + Y + Z + Wheel
// Used by keyboard firmware version >=0.5
// Support Up-Down  Left-Right Wheel Feature
//===============================================================

static u8 Mouse_HID_ReportDesc_v3[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)

    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)

    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x02,                    //     INPUT (Data, Var, Abs)

    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x01,                    //     INPUT (Cnst, Var, Abs)

    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x16, 0x01,0x80,               //     LOGICAL_MINIMUM (-32767)
    0x26, 0xff,0x7f,               //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x02,                    //    Report Count (2)
    0x81, 0x06,                    //     INPUT (Data, Var, Abs)

    0x09, 0x38,                    //     Usage (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data, Var, Abs)

    0x05, 0x0C,                    //    Usage Page (Consumer Devices)
    0x0A, 0x38, 0x02,              //    Usage (AC Pan)
    0x15, 0x80,                    //    Logical Minimum (-128)
    0x25, 0x7F,                    //    Logical Maximum (127)
    0x75, 0x08,                    //    Report Size (8)
    0x95, 0x01,                    //    Report Count (1)
    0x81, 0x06,                    //    Input (Data,Var,Rel,NWrp,Lin,Pref,NNul,Bit)

    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

static int create_hid_consumer_devices(struct hw_sw_core_data* core_data,struct platform_device *dev)
{
	struct sw_device* device = NULL;
	int ret = -1;
	SW_PRINT_FUNCTION_NAME;
	device = sw_allocate_device();
	if(!device)
	{
		return -1;
	}
	device->type = SW_HID_TYPE;
	device->bus	= SW_BUS_ANY;
	device->group	= SW_GROUP_ANY;
	device->vendor	= core_data->vendor;
	device->product = core_data->product;
	device->dev.parent = &dev->dev;

	device->rd_data = CustomKey_HID_ReportDesc;
	device->rd_size = sizeof(CustomKey_HID_ReportDesc) / sizeof(u8);

	dev_set_name(&device->dev, "SW%04X:%04X:%04X.%04X03", device->bus,
	device->vendor, device->product,device->type);

	ret = device_register(&device->dev);
	if(ret)
	{
		SW_PRINT_ERR("[SW_HID] device_add %x\n",ret);
	}

	core_data->key_device = device;

	return ret ;
}

static int create_hid_mouse_devices(struct hw_sw_core_data* core_data,struct platform_device *dev)
{
	struct sw_device* device= NULL;
	int ret = -1;
	SW_PRINT_FUNCTION_NAME;
	device = sw_allocate_device();
	if(!device)
	{
		return -1;
	}
	device->type = SW_HID_TYPE;
	device->bus	= SW_BUS_ANY;
	device->group	= SW_GROUP_ANY;
	device->vendor	= core_data->vendor;
	device->product = core_data->product;
	device->dev.parent = &dev->dev;

	//keyboard firmware verison < 0.3 , use old mouse desc
	// >= 0.3  <=0.4, use v2 mouse desc support wheel button
	// >=0.5, use v3 mouse desc support wheel button
	if((core_data->mainver==MOUSE_V1_MAINVER) && (core_data->subver < MOUSE_V1_SUBVER_LIMITE))
	{
		device->rd_data = Mouse_HID_ReportDesc;
		device->rd_size = sizeof(Mouse_HID_ReportDesc) / sizeof(u8);
	}else if((core_data->mainver==MOUSE_V2_MAINVER) && (core_data->subver < MOUSE_V2_SUBVER_LIMITE))
	{
		device->rd_data = Mouse_HID_ReportDesc_v2;
		device->rd_size = sizeof(Mouse_HID_ReportDesc_v2) / sizeof(u8);
	}else
	{
		device->rd_data = Mouse_HID_ReportDesc_v3;
		device->rd_size = sizeof(Mouse_HID_ReportDesc_v3) / sizeof(u8);
	}


	dev_set_name(&device->dev, "SW%04X:%04X:%04X.%04X02", device->bus,
	device->vendor, device->product,device->type);

	ret = device_register(&device->dev);
	if(ret)
	{
		SW_PRINT_ERR("[YY] device_add %x\n",ret);
	}
	SW_PRINT_ERR("[YY] sw alloc device end\n");

	core_data->mouse_device = device;

	return ret ;
}

static int create_hid_keyboard_devices(struct hw_sw_core_data* core_data,struct platform_device *dev)
{
	struct sw_device* device= NULL;
	int ret = -1;
	SW_PRINT_FUNCTION_NAME;
	device = sw_allocate_device();
	if(!device)
	{
		return -1;
	}
	device->type = SW_HID_TYPE;
	device->bus	= SW_BUS_ANY;
	device->group	= SW_GROUP_ANY;
	device->vendor	= core_data->vendor;
	device->product = core_data->product;
	device->dev.parent = &dev->dev;

	device->rd_data = KEYBOARD_HID_ReportDesc;
	device->rd_size = sizeof(KEYBOARD_HID_ReportDesc) /sizeof(u8);

	dev_set_name(&device->dev, "SW%04X:%04X:%04X.%04X01", device->bus,
	device->vendor, device->product,device->type);

	ret = device_register(&device->dev);
	if(ret)
	{
		SW_PRINT_ERR("[YY] device_add %x\n",ret);
	}
	SW_PRINT_ERR("[YY] sw alloc device end\n");

	core_data->device = device;

	return ret ;
}

int create_hid_devices(struct hw_sw_core_data* core_data,struct platform_device *dev)
{
	int ret = -1;
	int val = -1;
	if(!core_data->device)
	{
		val = create_hid_keyboard_devices(core_data,dev);
		if(val)
		{
			SW_PRINT_ERR(" create_hid_keyboard_devices Error (%x)\n",val);
			ret = val;
		}
	}

	if(!core_data->mouse_device)
	{
		val = create_hid_mouse_devices(core_data,dev);
		if(val)
		{
			SW_PRINT_ERR(" create_hid_mouse_devices Error (%x)\n",val);
			ret = val;
		}
	}

	if(!core_data->key_device)
	{
		val = create_hid_consumer_devices(core_data,dev);
		if(val)
		{
			SW_PRINT_ERR(" create_hid_consumer_devices Error (%x)\n",val);
			ret = val;
		}
	}
	return ret ;
}



void sw_recv_data_frame(struct hw_sw_core_data* core_data,struct sk_buff *skb)
{
	unsigned char hdr = 0;

	SW_PRINT_FUNCTION_NAME;
	if(!core_data  || !skb)
	{
		return ;
	}

	hdr = skb->data[0];

	if (hdr == PROTO_CMD_KEY_NORAML) //key
	{
		sw_hid_input_report(core_data->device,skb->data+5,skb->len-5);
	}

	if( hdr == PROTO_CMD_MOUSE ) // mouse
	{
		sw_hid_input_report(core_data->mouse_device,skb->data+5,skb->len-5);		
	}

	if( (hdr == PROTO_CMD_KEY_CONSUMER) || (hdr == PROTO_CMD_KEY_CONSUMER_1) ) // ConsumerKey
	{
		sw_hid_input_report(core_data->key_device,skb->data+5,skb->len-5);
	}

}

int sw_relese_hid_devices(struct hw_sw_core_data* core_data)
{
	int ret = -1;
	struct sw_device* device = NULL;
	struct kobject *kobj = NULL;
	int err = 0;

	if(!core_data)
		return ret ;

	if(core_data->device)
	{
		device = core_data->device;
		core_data->device = NULL;
		kobj = &device->dev.kobj;
		if(!kobj)
		{
			SW_PRINT_ERR(" kobj NULL\n");
			err = 1;
		}

		if(!kobj->sd)
		{
			SW_PRINT_ERR(" kobj->sd NULL\n");
				err = 1;
		}

		if(err)
		{
			SW_PRINT_ERR(" sw_relese_hid_devices ERROR \n");
			return -1;
		}
		SW_PRINT_ERR(" sw_relese_hid_devices device_del Begin \n");
		device_unregister(&device->dev);
	}

	if(core_data->mouse_device)
	{
		device = core_data->mouse_device;
		core_data->mouse_device = NULL;
		kobj = &device->dev.kobj;
		if(!kobj)
		{
			SW_PRINT_ERR("mouse_device kobj NULL\n");
			err = 1;
		}

		if(!kobj->sd)
		{
			SW_PRINT_ERR("mouse_device kobj->sd NULL\n");
			err = 1;
		}

		if(err)
		{
			SW_PRINT_ERR("mouse_device sw_relese_hid_devices ERROR \n");
			return -1;
		}
		SW_PRINT_ERR("mouse_device sw_relese_hid_devices device_del Begin \n");
		device_unregister(&device->dev);
	}
	if(core_data->key_device)
	{
		device = core_data->key_device;
		core_data->key_device = NULL;
		kobj = &device->dev.kobj;
		if(!kobj)
		{
			SW_PRINT_ERR("key_device kobj NULL\n");
			err = 1;
		}

		if(!kobj->sd)
		{
			SW_PRINT_ERR("key_device kobj->sd NULL\n");
			err = 1;
		}

		if(err)
		{
			SW_PRINT_ERR("key_device sw_relese_hid_devices ERROR \n");
			return -1;
		}
		SW_PRINT_ERR("key_device sw_relese_hid_devices device_del Begin \n");
		device_unregister(&device->dev);
	}
	ret = 0;

	return ret;

}