/****************************************************************************
 * FileName:        PDPolicy.c
 * Processor:       PIC32MX250F128B
 * Compiler:        MPLAB XC32
 * Company:         Fairchild Semiconductor
 *
 * Software License Agreement:
 *
 * The software supplied herewith by Fairchild Semiconductor (the ?Company?)
 * is supplied to you, the Company's customer, for exclusive use with its
 * USB Type C / USB PD products.  The software is owned by the Company and/or
 * its supplier, and is protected under applicable copyright laws.
 * All rights are reserved. Any use in violation of the foregoing restrictions
 * may subject the user to criminal sanctions under applicable laws, as well
 * as to civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN ?AS IS? CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *****************************************************************************/

#ifdef FSC_DEBUG
#include "Log.h"
#endif // FSC_DEBUG

#include <linux/kernel.h>
#include <linux/printk.h>

#include "PD_Types.h"
#include "PDPolicy.h"
#include "PDProtocol.h"
#include "TypeC.h"
#include "fusb30X.h"
#include "vendor_info.h"

#ifdef FSC_HAVE_VDM
#include "vdm/vdm_callbacks.h"
#include "vdm/vdm_callbacks_defs.h"
#include "vdm/vdm.h"
#include "vdm/vdm_types.h"
#include "vdm/bitfield_translators.h"
#include "vdm/DisplayPort/dp_types.h"
#include "vdm/DisplayPort/dp.h"
#include "vdm/DisplayPort/interface_dp.h"
#endif // FSC_HAVE_VDM

/////////////////////////////////////////////////////////////////////////////
//      Variables for use with the USB PD state machine
/////////////////////////////////////////////////////////////////////////////
#ifdef FSC_DEBUG
StateLog                         PDStateLog;                                     // Log for tracking state transitions and times
extern volatile FSC_U16          Timer_S;                                        // Tracks seconds elapsed for log timestamp
extern volatile FSC_U16          Timer_tms;                                      // Tracks tenths of milliseconds elapsed for log timestamp
#endif // FSC_DEBUG

extern PolicyState_t             originalPolicyState;

extern FSC_U8                    manualRetriesEnabled;                                  // Set to 1 to enable manual retries
extern FSC_U8                    nRetries;                                         // Number of tries for manual retry


extern FSC_BOOL                 g_Idle;                                         // Puts state machine into Idle state
extern USBTypeCCurrent          SourceCurrent;                                  // TypeC advertised current
extern FSC_U8                   DetachThreshold;

// Device Policy Manager Variables
FSC_BOOL                        USBPDTxFlag;                                    // Flag to indicate that we need to send a message (set by device policy manager)
FSC_BOOL                        IsHardReset;                                    // Variable indicating that a Hard Reset is occurring
FSC_BOOL                        IsPRSwap;                                       // Variable indicating that a PRSwap is occurring
FSC_BOOL                        IsVCONNSource;                                  // Indicates who is the VCONN source

sopMainHeader_t                 PDTransmitHeader = {0};                         // Definition of the PD packet to send
sopMainHeader_t                 CapsHeaderSink = {0};                           // Definition of the sink capabilities of the device
sopMainHeader_t                 CapsHeaderSource = {0};                         // Definition of the source capabilities of the device
sopMainHeader_t                 CapsHeaderReceived = {0};                       // Last capabilities header received (source or sink)
doDataObject_t                  PDTransmitObjects[7] = {{0}};                   // Data objects to send
doDataObject_t                  CapsSink[7] = {{0}};                            // Power object definitions of the sink capabilities of the device
doDataObject_t                  CapsSource[7] = {{0}};                          // Power object definitions of the source capabilities of the device
doDataObject_t                  CapsReceived[7] = {{0}};                        // Last power objects received (source or sink)
doDataObject_t                  USBPDContract = {0};                            // Current USB PD contract (request object)
doDataObject_t                  SinkRequest = {0};                              // Sink request message
FSC_U32                         SinkRequestMaxVoltage;                          // Maximum voltage that the sink will request
FSC_U32                         SinkRequestMaxPower;                            // Maximum power the sink will request (used to calculate current as well)
FSC_U32                         SinkRequestOpPower;                             // Operating power the sink will request (used to calculate current as well)
FSC_BOOL                        SinkGotoMinCompatible;                          // Whether the sink will respond to the GotoMin command
FSC_BOOL                        SinkUSBSuspendOperation;                        // Whether the sink wants to continue operation during USB suspend
FSC_BOOL                        SinkUSBCommCapable;                             // Whether the sink is USB communications capable

doDataObject_t                  PartnerCaps = {0};                              // Partner's Sink Capabilities

#ifdef FSC_DEBUG
FSC_BOOL                        SourceCapsUpdated;                              // Flag to indicate whether we have updated the source caps (for the GUI)
#endif // FSC_DEBUG

// Policy Variables
// removing static qualifier so PolicyState is visible to other code blocks.
// re-org coming soon!
       PolicyState_t            PolicyState;                                    // State variable for Policy Engine
       PolicyState_t            LastPolicyState;                                // State variable for Policy Engine
       FSC_U8                   PolicySubIndex;                                 // Sub index for policy states
       FSC_BOOL                 PolicyIsSource;                                 // Flag to indicate whether we are acting as a source or a sink
       FSC_BOOL                 PolicyIsDFP;                                    // Flag to indicate whether we are acting as a UFP or DFP
       FSC_BOOL                 PolicyHasContract;                              // Flag to indicate whether there is a contract in place
       FSC_U32                  VbusTransitionTime;                             // Time to wait for VBUS switch to transition
static FSC_U8                   CollisionCounter;                               // Collision counter for the policy engine
static FSC_U8                   HardResetCounter;                               // The number of times a hard reset has been generated
static FSC_U8                   CapsCounter;                                    // Number of capabilities messages sent
TIMER                  PolicyStateTimer;                               // Multi-function timer for the different policy states
TIMER                  NoResponseTimer;                                // Policy engine no response timer
TIMER                  SwapSourceStartTimer;                           // Delay after power role swap before starting source PD
       sopMainHeader_t          PolicyRxHeader = {0};                           // Header object for USB PD messages received
       sopMainHeader_t          PolicyTxHeader = {0};                           // Header object for USB PD messages to send
       SopType                  PolicyTxSop;
       doDataObject_t           PolicyRxDataObj[7] = {{0}};                     // Buffer for data objects received
       doDataObject_t           PolicyTxDataObj[7] = {{0}};                     // Buffer for data objects to send
static FSC_BOOL                 isContractValid;                                // Is PD Contract Valid
doPDO_t                         vendor_info_source[7] = {{0}};
doPDO_t                         vendor_info_sink[7] = {{0}};

#ifdef FSC_HAVE_VDM
// VDM Manager object
extern VdmManager               vdmm;
VdmDiscoveryState_t             AutoVdmState;

FSC_U32                         vdm_msg_length;
SopType                         vdm_sop;
doDataObject_t                  vdm_msg_obj[7] = {{0}};
PolicyState_t                   vdm_next_ps;
FSC_BOOL                        sendingVdmData;

TIMER                VdmTimer;
FSC_BOOL                        VdmTimerStarted;

FSC_U16                         auto_mode_disc_tracker;

extern FSC_BOOL                 mode_entered;
extern SvidInfo                 core_svid_info;
#endif // FSC_HAVE_VDM

#ifdef FSC_HAVE_DP
extern FSC_U32                  DpModeEntered;
extern FSC_S32                  AutoModeEntryObjPos;
#endif // FSC_HAVE_DP

extern FSC_BOOL                 ProtocolCheckRxBeforeTx;
extern FSC_U8                   loopCounter;        // Used to count the number of Unattach<->AttachWait loops
#define PD_FIXED_POWER_VOL_STEP 50
static FSC_U32 	pd_limit_voltage = PD_09_V;

void SetPDLimitVoltage(int vol)
{
	if (vol < PD_ADAPTER_5V || vol > PD_ADAPTER_20V) {
		FSC_PRINT("FUSB %s - Set limit voltage over range\n", __func__);
		return;
	}
	pd_limit_voltage = vol / PD_FIXED_POWER_VOL_STEP;
}
/////////////////////////////////////////////////////////////////////////////
//                  Timer Interrupt service routine
/////////////////////////////////////////////////////////////////////////////
void PolicyTick( void )
{

}

void InitializePDPolicyVariables(void)
{
    int i;
    platform_set_timer(&SwapSourceStartTimer, 0);

#ifdef FSC_HAVE_SNK
    SinkRequestMaxVoltage = 0;                                                // Maximum voltage that the sink will request (9V)
    SinkRequestMaxPower = PD_Power_as_Sink;                                                 // Maximum power the sink will request (0.5W, used to calculate current as well)
    SinkRequestOpPower = PD_Power_as_Sink;                                                  // Operating power the sink will request (0.5W, used to calculate current as well)
    SinkGotoMinCompatible = FALSE;                                              // Whether the sink will respond to the GotoMin command
    SinkUSBSuspendOperation = No_USB_Suspend_May_Be_Set;                                            // Whether the sink wants to continue operation during USB suspend
    SinkUSBCommCapable = USB_Comms_Capable;                                                 // Whether the sink is USB communications capable

    CapsHeaderSink.NumDataObjects = platform_sink_pdo_number();                                          // Set the number of power objects
    CapsHeaderSink.PortDataRole = 0;                                            // Set the data role to UFP by default
    CapsHeaderSink.PortPowerRole = 0;                                           // By default, set the device to be a sink
    CapsHeaderSink.SpecRevision = 1;                                            // Set the spec revision to 2.0
    CapsHeaderSink.Reserved0 = 0;
    CapsHeaderSink.Reserved1 = 0;

    vendor_info_sink[0].Voltage = Snk_PDO_Voltage1;
    vendor_info_sink[0].Current = Snk_PDO_Op_Current1;
    vendor_info_sink[0].MinVoltage = Snk_PDO_Min_Voltage1;
    vendor_info_sink[0].MaxVoltage = Snk_PDO_Max_Voltage1;
    vendor_info_sink[0].MaxPower = Snk_PDO_Op_Power1;
    vendor_info_sink[0].SupplyType = Snk_PDO_Supply_Type1;

    vendor_info_sink[1].Voltage = Snk_PDO_Voltage2;
    vendor_info_sink[1].Current = Snk_PDO_Op_Current2;
    vendor_info_sink[1].MinVoltage = Snk_PDO_Min_Voltage2;
    vendor_info_sink[1].MaxVoltage = Snk_PDO_Max_Voltage2;
    vendor_info_sink[1].MaxPower = Snk_PDO_Op_Power2;
    vendor_info_sink[1].SupplyType = Snk_PDO_Supply_Type2;

    vendor_info_sink[2].Voltage = Snk_PDO_Voltage3;
    vendor_info_sink[2].Current = Snk_PDO_Op_Current3;
    vendor_info_sink[2].MinVoltage = Snk_PDO_Min_Voltage3;
    vendor_info_sink[2].MaxVoltage = Snk_PDO_Max_Voltage3;
    vendor_info_sink[2].MaxPower = Snk_PDO_Op_Power3;
    vendor_info_sink[2].SupplyType = Snk_PDO_Supply_Type3;

    vendor_info_sink[3].Voltage = Snk_PDO_Voltage4;
    vendor_info_sink[3].Current = Snk_PDO_Op_Current4;
    vendor_info_sink[3].MinVoltage = Snk_PDO_Min_Voltage4;
    vendor_info_sink[3].MaxVoltage = Snk_PDO_Max_Voltage4;
    vendor_info_sink[3].MaxPower = Snk_PDO_Op_Power4;
    vendor_info_sink[3].SupplyType = Snk_PDO_Supply_Type4;

    vendor_info_sink[4].Voltage = Snk_PDO_Voltage5;
    vendor_info_sink[4].Current = Snk_PDO_Op_Current5;
    vendor_info_sink[4].MinVoltage = Snk_PDO_Min_Voltage5;
    vendor_info_sink[4].MaxVoltage = Snk_PDO_Max_Voltage5;
    vendor_info_sink[4].MaxPower = Snk_PDO_Op_Power5;
    vendor_info_sink[4].SupplyType = Snk_PDO_Supply_Type5;

    vendor_info_sink[5].Voltage = Snk_PDO_Voltage6;
    vendor_info_sink[5].Current = Snk_PDO_Op_Current6;
    vendor_info_sink[5].MinVoltage = Snk_PDO_Min_Voltage6;
    vendor_info_sink[5].MaxVoltage = Snk_PDO_Max_Voltage6;
    vendor_info_sink[5].MaxPower = Snk_PDO_Op_Power6;
    vendor_info_sink[5].SupplyType = Snk_PDO_Supply_Type6;

    vendor_info_sink[6].Voltage = Snk_PDO_Voltage7;
    vendor_info_sink[6].Current = Snk_PDO_Op_Current7;
    vendor_info_sink[6].MinVoltage = Snk_PDO_Min_Voltage7;
    vendor_info_sink[6].MaxVoltage = Snk_PDO_Max_Voltage7;
    vendor_info_sink[6].MaxPower = Snk_PDO_Op_Power7;
    vendor_info_sink[6].SupplyType = Snk_PDO_Supply_Type7;

    CapsSink[0].FPDOSink.Voltage = Snk_PDO_Voltage1;
    CapsSink[0].FPDOSink.OperationalCurrent = Snk_PDO_Op_Current1;
    CapsSink[0].FPDOSink.DataRoleSwap = DR_Swap_To_DFP_Supported;
    CapsSink[0].FPDOSink.USBCommCapable = USB_Comms_Capable;
    CapsSink[0].FPDOSink.ExternallyPowered = Externally_Powered;
    CapsSink[0].FPDOSink.HigherCapability = Higher_Capability_Set;
    CapsSink[0].FPDOSink.DualRolePower = Accepts_PR_Swap_As_Snk;
    CapsSink[0].FPDOSink.Reserved = 0;

    for (i=1; i<CapsHeaderSink.NumDataObjects; i++)
    {
        switch(vendor_info_sink[i].SupplyType)
        {
            case 0:
                CapsSink[i].FPDOSink.OperationalCurrent = vendor_info_sink[i].Current;
                CapsSink[i].FPDOSink.Voltage = vendor_info_sink[i].Voltage;
                CapsSink[i].FPDOSink.Reserved = 0;
                CapsSink[i].FPDOSink.DataRoleSwap = 0;
                CapsSink[i].FPDOSink.USBCommCapable = 0;
                CapsSink[i].FPDOSink.ExternallyPowered = 0;
                CapsSink[i].FPDOSink.HigherCapability = 0;
                CapsSink[i].FPDOSink.DualRolePower = 0;
                CapsSink[i].FPDOSink.SupplyType = vendor_info_sink[i].SupplyType;
                break;
            case 1:
                CapsSink[i].BPDO.MaxPower = vendor_info_sink[i].MaxPower;
                CapsSink[i].BPDO.MinVoltage = vendor_info_sink[i].MinVoltage;
                CapsSink[i].BPDO.MaxVoltage = vendor_info_sink[i].MaxVoltage;
                CapsSink[i].BPDO.SupplyType = vendor_info_sink[i].SupplyType;
                break;
            case 2:
                CapsSink[i].VPDO.MaxCurrent = vendor_info_sink[i].Current;
                CapsSink[i].BPDO.MinVoltage = vendor_info_sink[i].MinVoltage;
                CapsSink[i].BPDO.MaxVoltage = vendor_info_sink[i].MaxVoltage;
                CapsSink[i].BPDO.SupplyType = vendor_info_sink[i].SupplyType;
                break;
        }
    }
#endif // FSC_HAVE_SNK

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACCMODE))

    CapsHeaderSource.NumDataObjects = Num_Src_PDOs;                                        // Set the number of power objects

    CapsHeaderSource.PortDataRole = 0;                                          // Set the data role to UFP by default
    CapsHeaderSource.PortPowerRole = 1;                                         // By default, set the device to be a source
    CapsHeaderSource.SpecRevision = 1;                                          // Set the spec revision to 2.0
    CapsHeaderSource.Reserved0 = 0;
    CapsHeaderSource.Reserved1 = 0;

    vendor_info_source[0].Voltage = Src_PDO_Voltage1;
    vendor_info_source[0].Current = Src_PDO_Max_Current1;
    vendor_info_source[0].MinVoltage = Src_PDO_Min_Voltage1;
    vendor_info_source[0].MaxVoltage = Src_PDO_Max_Voltage1;
    vendor_info_source[0].MaxPower = Src_PDO_Max_Power1;
    vendor_info_source[0].SupplyType = Src_PDO_Supply_Type1;

    vendor_info_source[1].Voltage = Src_PDO_Voltage2;
    vendor_info_source[1].Current = Src_PDO_Max_Current2;
    vendor_info_source[1].MinVoltage = Src_PDO_Min_Voltage2;
    vendor_info_source[1].MaxVoltage = Src_PDO_Max_Voltage2;
    vendor_info_source[1].MaxPower = Src_PDO_Max_Power2;
    vendor_info_source[1].SupplyType = Src_PDO_Supply_Type2;

    vendor_info_source[2].Voltage = Src_PDO_Voltage3;
    vendor_info_source[2].Current = Src_PDO_Max_Current3;
    vendor_info_source[2].MinVoltage = Src_PDO_Min_Voltage3;
    vendor_info_source[2].MaxVoltage = Src_PDO_Max_Voltage3;
    vendor_info_source[2].MaxPower = Src_PDO_Max_Power3;
    vendor_info_source[2].SupplyType = Src_PDO_Supply_Type3;

    vendor_info_source[3].Voltage = Src_PDO_Voltage4;
    vendor_info_source[3].Current = Src_PDO_Max_Current4;
    vendor_info_source[3].MinVoltage = Src_PDO_Min_Voltage4;
    vendor_info_source[3].MaxVoltage = Src_PDO_Max_Voltage4;
    vendor_info_source[3].MaxPower = Src_PDO_Max_Power4;
    vendor_info_source[3].SupplyType = Src_PDO_Supply_Type4;

    vendor_info_source[4].Voltage = Src_PDO_Voltage5;
    vendor_info_source[4].Current = Src_PDO_Max_Current5;
    vendor_info_source[4].MinVoltage = Src_PDO_Min_Voltage5;
    vendor_info_source[4].MaxVoltage = Src_PDO_Max_Voltage5;
    vendor_info_source[4].MaxPower = Src_PDO_Max_Power5;
    vendor_info_source[4].SupplyType = Src_PDO_Supply_Type5;

    vendor_info_source[5].Voltage = Src_PDO_Voltage6;
    vendor_info_source[5].Current = Src_PDO_Max_Current6;
    vendor_info_source[5].MinVoltage = Src_PDO_Min_Voltage6;
    vendor_info_source[5].MaxVoltage = Src_PDO_Max_Voltage6;
    vendor_info_source[5].MaxPower = Src_PDO_Max_Power6;
    vendor_info_source[5].SupplyType = Src_PDO_Supply_Type6;

    vendor_info_source[6].Voltage = Src_PDO_Voltage7;
    vendor_info_source[6].Current = Src_PDO_Max_Current7;
    vendor_info_source[6].MinVoltage = Src_PDO_Min_Voltage7;
    vendor_info_source[6].MaxVoltage = Src_PDO_Max_Voltage7;
    vendor_info_source[6].MaxPower = Src_PDO_Max_Power7;
    vendor_info_source[6].SupplyType = Src_PDO_Supply_Type7;

    CapsSource[0].FPDOSupply.Voltage = Src_PDO_Voltage1;                                     // Set 5V for the first supply option
    CapsSource[0].FPDOSupply.MaxCurrent = Src_PDO_Max_Current1;                                  // Set 1.5A for the first supply option
    CapsSource[0].FPDOSupply.PeakCurrent = 0;                                   // Set peak equal to max
    CapsSource[0].FPDOSupply.DataRoleSwap = DR_Swap_To_UFP_Supported;                               // By default, don't enable DR_SWAP
    CapsSource[0].FPDOSupply.USBCommCapable = USB_Comms_Capable;                            // By default, USB communications is not allowed
    CapsSource[0].FPDOSupply.ExternallyPowered = Externally_Powered;                          // By default, state that we are externally powered
    CapsSource[0].FPDOSupply.USBSuspendSupport = SWAP(USB_Suspend_May_Be_Cleared);                         // By default, allow  USB Suspend
    CapsSource[0].FPDOSupply.DualRolePower = Accepts_PR_Swap_As_Src;                              // By default, enable PR_SWAP
    CapsSource[0].FPDOSupply.SupplyType = Snk_PDO_Supply_Type1;                                    // Fixed supply
    CapsSource[0].FPDOSupply.Reserved = 0;                                      // Clearing reserved bits

    for (i=1; i<CapsHeaderSource.NumDataObjects; i++)
    {
        switch(vendor_info_source[i].SupplyType)
        {
            case 0:
                CapsSource[i].FPDOSupply.Voltage = vendor_info_source[i].Voltage;
                CapsSource[i].FPDOSupply.MaxCurrent = vendor_info_source[i].Current;
                CapsSource[i].FPDOSupply.PeakCurrent = 0;
                CapsSource[i].FPDOSupply.DataRoleSwap = 0;
                CapsSource[i].FPDOSupply.USBCommCapable = 0;
                CapsSource[i].FPDOSupply.ExternallyPowered = 0;
                CapsSource[i].FPDOSupply.USBSuspendSupport = 0;
                CapsSource[i].FPDOSupply.DualRolePower = 0;
                CapsSource[i].FPDOSupply.SupplyType = vendor_info_source[i].SupplyType;
                break;
            case 1:
                CapsSource[i].BPDO.MaxPower = vendor_info_source[i].MaxPower;
                CapsSource[i].BPDO.MinVoltage = vendor_info_source[i].MinVoltage;
                CapsSource[i].BPDO.MaxVoltage = vendor_info_source[i].MaxVoltage;
                CapsSource[i].BPDO.SupplyType = vendor_info_source[i].SupplyType;
                break;
            case 2:
                CapsSource[i].VPDO.MaxCurrent = vendor_info_source[i].Current;
                CapsSource[i].BPDO.MinVoltage = vendor_info_source[i].MinVoltage;
                CapsSource[i].BPDO.MaxVoltage = vendor_info_source[i].MaxVoltage;
                CapsSource[i].BPDO.SupplyType = vendor_info_source[i].SupplyType;
                break;
        }
    }
