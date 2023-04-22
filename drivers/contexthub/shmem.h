#ifndef __LINUX_SHMEM_H__
#define __LINUX_SHMEM_H__
#include "../huawei_platform/inputhub/hi3660/protocol.h"

#include <iomcu_ddr_map.h>

#define HISI_RESERVED_CH_BLOCK_SHMEM_PHYMEM_BASE       DDR_SHMEM_LARGE_BLK_BASE_AP
#define HISI_RESERVED_CH_BLOCK_SHMEM_PHYMEM_SIZE       DDR_SHMEM_LARGE_BLK_SIZE

extern int shmem_notifier_register(obj_tag_t module_id,
	void (*notifier_call)(void __iomem *buf_addr, unsigned int buf_size));
extern int shmem_notifier_unregister(obj_tag_t module_id);
extern int shmem_send(obj_tag_t module_id, const void *usr_buf, unsigned int usr_buf_size);
extern int contexthub_shmem_init(void);
extern const pkt_header_t *shmempack(const char *buf, unsigned int length);
extern unsigned int shmem_get_capacity(void);
extern int shmem_send_resp(const pkt_header_t * head);

#endif
