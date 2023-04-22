/* **************************************************************************
 * scp.cpp
 *
 * Implements the SCP functionality.
 * ************************************************************************** */

#include "scp.h"
#include "platform.h"

static FSC_U32 power(FSC_U32 base, FSC_U32 exp)
{
  FSC_U32 i, result = 1;
  for (i = 0; i < exp; i++)
    result *= base;
  return result;
}

void ConnectSCP(struct Port *port)
{
  if (port->registers_.Event1.TYPEA_PLGIN) {
    port->activemode_ = Mode_SCP_A;

    platform_log(port->port_id_, "Connect: SCP - A", -1);
  }
  else {
    port->activemode_ = Mode_SCP_B;

    platform_log(port->port_id_, "Connect: SCP - B", -1);
  }

  ClearInterrupt(port, regEVENT_1,
                 MSK_SCP_PLGIN | MSK_DPDN_PLGIN |
                 MSK_TYPEA_PLGIN | MSK_TYPEB_PLGIN);

  /* Disable timers for now */
  port->registers_.SCPEnable1.EN_ACTIMER = 0;
  port->registers_.SCPEnable1.EN_ATIMER = 0;
  port->registers_.SCPEnable1.EN_STIMER = 0;
  WriteRegister(port, regSCP_ENABLE1);

  /* Disable PD while using SCP */
  PDDisable(port);

  /* Mask the plug-in interrupt and unmask the disconnect interrupt */
  port->registers_.SCPInt1Msk.M_SCP_DTCT = 0;
  port->registers_.SCPInt1Msk.M_DPD_DTCT = 1;
  WriteRegister(port, regSCP_INT1_MSK);

  port->scp_waitonack_ = FALSE;

  port->registers_.RTCmd = 0;
  port->scp_state_ = SCP_RequestDetails;
  port->scp_substate_ = 0;
  port->scp_setcount_ = 0;
}

void ProcessSCP(struct Port *port)
{
  /* Detach? */
  if (port->registers_.Event2.I_DPD_DTCT) {
    ClearInterrupt(port, regEVENT_2, MSK_SCP_EVENT2_ALL);

    /* Unmask the plug-in interrupt and mask the disconnect interrupt */
    port->registers_.SCPInt1Msk.M_SCP_DTCT = 1;
    port->registers_.SCPInt1Msk.M_DPD_DTCT = 0;
    WriteRegister(port, regSCP_INT1_MSK);

    port->activemode_ = Mode_None;
    port->registers_.RTCmd = 0;

    platform_log(port->port_id_, "Disconnect: SCP", -1);
  }

  if (port->scp_waitonack_) {
    /* Waiting for a response - bypass state machine for now until it arrives */
    /* TODO - Timeout? */
    if (port->registers_.Event2.I_SCP_ACK) {
      ClearInterrupt(port, regEVENT_2, MSK_I_SCP_ACK);

      port->scp_waitonack_ = FALSE;

      platform_log(port->port_id_, "SCP ACK", -1);
    }
    else if (port->registers_.Event2.I_SCP_NACK) {
      ClearInterrupt(port, regEVENT_2, MSK_I_SCP_NACK);

      port->scp_waitonack_ = FALSE;
      port->scp_substate_ = 0;
      port->scp_state_ = SCP_Idle;
      port->registers_.RTCmd = 0;

      platform_log(port->port_id_, "SCP NACK", -1);
    }
    return;
  }

  switch (port->scp_state_) {
  case SCP_Disabled:
    /* No SCP device - should not get here */
    break;
  case SCP_RequestStatus:
    platform_log(port->port_id_, "SCP ReqStatus", -1);
    GetSCPStatusValues(port);
    break;
  case SCP_RequestDetails:
    platform_log(port->port_id_, "SCP ReqDetails", -1);
    GetSCPSupportedValues(port);
    break;
  case SCP_SelectVoltage:
    platform_log(port->port_id_, "SCP SelVoltage", -1);
    SetSCPVoltage(port);
    break;
  case SCP_SelectCurrent:
    platform_log(port->port_id_, "SCP SelCurrent", -1);
    SetSCPCurrent(port);
    break;
  case SCP_Idle:
    /* Nothing to be done.  Wait for disconnect or V/I change request */
    break;
  default:
    break;
  }
}

