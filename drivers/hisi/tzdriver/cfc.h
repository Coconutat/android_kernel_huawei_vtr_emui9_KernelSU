#ifndef TZDRIVER_CFC_H
#define TZDRIVER_CFC_H

#define CFC_MARK_SYM(name) __cfc_mark_##name
#define CFC_SEND_DATA_START_SYM(name) __cfc_send_##name##_start
#define CFC_SEND_DATA_STOP_SYM(name) __cfc_send_##name##_stop
#define CFC_FUNC_ENTRY_SYM(func) __cfc_enter_##func
#define CFC_RETURN_SUCC_SYM(func, extra) __cfc_ret_##func##_succ##extra
#define CFC_RETURN_FAIL_SYM(func, extra) __cfc_ret_##func##_fail##extra

#ifndef CFC_GENRULES

#include <linux/spinlock.h>

#ifdef CONFIG_TEE_CFC

#define STRINGX(x) #x
#define STRING(x) STRINGX(x)

extern void cfc_enable_coresight(void);
extern void cfc_disable_coresight(void);
extern void cfc_prepare_clk_pm(void);
extern void cfc_unprepare_pm_clk(void);
extern spinlock_t cfc_coresight_spinlock;
extern unsigned int *cfc_seqlock;
extern bool cfc_is_enabled;

#define ___CFC_SEND_DATA(name, data)				\
	asm volatile(".pushsection .cfc.entries.text, \"ax\"\n"	\
		".global " STRING(CFC_SEND_DATA_START_SYM(name)) "\n"	\
		".global " STRING(CFC_SEND_DATA_STOP_SYM(name)) "\n"	\
		"1:\n"						\
		"isb\n"						\
		STRING(CFC_SEND_DATA_START_SYM(name)) ":\n"	\
		"isb\n"						\
		"msr contextidr_el1, %0\n"			\
		"isb\n"						\
		STRING(CFC_SEND_DATA_STOP_SYM(name)) ":\n"	\
		"msr contextidr_el1, %1\n"			\
		"b 2f\n"					\
		".popsection\n"					\
		"b 1b\n"					\
		"2: \n"						\
		::"r"(data), "r"(current->pid):)

#define ___CFC_MARK(symname)					\
	asm volatile(".pushsection .cfc.entries.text, \"ax\"\n"	\
		".type " STRING(symname) ", @function\n"	\
		".global " STRING(symname) "\n"			\
		STRING(symname)":\n"				\
		"1:\n"						\
		"b 2f\n"					\
		".popsection\n"					\
		"b 1b\n"					\
		"2: \n"						\
		)

/* 1.8G */
#define CFC_SEQLOCK_MAX_LOOP	1800000000

#define __CFC_SEQLOCK_BEGIN					\
	do {							\
		uint32_t old_seqlock;				\
		unsigned int __cfc_counter = CFC_SEQLOCK_MAX_LOOP;	\
		do

#define __CFC_SEQLOCK_END					\
		while(__cfc_counter && ((old_seqlock % 2) || (old_seqlock != READ_ONCE(*cfc_seqlock))));\
	} while (0)

#define __CFC_WRAPPER(content)							\
	__CFC_SEQLOCK_BEGIN {							\
		unsigned long __cfc_flags;						\
		if (!cfc_is_enabled)						\
			break;							\
		while ((__cfc_counter && (old_seqlock = READ_ONCE(*cfc_seqlock)) % 2))\
			__cfc_counter--;						\
		get_cpu();							\
		spin_lock_irqsave(&cfc_coresight_spinlock, __cfc_flags);		\
		if (__cfc_counter && ((old_seqlock = READ_ONCE(*cfc_seqlock)) % 2)) {	\
			spin_unlock_irqrestore(&cfc_coresight_spinlock, __cfc_flags);	\
			put_cpu();						\
			continue;						\
		}								\
		cfc_enable_coresight();						\
		content;							\
		cfc_disable_coresight();					\
		spin_unlock_irqrestore(&cfc_coresight_spinlock, __cfc_flags);		\
		put_cpu();							\
	} __CFC_SEQLOCK_END

#define __CFC_MARK(symname) __CFC_WRAPPER(___CFC_MARK(symname))
#define __CFC_SEND_DATA(name, data) __CFC_WRAPPER(___CFC_SEND_DATA(name, data))

#else
#define __CFC_MARK(symname) do { } while(0)
#define __CFC_SEND_DATA(symname, data) do { } while (0)
#define cfc_is_enabled false
#define cfc_prepare_clk_pm(ARG) do {} while(0)
#define cfc_unprepare_pm_clk(ARG) do {} while(0)
#endif

/* for linux kernel */
#define CFC_MARK(name) __CFC_MARK(CFC_MARK_SYM(name))
#define CFC_SEND_DATA(name, data) __CFC_SEND_DATA(name, data)

#define __CFC_RETURN(symname, val)				\
	do {							\
		__CFC_MARK(symname);				\
		return val;					\
	} while(0)

#define __CFC_RETURN_UNPREPARE_PM_CLK(symname, val)				\
	do {							\
		__CFC_MARK(symname);				\
		cfc_unprepare_pm_clk();				\
		return val;					\
	} while(0)

#define CFC_FUNC_ENTRY(func)					\
	__CFC_MARK(CFC_FUNC_ENTRY_SYM(func))

#define CFC_RETURN_SUCC(func, extra, val)			\
	__CFC_RETURN(CFC_RETURN_SUCC_SYM(func, extra), val)

#define CFC_RETURN_FAIL(func, extra, val)			\
	__CFC_RETURN(CFC_RETURN_FAIL_SYM(func, extra), val)

#define CFC_RETURN_SUCC_WITH_PMCLK(func, extra, val)			\
	__CFC_RETURN_UNPREPARE_PM_CLK(CFC_RETURN_SUCC_SYM(func, extra), val)

#define CFC_RETURN_FAIL_WITH_PMCLK(func, extra, val)			\
	__CFC_RETURN_UNPREPARE_PM_CLK(CFC_RETURN_FAIL_SYM(func, extra), val)

#define CFC_RETURN(func, extra, val)				\
	do {							\
		if (val)					\
			CFC_RETURN_FAIL(func, extra, val);	\
		CFC_RETURN_SUCC(func, extra, val);		\
	} while (0)

#define CFC_RETURN_PMCLK(func, extra, val)				\
	do {							\
		if (val)					\
			CFC_RETURN_FAIL_WITH_PMCLK(func, extra, val);	\
		CFC_RETURN_SUCC_WITH_PMCLK(func, extra, val);		\
	} while (0)

#define CFC_RETURN_PMCLK_ON_COND(func, extra, val, cond)		\
	do {							\
		if (cond)					\
			CFC_RETURN_PMCLK(func, extra, val);		\
		return val;					\
	} while (0)

#endif

#endif
