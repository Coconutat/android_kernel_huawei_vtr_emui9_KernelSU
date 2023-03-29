/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
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
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

/**
 *
 * @file    ilitek_drv_update.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_common.h"
#include "mstar_mp.h"
#include <linux/firmware.h>

/*=============================================================*/
// VARIABLE DECLARATION
/*=============================================================*/


/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

static int mstar_read_dq_mem_start(void);
static int mstar_read_dq_mem_end(void);
static int mstar_read_eflash_start(u16 nStartAddr, EmemType_e eEmemType);
static void mstar_read_eflash_do_read(u16 nReadAddr, u8 * pReadData);
static int mstar_read_eflash_end(void);

static void mstar_update_fw_swid_do_work(struct work_struct *pWork);
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static void mstar_fw_update_swid(void);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

u16 mstar_get_chip_type(void)
{
    s32 rc = 0;
    u16 nChipType = 0;

    TS_LOG_INFO("%s()\n", __func__);

    // Check chip type by using DbBus for MSG22XX/MSG28XX/MSG58XX/MSG58XXA/ILI2117A/ILI2118A.
    // Erase TP Flash first
    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("  %s()   mstar_dbbus_enter error\n", __func__);
    }
    rc = mstar_dbbus_i2c_response_ack();
    if (rc < 0) {
        TS_LOG_ERR("  %s()   mstar_dbbus_i2c_response_ack error\n", __func__);
    }
    // Stop MCU
    rc = mstar_set_reg_low_byte(0x0FE6, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Stop MCU error\n", __func__);
        return nChipType;
    }
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K    // for MSG22xx only
    // Exit flash low power mode
    rc = mstar_set_reg_low_byte(0x1619, BIT1);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Exit flash low power mode error\n", __func__);
        return nChipType;
    }
    // Change PIU clock to 48MHz
    rc = mstar_set_reg_low_byte(0x1E23, BIT6);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Change PIU clock to 48MHz error\n", __func__);
        return nChipType;
    }
    // Change mcu clock deglitch mux source
    rc = mstar_set_reg_low_byte(0x1E54, BIT0);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Change mcu clock deglitch mux sourceerror\n", __func__);
        return nChipType;
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Set Password
    rc = mstar_set_reg_low_byte(0x1616, 0xAA);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Set Password (0x1616, 0xAA)  error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1617, 0x55);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Set Password (0x1617, 0x55) error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1618, 0xA5);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Set Password (0x1618, 0xA5) error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1619, 0x5A);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   Set Password (0x1619, 0x5A) error\n", __func__);
        return nChipType;
    }
    // disable cpu read, initial; read
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   isable cpu read, initial; read (0x1608, 0x20) error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   isable cpu read, initial; read (0x1608, 0x20) error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1607, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   isable cpu read, initial; read (0x1608, 0x20) error\n", __func__);
        return nChipType;
    }
    // set info block
    rc = mstar_set_reg_low_byte(0x1607, 0x08);
    if (rc < 0) {
        TS_LOG_ERR("  %s()    set info block error\n", __func__);
        return nChipType;
    }
    // set info double buffer
    rc = mstar_set_reg_low_byte(0x1604, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   set info double buffer error\n", __func__);
        return nChipType;
    }
    // set eflash mode to read mode
    rc = mstar_set_reg_low_byte(0x1606, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("  %s()    set eflash mode to read mode (0x1606, 0x01)  error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1610, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("  %s()    set eflash mode to read mode (0x1610, 0x01)  error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1611, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("  %s()    set eflash mode to read mode (0x1611, 0x00)  error\n", __func__);
        return nChipType;
    }
    // set read address
    rc = mstar_set_reg_low_byte(0x1600, 0x05);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   set read address (0x1600, 0x05) error\n", __func__);
        return nChipType;
    }
    rc = mstar_set_reg_low_byte(0x1601, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("  %s()   set read address (0x1601, 0x00) error\n", __func__);
        return nChipType;
    }
    nChipType = mstar_get_reg_16bit(0x160A) & 0xFFFF;

	if (nChipType == CHIP_TYPE_MSG2836A || nChipType == CHIP_TYPE_MSG2846A || nChipType == CHIP_TYPE_MSG5846A
           || nChipType == CHIP_TYPE_MSG5856A) {
        tskit_mstar_data->chip_type_ori = nChipType;
        TS_LOG_INFO("----------------------MSG Chip Type=0x%x, MSG%xA-------------------------\n", nChipType,
                nChipType);
        nChipType = CHIP_TYPE_MSG58XXA;
    } else {
        nChipType = mstar_get_reg_16bit(0x1ECC) & 0xFF;

        TS_LOG_INFO("----------------------MSG Chip Type=0x%x-------------------------\n", nChipType);
        tskit_mstar_data->chip_type_ori = nChipType;

        if (nChipType != CHIP_TYPE_MSG21XX &&   // (0x01)
            nChipType != CHIP_TYPE_MSG21XXA &&  // (0x02)
            nChipType != CHIP_TYPE_MSG26XXM &&  // (0x03)
            nChipType != CHIP_TYPE_MSG22XX &&   // (0x7A)
            nChipType != CHIP_TYPE_MSG28XX &&   // (0x85)
            nChipType != CHIP_TYPE_MSG58XXA)    // (0xBF)
        {
            if (nChipType != 0) {
                nChipType = CHIP_TYPE_MSG58XXA;
            }
        }
    }
    TS_LOG_INFO("  tskit_mstar_data->chip_type_ori = 0x%x  \n", tskit_mstar_data->chip_type_ori);
    TS_LOG_INFO("  Chip Type = 0x%x  \n", nChipType);

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("  %s()   mstar_dbbus_exit error\n", __func__);
    }
    return nChipType;
}

static int mstar_get_customer_fw_ver_dbbus(EmemType_e eEmemType, u16 * pMajor, u16 * pMinor, u8 ** ppVersion) // support MSG28xx only
{
    int rc = NO_ERR;
    u16 nReadAddr = 0;
    u8 szTmpData[4] = { 0 };

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        rc = mstar_dbbus_enter();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_dbbus_enter error rc = %d\n", __func__, rc);
        }
        // Stop mcu
        rc = mstar_set_reg_low_byte(0x0FE6, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s Stop mcu error rc = %d\n", __func__, rc);
        }
        if (eEmemType == EMEM_MAIN) // Read SW ID from main block
        {
            rc = mstar_read_eflash_start(0x7FFD, EMEM_MAIN);
            if (rc < 0) {
                TS_LOG_ERR("%s EMEM_MAIN read eflash start error rc = %d\n", __func__, rc);
            }
            nReadAddr = 0x7FFD;
        } else if (eEmemType == EMEM_INFO)  // Read SW ID from info block
        {
            rc = mstar_read_eflash_start(0x81FB, EMEM_INFO);
            if (rc < 0) {
                TS_LOG_ERR("%s EMEM_INFO read eflash start error rc = %d\n", __func__, rc);
            }
            nReadAddr = 0x81FB;
        }

        mstar_read_eflash_do_read(nReadAddr, &szTmpData[0]);

        rc = mstar_read_eflash_end();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar read eflash end error rc = %d\n", __func__, rc);
        }
        /*
           Ex. Major in Main Block :
           Major low byte at address 0x7FFD

           Major in Info Block :
           Major low byte at address 0x81FB
         */

        *pMajor = (szTmpData[1] << 8);
        *pMajor |= szTmpData[0];
        *pMinor = (szTmpData[3] << 8);
        *pMinor |= szTmpData[2];

        TS_LOG_INFO("  Major = %d  \n", *pMajor);
        TS_LOG_INFO("  Minor = %d  \n", *pMinor);

        if (*ppVersion == NULL) {
            *ppVersion = kzalloc(sizeof(u8) * 12, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%05d.%05d", *pMajor, *pMinor);

        rc = mstar_dbbus_exit();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar dbbus exit error rc = %d\n", __func__, rc);
        }
    }
    return rc;
}

static void mstar_fw_store_data(u8 * pBuf, u32 nSize)
{
    u32 nCount = nSize / 1024;
    u32 nRemainder = nSize % 1024;
    u32 i;

    if (nCount > 0)     // nSize >= 1024
    {
        for (i = 0; i < nCount; i++) {
            memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], pBuf + (i * 1024), 1024);

            tskit_mstar_data->fw_update_data.fw_data_cont++;
        }

        if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
        {
            TS_LOG_INFO("nRemainder = %d\n", nRemainder);

            memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], pBuf + (i * 1024), nRemainder);

            tskit_mstar_data->fw_update_data.fw_data_cont++;
        }
    } else          // nSize < 1024
    {
        if (nSize > 0) {
            memcpy(tskit_mstar_data->fw_update_data.two_dimen_fw_data[tskit_mstar_data->fw_update_data.fw_data_cont], pBuf, nSize);

            tskit_mstar_data->fw_update_data.fw_data_cont++;
        }
    }
}

static void mstar_convert_fw_two_dimen_to_one(u8 ** szTwoDimenFwData, u8 * pOneDimenFwData)
{
    u32 i, j;

    TS_LOG_INFO("  %s()  \n", __func__);

    for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i++) {
        for (j = 0; j < 1024; j++) {
            pOneDimenFwData[i * 1024 + j] = szTwoDimenFwData[i][j];
        }
    }
}

static u32 mstar_calculate_crc(u8 * pFwData, u32 nOffset, u32 nSize)
{
    u32 i;
    u32 nData = 0, nCrc = 0;
    u32 nCrcRule = 0x0C470C06;  // 0000 1100 0100 0111 0000 1100 0000 0110

    for (i = 0; i < nSize; i += 4) {
        nData =
            (pFwData[nOffset + i]) | (pFwData[nOffset + i + 1] << 8) | (pFwData[nOffset + i + 2] << 16) |
            (pFwData[nOffset + i + 3] << 24);
        nCrc = (nCrc >> 1) ^ (nCrc << 1) ^ (nCrc & nCrcRule) ^ nData;
    }

    return nCrc;
}

static int mstar_access_eflash_init(void)
{
    int rc = NO_ERR;
    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
    }
    // Clear PROGRAM erase password
    rc = mstar_set_reg_16bit(0x1618, 0xA55A);
    if (rc < 0) {
        TS_LOG_ERR("%s Clear PROGRAM erase password error rc = %d\n", __func__, rc);
    }
    return rc;
}

