/* **************************************************************************
 * registers.h
 *
 * Defines register mappings for the FUSB3601.
 * ************************************************************************** */
#ifndef FSCPM_REGISTERS_H_
#define FSCPM_REGISTERS_H_

#include "platform.h"

/* Total number of registers in the device */
#define TOTAL_REGISTER_CNT 192

/* PD Rx/Tx buffer length in bytes */
#define COMM_BUFFER_LENGTH 28

/* SCP Rx/Tx buffer length in bytes */
#define SCP_BUFFER_LENGTH 16

/* Interrupt bit masks, organized by register */
/* Interrupt register masks: ALERTL */
#define MSK_I_CCSTAT          (1 << 0)
#define MSK_I_PORT_PWR        (1 << 1)
#define MSK_I_RXSTAT          (1 << 2)
#define MSK_I_RXHRDRST        (1 << 3)
#define MSK_I_TXFAIL          (1 << 4)
#define MSK_I_TXDISC          (1 << 5)
#define MSK_I_TXSUCC          (1 << 6)
#define MSK_I_VBUS_ALRM_HI    (1 << 7)
#define MSK_I_ALARM_LO_ALL    0xFF

/* Interrupt register masks: ALERTH */
#define MSK_I_VBUS_ALRM_LO    (1 << 0)
#define MSK_I_FAULT           (1 << 1)
#define MSK_I_RX_FULL         (1 << 2)
#define MSK_I_VBUS_SNK_DISC   (1 << 3)
#define MSK_I_ALARM_HI_ALL    0x0F

/* PWRSTAT register masks */
#define MSK_SNKVBUS				(1 << 0)
#define MSK_VCONN_VAL        		(1 << 1)
#define MSK_VBUS_VAL				(1 << 2)
#define MSK_VBUS_VAL_EN			(1 << 3)
#define MSK_SRC_VBUS				(1 << 4)
#define MSK_SRC_HV				(1 << 5)
#define MSK_INT					(1 << 6)
#define MSK_DEBUG_ACC				(1 << 7)
#define MSK_PWRSTATMSK_ALL		0xFF

/* Interrupt register masks: SCP_INT1_MSK */
#define MSK_M_DPD_DTCT        (1 << 0)
#define MSK_M_STMR            (1 << 1)
#define MSK_M_ATMR            (1 << 2)
#define MSK_M_SCP_NACK        (1 << 3)
#define MSK_M_SDATA           (1 << 4)
#define MSK_M_SCOM            (1 << 5)
#define MSK_M_SCP_DTCT        (1 << 7)
#define MSK_M_SCP_INT1_ALL    0xBF

/* Interrupt register masks: SCP_INT2_MSK */
#define MSK_M_ACK_PARRX       (1 << 0)
#define MSK_M_ACK_CRCRX       (1 << 1)
#define MSK_M_ACK_LGTH_CHK    (1 << 2)
#define MSK_M_I2C_LGTH_CHK    (1 << 3)
#define MSK_M_ALRM            (1 << 4)
#define MSK_M_SCP_ACK         (1 << 5)
#define MSK_M_CMD             (1 << 6)
#define MSK_M_SCP_INT2_ALL    0x7F

/* Interrupt register masks: SCP_EVENT_1 */
#define MSK_ACCP_PLGIN        (1 << 0)
#define MSK_TYPEB_PLGIN       (1 << 3)
#define MSK_TYPEA_PLGIN       (1 << 4)
#define MSK_DPDN_PLGIN        (1 << 5)
#define MSK_CC_PLGIN          (1 << 6)
#define MSK_SCP_PLGIN         (1 << 7)
#define MSK_SCP_EVENT1_ALL    0xF9

/* Interrupt register masks: SCP_EVENT_2 */
#define MSK_I_DPD_DTCT        (1 << 0)
#define MSK_I_STMR            (1 << 1)
#define MSK_I_ATMR            (1 << 2)
#define MSK_I_SCP_NACK        (1 << 3)
#define MSK_I_SCP_ALRM        (1 << 4)
#define MSK_I_SCP_ACK         (1 << 5)
#define MSK_I_CMD             (1 << 6)
#define MSK_SCP_EVENT2_ALL    0x7F

/* Interrupt register masks: SCP_EVENT_3 */
#define MSK_I_ACK_PARRX       (1 << 0)
#define MSK_I_ACK_CRCRX       (1 << 1)
#define MSK_I_ACK_LGTH_CHK    (1 << 2)
#define MSK_I_I2C_LGTH_CHK    (1 << 3)
#define MSK_I_SDATA           (1 << 4)
#define MSK_I_SCOM            (1 << 5)
#define MSK_SCP_EVENT3_ALL    0x3F

/* Interrupt register masks: MUS */
#define MSK_I_BC_ATTACH       (1 << 0)
#define MSK_I_BC_DETACH       (1 << 1)
#define MSK_I_H2O_DET		  (1 << 2)
#define MSK_I_OT_ERROR        (1 << 3)
#define MSK_I_FRSWAP_RX       (1 << 4)
#define MSK_I_SCP_EVENT       (1 << 5)
#define MSK_MUS_ALL           0x3F

/* Interrupt register masks: DATA OVP */
#define MSK_I_SBU_OVP         (1 << 0)
#define MSK_I_DPDN_BOT_OVP    (1 << 1)
#define MSK_I_DPDN_TOP_OVP    (1 << 2)
#define MSK_I_CC_OVP          (1 << 3)
#define MSK_OVP_ALL           0x0F