#endif // FSC_HAVE_SRC

#ifdef FSC_DEBUG
    SourceCapsUpdated = FALSE;                                                  // Set the flag to indicate to the GUI that our source caps have been updated
#endif // FSC_DEBUG

    VbusTransitionTime = tFPF2498Transition;                                 // Default VBUS transition time 20ms

#ifdef FSC_HAVE_VDM
    InitializeVdmManager();                                                // Initialize VDM Manager
    vdmInitDpm();
    auto_mode_disc_tracker = 0;
#endif // FSC_HAVE_VDM

#ifdef FSC_HAVE_DP
    AutoModeEntryObjPos = -1;
#endif // FSC_HAVE_DP

    ProtocolCheckRxBeforeTx = FALSE;
    isContractValid = FALSE;

    PolicyStateTimer.expired = FALSE;
    PolicyStateTimer.id = POLICY_STATE_TIMER;
    NoResponseTimer.expired = FALSE;
    NoResponseTimer.id = NO_RESPONSE_TIMER;

#ifdef FSC_DEBUG
    InitializeStateLog(&PDStateLog);
#endif // FSC_DEBUG
}

// ##################### USB PD Enable / Disable Routines ################### //

void USBPDEnable(FSC_BOOL DeviceUpdate, SourceOrSink TypeCDFP)
{
    FSC_U8 data[5];
    IsHardReset = FALSE;
    IsPRSwap = FALSE;
    HardResetCounter = 0;

	FSC_PRINT("FUSB %s - PDEnable Start\n", __func__);

    if (USBPDEnabled == TRUE)
    {
        if (blnCCPinIsCC1) {                                                    // If the CC pin is on CC1
            Registers.Switches.TXCC1 = 1;                                    // Enable the BMC transmitter on CC1
            Registers.Switches.MEAS_CC1 = 1;

            Registers.Switches.TXCC2 = 0;                                   // Disable the BMC transmitter on CC2
            Registers.Switches.MEAS_CC2 = 0;
        }
        else if (blnCCPinIsCC2) {                                               // If the CC pin is on CC2
            Registers.Switches.TXCC2 = 1;                                    // Enable the BMC transmitter on CC2
            Registers.Switches.MEAS_CC2 = 1;

            Registers.Switches.TXCC1 = 0;                                   // Disable the BMC transmitter on CC1
            Registers.Switches.MEAS_CC1 = 0;
        }
        if (blnCCPinIsCC1 || blnCCPinIsCC2)                                     // If we know what pin the CC signal is...
        {
            USBPDActive = TRUE;                                                 // Set the active flag
            platform_set_timer(&NoResponseTimer, T_TIMER_DISABLE);                                  // Disable the no response timer by default
            PolicyIsSource = (FSC_BOOL)TypeCDFP;                                          // Set whether we should be initially a source or sink
            PolicyIsDFP = (FSC_BOOL)TypeCDFP;
            IsVCONNSource = (FSC_BOOL)TypeCDFP;
            // Set the initial data port direction
            if (PolicyIsSource)                                                 // If we are a source...
            {
                PolicyState = peSourceStartup;                                  // initialize the policy engine state to source startup
                PolicySubIndex = 0;
                LastPolicyState = peDisabled;
                Registers.Switches.POWERROLE = 1;                               // Initialize to a SRC
                Registers.Switches.DATAROLE = 1;                                // Initialize to a DFP
            }
            else                                                                // Otherwise we are a sink...
            {
                PolicyState = peSinkStartup;                                    // initialize the policy engine state to sink startup
                PolicySubIndex = 0;
                platform_set_timer(&PolicyStateTimer, 0);
                LastPolicyState = peDisabled;
                Registers.Switches.POWERROLE = 0;                               // Initialize to a SNK
                Registers.Switches.DATAROLE = 0;                                // Initialize to a UFP

                Registers.Control.ENSOP1 = 0;
                Registers.Control.ENSOP1DP = 0;
                Registers.Control.ENSOP2 = 0;
                Registers.Control.ENSOP2DB = 0;
            }
            Registers.Switches.AUTO_CRC = 0;                                    // Disable auto CRC until startup
            Registers.Control.AUTO_PRE = 0;                                     // Disable AUTO_PRE since we are going to use AUTO_CRC
            Registers.Control.N_RETRIES = 3;                                    // Set the number of retries to 3


            if(manualRetriesEnabled)
            {
                Registers.Control.N_RETRIES = 0;                                // Set the number of retries to 0
            }


            Registers.Control.AUTO_RETRY = 1;                                   // Enable AUTO_RETRY to use the I_TXSENT interrupt - needed for auto-CRC to work

            data[0] = Registers.Slice.byte;                                     // Set the slice byte (building one transaction)
            data[1] = Registers.Control.byte[0] | 0x40;                         // Set the Control0 byte and set the TX_FLUSH bit (auto-clears)
            data[2] = Registers.Control.byte[1] | 0x04;                         // Set the Control1 byte and set the RX_FLUSH bit (auto-clears)
            data[3] = Registers.Control.byte[2];
            data[4] = Registers.Control.byte[3];
            DeviceWrite(regControl0, 4, &data[1]);
            if (DeviceUpdate)
            {
                DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);      // Commit the switch1 setting
            }
            Registers.Power.PD_RCVR_DIS = 1;
            DeviceWrite(regPower, 1, &Registers.Power.byte);                // Commit the power setting

			ResetProtocolLayer(TRUE);

            platform_start_timer(&PolicyStateTimer, 1); /* Clear SwapSourceStarttimer */

#ifdef FSC_DEBUG
            StoreUSBPDToken(TRUE, pdtAttach);                                   // Store the PD attach token
#endif // FSC_DEBUG
			FSC_PRINT("FUSB %s - PDEnable Complete\n", __func__);
        }

#ifdef FSC_INTERRUPT_TRIGGERED
        g_Idle = FALSE;                                                         // Go into active mode
        platform_enable_timer(TRUE);
#endif  // FSC_INTERRUPT_TRIGGERED

#ifdef FSC_HAVE_VDM
        if(Attempts_Discov_SOP)
        {
            AutoVdmState = AUTO_VDM_INIT;
        }
        else
        {
            AutoVdmState = AUTO_VDM_DONE;
        }
#endif /* FSC_HAVE_VDM */
    }
}

void USBPDDisable(FSC_BOOL DeviceUpdate)
{
	platform_cancel_hard_reset();
	if(Registers.Control.BIST_MODE2 != 0)
	{
		Registers.Control.BIST_MODE2 = 0;                                           // Disable BIST_Carrier_Mode_2 (PD_RESET does not do this)
		DeviceWrite(regControl1, 1, &Registers.Control.byte[1]);
	}
    IsHardReset = FALSE;
    IsPRSwap = FALSE;
#ifdef FSC_DEBUG
    if (USBPDActive == TRUE)                                                    // If we were previously active...
        StoreUSBPDToken(TRUE, pdtDetach);                                       // Store the PD detach token
    SourceCapsUpdated = TRUE;                                                   // Set the source caps updated flag to trigger an update of the GUI
#endif // FSC_DEBUG
    USBPDActive = FALSE;                                                        // Clear the USB PD active flag
    ProtocolState = PRLDisabled;                                                // Set the protocol layer state to disabled
    PolicyState = peDisabled;                                                   // Set the policy engine state to disabled
    PDTxStatus = txIdle;                                                        // Reset the transmitter status
    PolicyIsSource = FALSE;                                                     // Clear the is source flag until we connect again
    PolicyHasContract = FALSE;                                                  // Clear the has contract flag
    DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
    platform_notify_bist(FALSE);
    platform_notify_pd_contract(FALSE, 0, 0, 0);

    if (DeviceUpdate)
    {
        Registers.Switches.TXCC1 = 0;                                           // Disable the BMC transmitter (both CC1 & CC2)
        Registers.Switches.TXCC2 = 0;
        Registers.Switches.AUTO_CRC = 0;                                        // turn off Auto CRC
        DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);             // Commit the switch setting
    }
    Registers.Power.PWR = 0x7;                                         // Disable the internal oscillator for USB PD
    DeviceWrite(regPower, 1, &Registers.Power.byte);                // Commit the power setting
    ProtocolFlushRxFIFO();
    ProtocolFlushTxFIFO();


#ifdef FSC_INTERRUPT_TRIGGERED
        Registers.Mask.M_COLLISION = 1;                                         // Mask PD Interrupts
        DeviceWrite(regMask, 1, &Registers.Mask.byte);
        Registers.MaskAdv.M_RETRYFAIL = 1;
        Registers.MaskAdv.M_TXSENT = 1;
        Registers.MaskAdv.M_HARDRST = 1;
        DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
        Registers.MaskAdv.M_GCRCSENT = 1;
        DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);
#endif // FSC_INTERRUPT_TRIGGERED

	Registers.Status.I_COMP_CHNG = 1;   /* Force check for VBUS */

}



// ##################### USB PD Policy Engine Routines ###################### //

void USBPDPolicyEngine(void)
{
    if (LastPolicyState != PolicyState)                                    // Log Policy State for Debugging
    {
        FSC_PRINT("FUSB %s PolicyState: %d\n", __func__, PolicyState);
		#ifdef FSC_DEBUG
        WriteStateLog(&PDStateLog, PolicyState,
                platform_get_system_time(),
                0);
		#endif // FSC_DEBUG
    }


    LastPolicyState = PolicyState;

    switch (PolicyState)
    {
        case peDisabled:
            break;
        case peErrorRecovery:
            PolicyErrorRecovery();
            break;
        // ###################### Source States  ##################### //
#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACCMODE))
        case peSourceSendHardReset:
            PolicySourceSendHardReset();
            break;
        case peSourceSendSoftReset:
            PolicySourceSendSoftReset();
            break;
        case peSourceSoftReset:
            PolicySourceSoftReset();
            break;
        case peSourceStartup:
            PolicySourceStartup();
            break;
        case peSourceDiscovery:
            PolicySourceDiscovery();
            break;
        case peSourceSendCaps:
            PolicySourceSendCaps();
            break;
        case peSourceDisabled:
            PolicySourceDisabled();
            break;
        case peSourceTransitionDefault:
            PolicySourceTransitionDefault();
            break;
        case peSourceNegotiateCap:
            PolicySourceNegotiateCap();
            break;
        case peSourceCapabilityResponse:
            PolicySourceCapabilityResponse();
            break;
        case peSourceTransitionSupply:
            PolicySourceTransitionSupply();
            break;
        case peSourceReady:
            PolicySourceReady();
            break;
        case peSourceGiveSourceCaps:
            PolicySourceGiveSourceCap();
            break;
        case peSourceGetSinkCaps:
            PolicySourceGetSinkCap();
            break;
        case peSourceSendPing:
            PolicySourceSendPing();
            break;
        case peSourceGotoMin:
            PolicySourceGotoMin();
            break;
        case peSourceGiveSinkCaps:
            PolicySourceGiveSinkCap();
            break;
        case peSourceGetSourceCaps:
            PolicySourceGetSourceCap();
            break;
        case peSourceSendDRSwap:
            PolicySourceSendDRSwap();
            break;
        case peSourceEvaluateDRSwap:
            PolicySourceEvaluateDRSwap();
            break;
        case peSourceSendVCONNSwap:
            PolicySourceSendVCONNSwap();
            break;
        case peSourceSendPRSwap:
            PolicySourceSendPRSwap();
            break;
        case peSourceEvaluatePRSwap:
            PolicySourceEvaluatePRSwap();
            break;
        case peSourceWaitNewCapabilities:
            PolicySourceWaitNewCapabilities();
            break;
        case peSourceEvaluateVCONNSwap:
            PolicySourceEvaluateVCONNSwap();
            break;
#endif // FSC_HAVE_SRC
        // ###################### Sink States  ####################### //
#ifdef FSC_HAVE_SNK
        case peSinkStartup:
            PolicySinkStartup();
            break;
        case peSinkSendHardReset:
            PolicySinkSendHardReset();
            break;
        case peSinkSoftReset:
            PolicySinkSoftReset();
            break;
        case peSinkSendSoftReset:
            PolicySinkSendSoftReset();
            break;
        case peSinkTransitionDefault:
            PolicySinkTransitionDefault();
            break;
        case peSinkDiscovery:
            PolicySinkDiscovery();
            break;
        case peSinkWaitCaps:
            PolicySinkWaitCaps();
            break;
        case peSinkEvaluateCaps:
            PolicySinkEvaluateCaps();
            break;
        case peSinkSelectCapability:
            PolicySinkSelectCapability();
            break;
        case peSinkTransitionSink:
            PolicySinkTransitionSink();
            break;
        case peSinkReady:
            PolicySinkReady();
            break;
        case peSinkGiveSinkCap:
            PolicySinkGiveSinkCap();
            break;
        case peSinkGetSourceCap:
            PolicySinkGetSourceCap();
            break;
        case peSinkGetSinkCap:
            PolicySinkGetSinkCap();
            break;
        case peSinkGiveSourceCap:
            PolicySinkGiveSourceCap();
            break;
        case peSinkSendDRSwap:
            PolicySinkSendDRSwap();
            break;
        case peSinkEvaluateDRSwap:
            PolicySinkEvaluateDRSwap();
            break;
        case peSinkEvaluateVCONNSwap:
            PolicySinkEvaluateVCONNSwap();
            break;
        case peSinkSendPRSwap:
            PolicySinkSendPRSwap();
            break;
        case peSinkEvaluatePRSwap:
            PolicySinkEvaluatePRSwap();
            break;
#endif // FSC_HAVE_SNK

#ifdef FSC_HAVE_VDM
        case peGiveVdm:
            PolicyGiveVdm();
            break;
#endif // FSC_HAVE_VDM

        // ---------- BIST Receive Mode --------------------- //
        case PE_BIST_Receive_Mode:      // Bist Receive Mode
            policyBISTReceiveMode();
            break;
        case PE_BIST_Frame_Received:    // Test Frame received by Protocol layer
            policyBISTFrameReceived();
            break;

        // ---------- BIST Carrier Mode and Eye Pattern ----- //
        case PE_BIST_Carrier_Mode_2:     // BIST Carrier Mode 2
            policyBISTCarrierMode2();
            break;

        case PE_BIST_Test_Data:
            policyBISTTestData();
            break;

        default:
#ifdef FSC_HAVE_VDM
            if ((PolicyState >= FIRST_VDM_STATE) && (PolicyState <= LAST_VDM_STATE) ) {
                // valid VDM state
                PolicyVdm();
            } else
#endif // FSC_HAVE_VDM
            {
                // invalid state, reset
                PolicyInvalidState();
            }
            break;
    }
}

// ############################# Source States  ############################# //

void PolicyErrorRecovery(void)
{
    SetStateErrorRecovery();
}

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACCMODE))
void PolicySourceSendHardReset(void)
{
    FSC_U8 data;
    HardResetCounter++;
    data = Registers.Control.byte[3] | 0x40;                                    // Set the send hard reset bit
    DeviceWrite(regControl3, 1, &data);
}

void PolicySourceSoftReset(void)
{
    PolicySendCommand(CMTAccept, peSourceSendCaps, 0);
}

void PolicySourceSendSoftReset(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            if (PolicySendCommand(CMTSoftReset, peSourceSendSoftReset, 1) == STAT_SUCCESS) // Send the soft reset command to the protocol layer
            {
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Start the sender response timer to wait for an accept message once successfully sent
            }
            break;
        default:
            if (ProtocolMsgRx)                                                  // If we have received a message
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we've handled it here
                if ((PolicyRxHeader.NumDataObjects == 0) && (PolicyRxHeader.MessageType == CMTAccept))  // And it was the Accept...
                {
                    PolicyState = peSourceSendCaps;                             // Go to the send caps state
                }
                else                                                            // Otherwise it was a message that we didn't expect, so...
                    PolicyState = peSourceSendHardReset;                        // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            else if (platform_check_timer(&PolicyStateTimer))                  // If we didn't get a response to our request before timing out...
            {
                PolicyState = peSourceSendHardReset;                            // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
    }
}

