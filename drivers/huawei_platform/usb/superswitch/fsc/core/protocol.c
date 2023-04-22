/*
 * protocol.c
 *
 * Implements the Protocol state machine functions
 */

#include "protocol.h"

#include "platform.h"
#include "port.h"
#include "timer.h"
#include "vendor_info.h"
#include "vdm.h"
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
#define LINE_FUNCTION hwlog_info("%s %d++\n", __func__,__LINE__);

void FUSB3601_USBPDProtocol(struct Port *port)
{
	/* Received hard reset? */
	if ((port->registers_.AlertL.I_RXHRDRST) || (port->registers_.AlertL.I_TXSUCC && port->registers_.AlertL.I_TXFAIL)){
		if ((port->registers_.AlertL.I_RXHRDRST)) {
			hwlog_info("receice a hard reset\n");
		} else {
			/*according to the TCPC spec, if we send a hardrest, TXSUCC And TXFAIL should be set both*/
			hwlog_info("send a hard reset\n");
		}
		port->auto_vdm_state_ = AUTO_VDM_INIT;
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_RXHRDRST | MSK_I_TXSUCC | MSK_I_TXFAIL | MSK_I_TXDISC | MSK_I_RXSTAT);
		port->pd_tx_status_ = txIdle;
		if (port->policy_is_source_) {
			FUSB3601_TimerStart(&port->policy_state_timer_, ktPSHardReset);
			FUSB3601_set_policy_state(port, peSourceTransitionDefault);
		} else {
			FUSB3601_set_policy_state(port, peSinkTransitionDefault);
		}
	port->is_hard_reset_ = TRUE;
	port->policy_subindex_ = 0;

#ifdef FSC_LOGGING
	/* Store the hard reset */
		if(port->registers_.AlertL.I_RXHRDRST)
			FUSB3601_WritePDToken(&port->log_, FALSE, pdtHardReset);
#endif /* FSC_LOGGING */
	} else {
		if (port->registers_.AlertL.I_RXSTAT && !port->protocol_msg_rx_) {
			/* If we have received a message */
			FUSB3601_ProtocolGetRxPacket(port);
			/* Clear the interrupt here, as it also clears the RX data registers */
			FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_RXSTAT);
			if(port->double_check) {
				FUSB3601_ProtocolGetRxPacket(port);
				FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_RXSTAT);
			}
			/* If we happened to get here by receiving a msg during a sinktx event,
			* receive the new msg first and then disable the attempted sinktx event.
			*/
			if (port->registers_.SinkTransmit.DIS_SNK_TX == 0) {
				port->registers_.SinkTransmit.DIS_SNK_TX = 1;
				FUSB3601_WriteRegister(port, regSINK_TRANSMIT);
			}
		}
		FUSB3601_ProtocolSendingMessage(port);
	}
}

