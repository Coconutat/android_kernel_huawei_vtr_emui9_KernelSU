#ifndef _EMMC_RPMB_H_
#define _EMMC_RPMB_H_
#include <linux/mmc/card.h>
#define MAX_RPMB_REGION_NUM 4
#define MAX_RPMB_REGION_UNIT_SIZE (128 * 1024)
#define RPMB_BLK_SIZE 8
void emmc_get_rpmb_info(struct mmc_card *card, u8 *ext_csd);
#endif