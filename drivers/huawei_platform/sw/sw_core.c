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
#include <linux/crc16.h>
#include <linux/crc-ccitt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/hisi/hisi_adc.h>
#include "sw_core.h"
#include "sw_debug.h"
#include <linux/device.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/delay.h>
//#include <linux/sw_interface.h>
#include <linux/fb.h>

#ifdef CONFIG_HUAWEI_PLATFORM
#include <huawei_platform/log/log_exception.h>
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG sw_core
HWLOG_REGIST();
#else
#define hwlog_debug(fmt, args...)do { printk(KERN_DEBUG   "[hw_kb]" fmt, ## args); } while (0)
#define hwlog_info(fmt, args...) do { printk(KERN_INFO    "[hw_kb]" fmt, ## args); } while (0)
#define hwlog_warn(fmt, args...) do { printk(KERN_WARNING"[hw_kb]" fmt, ## args); } while (0)
#define hwlog_err(fmt, args...)  do { printk(KERN_ERR   "[hw_kb]" fmt, ## args); } while (0)
#endif

static struct class *hwkb_class = NULL;
static struct device *hwkb_device = NULL;
#define KB_DEVICE_NAME "hwsw_kb"

#define VERSION "1.0"

extern int  Process_KBChannelData(char *data, int count);
extern int kernel_send_kb_cmd(unsigned int cmd,int val);

extern int kbhb_get_hall_value(void);
#define HALL_COVERD     (1)

#define KBHBIO                         0xB1
#define KBHB_IOCTL_START			_IOW(KBHBIO, 0x01, short)
#define KBHB_IOCTL_STOP				_IOW(KBHBIO, 0x02, short)
#define KBHB_IOCTL_CMD				_IOW(KBHBIO, 0x06, short)

#define SCREEN_ON   0
#define SCREEN_OFF	1

int g_SW_LOGlevel = SW_LOG_DEBUG;
int g_SW_State = -1;

struct platform_device *hw_sw_device = NULL;

/* workqueue to process sw events */
static struct workqueue_struct *sw_wq;
static void sw_core_event(struct work_struct *work);

static void sw_keyboard_disconnected(struct hw_sw_disc_data *pdisc_data);

static struct hw_sw_disc_data * get_disc_data(void)
{
	struct platform_device *pdev = NULL;
	struct hw_sw_disc_data  *pdisc_data = NULL;

	pdev = hw_sw_device;
	if (!pdev)
	{
		SW_PRINT_ERR("%s pdev is NULL\n", __func__);
		return NULL;
	}

	pdisc_data  = dev_get_drvdata(&pdev->dev);
	if (NULL == pdisc_data)
	{
		SW_PRINT_ERR("pdisc_data is NULL\n");
		return NULL;
    }

	return pdisc_data;

}



static ssize_t stateinfo_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	int len=0;
	int ret=0;
	int buf_len = PAGE_SIZE-1;
	char* cur = buf;

	struct hw_sw_disc_data  *pdisc_data = NULL;
	SW_PRINT_FUNCTION_NAME;

	pdisc_data  = get_disc_data();
	if(pdisc_data)
	{
		if(pdisc_data->core_data)
		{
		   ret = snprintf(cur, buf_len, "keyboard is %s \n", (pdisc_data->core_data->kb_state == KBSTATE_ONLINE) ? "online" : "offline" );
		   buf_len -= ret;
		   cur += ret;
		   len += ret;

		   if(pdisc_data->core_data->kb_state == KBSTATE_ONLINE)
		   {
		        ret = snprintf(cur, buf_len, "keyboard vendor id is 0x%04X \n", pdisc_data->core_data->vendor);
		        buf_len -= ret;
		        cur += ret;
		        len += ret;

		        ret = snprintf(cur, buf_len, "keyboard version is %d.%d \n", pdisc_data->core_data->mainver,pdisc_data->core_data->subver);
		        buf_len -= ret;
		        cur += ret;
		        len += ret;
		   }
		}
		ret = snprintf(cur, buf_len, "keyboard irq-detect is %s \n",pdisc_data->irq_enabled ? "enabled" : "disabled" );
		buf_len -= ret;
		cur += ret;
		len += ret;

	}
	else
	{
		len = snprintf(buf, PAGE_SIZE, "no support \n");
	}
	return (len >= PAGE_SIZE) ? (PAGE_SIZE - 1) : len;
}


static DEVICE_ATTR_RO(stateinfo);

static struct attribute *sw_dev_attrs[] = {

		&dev_attr_stateinfo.attr,
		NULL,
};
static const struct attribute_group sw_platform_group = {
        .attrs = sw_dev_attrs,
};

static int sw_uevent(struct device *dev, struct kobj_uevent_env *env)
{
//	struct sw_device *sdev = container_of(dev, struct sw_device, dev);


	return 0;
}

static bool sw_match_one_id(struct sw_device *hdev,
							 const struct sw_device_id *id)
{
	SW_PRINT_FUNCTION_NAME;
	return (id->type == SW_TYPE_ANY || id->type == hdev->type) &&
		(id->bus == SW_BUS_ANY || id->bus == hdev->bus) &&
		(id->group == SW_GROUP_ANY || id->group == hdev->group) &&
		(id->vendor == SW_ANY_ID || id->vendor == hdev->vendor) &&
		(id->product == SW_ANY_ID || id->product == hdev->product);
}

static int sw_match_device(const struct sw_device_id *ids, struct sw_device *sdev)
{

	SW_PRINT_FUNCTION_NAME;

	for (; ids->bus; ids++)
		if (sw_match_one_id(sdev, ids))
			return 1;
	return 0;

}

