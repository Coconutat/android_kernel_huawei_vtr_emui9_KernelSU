#include <linux/kernel.h>
#include <linux/stat.h>           /* File permission masks */
#include <linux/types.h>          /* Kernel datatypes */
#include <linux/i2c.h>            /* I2C access, mutex */
#include <linux/errno.h>          /* Linux kernel error definitions */
#include <linux/hrtimer.h>        /* hrtimer */
#include <linux/workqueue.h>      /* work_struct, delayed_work */
#include <linux/wakelock.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/version.h>
#include "fusb3601_global.h"      /* Chip structure access */
#include "../core/core.h"         /* Core access */
#include "../core/platform.h"
#include "platform_helpers.h"

#ifdef FSC_DEBUG
#include "hostcomm.h"
#include "strings.h"
#include "../core/PDTypes.h"     /* State Log states */
#include "../core/TypeCTypes.h"  /* State Log states */
#endif /* FSC_DEBUG */
#include "../core/port.h"

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
#define LINE_FUNCTION hwlog_info("%s %d++\n", __func__,__LINE__);
/* *** GPIO Interface *** */

/* Device Tree names */
const char* FUSB3601_DT_INTERRUPT_INTN =    "fsc_interrupt_int_n";
#define FUSB_DT_GPIO_INTN               "fairchild,int_n"
#define FUSB_DT_GPIO_VBUS_5V            "fairchild,vbus5v"
#define FUSB_DT_GPIO_VCONN				"fairchild,vconn"

#ifdef FSC_DEBUG
#define FUSB_DT_GPIO_DEBUG_SM_TOGGLE    "fairchild,dbg_sm"
#endif  /* FSC_DEBUG */
extern struct workqueue_struct *system_highpri_wq;
extern int state_machine_need_resched;

#define FUSB_FORCE_ROLESWAP_TIMEOUT  500 * 1000

/* Internal forward declarations */
static irqreturn_t FUSB3601__fusb_isr_intn(int irq, void *dev_id);

static void FUSB3601_work_function(struct work_struct *work);

static enum hrtimer_restart FUSB3601_fusb_sm_timer_callback(struct hrtimer *timer);

static int driver_shutdown_start = 0;
void FUSB3601_set_driver_shutdown_flag(int flag)
{
	driver_shutdown_start = flag;
}
static void FUSB3601_set_drp_work_handler(struct kthread_work *work)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (!chip) {
		hwlog_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}

	hwlog_info("FUSB - %s\n", __func__);
	FUSB3601_core_set_try_snk(&chip->port);
}
void FUSB3601_fusb_StartTimer(struct hrtimer *timer, FSC_U32 time_us);
enum hrtimer_restart FUSB3601_force_state_timeout(struct hrtimer* timer)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (!chip) {
		hwlog_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return HRTIMER_NORESTART;
	}
	if (!timer) {
		hwlog_err("FUSB  %s - Error: High-resolution timer is NULL!\n", __func__);
		return HRTIMER_NORESTART;
	}

	hwlog_info("FUSB %s - Force State Timeout\n", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	kthread_queue_work(&chip->set_drp_worker, &chip->set_drp_work);
#else
	queue_kthread_work(&chip->set_drp_worker, &chip->set_drp_work);
#endif
	return HRTIMER_NORESTART;
}
static void FUSB3601_force_source(struct dual_role_phy_instance *dual_role)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		hwlog_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}
	hwlog_info("FUSB  %s - Force State Source\n",__func__);
	FUSB3601_ConfigurePortType(0x95,&chip->port);
	FUSB3601_fusb_StartTimer(&chip->timer_force_timeout,
					FUSB_FORCE_ROLESWAP_TIMEOUT);

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	if(dual_role){
		dual_role_instance_changed(dual_role);
	}
#endif
}

static void FUSB3601_force_sink(struct dual_role_phy_instance *dual_role)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();

	if (!chip) {
		hwlog_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}
	hwlog_info("FUSB  %s - Force State Sink\n",__func__);
	FUSB3601_ConfigurePortType(0x90,&chip->port);
	FUSB3601_fusb_StartTimer(&chip->timer_force_timeout,
					FUSB_FORCE_ROLESWAP_TIMEOUT);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	if(dual_role){
		dual_role_instance_changed(dual_role);
	}
#endif
}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static enum dual_role_property FUSB3601_dual_role_props[] = {
		DUAL_ROLE_PROP_SUPPORTED_MODES,
		DUAL_ROLE_PROP_MODE,
		DUAL_ROLE_PROP_PR,
		DUAL_ROLE_PROP_DR,
		DUAL_ROLE_PROP_VCONN_SUPPLY,
};

static int FUSB3601_get_dual_role_mode(void)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	int mode = DUAL_ROLE_PROP_MODE_NONE;
	hwlog_info("%s +\n",__func__);

	if(chip->orientation !=NONE) {
		if(chip->port.source_or_sink_ == Source) {
			mode = DUAL_ROLE_PROP_MODE_DFP;
		} else {
			mode = DUAL_ROLE_PROP_MODE_UFP;
		}
	} else if(chip->orientation == NONE) {
			mode = DUAL_ROLE_PROP_MODE_NONE;
	}
	hwlog_info("%s - orientation %d, mode %d\n",__func__,chip->orientation, mode);
	return mode;
}

