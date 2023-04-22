#ifndef __HISI_L3SHARE_H__
#define __HISI_L3SHARE_H__




typedef enum _MODULE_ID {
	DSS_IDLE = 0,
	VDEC,
	VENC,
	AUDIO,
} module_id;

typedef enum _SHARE_TYPE {
	PRIVATE = 0,
	SHARE,
} share_type;

struct l3_cache_request_params {
	module_id id;
	unsigned int request_size;
	share_type type;
};

struct l3_cache_release_params {
	module_id id;
};

#define	L3C_ACP_PENDING	0x1
#define	L3C_ACP_RELEASE	0x2

/*
L3 Cache request and release.
It will open/close all ACP port and poweron/poweroff L3 partition for ACP.
Must be called in pair. Call l3_cache_request twice will increase a vote.
Params ignored for test. You can pass 0 or anything.
*/
int l3_cache_request(struct l3_cache_request_params *request_params);
int l3_cache_release(struct l3_cache_release_params *release_params);

/*
Controls a group of ways to be marked as private to cpu/acp.
Last 4 bits of input params indicates which 1MB cpu/acp will use (L3 is 4MB totally).
If one bit is both assigned by cpu and acp, then that 1MB will be shared between them.
Diffrent masters that use acp will share the acp partition in L3 cache.
Default configuration: 0x1 & 0xe, private 3MB for acp.
*/

#ifdef CONFIG_HISI_L3CACHE_SHARE
int register_l3c_acp_notifier(struct notifier_block *nb);
int unregister_l3c_acp_notifier(struct notifier_block *nb);
#endif

#endif