FSC_BOOL ReadSCP(struct Port *port, FSC_U8 addr, FSC_U8 length, FSC_U8 *data)
{
  FSC_U8 i;

  /* Error case - TODO */
  if (length > SCP_BUFFER_LENGTH) return TRUE;

  if (port->registers_.RTCmd == 0) {
    /* Start address */
    port->registers_.RTAddr = addr;
    WriteRegister(port, regRT_ADDR);

    if (length == 1) {
      /* Write the command to kick off the read */
      port->registers_.RTCmd = SingleByte_Rd;
      WriteRegister(port, regRT_CMD);
    }
    else {
      /* Write the length, then the command to kick off the read */
      port->registers_.RTBufferTx[0] = length;
      platform_i2c_write(port->port_id_, regRT_BUFFER_TX0, 1,
          &port->registers_.RTBufferTx[0]);

      port->registers_.RTCmd = MultiByte_Rd;
      WriteRegister(port, regRT_CMD);
    }

    port->scp_waitonack_ = TRUE;
  }
  else {
    /* Still waiting... */
    if (port->scp_waitonack_ == TRUE) return FALSE;

    /* We've sent a command and received a response */
    /* TODO Temp delay */
    platform_delay(100000);

    for (i = 0; i < length; ++i) {
      /* Update each receive register */
      platform_i2c_read(port->port_id_, regRT_BUFFER_RX0 + i, 1,
          &port->registers_.RTBufferRx[i]);
      data[i] = port->registers_.RTBufferRx[i];
    }

    /* Clear the command register (auto-clears in device) */
    port->registers_.RTCmd = 0;

    return TRUE;
  }

  return FALSE;
}

FSC_BOOL WriteSCP(struct Port *port, FSC_U8 addr, FSC_U8 length, FSC_U8 *data)
{
  FSC_U8 i;

  /* Error case - TODO */
  if (length > SCP_BUFFER_LENGTH) return TRUE;

  if (port->registers_.RTCmd == 0) {
    /* Start address */
    port->registers_.RTAddr = addr;
    WriteRegister(port, regRT_ADDR);

    if (length == 1) {
      port->registers_.RTBufferTx[0] = data[0];
      platform_i2c_write(port->port_id_, regRT_BUFFER_TX0, 1,
          &port->registers_.RTBufferTx[0]);

      /* Write the command last to kick off the write */
      port->registers_.RTCmd = SingleByte_Wr;
      WriteRegister(port, regRT_CMD);
    }
    else {
      port->registers_.RTBufferTx[0] = length;
      platform_i2c_write(port->port_id_, regRT_BUFFER_TX0, 1,
          &port->registers_.RTBufferTx[0]);

      for (i = 0; i < length; ++i) {
        port->registers_.RTBufferTx[1 + i] = data[i];
        platform_i2c_write(port->port_id_, regRT_BUFFER_TX1 + i, 1,
            &port->registers_.RTBufferTx[1 + i]);
      }

      /* Write the command last to kick off the write */
      port->registers_.RTCmd = MultiByte_Wr;
      WriteRegister(port, regRT_CMD);

      port->scp_waitonack_ = TRUE;
    }
  }
  else {
    /* Still waiting... */
    if (port->scp_waitonack_ == TRUE) return FALSE;

    /* Check ACK byte */
    ReadRegister(port, regRT_ACK_RX);
    platform_log(port->port_id_, "ACK RX: ", port->registers_.RTAckRx);

    /* We've sent a command and received a response */
    port->registers_.RTCmd = 0;

    return TRUE;
  }

  return FALSE;
}

