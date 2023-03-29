#ifndef BOOTDEVICE_H
#define BOOTDEVICE_H
#include <linux/device.h>

enum bootdevice_type { BOOT_DEVICE_EMMC = 0, BOOT_DEVICE_UFS = 1 };
#define UFS_VENDOR_HYNIX       0x1AD

#define MAX_PARTITION_NAME_LENGTH       36
struct flash_find_index_user {
	char name[MAX_PARTITION_NAME_LENGTH];
	int index;
};
#define INDEXEACCESSDATA _IOWR('M', 26, struct flash_find_index_user)

void set_bootdevice_type(enum bootdevice_type type);
enum bootdevice_type get_bootdevice_type(void);
unsigned int get_bootdevice_manfid(void);
void set_bootdevice_name(struct device *dev);
void set_bootdevice_size(sector_t size);
void set_bootdevice_cid(u32 *cid);
void set_bootdevice_product_name(char *product_name);
void set_bootdevice_pre_eol_info(u8 pre_eol_info);
void set_bootdevice_life_time_est_typ_a(u8 life_time_est_typ_a);
void set_bootdevice_life_time_est_typ_b(u8 life_time_est_typ_b);
void set_bootdevice_manfid(unsigned int manfid);
void set_bootdevice_rev_handler(int (*get_rev_func)(const struct device *, char *));

#define MAX_RPMB_REGION_NUM 4
#define MAX_FRAME_BIT 64
/* we use 0x rpmb to indicate a valid value */
#define RPMB_DONE 0x72706D62 /* 'r', 'p', 'm', 'b' */
struct rpmb_config_info{
	u64 rpmb_total_blks;/*  rpmb total size is  (rpmb_total_blks * rpmb_blk_size)*/
	u64 rpmb_read_frame_support;/*bit 64 to mark the read frames support*/
	u64 rpmb_write_frame_support;/*bit 64 to mark the write frames support*/
	u64 rpmb_unit_size;/*default value is 128Kbyte*/
	u8 rpmb_region_size[MAX_RPMB_REGION_NUM];/*number of unit*/
	u8 rpmb_blk_size;/* one blk size is 2 << rpmb_blk_size*/
	u8 rpmb_read_align;/*0:no align 1:align*/
	u8 rpmb_write_align;/*0:no align 1:align*/
	u8 rpmb_region_enable;/*bit to enable region*/
};

void set_rpmb_total_blks(u64 total_blks);
void set_rpmb_blk_size(u8 blk_size);
void set_rpmb_read_frame_support(u64 read_frame_support);
void set_rpmb_write_frame_support(u64 write_frame_support);
void set_rpmb_read_align(u8 read_align);
void set_rpmb_write_align(u8 write_align);
void set_rpmb_region_enable(u8 region_enable);
void set_rpmb_unit_size(u64 unit_size);
void set_rpmb_region_size(int region_num, u8 region_size);
void set_rpmb_config_ready_status(void);
u64 get_rpmb_total_blks(void);
u8 get_rpmb_blk_size(void);
u64 get_rpmb_read_frame_support(void);
u64 get_rpmb_write_frame_support(void);
u8 get_rpmb_read_align(void);
u8 get_rpmb_write_align(void);
u8 get_rpmb_region_enable(void);
u64 get_rpmb_unit_size(void);
u8 get_rpmb_region_size(int region_num);
int get_rpmb_config_ready_status(void);
struct rpmb_config_info get_rpmb_config(void);
void set_rpmb_blk_count(uint64_t blk_count);
int get_rpmb_blk_count(uint64_t *blk_count, int delay_ms);

#endif