static int FUSB3601_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
	enum dual_role_property prop, unsigned int *val)
 {
	int ret =0;
	int mode = FUSB3601_get_dual_role_mode();

	hwlog_info("%s + prop =  %d, mode = %d\n",__func__,prop,mode);
	switch(prop){
	case DUAL_ROLE_PROP_SUPPORTED_MODES:
		*val = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
		break;
	case DUAL_ROLE_PROP_MODE:
		*val = mode;
		break;
	case DUAL_ROLE_PROP_PR:
		switch(mode)
		{
		case DUAL_ROLE_PROP_MODE_DFP:
			*val = DUAL_ROLE_PROP_PR_SRC;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_PR_SRC\n",__func__,prop);
			break;
		case DUAL_ROLE_PROP_MODE_UFP:
			*val = DUAL_ROLE_PROP_PR_SNK;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_PR_SNK\n",__func__,prop);
			break;
		default:
			*val = DUAL_ROLE_PROP_PR_NONE;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_PR_NONE\n",__func__,prop);
			 break;
		}
		break;
	case DUAL_ROLE_PROP_DR:
		switch(mode)
		{
		case DUAL_ROLE_PROP_MODE_DFP:
			*val = DUAL_ROLE_PROP_DR_HOST;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_HOST\n",__func__,prop);
			break;
		case DUAL_ROLE_PROP_MODE_UFP:
			*val = DUAL_ROLE_PROP_DR_DEVICE;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_DEVICE\n",__func__,prop);
			break;
		default:
			*val = DUAL_ROLE_PROP_DR_NONE;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_NONE\n",__func__,prop);
			break;
		}
		break;
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		switch(mode)
		{
		case DUAL_ROLE_PROP_MODE_DFP:
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_YES;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_HOST\n",__func__,prop);
			break;
		case DUAL_ROLE_PROP_MODE_UFP:
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_DEVICE\n",__func__,prop);
			break;
		default:
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
			hwlog_info("%s + prop =  %d, mode = DUAL_ROLE_PROP_DR_NONE\n",__func__,prop);
		break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	hwlog_info("%s - %d\n",__func__,ret);
	return ret;
}

static int FUSB3601_dual_role_prop_is_writeable(
       struct dual_role_phy_instance *dual_role, enum dual_role_property prop)
{
	hwlog_info("%s +\n",__func__);
	switch(prop)
	{
	case DUAL_ROLE_PROP_PR:
	case DUAL_ROLE_PROP_DR:
		return 0;
	}
	return 1;
}

static int FUSB3601_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
       enum dual_role_property prop, const unsigned int *val)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	int mode = DUAL_ROLE_PROP_MODE_NONE;
	mode = FUSB3601_get_dual_role_mode();

	hwlog_info("%s +  prop= %d   val=  %d   mode = %d\n",__func__,prop,*val,mode);

	switch(prop) {
	case DUAL_ROLE_PROP_MODE:
		if(*val != mode) {
			if(DUAL_ROLE_PROP_MODE_UFP  == mode)
				FUSB3601_force_source(dual_role);
			else if(DUAL_ROLE_PROP_MODE_DFP  == mode)
				FUSB3601_force_sink(dual_role);
		}
		break;
	case DUAL_ROLE_PROP_PR:
		hwlog_info("%s DUAL_ROLE_PROP_PR\n",__func__);
   		break;
	case DUAL_ROLE_PROP_DR:
		hwlog_info("%s DUAL_ROLE_PROP_DR\n",__func__);
		break;
	default:
		hwlog_err("%s default case\n",__func__);
		break;
	}
   	hwlog_info("%s -\n",__func__);
	return 0;
}

FSC_S32 FUSB3601_dual_role_phy_init(void)
{
	hwlog_info("%s +\n",__func__);
	struct dual_role_phy_desc *dual_desc;
	struct dual_role_phy_instance *dual_role;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	dual_desc = devm_kzalloc(&chip->client->dev,sizeof(struct dual_role_phy_desc),GFP_KERNEL);

	if (!dual_desc) {
		hwlog_err("unable to allocate dual role descriptor\n");
		return -ENOMEM;
	}
	dual_role = devm_kzalloc(&chip->client->dev,sizeof(struct dual_role_phy_instance),GFP_KERNEL);
	if (!dual_role){
		devm_kfree(&chip->client->dev, dual_desc);
		pr_err("unable to allocate dual role phy instance\n");
		return -ENOMEM;
	}
	dual_desc->name = "otg_default";
	dual_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	dual_desc->properties = FUSB3601_dual_role_props;
	dual_desc->num_properties = ARRAY_SIZE(FUSB3601_dual_role_props);
	dual_desc->get_property = FUSB3601_dual_role_get_prop;
	dual_desc->set_property = FUSB3601_dual_role_set_prop;
	dual_desc->property_is_writeable = FUSB3601_dual_role_prop_is_writeable;
	dual_role = devm_dual_role_instance_register(&chip->client->dev,dual_desc);
	if(IS_ERR(dual_role)) {
		hwlog_err("fusb fail to register dual role usb\n");
		return -EINVAL;
	}
	chip->dual_desc = dual_desc;
	chip->dual_role = dual_role;
	hwlog_info("%s -\n",__func__);
	return 0;
}
#endif

FSC_S32 FUSB3601_fusb_InitializeGPIO(void)
{
    FSC_S32 ret = 0;
    struct device_node* node;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return -ENOMEM;
    }
    /* Get our device tree node */
    node = chip->client->dev.of_node;

    /* Get our GPIO pins from the device tree, allocate them,
     * and then set their direction (input/output)
     */
    chip->gpio_IntN = of_get_named_gpio(node, FUSB_DT_GPIO_INTN, 0);
    if (!gpio_is_valid(chip->gpio_IntN))
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not get named GPIO for Int_N! Err: %d\n",
            __func__, chip->gpio_IntN);
        return chip->gpio_IntN;
    }

    /* Request our GPIO to reserve it in the system - this should help ensure
     * we have exclusive access (not guaranteed)
     */
    ret = gpio_request(chip->gpio_IntN, FUSB_DT_GPIO_INTN);
    if (ret < 0)
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not request GPIO for Int_N! Err: %d\n",
            __func__, ret);
        return ret;
    }

    ret = gpio_direction_input(chip->gpio_IntN);
    if (ret < 0)
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not set GPIO dir to input for Int_N! Err: %d\n",
            __func__, ret);
        return ret;
    }

#ifdef FSC_DEBUG
    /* Export to sysfs */
    gpio_export(chip->gpio_IntN, false);
    gpio_export_link(&chip->client->dev, FUSB_DT_GPIO_INTN, chip->gpio_IntN);
#endif /* FSC_DEBUG */

    pr_info("FUSB  %s - INT_N GPIO initialized as pin '%d'\n",
        __func__, chip->gpio_IntN);

    /* VBus 5V */
	/*
    chip->gpio_VBus5V = of_get_named_gpio(node, FUSB_DT_GPIO_VBUS_5V, 0);
    if (!gpio_is_valid(chip->gpio_VBus5V))
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not get named GPIO for VBus5V! Err: %d\n",
            __func__, chip->gpio_VBus5V);
        fusb_GPIO_Cleanup();
        return chip->gpio_VBus5V;
    }
	*/

    /* Request our GPIO to reserve it in the system - this should help
     * ensure we have exclusive access (not guaranteed)
     */
	 /*
    ret = gpio_request(chip->gpio_VBus5V, FUSB_DT_GPIO_VBUS_5V);
    if (ret < 0)
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not request GPIO for VBus5V! Err: %d\n",
            __func__, ret);
        return ret;
    }

    ret = gpio_direction_output(chip->gpio_VBus5V, chip->gpio_VBus5V_value);
    if (ret < 0)
    {
        dev_err(&chip->client->dev,
            "FUSB %s - Could not set GPIO dir to output for VBus5V! Err: %d\n",
            __func__, ret);
        fusb_GPIO_Cleanup();
        return ret;
    }
*/
#ifdef FSC_DEBUG
    /* Export to sysfs */
