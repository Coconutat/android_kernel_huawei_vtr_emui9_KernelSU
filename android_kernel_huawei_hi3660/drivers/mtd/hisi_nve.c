/*
 * FileName: kernel/drivers/mtd/nve.c
 * Description: complement NV partition(or called block) read and write
 * in kernel.
 * Copyright (C) Hisilicon technologies Co., Ltd All rights reserved.
 * Revision history:
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/random.h>
/* #include <linux/mtd/mtd.h> */
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/compat.h>

#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "hisi_nve.h"
#ifdef CONFIG_CRC_SUPPORT
#include "hisi_nve_crc.h"
#endif
#if CONFIG_HISI_NVE_WHITELIST
#include "hisi_nve_whitelist.h"
#endif
#include "hisi_nve_log_blacklist.h"

static struct semaphore nv_sem;
static struct class *nve_class;
static unsigned int dev_major_num;
static struct NVE_struct *g_nve_struct;
static int hisi_nv_setup_ok = 0;
static char *nve_block_device_name = NULL;
static char log_nv_info[NV_INFO_LEN];
static char temp_nv_info[NV_INFO_LEN];
#if CONFIG_HISI_NVE_WHITELIST
extern unsigned int get_userlock(void);
static int nve_whitelist_en = 1;
#endif

/*
 * Function name:update_header_valid_nvme_items.
 * Discription:update the actual valid item in ramdisk
 * Parameters:
 * @ nve_ramdisk:the ramdisk stores the total nv partition
 */
static void update_header_valid_nvme_items(struct NVE_partittion *nve_ramdisk)
{
	unsigned int i;
	struct NV_items_struct nve_item;
	unsigned int valid_items;
	for(i = 0;i < nve_ramdisk->header.valid_items;i++){
		/*find nve item's nv_number and check*/
		nve_item = nve_ramdisk->NV_items[i];
		if(i != nve_item.nv_number)
			break;
	}
	valid_items = i;
	/*update ram valid_items*/
	nve_ramdisk->header.valid_items = valid_items;
}

#ifdef CONFIG_CRC_SUPPORT
/*
 * Function name:check_crc_for_valid_items.
 * Discription:check the crc for nv items
 * Parameters:
 * @ offset:the start item in nv valid items
 * @ check_items:the number of check items
 * @ nve_ramdisk:the ramdisk stores the total nv partition
 * return value:
 * 0:check success;-1:check failed
 */
static int check_crc_for_valid_items(int nv_item_start, int check_items, struct NVE_partittion *nve_ramdisk)
{
	int i;
	uint8_t crc_data[NVE_NV_CRC_HEADER_SIZE + NVE_NV_CRC_DATA_SIZE];
	struct NV_items_struct *nv_item;
	uint32_t temp_crc;
	int nv_number;
	/*nv_item_start means the start nv number, range is 0-1022*/
	if(nv_item_start >= NV_ITEMS_MAX_NUMBER || nv_item_start < 0){
		pr_err("invalid nv_item_start in check crc fuction, nv_item_start = %d\n", nv_item_start);
		return -1;
	}
	/*check_items means the number of chek nv items, range is 1-1023*/
	if(check_items > NV_ITEMS_MAX_NUMBER || check_items <= 0){
		pr_err("invalid check_items in check crc fuction, check_items = %d\n", check_items);
		return -1;
	}
	for(i = 0;i < check_items;i++){
		nv_number = nv_item_start + i;
		if(nv_number >= NV_ITEMS_MAX_NUMBER){
			pr_err("invalid nv number in check crc fuction, nv_item_start = %d, check_items = %d\n", nv_item_start, check_items);
			return -1;
		}
		nv_item = &nve_ramdisk->NV_items[nv_number];
		memcpy(crc_data, nv_item, (unsigned long)NVE_NV_CRC_HEADER_SIZE);
		memcpy(crc_data + NVE_NV_CRC_HEADER_SIZE, nv_item->nv_data, (unsigned long)NVE_NV_CRC_DATA_SIZE);
		temp_crc = ~ crc32c_nve(CRC32C_REV_SEED, crc_data, sizeof(crc_data));

		if(nv_item->crc != temp_crc){
			pr_err("kernel nv item {%d}, old_crc_value = 0x%x, new_crc_value = 0x%x, crc_data = %s\n", nv_number, nv_item->crc, temp_crc, crc_data);
			break;
		}
	}
	if(i == check_items)
		return 0;
	else{
		pr_err("crc check failed, item_num = %d\n", i);
		return -1;
	}
}

/*
 * Function name:caculate_crc_for_valid_items.
 * Discription:caculate crc for nv item and update
 * Parameters:
 * @ nv_items:the pointer of nv item
 */
static void caculate_crc_for_valid_items(struct NV_items_struct *nv_items)
{
	uint8_t crc_data[NVE_NV_CRC_HEADER_SIZE + NVE_NV_CRC_DATA_SIZE];
	memcpy(crc_data, nv_items, (unsigned long)NVE_NV_CRC_HEADER_SIZE);
	memcpy(crc_data + NVE_NV_CRC_HEADER_SIZE, nv_items->nv_data, (unsigned long)NVE_NV_CRC_DATA_SIZE);
	nv_items->crc = ~ crc32c_nve(CRC32C_REV_SEED, crc_data, sizeof(crc_data));
}
#endif

/*
 * Function name:nve_increment.
 * Discription:complement NV block' increment automatically, when current block
 * has been writeen, block index will pointer to next block, and if current
 * block
 * is maximum block count ,then block index will be assigned "1", ensuring all
 * of
 * NV block will be used and written circularly.
 */
static void nve_increment(struct NVE_struct *nvep)
{
	if(nvep == NULL){
		pr_err("[NVE][%s]nve struct is not init %d.\n",__func__, __LINE__);
		return;
	}
	if (nvep->nve_current_id >= nvep->nve_partition_count - 1)
		nvep->nve_current_id = 1;
	else
		nvep->nve_current_id++;

	return;
}

/*
 * Function name:nve_decrement.
 * Discription:complement NV block' decrement automatically, when current block
 * has been writeen, block index will pointer to next block, if write failed, we will recover the index
 */
static void nve_decrement(struct NVE_struct *nvep)
{
	if(nvep == NULL){
		pr_err("[NVE][%s]nve struct is not init %d.\n",__func__, __LINE__);
		return;
	}
	/*we only use 1-7 partition, if id is 1, next decrement id is 7, after init nvep->nve_partition_count is 8*/
	if (nvep->nve_current_id <= 1)
		nvep->nve_current_id = nvep->nve_partition_count - 1;
	else
		nvep->nve_current_id--;

	return;
}

/*
 * Function name:nve_read.
 * Discription:read NV partition.
 * Parameters:
 *          @ mtd:struct mtd_info pointer.
 *          @ from:emmc start block number that will be read.
 *          @ len:total bytes that will be read from emmc.
 *          @ buf:buffer used to store bytes that is read from emmc.
 */