static int mstar_isp_burst_write_eflash_start(u16 nStartAddr, u8 * pFirstData, u32 nBlockSize, u16 nPageNum,
                           EmemType_e eEmemType)
{
    int rc = NO_ERR;
    u16 nWriteAddr = nStartAddr / 4;
    u8 szDbBusTxData[3] = { 0 };

    TS_LOG_INFO("  %s() nStartAddr = 0x%x, nBlockSize = %d, nPageNum = %d, eEmemType = %d  \n", __func__,
            nStartAddr, nBlockSize, nPageNum, eEmemType);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }
    // Set e-flash mode to page write mode
    rc = mstar_set_reg_16bit(0x1606, 0x0080);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode to page write mode error rc = %d\n", __func__, rc);
    }
    // Set data align
    rc = mstar_set_reg_low_byte(0x1640, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set data align error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_INFO) {
        rc = mstar_set_reg_low_byte(0x1607, 0x08);
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_set_reg_low_byte(0x1607, 0x08) error rc = %d\n", __func__, rc);
        }
    }
    // Set double buffer
    rc = mstar_set_reg_low_byte(0x1604, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set double buffer error rc = %d\n", __func__, rc);
    }
    // Set page write number
    rc = mstar_set_reg_16bit(0x161A, nPageNum);
    if (rc < 0) {
        TS_LOG_ERR("%s Set page write number error rc = %d\n", __func__, rc);
    }
    // Set e-flash mode trigger(Trigger write mode)
    rc = mstar_set_reg_low_byte(0x1606, 0x81);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode trigger(Trigger write mode) error rc = %d\n", __func__, rc);
    }
    // Set init data
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[0]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data pFirstData[0] error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[1]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data pFirstData[1] error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[2]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data pFirstData[2] error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[3]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data  pFirstData[3] error rc = %d\n", __func__, rc);
    }
    // Set initial address(for latch SA, CA)
    rc = mstar_set_reg_16bit(0x1600, nWriteAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s Set initial address(for latch SA, CA) error rc = %d\n", __func__, rc);
    }
    // Set initial address(for latch PA)
    rc = mstar_set_reg_16bit(0x1600, nWriteAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s Set initial address(for latch PA) error rc = %d\n", __func__, rc);
    }
    // Enable burst mode
    rc = mstar_set_reg_low_byte(0x1608, 0x21);
    if (rc < 0) {
        TS_LOG_ERR("%s Enable burst mode error rc = %d\n", __func__, rc);
    }
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    if (rc < 0) {
        TS_LOG_ERR("%s write data 0x10 0x16 0x02 error rc = %d\n", __func__, rc);
    }
    szDbBusTxData[0] = 0x20;
//    szDbBusTxData[1] = 0x00;
//    szDbBusTxData[2] = 0x00;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data 0x20 error rc = %d\n", __func__, rc);
    }
    return rc;
}

static int mstar_isp_burst_write_eflash_do_write(u8 * pBufferData, u32 nLength)
{
    u32 i;
    u8 szDbBusTxData[3 + MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = { 0 };

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;

    for (i = 0; i < nLength; i++) {
        szDbBusTxData[3 + i] = pBufferData[i];
    }

    return mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3 + nLength);
}

static int mstar_isp_burst_write_eflash_end(void)
{
    int rc = NO_ERR;
    u8 szDbBusTxData[1] = { 0 };

    TS_LOG_INFO("  %s()  \n", __func__);

    szDbBusTxData[0] = 0x21;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);
    if (rc < 0) {
        TS_LOG_ERR("  %s() write data 0x21 error  \n", __func__);
    }
    szDbBusTxData[0] = 0x7E;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);
    if (rc < 0) {
        TS_LOG_ERR("  %s() write data 0x7e error  \n", __func__);
    }
    // Clear burst mode
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("  %s() Clear burst mode error  \n", __func__);
    }
    return rc;
}

static int mstar_write_eflash_start(u16 nStartAddr, u8 * pFirstData, EmemType_e eEmemType)
{
    int rc = NO_ERR;
    u16 nWriteAddr = nStartAddr / 4;

    TS_LOG_INFO("  %s() nStartAddr = 0x%x, eEmemType = %d  \n", __func__, nStartAddr, eEmemType);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash  set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }
    // Set e-flash mode to write mode
    rc = mstar_set_reg_16bit(0x1606, 0x0040);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode to write mode error rc = %d\n", __func__, rc);
    }
    // Set data align
    rc = mstar_set_reg_low_byte(0x1640, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set data align error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_INFO) {
        rc = mstar_set_reg_low_byte(0x1607, 0x08);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO mstar_set_reg_low_byte(0x1607, 0x08) error rc = %d\n", __func__, rc);
        }
    }
    // Set double buffer
    rc = mstar_set_reg_low_byte(0x1604, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set double buffer error rc = %d\n", __func__, rc);
    }
    // Set e-flash mode trigger(Trigger write mode)
    rc = mstar_set_reg_low_byte(0x1606, 0x81);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode trigger(Trigger write mode) error rc = %d\n", __func__, rc);
    }
    // Set init data
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[0]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data 0 error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[1]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data 1 error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[2]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data 2 error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pFirstData[3]);
    if (rc < 0) {
        TS_LOG_ERR("%s Set init data 3  error rc = %d\n", __func__, rc);
    }
    // Set initial address(for latch SA, CA)
    rc = mstar_set_reg_16bit(0x1600, nWriteAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s Set initial address(for latch SA, CA) error rc = %d\n", __func__, rc);
    }
    // Set initial address(for latch PA)
    rc = mstar_set_reg_16bit(0x1600, nWriteAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s Set initial address(for latch PA) error rc = %d\n", __func__, rc);
    }
    return rc;
}

static int mstar_write_eflash_do_write(u16 nStartAddr, u8 * pBufferData)
{
    int rc = NO_ERR;
    u16 nWriteAddr = nStartAddr / 4;

    TS_LOG_INFO("  %s() nWriteAddr = %d  \n", __func__, nWriteAddr);

    // Write data
    rc = mstar_set_reg_low_byte(0x1602, pBufferData[0]);
    if (rc < 0) {
        TS_LOG_ERR("%s set_reg  0x1602 data0 error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, pBufferData[1]);
    if (rc < 0) {
        TS_LOG_ERR("%s set_reg  0x1602 data1 error rc = %d\n", __func__, rc);
    }
	
    rc = mstar_set_reg_low_byte(0x1602, pBufferData[2]);
    if (rc < 0) {
        TS_LOG_ERR("%s set_reg  0x1602 data2 error rc = %d\n", __func__, rc);
    }
	
    rc = mstar_set_reg_low_byte(0x1602, pBufferData[3]);
    if (rc < 0) {
        TS_LOG_ERR("%s set_reg  0x1602 data3 error rc = %d\n", __func__, rc);
    }

    // Set address
    rc = mstar_set_reg_16bit(0x1600, nWriteAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s set_reg  0x1600 addr error rc = %d\n", __func__, rc);
    }
    return rc;
}

static void mstar_write_eflash_end(void)
{
    TS_LOG_INFO("  %s()  \n", __func__);

    // Do nothing
}

static int mstar_read_eflash_start(u16 nStartAddr, EmemType_e eEmemType)
{
    int rc = NO_ERR;
    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s disable cpu read flash set reg (0x1608, 0x20)  error ret = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s disable cpu read flash set reg (0x1606, 0x20)  error ret = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s disable cpu read flash set reg (0x1606, 0x20)  error ret = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_16bit(0x1600, nStartAddr);
    if (rc < 0) {
        TS_LOG_ERR("%s disable cpu read flash set reg (0x1600, nStartAddr)  error ret = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) {
        // Set main block
        rc = mstar_set_reg_low_byte(0x1607, 0x00);
        if (rc < 0) {
            TS_LOG_ERR("%s set main block  error ret = %d\n", __func__, rc);
        }
        // Set main double buffer
        rc = mstar_set_reg_low_byte(0x1604, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s Set main double buffer error ret = %d\n", __func__, rc);
        }
        // Set e-flash mode to read mode for main
        rc = mstar_set_reg_16bit(0x1606, 0x0001);
        if (rc < 0) {
            TS_LOG_ERR("%s Set e-flash mode to read mode for main  error ret = %d\n", __func__, rc);
        }
    } else if (eEmemType == EMEM_INFO) {
        // Set info block
        rc = mstar_set_reg_low_byte(0x1607, 0x08);
        if (rc < 0) {
            TS_LOG_ERR("%s Set info block error ret = %d\n", __func__, rc);
        }
        // Set info double buffer
        rc = mstar_set_reg_low_byte(0x1604, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s Set info double buffer  error ret = %d\n", __func__, rc);
        }
        // Set e-flash mode to read mode for info
        rc = mstar_set_reg_16bit(0x1606, 0x0801);
        if (rc < 0) {
            TS_LOG_ERR("%s Set e-flash mode to read mode for info error ret = %d\n", __func__, rc);
        }
    }
    return rc;
}

static void mstar_read_eflash_do_read(u16 nReadAddr, u8 * pReadData)
{
    u16 nRegData1 = 0, nRegData2 = 0;

    TS_LOG_INFO("  %s() nReadAddr = 0x%x  \n", __func__, nReadAddr);

    // Set read address
    mstar_set_reg_16bit(0x1600, nReadAddr);

    // Read 16+16 bits
    nRegData1 = mstar_get_reg_16bit(0x160A);
    nRegData2 = mstar_get_reg_16bit(0x160C);

    pReadData[0] = nRegData1 & 0xFF;
    pReadData[1] = (nRegData1 >> 8) & 0xFF;
    pReadData[2] = nRegData2 & 0xFF;
    pReadData[3] = (nRegData2 >> 8) & 0xFF;
}

static int mstar_read_eflash_end(void)
{
    int rc = NO_ERR;
    TS_LOG_INFO("  %s()  \n", __func__);

    // Set read done
    rc = mstar_set_reg_low_byte(0x1606, 0x02);
    if (rc < 0) {
        TS_LOG_ERR("%s  Set read done error rc = %d\n", __func__, rc);
    }
    // Unset info flag
    rc = mstar_set_reg_low_byte(0x1607, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s Unset info flag error rc = %d\n", __func__, rc);
    }
    // Clear address
    rc = mstar_set_reg_16bit(0x1600, 0x0000);
    if (rc < 0) {
        TS_LOG_ERR("%s Clear address error rc = %d\n", __func__, rc);
    }
    return rc;
}

static int mstar_get_sft_addr3_value(void)
{
    int rc = NO_ERR;
    TS_LOG_INFO("  %s()  \n", __func__);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }
    // Set e-flash mode to read mode
    rc = mstar_set_reg_low_byte(0x1606, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode to read mode set reg (0x1606, 0x01) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1610, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode to read mode set reg (0x1610, 0x01) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1607, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Set e-flash mode to read mode set reg (0x1607, 0x20) error rc = %d\n", __func__, rc);
    }
    // Set read address
    rc = mstar_set_reg_low_byte(0x1600, 0x03);
    if (rc < 0) {
        TS_LOG_ERR("%s Set read address set reg (0x1600, 0x03) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1601, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s Set read address set reg (0x1601, 0x00) error rc = %d\n", __func__, rc);
    }
    tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 = mstar_get_reg_16bit(0x160A);
    tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 = mstar_get_reg_16bit(0x160C);

    TS_LOG_INFO("tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 = 0x%4X, tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 = 0x%4X\n",
        tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1,
            tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3);
    return rc;
}

static int mstar_unset_protect_bit(void)
{
    int rc = 0;
    u8 nB0, nB1, nB2, nB3;

    TS_LOG_INFO("  %s()  \n", __func__);

    rc = mstar_get_sft_addr3_value();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar get sft addr3 value error rc = %d\n", __func__, rc);
    }

    nB0 = tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 & 0xFF;
    nB1 = (tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 & 0xFF00) >> 8;

    nB2 = tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 & 0xFF;
    nB3 = (tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 & 0xFF00) >> 8;

    TS_LOG_INFO("nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    nB2 = nB2 & 0xBF;   // 10111111
    nB3 = nB3 & 0xFC;   // 11111100

    TS_LOG_INFO("nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1610, 0x80);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1610, 0x80) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1607, 0x10);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1607, 0x10) error rc = %d\n", __func__, rc);
    }
    // Trigger SFR write
    rc = mstar_set_reg_low_byte(0x1606, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Trigger SFR write set reg (0x1606, 0x01) error rc = %d\n", __func__, rc);
    }
    // Set write data
    rc = mstar_set_reg_low_byte(0x1602, nB0);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB0) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, nB1);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB1) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, nB2);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB2) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1602, nB3);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB3) error rc = %d\n", __func__, rc);
    }
    // Set write address
    rc = mstar_set_reg_low_byte(0x1600, 0x03);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write address set reg (0x1600, 0x03) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1601, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write address set reg (0x1601, 0x00) error rc = %d\n", __func__, rc);
    }
    // Set TM mode = 0
    rc = mstar_set_reg_low_byte(0x1607, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s Set TM mode = 0  error rc = %d\n", __func__, rc);
    }
