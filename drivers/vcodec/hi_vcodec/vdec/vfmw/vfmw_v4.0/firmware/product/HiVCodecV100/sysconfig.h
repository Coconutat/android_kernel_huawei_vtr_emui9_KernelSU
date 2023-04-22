/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWinjiDDUykL9e8pckESWBbMVmSWkBuyJO01cTiy3TdzKxGk0oBQa
mSMf7J4FkTpfvzHyMxSEsfcbL/G0fFswaAZ8tsS4we+PBWC6a/UNlzCWIaw+Ujkv9NAY+as0
fg7WZIRvw27AjvRqJbkRJvqFUORSa6KPQaSBMxCxJTGTTf//sQbjPOyYldN0OVR9ut4HFO4U
ZguGQVqcOAJQbE96v6175DqhuprKgQB8R+2fu7VD3qtX+ZJh/t0512oqv+e8YA==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
#ifndef __VFMW_SYSCONFIG_HEADER__
#define __VFMW_SYSCONFIG_HEADER__

#include "vfmw.h"


/* valid vdh num */
#define MAX_VDH_NUM              (1)
#define MAX_SCD_NUM              (MAX_VDH_NUM)

/* test memory config, not used in release version */
#define BOARD_MEM_BASE_ADDR      (0x0E000000)
#define BOARD_MEM_TOTAL_SIZE     (0x1F400000)//(500*1024*1024)

#ifdef ENV_SOS_KERNEL
#ifndef CONFIG_HISI_FAMA
#define HEAP_SEC_DRM_BASE (0x60000000U)
#else
#define HEAP_SEC_DRM_BASE (0x60000000U)
#endif
#define HEAP_SEC_DRM_SIZE (0x20000000U)//(512*1024*1024)

#define CRG_PERI_RegBaseAddr     (0xFFF35000)
//PERPWRSTAT[2].vdecpwrstat, 1: vdec power enable, 0:disable
#define CRG_PERPWRSTAT           (0x158)
//PERPWRACK[2].vdecpwrack, 1: vdec power on, 0:off
#define CRG_PERPWRACK            (0x15C)
#endif

/* register offset */
#define SCD_REG_OFFSET           (0xc000)
#define BPD_REG_OFFSET           (0xd000)

#define SOFTRST_REQ_OFFSET       (0xf80c)
#define SOFTRST_OK_OFFSET        (0xf810)

typedef enum {
	GLB_RESET_REQUIRE = 0,
	BPD_RESET_REQUIRE,
	SCD_RESET_REQUIRE,
	MFDE_RESET_REQUIRE,
	BUTT_RESET_REQUIRE,  //max undefine type
}RESET_REQUIRE_TYPE_E;

#define ALL_RESET_CTRL_BIT       (0)
#define MFDE_RESET_CTRL_BIT      (1)
#define SCD_RESET_CTRL_BIT       (2)
#define BPD_RESET_CTRL_BIT       (3)

#define ALL_RESET_OK_BIT         (0)
#define MFDE_RESET_OK_BIT        (1)
#define SCD_RESET_OK_BIT         (2)
#define BPD_RESET_OK_BIT         (3)

/* FPGA flag */
extern UINT32  gIsFPGA;

/* register base addr & range */
extern UINT32  gVdhRegBaseAddr;
extern UINT32  gScdRegBaseAddr;
extern UINT32  gBpdRegBaseAddr;
extern UINT32  gVdhRegRange;
extern UINT32  gSOFTRST_REQ_Addr;
extern UINT32  gSOFTRST_OK_ADDR;

/* smmu page table base addr */
extern UINT64  gSmmuPageBase;

/* peri crg base addr */
extern UINT32  gPERICRG_RegBaseAddr;

/* irq num */
extern UINT32  gMfdeIrqNum;
extern UINT32  gScdIrqNum;
extern UINT32  gBpdIrqNum;
extern UINT32  gSmmuIrqNum;
extern UINT32  gMfdeSafeIrqNum;
extern UINT32  gScdSafeIrqNum;
extern UINT32  gBpdSafeIrqNum;
extern UINT32  gSmmuSafeIrqNum;

#endif
