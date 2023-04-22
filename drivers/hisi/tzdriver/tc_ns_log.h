#ifndef TC_NS_LOG_H_
#define TC_NS_LOG_H_

#include <linux/printk.h>

enum {
	TZ_DEBUG_VERBOSE = 0,
	TZ_DEBUG_DEBUG,
	TZ_DEBUG_INFO,
	TZ_DEBUG_WARN,
	TZ_DEBUG_ERROR,
};

#ifdef DEF_ENG
#define TEE_LOG_MASK TZ_DEBUG_INFO
#else
#define TEE_LOG_MASK TZ_DEBUG_WARN
#endif

#define tlogv(fmt, args...) /*lint -save -e774*/ \
do { \
	if (TZ_DEBUG_VERBOSE >= TEE_LOG_MASK) \
		pr_info("(%i, %s)%s: " fmt, current->pid, current->comm,  __func__, ## args); \
} while (0) /*lint -restore*/


#define tlogd(fmt, args...) /*lint -save -e774*/ \
do { \
	if (TZ_DEBUG_DEBUG >= TEE_LOG_MASK) \
		pr_info("(%i, %s)%s: " fmt, current->pid, current->comm,  __func__, ## args); \
} while (0) /*lint -restore*/


#define tlogi(fmt, args...) /*lint -save -e774*/ \
do { \
	if (TZ_DEBUG_INFO >= TEE_LOG_MASK) \
		pr_info("(%i, %s)%s: " fmt, current->pid, current->comm,  __func__, ## args); \
} while (0) /*lint -restore*/


#define tlogw(fmt, args...) /*lint -save -e774*/ \
do { \
	if (TZ_DEBUG_WARN >= TEE_LOG_MASK) \
		pr_warn("(%i, %s)%s: " fmt, current->pid, current->comm,  __func__, ## args);\
} while (0) /*lint -restore*/


#define tloge(fmt, args...) \
		pr_err("(%i, %s)%s: " fmt, current->pid, current->comm,  __func__, ## args)

#endif
