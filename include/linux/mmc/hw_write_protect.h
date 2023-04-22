

#ifndef HW_WRITE_PROTECT_H
#define HW_WRITE_PROTECT_H

#include <linux/fs.h>
#include <linux/mmc/ioctl.h>
#include <linux/blkdev.h>

#define MMC_BLOCK_MAJOR 179
#define MMC_IOC_WP_CMD _IOWR(MMC_BLOCK_MAJOR, 0x0, char)

int mmc_hw_set_wp_state(struct block_device *bdev);
int mmc_hw_get_wp_state(struct block_device *bdev);
int should_trap_this_bio(int rw, struct bio *bio, unsigned int count);
int blk_set_ro_secure_debuggable(int state);
struct mmc_card *mmc_get_card_by_disk(struct gendisk *disk);

#endif
