#include "../core/callbacks.h"
#include "../core/core.h"
#include "../core/port.h"
#include <linux/printk.h>           /* pr_err, printk, etc */
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include "fusb3601_global.h"        /* Chip structure */
#include "platform_helpers.h"       /* Implementation details */
#include "../core/platform.h"

#include <huawei_platform/usb/hw_pd_dev.h>
#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif

#define TCP_VBUS_CTRL_PD_DETECT (1 << 7)
#define ATTACHED_SINK_COUNTER_THRESHOLD 4
#define ATTACHED_SINK_CHANGE_INTERVAL 500 /*ms*/
#define UNATTACHED_AFTER_ATTACED_SINK_INTERVAL 300 /*ms*/

typedef enum
{
	TCPC_CTRL_PLUG_ORIENTATION = (1 << 0),
	TCPC_CTRL_BIST_TEST_MODE   = (1 << 1)
} tcpc_ctrl_t;

struct pd_dpm_vbus_state get_vbus_state;

void FUSB3601_dump_register(void);
void FUSB3601_fusb_StopTimer(struct hrtimer *timer);
extern void FUSB3601_ConfigurePortType_WithoutUnattach(FSC_U8 Control, struct Port *port);
/*******************************************************************************
* Function:        platform_notify_cc_orientation
* Input:           orientation - Orientation of CC (NONE, CC1, CC2)
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current CC orientation. Called in SetStateAttached... and
*                  SetStateUnattached functions.
******************************************************************************/
void FUSB3601_handle_attached_sink_event(void)
{
	static int change_counter = 0;
	static bool first_enter = true;
	static struct timespec64 ts64_last;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;
	struct timespec64 ts64_sum;
	int change_counter_threshold = ATTACHED_SINK_COUNTER_THRESHOLD;

	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = ATTACHED_SINK_CHANGE_INTERVAL * NSEC_PER_MSEC;
	ts64_now = current_kernel_time64();
	if (first_enter) {
		first_enter = false;
	} else {
		ts64_sum = timespec64_add_safe(ts64_last, ts64_interval);
		if (ts64_sum.tv_sec == TIME_T_MAX) {
			pr_err("%s time overflow happend\n",__func__);
			change_counter = 0;
		} else if (timespec64_compare(&ts64_sum, &ts64_now) >= 0) {
			++change_counter;
			change_counter = change_counter > change_counter_threshold ? change_counter_threshold: change_counter;
			pr_info("%s attached_sink_counter = %d\n",__func__, change_counter);
		} else {
			change_counter = 0;
		}
	}
	if (change_counter >= change_counter_threshold) {
		pr_err("%s change_counter hit\n",__func__);
		superswitch_dsm_report(ERROR_SUPERSWITCH_INT_STORM);
		FUSB3601_dump_register();
	}

	ts64_last =  ts64_now;
}
void FUSB3601_platform_notify_cc_orientation(CCOrientation orientation)
{
	static int first_in = 1;
	FSC_U8 cc1State = 0, cc2State = 0;
	struct pd_dpm_typec_state tc_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;
	static struct timespec64 ts64_last_unattached;
	struct timespec64 ts64_attached_sink;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_sum;
	static int unattached_reported = 0;
	memset(&tc_state, 0, sizeof(tc_state));

	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = UNATTACHED_AFTER_ATTACED_SINK_INTERVAL * NSEC_PER_MSEC;
	if (!chip) {
		pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}
	port = &chip->port;
	if (!port) {
		pr_err("FUSB  %s - Error: port structure is NULL!\n", __func__);
		return;
	}
	chip->orientation = orientation;
	/* For Force States */
	if((orientation != NONE) && (port->port_type_ != USBTypeC_DRP)) {
		FUSB3601_fusb_StopTimer(&chip->timer_force_timeout);
		if (DEVICE_ROLE_DRP == fusb3601_get_device_role()) {
			port->port_type_ = USBTypeC_DRP;
			FUSB3601_ConfigurePortType_WithoutUnattach(FUSB3601_PORT_TYPE_DRP, port);
		}
	}
	tc_state.polarity = orientation;
	pr_info("FUSB  %s - orientation = %d, sourceOrSink = %s\n", __func__, orientation, (chip->port.source_or_sink_ == Source) ? "SOURCE" : "SINK");
	if (orientation != NONE) {
		if (chip->port.source_or_sink_ == Source) {
			tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SRC;
		} else {
			tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SNK;
			FUSB3601_handle_attached_sink_event();
			if (unattached_reported) {
				ts64_attached_sink = current_kernel_time64();
				ts64_sum = timespec64_add_safe(ts64_last_unattached, ts64_interval);
				if (ts64_sum.tv_sec == TIME_T_MAX) {
					pr_err("%s time overflow happend\n",__func__);
				} else if (timespec64_compare(&ts64_sum, &ts64_attached_sink) >= 0) {
					pr_info("%s attach happened after 300ms after unattach\n",__func__);
					if (first_in) {
						first_in = 0;
					} else {
						superswitch_dsm_report(ERROR_SUPERSWITCH_ABNORMAL_PLUG);
						FUSB3601_dump_register();
					}
				} else {
				}
			}
		}
	}else if (orientation == NONE) {
		tc_state.new_state = PD_DPM_TYPEC_UNATTACHED;
		ts64_last_unattached = current_kernel_time64();
		unattached_reported = 1;
	}

	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
	tc_state.cc2_status = chip->port.registers_.CCStat.CC2_STAT;
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
}
void FUSB3601_platform_notify_audio_accessory(void)
{
	struct pd_dpm_typec_state tc_state;

	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_AUDIO;

	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	pr_info("FUSB %s - platform_notify_audio_accessory\n", __func__);
}