static int nve_read(loff_t from, size_t len, u_char *buf)
{
	int ret;
	int fd;
	/*lint -e501*/
	mm_segment_t oldfs = get_fs();
	set_fs(get_ds());
	/*lint +e501*/
	fd = (int)sys_open(nve_block_device_name, O_RDONLY,
		      (int)S_IRWXU | S_IRWXG | S_IRWXO);

	if (fd < 0) {
		pr_err("[NVE][%s]open nv block device failed, and fd = %x!\n",__func__, fd);
		ret = -ENODEV;
		goto out;
	}

	ret = (int)sys_lseek((unsigned int)fd, from, SEEK_SET);
	if (ret == -1) {
		pr_err("[NVE][%s] Fatal seek error, read flash from = "
				"0x%llx, len = 0x%zx, ret = 0x%x.\n",
		       __func__, from, len, ret);
		ret = -EIO;
		goto out;
	}

	ret = (int)sys_read((unsigned int)fd, (char *)buf, len);
	if (ret == -1) {
		pr_err("[NVE][%s] Fatal read error, read flash from = "
				"0x%llx, len = 0x%zx, ret = 0x%x.\n",
		       __func__, from, len, ret);
		ret = -EIO;
		goto out;
	}

	sys_close((unsigned int)fd);

	set_fs(oldfs);

	return 0;

out:
	if (fd >= 0) {
		sys_close((unsigned int)fd);
	}
	set_fs(oldfs);
	return ret;
}

/*
 * Function name:nve_write.
 * Discription:write NV partition.
 * Parameters:
 *          @ mtd:struct mtd_info pointer.
 *          @ from:emmc start block number that will be written.
 *          @ len:total bytes that will be written from emmc.
 *          @ buf:given buffer whose bytes will be written to emmc.
 */
static int nve_write(loff_t from, size_t len, u_char *buf)
{
	int ret;
	int fd;
	/*lint -e501*/
	mm_segment_t oldfs = get_fs();
	set_fs(get_ds());
	/*lint +e501*/
	fd = (int)sys_open(nve_block_device_name, O_RDWR,
		      (int)S_IRWXU | S_IRWXG | S_IRWXO);

	if (fd < 0) {
		pr_err("[NVE][%s]open nv block device failed, and fd = %x!\n",
		       __func__, fd);
		ret = -ENODEV;
		goto out;
	}

	ret = (int)sys_lseek((unsigned int)fd, from, SEEK_SET);
	if (ret == -1) {
		pr_err("[NVE][%s] Fatal seek error, read flash from = "
				"0x%llx, len = 0x%zx, ret = 0x%x.\n",
		       __func__, from, len, ret);
		ret = -EIO;
		goto out;
	}

	ret = (int)sys_write((unsigned int)fd, (char *)buf, len);
	if (ret == -1) {
		pr_err("[NVE][%s] Fatal write error, read flash from "
				"= 0x%llx, len = 0x%zx, ret = 0x%x.\n",
		       __func__, from, len, ret);
		ret = -EIO;
		goto out;
	}

	ret = (int)sys_fsync((unsigned int)fd);
	if (ret < 0) {
		pr_err("[NVE][%s] Fatal sync error, read flash from = "
				"0x%llx, len = 0x%zx, ret = 0x%x.\n",
		       __func__, from, len, ret);
		ret = -EIO;
		goto out;
	}
	sys_close((unsigned int)fd);

	set_fs(oldfs);
	return 0;
out:
	if (fd >= 0) {
		sys_close((unsigned int)fd);
	}
	set_fs(oldfs);
	return ret;
}

/*
 * Function name:nve_check_partition.
 * Discription:check current NV partition is valid partition or not by means of
 * comparing current partition's name to NVE_HEADER_NAME.
 * Parameters:
 *          @ nve:struct NVE_struct pointer.
 *          @ index:indicates current NV partion that will be checked.
 * return value:
 *          @ 0 - current parition is valid.
 *          @ others - current parition is invalid.
 */
static int nve_check_partition(struct NVE_partittion *ramdisk, uint32_t index)
{
	int ret;
	struct NVE_partition_header *nve_partition_header = &ramdisk->header;

	ret = nve_read((loff_t)index * NVE_PARTITION_SIZE, (size_t)NVE_PARTITION_SIZE, (u_char *)ramdisk);
	if (ret) {
		pr_err("[NVE][%s]nve_read error in line %d!\n",
		       __func__, __LINE__);
	}else{
		/*update for valid nvme items*/
		update_header_valid_nvme_items(ramdisk);

		/*compare partition_name with const name,if return 0,then current partition is valid */
		ret = strncmp(NVE_HEADER_NAME, nve_partition_header->nve_partition_name, strlen(NVE_HEADER_NAME));
		if(ret){
			//printk(KERN_ERR"nve_check_partition header failed,index = %d", index);
			return ret;
		}

		#ifdef CONFIG_CRC_SUPPORT
		if(nve_partition_header->nve_crc_support == NVE_CRC_SUPPORT_VERSION){
			/*check the crc for valid nvme items*/
			ret = check_crc_for_valid_items(0, (int)nve_partition_header->valid_items, ramdisk);
			if(ret){
				pr_err("nve_check_partition{%d} valid_items:%d, version = %d, crc_support = %d\n", index, nve_partition_header->valid_items, nve_partition_header->nve_version, nve_partition_header->nve_crc_support);
				pr_err("nve crc check error:ret = %d, index = %d\n", ret, index);
			}
		}
		#endif
	}
	return ret;
}

/*
 * Function name:nve_find_valid_partition.
 * Discription:find valid NV partition in terms of checking every
 * partition circularly. when two or more NV paritions are both valid,
 * nve_age will be used to indicates which one is really valid, i.e. the
 * partition whose age is the biggest is valid partition.
 */
static void nve_find_valid_partition(struct NVE_struct *nvep)
{
	uint32_t i;
	uint32_t age_temp = 0;
	int partition_valid = 0;
	struct NVE_partition_header *nve_partition_header = &nvep->nve_store_ramdisk->header;
	nvep->nve_current_id = NVE_INVALID_NVM;
	for (i = 1; i < nvep->nve_partition_count; i++) {
		partition_valid = nve_check_partition(nvep->nve_store_ramdisk, i);

		if (partition_valid)
			continue;

		if (nve_partition_header->nve_age > age_temp) {
			nvep->nve_current_id = i;
			age_temp = nve_partition_header->nve_age;
		}
	}

	pr_info("[NVE][%s]current_id = %d valid_items = %d, version = %d, crc_support = %d\n", __func__, nvep->nve_current_id, nve_partition_header->valid_items, nve_partition_header->nve_version, nve_partition_header->nve_crc_support);

	return;
}

static int write_ramdisk_to_device(unsigned int id, struct NVE_partittion *ramdisk){
	struct NVE_partition_header *nve_partition_header = &ramdisk->header;
	int ret;
	loff_t start_addr;
	unsigned int nve_update_age = nve_partition_header->nve_age + 1;
	if(id >= NVE_PARTITION_COUNT){
		pr_err("[NVE][%s]invalid id in line %d!\n", __func__, __LINE__);
		return -1;
	}
	/*write to next partition a invalid age*/
	nve_partition_header->nve_age = NVE_PARTITION_INVALID_AGE;
	/*write the old partition head*/
	start_addr = (((loff_t)id + 1) * NVE_PARTITION_SIZE) - 512;
	ret = nve_write(start_addr, (size_t)512, ((unsigned char *)ramdisk + NVE_PARTITION_SIZE - 512));
	if(ret){
		pr_err("[NVE][%s]write old nv partition head failed in line %d!\n", __func__, __LINE__);
		/*recover the age*/
		nve_partition_header->nve_age = nve_update_age - 1;
		return ret;
	}
	/*write the partition data*/
	start_addr = (loff_t)id * NVE_PARTITION_SIZE;
	ret = nve_write(start_addr, (size_t)NVE_PARTITION_SIZE - 512, (unsigned char *)ramdisk);
	if(ret){
		pr_err("[NVE][%s]write nv partition data failed in line %d!\n", __func__, __LINE__);
		/*recover the age*/
		nve_partition_header->nve_age = nve_update_age - 1;
		return ret;
	}

	 /*after writing partition to device, read the partition again to check, if check not pass, return error and not update header age*/
	ret = nve_check_partition(ramdisk, id);
	if(ret){
		pr_err("[NVE][%s]after writing partition to device, read the partition again to check failed!\n", __func__);
		/*recover the age*/
		nve_partition_header->nve_age = nve_update_age - 1;
		return ret;
	}

	/*update the partition head age*/
	nve_partition_header->nve_age = nve_update_age;
	/*write the latest partition head*/
	start_addr = (((loff_t)id + 1) * NVE_PARTITION_SIZE) - 512;
	ret = nve_write(start_addr, (size_t)512, ((unsigned char *)ramdisk + NVE_PARTITION_SIZE - 512));
	if(ret){
		pr_err("[NVE][%s]write nv latest partition head failed in line %d!\n", __func__, __LINE__);
		return ret;
	}
	return ret;
}