void FUSB3601_ProtocolGetRxPacket(struct Port *port)
{
	FSC_U8 i = 0, j = 0;
	SopType rx_sop;

	port->double_check = 0;

#ifdef FSC_LOGGING
	sopMainHeader_t temp_header = {0};
#endif /* FSC_LOGGING */
	/* Read the Rx token, two header bytes, and the byte count */
	FUSB3601_ReadRegisters(port, regRXSTAT, 3);

	port->policy_rx_header_.byte[0] = port->registers_.RxHeadL;
	port->policy_rx_header_.byte[1] = port->registers_.RxHeadH;

	FUSB3601_platform_log(port->port_id_, "FUSB Rx Header 0:", port->policy_rx_header_.byte[0]);
	FUSB3601_platform_log(port->port_id_, "FUSB Rx Header 1:", port->policy_rx_header_.byte[1]);

#ifdef FSC_LOGGING
	/* Only setting the Tx header here so that we can store what we */
	/* expect was sent in our PD buffer for the GUI */
	temp_header.word = 0;
	temp_header.NumDataObjects = 0;
	temp_header.MessageType = CMTGoodCRC;
	temp_header.PortDataRole = port->policy_is_dfp_;
	temp_header.PortPowerRole = port->policy_is_source_;
	temp_header.SpecRevision = USBPDSPECREV;
	temp_header.MessageID = port->policy_rx_header_.MessageID;
#endif /* FSC_LOGGING */

	/* Figure out what SOP* the data came in on */
	rx_sop = FUSB3601_TokenToSopType(port->registers_.RxStat.RX_SOP);

	/* Make sure we support this SOP */
	if ((rx_sop == SOP_TYPE_SOP) || (rx_sop == SOP_TYPE_SOP1)) {
		if ((port->policy_rx_header_.NumDataObjects == 0) &&
			(port->policy_rx_header_.MessageType == CMTSoftReset)) {
			/* Clear the message ID counter for tx */
			port->message_id_counter_[rx_sop] = 0;
			/* Reset the message ID (always alert policy engine of soft reset) */
			port->message_id_[rx_sop] = 0xFF;
			port->protocol_msg_rx_sop_ = rx_sop;

			/* Set the flag to pass the message to the policy engine */
			port->protocol_msg_rx_ = TRUE;
#ifdef FSC_HAVE_USBHID
			/* Set the source caps updated flag to trigger a GUI update */
			port->source_caps_updated_ = TRUE;
#endif /* FSC_HAVE_USBHID */
		} else if (port->policy_rx_header_.MessageID != port->message_id_[rx_sop]) {
			/* If the message ID does not match the stored value */
			/* Update the stored message ID */
			port->message_id_[rx_sop] = port->policy_rx_header_.MessageID;
			port->protocol_msg_rx_sop_ = rx_sop;

			/* Set the flag to pass the message to the policy engine */
			port->protocol_msg_rx_ = TRUE;
		} else {
			FUSB3601_platform_log(port->port_id_, "FUSB Protocol duplicate Message ID Received: ", port->policy_rx_header_.MessageID);
			port->protocol_msg_rx_ = FALSE;
			port->double_check = 1;
		}
	}

	if (port->policy_rx_header_.NumDataObjects > 0) {
		/* Did we receive a data message? If so, we want to retrieve the data */
		FUSB3601_ReadRxRegisters(port, port->policy_rx_header_.NumDataObjects << 2);
		/* Load the FIFO data into the data objects (loop through each object) */
		for (i = 0; i < port->policy_rx_header_.NumDataObjects; i++) {
		/* Loop through each byte in the object */
			for (j = 0; j < 4; j++) {
				/* Store the actual bytes */
				port->policy_rx_data_obj_[i].byte[j] = port->registers_.RxData[(i * 4) + j];
			}
		}
	}

#ifdef FSC_LOGGING
	/* Store the received PD message */
	FUSB3601_WritePDMsg(&port->log_, port->policy_rx_header_, port->policy_rx_data_obj_, FALSE, rx_sop);

	/* Store the (recreated) GoodCRC message that we have sent (SOP) */
	FUSB3601_WritePDMsg(&port->log_, temp_header, 0, TRUE, rx_sop);
#endif /* FSC_LOGGING */

}

