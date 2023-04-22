#include <linux/printk.h>                                                       // pr_err, printk, etc
#include <linux/kthread.h>
#include "fusb30x_global.h"                                                     // Chip structure
#include "platform_helpers.h"                                                   // Implementation details
#include "../core/platform.h"
#include "../core/PD_Types.h"
#include "../core/TypeC_Types.h"
#include "../core/core.h"
#include <huawei_platform/usb/hw_pd_dev.h>
#include <linux/delay.h>
#include "../core/fusb30X.h"
#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif

static FSC_BOOL g_vbus = 0;
static FSC_BOOL g_pd_notified = FALSE;

//open vbus and switch to host
extern FSC_BOOL			PolicyHasContract;
extern doDataObject_t	USBPDContract;
extern USBTypeCCurrent	SourceCurrent;
extern USBTypeCPort            PortType;
struct pd_dpm_vbus_state g_vbus_state;
extern USBTypeCCurrent         SinkCurrent;
extern DeviceReg_t             Registers;

int notify_thread_source_vbus(void* vbus_state)
{
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS, (void *)vbus_state);
    return 0;
}

static void fusb_typec_open_vbus(FSC_BOOL blnEnable)
{
    struct pd_dpm_vbus_state vbus_state;

    if (blnEnable == FALSE) {
        FSC_PRINT("FUSB fusb_typec_open_vbus g_vbus=%d, blnEnable=%d g_pd_notified=%d, PolicyHasContract=%d\n",
            g_vbus,blnEnable,g_pd_notified,PolicyHasContract);
        g_pd_notified = FALSE;
        if (g_vbus == TRUE){
            FSC_PRINT("FUSB %s - Sending VBUS Disable Command\n", __func__);
            pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, NULL);
            }
    }
    else {   // blnEnable == TRUE

        FSC_PRINT("FUSB fusb_typec_open_vbus g_vbus=%d, blnEnable=%d g_pd_notified=%d, PolicyHasContract=%d\n",
            g_vbus,blnEnable,g_pd_notified,PolicyHasContract);
        if (g_vbus == FALSE) {
            vbus_state.mv = 5000;
            if (PolicyHasContract) {
                vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
                vbus_state.ma = USBPDContract.FVRDO.OpCurrent * 10;
                g_pd_notified = TRUE;
            }
            else {
                vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
                vbus_state.ma = 900;
            }
            FSC_PRINT("FUSB %s - Sending VBUS Enable Command\n", __func__);
            pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS, (void *)&vbus_state);
        }
        else { //VBUS rise up already
            if (PolicyHasContract) {
                if (g_pd_notified == FALSE) {
                    vbus_state.mv = 5000;
                    vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
                    vbus_state.ma = USBPDContract.FVRDO.OpCurrent * 10;
                    FSC_PRINT("FUSB %s - Sending VBUS Enable Command\n", __func__);
                    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS, (void *)&vbus_state);
                    g_pd_notified = TRUE;
                }
            }
        }
    }
}

/*******************************************************************************
* Function:        platform_set/get_vbus_lvl_enable
* Input:           VBUS_LVL - requested voltage
*                  Boolean - enable this voltage level
*                  Boolean - turn off other supported voltages
* Return:          Boolean - on or off
* Description:     Provide access to the VBUS control pins.
******************************************************************************/
void platform_set_vbus_lvl_enable(VBUS_LVL level, FSC_BOOL blnEnable, FSC_BOOL blnDisableOthers)
{
    FSC_U32 i;

    // Additional VBUS levels can be added here as needed.
    switch (level)
    {
    case VBUS_LVL_5V:
        // Enable/Disable the 5V Source
		FSC_PRINT("FUSB  %s - %s typeC 5V VBUS (g_vbus = %s) ...\n", __func__, blnEnable ? "enabling" : "disabling", g_vbus ? "TRUE" : "FALSE");
        fusb_typec_open_vbus(blnEnable);
		g_vbus = blnEnable;
        break;
    case VBUS_LVL_12V:
        // Enable/Disable the 12V Source
        break;
    default:
        // Otherwise, do nothing.
        break;
    }

    // Turn off other levels, if requested
    if (blnDisableOthers || ((level == VBUS_LVL_ALL) && (blnEnable == FALSE)))
    {
        i = 0;

        do {
            // Skip the current level
            if( i == level ) continue;

            // Turn off the other level(s)
            platform_set_vbus_lvl_enable( i, FALSE, FALSE );
        } while (++i < VBUS_LVL_COUNT);
    }

    return;
}