void PolicySourceStartup(void)
{
#ifdef FSC_HAVE_VDM
    FSC_S32 i;
#endif // FSC_HAVE_VDM
    CCTermType temp;



    switch (PolicySubIndex)
    {
        case 0:
            // Set masks for PD
            Registers.Mask.M_COLLISION = 0;
            Registers.Mask.M_VBUSOK = 0;
            //Registers.Mask.M_BC_LVL = 1;
            DeviceWrite(regMask, 1, &Registers.Mask.byte);
            Registers.MaskAdv.M_RETRYFAIL = 0;
            Registers.MaskAdv.M_HARDSENT = 0;
            Registers.MaskAdv.M_TXSENT = 0;
            Registers.MaskAdv.M_HARDRST = 0;
            DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
            Registers.MaskAdv.M_GCRCSENT = 0;
            DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);

            Registers.Control.ENSOP1 = 1;
            DeviceWrite(regControl1, 1, &Registers.Control.byte[1]);

            if(Registers.DeviceID.VERSION_ID == VERSION_302A)
            {
                if(Registers.Control.RX_FLUSH == 1)                                         // Disable Rx flushing if it has been enabled
                {
                    Registers.Control.RX_FLUSH = 0;
                }
            }
            else
            {
                if(Registers.Control.BIST_TMODE == 1)
                {
                    Registers.Control.BIST_TMODE = 0;                               // Disable auto-flush RxFIFO
                    DeviceWrite(regControl3, 1, &Registers.Control.byte[3]);
                }
            }

            USBPDContract.object = 0;                                           // Clear the USB PD contract (output power to 5V default)
            PartnerCaps.object = 0;                                             // Clear partner sink caps
            IsPRSwap = FALSE;
            PolicyIsSource = TRUE;                                              // Set the flag to indicate that we are a source (PRSwaps)
            Registers.Switches.POWERROLE = PolicyIsSource;
            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
            ResetProtocolLayer(FALSE);                                           // Reset the protocol layer
			Registers.Switches.AUTO_CRC = 1;                                            // turn on Auto CRC
			DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);	// Reset the protocol layer
			Registers.Power.PWR = 0xF;                                         // Enable the internal oscillator for USB PD
            Registers.Power.PD_RCVR_DIS = 0; /* Enable receiver */
            DeviceWrite(regPower, 1, &Registers.Power.byte);
            CapsCounter = 0;                                                    // Clear the caps counter
            CollisionCounter = 0;                                               // Reset the collision counter
            g_Idle = FALSE;
            //platform_set_timer(&PolicyStateTimer, 400);
            PolicySubIndex++;
            break;
        case 1:
			if ((isVBUSOverVoltage(VBUS_MDAC_4P62) || (ConnState == PoweredAccessory)) && (PolicyStateTimer.expired == TRUE))                    // Wait until we reach vSafe0V and delay if coming from PR Swap
            {
                FSC_PRINT("FUSB %s - PD Source Voltage Check Passed\n", __func__);
                platform_stop_timer(&PolicyStateTimer);
                platform_set_timer(&PolicyStateTimer, 0);                                                       // Reset the policy state timer
                platform_start_timer(&PolicyStateTimer, 30);

	#ifdef FSC_HAVE_VDM
                mode_entered = FALSE;

                auto_mode_disc_tracker = 0;

                core_svid_info.num_svids = 0;
                for (i = 0; i < MAX_NUM_SVIDS; i++) {
                    core_svid_info.svids[i] = 0;
                }
                if(Attempts_Discov_SOP){
                    AutoVdmState = AUTO_VDM_INIT;
                }
                else{
                    AutoVdmState = AUTO_VDM_DONE;
                }
	#endif // FSC_HAVE_VDM

	#ifdef FSC_HAVE_DP
                AutoModeEntryObjPos = -1;

                resetDp();
	#endif // FSC_HAVE_DP
                PolicySubIndex++;
            }

            break;
        case 2:
            if(PolicyStateTimer.expired == TRUE){
                PolicyState = peSourceSendCaps;                                             // Go to the source caps
                PolicySubIndex = 0;
                requestDiscoverIdentity(SOP_TYPE_SOP1);
            }
            break;
        default:
            PolicySubIndex = 0;
            break;
    }
}

void PolicySourceDiscovery(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            platform_start_timer(&PolicyStateTimer, tTypeCSendSourceCap);                             // Initialize the SourceCapabilityTimer
            PolicySubIndex++;                                                   // Increment the sub index
            g_Idle = TRUE;
            break;
        default:
            g_Idle = TRUE;
            if ((HardResetCounter > nHardResetCount) && (platform_check_timer(&NoResponseTimer)) && (PolicyHasContract == TRUE))
            {                                                                   // If we previously had a contract in place...
                g_Idle = FALSE;
                PolicyState = peErrorRecovery;                                  // Go to the error recovery state since something went wrong
                PolicySubIndex = 0;
            }
            else if ((HardResetCounter > nHardResetCount) && (platform_check_timer(&NoResponseTimer)) && (PolicyHasContract == FALSE))
            {                                                                   // Otherwise...
                g_Idle = FALSE;
                PolicyState = peSourceDisabled;                             // Go to the disabled state since we are assuming that there is no PD sink attached
                PolicySubIndex = 0;                                             // Reset the sub index for the next state
            }
            if (PolicyStateTimer.expired == TRUE)                                          // Once the timer expires...
            {
                g_Idle = FALSE;
                if (CapsCounter > nCapsCount)                                   // If we have sent the maximum number of capabilities messages...
                    PolicyState = peSourceDisabled;                             // Go to the disabled state, no PD sink connected
                else                                                            // Otherwise...
                    PolicyState = peSourceSendCaps;                             // Go to the send source caps state to send a source caps message
                PolicySubIndex = 0;                                             // Reset the sub index for the next state
            }
            break;
    }
}

void PolicySourceSendCaps(void)
{
    int temp;
    if ((HardResetCounter > nHardResetCount) && (platform_check_timer(&NoResponseTimer)))         // Check our higher level timeout
    {
        if (PolicyHasContract)                                                  // If USB PD was previously established...
            PolicyState = peErrorRecovery;                                      // Need to go to the error recovery state
        else                                                                    // Otherwise...
            PolicyState = peSourceDisabled;                                     // We are disabling PD and leaving the Type-C connections alone
    }
    else                                                                        // If we haven't timed out and maxed out on hard resets...
    {
        switch (PolicySubIndex)
        {
            case 0:
                if (PolicySendData(DMTSourceCapabilities, CapsHeaderSource.NumDataObjects, &CapsSource[0], peSourceSendCaps, 1, SOP_TYPE_SOP) == STAT_SUCCESS)
                {
                    HardResetCounter = 0;                                       // Clear the hard reset counter
                    CapsCounter = 0;                                            // Clear the caps counter
                    platform_set_timer(&NoResponseTimer, T_TIMER_DISABLE);                          // Stop the no response timer
					platform_hard_reset_timer(tSenderResponse);
                    //platform_set_timer(&PolicyStateTimer, tSenderResponse - tHardResetOverhead);                     // Set the sender response timer
                }
                break;
            default:
                if (ProtocolMsgRx)                                              // If we have received a message
                {
					platform_cancel_hard_reset();
                    ProtocolMsgRx = FALSE;                                      // Reset the message ready flag since we're handling it here
                    if ((PolicyRxHeader.NumDataObjects == 1) && (PolicyRxHeader.MessageType == DMTRequest)) // Was this a valid request message?
					{
                        PolicyState = peSourceNegotiateCap;                     // If so, go to the negotiate capabilities state
						PolicySubIndex = 0;                                         // Reset the sub index
						PDTxStatus = txIdle;                                        // Clear the transmitter status
						PolicySourceNegotiateCap();
						break;
					}
                    else                                                        // Otherwise it was a message that we didn't expect, so...
                        PolicyState = peSourceSendSoftReset;                    // Go onto issuing a soft reset
                    PolicySubIndex = 0;                                         // Reset the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                }
                else if (0)                                     // If we didn't get a response to our request before timing out...
                {
					temp = (int)(platform_get_system_time() - PolicyStateTimer.start_time);
                    ProtocolMsgRx = FALSE;                                      // Reset the message ready flag since we've timed out
                    PolicyState = peSourceSendHardReset;                        // Go to the hard reset state
                    PolicySubIndex = 0;                                         // Reset the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                }
                break;
        }
    }
}

void PolicySourceDisabled(void)
{
    USBPDContract.object = 0;                                                   // Clear the USB PD contract (output power to 5V default)
    // Wait for a hard reset or detach...
#ifdef FSC_INTERRUPT_TRIGGERED
    if(loopCounter == 0)
    {
        g_Idle = TRUE;                                                              // Idle until COMP or HARDRST or GCRCSENT
        platform_enable_timer(FALSE);
    }
#endif  // FSC_INTERRUPT_TRIGGERED
}

void PolicySourceTransitionDefault(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            FSC_PRINT("FUSB %s - Hard Reset Sequence Start\n", __func__);
            platform_vbus_timer(tPSHardReset);
            platform_set_timer(&PolicyStateTimer, tPSHardResetMax + tSafe0V + tSrcRecover);                       // Timeout wait for vSafe0V
            PolicySubIndex++;
            break;
        case 1:
            if(VbusVSafe0V())                                                   // Allow the voltage to drop to 0
            {
                FSC_PRINT("FUSB %s - PD Source Voltage vSafe0V\n", __func__);
                //nothing
                platform_set_vbus_discharge(FALSE);                             // Disable VBUS discharge path
                platform_set_timer(&PolicyStateTimer, tSrcRecover);

                Registers.Power.PD_RCVR_DIS = 1;
                DeviceWrite(regPower, 1, &Registers.Power.byte);                // Commit the power setting
                PolicyHasContract = FALSE;
                DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
                platform_notify_bist(FALSE);
                platform_notify_pd_contract(FALSE, 0, 0, 0);
                if(!PolicyIsDFP)                                                    // Make sure date role is DFP
                {
                    PolicyIsDFP = TRUE;;                                            // Set the current data role
                    Registers.Switches.DATAROLE = PolicyIsDFP;                      // Update the data role
                    DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);      // Commit the data role in the 302
                }
		if(IsVCONNSource)                                                   // Disable VCONN if VCONN Source
		{
                    Registers.Switches.VCONN_CC1 = 0;
                    Registers.Switches.VCONN_CC2 = 0;
                    DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);
                }
                ProtocolFlushTxFIFO();
                ProtocolFlushRxFIFO();

                PolicySubIndex++;
            }
            else if (platform_check_timer(&PolicyStateTimer))                                      // Break out if we never see 0V
            {
                if(PolicyHasContract)
                {
                    PolicyState = peErrorRecovery;
                    PolicySubIndex = 0;
                }
                else
                {
                    PolicySubIndex = 3;
                    platform_set_timer(&PolicyStateTimer, 0);
                }
            }
            break;
        case 2:
            if(platform_check_timer(&PolicyStateTimer))                                           // Wait tSrcRecover to turn VBUS on
            {
				FSC_PRINT("FUSB %s - tSrcRecoverComplete\n", __func__);
                PolicySubIndex++;
            }
            break;
        default:
				FSC_PRINT("FUSB %s - Hard Reset Sequence Ending\n", __func__);
                platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, FALSE);         // Enable the 5V source
                if(blnCCPinIsCC1)
                {
                    Registers.Switches.VCONN_CC2 = 1;
                }
                else
                {
                    Registers.Switches.VCONN_CC1 = 1;
                }
                DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);
                IsVCONNSource = TRUE;
                platform_set_timer(&NoResponseTimer, tNoResponse);                                  // Initialize the no response timer
				platform_start_timer(&PolicyStateTimer, 1); /* Clear SwapSourceStarttimer */
                PolicyState = peSourceStartup;                                  // Go to the startup state
                PolicySubIndex = 0;
            break;
    }
}

void PolicySourceNegotiateCap(void)
{
    // This state evaluates if the sink request can be met or not and sets the next state accordingly
    FSC_BOOL reqAccept = FALSE;                                                     // Set a flag that indicates whether we will accept or reject the request
    FSC_U8 objPosition;                                                          // Get the requested object position
    objPosition = PolicyRxDataObj[0].FVRDO.ObjectPosition;                      // Get the object position reference
    if ((objPosition > 0) && (objPosition <= CapsHeaderSource.NumDataObjects))  // Make sure the requested object number if valid, continue validating request
    {
        if ((PolicyRxDataObj[0].FVRDO.OpCurrent <= CapsSource[objPosition-1].FPDOSupply.MaxCurrent)) // Ensure the default power/current request is available
            reqAccept = TRUE;                                                   // If the source can supply the request, set the flag to respond
    }
    if (reqAccept)                                                              // If we have received a valid request...
    {
        PolicyState = peSourceTransitionSupply;                                 // Go to the transition supply state
		PolicySourceTransitionSupply();
    }
    else                                                                        // Otherwise the request was invalid...
        PolicyState = peSourceCapabilityResponse;                               // Go to the capability response state to send a reject/wait message
}

void PolicySourceTransitionSupply(void)
{
    FSC_U8 sourceVoltage = 0;
    switch (PolicySubIndex)
    {
        case 0:
            PolicySendCommand(CMTAccept, peSourceTransitionSupply, 1);          // Send the Accept message
            break;
        case 1:
            platform_set_timer(&PolicyStateTimer, tSrcTransition);                                  // Initialize the timer to allow for the sink to transition
            PolicySubIndex++;                                                   // Increment to move to the next sub state
            break;
        case 2:
            if (platform_check_timer(&PolicyStateTimer))                                              // If the timer has expired (the sink is ready)...
                PolicySubIndex++;                                               // Increment to move to the next sub state
            break;
        case 3:
            PolicyHasContract = TRUE;                                           // Set the flag to indicate that a contract is in place
            USBPDContract.object = PolicyRxDataObj[0].object;                   // Set the contract to the sink request

            //TODO: More robust selection of PDO
            sourceVoltage = CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage;
            if (sourceVoltage == 100)   // If the chosen object is 5V
            {
                if(platform_get_vbus_lvl_enable(VBUS_LVL_5V))                   // If the supply is already enabled, go to PS_READY
                {
				    platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, FALSE);
                    PolicySubIndex = 5;
                }
                else
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, FALSE);
                    platform_set_timer(&PolicyStateTimer, t5To12VTransition);                       // Set the policy state timer to allow the load switch time to turn off so we don't short our supplies
                    PolicySubIndex++;
                }
            }
#ifdef FM150911A
            else if (sourceVoltage == 240)  // If the chosen object is 12V
            {
                if(platform_get_vbus_lvl_enable(VBUS_LVL_12V))                  // If the supply is already enabled, go to PS_READY
                {
                    PolicySubIndex = 5;
                }
                else
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_12V, TRUE, FALSE);
                    platform_set_timer(&PolicyStateTimer, t5To12VTransition);                       // Set the policy state timer to allow the load switch time to turn off so we don't short our supplies
                    PolicySubIndex++;
                }
            }
#endif // FM150911A
            else                                                                // Default to vSafe5V
            {
                if(platform_get_vbus_lvl_enable(VBUS_LVL_5V))                   // If the supply is already enabled, go to PS_READY
                {
                    PolicySubIndex = 5;
                }
                else
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, FALSE);
                    platform_set_timer(&PolicyStateTimer, t5To12VTransition);                       // Set the policy state timer to allow the load switch time to turn off so we don't short our supplies
                    PolicySubIndex++;
                }
            }
            break;
        case 4:
            // Validate the output is ready prior to sending the ready message (only using a timer for now, could validate using an ADC as well)
            if (platform_check_timer(&PolicyStateTimer))
            {
                if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 100)
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, TRUE);      // Disable the "other" vbus outputs
                }
#ifdef FM150911A
                else if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 240)
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_12V, TRUE, TRUE);     // Disable the "other" vbus outputs
                }
#endif // FM150911A
                else
                {
                    platform_set_vbus_lvl_enable(VBUS_LVL_5V, TRUE, TRUE);      // Disable the "other" vbus outputs
                }

                platform_set_timer(&PolicyStateTimer, tSourceRiseTimeout);                          // Source rise timeout
                PolicySubIndex++;                                               // Increment to move to the next sub state
            }
            break;
        case 5:
            if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 100)
            {
                if((!isVBUSOverVoltage(VBUS_MDAC_5P46) && isVBUSOverVoltage(VBUS_MDAC_4P62)) || (platform_check_timer(&PolicyStateTimer)))   // Check that VBUS is between 4.6 and 5.5 V
                {
                    DetachThreshold = VBUS_MDAC_3P36;   // 5V detach threshold
                    PolicySubIndex++;
                }
            }
#ifdef FM150911A
            else if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 240)
            {
                if((isVBUSOverVoltage(VBUS_MDAC_11P76))  || (platform_check_timer(&PolicyStateTimer)))                           // Check that VBUS is over 11.8V
                {
                    DetachThreshold = VBUS_MDAC_9P66;   // 12V detach threshold
                    PolicySubIndex++;
                }
            }
#endif // FM150911A
            else if(platform_check_timer(&PolicyStateTimer))
            {
                PolicySubIndex++;
            }
            break;
        default:
            if (PolicySendCommand(CMTPS_RDY, peSourceReady, 0) == STAT_SUCCESS)                     // Send the PS_RDY message and move onto the Source Ready state
            {
				Registers.Control.HOST_CUR = 0b10;
				DeviceWrite(regControl0, 1, &Registers.Control.byte[0]);
                platform_notify_pd_contract(TRUE, PartnerCaps.FPDOSupply.Voltage, SinkRequest.FVRDO.MinMaxCurrent, PartnerCaps.FPDOSupply.ExternallyPowered);
            }
            break;
    }
}

void PolicySourceCapabilityResponse(void)
{
    if (PolicyHasContract)                                                      // If we currently have a contract, issue the reject and move back to the ready state
    {
        if(isContractValid)
        {
            PolicySendCommand(CMTReject, peSourceReady, 0);                     // Send the message and continue onto the ready state
        }
        else
        {
            PolicySendCommand(CMTReject, peSourceSendHardReset, 0);                     // Send the message and continue onto the ready state
        }
    }
    else                                                                        // If there is no contract in place, issue a hard reset
    {
        PolicySendCommand(CMTReject, peSourceWaitNewCapabilities, 0);           // Send Reject and continue onto the Source Wait New Capabilities state after success
    }
}

void PolicySourceReady(void)
{

    if (ProtocolMsgRx)                                                          // Have we received a message from the sink?
    {
        ProtocolMsgRx = FALSE;                                                  // Reset the message received flag since we're handling it here
        if (PolicyRxHeader.NumDataObjects == 0)                                 // If we have received a command
        {
            switch (PolicyRxHeader.MessageType)                                 // Determine which command was received
            {
                case CMTGetSourceCap:
                    PolicyState = peSourceGiveSourceCaps;                       // Send out the caps
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTGetSinkCap:                                             // If we receive a get sink capabilities message...
                    PolicyState = peSourceGiveSinkCaps;                         // Go evaluate whether we are going to send sink caps or reject
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTDR_Swap:                                                // If we get a DR_Swap message...
                    PolicyState = peSourceEvaluateDRSwap;                       // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTPR_Swap:
                    PolicyState = peSourceEvaluatePRSwap;                       // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTVCONN_Swap:                                             // If we get a VCONN_Swap message...
                    PolicyState = peSourceEvaluateVCONNSwap;                    // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTSoftReset:
                    PolicyState = peSourceSoftReset;                            // Go to the soft reset state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                default:                                                        // Send a reject message for all other commands
                    break;
            }
        }
        else                                                                    // If we received a data message... for now just send a soft reset
        {
            switch (PolicyRxHeader.MessageType)
            {
                case DMTRequest:
                    PolicyState = peSourceNegotiateCap;                         // If we've received a request object, go to the negotiate capabilities state
                    break;
#ifdef FSC_HAVE_VDM
                case DMTVenderDefined:
                    originalPolicyState = PolicyState;
                    convertAndProcessVdmMessage(ProtocolMsgRxSop);
                    break;
#endif // FSC_HAVE_VDM
                case DMTBIST:
                    processDMTBIST();
                    break;
                default:                                                        // Otherwise we've received a message we don't know how to handle yet
                    break;
            }
            PolicySubIndex = 0;                                                 // Clear the sub index
            PDTxStatus = txIdle;                                                // Clear the transmitter status
        }
    }
    else if (USBPDTxFlag)                                                       // Has the device policy manager requested us to send a message?
    {
        if (PDTransmitHeader.NumDataObjects == 0)
        {
            switch (PDTransmitHeader.MessageType)                               // Determine which command we need to send
            {
                case CMTGetSinkCap:
                    PolicyState = peSourceGetSinkCaps;                          // Go to the get sink caps state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTGetSourceCap:
                    PolicyState = peSourceGetSourceCaps;                        // Go to the get source caps state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTPing:
                    PolicyState = peSourceSendPing;                             // Go to the send ping state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTGotoMin:
                    PolicyState = peSourceGotoMin;                              // Go to the source goto min state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
#ifdef FSC_HAVE_DRP
                case CMTPR_Swap:
                    PolicyState = peSourceSendPRSwap;                       // Issue a PR_Swap message
                    PolicySubIndex = 0;                                     // Clear the sub index
                    PDTxStatus = txIdle;                                    // Clear the transmitter status
                    break;
#endif // FSC_HAVE_DRP
                case CMTDR_Swap:
                    PolicyState = peSourceSendDRSwap;                           // Issue a DR_Swap message
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTVCONN_Swap:
                    PolicyState = peSourceSendVCONNSwap;                        // Issue a VCONN_Swap message
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTSoftReset:
                    PolicyState = peSourceSendSoftReset;                        // Go to the soft reset state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                default:                                                        // Don't send any commands we don't know how to handle yet
                    break;
            }
        }
        else
        {
            switch (PDTransmitHeader.MessageType)
            {
                case DMTSourceCapabilities:
                    PolicyState = peSourceSendCaps;
                    PolicySubIndex = 0;
                    PDTxStatus = txIdle;
                    break;
                case DMTVenderDefined:
                    PolicySubIndex = 0;
#ifdef FSC_HAVE_VDM
                    doVdmCommand();
#endif // FSC_HAVE_VDM
                    break;
                default:
                    break;
            }
        }
        USBPDTxFlag = FALSE;
    }
    else if((UUT_Device_Type != 3) && (Requests_PR_Swap_As_Src))
    {
        PolicyState = peSourceSendPRSwap;                       // Issue a PR_Swap message
        PolicySubIndex = 0;                                     // Clear the sub index
        PDTxStatus = txIdle;                                    // Clear the transmitter status
    }
#ifdef FSC_HAVE_VDM
    else if(PolicyIsDFP && (AutoVdmState != AUTO_VDM_DONE))
    {
        autoVdmDiscovery();
    }
#endif // FSC_HAVE_VDM
    else
    {
#ifdef FSC_INTERRUPT_TRIGGERED
    if(loopCounter == 0)
    {
        g_Idle = TRUE;                                                              // Idle until COMP or HARDRST or GCRCSENT
        platform_enable_timer(FALSE);
    }
#endif  // FSC_INTERRUPT_TRIGGERED
    }

}