void FUSB3601_ProtocolTransmitMessage(struct Port *port)
{
	FSC_U8 i = 0, j = 0;
	sopMainHeader_t temp_PolicyTxHeader = {0};
	temp_PolicyTxHeader.word = port->policy_tx_header_.word;
	temp_PolicyTxHeader.word &= 0x7FEF; /* Clear reserved bits */

	if ((temp_PolicyTxHeader.NumDataObjects == 0) && (temp_PolicyTxHeader.MessageType == CMTSoftReset)) {
		/* Clear the message ID counter if transmitting a soft reset */
		port->message_id_counter_[port->protocol_msg_tx_sop_] = 0;
		/* Reset the message ID if transmitting a soft reset */
		port->message_id_[port->protocol_msg_tx_sop_] = 0xFF;

#ifdef FSC_HAVE_USBHID
		/* Set the source caps updated flag to trigger an update of the GUI */
		port->source_caps_updated_ = TRUE;
#endif /* FSC_HAVE_USBHID */
	}

	/* Update the tx message id to send */
	temp_PolicyTxHeader.MessageID =
	port->message_id_counter_[port->protocol_msg_tx_sop_];
	/* Load the TXBYTECNT register with the number of bytes in the packet */
	port->registers_.TxByteCnt = 2 + (temp_PolicyTxHeader.NumDataObjects << 2);
	/* Load in the first byte of the header */
	port->registers_.TxHeadL = temp_PolicyTxHeader.byte[0];
	/* Load in the second byte of the header */
	port->registers_.TxHeadH = temp_PolicyTxHeader.byte[1];
	FUSB3601_WriteRegisters(port, regTXBYTECNT, 3);

	if (temp_PolicyTxHeader.NumDataObjects > 0) {
		/* If this is a data object... */
		for (i = 0; i < temp_PolicyTxHeader.NumDataObjects; i++) {
			/* Load the data objects */
			for (j = 0; j < 4; ++j) {
				/* Loop through each byte in the object */
				port->registers_.TxData[(i * 4) + j] =
			port->policy_tx_data_obj_[i].byte[j];
			}
		}
	}

	/* Commit the TxData array to the device */
	FUSB3601_WriteTxRegisters(port, temp_PolicyTxHeader.NumDataObjects << 2);

	/* JIRA - 297 Test */
	if((temp_PolicyTxHeader.NumDataObjects !=0) && (temp_PolicyTxHeader.MessageType == DMTVendorDefined)) {
		FUSB3601_platform_delay(500);
	}

	/* Send the SOP indicator to enable the transmitter */
	//TODO: RetryCnt is always 3 -> make define and put in init
	if (port->protocol_use_sinktx_ == FALSE) {
		port->registers_.Transmit.TX_SOP = port->protocol_msg_tx_sop_;
		port->registers_.Transmit.RETRY_CNT = 3;
		FUSB3601_WriteRegister(port, regTRANSMIT);
		/* Disable SinkTX for normal transmits */
		if (port->registers_.SinkTransmit.DIS_SNK_TX == 0) {
			port->registers_.SinkTransmit.DIS_SNK_TX = 1;
			FUSB3601_WriteRegister(port, regSINK_TRANSMIT);
		}
	} else {
		port->registers_.SinkTransmit.DIS_SNK_TX = 0;
		port->registers_.SinkTransmit.TRANSMIT_SOP = port->protocol_msg_tx_sop_;
		port->registers_.SinkTransmit.RETRY_CNT = 3;
		FUSB3601_WriteRegister(port, regSINK_TRANSMIT);
		// Clear for next time...
		port->protocol_use_sinktx_ = FALSE;
	}

#ifdef FSC_LOGGING
	/* Store all messages that we attempt to send for debugging */
	FUSB3601_WritePDMsg(&port->log_, temp_PolicyTxHeader, port->policy_tx_data_obj_, TRUE, port->protocol_msg_tx_sop_);
#endif /* FSC_LOGGING */

	if(port->flag == 0) {
		port->flag = 1;
	}
}

void FUSB3601_ProtocolSendingMessage(struct Port *port)
{
	SopType rx_sop = port->protocol_msg_tx_sop_;
	SopType sop = 0;

#ifdef FSC_LOGGING
	sopMainHeader_t temp_header = {0};
#endif /* FSC_LOGGING */

	if (port->registers_.AlertL.I_TXSUCC) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_TXSUCC);
		FUSB3601_platform_log(port->port_id_, "FUSB Tx Success", -1);
		FUSB3601_WritePEState(&port->log_, 210, port->policy_state_);

