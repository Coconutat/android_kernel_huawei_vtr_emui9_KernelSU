#include "core.h"
#include "TypeC.h"
#include "PDProtocol.h"
#include "PDPolicy.h"
#include "TypeC_Types.h"
#include "PD_Types.h"

#ifdef FSC_DEBUG
#include "version.h"
#endif // FSC_DEBUG

extern FSC_BOOL c2a_cable;
extern FSC_BOOL blnCCPinIsCC1;
extern FSC_BOOL blnCCPinIsCC2;
extern FSC_BOOL         PolicyHasContract;
extern doDataObject_t   USBPDContract;
extern SourceOrSink     sourceOrSink;
extern USBTypeCCurrent  SinkCurrent;
extern FSC_U8           loopCounter;
extern TIMER CCDebounceTimer;
extern TIMER PDDebounceTimer;
extern TIMER StateTimer;
extern TIMER PolicyStateTimer;
extern TIMER NoResponseTimer;

FSC_U8 last_orientation = 0;

/*
 * Call this function to initialize the core.
 */
void core_initialize(void)
{
    InitializeTypeCVariables();                     // Initialize the TypeC variables for the state machine
    InitializePDProtocolVariables();                // Initialize the USB PD variables
    InitializePDPolicyVariables();                  // Initialize the USB PD variables
    InitializeTypeC();
}

/*
 * Call this function to enable or disable the core Type-C State Machine.
 * TRUE  -> enable the core state machine
 * FALSE -> disable the core state machine
 */
void core_enable_typec(FSC_BOOL enable)
{
    if (enable == TRUE) EnableTypeCStateMachine();
    else                DisableTypeCStateMachine();
}

/*
 * Call this function to run the state machines.
 */
void core_state_machine(void)
{
    StateMachineTypeC();
}

/*
 * Call this function every 100us for the core's timers.
 */
void core_tick(void)
{
    TypeCTick();
    PolicyTick();
    ProtocolTick();

#ifdef FSC_DEBUG
    LogTickAt100us();
#endif // FSC_DEBUG
}

#ifdef FSC_DEBUG
/*
 * Call this function to get the lower 8-bits of the core revision number.
 */
FSC_U8 core_get_rev_lower(void)
{
    return FSC_TYPEC_CORE_FW_REV_LOWER;
}

/*
 * Call this function to get the middle 8-bits of the core revision number.
 */
FSC_U8 core_get_rev_middle(void)
{
    return FSC_TYPEC_CORE_FW_REV_MIDDLE;
}

/*
 * Call this function to get the upper 8-bits of the core revision number.
 */
FSC_U8 core_get_rev_upper(void)
{
    return FSC_TYPEC_CORE_FW_REV_UPPER;
}
#endif // FSC_DEBUG

void core_set_vbus_transition_time(FSC_U32 time_ms)
{
    SetVbusTransitionTime(time_ms);
}
FSC_U8 core_get_cc_orientation(void)
{
    if (blnCCPinIsCC1 == TRUE) {
        last_orientation = 0;
        return 0;
    }
    else if (blnCCPinIsCC2 == TRUE) {
        last_orientation = 1;
        return 1;
    }
    else {
        return last_orientation;
    }
}

#ifdef FSC_DEBUG
void core_configure_port_type(FSC_U8 config)
{
    ConfigurePortType(config);
}

void core_enable_pd(FSC_BOOL enable)
{
    if (enable == TRUE) EnableUSBPD();
    else                DisableUSBPD();
}

#ifdef FSC_HAVE_SRC
void core_set_source_caps(FSC_U8* buf)
{
    WriteSourceCapabilities(buf);
}

void core_get_source_caps(FSC_U8* buf)
{
    ReadSourceCapabilities(buf);
}
#endif // FSC_HAVE_SRC

#ifdef FSC_HAVE_SNK
void core_set_sink_caps(FSC_U8* buf)
{
    WriteSinkCapabilities(buf);
}

void core_get_sink_caps(FSC_U8* buf)
{
    ReadSinkCapabilities(buf);
}

void core_set_sink_req(FSC_U8* buf)
{
    WriteSinkRequestSettings(buf);
}

