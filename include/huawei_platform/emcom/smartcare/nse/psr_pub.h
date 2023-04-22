

#ifndef __PSR_PUB_H__
#define __PSR_PUB_H__

#include <linux/kernel.h>

#define PSR_MAX_UINT8		0xFF
#define PSR_MAX_UINT16		0xFFFF
#define PSR_MAX_UINT32		0XFFFFFFFF
#define PSR_MAX_HTTP_RSP_CODE   999

#define PSR_EN_INVALID		-1
#define PSR_EN_BUTT		0x7FFFFFFF

#define PSR_MAX_RECU_DEPTH	20

#define PSR_LOG(ret_code)						\
	do {								\
		printk(KERN_ERR "[PSR]: [%s %s] [L: %u] [%u]\n",	\
			kbasename(__FILE__), __func__, __LINE__, ret_code); \
	} while (0)

extern uint32_t psr_deinit(void);
extern uint8_t psr_proc(uint32_t proto, uint32_t direction, uint64_t ts,
			uint32_t pkt_len, uint8_t *pkt_data, void *ctx_buf,
			void *result);

#endif/* __PSR_PUB_H__ */
