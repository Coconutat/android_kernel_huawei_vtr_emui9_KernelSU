#ifndef __NM_LOG_H__
#define __NM_LOG_H__

#include <huawei_platform/log/hw_log.h>

#undef  HWLOG_TAG
#define HWLOG_TAG	EMCOM_NM
HWLOG_REGIST();

#define NM_ERR(msg, ...) \
	do { \
		hwlog_err("[%s:%d %s]: "msg"\n", kbasename(__FILE__), \
			  __LINE__, __func__, ## __VA_ARGS__); \
	} while (0)

#define NM_INFO(msg, ...) \
	do { \
		hwlog_info("[%s:%d %s]: "msg"\n", kbasename(__FILE__), \
			   __LINE__, __func__, ## __VA_ARGS__); \
	} while (0)

#define NM_DEBUG(msg, ...) \
	do { \
		hwlog_debug("[%s:%d %s]: "msg"\n", kbasename(__FILE__), \
			    __LINE__, __func__, ## __VA_ARGS__); \
	} while (0)
#endif /* __NM_LOG_H__ */