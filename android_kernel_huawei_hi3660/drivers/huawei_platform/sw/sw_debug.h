#ifndef __SW_DEBUG_H__
#define __SW_DEBUG_H__


/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "sw_core.h"

#define CHR_LOG(prio, tag, fmt...)


enum SW_LOGLEVLE{
	SW_LOG_ALERT = 0,
	SW_LOG_ERR = 1,
	SW_LOG_WARNING = 2,
	SW_LOG_INFO = 3,
	SW_LOG_DEBUG = 4,
};

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define SW_PRINT_FUNCTION_NAME				do { \
		if (SW_LOG_DEBUG <= g_SW_LOGlevel) \
		{ \
			printk(KERN_DEBUG KBUILD_MODNAME ":D]%s]" ,__func__);     \
		} \
	}while(0)

#define SW_PRINT_DBG(s, args...)            do{ \
		if (SW_LOG_DEBUG <= g_SW_LOGlevel) \
		{ \
			printk(KERN_DEBUG KBUILD_MODNAME ":D]%s]" s,__func__, ## args); \
		}\
	}while(0)

#define SW_PRINT_INFO(s, args...)           do{ \
		if (SW_LOG_INFO <= g_SW_LOGlevel) \
		{ \
			printk(KERN_DEBUG KBUILD_MODNAME ":I]%s]" s,__func__, ## args);\
			CHR_LOG(CHR_LOG_INFO, CHR_LOG_TAG_PLAT, s, ##args); \
		} \
	}while(0)

#define SW_PRINT_SUC(s, args...)            do{ \
		if (SW_LOG_INFO <= g_SW_LOGlevel) \
		{ \
			printk(KERN_DEBUG KBUILD_MODNAME ":S]%s]" s,__func__, ## args); \
			CHR_LOG(CHR_LOG_INFO, CHR_LOG_TAG_PLAT, s, ##args); \
		} \
	}while(0)

#define SW_PRINT_WARNING(s, args...)        do{ \
		if (SW_LOG_WARNING <= g_SW_LOGlevel) \
		{ \
			printk(KERN_WARNING KBUILD_MODNAME ":W]%s]" s,__func__, ## args);\
			CHR_LOG(CHR_LOG_WARN, CHR_LOG_TAG_PLAT, s, ##args); \
		} \
	}while(0)

#define SW_PRINT_ERR(s, args...)            do{ \
		if (SW_LOG_ERR <= g_SW_LOGlevel) \
		{ \
			printk(KERN_ERR KBUILD_MODNAME ":E]%s]" s,__func__, ## args); \
			CHR_LOG(CHR_LOG_ERROR, CHR_LOG_TAG_PLAT, s, ##args); \
		} \
	}while(0)

#define SW_PRINT_ALERT(s, args...)          do{ \
		if (SW_LOG_ALERT <= g_SW_LOGlevel) \
		{ \
			printk(KERN_ALERT KBUILD_MODNAME ":ALERT]%s]" s,__func__, ## args); \
			CHR_LOG(CHR_LOG_ERROR, CHR_LOG_TAG_PLAT, s, ##args); \
		} \
	}while(0)





struct sw_debug_list {
	char *hid_debug_buf;
	int head;
	int tail;
	struct fasync_struct *fasync;
	struct sw_device *hdev;
	struct list_head node;
	struct mutex read_mutex;
};

#ifdef __cplusplus
#if __cplusplus
 extern "C" {
#endif
#endif
extern void sw_debug_dump_data(const u8 *data,int count);

#ifdef __cplusplus
#if __cplusplus
        }
#endif
#endif

#endif /* SW_DEBUG_H */