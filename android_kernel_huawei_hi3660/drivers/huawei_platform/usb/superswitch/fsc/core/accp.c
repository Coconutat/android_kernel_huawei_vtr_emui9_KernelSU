/* **************************************************************************
 * accp.cpp
 *
 * Implements the ACCP functionality.
 * ************************************************************************** */

#include "accp.h"
#include "platform.h"

void ConnectACCP(struct Port *port)
{
  FSC_U8 i;

  ClearInterrupt(port, regEVENT_1, MSK_ACCP_PLGIN);

  port->activemode_ = Mode_ACCP;

  /* Disable PD while using ACCP */
  PDDisable(port);

  /* Mask the plug-in interrupt and unmask the disconnect interrupt */
  port->registers_.SCPInt1Msk.M_SCP_DTCT = 0;
  port->registers_.SCPInt1Msk.M_DPD_DTCT = 1;
  WriteRegister(port, regSCP_INT1_MSK);

  /* Unmask the ACK interrupt */
  port->registers_.SCPInt2Msk.M_SCP_ACK = 1;
  WriteRegister(port, regSCP_INT2_MSK);

  port->accp_state_ = ACCP_RequestVoltages;
  port->accp_substate_ = 0;
  port->accp_waitonack_ = FALSE;

  for (i = 0; i < 10; ++i) {
    port->accp_supported_V_[i] = 0;
    port->accp_supported_I_[i] = 0;
  }

  platform_log(port->port_id_, "Connect: ACCP", -1);
}

void ProcessACCP(struct Port *port)
{
  /* Detach? */
  if (port->registers_.Event2.I_DPD_DTCT) {
    ClearInterrupt(port, regEVENT_2, MSK_I_DPD_DTCT);

    /* Unmask the plug-in interrupt and mask the disconnect interrupt */
    port->registers_.SCPInt1Msk.M_SCP_DTCT = 1;
    port->registers_.SCPInt1Msk.M_DPD_DTCT = 0;
    WriteRegister(port, regSCP_INT1_MSK);

    /* Clear the command register (auto-clears in device) */
    port->registers_.RTCmd = 0;

    port->activemode_ = Mode_None;

    platform_log(port->port_id_, "Disconnect: ACCP", -1);
  }

  if (port->accp_waitonack_) {
    /* Waiting for a response - bypass state machine for now until it arrives */
    /* TODO - Timeout? */
    if (port->registers_.Event2.I_SCP_ACK) {
      ClearInterrupt(port, regEVENT_2, MSK_I_SCP_ACK);

      port->accp_waitonack_ = FALSE;

      /* Clear the command register (auto-clears in device) */
      port->registers_.RTCmd = 0;
    }
    else {
      return;
    }
  }

  switch (port->accp_state_) {
  case ACCP_Disabled:
    /* No ACCP device - should not get here */
    break;
  case ACCP_RequestVoltages:
    GetACCPSupportedVoltages(port);
    break;
  case ACCP_SelectVoltage:
    SetACCPVoltage(port);
    break;
  case ACCP_RequestCurrents:
    GetACCPSupportedCurrents(port);
    break;
  case ACCP_SelectCurrent:
    SetACCPCurrent(port);
    break;
  case ACCP_Idle:
    /* Nothing to be done.  Wait for disconnect or V/I change request */
    break;
  default:
    break;
  }
}

FSC_BOOL ReadACCP(struct Port *port, FSC_U8 addr, FSC_U8 *data)
{
  if (port->registers_.RTCmd == 0) {
    /* Start address */
    port->registers_.RTAddr = addr;
    WriteRegister(port, regRT_ADDR);

    /* Write the command to kick off the read */
    port->registers_.RTCmd = SingleByte_Rd;
    WriteRegister(port, regRT_CMD);

    port->accp_waitonack_ = TRUE;
  }
  else {
    /* Still waiting... */
    if (port->accp_waitonack_ == TRUE) return FALSE;

    platform_i2c_read(port->port_id_, regRT_BUFFER_RX0, 1,
        &port->registers_.RTBufferRx[0]);
    *data = port->registers_.RTBufferRx[0];

    /* Clear the command register (auto-clears in device) */
    port->registers_.RTCmd = 0;

    return TRUE;
  }

  return FALSE;
}