static int sw_bus_match(struct device *dev, struct device_driver *drv)
{
	struct sw_driver *sdrv = container_of(drv, struct sw_driver, driver);
	struct sw_device *sdev = container_of(dev, struct sw_device, dev);
	if(!sdrv)
	{
		return -1;
	}
	return sw_match_device(sdrv->id_table, sdev) ;
}

static int sw_connect_driver(struct sw_device *dev, struct sw_driver *drv)
{
	int retval;

	mutex_lock(&dev->drv_mutex);
	dev->drv = drv;
	retval = drv->probe(dev);
	if(retval)
	{
		dev->drv = NULL;
	}
	mutex_unlock(&dev->drv_mutex);

	return retval;
}
static void sw_disconnect_driver(struct sw_device *dev)
{
	mutex_lock(&dev->drv_mutex);
	if (dev->drv)
		dev->drv->disconnect(dev);
	mutex_unlock(&dev->drv_mutex);
}

static int sw_device_probe(struct device *dev)
{
	struct sw_driver *sdrv = container_of(dev->driver,
		struct sw_driver, driver);
	struct sw_device *sdev = container_of(dev, struct sw_device, dev);
	SW_PRINT_FUNCTION_NAME;

	return sw_connect_driver(sdev, sdrv);
}


static int sw_device_remove(struct device *dev)
{
	struct sw_device *sdev = container_of(dev, struct sw_device, dev);
	if(!sdev)
	{
		return FAILURE;
	}
	sw_disconnect_driver(sdev);
	return 0;
}

static struct bus_type sw_bus_type = {
		.name		= "sw",
		.match		= sw_bus_match,
		.probe		= sw_device_probe,
		.remove		= sw_device_remove,
		.uevent		= sw_uevent,
};

/*
 * Free a device structure, all reports, and all fields.
 */


static void sw_device_release(struct device *dev)
{
	struct sw_device *sdev = container_of(dev, struct sw_device, dev);

	SW_PRINT_FUNCTION_NAME;
	if(sdev)
	{
		kfree(sdev);
	}
}

struct sw_device *sw_allocate_device(void)
{
	struct sw_device *sdev;

	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (sdev == NULL)
	{
		return NULL;
	}
	sdev->dev.release = sw_device_release;
	sdev->dev.bus = &sw_bus_type;

	mutex_init(&sdev->drv_mutex);

	return sdev;
}


/////////////////////////////////////////////////////////////

int sw_get_core_reference(struct hw_sw_core_data **core_data)
{
    struct platform_device *pdev = NULL;
    struct hw_sw_disc_data  *pdisc_data = NULL;

    pdev = hw_sw_device;
    if (!pdev)
    {
        *core_data = NULL;
        SW_PRINT_ERR("%s pdev is NULL\n", __func__);
        return FAILURE;
    }

    pdisc_data  = dev_get_drvdata(&pdev->dev);
    if (NULL == pdisc_data)
    {
        SW_PRINT_ERR("pdisc_data is NULL\n");
        return FAILURE;
    }

    *core_data = pdisc_data->core_data;

    return SUCCESS;
}

#define DEFAULT_WAKE_TIMEOUT    (5 * HZ)
static void swkb_wake_lock_timeout(struct hw_sw_disc_data * pdisc_data,long timeout)
{
	if (!wake_lock_active(&pdisc_data->kb_wakelock)) {
		wake_lock_timeout(&pdisc_data->kb_wakelock, timeout);
		pr_info("swkb wake lock\n");
	}
}

static void swkb_wake_lock(struct hw_sw_disc_data * pdisc_data)
{
	if (!wake_lock_active(&pdisc_data->kb_wakelock)) {
		wake_lock(&pdisc_data->kb_wakelock);
		pr_info("swkb wake lock\n");
	}

	//wake_lock_timeout(&pdisc_data->kb_wakelock, 5 * HZ);
}

static void swkb_wake_unlock(struct hw_sw_disc_data * pdisc_data)
{
	if (wake_lock_active(&pdisc_data->kb_wakelock)) {
		wake_unlock(&pdisc_data->kb_wakelock);
		pr_info("swkb wake unlock\n");
	}
}

static void sw_notify_android_uevent(int isConnOrDisconn)
{
     char *disconnected[2] = { "KB_STATE=DISCONNECTED", NULL };
     char *connected[2]    = { "KB_STATE=CONNECTED", NULL };

	 if(IS_ERR(hwkb_device))
	 {
	    pr_info("sw_notify_android_uevent  device uninit \n");
	    return;
	 }
	 switch(isConnOrDisconn)
	 {
	    case KBSTATE_ONLINE:
	       {
	          kobject_uevent_env(&hwkb_device->kobj,
                                        KOBJ_CHANGE, connected);
	          pr_debug("sw_notify_android_uevent  kobject_uevent_env connected \n");
	       }
	       break;
	    case KBSTATE_OFFLINE:
	       {
	          kobject_uevent_env(&hwkb_device->kobj,
                                        KOBJ_CHANGE, disconnected);
	          pr_debug("sw_notify_android_uevent  kobject_uevent_env disconnected \n");
	       }
	       break;
	 }
}

static void sw_core_event(struct work_struct *work);

int sw_core_init(struct hw_sw_core_data **core_data)
{
	struct hw_sw_core_data *psw_core_data = NULL;

    SW_PRINT_FUNCTION_NAME;

    psw_core_data = kzalloc(sizeof(struct hw_sw_core_data), GFP_KERNEL);
    if (!psw_core_data)
    {
        SW_PRINT_ERR("memory allocation failed\n");
        return -ENOMEM;
    }


    skb_queue_head_init(&psw_core_data->rx_data_seq);
    skb_queue_head_init(&psw_core_data->tx_data_seq);

    psw_core_data->device = NULL;

	INIT_WORK(&psw_core_data->events, sw_core_event);


    *core_data = psw_core_data;
    return 0;

}