/*
    gpio_export(chip->gpio_VBus5V, false);
    gpio_export_link(&chip->client->dev, FUSB_DT_GPIO_VBUS_5V,
                     chip->gpio_VBus5V);
*/
#endif /* FSC_DEBUG */
/*
    pr_info("FUSB  %s - VBus 5V initialized as pin '%d' and is set to '%d'\n",
        __func__, chip->gpio_VBus5V, chip->gpio_VBus5V_value ? 1 : 0);
*/
    return 0;

}

void FUSB3601_fusb_GPIO_Set_VBus5v(FSC_BOOL set)
{
    return ;
}
FSC_BOOL FUSB3601_fusb_GPIO_Get_VBus5v(void)
{
    return FALSE;
}

void FUSB3601_fusb_GPIO_Set_Vconn(FSC_BOOL set)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }
    else
    {
        /*
         * If your system routes GPIO calls through a queue of some kind, then
         * it may need to be able to sleep. If so, this call must be used.
         */
        if (gpio_cansleep(chip->gpio_Vconn))
        {
            gpio_set_value_cansleep(chip->gpio_Vconn, set ? 1: 0);
        }
        else
        {
            gpio_set_value(chip->gpio_Vconn, set ? 1 : 0);
        }

		chip->gpio_Vconn_value = set;
    }
}
FSC_BOOL FUSB3601_fusb_GPIO_Get_Vconn(void)
{
	FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return false;
    }
    else
    {
        /*
         * If your system routes GPIO calls through a queue of some kind, then
         * it may need to be able to sleep. If so, this call must be used.
         */
        if (gpio_cansleep(chip->gpio_Vconn))
        {
            ret = !gpio_get_value_cansleep(chip->gpio_Vconn);
        }
        else
        {
            ret = !gpio_get_value(chip->gpio_Vconn);
        }
        return (ret != 0);
    }
}

FSC_BOOL FUSB3601_fusb_GPIO_Get_IntN(void)
{
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return false;
    }
    else
    {
        /*
         * If your system routes GPIO calls through a queue of some kind, then
         * it may need to be able to sleep. If so, this call must be used.
         */
        if (gpio_cansleep(chip->gpio_IntN))
        {
            ret = !gpio_get_value_cansleep(chip->gpio_IntN);
        }
        else
        {
            ret = !gpio_get_value(chip->gpio_IntN);
        }
        return (ret != 0);
    }
}

#ifdef FSC_DEBUG
void FUSB3601_dbg_fusb_GPIO_Set_SM_Toggle(FSC_BOOL set)
{

}

FSC_BOOL FUSB3601_dbg_fusb_GPIO_Get_SM_Toggle(void)
{
	return 0;
}
#endif  /* FSC_DEBUG */

void FUSB3601_fusb_GPIO_Cleanup(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }

    /* -1 indicates that we don't have an IRQ to free */
    if (gpio_is_valid(chip->gpio_IntN) && chip->gpio_IntN_irq != -1)
    {
        devm_free_irq(&chip->client->dev, chip->gpio_IntN_irq, chip);
    }

    wake_lock_destroy(&chip->fusb3601_wakelock);

    if (gpio_is_valid(chip->gpio_IntN) >= 0)
    {
#ifdef FSC_DEBUG
        gpio_unexport(chip->gpio_IntN);
#endif /* FSC_DEBUG */

        gpio_free(chip->gpio_IntN);
    }
}

/* *** I2C Interface *** */
FSC_BOOL FUSB3601_fusb_I2C_WriteData(FSC_U8 address, FSC_U8 length, FSC_U8* data)
{
    FSC_S32 i = 0;
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();

//    pr_err("%s:address = 0x%x,data = 0x%x!\n",__func__,address,*data);
    if (chip == NULL || chip->client == NULL || data == NULL)
    {
        pr_err("FUSB %s - %s is NULL!\n",
            __func__, (chip == NULL ? "Internal chip structure"
            : (chip->client == NULL ? "I2C Client"
            : "Write data buffer")));
        return false;
    }

    mutex_lock(&chip->lock);

    /* Retry on failure up to the retry limit */
    for (i = 0; i <= chip->numRetriesI2C; i++)
    {
        ret = i2c_smbus_write_i2c_block_data(chip->client, address,
                                             length, data);

        if (ret < 0)
        {
            /* Errors report as negative */
            dev_err(&chip->client->dev,
                "%s - I2C error block writing byte data. "
                "Address: '0x%02x', Return: '%d'.  Attempt #%d / %d...\n",
                __func__, address, ret, i, chip->numRetriesI2C);
        }
        else
        {
            /* Successful i2c writes should always return 0 */
            break;
        }
    }

    mutex_unlock(&chip->lock);
    return (ret >= 0);
}

FSC_BOOL FUSB3601_fusb_I2C_ReadData(FSC_U8 address, FSC_U8* data)
{
    FSC_S32 i = 0;
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
 //   pr_err("%s:address = 0x%x!\n",__func__,address);
    if (chip == NULL || chip->client == NULL || data == NULL)
    {
        pr_err("FUSB %s - %s is NULL!\n",
            __func__, (chip == NULL ? "Internal chip structure"
            : (chip->client == NULL ? "I2C Client"
            : "read data buffer")));
        return false;
    }

    mutex_lock(&chip->lock);

    /* Retry on failure up to the retry limit */
    for (i = 0; i <= chip->numRetriesI2C; i++)
    {
        ret = i2c_smbus_read_byte_data(chip->client, (u8)address);

        if (ret < 0)
        {
            /* Errors report as negative */
            dev_err(&chip->client->dev,
                "%s - I2C error reading byte data. "
                "Address: '0x%02x', Return: '%d'.  Attempt #%d / %d...\n",
                __func__, address, ret, i, chip->numRetriesI2C);
        }
        else
        {
            /* Successful i2c writes should always return 0 */
            *data = (FSC_U8)ret;
            break;
        }
    }

    mutex_unlock(&chip->lock);
    return (ret >= 0);
}
int FUSB3601_fusb_i2c_write_mask(u8 reg,u8 MASK,u8 SHIFT,u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = FUSB3601_fusb_I2C_ReadData(reg,&val);
	if( ret < 0)
		return ret;
	val &= ~MASK;
	val |=((value << SHIFT)&MASK);
	pr_info("%s:val= %d\n",__func__,val);
	ret = FUSB3601_fusb_I2C_WriteData(reg,1,&val);
	return ret;
}
FSC_BOOL FUSB3601_fusb_I2C_ReadBlockData(FSC_U8 address, FSC_U8 length, FSC_U8* data)
{
    FSC_S32 i = 0;
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL || chip->client == NULL || data == NULL)
    {
        pr_err("FUSB %s - %s is NULL!\n",
            __func__, (chip == NULL ? "Internal chip structure"
            : (chip->client == NULL ? "I2C Client"
            : "block read data buffer")));
        return false;
    }

    mutex_lock(&chip->lock);

    /* Retry on failure up to the retry limit */
    for (i = 0; i <= chip->numRetriesI2C; i++)
    {
        ret = i2c_smbus_read_i2c_block_data(chip->client, (u8)address,
                                            (u8)length, (u8*)data);

        if (ret < 0)
        {
            /* Errors report as negative */
            dev_err(&chip->client->dev,
                "%s - I2C error block reading byte data. "
                "Address: '0x%02x', Return: '%d'.  Attempt #%d / %d...\n",
                __func__, address, ret, i, chip->numRetriesI2C);
        }
        else if (ret != length)
        {
            /* We didn't read everything we wanted */
            dev_err(&chip->client->dev,
          "FUSB %s - Block read request of %u bytes truncated to %u bytes.\n",
                __func__, length, I2C_SMBUS_BLOCK_MAX);
        }
        else
        {
            /* Success, don't retry */
            break;
        }
    }

    mutex_unlock(&chip->lock);
    return (ret == length);
}

