#ifndef __LINUX_HWLOG_KERNEL_H
#define __LINUX_HWLOG_KERNEL_H

#include <huawei_platform/log/janklogconstants.h>
#define HW_LOG_PRIO_VERBOSE  (2)
#define HW_LOG_PRIO_DEBUG (3)
#define HW_LOG_PRIO_INFO (4)
#define HW_LOG_PRIO_WARN (5)
#define HW_LOG_PRIO_ERROR (6)

typedef  enum hwlog_id{
	HW_LOG_ID_MIN = 0,
	HW_LOG_ID_EXCEPTION = HW_LOG_ID_MIN,
	HW_LOG_ID_JANK = 1,
	HW_LOG_ID_DUBAI = 2,
	HW_LOG_ID_MAX
} hw_bufid_t;

#if defined (CONFIG_HWLOG_KERNEL)
int hwlog_to_write(int prio, int bufid, const char* tag, const char* fmt, ...);
int hwlog_to_jank(int tag, int prio, const char* fmt, ...);
//for the forward compatibility ,HW_LOG_PRIO_DEBUG level is just for HW service.
//and the interface name stay the same "pr_HW"
//use LOG_HW_W LOG_HW_V  LOG_HW_I   LOG_HW_E  for other purpose
#ifndef pr_jank
#define pr_jank(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_DEBUG, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_JANK_D
#define LOG_JANK_D(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_DEBUG, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_JANK_W
#define LOG_JANK_W(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_WARN, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_JANK_V
#define LOG_JANK_V(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_VERBOSE, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_JANK_I
#define LOG_JANK_I(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_INFO, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_JANK_E
#define LOG_JANK_E(tag, fmt, ...) hwlog_to_jank(tag, HW_LOG_PRIO_ERROR, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_pr
#define HWDUBAI_pr(tag, fmt, ...) hwlog_to_write(HW_LOG_PRIO_DEBUG, HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_LOGV
#define HWDUBAI_LOGV(tag, fmt, ...) hwlog_to_write(HW_LOG_PRIO_VERBOSE, HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_LOGD
#define HWDUBAI_LOGD(tag, fmt,...) hwlog_to_write(HW_LOG_PRIO_DEBUG, HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_LOGI
#define HWDUBAI_LOGI(tag, fmt, ...) hwlog_to_write(HW_LOG_PRIO_INFO,  HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_LOGW
#define HWDUBAI_LOGW(tag, fmt, ...) hwlog_to_write(HW_LOG_PRIO_WARN,  HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#ifndef HWDUBAI_LOGE
#define HWDUBAI_LOGE(tag, fmt, ...) hwlog_to_write(HW_LOG_PRIO_ERROR,  HW_LOG_ID_DUBAI, tag, fmt, ##__VA_ARGS__)
#endif

#else
#define pr_jank(tag, fmt, ...)	(-ENOENT)
#define LOG_JANK_D(tag, fmt, ...)	(-ENOENT)
#define LOG_JANK_W(tag, fmt, ...)	(-ENOENT)
#define LOG_JANK_V(tag, fmt, ...)	(-ENOENT)
#define LOG_JANK_I(tag, fmt, ...)	(-ENOENT)
#define LOG_JANK_E(tag, fmt, ...)	(-ENOENT)

#define HWDUBAI_pr(tag, fmt, ...)	(-ENOENT)
#define HWDUBAI_LOGV(tag, fmt, ...)	(-ENOENT)
#define HWDUBAI_LOGD(tag, fmt, ...)	(-ENOENT)
#define HWDUBAI_LOGI(tag, fmt, ...)	(-ENOENT)
#define HWDUBAI_LOGW(tag, fmt, ...)	(-ENOENT)
#define HWDUBAI_LOGE(tag, fmt, ...)	(-ENOENT)
#endif
#endif
