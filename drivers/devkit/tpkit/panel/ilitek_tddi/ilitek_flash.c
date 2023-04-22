/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "ilitek_common.h"
#include "ilitek_dts.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_flash.h"

/*
 * The table contains fundamental data used to program our flash, which
 * would be different according to the vendors.
 */
struct flash_table ft[] = {
    {0xEF, 0x6011, (128 * 1024), 256, (4 * 1024), (64 * 1024)},    /*  W25Q10EW  */
    {0xEF, 0x6012, (256 * 1024), 256, (4 * 1024), (64 * 1024)},    /*  W25Q20EW  */
    {0xC8, 0x6012, (256 * 1024), 256, (4 * 1024), (64 * 1024)},    /*  GD25LQ20B */
    {0xC8, 0x6013, (512 * 1024), 256, (4 * 1024), (64 * 1024)},    /*  GD25LQ40 */
};

struct flash_table *flashtab = NULL;

int core_flash_poll_busy(void)
{
    int timer = 500, res = 0;

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x5, 1);
    while (timer > 0) {
        ilitek_config_ice_mode_write(0x041008, 0xFF, 1);

        mdelay(1);

        if ((ilitek_config_read_write_onebyte(0x041010) & 0x03) == 0x00)
            goto out;

        timer--;
    }

    ilitek_err("Polling Busy Time Out !\n");
    res = -1;
out:
    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */
    return res;
}

int core_flash_write_enable(void)
{
    if (ilitek_config_ice_mode_write(0x041000, 0x0, 1) < 0)
        goto out;
    if (ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3) < 0)
        goto out;
    if (ilitek_config_ice_mode_write(0x041008, 0x6, 1) < 0)
        goto out;
    if (ilitek_config_ice_mode_write(0x041000, 0x1, 1) < 0)
        goto out;

    return 0;

out:
    ilitek_err("Write enable failed !\n");
    return -EIO;
}

void core_flash_enable_protect(bool enable)
{
    ilitek_info("Set flash protect as (%d)\n", enable);

    if (core_flash_write_enable() < 0) {
        ilitek_err("Failed to config flash's write enable\n");
        return;
    }

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    switch (flashtab->mid) {
    case 0xEF:
        if (flashtab->dev_id == 0x6012 || flashtab->dev_id == 0x6011) {
            ilitek_config_ice_mode_write(0x041008, 0x1, 1);
            ilitek_config_ice_mode_write(0x041008, 0x00, 1);

            if (enable)
                ilitek_config_ice_mode_write(0x041008, 0x7E, 1);
            else
                ilitek_config_ice_mode_write(0x041008, 0x00, 1);
        }
        break;
    case 0xC8:
        if (flashtab->dev_id == 0x6012 || flashtab->dev_id == 0x6013) {
            ilitek_config_ice_mode_write(0x041008, 0x1, 1);
            ilitek_config_ice_mode_write(0x041008, 0x00, 1);

            if (enable)
                ilitek_config_ice_mode_write(0x041008, 0x7A, 1);
            else
                ilitek_config_ice_mode_write(0x041008, 0x00, 1);
        }
        break;
    default:
        ilitek_err("Can't find flash id, ignore protection\n");
        break;
    }

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */
    mdelay(5);
}

void core_flash_init(u16 mid, u16 did)
{
    int i = 0;

    ilitek_info("M_ID = %x, DEV_ID = %x\n", mid, did);

    flashtab = kzalloc(sizeof(ft), GFP_KERNEL);
    if (ERR_ALLOC_MEM(flashtab)) {
        ilitek_err("Failed to allocate flashtab memory, %ld\n", PTR_ERR(flashtab));
        return;
    }

    for (; i < ARRAY_SIZE(ft); i++) {
        if (mid == ft[i].mid && did == ft[i].dev_id) {
            ilitek_info("Find them in flash table\n");

            flashtab->mid = mid;
            flashtab->dev_id = did;
            flashtab->mem_size = ft[i].mem_size;
            flashtab->program_page = ft[i].program_page;
            flashtab->sector = ft[i].sector;
            flashtab->block = ft[i].block;
            break;
        }
    }

    if (i >= ARRAY_SIZE(ft)) {
        ilitek_err("Can't find them in flash table, apply default flash config\n");
        flashtab->mid = mid;
        flashtab->dev_id = did;
        flashtab->mem_size = (256 * 1024);
        flashtab->program_page = 256;
        flashtab->sector = (4 * 1024);
        flashtab->block = (64 * 1024);
    }

    ilitek_info("Max Memory size = %d\n", flashtab->mem_size);
    ilitek_info("Per program page = %d\n", flashtab->program_page);
    ilitek_info("Sector size = %d\n", flashtab->sector);
    ilitek_info("Block size = %d\n", flashtab->block);
}

void core_flash_remove(void)
{
    ilitek_info("Remove core-flash memebers\n");

    ipio_kfree((void **)&flashtab);
}
