#ifndef _VDM_HAL_API_HEADER_
#define _VDM_HAL_API_HEADER_


#include "basedef.h"
#include "mem_manage.h"
#include "vfmw.h"
#include "vdm_hal_local.h"
#include "vfmw_ctrl.h"
#ifdef __cplusplus
extern "C" {
#endif

SINT32 VDMHAL_IMP_GetHalMemSize(VOID);
SINT32 VDMHAL_IMP_OpenHAL(VDMHAL_OPENPARAM_S *pOpenParam);
VOID   VDMHAL_IMP_CloseHAL(SINT32 VdhId);
SINT32 VDMHAL_IMP_ArrangeMem( UADDR MemAddr, SINT32 MemSize, SINT32 Width, SINT32 Height, SINT32 PmvNum, SINT32 FrameNum, ARRANGE_FLAG_E eFlag, VDMHAL_MEM_ARRANGE_S *pVdmMemArrange );
VOID   VDMHAL_IMP_ResetVdm( SINT32 VdhId );
VOID   VDMHAL_IMP_GlbReset( VOID );
VOID   VDMHAL_IMP_ClearIntState( SINT32 VdhId );
SINT32 VDMHAL_IMP_CheckReg(REG_ID_E reg_id, SINT32 VdhId);
VOID   VDMHAL_IMP_StartHwRepair(SINT32 VdhId);
VOID   VDMHAL_IMP_StartHwDecode(SINT32 VdhId);
SINT32 VDMHAL_IMP_PrepareDec( VID_STD_E VidStd, VOID *pDecParam, SINT32 VdhId );
SINT32 VDMHAL_IMP_IsVdmReady(SINT32 VdhId);
SINT32 VDMHAL_IMP_IsVdmRun(SINT32 VdhId);
SINT32 VDMHAL_IMP_PrepareRepair( VID_STD_E VidStd, VOID *pDecParam, SINT32 RepairTime, SINT32 VdhId );
SINT32 VDMHAL_IMP_MakeDecReport(MAKE_DEC_REPORT_S *pMakeDecReport);
SINT32 VDMHAL_IMP_BackupInfo(BACKUP_INFO_S *pBackUpInfo);
VOID   VDMHAL_IMP_GetCharacter(VOID);
VOID   VDMHAL_IMP_WriteScdEMARID(VOID);

UINT32 VDMHAL_EXT_ReduceFrequency(RESET_REQUIRE_TYPE_E eResetRequire);
VOID   VDMHAL_EXT_RestoreFrequency(RESET_REQUIRE_TYPE_E eResetRequire, UINT32 DivValue);

#ifdef __cplusplus
}
#endif


#endif