void PolicySourceGiveSourceCap(void)
{
    //PolicySendData(DMTSourceCapabilities, CapsHeaderSource.NumDataObjects, &CapsSource[0], peSourceReady, 0, SOP_TYPE_SOP);
    PolicyState = peSourceSendCaps;
    PolicySubIndex = 0;
    PDTxStatus = txIdle;
    PolicySourceSendCaps();
}

void PolicySourceGetSourceCap(void)
{
    PolicySendCommand(CMTGetSourceCap, peSourceReady, 0);
}

void PolicySourceGetSinkCap(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            if (PolicySendCommand(CMTGetSinkCap, peSourceGetSinkCaps, 1) == STAT_SUCCESS) // Send the get sink caps command upon entering state
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Start the sender response timer upon receiving the good CRC message
            break;
        default:
            if (ProtocolMsgRx)                                                  // If we have received a message
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message ready flag since we're handling it here
                if ((PolicyRxHeader.NumDataObjects > 0) && (PolicyRxHeader.MessageType == DMTSinkCapabilities))
                {
                    UpdateCapabilitiesRx(FALSE);
                    PolicyState = peSourceReady;                                // Go onto the source ready state
                }
                else                                                            // If we didn't receive a valid sink capabilities message...
                {
                    PolicyState = peSourceSendHardReset;                        // Go onto issuing a hard reset
                }
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If we didn't get a response to our request before timing out...
            {
                PolicyState = peSourceReady;                            // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
    }
}

void PolicySourceGiveSinkCap(void)
{
#ifdef FSC_HAVE_DRP
    if (PortType == USBTypeC_DRP)
        PolicySendData(DMTSinkCapabilities, CapsHeaderSink.NumDataObjects, &CapsSink[0], peSourceReady, 0, SOP_TYPE_SOP);
    else
#endif // FSC_HAVE_DRP
        PolicySendCommand(CMTReject, peSourceReady, 0);                         // Send the reject message and continue onto the ready state
}

void PolicySourceSendPing(void)
{
    PolicySendCommand(CMTPing, peSourceReady, 0);
}

void PolicySourceGotoMin(void)
{
    if (ProtocolMsgRx)
    {
        ProtocolMsgRx = FALSE;                                                  // Reset the message ready flag since we're handling it here
        if (PolicyRxHeader.NumDataObjects == 0)                                 // If we have received a control message...
        {
            switch(PolicyRxHeader.MessageType)                                  // Determine the message type
            {
                case CMTSoftReset:
                    PolicyState = peSourceSoftReset;                            // Go to the soft reset state if we received a reset command
                    PolicySubIndex = 0;                                         // Reset the sub index
                    PDTxStatus = txIdle;                                        // Reset the transmitter status
                    break;
                default:                                                        // If we receive any other command (including Reject & Wait), just go back to the ready state without changing
                    break;
            }
        }
    }
    else
    {
        switch (PolicySubIndex)
        {
            case 0:
                PolicySendCommand(CMTGotoMin, peSourceGotoMin, 1);                  // Send the GotoMin message
                break;
            case 1:
                platform_set_timer(&PolicyStateTimer, tSrcTransition);                                  // Initialize the timer to allow for the sink to transition
                PolicySubIndex++;                                                   // Increment to move to the next sub state
                break;
            case 2:
                if (platform_check_timer(&PolicyStateTimer))                                              // If the timer has expired (the sink is ready)...
                    PolicySubIndex++;                                               // Increment to move to the next sub state
                break;
            case 3:
                // Adjust the power supply if necessary...
                PolicySubIndex++;                                                   // Increment to move to the next sub state
                break;
            case 4:
                // Validate the output is ready prior to sending the ready message
                PolicySubIndex++;                                                   // Increment to move to the next sub state
                break;
            default:
                PolicySendCommand(CMTPS_RDY, peSourceReady, 0);                     // Send the PS_RDY message and move onto the Source Ready state
                break;
        }
    }
}

void PolicySourceSendDRSwap(void)
{
    FSC_U8 Status;
    switch (PolicySubIndex)
    {
        case 0:
            Status = PolicySendCommandNoReset(CMTDR_Swap, peSourceSendDRSwap, 1);   // Send the DR_Swap message
            if (Status == STAT_SUCCESS)                                         // If we received the good CRC message...
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Initialize for SenderResponseTimer
            else if (Status == STAT_ERROR)                                      // If there was an error...
                PolicyState = peErrorRecovery;                                  // Go directly to the error recovery state
            break;
        default:
            if (ProtocolMsgRx)
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message ready flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a control message...
                {
                    switch(PolicyRxHeader.MessageType)                          // Determine the message type
                    {
                        case CMTAccept:
                            PolicyIsDFP = (PolicyIsDFP == TRUE) ? FALSE : TRUE; // Flip the current data role
                            Registers.Switches.DATAROLE = PolicyIsDFP;          // Update the data role
                            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]); // Commit the data role in the 302 for the auto CRC
                            PolicyState = peSourceReady;                        // Source ready state
                            break;
                        case CMTSoftReset:
                            PolicyState = peSourceSoftReset;                    // Go to the soft reset state if we received a reset command
                            break;
                        default:                                                // If we receive any other command (including Reject & Wait), just go back to the ready state without changing
                            PolicyState = peSourceReady;                        // Go to the source ready state
                            break;
                    }
                }
                else                                                            // Otherwise we received a data message...
                {
                    PolicyState = peSourceReady;                                // Go to the sink ready state if we received a unexpected data message (ignoring message)
                }
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            else if (platform_check_timer(&PolicyStateTimer))                                     // If the sender response timer times out...
            {
                PolicyState = peSourceReady;                                    // Go to the source ready state if the SenderResponseTimer times out
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            break;
    }
}

void PolicySourceEvaluateDRSwap(void)
{
    FSC_U8 Status;

#ifdef FSC_HAVE_VDM
    if (mode_entered == TRUE)                                               // If were are in modal operation, send a hard reset
    {
        PolicyState = peSourceSendHardReset;
        PolicySubIndex = 0;
        return;
    }
#endif // FSC_HAVE_VDM
    if(!DR_Swap_To_UFP_Supported)
    {
        PolicySendCommandNoReset(CMTReject, peSourceReady, 0);
    }
    else
    {
        Status = PolicySendCommandNoReset(CMTAccept, peSourceReady, 0);         // Send the Accept message and wait for the good CRC
        if (Status == STAT_SUCCESS)                                             // If we received the good CRC...
        {
            PolicyIsDFP = (PolicyIsDFP == TRUE) ? FALSE : TRUE;                 // We're not really doing anything except flipping the bit
            Registers.Switches.DATAROLE = PolicyIsDFP;                          // Update the data role
            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);          // Commit the data role in the 302 for the auto CRC
        }
        else if (Status == STAT_ERROR)                                          // If we didn't receive the good CRC...
        {
            PolicyState = peErrorRecovery;                                      // Go to the error recovery state
            PolicySubIndex = 0;                                                 // Clear the sub-index
            PDTxStatus = txIdle;                                                // Clear the transmitter status
        }
    }
}

void PolicySourceSendVCONNSwap(void)
{
    switch(PolicySubIndex)
    {
        case 0:
            if (PolicySendCommand(CMTVCONN_Swap, peSourceSendVCONNSwap, 1) == STAT_SUCCESS) // Send the VCONN_Swap message and wait for the good CRC
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Once we receive the good CRC, set the sender response timer
            break;
        case 1:
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTAccept:                                         // If we get the Accept message...
                            PolicySubIndex++;                                   // Increment the subindex to move onto the next step
                            break;
                        case CMTWait:                                           // If we get either the reject or wait message...
                        case CMTReject:
                            PolicyState = peSourceReady;                        // Go back to the source ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the SenderResponseTimer times out...
            {
                PolicyState = peSourceReady;                                    // Go back to the source ready state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        case 2:
            if (IsVCONNSource)                                                  // If we are currently sourcing VCONN...
            {
                platform_set_timer(&PolicyStateTimer, tVCONNSourceOn);                              // Enable the VCONNOnTimer and wait for a PS_RDY message
                PolicySubIndex++;                                               // Increment the subindex to move to waiting for a PS_RDY message
            }
            else                                                                // Otherwise we need to start sourcing VCONN
            {
                if (blnCCPinIsCC1)                                              // If the CC pin is CC1...
                    Registers.Switches.VCONN_CC2 = 1;                           // Enable VCONN for CC2
                else                                                            // Otherwise the CC pin is CC2
                    Registers.Switches.VCONN_CC1 = 1;                           // so enable VCONN on CC1
                DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);      // Commit the register setting to the device
                IsVCONNSource = TRUE;
                platform_set_timer(&PolicyStateTimer, VbusTransitionTime);                          // Allow time for the FPF2498 to enable...
                PolicySubIndex = 4;                                             // Skip the next state and move onto sending the PS_RDY message after the timer expires            }
            }
            break;
        case 3:
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            Registers.Switches.VCONN_CC1 = 0;                   // Disable the VCONN source
                            Registers.Switches.VCONN_CC2 = 0;                   // Disable the VCONN source
                            DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]); // Commit the register setting to the device
                            IsVCONNSource = FALSE;
                            PolicyState = peSourceReady;                        // Move onto the Sink Ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the VCONNOnTimer times out...
            {
                PolicyState = peSourceSendHardReset;                            // Issue a hard reset
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:
            if (platform_check_timer(&PolicyStateTimer))
            {
                PolicySendCommand(CMTPS_RDY, peSourceReady, 0);                 // Send the Accept message and wait for the good CRC
            }
            break;
    }
}

void PolicySourceSendPRSwap(void)
{
#ifdef FSC_HAVE_DRP
    FSC_U8 Status;
    switch(PolicySubIndex)
    {
        case 0: // Send the PRSwap command
            if (PolicySendCommand(CMTPR_Swap, peSourceSendPRSwap, 1) == STAT_SUCCESS) // Send the PR_Swap message and wait for the good CRC
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Once we receive the good CRC, set the sender response timer
            break;
        case 1:  // Require Accept message to move on or go back to ready state
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTAccept:                                         // If we get the Accept message...
                            IsPRSwap = TRUE;
                            PolicyHasContract = FALSE;
                            DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
                            platform_notify_pd_contract(FALSE, 0, 0, 0);
                            platform_set_timer(&PolicyStateTimer, tSrcTransition);                  // Start the sink transition timer
                            PolicySubIndex++;                                   // Increment the subindex to move onto the next step
                            break;
                        case CMTWait:                                           // If we get either the reject or wait message...
                        case CMTReject:
                            PolicyState = peSourceReady;                        // Go back to the source ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            IsPRSwap = FALSE;
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the SenderResponseTimer times out...
            {
                PolicyState = peSourceReady;                                    // Go back to the source ready state
                PolicySubIndex = 0;                                             // Clear the sub index
                IsPRSwap = FALSE;
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        case 2: // Wait for tSrcTransition and then turn off power (and Rd on/Rp off)
            if (platform_check_timer(&PolicyStateTimer))
            {
                platform_set_vbus_lvl_enable(VBUS_LVL_ALL, FALSE, FALSE);       // Disable VBUS output
                platform_set_vbus_discharge(TRUE);                              // Enabled VBUS discharge path
                platform_set_timer(&PolicyStateTimer, tPSHardResetMax + tSafe0V);                       // Timeout wait for vSafe0V
                PolicySubIndex++;                                               // Increment the sub-index to move onto the next state
            }
            break;
        case 3:
            if (VbusVSafe0V())
            {
                RoleSwapToAttachedSink();
                platform_set_vbus_discharge(FALSE);                             // Disable VBUS discharge path
                PolicyIsSource = FALSE;
                Registers.Switches.POWERROLE = PolicyIsSource;
                DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
                PolicySubIndex++;
            }
            else if (platform_check_timer(&PolicyStateTimer))                                      // Break out if we never see 0V
            {
                PolicyState = peErrorRecovery;
                PolicySubIndex = 0;
            }
            break;
        case 4: // Allow time for the supply to fall and then send the PS_RDY message
                Status = PolicySendCommandNoReset(CMTPS_RDY, peSourceSendPRSwap, 5);
                if (Status == STAT_SUCCESS)                                     // If we successfully sent the PS_RDY command and received the goodCRC
                    platform_set_timer(&PolicyStateTimer, tPSSourceOn);                          // Start the PSSourceOn timer to allow time for the new supply to come up
                else if (Status == STAT_ERROR)
                    PolicyState = peErrorRecovery;                              // If we get an error, go to the error recovery state
            break;
        case 5: // Wait to receive a PS_RDY message from the new DFP
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            PolicySubIndex++;                                 // Clear the sub index
                            platform_set_timer(&PolicyStateTimer, tGoodCRCDelay);                   // Make sure GoodCRC has time to send
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the PSSourceOn times out...
            {
                PolicyState = peErrorRecovery;                                  // Go to the error recovery state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:
            if (platform_check_timer(&PolicyStateTimer))
            {
                PolicyState = peSinkStartup;                        // Go to the sink startup state
                PolicySubIndex = 0;                                 // Clear the sub index
                platform_set_timer(&PolicyStateTimer, 0);                   // Make sure GoodCRC has time to send
            }
            break;
    }
#endif // FSC_HAVE_DRP
}

void PolicySourceEvaluatePRSwap(void)
{
#ifdef FSC_HAVE_DRP
    FSC_U8 Status;
    switch(PolicySubIndex)
    {
        case 0: // Send either the Accept or Reject command
            if (!Accepts_PR_Swap_As_Src || (UUT_Device_Type == 3) )
            {
                PolicySendCommand(CMTReject, peSourceReady, 0);                 // Send the reject if we are not a DRP
            }
            else
            {
                if (PolicySendCommand(CMTAccept, peSourceEvaluatePRSwap, 2) == STAT_SUCCESS) // Send the Accept message and wait for the good CRC
                {
                    IsPRSwap = TRUE;
                    PolicyHasContract = FALSE;
                    DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
                    platform_notify_pd_contract(FALSE, 0, 0, 0);
					platform_vbus_timer(tSrcTransition);
                    RoleSwapToAttachedSink();
                    //platform_set_timer(&PolicyStateTimer, tSrcTransition);
					platform_start_timer(&PolicyStateTimer, tPSHardResetMax + tSafe0V + tSrcTurnOn); // Timeout wait for vSafe0V
					g_Idle = TRUE;
                }
            }
            break;
        case 1: //Skipping this state
            if(platform_check_timer(&PolicyStateTimer))
            {
                platform_set_vbus_lvl_enable(VBUS_LVL_ALL, FALSE, FALSE);       // Disable VBUS output
                platform_set_vbus_discharge(TRUE);                              // Enabled VBUS discharge path
                platform_set_timer(&PolicyStateTimer, tPSHardResetMax + tSafe0V);                       // Timeout wait for vSafe0V
                PolicySubIndex++;
            }
            break;
        case 2:
			g_Idle = TRUE;
            if (VbusVSafe0V())  // Allow time for the supply to fall and then send the PS_RDY message
            {
                platform_set_timer(&PolicyStateTimer, tSrcTransition - 5);                              // Allow some extra time for VBUS to discharge
                PolicyIsSource = FALSE;
                Registers.Switches.POWERROLE = PolicyIsSource;
                DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
				Registers.Measure.MDAC = DetachThreshold;
				DeviceWrite(regMeasure, 1, &Registers.Measure.byte);
                PolicySubIndex++;
				g_Idle = FALSE;
            }
            else if (PolicyStateTimer.expired == TRUE)                                      // Break out if we never see 0V
            {
                PolicyState = peErrorRecovery;
                PolicySubIndex = 0;
				g_Idle = FALSE;
            }
            break;
        case 3:
            if(platform_check_timer(&PolicyStateTimer))
            {
                platform_set_vbus_discharge(FALSE);                             // Disable VBUS discharge path
                PolicySubIndex++;
            }
            break;
        case 4:
            Status = PolicySendCommandNoReset(CMTPS_RDY, peSourceEvaluatePRSwap, 5);    // Send the PS_RDY message
            if (Status == STAT_SUCCESS)                                     // If we successfully sent the PS_RDY command and received the goodCRC
			{
                platform_start_timer(&PolicyStateTimer, tPSSourceOn);                          // Start the PSSourceOn timer to allow time for the new supply to come up
				g_Idle = FALSE;
			}
            else if (Status == STAT_ERROR)
                PolicyState = peErrorRecovery;                              // If we get an error, go to the error recovery state
            break;
        case 5: // Wait to receive a PS_RDY message from the new DFP
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
				g_Idle = FALSE;
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            if(!platform_get_vconn_swap_to_on_supported()) {
                                platform_delay_10us(3000);
                            }
                            PolicySubIndex++;                                 // Increment the sub index
                            IsPRSwap = FALSE;
                            platform_set_timer(&PolicyStateTimer, tGoodCRCDelay);                   // Make sure GoodCRC has time to send
							Registers.Status.I_COMP_CHNG = 1;   /* Force check for VBUS */
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (PolicyStateTimer.expired == TRUE)                                         // If the PSSourceOn times out...
            {
				g_Idle = FALSE;
                IsPRSwap = FALSE;
                PolicyState = peErrorRecovery;                                  // Go to the error recovery state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:
                if (platform_check_timer(&PolicyStateTimer))
                {
                    PolicyState = peSinkStartup;                        // Go to the sink startup state
                    PolicySubIndex = 0;                                 // Clear the sub index
                    platform_set_timer(&PolicyStateTimer, 0);
					PolicySinkStartup();
                }
            break;
    }
#else
    PolicySendCommand(CMTReject, peSourceReady, 0);                             // Send the reject if we are not a DRP
#endif // FSC_HAVE_DRP
}

void PolicySourceWaitNewCapabilities(void)                                      // TODO: DPM integration
{
#ifdef FSC_INTERRUPT_TRIGGERED
    if(loopCounter == 0)
    {
        g_Idle = TRUE;                                                              // Wait for COMP or HARDRST or GCRCSENT
        Registers.Mask.M_COMP_CHNG = 0;
        DeviceWrite(regMask, 1, &Registers.Mask.byte);
        Registers.MaskAdv.M_HARDRST = 0;
        DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
        Registers.MaskAdv.M_GCRCSENT = 0;
        DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);
        platform_enable_timer(FALSE);
    }
#endif // FSC_INTERRUPT_TRIGGERED
    switch(PolicySubIndex)
    {
        case 0:
            // Wait for Policy Manager to change source capabilities
            break;
        default:
            // Transition to peSourceSendCapabilities
            PolicyState = peSourceSendCaps;
            PolicySubIndex = 0;
            break;
    }
}
#endif // FSC_HAVE_SRC

void PolicySourceEvaluateVCONNSwap(void)
{
    switch(PolicySubIndex)
    {
        case 0:
            if(!platform_get_vconn_swap_to_off_supported())
            {
                PolicySendCommand(CMTReject, peSourceReady, 0);
            }
            else
            {
                PolicySendCommand(CMTAccept, peSourceEvaluateVCONNSwap, 1);         // Send the Accept message and wait for the good CRC
            }
            break;
        case 1:
            if (IsVCONNSource)                                                  // If we are currently sourcing VCONN...
            {
                platform_set_timer(&PolicyStateTimer, tVCONNSourceOn);                              // Enable the VCONNOnTimer and wait for a PS_RDY message
                PolicySubIndex++;                                               // Increment the subindex to move to waiting for a PS_RDY message
            }
            else                                                                // Otherwise we need to start sourcing VCONN
            {
                if (blnCCPinIsCC1)                                              // If the CC pin is CC1...
                {
                    Registers.Switches.VCONN_CC2 = 1;                           // Enable VCONN for CC2
                    Registers.Switches.PDWN2 = 0;                               // Disable the pull-down on CC2 to avoid sinking unnecessary current
                }
                else                                                            // Otherwise the CC pin is CC2
                {
                    Registers.Switches.VCONN_CC1 = 1;                           // Enable VCONN for CC1
                    Registers.Switches.PDWN1 = 0;                               // Disable the pull-down on CC1 to avoid sinking unnecessary current
                }
                platform_set_vconn(TRUE);
                DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);      // Commit the register setting to the device
                IsVCONNSource = TRUE;
                platform_set_timer(&PolicyStateTimer, VbusTransitionTime);                          // Allow time for the FPF2498 to enable...
                PolicySubIndex = 3;                                             // Skip the next state and move onto sending the PS_RDY message after the timer expires            }
            }
            break;
        case 2:
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            Registers.Switches.VCONN_CC1 = 0;                   // Disable the VCONN source
                            Registers.Switches.VCONN_CC2 = 0;                   // Disable the VCONN source
                            //Registers.Switches.PDWN1 = 1;                       // Ensure the pull-down on CC1 is enabled
                           // Registers.Switches.PDWN2 = 1;                       // Ensure the pull-down on CC2 is enabled
                            platform_set_vconn(FALSE);
                            DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]); // Commit the register setting to the device
                            IsVCONNSource = FALSE;
                            PolicyState = peSourceReady;                          // Move onto the Sink Ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the VCONNOnTimer times out...
            {
                PolicyState = peSourceSendHardReset;                            // Issue a hard reset
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:
            if (platform_check_timer(&PolicyStateTimer))
            {
                PolicySendCommand(CMTPS_RDY, peSourceReady, 0);                       // Send the Accept message and wait for the good CRC
            }
            break;
    }
}