FSC_BOOL platform_get_vbus_lvl_enable(VBUS_LVL level)
{
    // Additional VBUS levels can be added here as needed.
    switch (level)
    {
    case VBUS_LVL_5V:
        // Return the state of the 5V VBUS Source.
        return g_vbus;

    case VBUS_LVL_12V:
        // Return the state of the 12V VBUS Source.

    default:
        // Otherwise, return FALSE.
        return FALSE;
    }
}

/*******************************************************************************
* Function:        platform_set_vbus_discharge
* Input:           Boolean
* Return:          None
* Description:     Enable/Disable Vbus Discharge Path
******************************************************************************/
void platform_set_vbus_discharge(FSC_BOOL blnEnable)
{
    // TODO - Implement if required for platform
}

/*******************************************************************************
* Function:        platform_get_device_irq_state
* Input:           None
* Return:          Boolean.  TRUE = Interrupt Active
* Description:     Get the state of the INT_N pin.  INT_N is active low.  This
*                  function handles that by returning TRUE if the pin is
*                  pulled low indicating an active interrupt signal.
******************************************************************************/
FSC_BOOL platform_get_device_irq_state(void)
{
    return fusb_GPIO_Get_IntN() ? TRUE : FALSE;
}

/*******************************************************************************
* Function:        platform_i2c_write
* Input:           SlaveAddress - Slave device bus address
*                  RegAddrLength - Register Address Byte Length
*                  DataLength - Length of data to transmit
*                  PacketSize - Maximum size of each transmitted packet
*                  IncSize - Number of bytes to send before incrementing addr
*                  RegisterAddress - Internal register address
*                  Data - Buffer of char data to transmit
* Return:          Error state
* Description:     Write a char buffer to the I2C peripheral.
******************************************************************************/
FSC_BOOL platform_i2c_write(FSC_U8 SlaveAddress,
                        FSC_U8 RegAddrLength,
                        FSC_U8 DataLength,
                        FSC_U8 PacketSize,
                        FSC_U8 IncSize,
                        FSC_U32 RegisterAddress,
                        FSC_U8* Data)
{
    FSC_BOOL ret = FALSE;
    if (Data == NULL)
    {
        pr_err("%s - Error: Write data buffer is NULL!\n", __func__);
        ret = TRUE;
    }
    else if (fusb_I2C_WriteData((FSC_U8)RegisterAddress, DataLength, Data))
    {
        ret = FALSE;
    }
    else  // I2C Write failure
    {
        ret = TRUE;       // Write data block to the device
    }
    return ret;
}

