/* **************************************************************************
 * accp.h
 *
 * Defines the ACCP spec implementation.
 * ************************************************************************** */
#ifndef FSC_ACCP_H_
#define FSC_ACCP_H_

#include "platform.h"
#include "port.h"

enum ACCP_REGISTERS {
  DVCTYPE               = 0x00, /* R   Type of slave device */
  SPEC_VER              = 0x01, /* R   ACCP specification version */
  SSTAT                 = 0x03, /* RR  Slave status register */
  ID_OUI0               = 0x04, /* R   Unique ID. Differentiate slave caps */
  CAPABILITIES          = 0x20, /* R   ID what type of charger is connected */
  DISCRETE_CAPABILITIES = 0x21, /* R   ID how many V/I steps are supported */
  MAX_PWR               = 0x22, /* R   Defines the max output power */
  ADAPTER_STATUS        = 0x28, /* RR  Provides real-time status */
  VOUT_STATUS           = 0x29, /* R   Reflects the current output voltage */
  ICHG_STATUS           = 0x2A, /* R   Reflects the constant current setting */
  OUTPUT_CONTROL        = 0x2B, /* R/W Initiates output V and I limit changes */
  VOUT_CONFIG           = 0x2C, /* R/W Configures the output voltage */
  ICHG_CONFIG           = 0x2D, /* R/W Configures the constant current */
  DISCRETE_VOUT_0       = 0x30, /* R   Discrete output voltage #1 */
  DISCRETE_VOUT_1       = 0x31, /* R   Discrete output voltage #2 */
  DISCRETE_VOUT_2       = 0x32, /* R   Discrete output voltage #3 */
  DISCRETE_VOUT_3       = 0x33, /* R   Discrete output voltage #4 */
  DISCRETE_VOUT_4       = 0x34, /* R   Discrete output voltage #5 */
  DISCRETE_VOUT_5       = 0x35, /* R   Discrete output voltage #6 */
  DISCRETE_VOUT_6       = 0x36, /* R   Discrete output voltage #7 */
  DISCRETE_VOUT_7       = 0x37, /* R   Discrete output voltage #8 */
  DISCRETE_VOUT_8       = 0x38, /* R   Discrete output voltage #9 */
  DISCRETE_VOUT_9       = 0x39, /* R   Discrete output voltage #10 */
  DISCRETE_ICHG_0       = 0x50, /* R   Discrete constant current setting #1 */
  DISCRETE_ICHG_1       = 0x51, /* R   Discrete constant current setting #2 */
  DISCRETE_ICHG_2       = 0x52, /* R   Discrete constant current setting #3 */
  DISCRETE_ICHG_3       = 0x53, /* R   Discrete constant current setting #4 */
  DISCRETE_ICHG_4       = 0x54, /* R   Discrete constant current setting #5 */
  DISCRETE_ICHG_5       = 0x55, /* R   Discrete constant current setting #6 */
  DISCRETE_ICHG_6       = 0x56, /* R   Discrete constant current setting #7 */
  DISCRETE_ICHG_7       = 0x57, /* R   Discrete constant current setting #8 */
  DISCRETE_ICHG_8       = 0x58, /* R   Discrete constant current setting #9 */
  DISCRETE_ICHG_9       = 0x59  /* R   Discrete constant current setting #10 */
};

void ConnectACCP(struct Port *port);
void ProcessACCP(struct Port *port);

/* Generic read/write commands for ACCP.
 * Returns TRUE when operation is completed.
 */
FSC_BOOL ReadACCP(struct Port *port, FSC_U8 addr, FSC_U8 *data);
FSC_BOOL WriteACCP(struct Port *port, FSC_U8 addr, FSC_U8 *data);

/* ACCP Supports a list of 10 discrete voltage settings.
 * Each value is represented as (Volts * 10)
 * Arguments: Port object pointer
 */
void GetACCPSupportedVoltages(struct Port *port);
void SetACCPVoltage(struct Port *port);

/* ACCP Supports a list of 10 discrete charge current settings.
 * Each value is represented as (Current * 10)
 * Arguments: Port object pointer
 */
void GetACCPSupportedCurrents(struct Port *port);
void SetACCPCurrent(struct Port *port);

#endif /* FSC_ACCP_H_ */