#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    rc = mstar_set_reg_low_byte(0x1606, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s set reg (0x1606, 0x01) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    return rc;
}

int mstar_set_protect_bit(void)
{
    int rc = NO_ERR;
    u8 nB0, nB1, nB2, nB3;

    TS_LOG_INFO("  %s()  \n", __func__);

    nB0 = tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 & 0xFF;
    nB1 = (tskit_mstar_data->fw_update_data.sfr_addr3_byte0_to_1 & 0xFF00) >> 8;

    nB2 = tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 & 0xFF;
    nB3 = (tskit_mstar_data->fw_update_data.sfr_addr3_byte2_to_3 & 0xFF00) >> 8;

    TS_LOG_INFO("nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    rc = mstar_set_reg_low_byte(0x1608, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1606, 0x20);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1610, 0x80);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1610, 0x80) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1607, 0x10);
    if (rc < 0) {
        TS_LOG_ERR("%s Disable cpu read flash set reg (0x1607, 0x10) error rc = %d\n", __func__, rc);
    }	
    // Trigger SFR write
    rc = mstar_set_reg_low_byte(0x1606, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Trigger SFR write error rc = %d\n", __func__, rc);
    }	
    // Set write data
    rc = mstar_set_reg_low_byte(0x1602, nB0);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB0) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1602, nB1);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB1) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1602, nB2);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB2) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1602, nB3);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write data set reg (0x1602, nB3) error rc = %d\n", __func__, rc);
    }	
    // Set write address
    rc = mstar_set_reg_low_byte(0x1600, 0x03);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write address set reg (0x1600, 0x03) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1601, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write address set reg (0x1601, 0x00) error rc = %d\n", __func__, rc);
    }	
    rc = mstar_set_reg_low_byte(0x1606, 0x02);
    if (rc < 0) {
        TS_LOG_ERR("%s Set write address set reg (0x1606, 0x02) error rc = %d\n", __func__, rc);
    }
    return rc;
}

void mstar_erase_emem(EmemType_e eEmemType)
{
    int rc = 0;
    u32 nInfoAddr = 0x20;
    u32 nTimeOut = 0;
    u8 nRegData = 0;

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    mstar_dev_hw_reset();

    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_enter error rc = %d\n", __func__, rc);
    }
    rc = mstar_dbbus_i2c_response_ack();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_i2c_response_ack error rc = %d\n", __func__, rc);
    }
    TS_LOG_INFO("Erase start\n");

    rc = mstar_access_eflash_init();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar access eflash init error rc = %d\n", __func__, rc);
    }
    // Stop mcu
    rc = mstar_set_reg_low_byte(0x0FE6, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Stop mcu error rc = %d\n", __func__, rc);
    }
    // Set PROGRAM erase password
    rc = mstar_set_reg_16bit(0x1618, 0x5AA5);
    if (rc < 0) {
        TS_LOG_ERR("%s Set PROGRAM erase password error rc = %d\n", __func__, rc);
    }
    rc = mstar_unset_protect_bit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar unset protect bit error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) // 128KB
    {
        TS_LOG_INFO("Erase main block\n");

        // Set main block
        rc = mstar_set_reg_low_byte(0x1607, 0x00);
        if (rc < 0) {
            TS_LOG_ERR("%s Set main block error rc = %d\n", __func__, rc);
        }
        // Set e-flash mode to erase mode
        rc = mstar_set_reg_low_byte(0x1606, 0xC0);
        if (rc < 0) {
            TS_LOG_ERR("%s Set e-flash mode to erase mode error rc = %d\n", __func__, rc);
        }
        // Set page erase main
        rc = mstar_set_reg_low_byte(0x1607, 0x03);
        if (rc < 0) {
            TS_LOG_ERR("%s Set page erase main error rc = %d\n", __func__, rc);
        }
        // e-flash mode trigger
        rc = mstar_set_reg_low_byte(0x1606, 0xC1);
        if (rc < 0) {
            TS_LOG_ERR("%s e-flash mode trigger error rc = %d\n", __func__, rc);
        }
        nTimeOut = 0;
        while (1)   // Wait erase done
        {
            nRegData = mstar_get_reg_low_byte(0x160E);
            nRegData = (nRegData & BIT3);

            TS_LOG_INFO("Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT3) {
                break;
            }

            mdelay(10);

            if ((nTimeOut++) > 10) {
                TS_LOG_INFO("Erase main block failed. Timeout.\n");

                goto EraseEnd;
            }
        }
    } else if (eEmemType == EMEM_INFO)  // 2KB
    {
        TS_LOG_INFO("Erase info block\n");

        // Set info block
        rc = mstar_set_reg_low_byte(0x1607, 0x08);
        if (rc < 0) {
            TS_LOG_ERR("%s Set info block error rc = %d\n", __func__, rc);
        }
        // Set info double buffer
        rc = mstar_set_reg_low_byte(0x1604, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s Set info double buffer error rc = %d\n", __func__, rc);
        }
        // Set e-flash mode to erase mode
        rc = mstar_set_reg_low_byte(0x1606, 0xC0);
        if (rc < 0) {
            TS_LOG_ERR("%s Set e-flash mode to erase mode error rc = %d\n", __func__, rc);
        }
        // Set page erase info
        rc = mstar_set_reg_low_byte(0x1607, 0x09);
        if (rc < 0) {
            TS_LOG_ERR("%s Set page erase info error rc = %d\n", __func__, rc);
        }
        for (nInfoAddr = 0x20; nInfoAddr <= MSG28XX_EMEM_INFO_MAX_ADDR; nInfoAddr += 0x20) {
            TS_LOG_INFO("nInfoAddr = 0x%x\n", nInfoAddr);   // add for debug

            // Set address
            rc = mstar_set_reg_16bit(0x1600, nInfoAddr);
            if (rc < 0) {
                TS_LOG_ERR("%s Set address error rc = %d\n", __func__, rc);
            }
            // e-flash mode trigger
            rc = mstar_set_reg_low_byte(0x1606, 0xC1);
            if (rc < 0) {
                TS_LOG_ERR("%s e-flash mode trigger error rc = %d\n", __func__, rc);
            }
            nTimeOut = 0;
            while (1)   // Wait erase done
            {
                nRegData = mstar_get_reg_low_byte(0x160E);
                nRegData = (nRegData & BIT3);

                TS_LOG_INFO("Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT3) {
                    break;
                }

                mdelay(10);

                if ((nTimeOut++) > 10) {
                    TS_LOG_INFO("Erase info block failed. Timeout.\n");

                    // Set main block
                    rc = mstar_set_reg_low_byte(0x1607, 0x00);
                    if (rc < 0) {
                        TS_LOG_ERR("%s Set main block error rc = %d\n", __func__, rc);
                    }
                    goto EraseEnd;
                }
            }
        }

        // Set main block
        rc = mstar_set_reg_low_byte(0x1607, 0x00);
        if (rc < 0) {
            TS_LOG_ERR("%s Set main block error rc = %d\n", __func__, rc);
        }
    }

EraseEnd:

    rc = mstar_set_protect_bit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar set protect bit error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1606, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_set_reg_low_byte(0x1606, 0x00) error rc = %d\n", __func__, rc);
    }
    rc = mstar_set_reg_low_byte(0x1607, 0x00);
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_set_reg_low_byte(0x1607, 0x00) error rc = %d\n", __func__, rc);
    }
    // Clear PROGRAM erase password
    rc = mstar_set_reg_16bit(0x1618, 0xA55A);
    if (rc < 0) {
        TS_LOG_ERR("%s Clear PROGRAM erase password error rc = %d\n", __func__, rc);
    }
    TS_LOG_INFO("Erase end\n");

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_exit error rc = %d\n", __func__, rc);
    }
}

