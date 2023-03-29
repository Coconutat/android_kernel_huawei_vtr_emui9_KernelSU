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
#include "ilitek_firmware.h"
#include "ilitek_flash.h"

#ifdef BOOT_FW_UPGRADE
#include "ilitek_fw.h"
#endif

/* The size of firmware upgrade */
#define MAX_HEX_FILE_SIZE                      (160*1024)
#define MAX_FLASH_FIRMWARE_SIZE                (256*1024)
#define MAX_IRAM_FIRMWARE_SIZE                 (60*1024)

/*
 * the size of two arrays is different depending on
 * which of methods to upgrade firmware you choose for.
 */
u8 *flash_fw = NULL;
u8 iram_fw[MAX_IRAM_FIRMWARE_SIZE] = { 0 };

/* the length of array in each sector */
int g_section_len = 0;
int g_total_sector = 0;

#ifdef BOOT_FW_UPGRADE
/* The addr of block reserved for customers */
int g_start_resrv = 0x1D000;
int g_end_resrv = 0x1DFFF;
#endif

struct flash_sector *g_flash_sector = NULL;
struct flash_block_info g_flash_block_info[4] = {0};
struct core_firmware_data *core_firmware = NULL;

static u32 calc_crc32(u32 start_addr, u32 end_addr, u8 *data)
{
    int i, j;
    u32 CRC_POLY = 0x04C11DB7;
    u32 ReturnCRC = 0xFFFFFFFF;
    u32 len = start_addr + end_addr;

    for (i = start_addr; i < len; i++) {
        ReturnCRC ^= (data[i] << 24);

        for (j = 0; j < 8; j++) {
            if ((ReturnCRC & 0x80000000) != 0) {
                ReturnCRC = ReturnCRC << 1 ^ CRC_POLY;
            } else {
                ReturnCRC = ReturnCRC << 1;
            }
        }
    }

    return ReturnCRC;
}

static u32 tddi_check_data(u32 start_addr, u32 end_addr)
{
    int timer = 500;
    u32 busy = 0;
    u32 write_len = 0;
    u32 iram_check = 0;
    u32 id = 0;
    u32 type = 0;
    struct ilitek_config * p_cfg = g_ilitek_ts->cfg;

    id = p_cfg->chip_info->chip_id;
    type = p_cfg->chip_info->chip_type;

    write_len = end_addr;

    ilitek_debug(DEBUG_FIRMWARE, "start = 0x%x , write_len = 0x%x, max_count = %x\n",
        start_addr, end_addr, core_firmware->max_count);

    if (write_len > core_firmware->max_count) {
        ilitek_err("The length (%x) written to firmware is greater than max count (%x)\n",
            write_len, core_firmware->max_count);
        goto out;
    }

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x3b, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0xFF0000) >> 16, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x00FF00) >> 8, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x0000FF), 1);

    ilitek_config_ice_mode_write(0x041003, 0x01, 1);    /* Enable Dio_Rx_dual */
    ilitek_config_ice_mode_write(0x041008, 0xFF, 1);    /* Dummy */

    /* Set Receive count */
    if (core_firmware->max_count == 0xFFFF)
        ilitek_config_ice_mode_write(0x04100C, write_len, 2);
    else if (core_firmware->max_count == 0x1FFFF)
        ilitek_config_ice_mode_write(0x04100C, write_len, 3);

    if (id == ILITEK_ILI9881 && type == ILI9881_TYPE_F) {
        /* Checksum_En */
        ilitek_config_ice_mode_write(0x041014, 0x10000, 3);
    } else if (id == ILITEK_ILI9881 && type == ILI9881_TYPE_H) {
        /* Clear Int Flag */
        ilitek_config_ice_mode_write(0x048007, 0x02, 1);

        /* Checksum_En */
        ilitek_config_ice_mode_write(0x041016, 0x00, 1);
        ilitek_config_ice_mode_write(0x041016, 0x01, 1);
    }

    /* Start to receive */
    ilitek_config_ice_mode_write(0x041010, 0xFF, 1);

    while (timer > 0) {

        mdelay(1);

        if (id == ILITEK_ILI9881 && type == ILI9881_TYPE_F)
            busy = ilitek_config_read_write_onebyte(0x041014);
        else if (id == ILITEK_ILI9881 && type == ILI9881_TYPE_H) {
            busy = ilitek_config_read_write_onebyte(0x048007);
            busy = busy >> 1;
        } else {
            ilitek_err("Unknow chip type\n");
            break;
        }

        if ((busy & 0x01) == 0x01)
            break;

        timer--;
    }

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */

    if (timer >= 0) {
        /* Disable dio_Rx_dual */
        ilitek_config_ice_mode_write(0x041003, 0x0, 1);
        iram_check =  core_firmware->isCRC ? ilitek_config_ice_mode_read(0x4101C) : ilitek_config_ice_mode_read(0x041018);
    } else {
        ilitek_err("TIME OUT\n");
        goto out;
    }

    return iram_check;