void GetSCPStatusValues(struct Port *port)
{
  FSC_U8 addresses[] = {0x80, 0x81, 0xA2, 0xA3, 0x00};
  FSC_U8 value;
  FSC_U32 tmp;

  if (ReadSCP(port, addresses[port->scp_substate_], 1, &value))
  {
    tmp = ((FSC_U32)addresses[port->scp_substate_] << 16) +
        port->registers_.RTBufferRx[0];

    platform_log(port->port_id_, "SCP Status: ", tmp);

    port->scp_substate_ += 1;

     if (addresses[port->scp_substate_] == 0x00) {
       /* Move on to Request Details */
       port->scp_substate_ = 0;
       port->scp_state_ = SCP_Idle;
     }
  }
}

void GetSCPSupportedValues(struct Port *port)
{
  FSC_U8 buffer[6] = {0};

  if (ReadSCP(port, MIN_VOUT, 6, buffer)) {
    port->scp_minV = buffer[0];
    port->scp_maxV = buffer[1];
    port->scp_minI = buffer[2];
    port->scp_maxI = buffer[3];
    port->scp_stepV = buffer[4];
    port->scp_stepI = buffer[5];

    platform_log(port->port_id_, "SCP Min V ", port->scp_minV);
    platform_log(port->port_id_, "SCP Max V ", port->scp_maxV);
    platform_log(port->port_id_, "SCP Min I ", port->scp_minI);
    platform_log(port->port_id_, "SCP Max I ", port->scp_maxI);
    platform_log(port->port_id_, "SCP Stp V ", port->scp_stepV);
    platform_log(port->port_id_, "SCP Stp I ", port->scp_stepI);

    /* Move on to voltage selection */
    port->scp_substate_ = 0;
    port->scp_state_ = SCP_SelectVoltage;
  }
}

void SetSCPVoltage(struct Port *port)
{
  FSC_U8 buffer[2];
  FSC_U16 minVmv = 0;
  FSC_U16 maxVmv = 0;

  FSC_U16 reqVoltage = 5500; /* Request 5V for now */

  /* Refer to SCP spec for this formula */
  minVmv = power(10, (port->scp_minV & 0xC0) >> 6) * (port->scp_minV & 0x3F);
  maxVmv = power(10, (port->scp_maxV & 0xC0) >> 6) * (port->scp_maxV & 0x3F);

  /* Check the request and send the command */
  if (reqVoltage >= minVmv && reqVoltage <= maxVmv) {
      buffer[0] = (FSC_U8)((reqVoltage & 0xFF00) >> 8);
      buffer[1] = (FSC_U8)(reqVoltage & 0x00FF);

    if (WriteSCP(port, VSET_H, 2, buffer)) {
      /* Command sent, move on to current selection */
      port->scp_substate_ = 0;
      port->scp_state_ = SCP_Idle;

      platform_log(port->port_id_, "SCP Voltage Requested: ", reqVoltage);
    }
  }
  else {
    /* Requested value not supported. */
    /* Move on to current selection */
    port->scp_substate_ = 0;
    port->scp_state_ = SCP_Idle;

    platform_log(port->port_id_, "SCP Voltage not supported: ", reqVoltage);
  }
}


void SetSCPCurrent(struct Port *port)
{
  FSC_U8 buffer[2];
  FSC_U16 minIma = 0;
  FSC_U16 maxIma = 0;

  FSC_U16 reqCurrent = 1000; /* Request 1A for now */

  /* Refer to SCP spec for this formula */
  minIma = power(10, (port->scp_minI & 0xC0) >> 6) * (port->scp_minI & 0x3F);
  maxIma = power(10, (port->scp_maxI & 0xC0) >> 6) * (port->scp_maxI & 0x3F);

  /* Check the request and send the command */
  if (reqCurrent >= minIma && reqCurrent <= maxIma) {
    /* Request the voltage */
    if (WriteSCP(port, ISSET, 1, buffer)) {
      port->scp_substate_ = 0;
      port->scp_state_ = SCP_Idle;

      platform_log(port->port_id_, "SCP Current Requested: ", reqCurrent);
    }
  }
  else {
    /* Requested value not supported. */
    /* Move on to Idle */
    port->scp_substate_ = 0;
    port->scp_state_ = SCP_Idle;

    platform_log(port->port_id_, "SCP Current not supported: ", reqCurrent);
  }
}