void core_get_sink_req(FSC_U8* buf)
{
    ReadSinkRequestSettings(buf);
}
#endif // FSC_HAVE_SNK

void core_send_hard_reset(void)
{
    SendUSBPDHardReset();
}

void core_process_pd_buffer_read(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessPDBufferRead(InBuffer, OutBuffer);
}

void core_process_typec_pd_status(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessTypeCPDStatus(InBuffer, OutBuffer);
}

void core_process_typec_pd_control(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessTypeCPDControl(InBuffer, OutBuffer);
}

void core_process_local_register_request(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessLocalRegisterRequest(InBuffer, OutBuffer);
}

void core_process_set_typec_state(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessSetTypeCState(InBuffer, OutBuffer);
}

void core_process_read_typec_state_log(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
   ProcessReadTypeCStateLog(InBuffer, OutBuffer);
}

void core_process_read_pd_state_log(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    ProcessReadPDStateLog(InBuffer, OutBuffer);
}

void core_set_alternate_modes(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    // Deprecated
}

void core_set_manual_retries(FSC_U8* InBuffer, FSC_U8* OutBuffer)
{
    setManualRetries(InBuffer[4]);
}

FSC_U8 core_get_alternate_modes(void)
{
   // Deprecated
    return 0;
}

FSC_U8 core_get_manual_retries(void)
{
    return getManualRetries();
}

void core_set_state_unattached(void)
{
    SetStateUnattached();
}
#endif // FSC_DEBUG

FSC_U16 core_get_advertised_current(void)
{
    FSC_U16 current = 0;                                                        // Current advertisement in mA
    if(sourceOrSink == SINK)
    {
        if(PolicyHasContract)                                                   // If there is a PD contract
        {
            current = USBPDContract.FVRDO.OpCurrent * 10;                       // Return contracted current in mA
        }
        else                                                                    // We check Type-C current
        {
            switch(SinkCurrent)
            {
                /* Note for Default: This can be
                 * 500mA for USB 2.0
                 * 900mA for USB 3.1
                 * Up to 1.5A for USB BC 1.2
                 */
                case utccDefault:
                    current = 500;
                    break;
                case utcc1p5A:
                    current = 1500;
                    break;
                case utcc3p0A:
                    current = 3000;
                    break;
                case utccNone:
                default:
                    current = 0;
                    break;
            }
        }
    }
    return current;
}

void core_reset_pd(void)
{
    EnableUSBPD();
    USBPDEnable(TRUE, sourceOrSink);
}

void core_set_drp(void)
{
	ConfigurePortType(0x96);
}
void core_set_try_snk(void)
{
	ConfigurePortType(0xD6);
}
void core_set_try_src(void)
{
	ConfigurePortType(0x9E);
}
void core_set_source(void)
{
	ConfigurePortType(0x95);
}
void core_set_sink(void)
{
	ConfigurePortType(0x90);
}
void core_set_expire(TIMER2 timer)
{
	switch(timer)
	{
		case CC_DEBOUNCE:
			CCDebounceTimer.expired = TRUE;
			break;
		case PD_DEBOUNCE:
			PDDebounceTimer.expired = TRUE;
			break;
		case STATE_TIMER:
			StateTimer.expired = TRUE;
			break;
		case POLICY_STATE_TIMER:
			PolicyStateTimer.expired = TRUE;
			break;
		case NO_RESPONSE_TIMER:
			NoResponseTimer.expired = TRUE;
			break;
		default:
			break;
	}
}
void core_clear_loop_counter(void)
{
   loopCounter = 0;
}

/* Returns TRUE if C to A cable detected, else FALSE */
FSC_BOOL core_is_c2a_cable(void)
{
    return c2a_cable;
}

int core_cc_disable(FSC_BOOL disabled)
{
	if(disabled == FALSE){
		SetStateUnattached();
	} else {
		clearState();
		Registers.Control.TOGGLE = 0;                                   // Disable the toggle state machine
		DeviceWrite(regControl2, 1, &Registers.Control.byte[2]);       // Commit the control state
	}
	return 0;
}
