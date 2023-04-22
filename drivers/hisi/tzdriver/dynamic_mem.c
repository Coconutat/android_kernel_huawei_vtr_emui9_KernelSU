#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <stdarg.h>
#include <linux/hisi/hisi_ion.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

#include "tc_ns_log.h"
#include "securec.h"
#include "teek_ns_client.h"
#include "smc.h"
#include "teek_client_api.h"
#include "teek_client_constants.h"
#include "mailbox_mempool.h"
#include "dynamic_mem.h"
/*
extern void create_mapping_late(phys_addr_t phys, unsigned long virt,
		phys_addr_t size, pgprot_t prot);
*/
static const char *dynion_name = "DYN_ION";
static struct ion_client *dynion_client = NULL;
static DEFINE_MUTEX(dynamic_mem_lock);
struct dynamic_mem_list {
	struct list_head list;
};

#define TEE_SERVICE_UT \
{ \
    0x03030303, \
    0x0303, \
    0x0303, \
    { \
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 \
    } \
}

//8780dda1-a49e-45f4-9697-c7ed9e385e83
#define TEE_SECIDENTIFICATION1 \
{\
    0x8780dda1, \
    0xa49e, \
    0x45f4, \
    { \
        0x96, 0x97, 0xc7, 0xed, 0x9e, 0x38, 0x5e, 0x83 \
    } \
}

//335129cd-41fa-4b53-9797-5ccb202a52d4
#define TEE_SECIDENTIFICATION3 \
{\
    0x335129cd, \
    0x41fa, \
    0x4b53, \
    { \
        0x97, 0x97, 0x5c, 0xcb, 0x20, 0x2a, 0x52, 0xd4 \
    } \
}

TEEC_UUID g_dynamic_mem_uuid[]={
	TEE_SECIDENTIFICATION1,
	TEE_SECIDENTIFICATION3,
};

static struct dynamic_mem_list g_dynamic_mem_list;
uint32_t g_dynion_uuid_num = sizeof(g_dynamic_mem_uuid) / sizeof(g_dynamic_mem_uuid[0]);

static int send_loadapp_ion(void)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret;
	struct mb_cmd_pack *mb_pack;
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		return -ENOMEM;
	}
	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = (unsigned int)virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_LOAD_SECURE_APP_ION;
	mb_pack->operation.paramTypes = TEEC_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
	smc_cmd.operation_phys = (unsigned int)virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	ret = (int)TC_NS_SMC(&smc_cmd, 0);
	tlogd("send_loadapp_ion ret=%d\n",ret);
	mailbox_free(mb_pack);
	return ret;
}

static int register_to_tee(struct dynamic_mem_item* mem_item)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret;
	struct mb_cmd_pack *mb_pack;
	if ( !mem_item ) {
		tloge("mem_item is null\n");
		return -1;
	}
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		return -ENOMEM;
	}

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = (unsigned int)virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_ADD_DYNAMIC_ION;
	mb_pack->operation.paramTypes = TEEC_PARAM_TYPES(
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_NONE);

	mb_pack->operation.params[0].value.a = (uint32_t)(mem_item->memory.ion_phys_addr  & 0xFFFFFFFF);
	mb_pack->operation.params[0].value.b = (uint32_t)(mem_item->memory.ion_phys_addr >> 32);
	mb_pack->operation.params[1].value.a = (uint32_t)mem_item->memory.len;
	mb_pack->operation.params[2].value.a = mem_item->configid;
	mb_pack->operation.params[2].value.b = mem_item->cafd[0];
	smc_cmd.operation_phys = (unsigned int)virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	ret = (int)TC_NS_SMC(&smc_cmd, 0);
	mailbox_free(mb_pack);
	return ret;
}

