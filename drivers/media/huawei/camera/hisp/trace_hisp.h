#undef TRACE_SYSTEM
#define TRACE_SYSTEM hisp

#if !defined(__TRACE_HISP_H__) || defined(TRACE_HEADER_MULTI_READ)
#define __TRACE_HISP_H__

#if defined( HISP150_CAMERA  )
#include <media/huawei/hisp150_cfg.h>
#elif defined( HISP160_CAMERA  )
#include <media/huawei/hisp160_cfg.h>
#elif defined( HISP120_CAMERA  )
#include <media/huawei/hisp120_cfg.h>
#elif defined( HISP200_CAMERA  )
#include <media/huawei/hisp200_cfg.h>
#elif defined( HISP250_CAMERA  )
#include <media/huawei/hisp250_cfg.h>
#elif defined( HISP210_CAMERA  )
#include <media/huawei/hisp210_cfg.h>
#else
#include <media/huawei/hisp_cfg.h>
#endif

#include <linux/tracepoint.h>
#include <media/huawei/camera.h>
#include "hisp_intf.h"

DECLARE_EVENT_CLASS(hisp_rpmsg_internel,/* [false alarm]:will not cause error */

	TP_PROTO(hisp_msg_t* msg),

	TP_ARGS(msg),

	TP_STRUCT__entry(
		__field(unsigned,	api_name)
		__field(unsigned,	message_id)
	),

	TP_fast_assign(
		__entry->api_name	    = msg->api_name;
		__entry->message_id     = msg->message_id;
	),

    TP_printk("api=0x%x msg_id=%hd",
              __entry->api_name, __entry->message_id)
);

DEFINE_EVENT(hisp_rpmsg_internel, hisp_rpmsg_send,

	TP_PROTO(hisp_msg_t* msg),

	TP_ARGS(msg)
);

DEFINE_EVENT(hisp_rpmsg_internel, hisp_rpmsg_ept_cb,

	TP_PROTO(hisp_msg_t* msg),

	TP_ARGS(msg)
);

DEFINE_EVENT(hisp_rpmsg_internel, hisp_rpmsg_notify,

	TP_PROTO(hisp_msg_t* msg),

	TP_ARGS(msg)
);

DEFINE_EVENT(hisp_rpmsg_internel, hisp_rpmsg_recv,

	TP_PROTO(hisp_msg_t* msg),

	TP_ARGS(msg)
);

#endif /*  __TRACE_HISP_H__ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../../../../drivers/media/huawei/camera/hisp
#define TRACE_INCLUDE_FILE trace_hisp
#include <trace/define_trace.h>