out:
    ilitek_err("Failed to read Checksum/CRC from IC\n");
    return -1;

}

int tddi_check_fw_upgrade(void)
{
    int ret = NO_NEED_UPDATE;
    int i, crc_byte_len = 4;
    u32 start_addr = 0, end_addr = 0;

    if (flash_fw == NULL) {
        ilitek_err("Flash data is null, ignore upgrade\n");
        return CHECK_FW_FAIL;
    }

    /* To get HW CRC, we must enter to ICE mode */
    ilitek_config_ice_mode_enable();

    mdelay(30);

    for(i = 0; i < ARRAY_SIZE(g_flash_block_info); i++) {
        start_addr = g_flash_block_info[i].start_addr;
        end_addr = g_flash_block_info[i].end_addr;

        /* Invaild end address */
        if (end_addr == 0)
            continue;

        g_flash_block_info[i].hex_crc = (flash_fw[end_addr - 3] << 24) + (flash_fw[end_addr - 2] << 16) + (flash_fw[end_addr - 1] << 8) + flash_fw[end_addr];

        /* Get HW CRC for each block */
        g_flash_block_info[i].block_crc = tddi_check_data(start_addr, end_addr - start_addr - crc_byte_len + 1);

        ilitek_info("block = %d, start_addr = 0x%06x, end_addr = 0x%06x, H_CRC = 0x%06x, B_CRC = 0x%06x\n",
            i, start_addr, end_addr, g_flash_block_info[i].hex_crc, g_flash_block_info[i].block_crc);

        /* Compare Hex to HW's CRC directly instead of fw version */
        if(g_flash_block_info[i].hex_crc != g_flash_block_info[i].block_crc)
            ret = NEED_UPDATE;
    }

out:
    ilitek_config_ice_mode_disable();
    ilitek_chip_reset();
    return ret;
}

static void calc_verify_data(u32 sa, u32 se, u32 *check)
{
    u32 i = 0;
    u32 tmp_ck = 0, tmp_crc = 0;

    if (core_firmware->isCRC) {
        tmp_crc = calc_crc32(sa, se, flash_fw);
        *check = tmp_crc;
    } else {
        for (i = sa; i < (sa + se); i++)
            tmp_ck = tmp_ck + flash_fw[i];

        *check = tmp_ck;
    }
}

static int do_check(u32 start, u32 len)
{
    int res = 0;
    u32 vd = 0, lc = 0;

    calc_verify_data(start, len, &lc);
    vd = tddi_check_data(start, len);
    res = CHECK_EQUAL(vd, lc);

    ilitek_info("%s (%x) : (%x)\n", (res < 0 ? "Invalid !" : "Correct !"), vd, lc);

    return res;
}

static int verify_flash_data(void)
{
    int i = 0, res = 0, len = 0;
    int fps = flashtab->sector;
    u32 ss = 0x0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    /* check chip type with its max count */
    if (p_cfg->chip_info->chip_id == ILITEK_ILI7807 && p_cfg->chip_info->chip_type == ILI7807_TYPE_H) {
        core_firmware->max_count = 0x1FFFF;
        core_firmware->isCRC = true;
    }

    for (i = 0; i < g_section_len + 1; i++) {
        if (g_flash_sector[i].data_flag) {
            if (ss > g_flash_sector[i].ss_addr || len == 0)
                ss = g_flash_sector[i].ss_addr;

            len = len + g_flash_sector[i].dlength;

            /* if larger than max count, then committing data to check */
            if (len >= (core_firmware->max_count - fps)) {
                res = do_check(ss, len);
                if (res < 0)
                    goto out;

                ss = g_flash_sector[i].ss_addr;
                len = 0;
            }
        } else {
            /* split flash sector and commit the last data to fw */
            if (len != 0) {
                res = do_check(ss, len);
                if (res < 0)
                    goto out;

                ss = g_flash_sector[i].ss_addr;
                len = 0;
            }
        }
    }

    /* it might be lower than the size of sector if calc the last array. */
    if (len != 0 && res != -1)
        res = do_check(ss, core_firmware->end_addr - ss);

out:
    return res;
}