static int unregister_from_tee(struct dynamic_mem_item* mem_item, uint32_t by_fd_flag)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret;
	struct mb_cmd_pack *mb_pack;
	if ( !mem_item ) {
		tloge("mem_item is null\n");
		return -1;
	}
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		return -ENOMEM;
	}
	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = (unsigned int)virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_DEL_DYNAMIC_ION;
	mb_pack->operation.paramTypes = TEEC_PARAM_TYPES(
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_NONE);

	mb_pack->operation.params[0].value.a = (uint32_t)(mem_item->memory.ion_phys_addr  & 0xFFFFFFFF);
	mb_pack->operation.params[0].value.b = (uint32_t)(mem_item->memory.ion_phys_addr >> 32);
	mb_pack->operation.params[1].value.a = (uint32_t)mem_item->memory.len;
	mb_pack->operation.params[2].value.a = mem_item->configid;
	mb_pack->operation.params[2].value.b = by_fd_flag;
	smc_cmd.operation_phys = (unsigned int)virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	ret = (int)TC_NS_SMC(&smc_cmd, 0);
	tlogd("unregister_from_tee ret = %d\n", ret);
	mailbox_free(mb_pack);
	return ret;
}
static struct dynamic_mem_item* find_memitem_by_configid_locked(uint32_t configid)
{
	struct dynamic_mem_item *item;

	list_for_each_entry(item, &g_dynamic_mem_list.list, head) {
		if (item->configid == configid) {
			return item;
		}
	}
	return NULL;

}
static struct dynamic_mem_item* find_memitem_by_uuid_locked(TEEC_UUID *uuid)
{
	struct dynamic_mem_item *item;

	list_for_each_entry(item, &g_dynamic_mem_list.list, head) {
		if (memcmp(&item->uuid,uuid,sizeof(TEEC_UUID)) == 0) {
			return item;
		}
	}
	return NULL;

}