#ifdef FSC_LOGGING
	/* The 305 doesn't provide received goodcrc messages, */
	/* so create a temporary one here for logging. */
	/* TODO - might need to use the opposite for dfp/source */
	temp_header.MessageType = CMTGoodCRC;
	temp_header.PortDataRole =  port->policy_is_dfp_;
	temp_header.SpecRevision = USBPDSPECREV;
	temp_header.PortPowerRole =  port->policy_is_source_;
	temp_header.MessageID = port->message_id_counter_[rx_sop];
	temp_header.NumDataObjects = 0;
	FUSB3601_WritePDMsg(&port->log_, temp_header, 0, FALSE, rx_sop);
#endif /* FSC_LOGGING */

	/* Transmission successful */
	port->message_id_counter_[rx_sop] = (port->message_id_counter_[rx_sop] + 1) & 0x07;
	port->pd_tx_status_ = txSuccess;
	FUSB3601_TimerStart(&port->policy_state_timer_, port->policy_timeout);
	} else if (port->registers_.AlertL.I_TXFAIL) {
		FUSB3601_platform_log(port->port_id_, "FUSB Tx Fail", -1);
		FUSB3601_WritePEState(&port->log_, 211, port->policy_state_);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_TXFAIL);
		port->expecting_vdm_response_ = FALSE;
		FUSB3601_set_policy_state(port, port->fail_state);
		port->pd_tx_status_ = txIdle;
#ifdef FSC_LOGGING
		/* The 305 doesn't provide received goodcrc messages, */
		/* so create a temporary one here for logging. */
		/* TODO - might need to use the opposite for dfp/source */
		sop = SOP_TYPE_SOP1_DEBUG;
		temp_header.MessageType = CMTGoodCRC;
		temp_header.PortDataRole =  port->policy_is_dfp_;
		temp_header.SpecRevision = USBPDSPECREV;
		temp_header.PortPowerRole =  port->policy_is_source_;
		temp_header.MessageID = port->message_id_counter_[rx_sop];
		temp_header.NumDataObjects = 0;
		FUSB3601_WritePDMsg(&port->log_, temp_header, 0, FALSE, sop);
#endif /* FSC_LOGGING */
	} else if(port->registers_.AlertL.I_TXDISC) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_TXDISC);
		FUSB3601_platform_log(port->port_id_, "FUSB Tx Discard", -1);
		sop = SOP_TYPE_SOP2_DEBUG;
		FUSB3601_WritePEState(&port->log_, 212, port->policy_state_);
		port->expecting_vdm_response_ = FALSE;
		if(port->ams)
			FUSB3601_set_policy_state(port, port->fail_state);
		else if(port->source_or_sink_ == Source)
			FUSB3601_set_policy_state(port, peSourceReady);
		else if(port->source_or_sink_ == Sink)
			FUSB3601_set_policy_state(port, peSinkReady);
		port->pd_tx_status_ = txIdle;
#ifdef FSC_LOGGING
		/* The 305 doesn't provide received goodcrc messages, */
		/* so create a temporary one here for logging. */
		/* TODO - might need to use the opposite for dfp/source */
		temp_header.MessageType = CMTGoodCRC;
		temp_header.PortDataRole =  port->policy_is_dfp_;
		temp_header.SpecRevision = USBPDSPECREV;
		temp_header.PortPowerRole =  port->policy_is_source_;
		temp_header.MessageID = port->message_id_counter_[rx_sop];
		temp_header.NumDataObjects = 0;
		FUSB3601_WritePDMsg(&port->log_, temp_header, 0, FALSE, sop);
#endif /* FSC_LOGGING */
	} else if((port->pd_tx_status_ == txSend) && FUSB3601_TimerExpired(&port->policy_state_timer_)) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_ProtocolSendHardReset(port);
	}
}

void FUSB3601_ProtocolSendHardReset(struct Port *port)
{
	/* Set the send hard reset TRANSMIT register code */
	FSC_U8 data = TRANSMIT_HARDRESET;

	/* Send the hard reset */
	hwlog_info("%s\n", __func__);
	FUSB3601_platform_i2c_write(port->port_id_, regTRANSMIT, 1, &data);

#ifdef FSC_LOGGING
	/* Store the hard reset */
	FUSB3601_WritePDToken(&port->log_, TRUE, pdtHardReset);
#endif /* FSC_LOGGING */
}

