#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include "emmc-rpmb.h"
#ifdef CONFIG_HISI_BOOTDEVICE
#include <linux/bootdevice.h>
#endif
/**
 * emmc_get_rpmb_info - get rpmb info from emmc device and set the rpmb config
 *
 */
void emmc_get_rpmb_info(struct mmc_card *card, u8 *ext_csd)
{
		struct rpmb_config_info rpmb_config = {0};
		int i;
		rpmb_config.rpmb_unit_size = MAX_RPMB_REGION_UNIT_SIZE;
		rpmb_config.rpmb_blk_size = RPMB_BLK_SIZE;
		/*emmc read support 32bit on means 1-32 frames*/
		rpmb_config.rpmb_read_frame_support = 0xFFFFFFFF;

		/*emmc write must align*/
		rpmb_config.rpmb_write_align = 0x1;

		/*according to EXT_CSD_RPMB_MULT, get the total units in rpmb and calculate the total blks*/
		card->ext_csd.raw_rpmb_size_mult = ext_csd[EXT_CSD_RPMB_MULT];
		rpmb_config.rpmb_total_blks = (card->ext_csd.raw_rpmb_size_mult * rpmb_config.rpmb_unit_size) >> rpmb_config.rpmb_blk_size;

		/*according to WR_REL_PARAM bit 4 to set max rpmb frames*/
		card->ext_csd.raw_wr_rel_param = ext_csd[EXT_CSD_WR_REL_PARAM];
		if(((card->ext_csd.raw_wr_rel_param >> 4) & 0x1) == 1)
			rpmb_config.rpmb_write_frame_support = ((uint64_t)1 << 31) | ((uint64_t)1 << 1) | ((uint64_t)1 << 0);
		else {
			rpmb_config.rpmb_write_frame_support = ((uint64_t)1 << 1) | ((uint64_t)1 << 0);
			/*emmc 5.0, rpmb read need to align*/
			rpmb_config.rpmb_read_align = 0x1;
		}
		/*according to EXT_CSD_VENDOR_FEATURE_SUPPORT bit 0 to set max rpmb frames*/
		card->ext_csd.raw_vendor_feature_support = ext_csd[EXT_CSD_VENDOR_FEATURE_SUPPORT];
		if((card->ext_csd.raw_vendor_feature_support & 0x1) == 0x0)
			rpmb_config.rpmb_region_enable = 0x1;
		else
			rpmb_config.rpmb_region_enable = 0xF;

		/*get four region size*/
		card->ext_csd.raw_rpmb_region1_size_mult = ext_csd[EXT_CSD_RPMB_REGION1_SIZE_MULT];
		card->ext_csd.raw_rpmb_region2_size_mult = ext_csd[EXT_CSD_RPMB_REGION2_SIZE_MULT];
		card->ext_csd.raw_rpmb_region3_size_mult = ext_csd[EXT_CSD_RPMB_REGION3_SIZE_MULT];
		card->ext_csd.raw_rpmb_region4_size_mult = ext_csd[EXT_CSD_RPMB_REGION4_SIZE_MULT];
		rpmb_config.rpmb_region_size[0] = card->ext_csd.raw_rpmb_region1_size_mult;
		rpmb_config.rpmb_region_size[1] = card->ext_csd.raw_rpmb_region2_size_mult;
		rpmb_config.rpmb_region_size[2] = card->ext_csd.raw_rpmb_region3_size_mult;
		rpmb_config.rpmb_region_size[3] = card->ext_csd.raw_rpmb_region4_size_mult;
		/*if emmc does not support multi key, we will set region 1 size is total units*/
		if((rpmb_config.rpmb_region_size[0] | rpmb_config.rpmb_region_size[1] | rpmb_config.rpmb_region_size[2] | rpmb_config.rpmb_region_size[3]) == 0x0)
			rpmb_config.rpmb_region_size[0] = card->ext_csd.raw_rpmb_size_mult;

		set_rpmb_total_blks(rpmb_config.rpmb_total_blks);
		set_rpmb_blk_size(rpmb_config.rpmb_blk_size);
		set_rpmb_unit_size(rpmb_config.rpmb_unit_size);
		set_rpmb_region_enable(rpmb_config.rpmb_region_enable);
		set_rpmb_read_align(rpmb_config.rpmb_read_align);
		set_rpmb_read_frame_support(rpmb_config.rpmb_read_frame_support);
		set_rpmb_write_align(rpmb_config.rpmb_write_align);
		set_rpmb_write_frame_support(rpmb_config.rpmb_write_frame_support);
		for(i = 0;i < MAX_RPMB_REGION_NUM;i++){
			set_rpmb_region_size(i, rpmb_config.rpmb_region_size[i]);
		}
		set_rpmb_config_ready_status();
}