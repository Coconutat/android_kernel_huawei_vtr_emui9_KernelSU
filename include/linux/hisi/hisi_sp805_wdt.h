#ifndef __HISI_SP805_WDT_H
#define __HISI_SP805_WDT_H

typedef struct rdr_arctimer_s {
	u32 cntv_ctl_el0;
	u32 cntv_tval_el0;
	u32 cntp_ctl_el0;
	u32 cntp_tval_el0;
	u32 cntfrq_el0;

	u64 cntv_cval_el0;
	u64 cntp_cval_el0;
	u64 cntvct_el0;
	u64 cntpct_el0;
} rdr_arctimer_t;

#ifdef CONFIG_HISI_SP805_WATCHDOG
extern unsigned int get_wdt_kick_time(void);
extern unsigned long get_wdt_expires_time(void);
extern void sp805_wdt_dump(void);
extern void rdr_arctimer_register_read(rdr_arctimer_t *arctimer);
extern void rdr_archtime_register_print(rdr_arctimer_t *arctimer, bool after);

extern rdr_arctimer_t g_rdr_arctimer_record;
static inline rdr_arctimer_t *rdr_get_arctimer_record(void)
{
	return &g_rdr_arctimer_record;
}

#else
static inline unsigned int get_wdt_kick_time(void) { return 0; }
static inline unsigned long get_wdt_expires_time(void) { return 0; }
static inline void sp805_wdt_dump(void) { return; }
static inline void rdr_arctimer_register_read(rdr_arctimer_t *arctimer){ return; }
static inline void rdr_archtime_register_print(rdr_arctimer_t *arctimer, bool after){ return; }
static inline rdr_arctimer_t *rdr_get_arctimer_record(void){ return NULL; }
#endif

#endif /*__HISI_SP805_WDT_H*/