/* FaultStat register masks: FAULTSTAT */
#define MSK_I2C_ERROR         (1 << 0)
#define MSK_VCONN_OCP         (1 << 1)
#define MSK_VBUS_OVP          (1 << 2)
#define MSK_FORCE_DISCH_FAIL  (1 << 4)
#define MSK_AUTO_DISCH_FAIL   (1 << 5)
#define MSK_ALL_REGS_RESET    (1 << 7)
#define MSK_FAULTSTAT_ALL     0xB7

/* List of valid values to write to regCOMMAND */
enum DeviceCommand {
  WakeI2C             = 0b00010001,
  DisableVbusDetect   = 0b00100010,
  EnableVbusDetect    = 0b00110011,
  DisableSinkVbus     = 0b01000100,
  SinkVbus            = 0b01010101,
  DisableSourceVbus   = 0b01100110,
  SourceVbusDefaultV  = 0b01110111,
  SourceVbusHighV     = 0b10001000,
  Look4Con            = 0b10011001,
  RxOneMore           = 0b10101010,
  I2CIdle             = 0b11111111
};

/* TRANSMIT register command values */
#define TRANSMIT_HARDRESET      0b101
#define TRANSMIT_CABLERESET     0b110
#define TRANSMIT_BIST_CM2       0b111

/* ACCP/SCP Command Values */
enum ACCPSCPCommands {
  SingleByte_Wr = 0b00001011,
  SingleByte_Rd = 0b00001100,
  MultiByte_Wr  = 0b00011011,
  MultiByte_Rd  = 0b00011100
};

