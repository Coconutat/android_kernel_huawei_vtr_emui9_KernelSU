/* **************************************************************************
 * port.h
 *
 * Defines the port implementation.
 * ************************************************************************** */
#ifndef FSCPM_PORT_H_
#define FSCPM_PORT_H_

#include "platform.h"
#include "TypeCTypes.h"
#include "PDTypes.h"
#include "SCPTypes.h"
#include "log.h"
#include "registers.h"
#include "timer.h"

#ifdef FSC_HAVE_VDM
#include "vdm_types.h"  /* VdmHandler class */
#endif /* FSC_HAVE_VDM */

#ifdef FSC_HAVE_DP
#include "display_port_types.h"
#endif /* FSC_HAVE_DP */

/* Voltage levels given in 25mV resolution for vSafe0V and vSafe5V levels */
#define FSC_VSAFE0V       32    /* 0.8V  : vSafe0V */
#define FSC_VEXTRASAFE0V  12    /* 0.3V  : vSafe0V */
#define FSC_VSAFE0V_DISCH 8    /* 0.2V  : Discharge slightly below vSafe5V */
#define FSC_VSAFE_DISCH	96    // 160	/* 4.0V : VOUT No Longer Sourced */
#define FSC_VSAFE_FOR_DOCK_LEAKAGE	160    // 160	/* 4.0V : VOUT No Longer Sourced */

#define FSC_VSAFE5V_DISC  146   /* 3.67V : Disconnect level */
#define FSC_VSAFE5V_L     190   /* 4.75V : vSafe5V - 5% */
#define FSC_VSAFE5V       200   /* 5.0V  : vSafe5V */
#define FSC_VSAFE5V_H     220   /* 5.5V  : vSafe5V + 10% */

#define FSC_MOISTURE_DET  0x6D

/* Macro definitions of the disconnect voltage (80% of V) and the high/low
 * range (+/- 5%) representing an acceptable value for a given VBus level.
 * volts is expected to be an FSC_U16 or FSC_U32 type.
 * Result value is given in 25mV resolution to match hardware-reported values.
 */
/* TODO test this to verify values and make sure this is how we want to
 * represent these values in the code.
 */
#define FSC_VBUS_05_V             5
#define FSC_VBUS_09_V             9
#define FSC_VBUS_12_V             12
#define FSC_VBUS_15_V             15
#define FSC_VBUS_20_V             20

#define FSC_VBUS_LVL_DISC(volts)  ((volts * 1000) / 25 - (volts * 200) / 25)
#define FSC_VBUS_LVL_PD_DISC(pdv) (pdv - (pdv * 20 / 100))
#define FSC_VBUS_LVL_L(volts)     ((volts * 1000) / 25 - (volts * 50) / 25)
#define FSC_VBUS_LVL(volts)       ((volts * 1000) / 25)
#define FSC_VBUS_LVL_H(volts)     ((volts * 1000) / 25 + (volts * 50) / 25)

/* This one is used as an alarm high val when we only care about the low end */
#define FSC_VBUS_LVL_HIGHEST      (FSC_VBUS_LVL_H(FSC_VBUS_20_V))

/* This one returns the voltage in 50mV resolution for PD */
#define FSC_VBUS_LVL_PD(volts)    ((volts * 1000) / 50)

/* Enum defining currently active mode */
typedef enum {
  Mode_None,
  Mode_PD,
  Mode_ACCP,
  Mode_SCP_A,
  Mode_SCP_B
} ModeActive_t;

typedef enum {
  Sbu_None,
  Sbu_Close_Aux,
  Sbu_Cross_Close_Aux,
  Sbu_Close_TXD_RXD,
  Sbu_Cross_Close_TXD_RXD
} SbuSwitch_t;
/*
 * The Port struct contains all port-related data and state information,
 * timer references, register map, etc.
 */