static int do_program_flash(u32 start_addr)
{
    int res = 0;
    u32 k;
    u8 buf[512] = { 0 };

    res = core_flash_write_enable();
    if (res < 0)
        goto out;

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x02, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0xFF0000) >> 16, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x00FF00) >> 8, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x0000FF), 1);

    buf[0] = 0x25;
    buf[3] = 0x04;
    buf[2] = 0x10;
    buf[1] = 0x08;

    for (k = 0; k < flashtab->program_page; k++) {
        if (start_addr + k <= core_firmware->end_addr)
            buf[4 + k] = flash_fw[start_addr + k];
        else
            buf[4 + k] = 0xFF;
    }

    if (ilitek_i2c_write(buf, flashtab->program_page + 4) < 0) {
        ilitek_err("Failed to write data at start_addr = 0x%X, k = 0x%X, addr = 0x%x\n",
            start_addr, k, start_addr + k);
        res = -EIO;
        goto out;
    }

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */

    res = core_flash_poll_busy();
    if (res < 0)
        goto out;

    core_firmware->update_status = (start_addr * 101) / core_firmware->end_addr;

    /* holding the status until finish this upgrade. */
    if (core_firmware->update_status > 90)
        core_firmware->update_status = 90;

    /* Don't use ilitek_info to print log because it needs to be kpet in the same line */
    printk("%cUpgrading firmware ... start_addr = 0x%x, %02d%c\n", 0x0D, start_addr, core_firmware->update_status,
           '%');

out:
    return res;
}

static int flash_program_sector(void)
{
    int i, j, res = 0;

    for (i = 0; i < g_section_len + 1; i++) {
        /*
         * If running the boot stage, fw will only be upgrade data with the flag of block,
         * otherwise data with the flag itself will be programed.
         */
        if (core_firmware->isboot) {
            if (!g_flash_sector[i].inside_block)
                continue;
        } else {
            if (!g_flash_sector[i].data_flag)
                continue;
        }

        /* programming flash by its page size */
        for (j = g_flash_sector[i].ss_addr; j < g_flash_sector[i].se_addr; j += flashtab->program_page) {
            if (j > core_firmware->end_addr)
                goto out;

            res = do_program_flash(j);
            if (res < 0)
                goto out;
        }
    }

out:
    return res;
}

static int do_erase_flash(u32 start_addr)
{
    int res = 0;
    u32 temp_buf = 0;

    res = core_flash_write_enable();
    if (res < 0) {
        ilitek_err("Failed to config write enable\n");
        goto out;
    }

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x20, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0xFF0000) >> 16, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x00FF00) >> 8, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x0000FF), 1);

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */

    mdelay(1);

    res = core_flash_poll_busy();
    if (res < 0)
        goto out;

    ilitek_config_ice_mode_write(0x041000, 0x0, 1);    /* CS low */
    ilitek_config_ice_mode_write(0x041004, 0x66aa55, 3);    /* Key */

    ilitek_config_ice_mode_write(0x041008, 0x3, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0xFF0000) >> 16, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x00FF00) >> 8, 1);
    ilitek_config_ice_mode_write(0x041008, (start_addr & 0x0000FF), 1);
    ilitek_config_ice_mode_write(0x041008, 0xFF, 1);

    temp_buf = ilitek_config_read_write_onebyte(0x041010);
    if (temp_buf != 0xFF) {
        ilitek_err("Failed to erase data(0x%x) at 0x%x\n", temp_buf, start_addr);
        res = -EINVAL;
        goto out;
    }

    ilitek_config_ice_mode_write(0x041000, 0x1, 1);    /* CS high */

    ilitek_debug(DEBUG_FIRMWARE, "Earsing data at start addr: %x\n", start_addr);

out:
    return res;
}

static int flash_erase_sector(void)
{
    int i, res = 0;

    for (i = 0; i < g_total_sector; i++) {
        if (core_firmware->isboot) {
            if (!g_flash_sector[i].inside_block)
                continue;
        } else {
            if (!g_flash_sector[i].data_flag && !g_flash_sector[i].inside_block)
                continue;
        }

        res = do_erase_flash(g_flash_sector[i].ss_addr);
        if (res < 0)
            goto out;
    }

out:
    return res;
}

static int iram_upgrade(void)
{
    int i, j, res = 0;
    u8 buf[512];
    int upl = flashtab->program_page;

    /* doing reset for erasing iram data before upgrade it. */
    ilitek_chip_reset();

    mdelay(1);

    ilitek_info("Upgrade firmware written data into IRAM directly\n");

    res = ilitek_config_ice_mode_enable();
    if (res < 0) {
        ilitek_err("Failed to enter ICE mode, res = %d\n", res);
        return res;
    }

    mdelay(20);

    ilitek_debug(DEBUG_FIRMWARE, "nStartAddr = 0x%06X, nEndAddr = 0x%06X, nChecksum = 0x%06X\n",
        core_firmware->start_addr, core_firmware->end_addr, core_firmware->checksum);

    /* write hex to the addr of iram */
    ilitek_info("Writing data into IRAM ...\n");
    for (i = core_firmware->start_addr; i < core_firmware->end_addr; i += upl) {
        if ((i + 256) > core_firmware->end_addr) {
            upl = core_firmware->end_addr % upl;
        }

        buf[0] = 0x25;
        buf[3] = (char)((i & 0x00FF0000) >> 16);
        buf[2] = (char)((i & 0x0000FF00) >> 8);
        buf[1] = (char)((i & 0x000000FF));

        for (j = 0; j < upl; j++)
            buf[4 + j] = iram_fw[i + j];

        if (ilitek_i2c_write(buf, upl + 4)) {
            ilitek_err("Failed to write data via i2c, address = 0x%X, start_addr = 0x%X, end_addr = 0x%X\n",
                (int)i, (int)core_firmware->start_addr, (int)core_firmware->end_addr);
            res = -EIO;
            return res;
        }

        core_firmware->update_status = (i * 101) / core_firmware->end_addr;
        printk("%cupgrade firmware(ap code), %02d%c\n", 0x0D, core_firmware->update_status, '%');

        mdelay(3);
    }

    /* ice mode code reset */
    ilitek_info("Doing code reset ...\n");
    ilitek_config_ice_mode_write(0x40040, 0xAE, 1);
    ilitek_config_ice_mode_write(0x40040, 0x00, 1);

    mdelay(10);

    ilitek_config_ice_mode_disable();

    /*TODO: check iram status */

    return res;
}

