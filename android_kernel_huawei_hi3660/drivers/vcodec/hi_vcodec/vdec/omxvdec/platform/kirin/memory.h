

#ifndef __VDEC_MEMORY_H__
#define	__VDEC_MEMORY_H__

#include "hi_type.h"

#define  OMXVDEC_ZONE   "OMXVDEC"

typedef struct
{
    HI_U8    u8IsSecure;
    HI_U8    u8IsCached;
    HI_U8    u8IsContiguous;
    HI_U32   u32StartPhyAddr;
    HI_U32   u32Size;
    HI_VOID *pStartVirAddr;
}MEM_BUFFER_S;

HI_S32   VDEC_MEM_Probe(HI_VOID);
HI_S32   VDEC_MEM_Init(HI_VOID);
HI_S32   VDEC_MEM_Exit(HI_VOID);
HI_S32   VDEC_MEM_AllocAndMap(const char *bufname, char *zone_name, MEM_BUFFER_S *psMBuf);
HI_S32   VDEC_MEM_UnmapAndRelease(MEM_BUFFER_S *psMBuf);
HI_S32   VDEC_MEM_MapKernel(HI_S32 share_fd, MEM_BUFFER_S *psMBuf);
HI_S32   VDEC_MEM_UnmapKernel(MEM_BUFFER_S *psMBuf);
HI_S32   VDEC_MEM_AllocAndShare(const HI_CHAR *bufname, HI_CHAR *zone_name, MEM_BUFFER_S *psMBuf, HI_S32 *pShareFd);
HI_S32   VDEC_MEM_CloseAndFree(MEM_BUFFER_S *psMBuf, HI_S32 *pShareFd);
HI_S32   VDEC_MEM_GetVirAddr_FromPhyAddr(HI_U8 **ppVirAddr, HI_U32 PhyAddr, HI_U32 Size);
HI_S32   VDEC_MEM_KAlloc(const HI_CHAR *bufname, HI_CHAR *zone_name, MEM_BUFFER_S *psMBuf, HI_U8 NeedZero);
HI_S32   VDEC_MEM_KFree(MEM_BUFFER_S *psMBuf);
HI_VOID  VDEC_MEM_Read_Proc(HI_VOID *p, HI_VOID *v);

#endif

