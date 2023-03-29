#ifndef __HI_BASE_H__
#define __HI_BASE_H__ 
#include <osl_bio.h>
#include <bsp_memmap.h>
#if defined(__KERNEL__)
#undef INLINE
#define INLINE inline
#ifndef HI_SET_GET
#define HI_SET_GET(__full_name__,__reg_name,__reg_type,__reg_base,__reg_offset) \
 static INLINE void set_##__full_name__(unsigned int val) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((volatile void *)((__reg_base##_VIRT) + __reg_offset)); \
  reg_obj.bits.__reg_name = val; \
  writel(reg_obj.u32, (volatile void *)((__reg_base##_VIRT) + __reg_offset)); \
 } \
 static INLINE unsigned int get_##__full_name__(void) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((volatile void *)((__reg_base##_VIRT) + __reg_offset)); \
  return reg_obj.bits.__reg_name; \
 }
#endif
#elif defined(__CMSIS_RTOS) || (defined(__OS_VXWORKS__) && defined(__GNUC__))
#undef INLINE
#define INLINE inline
#define HI_SET_GET(__full_name__,__reg_name,__reg_type,__reg_base,__reg_offset) \
 static INLINE void set_##__full_name__(unsigned int val) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  reg_obj.bits.__reg_name = val; \
  writel(reg_obj.u32,(void*)( (__reg_base##_VIRT) + __reg_offset)); \
 } \
 static INLINE unsigned int get_##__full_name__(void) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  return reg_obj.bits.__reg_name; \
 }
#elif defined(__OS_VXWORKS__)
#undef INLINE
#define INLINE __inline__
#define HI_SET_GET(__full_name__,__reg_name,__reg_type,__reg_base,__reg_offset) \
 static INLINE void set_##__full_name__(unsigned int val) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((__reg_base##_VIRT) + __reg_offset); \
  reg_obj.bits.##__reg_name = val; \
  writel(reg_obj.u32, (__reg_base##_VIRT) + __reg_offset); \
 } \
 static INLINE unsigned int get_##__full_name__(void) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((__reg_base##_VIRT) + __reg_offset); \
  return reg_obj.bits.##__reg_name; \
 }
#elif defined(__OS_RTOSCK__)
#define HI_SET_GET(__full_name__,__reg_name,__reg_type,__reg_base,__reg_offset) \
 static inline void set_##__full_name__(unsigned int val) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  reg_obj.bits.__reg_name = val; \
  writel(reg_obj.u32,(void*)( (__reg_base##_VIRT) + __reg_offset)); \
 } \
 \
 static inline unsigned int get_##__full_name__(void) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  return reg_obj.bits.__reg_name; \
 }
#else
#include <boot/boot.h>
#undef INLINE
#define INLINE inline
#define HI_SET_GET(__full_name__,__reg_name,__reg_type,__reg_base,__reg_offset) \
 static INLINE void set_##__full_name__(unsigned int val) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  reg_obj.bits.__reg_name = val; \
  writel(reg_obj.u32, (void*)((__reg_base##_VIRT) + __reg_offset)); \
 } \
 static INLINE unsigned int get_##__full_name__(void) \
 {\
  __reg_type reg_obj; \
  reg_obj.u32 = readl((void*)((__reg_base##_VIRT) + __reg_offset)); \
  return reg_obj.bits.__reg_name; \
 }
#endif
#endif