static void mstar_program_emem(EmemType_e eEmemType)
{
    int rc = 0;
    u32 i, j;
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
    u32 k;
#endif
    u32 nPageNum = 0, nLength = 0, nIndex = 0, nWordNum = 0;
    u32 nRetryTime = 0;
    u8 nRegData = 0;
    u8 szFirstData[MSG28XX_EMEM_SIZE_BYTES_ONE_WORD] = { 0 };
    u8 szBufferData[MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = { 0 };
#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    u8 szWriteData[3 + MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * 2] = { 0 };
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);
    mstar_dev_hw_reset();
    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_enter error rc = %d\n", __func__, rc);
    }
    rc = mstar_dbbus_i2c_response_ack();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_i2c_response_ack error rc = %d\n", __func__, rc);
    }
    TS_LOG_INFO("Program start\n");

    rc = mstar_access_eflash_init();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_access_eflash_init error rc = %d\n", __func__, rc);
    }
    // Stop mcu
    rc = mstar_set_reg_low_byte(0x0FE6, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Stop mcu error rc = %d\n", __func__, rc);
    }
    // Set PROGRAM erase password
    rc = mstar_set_reg_16bit(0x1618, 0x5AA5);
    if (rc < 0) {
        TS_LOG_ERR("%s Set PROGRAM erase password error rc = %d\n", __func__, rc);
    }
    rc = mstar_unset_protect_bit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar unset protect bit error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) // Program main block
    {
        TS_LOG_INFO("Program main block\n");

#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;    // 128*1024/128=1024

        // Set ISP mode
        rc = mstar_set_reg_16bit_on(0x1EBE, BIT15);
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP mode set reg 0x1EBE error rc = %d\n", __func__, rc);
        } 
        rc = mstar_set_reg_low_byte(0x1604, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP mode set reg 0x1604 error rc = %d\n", __func__, rc);
        } 
        rc = mstar_set_reg_16bit(0x161A, nPageNum);
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP mode set reg 0x161A error rc = %d\n", __func__, rc);
        } 
        rc = mstar_set_reg_16bit(0x1600, 0x0000);    // Set initial address
        if (rc < 0) {
            TS_LOG_ERR("%s Set initial address error rc = %d\n", __func__, rc);
        } 
        
        rc = mstar_set_reg_16bit_on(0x3C00, BIT0);   // Disable INT GPIO mode
        if (rc < 0) {
            TS_LOG_ERR("%s Disable INT GPIO mode error rc = %d\n", __func__, rc);
        } 
        
        rc = mstar_set_reg_16bit_on(0x1EA0, BIT1);   // Set ISP INT enable
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP INT enable error rc = %d\n", __func__, rc);
        } 
        
        rc = mstar_set_reg_16bit(0x1E34, 0x0000);    // Set DQMem start address
        if (rc < 0) {
            TS_LOG_ERR("%s Set DQMem start address error rc = %d\n", __func__, rc);
        } 
        rc = mstar_read_dq_mem_start();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar read dq mem start error rc = %d\n", __func__, rc);
        } 
        szWriteData[0] = 0x10;
        szWriteData[1] = 0x00;
        szWriteData[2] = 0x00;

        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * 2; //128*2=256

        for (j = 0; j < nLength; j++) {
            szWriteData[3 + j] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][j];
        }
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3 + nLength); // Write the first two pages(page 0 & page 1)
        if (rc < 0) {
            TS_LOG_ERR("%s Write the first two pages(page 0 & page 1) error rc = %d\n", __func__, rc);
        } 
        rc = mstar_read_dq_mem_end();
        if (rc < 0) {
            TS_LOG_ERR("%s read dq mem end error rc = %d\n", __func__, rc);
        } 
        rc = mstar_set_reg_16bit_on(0x1EBE, BIT15);  // Set ISP mode
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP mode error rc = %d\n", __func__, rc);
        } 
        rc = mstar_set_reg_16bit_on(0x1608, BIT0);   // Set burst mode
        if (rc < 0) {
            TS_LOG_ERR("%s Set burst mode error rc = %d\n", __func__, rc);
        } 
        
        rc = mstar_set_reg_16bit_on(0x161A, BIT13);  // Set ISP trig
        if (rc < 0) {
            TS_LOG_ERR("%s Set ISP trig error rc = %d\n", __func__, rc);
        } 

        udelay(2000);   // delay about 2ms

        rc = mstar_read_dq_mem_start();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_start error rc = %d\n", __func__, rc);
        } 
        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        for (i = 2; i < nPageNum; i++) {
            if (i == 2) {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x00;

                for (j = 0; j < nLength; j++) {
                    szWriteData[3 + j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE *
                                 (i - (8 * (i / 8))) + j];
                }

                rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3 + nLength);
                if (rc < 0) {
                    TS_LOG_ERR("%s first write upgrade data error rc = %d\n", __func__, rc);
                } 
            } else if (i == (nPageNum - 1)) {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x80;

                for (j = 0; j < nLength; j++) {
                    szWriteData[3 + j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE *
                                 (i - (8 * (i / 8))) + j];
                }

                szWriteData[3 + 128] = 0xFF;
                szWriteData[3 + 129] = 0xFF;
                szWriteData[3 + 130] = 0xFF;
                szWriteData[3 + 131] = 0xFF;

                rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3 + nLength + 4);
                if (rc < 0) {
                    TS_LOG_ERR("%s end write upgrade data error rc = %d\n", __func__, rc);
                } 
            } else {
//                szWriteData[0] = 0x10;
//                szWriteData[1] = 0x00;
                if (szWriteData[2] == 0x00) {
                    szWriteData[2] = 0x80;
                }
                else  // szWriteData[2] == 0x80
                {
                    szWriteData[2] = 0x00;
                }

                for (j = 0; j < nLength; j++) {
                    szWriteData[3 + j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE *
                                 (i - (8 * (i / 8))) + j];
                }

                rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3 + nLength);
                if (rc < 0) {
                    TS_LOG_ERR("%s middle write upgrade data error rc = %d\n", __func__, rc);
                } 
            }
        }

        rc = mstar_read_dq_mem_end();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_end error rc = %d\n", __func__, rc);
        } 
#else

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 8;   // 128*1024/8=16384
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 32;  // 128*1024/32=4096
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;    // 128*1024/128=1024
#endif

        nIndex = 0;

        for (i = 0; i < nPageNum; i++) {
            if (i == 0) {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

                szFirstData[0] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][0];
                szFirstData[1] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][1];
                szFirstData[2] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][2];
                szFirstData[3] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][3];

                rc = mstar_isp_burst_write_eflash_start(nIndex, &szFirstData[0],
                                   MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024, nPageNum,
                                   EMEM_MAIN);
                if (rc < 0) {
                    TS_LOG_ERR("%s mstar_isp_burst_write_eflash_start error rc = %d\n", __func__, rc);
                } 
                nIndex += nLength;

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 4 = 8 - 4
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;    // 28 = 32 - 4
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;  // 124 = 128 - 4
#endif

                for (j = 0; j < nLength; j++) {
                    szBufferData[j] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[0][4 + j];
                }
            } else {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8;
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32;
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128
#endif

                for (j = 0; j < nLength; j++) {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                    szBufferData[j] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 128][8 * (i - (128 * (i / 128))) + j];
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                    szBufferData[j] = tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 32][32 * (i - (32 * (i / 32))) + j];
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                    szBufferData[j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[i / 8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE *
                                 (i - (8 * (i / 8))) + j];
#endif
                }
            }

            rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], nLength);
            if (rc < 0) {
                TS_LOG_ERR("%s mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
            } 
            udelay(2000);   // delay about 2ms

            nIndex += nLength;
        }

        rc = mstar_isp_burst_write_eflash_end();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_isp_burst_write_eflash_end error rc = %d\n", __func__, rc);
        } 
        // Set write done
        rc = mstar_set_reg_16bit_on(0x1606, BIT2);
        if (rc < 0) {
            TS_LOG_ERR("%s Set write done error rc = %d\n", __func__, rc);
        } 
        // Check RBB
        nRegData = mstar_get_reg_low_byte(0x160E);
        nRetryTime = 0;

        while ((nRegData & BIT3) != BIT3) {
            mdelay(10);

            nRegData = mstar_get_reg_low_byte(0x160E);

            if (nRetryTime++ > 100) {
                TS_LOG_INFO("main block can't wait write to done.\n");

                goto ProgramEnd;
            }
        }
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    } else if (eEmemType == EMEM_INFO)  // Program info block
    {
        TS_LOG_INFO("Program info block\n");

        nPageNum = (MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;    // 2*1024/128=16

        nIndex = 0;
        nIndex += MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        // Skip firt page(page 0) & Update page 1~14 by isp burst write mode
        for (i = 1; i < (nPageNum - 1); i++)    // Skip the first 128 byte and the last 128 byte of info block
        {
            if (i == 1) {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

                szFirstData[0] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE +
                                        1];
                szFirstData[2] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE +
                                        2];
                szFirstData[3] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE +
                                        3];

                rc = mstar_isp_burst_write_eflash_start(nIndex, &szFirstData[0],
                                   MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024,
                                   nPageNum - 1, EMEM_INFO);
                if (rc < 0) {
                    TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_start error rc = %d\n", __func__, rc);
                } 
                nIndex += nLength;

                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;  // 124 = 128 - 4

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                for (j = 0; j < (nLength / 8); j++) // 124/8 = 15
                {
                    for (k = 0; k < 8; k++) {
                        szBufferData[k] =
                            tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                            [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 4 + (8 * j) + k];
                    }

                    rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 8);
                    if (rc < 0) {
                        TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                    } 
                    udelay(2000);   // delay about 2ms
                }

                for (k = 0; k < (nLength % 8); k++) // 124%8 = 4
                {
                    szBufferData[k] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                        [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 4 + (8 * j) + k];
                }

                rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], (nLength % 8)); // 124%8 = 4
                if (rc < 0) {
                    TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                } 
                udelay(2000);   // delay about 2ms
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                for (j = 0; j < (nLength / 32); j++)    // 124/8 = 3
                {
                    for (k = 0; k < 32; k++) {
                        szBufferData[k] =
                            tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                            [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 4 + (32 * j) + k];
                    }

                    rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 32);
                    if (rc < 0) {
                        TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                    } 
                    udelay(2000);   // delay about 2ms
                }

                for (k = 0; k < (nLength % 32); k++)    // 124%32 = 28
                {
                    szBufferData[k] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                        [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 4 + (32 * j) + k];
                }

                rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], (nLength % 32));    // 124%8 = 28
                if (rc < 0) {
                    TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                } 
                udelay(2000);   // delay about 2ms
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                for (j = 0; j < nLength; j++) {
                    szBufferData[j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                        [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 4 + j];
                }
#endif
            } else {
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                if (i < 8)  // 1 < i < 8
                {
                    for (j = 0; j < (nLength / 8); j++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k++) {
                            szBufferData[k] =
                                tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                                [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * i + (8 * j) + k];
                        }

                        rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 8);
                        if (rc < 0) {
                           TS_LOG_ERR("%s EMEM_INFO  i < 8 mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                        } 
                        udelay(2000);   // delay about 2ms
                    }
                } else  // i >= 8
                {
                    for (j = 0; j < (nLength / 8); j++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k++) {
                            szBufferData[k] =
                                tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                                      1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * (i -
                                                         8) +
                                     (8 * j) + k];
                        }

                        rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 8);
                        if (rc < 0) {
                           TS_LOG_ERR("%s EMEM_INFO  i >= 8 mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                        } 
                        udelay(2000);   // delay about 2ms
                    }
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                if (i < 8)  // 1 < i < 8
                {
                    for (j = 0; j < (nLength / 32); j++)    // 128/32 = 4
                    {
                        for (k = 0; k < 32; k++) {
                            szBufferData[k] =
                                tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                                [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * i + (32 * j) + k];
                        }

                        rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 32);
                        if (rc < 0) {
                           TS_LOG_ERR("%s EMEM_INFO  i < 8 mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                        } 
                        udelay(2000);   // delay about 2ms
                    }
                } else  // i >= 8
                {
                    for (j = 0; j < (nLength / 32); j++)    // 128/32 = 4
                    {
                        for (k = 0; k < 32; k++) {
                            szBufferData[k] =
                                tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                                      1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * (i -
                                                         8) +
                                     (32 * j) + k];
                        }

                        rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], 32);
                        if (rc < 0) {
                           TS_LOG_ERR("%s EMEM_INFO  i >= 8 mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
                        } 
                        udelay(2000);   // delay about 2ms
                    }
                }
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                if (i < 8)  // 1 < i < 8
                {
                    for (j = 0; j < nLength; j++) {
                        szBufferData[j] =
                            tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE]
                            [MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * i + j];
                    }
                } else  // i >= 8
                {
                    for (j = 0; j < nLength; j++) {
                        szBufferData[j] =
                            tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                                  1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE * (i - 8) + j];
                    }
                }