static int erase_ramdisk_to_device(unsigned int id, struct NVE_partittion *ramdisk){
	int ret;
	loff_t start_addr;
	if(id >= NVE_PARTITION_COUNT){
		pr_err("[NVE][%s]invalid id in line %d!\n", __func__, __LINE__);
		return -1;
	}
	(void)memset((void *)ramdisk, 0, (unsigned long)NVE_PARTITION_SIZE);
	/*erase partition head */
	start_addr = (((loff_t)id + 1) * NVE_PARTITION_SIZE) - 512;
	ret = nve_write(start_addr,  (size_t)512, ((unsigned char *)ramdisk + NVE_PARTITION_SIZE - 512));
	if(ret){
		pr_err("[NVE][%s]erase partition head failed in line %d!\n", __func__, __LINE__);
		return ret;
	}
	/*erase partition data*/
	start_addr = (loff_t)id * NVE_PARTITION_SIZE;
	ret = nve_write(start_addr, (size_t)NVE_PARTITION_SIZE - 512, (unsigned char *)ramdisk);
	if(ret){
		pr_err("[NVE][%s]erase partition data failed in line %d!\n", __func__, __LINE__);
		return ret;
	}
	return ret;
}

/*
 * Function name:nve_update_and_check_item
 * Discription:update the nv item
 * Parameters:
 *          @ 0  - success
 *          @ -1 - failure
 */
int nve_update_and_check_item(unsigned int update_items, unsigned int valid_check_items, struct NVE_partittion *nve_store_ramdisk, struct NVE_partittion *nve_update_ramdisk)
{
	unsigned int i;
	#ifdef CONFIG_CRC_SUPPORT
	struct NVE_partition_header *nve_store_partition_header = &nve_store_ramdisk->header;
	struct NVE_partition_header *nve_update_partition_header = &nve_update_ramdisk->header;
	#endif
	/*this place is update the latest nvpartition's nv items*/
	for (i = 0; i < update_items; i++) {
		/*min valid items of two partition should check the name and property*/
		if(i < valid_check_items){
			/*check the name is normal and reserved*/
			if(strncmp(nve_store_ramdisk->NV_items[i].nv_name, nve_update_ramdisk->NV_items[i].nv_name, (unsigned int)sizeof(nve_store_ramdisk->NV_items[i].nv_name))){
				pr_err("current nv [%d] name is different, please notoce!\n", i);
			}
			if (nve_store_ramdisk->NV_items[i].nv_property){
				#ifdef CONFIG_CRC_SUPPORT
					/*when version is same, skip non-volatile NV item;when version is diff, to do something to non-volatile NV item*/
				if(nve_update_partition_header->nve_crc_support != nve_store_partition_header->nve_crc_support){
						/*when version is diff, update is support CRC, only caculate the old item crc and change, then skip*/
					if(nve_update_partition_header->nve_crc_support == NVE_CRC_SUPPORT_VERSION)
						caculate_crc_for_valid_items(&nve_store_ramdisk->NV_items[i]);
						/*when version is diff, update is not support CRC, clear old item crc, then skip*/
					else
						nve_store_ramdisk->NV_items[i].crc = 0;
				}
				#endif
				continue;
			}
			if(nve_store_ramdisk->NV_items[i].valid_size != nve_update_ramdisk->NV_items[i].valid_size)
				pr_warn("current nv [%d] valid size is different, old valid size = %d, new valid size = %d!\n", i, nve_store_ramdisk->NV_items[i].valid_size, nve_update_ramdisk->NV_items[i].valid_size);
		}
		/*update current partition ram*/
		memcpy((void *)&nve_store_ramdisk->NV_items[i], (void *)&nve_update_ramdisk->NV_items[i], sizeof(struct NV_items_struct));
	}
	return 0;
}


/*
 * Function name:nve_restore.
 * Discription:NV is divided into 8 partitions(partition0 - parititon 7),
 * when we need to add new NV items, we should update partition0 first,
 * and then restore parition0 to current valid partition which shall be
 * one of partition0 - partition7.
 * Parameters:
 *          @ 0  - success
 *          @ -1 - failure
 */