struct Port {
  FSC_U8 port_id_;                  /* Each port has an "ID", one indexed */
  FSC_U8 i2c_addr_;                 /* Assigned hardware I2C address */
  DeviceReg_t registers_;           /* Chip register object */
  FSC_BOOL idle_;                   /* If true, may give up processor */
  FSC_BOOL initialized_;            /* False until the INIT INT allows config */

  FSC_BOOL bc12_active_;
  ModeActive_t activemode_;         /* Currently active mode */
  FSC_BOOL watchdogenabled_;        /* Watchdog and timer interrupt enabled */

  /* *** ACCP Objects */
  ACCPState accp_state_;
  FSC_U8 accp_substate_;
  FSC_BOOL accp_waitonack_;
  FSC_U8 accp_supported_V_[10];     /* Values are (V * 10) */
  FSC_U8 accp_supported_I_[10];     /* Values are (I * 10) */

  /* *** SCP Objects */
  SCPState scp_state_;
  FSC_U8 scp_substate_;
  FSC_BOOL scp_waitonack_;
  FSC_U8 scp_setcount_;
  FSC_U8 scp_minV;                  /* See SCP spec for value formula */
  FSC_U8 scp_maxV;
  FSC_U8 scp_minI;
  FSC_U8 scp_maxI;
  FSC_U8 scp_stepV;
  FSC_U8 scp_stepI;

  /* *** Timer Objects */
  struct TimerObj tc_state_timer_; //TODO: Can be 16 bit (tDRPTryWait's fault - can be 8 bit w/o SNK + ACC)
  struct TimerObj policy_state_timer_; //TODO: Can be 16 bit
  struct TimerObj cc_debounce_timer_; //TODO: Can be 8 bit
  struct TimerObj pd_debounce_timer_; //TODO: Can be 8 bit

  /* *** Type-C (TC) port items */
  USBTypeCPort port_type_;           /* Src/Snk/DRP as config'd in the device */
  SourceOrSink source_or_sink_;      /* Src/Snk as current connection state */
  FSC_BOOL tc_enabled_;              /* TC state machine enabled */
  TypeCState tc_state_;              /* TC state machine current state */
  FSC_U8 tc_substate_;               /* TC state machine current sub-state */
  FSC_BOOL src_preferred_;           /* DRP, Src preferred */
  FSC_BOOL snk_preferred_;           /* DRP, Snk preferred */
  FSC_BOOL acc_support_;             /* Accessory support */
  USBTypeCCurrent snk_current_;      /* Current capability received */
  USBTypeCCurrent src_current_;      /* Current capability broadcasting */
  CCOrientation cc_pin_;             /* CC detected on CC1 or CC2 */
  CCTermType cc_term_;               /* Termination on CC pin */
  CCTermType vconn_term_;            /* Termination on VConn pin */
  FSC_BOOL is_hard_reset_;
  FSC_BOOL is_pr_swap_;
  FSC_BOOL is_vconn_swap_;
  FSC_U8 unattach_loop_counter_;

  /* *** PD Protocol port items */
  FSC_BOOL pd_active_;                /* PD active during valid connection */
  FSC_BOOL pd_enabled_;               /* PD state machine enabled state */
  ProtocolState_t protocol_state_;
  PDTxStatus_t pd_tx_status_;
  FSC_BOOL pd_tx_flag_;
  SopType policy_msg_tx_sop_;

  /*
   * message_id_ and message_id_counter_ to handle SOP/SOP'.
   * +1 to offset 0-indexed enum.
   * NOTE - If adding additional SOP types, make sure to update the array's
   * initializer loop condition to match!
   */
  FSC_U32 message_id_counter_[SOP_TYPE_SOP1 + 1];
  FSC_U32 message_id_[SOP_TYPE_SOP1 + 1];
  FSC_BOOL protocol_msg_rx_;
  SopType protocol_msg_rx_sop_;
  SopType protocol_msg_tx_sop_;
  FSC_U8 protocol_tx_bytes_;
  FSC_BOOL protocol_check_rx_b4_tx_;
  FSC_U8 protocol_tx_buffer_[FSC_PROTOCOL_BUFFER_SIZE];
  FSC_U8 protocol_rx_buffer_[FSC_PROTOCOL_BUFFER_SIZE];
  FSC_U8 protocol_retries_;
  FSC_BOOL protocol_use_sinktx_;