#endif
            }

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
            // Do nothing here
#else
            rc = mstar_isp_burst_write_eflash_do_write(&szBufferData[0], nLength);
            if (rc < 0) {
                TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_do_write error rc = %d\n", __func__, rc);
            } 
            udelay(2000);   // delay about 2ms
#endif
            nIndex += nLength;
        }

        rc = mstar_isp_burst_write_eflash_end();
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO mstar_isp_burst_write_eflash_end error rc = %d\n", __func__, rc);
        } 
        // Set write done
        rc = mstar_set_reg_16bit_on(0x1606, BIT2);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Set write done error rc = %d\n", __func__, rc);
        } 
        // Check RBB
        nRegData = mstar_get_reg_low_byte(0x160E);
        nRetryTime = 0;

        while ((nRegData & BIT3) != BIT3) {
            mdelay(10);

            nRegData = mstar_get_reg_low_byte(0x160E);

            if (nRetryTime++ > 100) {
                TS_LOG_INFO("Info block page 1~14 can't wait write to done.\n");

                goto ProgramEnd;
            }
        }

        rc = mstar_set_reg_16bit_off(0x1EBE, BIT15);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO set reg 0x1ebe error rc = %d\n", __func__, rc);
        } 
        // Update page 15 by write mode
        nIndex = 15 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;
        nWordNum = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE / MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 128/4=32
        nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

        for (i = 0; i < nWordNum; i++) {
            if (i == 0) {
                szFirstData[0] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                          1][7 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                          1][7 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 1];
                szFirstData[2] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                          1][7 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 2];
                szFirstData[3] =
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                          1][7 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + 3];

                rc = mstar_write_eflash_start(nIndex, &szFirstData[0], EMEM_INFO);
                if (rc < 0) {
                    TS_LOG_ERR("%s EMEM_INFO mstar_write_eflash_start error rc = %d\n", __func__, rc);
                } 
            } else {
                for (j = 0; j < nLength; j++) {
                    szFirstData[j] =
                        tskit_mstar_data->fw_update_data.two_dimen_fw_data[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE +
                              1][7 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE + (4 * i) + j];
                }

                rc = mstar_write_eflash_do_write(nIndex, &szFirstData[0]);
                if (rc < 0) {
                    TS_LOG_ERR("%s EMEM_INFO mstar_write_eflash_do_write error rc = %d\n", __func__, rc);
                } 
            }

            udelay(2000);   // delay about 2ms

            nIndex += nLength;
        }

        mstar_write_eflash_end();
        // Set write done
        rc = mstar_set_reg_16bit_on(0x1606, BIT2);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Set write done error rc = %d\n", __func__, rc);
        } 
        // Check RBB
        nRegData = mstar_get_reg_low_byte(0x160E);
        nRetryTime = 0;

        while ((nRegData & BIT3) != BIT3) {
            mdelay(10);

            nRegData = mstar_get_reg_low_byte(0x160E);

            if (nRetryTime++ > 100) {
                TS_LOG_INFO("Info block page 15 can't wait write to done.\n");

                goto ProgramEnd;
            }
        }
    }

ProgramEnd:

    rc = mstar_set_protect_bit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar set protect bit error rc = %d\n", __func__, rc);
    }
    // Clear PROGRAM erase password
    rc = mstar_set_reg_16bit(0x1618, 0xA55A);
    if (rc < 0) {
        TS_LOG_ERR("%s Clear PROGRAM erase password error rc = %d\n", __func__, rc);
    }
    TS_LOG_INFO("Program end\n");

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("%s Clear mstar_dbbus_exit error rc = %d\n", __func__, rc);
    }
}

u16 mstar_get_swid(EmemType_e eEmemType)
{
    int rc = NO_ERR;
    u16 nRetVal = 0;
    u16 nReadAddr = 0;
    u8 szTmpData[4] = { 0 };

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar dbbus enter error rc = %d\n", __func__, rc);
    }
    // Stop MCU
    mstar_set_reg_low_byte(0x0FE6, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Stop MCU error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        rc = mstar_read_eflash_start(0x7FFD, EMEM_MAIN);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_MAIN read eflash start error rc = %d\n", __func__, rc);
        }
        nReadAddr = 0x7FFD;
    } else if (eEmemType == EMEM_INFO)  // Read SW ID from info block
    {
        rc = mstar_read_eflash_start(0x81FB, EMEM_INFO);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO read eflash start error rc = %d\n", __func__, rc);
        }
        nReadAddr = 0x81FB;
    }

    mstar_read_eflash_do_read(nReadAddr, &szTmpData[0]);

    rc = mstar_read_eflash_end();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar read eflash end error rc = %d\n", __func__, rc);
    }
    /*
       Ex. SW ID in Main Block :
       Major low byte at address 0x7FFD

       SW ID in Info Block :
       Major low byte at address 0x81FB
     */

    nRetVal = (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];

    TS_LOG_INFO("SW ID = 0x%x\n", nRetVal);

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar dbbus exit error rc = %d\n", __func__, rc);
    }
    return nRetVal;
}

static u32 mstar_get_fw_crc_hw(EmemType_e eEmemType)
{
    int rc = NO_ERR;
    u32 nRetVal = 0;
    u32 nRetryTime = 0;
    u32 nCrcEndAddr = 0;
    u16 nCrcDown = 0;

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    mstar_dev_hw_reset();

    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_enter error rc = %d\n", __func__, rc);
    }
    rc = mstar_access_eflash_init();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_access_eflash_init error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) {
        // Disable cpu read flash
        rc = mstar_set_reg_low_byte(0x1608, 0x20);
        if (rc < 0) {
            TS_LOG_ERR("%s Disable cpu read flash set reg 0x1608 error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_low_byte(0x1606, 0x20);
        if (rc < 0) {
            TS_LOG_ERR("%s Disable cpu read flash set reg 0x1606 error rc = %d\n", __func__, rc);
        }
        // Set read flag
        rc = mstar_set_reg_16bit(0x1610, 0x0001);
        if (rc < 0) {
            TS_LOG_ERR("%s Set read flag error rc = %d\n", __func__, rc);
        }
        // Mode reset main block
        rc = mstar_set_reg_16bit(0x1606, 0x0000);
        if (rc < 0) {
            TS_LOG_ERR("%s Mode reset main block error rc = %d\n", __func__, rc);
        }
        // CRC reset
        rc = mstar_set_reg_16bit(0x1620, 0x0002);
        if (rc < 0) {
            TS_LOG_ERR("%s CRC reset set reg (0x1620, 0x0002)  error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_16bit(0x1620, 0x0000);
        if (rc < 0) {
            TS_LOG_ERR("%s CRC reset set reg (0x1620, 0x0000)error rc = %d\n", __func__, rc);
        }
        // Set CRC e-flash block start address => Main Block : 0x0000 ~ 0x7FFE
        rc = mstar_set_reg_16bit(0x1600, 0x0000);
        if (rc < 0) {
            TS_LOG_ERR("%s Set CRC e-flash block start address => Main Block : 0x0000 ~ 0x7FFE error rc = %d\n", __func__, rc);
        }
        nCrcEndAddr = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 4 - 2;

        rc = mstar_set_reg_16bit(0x1622, nCrcEndAddr);
        if (rc < 0) {
            TS_LOG_ERR("%s Set CRC e-flash block start address => Main Block : 0x0000 ~ 0x7FFE end addr error rc = %d\n", __func__, rc);
        }
        // Trigger CRC check
        rc = mstar_set_reg_16bit(0x1620, 0x0001);
        if (rc < 0) {
            TS_LOG_ERR("%s Trigger CRC check error rc = %d\n", __func__, rc);
        }
        nCrcDown = mstar_get_reg_16bit(0x1620);

        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0) {
            mdelay(10);

            nCrcDown = mstar_get_reg_16bit(0x1620);
            nRetryTime++;

            if (nRetryTime > 30) {
                TS_LOG_INFO("Wait main block nCrcDown failed.\n");
                break;
            }
        }

        nRetVal = mstar_get_reg_16bit(0x1626);
        nRetVal = (nRetVal << 16) | mstar_get_reg_16bit(0x1624);
    } else if (eEmemType == EMEM_INFO) {
        // Disable cpu read flash
        rc = mstar_set_reg_low_byte(0x1608, 0x20);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Disable cpu read flash set reg (0x1608, 0x20) error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_low_byte(0x1606, 0x20);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Disable cpu read flash set reg (0x1606, 0x20) error rc = %d\n", __func__, rc);
        }
        // Set read flag
        rc = mstar_set_reg_16bit(0x1610, 0x0001);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Set read flag error rc = %d\n", __func__, rc);
        }
        // Mode reset info block
        rc = mstar_set_reg_16bit(0x1606, 0x0800);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Mode reset info block set reg (0x1606, 0x0800) error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_low_byte(0x1604, 0x01);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Mode reset info block set reg (0x1604, 0x01) error rc = %d\n", __func__, rc);
        }
        // CRC reset
        rc = mstar_set_reg_16bit(0x1620, 0x0002);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO CRC reset (0x1620, 0x0002) error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_16bit(0x1620, 0x0000);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO CRC reset (0x1620, 0x0000) error rc = %d\n", __func__, rc);
        }
        // Set CRC e-flash block start address => Info Block : 0x0020 ~ 0x01FE
        rc = mstar_set_reg_16bit(0x1600, 0x0020);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Set CRC e-flash block start address => Info Block : 0x0020 ~ 0x01FE error rc = %d\n", __func__, rc);
        }
        rc = mstar_set_reg_16bit(0x1622, 0x01FE);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Set CRC e-flash block start address => Info Block : 0x0020 ~ 0x01FE end addr error rc = %d\n", __func__, rc);
        }
        // Trigger CRC check
        rc = mstar_set_reg_16bit(0x1620, 0x0001);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO Trigger CRC check error rc = %d\n", __func__, rc);
        }
        nCrcDown = mstar_get_reg_16bit(0x1620);

        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0) {
            mdelay(10);

            nCrcDown = mstar_get_reg_16bit(0x1620);
            nRetryTime++;

            if (nRetryTime > 30) {
                TS_LOG_INFO("Wait info block nCrcDown failed.\n");
                break;
            }
        }

        nRetVal = mstar_get_reg_16bit(0x1626);
        nRetVal = (nRetVal << 16) | mstar_get_reg_16bit(0x1624);
    }

    TS_LOG_INFO("Hardware CRC = 0x%x\n", nRetVal);

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_dbbus_exit error rc = %d\n", __func__, rc);
    }
    return nRetVal;
}

