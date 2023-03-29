/*
 * FileName: kernel/linux/mtd/nve.h
 * Description: define some macros and declear some functions that will be
 * used in file nve.c.
 * Copyright (C) Hisilicon technologies Co., Ltd All rights reserved.
 * Revision history:
 */

#ifndef __NVE_H
#define __NVE_H

#include "hisi_partition.h"
#include <linux/mtd/hisi_nve_interface.h>

#define TEST_NV_IN_KERNEL 1
#define CONFIG_CRC_SUPPORT
#define NVE_PARTITION_SIZE (128 * 1024)
#define NVE_PARTITION_NUMBER 7
#define NVE_INVALID_NVM 0xFFFFFFFF
#define NVE_PARTITION_INVALID_AGE         0x0
#define NVE_PARTITION_COUNT 8
#define NVE_BASE_VERSION         0x1
#define NVE_CRC_SUPPORT_VERSION         0x2
#define NV_ITEMS_MAX_NUMBER     1023
#define NVE_HEADER_NAME "Hisi-NV-Partition" /* ReliableData area */
#define NVE_BLOCK_SIZE	512
#define NVE_NV_DATA_SIZE	104
/* #define NV_DEVICE_NAME          "/dev/block/mmcblk0p7" */
#define NV_DEVICE_NAME "/dev/block/by-name/nvme"

#define NV_INFO_LEN 1024
/*
 *In case accidently power-off happened when NV
 * is writing,we put the partition_header at the
 * position that locate the last 128 Bytes of every
 * partition,so even if power-off happend,current
 * partition's age will not increase which means current
 * partition is not updated and is invalid partiton.
 */
#define NVE_NV_CRC_HEADER_SIZE 20
#define NVE_NV_CRC_DATA_SIZE 104
#define PARTITION_HEADER_SIZE 128
#define PARTITION_HEADER_OFFSET (NVE_PARTITION_SIZE - PARTITION_HEADER_SIZE)

#define NVE_ERROR_NO_MEM 1
#define NVE_ERROR_PARAM 2
#define NVE_ERROR_PARTITION 3

struct NVE_partition_header {
	char nve_partition_name[32];
	unsigned int nve_version;  /*should be built in image with const value*/
	unsigned int nve_block_id; /*should be built in image with const value*/
	unsigned int nve_block_count;  /*should be built in image with const value*/
	unsigned int valid_items; /*should be built in image with const value*/
	unsigned int nv_checksum;
	unsigned int nve_crc_support;
	unsigned char reserved[68];
	unsigned int nve_age; /*changed by run-time image*/
};

/*
 * NV_items_struct and NVE_partittion struct
 * are used for get NV partition information,
 * only used for debug and test.
 */
struct NV_items_struct {
	unsigned int nv_number;
	char nv_name[NV_NAME_LENGTH];
	unsigned int nv_property;
	unsigned int valid_size;
	unsigned int crc;
	char nv_data[NVE_NV_DATA_SIZE];
};

struct NVE_partittion {
	struct NV_items_struct NV_items[NV_ITEMS_MAX_NUMBER];
	struct NVE_partition_header header;
};

struct NVE_struct {
	int nve_major_number;
	int initialized;
	unsigned int nve_partition_count;
	unsigned int nve_current_id;
	struct NVE_partittion *nve_current_ramdisk;
	struct NVE_partittion *nve_update_ramdisk;
	struct NVE_partittion *nve_store_ramdisk;
};

#endif