/* Register map for the FUSB3601 */
enum RegAddress {
  /* Product Registers */
  regVENDIDL                = 0x00,
  regVENDIDH                = 0x01,
  regPRODIDL                = 0x02,
  regPRODIDH                = 0x03,
  regDEVIDL                 = 0x04,
  regDEVIDH                 = 0x05,
  regTYPECREVL              = 0x06,
  regTYPECREVH              = 0x07,
  regUSBPDVER               = 0x08,
  regUSBPDREV               = 0x09,
  regPDIFREVL               = 0x0A,
  regPDIFREVH               = 0x0B,
  /* 0x0C - 0x0F Reserved */
  /* Type-C and PD Registers */
  regALERTL                 = 0x10,
  regALERTH                 = 0x11,
  regALERTMSKL              = 0x12,
  regALERTMSKH              = 0x13,
  regPWRSTATMSK             = 0x14,
  regFAULTSTATMSK           = 0x15,
  /* 0x16 - 0x17 Reserved */
  regSTD_OUT_CFG            = 0x18,
  regTCPC_CTRL              = 0x19,
  regROLECTRL               = 0x1A,
  regFAULTCTRL              = 0x1B,
  regPWRCTRL                = 0x1C,
  regCCSTAT                 = 0x1D,
  regPWRSTAT                = 0x1E,
  regFAULTSTAT              = 0x1F,
  /* 0x20 - 0x22 Reserved */
  regCOMMAND                = 0x23,
  regDEVCAP1L               = 0x24,
  regDEVCAP1H               = 0x25,
  regDEVCAP2L               = 0x26,
  regDEVCAP2H               = 0x27, /* Reserved */
  regSTD_IN_CAP             = 0x28, /* Reserved */
  regSTD_OUT_CAP            = 0x29,
  /* 0x2A - 0x2D Reserved */
  regMSGHEADR               = 0x2E,
  regRXDETECT               = 0x2F,
  regRXBYTECNT              = 0x30,
  regRXSTAT                 = 0x31,
  regRXHEADL                = 0x32,
  regRXHEADH                = 0x33,
  regRXDATA_00              = 0x34,
  regRXDATA_01              = 0x35,
  regRXDATA_02              = 0x36,
  regRXDATA_03              = 0x37,
  regRXDATA_04              = 0x38,
  regRXDATA_05              = 0x39,
  regRXDATA_06              = 0x3A,
  regRXDATA_07              = 0x3B,
  regRXDATA_08              = 0x3C,
  regRXDATA_09              = 0x3D,
  regRXDATA_10              = 0x3E,
  regRXDATA_11              = 0x3F,
  regRXDATA_12              = 0x40,
  regRXDATA_13              = 0x41,
  regRXDATA_14              = 0x42,
  regRXDATA_15              = 0x43,
  regRXDATA_16              = 0x44,
  regRXDATA_17              = 0x45,
  regRXDATA_18              = 0x46,
  regRXDATA_19              = 0x47,
  regRXDATA_20              = 0x48,
  regRXDATA_21              = 0x49,
  regRXDATA_22              = 0x4A,
  regRXDATA_23              = 0x4B,
  regRXDATA_24              = 0x4C,
  regRXDATA_25              = 0x4D,
  regRXDATA_26              = 0x4E,
  regRXDATA_27              = 0x4F,
  regTRANSMIT               = 0x50,
  regTXBYTECNT              = 0x51,
  regTXHEADL                = 0x52,
  regTXHEADH                = 0x53,
  regTXDATA_00              = 0x54,
  regTXDATA_01              = 0x55,
  regTXDATA_02              = 0x56,
  regTXDATA_03              = 0x57,
  regTXDATA_04              = 0x58,
  regTXDATA_05              = 0x59,
  regTXDATA_06              = 0x5A,
  regTXDATA_07              = 0x5B,
  regTXDATA_08              = 0x5C,
  regTXDATA_09              = 0x5D,
  regTXDATA_10              = 0x5E,
  regTXDATA_11              = 0x5F,
  regTXDATA_12              = 0x60,
  regTXDATA_13              = 0x61,
  regTXDATA_14              = 0x62,
  regTXDATA_15              = 0x63,
  regTXDATA_16              = 0x64,
  regTXDATA_17              = 0x65,
  regTXDATA_18              = 0x66,
  regTXDATA_19              = 0x67,
  regTXDATA_20              = 0x68,
  regTXDATA_21              = 0x69,
  regTXDATA_22              = 0x6A,
  regTXDATA_23              = 0x6B,
  regTXDATA_24              = 0x6C,
  regTXDATA_25              = 0x6D,
  regTXDATA_26              = 0x6E,
  regTXDATA_27              = 0x6F,
  regVBUS_VOLTAGE_L         = 0x70,
  regVBUS_VOLTAGE_H         = 0x71,
  regVBUS_SNK_DISCL         = 0x72,
  regVBUS_SNK_DISCH         = 0x73,
  regVBUS_STOP_DISCL        = 0x74,
  regVBUS_STOP_DISCH        = 0x75,
  regVALARMHCFGL            = 0x76,
  regVALARMHCFGH            = 0x77,
  regVALARMLCFGL            = 0x78,
  regVALARMLCFGH            = 0x79,
  /* 0x7A - 0x7F Reserved */
  /* SCP Control and Status Registers */
  regSCP_ENABLE1            = 0x80,
  regSCP_ENABLE2            = 0x81,
  regSCP_INT1_MSK           = 0x82,
  regSCP_INT2_MSK           = 0x83,
  regTIMER_SET1             = 0x84,
  regTIMER_SET2             = 0x85,
  regEVENT_1                = 0x86,
  regEVENT_2                = 0x87,
  regEVENT_3                = 0x88,
  /* 0x89 - 0x8F Reserved */
  /* SCP Command Registers */
  regDEFAULT_CMD            = 0x90,
  regDEFAULT_ADDR           = 0x91,
  regAUTO_CMD               = 0x92,
  regAUTO_ADDR              = 0x93,
  regAUTO_DATA0             = 0x94,
  regAUTO_DATA1             = 0x95,
  regAUTO_DATA2             = 0x96,
  regAUTO_DATA3             = 0x97,
  regAUTO_DATA4             = 0x98,
  regAUTO_BUFFER_ACK        = 0x99,
  regAUTO_BUFFER_RX0        = 0x9A,
  regAUTO_BUFFER_RX1        = 0x9B,
  regAUTO_BUFFER_RX2        = 0x9C,
  regAUTO_BUFFER_RX3        = 0x9D,
  /* 0x9E - 0x9F Reserved */
  /* SCP Real Time Command Registers */
  regRT_CMD                 = 0xA0,
  regRT_ADDR                = 0xA1,
  regRT_BUFFER_TX0          = 0xA2,
  regRT_BUFFER_TX1          = 0xA3,
  regRT_BUFFER_TX2          = 0xA4,
  regRT_BUFFER_TX3          = 0xA5,
  regRT_BUFFER_TX4          = 0xA6,
  regRT_BUFFER_TX5          = 0xA7,
  regRT_BUFFER_TX6          = 0xA8,
  regRT_BUFFER_TX7          = 0xA9,
  regRT_BUFFER_TX8          = 0xAA,
  regRT_BUFFER_TX9          = 0xAB,
  regRT_BUFFER_TX10         = 0xAC,
  regRT_BUFFER_TX11         = 0xAD,
  regRT_BUFFER_TX12         = 0xAE,
  regRT_BUFFER_TX13         = 0xAF,
  regRT_BUFFER_TX14         = 0xB0,
  regRT_BUFFER_TX15         = 0xB1,
  regRT_BUFFER_TX16         = 0xB2,
  /* 0xB3 - 0xBF Reserved */
  regRT_ACK_RX              = 0xC0,
  regRT_BUFFER_RX0          = 0xC1,
  regRT_BUFFER_RX1          = 0xC2,
  regRT_BUFFER_RX2          = 0xC3,
  regRT_BUFFER_RX3          = 0xC4,
  regRT_BUFFER_RX4          = 0xC5,
  regRT_BUFFER_RX5          = 0xC6,
  regRT_BUFFER_RX6          = 0xC7,
  regRT_BUFFER_RX7          = 0xC8,
  regRT_BUFFER_RX8          = 0xC9,
  regRT_BUFFER_RX9          = 0xCA,
  regRT_BUFFER_RX10         = 0xCB,
  regRT_BUFFER_RX11         = 0xCC,
  regRT_BUFFER_RX12         = 0xCD,
  regRT_BUFFER_RX13         = 0xCE,
  regRT_BUFFER_RX14         = 0xCF,
  regRT_BUFFER_RX15         = 0xD0,
  /* MUS Registers */
  regMUS_CONTROL_1          = 0xD1,
  regMUS_CONTROL_2          = 0xD2,
  regMUS_INTERRUPT          = 0xD3,
  regMUS_INTERRUPT_MSK      = 0xD4,
  regMUS_TIMING             = 0xD5,
  regDEVICE_TYPE            = 0xD6,
  regWD_RESET               = 0xD7,
  regWD_TIMING              = 0xD8,
  regJIG_TIMING             = 0xD9,
  regWD_STATUS              = 0xDA,
  regWD_HISTORY_RESET       = 0xDB,
  regFM_CONTROL1            = 0xDC,
  regFM_CONTROL2            = 0xDD,
  regFM_CONTROL3            = 0xDE,
  regFM_CONTROL4            = 0xDF,
  regFM_STATUS              = 0xE0,
  /* Vender Defined Registers */
  regVCONN_OCP              = 0xE1,
  regPD_RESET               = 0xE2,
  regDRP_TOGGLE             = 0xE3,
  regSINK_TRANSMIT          = 0xE4,
  regSRC_FRSWAP             = 0xE5,
  regSNK_FRSWAP             = 0xE6,
  regDATA_OVP_CONTROL       = 0xE7,
  regDATA_OVP_INT           = 0xE8,
  regDATA_OVP_INT_MSK       = 0xE9
  /* 0xEA - 0xFF Reserved */
};