int sw_skb_enqueue(struct hw_sw_core_data *psw_core_data, struct sk_buff *skb, u8 type)
{
    if (unlikely(NULL == psw_core_data))
    {
        SW_PRINT_ERR(" psw_core_data is NULL\n");
        return -EINVAL;
    }

    if (unlikely(NULL == skb))
    {
        SW_PRINT_ERR(" skb is NULL\n");
        return -EINVAL;
    }

    switch (type)
    {

	case TX_DATA_QUEUE:
		skb_queue_tail(&psw_core_data->tx_data_seq, skb);
		break;
	case RX_DATA_QUEUE:
	    skb_queue_tail(&psw_core_data->rx_data_seq, skb);
		break;
	default:
		SW_PRINT_ERR("queue type is error, type=%d\n", type);
        break;
	}

	return 0;
}


int sw_push_skb_queue(struct hw_sw_core_data *psw_core_data, u8 *buf_ptr, int pkt_len, u8 type)
{
    struct sk_buff *skb = NULL;

    if (NULL == psw_core_data)
    {
        SW_PRINT_ERR("psw_core_data is NULL\n");
        return -EINVAL;
    }

    if (NULL == buf_ptr)
    {
        SW_PRINT_ERR("buf_ptr is NULL\n");
        return -EINVAL;
    }

    skb = alloc_skb(pkt_len, GFP_ATOMIC);
    if (NULL == skb)
    {
        SW_PRINT_ERR("can't allocate mem for new skb, len=%d\n", pkt_len);
        return -EINVAL;
    }

    skb_put(skb, pkt_len);
    memcpy(skb->data, buf_ptr, pkt_len);
    sw_skb_enqueue(psw_core_data, skb, type);

    return 0;
}


struct sk_buff *sw_skb_dequeue(struct hw_sw_core_data *psw_core_data, u8 type)
{
    struct sk_buff *curr_skb = NULL;

    if (NULL == psw_core_data)
    {
        SW_PRINT_ERR("psw_core_data is NULL\n");
        return curr_skb;
    }

    switch (type)
    {

	case TX_DATA_QUEUE:
		  curr_skb = skb_dequeue(&psw_core_data->tx_data_seq);
		break;
	case RX_DATA_QUEUE:
		  curr_skb = skb_dequeue(&psw_core_data->rx_data_seq);
		break;
	default:
        SW_PRINT_ERR("queue type is error, type=%d\n", type);
        break;
    }

    return curr_skb;
}


void sw_kfree_skb(struct hw_sw_core_data *psw_core_data, u8 type)
{
    struct sk_buff *skb = NULL;

    SW_PRINT_FUNCTION_NAME;
    if (NULL == psw_core_data)
    {
        SW_PRINT_ERR("psw_core_data is NULL");
        return;
    }

    while ((skb = sw_skb_dequeue(psw_core_data, type)))
    {
        kfree_skb(skb);
    }

    switch (type)
    {
    case TX_DATA_QUEUE:
        skb_queue_purge(&psw_core_data->tx_data_seq);
        break;
    case RX_DATA_QUEUE:
        skb_queue_purge(&psw_core_data->rx_data_seq);
        break;
	default:
		SW_PRINT_ERR("queue type is error, type=%d\n", type);
        break;
	}
}

static void sw_kb_int_disable_irq(struct hw_sw_disc_data * pdisc_data)
{
	unsigned long flags;

	spin_lock_irqsave(&pdisc_data->irq_enabled_lock, flags);
	if (pdisc_data->irq_enabled) {
		disable_irq_nosync(pdisc_data->kb_int_irq);
		pdisc_data->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pdisc_data->irq_enabled_lock, flags);
}

static void sw_kb_int_enable_irq(struct hw_sw_disc_data * pdisc_data)
{
	unsigned long flags;

	spin_lock_irqsave(&pdisc_data->irq_enabled_lock, flags);
	if (!pdisc_data->irq_enabled) {
		enable_irq(pdisc_data->kb_int_irq);
		pdisc_data->irq_enabled = true;
	}
	spin_unlock_irqrestore(&pdisc_data->irq_enabled_lock, flags);
}


static void sw_keyboard_disconnected(struct hw_sw_disc_data *pdisc_data)
{
    SW_PRINT_FUNCTION_NAME;
	if(pdisc_data->kb_online==1)
	{
		SW_PRINT_INFO("sw_keyboard_disconnected, Enable irq \n");
		pdisc_data->kb_online = 0;
		gpio_set_value(pdisc_data->kb_vdd_ctrl,0);
	}
	sw_kb_int_enable_irq(pdisc_data);
	pdisc_data->start_detect = 1;

//	swkb_wake_unlock(pdisc_data);
}

static void sw_keyboard_connected(struct hw_sw_disc_data *pdisc_data)
{
	SW_PRINT_FUNCTION_NAME;
	if(pdisc_data->kb_online==0)
	{
		SW_PRINT_INFO("sw_keyboard_connected, Notify sensorhub \n");
		gpio_set_value(pdisc_data->kb_vdd_ctrl,1);
		pdisc_data->kb_online = 1;

		swkb_wake_lock_timeout(pdisc_data,DEFAULT_WAKE_TIMEOUT);
		//notify sensorhub start work
		kernel_send_kb_cmd(KBHB_IOCTL_START,0);
		//queue_delayed_work(sw_wq,&pdisc_data->kb_channel_work,msecs_to_jiffies(0));
	}
	pdisc_data->start_detect = 0;
//	swkb_wake_lock(pdisc_data);

}

