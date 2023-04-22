/*
 * systempolicy.c
 *
 * Implements communication with system level operators/code and
 * provides access to the Type-C and PD world.
 */

#include "systempolicy.h"

#include "port.h"
#include "platform.h"

#ifdef FSC_HAVE_USBHID
#include "usbd_hid.h"
#include "hostcomm.h"
#endif /* FSC_HAVE_USBHID */

/* Character buffer for UART receive messages.  Used for debugging, etc. */
FSC_U8 UARTRecBuffer[64];
FSC_U8 UARTRecIndex = 0;
FSC_BOOL UARTRecHaveMsg = FALSE;

#ifdef FSC_HAVE_USBHID
/* Encode items into a byte array */
void FUSB3601_EncodeSystemPolicy(FSC_U8 *msg) {
}

/* Decode items from a byte array */
void FUSB3601_DecodeSystemPolicy(FSC_U8 *msg) {
}

/* This is a Hostcomm protocol extension for accessing system policy items */
void FUSB3601_SystemPolicyProcessMsg(FSC_U8 *inMsgBuffer, FSC_U8 *outMsgBuffer) {
  FSC_U32 i = 0;

  /* Clear the buffer to start */
  for (i = 0; i < FSC_HOSTCOMM_BUFFER_SIZE; ++i) {
    outMsgBuffer[i] = 0x00;
  }

  /* Echo the request */
  outMsgBuffer[0] = inMsgBuffer[0];

  /* Echo the port ID */
  outMsgBuffer[2] = inMsgBuffer[2];

  switch(inMsgBuffer[0]) {
  case CMD_SET_SYSTEM_POLICY:
    /* Set the system policy configuration */
    FUSB3601_DecodeSystemPolicy(&inMsgBuffer[3]);

    /* Return the current system policy */
    FUSB3601_EncodeSystemPolicy(&outMsgBuffer[3]);
    break;
  case CMD_GET_SYSTEM_POLICY:
    /* Return the current system policy */
    FUSB3601_EncodeSystemPolicy(&outMsgBuffer[3]);
    break;
  default:
    /* Return that the request is not implemented */
    outMsgBuffer[1] = 0x01;
    break;
  }
}
#endif /* FSC_HAVE_USBHID */

void FUSB3601_SystemPolicyProcess(struct Port *port) {
  /* Process any current HostComm interactions */
#ifdef FSC_HAVE_USBHID
  if (haveUSBInMsg) {
    /* Make sure the message goes to the correct port */
    switch (USBInputMsg[2]) {
      case 0xFF:
        FUSB3601_SystemPolicyProcessMsg(USBInputMsg, USBOutputMsg);
        break;
      case 0:
      case 1:
        /* HostComm */
        ProcessMsg(USBInputMsg, USBOutputMsg, port);
        break;
      default:
        break;
    }
    haveUSBInMsg = FALSE;
    USBD_HID_SendReport(&USBD_Device, USBOutputMsg, USB_MSG_LENGTH);
  }
#endif /* FSC_HAVE_USBHID */

  /* Look for incoming UART commands */
  if (UARTRecHaveMsg) {
/* For Example
 *    if (UARTRecBuffer[0] == 'r') {
 *      DoSomethingAboutR();
 *    }
 */

    UARTRecHaveMsg = FALSE;
    UARTRecIndex = 0;
  }
}