  /* *** PD Policy port items */
  PolicyState_t policy_state_;           /* Policy SM */
  PolicyState_t last_policy_state_;      /* Policy SM */
  PolicyState_t fail_state;
  FSC_U8 policy_subindex_;               /* Policy SM */
  FSC_BOOL policy_is_source_;            /* Power Source or Sink */
  FSC_BOOL policy_is_dfp_;               /* Data Role DFP or UFP */
  FSC_BOOL is_contract_valid_;           /* Is PD Contract valid? */
  FSC_BOOL is_vconn_source_;             /* Sourcing VConn? */
  FSC_U8 collision_counter_;
  FSC_U8 hard_reset_counter_;            /* Track how many hard resets sent */
  FSC_U8 caps_counter_;                  /* Track how many caps messages sent */
  FSC_BOOL policy_has_contract_;         /* Contract in place? */
  FSC_BOOL renegotiate_;                 /* Signal to re-negotiate contract */
  sopMainHeader_t policy_rx_header_;     /* Header for PD messages received */
  sopMainHeader_t policy_tx_header_;     /* Header for PD messages to send */
  doDataObject_t policy_rx_data_obj_[7]; /* Buffer for data objects received */
  doDataObject_t policy_tx_data_obj_[7]; /* Buffer for data objects to send */
  sopMainHeader_t pd_transmit_header_;   /* PD packet to send */
  sopMainHeader_t caps_header_sink_;     /* Sink caps header */
  sopMainHeader_t caps_header_source_;   /* Source caps header */
  sopMainHeader_t caps_header_received_; /* Last capabilities header received */
  doDataObject_t pd_transmit_objects_[7];/* Data objects to send */
  doDataObject_t caps_sink_[7];          /* Power object defs of the snk caps */
  doDataObject_t caps_source_[7];        /* Power object defs of the src caps */
  doDataObject_t caps_received_[7];      /* Last power objects received */
  doDataObject_t usb_pd_contract_;       /* Current USB PD contract (req obj) */
  doDataObject_t sink_request_;          /* Sink request message */
  FSC_U32 sink_selected_voltage_;        /* Sink request voltage */
  FSC_U32 sink_request_max_voltage_;     /* Max voltage the sink will request */
  FSC_U32 sink_request_max_power_;       /* Max power the sink will request */
  FSC_U32 sink_request_op_power_;        /* Op power the snk will request */
  FSC_U32 sink_partner_max_power_;       /* Max power advert'd by src partner */
  FSC_BOOL sink_request_low_power_;      /* Select the lower power PDO? */
  FSC_BOOL sink_goto_min_compatible_;    /* GotoMin command supported */
  FSC_BOOL sink_usb_suspend_compatible_; /* Sink suspend */
                                         /* operation during USB suspend */
  FSC_BOOL sink_usb_comm_capable_;       /* USB coms capable */
  doDataObject_t partner_caps_;          /* Partner's Sink Capabilities */

  FSC_U32 pd_HV_option_;                 /* Supported high voltage value */
  FSC_U32 policy_timeout;

  FSC_BOOL ams;

  FSC_U8 flag;

#ifdef FSC_HAVE_VDM
  /* VDM-specific data members */
  struct TimerObj vdm_timer_;            /* VDM-specific timer */
  PolicyState_t vdm_next_ps_;            /* Next VDM policy state */
  PolicyState_t original_policy_state_;
  FSC_BOOL expecting_vdm_response_;      /* True if expecting a VDM response */
  FSC_BOOL sending_vdm_data_;
  VdmDiscoveryState_t auto_vdm_state_;
  FSC_U32 vdm_msg_length_;
  SopType vdm_msg_tx_sop_;
  doDataObject_t partner_id_header_vdo;
  doDataObject_t partner_cert_stat_vdo;
  doDataObject_t partner_product_vdo;
  doDataObject_t partner_product_type_vdo;