int sw_core_exit(struct hw_sw_core_data * pcore_data)
{

    SW_PRINT_FUNCTION_NAME;

    if (pcore_data != NULL)
    {
        /* Free PS tx and rx queue */
        sw_kfree_skb(pcore_data, TX_DATA_QUEUE);
        sw_kfree_skb(pcore_data, RX_DATA_QUEUE);

        kfree(pcore_data);
    }
    return 0;
}


static void kick_sw_wq(void)
{
  struct hw_sw_core_data *sw_core_data = NULL;
  int ret = -1;

  ret = sw_get_core_reference(&sw_core_data);

 if(ret!=SUCCESS)
	 return;

 if (work_pending(&sw_core_data->events))
		return;

 if (queue_work(sw_wq, &sw_core_data->events))
		return;

}
static void sw_send_power_key(struct hw_sw_core_data* core_data)
{
	char powerkey_down[]={0x0,0x7,0x0,0x0,0x0,0x30,0x0};
	char powerkey_up[]=  {0x0,0x7,0x0,0x0,0x0,0x0,0x0};
	 struct sk_buff *skb = NULL;
	if(!core_data)
	{
		return;
	}
	SW_PRINT_DBG(" sw_send_power_key ++++!\n");

	skb = alloc_skb(7, GFP_ATOMIC);
    if (NULL == skb)
    {
        SW_PRINT_ERR("can't allocate mem for new skb\n");
        return ;
    }
    skb_put(skb, 7);
    memcpy(skb->data, powerkey_down, 7);
	sw_recv_data_frame(core_data, skb);
	msleep(5);
	memcpy(skb->data, powerkey_up, 7);
	sw_recv_data_frame(core_data, skb);
	kfree_skb(skb);
	SW_PRINT_DBG(" sw_send_power_key ----!\n");
}


/*
*  handshake format
*  cmd(1) + devno(1) + len(1) + crc(2) + payload(1) + vendor(2)+ mainver(1) +  subver(1)    
*/
static int sw_process_cmd_handshake(struct hw_sw_core_data * core_data,struct sk_buff *skb)
{
	int ret = -1;
	int handshakesize = 0x0A;

	SW_PRINT_DBG(" sw_core_event Connect Keyboard!\n");

	if(!skb || skb->len < handshakesize )
	{
		return ret;
	}

	core_data->vendor = (u32)((skb->data[6] << 8) | skb->data[7]);
	core_data->mainver = (u8)skb->data[8] ;
	core_data->subver  = (u8)skb->data[9] ;
	core_data->kb_state = KBSTATE_ONLINE;
	core_data->product = 0x12d1;

	if(!core_data->device)
	{
		ret = create_hid_devices(core_data,hw_sw_device);
		sw_notify_android_uevent(KBSTATE_ONLINE);
	}else
	{
		ret = 0;
	}
	//complete(&pdisc_data->kb_comm_complete);
	return ret;
}

static int sw_process_cmd_disconnect(struct hw_sw_core_data * core_data,struct sk_buff *skb)
{
	int ret = -1;
	struct hw_sw_disc_data* pdisc_data = NULL;
	pdisc_data = (struct hw_sw_disc_data*)(core_data->pm_data);
	if(!pdisc_data)
	{
		return -1;
	}
	SW_PRINT_DBG(" sw_core_event Disconnect Keyboard!\n");

	//disable kb device
	ret = sw_relese_hid_devices(core_data);
	core_data->vendor = 0;
	core_data->mainver = 0;
	core_data->subver = 0;
	core_data->kb_state = KBSTATE_OFFLINE;
	sw_keyboard_disconnected(pdisc_data);
	if(ret==0)
	{
	   swkb_wake_lock_timeout(pdisc_data,DEFAULT_WAKE_TIMEOUT);
	   kernel_send_kb_cmd(KBHB_IOCTL_STOP,0);
	   sw_notify_android_uevent(KBSTATE_OFFLINE);
	}
	return ret;
}

static int sw_process_cmd_changeworkmode(struct hw_sw_core_data * core_data,struct sk_buff *skb)
{
	int ret = -1;
	SW_PRINT_DBG(" sw_core_event Change Keyboard MODE !\n");
	if(skb->len < 4)
	{
		return -1;
	}
	switch(skb->data[3])
	{
		case KBMODE_PCMODE:
			ret = create_hid_devices(core_data,hw_sw_device);
			break;
		case KBMODE_PADMODE:
			ret = sw_relese_hid_devices(core_data);
			break;
		default:
			break;
	}
	return ret;
}

static int sw_process_cmd_default(struct hw_sw_core_data * core_data,struct sk_buff *skb)
{
	struct hw_sw_disc_data* pdisc_data = NULL;

	pdisc_data = (struct hw_sw_disc_data*)(core_data->pm_data);
	if(!pdisc_data)
	{
		return -1;
	}

	if(pdisc_data->fb_state == SCREEN_OFF)
	{
	   if(pdisc_data->wait_fb_on == 0)
	   {
	      pdisc_data->wait_fb_on = 1;
	      sw_send_power_key(core_data);
	   }
	}

	if(pdisc_data->fb_state == SCREEN_ON)
	{
	   if(pdisc_data->wait_fb_on == 1)
	   {
		   pdisc_data->wait_fb_on = 0;
	   }
	  sw_recv_data_frame(core_data, skb);
	}

	return 0;
}