#ifdef FSC_DEBUG
/* *** SysFS Interface *** */

/*******************************************************************************
* Function:        fusb_timestamp_bytes_to_time
* Input:           outSec: Seconds part of output is stored here
*                  outMS10ths: 10ths of MS part of output is stored here
*                  inBuf: Ptr to first of 4 timestamp bytes,
*                  where the timestamp is in this format:
*                    [HI-10thsMS LO-10thsMS HI-Sec LO-Sec]
* Return:          None
* Description:     Parses the 4 bytes in inBuf into a 2-part timestamp:
*                    Seconds and 10ths of MS
*******************************************************************************/
void FUSB3601_fusb_timestamp_bytes_to_time(FSC_U32* outSec, FSC_U32* outMS10ths,
                                  FSC_U8* inBuf)
{
    if (outSec && outMS10ths && inBuf)
    {
        *outMS10ths = ((FSC_U32)inBuf[0] << 8) | inBuf[1];
        *outSec = ((FSC_U32)inBuf[2] << 8) | inBuf[3];
    }
}

/*******************************************************************************
* Function:        fusb_get_pd_message_type
* Input:           header: PD message header. Bits 4..0 are the pd message
*                   type, bits 14..12 are num data objs
*                  out: Buffer to which the message type will be written,
*                   should be at least 32 bytes long
* Return:          int - Number of chars written to out, negative on error
* Description:     Parses both PD message header bytes for the message type
*                   as a null-terminated string.
*******************************************************************************/
FSC_S32 FUSB3601_fusb_get_pd_message_type(FSC_U16 header, FSC_U8* out)
{
    FSC_S32 numChars = -1;   /* Number of chars written, return value */

    if ((!out) || !(out + 31))    /* Check for our 32 byte buffer */
    {
        pr_err("%s FUSB - Invalid input buffer! header: 0x%x\n",
            __func__, header);
        return -1;
    }

    /* Bits 14..12 give num of data obj. This is a data message if there
     * are data objects, otherwise it's a control message
     * See the PD spec, Table 6-1 "Message Header", for more details.
     */
    if ((header & 0x7000) > 0)
    {
        switch (header & 0x0F)
        {
            case DMTSourceCapabilities:
            {
                numChars = sprintf(out, "Source Capabilities");
                break;
            }
            case DMTRequest:
            {
                numChars = sprintf(out, "Request");
                break;
            }
            case DMTBIST:
            {
                numChars = sprintf(out, "BIST");
                break;
            }
            case DMTSinkCapabilities:
            {
                numChars = sprintf(out, "Sink Capabilities");
                break;
            }
            case 0b00101:
            {
                numChars = sprintf(out, "Battery Status");
                break;
            }
            case 0b00110:
            {
                numChars = sprintf(out, "Source Alert");
                break;
            }
            case DMTVendorDefined:
            {
                numChars = sprintf(out, "Vendor Defined");
                break;
            }
            default:
            {
                numChars = sprintf(out, "Reserved (Data) (0x%x)", header);
                break;
            }
        }
    }
    else
    {
        switch (header & 0x0F)
        {
            case CMTGoodCRC:
            {
                numChars = sprintf(out, "Good CRC");
                break;
            }
            case CMTGotoMin:
            {
                numChars = sprintf(out, "Go to Min");
                break;
            }
            case CMTAccept:
            {
                numChars = sprintf(out, "Accept");
                break;
            }
            case CMTReject:
            {
                numChars = sprintf(out, "Reject");
                break;
            }
            case CMTPing:
            {
                numChars = sprintf(out, "Ping");
                break;
            }
            case CMTPS_RDY:
            {
                numChars = sprintf(out, "PS_RDY");
                break;
            }
            case CMTGetSourceCap:
            {
                numChars = sprintf(out, "Get Source Capabilities");
                break;
            }
            case CMTGetSinkCap:
            {
                numChars = sprintf(out, "Get Sink Capabilities");
                break;
            }
            case CMTDR_Swap:
            {
                numChars = sprintf(out, "Data Role Swap");
                break;
            }
            case CMTPR_Swap:
            {
                numChars = sprintf(out, "Power Role Swap");
                break;
            }
            case CMTVCONN_Swap:
            {
                numChars = sprintf(out, "VConn Swap");
                break;
            }
            case CMTWait:
            {
                numChars = sprintf(out, "Wait");
                break;
            }
            case CMTSoftReset:
            {
                numChars = sprintf(out, "Soft Reset");
                break;
            }
            case 0b01110:
            {
                numChars = sprintf(out, "Not Supported");
                break;
            }
            case 0b01111:
            {
                numChars = sprintf(out, "Get Source Cap Ext");
                break;
            }
            case 0b10000:
            {
                numChars = sprintf(out, "Get Source Status");
                break;
            }
            case 0b10001:
            {
                numChars = sprintf(out, "FR Swap");
                break;
            }
            default:
            {
                numChars = sprintf(out, "Reserved (CMD) (0x%x)", header);
                break;
            }
        }
    }
    return numChars;
}