static int nve_restore(struct NVE_struct *nvep)
{
	int ret;
	unsigned int valid_check_items = 0;
	unsigned int update_items = 0;
	unsigned int nve_age_temp = 0;
	struct NVE_partition_header *nve_store_partition_header;
	struct NVE_partition_header *nve_update_partition_header;

	if (NVE_INVALID_NVM == nvep->nve_current_id) {
		ret = nve_read((loff_t)0, (size_t)NVE_PARTITION_SIZE, (u_char *)nvep->nve_store_ramdisk);
		if (ret) {
			pr_err("[NVE][%s] nve read error %d in line [%d].\n",  __func__, ret, __LINE__);
			return -ENODEV;
		}
		/*update nv ram's header valid items*/
		update_header_valid_nvme_items(nvep->nve_store_ramdisk);
		nve_store_partition_header = &nvep->nve_store_ramdisk->header;
		nvep->nve_current_id = 0;
		valid_check_items = 0;
		update_items = nve_store_partition_header->valid_items;
	} else {
		if (nve_read((loff_t)nvep->nve_current_id * NVE_PARTITION_SIZE, (size_t)NVE_PARTITION_SIZE, (u_char *)nvep->nve_store_ramdisk)) {
				pr_err("[NVE][%s] nve read error in line [%d].\n", __func__, __LINE__);
				return -EFAULT;
		}
		/*update nv ram's header valid items*/
		update_header_valid_nvme_items(nvep->nve_store_ramdisk);
		nve_store_partition_header = &nvep->nve_store_ramdisk->header;

		if (nve_read((loff_t)0, (size_t)NVE_PARTITION_SIZE, (u_char *)nvep->nve_update_ramdisk)) {
				pr_err("[NVE][%s] nve read error in line [%d].\n", __func__, __LINE__);
				return -EFAULT;
		}
		/*update nv ram's header valid items*/
		update_header_valid_nvme_items(nvep->nve_update_ramdisk);
		nve_update_partition_header = &nvep->nve_update_ramdisk->header;

		/*get the min items in partition 0 and current partition*/
		/*check the valid head for min items*/
		valid_check_items = min(nve_store_partition_header->valid_items, nve_update_partition_header->valid_items);
		/*get the max items in partition 0 and current partition*/
		/*if 0 partition valid item is less than current partition , the delete items should also be updated*/
		update_items = max(nve_store_partition_header->valid_items, nve_update_partition_header->valid_items);
		pr_info("valid_items [%d] and update_items [%d], update version = %d, crc_support = %d!\n", valid_check_items, update_items, nve_update_partition_header->nve_version, nve_update_partition_header->nve_crc_support);

		ret = nve_update_and_check_item(update_items, valid_check_items, nvep->nve_store_ramdisk, nvep->nve_update_ramdisk);
		if(ret){
			pr_err("[nve_restore]ERROR!!!nve_update_and_check_item failed!\n");
			return ret;
		}

		/*when current partition header is not valid, we will update the header and set the age*/
		if (strncmp(NVE_HEADER_NAME, nve_store_partition_header->nve_partition_name, (unsigned int)strlen(NVE_HEADER_NAME))) {
			/*nve partition is corrupt,we need to recover the header too*/
			pr_err("[nve_restore]ERROR!!! partition %d is corrupted invalidly,recover the header!\n", nvep->nve_current_id);
			/*store the orignal age*/
			nve_age_temp = nve_store_partition_header->nve_age;
			/*update current header*/
			memcpy(nve_store_partition_header, nve_update_partition_header, PARTITION_HEADER_SIZE);
			/*set the orignal age*/
			nve_store_partition_header->nve_age = nve_age_temp;
		}

		nve_store_partition_header->valid_items = nve_update_partition_header->valid_items;
		nve_store_partition_header->nve_version = nve_update_partition_header->nve_version;
		nve_store_partition_header->nve_crc_support = nve_update_partition_header->nve_crc_support;
	}

	nve_increment(nvep);
	/*write to next partition*/
	ret = write_ramdisk_to_device(nvep->nve_current_id, nvep->nve_store_ramdisk);
	if(ret){
		pr_err("[NVE][%s]write to device failed in line [%d].\n", __func__, __LINE__);
		/*recover the current id*/
		nve_decrement(nvep);
		return ret;
	}

	/*if nve item update items is not same we will restore an other one*/
	if(valid_check_items != update_items){
		nve_increment(nvep);
		/*write to next partition*/
		ret = write_ramdisk_to_device(nvep->nve_current_id, nvep->nve_store_ramdisk);
		if(ret){
			pr_err("[NVE][%s]write to device failed in line [%d].\n", __func__, __LINE__);
			/*recover the current id*/
			nve_decrement(nvep);
			return ret;
		}
	}
	/*OK we will update the current ramdisk*/
	memcpy(nvep->nve_current_ramdisk, nvep->nve_store_ramdisk, NVE_PARTITION_SIZE);
	/*clear 0 partition*/
	ret = erase_ramdisk_to_device(0, nvep->nve_update_ramdisk);
	if(ret){
		pr_err("[NVE][%s]erase 0 partition failed in line [%d].\n", __func__, __LINE__);
		return ret;
	}

	return ret;
}

/* test NV items in kernel. if you want to use this, please set macro
 * TEST_NV_IN_KERNEL to "1".
 */
#if 1
#define NVE_TEST_TIMES 20
#define NVE_TEST_STRESS_TIMES 50
#define NVE_TEST_WRITE_VALUE "test_data"
#define NVE_TEST_VALID_SIZE sizeof(NVE_TEST_WRITE_VALUE)
#define NVE_TEST_OK 0
#define NVE_TEST_ERR 1
extern u64 hisi_getcurtime(void);
uint64_t g_nve_write_start_time;
uint64_t g_nve_write_end_time;
uint64_t g_nve_read_start_time;
uint64_t g_nve_read_end_time;
uint64_t g_nve_cost_time;
struct hisi_nve_info_user nv_read_info;
struct hisi_nve_info_user nv_write_info;
struct hisi_nve_info_user nv_init_info;
#if CONFIG_HISI_NVE_WHITELIST
void nve_whitelist_en_set(int en_whitelist)
{
	nve_whitelist_en = en_whitelist;
}

void nve_dump_whitelist(void)
{
	unsigned int i;

	pr_err("nv_num whitelist:\n");
	for (i = 0; i < ARRAY_SIZE(nv_num_whitelist); i++) {
		pr_err("%d ", nv_num_whitelist[i]);
	}
	pr_err("\n\n");

	for (i = 0; i < ARRAY_SIZE(nv_process_whitelist); i++) {
		pr_err("%s\n", nv_process_whitelist[i]);
	}
	pr_err("\n");
}
#endif /* CONFIG_HISI_NVE_WHITELIST */

static int nve_print_partition_test(struct NVE_partittion *nve_partition)
{
	struct NVE_partition_header *nve_partiiton_header;
	uint32_t i;
	nve_partiiton_header = &nve_partition->header;

	pr_err("[NVE]partition name  :%s\n", nve_partiiton_header->nve_partition_name);
	pr_err("[NVE]nve version     :%d\n", nve_partiiton_header->nve_version);
	pr_err("[NVE]nve age         :%d\n", nve_partiiton_header->nve_age);
	pr_err("[NVE]nve blockid     :%d\n", nve_partiiton_header->nve_block_id);
	pr_err("[NVE]nve blockcount  :%d\n", nve_partiiton_header->nve_block_count);
	pr_err("[NVE]valid items:%d\n", nve_partiiton_header->valid_items);
	pr_err("nv checksum     :%d\n", nve_partiiton_header->nv_checksum);
	pr_err("nv crc support     :%d\n", nve_partiiton_header->nve_crc_support);

	for (i = 0; i < nve_partiiton_header->valid_items; i++) {
		pr_err("%d %s %d %d 0x%x %s\n",
			nve_partition->NV_items[i].nv_number,
			nve_partition->NV_items[i].nv_name,
			nve_partition->NV_items[i].nv_property,
			nve_partition->NV_items[i].valid_size,
			nve_partition->NV_items[i].crc,
			nve_partition->NV_items[i].nv_data);
	}
	return 0;
}

int nve_write_test(uint32_t nv_item_num, uint32_t valid_size)
{
	int ret;
	unsigned char *data = (unsigned char *)NVE_TEST_WRITE_VALUE;
	memset(&nv_write_info, 0, sizeof(nv_write_info));
	strncpy(nv_write_info.nv_name, "NVTEST", (sizeof("NVTEST") - 1));
	nv_write_info.nv_name[sizeof("NVTEST") - 1] = '\0';

	nv_write_info.nv_number = nv_item_num;

	nv_write_info.valid_size = valid_size;
	nv_write_info.nv_operation = NV_WRITE;
	memset(nv_write_info.nv_data, 0x0, (size_t)NVE_NV_DATA_SIZE);
	memcpy(nv_write_info.nv_data, data, (size_t)(strlen((const char *)data) + 1));

	ret = hisi_nve_direct_access(&nv_write_info);
	if (ret == 0) {
		pr_err("test nv write 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x success!\n",
		       nv_write_info.nv_data[0], nv_write_info.nv_data[1],
		       nv_write_info.nv_data[2], nv_write_info.nv_data[3],
		       nv_write_info.nv_data[4], nv_write_info.nv_data[5]);
		return NVE_TEST_OK;
	} else {
		pr_err("test nv write faild!\n");
		return NVE_TEST_ERR;
	}
}
EXPORT_SYMBOL(nve_write_test);