  /* VDM handling data used by VDM handling example */
  FSC_BOOL svid_enable_;
  FSC_BOOL mode_enable_;
  FSC_U16 my_svid_;
  FSC_U32 my_mode_;
  FSC_BOOL mode_entered_;
  SvidInfo core_svid_info_;
  FSC_U16 auto_mode_disc_tracker_;

#ifdef FSC_HAVE_DP
  DisplayPortData_t display_port_data_; /* struct containing relevant DP data */
#endif /* FSC_HAVE_DP */
#endif /* FSC_HAVE_VDM */

#ifdef FSC_LOGGING
  /* Log object */
  struct Log log_;
#endif /* FSC_LOGGING */

#if defined(FSC_DEBUG) || defined(FSC_HAVE_USBHID)
  FSC_BOOL source_caps_updated_;  /* Signal GUI that source caps have changed */
#endif /* defined(FSC_DEBUG) || defined(FSC_HAVE_USBHID) */

  FSC_BOOL pd_notified_;
  FSC_BOOL vbus_enabled_;

  FSC_BOOL factory_mode_;
  FSC_BOOL double56k;
  FSC_BOOL double_check;
  FSC_BOOL c2a_cable_;
  FSC_U32 redobc12_cnt;
}; /* struct Port */

/* Initialize the port and hardware interface. */
/* Note: Must be called after hardware setup is complete (including I2C coms) */
void FUSB3601_InitializeVars(struct Port *port, FSC_U8 id, FSC_U8 i2c_addr);
void FUSB3601_InitializePort(struct Port *port);

/* Watchdog Feed Interrupt Function */
void FUSB3601_FeedTheDog(struct Port *port);

/* Register Update Functions */
FSC_BOOL FUSB3601_ReadRegister(struct Port *port, enum RegAddress regaddress);
FSC_BOOL FUSB3601_ReadRegisters(struct Port *port, enum RegAddress regaddr, FSC_U8 cnt);
void FUSB3601_ReadStatusRegisters(struct Port *port);
void FUSB3601_ReadAllRegisters(struct Port *port);
void FUSB3601_ReadRxRegisters(struct Port *port, FSC_U8 numbytes);
void FUSB3601_WriteRegister(struct Port *port, enum RegAddress regaddress);
void FUSB3601_WriteRegisters(struct Port *port, enum RegAddress regaddress, FSC_U8 cnt);
void FUSB3601_WriteTxRegisters(struct Port *port, FSC_U8 numbytes);
void FUSB3601_ClearInterrupt(struct Port *port, enum RegAddress address, FSC_U8 mask);
void FUSB3601_SendCommand(struct Port *port, enum DeviceCommand cmd);

/* *** Type-C Functionality */
void FUSB3601_SetRpValue(struct Port *port, USBTypeCCurrent currentVal);
void FUSB3601_UpdateSourceCurrent(struct Port *port, USBTypeCCurrent currentVal);
void FUSB3601_UpdateSinkCurrent(struct Port *port);

void FUSB3601_SetVBusSource5V(struct Port *port, FSC_BOOL enable);
FSC_U16 FUSB3601_GetVBusVoltage(struct Port *port);
FSC_BOOL FUSB3601_IsVbusVSafe0V(struct Port *port);
FSC_BOOL FUSB3601_IsVbusVSafe5V(struct Port *port);
FSC_BOOL FUSB3601_IsVbusOverVoltage(struct Port *port, FSC_U8 voltage);

/* Helper functions to set the level registers
 * level is in 25mV steps - call using macros defined above
 * e.g. SetVBusSnkDisc(port,FSC_VBUS_LVL_DISC(FSC_VBUS_05_V));
 */