static void sw_core_event(struct work_struct *work)
{
	struct hw_sw_core_data* core_data = NULL;
	struct sk_buff* skb = NULL;
	u8 hdr = 0;
	int ret = -1;
	core_data = container_of(work, struct hw_sw_core_data, events);

	SW_PRINT_FUNCTION_NAME;

	if(!core_data)
		return;

	while ((skb = sw_skb_dequeue(core_data,RX_DATA_QUEUE)))
	{
		skb_orphan(skb);
		if (!skb_linearize(skb))
		{
			hdr = (u8)skb->data[0];
			switch(hdr)
			{
				case PROTO_CMD_HANDSHAKE:
					ret = sw_process_cmd_handshake(core_data,skb);
				    break;
				case PROTO_CMD_DISCONNECT:
					ret = sw_process_cmd_disconnect(core_data,skb);
				    break;
				case PROTO_CMD_WORKMODE:
				    ret = sw_process_cmd_changeworkmode(core_data,skb);
				    break;
				default:
					ret = sw_process_cmd_default(core_data,skb);
					break;
			}

			kfree_skb(skb);
		}
		else
		{
			kfree_skb(skb);
		}
	}
}


int sw_core_recv(void *core_data,  u8 *data, int count)
{
    struct hw_sw_core_data *sw_core_data = NULL;
	int ret = 0;

	SW_PRINT_FUNCTION_NAME;
	sw_core_data = (struct hw_sw_core_data *)core_data;
    if (NULL == sw_core_data)
    {
        SW_PRINT_ERR(" core_data is null \n");
        return 0;
    }

    if (unlikely(NULL == data))
    {
        SW_PRINT_ERR(" received null from TTY \n");
        return 0;
    }

    if (count < 1)
    {
        SW_PRINT_ERR(" received count from TTY err\n");
        return 0;
    }

	ret = sw_push_skb_queue(sw_core_data,(u8*)data,count,RX_DATA_QUEUE);
	if(ret == 0)
	{
		kick_sw_wq();
	}

	return ret ;
}

/*
* if keyboard coverd, we mean some messages from keyborad  may not be processed.
* Such as key msg\touch msg\work-mode msg etc, but except connect or disconnect msg.
*/
static bool is_ignore_KBChannelData(char *data, int count)
{
	u8 hdr = 0;
	int hall_val = kbhb_get_hall_value();
	if (unlikely(NULL == data))
	{
           SW_PRINT_ERR(" received null from KB \n");
           return true;
	}

	if (count < 1)
	{
           SW_PRINT_ERR(" received count from KB err\n");
           return true;
	}

	/*if keyboard coverd , don't respond some msg ,except Disconnect and handshake msg */
	if(hall_val & HALL_COVERD)
	{
           hdr = (u8)data[0];
           if((hdr != PROTO_CMD_DISCONNECT) && (hdr != PROTO_CMD_HANDSHAKE))
           {
                 //SW_PRINT_DBG("[MCU]received KB cmd= [%x,%x] \n",data[0],data[1]);
                 return true;
           }
	}

	return false;
}

/*
* if dts config hwsw_kb  g_SW_state = 0
* in inputhub  kbhub_channel model will init  when  g_SW_state = 0
*/
int sw_get_statue(void)
{

	return g_SW_State ;
}

EXPORT_SYMBOL(sw_get_statue);


void Notify_KB_DoDetect(void)
{
	struct hw_sw_disc_data  *pdisc_data = NULL;

	pdisc_data  = get_disc_data();
	if(!pdisc_data)
	{
		return;
	}

	schedule_work(&pdisc_data->irq_work);

}
EXPORT_SYMBOL(Notify_KB_DoDetect);

int  Process_KBChannelData(char *data, int count)
{
	struct hw_sw_core_data *sw_core_data = NULL;
	int ret = -1;
	sw_get_core_reference(&sw_core_data);

	if(!sw_core_data)
		return -1;

	if(is_ignore_KBChannelData(data,count))
	{
		return -1;
	}
	ret = sw_core_recv(sw_core_data,(u8*)data,count);
	return ret ;
}
EXPORT_SYMBOL(Process_KBChannelData);


/**********************************************************/


static int sw_poweroff_notify_sys(struct notifier_block *this, unsigned long code,void *unused)
{
	return 0;
}

static struct notifier_block  plat_poweroff_notifier = {
	.notifier_call = sw_poweroff_notify_sys,
};


static void sw_fb_notifier_action(int screen_on_off)
{
	struct hw_sw_disc_data  *pdisc_data = NULL;

    pdisc_data  = get_disc_data();
	if(!pdisc_data)
		return ;

	pdisc_data->fb_state = screen_on_off;
	if((pdisc_data->fb_state==SCREEN_ON) && (pdisc_data->wait_fb_on == 1) )
	{
		pdisc_data->wait_fb_on = 0;
	}

}


static int sw_fb_notifier(struct notifier_block *nb,
                                 unsigned long action, void *data)
{
        switch (action) {
        case FB_EVENT_BLANK:    /*change finished*/
                {
                        struct fb_event *event = data;
                        int *blank = event->data;
                        switch (*blank) {
                        case FB_BLANK_UNBLANK:  /*screen on */
                            sw_fb_notifier_action(SCREEN_ON);
                                break;

                        case FB_BLANK_POWERDOWN:        /* screen off */
                             sw_fb_notifier_action(SCREEN_OFF);
                                break;

                        default:
                                SW_PRINT_ERR("unknown---> lcd unknown in %s\n",__func__);
                                break;
                        }
                        break;
                }
        default:
                break;
        }

        return NOTIFY_OK;
}

static struct notifier_block fb_notify = {
        .notifier_call = sw_fb_notifier,
};