/* Register unions */
typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_CCSTAT         : 1;
    FSC_U8 I_PORT_PWR       : 1;
    FSC_U8 I_RXSTAT         : 1;
    FSC_U8 I_RXHRDRST       : 1;
    FSC_U8 I_TXFAIL         : 1;
    FSC_U8 I_TXDISC         : 1;
    FSC_U8 I_TXSUCC         : 1;
    FSC_U8 I_VBUS_ALRM_HI   : 1;
  };
} regAlertL_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_VBUS_ALRM_LO   : 1;
    FSC_U8 I_FAULT          : 1;
    FSC_U8 I_RX_FULL        : 1;
    FSC_U8 I_VBUS_SNK_DISC  : 1;
    /* [7..4] Reserved */
  };
} regAlertH_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_CCSTAT         : 1;
    FSC_U8 M_PORT_PWR       : 1;
    FSC_U8 M_RXSTAT         : 1;
    FSC_U8 M_RXHRDRST       : 1;
    FSC_U8 M_TXFAIL         : 1;
    FSC_U8 M_TX_DISC        : 1;
    FSC_U8 M_TXSUCC         : 1;
    FSC_U8 M_VBUS_ALRM_HI   : 1;
  };
} regAlertMskL_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_VBUS_ALRM_LO   : 1;
    FSC_U8 M_FAULT          : 1;
    FSC_U8 M_RX_FULL        : 1;
    FSC_U8 M_VBUS_SNK_DISC  : 1;
    /* [4..7] Reserved */
  };
} regAlertMskH_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_SNKVBUS        : 1;
    FSC_U8 M_VCONN_VAL      : 1;
    FSC_U8 M_VBUS_VAL       : 1;
    FSC_U8 M_VBUS_VAL_EN    : 1;
    FSC_U8 M_SRC_VBUS       : 1;
    FSC_U8 M_SRC_HV         : 1;
    FSC_U8 M_INIT           : 1;
    FSC_U8 M_DEBUG_ACC      : 1;
  };
} regPwrStatMsk_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_I2C_ERR          : 1;
    FSC_U8 M_VCONN_OCP        : 1;
    FSC_U8 M_VBUS_OVP         : 1;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 M_FORCE_DISCH_FAIL : 1;
    FSC_U8 M_AUTO_DISCH_FAIL  : 1;
    FSC_U8 Reserved_1         : 1;
    FSC_U8 M_ALL_REGS_RESET   : 1;
  };
} regFaultStatMsk_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 ORIENT             : 1;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 MUX_CTRL           : 2;
    FSC_U8 Reserved_1         : 2;
    FSC_U8 DEBUG_ACC          : 1;
    FSC_U8 TRI_STATE          : 1;
  };
} regStdOutCfg_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 ORIENT             : 1;
    FSC_U8 BIST_TMODE         : 1;
    FSC_U8 I2C_CLK_STRETCH    : 2;  /* Read-only */
    FSC_U8 DEBUG_ACC_CTRL     : 1;
    FSC_U8 Reserved           : 3;
  };
} regTcpcCtrl_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 CC1_TERM           : 2;
    FSC_U8 CC2_TERM           : 2;
    FSC_U8 RP_VAL             : 2;
    FSC_U8 DRP                : 1;
    FSC_U8 Reserved           : 1;
  };
} regRoleCtrl_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VCONN_OCP_EN       : 1;
    FSC_U8 Reserved_0         : 2;
    FSC_U8 DISCH_TIMER_EN     : 1;
    FSC_U8 Reserved_1         : 4;
  };
} regFaultCtrl_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 EN_VCONN           : 1;
    FSC_U8 VCONN_PWR          : 1;
    FSC_U8 FORCE_DISCH        : 1;
    FSC_U8 EN_BLEED_DISCH     : 1;
    FSC_U8 AUTO_DISCH         : 1;
    FSC_U8 DIS_VALARM         : 1;
    FSC_U8 DIS_VBUS_MON       : 1;
    FSC_U8 Reserved           : 1;
  };
} regPwrCtrl_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 CC1_STAT           : 2;
    FSC_U8 CC2_STAT           : 2;
    FSC_U8 CON_RES            : 1;
    FSC_U8 LOOK4CON           : 1;
    FSC_U8 Reserved           : 2;
  };
} regCCStat_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 SNKVBUS            : 1;
    FSC_U8 VCONN_VAL          : 1;
    FSC_U8 VBUS_VAL           : 1;
    FSC_U8 VBUS_VAL_EN        : 1;
    FSC_U8 SOURCE_VBUS        : 1;
    FSC_U8 SOURCE_HV          : 1;
    FSC_U8 TCPC_INIT          : 1;
    FSC_U8 DEBUG_ACC          : 1;
  };
} regPwrStat_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I2C_ERR            : 1; /* R/WC */
    FSC_U8 VCONN_OCP          : 1; /* R/WC */
    FSC_U8 VBUS_OVP           : 1; /* R/WC */
    FSC_U8 Reserved_0         : 1; /* Read-only */
    FSC_U8 FORCE_DISCH_FAIL   : 1; /* R/WC */
    FSC_U8 AUTO_DISCH_FAIL    : 1; /* R/WC */
    FSC_U8 Reserved_1         : 1; /* Read-only */
    FSC_U8 ALL_REGS_RESET     : 1; /* R/WC */
  };
} regFaultStat_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 SRC_VBUS           : 1;
    FSC_U8 SRC_HV             : 1;
    FSC_U8 SNK_VBUS           : 1;
    FSC_U8 SWITCH_VCONN       : 1;
    FSC_U8 SOP_SUPPORT        : 1;
    FSC_U8 ROLES_SUPPORT      : 3;
  };
} regDevCap1L_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 RP_SUPPORT         : 2;
    FSC_U8 VBUS_MEAS_ALRM     : 1;
    FSC_U8 FORCE_DIS          : 1;
    FSC_U8 BLEED_DIS          : 1;
    FSC_U8 VBUS_OVP           : 1;
    FSC_U8 Reserved           : 2;
  };
} regDevCap1H_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VCONN_FAULT_CAP    : 1;
    FSC_U8 VCONN_POWER_CAP    : 3;
    FSC_U8 VBUS_ALRM_LSB      : 2;
    FSC_U8 STOP_DISCH         : 1;
    FSC_U8 SNK_DISC_DETECT    : 1;
  };
} regDevCap2L_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 ORIENT             : 1;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 MUX_CTRL           : 1;
    FSC_U8 Reserved_1         : 3;
    FSC_U8 DEBUG_ACC          : 1;
    FSC_U8 Reserved_2         : 1;
  };
} regStdOutCap_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 POWER_ROLE         : 1;
    FSC_U8 USBPD_REV          : 2;
    FSC_U8 DATA_ROLE          : 1;
    FSC_U8 CABLE_PLUG         : 1;
    FSC_U8 Reserved           : 3;
  };
} regMsgHeadr_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 EN_SOP             : 1;
    FSC_U8 EN_SOP1            : 1;
    FSC_U8 EN_SOP2            : 1;
    FSC_U8 EN_SOP1_DBG        : 1;
    FSC_U8 EN_SOP2_DBG        : 1;
    FSC_U8 EN_HRD_RST         : 1;
    FSC_U8 EN_CABLE_RST       : 1;
    FSC_U8 Reserved           : 1;
  };
} regRxDetect_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 RX_SOP             : 3;
    FSC_U8 Reserved           : 5;
  };
} regRxStat_t; /* Read-Only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 TX_SOP             : 3;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 RETRY_CNT          : 2;
    FSC_U8 Reserved_1         : 2;
  };
} regTransmit_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_V_LO          : 8;
  };
} regVBusVoltageL_t; /* Read-only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_V_HI          : 2;
    FSC_U8 VBUS_SCALE         : 2;
    FSC_U8 Reserved           : 4;
  };
} regVBusVoltageH_t; /* Read-only */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_SNK_DISC_LO   : 8;
  };
} regVBusSnkDiscL_t;

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_SNK_DISC_HI   : 2;
    FSC_U8 Reserved           : 6;
  };
} regVBusSnkDiscH_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_HI        : 8;
  };
} regVBusStopDiscL_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_LO        : 2;
    FSC_U8 Reserved           : 6;
  };
} regVBusStopDiscH_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_LO        : 8;
  };
} regVAlarmHCfgL_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_HI        : 2;
    FSC_U8 Reserved           : 6;
  };
} regVAlarmHCfgH_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_LO        : 8;
  };
} regVAlarmLCfgL_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBUS_VTH_HI        : 2;
    FSC_U8 Reserved           : 6;
  };
} regVAlarmLCfgH_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 Reserved           : 1;
    FSC_U8 INT_MASK           : 1;
    FSC_U8 EN_CHANNEL         : 1;
    FSC_U8 EN_STIMER          : 1;
    FSC_U8 EN_ATIMER          : 1;
    FSC_U8 EN_ACTIMER         : 1;
    FSC_U8 RESET              : 1;
    FSC_U8 ENABLE_SCP         : 1;
  };
} regSCP_Enable1_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 IO_LEVEL           : 1;
    FSC_U8 MSTR_RST           : 1;
    FSC_U8 Reserved_0         : 3;
    FSC_U8 DPD_EN             : 1;
    FSC_U8 Reserved_1         : 1;
    FSC_U8 EN_BC12            : 1;
  };
} regSCP_Enable2_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_DPD_DTCT         : 1;
    FSC_U8 M_STMR             : 1;
    FSC_U8 M_ATMR             : 1;
    FSC_U8 M_SCP_NACK         : 1;
    FSC_U8 M_SDATA            : 1;
    FSC_U8 M_SCOM             : 1;
    FSC_U8 Reserved           : 1;
    FSC_U8 M_SCP_DTCT         : 1;
  };
} regSCP_Int1_Msk_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_ACK_PARRX        : 1;
    FSC_U8 M_ACK_CRCRX        : 1;
    FSC_U8 M_ACK_LGTH_CHK     : 1;
    FSC_U8 M_I2C_LGTH_CHK     : 1;
    FSC_U8 M_ALRM             : 1;
    FSC_U8 M_SCP_ACK          : 1;
    FSC_U8 M_CMD              : 1;
    FSC_U8 Reserved           : 1;
  };
} regSCP_Int2_Msk_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 ACTIMER            : 8;
  };
} regTimer_Set1_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 STIMER             : 3;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 ATIMER             : 3;
    FSC_U8 Reserved_1         : 1;
  };
} regTimer_Set2_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 ACCP_PLGIN         : 1;
    FSC_U8 Reserved           : 2;
    FSC_U8 TYPEB_PLGIN        : 1;
    FSC_U8 TYPEA_PLGIN        : 1;
    FSC_U8 DPDN_PLGIN         : 1;
    FSC_U8 CC_PLGIN           : 1;
    FSC_U8 SCP_PLGIN          : 1;
  };
} regEvent_1_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_DPD_DTCT         : 1;
    FSC_U8 I_STMR             : 1;
    FSC_U8 I_ATMR             : 1;
    FSC_U8 I_SCP_NACK         : 1;
    FSC_U8 I_SCP_ALRM         : 1;
    FSC_U8 I_SCP_ACK          : 1;
    FSC_U8 I_CMD              : 1;
    FSC_U8 Reserved           : 1;
  };
} regEvent_2_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_ACK_PARRX        : 1;
    FSC_U8 I_ACK_CRCRX        : 1;
    FSC_U8 I_ACK_LGTH_CHK     : 1;
    FSC_U8 I_I1C_LGTH_CHK     : 1;
    FSC_U8 I_SDATA            : 1;
    FSC_U8 I_SCOM             : 1;
    FSC_U8 Reserved           : 2;
  };
} regEvent_3_t; /* R/WC */

