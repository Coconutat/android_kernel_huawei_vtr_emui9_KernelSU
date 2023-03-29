#ifndef FSCPM_TYPECTYPES_H_
#define FSCPM_TYPECTYPES_H_

#define SRC_OPEN    0b00
#define SRC_RA      0b01
#define SRC_RD      0b10

#define SNK_OPEN    0b00
#define SNK_DEFAULT 0b01
#define SNK_1P5     0b10
#define SNK_3P0     0b11

/* CC1 and CC2 must match TCPCCtrl.ORIENT bit */
typedef enum {
  CC1 = 0,
  CC2 = 1,
  NONE,
} CCOrientation;

typedef enum {
  Source = 0,
  Sink
} SourceOrSink;

typedef enum {
  VBUS_DETACH_ENABLE = 0,
  VBUS_DETACH_DISABLE
} VbusDetach_t;

typedef enum {
  USBTypeC_Sink = 0,
  USBTypeC_Source,
  USBTypeC_DRP,
  USBTypeC_UNDEFINED = 99
} USBTypeCPort;

typedef enum {
  Disabled = 0,
  ErrorRecovery,
  Unattached,
  AttachWaitSink,
  AttachedSink,
  AttachWaitSource,
  AttachedSource,
  TrySource,
  TryWaitSink,
  TrySink,
  TryWaitSource,
  AudioAccessory,
  AttachWaitAccessory,
  DebugAccessorySource,
  DebugAccessorySink,
  PoweredAccessory,
  UnsupportedAccessory,
  DelayUnattached,
  UnattachedSource,
  IllegalCable
} TypeCState;

typedef enum {
  CCTypeOpen = 0,
  CCTypeRa,
  CCTypeRdUSB,
  CCTypeRd1p5,
  CCTypeRd3p0,
  CCTypeUndefined
} CCTermType;

typedef enum {
  CCTermSrcOpen = 0,
  CCTermSrcRa = 0b01,
  CCTermSrcRd = 0b10,
  CCTermSrcUndefined = 0b11
} CCSourceTermType;

typedef enum {
  CCTermSnkOpen = 0,
  CCTermSnkDefault = 0b01,
  CCTermSnkRp1p5 = 0b10,
  CCTermSnkRp3p0 = 0b11
} CCSinkTermType;

typedef enum {
  CCRoleRa = 0,
  CCRoleRp = 0b01,
  CCRoleRd = 0b10,
  CCRoleOpen = 0b11
} CCRoleTermType;

typedef enum {
  RpValDefault = 0,
  RpVal1p5 = 0b01,
  RpVal3p0 = 0b10,
  RpValUndefined = 0b11
} RoleRpVal;

typedef enum {
  TypeCPin_None = 0,
  TypeCPin_GND1,
  TypeCPin_TXp1,
  TypeCPin_TXn1,
  TypeCPin_VBUS1,
  TypeCPin_CC1,
  TypeCPin_Dp1,
  TypeCPin_Dn1,
  TypeCPin_SBU1,
  TypeCPin_VBUS2,
  TypeCPin_RXn2,
  TypeCPin_RXp2,
  TypeCPin_GND2,
  TypeCPin_GND3,
  TypeCPin_TXp2,
  TypeCPin_TXn2,
  TypeCPin_VBUS3,
  TypeCPin_CC2,
  TypeCPin_Dp2,
  TypeCPin_Dn2,
  TypeCPin_SBU2,
  TypeCPin_VBUS4,
  TypeCPin_RXn1,
  TypeCPin_RXp1,
  TypeCPin_GND4
} TypeCPins_t;

typedef enum {
  utccOpen = 0,
  utccDefault = 1,
  utcc1p5A = 2,
  utcc3p0A = 3
} USBTypeCCurrent;

#endif /* FSCPM_TYPECTYPES_H_ */
