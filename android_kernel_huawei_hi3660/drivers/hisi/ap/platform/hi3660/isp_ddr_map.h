#ifndef __ISP_DDR_MAP_H__
#define __ISP_DDR_MAP_H__ 
#include <global_ddr_map.h>
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define ISP_PCTRL_PERI_STAT_ADDR (0x000000BC)
#define ISP_PCTRL_PERI_FLAG ((1 << 21) | (1 << 20))
#define MEM_ISPFW_SIZE (0x00800000)
#define MEM_PMD_ADDR_OFFSET (0x00002000)
#define MEM_PTE_ADDR_OFFSET (MEM_ISPFW_SIZE + 0x10000)
#define MEM_SMMU_ERRRD_ADDR_OFFSET (0x0000F000)
#define MEM_SMMU_ERRWR_ADDR_OFFSET (0x0000F800)
#define MEM_RSCTABLE_ADDR_OFFSET (0x00014000)
#define MEM_RSCTABLE_SIZE (0x00004000)
#define MEM_ISPDTS_SIZE (0x02000000)
#define TEXT_BASE (0xc0000000)
#define ISP_TEXT_SIZE (0x00300000)
#define DATA_BASE (0xc0300000)
#define ISP_DATA_SIZE (0x00608000)
#define ISP_BIN_DATA_SIZE (0x00300000)
#define ISP_FW_SIZE (ISP_TEXT_SIZE + ISP_DATA_SIZE)
#define ISP_BIN_FW_SIZE (ISP_TEXT_SIZE + ISP_BIN_DATA_SIZE)
#define ISP_CORE_CFG_BASE_ADDR (0xE8400000)
#define ISP_PMC_BASE_ADDR (0xFFF31000)
#define MEM_MDC_DA (0)
#define MEM_MDC_SIZE (0)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
