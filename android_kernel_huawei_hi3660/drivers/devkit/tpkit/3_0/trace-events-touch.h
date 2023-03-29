
#undef TRACE_SYSTEM
#define TRACE_SYSTEM touch

#if !defined(_TRACE_EVENT_TOUCH_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_EVENT_TOUCH_H

#include <linux/tracepoint.h>

#ifndef __TRACE_EVENT_TOUCH_HELPER_FUNCTIONS
#define __TRACE_EVENT_TOUCH_HELPER_FUNCTIONS
enum {
	TOUCH_TRACE_IRQ_TOP,
	TOUCH_TRACE_IRQ_BOTTOM,
	TOUCH_TRACE_I2C,
	TOUCH_TRACE_SPI,
	TOUCH_TRACE_INPUT,
	TOUCH_TRACE_DATA2ALGO,
	TOUCH_TRACE_ALGO_GET_DATA,
	TOUCH_TRACE_ALGO_SET_EVENT,
	TOUCH_TRACE_GET_FRAME,
	TOUCH_TRACE_PEN_REPORT,
	TOUCH_TRACE_GET_ROI_OR_DIFF_DATA,
	TOUCH_TRACE_DIFF_DATA_TO_DEAMON,
};

enum {
	TOUCH_TRACE_FUNC_IN,
	TOUCH_TRACE_FUNC_OUT,
};

#endif
TRACE_DEFINE_ENUM(TOUCH_TRACE_IRQ_TOP);
TRACE_DEFINE_ENUM(TOUCH_TRACE_IRQ_BOTTOM);
TRACE_DEFINE_ENUM(TOUCH_TRACE_I2C);
TRACE_DEFINE_ENUM(TOUCH_TRACE_SPI);
TRACE_DEFINE_ENUM(TOUCH_TRACE_INPUT);
TRACE_DEFINE_ENUM(TOUCH_TRACE_FUNC_IN);
TRACE_DEFINE_ENUM(TOUCH_TRACE_FUNC_OUT);
TRACE_DEFINE_ENUM(TOUCH_TRACE_DATA2ALGO);
TRACE_DEFINE_ENUM(TOUCH_TRACE_ALGO_GET_DATA);
TRACE_DEFINE_ENUM(TOUCH_TRACE_ALGO_SET_EVENT);
TRACE_DEFINE_ENUM(TOUCH_TRACE_GET_FRAME);
TRACE_DEFINE_ENUM(TOUCH_TRACE_PEN_REPORT);
TRACE_DEFINE_ENUM(TOUCH_TRACE_GET_ROI_OR_DIFF_DATA);
TRACE_DEFINE_ENUM(TOUCH_TRACE_DIFF_DATA_TO_DEAMON);

TRACE_EVENT(touch,

	TP_PROTO(int task, int phase, const char *string),

	TP_ARGS(task, phase, string),
	TP_STRUCT__entry(
		__field(	int,	task		)
		__field(	int,	phase		)
		__string(	str,	string		)
	),

	TP_fast_assign(
		__entry->task = task;
		__entry->phase = phase;
		__assign_str(str, string);
	),
	TP_printk("touch_trace: %s %s %s",
		  __print_symbolic(__entry->task,
				   { TOUCH_TRACE_IRQ_TOP, "irq top" },
				   { TOUCH_TRACE_IRQ_BOTTOM, "irq bottom" },
				   { TOUCH_TRACE_I2C, "i2c trans" },
				   { TOUCH_TRACE_SPI, "spi trans" },
				   { TOUCH_TRACE_INPUT, "input report" },
				   { TOUCH_TRACE_DATA2ALGO, "raw info to algo buferr" },
				   { TOUCH_TRACE_ALGO_GET_DATA, "algo get event" },
				   { TOUCH_TRACE_ALGO_SET_EVENT, "algo set event" },
				   { TOUCH_TRACE_GET_FRAME, "daemon get farme" },
				   { TOUCH_TRACE_PEN_REPORT, "pen report" },
				   { TOUCH_TRACE_DIFF_DATA_TO_DEAMON, "read diff tp deamon" },
				   { TOUCH_TRACE_GET_ROI_OR_DIFF_DATA, "get ROI or diff data" }),
		__print_symbolic(__entry->phase,
		  		   { TOUCH_TRACE_FUNC_IN, "in" },
				   { TOUCH_TRACE_IRQ_BOTTOM, "out" }),
		__get_str(str))
);

#endif

/***** NOTICE! The #if protection ends here. *****/

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
/*
 * TRACE_INCLUDE_FILE is not needed if the filename and TRACE_SYSTEM are equal
 */
#define TRACE_INCLUDE_FILE trace-events-touch
#include <trace/define_trace.h>

