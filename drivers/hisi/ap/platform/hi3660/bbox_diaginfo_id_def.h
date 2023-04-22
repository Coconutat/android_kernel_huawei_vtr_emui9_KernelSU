#ifndef __BBOX_DIAGINFO_ID_DEF_H__
#define __BBOX_DIAGINFO_ID_DEF_H__ 
enum bbox_diaginfo_errid {
 SoC_DIAGINFO_START=925200000,
 L3_ECC_1BIT_ERROR = SoC_DIAGINFO_START,
 CPU_UP_FAIL,
 DDR_DIAGINFO_START=925202000,
 LPM3_DDR_FAIl = DDR_DIAGINFO_START,
 BBOX_DIAGINFO_END = 925299999
};
enum bbox_diaginfo_module {
 SoC_AP = 1,
 DSS,
 DDR,
};
enum bbox_diaginfo_level {
 Critical = 1,
 Warning,
 INFO,
};
enum bbox_diaginfo_type {
 HW = 1,
 SW,
};
#endif