static int tddi_fw_upgrade(bool isIRAM)
{
    int res = 0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;
    struct ts_kit_platform_data *ts_platform_data = g_ilitek_ts->ts_dev_data->ts_platform_data;

    if (isIRAM) {
        res = iram_upgrade();
        return res;
    }

    ilitek_chip_reset();

    ilitek_info("Enter to ICE Mode\n");

    res = ilitek_config_ice_mode_enable();
    if (res < 0) {
        ilitek_err("Failed to enable ICE mode\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_platform_data->dsm_info.constraints_UPDATE_status = ILITEK_ENABLE_ICE_MODE_FAIL;
#endif
        goto out;
    }

    mdelay(5);

    /*
     * This command is used to fix the bug of spi clk in 7807F-AB
     * while operating with flash.
     */
    if (p_cfg->chip_info->chip_id == ILITEK_ILI7807 && p_cfg->chip_info->chip_type == ILI7807_TYPE_F_AB) {
        res = ilitek_config_ice_mode_write(0x4100C, 0x01, 1);
        if (res < 0)
            goto out;
    }

    mdelay(25);

    /* Disable flash protection from being written */
    core_flash_enable_protect(false);

    res = flash_erase_sector();
    if (res < 0) {
        ilitek_err("Failed to erase flash\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_platform_data->dsm_info.constraints_UPDATE_status = ILITEK_ERASE_FLASH_FAIL;
#endif
        goto out;
    }

    mdelay(1);

    res = flash_program_sector();
    if (res < 0) {
        ilitek_err("Failed to program flash\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_platform_data->dsm_info.constraints_UPDATE_status = ILITEK_PROGRAM_SECTOR_FAIL;
#endif
        goto out;
    }

    /* We do have to reset chip in order to move new code from flash to iram. */
    ilitek_info("Doing Soft Reset ..\n");
    ilitek_config_ic_reset();

    /* the delay time moving code depends on what the touch IC you're using. */
    mdelay(core_firmware->delay_after_upgrade);

    /* ensure that the chip has been updated */
    ilitek_info("Enter to ICE Mode again\n");
    res = ilitek_config_ice_mode_enable();
    if (res < 0) {
        ilitek_err("Failed to enable ICE mode\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_platform_data->dsm_info.constraints_UPDATE_status = ILITEK_ENABLE_ICE_MODE2_FAIL;
#endif
        goto out;
    }

    mdelay(20);

    /* check the data that we've just written into the iram. */
    res = verify_flash_data();
    if (res == 0) {
        ilitek_info("Data Correct !\n");
    } else {
        ilitek_info("Data Wrong !\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_platform_data->dsm_info.constraints_UPDATE_status = ILITEK_CHECK_DATA_FAIL;
#endif
    }

out:
    ilitek_config_ice_mode_disable();
    return res;
}

#ifdef BOOT_FW_UPGRADE
static int convert_hex_array(void)
{
    int i, j, index = 0;
    int block = 0, blen = 0, bindex = 0;
    u32 tmp_addr = 0x0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    core_firmware->start_addr = 0;
    core_firmware->end_addr = 0;
    core_firmware->checksum = 0;
    core_firmware->crc32 = 0;
    core_firmware->hasBlockInfo = false;

    ilitek_info("CTPM_FW = %d\n", (int)ARRAY_SIZE(CTPM_FW));

    if (ARRAY_SIZE(CTPM_FW) <= 0) {
        ilitek_err("The size of CTPM_FW is invaild (%d)\n", (int)ARRAY_SIZE(CTPM_FW));
        goto out;
    }

    /* Get new version from ILI array */
    if(p_pro->ver->data[1] >= 0x3) {
        core_firmware->new_fw_ver[0] = CTPM_FW[18];
        core_firmware->new_fw_ver[1] = CTPM_FW[19];
        core_firmware->new_fw_ver[2] = CTPM_FW[20];
        core_firmware->new_fw_ver[3] = CTPM_FW[21];
    } else {
        core_firmware->new_fw_ver[0] = CTPM_FW[19];
        core_firmware->new_fw_ver[1] = CTPM_FW[20];
        core_firmware->new_fw_ver[2] = CTPM_FW[21];
    }

    ilitek_info("HW ver: %d.%d.%d.%d\n",core_firmware->old_fw_ver[0],
        core_firmware->old_fw_ver[1], core_firmware->old_fw_ver[2], core_firmware->old_fw_ver[3]);
    ilitek_info("Hex ver: %d.%d.%d.%d\n", core_firmware->new_fw_ver[0],
        core_firmware->new_fw_ver[1], core_firmware->new_fw_ver[2], core_firmware->new_fw_ver[3]);


    /* Extract block info */
    block = CTPM_FW[33];

    if (block > 0) {
        core_firmware->hasBlockInfo = true;

        /* Initialize block's index and length */
        blen = 6;
        bindex = 34;

        for (i = 0; i < block; i++) {
            for (j = 0; j < blen; j++) {
                if (j < 3)
                    g_flash_block_info[i].start_addr =
                        (g_flash_block_info[i].start_addr << 8) | CTPM_FW[bindex + j];
                else
                    g_flash_block_info[i].end_addr =
                        (g_flash_block_info[i].end_addr << 8) | CTPM_FW[bindex + j];
            }

            bindex += blen;
        }
    }

    /* Fill data into buffer */
    for (i = 0; i < ARRAY_SIZE(CTPM_FW) - 64; i++) {
        flash_fw[i] = CTPM_FW[i + 64];
        index = i / flashtab->sector;
        if (!g_flash_sector[index].data_flag) {
            g_flash_sector[index].ss_addr = index * flashtab->sector;
            g_flash_sector[index].se_addr = (index + 1) * flashtab->sector - 1;
            g_flash_sector[index].dlength =
                (g_flash_sector[index].se_addr - g_flash_sector[index].ss_addr) + 1;
            g_flash_sector[index].data_flag = true;
        }
    }

    g_section_len = index;

    if (g_flash_sector[g_section_len].se_addr > flashtab->mem_size) {
        ilitek_err("The size written to flash is larger than it required (%x) (%x)\n",
            g_flash_sector[g_section_len].se_addr, flashtab->mem_size);
        goto out;
    }

    for (i = 0; i < g_total_sector; i++) {
        /* fill meaing address in an array where is empty */
        if (g_flash_sector[i].ss_addr == 0x0 && g_flash_sector[i].se_addr == 0x0) {
            g_flash_sector[i].ss_addr = tmp_addr;
            g_flash_sector[i].se_addr = (i + 1) * flashtab->sector - 1;
        }

        tmp_addr += flashtab->sector;

        /* set erase flag in the block if the addr of sectors is between them. */
        if (core_firmware->hasBlockInfo) {
            for (j = 0; j < ARRAY_SIZE(g_flash_block_info); j++) {
                if (g_flash_sector[i].ss_addr >= g_flash_block_info[j].start_addr
                    && g_flash_sector[i].se_addr <= g_flash_block_info[j].end_addr) {
                    g_flash_sector[i].inside_block = true;
                    break;
                }
            }
        }

        /*
         * protects the reserved address been written and erased.
         * This feature only applies on the boot upgrade. The addr is progrmmable in normal case.
         */
        if (g_flash_sector[i].ss_addr == g_start_resrv && g_flash_sector[i].se_addr == g_end_resrv) {
            g_flash_sector[i].inside_block = false;
        }
    }

    /* DEBUG: for showing data with address that will write into fw or be erased */
    for (i = 0; i < g_total_sector; i++) {
        ilitek_info
            ("g_flash_sector[%d]: ss_addr = 0x%x, se_addr = 0x%x, length = %x, data = %d, inside_block = %d\n",
             i, g_flash_sector[i].ss_addr, g_flash_sector[i].se_addr, g_flash_sector[index].dlength,
             g_flash_sector[i].data_flag, g_flash_sector[i].inside_block);
    }

    core_firmware->start_addr = 0x0;
    core_firmware->end_addr = g_flash_sector[g_section_len].se_addr;
    ilitek_info("start_addr = 0x%06X, end_addr = 0x%06X\n", core_firmware->start_addr, core_firmware->end_addr);
    return 0;

out:
    ilitek_err("Failed to convert ILI FW array\n");
    return -1;
}

int core_firmware_boot_upgrade(void)
{
    int res = 0;
    bool power = false;

    ilitek_info("BOOT: Starting to upgrade firmware ...\n");

    core_firmware->isUpgrading = true;
    core_firmware->update_status = 0;

    if (ipd->isEnablePollCheckPower) {
        ipd->isEnablePollCheckPower = false;
        cancel_delayed_work_sync(&ipd->check_power_status_work);
        power = true;
    }

    if (flashtab == NULL) {
        ilitek_err("Flash table isn't created\n");
        res = -ENOMEM;
        goto out;
    }

    flash_fw = kcalloc(flashtab->mem_size, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(flash_fw)) {
        ilitek_err("Failed to allocate flash_fw memory, %ld\n", PTR_ERR(flash_fw));
        res = -ENOMEM;
        goto out;
    }

    memset(flash_fw, 0xff, (int)sizeof(u8) * flashtab->mem_size);

    g_total_sector = flashtab->mem_size / flashtab->sector;
    if (g_total_sector <= 0) {
        ilitek_err("Flash configure is wrong\n");
        res = -1;
        goto out;
    }

    g_flash_sector = kcalloc(g_total_sector, sizeof(struct flash_sector), GFP_KERNEL);
    if (ERR_ALLOC_MEM(g_flash_sector)) {
        ilitek_err("Failed to allocate g_flash_sector memory, %ld\n", PTR_ERR(g_flash_sector));
        res = -ENOMEM;
        goto out;
    }

    res = convert_hex_array();
    if (res < 0) {
        ilitek_err("Failed to covert firmware data, res = %d\n", res);
        goto out;
    }

    res = tddi_check_fw_upgrade();
    if (res == NEED_UPDATE) {
        ilitek_info("FW CRC is different, doing upgrade\n");
    } else if (res == NO_NEED_UPDATE) {
        ilitek_info("FW CRC is the same, doing nothing\n");
        goto out;
    } else {
        ilitek_info("FW check is incorrect, unexpected errors\n");
        goto out;
    }

    /* calling that function defined at init depends on chips. */
    res = core_firmware->upgrade_func(false);
    if (res < 0) {
        core_firmware->update_status = res;
        ilitek_err("Failed to upgrade firmware, res = %d\n", res);
        goto out;
    }

    core_firmware->update_status = 100;
    ilitek_info("Update firmware information...\n");
    ilitek_config_read_ic_info(ILITEK_FW_INFO);
    ilitek_config_read_ic_info(ILITEK_PRO_INFO);
    ilitek_config_read_ic_info(ILITEK_CORE_INFO);
    ilitek_config_read_ic_info(ILITEK_TP_INFO);
    ilitek_config_read_ic_info(ILITEK_KEY_INFO);

out:
    if (power) {
        ipd->isEnablePollCheckPower = true;
        queue_delayed_work(ipd->check_power_status_queue, &ipd->check_power_status_work, ipd->work_delay);
    }

    ipio_kfree((void **)&flash_fw);
    ipio_kfree((void **)&g_flash_sector);
    core_firmware->isUpgrading = false;
    return res;
}
#endif /* BOOT_FW_UPGRADE */

int convert_hex_file(u8 *pBuf, u32 nSize, bool isIRAM)
{
    u32 i = 0, j = 0, k = 0;
    u32 nLength = 0, nAddr = 0, nType = 0;
    u32 nStartAddr = 0x0, nEndAddr = 0x0, nChecksum = 0x0, nExAddr = 0;
    u32 tmp_addr = 0x0;
    int index = 0, block = 0;

    core_firmware->start_addr = 0;
    core_firmware->end_addr = 0;
    core_firmware->checksum = 0;
    core_firmware->crc32 = 0;
    core_firmware->hasBlockInfo = false;

    memset(g_flash_block_info, 0x0, sizeof(g_flash_block_info));

    /* Parsing HEX file */
    for (; i < nSize;) {
        s32 nOffset;

        nLength = HexToDec(&pBuf[i + 1], 2);
        nAddr = HexToDec(&pBuf[i + 3], 4);
        nType = HexToDec(&pBuf[i + 7], 2);

        /* calculate checksum */
        for (j = 8; j < (2 + 4 + 2 + (nLength * 2)); j += 2) {
            if (nType == 0x00) {
                /* for ice mode write method */
                nChecksum = nChecksum + HexToDec(&pBuf[i + 1 + j], 2);
            }
        }

        if (nType == 0x04) {
            nExAddr = HexToDec(&pBuf[i + 9], 4);
        }

        if (nType == 0x02) {
            nExAddr = HexToDec(&pBuf[i + 9], 4);
            nExAddr = nExAddr >> 12;
        }

        if (nType == 0xAE) {
            core_firmware->hasBlockInfo = true;
            /* insert block info extracted from hex */
            if (block < 4) {
                g_flash_block_info[block].start_addr = HexToDec(&pBuf[i + 9], 6);
                g_flash_block_info[block].end_addr = HexToDec(&pBuf[i + 9 + 6], 6);
                ilitek_debug(DEBUG_FIRMWARE, "Block[%d]: start_addr = %x, end = %x\n",
                    block, g_flash_block_info[block].start_addr, g_flash_block_info[block].end_addr);
            }
            block++;
        }

        nAddr = nAddr + (nExAddr << 16);
        if (pBuf[i + 1 + j + 2] == 0x0D) {
            nOffset = 2;
        } else {
            nOffset = 1;
        }

        if (nType == 0x00) {
            if (nAddr > MAX_HEX_FILE_SIZE) {
                ilitek_err("Invalid hex format\n");
                goto out;
            }

            if (nAddr < nStartAddr) {
                nStartAddr = nAddr;
            }
            if ((nAddr + nLength) > nEndAddr) {
                nEndAddr = nAddr + nLength;
            }

            /* fill data */
            for (j = 0, k = 0; j < (nLength * 2); j += 2, k++) {
                if (isIRAM)
                    iram_fw[nAddr + k] = HexToDec(&pBuf[i + 9 + j], 2);
                else {
                    flash_fw[nAddr + k] = HexToDec(&pBuf[i + 9 + j], 2);

                    if ((nAddr + k) != 0) {
                        index = ((nAddr + k) / flashtab->sector);
                        if (!g_flash_sector[index].data_flag) {
                            g_flash_sector[index].ss_addr = index * flashtab->sector;
                            g_flash_sector[index].se_addr =
                                (index + 1) * flashtab->sector - 1;
                            g_flash_sector[index].dlength =
                                (g_flash_sector[index].se_addr -
                                 g_flash_sector[index].ss_addr) + 1;
                            g_flash_sector[index].data_flag = true;
                        }
                    }
                }
            }
        }
        i += 1 + 2 + 4 + 2 + (nLength * 2) + 2 + nOffset;
    }

    /* Update the length of section */
    g_section_len = index;

    if (g_flash_sector[g_section_len - 1].se_addr > flashtab->mem_size) {
        ilitek_err("The size written to flash is larger than it required (%x) (%x)\n",
            g_flash_sector[g_section_len - 1].se_addr, flashtab->mem_size);
        goto out;
    }

    for (i = 0; i < g_total_sector; i++) {
        /* fill meaing address in an array where is empty */
        if (g_flash_sector[i].ss_addr == 0x0 && g_flash_sector[i].se_addr == 0x0) {
            g_flash_sector[i].ss_addr = tmp_addr;
            g_flash_sector[i].se_addr = (i + 1) * flashtab->sector - 1;
        }

        tmp_addr += flashtab->sector;

        /* set erase flag in the block if the addr of sectors is between them. */
        if (core_firmware->hasBlockInfo) {
            for (j = 0; j < ARRAY_SIZE(g_flash_block_info); j++) {
                if (g_flash_sector[i].ss_addr >= g_flash_block_info[j].start_addr
                    && g_flash_sector[i].se_addr <= g_flash_block_info[j].end_addr) {
                    g_flash_sector[i].inside_block = true;
                    break;
                }
            }
        }
    }

    /* DEBUG: for showing data with address that will write into fw or be erased */
    for (i = 0; i < g_total_sector; i++) {
        ilitek_debug(DEBUG_FIRMWARE,
            "g_flash_sector[%d]: ss_addr = 0x%x, se_addr = 0x%x, length = %x, data = %d, inside_block = %d\n", i,
            g_flash_sector[i].ss_addr, g_flash_sector[i].se_addr, g_flash_sector[index].dlength,
            g_flash_sector[i].data_flag, g_flash_sector[i].inside_block);
    }

    core_firmware->start_addr = nStartAddr;
    core_firmware->end_addr = nEndAddr;
    ilitek_info("nStartAddr = 0x%06X, nEndAddr = 0x%06X\n", nStartAddr, nEndAddr);
    return 0;

out:
    ilitek_err("Failed to convert HEX data\n");
    return -1;
}

/*
 * It would basically be called by ioctl when users want to upgrade firmware.
 *
 * @pFilePath: pass a path where locates user's firmware file.
 *
 */
int core_firmware_upgrade(const char *pFilePath, bool isIRAM)
{
    int res = 0, fsize;
    u8 *hex_buffer = NULL;
    bool power = false;

    struct file *pfile = NULL;
    mm_segment_t old_fs;
    loff_t pos = 0;

    core_firmware->isUpgrading = true;
    core_firmware->update_status = 0;

#if BATTERY_CHECK
    if (g_ilitek_ts->isEnablePollCheckPower) {
        g_ilitek_ts->isEnablePollCheckPower = false;
        cancel_delayed_work_sync(&g_ilitek_ts->check_power_status_work);
        power = true;
    }
#endif

    pfile = filp_open(pFilePath, O_RDONLY, 0);
    if (ERR_ALLOC_MEM(pfile)) {
        ilitek_err("Failed to open the file at %s.\n", pFilePath);
        res = -ENOENT;
        return res;
    }

    fsize = pfile->f_inode->i_size;

    ilitek_info("fsize = %d\n", fsize);

    if (fsize <= 0) {
        ilitek_err("The size of file is zero\n");
        res = -EINVAL;
        goto out;
    }

    if (flashtab == NULL) {
        ilitek_err("Flash table isn't created\n");
        res = -ENOMEM;
        goto out;
    }

    hex_buffer = kcalloc(fsize, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(hex_buffer)) {
        ilitek_err("Failed to allocate hex_buffer memory, %ld\n", PTR_ERR(hex_buffer));
        res = -ENOMEM;
        goto out;
    }

    flash_fw = kcalloc(flashtab->mem_size, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(flash_fw)) {
        ilitek_err("Failed to allocate flash_fw memory, %ld\n", PTR_ERR(flash_fw));
        res = -ENOMEM;
        goto out;
    }

    memset(flash_fw, 0xff, sizeof(u8) * flashtab->mem_size);

    g_total_sector = flashtab->mem_size / flashtab->sector;
    if (g_total_sector <= 0) {
        ilitek_err("Flash configure is wrong\n");
        res = -1;
        goto out;
    }

    g_flash_sector = kcalloc(g_total_sector, sizeof(*g_flash_sector), GFP_KERNEL);
    if (ERR_ALLOC_MEM(g_flash_sector)) {
        ilitek_err("Failed to allocate g_flash_sector memory, %ld\n", PTR_ERR(g_flash_sector));
        res = -ENOMEM;
        goto out;
    }

    /* store current userspace mem segment. */
    old_fs = get_fs();

    /* set userspace mem segment equal to kernel's one. */
    set_fs(get_ds());

    /* read firmware data from userspace mem segment */
    vfs_read(pfile, hex_buffer, fsize, &pos);

    /* restore userspace mem segment after read. */
    set_fs(old_fs);

    res = convert_hex_file(hex_buffer, fsize, isIRAM);
    if (res < 0) {
        ilitek_err("Failed to covert firmware data, res = %d\n", res);
        goto out;
    }

    res = tddi_check_fw_upgrade();
    if (res == NEED_UPDATE) {
        ilitek_info("FW CRC is different, doing upgrade\n");
    } else if (res == NO_NEED_UPDATE) {
        ilitek_info("FW CRC is the same, doing nothing\n");
        goto out;
    } else {
        ilitek_info("FW check is incorrect, unexpected errors\n");
        goto out;
    }

    /* calling that function defined at init depends on chips. */
    res = core_firmware->upgrade_func(isIRAM);
    if (res < 0) {
        ilitek_err("Failed to upgrade firmware, res = %d\n", res);
        goto out;
    }

    ilitek_info("Update TP/Firmware information...\n");
    ilitek_config_read_ic_info(ILITEK_FW_INFO);
    ilitek_config_read_ic_info(ILITEK_PRO_INFO);
    ilitek_config_read_ic_info(ILITEK_CORE_INFO);
    ilitek_config_read_ic_info(ILITEK_TP_INFO);
    ilitek_config_read_ic_info(ILITEK_KEY_INFO);

out:
#if BATTERY_CHECK
    if (power) {
        g_ilitek_ts->isEnablePollCheckPower = true;
        queue_delayed_work(g_ilitek_ts->check_power_status_queue, &g_ilitek_ts->check_power_status_work, g_ilitek_ts->work_delay);
    }
#endif
    filp_close(pfile, NULL);
    ipio_kfree((void **)&hex_buffer);
    ipio_kfree((void **)&flash_fw);
    ipio_kfree((void **)&g_flash_sector);
    core_firmware->isUpgrading = false;
    return res;
}

int core_firmware_init(void)
{
    int i = 0, j = 0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    core_firmware = kzalloc(sizeof(*core_firmware), GFP_KERNEL);
    if (ERR_ALLOC_MEM(core_firmware)) {
        ilitek_err("Failed to allocate core_firmware mem, %ld\n", PTR_ERR(core_firmware));
        core_firmware_remove();
        return -ENOMEM;
    }

    core_firmware->hasBlockInfo = false;
    core_firmware->isboot = false;

    for (i = 0; i < ILITEK_IC_NUMS; i++) {
            for (j = 0; j < 4; j++) {
                core_firmware->old_fw_ver[i] = p_cfg->fw_ver.data[i];
                core_firmware->new_fw_ver[i] = 0x0;
            }

            if (p_cfg->chip_info->chip_id == ILITEK_ILI7807) {
                core_firmware->max_count = 0xFFFF;
                core_firmware->isCRC = false;
                core_firmware->upgrade_func = tddi_fw_upgrade;
                core_firmware->delay_after_upgrade = 100;
            } else if (p_cfg->chip_info->chip_id == ILITEK_ILI9881) {
                core_firmware->max_count = 0x1FFFF;
                core_firmware->isCRC = true;
                core_firmware->upgrade_func = tddi_fw_upgrade;
                core_firmware->delay_after_upgrade = 200;
            }
            return 0;
    }

    ilitek_err("Can't find this chip in support list\n");
    return 0;
}

void core_firmware_remove(void)
{
    ilitek_info("Remove core-firmware members\n");
    ipio_kfree((void **)&core_firmware);
}
