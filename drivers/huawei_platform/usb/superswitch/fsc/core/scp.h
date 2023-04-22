/* **************************************************************************
 * scp.h
 *
 * Defines the SCP spec implementation.
 * ************************************************************************** */
#ifndef FSC_SCP_H_
#define FSC_SCP_H_

#include "platform.h"
#include "port.h"

enum SCP_REGISTERS {
  DEBUG_RSVD       = 0x7F, /* R/W Debug register for Huawei */
  ADP_TYPE         = 0x80, /* R   Huawei charger type register */
  B_ADP_TYPE       = 0x81, /* R   Class B charger type */
  VENDOR_ID_H      = 0x82, /* R   Unique manufacturer ID */
  VENDOR_ID_L      = 0x83, /* R   Unique manufacturer ID */
  MODULE_ID_H      = 0x84, /* R   Class B model code */
  MODULE_ID_L      = 0x85, /* R   Class B model code */
  SERIAL_NO_H      = 0x86, /* R   Class B batch code */
  SERIAL_NO_L      = 0x87, /* R   Class B batch code */
  PCHIP_ID         = 0x88, /* R   Protocol chip model code */
  HWVER            = 0x89, /* R   Class B charger hardware version code */
  FWVER_H          = 0x8A, /* R   SCP protocol firmware version */
  FWVER_L          = 0x8B, /* R   SCP protocol firmware version */
  SPID             = 0x8C, /* R   Special adapter identification */
  MAX_POWER        = 0x90, /* R   Maximum rated output power */
  CNT_POWER        = 0x91, /* R   Constant output power */
  MIN_VOUT         = 0x92, /* R   Minimum supported output voltage */
  MAX_VOUT         = 0x93, /* R   Maximum supported output voltage */
  MIN_IOUT         = 0x94, /* R   Minimum supported current limit */
  MAX_IOUT         = 0x95, /* R   Maximum supported current limit */
  VSTEP            = 0x96, /* R   Voltage resolution */
  ISTEP            = 0x97, /* R   Current limit resolution */
  MAX_VERR         = 0x98, /* R   Maximum voltage error */
  MAX_IERR         = 0x99, /* R   Maximum current limit error */
  MAX_STTIME       = 0x9A, /* R   Maximum startup time */
  MAX_RSPTIME      = 0x9B, /* R   Maximum response time */
  SCTRL            = 0x9E, /* R   Special Control Register */
  STATUS_BYTE0     = 0xA2, /* R   Adapter status register 0 */
  STATUS_BYTE1     = 0xA3, /* R   Adapter status register 1 */
  INSIDE_TMP       = 0xA6, /* R   Class B charger internal temperature */
  PORT_TMP         = 0xA7, /* R   Class B charger port temp (i.e. ext NTC) */
  READ_VOUT_H      = 0xA8, /* R   ADC readout of the output voltage (MSB) */
  READ_VOUT_L      = 0xA9, /* R   ADC readout of the output voltage (LSB) */
  READ_IOUT_H      = 0xAA, /* R   ADC readout of the output current (MSB) */
  READ_IOUT_L      = 0xAB, /* R   ADC readout of the output current (LSB) */
  DAC_VSET_H       = 0xAC, /* R   Class B charger output V DAC step size MSB */
  DAC_VSET_L       = 0xAD, /* R   Class B charger output V DAC step size LSB */
  DAC_ISET_H       = 0xAE, /* R   Class B charger I limit DAC step size MSB */
  DAC_ISET_L       = 0xAF, /* R   Class B charger I limit DAC step size LSB */
  SREAD_VOUT       = 0xC8, /* R   Single byte val - output V from the ADC */
  SREAD_IOUT       = 0xC9, /* R   Single byte val - output I from the ADC */
  CTRL_BYTE0       = 0xA0, /* R/W SCP protocol control register */
  CTRL_BYTE1       = 0xA1, /* R/W SCP protocol control register */
  VSET_BOUND_H     = 0xB0, /* R/W Maximum output voltage limit (MSB) */
  VSET_BOUND_L     = 0xB1, /* R/W Maximum output voltage limit (LSB) */
  ISET_BOUND_H     = 0xB2, /* R/W Maximum current limit (MSB) */
  ISET_BOUND_L     = 0xB3, /* R/W Maximum current limit (LSB) */
  MAX_VSET_OFF     = 0xB4, /* R/W Set the maximum voltage offset */
  MAX_ISET_OFF     = 0xB5, /* R/W Set the maximum current limit offset */
  VSET_H           = 0xB8, /* R/W Set the target output voltage (MSB) */
  VSET_L           = 0xB9, /* R/W Set the target output voltage (LSB) */
  ISET_H           = 0xBA, /* R/W Set the current limit (MSB) */
  ISET_L           = 0xBB, /* R/W Set the current limit (LSB) */
  VSET_OFFSET_H    = 0xBC, /* W/C Set the output voltage offset (MSB) */
  VSET_OFFSET_L    = 0xBD, /* W/C Set the output voltage offset (LSB) */
  ISET_OFFSET_H    = 0xBE, /* W/C Set the current limit offset (MSB) */
  ISET_OFFSET_L    = 0xBF, /* W/C Set the current limit offset (LSB) */
  VSSET            = 0xCA, /* R/W Set the output voltage with a single byte */
  ISSET            = 0xCB, /* R/W Set the current limit with a single byte */
  STEP_VSET_OFFSET = 0xCC, /* W/C Set the max allowable V offset */
  STEP_ISET_OFFSET = 0xCD  /* W/C Set the max allowable I limit offset */
};

void ConnectSCP(struct Port *port);
void ProcessSCP(struct Port *port);

/* Generic read/write commands for SCP.
 * Returns TRUE when operation is completed.
 */
FSC_BOOL ReadSCP(struct Port *port, FSC_U8 addr, FSC_U8 length, FSC_U8 *data);
FSC_BOOL WriteSCP(struct Port *port, FSC_U8 addr, FSC_U8 length, FSC_U8 *data);

/* SCP supports a range of voltage and current settings with a fixed step size.
 * Arguments: Port object pointer
 */
void GetSCPStatusValues(struct Port *port);
void GetSCPSupportedValues(struct Port *port);
void SetSCPVoltage(struct Port *port);
void SetSCPCurrent(struct Port *port);

#endif /* FSC_SCP_H_ */