/* MUS Register Unions */
typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 Reserved_0         : 1;
    FSC_U8 SW_WAIT            : 1;
    FSC_U8 MANUAL_SW          : 1;
    FSC_U8 SWITCH_OPEN        : 1;
    FSC_U8 DCD_TIMEOUT        : 1;
    FSC_U8 DCP_ENABLE         : 1;
    FSC_U8 Reserved_1         : 1;
    FSC_U8 SNK_ENABLE         : 1;
  };
} regMUS_Control_1_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 Reserved           : 2;
    FSC_U8 H2O_DET			  : 1;
    FSC_U8 WD_EN              : 1;
    FSC_U8 I2C_RESET_EN       : 1;
    FSC_U8 OVP_LEVEL          : 3;
  };
} regMUS_Control_2_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_BC_ATTACH        : 1;
    FSC_U8 I_BC_DETACH        : 1;
    FSC_U8 I_H2O_DET          : 1;
    FSC_U8 I_OT_ERROR         : 1;
    FSC_U8 I_FRSWAP_RX        : 1;
    FSC_U8 I_SCP_EVENT        : 1;
    FSC_U8 Reserved_1         : 2;
  };
} regMUS_Int_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_BC_ATTACH        : 1;
    FSC_U8 M_BC_DETACH        : 1;
    FSC_U8 M_H2O_DET          : 1;
    FSC_U8 M_OT_ERROR         : 1;
    FSC_U8 M_FRSWAP_RX        : 1;
    FSC_U8 M_SCP_EVENT        : 1;
    FSC_U8 Reserved_1         : 2;
  };
} regMUS_Int_Msk_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 PHONE_OFF_WAIT     : 3;
    FSC_U8 SWITCH_WAIT        : 4;
    FSC_U8 Reserved           : 1;
  };
} regMUS_Timing_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 Reserved_0         : 2;
    FSC_U8 Sdp                : 1;
    FSC_U8 Reserved_1         : 2;
    FSC_U8 Cdp                : 1;
    FSC_U8 Dcp                : 1;
    FSC_U8 Reserved_2         : 1;
  };
} regDevice_Type_t; /* R/W */
typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 JIG_HIGH_TIMER     : 5;
    FSC_U8 Reserved           : 3;
  };
} regJig_Timing_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 WD_TIMEOUT_HISTORY : 4;
    FSC_U8 SYSTEM_RESET       : 1;
    FSC_U8 FEED_DOG           : 1;
    FSC_U8 Reserved           : 2;
  };
} regWD_Status_t; /* R */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 RESET_WD_HISTORY   : 1;
    FSC_U8 Reserved           : 7;
  };
} regWD_History_Reset_t; /* WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 MSW                : 2;
    FSC_U8 VBUS_5V            : 2;
    FSC_U8 FAST_ROLE_SWAP     : 2;
    FSC_U8 PWRON              : 2;
  };
} regFM_Control_1_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 BOOT               : 2;
    FSC_U8 JIG                : 2;
    FSC_U8 SEL1               : 2;
    FSC_U8 SEL0               : 2;
  };
} regFM_Control_2_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 VBAT_SWITCH        : 2;
    FSC_U8 VOUT_SWITCH        : 2;
    FSC_U8 SBU_SWITCH         : 3;
    FSC_U8 Reserved           : 1;
  };
} regFM_Control_3_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 USB_SWITCH         : 3;
    FSC_U8 VDDIO_RESET        : 2;
    FSC_U8 VBUS_DETATCH_DET   : 1;
    FSC_U8 Reserved           : 2;
  };
} regFM_Control_4_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 RESET_STAT         : 1;
    FSC_U8 VDDIO_STAT         : 1;
    FSC_U8 VBAT_STAT          : 1;
    FSC_U8 VBUS_STAT          : 1;
    FSC_U8 Reserved           : 4;
  };
} regFM_Status_t; /* R */