/*******************************************************************************
* Function:        platform_i2c_read
* Input:           SlaveAddress - Slave device bus address
*                  RegAddrLength - Register Address Byte Length
*                  DataLength - Length of data to attempt to read
*                  PacketSize - Maximum size of each received packet
*                  IncSize - Number of bytes to recv before incrementing addr
*                  RegisterAddress - Internal register address
*                  Data - Buffer for received char data
* Return:          Error state.
* Description:     Read char data from the I2C peripheral.
******************************************************************************/
FSC_BOOL platform_i2c_read(FSC_U8 SlaveAddress,
                       FSC_U8 RegAddrLength,
                       FSC_U8 DataLength,
                       FSC_U8 PacketSize,
                       FSC_U8 IncSize,
                       FSC_U32 RegisterAddress,
                       FSC_U8* Data)
{
    FSC_BOOL ret = FALSE;
    FSC_S32 i = 0;
    FSC_U8 temp = 0;
    struct fusb30x_chip* chip = fusb30x_GetChip();
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
    else if (DataLength > 1 && chip->use_i2c_blocks)    // Do block reads if able and necessary
    {
        if (!fusb_I2C_ReadBlockData(RegisterAddress, DataLength, Data))
        {
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
    }
    else
    {
        for (i = 0; i < DataLength; i++)
        {
            if (fusb_I2C_ReadData((FSC_U8)RegisterAddress + i, &temp))
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

    return ret;
}

/*****************************************************************************
* Function:        platform_enable_timer
* Input:           enable - TRUE to enable platform timer, FALSE to disable
* Return:          None
* Description:     Enables or disables platform timer
******************************************************************************/
void platform_enable_timer(FSC_BOOL enable)
{

}

/*****************************************************************************
* Function:        platform_delay_10us
* Input:           delayCount - Number of 10us delays to wait
* Return:          None
* Description:     Perform a software delay in intervals of 10us.
******************************************************************************/
void platform_delay_10us(FSC_U32 delayCount)
{
    fusb_Delay10us(delayCount);
}

/*******************************************************************************
* Function:        platform_notify_cc_orientation
* Input:           orientation - Orientation of CC (NONE, CC1, CC2)
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current CC orientation. Called in SetStateAttached... and
*                  SetStateUnattached functions.
******************************************************************************/
extern SourceOrSink		sourceOrSink;							// are we currently a source or a sink
extern CCTermType		CCTermPrevious;
extern CCTermType		VCONNTerm;								// vconn line status
void platform_notify_cc_orientation(CC_ORIENTATION orientation)
{
	// Optional: Notify platform of CC orientation
    FSC_U8 cc1State = 0, cc2State = 0;
    struct pd_dpm_typec_state tc_state;
    struct fusb30x_chip* chip;

    memset(&tc_state, 0, sizeof(tc_state));
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

    chip->orientation = orientation;
    chip->sourceOrSink = sourceOrSink;
	/* For Force States */
    if((orientation != NONE) && (PortType != USBTypeC_DRP))
    {
        fusb_StopTimers(&chip->timer_force_timeout);
        PortType = USBTypeC_DRP;
    }

    tc_state.polarity = core_get_cc_orientation();
    FSC_PRINT("FUSB  %s - orientation = %d, sourceOrSink = %s\n", __func__, orientation, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK");
    if (orientation != NONE)
    {
        if (sourceOrSink == SOURCE)
        {
            tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SRC;
        }
        else
        {
            tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SNK;
        }
    }
    else if(orientation == NONE)
    {
        tc_state.new_state = PD_DPM_TYPEC_UNATTACHED;
    }

    // Optional: Notify platform of CC pull-up/pull-down status
    switch (orientation)
    {
        case NONE:
            cc1State = 0;
            cc2State = 0;
            break;
        case CC1:
            cc1State = CCTermPrevious;
            cc2State = VCONNTerm;
            break;
        case CC2:
            cc1State = VCONNTerm;
            cc2State = CCTermPrevious;
            break;
    }
    // cc1State & cc2State = pull-up or pull-down status of each CC line:
    // CCTypeOpen		= 0
    // CCTypeRa		= 1
    // CCTypeRdUSB		= 2
    // CCTypeRd1p5		= 3
    // CCTypeRd3p0		= 4
    // CCTypeUndefined	= 5
    tc_state.cc1_status = cc1State;
    tc_state.cc2_status = cc2State;
    FSC_PRINT("FUSB %s - Entering Orientation Function\n", __func__);
    if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
    }
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
    FSC_PRINT("FUSB %s - Exiting Orientation Function\n", __func__);
}

void platform_notify_debug_accessory_snk(CC_ORIENTATION orientation)
{
    struct pd_dpm_typec_state tc_state;
    struct fusb30x_chip* chip;

    memset(&tc_state, 0, sizeof(tc_state));
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

   chip->orientation = orientation;

    tc_state.polarity = (orientation == CC2) ? TCPC_CTRL_PLUG_ORIENTATION : 0;
    FSC_PRINT("FUSB  %s - orientation = %d, sourceOrSink = %s\n", __func__, orientation, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK");

    tc_state.new_state = PD_DPM_TYPEC_ATTACHED_DBGACC_SNK;

    if (chip->dual_role) {
        dual_role_instance_changed(chip->dual_role);
    }

    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
    FSC_PRINT("FUSB %s - \n", __func__);
}

void platform_notify_audio_accessory(CC_ORIENTATION orientation)
{
	// Optional: Notify platform of CC orientation
    struct pd_dpm_typec_state tc_state;
    struct fusb30x_chip* chip;

    memset(&tc_state, 0, sizeof(tc_state));
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

   chip->orientation = orientation;

    tc_state.polarity = (orientation == CC2) ? TCPC_CTRL_PLUG_ORIENTATION : 0;
    FSC_PRINT("FUSB  %s - orientation = %d, sourceOrSink = %s\n", __func__, orientation, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK");

    tc_state.new_state = PD_DPM_TYPEC_ATTACHED_AUDIO;

    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
    FSC_PRINT("FUSB %s - platform_notify_audio_accessory\n", __func__);
}

/*******************************************************************************
* Function:        platform_notify_pd_contract
* Input:           contract - TRUE: Contract, FALSE: No Contract
*				   PDvoltage - PD contract voltage in 50mV steps
*				   PDcurrent - PD contract current in 10mA steps
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current PD contract status. Called in PDPolicy.
*******************************************************************************/

int notify_thread_sink_vbus(void* vbus_state)
{
    struct pd_dpm_vbus_state *p_vbus_state = (struct pd_dpm_vbus_state *)vbus_state;
    switch (Registers.Status.BC_LVL)            // Determine which level
    {
        case DPM_CC_VOLT_LEVEL_0:                              // If BC_LVL is lowest it's open
            p_vbus_state->remote_rp_level = FUSB_DPM_CC_VOLT_OPEN;
            break;
        case DPM_CC_VOLT_LEVEL_1:                              // If BC_LVL is 1, it's default
            p_vbus_state->remote_rp_level = FUSB_DPM_CC_VOLT_SNK_DFT;
            break;
        case DPM_CC_VOLT_LEVEL_2:                              // If BC_LVL is 2, it's vRd1p5
            p_vbus_state->remote_rp_level = FUSB_DPM_CC_VOLT_SNK_1_5;
            break;
        default:                                // Otherwise it's vRd3p0
            p_vbus_state->remote_rp_level = FUSB_DPM_CC_VOLT_SNK_3_0;
            break;
    }
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, (void*)p_vbus_state);
    return 0;
}
void platform_notify_pd_contract(FSC_BOOL contract, FSC_U32 PDvoltage, FSC_U32 PDcurrent, FSC_U8 externally_powered_bit)
{
    // Optional: Notify platform of PD contract
    FSC_PRINT("FUSB  %s, contract=%d - PD Contract: %dmV/%dmA; g_vbus=%d,sourceOrSink: %s, Externally Powered: %d\n", __func__, contract, PDvoltage * 50, PDcurrent *10, g_vbus, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK", externally_powered_bit);
    if (contract)
    {
        g_vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
        g_vbus_state.mv = PDvoltage * 50;
        g_vbus_state.ma = PDcurrent * 10;
	g_vbus_state.ext_power = externally_powered_bit;
        if (sourceOrSink == SINK)
        {
            kthread_run(notify_thread_sink_vbus, (void*)&g_vbus_state, "notitfy_vbus");
        }
    }
    else {
        if (sourceOrSink == SINK)
        {
            pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, NULL);
        }
    }
    FSC_PRINT("FUSB  %s - PD Contract: %dmV/%dmA; g_vbus=%d,sourceOrSink: %s\n", __func__, PDvoltage * 50, PDcurrent *10, g_vbus, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK");
}

void platform_notify_attached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;
	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_VBUS_ONLY;

	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	FSC_PRINT("FUSB %s - platform_notify_attached_vbus_only\n", __func__);
}

void platform_notify_unattached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;
	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_UNATTACHED_VBUS_ONLY;

	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	FSC_PRINT("FUSB %s - platform_notify_unattached_vbus_only\n", __func__);
}
/*******************************************************************************
* Function:        platform_notify_pd_state
* Input:           state - SOURCE or SINK
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  PD state status when entering SOURCE or SINK. Called in PDPolicy.
*******************************************************************************/
void platform_notify_pd_state(SourceOrSink state)
{
    // Optional: Notify platform of PD SRC or SNK state
    struct pd_dpm_pd_state pd_state;
	pd_state.connected = (state == SOURCE) ? PD_CONNECT_PE_READY_SRC : PD_CONNECT_PE_READY_SNK;
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_PD_STATE, (void *)&pd_state);
	FSC_PRINT("FUSB %s, %s\n", __func__, (state == SOURCE)?"SourceReady":"Sink Ready");
}

/*******************************************************************************
* Function:        platform_notify_unsupported_accessory
* Input:           None
* Return:          None
* Description:     A callback used by the core to report entry to the
*                  Unsupported Accessory state. The platform may implement
*                  USB Billboard.
*******************************************************************************/
void platform_notify_unsupported_accessory(void)
{
    // Optional: Implement USB Billboard
}

/*******************************************************************************
* Function:        platform_notify_data_role
* Input:           PolicyIsDFP - Current data role
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void platform_notify_data_role(FSC_BOOL PolicyIsDFP)
{
	// Optional: Notify platform of data role change
    struct pd_dpm_swap_state swap_state;
	swap_state.new_role = (sourceOrSink == SOURCE) ? PD_ROLE_DFP : PD_ROLE_UFP;
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DR_SWAP, (void *)&swap_state);
}

/*******************************************************************************
* Function:        platform_set_data_role
* Input:           PolicyIsDFP - Current data role
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void platform_set_data_role(FSC_BOOL PolicyIsDFP)
{
    // Optional: Control Data Direction
}

/*******************************************************************************
* Function:        platform_notify_bist
* Input:           bistEnabled - TRUE when BIST enabled, FALSE when disabled
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void platform_notify_bist(FSC_BOOL bistEnabled)
{

}

void platform_set_timer(TIMER *timer, FSC_U16 timeout)
{
    timer->start_time = get_system_time();
    timer->timeout = timeout;
}

FSC_BOOL platform_check_timer(TIMER *timer)
{
    return (((FSC_U16)(get_system_time() - timer->start_time) > timer->timeout) ? TRUE: FALSE);
}

FSC_U16 platform_get_system_time()
{
    return get_system_time();
}

void platform_hard_reset_timer(FSC_U32 timeout)
{
	struct fusb30x_chip* chip;

    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	fusb_StartTimers(&chip->timer_state_machine, timeout);
}

void platform_cancel_hard_reset(void)
{
	struct fusb30x_chip* chip;

    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	fusb_StopTimers(&chip->timer_state_machine);
}

void platform_vbus_timer(FSC_U32 timeout)
{
	struct fusb30x_chip* chip;

    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	fusb_StartTimers(&chip->timer_vbus_timeout, timeout);
}
void platform_start_timer(TIMER *timer, FSC_U32 timeout)
{

    struct fusb30x_chip* chip;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }
    FSC_PRINT("FUSB %s - Starting timer\n", __func__);
    timer->expired = FALSE;
    switch(timer->id)
    {
        case CC_DEBOUNCE:
            fusb_StartTimers(&chip->timer_ccdebounce, timeout);
            break;
        case PD_DEBOUNCE:
            fusb_StartTimers(&chip->timer_pddebounce, timeout);
            break;
        case STATE_TIMER:
            fusb_StartTimers(&chip->timer_statetimer, timeout);
            break;
        case POLICY_STATE_TIMER:
            fusb_StartTimers(&chip->timer_policystatetimer, timeout);
            break;
        case NO_RESPONSE_TIMER:
            fusb_StartTimers(&chip->timer_noresponsetimer, timeout);
            break;
        case LOOP_RESET_TIMER:
            fusb_StartTimers(&chip->timer_loopresettimer, timeout);
            break;
        default:
            break;
	}
}

void platform_stop_timer(TIMER *timer)
{
    struct fusb30x_chip* chip;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }
    timer->expired = FALSE;
    switch(timer->id)
    {
        case CC_DEBOUNCE:
            fusb_StopTimers(&chip->timer_ccdebounce);
            break;
        case PD_DEBOUNCE:
            fusb_StopTimers(&chip->timer_pddebounce);
            break;
        case STATE_TIMER:
            fusb_StopTimers(&chip->timer_statetimer);
            break;
        case POLICY_STATE_TIMER:
            fusb_StopTimers(&chip->timer_policystatetimer);
            break;
        case NO_RESPONSE_TIMER:
            fusb_StopTimers(&chip->timer_noresponsetimer);
            break;
        case LOOP_RESET_TIMER:
            fusb_StopTimers(&chip->timer_loopresettimer);
            break;
        default:
            break;
    }

}

void platform_set_vconn(FSC_BOOL enable)
{
    pr_err("FUSB %s - Setting VCONN boost to %d\n", __func__, enable);
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VCONN, (void *)&enable);
}

FSC_BOOL platform_get_vconn_swap_to_on_supported(void)
{
    struct fusb30x_chip* chip = NULL;
    FSC_BOOL ret = 0;
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }
    ret = chip->vconn_swap_to_on_supported;
    pr_err("%s - Platform call check vconn_swap_to_on_supported: %d\n", __func__, ret);
    return ret;
}

FSC_BOOL platform_get_vconn_swap_to_off_supported(void)
{
    struct fusb30x_chip* chip = NULL;
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }
    return chip->vconn_swap_to_off_supported;
}

FSC_BOOL platform_get_dp_enabled(void)
{
    struct fusb30x_chip* chip = NULL;
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }
    return chip->dp_enabled;
}

FSC_BOOL platform_get_product_type_ama(void)
{
    struct fusb30x_chip* chip = NULL;
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }
    return chip->product_type_ama;
}


FSC_BOOL platform_get_modal_operation_supported(void)
{
    struct fusb30x_chip* chip = NULL;
    chip = fusb30x_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }
    return chip->modal_operation_supported;
}

FSC_BOOL platform_discover_mode_supported(void)
{
    struct fusb30x_chip* chip = NULL;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }

    return chip->discover_mode_supported;
}
FSC_BOOL platform_enter_mode_supported(void)
{
    struct fusb30x_chip* chip = NULL;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }

    return chip->enter_mode_supported;
}
FSC_BOOL platform_discover_svid_supported(void)
{
    struct fusb30x_chip* chip = NULL;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }

    return chip->discover_svid_supported;
}
FSC_U32 platform_sink_pdo_number(void)
{
    struct fusb30x_chip* chip = NULL;

    chip = fusb30x_GetChip();
    if(!chip)
    {
        pr_err("FUSB %s - Error: Chip structure is NULL!\n", __func__);
        return 0;
    }

    return chip->sink_pdo_number;
}

void platform_double_56k_cable(void)
{
    struct pd_dpm_typec_state typec_state;
    memset(&typec_state, 0, sizeof(typec_state));
    typec_state.new_state = PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC;
    FSC_PRINT("FUSB %s Enter\n", __func__);
    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, &typec_state);
}