int nve_read_init_value(uint32_t nv_item_num, uint32_t valid_size){
	int ret;
	memset(&nv_init_info, 0, sizeof(nv_init_info));
	strncpy(nv_init_info.nv_name, "NVTEST", (sizeof("NVTEST") - 1));
	nv_init_info.nv_name[sizeof("NVTEST") - 1] = '\0';
	nv_init_info.nv_number = nv_item_num;
	nv_init_info.valid_size = valid_size;
	nv_init_info.nv_operation = NV_READ;

	ret = hisi_nve_direct_access(&nv_init_info);
	if (ret == 0) {
		pr_err("test nv read 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x success!\n",
		       nv_init_info.nv_data[0], nv_init_info.nv_data[1],
		       nv_init_info.nv_data[2], nv_init_info.nv_data[3],
		       nv_init_info.nv_data[4], nv_init_info.nv_data[5]);
		pr_err("test nv read value:%s!\n", nv_init_info.nv_data);
		return NVE_TEST_OK;
	} else {
		pr_err("test nv read faild!\n");
		return NVE_TEST_ERR;
	}
}

int nve_write_init_value(void){
	int ret;
	nv_init_info.nv_operation = NV_WRITE;
	ret = hisi_nve_direct_access(&nv_init_info);
	if (ret == 0) {
		pr_err("test nv write 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x success!\n",
		       nv_init_info.nv_data[0], nv_init_info.nv_data[1],
		       nv_init_info.nv_data[2], nv_init_info.nv_data[3],
		       nv_init_info.nv_data[4], nv_init_info.nv_data[5]);
		return NVE_TEST_OK;
	} else {
		pr_err("test nv write faild!\n");
		return NVE_TEST_ERR;
	}
}


int nve_read_test(uint32_t nv_item_num, uint32_t valid_size)
{
	int ret;
	memset(&nv_read_info, 0, sizeof(nv_read_info));
	strncpy(nv_read_info.nv_name, "NVTEST", (sizeof("NVTEST") - 1));
	nv_read_info.nv_name[sizeof("NVTEST") - 1] = '\0';
	nv_read_info.nv_number = nv_item_num;
	nv_read_info.valid_size = valid_size;
	nv_read_info.nv_operation = NV_READ;

	ret = hisi_nve_direct_access(&nv_read_info);
	if (ret == 0) {
		pr_err("test nv read 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x success!\n",
		       nv_read_info.nv_data[0], nv_read_info.nv_data[1],
		       nv_read_info.nv_data[2], nv_read_info.nv_data[3],
		       nv_read_info.nv_data[4], nv_read_info.nv_data[5]);
		pr_err("test nv read value:%s!\n", nv_read_info.nv_data);
		return NVE_TEST_OK;
	} else {
		pr_err("test nv read faild!\n");
		return NVE_TEST_ERR;
	}
}
EXPORT_SYMBOL(nve_read_test);

int nve_read_write_auto(uint32_t nv_item_num, uint32_t valid_size)
{
	int i;
	int ret;
	ret = nve_read_init_value(nv_item_num, valid_size);
	if(ret){
		pr_err("nve_read_init_value test failed!\n");
		return ret;
	}
	for (i = 0; i < NVE_TEST_TIMES; i++) {
		ret = nve_write_test(nv_item_num, valid_size);
		if(ret){
			pr_err("nve_write_test test failed!\n");
			return ret;
		}
		ret = nve_read_test(nv_item_num, valid_size);
		if(ret){
			pr_err("nve_read_test test failed!\n");
			return ret;
		}
		if (strncmp((const char*)nv_read_info.nv_data, (const char*)nv_write_info.nv_data,
			    (size_t)NVE_TEST_VALID_SIZE) == 0){
			pr_err("test nve write and read value is same, test "
			       "successed!\n");
		}else{
			pr_err("test nve write and read value is not same, "
			       "test failed!\n");
			return NVE_TEST_ERR;
		}
	}
	ret = nve_write_init_value();
	if(ret){
		pr_err("nve_write_init_value test failed!\n");
		return ret;
	}
	pr_err("test nve auto end!\n");
	return ret;
}

EXPORT_SYMBOL(nve_read_write_auto);

uint64_t nve_write_time_test(uint32_t nv_item_num, uint32_t valid_size)
{
	uint64_t total_nve_write_time = 0;
	uint64_t average_nve_write_time;
	int i;
	int ret;
	ret = nve_read_init_value(nv_item_num, valid_size);
	if(ret){
		pr_err("nve_read_init_value test failed!\n");
		return NVE_TEST_ERR;
	}
	for(i = 0;i < NVE_TEST_STRESS_TIMES;i++){
		g_nve_write_start_time = hisi_getcurtime();
		ret = nve_write_test(nv_item_num, valid_size);
		g_nve_write_end_time = hisi_getcurtime();
		if(ret){
			pr_err("nve_write_test test failed!\n");
			return NVE_TEST_ERR;
		}
		g_nve_cost_time = g_nve_write_end_time - g_nve_write_start_time;
		total_nve_write_time += g_nve_cost_time;
		pr_err("test nv cost time [%llu]ns,test_time = %d!\n", g_nve_cost_time, i);
		msleep(2);
	}
	ret = nve_write_init_value();
	if(ret){
		pr_err("nve_write_init_value test failed!\n");
		return NVE_TEST_ERR;
	}
	average_nve_write_time = total_nve_write_time / NVE_TEST_STRESS_TIMES;
	return average_nve_write_time;
}

EXPORT_SYMBOL(nve_write_time_test);

uint64_t nve_read_time_test(uint32_t nv_item_num, uint32_t valid_size)
{
	uint64_t total_nve_read_time = 0;
	uint64_t average_nve_read_time;
	int i;
	int ret;
	for(i = 0;i < NVE_TEST_STRESS_TIMES;i++){
		g_nve_read_start_time = hisi_getcurtime();
		ret = nve_read_test(nv_item_num, valid_size);
		g_nve_read_end_time = hisi_getcurtime();
		if(ret){
			pr_err("nve_write_test test failed!\n");
			return NVE_TEST_ERR;
		}
		g_nve_cost_time = g_nve_read_end_time - g_nve_read_start_time;
		total_nve_read_time += g_nve_cost_time;
		pr_err("test nv cost time [%llu]ns,test_time = %d!\n", g_nve_cost_time, i);
		msleep(2);
	}
	average_nve_read_time = total_nve_read_time / NVE_TEST_STRESS_TIMES;
	return average_nve_read_time;
}

EXPORT_SYMBOL(nve_read_time_test);
#ifdef CONFIG_CRC_SUPPORT
uint64_t nve_item_check_crc_test(int nv_number){
	int ret;
	int i;
	uint64_t total_check_crc_time = 0;
	uint64_t average_check_crc_time;
	uint64_t check_crc_start_time;
	uint64_t check_crc_end_time;
	if(g_nve_struct->nve_current_ramdisk->header.nve_crc_support != NVE_CRC_SUPPORT_VERSION){
		pr_err("current partition is not support CRC!\n");
		return NVE_TEST_ERR;
	}
	for(i = 0;i < NVE_TEST_STRESS_TIMES;i++){
		check_crc_start_time = hisi_getcurtime();
		ret = check_crc_for_valid_items(nv_number, 1, g_nve_struct->nve_current_ramdisk);
		check_crc_end_time = hisi_getcurtime();
		if(!ret)
			pr_err("NVE item CRC check success!cost time [%llu]ns,test_time = %d\n!", (check_crc_end_time - check_crc_start_time), i);
		else{
			pr_err("NVE item CRC check failed!\n");
			return NVE_TEST_ERR;
		}
		total_check_crc_time += (check_crc_end_time - check_crc_start_time);
		msleep(2);
	}
	average_check_crc_time = total_check_crc_time / NVE_TEST_STRESS_TIMES;
	return average_check_crc_time;
}
EXPORT_SYMBOL(nve_item_check_crc_test);

