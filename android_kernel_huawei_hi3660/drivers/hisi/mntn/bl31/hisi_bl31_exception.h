
#ifndef __BL31_EXCEPTION_H
#define __BL31_EXCEPTION_H

#define BL31_PANIC_MAGIC 0xdead
#define BL31_DEBUG_FN_VAL 0xc800aa02
#define BL31_WA_COUNTER_FN_VAL 0xc800aa03

#ifdef CONFIG_HISI_BL31_MNTN
extern void bl31_panic_ipi_handle(void);
extern u32 get_bl31_exception_flag(void);
extern int rdr_exception_trace_bl31_cleartext_print(char *dir_path, u64 log_addr, u32 log_len);
extern int rdr_exception_trace_bl31_init(u8 *phy_addr, u8 *virt_addr, u32 log_len);
extern int rdr_exception_analysis_bl31(u64 etime, u8 *addr, u32 len,
	struct rdr_exception_info_s *exception);
extern void rdr_init_sucess_notify_bl31(void);
#else
static inline void bl31_panic_ipi_handle(void)
{
}
static inline u32 get_bl31_exception_flag(void)
{
	return 0;
}
static inline int rdr_exception_trace_bl31_cleartext_print(char *dir_path, u64 log_addr, u32 log_len)
{
	return 0;
}
static inline int rdr_exception_trace_bl31_init(u8 *phy_addr, u8 *virt_addr, u32 log_len)
{
	return 0;
}
static inline void rdr_init_sucess_notify_bl31(void)
{
	return;
}
static inline int rdr_exception_analysis_bl31(u64 etime, u8 *addr, u32 len,
	struct rdr_exception_info_s *exception)
{
	return 0;
}
#endif
#endif