// ############################## Sink States  ############################## //

#ifdef FSC_HAVE_SNK
void PolicySinkSendHardReset(void)
{
    FSC_U8 data;
    HardResetCounter++;
    IsHardReset = TRUE;
    data = Registers.Control.byte[3] | 0x40;                                    // Set the send hard reset bit
    DeviceWrite(regControl3, 1, &data);                                         // Send the hard reset
}

void PolicySinkSoftReset(void)
{
    if (PolicySendCommand(CMTAccept, peSinkWaitCaps, 0) == STAT_SUCCESS)
        platform_set_timer(&PolicyStateTimer, tSinkWaitCap);
}

void PolicySinkSendSoftReset(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            if (PolicySendCommand(CMTSoftReset, peSinkSendSoftReset, 1) == STAT_SUCCESS)    // Send the soft reset command to the protocol layer
            {
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Start the sender response timer to wait for an accept message once successfully sent
            }
            break;
        default:
            if (ProtocolMsgRx)                                                  // If we have received a message
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we've handled it here
                if ((PolicyRxHeader.NumDataObjects == 0) && (PolicyRxHeader.MessageType == CMTAccept))  // And it was the Accept...
                {
                    PolicyState = peSinkWaitCaps;                               // Go to the wait for capabilities state
                    platform_set_timer(&PolicyStateTimer, tSinkWaitCap);                            // Set the state timer to tSinkWaitCap
                }
                else                                                            // Otherwise it was a message that we didn't expect, so...
                    PolicyState = peSinkSendHardReset;                          // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If we didn't get a response to our request before timing out...
            {
                PolicyState = peSinkSendHardReset;                              // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
    }
}

void PolicySinkTransitionDefault(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            Registers.Power.PWR = 0x7;
            Registers.Power.PD_RCVR_DIS = 1;
            DeviceWrite(regPower, 1, &Registers.Power.byte);                    // Commit the power setting

            HardResetCounter++;
            Registers.Control.BIST_TMODE = 0;                               // Disable auto-flush RxFIFO
            DeviceWrite(regControl3, 1, &Registers.Control.byte[3]);
            ProtocolFlushRxFIFO();
            ProtocolFlushTxFIFO();
            IsHardReset = TRUE;
            PolicyHasContract = FALSE;
            DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
            platform_notify_bist(FALSE);
            platform_notify_pd_contract(FALSE, 0, 0, 0);

            platform_set_timer(&PolicyStateTimer, tPSHardResetMax + tSafe0V);                       // Timeout wait for vSafe0V
            platform_set_timer(&NoResponseTimer, tNoResponse);                                      // Initialize the no response timer
            if(PolicyIsDFP)                                                     // Make sure data role is UFP
            {
                PolicyIsDFP = FALSE;                                            // Set the current data role
                Registers.Switches.DATAROLE = PolicyIsDFP;                      // Update the data role
                DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);      // Commit the data role in the 302
            }
            if(IsVCONNSource)                                                   // Disable VCONN if VCONN Source
            {
                Registers.Switches.VCONN_CC1 = 0;
                Registers.Switches.VCONN_CC2 = 0;
                DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);
                IsVCONNSource = FALSE;
            }
            PolicySubIndex++;
            break;
        case 1:
            if(VbusVSafe0V())
            {
                PolicySubIndex++;
                platform_set_timer(&PolicyStateTimer, tSrcRecoverMax + tSrcTurnOn);                 // Timeout wait for vSafe5V
            }
            else if (platform_check_timer(&PolicyStateTimer))                                      // Break out if we never see 0V
            {
                if(PolicyHasContract)
                {
                    PolicyState = peErrorRecovery;
                    PolicySubIndex = 0;
                }
                else
                {
                    PolicyState = peSinkStartup;
                    PolicySubIndex = 0;
                    platform_set_timer(&PolicyStateTimer, 0);
                }
            }
            break;
        case 2:
            if(isVBUSOverVoltage(VBUS_MDAC_4P20))
            {
                PolicySubIndex++;
            }
            else if (Registers.Status.BC_LVL == 0)
            {
                PolicyState = peSinkStartup;//
                PolicySubIndex = 0;
            }
            else if (platform_check_timer(&PolicyStateTimer))                                      // Break out if we never see 0V
            {
                if(PolicyHasContract)
                {
                    PolicyState = peErrorRecovery;
                    PolicySubIndex = 0;
                }
                else
                {
                    PolicyState = peSinkStartup;
                    PolicySubIndex = 0;
                    platform_set_timer(&PolicyStateTimer, 0);
                }
            }
            break;
        default:
            PolicyState = peSinkStartup;                                                // Go to the startup state
            PolicySubIndex = 0;                                                         // Clear the sub index
            platform_set_timer(&PolicyStateTimer, 0);
            PDTxStatus = txIdle;                                                        // Reset the transmitter status
            break;
    }
}

void PolicySinkStartup(void)
{
#ifdef FSC_HAVE_VDM
    FSC_S32 i;
#endif // FSC_HAVE_VDM
    IsHardReset = FALSE;
    Registers.Status.I_COMP_CHNG = 1;   /* Force check for VBUS */

    Registers.Mask.M_COLLISION = 0;
    DeviceWrite(regMask, 1, &Registers.Mask.byte);
    Registers.MaskAdv.M_RETRYFAIL = 0;
    Registers.MaskAdv.M_HARDSENT = 0;
    Registers.MaskAdv.M_TXSENT = 0;
    Registers.MaskAdv.M_HARDRST = 0;
    DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
    Registers.MaskAdv.M_GCRCSENT = 0;
    DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);

    if(Registers.DeviceID.VERSION_ID == VERSION_302A)
    {
        if(Registers.Control.RX_FLUSH == 1)                                         // Disable Rx flushing if it has been enabled
        {
            Registers.Control.RX_FLUSH = 0;
        }
    }
    else
    {
        if(Registers.Control.BIST_TMODE == 1)
        {
            Registers.Control.BIST_TMODE = 0;                               // Disable auto-flush RxFIFO
            DeviceWrite(regControl3, 1, &Registers.Control.byte[3]);
        }
    }

    USBPDContract.object = 0;                                           // Clear the USB PD contract (output power to 5V default)
    PartnerCaps.object = 0;                                         // Clear partner sink caps
    IsPRSwap = FALSE;
    PolicyIsSource = FALSE;                                                     // Clear the flag to indicate that we are a sink (for PRSwaps)
    Registers.Switches.POWERROLE = PolicyIsSource;
    DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
    ResetProtocolLayer(FALSE);
    Registers.Switches.AUTO_CRC = 1;                                            // turn on Auto CRC
    DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);	// Reset the protocol layer
    Registers.Power.PWR = 0xF;                                         // Enable the internal oscillator for USB PD
    Registers.Power.PD_RCVR_DIS = 0; /* Enable receiver */
    DeviceWrite(regPower, 1, &Registers.Power.byte);

    CapsCounter = 0;                                                            // Clear the caps counter
    CollisionCounter = 0;                                                       // Reset the collision counter
    platform_set_timer(&PolicyStateTimer, 0);                                                       // Reset the policy state timer
    PolicyState = peSinkDiscovery;                                              // Go to the sink discovery state
    PolicySubIndex = 0;                                                         // Reset the sub index

#ifdef FSC_HAVE_VDM
    auto_mode_disc_tracker = 0;

    mode_entered = FALSE;

    core_svid_info.num_svids = 0;
    for (i = 0; i < MAX_NUM_SVIDS; i++) {
        core_svid_info.svids[i] = 0;
    }
#endif // FSC_HAVE_VDM

#ifdef FSC_HAVE_DP
    AutoModeEntryObjPos = -1;

    resetDp();
#endif // FSC_HAVE_DP
    PolicySinkDiscovery();
}

void PolicySinkDiscovery(void)
{
    PolicyState = peSinkWaitCaps;
    PolicySubIndex = 0;
    if (HardResetCounter <= nHardResetCount)
	platform_hard_reset_timer(tTypeCSinkWaitCap);
}

void PolicySinkWaitCaps(void)
{
    if (ProtocolMsgRx)                                                          // If we have received a message...
    {
        platform_cancel_hard_reset();
        ProtocolMsgRx = FALSE;                                                  // Reset the message ready flag since we're handling it here
        if ((PolicyRxHeader.NumDataObjects > 0) && (PolicyRxHeader.MessageType == DMTSourceCapabilities)) // Have we received a valid source cap message?
        {
            UpdateCapabilitiesRx(TRUE);                                         // Update the received capabilities
            PolicyState = peSinkEvaluateCaps;                                   // Set the evaluate source capabilities state
            PolicySubIndex = 0;
            PolicySinkEvaluateCaps();
        }
        else if ((PolicyRxHeader.NumDataObjects == 0) && (PolicyRxHeader.MessageType == CMTSoftReset))
        {
            PolicyState = peSinkSoftReset;                                      // Go to the soft reset state
            PolicySubIndex = 0;
        }
                                                             // Reset the sub index
    }
    else if ((PolicyHasContract == TRUE) && (platform_check_timer(&NoResponseTimer)) && (HardResetCounter > nHardResetCount))
    {
        platform_cancel_hard_reset();
        PolicyState = peErrorRecovery;
        PolicySubIndex = 0;
    }
    else if (0 && (platform_check_timer(&PolicyStateTimer)) && (HardResetCounter <= nHardResetCount))
    {
        PolicyState = peSinkSendHardReset;
        PolicySubIndex = 0;
    }
    else if ((PolicyHasContract == FALSE) && (platform_check_timer(&NoResponseTimer)) && (HardResetCounter > nHardResetCount))
    {
        platform_cancel_hard_reset();
#ifdef FSC_INTERRUPT_TRIGGERED
        g_Idle = TRUE;                                                          // Wait for VBUSOK or HARDRST or GCRCSENT
        platform_enable_timer(FALSE);
#endif // FSC_INTERRUPT_TRIGGERED
    }
    else
    {
        platform_delay_10us(100);
        DeviceRead(regStatus0, 2, &Registers.Status.byte[4]);
        if((Registers.Status.ACTIVITY == 0) && !Registers.Status.RX_EMPTY)
        {
            FSC_PRINT("FUSB %s - Triggering SinkWaitCap Backup Read\n", __func__);
            ProtocolGetRxPacket();
        }
    }
}

void PolicySinkEvaluateCaps(void)
{
    // Due to latency with the PC and evaluating capabilities, we are always going to select the first one by default (5V default)
    // This will allow the software time to determine if they want to select one of the other capabilities (user selectable)
    // If we want to automatically show the selection of a different capabilities message, we need to build in the functionality here
    // The evaluate caps
    FSC_S32 i, reqPos;
    FSC_U32 objVoltage = 0;
    FSC_U32 optional_max_power = 0;
    FSC_U32 objCurrent, objPower, MaxPower, SelVoltage, ReqCurrent;
    objCurrent = 0;
    platform_set_timer(&NoResponseTimer, T_TIMER_DISABLE);                                          // Stop the no response timer
    HardResetCounter = 0;                                                       // Reset the hard reset counter                                                        // Indicate Hard Reset is over
    SelVoltage = 0;
    MaxPower = 0;
    reqPos = 0;                                                                 // Select nothing in case there is an error...

    /* Calculate max voltage - algorithm in case of non-compliant ordering */
    for(i = 0; i < CapsHeaderSink.NumDataObjects; i++)
    {
        SinkRequestMaxVoltage =
                ((CapsSink[i].FPDOSink.Voltage > SinkRequestMaxVoltage)
                ? CapsSink[i].FPDOSink.Voltage : SinkRequestMaxVoltage);
    }

    for (i=0; i<CapsHeaderReceived.NumDataObjects; i++)                         // Going to select the highest power object that we are compatible with
    {
        switch (CapsReceived[i].PDO.SupplyType)
        {
            case pdoTypeFixed:
                objVoltage = CapsReceived[i].FPDOSupply.Voltage;                // Get the output voltage of the fixed supply
                if (objVoltage > SinkRequestMaxVoltage)                         // If the voltage is greater than our limit...
                    continue;                                                   // Set the power to zero to ignore the object
                else                                                            // Otherwise...
                {
                    objCurrent = CapsReceived[i].FPDOSupply.MaxCurrent;
                    objPower = objVoltage * objCurrent;                         // Calculate the power for comparison
                }
                break;
            case pdoTypeVariable:
                objVoltage = CapsReceived[i].VPDO.MaxVoltage;                   // Grab the maximum voltage of the variable supply
                if (objVoltage > SinkRequestMaxVoltage)                         // If the max voltage is greater than our limit...
                    continue;                                               // Set the power to zero to ignore the object
                else                                                            // Otherwise...
                {
                    objVoltage = CapsReceived[i].VPDO.MinVoltage;               // Get the minimum output voltage of the variable supply
                    objCurrent = CapsReceived[i].VPDO.MaxCurrent;               // Get the maximum output current of the variable supply
                    objPower = objVoltage * objCurrent;                         // Calculate the power for comparison (based on min V/max I)
                }
                break;
            case pdoTypeBattery:                                                // We are going to ignore battery powered sources for now
            default:                                                            // We are also ignoring undefined supply types
                objPower = 0;                                                   // Set the object power to zero so we ignore for now
                break;
        }

        if (objPower > optional_max_power) {
            optional_max_power = objPower;
        }

        if (objVoltage > pd_limit_voltage)
                continue;

        if (objPower >= MaxPower)                                               // If the current object has power greater than or equal the previous objects
        {
            MaxPower = objPower;                                                // Store the objects power
            SelVoltage = objVoltage;                                            // Store the objects voltage (used for calculations)
            reqPos = i + 1;                                                     // Store the position of the object
        }
    }
    if ((reqPos > 0) && (SelVoltage > 0))
    {
        PartnerCaps.object = CapsReceived[0].object;
        SinkRequest.FVRDO.ObjectPosition = reqPos & 0x07;                       // Set the object position selected
        SinkRequest.FVRDO.GiveBack = SinkGotoMinCompatible;                     // Set whether we will respond to the GotoMin message
        SinkRequest.FVRDO.NoUSBSuspend = SinkUSBSuspendOperation;               // Set whether we want to continue pulling power during USB Suspend
        SinkRequest.FVRDO.USBCommCapable = SinkUSBCommCapable;                  // Set whether USB communications is active
        ReqCurrent = SinkRequestOpPower / SelVoltage;                           // Calculate the requested operating current
        SinkRequest.FVRDO.OpCurrent = (ReqCurrent & 0x3FF);                     // Set the current based on the selected voltage (in 10mA units)
        ReqCurrent = SinkRequestMaxPower / SelVoltage;                          // Calculate the requested maximum current
        SinkRequest.FVRDO.MinMaxCurrent = (ReqCurrent & 0x3FF);                 // Set the min/max current based on the selected voltage (in 10mA units)
        if (SinkGotoMinCompatible)                                              // If the give back flag is set...
            SinkRequest.FVRDO.CapabilityMismatch = FALSE;                       // There can't be a capabilities mismatch
        else                                                                    // Otherwise...
        {
            if (objCurrent < ReqCurrent)                                        // If the max power available is less than the max power requested...
            {
                SinkRequest.FVRDO.CapabilityMismatch = TRUE;                    // flag the source that we need more power
                SinkRequest.FVRDO.MinMaxCurrent = objCurrent;
                SinkRequest.FVRDO.OpCurrent =  objCurrent;
            }
            else                                                                // Otherwise...
            {
                SinkRequest.FVRDO.CapabilityMismatch = FALSE;                   // there is no mismatch in the capabilities
            }
        }

        if (optional_max_power >= PD_18_W)
        {
            pd_dpm_set_optional_max_power_status(true);
        } else {
            pd_dpm_set_optional_max_power_status(false);
        }

        PolicyState = peSinkSelectCapability;                                   // Go to the select capability state
        PolicySubIndex = 0;                                                     // Reset the sub index
        platform_set_timer(&PolicyStateTimer, tSenderResponse);                                     // Initialize the sender response timer
		PolicySinkSelectCapability();
    }
    else
    {
        // For now, we are just going to go back to the wait state instead of sending a reject or reset (may change in future)
        PolicyState = peSinkWaitCaps;                                           // Go to the wait for capabilities state
        platform_set_timer(&PolicyStateTimer, tTypeCSinkWaitCap);                                        // Set the state timer to tSinkWaitCap
    }
}