uint64_t nve_current_partition_check_crc_test(void){
	int ret;
	int i;
	uint64_t total_check_crc_time = 0;
	uint64_t average_check_crc_time;
	uint64_t check_crc_start_time;
	uint64_t check_crc_end_time;
	struct NVE_partition_header *nve_partition_header = &g_nve_struct->nve_current_ramdisk->header;
	if(nve_partition_header->nve_crc_support != NVE_CRC_SUPPORT_VERSION){
		pr_err("current partition is not support CRC!");
		return NVE_TEST_ERR;
	}

	for(i = 0;i < NVE_TEST_STRESS_TIMES;i++){
		check_crc_start_time = hisi_getcurtime();
		ret = check_crc_for_valid_items(0, (int)nve_partition_header->valid_items, g_nve_struct->nve_current_ramdisk);
		check_crc_end_time = hisi_getcurtime();
		if(!ret)
			pr_err("NVE partition CRC check success!cost time [%llu]ns,test_time = %d!\n", (check_crc_end_time - check_crc_start_time), i);
		else{
			pr_err("NVE partition CRC check failed!\n");
			return NVE_TEST_ERR;
		}
		total_check_crc_time += (check_crc_end_time - check_crc_start_time);
		msleep(2);
	}
	average_check_crc_time = total_check_crc_time / NVE_TEST_STRESS_TIMES;
	return average_check_crc_time;
}
EXPORT_SYMBOL(nve_current_partition_check_crc_test);
#endif
void nve_all_test(void)
{
	nve_print_partition_test(g_nve_struct->nve_current_ramdisk);
}
EXPORT_SYMBOL(nve_all_test);
#endif

/*
 * Function name:nve_open_ex.
 * Discription:open NV device.
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
static int read_nve_to_ramdisk(void)
{
	int ret = 0;
	/*the driver is not initialized successfully, return error*/
	if (NULL == g_nve_struct) {
		ret = -ENOMEM;
		pr_err("[NVE][%s]:g_nve_struct has not been alloced.\n", __func__);
		goto out;
	}

	/*Total avaliable NV partition size is 4M,but we only use 1M*/
	g_nve_struct->nve_partition_count = NVE_PARTITION_COUNT;

	/*
	 * partition count must bigger than 3,
	 * one for partition 0,one for update, the other for runtime.
	 */
	if (g_nve_struct->nve_partition_count <= 3) {
		ret = -ENODEV;
		goto out;
	}

	nve_find_valid_partition(g_nve_struct);

	/*check partiton 0 is valid or not*/
	ret = nve_check_partition(g_nve_struct->nve_store_ramdisk, 0);

	if (!ret) {
		/*partiton 0 is valid, restore it to current partition*/
		pr_info("[NVE]partition0 is valid, restore it to "
				"current partition.\n");
		ret = nve_restore(g_nve_struct);
	}

	if (ret) {
		if (NVE_INVALID_NVM == g_nve_struct->nve_current_id) {
			pr_err("[NVE][%s]: no valid NVM.\n", __func__);
			ret = -ENODEV;
			goto out;
		} else
			ret = 0;
	}

	/*read the current NV partiton and store into Ramdisk */
	if (nve_read((unsigned long)g_nve_struct->nve_current_id * NVE_PARTITION_SIZE, (size_t)NVE_PARTITION_SIZE, (unsigned char *)g_nve_struct->nve_current_ramdisk)) {
		pr_err("[init_nve]nve_read error!\n");
		return -1;
	}
	update_header_valid_nvme_items(g_nve_struct->nve_current_ramdisk);

	g_nve_struct->initialized = 1;
out:
	return ret;
}
/*
 * Function name:nve_out_log.
 * Discription:output log of reading and writing nv.
 * Parameters:
 *          @ struct hisi_nve_info_user *user_info pointer.
	    @ bool isRead
 * return value:
 *          void
 */
static void nve_out_log(struct hisi_nve_info_user *user_info, int isRead)
{
	int index;
	if (NULL == user_info) {
		pr_devel("[NVE][%s]:user_info is null! \n",
		       __func__);
		return;
	}
	if (isRead) {
		pr_devel("[NVE][%s]:read nv:ID= %d \n", __func__,
		       user_info->nv_number);
	} else {
		pr_devel("[NVE][%s]:write nv:ID= %d \n", __func__,
		       user_info->nv_number);
	}
	for (index = 0; index < (int)ARRAY_SIZE(nv_num_blacklist); index++) {
		if (user_info->nv_number == nv_num_blacklist[index]) {
			pr_devel("[NVE][%s]:nv:ID= %d is in blacklist. Forbid print nve info!\n", __func__,
		       user_info->nv_number);
			return;
		}
	}
	memset(log_nv_info, 0, sizeof(log_nv_info));
	memset(temp_nv_info, 0, sizeof(temp_nv_info));
	for (index = 0; index < (int)user_info->valid_size; index++) {
		snprintf(temp_nv_info, (size_t)(NV_INFO_LEN - 1), "%s,0x%x", log_nv_info,
			 user_info->nv_data[index]);/*[false alarm]: temp_nv_info length is NV_INFO_LEN*/
		memset(log_nv_info, 0, sizeof(log_nv_info));
		snprintf(log_nv_info, (size_t)(NV_INFO_LEN - 1), "%s", temp_nv_info);
		if ((index % 20 == 0) && (index > 0)) {
			pr_err("%s\n", log_nv_info);
			memset(log_nv_info, 0, sizeof(log_nv_info));
		}
		memset(temp_nv_info, 0, sizeof(temp_nv_info));
	}
	pr_devel("%s\n", log_nv_info);
#ifdef CONFIG_HISI_FACTORY_VERSION
	if (isRead) {
		pr_err("[NVE][%s]:read data = %s\n", __func__,
		       user_info->nv_data);
	} else {
		pr_err("[NVE][%s]:write data = %s\n", __func__,
		       user_info->nv_data);
	}
#else
	if (isRead) {
		pr_err("[NVE][%s]:read data \n", __func__);
	} else {
		pr_err("[NVE][%s]:write data \n", __func__);
	}
#endif
	return;
}
#ifdef CONFIG_CRC_SUPPORT
int nve_check_crc_and_recover(int nv_item_start, int check_items, struct NVE_struct *nvep)
{
	int ret;
	/*crc check for ram to one item*/
	ret = check_crc_for_valid_items(nv_item_start, check_items, nvep->nve_current_ramdisk);
	/*retry to read to ramdisk*/
	if(ret){
		pr_err("[NVE], one item crc check failed, %d!\n", ret);
		/*this place can optmize*/
		nve_find_valid_partition(nvep);
		if(NVE_INVALID_NVM == nvep->nve_current_id){
			pr_err("[NVE]can't find the valid partition in 1-7!\n");
			return -EIO;
		}else{
			/*read the current NV partiton and store into Ramdisk */
			if (nve_read((unsigned long)nvep->nve_current_id * NVE_PARTITION_SIZE, NVE_PARTITION_SIZE, (unsigned char *)nvep->nve_current_ramdisk)) {
				pr_err("[NVE]nve_read error!\n");
				return -EIO;
			}
			update_header_valid_nvme_items(nvep->nve_current_ramdisk);
		}
	}
	return 0;
}
#endif

#if CONFIG_HISI_NVE_WHITELIST
static bool nve_number_in_whitelist(uint32_t nv_number)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(nv_num_whitelist); i++) {
		if (nv_number == nv_num_whitelist[i])
			return true;
	}

	return false;
}

static bool nve_process_in_whitelist(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(nv_process_whitelist); i++) {
		if (!strncmp(current->comm, nv_process_whitelist[i], strlen(nv_process_whitelist[i]))) {

			return true;
		}
	}

	return false;
}