void FUSB3601_SetVBusSnkDisc(struct Port *port, FSC_U16 level);
void FUSB3601_SetVBusStopDisc(struct Port *port, FSC_U16 level);
void FUSB3601_SetVBusAlarm(struct Port *port, FSC_U16 levelL, FSC_U16 levelH);

#ifdef FSC_HAVE_SNK
FSC_BOOL FUSB3601_IsVbusUnder5V(struct Port *port);
#endif /* FSC_HAVE_SNK */

CCTermType FUSB3601_DecodeCCTermination(struct Port *port);
#ifdef FSC_HAVE_DRP
CCTermType FUSB3601_DecodeCCTerminationDRP(struct Port *port);
#endif /* FSC_HAVE_DRP */

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
CCTermType FUSB3601_DecodeCCTerminationSource(struct Port *port);
void FUSB3601_SetStateSource(struct Port *port, FSC_BOOL vconn);

#endif /* FSC_HAVE_SRC || (FSC_HAVE_SNK && FSC_HAVE_ACC) */
#ifdef FSC_HAVE_SNK
CCTermType FUSB3601_DecodeCCTerminationSink(struct Port *port);
void FUSB3601_SetStateSink(struct Port *port);
#endif /* FSC_HAVE_SNK */

void FUSB3601_ResetDebounceVariables(struct Port *port);
void FUSB3601_DebounceCC(struct Port *port);
void FUSB3601_DetectCCPin(struct Port *port);
void FUSB3601_DetectCCPinOriented(struct Port *port);
void FUSB3601_SetOrientation(struct Port *port);
void FUSB3601_UpdateVConnTermination(struct Port *port);
void FUSB3601_UpdateOrientation(struct Port *port);
void FUSB3601_ClearState(struct Port *port);

#ifdef FSC_HAVE_ACC
FSC_BOOL FUSB3601_CheckForAccessory(struct Port *port);
#endif /* FSC_HAVE_ACC */

void FUSB3601_set_policy_msg_tx_sop(struct Port *port, SopType sop);
SopType FUSB3601_DecodeSopFromPdMsg(FSC_U8 msg);

void FUSB3601_set_msg_id_count(struct Port *port, SopType sop, FSC_U32 count);
void FUSB3601_set_message_id(struct Port *port, SopType sop, FSC_U32 id);

void FUSB3601_PDDisable(struct Port *port);
void FUSB3601_PDEnable(struct Port *port, FSC_BOOL is_source);

void FUSB3601_ResetProtocolLayer(struct Port *port);
int FUSB3601_GetStateCC(struct Port *port);
/* Policy helpers */
void FUSB3601_set_policy_state(struct Port *port, PolicyState_t state);
void FUSB3601_f_set_policy_state(struct Port *port, void (*f)(struct Port *));

#ifdef FSC_HAVE_VDM
/* VDM-specific functionality */
void FUSB3601_set_vdm_msg_tx_sop(struct Port *port, SopType sop);
#endif /* FSC_HAVE_VDM */

/* Logging */
void FUSB3601_LogTCState(struct Port *port);
void FUSB3601_LogPEState(struct Port *port);
void FUSB3601_DisableRetries(struct Port *port);

void FUSB3601_PolicySourceStartupHelper(struct Port *port);
void FUSB3601_set_sbu_switch(struct Port *port, SbuSwitch_t SbuSwitch);
void FUSB3601_set_usb_switch(struct Port *port, CCOrientation orientation);
void FUSB3601_start_vbus_discharge(struct Port *port);
void FUSB3601_set_force_discharge(struct Port *port);
void FUSB3601_disable_vbus_adc(struct Port *port);
void FUSB3601_enable_vbus_adc(struct Port *port);
void reset_adc(struct Port *port);
void FUSB3601_set_vbus_detach(struct Port *port, VbusDetach_t enable);
void superswitch_dsm_report(int num);

#endif /* FSCPM_PORT_H_ */