static irqreturn_t sw_kb_int_irq(int irq, void *dev_data)
{
	struct hw_sw_disc_data * pdisc_data  = dev_data;
	int gp = -1;
	if(!pdisc_data )
	{
		SW_PRINT_INFO("sw_kb_int_irq pdisc_data=NULL\n");
		return IRQ_HANDLED;
	}
	if(0==pdisc_data->start_detect)
	{
		SW_PRINT_INFO("sw_kb_int_irq pdisc_data->start_detect=0\n");
		return IRQ_HANDLED;
	}
	gp = gpio_get_value(pdisc_data->kb_int_gpio);
	SW_PRINT_INFO("sw_wakeup_irq GPIO  %x\n", gp);

	sw_kb_int_disable_irq(pdisc_data);
	schedule_work(&pdisc_data->irq_work);

	return IRQ_HANDLED;
}


#define KB_ONLINE_CONN_MIN_ADC_LIMIT    200
#define KB_ONLINE_CONN_MAX_ADC_LIMIT    350

#define KB_ONLINE_MIN_ADC_LIMIT    1450
#define KB_ONLINE_MAX_ADC_LIMIT    1550

#define KB_DETECT_DELAY_TIME_MS	  (300)

static bool sw_is_kb_online(struct hw_sw_disc_data *pdisc_data)
{
	int adc_val = 0;
	int val = -1;
	int count = 5;
	int i =0;
	int check_ok = 0;
	int ret_check=3;
	if(!pdisc_data)
	{
		SW_PRINT_INFO("sw_is_kb_online Param  is NULL EXCEPTION!!!!\n");
		return false;
	}
	msleep(KB_DETECT_DELAY_TIME_MS);

retry_check:

	for(i=0;i<count;i++)
	{
		val = gpio_get_value(pdisc_data->kb_vdd_ctrl);
		adc_val = hkadc_detect_kb(pdisc_data->kb_detect_adc);

		//VDD is disabled , ADC in [KB_ONLINE_MIN_ADC_LIMIT KB_ONLINE_CONN_MAX_ADC_LIMIT] mean kb online
		//VDD is enable , ADC maybe in [KB_ONLINE_MIN_ADC_LIMIT KB_ONLINE_MAX_ADC_LIMIT]
		if(0==val)
		{
		  if( (adc_val>KB_ONLINE_CONN_MIN_ADC_LIMIT) && (adc_val<KB_ONLINE_CONN_MAX_ADC_LIMIT) )
		  {
		   check_ok++;
		  }
		}else
		{
		  if( (adc_val>KB_ONLINE_MIN_ADC_LIMIT) && (adc_val<KB_ONLINE_MAX_ADC_LIMIT) )
		  {
			check_ok++;
		  }
		}
	}

	//if ADC check all sucess  ,  mean connected , return true
	if(check_ok==count)
	 {
		return true;
	 }

	 //if ADC check had failed ,need retry check ;
	 //if retry_check < 0 , mean disconnect ,but this checked will have some mistake
	 if(ret_check > 0 )
	 {
		ret_check--;
		check_ok = 0;
		msleep(1);
		goto retry_check;
	 }

	return false;
}


/**********************************************************
*  Function:       sw_detect_irq_work
*  Discription:    handler for kb detect irq
*  Parameters:     work:kb detect fault interrupt workqueue
*  return value:   NULL
**********************************************************/

/*lint -save -e* */
static void sw_kb_detect_irq_work(struct work_struct *work)
{
	struct hw_sw_disc_data *pdisc_data = container_of(work, struct hw_sw_disc_data, irq_work);

	if(sw_is_kb_online(pdisc_data))
	{
		SW_PRINT_INFO("hkadc_detect_kb, KB Connected. Enable VDD \n");
		sw_keyboard_connected(pdisc_data);
	}else
	{
		sw_keyboard_disconnected(pdisc_data);
	}

}

static void sw_setup_kbint_config(struct hw_sw_disc_data *pdev)
{
	int irq = -1;
	int ret = -1;

	if(!pdev)
	{
		return;
	}

	if (!gpio_is_valid(pdev->kb_int_gpio))
	{
		SW_PRINT_ERR("kb_int_gpio no valid\n");
		pdev->kb_int_gpio = -1;
		return;
	}

	irq = gpio_to_irq(pdev->kb_int_gpio);
	if (irq < 0)
	{
		SW_PRINT_ERR("kb_int_gpio gpio_to_irq fail %x\n",irq);
		goto out_free;
	}

	ret = request_irq(irq, sw_kb_int_irq, /*IRQF_TRIGGER_RISING |*/
		IRQF_TRIGGER_FALLING, "sw_kb_int", pdev);
	if (ret)
	{
		SW_PRINT_ERR("kb_int_gpio request_irq fail\n");
		goto out_free;
	}
	pdev->kb_int_irq = irq;

	//default disable irq , because sensorhub maybe not ready when sys start 
	sw_kb_int_disable_irq(pdev);

	return;

out_free:
	gpio_free(pdev->kb_int_gpio);
	pdev->kb_int_gpio = -1;

}

static int sw_get_named_gpio(struct device_node *np, const char *propname,enum gpiod_flags flags)
{
	int gpio;
	int ret = -1;

	if(!np || !propname)
		return -1;

	gpio = of_get_named_gpio(np, propname, 0);
	if (gpio == -EPROBE_DEFER)
		gpio = of_get_named_gpio(np,propname, 0);

	if (!gpio_is_valid(gpio))
	{
		SW_PRINT_ERR("sw_get_named_gpio get gpio [%s] fail\n",propname);
		return -1;
	}

	ret = gpio_request(gpio, propname);
	if (ret < 0)
	{
		SW_PRINT_ERR("sw_get_named_gpio request gpio [%s] fail\n",propname);
		gpio_free(gpio);
		return -1;
	}

	if(flags == GPIOD_OUT_LOW)
	{
		ret = gpio_direction_output(gpio, 0);
	}else if(flags == GPIOD_OUT_HIGH)
	{
		ret = gpio_direction_output(gpio, 1);
	}else if(flags == GPIOD_IN)
	{
		ret = gpio_direction_input(gpio);
	}

	if (ret < 0)
	{
		SW_PRINT_ERR("sw_get_named_gpio set gpio [%s] flags fail\n",propname);
		gpio_free(gpio);
		return -1;
	}

	return gpio;
}