static int hisi_nve_whitelist_check(struct hisi_nve_info_user *user_info)
{

	/*input parameter invalid, return.*/
	if (NULL == user_info) {
		pr_err("[NVE][%s] input parameter is NULL.\n", __func__);
		return -1;
	}

	if (nve_whitelist_en && (user_info->nv_operation != NV_READ)) {
		if ( nve_number_in_whitelist(user_info->nv_number) && (!nve_process_in_whitelist() || !get_userlock()) ) {
			pr_err("%s nv_number: %d process %s is not in whitelist, or phone was unlocked. Forbid write to NVE!\n",
					__func__, user_info->nv_number, current->comm);

			return -EPERM;
		}
	}

	return 0;
}
#endif /* CONFIG_HISI_NVE_WHITELIST  */

int hisi_nve_direct_access_for_ramdisk(struct hisi_nve_info_user *user_info){
	int ret = 0;
	struct NV_items_struct *nv_item;
	struct NV_items_struct nv_item_backup;
	struct NVE_partition_header *nve_partition_header;

#if CONFIG_HISI_NVE_WHITELIST
	if (hisi_nve_whitelist_check(user_info)) {
		pr_err("%s hisi_nve_whitelist_check Failed!\n", __func__);
		return -EINVAL;
	}
#else
	/*input parameter invalid, return.*/
	if (NULL == user_info) {
		pr_err("[NVE][%s] input parameter is NULL.\n", __func__);
		return -1;
	}
#endif /* CONFIG_HISI_NVE_WHITELIST */

	/*get nve partition header to check nv number*/
	nve_partition_header = &g_nve_struct->nve_current_ramdisk->header;
	if (user_info->nv_number >= nve_partition_header->valid_items) {
		pr_err("[NVE][%s] NV items[%d] is not defined.\n", __func__, user_info->nv_number);
		ret = -EINVAL;
		goto out;
	}

	/*check nv valid size, it is same to old*/
	nv_item = &g_nve_struct->nve_current_ramdisk->NV_items[user_info->nv_number];
	if (user_info->valid_size > nv_item->valid_size || user_info->valid_size == 0) {
		pr_err("[NVE][%s]Bad parameter:valid size is error, %d, %d!\n", __func__, user_info->nv_number, user_info->valid_size);
		user_info->valid_size = nv_item->valid_size;
	}
	if(user_info->valid_size > NVE_NV_DATA_SIZE){
		pr_err("[NVE][%s] user info valid size is error, %d.\n", __func__, user_info->valid_size);
		ret = -EINVAL;
		goto out;
	}
	if (NV_READ == user_info->nv_operation) {
		#ifdef CONFIG_CRC_SUPPORT
		if(nve_partition_header->nve_crc_support == NVE_CRC_SUPPORT_VERSION){
			ret = nve_check_crc_and_recover((int)user_info->nv_number, 1, g_nve_struct);
			if(ret){
				pr_err("[NVE]nve_check_crc_and_recover failed, ret = %d\n", ret);
				return ret;
			}
		}
		#endif
		/*read nv from ramdisk*/
		memcpy(user_info->nv_data, nv_item->nv_data, (size_t)user_info->valid_size);
		nve_out_log(user_info, true);
	} else {
		#ifdef CONFIG_CRC_SUPPORT
		if(nve_partition_header->nve_crc_support == NVE_CRC_SUPPORT_VERSION){
			ret = nve_check_crc_and_recover(0, (int)nve_partition_header->valid_items, g_nve_struct);
			if(ret){
				pr_err("[NVE]nve_check_crc_and_recover failed, ret = %d\n", ret);
				return ret;
			}
		}
		#endif
		/*backup the original item, if write to device failed, we will recover the item*/
		memcpy(&nv_item_backup, nv_item, sizeof(struct NV_items_struct));
		/*write nv to ram */
		memset(nv_item->nv_data, 0x0, (unsigned long)NVE_NV_DATA_SIZE);
		memcpy(nv_item->nv_data, user_info->nv_data, (unsigned long)user_info->valid_size);	/*[false alarm]:valid_size <= NVE_NV_DATA_SIZE */
		#ifdef CONFIG_CRC_SUPPORT
		if(nve_partition_header->nve_crc_support == NVE_CRC_SUPPORT_VERSION){
			/*caculate crc to update*/
			caculate_crc_for_valid_items(nv_item);
		}
		#endif
		/*update the current id*/
		nve_increment(g_nve_struct);
		/*write the total ramdisk to device*/
		ret = write_ramdisk_to_device(g_nve_struct->nve_current_id, g_nve_struct->nve_current_ramdisk);
		if(ret){
			pr_err("[NVE][%s]write to device failed in line %d, and nv_number = %d!\n", __func__, __LINE__, user_info->nv_number);
			/*if write to device failed, we will recover something*/
			/*recover the item*/
			memcpy(nv_item, &nv_item_backup, sizeof(struct NV_items_struct));
			/*recover the current id*/
			nve_decrement(g_nve_struct);
			goto out;
		}

		nve_out_log(user_info, false);
	}
	out:
		return ret;
}


/*
 * Function name:hisi_nve_direct_access.
 * Discription:read or write NV items interfaces that will be called by other
 * functions.
 * Parameters:
 *          @ user_info:struct hisi_nve_info_user pointer.
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
int hisi_nve_direct_access(struct hisi_nve_info_user *user_info)
{
	int ret;

	if(g_nve_struct == NULL){
		pr_err("[NVE][%s] NVE struct not alloc.\n", __func__);
		return -ENOMEM;
	}
	/*the interface check the nv init*/
	if(g_nve_struct->initialized == 0){
		pr_err("[NVE][%s] NVE init is not done, please wait.\n" ,__func__);
		return -ENODEV;
	}

	/*ensure only one process can visit NV at the same time in
	 * kernel*/
	if (down_interruptible(&nv_sem))
		return -EBUSY;

	ret = hisi_nve_direct_access_for_ramdisk(user_info);
	if(ret){
		pr_err("[NVE][%s]access for nve according ramdisk failed in line %d!\n", __func__, __LINE__);
		goto out;
	}
out:
	/*release the semaphore*/
	up(&nv_sem);
	return ret;
}

/*
 * Function name:nve_open.
 * Discription:open NV device in terms of calling nve_open_ex().
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
static int nve_open(struct inode *inode, struct file *file)
{
	if(g_nve_struct == NULL){
		pr_err("[NVE][%s] NVE struct not alloc.\n" ,__func__);
		return -ENOMEM;
	}
	if(g_nve_struct->initialized)
		return 0;
	else
		return -ENODEV;
}

static int nve_close(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 * Function name:nve_ioctl.
 * Discription:complement read or write NV by terms of sending command-words.
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
static long nve_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	u_int size;
	struct hisi_nve_info_user info;

	/*make sure nve is init*/
	if(g_nve_struct == NULL){
		pr_err("[NVE][%s] NVE struct not alloc.\n", __func__);
		return -ENOMEM;
	}
	/*the interface check the nv init*/
	if(g_nve_struct->initialized == 0){
		pr_err("[NVE][%s] NVE init is not done, please wait.\n" ,__func__);
		return -ENODEV;
	}

	/*ensure only one process can visit NV device at the same time in API*/
	if (down_interruptible(&nv_sem))
		return -EBUSY;

	size = ((cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT);

	if (cmd & IOC_IN) {
		if (!access_ok(VERIFY_READ, argp, size)) {
			pr_err("[NVE][%s]access_in error!\n",
			       __func__);
			up(&nv_sem);
			return -EFAULT;
		}
	}

	if (cmd & IOC_OUT) {
		if (!access_ok(VERIFY_WRITE, argp, size)) {
			pr_err("[NVE][%s]access_out error!\n",
			       __func__);
			up(&nv_sem);
			return -EFAULT;
		}
	}

	switch (cmd) {
	case NVEACCESSDATA:
		if (copy_from_user(&info, argp,
				   sizeof(struct hisi_nve_info_user))) {
			up(&nv_sem);
			return -EFAULT;
		}
		ret = hisi_nve_direct_access_for_ramdisk(&info);
		if(ret){
			pr_err("[NVE][%s]nve access failed in line %d!\n", __func__, __LINE__);
			goto out;
		}
		if (NV_READ == info.nv_operation) {
			if (copy_to_user(argp, &info,
					 sizeof(struct hisi_nve_info_user))) {
				up(&nv_sem);
				return -EFAULT;
			}
		}
		break;
	default:
		pr_err("[NVE][%s] Unknow command!\n", __func__);
		ret = -ENOTTY;
		break;
	}
out:
	up(&nv_sem);
	return (long)ret;
}