static int alloc_from_hisi(struct dynamic_mem_item* mem_item)
{
	if (!dynion_client || !mem_item)
		return -1;
	mem_item->memory.ion_handle = ion_alloc(dynion_client, mem_item->size, /*lint !e647 */
			SZ_2M, ION_HEAP(ION_MISC_HEAP_ID), ION_FLAG_CACHED);
	if (IS_ERR(mem_item->memory.ion_handle)) {
		tloge("Failed to get ion handle configid=%d\n",mem_item->configid);
		mem_item->memory.ion_phys_addr = 0;
		return -1;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	if(ion_secmem_get_phys(dynion_client, mem_item->memory.ion_handle, &(mem_item->memory.ion_phys_addr), &(mem_item->memory.len)) < 0) {
#else
	if(ion_phys(dynion_client, mem_item->memory.ion_handle, &(mem_item->memory.ion_phys_addr), &(mem_item->memory.len)) < 0) {
#endif
		ion_free(dynion_client,  mem_item->memory.ion_handle);
		tloge("Failed ion_phys configid=%d\n",mem_item->configid);
		mem_item->memory.ion_phys_addr = 0;
		return -1;
	}
	mem_item->memory.ion_virt_addr = (void *)phys_to_virt(mem_item->memory.ion_phys_addr);
	if (IS_ERR_OR_NULL(mem_item->memory.ion_virt_addr))  {
		tloge("Map  font ion mem failed.configid=%d\n",mem_item->configid);
		ion_free(dynion_client, mem_item->memory.ion_handle);
		mem_item->memory.ion_phys_addr = 0;
		return -1;
	}
	__dma_map_area(mem_item->memory.ion_virt_addr,mem_item->memory.len, DMA_BIDIRECTIONAL);
	flush_tlb_all();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(mem_item->memory.ion_phys_addr,
			(unsigned long)phys_to_virt(mem_item->memory.ion_phys_addr), (phys_addr_t)mem_item->memory.len,
			__pgprot(PROT_DEVICE_nGnRE));
#else
	create_mapping_late(mem_item->memory.ion_phys_addr,
			(unsigned long)phys_to_virt(mem_item->memory.ion_phys_addr), (phys_addr_t)mem_item->memory.len,
			__pgprot(PROT_DEVICE_nGnRE));
#endif
	return 0;
}
static void free_to_hisi(struct dynamic_mem_item* mem_item)
{
	if (!dynion_client || !mem_item)
		return ;
	if (IS_ERR(mem_item->memory.ion_handle) || NULL == mem_item->memory.ion_handle ||
		0 == mem_item->memory.ion_phys_addr) {
		return;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(mem_item->memory.ion_phys_addr,
			(unsigned long)phys_to_virt(mem_item->memory.ion_phys_addr), (phys_addr_t)mem_item->memory.len,
			PAGE_KERNEL);
#else
	create_mapping_late(mem_item->memory.ion_phys_addr,
			(unsigned long)phys_to_virt(mem_item->memory.ion_phys_addr), (phys_addr_t)mem_item->memory.len,
			__pgprot(PAGE_KERNEL));
#endif
	flush_tlb_all();

	ion_free(dynion_client, mem_item->memory.ion_handle);
	mem_item->memory.ion_handle = NULL;

	return;
}
int init_dynamic_mem(void)
{
	INIT_LIST_HEAD(&(g_dynamic_mem_list.list));
	dynion_client = hisi_ion_client_create(dynion_name);
	if ( !dynion_client) {
		tloge("create dynion client failed\n");
		return -1;
	}
	return 0;
}
void exit_dynamic_mem(void)
{
	if ( dynion_client ) {
		ion_client_destroy(dynion_client);
		dynion_client = NULL;
	}

}

static int trans_configid2memid(uint32_t configid, uint32_t cafd,  TEEC_UUID* uuid, uint32_t size)
{
	int result = -1;

	if (uuid == NULL) {
		return -1;
	}
	/*if config id is memid map,and can reuse */
	mutex_lock(&dynamic_mem_lock);
	do {
		struct dynamic_mem_item* mem_item = find_memitem_by_configid_locked(configid);
		if (mem_item != NULL) {
			result = -2;
			break;
		}
		/* alloc from hisi */
		mem_item = kzalloc(sizeof(struct dynamic_mem_item),GFP_KERNEL);
		if (mem_item == NULL) {
			result = -1;
			break;
		}

		mem_item->configid = configid;
		mem_item->size = size;
		mem_item->cafd[0]= cafd;
		SET_BIT(mem_item->cafd_count_bitmap,0);
		mem_item->cafd_count++;
		result = memcpy_s(&mem_item->uuid, sizeof(mem_item->uuid), uuid, sizeof(*uuid));
		if (result != 0 ) {
			kfree(mem_item);
			tloge("memcpy_s failed\n");
			break;
		}
		result = alloc_from_hisi(mem_item);
		if(result != 0) {
			tloge("alloc from hisi failed ,ret = %d\n", result);
			kfree(mem_item);
			break;
		}
		/* register to tee*/
		result = register_to_tee(mem_item);
		if(result != 0) {
			tloge("register to tee failed ,result =%d\n", result);
			free_to_hisi(mem_item);
			kfree(mem_item);
			break;
		}
		list_add_tail(&mem_item->head, &g_dynamic_mem_list.list);
		tloge("log import:alloc ion from hisi configid=%d\n", mem_item->configid);
	}while(0);

	mutex_unlock(&dynamic_mem_lock);
	return result;
}
static void release_configid_mem_locked(uint32_t configid, uint32_t by_fd_flag)
{
	int result = -1;
	/*if config id is memid map,and can reuse */
	do {
		struct dynamic_mem_item* mem_item = find_memitem_by_configid_locked(configid);
		if (mem_item == NULL) {
			tloge("fail to find memitem by configid\n");
			break;
		}
		/*register to tee*/
		result = unregister_from_tee(mem_item, by_fd_flag);
		if(result != 0) {
			tloge("unregister_from_tee configid=%d,result=%d\n", mem_item->configid, result);
			break;
		}
		free_to_hisi(mem_item);
		list_del(&mem_item->head);
		kfree(mem_item);
		tloge("log import: free ion to hisi\n");
	}while(0);

	return ;
}
void release_configid_mem(uint32_t configid)
{
	mutex_lock(&dynamic_mem_lock);
	release_configid_mem_locked(configid,0);
	mutex_unlock(&dynamic_mem_lock);
	return ;
}
int load_app_use_configid(uint32_t configid, uint32_t cafd,  TEEC_UUID* uuid, uint32_t size)
{
	int result = -1;
	if ( uuid == NULL) {
		return -1;
	}
	result = trans_configid2memid(configid,cafd,uuid,size);
	if ( result !=0) {
		tloge("trans_configid2memid faild ret=%d\n", result);
		return -1;
	}
	result = send_loadapp_ion();
	if (result != 0) {
		release_configid_mem(configid);
		tloge("send_loadapp_ion failed ret=%d\n", result);
	}
	return result;
}
void kill_ion_by_uuid(TEEC_UUID* uuid)
{
	if (uuid == NULL) {
		tloge("uuid is null\n");
		return;
	}
	mutex_lock(&dynamic_mem_lock);
	do {
		struct dynamic_mem_item* mem_item = find_memitem_by_uuid_locked(uuid);
		if (mem_item == NULL) {
			break;
		}
		tlogd("kill ION by UUID\n");
		release_configid_mem_locked(mem_item->configid, 0);
	}while(0);
	mutex_unlock(&dynamic_mem_lock);
}

void kill_ion_by_cafd(unsigned int cafd)
{
	tlogd("kill_ion_by_cafd:\n");
	mutex_lock(&dynamic_mem_lock);
	struct dynamic_mem_item *item;
	struct dynamic_mem_item *temp;
	int32_t i;

	list_for_each_entry_safe(item, temp, &g_dynamic_mem_list.list, head) {
		for (i = 0; i < CAFD_MAX; i++) {
			if (item->cafd[i] == cafd) {
				item->cafd_count--;
				item->cafd[i] = 0;
				tlogd("uuid %x cafd_count-- = %d\n",item->uuid.timeLow,item->cafd_count);
				CLR_BIT(item->cafd_count_bitmap, i);
				if(item->cafd_count == 0){
					tlogd("kill ION by CAFD!!\n");
					release_configid_mem_locked(item->configid, 1);
					break;
				}
			}
		}
	}
	mutex_unlock(&dynamic_mem_lock);
}

int32_t get_cafd_id(uint32_t cafd_count_bitmap)
{
	int32_t id = -1;
	int32_t i;

	for (i = 0; i < CAFD_MAX; i++) {
		if (!(cafd_count_bitmap & (0x1 << i))) {
			id = i;
			break;
		}
	}

	return id;
}

int add_cafd_count_by_uuid(TEEC_UUID* uuid, uint32_t cafd)
{
	int32_t index = 0;
	int ret = -1;
	if (uuid == NULL) {
		tloge("DYNION add cafd's uuid is null, ignore it\n");
		return 0;
	}
	mutex_lock(&dynamic_mem_lock);
	do {
		struct dynamic_mem_item* mem_item = find_memitem_by_uuid_locked(uuid);
		if (mem_item == NULL) {
			tlogd("fail to find item\n");
			ret = 0;
			break;
		}
		index = get_cafd_id(mem_item->cafd_count_bitmap);
		if(index >= 0){
			mem_item->cafd[index] = cafd;
			mem_item->cafd_count++;
			tlogd("uuid %x cafd_count++ = %d\n", uuid->timeLow, mem_item->cafd_count);
			SET_BIT(mem_item->cafd_count_bitmap,index);
			ret = 0;
		} else {
			tloge("DYNION add cafd failed count: %d. MAX:%d\n", mem_item->cafd_count, CAFD_MAX);
			ret = -1;
		}
	}while(0);
	mutex_unlock(&dynamic_mem_lock);

	return ret;
}

int is_used_dynamic_mem(TEEC_UUID *uuid)
{
	uint32_t i;
	if (uuid == NULL) {
		tloge("param is null\n");
		return 0;
	}
	for (i=0; i < g_dynion_uuid_num; i++) {
		if (!memcmp(uuid, &g_dynamic_mem_uuid[i], sizeof(TEEC_UUID))) {
			return 1;
		}
	}
	return 0;
}