#define DEFAULT_KB_ADC_CHANNEL  7
static int sw_parse_devtree(struct hw_sw_disc_data * pdisc_data)
{
	struct device_node *np=NULL;

	u32 val = 0 ;
	int ret = -1;

	pdisc_data->kb_int_gpio = -1;
	pdisc_data->kb_tx_gpio = -1;
	pdisc_data->kb_vdd_ctrl = -1;
	pdisc_data->kb_int_irq = -1;

	np = of_find_compatible_node(NULL, NULL,"huawei,sw_kb");	// should be the same as dts node compatible property
	if (NULL == np)
	{
		SW_PRINT_ERR("Unable to find %s\n","huawei,sw_kb");
		return -ENOENT;
	}

	//read adc channel
	if (of_property_read_u32(np, "adc_kb_detect",&val))
	{
		SW_PRINT_ERR("dts:can not get kb detect adc channel,use default channel: %d!\n",val);
		val = DEFAULT_KB_ADC_CHANNEL;
    }
	pdisc_data->kb_detect_adc = val;

	//read keyborad int gpio
	pdisc_data->kb_int_gpio = sw_get_named_gpio(np,"gpio_kb_int",GPIOD_IN);
	sw_setup_kbint_config(pdisc_data);
	if(pdisc_data->kb_int_gpio<0)
	{
		ret = -EINVAL;
		goto err_free_gpio;
	}

	//read keyborad TX gpio,default HIGH
	pdisc_data->kb_tx_gpio = sw_get_named_gpio(np,"gpio_kb_tx",GPIOD_OUT_HIGH);
	if(pdisc_data->kb_tx_gpio < 0 )
	{
		ret = -EINVAL;
		goto err_free_irq;
	}

	//read keyboard VDD control ,default LOW
	pdisc_data->kb_vdd_ctrl = sw_get_named_gpio(np,"gpio_kb_vdd_ctrl",GPIOD_OUT_LOW);
	if(pdisc_data->kb_vdd_ctrl < 0 )
	{
		ret = -EINVAL;
		goto err_free_irq;
	}

	return 0;

err_free_irq:
	free_irq(pdisc_data->kb_int_irq,pdisc_data);
	pdisc_data->kb_int_irq = -1;

err_free_gpio:
	if(pdisc_data->kb_int_gpio >= 0 )
	{
		gpio_free(pdisc_data->kb_int_gpio);
		pdisc_data->kb_int_gpio = -1;
	}

	if(pdisc_data->kb_tx_gpio >= 0)
	{
		gpio_free(pdisc_data->kb_tx_gpio);
		pdisc_data->kb_tx_gpio = -1;
	}

	if(pdisc_data->kb_vdd_ctrl >= 0)
	{
		gpio_free(pdisc_data->kb_vdd_ctrl);
		pdisc_data->kb_vdd_ctrl = -1;
	}

	return ret ;
}



static void swkb_channel_com_work(struct work_struct *work)
{
	struct hw_sw_disc_data *pdisc_data = container_of(work, struct hw_sw_disc_data, kb_channel_work);
	if(!pdisc_data)
		return;

	pr_info("%s: start\n", __func__);
	swkb_wake_lock(pdisc_data);
    wait_for_completion_timeout(&pdisc_data->kb_comm_complete, msecs_to_jiffies(1000));

	//schedule_work(&pdisc_data->irq_work);
	reinit_completion(&pdisc_data->kb_comm_complete);

	swkb_wake_unlock(pdisc_data);
	pr_info("%s: end\n", __func__);
}


static void sw_destroy_monitor_device(struct platform_device *pdev)
{
	if(!pdev)
	{
	   return ;
	}
	if(!IS_ERR(hwkb_device))
	{
	  device_destroy(hwkb_device->class, hwkb_device->devt);
	}
	if (!IS_ERR(hwkb_class))
	{
	  class_destroy(hwkb_class);
	}
	hwkb_device = NULL;
	hwkb_class = NULL;
}

static void sw_init_monitor_device(struct platform_device *pdev)
{
     int ret = -1;
     if(hwkb_device  || hwkb_class)
     {
		sw_destroy_monitor_device(pdev);
     }
     hwkb_class = class_create(THIS_MODULE, KB_DEVICE_NAME);
     if (IS_ERR(hwkb_class))
     {
	   ret =  PTR_ERR(hwkb_class);
	   goto err_init;
     }

     hwkb_device = device_create(hwkb_class, NULL,0, NULL, "hwkb");
     if (IS_ERR(hwkb_device))
     {
	    ret =  PTR_ERR(hwkb_device);
		goto err_init;
     }
     return ;
err_init:
     SW_PRINT_ERR("sw_init_monitor_device failed %x \n",ret);
     sw_destroy_monitor_device(pdev);

    return;
}