SopType FUSB3601_TokenToSopType(FSC_U8 data)
{
	SopType ret = SOP_TYPE_ERROR;

	/* Figure out what SOP* the data came in on */
	/* The register value from the FUSB305 maps directly to our SOP_TYPE_ enum */
	if ((data & 0x0b00000111) <= SOP_TYPE_LAST_VALUE) {
		ret = (SopType)(data & 0x0b00000111);
	}
	return ret;
}

/* AMS implemented as "go to fail state if tx_disc" */
FSC_U8 FUSB3601_ProtocolSendData(struct Port *port, FSC_U8 message_type,
                      FSC_U8 num_data_objects,
                      doDataObject_t *data_objects, FSC_U32 response_timeout,
                      PolicyState_t error_state, FSC_BOOL ams, SopType sop)
{
	FSC_U8 i = 0;
	FSC_U8 status = STAT_BUSY;
	switch (port->pd_tx_status_) {
		case txIdle:
			if (num_data_objects > 7)
				num_data_objects = 7;

			port->policy_tx_header_.word = 0x0000;
			port->policy_tx_header_.NumDataObjects = num_data_objects;
			port->policy_tx_header_.MessageType = message_type & 0x0F;
			port->policy_tx_header_.PortDataRole = port->policy_is_dfp_;
			port->policy_tx_header_.PortPowerRole = port->policy_is_source_;
			if(sop != SOP_TYPE_SOP) {
				port->policy_tx_header_.PortDataRole = 0;
				port->policy_tx_header_.PortPowerRole = 0;
			}
			port->policy_tx_header_.SpecRevision = USBPDSPECREV;
			for (i = 0; i < num_data_objects; i++) {
				port->policy_tx_data_obj_[i].object = data_objects[i].object;
			}
			if (port->policy_state_ == peSourceSendCaps)
				port->caps_counter_++;
			port->protocol_msg_tx_sop_ = sop;
			port->pd_tx_status_ = txSend;
			FUSB3601_ProtocolTransmitMessage(port);
			port->fail_state = error_state;
			port->policy_timeout = response_timeout;
			port->ams = ams;
			/* TODO: Should be ok to use this timer */
			FUSB3601_TimerStart(&port->policy_state_timer_, ktTxTimeout);
			break;
		case txSend:
		case txSuccess:
		default:
			break;
		}

	return status;
}

void FUSB3601_PolicySendVDM(struct Port *port, Command command, FSC_U16 svid, SopType sop, FSC_U8 obj_pos) {

	doDataObject_t vdmh = {0};
	FSC_U32 timeout;
	PolicyState_t fail_state;

	port->expecting_vdm_response_ = TRUE;

	port->protocol_check_rx_b4_tx_ = TRUE;

	vdmh.SVDM.SVID = svid;
	vdmh.SVDM.VDMType = STRUCTURED_VDM;
	vdmh.SVDM.Version = Structured_VDM_Version_SOP;
	vdmh.SVDM.ObjPos = obj_pos;
	vdmh.SVDM.CommandType = INITIATOR;
	vdmh.SVDM.Command = command;

	timeout = FUSB3601_GetVdmTimer(command);

	//TODO: Check this for all cases
	if(port->source_or_sink_ == Source)
		fail_state = peSourceSendSoftReset;
	else
		fail_state = peSinkSendSoftReset;

	//TODO: Temporary for SOP' DiscID Sends
	if(sop == SOP_TYPE_SOP1) {
		fail_state = peSourceDiscovery;
		FUSB3601_platform_log(port->port_id_, "FUSB Sending SOP' VDM, fail_state: ", fail_state);
	}

	FUSB3601_ProtocolSendData(port, DMTVendorDefined, 1, &vdmh,
			timeout, fail_state, FALSE, sop);

	/* Ready state will handle interruptions, timer expiring,
	 * and successful sending.
	 */

	//TODO: No ready state for SOP' disc id, source startup
}
