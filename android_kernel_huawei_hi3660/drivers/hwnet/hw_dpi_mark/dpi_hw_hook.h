
/* kernel\drivers\net\hw_dpi_mark
 * this file is used by the netfilter hook for
 * app download monitor and ad filter
 *
 * Copyright (C) 2016 HUAWEI Technology Co., ltd.
 *
 */
#ifndef _NF_HW_HOOK
#define _NF_HW_HOOK

#define NETLINK_DOWNLOADEVENT_TO_USER		1000

typedef enum{
	NETLINK_REG_TO_KERNEL   = 0,
	NETLINK_UNREG_TO_KERNEL,
	NETLINK_CMM_TO_KERNEL,
	NETLINK_SET_RULE_TO_KERNEL,
	NETLINK_STOP_MARK,
	NETLINK_START_MARK,
}ntl_cmd_type_t;

typedef enum{
	DMR_MT_BEGIN = 0,
	DMR_MT_TP,
	DMR_MT_END,
}dmr_match_type_t;

typedef enum{
	MR_NOT_REC = 0, /* init state, not recognized */
	MR_TMGP,	/* gaming socket */
	MR_OTHER,	/* other socket type  */
}mark_ret_t;

#endif /*_NF_HW_HOOK*/
