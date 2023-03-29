#ifndef _XT_OWNER_MATCH_H
#define _XT_OWNER_MATCH_H

#include <linux/types.h>

enum {
	XT_OWNER_UID    = 1 << 0,
	XT_OWNER_GID    = 1 << 1,
	XT_OWNER_SOCKET = 1 << 2,
	XT_OWNER_PID    = 1 << 3,/*pid_min and pid_max store in xt_owner_match_info.gid_min and xt_owner_match_info.gid_max*/
};

struct xt_owner_match_info {
	__u32 uid_min, uid_max;
	__u32 gid_min, gid_max;
	__u8 match, invert;
	/* To avoid modifing the struct length,
	* reuse the old fields for pid_min/max. */
	#define xt_pid_min gid_min
	#define xt_pid_max gid_max
};

#endif /* _XT_OWNER_MATCH_H */