/* Vender Defined Register Unions */
typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 OCP_CUR            : 3;
    FSC_U8 OCP_RANGE          : 1;
    FSC_U8 Reserved           : 4;
  };
} regVConn_OCP_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 PD_RST             : 1;
    FSC_U8 Reserved           : 1;
  };
} regPD_Reset_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 DRPTOGGLE          : 1;
    FSC_U8 Reserved           : 1;
  };
} regDRPToggle_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 TRANSMIT_SOP       : 3;
    FSC_U8 Reserved_0         : 1;
    FSC_U8 RETRY_CNT          : 2;
    FSC_U8 DIS_SNK_TX         : 1;
    FSC_U8 Reserved_1         : 1;
  };
} regSink_Transmit_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 FR_SWAP            : 1;
    FSC_U8 MANUAL_SNK_EN      : 1;
    FSC_U8 FRSWAP_SNK_DELAY   : 2;
    FSC_U8 Reserved           : 4;
  };
} regSrc_FRSwap_t; /* R/W(C) */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 EN_FRSWAP_DTCT     : 1;
    FSC_U8 Reserved           : 7;
  };
} regSnk_FRSwap_t; /* R/WC */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 SBU_OVP            : 1;
    FSC_U8 DPDN_BOT_OVP       : 1;
    FSC_U8 DPDN_TOP_OVP       : 1;
    FSC_U8 CC_OVP             : 1;
    FSC_U8 Reserved           : 4;
  };
} regData_OVP_Control_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 I_SBU_OVP          : 1;
    FSC_U8 I_DPDN_BOT_OVP     : 1;
    FSC_U8 I_DPDN_TOP_OVP     : 1;
    FSC_U8 I_CC_OVP           : 1;
    FSC_U8 Reserved           : 4;
  };
} regData_OVP_Int_t; /* R/W */

