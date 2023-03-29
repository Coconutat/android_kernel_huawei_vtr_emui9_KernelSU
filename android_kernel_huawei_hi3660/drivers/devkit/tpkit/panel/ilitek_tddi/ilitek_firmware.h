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

#ifndef __ILITEK_FIRMWARE_H__
#define __ILITEK_FIRMWARE_H__

#define CHECK_FW_FAIL                          -1
#define NEED_UPDATE                            1
#define NO_NEED_UPDATE                         0

#if defined (CONFIG_HUAWEI_DSM)
enum ilitek_fw_upgrade_dsm_err {
    ILITEK_ENABLE_ICE_MODE_FAIL = 0,
    ILITEK_ERASE_FLASH_FAIL,
    ILITEK_PROGRAM_SECTOR_FAIL,
    ILITEK_ENABLE_ICE_MODE2_FAIL,
    ILITEK_CHECK_DATA_FAIL,
    TS_UPDATE_STATE_UNDEFINE = 255,
};
#endif

struct core_firmware_data {
    u8 new_fw_ver[4];
    u8 old_fw_ver[4];

    u32 start_addr;
    u32 end_addr;
    u32 checksum;
    u32 crc32;

    u32 update_status;
    u32 max_count;

    int delay_after_upgrade;

    bool isUpgrading;
    bool isCRC;
    bool isboot;
    bool hasBlockInfo;

    int (*upgrade_func)(bool isIRAM);
};

struct flash_sector {
    u32 ss_addr;
    u32 se_addr;
    u32 checksum;
    u32 crc32;
    u32 dlength;
    bool data_flag;
    bool inside_block;
};

struct flash_block_info {
    u32 start_addr;
    u32 end_addr;
    u32 hex_crc;
    u32 block_crc;
};

extern struct core_firmware_data *core_firmware;
extern struct flash_sector *g_flash_sector;
extern struct flash_block_info g_flash_block_info[];
extern u8 *flash_fw;
extern int g_section_len;
extern int g_total_sector;

int tddi_check_fw_upgrade(void);
#ifdef BOOT_FW_UPGRADE
int core_firmware_boot_upgrade(void);
#endif
int convert_hex_file(u8 *pBuf, u32 nSize, bool isIRAM);
int core_firmware_upgrade(const char *, bool isIRAM);
int core_firmware_init(void);
void core_firmware_remove(void);

#endif /* __ILITEK_FIRMWARE_H__ */

