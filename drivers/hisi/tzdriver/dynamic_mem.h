#ifndef _DYNAMIC_MMEM_H_
#define _DYNAMIC_MMEM_H_
#include <linux/hisi/hisi_ion.h>
#include "teek_ns_client.h"
#define CAFD_MAX         10 //concurrent opened session count
#define SET_BIT(map, bit) (map |= (0x1<<(bit)))
#define CLR_BIT(map, bit) (map &= (~(unsigned)(0x1<<(bit))))
struct sg_memory {
	struct ion_handle *ion_handle;
	ion_phys_addr_t ion_phys_addr;
	size_t len;
	void *ion_virt_addr;
};
struct dynamic_mem_item{
	struct list_head head;
	uint32_t configid;
	uint32_t size;
	struct sg_memory memory;
	uint32_t cafd[CAFD_MAX];
	uint32_t cafd_count_bitmap;
	uint32_t cafd_count;
	TEEC_UUID uuid;
};
int init_dynamic_mem(void);
void exit_dynamic_mem(void);
int load_app_use_configid(uint32_t configid, uint32_t cafd,  TEEC_UUID* uuid, uint32_t size);
void kill_ion_by_cafd(unsigned int cafd);
void kill_ion_by_uuid(TEEC_UUID* uuid);
int add_cafd_count_by_uuid(TEEC_UUID* uuid, uint32_t cafd);
int is_used_dynamic_mem(TEEC_UUID *uuid);
#endif
