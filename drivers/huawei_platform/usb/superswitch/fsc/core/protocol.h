/*
 * protocol.h
 *
 * Defines the PD Protocol state machine functions
 */
#ifndef FSCPM_PROTOCOL_H_
#define FSCPM_PROTOCOL_H_

#include "platform.h"
#include "port.h"

void FUSB3601_USBPDProtocol(struct Port *port);
void FUSB3601_ProtocolGetRxPacket(struct Port *port);
void FUSB3601_ProtocolTransmitMessage(struct Port *port);
void FUSB3601_ProtocolSendingMessage(struct Port *port);
void FUSB3601_ProtocolSendHardReset(struct Port *port);
SopType FUSB3601_TokenToSopType(FSC_U8 data);
FSC_U8 FUSB3601_ProtocolSendData(struct Port *port, FSC_U8 message_type,
                      FSC_U8 num_data_objects,
                      doDataObject_t *data_objects, FSC_U32 response_timeout,
                      PolicyState_t error_state, FSC_BOOL ams, SopType sop);
void FUSB3601_PolicySendVDM(struct Port *port, Command command, FSC_U16 svid, SopType sop, FSC_U8 obj_pos);

#endif /* FSCPM_PROTOCOL_H_ */