/*******************************************************************************
* Function:        fusb_Sysfs_Handle_Read
* Input:           output: Buffer to which the output will be written
* Return:          Number of chars written to output
* Description:     Reading this file will output the most recently
*                   saved hostcomm output buffer
*******************************************************************************/

/* Arbitrary temp buffer for parsing out driver data to sysfs */
#define FUSB_MAX_BUF_SIZE 256

static ssize_t FUSB3601__fusb_Sysfs_Hostcomm_show(struct device* dev,
                                         struct device_attribute* attr,
                                         char* buf)
{
    FSC_S32 i = 0;
    FSC_S32 numLogs = 0;
    FSC_S32 numChars = 0;
    FSC_U32 TimeStampSeconds = 0; /* Timestamp value in seconds */
    FSC_U32 TimeStampMS10ths = 0; /* Timestamp fraction in 10ths of ms */
    FSC_S8 tempBuf[FUSB_MAX_BUF_SIZE] = { 0 };

    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return 0;
    }
    else if (buf == NULL || chip->HostCommBuf == NULL)
    {
        pr_err("%s - Buffer is null!\n", __func__);
        return 0;
    }
    else if (chip->HostCommBuf[0] == CMD_READ_PD_STATE_LOG)
    {
        /* Parse out the PD state log */

        /* First byte echos the command, 4th byte is number
         * of logs (2nd and 3rd bytes reserved as 0) */

        numLogs = chip->HostCommBuf[3];

        numChars += sprintf(tempBuf, "PD State Log has %u entries:\n", numLogs);
        strcat(buf, tempBuf);

        /* Relevant data starts at 5th byte in this format:
         * CMD 0 0 #Logs PDState time time time time
         *
         * Must be able to peek 4 bytes ahead, and don't overflow the
         * output buffer (PAGE_SIZE)
         */
        for (i = 4; (i + 4 < FSC_HOSTCOMM_BUFFER_SIZE) &&
                    (numChars < PAGE_SIZE) && (numLogs > 0); i += 5, numLogs--)
        {
            FUSB3601_fusb_timestamp_bytes_to_time(&TimeStampSeconds, &TimeStampMS10ths,
                &chip->HostCommBuf[i + 1]);

            /* sprintf should be safe here because we're controlling
             * the strings being printed, just make sure the strings are
             * less than FUSB_MAX_BUF_SIZE+1
             */
            numChars += sprintf(tempBuf, "[%u.%04u]\t%s\n",
                TimeStampSeconds, TimeStampMS10ths,
                str_PDStateNames[(u8)chip->HostCommBuf[i]]);

            strcat(buf, tempBuf);
        }

        strcat(buf, "\n");   /* Append a newline for pretty++ */
        numChars++;          /* Account for newline */
    }
    else if (chip->HostCommBuf[0] == CMD_READ_STATE_LOG)
    {
        /* Parse out the Type-C state log */

        /* First byte echos the command, 4th byte is number of logs
         * (2nd and 3rd bytes reserved as 0) */

        numLogs = chip->HostCommBuf[3];
        numChars += sprintf(tempBuf,
            "Type-C State Log has %u entries:\n", numLogs);
        strcat(buf, tempBuf);

        /* Relevant data starts at 5th byte in this format:
         * CMD 0 0 #Logs State time time time time
         *
         * Must be able to peek 4 bytes ahead, and don't overflow the
         * output buffer (PAGE_SIZE), only print logs we have
         */
        for (i = 4; (i + 4 < FSC_HOSTCOMM_BUFFER_SIZE) &&
            (numChars < PAGE_SIZE) && (numLogs > 0); i += 5, numLogs--)
        {
            /* Parse out the timestamp */
            FUSB3601_fusb_timestamp_bytes_to_time(&TimeStampSeconds, &TimeStampMS10ths,
                &chip->HostCommBuf[i + 1]);

            /* sprintf should be safe here because we're controlling
             * the strings being printed, just make sure the strings are
             * less than FUSB_MAX_BUF_SIZE+1
             */
            numChars += sprintf(tempBuf, "[%u.%04u]\t%s\n",
                TimeStampSeconds, TimeStampMS10ths,
                str_TypeCStateNames[(u8)chip->HostCommBuf[i]]);

            strcat(buf, tempBuf);
        }

        strcat(buf, "\n");   /* Append a newline for pretty++ */
        numChars++;          /* Account for newline */
    }
    else
    {
        for (i = 0; i < FSC_HOSTCOMM_BUFFER_SIZE; i++)
        {
            numChars += scnprintf(tempBuf, 6 * sizeof(char), "0x%02x ",
                chip->HostCommBuf[i]); /* Copy 1 byte + null term */

            strcat(buf, tempBuf); /* Append each number to the output buffer */
        }

        strcat(buf, "\n");   /* Append a newline for pretty++ */
        numChars++;          /* Account for newline */
    }

    return numChars;
}

/*******************************************************************************
* Function:        fusb_Sysfs_Handle_Write
* Input:           input: Buffer passed in from OS
*                   (space-separated list of 8-bit hex values)
*                  size: Number of chars in input
*                  output: Buffer to which the output will be written
* Return:          Number of chars written to output
* Description:     Performs hostcomm duties, and stores output buffer
*                   in chip structure
*******************************************************************************/
static ssize_t FUSB3601__fusb_Sysfs_Hostcomm_store(struct device* dev,
                                          struct device_attribute* attr,
                                          const char* input,
                                          size_t size)
{
    FSC_S32 ret = 0;
    FSC_S32 i = 0;
    FSC_S32 j = 0;
    FSC_S8 tempByte = 0;
    FSC_S32 numBytes = 0;

    /* Temp buffer to parse out individual hex vals, +1 for null terminator */
    FSC_S8 temp[6] = { 0 };

    FSC_S8 temp_input[FSC_HOSTCOMM_BUFFER_SIZE] = { 0 };
    FSC_S8 output[FSC_HOSTCOMM_BUFFER_SIZE] = { 0 };
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return 0;
    }
    else if (input == NULL)
    {
        pr_err("FUSB %s - Input buffer is NULL!\n", __func__);
        return 0;
    }
    else
    {
        /* Convert the buffer to hex values */
        for (i = 0; i < size; i = i + j)
        {
            /* Parse out a hex number (at most 5 chars: "0x## ") */
            for (j = 0; (j < 5) && (j + i < size); j++)
            {
                /* End of the hex number (space-delimited) */
                if (input[i + j] == ' ')
                {
                    /* We found a space, stop copying and convert */
                    break;
                }

                /* Copy the non-space byte into the temp buffer */
                temp[j] = input[i + j];
            }

            /* Add a null terminator and move past the space */
            temp[j++] = 0;

            /* We have a hex digit (hopefully), now convert it */
            ret = kstrtou8(temp, 16, &tempByte);

            if (ret != 0)
            {
                pr_err(
                    "FUSB  %s - Hostcomm input is not valid! Return: '%d'\n",
                    __func__, ret);
                return 0;  /* Quit on error */
            }
            else
            {
                temp_input[numBytes++] = tempByte;
                if (numBytes >= FSC_HOSTCOMM_BUFFER_SIZE)
                {
                    break;
                }
            }
        }

        /* Handle the message */
        FUSB3601_fusb_ProcessMsg(&chip->port, temp_input, output);

        /* Copy input into temp buffer */
        memcpy(chip->HostCommBuf, output, FSC_HOSTCOMM_BUFFER_SIZE);

        /* Schedule the process to handle the state machine processing */
        // queue_work(system_highpri_wq, &chip->sm_worker);
    }

    return size;
}

