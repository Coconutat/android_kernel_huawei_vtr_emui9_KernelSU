
#include <linux/printk.h>           /* pr_err, printk, etc */
#include <linux/delay.h>            /* udelay, etc. */
#include <linux/kthread.h>
#include "fusb3601_global.h"        /* Chip structure */
#include "platform_helpers.h"       /* Implementation details */
#include "../core/platform.h"
#include <huawei_platform/usb/hw_pd_dev.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/delay.h>
#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define TCP_VBUS_CTRL_PD_DETECT (1 << 7)
typedef enum
{
	TCPC_CTRL_PLUG_ORIENTATION = (1 << 0),
	TCPC_CTRL_BIST_TEST_MODE = (1 << 1)
}tcpc_ctrl_t;

//FSC_BOOL g_pd_notified;
//struct pd_dpm_vbus_state g_vbus_state;
/*******************************************************************************
* Function:        platform_get_device_irq_state
* Input:           Port ID - 0 if one port system
* Return:          Boolean.  TRUE = Interrupt Active
* Description:     Get the state of the INT_N pin.  INT_N is active low.  This
*                  function handles that by returning TRUE if the pin is
*                  pulled low indicating an active interrupt signal.
******************************************************************************/
FSC_BOOL FUSB3601_platform_get_device_irq_state(FSC_U8 port)
{
	if (port == 1)
		return FUSB3601_fusb_GPIO_Get_IntN() ? TRUE : FALSE;
	else {
		hwlog_err("%s - Error: Invalid Port ID!\n", __func__);
		return FALSE;
	}
}

/* Temporary GPIO Src Vbus control */
void FUSB3601_platform_set_vbus_output(FSC_BOOL enable)
{
	struct pd_dpm_vbus_state vbus_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if(!chip) {
		pr_err("FUSB %s - Error:Chip structure is NULL!\n",__func__);
		return;
	}
	if(enable==FALSE) {
		chip->port.pd_notified_ = FALSE;
		if(chip->port.vbus_enabled_ == TRUE) {
			hwlog_info("FUSB Sending VBUS Disable Command\n");
			pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL,NULL);
			hwlog_info("FUSB Returning from VBUS Disable Command\n");
		}
	} else {
		if(chip->port.vbus_enabled_ == FALSE) {
			vbus_state.mv=5000;
			if(chip->port.policy_has_contract_) {
				vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
				vbus_state.ma = chip->port.usb_pd_contract_.FVRDO.OpCurrent * 10;
				chip->port.pd_notified_ = TRUE;
			} else {
				vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
				vbus_state.ma = 900;
			}	
			pr_info("FUSB Sending VBUS Enable Command\n");
			pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS,(void*)&vbus_state);
			pr_info("FUSB Returning from VBUS Enable Command\n");
		} else {
			if(chip->port.policy_has_contract_) {
				if(chip->port.pd_notified_ == FALSE) {
					vbus_state.mv=5000;
					vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
					vbus_state.ma = chip->port.usb_pd_contract_.FVRDO.OpCurrent * 10;
					pr_info("FUSB Sending VBUS Enable Command\n");
					pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS,(void*)&vbus_state);
					pr_info("FUSB Returning from VBUS Enable Command\n");
					chip->port.pd_notified_ = TRUE;
				}
			}
		}
	}
	chip->port.vbus_enabled_ = enable;
	return;	
}

/*******************************************************************************
* Function:        platform_i2c_write
* Input:           SlaveAddress - Slave device bus address
*                  RegisterAddress - Internal register address
*                  DataLength - Length of data to transmit
*                  Data - Buffer of char data to transmit
* Return:          Error state
* Description:     Write a char buffer to the I2C peripheral.
******************************************************************************/
FSC_BOOL FUSB3601_platform_i2c_write(FSC_U8 SlaveAddress,
                            FSC_U8 RegisterAddress,
                            FSC_U8 DataLength,
                            FSC_U8* Data)
{
	FSC_BOOL ret = FALSE;
	if (Data == NULL) {
        pr_err("%s - Error: Write data buffer is NULL!\n", __func__);
        ret = TRUE;
    }
    else if (FUSB3601_fusb_I2C_WriteData(RegisterAddress, DataLength, Data))
    {
        ret = FALSE;
    }
    else  /* I2C Write failure */
    {
        ret = TRUE;
    }
    return ret;
}