typedef union {
  FSC_U8 byte;
  struct {
    FSC_U8 M_SBU_OVP          : 1;
    FSC_U8 M_DPDN_BOT_OVP     : 1;
    FSC_U8 M_DPDN_TOP_OVP     : 1;
    FSC_U8 M_CC_OVP           : 1;
    FSC_U8 Reserved           : 4;
  };
} regData_OVP_Msk_t; /* R/W */

/* Register Structure */
typedef struct
{
  /* Type-C and PD Registers */
  FSC_U8              VendIDL;
  FSC_U8              VendIDH;
  FSC_U8              ProdIDL;
  FSC_U8              ProdIDH;
  FSC_U8              DevIDL;
  FSC_U8              DevIDH;
  FSC_U8              TypeCRevL;
  FSC_U8              TypeCRevH;
  FSC_U8              USBPDVer;
  FSC_U8              USBPDRev;
  FSC_U8              PDIFRevL;
  FSC_U8              PDIFRevH;
  regAlertL_t         AlertL;
  regAlertH_t         AlertH;
  regAlertMskL_t      AlertMskL;
  regAlertMskH_t      AlertMskH;
  regPwrStatMsk_t     PwrStatMsk;
  regFaultStatMsk_t   FaultStatMsk;
  regStdOutCfg_t      StdOutCfg;
  regTcpcCtrl_t       TcpcCtrl;
  regRoleCtrl_t       RoleCtrl;
  regFaultCtrl_t      FaultCtrl;
  regPwrCtrl_t        PwrCtrl;
  regCCStat_t         CCStat;
  regPwrStat_t        PwrStat;
  regFaultStat_t      FaultStat;
  FSC_U8              Command;
  regDevCap1L_t       DevCap1L;
  regDevCap1H_t       DevCap1H;
  regDevCap2L_t       DevCap2L;
  regStdOutCap_t      StdOutCap;
  regMsgHeadr_t       MsgHeadr;
  regRxDetect_t       RxDetect;
  FSC_U8              RxByteCnt;
  regRxStat_t         RxStat;
  FSC_U8              RxHeadL;
  FSC_U8              RxHeadH;
  FSC_U8              RxData[COMM_BUFFER_LENGTH];
  regTransmit_t       Transmit;
  FSC_U8              TxByteCnt;
  FSC_U8              TxHeadL;
  FSC_U8              TxHeadH;
  FSC_U8              TxData[COMM_BUFFER_LENGTH];
  regVBusVoltageL_t   VBusVoltageL;
  regVBusVoltageH_t   VBusVoltageH;
  regVBusSnkDiscL_t   VBusSnkDiscL;
  regVBusSnkDiscH_t   VBusSnkDiscH;
  regVBusStopDiscL_t  VBusStopDiscL;
  regVBusStopDiscH_t  VBusStopDiscH;
  regVAlarmHCfgL_t    VAlarmHCfgL;
  regVAlarmHCfgH_t    VAlarmHCfgH;
  regVAlarmLCfgL_t    VAlarmLCfgL;
  regVAlarmLCfgH_t    VAlarmLCfgH;
  /* SCP Control and Status Registers */
  regSCP_Enable1_t    SCPEnable1;
  regSCP_Enable2_t    SCPEnable2;
  regSCP_Int1_Msk_t   SCPInt1Msk;
  regSCP_Int2_Msk_t   SCPInt2Msk;
  regTimer_Set1_t     TimerSet1;
  regTimer_Set2_t     TimerSet2;
  regEvent_1_t        Event1;
  regEvent_2_t        Event2;
  regEvent_3_t        Event3;
  /* SCP Command Registers */
  FSC_U8              DefaultCmd;
  FSC_U8              DefaultAddr;
  FSC_U8              AutoCmd;
  FSC_U8              AutoAddr;
  FSC_U8              AutoData0;
  FSC_U8              AutoData1;
  FSC_U8              AutoData2;
  FSC_U8              AutoData3;
  FSC_U8              AutoData4;
  FSC_U8              AutoBufferAck;
  FSC_U8              AutoBufferRx0;
  FSC_U8              AutoBufferRx1;
  FSC_U8              AutoBufferRx2;
  FSC_U8              AutoBufferRx3;
  /* SCP Real Time Command Registers */
  FSC_U8              RTCmd;
  FSC_U8              RTAddr;
  FSC_U8              RTBufferTx[SCP_BUFFER_LENGTH+1];
  FSC_U8              RTAckRx;
  FSC_U8              RTBufferRx[SCP_BUFFER_LENGTH];
  /* MUS Registers */
  regMUS_Control_1_t  MUSControl1;
  regMUS_Control_2_t  MUSControl2;
  regMUS_Int_t        MUSInterrupt;
  regMUS_Int_Msk_t    MUSIntMask;
  regMUS_Timing_t     MUSTiming;
  regDevice_Type_t    DeviceType;
  FSC_U8              WDReset;
  FSC_U8              WDTiming;
  regJig_Timing_t     JigTiming;
  regWD_Status_t      WDStatus;
  regWD_History_Reset_t WDHistoryReset;
  regFM_Control_1_t   FMControl1;
  regFM_Control_2_t   FMControl2;
  regFM_Control_3_t   FMControl3;
  regFM_Control_4_t   FMControl4;
  regFM_Status_t      FMStatus;
  /* Vender Defined Registers */
  regVConn_OCP_t      VConnOCP;
  regPD_Reset_t       PDReset;
  regDRPToggle_t      DRPToggle;
  regSink_Transmit_t  SinkTransmit;
  regSrc_FRSwap_t     SrcFRSwap;
  regSnk_FRSwap_t     SnkFRSwap;
  regData_OVP_Control_t DataOVPControl;
  regData_OVP_Int_t   DataOVPInt;
  regData_OVP_Msk_t   DataOVPMsk;
} DeviceReg_t;