void PolicySinkSelectCapability(void)
{
    switch (PolicySubIndex)
    {
        case 0:
            FSC_PRINT("FUSB %s - PolicySendData PDTxStatus: %d\n", __func__, PDTxStatus);
            if (PolicySendData(DMTRequest, 1, &SinkRequest, peSinkSelectCapability, 1, SOP_TYPE_SOP) == STAT_SUCCESS)
            {
                platform_hard_reset_timer(tSenderResponse);
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                                   // If there is a good CRC start retry timer
            }
            break;
       case 1:
            if (ProtocolMsgRx)
            {
                platform_cancel_hard_reset();
                ProtocolMsgRx = FALSE;                                          // Reset the message ready flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a control message...
                {
                    switch(PolicyRxHeader.MessageType)                          // Determine the message type
                    {
                        case CMTAccept:
                            PolicyHasContract = TRUE;                           // Set the flag to indicate that a contract is in place
                            USBPDContract.object = SinkRequest.object;          // Set the actual contract that is in place
                            platform_set_timer(&PolicyStateTimer, tPSTransition);                   // Set the transition timer here
                            PolicyState = peSinkTransitionSink;                 // Go to the transition state if the source accepted the request
                            break;
                        case CMTWait:
                        case CMTReject:
                            if(PolicyHasContract)
                            {
                                PolicyState = peSinkReady;                      // Go to the sink ready state if the source rejects or has us wait
                            }
                            else
                            {
                                PolicyState = peSinkWaitCaps;                   // If we didn't have a contract, go wait for new source caps
                                HardResetCounter = nHardResetCount + 1;         // Make sure we don't send hard reset to prevent infinite loop
                            }
                            break;
                        case CMTSoftReset:
                            PolicyState = peSinkSoftReset;                      // Go to the soft reset state if we received a reset command
                            break;
                        default:
                            PolicyState = peSinkSendSoftReset;                  // We are going to send a reset message for all other commands received
                            break;
                    }
                }
                else                                                            // Otherwise we received a data message...
                {
                    switch (PolicyRxHeader.MessageType)
                    {
                        case DMTSourceCapabilities:                             // If we received a new source capabilities message
                            UpdateCapabilitiesRx(TRUE);                         // Update the received capabilities
                            PolicyState = peSinkEvaluateCaps;                   // Go to the evaluate caps state
                            break;
                        default:
                            PolicyState = peSinkSendSoftReset;                  // Send a soft reset to get back to a known state
                            break;
                    }
                }
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            else if (0)                                     // If the sender response timer times out...
            {
                PolicyState = peSinkSendHardReset;                              // Go to the hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            break;
    }
}

void PolicySinkTransitionSink(void)
{
    if (ProtocolMsgRx)
    {
        ProtocolMsgRx = FALSE;                                                  // Reset the message ready flag since we're handling it here
        if (PolicyRxHeader.NumDataObjects == 0)                                 // If we have received a control message...
        {
            switch(PolicyRxHeader.MessageType)                                  // Determine the message type
            {
                case CMTPS_RDY:
                    PolicyState = peSinkReady;                                  // Go to the ready state
                    platform_notify_pd_contract(TRUE, CapsReceived[SinkRequest.FVRDO.ObjectPosition-1].FPDOSupply.Voltage, SinkRequest.FVRDO.OpCurrent, PartnerCaps.FPDOSupply.ExternallyPowered);
                    break;
                case CMTSoftReset:
                    PolicyState = peSinkSoftReset;                              // Go to the soft reset state if we received a reset command
                    break;
                default:
                    PolicyState = peSinkSendHardReset;                          // We are going to send a reset message for all other commands received
                    break;
            }
			platform_delay_10us(100);
			DeviceRead(regStatus0, 2, &Registers.Status.byte[4]);
			if((Registers.Status.ACTIVITY == 0) && !Registers.Status.RX_EMPTY)
			{
				FSC_PRINT("FUSB %s - Triggering SinkTransitionSink Backup Read\n", __func__);
				ProtocolGetRxPacket();
			}
        }
        else                                                                    // Otherwise we received a data message...
        {
            switch (PolicyRxHeader.MessageType)                                 // Determine the message type
            {
                case DMTSourceCapabilities:                                     // If we received new source capabilities...
                    UpdateCapabilitiesRx(TRUE);                                 // Update the source capabilities
                    PolicyState = peSinkEvaluateCaps;                           // And go to the evaluate capabilities state
                    break;
                default:                                                        // If we receieved an unexpected data message...
                    PolicyState = peSinkSendHardReset;
                    break;
            }
        }
        PolicySubIndex = 0;                                                     // Reset the sub index
        PDTxStatus = txIdle;                                                    // Reset the transmitter status
    }
    else if (platform_check_timer(&PolicyStateTimer))                                             // If the PSTransitionTimer times out...
    {
        PolicyState = peSinkSendHardReset;                                      // Issue a hard reset
        PolicySubIndex = 0;                                                     // Reset the sub index
        PDTxStatus = txIdle;                                                    // Reset the transmitter status
    }
}

void PolicySinkReady(void)
{

    if (ProtocolMsgRx)                                                          // Have we received a message from the source?
    {
        ProtocolMsgRx = FALSE;                                                  // Reset the message received flag since we're handling it here
        if (PolicyRxHeader.NumDataObjects == 0)                                 // If we have received a command
        {
            switch (PolicyRxHeader.MessageType)                                 // Determine which command was received
            {
                case CMTGotoMin:
                    PolicyState = peSinkTransitionSink;                         // Go to transitioning the sink power
                    platform_set_timer(&PolicyStateTimer, tPSTransition);                           // Set the transition timer here
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTGetSinkCap:
                    PolicyState = peSinkGiveSinkCap;                            // Go to sending the sink caps state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    PolicySinkGiveSinkCap();
                    break;
                case CMTGetSourceCap:
                    PolicyState = peSinkGiveSourceCap;                          // Go to sending the source caps if we are dual-role
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTDR_Swap:                                                // If we get a DR_Swap message...
                    PolicyState = peSinkEvaluateDRSwap;                         // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTPR_Swap:
                    PolicyState = peSinkEvaluatePRSwap;                         // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTVCONN_Swap:                                             // If we get a VCONN_Swap message...
                    PolicyState = peSinkEvaluateVCONNSwap;                      // Go evaluate whether we are going to accept or reject the swap
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTSoftReset:
                    PolicyState = peSinkSoftReset;                              // Go to the soft reset state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                default:                                                        // For all other commands received, simply ignore them
                    PolicySubIndex = 0;                                         // Reset the sub index
                    PDTxStatus = txIdle;                                        // Reset the transmitter status
                    break;
            }
        }
        else
        {
            switch (PolicyRxHeader.MessageType)
            {
                case DMTSourceCapabilities:
                    UpdateCapabilitiesRx(TRUE);                                 // Update the received capabilities
                    PolicyState = peSinkEvaluateCaps;                           // Go to the evaluate capabilities state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
#ifdef FSC_HAVE_VDM
                case DMTVenderDefined:
                    convertAndProcessVdmMessage(ProtocolMsgRxSop);
                    break;
#endif // FSC_HAVE_VDM
                case DMTBIST:
                    processDMTBIST();
                    break;
                default:                                                        // If we get something we are not expecting... simply ignore them
                    PolicySubIndex = 0;                                                     // Reset the sub index
                    PDTxStatus = txIdle;                                                    // Reset the transmitter status
                    break;
            }
        }
    }
    else if (USBPDTxFlag)                                                       // Has the device policy manager requested us to send a message?
    {
        if (PDTransmitHeader.NumDataObjects == 0)
        {
            switch (PDTransmitHeader.MessageType)
            {
                case CMTGetSourceCap:
                    PolicyState = peSinkGetSourceCap;                           // Go to retrieve the source caps state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                   break;
                case CMTGetSinkCap:
                    PolicyState = peSinkGetSinkCap;                             // Go to retrieve the sink caps state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                case CMTDR_Swap:
                    PolicyState = peSinkSendDRSwap;                         // Issue a DR_Swap message
                    PolicySubIndex = 0;                                     // Clear the sub index
                    PDTxStatus = txIdle;                                    // Clear the transmitter status
                    break;
#ifdef FSC_HAVE_DRP
                case CMTPR_Swap:
                    PolicyState = peSinkSendPRSwap;                         // Issue a PR_Swap message
                    PolicySubIndex = 0;                                     // Clear the sub index
                    PDTxStatus = txIdle;                                    // Clear the transmitter status
                    break;
#endif // FSC_HAVE_DRP
                case CMTSoftReset:
                    PolicyState = peSinkSendSoftReset;                          // Go to the send soft reset state
                    PolicySubIndex = 0;                                         // Clear the sub index
                    PDTxStatus = txIdle;                                        // Clear the transmitter status
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch (PDTransmitHeader.MessageType)
            {
                case DMTRequest:
                    SinkRequest.object = PDTransmitObjects[0].object;           // Set the actual object to request
                    PolicyState = peSinkSelectCapability;                       // Go to the select capability state
                    PolicySubIndex = 0;                                         // Reset the sub index
                    platform_set_timer(&PolicyStateTimer, tSenderResponse);                         // Initialize the sender response timer
                    break;
                case DMTVenderDefined:
#ifdef FSC_HAVE_VDM
                    doVdmCommand();
#endif // FSC_HAVE_VDM
                    break;
                default:
                    break;
            }
        }
        USBPDTxFlag = FALSE;
    }
    else if((UUT_Device_Type != 0) && (Requests_PR_Swap_As_Snk) && (PartnerCaps.FPDOSupply.DualRolePower == TRUE))
    {
        PolicyState = peSinkSendPRSwap;                       // Issue a PR_Swap message
        PolicySubIndex = 0;                                     // Clear the sub index
        PDTxStatus = txIdle;                                    // Clear the transmitter status
    }
    else
    {
#ifdef FSC_INTERRUPT_TRIGGERED
        g_Idle = TRUE;                                                          // Wait for VBUSOK or HARDRST or GCRCSENT
		FSC_PRINT("FUSB %s, 0E=%2x, 0F=%2x\n", __func__, Registers.MaskAdv.byte[0], Registers.MaskAdv.byte[1]);
        platform_enable_timer(FALSE);
#endif  // FSC_INTERRUPT_TRIGGERED
    }
}

void PolicySinkGiveSinkCap(void)
{

    if(PolicySendData(DMTSinkCapabilities, CapsHeaderSink.NumDataObjects, &CapsSink[0], peSinkReady, 0, SOP_TYPE_SOP) == STAT_ERROR)
	{

	}
}

void PolicySinkGetSinkCap(void)
{
    PolicySendCommand(CMTGetSinkCap, peSinkReady, 0);
}

void PolicySinkGiveSourceCap(void)
{
#ifdef FSC_HAVE_DRP
    if (PortType == USBTypeC_DRP)
        PolicySendData(DMTSourceCapabilities, CapsHeaderSource.NumDataObjects, &CapsSource[0], peSinkReady, 0, SOP_TYPE_SOP);
    else
#endif // FSC_HAVE_DRP
        PolicySendCommand(CMTReject, peSinkReady, 0);                           // Send the reject message and continue onto the ready state
}

void PolicySinkGetSourceCap(void)
{
    PolicySendCommand(CMTGetSourceCap, peSinkReady, 0);
}

void PolicySinkSendDRSwap(void)
{
    FSC_U8 Status;
    switch (PolicySubIndex)
    {
        case 0:
            Status = PolicySendCommandNoReset(CMTDR_Swap, peSinkSendDRSwap, 1); // Send the DR_Swap command
            if (Status == STAT_SUCCESS)                                         // If we received a good CRC message...
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Initialize for SenderResponseTimer if we received the GoodCRC
            else if (Status == STAT_ERROR)                                      // If there was an error...
                PolicyState = peErrorRecovery;                                  // Go directly to the error recovery state
            break;
        default:
            if (ProtocolMsgRx)
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message ready flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a control message...
                {
                    switch(PolicyRxHeader.MessageType)                          // Determine the message type
                    {
                        case CMTAccept:
                            PolicyIsDFP = (PolicyIsDFP == TRUE) ? FALSE : TRUE; // Flip the current data role

                            Registers.Switches.DATAROLE = PolicyIsDFP;          // Update the data role
                            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]); // Commit the data role in the 302 for the auto CRC
                            PolicyState = peSinkReady;                          // Sink ready state
                            break;
                        case CMTSoftReset:
                            PolicyState = peSinkSoftReset;                      // Go to the soft reset state if we received a reset command
                            break;
                        default:                                                // If we receive any other command (including Reject & Wait), just go back to the ready state without changing
                            PolicyState = peSinkReady;                          // Go to the sink ready state
                            break;
                    }
                }
                else                                                            // Otherwise we received a data message...
                {
                    PolicyState = peSinkReady;                                  // Go to the sink ready state if we received a unexpected data message (ignoring message)
                }
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            else if (platform_check_timer(&PolicyStateTimer))                                     // If the sender response timer times out...
            {
                PolicyState = peSinkReady;                                      // Go to the sink ready state if the SenderResponseTimer times out
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txIdle;                                            // Reset the transmitter status
            }
            break;
    }
}

void PolicySinkEvaluateDRSwap(void)
{
    FSC_U8 Status;

#ifdef FSC_HAVE_VDM
    if (mode_entered == TRUE)                                              // If were are in modal operation, send a hard reset
    {
        PolicyState = peSinkSendHardReset;
        PolicySubIndex = 0;
        return;
    }
#endif // FSC_HAVE_VDM

    if(!DR_Swap_To_DFP_Supported)
    {
        PolicySendCommandNoReset(CMTReject, peSinkReady, 0);
    }
    else
    {
        Status = PolicySendCommandNoReset(CMTAccept, peSinkReady, 0);           // Send the Accept message and wait for the good CRC
        if (Status == STAT_SUCCESS)                                             // If we received the good CRC...
        {
            PolicyIsDFP = (PolicyIsDFP == TRUE) ? FALSE : TRUE;                 // We're not really doing anything except flipping the bit
            Registers.Switches.DATAROLE = PolicyIsDFP;                          // Update the data role
            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);         // Commit the data role in the 302 for the auto CRC
        }
        else if (Status == STAT_ERROR)                                          // If we didn't receive the good CRC...
        {
            PolicyState = peErrorRecovery;                                      // Go to the error recovery state
            PolicySubIndex = 0;                                                 // Clear the sub-index
            PDTxStatus = txIdle;                                                // Clear the transmitter status
        }
    }
}

void PolicySinkEvaluateVCONNSwap(void)
{
    switch(PolicySubIndex)
    {
        case 0:
            if(!platform_get_vconn_swap_to_on_supported())
            {
                PolicySendCommand(CMTReject, peSinkReady, 0);
                pr_err("%s - Policy check vconn_swap_to_on_supported: FALSE\n", __func__);
            }
            else
            {
                PolicySendCommand(CMTAccept, peSinkEvaluateVCONNSwap, 1);           // Send the Accept message and wait for the good CRC
            }
            break;
        case 1:
            if (IsVCONNSource)                                                  // If we are currently sourcing VCONN...
            {
                platform_set_timer(&PolicyStateTimer, tVCONNSourceOn);                              // Enable the VCONNOnTimer and wait for a PS_RDY message
                PolicySubIndex++;                                               // Increment the subindex to move to waiting for a PS_RDY message
            }
            else                                                                // Otherwise we need to start sourcing VCONN
            {
                if (blnCCPinIsCC1)                                              // If the CC pin is CC1...
                {
                    Registers.Switches.VCONN_CC2 = 1;                           // Enable VCONN for CC2
                    Registers.Switches.PDWN2 = 0;                               // Disable the pull-down on CC2 to avoid sinking unnecessary current
                }
                else                                                            // Otherwise the CC pin is CC2
                {
                    Registers.Switches.VCONN_CC1 = 1;                           // Enable VCONN for CC1
                    Registers.Switches.PDWN1 = 0;                               // Disable the pull-down on CC1 to avoid sinking unnecessary current
                }
                platform_set_vconn(TRUE);
                DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]);      // Commit the register setting to the device
                IsVCONNSource = TRUE;
                platform_set_timer(&PolicyStateTimer, VbusTransitionTime);                          // Allow time for the FPF2498 to enable...
                PolicySubIndex = 3;                                             // Skip the next state and move onto sending the PS_RDY message after the timer expires            }
            }
            break;
        case 2:
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            Registers.Switches.VCONN_CC1 = 0;                   // Disable the VCONN source
                            Registers.Switches.VCONN_CC2 = 0;                   // Disable the VCONN source
                            Registers.Switches.PDWN1 = 1;                       // Ensure the pull-down on CC1 is enabled
                            Registers.Switches.PDWN2 = 1;                       // Ensure the pull-down on CC2 is enabled
                            platform_set_vconn(FALSE);
                            DeviceWrite(regSwitches0, 1, &Registers.Switches.byte[0]); // Commit the register setting to the device
                            IsVCONNSource = FALSE;
                            PolicyState = peSinkReady;                          // Move onto the Sink Ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the VCONNOnTimer times out...
            {
                PolicyState = peSourceSendHardReset;                            // Issue a hard reset
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:
            if (platform_check_timer(&PolicyStateTimer))
            {
                PolicySendCommand(CMTPS_RDY, peSinkReady, 0);                       // Send the Accept message and wait for the good CRC
            }
            break;
    }
}