static u32 mstar_get_fw_crc_eflash(EmemType_e eEmemType)
{
    int rc = NO_ERR;
    u32 nRetVal = 0;
    u16 nReadAddr = 0;
    u8 szTmpData[4] = { 0 };

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    rc = mstar_dbbus_enter();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar dbbus enter error rc = %d\n", __func__, rc);
    }
    // Stop MCU
    rc = mstar_set_reg_low_byte(0x0FE6, 0x01);
    if (rc < 0) {
        TS_LOG_ERR("%s Stop MCU error rc = %d\n", __func__, rc);
    }
    if (eEmemType == EMEM_MAIN) // Read main block CRC(128KB-4) from main block
    {
        rc = mstar_read_eflash_start(0x7FFF, EMEM_MAIN);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_MAIN read eflash start error rc = %d\n", __func__, rc);
        }
        nReadAddr = 0x7FFF;
    } else if (eEmemType == EMEM_INFO)  // Read info block CRC(2KB-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-4) from info block
    {
        rc = mstar_read_eflash_start(0x81FF, EMEM_INFO);
        if (rc < 0) {
            TS_LOG_ERR("%s EMEM_INFO read eflash start error rc = %d\n", __func__, rc);
        }
        nReadAddr = 0x81FF;
    }

    mstar_read_eflash_do_read(nReadAddr, &szTmpData[0]);

    TS_LOG_DEBUG("szTmpData[0] = 0x%x\n", szTmpData[0]); // add for debug
    TS_LOG_DEBUG("szTmpData[1] = 0x%x\n", szTmpData[1]); // add for debug
    TS_LOG_DEBUG("szTmpData[2] = 0x%x\n", szTmpData[2]); // add for debug
    TS_LOG_DEBUG("szTmpData[3] = 0x%x\n", szTmpData[3]); // add for debug

    rc = mstar_read_eflash_end();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar read eflash end error rc = %d\n", __func__, rc);
    }
    nRetVal = (szTmpData[3] << 24);
    nRetVal |= (szTmpData[2] << 16);
    nRetVal |= (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];

    TS_LOG_INFO("CRC = 0x%x\n", nRetVal);

    rc = mstar_dbbus_exit();
    if (rc < 0) {
        TS_LOG_ERR("%s mstar dbbus exit error rc = %d\n", __func__, rc);
    }
    return nRetVal;
}

static u32 mstar_get_fw_crc_bin(u8 ** szTmpBuf, EmemType_e eEmemType)
{
    u32 nRetVal = 0;

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    if (szTmpBuf != NULL) {
        if (eEmemType == EMEM_MAIN) {
            nRetVal = szTmpBuf[127][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1020];
        } else if (eEmemType == EMEM_INFO) {
            nRetVal = szTmpBuf[129][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1020];
        }
    }

    return nRetVal;
}

static s32 mstar_check_fw_bin_ingerity(u8 ** szFwData)
{
    u32 nCrcMain = 0, nCrcMainBin = 0;
    u32 nCrcInfo = 0, nCrcInfoBin = 0;
    u32 nRetVal = 0;

    TS_LOG_INFO("  %s()  \n", __func__);

    mstar_convert_fw_two_dimen_to_one(szFwData, tskit_mstar_data->fw_update_data.one_dimen_fw_data);

    /* Calculate main block CRC & info block CRC by device driver itself */
    nCrcMain =
        mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data, 0,
                MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
    nCrcInfo =
        mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data,
                MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 + MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE,
                MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE -
                MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

    /* Read main block CRC & info block CRC from firmware bin file */
    nCrcMainBin = mstar_get_fw_crc_bin(szFwData, EMEM_MAIN);
    nCrcInfoBin = mstar_get_fw_crc_bin(szFwData, EMEM_INFO);

    TS_LOG_INFO("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainBin=0x%x, nCrcInfoBin=0x%x\n",
            nCrcMain, nCrcInfo, nCrcMainBin, nCrcInfoBin);

    if ((nCrcMainBin != nCrcMain) || (nCrcInfoBin != nCrcInfo)) {
        TS_LOG_INFO("CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");
        nRetVal = -1;
    } else {
        TS_LOG_INFO("CHECK FIRMWARE BIN FILE INTEGRITY SUCCESS. PROCEED UPDATE FIRMWARE.\n");
        nRetVal = 0;
    }

    return nRetVal;
}

static u32 mstar_28xx_fw_update(u8 ** szFwData, EmemType_e eEmemType)
{
    u32 nCrcMain = 0, nCrcMainHardware = 0, nCrcMainEflash = 0;
    u32 nCrcInfo = 0, nCrcInfoHardware = 0, nCrcInfoEflash = 0;

    TS_LOG_INFO("  %s() eEmemType = %d  \n", __func__, eEmemType);

    if (mstar_check_fw_bin_ingerity(szFwData) < 0) {
        TS_LOG_ERR("CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");

        tskit_mstar_data->fw_update_data.fw_data_cont = 0; // reset g_fw_data_cont to 0

        mstar_dev_hw_reset();

        return -1;
    }

     tskit_mstar_data->fw_updating = TRUE; // Set flag to 0x01 for indicating update firmware is processing

    // Erase
    if (eEmemType == EMEM_ALL) {
        mstar_erase_emem(EMEM_MAIN);
        mstar_erase_emem(EMEM_INFO);
    } else if (eEmemType == EMEM_MAIN) {
        mstar_erase_emem(EMEM_MAIN);
    } else if (eEmemType == EMEM_INFO) {
        mstar_erase_emem(EMEM_INFO);
    }

    TS_LOG_INFO("erase OK\n");

    // Program
    if (eEmemType == EMEM_ALL) {
        mstar_program_emem(EMEM_MAIN);
        mstar_program_emem(EMEM_INFO);
    } else if (eEmemType == EMEM_MAIN) {
        mstar_program_emem(EMEM_MAIN);
    } else if (eEmemType == EMEM_INFO) {
        mstar_program_emem(EMEM_INFO);
    }

    TS_LOG_INFO("program OK\n");

    /* Calculate main block CRC & info block CRC by device driver itself */
    mstar_convert_fw_two_dimen_to_one(szFwData, tskit_mstar_data->fw_update_data.one_dimen_fw_data);

    /* Read main block CRC & info block CRC from TP */
    if (eEmemType == EMEM_ALL) {
        nCrcMain =
            mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data, 0,
                    MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfo =
            mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data,
                    MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 + MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE,
                    MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE -
                    MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

        nCrcMainHardware = mstar_get_fw_crc_hw(EMEM_MAIN);
        nCrcInfoHardware = mstar_get_fw_crc_hw(EMEM_INFO);

        nCrcMainEflash = mstar_get_fw_crc_eflash(EMEM_MAIN);
        nCrcInfoEflash = mstar_get_fw_crc_eflash(EMEM_INFO);
    } else if (eEmemType == EMEM_MAIN) {
        nCrcMain =
            mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data, 0,
                    MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcMainHardware = mstar_get_fw_crc_hw(EMEM_MAIN);
        nCrcMainEflash = mstar_get_fw_crc_eflash(EMEM_MAIN);
    } else if (eEmemType == EMEM_INFO) {
        nCrcInfo =
            mstar_calculate_crc(tskit_mstar_data->fw_update_data.one_dimen_fw_data,
                    MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024 + MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE,
                    MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024 - MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE -
                    MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfoHardware = mstar_get_fw_crc_hw(EMEM_INFO);
        nCrcInfoEflash = mstar_get_fw_crc_eflash(EMEM_INFO);
    }

    TS_LOG_INFO
        ("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainHardware=0x%x, nCrcInfoHardware=0x%x, nCrcMainEflash=0x%x, nCrcInfoEflash=0x%x\n",
         nCrcMain, nCrcInfo, nCrcMainHardware, nCrcInfoHardware, nCrcMainEflash, nCrcInfoEflash);

    tskit_mstar_data->fw_update_data.fw_data_cont = 0; // Reset g_fw_data_cont to 0 after update firmware

    mstar_dev_hw_reset();
    mdelay(300);

    tskit_mstar_data->fw_updating = FALSE; // Set flag to 0x00 for indicating update firmware is finished

    if (eEmemType == EMEM_ALL) {
        if ((nCrcMainHardware != nCrcMain) || (nCrcInfoHardware != nCrcInfo) || (nCrcMainEflash != nCrcMain)
            || (nCrcInfoEflash != nCrcInfo)) {
            TS_LOG_INFO("Update FAILED\n");

            return -1;
        }
    } else if (eEmemType == EMEM_MAIN) {
        if ((nCrcMainHardware != nCrcMain) || (nCrcMainEflash != nCrcMain)) {
            TS_LOG_INFO("Update FAILED\n");

            return -1;
        }
    } else if (eEmemType == EMEM_INFO) {
        if ((nCrcInfoHardware != nCrcInfo) || (nCrcInfoEflash != nCrcInfo)) {
            TS_LOG_INFO("Update FAILED\n");

            return -1;
        }
    }

    TS_LOG_INFO("Update SUCCESS\n");

    return 0;
}

s32 mstar_update_fw_cash(u8 ** szFwData, EmemType_e eEmemType)
{
    u16 ic_sw_id = 0x0000;
    u16 fw_sw_id = 0x0000;

    TS_LOG_INFO("%s:tskit_mstar_data->chip_type = 0x%x\n", __func__, tskit_mstar_data->chip_type);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA)
    {
        TS_LOG_INFO("FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", tskit_mstar_data->apk_info->force_update_firmware_enable);

        // force to update firmware, don't check sw id
        if (tskit_mstar_data->apk_info->force_update_firmware_enable) {
            return mstar_28xx_fw_update(szFwData, EMEM_MAIN);
        } else {
            fw_sw_id = szFwData[129][1005] << 8 | szFwData[129][1004];
            ic_sw_id = mstar_get_swid(EMEM_INFO);
            TS_LOG_INFO("Firmware sw id = 0x%x, IC sw id = 0x%x\n", fw_sw_id, ic_sw_id);

           /* if ((ic_sw_id != fw_sw_id) && (ic_sw_id != 0xFFFF)) {
                mstar_dev_hw_reset();   // Reset HW here to avoid touch may be not worked after get sw id.
                TS_LOG_INFO("The sw id of the update firmware file is not equal to sw id on e-flash. Not allow to update\n");
                tskit_mstar_data->fw_update_data.fw_data_cont = 0;     // Reset g_fw_data_cont to 0
                return -1;
            } else */{
                return mstar_28xx_fw_update(szFwData, EMEM_MAIN);
            }
        }
    } else {
        TS_LOG_INFO("Unsupported chip type = 0x%x\n", tskit_mstar_data->chip_type);
        tskit_mstar_data->fw_update_data.fw_data_cont = 0; // Reset g_fw_data_cont to 0
        return -1;
    }
}