FSC_BOOL WriteACCP(struct Port *port, FSC_U8 addr, FSC_U8 *data)
{
  if (port->registers_.RTCmd == 0) {
    /* Start address */
    port->registers_.RTAddr = addr;
    WriteRegister(port, regRT_ADDR);

    port->registers_.RTBufferTx[0] = *data;
    platform_i2c_write(port->port_id_, regRT_BUFFER_TX0, 1,
        &port->registers_.RTBufferTx[0]);

    /* Write the command last to kick off the write */
    port->registers_.RTCmd = SingleByte_Wr;
    WriteRegister(port, regRT_CMD);

    port->accp_waitonack_ = TRUE;
  }
  else {
    /* Still waiting... */
    if (port->accp_waitonack_ == TRUE) return FALSE;

    /* Check ACK byte */
    ReadRegister(port, regRT_ACK_RX);
    platform_log(port->port_id_, "ACK RX: ", port->registers_.RTAckRx);

    /* We've sent a command and received a response */
    port->registers_.RTCmd = 0;

    return TRUE;
  }

  return FALSE;
}

void GetACCPSupportedVoltages(struct Port *port)
{
  FSC_U8 data;

  if (port->accp_substate_ < 10) {
    /* Request each supported voltage option */
    if (ReadACCP(port, DISCRETE_VOUT_0 + port->accp_substate_, &data)) {
      port->accp_supported_V_[port->accp_substate_] = data;

      if (port->accp_supported_V_[port->accp_substate_] == 0) {
        port->accp_substate_ = 0;
        port->accp_state_ = ACCP_SelectVoltage;
      }
      else {
        port->accp_substate_ += 1;
      }
    }
  }
  else {
    /* Have requested all available voltages */
    port->accp_substate_ = 0;
    port->accp_state_ = ACCP_SelectVoltage;
  }
}

void SetACCPVoltage(struct Port *port)
{
  FSC_U8 i;
  FSC_U8 voltage_requested = 50; /* 5V */
  FSC_BOOL voltage_found = FALSE;

  switch (port->accp_substate_) {
  case 0: /* Selection */
    /* TODO - Need additional logic for voltage selection.
     * For now just select the 5V option, if available. */
    for (i = 0; i < 10; ++i) {
      if (port->accp_supported_V_[i] == voltage_requested) voltage_found = TRUE;
    }

    if (voltage_found) {
      /* Request the voltage */
      if (WriteACCP(port, VOUT_CONFIG, &voltage_requested)) {
        port->accp_substate_ += 1;
      }
    }
    else {
      /* Leave at the current/default voltage selection and move on. */
      port->accp_substate_ = 0;
      port->accp_state_ = ACCP_RequestCurrents;
    }
    break;

  case 1:
    /* Request the selected voltage transition */
    i = 0x01; /* This is the SET_OUT bit */

    if (WriteACCP(port, OUTPUT_CONTROL, &i)) {
      port->accp_substate_ += 1;
    }
    break;

  case 2:
  default:
    port->accp_substate_ = 0;
    port->accp_state_ = ACCP_RequestCurrents;
    break;
  }
}


void GetACCPSupportedCurrents(struct Port *port)
{
  FSC_U8 data;

  if (port->accp_substate_ < 10) {
    /* Request each supported current option */
    if (ReadACCP(port, DISCRETE_ICHG_0 + port->accp_substate_, &data)) {
      port->accp_supported_I_[port->accp_substate_] = data;

      /* If the returned value is 0, we can move on */
      if (port->accp_supported_I_[port->accp_substate_] == 0) {
        port->accp_substate_ = 0;
        port->accp_state_ = ACCP_SelectCurrent;
      }
      else {
        port->accp_substate_ += 1;
      }
    }
  }
  else {
    /* Have requested all available current */
    port->accp_substate_ = 0;
    port->accp_state_ = ACCP_SelectCurrent;
  }
}


void SetACCPCurrent(struct Port *port)
{
  FSC_U8 i;
  FSC_U8 current_requested = 10; /* 1A */
  FSC_BOOL current_found = FALSE;

  switch (port->accp_substate_) {
  case 0: /* Selection */
    /* TODO - Need additional logic for current selection.
     * For now just select the 1A option, if available. */
    for (i = 0; i < 10; ++i) {
      if (port->accp_supported_I_[i] == current_requested) current_found = TRUE;
    }

    if (current_found) {
      /* Request the current */
      if (WriteACCP(port, ICHG_CONFIG, &current_requested)) {
        port->accp_substate_ += 1;
      }
    }
    else {
      /* Leave at the current/default voltage selection and move on. */
      port->accp_substate_ = 0;
      port->accp_state_ = ACCP_Idle;
    }
    break;

  case 1:
    /* Request the selected current transition */
    i = 0x01; /* This is the SET_OUT bit */

    if (WriteACCP(port, OUTPUT_CONTROL, &i)) {
      port->accp_substate_ += 1;
    }
    break;

  case 2:
  default:
    port->accp_substate_ = 0;
    port->accp_state_ = ACCP_Idle;
    break;
  }
}