static int sw_probe(struct platform_device *pdev)
{
	struct hw_sw_disc_data * pdisc_data = NULL;
	int ret = -1;

	SW_PRINT_FUNCTION_NAME;

	hw_sw_device = pdev;

	pdisc_data = kzalloc(sizeof(struct hw_sw_disc_data), GFP_KERNEL);
	if(NULL == pdisc_data)
	{
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, pdisc_data);

	//init kb int irq control
	spin_lock_init(&pdisc_data->irq_enabled_lock);
	pdisc_data->irq_enabled = true;
//	mutex_init(&pdisc_data->irq_mutex_lock);

	ret = sw_parse_devtree(pdisc_data);
	if(ret < 0)
	{
		ret = -EINVAL;
		goto err_core_init;
	}

	ret = sw_core_init(&pdisc_data->core_data);
	if (0 != ret)
    {
        SW_PRINT_ERR(" SW core init failed\n");
        goto err_core_init;
    }
	/* refer to itself */
	pdisc_data->core_data->pm_data = pdisc_data;
	/* get reference of pdev */
    pdisc_data->pm_pdev = pdev;

    INIT_WORK(&pdisc_data->irq_work, sw_kb_detect_irq_work);

	wake_lock_init(&pdisc_data->kb_wakelock, WAKE_LOCK_SUSPEND, "swkb_wakelock");
	INIT_DELAYED_WORK(&pdisc_data->kb_channel_work, swkb_channel_com_work);

	init_completion(&pdisc_data->kb_comm_complete);

	pdisc_data->kb_online = 0;


	ret = sysfs_create_group(&pdev->dev.kobj, &sw_platform_group);
	if (ret) {
		SW_PRINT_ERR("sw_probe sysfs_create_group error ret =%d. \n", ret);
	}
	sw_init_monitor_device(pdev);

	pdisc_data->fb_state = SCREEN_ON;
	pdisc_data->wait_fb_on = 0;

    fb_register_client(&fb_notify);

	ret = register_reboot_notifier(&plat_poweroff_notifier);
    if (ret)
    {
        SW_PRINT_ERR("Failed to registe plat_poweroff_notifier (err=%d)\n",ret);
		goto err_exception;
	}

	g_SW_State = 0;
	return 0;

err_exception:
	sw_core_exit(pdisc_data->core_data);
err_core_init:
	wake_lock_destroy(&pdisc_data->kb_wakelock);
    kfree(pdisc_data);

	return ret ;
}

static int sw_remove(struct platform_device *pdev)
{
	struct hw_sw_disc_data *pdisc_data = NULL;

    SW_PRINT_FUNCTION_NAME;

    pdisc_data = dev_get_drvdata(&pdev->dev);
    if (NULL == pdisc_data)
    {
        SW_PRINT_ERR("pdisc_data is null\n");
    }

    if (NULL != pdisc_data)
    {
        pdisc_data->pm_pdev = NULL;
        sw_core_exit(pdisc_data->core_data);
    }

    if (NULL != pdisc_data)
    {
        kfree(pdisc_data);
        pdisc_data = NULL;
    }

    sw_destroy_monitor_device(pdev);
	return 0;
}

int sw_suspend(struct platform_device *pdev, pm_message_t state)
{
	SW_PRINT_DBG("sw_suspend enter------------.\n");

	return 0;
}

int sw_resume(struct platform_device *pdev)
{
	SW_PRINT_DBG("sw_resume enter------------.\n");

	return 0;
}

int sw_register_driver(struct sw_driver *drv, struct module *owner,
						const char *mod_name)
{
	int error;
	SW_PRINT_FUNCTION_NAME;

	drv->driver.bus = &sw_bus_type;
	drv->driver.owner = owner;
	drv->driver.mod_name = mod_name;
	error = driver_register(&drv->driver);
	if (error) {
		pr_err("driver_register() failed for %s, error: %d\n",
			drv->driver.name, error);
		return error;
	}
/*
	error = driver_create_file(&drv->driver, &driver_attr_new_id);
	if (error)
		driver_unregister(&drv->driver);
*/
	return error;
}
EXPORT_SYMBOL_GPL(sw_register_driver);

void sw_deregister(struct sw_driver *driver)
{
	pr_info("sw deregistering interface driver %s\n", driver->name);

}
EXPORT_SYMBOL_GPL(sw_deregister);


#define DTS_HW_SW_NAME		"huawei,sw_kb"
static struct of_device_id sw_match_table[] = {
	{
		.compatible = DTS_HW_SW_NAME,
		.data = NULL,
    },
	{ },
};

/*  platform_driver struct for PS module */
static struct platform_driver sw_platform_driver = {
	.probe      = sw_probe,
    .remove     = sw_remove,
    .suspend    = sw_suspend,
    .resume     = sw_resume,
    .driver     = {
	.name   = "hwsw_kb",
    .owner  = THIS_MODULE,
	.of_match_table	= sw_match_table,
	},
};



static int __init sw_init(void)
{
	int ret;

	SW_PRINT_FUNCTION_NAME;
	pr_info("Huawei (3 popo-pin by UART to HID)  \n");

	ret = bus_register(&sw_bus_type);
	if (ret) {
		pr_err("can't register sw bus\n");
		goto err;
	}

    ret = platform_driver_register(&sw_platform_driver);
    if (ret)
    {
        SW_PRINT_ERR("Unable to register platform sw driver.\n");
		goto err_bus;
	}

	sw_wq = alloc_workqueue("sw_core_wq", WQ_FREEZABLE, 0);
	if (sw_wq)
		return 0;

	return 0;

err_bus:
	bus_unregister(&sw_bus_type);
err:
	return ret;
}

static void __exit sw_exit(void)
{
  platform_driver_unregister(&sw_platform_driver);
}

module_init(sw_init);
module_exit(sw_exit);

MODULE_AUTHOR("c00217097 <chenquanquan@huawei.com>");
MODULE_DESCRIPTION("Huawei  3 Popo-pin core driver by UART ");
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("hw-sw-kb");