/*******************************************************************************
* Function:        platform_i2c_read
* Input:           SlaveAddress - Slave device bus address
*                  RegisterAddress - Internal register address
*                  DataLength - Length of data to attempt to read
*                  Data - Buffer for received char data
* Return:          Error state.
* Description:     Read char data from the I2C peripheral.
******************************************************************************/
FSC_BOOL FUSB3601_platform_i2c_read(FSC_U8 SlaveAddress,
                           FSC_U8 RegisterAddress,
                           FSC_U8 DataLength,
                           FSC_U8* Data)
{
    FSC_BOOL ret = FALSE;
    //FSC_S32 i = 0;
    //FSC_U8 temp = 0;
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return TRUE;
    }

    if (Data == NULL)
    {
        pr_err("%s - Error: Read data buffer is NULL!\n", __func__);
        ret = TRUE;
    }
    else /* if (DataLength > 1 && chip->use_i2c_blocks) */
    {
        /* Do block reads if able and necessary */
        if (!FUSB3601_fusb_I2C_ReadBlockData(RegisterAddress, DataLength, Data))
        {
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    }
    /*
    else
    {
        for (i = 0; i < DataLength; i++)
        {
            if (fusb_I2C_ReadData(RegisterAddress + i, &temp))
            {
                Data[i] = temp;
                ret = FALSE;
            }
            else
            {
                ret = TRUE;
                break;
            }
        }
    }
    */

    return ret;
}

/*****************************************************************************
* Function:        platform_enable_timer
* Input:           enable - TRUE to enable platform timer, FALSE to disable
* Return:          None
* Description:     Enables or disables platform timer
******************************************************************************/
void FUSB3601_platform_enable_timer(FSC_BOOL enable)
{

}
FSC_U32 FUSB3601_get_system_time(void)
{
   unsigned long ms;
   ms = sched_clock() / NSEC_PER_MSEC;
   return (FSC_U32)ms;
}
void FUSB3601_platform_delay(FSC_U32 microseconds)
{
     /*sanity check 1 second*/
     if(microseconds > 1000000)
     	{
     		pr_err(" FUSB %s - Delay of '%u' is too long! Must be less than '%u'.\n",__func__,microseconds,1000000);
     		return;
     	}
     	if(microseconds <=10){
     		udelay(microseconds);
     	}
     	else if(microseconds < 20000){
     		usleep_range(microseconds,microseconds + (microseconds/10));
     	}
     	else{
     		msleep(microseconds / 1000);
     	}
}

FSC_U32 FUSB3601_platform_current_time(void)
{
	return FUSB3601_get_system_time();
}
FSC_U32 FUSB3601_platform_timestamp(void)
{
	FSC_U32 time_ms = FUSB3601_get_system_time();
	FSC_U32 timestamp = time_ms / 1000;
	time_ms -= timestamp*1000;
	timestamp = timestamp << 16;
	timestamp += time_ms*10;
	
	return timestamp;
}
FSC_U32 FUSB3601_platform_get_system_time(void)
{
    return FUSB3601_get_system_time();
}

void FUSB3601_platform_log(FSC_U8 port, const char *str, FSC_S32 value)
{
    pr_err("%s %d\n", str, value);
}

void FUSB3601_platform_set_vconn(FSC_BOOL enable)
{
	pr_err("FUSB %s - Setting VCONN boost to %d\n", __func__, enable);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VCONN, (void *)&enable);
}

