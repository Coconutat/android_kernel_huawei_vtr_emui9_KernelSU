#ifndef __RDR_DFX_CORE_H__
#define __RDR_DFX_CORE_H__

typedef enum{
	DFX_NOREBOOT = 0,
	DFX_ZEROHUNG,
	DFX_MAX_MODULE
}dfx_module_type;

#ifdef CONFIG_HISI_DFX_CORE
int dfx_read(u32 module, void *buffer, u32 size);
int dfx_write(u32 module,void *buffer, u32 size);
#else
static inline int dfx_read(u32 module, void *buffer, u32 size) {return 0;}
static inline int dfx_write(u32 module,void *buffer, u32 size) {return 0;}
#endif

#endif