/* Fetch and display the PD state log */
static ssize_t FUSB3601__fusb_Sysfs_PDStateLog_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    FSC_S32 i = 0;
    FSC_S32 numChars = 0;
    FSC_S32 numLogs = 0;
    FSC_U32 TimeStampSeconds = 0;   /* Timestamp value in seconds */
    FSC_U32 TimeStampMS10ths = 0;   /* Timestamp fraction in 10ths of ms's */
    FSC_U8 output[FSC_HOSTCOMM_BUFFER_SIZE] = { 0 };
    FSC_U8 tempBuf[FUSB_MAX_BUF_SIZE] = { 0 };
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return 0;
    }

    tempBuf[0] = CMD_READ_PD_STATE_LOG; /* To request the PD statelog */

    /* Get the PD State Log */
    FUSB3601_fusb_ProcessMsg(&chip->port, tempBuf, output);

    /* First byte echos the command, 4th byte is number of
     * logs (2nd and 3rd bytes reserved as 0) */

    numLogs = output[3];
    numChars += sprintf(tempBuf, "PD State Log has %u entries:\n", numLogs);
    strcat(buf, tempBuf);

    /* Relevant data starts at 5th byte in this format:
     * CMD 0 0 #Logs PDState time time time time
     * Must be able to peek 4 bytes ahead, and don't overflow
     * the output buffer (PAGE_SIZE)
     */
    for (i = 4; (i + 4 < FSC_HOSTCOMM_BUFFER_SIZE) &&
        (numChars < PAGE_SIZE) && (numLogs > 0); i += 5, numLogs--)
    {
        FUSB3601_fusb_timestamp_bytes_to_time(&TimeStampSeconds, &TimeStampMS10ths,
            &output[i + 1]);

        /* sprintf should be safe here because we're controlling the
         * strings being printed, just make sure the strings are less
         * than FUSB_MAX_BUF_SIZE+1
         */
        numChars += sprintf(tempBuf, "[%u.%04u]\t%s\n",
            TimeStampSeconds, TimeStampMS10ths, str_PDStateNames[output[i]]);

        strcat(buf, tempBuf);
    }

    strcat(buf, "\n");   /* Append a newline for pretty++ */
    return ++numChars;   /* Account for newline and return number of bytes */
}

/* Fetch and display the Type-C state log */
static ssize_t FUSB3601__fusb_Sysfs_TypeCStateLog_show(struct device* dev,
                                              struct device_attribute* attr,
                                              char* buf)
{
    FSC_S32 i = 0;
    FSC_S32 numChars = 0;
    FSC_S32 numLogs = 0;
    FSC_U32 TimeStampSeconds = 0;
    FSC_U32 TimeStampMS10ths = 0;
    FSC_S8 output[FSC_HOSTCOMM_BUFFER_SIZE] = { 0 };
    FSC_S8 tempBuf[FUSB_MAX_BUF_SIZE] = { 0 };
    struct fusb3601_chip* chip = fusb3601_GetChip();

    tempBuf[0] = CMD_READ_STATE_LOG;

    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return 0;
    }

    /* Get the PD State Log */
    FUSB3601_fusb_ProcessMsg(&chip->port, tempBuf, output);

    /* First byte echos the command, 4th byte is number of logs
     * (2nd and 3rd bytes reserved as 0) */

    numLogs = output[3];
    numChars += sprintf(tempBuf, "Type-C State Log has %u entries:\n", numLogs);
    strcat(buf, tempBuf);

    /* Relevant data starts at 5th byte in this format:
     * CMD 0 0 #Logs State time time time time
     * Must be able to peek 4 bytes ahead, and don't overflow the
     * output buffer (PAGE_SIZE), only print logs we have
     */
    for (i = 4; (i + 4 < FSC_HOSTCOMM_BUFFER_SIZE) &&
        (numChars < PAGE_SIZE) && (numLogs > 0); i += 5, numLogs--)
    {
        /* Parse out the timestamp */
        FUSB3601_fusb_timestamp_bytes_to_time(&TimeStampSeconds, &TimeStampMS10ths,
            &output[i + 1]);

        /* sprintf should be safe here because we're controlling the strings
         * being printed, just make sure the strings are less than
         * FUSB_MAX_BUF_SIZE+1
         */
        numChars += sprintf(tempBuf, "[%u.%04u]\t%s\n",
            TimeStampSeconds, TimeStampMS10ths, str_TypeCStateNames[output[i]]);

        strcat(buf, tempBuf);
    }

    strcat(buf, "\n");   /* Append a newline for pretty++ */
    return ++numChars;   /* Account for newline and return number of bytes */
}

/* Reinitialize the FUSB3601 */
static ssize_t FUSB3601__fusb_Sysfs_Reinitialize_fusb3601(struct device* dev,
                                                struct device_attribute* attr,
                                                char* buf)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL)
    {
        return sprintf(buf,
            "FUSB3601 Error: Internal chip structure pointer is NULL!\n");
    }

    /* Make sure that we are doing this in a thread-safe manner */

    /* Wait for current IRQ handler to return, then disables it */
    disable_irq(chip->gpio_IntN_irq);

    FUSB3601_core_initialize(&chip->port);
    pr_debug ("FUSB  %s - Core is initialized!\n", __func__);
    FUSB3601_core_enable_typec(&chip->port, TRUE);
    pr_debug ("FUSB  %s - Type-C State Machine is enabled!\n", __func__);

    FUSB3601_core_state_machine(&chip->port);

    enable_irq(chip->gpio_IntN_irq);

    return sprintf(buf, "FUSB3601 Reinitialized!\n");
}