/*
 * AddressToRegister
 *
 * Arguments:   registers - ptr to register object to get registers from
 *              address - Register Address
 * Return:      Pointer to the register struct byte value
 * Description: A shortcut for reads and writes.
 * Note:        The only registers not included here are reserved registers.
 */
FSC_U8 *FUSB3601_AddressToRegister(DeviceReg_t *registers, enum RegAddress address);

/*
 * GetLocalRegisters
 *
 * Arguments:   registers - ptr to register object to get registers from
 *              data - ptr to output array, must be >= TOTAL_REGISTER_CNT bytes
 *              length - length of data array
 * Description: Records the contents of the local register structs into a
 *              buffer to pass up to the caller.
 * Note:        Data buffer length must be at least TOTAL_REGISTER_CNT bytes.
 *              Does not get values from reserved registers.
 */
void FUSB3601_GetLocalRegisters(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length);

/*
 * RegGetRxData
 *
 * Arguments:   registers - ptr to register object to get data registers from
 *              data - ptr to output array
 *              length - length of data array and number of data bytes to get,
 *                up to a total of COMM_BUFFER_LENGTH.
 * Description: Records the contents of the local Rx data registers into a
 *              buffer to pass up to the caller.
 * Note:        Length may be > COMM_BUFFER_LENGTH, but only the first
 *              COMM_BUFFER_LENGTH bytes will be written.
 */
void FUSB3601_RegGetRxData(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length);

/*
 * RegSetTxData
 *
 * Arguments:   registers - ptr to register object in which to set data registers
 *              data - ptr to input array
 *              length - Number of bytes to set, up to COMM_BUFFER_LENGTH
 * Description: Copies the contents of data (up to COMM_BUFFER_LENGTH bytes) to
 *              the Tx registers
 * Note:        Length may be > COMM_BUFFER_LENGTH, but only the first
 *              COMM_BUFFER_LENGTH bytes will be written.
 */
void FUSB3601_RegSetTxData(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length);

/*
 * RegSetBits
 *
 * Arguments:   registers - ptr to register object
 *              address - register to set bits of
 *              mask - Bits to set
 * Description: Sets the bits indicated as 1's in mask in register address.
 */
void FUSB3601_RegSetBits(DeviceReg_t *registers, enum RegAddress address, FSC_U8 mask);

/*
 * RegClearBits
 *
 * Arguments:   registers - ptr to register object
 *              address - register to set bits of
 *              mask - Bits to set
 * Description: Clears the bits indicated as 1's in mask in register address.
 */
void FUSB3601_RegClearBits(DeviceReg_t *registers, enum RegAddress address, FSC_U8 mask);
#endif /*  FSCPM_REGISTERS_H_ */