void PolicySinkSendPRSwap(void)
{
#ifdef FSC_HAVE_DRP
    FSC_U8 Status;
    switch(PolicySubIndex)
    {
        case 0: // Send the PRSwap command
            if (PolicySendCommand(CMTPR_Swap, peSinkSendPRSwap, 1) == STAT_SUCCESS) // Send the PR_Swap message and wait for the good CRC
                platform_set_timer(&PolicyStateTimer, tSenderResponse);                             // Once we receive the good CRC, set the sender response timer
            break;
        case 1:  // Require Accept message to move on or go back to ready state
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTAccept:                                         // If we get the Accept message...
                            IsPRSwap = TRUE;
                            PolicyHasContract = FALSE;
                            DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
                            platform_notify_pd_contract(FALSE, 0, 0, 0);
                            platform_set_timer(&PolicyStateTimer, tPSSourceOff);                    // Start the sink transition timer
                            PolicySubIndex++;                                   // Increment the subindex to move onto the next step
                            break;
                        case CMTWait:                                           // If we get either the reject or wait message...
                        case CMTReject:
                            PolicyState = peSinkReady;                          // Go back to the sink ready state
                            PolicySubIndex = 0;                                 // Clear the sub index
                            IsPRSwap = FALSE;
                            PDTxStatus = txIdle;                                // Clear the transmitter status
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the SenderResponseTimer times out...
            {
                PolicyState = peSinkReady;                                      // Go back to the sink ready state
                PolicySubIndex = 0;                                             // Clear the sub index
                IsPRSwap = FALSE;
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        case 2:     // Wait for a PS_RDY message to be received to indicate that the original source is no longer supplying VBUS
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            RoleSwapToAttachedSource();                         // Initiate the Type-C state machine for a power role swap
                            PolicyIsSource = TRUE;
                            Registers.Switches.POWERROLE = PolicyIsSource;
                            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
                            platform_set_timer(&PolicyStateTimer, 0);
                            PolicySubIndex++;                                   // Increment the sub-index to move onto the next state
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the PSSourceOn times out...
            {
                PolicyState = peErrorRecovery;                                  // Go to the error recovery state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default: // Allow time for the supply to rise and then send the PS_RDY message
            if (isVBUSOverVoltage(VBUS_MDAC_4P20))
            {
                Status = PolicySendCommandNoReset(CMTPS_RDY, peSourceStartup, 0);   // When we get the good CRC, we move onto the source startup state to complete the swap
                if (Status == STAT_ERROR)
                    PolicyState = peErrorRecovery;                              // If we get an error, go to the error recovery state
                platform_start_timer(&PolicyStateTimer, tSwapSourceStart);
            }
            break;
    }
#endif // FSC_HAVE_DRP
}

void PolicySinkEvaluatePRSwap(void)
{
#ifdef FSC_HAVE_DRP
    FSC_U8 Status;
    switch(PolicySubIndex)
    {
        case 0: // Send either the Accept or Reject command
            if (!Accepts_PR_Swap_As_Snk || (UUT_Device_Type == 0) ||
                    ((PartnerCaps.FPDOSupply.SupplyType == pdoTypeFixed) && (PartnerCaps.FPDOSupply.DualRolePower == FALSE)))      // Determine Accept/Reject based on partner's Dual Role Power
            {
                PolicySendCommand(CMTReject, peSinkReady, 0);                   // Send the reject if we are not a DRP
            }
            else
            {
				IsPRSwap = TRUE;
                if (PolicySendCommand(CMTAccept, peSinkEvaluatePRSwap, 1) == STAT_SUCCESS) // Send the Accept message and wait for the good CRC
                {
                    PolicyHasContract = FALSE;
                    DetachThreshold = VBUS_MDAC_3P36;    // Default to 5V detach threshold
                    platform_notify_pd_contract(FALSE, 0, 0, 0);
                    platform_set_timer(&PolicyStateTimer, tPSSourceOff);                         // Start the sink transition timer
                }
            }
            break;
        case 1: // Wait for the PS_RDY command to come in and indicate the source has turned off VBUS
            if (ProtocolMsgRx)                                                  // Have we received a message from the source?
            {
                ProtocolMsgRx = FALSE;                                          // Reset the message received flag since we're handling it here
                if (PolicyRxHeader.NumDataObjects == 0)                         // If we have received a command
                {
                    switch (PolicyRxHeader.MessageType)                         // Determine which command was received
                    {
                        case CMTPS_RDY:                                         // If we get the PS_RDY message...
                            RoleSwapToAttachedSource();                         // Initiate the Type-C state machine for a power role swap
                            PolicyIsSource = TRUE;
                            Registers.Switches.POWERROLE = PolicyIsSource;
                            DeviceWrite(regSwitches1, 1, &Registers.Switches.byte[1]);
                            platform_set_timer(&PolicyStateTimer, 0);                               // Deprecated to use VBUS voltage
                            PolicySubIndex++;                                   // Increment the sub-index to move onto the next state
                            break;
                        default:                                                // For all other commands received, simply ignore them
                            break;
                    }
                }
            }
            else if (platform_check_timer(&PolicyStateTimer))                                         // If the PSSourceOn times out...
            {
                PolicyState = peErrorRecovery;                                  // Go to the error recovery state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
        default:    // Wait for VBUS to rise and then send the PS_RDY message
            if (isVBUSOverVoltage(VBUS_MDAC_4P62))
            {
                platform_delay_10us(3000);
                Status = PolicySendCommandNoReset(CMTPS_RDY, peSourceStartup, 0);   // When we get the good CRC, we move onto the source startup state to complete the swap
                if (Status == STAT_ERROR) PolicyState = peErrorRecovery;        // If we get an error, go to the error recovery state
                IsPRSwap = FALSE;
                platform_start_timer(&PolicyStateTimer, tSwapSourceStart);
            }
            break;
    }
#else
    PolicySendCommand(CMTReject, peSinkReady, 0);                               // Send the reject if we are not a DRP
#endif // FSC_HAVE_DRP
}
#endif // FSC_HAVE_SNK

#ifdef FSC_HAVE_VDM

void PolicyGiveVdm(void) {

    if (ProtocolMsgRx && PolicyRxHeader.MessageType == DMTVenderDefined)        // Have we received a VDM message
    {
        sendVdmMessageFailed();                                                 // if we receive anything, kick out of here (interruptible)
        PolicySubIndex = 0;                                                     // Reset the sub index
        PDTxStatus = txIdle;                                                    // Reset the transmitter status
    }
    else if (sendingVdmData)
    {

        FSC_U8 result = PolicySendData(DMTVenderDefined, vdm_msg_length, vdm_msg_obj, vdm_next_ps, 0, vdm_sop);
        if (result == STAT_SUCCESS)
        {
            FSC_PRINT("FUSB %s - VDM Sent Via PolicyGiveVDM Pass\n",__func__);
            if (expectingVdmResponse())
            {
                startVdmTimer(PolicyState);
            }
            else
            {
                resetPolicyState();
				sendingVdmData = FALSE;
				if(Registers.Switches.POWERROLE == 0)
				{
					PolicySinkReady();
				}
				else
				{
					PolicySourceReady();
				}
				return;
            }
            sendingVdmData = FALSE;
        }
        else if (result == STAT_ERROR)
        {
            FSC_PRINT("FUSB %s - VDM Sent Via PolicyGiveVDM Fail\n",__func__);
            sendVdmMessageFailed();
            sendingVdmData = FALSE;
        }
    }
    else
    {
        sendVdmMessageFailed();
    }

    if (VdmTimerStarted && (platform_check_timer(&VdmTimer)))
    {
        vdmMessageTimeout();
    }
}

void PolicyVdm (void) {

    FSC_U8 result;

    if (ProtocolMsgRx)                                                          // Have we received a message from the source?
    {
        ProtocolMsgRx = FALSE;                                                  // Reset the message received flag since we're handling it here
        if (PolicyRxHeader.NumDataObjects != 0)                                 // If we have received a command
        {
            switch (PolicyRxHeader.MessageType)
            {
                case DMTVenderDefined:
                    convertAndProcessVdmMessage(ProtocolMsgRxSop);
                    break;
                default:                                                        // If we get something we are not expecting... simply ignore them
                    resetPolicyState();                                    // if not a VDM message, kick out of VDM state (interruptible)
                    ProtocolMsgRx = TRUE;                                       // reset flag so other state can see the message and process
                    break;
            }
        }
        else
        {
            resetPolicyState();                                            // if not a VDM message, kick out of VDM state (interruptible)
            ProtocolMsgRx = TRUE;                                               // reset flag so other state can see the message and process
        }
        PolicySubIndex = 0;                                                     // Reset the sub index
        PDTxStatus = txIdle;                                                    // Reset the transmitter status
    }
    else
    {
        if (sendingVdmData)
        {
            result = PolicySendData(DMTVenderDefined, vdm_msg_length, vdm_msg_obj, vdm_next_ps, 0, vdm_sop);
            if (result == STAT_SUCCESS || result == STAT_ERROR)
            {
                FSC_PRINT("FUSB %s - VDM Sent Via PolicyVDM\n",__func__);
                sendingVdmData = FALSE;
            }
        }
    }

    if (VdmTimerStarted && (platform_check_timer(&VdmTimer)))
    {
        if(PolicyState == peDfpUfpVdmIdentityRequest)
        {
            AutoVdmState = AUTO_VDM_DONE;
            // Debug message here
            FSC_PRINT("FUSB %s - AutoVdmState = %d \n",__func__,AUTO_VDM_DONE);
        }
        vdmMessageTimeout();
    }
}

#endif // FSC_HAVE_VDM

void PolicyInvalidState (void) {
    // reset if we get to an invalid state
    if (PolicyIsSource)
    {
        PolicyState = peSourceSendHardReset;
    }
    else
    {
        PolicyState = peSinkSendHardReset;
    }
}

// ########################## General PD Messaging ########################## //

FSC_BOOL PolicySendHardReset(PolicyState_t nextState, FSC_U32 delay)
{
    FSC_BOOL Success = FALSE;
    switch (PolicySubIndex)
    {
        case 0:
            switch (PDTxStatus)
            {
                case txReset:
                case txWait:
                    // Do nothing until the protocol layer finishes generating the hard reset signaling
                    // The next state should be either txCollision or txSuccess
                    break;
                case txSuccess:
                    platform_set_timer(&PolicyStateTimer, delay);                                   // Set the amount of time prior to proceeding to the next state
                    PolicySubIndex++;                                           // Move onto the next state
                    Success = TRUE;
                    break;
                default:                                                        // None of the other states should actually occur, so...
                    PDTxStatus = txReset;                                       // Set the transmitter status to resend a hard reset
                    break;
            }
            break;
        default:
            if (platform_check_timer(&PolicyStateTimer))                                          // Once tPSHardReset has elapsed...
            {
                platform_set_timer(&PolicyStateTimer, tPSHardReset - tHardResetOverhead);
                HardResetCounter++;                                             // Increment the hard reset counter once successfully sent
                PolicyState = nextState;                                        // Go to the state to transition to the default sink state
                PolicySubIndex = 0;                                             // Clear the sub index
                PDTxStatus = txIdle;                                            // Clear the transmitter status
            }
            break;
    }
    return Success;
}

FSC_U8 PolicySendCommand(FSC_U8 Command, PolicyState_t nextState, FSC_U8 subIndex)
{
    FSC_U8 Status = STAT_BUSY;
    switch (PDTxStatus)
    {
        case txIdle:
            PolicyTxSop = SOP_TYPE_SOP;
            PolicyTxHeader.word = 0;                                            // Clear the word to initialize for each transaction
            PolicyTxHeader.NumDataObjects = 0;                                  // Clear the number of objects since this is a command
            PolicyTxHeader.MessageType = Command & 0x0F;                        // Sets the message type to the command passed in
            PolicyTxHeader.PortDataRole = PolicyIsDFP;                          // Set whether the port is acting as a DFP or UFP
            PolicyTxHeader.PortPowerRole = PolicyIsSource;                      // Set whether the port is serving as a power source or sink
            PolicyTxHeader.SpecRevision = USBPDSPECREV;                         // Set the spec revision
            PDTxStatus = txSend;                                                // Indicate to the Protocol layer that there is something to send
			if(ProtocolState == PRLIdle) ProtocolIdle();
            break;
        case txSend:
        case txBusy:
        case txWait:
            // Waiting for GoodCRC or timeout of the protocol
            // May want to put in a second level timeout in case there's an issue with the protocol getting hung
            break;
        case txSuccess:
            PolicyState = nextState;                                            // Go to the next state requested
            PolicySubIndex = subIndex;
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_SUCCESS;
            break;
        case txError:                                                           // Didn't receive a GoodCRC message...
            if (PolicyState == peSourceSendSoftReset)                           // If as a source we already failed at sending a soft reset...
                PolicyState = peSourceSendHardReset;                            // Move onto sending a hard reset (source)
            else if (PolicyState == peSinkSendSoftReset)                        // If as a sink we already failed at sending a soft reset...
                PolicyState = peSinkSendHardReset;                              // Move onto sending a hard reset (sink)
            else if (PolicyIsSource)                                            // Otherwise, if we are a source...
                PolicyState = peSourceSendSoftReset;                            // Attempt to send a soft reset (source)
            else                                                                // We are a sink, so...
                PolicyState = peSinkSendSoftReset;                              // Attempt to send a soft reset (sink)
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_ERROR;
            break;
        case txCollision:
            CollisionCounter++;                                                 // Increment the collision counter
            if (CollisionCounter > nRetryCount)                                 // If we have already retried two times
            {
                if (PolicyIsSource)
                    PolicyState = peSourceSendHardReset;                        // Go to the source hard reset state
                else
                    PolicyState = peSinkSendHardReset;                          // Go to the sink hard reset state
                PolicySubIndex = 0;                                             // Reset the sub index
                PDTxStatus = txReset;                                           // Set the transmitter status to send a hard reset
                Status = STAT_ERROR;
            }
            else                                                                // Otherwise we are going to try resending the soft reset
                PDTxStatus = txIdle;                                            // Clear the transmitter status for the next operation
            break;
        default:                                                                // For an undefined case, reset everything (shouldn't get here)
            if (PolicyIsSource)
                PolicyState = peSourceSendHardReset;                            // Go to the source hard reset state
            else
                PolicyState = peSinkSendHardReset;                              // Go to the sink hard reset state
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txReset;                                               // Set the transmitter status to send a hard reset
            Status = STAT_ERROR;
            break;
    }
    return Status;
}

FSC_U8 PolicySendCommandNoReset(FSC_U8 Command, PolicyState_t nextState, FSC_U8 subIndex)
{
    FSC_U8 Status = STAT_BUSY;
    switch (PDTxStatus)
    {
        case txIdle:
            PolicyTxSop = SOP_TYPE_SOP;
            PolicyTxHeader.word = 0;                                            // Clear the word to initialize for each transaction
            PolicyTxHeader.NumDataObjects = 0;                                  // Clear the number of objects since this is a command
            PolicyTxHeader.MessageType = Command & 0x0F;                        // Sets the message type to the command passed in
            PolicyTxHeader.PortDataRole = PolicyIsDFP;                          // Set whether the port is acting as a DFP or UFP
            PolicyTxHeader.PortPowerRole = PolicyIsSource;                      // Set whether the port is serving as a power source or sink
            PolicyTxHeader.SpecRevision = USBPDSPECREV;                         // Set the spec revision
            PDTxStatus = txSend;                                                // Indicate to the Protocol layer that there is something to send
			if(ProtocolState == PRLIdle) ProtocolIdle();
            break;
        case txSend:
        case txBusy:
        case txWait:
            // Waiting for GoodCRC or timeout of the protocol
            // May want to put in a second level timeout in case there's an issue with the protocol getting hung
            break;
        case txSuccess:
            PolicyState = nextState;                                            // Go to the next state requested
            PolicySubIndex = subIndex;
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_SUCCESS;
            break;
        default:                                                                // For all error cases (and undefined),
            PolicyState = peErrorRecovery;                                      // Go to the error recovery state
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txReset;                                               // Set the transmitter status to send a hard reset
            Status = STAT_ERROR;
            break;
    }
    return Status;
}

FSC_U8 PolicySendData(FSC_U8 MessageType, FSC_U8 NumDataObjects, doDataObject_t* DataObjects, PolicyState_t nextState, FSC_U8 subIndex, SopType sop)
{
    FSC_U8 Status = STAT_BUSY;
    FSC_U32 i;
    switch (PDTxStatus)
    {
        case txIdle:
            PolicyTxSop = sop;
            if (NumDataObjects > 7)
                NumDataObjects = 7;
            PolicyTxHeader.word = 0x0000;                                       // Clear the word to initialize for each transaction

            PolicyTxHeader.NumDataObjects = NumDataObjects;                     // Set the number of data objects to send
            PolicyTxHeader.MessageType = MessageType & 0x0F;                    // Sets the message type to the what was passed in
            PolicyTxHeader.PortDataRole = PolicyIsDFP;                          // Set whether the port is acting as a DFP or UFP
            PolicyTxHeader.PortPowerRole = PolicyIsSource;                      // Set whether the port is serving as a power source or sink
            if(sop != SOP_TYPE_SOP) {
                PolicyTxHeader.PortDataRole = 0;
                PolicyTxHeader.PortPowerRole = 0;
            }
            PolicyTxHeader.SpecRevision = USBPDSPECREV;                         // Set the spec revision
            for (i=0; i<NumDataObjects; i++)                                    // Loop through all of the data objects sent
                PolicyTxDataObj[i].object = DataObjects[i].object;              // Set each buffer object to send for the protocol layer
            if (PolicyState == peSourceSendCaps)                                // If we are in the send source caps state...
                CapsCounter++;                                                  // Increment the capabilities counter
            PDTxStatus = txSend;                                                // Indicate to the Protocol layer that there is something to send
			if(ProtocolState == PRLIdle) ProtocolIdle();
            break;
        case txSend:
        case txBusy:
        case txWait:
            // Waiting for GoodCRC or timeout of the protocol
            // May want to put in a second level timeout in case there's an issue with the protocol getting hung
            break;
        case txCollision:
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txIdle;                                               // Set the transmitter status to send a hard reset
            Status = STAT_ERROR;
            break;
        case txSuccess:
            PolicyState = nextState;                                            // Go to the next state requested
            PolicySubIndex = subIndex;
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_SUCCESS;
            break;
        case txError:                                                           // Didn't receive a GoodCRC message...
            if (PolicyState == peSourceSendCaps)                                // If we were in the send source caps state when the error occurred...
                PolicyState = peSourceDiscovery;                                // Go to the discovery state
            else if (PolicyIsSource)                                            // Otherwise, if we are a source...
                PolicyState = peSourceSendSoftReset;                            // Attempt to send a soft reset (source)
            else                               // Otherwise...
                PolicyState = peSinkSendSoftReset;                              // Go to the soft reset state
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_ERROR;
            break;
        default:                                                                // For undefined cases, reset everything
            if (PolicyIsSource)
                PolicyState = peSourceSendHardReset;                            // Go to the source hard reset state
            else
                PolicyState = peSinkSendHardReset;                              // Go to the sink hard reset state
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txReset;                                               // Set the transmitter status to send a hard reset
            Status = STAT_ERROR;
            break;
    }
    return Status;
}

FSC_U8 PolicySendDataNoReset(FSC_U8 MessageType, FSC_U8 NumDataObjects, doDataObject_t* DataObjects, PolicyState_t nextState, FSC_U8 subIndex)
{
    FSC_U8 Status = STAT_BUSY;
    FSC_U32 i;
    switch (PDTxStatus)
    {
        case txIdle:
            PolicyTxSop = SOP_TYPE_SOP;
            if (NumDataObjects > 7)
                NumDataObjects = 7;
            PolicyTxHeader.word = 0x0000;                                       // Clear the word to initialize for each transaction
            PolicyTxHeader.NumDataObjects = NumDataObjects;                     // Set the number of data objects to send
            PolicyTxHeader.MessageType = MessageType & 0x0F;                    // Sets the message type to the what was passed in
            PolicyTxHeader.PortDataRole = PolicyIsDFP;                          // Set whether the port is acting as a DFP or UFP
            PolicyTxHeader.PortPowerRole = PolicyIsSource;                      // Set whether the port is serving as a power source or sink
            PolicyTxHeader.SpecRevision = USBPDSPECREV;                         // Set the spec revision
            for (i=0; i<NumDataObjects; i++)                                    // Loop through all of the data objects sent
                PolicyTxDataObj[i].object = DataObjects[i].object;              // Set each buffer object to send for the protocol layer
            if (PolicyState == peSourceSendCaps)                                // If we are in the send source caps state...
                CapsCounter++;                                                  // Increment the capabilities counter
            PDTxStatus = txSend;                                                // Indicate to the Protocol layer that there is something to send
			if(ProtocolState == PRLIdle) ProtocolIdle();
            break;
        case txSend:
        case txBusy:
        case txWait:
            // Waiting for GoodCRC or timeout of the protocol
            // May want to put in a second level timeout in case there's an issue with the protocol getting hung
            break;
        case txSuccess:
            PolicyState = nextState;                                            // Go to the next state requested
            PolicySubIndex = subIndex;
            PDTxStatus = txIdle;                                                // Reset the transmitter status
            Status = STAT_SUCCESS;
            break;
        default:                                                                // For error cases (and undefined), ...
            PolicyState = peErrorRecovery;                                      // Go to the error recovery state
            PolicySubIndex = 0;                                                 // Reset the sub index
            PDTxStatus = txReset;                                               // Set the transmitter status to send a hard reset
            Status = STAT_ERROR;
            break;
    }
    return Status;
}

void UpdateCapabilitiesRx(FSC_BOOL IsSourceCaps)
{
    FSC_U32 i;
#ifdef FSC_DEBUG
    SourceCapsUpdated = IsSourceCaps;                                           // Set the flag to indicate that the received capabilities are valid
#endif // FSC_DEBUG
    CapsHeaderReceived.word = PolicyRxHeader.word;                              // Store the header for the latest capabilities received
    for (i=0; i<CapsHeaderReceived.NumDataObjects; i++)                         // Loop through all of the received data objects
        CapsReceived[i].object = PolicyRxDataObj[i].object;                     // Store each capability
    for (i=CapsHeaderReceived.NumDataObjects; i<7; i++)                         // Loop through all of the invalid objects
        CapsReceived[i].object = 0;                                             // Clear each invalid object
    PartnerCaps.object = CapsReceived[0].object;
}

// ---------- BIST Receive Mode --------------------- //

void policyBISTReceiveMode(void)    // Not Implemented
{
    // Tell protocol layer to go to BIST Receive Mode
    // Go to BIST_Frame_Received if a test frame is received
    // Transition to SRC_Transition_to_Default, SNK_Transition_to_Default, or CBL_Ready when Hard_Reset received
}

void policyBISTFrameReceived(void)  // Not Implemented
{
    // Consume BIST Transmit Test Frame if received
    // Transition back to BIST_Frame_Received when a BIST Test Frame has been received
    // Transition to SRC_Transition_to_Default, SNK_Transition_to_Default, or CBL_Ready when Hard_Reset received
}

// ---------- BIST Carrier Mode and Eye Pattern ----- //

/* TODO: Flush TxFIFO before sending Hard Reset */
void policyBISTCarrierMode2(void)
{
    switch (PolicySubIndex)
    {
        default:
        case 0:
            Registers.Control.BIST_MODE2 = 1;                                   // Tell protocol layer to go to BIST_Carrier_Mode_2
            DeviceWrite(regControl1, 1, &Registers.Control.byte[1]);
            Registers.Control.TX_START = 1;                                             // Set the bit to enable the transmitter
            DeviceWrite(regControl0, 1, &Registers.Control.byte[0]);                    // Commit TX_START to the device
            Registers.Control.TX_START = 0;                                             // Clear this bit (write clear)
            platform_start_timer(&PolicyStateTimer, tBISTContMode);                                        // Initialize and run BISTContModeTimer
            PolicySubIndex = 1;
			g_Idle = TRUE;
            break;
        case 1:
            if(PolicyStateTimer.expired == TRUE)                                                   // Transition to SRC_Transition_to_Default, SNK_Transition_to_Default, or CBL_Ready when BISTContModeTimer times out
            {
                Registers.Control.BIST_MODE2 = 0;                                           // Disable BIST_Carrier_Mode_2 (PD_RESET does not do this)
                DeviceWrite(regControl1, 1, &Registers.Control.byte[1]);
                platform_start_timer(&PolicyStateTimer, 5);                               // Delay to allow preamble to finish
                PolicySubIndex++;
				g_Idle = TRUE;
            }
            break;
        case 2:
            if(PolicyStateTimer.expired == TRUE)                                                   // Transition to SRC_Transition_to_Default, SNK_Transition_to_Default, or CBL_Ready when BISTContModeTimer times out
            {
                ProtocolFlushTxFIFO();
                if (PolicyIsSource)                                                     // If we are the source...
                {
#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACCMODE))
                    PolicyState = peSourceSendHardReset;                                // This will hard reset then transition to default
                    PolicySubIndex = 0;
#endif // FSC_HAVE_SRC
                }
                else                                                                    // Otherwise we are the sink...
                {
#ifdef FSC_HAVE_SNK
                    PolicyState = peSinkSendHardReset;                                // This will hard reset then transition to default
                    PolicySubIndex = 0;
#endif // FSC_HAVE_SNK
                }
            }
            break;
    }

}

void policyBISTTestData(void)
{
        if(Registers.DeviceID.VERSION_ID == VERSION_302A)
        {
            DeviceWrite(regControl1, 1, &Registers.Control.byte[1]);
        }
        else
        {
            // Do Nothing
        }
}

#ifdef FSC_HAVE_VDM

void InitializeVdmManager(void)
{
	initializeVdm();

	// configure callbacks
	vdmm.req_id_info		= &vdmRequestIdentityInfo;
	vdmm.req_svid_info		= &vdmRequestSvidInfo;
	vdmm.req_modes_info	= &vdmRequestModesInfo;
	vdmm.enter_mode_result  = &vdmEnterModeResult;
	vdmm.exit_mode_result   = &vdmExitModeResult;
	vdmm.inform_id			= &vdmInformIdentity;
	vdmm.inform_svids		= &vdmInformSvids;
	vdmm.inform_modes		= &vdmInformModes;
	vdmm.inform_attention   = &vdmInformAttention;
	vdmm.req_mode_entry		= &vdmModeEntryRequest;
	vdmm.req_mode_exit		= &vdmModeExitRequest;
}
void convertAndProcessVdmMessage(SopType sop)
{
    FSC_U32 i;
    // form the word arrays that VDM block expects
    // note: may need to rethink the interface. but this is quicker to develop right now
    FSC_U32 vdm_arr[MAX_VDM_LEN] = {0};
    for (i = 0; i < PolicyRxHeader.NumDataObjects; i++) {
        vdm_arr[i] = PolicyRxDataObj[i].object;
    }
    if (processVdmMessage(sop, vdm_arr, PolicyRxHeader.NumDataObjects) == 0)
	{
		//sendingVdmData = TRUE;
		//ProtocolCheckRxBeforeTx = TRUE;
		//PolicyState = peGiveVdm;
		//PolicyGiveVdm();
	}
	else
	{
		resetPolicyState();
	}
}

void sendVdmMessage(SopType sop, FSC_U32* arr, FSC_U32 length, PolicyState_t next_ps) {
    FSC_U32 i;
    // 'cast' to type that PolicySendData expects
    // didn't think this was necessary, but it fixed some problems. - Gabe
    vdm_msg_length = length;
    vdm_next_ps = next_ps;
    vdm_sop = sop;
    for (i = 0; i < vdm_msg_length; i++) {
        vdm_msg_obj[i].object = arr[i];
    }
    sendingVdmData = TRUE;
    ProtocolCheckRxBeforeTx = TRUE;
    VdmTimerStarted = FALSE;
    PolicyState = peGiveVdm;
	PolicyGiveVdm();
}

void doVdmCommand(void)
{
    FSC_U32 command;
    FSC_U32 svid;
    FSC_U32 mode_index;
    SopType sop;


    command = PDTransmitObjects[0].byte[0] & 0x1F;
    svid = 0;
    svid |= (PDTransmitObjects[0].byte[3] << 8);
    svid |= (PDTransmitObjects[0].byte[2] << 0);

    mode_index = 0;
    mode_index = PDTransmitObjects[0].byte[1] & 0x7;

    // only SOP today
    sop = SOP_TYPE_SOP;

#ifdef FSC_HAVE_DP
    if (svid == DP_SID) {
        if (command == DP_COMMAND_STATUS) {
            requestDpStatus();
        } else if (command == DP_COMMAND_CONFIG) {
            DisplayPortConfig_t temp;
            temp.word = PDTransmitObjects[1].object;
            requestDpConfig();
        }
    }
#endif // FSC_HAVE_DP

    if (command == DISCOVER_IDENTITY) {
        requestDiscoverIdentity(sop);
    } else if (command == DISCOVER_SVIDS) {
        requestDiscoverSvids(sop);
    } else if (command == DISCOVER_MODES) {
        requestDiscoverModes(sop, svid);
    } else if (command == ENTER_MODE) {
        requestEnterMode(sop, svid, mode_index);
    } else if (command == EXIT_MODE) {
        requestExitMode(sop, svid, mode_index);
    }

}

// this function assumes we're already in either Source or Sink Ready states!
void autoVdmDiscovery (void)
{
    if (PDTxStatus == txIdle) { // wait for protocol layer to become idle
        switch (AutoVdmState) {
            case AUTO_VDM_INIT:
            case AUTO_VDM_DISCOVER_ID_PP:
                AutoVdmState = AUTO_VDM_DISCOVER_SVIDS_PP;
                requestDiscoverIdentity(SOP_TYPE_SOP);
                FSC_PRINT("FUSB %s - AUTO_VDM_DISCOVER_ID_PP\n",__func__);
                break;
            case AUTO_VDM_DISCOVER_SVIDS_PP:
		if(platform_get_modal_operation_supported()) {
			AutoVdmState = AUTO_VDM_DISCOVER_MODES_PP;
			requestDiscoverSvids(SOP_TYPE_SOP);
			FSC_PRINT("FUSB %s - AUTO_VDM_DISCOVER_SVIDS_PP\n",__func__);
                } else {
			AutoVdmState = AUTO_VDM_DONE;
			FSC_PRINT("FUSB %s - Modal Operation Not Supported - AutoVDM Done\n",__func__);
		}
                break;
            case AUTO_VDM_DISCOVER_MODES_PP:
                if (auto_mode_disc_tracker == core_svid_info.num_svids) {
                    AutoVdmState = AUTO_VDM_ENTER_MODE_PP;
                    auto_mode_disc_tracker = 0;
                    FSC_PRINT("FUSB %s - AUTO_VDM_DISCOVER_MODES_PP\n",__func__);
                } else {
                    requestDiscoverModes(SOP_TYPE_SOP, core_svid_info.svids[auto_mode_disc_tracker]);
                    auto_mode_disc_tracker++;
                    FSC_PRINT("FUSB %s - NOT_AUTO_VDM_DISCOVER_MODES_PP\n",__func__);
                }
                break;
            case AUTO_VDM_ENTER_MODE_PP:
                if (AutoModeEntryObjPos > 0) {
                    AutoVdmState = AUTO_VDM_DP_GET_STATUS;
                    requestEnterMode(SOP_TYPE_SOP, SVID_AUTO_ENTRY, AutoModeEntryObjPos);
                    FSC_PRINT("FUSB %s - AUTO_VDM_ENTER_MODE_PP,AutoModeEntryObjPos = %d\n",__func__,AutoModeEntryObjPos);
                } else {
                    AutoVdmState = AUTO_VDM_DONE;
                    FSC_PRINT("FUSB %s - NOT AUTO_VDM_ENTER_MODE_PP\n",__func__);
                }
                break;
            case AUTO_VDM_DP_GET_STATUS:
                if (DpModeEntered) {
                    AutoVdmState = AUTO_VDM_GET_CONFIG;
                    requestDpStatus();
                }
                else{
                    AutoVdmState = AUTO_VDM_DONE;
                }
                break;
            case AUTO_VDM_GET_CONFIG:
                requestDpConfig();
                AutoVdmState = AUTO_VDM_DONE;
                break;
            default:
                AutoVdmState = AUTO_VDM_DONE;
                break;
        }
    }
}

#endif // FSC_HAVE_VDM

// This function is FUSB302 specific
SopType TokenToSopType(FSC_U8 data)
{
    SopType ret;
    // figure out what SOP* the data came in on
    if ((data & 0b11100000) == 0b11100000) {
        ret = SOP_TYPE_SOP;
    } else if ((data & 0b11100000) == 0b11000000) {
        ret = SOP_TYPE_SOP1;
    } else if ((data & 0b11100000) == 0b10100000) {
        ret = SOP_TYPE_SOP2;
    } else if ((data & 0b11100000) == 0b10000000) {
        ret = SOP_TYPE_SOP1_DEBUG;
    } else if ((data & 0b11100000) == 0b01100000) {
        ret = SOP_TYPE_SOP2_DEBUG;
    } else {
        ret = SOP_TYPE_ERROR;
    }
    return ret;
}

void resetLocalHardware(void)
{
    FSC_U8 data = 0x20;
    DeviceWrite(regReset, 1, &data);   // Reset PD

    DeviceRead(regSwitches1, 1, &Registers.Switches.byte[1]);  // Re-read PD Registers
    DeviceRead(regSlice, 1, &Registers.Slice.byte);
    DeviceRead(regControl0, 1, &Registers.Control.byte[0]);
    DeviceRead(regControl1, 1, &Registers.Control.byte[1]);
    DeviceRead(regControl3, 1, &Registers.Control.byte[3]);
    DeviceRead(regMask, 1, &Registers.Mask.byte);
    DeviceRead(regMaska, 1, &Registers.MaskAdv.byte[0]);
    DeviceRead(regMaskb, 1, &Registers.MaskAdv.byte[1]);
    DeviceRead(regStatus0a, 2, &Registers.Status.byte[0]);
    DeviceRead(regStatus0, 2, &Registers.Status.byte[4]);
}

void processDMTBIST(void)
{
    FSC_U8 bdo = PolicyRxDataObj[0].byte[3]>>4;

    platform_notify_bist(TRUE);

    switch (bdo)
    {
        case BDO_BIST_Carrier_Mode_2:
            if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 100)  // Only enter BIST for 5V contract
            {
                PolicyState = PE_BIST_Carrier_Mode_2;
                PolicySubIndex = 0;
                ProtocolState = PRLIdle;
            }
            break;
        default:
        case BDO_BIST_Test_Data:
            if(CapsSource[USBPDContract.FVRDO.ObjectPosition-1].FPDOSupply.Voltage == 100) // Only enter BIST for 5V contract
            {
                if(Registers.DeviceID.VERSION_ID == VERSION_302A)
                {
                    Registers.Mask.byte = 0xFF;                                                 // Mask for VBUS and Hard Reset
                    Registers.Mask.M_VBUSOK = 0;
                    Registers.Mask.M_COMP_CHNG = 0;
                    DeviceWrite(regMask, 1, &Registers.Mask.byte);
                    Registers.MaskAdv.byte[0] = 0xFF;
                    Registers.MaskAdv.M_HARDRST = 0;
                    DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
                    Registers.MaskAdv.M_GCRCSENT = 1;
                    DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);

                    Registers.Control.RX_FLUSH = 1;                                     // Enable RxFIFO flushing
                }
                else
                {
					ProtocolFlushRxFIFO();

                    Registers.Control.BIST_TMODE = 1;                               // Auto-flush RxFIFO
                    DeviceWrite(regControl3, 1, &Registers.Control.byte[3]);

					Registers.Mask.byte = 0xFF;                                                 // Mask for VBUS and Hard Reset
                    Registers.Mask.M_VBUSOK = 0;
                    Registers.Mask.M_COMP_CHNG = 0;
                    DeviceWrite(regMask, 1, &Registers.Mask.byte);
                    Registers.MaskAdv.byte[0] = 0xFF;
                    Registers.MaskAdv.M_HARDRST = 0;
                    DeviceWrite(regMaska, 1, &Registers.MaskAdv.byte[0]);
                    Registers.MaskAdv.M_GCRCSENT = 1;
                    DeviceWrite(regMaskb, 1, &Registers.MaskAdv.byte[1]);

					g_Idle = TRUE;
                }

                PolicyState = PE_BIST_Test_Data;
                ProtocolState = PRLDisabled;                                        // Disable Protocol layer so we don't read FIFO
            }
            break;
    }
}

#ifdef FSC_DEBUG
void SendUSBPDHardReset(void)
{
    if (PolicyIsSource)                                                         // If we are the source...
        PolicyState = peSourceSendHardReset;                                    // set the source state to send a hard reset
    else                                                                        // Otherwise we are the sink...
        PolicyState = peSinkSendHardReset;                                      // so set the sink state to send a hard reset
    PolicySubIndex = 0;
    PDTxStatus = txIdle;                                                        // Reset the transmitter status
}

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACCMODE))
void WriteSourceCapabilities(FSC_U8* abytData)
{
    FSC_U32 i, j;
    sopMainHeader_t Header = {0};
    Header.byte[0] = *abytData++;                                               // Set the 1st PD header byte
    Header.byte[1] = *abytData++;                                               // Set the 2nd PD header byte
    if ((Header.NumDataObjects > 0) && (Header.MessageType == DMTSourceCapabilities))   // Only do anything if we decoded a source capabilities message
    {
        CapsHeaderSource.word = Header.word;                                    // Set the actual caps source header
        for (i=0; i<CapsHeaderSource.NumDataObjects; i++)                       // Loop through all the data objects
        {
            for (j=0; j<4; j++)                                                 // Loop through each byte of the object
                CapsSource[i].byte[j] = *abytData++;                            // Set the actual bytes
        }
        if (PolicyIsSource)                                                     // If we are currently acting as the source...
        {
            PDTransmitHeader.word = CapsHeaderSource.word;                      // Set the message type to capabilities to trigger sending the caps (only need the header to trigger)
            USBPDTxFlag = TRUE;                                                 // Set the flag to send the new caps when appropriate...
            SourceCapsUpdated = TRUE;                                           // Set the flag to indicate to the software that the source caps were updated
        }
    }
}
#endif // FSC_HAVE_SRC