/* Define our device attributes to export them to sysfs */
static DEVICE_ATTR(hostcomm, S_IRWXU | S_IRWXG , FUSB3601__fusb_Sysfs_Hostcomm_show, FUSB3601__fusb_Sysfs_Hostcomm_store);
static DEVICE_ATTR(pd_state_log, S_IRUSR | S_IRGRP , FUSB3601__fusb_Sysfs_PDStateLog_show, NULL);
static DEVICE_ATTR(typec_state_log, S_IRUSR | S_IRGRP, FUSB3601__fusb_Sysfs_TypeCStateLog_show, NULL);
static DEVICE_ATTR(reinitialize, S_IRUSR | S_IRGRP, FUSB3601__fusb_Sysfs_Reinitialize_fusb3601, NULL);

static struct attribute *fusb3601_sysfs_attrs[] = {
    &dev_attr_hostcomm.attr,
    &dev_attr_pd_state_log.attr,
    &dev_attr_typec_state_log.attr,
    &dev_attr_reinitialize.attr,
    NULL
};

static struct attribute_group fusb3601_sysfs_attr_grp = {
    .name = "control",
    .attrs = fusb3601_sysfs_attrs,
};

void FUSB3601_fusb_Sysfs_Init(void)
{
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return;
    }
    /* create attribute group for accessing the FUSB3601 */
    ret = sysfs_create_group(&chip->client->dev.kobj, &fusb3601_sysfs_attr_grp);
    if (ret)
    {
        pr_err("FUSB %s - Error creating sysfs attributes!\n", __func__);
    }
}

#endif /* FSC_DEBUG */

/* *** Driver Helpers *** */
void FUSB3601_fusb_InitializeCore(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }

    FUSB3601_core_initialize(&chip->port);
    pr_debug("FUSB  %s - Core is initialized!\n", __func__);
    FUSB3601_core_enable_typec(&chip->port, TRUE);
    pr_debug("FUSB  %s - Type-C State Machine is enabled!\n", __func__);
}

FSC_BOOL FUSB3601_fusb_IsDeviceValid(void)
{
    FSC_U8 val = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return FALSE;
    }

    /* Test to see if we can do a successful I2C read */
    if (!FUSB3601_fusb_I2C_ReadData((FSC_U8)0x01, &val))
    {
        pr_err("FUSB  %s - Could not communicate with device over I2C!\n",
            __func__);
        return FALSE;
    }

    return TRUE;
}

FSC_BOOL FUSB3601_fusb_reset(void)
{
    FSC_U8 val = 0x40;
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return FALSE;
    }

    if (!FUSB3601_fusb_I2C_WriteData((FSC_U8)0x80, 1, &val))
    {
        pr_err("FUSB  %s - Could not communicate with device over I2C!\n",
            __func__);
        return FALSE;
    }
    /* TODO need to reset stored port config */

    return TRUE;

}
FSC_BOOL FUSB3601_fusb_reset_with_adc_reset(void)
{
    FSC_U8 val = 0x40;
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return FALSE;
    }

    /* Reset the FUSB3601 and including the I2C registers to their defaults. */
    if (!FUSB3601_fusb_I2C_WriteData((FSC_U8)0x80, 1, &val))
    {
        pr_err("FUSB  %s - Could not communicate with device over I2C!\n",
            __func__);
        return FALSE;
    }
    reset_adc(&chip->port);

    if (!FUSB3601_fusb_I2C_WriteData((FSC_U8)0x80, 1, &val))
    {
        pr_err("FUSB  %s - Could not communicate with device over I2C!\n",
            __func__);
        return FALSE;
    }
    /* TODO need to reset stored port config */

    return TRUE;
}

void FUSB3601_fusb_InitChipData(void)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();

    if (chip == NULL)
    {
        pr_err("%s - Chip structure is null!\n", __func__);
        return;
    }

#ifdef FSC_DEBUG
    chip->dbgTimerTicks = 0;
    chip->dbgTimerRollovers = 0;
    chip->dbgSMTicks = 0;
    chip->dbgSMRollovers = 0;
    chip->dbg_gpio_StateMachine = -1;
    chip->dbg_gpio_StateMachine_value = false;

    chip->debugfs_parent = NULL;
#endif  /* FSC_DEBUG */

    /* GPIO Defaults */
    chip->gpio_VBus5V = -1;
    chip->gpio_VBus5V_value = false;
    chip->gpio_IntN = -1;
    chip->gpio_IntN_irq = -1;
	chip->gpio_Vconn = -1;
    chip->gpio_Vconn_value = false;

    /* I2C Configuration */
    chip->InitDelayMS = INIT_DELAY_MS;  /* Time to wait before device init */
    chip->numRetriesI2C = RETRIES_I2C;  /* Number of times to retry */
    chip->use_i2c_blocks = false;       /* Assume failure */

    /* Worker thread setup */
    INIT_WORK(&chip->sm_worker, FUSB3601_work_function);
    chip ->queued = FALSE;

    chip ->highpri_wq = alloc_workqueue("FUSB WQ",WQ_HIGHPRI|WQ_UNBOUND,1);

    if(chip->highpri_wq == NULL)
	{
		pr_err("FUSB %s - Unable to creat new work queue\n",__func__);
		return;
	}

    /* HRTimer Setup */
    hrtimer_init(&chip->sm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
    chip->sm_timer.function = FUSB3601_fusb_sm_timer_callback;
    hrtimer_init(&chip->timer_force_timeout, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    chip->timer_force_timeout.function = FUSB3601_force_state_timeout;

    /* Initialize latency values */

}

/* *** IRQ/Threading Helpers *** */
FSC_S32 FUSB3601_fusb_EnableInterrupts(void)
{
    FSC_S32 ret = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return -ENOMEM;
    }

    wake_lock_init(&chip->fusb3601_wakelock, WAKE_LOCK_SUSPEND,
        "fusb3601wakelock");

    /* Set up IRQ for INT_N GPIO */
    ret = gpio_to_irq(chip->gpio_IntN); /* Returns negative errno on error */
    if (ret < 0)
    {
        pr_err( "FUSB %s - Unable to request IRQ for INT_N GPIO! Err: %d\n",
            __func__, ret);
        dev_err(&chip->client->dev,
            "FUSB %s - Unable to request IRQ for INT_N GPIO! Err: %d\n",
            __func__, ret);
        chip->gpio_IntN_irq = -1; /* Set to indicate error */
        FUSB3601_fusb_GPIO_Cleanup();
        return ret;
    }

    chip->gpio_IntN_irq = ret;

    pr_info("%s - Success: Requested INT_N IRQ: '%d'\n",
        __func__, chip->gpio_IntN_irq);

    /* Use NULL thread_fn as we will be queueing a work function in the handler.
     * Trigger is active-low, don't handle concurrent interrupts.
     * devm_* allocation/free handled by system */
    ret = devm_request_threaded_irq(&chip->client->dev, chip->gpio_IntN_irq,
        NULL, FUSB3601__fusb_isr_intn, IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
        FUSB3601_DT_INTERRUPT_INTN, chip);

    if (ret)
    {
        dev_err(&chip->client->dev,
          "FUSB %s - Unable to request threaded IRQ for INT_N GPIO! Err: %d\n",
            __func__, ret);
        FUSB3601_fusb_GPIO_Cleanup();
        return ret;
    }

    enable_irq_wake(chip->gpio_IntN_irq);

    /* Run the state machines to initialize everything. */
    wake_lock(&chip->fusb3601_wakelock);
    queue_work(chip->highpri_wq, &chip->sm_worker);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
    kthread_init_worker(&chip->set_drp_worker);
#else
    init_kthread_worker(&chip->set_drp_worker);
#endif
    chip->set_drp_worker_task = kthread_run(kthread_worker_fn,
        &chip->set_drp_worker, "fusb3601 set drp worker");
    sched_setscheduler(chip->set_drp_worker_task, SCHED_FIFO, &param);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
    kthread_init_work(&chip->set_drp_work, FUSB3601_set_drp_work_handler);
#else
    init_kthread_work(&chip->set_drp_work, FUSB3601_set_drp_work_handler);
#endif

    return 0;
}