s32 mstar_fw_update_sdcard(const char *pFilePath, u8 mode)
{
    s32 ret = -1, i = 0;
    struct file *pfile = NULL;
    struct inode *inode;
    s32 fsize = 0;
    mm_segment_t old_fs;
    loff_t pos;
    u16 eSwId = 0x0000;
    u16 eVendorId = 0x0000;
    const struct firmware *fw = NULL;

    TS_LOG_INFO("  %s()  \n", __func__);

    tskit_mstar_data->fw_update_data.fw_data_buf = vmalloc(MSG28XX_FIRMWARE_WHOLE_SIZE * 1024 * sizeof(u8));
    tskit_mstar_data->fw_update_data.one_dimen_fw_data = vmalloc(MSG28XX_FIRMWARE_WHOLE_SIZE * 1024 * sizeof(u8));

    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.fw_data_buf) || ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        TS_LOG_ERR("Failed to allocate FW buffer \n");
        goto out;
    }

    tskit_mstar_data->fw_update_data.two_dimen_fw_data = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        TS_LOG_ERR("Failed to allocate FW buffer\n");
        goto out;
    }

    for (i = 0; i < 130; i++) {
        tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);

        if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
            TS_LOG_ERR("Failed to allocate FW buffer\n");
            goto out;
        }
    }

    if (mode == 1) {
        pfile = filp_open(pFilePath, O_RDONLY, 0);
        if (IS_ERR(pfile)) {
            TS_LOG_INFO("Error occurred while opening file %s.\n", pFilePath);
            goto out;
        }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 18, 0)
        inode = pfile->f_dentry->d_inode;
#else
        inode = pfile->f_path.dentry->d_inode;
#endif

        fsize = inode->i_size;

        TS_LOG_INFO("fsize = %d\n", fsize);

        if (fsize <= 0) {
            filp_close(pfile, NULL);
            goto out;
        }
        // read firmware
        memset(tskit_mstar_data->fw_update_data.fw_data_buf, 0, MSG28XX_FIRMWARE_WHOLE_SIZE * 1024);

        old_fs = get_fs();
        set_fs(KERNEL_DS);

        pos = 0;
        vfs_read(pfile, tskit_mstar_data->fw_update_data.fw_data_buf, fsize, &pos);

        filp_close(pfile, NULL);
        set_fs(old_fs);
    } else {
        ret = request_firmware(&fw, pFilePath, &tskit_mstar_data->mstar_chip_data->ts_platform_data->ts_dev->dev);
        if (ret) {
            TS_LOG_INFO("[MSTAR] failed to request firmware %d\n", ret);
            goto out;
        }
        if ((int)fw->size < MSG28XX_FIRMWARE_WHOLE_SIZE * 1024) {
            TS_LOG_ERR("request firmware size is less than 130KB\n");
            goto out;
        }
        memcpy(tskit_mstar_data->fw_update_data.fw_data_buf, fw->data, MSG28XX_FIRMWARE_WHOLE_SIZE * 1024);
        fsize = MSG28XX_FIRMWARE_WHOLE_SIZE * 1024;
    }

    mstar_fw_store_data(tskit_mstar_data->fw_update_data.fw_data_buf, fsize);

    mstar_finger_touch_report_disable();

    if ((tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA)
        && fsize == 133120) {
        ret = mstar_update_fw_cash(tskit_mstar_data->fw_update_data.two_dimen_fw_data, EMEM_MAIN);   // update main block only, do not update info block.
    } else {
        mstar_dev_hw_reset();
        TS_LOG_INFO("The file size of the update firmware bin file is invalid, fsize = %d\n", fsize);
    }

out:
    tskit_mstar_data->fw_update_data.fw_data_cont = 0; // reset g_fw_data_cont to 0 after update firmware

    mstar_finger_touch_report_enable();

    if (!ERR_ALLOC_MEM(fw)) {
        release_firmware(fw);
        fw = NULL;
    }

    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.fw_data_buf)) {
        vfree(tskit_mstar_data->fw_update_data.fw_data_buf);
        tskit_mstar_data->fw_update_data.fw_data_buf = NULL;
    }

    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        vfree(tskit_mstar_data->fw_update_data.one_dimen_fw_data);
        tskit_mstar_data->fw_update_data.one_dimen_fw_data = NULL;
    }

    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        for (i = 0; i < 130; i++) {
            if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i]);
                tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = NULL;
            }
        }

        kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data);
        tskit_mstar_data->fw_update_data.two_dimen_fw_data = NULL;
    }

    return ret;
}

static int mstar_read_dq_mem_start(void)
{
    int rc = NO_ERR;
    u8 nParCmdSelUseCfg = 0x7F;
    u8 nParCmdAdByteEn0 = 0x50;
    u8 nParCmdAdByteEn1 = 0x51;
    u8 nParCmdDaByteEn0 = 0x54;
    u8 nParCmdUSetSelB0 = 0x80;
    u8 nParCmdUSetSelB1 = 0x82;
    u8 nParCmdSetSelB2 = 0x85;
    u8 nParCmdIicUse = 0x35;

    TS_LOG_INFO("  %s()  \n", __func__);

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdSelUseCfg error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdAdByteEn0 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdAdByteEn1 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdDaByteEn0 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdUSetSelB0 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdUSetSelB1 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdSetSelB2 error %d\n", __func__, rc);
    }
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data nParCmdIicUse error %d\n", __func__, rc);
    }
    return rc;
}

static int mstar_read_dq_mem_end(void)
{
    u8 nParCmdNSelUseCfg = 0x7E;

    TS_LOG_INFO("  %s()  \n", __func__);

    return mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1);
}

u32 mstar_read_dq_mem_value(u16 nAddr)
{
    int rc = NO_ERR;
    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 tx_data[3] = { 0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF };
        u8 rx_data[4] = { 0 };

        TS_LOG_INFO("  %s()  \n", __func__);

        TS_LOG_INFO("DQMem Addr = 0x%x\n", nAddr);

        rc = mstar_dbbus_enter();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_dbbus_enter error %d\n", __func__, rc);
        }
        // Stop mcu
        rc = mstar_set_reg_low_byte(0x0FE6, 0x01);   //bank:mheg5, addr:h0073
        if (rc < 0) {
            TS_LOG_ERR("%s Stop mcu error %d\n", __func__, rc);
        }
        mdelay(100);

        rc = mstar_read_dq_mem_start();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_start error %d\n", __func__, rc);
        }
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
        if (rc < 0) {
            TS_LOG_ERR("%s write data error %d\n", __func__, rc);
        }
        rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &rx_data[0], 4);
        if (rc < 0) {
            TS_LOG_ERR("%s read data error %d\n", __func__, rc);
        }
        rc = mstar_read_dq_mem_end();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_end error %d\n", __func__, rc);
        }
        // Start mcu
        rc = mstar_set_reg_low_byte(0x0FE6, 0x00);   //bank:mheg5, addr:h0073
        if (rc < 0) {
            TS_LOG_ERR("%s Start mcu error %d\n", __func__, rc);
        }
        rc = mstar_dbbus_exit();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_dbbus_exit error %d\n", __func__, rc);
        }
        return (rx_data[3] << 24 | rx_data[2] << 16 | rx_data[1] << 8 | rx_data[0]);
    } else {
        TS_LOG_INFO("  %s()  \n", __func__);

        // TODO : not support yet

        return 0;
    }
}

void mstar_write_dq_mem_value(u16 nAddr, u32 nData)
{
    int rc = NO_ERR;
    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 szDbBusTxData[7] = { 0 };

        TS_LOG_INFO("  %s()  \n", __func__);

        TS_LOG_INFO("DQMem Addr = 0x%x\n", nAddr);

        rc = mstar_dbbus_enter();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_dbbus_enter error %d\n", __func__, rc);
        }
        // Stop mcu
        rc = mstar_set_reg_low_byte(0x0FE6, 0x01);   //bank:mheg5, addr:h0073
        if (rc < 0) {
            TS_LOG_ERR("%s Stop mcu error %d\n", __func__, rc);
        }
        mdelay(100);

        rc = mstar_read_dq_mem_start();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_start error %d\n", __func__, rc);
        }
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = ((nAddr >> 8) & 0xff);
        szDbBusTxData[2] = (nAddr & 0xff);
        szDbBusTxData[3] = nData & 0x000000FF;
        szDbBusTxData[4] = ((nData & 0x0000FF00) >> 8);
        szDbBusTxData[5] = ((nData & 0x00FF0000) >> 16);
        szDbBusTxData[6] = ((nData & 0xFF000000) >> 24);
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 7);
        if (rc < 0) {
            TS_LOG_ERR("%s write data error %d\n", __func__, rc);
        }
        rc = mstar_read_dq_mem_end();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_read_dq_mem_end error %d\n", __func__, rc);
        }
        // Start mcu
        rc = mstar_set_reg_low_byte(0x0FE6, 0x00);   //bank:mheg5, addr:h0073
        if (rc < 0) {
            TS_LOG_ERR("%s Start mcu error %d\n", __func__, rc);
        }
        mdelay(100);

        rc = mstar_dbbus_exit();
        if (rc < 0) {
            TS_LOG_ERR("%s mstar_dbbus_exit error %d\n", __func__, rc);
        }
    } else {
        TS_LOG_INFO("  %s()  \n", __func__);

        // TODO : not support yet
    }
}

void mstar_fw_update_swid_entry(void)
{
    TS_LOG_INFO("  %s()  \n", __func__);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        mstar_fw_update_swid();
#endif
    }
}

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static void mstar_fw_update_swid(void)
{
    u32 nCrcMainA, nCrcMainB;
    u32 i, j, ret = 0;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 nIsSwIdValid = 0;
    u8 *pVersion = NULL;
    char fw_name[128] = { 0 };
    int copy_count = 0;
    int k = 0, l = 0;
    const struct firmware *fw = NULL;
    u16 ic_sw_id = 0;
    u16 fw_sw_id = 0;

    if(tskit_mstar_data->fw_only_depend_on_lcd){
        sprintf(fw_name, "ts/%s_%s_%s_%s.bin", tskit_mstar_data->mstar_chip_data->ts_platform_data->product_name,
			MSTAR_CHIP_NAME, tskit_mstar_data->project_id, tskit_mstar_data->lcd_module_name);
    }else{
        sprintf(fw_name, "ts/%s_%s_%s_%s.bin", tskit_mstar_data->mstar_chip_data->ts_platform_data->product_name,
			MSTAR_CHIP_NAME, tskit_mstar_data->project_id, tskit_mstar_data->mstar_chip_data->module_name);
    }
    TS_LOG_INFO("fw_name is %s\n", fw_name);
    ret = request_firmware(&fw, fw_name, &tskit_mstar_data->mstar_chip_data->ts_platform_data->ts_dev->dev);
    if (ret) {
        TS_LOG_ERR("[MSTAR] failed to request firmware %d\n", ret);
        goto exit;
    }

    TS_LOG_INFO("mstar fw->size = %d\n", (int)fw->size);

    if ((int)fw->size < (130 * 1024)) {
        TS_LOG_ERR("The size of firmware is too smaller\n");
        goto exit;
    }

    tskit_mstar_data->fw_update_data.one_dimen_fw_data = vmalloc(MSG28XX_FIRMWARE_WHOLE_SIZE * 1024 * sizeof(u8));
    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        TS_LOG_ERR("Failed to allocate FW buffer\n");
        goto out;
    }

    mstar_finger_touch_report_disable();
    nCrcMainA = mstar_get_fw_crc_hw(EMEM_MAIN);
    nCrcMainB = mstar_get_fw_crc_eflash(EMEM_MAIN);
    TS_LOG_INFO("main_hw_crc = 0x%x, main_eflash_crc = 0x%x\n", nCrcMainA, nCrcMainB);