void ReadSourceCapabilities(FSC_U8* abytData)
{
    FSC_U32 i, j;
    *abytData++ = CapsHeaderSource.byte[0];
    *abytData++ = CapsHeaderSource.byte[1];
    for (i=0; i<CapsHeaderSource.NumDataObjects; i++)
    {
        for (j=0; j<4; j++)
            *abytData++ = CapsSource[i].byte[j];
    }
}

#ifdef FSC_HAVE_SNK
void WriteSinkCapabilities(FSC_U8* abytData)
{
    FSC_U32 i, j;
    sopMainHeader_t Header = {0};
    Header.byte[0] = *abytData++;                                               // Set the 1st PD header byte
    Header.byte[1] = *abytData++;                                               // Set the 2nd PD header byte
    if ((Header.NumDataObjects > 0) && (Header.MessageType == DMTSinkCapabilities))   // Only do anything if we decoded a source capabilities message
    {
        CapsHeaderSink.word = Header.word;                                      // Set the actual caps sink header
        for (i=0; i<CapsHeaderSink.NumDataObjects; i++)                         // Loop through all the data objects
        {
            for (j=0; j<4; j++)                                                 // Loop through each byte of the object
                CapsSink[i].byte[j] = *abytData++;                              // Set the actual bytes
        }
        // We could also trigger sending the caps or re-evaluating, but we don't do anything with this info here...
    }
}

void WriteSinkRequestSettings(FSC_U8* abytData)
{
    FSC_U32 uintPower;
    SinkGotoMinCompatible = *abytData & 0x01 ? TRUE : FALSE;
    SinkUSBSuspendOperation = *abytData & 0x02 ? TRUE : FALSE;
    SinkUSBCommCapable = *abytData++ & 0x04 ? TRUE : FALSE;
    SinkRequestMaxVoltage = (FSC_U32) *abytData++;
    SinkRequestMaxVoltage |= ((FSC_U32) (*abytData++) << 8);                     // Voltage resolution is 50mV
    uintPower = (FSC_U32) *abytData++;
    uintPower |= ((FSC_U32) (*abytData++) << 8);
    uintPower |= ((FSC_U32) (*abytData++) << 16);
    uintPower |= ((FSC_U32) (*abytData++) << 24);
    SinkRequestOpPower = uintPower;                                             // Power resolution is 0.5mW
    uintPower = (FSC_U32) *abytData++;
    uintPower |= ((FSC_U32) (*abytData++) << 8);
    uintPower |= ((FSC_U32) (*abytData++) << 16);
    uintPower |= ((FSC_U32) (*abytData++) << 24);
    SinkRequestMaxPower = uintPower;                                            // Power resolution is 0.5mW
    // We could try resetting and re-evaluating the source caps here, but lets not do anything until requested by the user (soft reset or detach)
}

void ReadSinkRequestSettings(FSC_U8* abytData)
{
    *abytData = SinkGotoMinCompatible ? 0x01 : 0;
    *abytData |= SinkUSBSuspendOperation ? 0x02 : 0;
    *abytData++ |= SinkUSBCommCapable ? 0x04 : 0;
    *abytData++ = (FSC_U8) (SinkRequestMaxVoltage & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestMaxVoltage >> 8) & 0xFF);
    *abytData++ = (FSC_U8) (SinkRequestOpPower & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestOpPower >> 8) & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestOpPower >> 16) & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestOpPower >> 24) & 0xFF);
    *abytData++ = (FSC_U8) (SinkRequestMaxPower & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestMaxPower >> 8) & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestMaxPower >> 16) & 0xFF);
    *abytData++ = (FSC_U8) ((SinkRequestMaxPower >> 24) & 0xFF);
}
#endif // FSC_HAVE_SNK

void ReadSinkCapabilities(FSC_U8* abytData)
{
    FSC_U32 i, j;
    *abytData++ = CapsHeaderSink.byte[0];
    *abytData++ = CapsHeaderSink.byte[1];
    for (i=0; i<CapsHeaderSink.NumDataObjects; i++)
    {
        for (j=0; j<4; j++)
            *abytData++ = CapsSink[i].byte[j];
    }
}



void DisableUSBPD(void)
{
    if (USBPDEnabled)                                                          // If we are already disabled...
    {
        USBPDEnabled = FALSE;                                                   // Set the USBPD state machine to enabled
    }
}

FSC_BOOL GetPDStateLog(FSC_U8 * data){   // Loads log into byte array
    FSC_U32 i;
    FSC_U32 entries = PDStateLog.Count;
    FSC_U16 state_temp;
    FSC_U16 time_tms_temp;
    FSC_U16 time_s_temp;


    for(i=0; ((i<entries) && (i<12)); i++)
    {
        ReadStateLog(&PDStateLog, &state_temp, &time_tms_temp, &time_s_temp);

        data[i*5+1] = state_temp;
        data[i*5+2] = (time_tms_temp>>8);
        data[i*5+3] = (FSC_U8)time_tms_temp;
        data[i*5+4] = (time_s_temp)>>8;
        data[i*5+5] = (FSC_U8)time_s_temp;
    }

    data[0] = i;    // Send number of log packets

    return TRUE;
}

void ProcessReadPDStateLog(FSC_U8* MsgBuffer, FSC_U8* retBuffer)
{
    if (MsgBuffer[1] != 0)
    {
        retBuffer[1] = 0x01;             // Return that the version is not recognized
        return;
    }

    GetPDStateLog(&retBuffer[3]);   // Designed for 64 byte buffer
}

void ProcessPDBufferRead(FSC_U8* MsgBuffer, FSC_U8* retBuffer)
{
    if (MsgBuffer[1] != 0)
        retBuffer[1] = 0x01;                                             // Return that the version is not recognized
    else
    {
        retBuffer[4] = GetUSBPDBufferNumBytes();                         // Return the total number of bytes in the buffer
        retBuffer[5] = ReadUSBPDBuffer((FSC_U8*)&retBuffer[6], 58); // Return the number of bytes read and return the data
    }
}

#endif // FSC_DEBUG

void EnableUSBPD(void)
{
    if (!USBPDEnabled)                                                           // If we are not already enabled...
    {
        USBPDEnabled = TRUE;                                                    // Set the USBPD state machine to enabled                                                               // return since we don't have to do anything
    }
}

void SetVbusTransitionTime(FSC_U32 time_ms) {
    VbusTransitionTime = time_ms * TICK_SCALE_TO_MS;
}

void SetPolicyState(PolicyState_t state)
{
	PolicyState = state;
	PolicySubIndex = 0;
	PDTxStatus = txIdle;
}
