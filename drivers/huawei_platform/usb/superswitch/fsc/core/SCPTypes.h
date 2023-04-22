
#ifndef FSC_SCPTYPES_H_
#define FSC_SCPTYPES_H_

typedef enum {
  ACCP_Disabled,
  ACCP_RequestVoltages,
  ACCP_SelectVoltage,
  ACCP_RequestCurrents,
  ACCP_SelectCurrent,
  ACCP_Idle
} ACCPState;

typedef enum {
  SCP_Disabled,
  SCP_RequestStatus,
  SCP_RequestDetails,
  SCP_SelectVoltage,
  SCP_SelectCurrent,
  SCP_Idle
} SCPState;

#endif /* FSC_SCPTYPES_H_ */