void FUSB3601_fusb_StartTimer(struct hrtimer *timer, FSC_U32 time_us)
{
    ktime_t ktime;
    struct fusb3601_chip *chip = fusb3601_GetChip();
    if (!chip) {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }

    mutex_lock(&chip->lock);

    ktime = ktime_set(0, time_us * 1000);
    hrtimer_start(timer, ktime, HRTIMER_MODE_REL);

    mutex_unlock(&chip->lock);

    return;
}

void FUSB3601_fusb_StopTimer(struct hrtimer *timer)
{
    struct fusb3601_chip *chip = fusb3601_GetChip();
    if (!chip) {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }

    mutex_lock(&chip->lock);

    if (hrtimer_active(timer) != 0) {
        hrtimer_cancel(timer);
    }

    if (hrtimer_is_queued(timer) != 0) {
        hrtimer_cancel(timer);
    }

    mutex_unlock(&chip->lock);

    return;
}

/*******************************************************************************
* Function:        _fusb_isr_intn
* Input:           irq - IRQ that was triggered
*                  dev_id - Ptr to driver data structure
* Return:          irqreturn_t - IRQ_HANDLED on success, IRQ_NONE on failure
* Description:     Activates the core
*******************************************************************************/
static irqreturn_t FUSB3601__fusb_isr_intn(FSC_S32 irq, void *dev_id)
{
    struct fusb3601_chip* chip = dev_id;
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return IRQ_NONE;
    }

    pr_err("FUSB INTN\n");

    /* Schedule the process to handle the state machine processing */

    //if(!chip->queued)
	{
		//chip->queued = TRUE;
		wake_lock(&chip->fusb3601_wakelock);
		queue_work(chip->highpri_wq, &chip->sm_worker);
	}


    return IRQ_HANDLED;
}

static enum hrtimer_restart FUSB3601_fusb_sm_timer_callback(struct hrtimer *timer)
{
    struct fusb3601_chip* chip =
        container_of(timer, struct fusb3601_chip, sm_timer);

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return HRTIMER_NORESTART;
    }

    pr_err("FUSB TIMER\n");
    //pr_err("FUSB %s TIMER Interrupt Triggered\n",__func__);
//#endif /* FUSB_DRIVER_PRINTS */

    /* Schedule the process to handle the state machine processing */
     //if(!chip->queued)
	{
		//chip->queued = TRUE;
		wake_lock(&chip->fusb3601_wakelock);
		queue_work(chip->highpri_wq, &chip->sm_worker);
	}



    return HRTIMER_NORESTART;
}

static void FUSB3601_work_function(struct work_struct *work)
{
    FSC_U32 timeout = 0;

    struct Port *port;
    struct fusb3601_chip* chip =
        container_of(work, struct fusb3601_chip, sm_worker);

    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }
    port = &chip->port;
    if (!port) {
        pr_err("FUSB  %s - Port structure is NULL!\n", __func__);
        return;
    }

    pr_err("FUSB WORK\n");

    down(&chip->suspend_lock);

//#ifdef FSC_DEBUG
    /* Toggle debug GPIO when SM is called to measure thread tick rate */
    //dbg_fusb_GPIO_Set_SM_Toggle(TRUE);

    /* Run the state machine */
    do
    {
        pr_err("FUSB CALLING CORE STATE MACHINE\n");
        FUSB3601_core_state_machine(&chip->port);
        pr_err("FUSB driver_shutdown_start: %d, irq state: %d\n", driver_shutdown_start, FUSB3601_fusb_GPIO_Get_IntN());
    } while(!driver_shutdown_start && FUSB3601_platform_get_device_irq_state(chip->port.port_id_));

//#ifdef FSC_DEBUG
    /* Toggle debug GPIO when SM is called to measure thread tick rate */
    //dbg_fusb_GPIO_Set_SM_Toggle(FALSE);
//#endif /* FSC_DEBUG */
    //if (platform_get_device_irq_state(chip->port.port_id_))
    //{
    //    queue_work(chip ->highpri_wq, &chip->sm_worker);
    //}
    //else
    //	{
    //chip->queued = FALSE;
    /* Scan through the timers to see if we need a timer callback */
    timeout = FUSB3601_core_get_next_timeout(&chip->port);
    //pr_err("FUSB %s Queueing Timeout: %d\n",__func__,timeout);
    if (timeout > 0) {
        FUSB3601_fusb_StartTimer(&chip->sm_timer, timeout * 1000);
    } else if (state_machine_need_resched || (port->tc_state_ == AttachedSource && FUSB3601_TimerExpired(&port->cc_debounce_timer_))) {
        FUSB3601_core_state_machine(&chip->port);
    } else {
        FUSB3601_fusb_StopTimer(&chip->sm_timer);
    }
	//}

    pr_err("FUSB DONE\n");

    up(&chip->suspend_lock);
    wake_unlock(&chip->fusb3601_wakelock);
}