#ifdef CONFIG_ENABLE_CODE_FOR_DEBUG
    if (nCrcMainA != nCrcMainB) {
        for (i = 0; i < 5; i++) {
            nCrcMainA = mstar_get_fw_crc_hw(EMEM_MAIN);
            nCrcMainB = mstar_get_fw_crc_eflash(EMEM_MAIN);
            TS_LOG_INFO("  Retry[%d] : main_hw_crc=0x%x, main_eflash_crc=0x%x  \n", i, nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB) {
                break;
            }
            mdelay(50);
        }
    }
#endif

        ic_sw_id = mstar_get_swid(EMEM_MAIN);
        fw_sw_id = fw->data[0x1FFF5] << 8 | fw->data[0x1FFF4];
        TS_LOG_INFO("Main mem ic swid = 0x%x, firmware file swid = 0x%x\n", ic_sw_id, fw_sw_id);
        if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG28XXA){
            if((fw_sw_id != LENS_AUO_SWID) && (fw_sw_id != LENS_EBBG_SWID) && (fw_sw_id != EELY_AUO_SWID) && (fw_sw_id != EELY_EBBG_SWID))
            {
                TS_LOG_ERR("%s sw_id cannot match success, firmware file swid = 0x%x\n",__func__,fw_sw_id);
                goto out;
            }
        }
        if (ic_sw_id != fw_sw_id) {
		TS_LOG_INFO("The MAIN_MEM swid is not equal updated fw swid, go to normal boot up process\n");
        }

    if ((nCrcMainA == nCrcMainB) && (ic_sw_id == fw_sw_id)) {

            TS_LOG_INFO("sw id is match\n");
            tskit_mstar_data->fw_update_data.swid_data.pUpdateBin = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
            if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin)) {
                TS_LOG_ERR("Failed to allocate FW buffer\n");
                goto out;
            }

            for (i = 0; i < 130; i++) {
                tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);

                if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i])) {
                    TS_LOG_ERR("Failed to allocate FW buffer\n");
                    goto out;
                }
            }

            for (k = 0; k < 130; k++) {
                for (l = 0; l < 1024; l++) {
                    tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[k][l] = fw->data[copy_count];
                    copy_count++;
                }
            }

#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
            nUpdateBinMajor =
                tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[127][1013] << 8 | tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[127][1012];
            nUpdateBinMinor =
                tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[127][1015] << 8 | tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[127][1014];
#else
            nUpdateBinMajor =
                (*(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + 0x1FFF5)) << 8 |
                (*(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + 0x1FFF4));
            nUpdateBinMinor =
                (*(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + 0x1FFF7)) << 8 |
                (*(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + 0x1FFF6));
#endif
            TS_LOG_INFO("The updated firmware file major=0x%x, minor=0x%x\n", nUpdateBinMajor, nUpdateBinMinor);
            mstar_get_customer_fw_ver_dbbus(EMEM_MAIN, &nMajor, &nMinor, &pVersion);
            TS_LOG_INFO("The IC fw version = 0x%x\n", nMinor);

            if ((nUpdateBinMinor & 0xFF) > (nMinor & 0xFF)) {
                if (tskit_mstar_data->fw_data.ver_flag) {
                    TS_LOG_DEBUG
                        ("SwId=0x%x, nMajor=%u, nMinor=%u.%u, nUpdateBinMajor=%u, nUpdateBinMinor=%u.%u\n",
                         ic_sw_id, nMajor, nMinor & 0xFF, (nMinor & 0xFF00) >> 8, nUpdateBinMajor,
                         nUpdateBinMinor & 0xFF, (nUpdateBinMinor & 0xFF00) >> 8);
                } else {
                    TS_LOG_DEBUG
                        ("SwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%u, nUpdateBinMinor=%u\n",
                         ic_sw_id, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);
                }

                tskit_mstar_data->fw_update_data.two_dimen_fw_data = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
                if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
                    TS_LOG_ERR("Failed to allocate FW buffer\n");
                    goto out;
                }

                for (i = 0; i < 130; i++) {
                    tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);

                    if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                        TS_LOG_ERR("Failed to allocate FW buffer\n");
                        goto out;
                    }
                }

                for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i++) {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
                    mstar_fw_store_data(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i], 1024);
#else
                    mstar_fw_store_data((tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + i * 1024), 1024);
#endif
                }

                if ((ic_sw_id != 0x0000) && (ic_sw_id != 0xFFFF)) {
                    tskit_mstar_data->fw_update_data.fw_data_cont = 0;             // Reset g_fw_data_cont to 0 after copying update firmware data to temp buffer
                    tskit_mstar_data->fw_update_data.update_retry_cont = UPDATE_FIRMWARE_RETRY_COUNT;
                    //g_update_info_block_first = 1;  // Set 1 for indicating main block is complete
                    tskit_mstar_data->fw_updating = TRUE;

                    TS_LOG_INFO("Start to upgrade new firmware process\n");
                    mstar_update_fw_swid_do_work(NULL);

                } else {
                    TS_LOG_INFO("The MAIN_MEM swid is invalid, go to normal boot up process\n");
                }
            } else {
                TS_LOG_INFO("update version is older than or equal to the current version, go to normal boot up process\n");
            }
    } else {

        TS_LOG_INFO("IC firmware is lost, force to upgrade app firmware\n");
        tskit_mstar_data->fw_update_data.swid_data.pUpdateBin = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
        if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin)) {
            TS_LOG_ERR("Failed to allocate FW buffer\n");
            goto out;
        }

        for (i = 0; i < 130; i++) {
            tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);
            if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i])) {
                TS_LOG_ERR("Failed to allocate FW buffer\n");
                goto out;
            }
        }

        tskit_mstar_data->fw_update_data.two_dimen_fw_data = (u8 **) kmalloc(130 * sizeof(u8 *), GFP_KERNEL);
        if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
            TS_LOG_ERR("Failed to allocate FW buffer\n");
            goto out;
        }

        for (i = 0; i < 130; i++) {
            tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = (u8 *) kmalloc(1024 * sizeof(u8), GFP_KERNEL);
            if (ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                TS_LOG_ERR("Failed to allocate FW buffer\n");
                goto out;
            }
        }

        for (k = 0; k < 130; k++) {
            for (l = 0; l < 1024; l++) {
                tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[k][l] = fw->data[copy_count];
                copy_count++;
            }
        }

        for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i++) {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
            mstar_fw_store_data(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i], 1024);
#else
            mstar_fw_store_data((tskit_mstar_data->fw_update_data.swid_data.pUpdateBin + i * 1024), 1024);
#endif
        }

        tskit_mstar_data->fw_update_data.fw_data_cont = 0;             // Reset g_fw_data_cont to 0 after copying update firmware data to temp buffer
        tskit_mstar_data->fw_update_data.update_retry_cont = UPDATE_FIRMWARE_RETRY_COUNT;
        //g_update_info_block_first = 0;  // Set 0 for indicating main block is broken
        tskit_mstar_data->fw_updating = TRUE;
        mstar_update_fw_swid_do_work(NULL);
    }

out:
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin)) {
        for (i = 0; i < 130; i++) {
            if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i])) {
                kfree(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i]);
                tskit_mstar_data->fw_update_data.swid_data.pUpdateBin[i] = NULL;
            }
        }

        kfree(tskit_mstar_data->fw_update_data.swid_data.pUpdateBin);
        tskit_mstar_data->fw_update_data.swid_data.pUpdateBin = NULL;
    }
#endif
    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.one_dimen_fw_data)) {
        vfree(tskit_mstar_data->fw_update_data.one_dimen_fw_data);
        tskit_mstar_data->fw_update_data.one_dimen_fw_data = NULL;
    }

    if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data)) {
        for (i = 0; i < 130; i++) {
            if (!ERR_ALLOC_MEM(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i])) {
                kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data[i]);
                tskit_mstar_data->fw_update_data.two_dimen_fw_data[i] = NULL;
            }
        }

        kfree(tskit_mstar_data->fw_update_data.two_dimen_fw_data);
        tskit_mstar_data->fw_update_data.two_dimen_fw_data = NULL;
    }
exit:
    mstar_dev_hw_reset();
    mstar_finger_touch_report_enable();
}
#endif

static void mstar_update_fw_swid_do_work(struct work_struct *pWork)
{
    s32 nRetVal = -1;

    TS_LOG_INFO("  %s() g_update_retry_cont = %d  \n", __func__, tskit_mstar_data->fw_update_data.update_retry_cont);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        nRetVal = mstar_28xx_fw_update(tskit_mstar_data->fw_update_data.two_dimen_fw_data, EMEM_MAIN);
#endif
    } else {
        TS_LOG_INFO("This chip type (0x%x) does not support update firmware by sw id\n", tskit_mstar_data->chip_type);

        mstar_dev_hw_reset();

        mstar_finger_touch_report_enable();

        nRetVal = -1;
        return;
    }

    TS_LOG_INFO("  Update firmware by sw id result = %d  \n", nRetVal);

    if (nRetVal == 0) {
        TS_LOG_INFO("Update firmware by sw id success\n");

        mstar_dev_hw_reset();

        mstar_finger_touch_report_enable();

        if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX
            || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
            //g_update_info_block_first = 0;
            tskit_mstar_data->fw_updating = FALSE;
        }
    } else {         // nRetVal == -1 for MSG22xx/MSG28xx/MSG58xxa or nRetVal == 2/3/4 for ILI21xx
        tskit_mstar_data->fw_update_data.update_retry_cont--;
        if (tskit_mstar_data->fw_update_data.update_retry_cont > 0) {
            TS_LOG_INFO("g_update_retry_cont = %d\n", tskit_mstar_data->fw_update_data.update_retry_cont);
            mstar_update_fw_swid_do_work(NULL);
        } else {
            TS_LOG_INFO("Update firmware by sw id failed\n");

            mstar_dev_hw_reset();

            mstar_finger_touch_report_enable();

            if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX
                || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
                //g_update_info_block_first = 0;
                tskit_mstar_data->fw_updating = FALSE;
            }
#if defined (CONFIG_HUAWEI_DSM)
            if (!dsm_client_ocuppy(ts_dclient)) {
                dsm_client_record(ts_dclient, "Mstar Update firmware by sw id failed\n");
                dsm_client_notify(ts_dclient,DSM_TP_FWUPDATE_ERROR_NO);
            }
#endif
        }
    }
}