#ifdef CONFIG_COMPAT
static long nve_compat_ioctl(struct file *file, u_int cmd, u_long arg)
{
	return nve_ioctl(file, cmd, (unsigned long)compat_ptr((unsigned int)arg));
}
#endif

static const struct file_operations nve_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = nve_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nve_compat_ioctl,
#endif
	.open = nve_open,
	.release = nve_close,
};

static int __init init_nve(void)
{
	int error;

	/*semaphore initial*/
	sema_init(&nv_sem, 1);

	/*alloc nve_struct*/
	g_nve_struct = kzalloc(sizeof(struct NVE_struct), GFP_KERNEL);
	if (g_nve_struct == NULL) {
		pr_err("[NVE][%s]failed to allocate driver data in line %d.\n",
		       __func__, __LINE__);
		return -NVE_ERROR_NO_MEM;
	}

	/*alloc ramdisk*/
	g_nve_struct->nve_current_ramdisk = (struct NVE_partittion *)kzalloc((size_t)NVE_PARTITION_SIZE, GFP_KERNEL);
	if (NULL == g_nve_struct->nve_current_ramdisk) {
		pr_err("[NVE][%s]failed to allocate current ramdisk buffer in "
				"line %d.\n",
		       __func__, __LINE__);
		error = -NVE_ERROR_NO_MEM;
		goto failed_free_driver_data;
	}

	g_nve_struct->nve_update_ramdisk = (struct NVE_partittion *)kzalloc((size_t)NVE_PARTITION_SIZE, GFP_KERNEL);
	if (NULL == g_nve_struct->nve_update_ramdisk) {
		pr_err("[NVE][%s]failed to allocate update ramdisk buffer in "
				"line %d.\n",
		       __func__, __LINE__);
		error = -NVE_ERROR_NO_MEM;
		goto failed_free_current_ramdisk;
	}

	g_nve_struct->nve_store_ramdisk = (struct NVE_partittion *)kzalloc((size_t)NVE_PARTITION_SIZE, GFP_KERNEL);
	if (NULL == g_nve_struct->nve_store_ramdisk) {
		pr_err("[NVE][%s]failed to allocate store ramdisk buffer in "
				"line %d.\n",
		       __func__, __LINE__);
		error = -NVE_ERROR_NO_MEM;
		goto failed_free_update_ramdisk;
	}

	/* register a device in kernel, return the number of major device */
	g_nve_struct->nve_major_number = register_chrdev(0, "nve", &nve_fops);
	if (g_nve_struct->nve_major_number < 0) {
		pr_err("[NVE]Can't allocate major number for "
				"Non-Volatile memory Extension device.\n");
		error = -NVE_ERROR_NO_MEM;
		goto failed_free_store_ramdisk;
	}

	/* register a class, make sure that mdev can create device node in
	 * "/dev" */
	nve_class = class_create(THIS_MODULE, "nve");
	if (IS_ERR(nve_class)) {
		pr_err("[NVE]Error creating nve class.\n");
		unregister_chrdev((unsigned int)g_nve_struct->nve_major_number, "nve");
		error = -NVE_ERROR_NO_MEM;
		goto failed_free_store_ramdisk;
	}

	return 0;
failed_free_store_ramdisk:
	kfree(g_nve_struct->nve_store_ramdisk);
	g_nve_struct->nve_store_ramdisk = NULL;
failed_free_update_ramdisk:
	kfree(g_nve_struct->nve_update_ramdisk);
	g_nve_struct->nve_update_ramdisk = NULL;
failed_free_current_ramdisk:
	kfree(g_nve_struct->nve_current_ramdisk);
	g_nve_struct->nve_current_ramdisk = NULL;
failed_free_driver_data:
	kfree(g_nve_struct);
	g_nve_struct = NULL;
	return error;
}

static void __exit cleanup_nve(void)
{
	device_destroy(nve_class, MKDEV(dev_major_num, 0));
	class_destroy(nve_class);
	if (NULL != g_nve_struct) {
		unregister_chrdev((unsigned int)g_nve_struct->nve_major_number, "nve");
		kfree(g_nve_struct->nve_store_ramdisk);
		g_nve_struct->nve_store_ramdisk = NULL;
		kfree(g_nve_struct->nve_update_ramdisk);
		g_nve_struct->nve_update_ramdisk = NULL;
		kfree(g_nve_struct->nve_current_ramdisk);
		g_nve_struct->nve_current_ramdisk = NULL;
		kfree(g_nve_struct);
		g_nve_struct = NULL;
		kfree(nve_block_device_name);
	}

	return;
}

static int nve_setup(const char *val, struct kernel_param *kp)
{
	int ret;
	struct device *nve_dev = NULL;

	if (1 == hisi_nv_setup_ok) {
		pr_err("[NVE][%s]has been done.\n",__func__);
		return 0;
	}
	/*get param by cmdline*/
	if (!val)
		return -EINVAL;
	nve_block_device_name = kzalloc(strlen(val) + 1, GFP_KERNEL);
	if(nve_block_device_name == NULL){
		pr_err("[NVE][%s]failed to allocate nve_block_device_name mem in line %d.\n",
		       __func__, __LINE__);
		return -ENOMEM;
	}

	memcpy(nve_block_device_name, val, strlen(val) + 1);
	pr_info("[NVE][%s] device name = %s\n", __func__, nve_block_device_name);

	/*read nve partition to ramdisk*/
	ret = read_nve_to_ramdisk();
	if (ret < 0){
		pr_err("[NVE][%s] read nve to ramdisk failed!\n",__func__);
		return -1;
	}
	dev_major_num = (unsigned int)(g_nve_struct->nve_major_number);
	/* create a device node for application*/
	nve_dev = device_create(nve_class, NULL, MKDEV(dev_major_num, 0), NULL, "nve0");
	if(IS_ERR(nve_dev)){
		pr_err("[NVE][%s]failed to create nve device in line %d.\n",__func__, __LINE__);
		return PTR_ERR(nve_dev);
	}

	hisi_nv_setup_ok = 1;

	return 0;
}

module_param_call(nve, nve_setup, NULL, NULL, 0664);

module_init(init_nve);
module_exit(cleanup_nve);

/*export hisi_nve_direct_access,so we can use it in other procedures*/
EXPORT_SYMBOL(hisi_nve_direct_access);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hisi-nve");
MODULE_DESCRIPTION("Direct character-device access to NVE devices");
