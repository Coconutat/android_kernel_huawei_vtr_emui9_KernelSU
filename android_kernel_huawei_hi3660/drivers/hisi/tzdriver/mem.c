/*******************************************************************************
 * All rights reserved, Copyright (C) huawei LIMITED 2012
 *
 * This source code has been made available to you by HUAWEI on an
 * AS-IS basis. Anyone receiving this source code is licensed under HUAWEI
 * copyrights to use it in any way he or she deems fit, including copying it,
 * modifying it, compiling it, and redistributing it either with or without
 * modifications. Any person who transfers this source code or any derivative
 * work must include the HUAWEI copyright notice and this paragraph in
 * the transferred software.
*******************************************************************************/
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/vmalloc.h>
#include <linux/of_reserved_mem.h>

#include "mem.h"
#include "smc.h"
#include "tc_ns_client.h"
#include "teek_ns_client.h"
#include "agent.h"
#include "securec.h"
#include "tc_ns_log.h"
#include "mailbox_mempool.h"

void tc_mem_free(TC_NS_Shared_MEM *shared_mem)
{
	if (NULL == shared_mem)
		return;

	if (shared_mem->from_mailbox) {
		if (shared_mem->kernel_addr != NULL) {
			mailbox_free(shared_mem->kernel_addr);
		}
	}
	else {
		if (shared_mem->kernel_addr != NULL) {
			vfree(shared_mem->kernel_addr);
		}
	}

	kfree(shared_mem);
}

TC_NS_Shared_MEM *tc_mem_allocate(size_t len, bool from_mailbox)
{
	TC_NS_Shared_MEM *shared_mem = NULL;
	void *addr = NULL;


	shared_mem = kmalloc(sizeof(TC_NS_Shared_MEM), GFP_KERNEL|__GFP_ZERO);
	if (!shared_mem) {
		tloge("shared_mem kmalloc failed\n");
		return ERR_PTR(-ENOMEM);
	}

	if (from_mailbox)
		addr = mailbox_alloc(len, MB_FLAG_ZERO);
	else {
		len = ALIGN(len, SZ_4K);
		if (len > MAILBOX_POOL_SIZE) {
			tloge("alloc sharemem size(%zu) is too large\n", len);
			kfree(shared_mem);
			return ERR_PTR(-EINVAL);
		}
		addr = vmalloc_user(len);
	}

	if (!addr) {
		tloge("alloc maibox failed\n");
		kfree(shared_mem);
		return ERR_PTR(-ENOMEM);
	}

	shared_mem->from_mailbox = from_mailbox;
	shared_mem->kernel_addr = addr;
	shared_mem->len = len;

	return shared_mem;
}

static u64 g_ion_mem_addr;
static u64 g_ion_mem_size;

static int supersonic_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem != NULL) {
		g_ion_mem_addr = rmem->base;
		g_ion_mem_size = rmem->size;
	} else {
		tloge("rmem is NULL\n");
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(supersonic, "hisi-supersonic", supersonic_reserve_tee_mem); /*lint !e611 */

static u64 g_secfacedetect_mem_addr;
static u64 g_secfacedetect_mem_size;
static int secfacedetect_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem != NULL) {
		g_secfacedetect_mem_addr = rmem->base;
		g_secfacedetect_mem_size = rmem->size;
	} else {
		tloge("secfacedetect_reserve_tee_mem mem is NULL\n");
	}
	return 0;
}
RESERVEDMEM_OF_DECLARE(secfacedetect, "hisi-secfacedetect",secfacedetect_reserve_tee_mem); /*lint !e611 */

static u64 g_voiceid_addr;
static u64 g_voiceid_size;
static int voiceid_reserve_tee_mem(struct reserved_mem *rmem)
{
	if (rmem != NULL) {
		g_voiceid_addr = rmem->base;
		g_voiceid_size = rmem->size;
	} else {
		tloge("voiceid_reserve_tee_mem  mem is NULL.\n");
	}
	return 0;
}
RESERVEDMEM_OF_DECLARE(voiceid, "hisi-voiceid",voiceid_reserve_tee_mem); /*lint !e611 */


#define ION_MEM_MAX_SIZE 5
struct register_ion_mem_tag {
	uint32_t size;
	uint64_t memaddr[ION_MEM_MAX_SIZE];
	uint32_t memsize[ION_MEM_MAX_SIZE];
};
/*send the ion static memory to tee.*/
int TC_NS_register_ion_mem(void)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret = 0;
	struct mb_cmd_pack *mb_pack;
	struct register_ion_mem_tag *memtag;
	uint32_t pos = 0;
	tloge("ion mem static reserved for tee face=%d,finger=%d,voiceid=%d\n",(uint32_t)g_secfacedetect_mem_size,(uint32_t)g_ion_mem_size,(uint32_t)g_voiceid_size);


	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("mailbox alloc failed\n");
		return -ENOMEM;
	}
	memtag = mailbox_alloc(sizeof(struct register_ion_mem_tag),0);
	if (memtag == NULL) {
		mailbox_free(mb_pack);
		return -ENOMEM;
	}
	if ((u64)0 != g_ion_mem_addr && (u64)0 != g_ion_mem_size) {
		memtag->memaddr[pos] = g_ion_mem_addr;
		memtag->memsize[pos] = g_ion_mem_size;
		pos++;
	}
	if ((u64)0 != g_secfacedetect_mem_addr && (u64)0 != g_secfacedetect_mem_size) {
		 memtag->memaddr[pos] = g_secfacedetect_mem_addr;
		 memtag->memsize[pos] = g_secfacedetect_mem_size;
		 pos++;
	}
	if ((u64)0 != g_voiceid_addr && (u64)0 != g_voiceid_size) {
		 memtag->memaddr[pos] = g_voiceid_addr;
		 memtag->memsize[pos] = g_voiceid_size;
		 pos++;
	}

	memtag->size = pos;


	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_ION_MEM;

	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_MEMREF_INPUT;
	mb_pack->operation.params[0].memref.buffer = virt_to_phys((void*)memtag);
	mb_pack->operation.buffer_h_addr[0] = virt_to_phys((void *)memtag) >> 32;
	mb_pack->operation.params[0].memref.size =  sizeof(struct register_ion_mem_tag);

	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/

	ret = TC_NS_SMC(&smc_cmd, 0);
	mailbox_free(mb_pack);
	mailbox_free(memtag);
	if (ret) {
	    tloge("Send ion mem info failed.\n");
	}

	return ret;
}

int tc_mem_init(void)
{
	int ret;

	tlogi("tc_mem_init\n");

	ret = mailbox_mempool_init();
	if (ret) {
		tloge("tz mailbox init failed\n");
		return -ENOMEM;
	}

	return 0;
}

void tc_mem_destroy(void)
{
	tlogi("tc_client exit\n");

	mailbox_mempool_destroy();
}