void FUSB3601_platform_notify_attached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_VBUS_ONLY;

	chip->orientation = CC1;
	chip->port.source_or_sink_ = Sink;
	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	pr_info("FUSB %s - platform_notify_attached_vbus_only\n", __func__);
}

void FUSB3601_platform_notify_unattached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_UNATTACHED_VBUS_ONLY;

	chip->orientation = NONE;
	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	pr_info("FUSB %s - platform_notify_unattached_vbus_only\n", __func__);
}

void FUSB3601_platform_notify_double_56k(void)
{
	struct pd_dpm_typec_state tc_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC;

	chip->orientation = CC1;
	chip->port.source_or_sink_ = Sink;
	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	pr_info("FUSB %s - platform_notify_custom_source\n", __func__);
}

void FUSB3601_platform_notify_DebugAccessorySink(void)
{
	struct pd_dpm_typec_state tc_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();

	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_DBGACC_SNK;
	chip->orientation = CC1;
	chip->port.source_or_sink_ = Sink;
	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
	pr_info("FUSB %s\n", __func__);
}

/*******************************************************************************
* Function:        platform_notify_pd_contract
* Input:           contract - TRUE: Contract, FALSE: No Contract
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current PD contract status. Called in PDPolicy.
*******************************************************************************/
int FUSB3601_notify_thread_sink_vbus(void* vbus_state)
{
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, (void *)vbus_state);
	return 0;
}
void FUSB3601_platform_notify_pd_contract(FSC_BOOL contract,FSC_U32 pd_voltage, FSC_U32 pd_current, FSC_BOOL externally_powered)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}

	if (contract) {
		get_vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
		get_vbus_state.mv = pd_voltage * 50;
		get_vbus_state.ma = pd_current * 10;
		printk("Enter function: %s, voltage: %d, current: %d\n", __func__, get_vbus_state.mv, (pd_current * 10));   //PengYalong 20170517
		get_vbus_state.ext_power = externally_powered;
		if (chip->port.source_or_sink_ == Sink) {
			kthread_run(FUSB3601_notify_thread_sink_vbus, (void*)&get_vbus_state, "notitfy_vbus");
		}
	} else {
		if (chip->port.source_or_sink_ == Sink)	{
			pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, NULL);
		}
	}
}

/*******************************************************************************
* Function:        platform_set_data_role
* Input:           PolicyIsDFP - Current data role
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void FUSB3601_platform_notify_data_role(FSC_BOOL PolicyIsDFP)
{
	struct pd_dpm_swap_state swap_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}

	swap_state.new_role = (PolicyIsDFP) ? 1 : 0;
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DR_SWAP, (void *)&swap_state);
}
void FUSB3601_platform_notify_dual_role_instance_changed(void)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
		pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}
	if (chip->dual_role) {
		dual_role_instance_changed(chip->dual_role);
	}
}
void FUSB3601_platform_notify_sink_current(struct Port *port) {
	FSC_U8 sink_rp;

	if(!port->policy_has_contract_) {
		get_vbus_state.mv = 5000;
		get_vbus_state.ma = 500;
		get_vbus_state.vbus_type = 0;
		get_vbus_state.ext_power = 0;
		switch(port->snk_current_) {
			case utccDefault:
				sink_rp = 5;
				break;
			case utcc1p5A:
				sink_rp = 6;
				break;
			case utcc3p0A:
				sink_rp = 7;
				break;
			case utccOpen:
			default:
				sink_rp = 0;
				break;
		}

		get_vbus_state.remote_rp_level = sink_rp;
		kthread_run(FUSB3601_notify_thread_sink_vbus, (void*)&get_vbus_state, "notitfy_vbus");
	}
}
