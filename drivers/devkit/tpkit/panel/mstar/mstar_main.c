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
 * @file    ilitek_drv_main.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */
#include <linux/regulator/consumer.h>
#include "mstar_apknode.h"
#include "mstar_common.h"
#include "mstar_mp.h"
#include "mstar_dts.h"
#include <huawei_platform/log/log_jank.h>
#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
#include <linux/dma-mapping.h>
#include <linux/i2c/i2c-msm-v2.h>
#endif


struct mstar_core_data *tskit_mstar_data = NULL;

u8 g_disable_suspend_touch = 0;

extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];

static void mstar_gesture_convert_coordinate(u8 * pRawData, u32 * pTranX, u32 * pTranY);
static void mstar_easy_wakeup_gesture_report_coordinate(u32 * buf, u32 count);

static s32 mstar_mutual_parse_packet(u8 * pPacket, u16 nLength, MutualTouchInfo_t * pInfo, struct ts_fingers *info);
static void mstar_exit_sleep_mode(void);
static void mstar_esd_check(void);
static int mstar_power_on(void);
static int mstar_power_off(void);
int mstar_enable_gesture_wakeup(u32 * pMode);
u16 mstar_get_reg_16bit(u16 nAddr)
{
    int rc = NO_ERR;
    u8 szTxData[3] = { 0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF };
    u8 szRxData[2] = { 0 };

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
		goto out;
    }
    rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &szRxData[0], 2);
    if (rc < 0) {
        TS_LOG_ERR("%s read data error\n", __func__);
		goto out;
    }

    return (szRxData[1] << 8 | szRxData[0]);

out:
	return rc;
}

u8 mstar_get_reg_low_byte(u16 nAddr)
{
    int rc = NO_ERR;
    u8 szTxData[3] = { 0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF };
    u8 szRxData = { 0 };

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
		goto out;
    }

    rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &szRxData, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s read data error\n", __func__);
		goto out;
    }

    return (szRxData);

out:
	return rc;
}

u8 mstar_get_reg_high_byte(u16 nAddr)
{
    int rc = NO_ERR;
    u8 szTxData[3] = { 0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1 };
    u8 szRxData = { 0 };

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
		goto out;
    }

    rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &szRxData, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s read data error\n", __func__);
		goto out;
    }

    return (szRxData);

out:
	return rc;
}

int mstar_get_reg_xbit(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit)
{
    int rc = NO_ERR;
    u16 nReadAddr = nAddr;
    u16 nReadSize = 0;
    u16 nLeft = nLength;
    u16 nOffset = 0;
    u8 szTxData[3] = { 0 };

    szTxData[0] = 0x10;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (nLeft > 0) {
        if (nLeft >= nMaxI2cLengthLimit) {
            nReadSize = nMaxI2cLengthLimit;

            szTxData[1] = (nReadAddr >> 8) & 0xFF;
            szTxData[2] = nReadAddr & 0xFF;

            rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
            if (rc < 0) {
                TS_LOG_ERR("%s write data error\n", __func__);
                goto out;
			}

            rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
			if (rc < 0) {
				TS_LOG_ERR("%s read data error\n", __func__);
				goto out;
			}

            nReadAddr = nReadAddr + nReadSize;  //set next read address
            nLeft = nLeft - nReadSize;
            nOffset = nOffset + nReadSize;
        } else {
            nReadSize = nLeft;

            szTxData[1] = (nReadAddr >> 8) & 0xFF;
            szTxData[2] = nReadAddr & 0xFF;

            rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
			if (rc < 0) {
				TS_LOG_ERR("%s write data error\n", __func__);
				goto out;
			}
            rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
			if (rc < 0) {
				TS_LOG_ERR("%s read data error\n", __func__);
				goto out;
			}
            nLeft = 0;
            nOffset = nOffset + nReadSize;
        }
    }

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);
    return rc;
}

int mstar_reg_get_xbit_write_4byte_value(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit)
{
    int rc = 0;
    u16 nReadAddr = nAddr;
    u16 nReadSize = 0;
    u16 nLeft = nLength;
    u16 nOffset = 0;
    u8 szTxData[4] = { 0 };

    szTxData[0] = 0x10;

    mutex_lock(&tskit_mstar_data->mutex_common);

    while (nLeft > 0) {
        if (nLeft >= nMaxI2cLengthLimit) {
			nReadSize = nMaxI2cLengthLimit;

			TS_LOG_DEBUG(" RegGetXBitValue# Length >= I2cMax   nReadAddr=%x, nReadSize=%d n", nReadAddr,nReadSize);

			szTxData[2] = (nReadAddr >> 8) & 0xFF;
			szTxData[3] = nReadAddr & 0xFF;

			rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
			if (rc < 0) {
				TS_LOG_ERR("%s write data error\n", __func__);
				goto out;
			}

			rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
			if (rc < 0) {
				TS_LOG_ERR("%s read data error\n", __func__);
				goto out;
			}

			nReadAddr = nReadAddr + nReadSize;
			nLeft = nLeft - nReadSize;
			nOffset = nOffset + nReadSize;
			TS_LOG_DEBUG( " RegGetXBitValue# Length >= I2cMax   nLeft=%d, nOffset=%d \n",nLeft, nOffset);
        } else {
			nReadSize = nLeft;

			TS_LOG_DEBUG("  RegGetXBitValue# Length < I2cMax   nReadAddr=%x, nReadSize=%d  \n", nReadAddr,nReadSize);

			szTxData[2] = (nReadAddr >> 8) & 0xFF;
			szTxData[3] = nReadAddr & 0xFF;

			rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);

			if (rc < 0) {
				TS_LOG_ERR("%s write data error\n", __func__);
				goto out;
			}

			rc = mstar_iic_read_data(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
			if (rc < 0) {
				TS_LOG_ERR("%s read data error\n", __func__);
				goto out;
			}

			nLeft = 0;
			nOffset = nOffset + nReadSize;
			TS_LOG_DEBUG( "  RegGetXBitValue# Length < I2cMax   nLeft=%d, nOffset=%d  \n", nLeft, nOffset);
        }
    }

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);
    return rc;
}

s32 mstar_set_reg_16bit(u16 nAddr, u16 nData)
{
    s32 rc = NO_ERR;
    u8 szTxData[5] = { 0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData & 0xFF, nData >> 8 };

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 5);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

s32 mstar_set_reg_16bit_retry(u16 nAddr, u16 nData)
{
	int count = 5, delay_time = 5;
    s32 rc = NO_ERR;
    u32 nRetryCount = 0;

    while (nRetryCount < count) {
        mdelay(delay_time);
        rc = mstar_set_reg_16bit(nAddr, nData);
        if (rc >= 0) {
            TS_LOG_INFO("mstar_set_reg_16bit(0x%x, 0x%x) success, rc = %d\n", nAddr, nData, rc);
            break;
        }
        nRetryCount++;
    }

    if (nRetryCount == count) {
        TS_LOG_INFO("mstar_set_reg_16bit(0x%x, 0x%x) failed, rc = %d\n", nAddr, nData, rc);
    }

    return rc;
}

int mstar_set_reg_low_byte(u16 nAddr, u8 nData)
{
    int rc = NO_ERR;
    u8 szTxData[4] = { 0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData };

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_set_reg_16bit_on(u16 nAddr, u16 nData)   //set bit on nData from 0 to 1
{
    u16 rData = mstar_get_reg_16bit(nAddr);

    rData |= nData;

    return mstar_set_reg_16bit(nAddr, rData);
}

int mstar_set_reg_16bit_off(u16 nAddr, u16 nData)  //set bit on nData from 1 to 0
{
    u16 rData = mstar_get_reg_16bit(nAddr);

    rData &= (~nData);

    return mstar_set_reg_16bit(nAddr, rData);
}

u16 mstar_get_reg_16bit_by_addr(u16 nAddr, AddressMode_e eAddressMode)
{
    u16 nData = 0;

    if (eAddressMode == ADDRESS_MODE_16BIT) {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }

    nData = mstar_get_reg_16bit(nAddr);

    return nData;
}

int mstar_set_reg_16bit_by_addr(u16 nAddr, u16 nData, AddressMode_e eAddressMode)
{
    if (eAddressMode == ADDRESS_MODE_16BIT) {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }

    return mstar_set_reg_16bit(nAddr, nData);
}

int mstar_reg_mask_16bit(u16 nAddr, u16 nMask, u16 nData, AddressMode_e eAddressMode)
{
    u16 nTmpData = 0;

    if (nData > nMask) {
		TS_LOG_ERR("%s nData > nMask", __func__);
        return -1;
    }

    nTmpData = mstar_get_reg_16bit_by_addr(nAddr, eAddressMode);
    nTmpData = (nTmpData & (~nMask));
    nTmpData = (nTmpData | nData);

    return mstar_set_reg_16bit_by_addr(nAddr, nTmpData, eAddressMode);
}

s32 mstar_dbbus_enter_serial_debug(void)
{
    s32 rc = NO_ERR;
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;

    TS_LOG_INFO("mstar_dbbus_enter_serial_debug\n");

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 5);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_exit_serial_debug(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // i2c response ack
    rc = mstar_reg_mask_16bit(0x1E4F, BIT15, BIT15, ADDRESS_MODE_16BIT);
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_reg_mask_16bit  0x1E4F error\n", __func__);
		goto out;
    }

    // Exit the Serial Debug Mode
    data[0] = 0x45;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
		goto out;
    }

    TS_LOG_INFO("%s success\n",__func__);

out:
    return rc;
}

int mstar_dbbus_i2c_response_ack(void)
{
    // i2c response ack
    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        return mstar_reg_mask_16bit(0x1E4F, BIT15, BIT15, ADDRESS_MODE_16BIT);
    }
}

int mstar_dbbus_iic_use_bus(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_iic_not_use_bus(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_iic_reshape(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_stop_mcu(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_not_stop_mcu(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
    }

    return rc;
}

int mstar_dbbus_wait_mcu(void)
{
    int rc = NO_ERR;
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write 0x37 data error\n", __func__);
		return rc;
    }

    data[0] = 0x61;
    rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, data, 1);
    if (rc < 0) {
        TS_LOG_ERR("%s write 0x61 data error\n", __func__);
	    return rc;
    }

	return NO_ERR;
}

int tpkit_i2c_read(u8 * values, u16 values_size)
{
    int ret = NO_ERR;
    struct ts_bus_info *bops = NULL;

    bops = tskit_mstar_data->mstar_chip_data->ts_platform_data->bops;

    ret = bops->bus_read(NULL, 0, values, values_size);
    if (ret) {
        TS_LOG_ERR("%s:fail, addrs: 0x%x\n", __func__, tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr);
		ret = -1;
    }
    if (ret == 0) {
        ret = values_size;
    }
    return ret;
}

int tpkit_i2c_write(u8 * values, u16 values_size)
{
    int ret = NO_ERR;
    struct ts_bus_info *bops = NULL;
    u8 default_zero = 0;

    bops = tskit_mstar_data->mstar_chip_data->ts_platform_data->bops;

    ret = bops->bus_write(values,values_size);
    if (ret) {
        TS_LOG_ERR("%s:fail, addrs: 0x%x\n", __func__, tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr);
		ret = -1;
    }

    return ret;
}

s32 mstar_iic_write_data(u8 nSlaveId, u8 * pBuf, u16 nSize)
{
    s32 rc = NO_ERR;

    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->client != NULL) {
        if ((tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA)
            && nSlaveId == SLAVE_I2C_ID_DWI2C && tskit_mstar_data->fw_updating) {
            TS_LOG_ERR("Not allow to execute SmBus command while update firmware.\n");
			rc = -1;
			goto out;
        }
		
		tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = nSlaveId;
		rc = tpkit_i2c_write(pBuf, nSize);
		
		// no return error if command is for serialDebug mode
		if (nSize == 5) {
                if (pBuf[0] == 0x53 && pBuf[1] == 0x45 && pBuf[2] == 0x52
                    && pBuf[3] == 0x44 && pBuf[4] == 0x42) {
				rc = nSize;
				goto out;
			}
		}
		
		if (nSize == 1) {
			if (pBuf[0] == 0x45) {
				rc = nSize;
				goto out;
			}
		}
		
        if (rc < 0) {
			TS_LOG_ERR("%s: error (%d), slave = %d, nSize= %d\n",__func__, rc, nSlaveId, nSize);
		}
    } else {
        TS_LOG_ERR("%s: i2c client is NULL\n",__func__);
		rc = -1;
    }

out:
    return rc;
}

s32 mstar_iic_read_data(u8 nSlaveId, u8 * pBuf, u16 nSize)
{
    s32 rc = NO_ERR;

    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->client != NULL) {
        if ((tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA)
            && nSlaveId == SLAVE_I2C_ID_DWI2C &&  tskit_mstar_data->fw_updating) {
            TS_LOG_ERR("%s: Not allow to execute SmBus command while update firmware.\n",__func__);
			rc = -1;
			goto out;
        } else {
            tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = nSlaveId;
            rc = tpkit_i2c_read(pBuf, nSize);
            if (rc < 0) {
                TS_LOG_ERR("%s: error (%d), slave = %d, size = %d\n",__func__, rc, nSlaveId, nSize);
				goto out;
            }
        }
    } else {
        TS_LOG_ERR("%s: i2c client is NULL\n",__func__);
		rc = -1;
    }

out:
    return rc;
}

s32 mstar_ii2c_segment_read_dbbus(u8 nRegBank, u8 nRegAddr, u8 * pBuf, u16 nSize, u16 nMaxI2cLengthLimit)
{
    s32 rc = NO_ERR;
    u16 nLeft = nSize;
    u16 nOffset = 0;
    u16 nSegmentLength = 0;
    u16 nReadSize = 0;
    u16 nOver = 0;
    u8 szWriteBuf[3] = { 0 };
    u8 nNextRegBank = nRegBank;
    u8 nNextRegAddr = nRegAddr;

    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->client != NULL) {
        u8 *pReadBuf = NULL;
        u16 nLength = 0;
        u8 nAddrBefore = tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr;

        tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = SLAVE_I2C_ID_DBBUS;

        if (nMaxI2cLengthLimit >= 256) {
            nSegmentLength = 256;
        } else {
            nSegmentLength = 128;
        }

        TS_LOG_DEBUG("nSegmentLength = %d\n", nSegmentLength);

        while (nLeft > 0) {
            szWriteBuf[0] = 0x10;
            nRegBank = nNextRegBank;
            szWriteBuf[1] = nRegBank;
            nRegAddr = nNextRegAddr;
            szWriteBuf[2] = nRegAddr;

            TS_LOG_DEBUG("nRegBank = 0x%x, nRegAddr = 0x%x\n", nRegBank, nRegAddr);

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nSegmentLength) {
                if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE) {
                    nNextRegAddr = nRegAddr + nSegmentLength;

                    TS_LOG_DEBUG("nNextRegAddr = 0x%x\n", nNextRegAddr);

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                } else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE) {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1;    // shift to read data from next register bank

                    TS_LOG_DEBUG("nNextRegBank = 0x%x\n", nNextRegBank);

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                } else {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1;    // shift to read data from next register bank

                    TS_LOG_DEBUG("nNextRegBank = 0x%x\n", nNextRegBank);

                    nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    TS_LOG_DEBUG("nOver = 0x%x\n", nOver);

                    nLength = nSegmentLength - nOver;
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            } else {
                if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE) {
                    nNextRegAddr = nRegAddr + nLeft;

                    TS_LOG_DEBUG("nNextRegAddr = 0x%x\n", nNextRegAddr);

                    nLength = nLeft;
                    nLeft = 0;
                } else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE) {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1;    // shift to read data from next register bank

                    TS_LOG_DEBUG("nNextRegBank = 0x%x\n", nNextRegBank);

                    nLength = nLeft;
                    nLeft = 0;
                } else {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1;    // shift to read data from next register bank

                    TS_LOG_DEBUG("nNextRegBank = 0x%x\n", nNextRegBank);

                    nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    TS_LOG_DEBUG("nOver = 0x%x\n", nOver);

                    nLength = nLeft - nOver;
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            }

            rc = mstar_iic_write_data(tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr, &szWriteBuf[0], 3);
            if (rc < 0) {
                TS_LOG_ERR("mstar_ii2c_segment_read_dbbus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            rc = mstar_iic_read_data(tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr, pReadBuf, nLength);
            if (rc < 0) {
                TS_LOG_ERR("mstar_ii2c_segment_read_dbbus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            } else {
                nReadSize = nReadSize + nLength;
            }
        }
        tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = nAddrBefore;
    } else {
        TS_LOG_ERR("i2c client is NULL\n");
    }

    return nReadSize;
}

s32 mstar_ii2c_segment_read_smbus(u16 nAddr, u8 * pBuf, u16 nSize, u16 nMaxI2cLengthLimit)
{
    s32 rc = NO_ERR;
    u16 nLeft = nSize;
    u16 nOffset = 0;
    u16 nReadSize = 0;
    u8 szWriteBuf[3] = { 0 };
    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->client != NULL) {
        u8 *pReadBuf = NULL;
        u16 nLength = 0;
        u8 nAddrBefore = tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr;

        tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = SLAVE_I2C_ID_DWI2C;
        while (nLeft > 0) {
            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nMaxI2cLengthLimit) {
                nLength = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
                nOffset += nLength;
            } else {
                nLength = nLeft;
                nLeft = 0;
            }

            rc = mstar_iic_write_data(tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr, &szWriteBuf[0], 3);
            if (rc < 0) {
                TS_LOG_ERR("mstar_ii2c_segment_read_smbus() -> i2c_master_send() error %d\n", rc);
                return rc;
            }

            rc = mstar_iic_read_data(tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr, pReadBuf, nLength);
            if (rc < 0) {
                TS_LOG_ERR("mstar_ii2c_segment_read_smbus() -> i2c_master_recv() error %d\n", rc);
                return rc;
            } else {
                nReadSize = nReadSize + nLength;
            }
        }
        tskit_mstar_data->mstar_chip_data->ts_platform_data->client->addr = nAddrBefore;
    } else {
        TS_LOG_ERR("i2c client is NULL\n");
    }

    return nReadSize;
}

static u8 mstar_calculate_checksum(u8 * pMsg, u32 nLength)
{
    s32 nCheckSum = 0;
    u32 i;

    for (i = 0; i < nLength; i++) {
        nCheckSum += pMsg[i];
    }

    return (u8) ((-nCheckSum) & 0xFF);
}

u32 mstar_convert_char_to_hex_digit(char *pCh, u32 nLength)
{
    u32 nRetVal = 0;
    u32 i;

    TS_LOG_DEBUG("%s: nLength = %d\n",__func__, nLength);

    for (i = 0; i < nLength; i++) {
        char ch = *pCh++;
        u32 n = 0;
        u8 nIsValidDigit = 0;

        if ((i == 0 && ch == '0') || (i == 1 && ch == 'x')) {
            continue;
        }

        if ('0' <= ch && ch <= '9') {
            n = ch - '0';
            nIsValidDigit = 1;
        } else if ('a' <= ch && ch <= 'f') {
            n = 10 + ch - 'a';
            nIsValidDigit = 1;
        } else if ('A' <= ch && ch <= 'F') {
            n = 10 + ch - 'A';
            nIsValidDigit = 1;
        }

        if (1 == nIsValidDigit) {
            nRetVal = n + nRetVal * 16;
        }
    }

    return nRetVal;
}

void mstar_read_file(char *pFilePath, u8 * pBuf, u16 nLength)
{
    struct file *pFile = NULL;
    mm_segment_t old_fs;
    ssize_t nReadBytes = 0;

    old_fs = get_fs();
    set_fs(get_ds());

    pFile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pFile) || pFile == NULL) {
        TS_LOG_ERR("%s: Open file failed: %s\n", __func__, pFilePath);
        return;
    }

    pFile->f_op->llseek(pFile, 0, SEEK_SET);
    nReadBytes = pFile->f_op->read(pFile, pBuf, nLength, &pFile->f_pos);

    TS_LOG_DEBUG("%s: Read %d bytes!\n", __func__, (int)nReadBytes);

    set_fs(old_fs);
    filp_close(pFile, NULL);
}

int mstar_hw_reset(void)
{
    int ret = NO_ERR;
	int high = 1, low = 0;
	int reset_gpio = tskit_mstar_data->mstar_chip_data->ts_platform_data->reset_gpio;

    ret = gpio_direction_output(reset_gpio, high);
    if (ret) {
        TS_LOG_ERR("%s:gpio direction output to high fail, ret=%d\n",__func__, ret);
		ret = -1;
        goto out;
    }

    msleep(10);

    ret = gpio_direction_output(reset_gpio, low);
    if (ret) {
        TS_LOG_ERR("%s:gpio direction output to low fail, ret=%d\n",__func__, ret);
		ret = -1;
        goto out;
    }

    msleep(10);

    ret = gpio_direction_output(reset_gpio, high);
    if (ret) {
        TS_LOG_ERR("%s:gpio direction output to high fail, ret=%d\n",__func__, ret);
		ret = -1;
        goto out;
    }
    msleep(25);
    tskit_mstar_data->reset_touch_flag = true;
    TS_LOG_INFO("%s: End Reset,tskit_mstar_data->reset_touch_flag=%d\n",__func__, tskit_mstar_data->reset_touch_flag);
out:
    return ret;

}

void mstar_dev_hw_reset(void)
{
    int ret = NO_ERR;

	TS_LOG_DEBUG("%s: Do Reset\n",__func__);

    ret = mstar_hw_reset();
    if(ret < 0){
        TS_LOG_ERR("%s Error\n",__func__);
#if defined (CONFIG_HUAWEI_DSM)
        ts_dmd_report(DSM_TP_ESD_ERROR_NO, "Mstar ESD reset fail.\n");
#endif
		goto out;
    }


    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status) {
        tskit_mstar_data->esd_enable = TRUE;
    }

out:
	return ret;
}

void mstar_exit_sleep_mode(void)
{
    int ret = NO_ERR;

    ret = mstar_hw_reset();
    if(ret < 0){
        TS_LOG_ERR("%s Error\n",__func__);
		goto out;
    }

    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status) {
        tskit_mstar_data->esd_enable = TRUE;
    }

out:
	return ret;
}

int mstar_optimize_current_consump(void)
{
    u32 i;
    int rc = NO_ERR;
    u8 szDbBusTxData[35] = { 0 };

    TS_LOG_INFO("%s tskit_mstar_data->chip_type = 0x%x\n", __func__, tskit_mstar_data->chip_type);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (tskit_mstar_data->gesture_data.wakeup_flag) {
		TS_LOG_ERR("%s: Do nothing in gesture mode\n",__func__);
        return rc;
    }
#endif

	if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
		TS_LOG_ERR("%s: This chip is MSG58XXA, do nothing\n",__func__);
		return rc;
	}

	mutex_lock(&tskit_mstar_data->mutex_common);

	mstar_dev_hw_reset();
        rc = mstar_dbbus_enter();
        if (rc < 0) {
		TS_LOG_ERR("%s:mstar_dbbus_enter error\n",__func__);
		goto out;
	}
	// Enable burst mode
        rc = mstar_set_reg_low_byte(0x1608, 0x21);
        if (rc < 0) {
		TS_LOG_ERR("%s:Enable burst mode error\n",__func__);
		goto out;
	}

	szDbBusTxData[0] = 0x10;
	szDbBusTxData[1] = 0x15;
	szDbBusTxData[2] = 0x20;    //bank:0x15, addr:h0010

	for (i = 0; i < 8; i++) {
		szDbBusTxData[i + 3] = 0xFF;
	}

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3 + 8); // Write 0xFF for reg 0x1510~0x1513
        if (rc < 0) {
		TS_LOG_ERR("%s:Write 0xFF for reg 0x1510~0x1513 error\n",__func__);
		goto out;
	}

	szDbBusTxData[0] = 0x10;
	szDbBusTxData[1] = 0x15;
	szDbBusTxData[2] = 0x28;    //bank:0x15, addr:h0014

	for (i = 0; i < 16; i++) {
		szDbBusTxData[i + 3] = 0x00;
	}

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3 + 16);    // Write 0x00 for reg 0x1514~0x151B
        if (rc < 0) {
		TS_LOG_ERR("%s:Write 0x00 for reg 0x1514~0x151B error\n",__func__);
	  goto out;
	}

	szDbBusTxData[0] = 0x10;
	szDbBusTxData[1] = 0x21;
	szDbBusTxData[2] = 0x40;    //bank:0x21, addr:h0020

	for (i = 0; i < 8; i++) {
		szDbBusTxData[i + 3] = 0xFF;
	}

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3 + 8); // Write 0xFF for reg 0x2120~0x2123
        if (rc < 0) {
		TS_LOG_ERR("%s:Write 0xFF for reg 0x2120~0x2123 error\n",__func__);
		goto out;
	}

	szDbBusTxData[0] = 0x10;
	szDbBusTxData[1] = 0x21;
	szDbBusTxData[2] = 0x20;    //bank:0x21, addr:h0010

	for (i = 0; i < 32; i++) {
		szDbBusTxData[i + 3] = 0xFF;
	}

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3 + 32);    // Write 0xFF for reg 0x2110~0x211F
        if (rc < 0) {
		TS_LOG_ERR("%s:Write 0xFF for reg 0x2110~0x211F error\n",__func__);
		goto out;
	}

	// Clear burst mode
        rc = mstar_set_reg_low_byte(0x1608, 0x20);
        if (rc < 0) {
		TS_LOG_ERR("%s:Clear burst mode error\n",__func__);
		goto out;
	}

        rc = mstar_dbbus_exit();
        if (rc < 0) {
		TS_LOG_ERR("%s:mstar_dbbus_exit error\n",__func__);
		goto out;
	}

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);
    return rc;
}

int mstar_enter_sleep_mode(void)
{
    int reset_gpio = 0;
    int ret = NO_ERR;

    reset_gpio = tskit_mstar_data->mstar_chip_data->ts_platform_data->reset_gpio;
    ret = mstar_optimize_current_consump();
    if (ret < 0) {
        TS_LOG_ERR("%s:mstar_optimize_current_consump error\n",__func__);
        return ret;
    }

	// reset_gpio must be a value other than 1
    if (reset_gpio >= 0 && reset_gpio != 1) {
        ret = gpio_direction_output(reset_gpio, 0);
        if (ret) {
            TS_LOG_ERR("%s:gpio direction output to 0 fail, ret=%d\n",__func__, ret);
            return ret;
        }
    }

    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status){
        tskit_mstar_data->esd_enable = FALSE;
    }
    return NO_ERR;
}

void mstar_finger_touch_report_disable(void)
{
    unsigned long nIrqFlag;

    TS_LOG_DEBUG("%s: int_flag = %d\n", __func__, tskit_mstar_data->int_flag);

    spin_lock_irqsave(&tskit_mstar_data->irq_lock, nIrqFlag);

    if (tskit_mstar_data->int_flag) {
        disable_irq_nosync(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_id);
        tskit_mstar_data->int_flag = FALSE;
    }

    spin_unlock_irqrestore(&tskit_mstar_data->irq_lock, nIrqFlag);
}

void mstar_finger_touch_report_enable(void)
{
    unsigned long nIrqFlag;

    TS_LOG_DEBUG("%s: int_flag = %d\n", __func__, tskit_mstar_data->int_flag);

    spin_lock_irqsave(&tskit_mstar_data->irq_lock, nIrqFlag);

    if (!tskit_mstar_data->int_flag) {
        enable_irq(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_id);

        tskit_mstar_data->int_flag = TRUE;
    }

    spin_unlock_irqrestore(&tskit_mstar_data->irq_lock, nIrqFlag);
}

void mstar_finger_touch_release(void)
{
    struct input_dev *dev;

    TS_LOG_DEBUG("point touch released\n");

    dev = tskit_mstar_data->mstar_chip_data->ts_platform_data->input_dev;

    if (dev) {
        input_mt_sync(dev);
        input_report_key(dev, BTN_TOUCH, 0);
        input_sync(dev);
        TS_LOG_DEBUG("%s: input_dev->name = %s\n", __func__,
                 dev->name);
    }
}

void mstar_esd_check(void)
{
    u8 szData[1] = { 0x00 };
    u32 i = 0, count = 3;
    s32 rc = 0;

    TS_LOG_DEBUG("%s: ESD enable = %d\n", __func__, tskit_mstar_data->esd_enable);

    mutex_lock(&tskit_mstar_data->mutex_common);
    if (!tskit_mstar_data->esd_enable) {
        goto out;
    }

    if (!tskit_mstar_data->int_flag)    // Skip ESD check while finger touch
    {
        TS_LOG_DEBUG("%s: Not allow to do ESD check while finger touch\n",__func__);
        goto out;
    }

    if (tskit_mstar_data->fw_updating)   // Check whether update frimware is finished
    {
        TS_LOG_DEBUG("%s: Not allow to do ESD check while update firmware is proceeding\n",__func__);
        goto out;
    }

    if (tskit_mstar_data->mp_testing) // Check whether mp test is proceeding
    {
        TS_LOG_DEBUG("%s: Not allow to do ESD check while mp test is proceeding.\n",__func__);
        goto out;
    }

    if (!tskit_mstar_data->esd_check)
    {
        TS_LOG_DEBUG("%s: Not allow to do ESD check while screen is being touched\n",__func__);
        goto out;
    }

    if (tskit_mstar_data->apk_info->firmware_log_enable) {
        if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE) {
            TS_LOG_DEBUG("%s: Not allow to do ESD check while firmware mode is DEBUG MODE.\n",__func__);
            goto out;
        }
    }

    if (tskit_mstar_data->apk_info->disable_esd_protection_check) {
        TS_LOG_DEBUG
            ("%s: Not allow to do ESD check while mp test is triggered by MTPTool APK through JNI interface.\n",__func__);
        goto out;
    }

    szData[0] = 0x00;   // Dummy command for ESD check

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX
            || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
            rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szData[0], 1);
            if(rc < 0){
                TS_LOG_ERR("%s: ESD check command fail\n",__func__);
            }
        } else {
            TS_LOG_ERR("Un-recognized chip type = 0x%x\n", tskit_mstar_data->chip_type);
            break;
        }

        if (rc >= 0) {
            TS_LOG_DEBUG("%s: ESD check success\n",__func__);
            break;
        }
        i++;
    }

    if (i >= count) {
        TS_LOG_ERR("%s: ESD check failed, rc = %d\n", __func__, rc);
        mstar_dev_hw_reset();
    }

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);
    tskit_mstar_data->esd_check = TRUE;
}


int mstar_get_customer_fw_ver(char *ppVersion)
{
    int rc = 0;
    u16 pMajor = 0;
    u16 pMinor = 0;

    mutex_lock(&tskit_mstar_data->mutex_common);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 szTxData[3] = { 0 };
        u8 szRxData[4] = { 0 };

        szTxData[0] = 0x03;

        //mstar_dev_hw_reset();

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
        if (rc < 0) {
            TS_LOG_ERR("%s write data error\n", __func__);
			goto out;
        }

        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);

        rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 4);
        if (rc < 0) {
            TS_LOG_ERR("%s read data error\n", __func__);
			goto out;
        }

        TS_LOG_DEBUG("%s: szRxData[0] = 0x%x, szRxData[1] = 0x%x, szRxData[2] = 0x%x, szRxData[3] = 0x%x\n",
                __func__, szRxData[0], szRxData[1], szRxData[2], szRxData[3]);

        pMajor = (szRxData[1] << 8) + szRxData[0];
        pMinor = (szRxData[3] << 8) + szRxData[2];
    }

    if (tskit_mstar_data->fw_data.ver_flag) {
        TS_LOG_INFO("%s: Major = %d,Minor = %d.%d\n",__func__,pMajor, (pMinor & 0xFF), ((pMinor >> 8) & 0xFF));
    } else {
        TS_LOG_INFO("%s: Major = %d,Minor = %d\n",__func__, pMajor,pMinor);
    }
    sprintf(ppVersion, "%05d.%05d", pMajor, pMinor);

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);

    return NO_ERR;
}

int mstar_get_platform_fw_ver(u8 ** ppVersion)
{
    int rc = NO_ERR;

    mutex_lock(&tskit_mstar_data->mutex_common);
    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 szTxData[1] = { 0 };
        u8 szRxData[10] = { 0 };

        szTxData[0] = 0x0C;


        //mstar_dev_hw_reset();

        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
        if (rc < 0) {
            TS_LOG_ERR("%s write data error\n", __func__);
            goto out;
        }
        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);
        rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 10);
        if (rc < 0) {
            TS_LOG_ERR("%s read data error\n", __func__);
            goto out;
        }

        TS_LOG_DEBUG("%s: szRxData[0] = 0x%x , %c \n",__func__, szRxData[0], szRxData[0]);
        TS_LOG_DEBUG("%s: szRxData[1] = 0x%x , %c \n",__func__, szRxData[1], szRxData[1]);
        TS_LOG_DEBUG("%s: szRxData[2] = 0x%x , %c \n",__func__, szRxData[2], szRxData[2]);
        TS_LOG_DEBUG("%s: szRxData[3] = 0x%x , %c \n",__func__, szRxData[3], szRxData[3]);
        TS_LOG_DEBUG("%s: szRxData[4] = 0x%x , %c \n",__func__, szRxData[4], szRxData[4]);
        TS_LOG_DEBUG("%s: szRxData[5] = 0x%x , %c \n",__func__, szRxData[5], szRxData[5]);
        TS_LOG_DEBUG("%s: szRxData[6] = 0x%x , %c \n",__func__, szRxData[6], szRxData[6]);
        TS_LOG_DEBUG("%s: szRxData[7] = 0x%x , %c \n",__func__, szRxData[7], szRxData[7]);
        TS_LOG_DEBUG("%s: szRxData[8] = 0x%x , %c \n",__func__, szRxData[8], szRxData[8]);
        TS_LOG_DEBUG("%s: szRxData[9] = 0x%x , %c \n",__func__, szRxData[9], szRxData[9]);

        if (*ppVersion == NULL) {
            *ppVersion = kzalloc(sizeof(u8) * 16, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%.10s", szRxData);
        sscanf(*ppVersion, "V%u.%u.%u\n", 
			&tskit_mstar_data->fw_data.platform_ver[0], 
			&tskit_mstar_data->fw_data.platform_ver[1], 
			&tskit_mstar_data->fw_data.platform_ver[2]);

        if (tskit_mstar_data->fw_data.platform_ver[0] * 100000 +
			(tskit_mstar_data->fw_data.platform_ver[1]) * 100 + 
			tskit_mstar_data->fw_data.platform_ver[2] >= 101003) {
            tskit_mstar_data->fw_data.ver_flag = 1;
        }
    } else {
        if (*ppVersion == NULL) {
            *ppVersion = kzalloc(sizeof(u8) * 16, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%s", "N/A");
    }

    TS_LOG_INFO("%s: platform firmware version = %s\n", __func__, *ppVersion);

out:
    mutex_unlock(&tskit_mstar_data->mutex_common);
	return rc;
}

s32 mstar_update_fw(u8 ** szFwData, EmemType_e eEmemType)
{
    TS_LOG_DEBUG("%s() enter\n", __func__);
    return mstar_update_fw_cash(szFwData, eEmemType);
}

s32 mstar_update_fw_sdcard(const char *pFilePath)
{
    s32 nRetVal = -1;
    TS_LOG_INFO("%s() enter\n", __func__);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        nRetVal = mstar_fw_update_sdcard(pFilePath, 1);
    } else {
        TS_LOG_INFO("This chip type (0x%x) does not support update firmware by sd card\n", tskit_mstar_data->chip_type);
    }
    return nRetVal;
}

u16 mstar_change_fw_mode(u16 nMode)
{
	int count = 5;

    TS_LOG_INFO("%s: nMode = 0x%x\n", __func__, nMode);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 szTxData[2] = { 0 };
        u32 i = 0;
        s32 rc;

        tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware.

		szTxData[0] = 0x02;
        szTxData[1] = (u8) nMode;

        mutex_lock(&tskit_mstar_data->mutex_common);
        while (i < count) {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
            if (rc >= 0) {
                TS_LOG_INFO("%s: Change firmware mode success\n",__func__);
                break;
            }

            i++;
        }
        if (i == count) {
            TS_LOG_ERR("%s: Change firmware mode failed, rc = %d\n",__func__, rc);
        }

        mutex_unlock(&tskit_mstar_data->mutex_common);
        tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
    }

    return nMode;
}

int mstar_mutual_get_fw_info(MutualFirmwareInfo_t * pInfo)
{
    int rc = NO_ERR, count = 5;

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        u8 szTxData[1] = { 0 };
        u8 szRxData[10] = { 0 };
        u32 i = 0;
        szTxData[0] = 0x01;
        tskit_mstar_data->finger_touch_disable = TRUE; // Skip finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware.

        mutex_lock(&tskit_mstar_data->mutex_common);

        while (i < count) {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
            if (rc >= 0) {
                TS_LOG_INFO("%s: Get firmware info mstar_iic_write_data() success\n",__func__);
            }

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 10);
            if (rc >= 0) {
                TS_LOG_INFO("%s: Get firmware info mstar_iic_read_data() success\n",__func__);
                TS_LOG_INFO("%s: FW mode: 0x%x\n",__func__, szRxData[1]);
                if (szRxData[1] == MSG28XX_FIRMWARE_MODE_DEMO_MODE || szRxData[1] == MSG28XX_FIRMWARE_MODE_DEBUG_MODE) {
                    break;
                } else {
                    i = 0;
                }
            }
            i++;
        }
        if (i == count) {
            TS_LOG_ERR("%s: Get firmware info failed, rc = %d\n",__func__, rc);
        }

        mutex_unlock(&tskit_mstar_data->mutex_common);

        // add protection for incorrect firmware info check
        if ((szRxData[1] == MSG28XX_FIRMWARE_MODE_DEBUG_MODE && szRxData[2] == 0xA7
             && (szRxData[5] == PACKET_TYPE_TOOTH_PATTERN || szRxData[5] == PACKET_TYPE_CSUB_PATTERN
			 || szRxData[5] == PACKET_TYPE_FOUT_PATTERN || szRxData[5] == PACKET_TYPE_FREQ_PATTERN))
			 || (szRxData[1] == MSG28XX_FIRMWARE_MODE_DEMO_MODE && szRxData[2] == 0x5A)) {
            pInfo->nFirmwareMode = szRxData[1];
            TS_LOG_INFO("%s: pInfo->nFirmwareMode = 0x%x\n",__func__, pInfo->nFirmwareMode);

            pInfo->nLogModePacketHeader = szRxData[2];
            pInfo->nLogModePacketLength = (szRxData[3] << 8) + szRxData[4];
            pInfo->nType = szRxData[5];
            pInfo->nMy = szRxData[6];
            pInfo->nMx = szRxData[7];
            pInfo->nSd = szRxData[8];
            pInfo->nSs = szRxData[9];

            TS_LOG_DEBUG("%s: pInfo->nLogModePacketHeader = 0x%x\n",__func__, pInfo->nLogModePacketHeader);
            TS_LOG_DEBUG("%s: pInfo->nLogModePacketLength = %d\n",__func__, pInfo->nLogModePacketLength);
            TS_LOG_DEBUG("%s: pInfo->nType = 0x%x\n",__func__, pInfo->nType);
            TS_LOG_DEBUG("%s: pInfo->nMy = %d\n",__func__, pInfo->nMy);
            TS_LOG_DEBUG("%s: pInfo->nMx = %d\n",__func__, pInfo->nMx);
            TS_LOG_DEBUG("%s: pInfo->nSd = %d\n",__func__, pInfo->nSd);
            TS_LOG_DEBUG("%s: pInfo->nSs = %d\n",__func__, pInfo->nSs);
        } else {
            TS_LOG_INFO("%s: Firmware info before correcting :\n",__func__);

            TS_LOG_DEBUG("%s: FirmwareMode = 0x%x\n",__func__, szRxData[1]);
            TS_LOG_DEBUG("%s: LogModePacketHeader = 0x%x\n",__func__, szRxData[2]);
            TS_LOG_DEBUG("%s: LogModePacketLength = %d\n",__func__, (szRxData[3] << 8) + szRxData[4]);
            TS_LOG_DEBUG("%s: Type = 0x%x\n",__func__, szRxData[5]);
            TS_LOG_DEBUG("%s: My = %d\n",__func__, szRxData[6]);
            TS_LOG_DEBUG("%s: Mx = %d\n",__func__, szRxData[7]);
            TS_LOG_DEBUG("%s: Sd = %d\n",__func__, szRxData[8]);
            TS_LOG_DEBUG("%s: Ss = %d\n",__func__, szRxData[9]);

            // Set firmware mode to demo mode(default)
            pInfo->nFirmwareMode = MSG28XX_FIRMWARE_MODE_DEMO_MODE;
            pInfo->nLogModePacketHeader = 0x5A;
            pInfo->nLogModePacketLength = MUTUAL_DEMO_MODE_PACKET_LENGTH;
            pInfo->nType = 0;
            pInfo->nMy = 0;
            pInfo->nMx = 0;
            pInfo->nSd = 0;
            pInfo->nSs = 0;

            TS_LOG_INFO("%s: Firmware info after correcting :\n",__func__);

            TS_LOG_DEBUG("%s: pInfo->nFirmwareMode = 0x%x\n",__func__, pInfo->nFirmwareMode);
            TS_LOG_DEBUG("%s: pInfo->nLogModePacketHeader = 0x%x\n",__func__, pInfo->nLogModePacketHeader);
            TS_LOG_DEBUG("%s: pInfo->nLogModePacketLength = %d\n",__func__, pInfo->nLogModePacketLength);
            TS_LOG_DEBUG("%s: pInfo->nType = 0x%x\n",__func__, pInfo->nType);
            TS_LOG_DEBUG("%s: pInfo->nMy = %d\n",__func__, pInfo->nMy);
            TS_LOG_DEBUG("%s: pInfo->nMx = %d\n",__func__, pInfo->nMx);
            TS_LOG_DEBUG("%s: pInfo->nSd = %d\n",__func__, pInfo->nSd);
            TS_LOG_DEBUG("%s: pInfo->nSs = %d\n",__func__, pInfo->nSs);
        }
        tskit_mstar_data->finger_touch_disable = FALSE; // Resume finger touch ISR handling after device driver have sent i2c command to firmware.
    }
    return rc;
}

void mstar_restore_fw_to_log_data(void)
{
    int ret = NO_ERR;

    TS_LOG_INFO("%s: g_switch_mode_apk = %d\n", __func__, tskit_mstar_data->apk_info->switch_mode);

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
        if (tskit_mstar_data->apk_info->switch_mode == 1) {
            MutualFirmwareInfo_t tInfo;

            memset(&tInfo, 0x0, sizeof(MutualFirmwareInfo_t));

            ret = mstar_mutual_get_fw_info(&tInfo);
            if (ret < 0) {
                TS_LOG_ERR("%s mutual_get_fw_info error ret = %d\n", __func__, ret);
            }
            TS_LOG_INFO("%s: tskit_mstar_data->fw_mode = 0x%x, tInfo.nFirmwareMode = 0x%x\n",__func__, tskit_mstar_data->fw_mode, tInfo.nFirmwareMode);

            // since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE && MSG28XX_FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode) {
                tskit_mstar_data->fw_mode = mstar_change_fw_mode(MSG28XX_FIRMWARE_MODE_DEBUG_MODE);
            } else {
                TS_LOG_INFO("%s: firmware mode is not restored\n",__func__);
            }
        }
    }
}

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
int mstar_get_touch_packet_addr(u16 * pDataAddress, u16 * pFlagAddress)
{
    int rc = NO_ERR;
    u32 i = 0, count = 5;
    u8 szTxData[1] = { 0 };
    u8 szRxData[4] = { 0 };

    if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA
        || (tskit_mstar_data->chip_type == CHIP_TYPE_MSG22XX && tskit_mstar_data->apk_info->self_freq_scan_enable)) {
        szTxData[0] = 0x05;
        mutex_lock(&tskit_mstar_data->mutex_common);

        while (i < count) {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
            if (rc >= 0) {
                TS_LOG_INFO("%s: Get touch packet address mstar_iic_write_data() success\n",__func__);
            }

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &szRxData[0], 4);
            if (rc >= 0) {
                TS_LOG_INFO("%s: Get touch packet address mstar_iic_read_data() success\n",__func__);
                break;
            }

            i++;
        }

        if (i == count) {
            TS_LOG_ERR("%s: Get touch packet address failed, rc = %d\n",__func__, rc);
        }

        if (rc < 0) {
            tskit_mstar_data->fw_data.support_seg = FALSE;
        } else {
			*pDataAddress = (szRxData[0] << 8) + szRxData[1];
			*pFlagAddress = (szRxData[2] << 8) + szRxData[3];
			tskit_mstar_data->fw_data.support_seg = TRUE;
			TS_LOG_DEBUG("  *pDataAddress = 0x%2X  \n", *pDataAddress);
			TS_LOG_DEBUG("  *pFlagAddress = 0x%2X  \n", *pFlagAddress);
        }
        mutex_unlock(&tskit_mstar_data->mutex_common);
    }
    return rc;
}

static int mstar_finger_touch_packet_flag_bit1(void)
{
    u8 szTxData[3] = { 0 };
    s32 nRetVal = NO_ERR;

    szTxData[0] = 0x53;
    szTxData[1] = (tskit_mstar_data->fw_data.packet_flag_addr >> 8) & 0xFF;
    szTxData[2] = tskit_mstar_data->fw_data.packet_flag_addr & 0xFF;

    nRetVal = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    if (nRetVal < 0) {
        TS_LOG_ERR("%s write data error\n", __func__);
        return nRetVal;
    }
    nRetVal = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &tskit_mstar_data->fw_data.packet_flag[0], 2);
    if (nRetVal < 0) {
        TS_LOG_ERR("%s read data error\n", __func__);
        return nRetVal;
    }

    if ((tskit_mstar_data->fw_data.packet_flag[0] & BIT1) == 0x00) {
        nRetVal = 0;    // Bit1 is 0
    } else {
        nRetVal = 1;    // Bit1 is 1
    }
    TS_LOG_INFO("%s: Bit1 = %d\n",__func__, nRetVal);

    return nRetVal;
}

static int mstar_reset_finger_touch_packet_flag_bit1(void)
{
    int rc = NO_ERR;
    u8 szTxData[4] = { 0 };

    szTxData[0] = 0x52;
    szTxData[1] = (tskit_mstar_data->fw_data.packet_flag_addr >> 8) & 0xFF;
    szTxData[2] = tskit_mstar_data->fw_data.packet_flag_addr & 0xFF;
    szTxData[3] = tskit_mstar_data->fw_data.packet_flag[0] | BIT1;

    rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
    if (rc < 0) {
        TS_LOG_INFO("%s write data error\n", __func__);
    }
    return rc;
}
#endif

static s32 mstar_mutual_read_ftd(u8 * pPacket, u16 nReportPacketLength)
{
    s32 rc = NO_ERR;
    TS_LOG_DEBUG("%s: read packet_data into\n", __func__);
    if (tskit_mstar_data->apk_info->firmware_log_enable) {
        if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEMO_MODE) {
            rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0) {
                TS_LOG_ERR("I2C read packet data failed, rc = %d\n", rc);
                return -1;
            }
        } else if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE) {
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            TS_LOG_DEBUG("%s: packet_data_addr = 0x%2X  \n", __func__, tskit_mstar_data->fw_data.packet_data_addr);
            TS_LOG_DEBUG("%s: packet_flag_addr = 0x%2X  \n", __func__, tskit_mstar_data->fw_data.packet_flag_addr);

            if (tskit_mstar_data->fw_data.support_seg== FALSE) {
                TS_LOG_ERR("%s: packet_data_addr & packet_flag_addr is un-initialized\n",__func__);
                return -1;
            }

            if (tskit_mstar_data->finger_touch_disable) {
                TS_LOG_ERR("%s: Skip finger touch for handling get firmware info or change firmware mode\n",__func__);
                return -1;
            }

            if (0 != mstar_finger_touch_packet_flag_bit1()) {
                TS_LOG_ERR("%s: Bit1 is not 0. FW is not ready for providing debug mode packet to Device Driver\n",__func__);
                return -1;
            }

            rc = mstar_ii2c_segment_read_smbus(tskit_mstar_data->fw_data.packet_data_addr, &pPacket[0], nReportPacketLength,
                               MAX_I2C_TRANSACTION_LENGTH_LIMIT);

            mstar_reset_finger_touch_packet_flag_bit1();

            if (rc < 0) {
                TS_LOG_ERR("%s: I2C read debug mode packet data failed, rc = %d\n",__func__, rc);
                return -1;
            }
#else

            rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0) {
                TS_LOG_ERR("%s: I2C read packet data failed, rc = %d\n",__func__, rc);
                return -1;
            }
#endif
        } else {
            TS_LOG_ERR("%s: WRONG FIRMWARE MODE : 0x%x\n",__func__, tskit_mstar_data->fw_mode);
            return -1;
        }
    } else {
        rc = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
        if (rc < 0) {
            TS_LOG_ERR("%s: I2C read packet data failed, rc = %d\n",__func__, rc);
            return -1;
        }
    }

    return NO_ERR;
}


/**
 *  Receive data when fw mode stays at i2cuart mode.
 *
 *  the first is to receive N bytes depending on the mode that firmware stays
 *  before going in this function, and it would check with i2c buffer if it
 *  remains the rest of data.
 */
static int i2cuart_recv_packet(u8 * pPacket, u16 nReportPacketLength)
{
	int res = NO_ERR, need_read_len = 0, one_data_bytes = 0;
	int type = pPacket[3] & 0x0F;
	int actual_len = nReportPacketLength - 5;

	TS_LOG_DEBUG("%s: pid = %x, data[3] = %x, type = %x, actual_len = %d\n",
	    __func__, pPacket[0], pPacket[3], type, actual_len);

	need_read_len = pPacket[1] * pPacket[2];

	if (type == 0 || type == 1 || type == 6) {
		one_data_bytes = 1;
	} else if (type == 2 || type == 3) {
		one_data_bytes = 2;
	} else if (type == 4 || type == 5) {
		one_data_bytes = 4;
	}

	TS_LOG_DEBUG("%s: need_read_len = %d  one_data_bytes = %d\n",__func__, need_read_len, one_data_bytes);

	need_read_len = need_read_len * one_data_bytes + 1;
	tskit_mstar_data->i2cuart_len = need_read_len + 5;

	if (need_read_len > actual_len) {

		tskit_mstar_data->i2cuart_data = kzalloc(tskit_mstar_data->i2cuart_len, GFP_ATOMIC);
		if (ERR_ALLOC_MEM(tskit_mstar_data->i2cuart_data)) {
			TS_LOG_ERR("%s: Failed to allocate g_fr_uart memory \n",__func__);
			return -1;
		}
		
		memcpy(tskit_mstar_data->i2cuart_data, pPacket, nReportPacketLength);
		res = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, tskit_mstar_data->i2cuart_data + nReportPacketLength, need_read_len - actual_len);
		if (res < 0) {
			TS_LOG_ERR("%s: Failed to read i2c uart packet\n",__func__);
		}
	}
	return res;
}

static void mstar_read_touch_data(struct ts_fingers *info)
{
    MutualTouchInfo_t tInfo;
    static u32 nLastCount = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;
    int try = 0;
    int delay_time = 0;
    struct i2c_adapter* adapter = NULL;
    bool gesture_recovery = false;
    int i2c_retries = I2C_RW_TRIES;
#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
	struct i2c_msm_ctrl *ctrl = NULL;
#endif
    struct ts_easy_wakeup_info *gestrue_info = &tskit_mstar_data->mstar_chip_data->easy_wakeup_info;
    if (tskit_mstar_data->finger_touch_disable) {
        TS_LOG_ERR("%s: Skip finger touch for handling get firmware info or change firmware mode\n",__func__);
        return;
    }

    if (g_disable_suspend_touch) {
        TS_LOG_ERR("%s:skip incorrect read during gesture suspend\n",__func__);
        return;
    }
    if(tskit_mstar_data->reset_touch_flag)
    {
        TS_LOG_INFO("%s:skip finger touch for hw_reset trigger INT\n",__func__);
        tskit_mstar_data->reset_touch_flag = false;
        return;
    }
   adapter = i2c_get_adapter(tskit_mstar_data->mstar_chip_data->ts_platform_data->bops->bus_id);
    if (!adapter)
    {
	TS_LOG_ERR("%s i2c_get_adapter failed\n",__func__);
	return;
    }
#if defined(CONFIG_HUAWEI_DEVKIT_QCOM)
    ctrl = (struct i2c_msm_ctrl *)adapter->dev.driver_data;
    /*if the easy_wakeup_flag is false,status not is at sleep state;switch_value is false,gesture is no supported*/
    if ((true == tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value) &&
        (true == tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_flag)){
		do {
			if (ctrl->pwr_state == I2C_MSM_PM_SYS_SUSPENDED) {
				TS_LOG_INFO("%s gesture mode, waiting for i2c bus resume\n",__func__);
				++ try;
				msleep(I2C_WAIT_TIME);
			} else { /*I2C_MSM_PM_RT_SUSPENDED or I2C_MSM_PM_RT_ACTIVE*/
				delay_time = try * I2C_WAIT_TIME;
				TS_LOG_INFO("%s i2c bus resuming or resumed,wait system resume after %d ms,try %d times\n",__func__,delay_time,try);
				break;
			}
		} while (i2c_retries--);
	    if (ctrl->pwr_state == I2C_MSM_PM_SYS_SUSPENDED) {
		delay_time = try * I2C_WAIT_TIME;
		TS_LOG_INFO("%s trigger gesture irq in system suspending,i2c bus can't resume, so ignore irq;wait %d ms \n",__func__,delay_time);
		return;
	    }
	}
#endif

    mutex_lock(&tskit_mstar_data->mutex_common);
    mutex_lock(&tskit_mstar_data->mutex_protect);
    memset(&tInfo, 0x0, sizeof(MutualTouchInfo_t));

    if (tskit_mstar_data->apk_info->firmware_log_enable) {
        if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEMO_MODE) {
            TS_LOG_DEBUG("%s: FIRMWARE_MODE_DEMO_MODE\n",__func__);

            nReportPacketLength = MUTUAL_DEMO_MODE_PACKET_LENGTH;
            pPacket = tskit_mstar_data->demo_packet;
        } else if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE) {
            TS_LOG_DEBUG("%s: FIRMWARE_MODE_DEBUG_MODE\n",__func__);

            if (tskit_mstar_data->mutual_fw_info.nLogModePacketHeader != 0xA7) {
                TS_LOG_ERR("%s: WRONG DEBUG MODE HEADER : 0x%x\n",__func__, tskit_mstar_data->mutual_fw_info.nLogModePacketHeader);
                goto TouchHandleEnd;
            }

            nReportPacketLength = tskit_mstar_data->mutual_fw_info.nLogModePacketLength;
            pPacket = tskit_mstar_data->debug_packet;
        } else {
            TS_LOG_ERR("%s: WRONG FIRMWARE MODE : 0x%x\n",__func__, tskit_mstar_data->fw_mode);
            goto TouchHandleEnd;
        }
    } else {
        TS_LOG_DEBUG("%s: FIRMWARE_MODE_DEMO_MODE\n",__func__);
        nReportPacketLength = MUTUAL_DEMO_MODE_PACKET_LENGTH;
        pPacket = tskit_mstar_data->demo_packet;
    }

    if (tskit_mstar_data->gesture_data.wakeup_flag) {
        TS_LOG_INFO("%s: Set gesture wakeup packet length\n",__func__);
		if(!tskit_mstar_data->ges_self_debug) {
			nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
			pPacket = tskit_mstar_data->gesture_data.wakeup_packet;
		} else {
			TS_LOG_DEBUG("%s: Set length and packet to debug in gesture mode\n",__func__);
			nReportPacketLength = MUTUAL_DEBUG_MODE_PACKET_LENGTH;
			pPacket = tskit_mstar_data->debug_packet;
		}
    }

    TS_LOG_DEBUG("%s  nReportPacketLength = %d\n", __func__,  nReportPacketLength);

    if (0 != mstar_mutual_read_ftd(&pPacket[0], nReportPacketLength)) {
		TS_LOG_ERR("%s: Failed to read touch data\n",__func__);
        goto TouchHandleEnd;
    }

    if (pPacket[0] == 0x7A) {
    	  TS_LOG_DEBUG("%s: I2CUART(0x%x): prepare to receive rest of data\n",__func__, pPacket[0]);
    	  if (i2cuart_recv_packet(&pPacket[0], nReportPacketLength) < 0) {
            TS_LOG_ERR("%s read i2cuart packet error\n", __func__);
        }
    }
    if (tskit_mstar_data->gesture_data.wakeup_flag) {
        if(pPacket[0] == GESTURE_PACKET_HEADER)
        {
		    gesture_recovery = true;
		    goto TouchHandleEnd;
        }
    }
    mutex_lock(&tskit_mstar_data->apk_info->debug_mutex);
    if (tskit_mstar_data->apk_info->debug_node_open) {
    	memset(tskit_mstar_data->apk_info->fw_debug_buf[tskit_mstar_data->apk_info->debug_data_frame], 0x00,
    	       (uint8_t) sizeof(uint8_t) * 2048);
        if (!ERR_ALLOC_MEM(tskit_mstar_data->i2cuart_data)) {
             	memcpy(tskit_mstar_data->apk_info->fw_debug_buf[tskit_mstar_data->apk_info->debug_data_frame], tskit_mstar_data->i2cuart_data, tskit_mstar_data->i2cuart_len);
             kfree(tskit_mstar_data->i2cuart_data);
             tskit_mstar_data->i2cuart_data = NULL;
        }
        else {
    	        memcpy(tskit_mstar_data->apk_info->fw_debug_buf[tskit_mstar_data->apk_info->debug_data_frame], pPacket, nReportPacketLength);
        }
    	tskit_mstar_data->apk_info->debug_data_frame++;
    	if (tskit_mstar_data->apk_info->debug_data_frame > 1) {
    		TS_LOG_DEBUG("%s: debug_data_frame = %d\n",__func__, tskit_mstar_data->apk_info->debug_data_frame);
    	}
    	if (tskit_mstar_data->apk_info->debug_data_frame > 1023) {
    		TS_LOG_DEBUG("debug_data_frame = %d > 1024\n",__func__,
    			tskit_mstar_data->apk_info->debug_data_frame);
    		tskit_mstar_data->apk_info->debug_data_frame = 1023;
    	}
    	wake_up(&(tskit_mstar_data->apk_info->inq));
    }
    mutex_unlock(&tskit_mstar_data->apk_info->debug_mutex);

    if (pPacket[0] == 0x7A) {
        goto TouchHandleEnd;
    }

    if (0 == mstar_mutual_parse_packet(pPacket, nReportPacketLength, &tInfo, info)) {
        TS_LOG_DEBUG("%s: tInfo.nCount = %d, nLastCount = %d\n",__func__, tInfo.nCount, nLastCount);
    }

TouchHandleEnd:
    mutex_unlock(&tskit_mstar_data->mutex_protect);
    mutex_unlock(&tskit_mstar_data->mutex_common);
    if(gesture_recovery)
    {
        TS_LOG_INFO("%s:Gesture Header is fail, recovery gesture mode form demo mode \n",__func__);
        mdelay(20);
        gestrue_info->easy_wakeup_flag = false;
        tskit_mstar_data->reset_touch_flag = true;
        mstar_enable_gesture_wakeup(&tskit_mstar_data->gesture_data.wakeup_mode[0]);
        mdelay(50);
    }
}

s32 mstar_chip_detect_init(void)
{
    s32 nRetVal = NO_ERR;

    tskit_mstar_data->apk_info = mstar_malloc_apknode();
    if(tskit_mstar_data->apk_info == NULL) {
		return -ENODEV;
    }

    nRetVal = mstar_apknode_para_init();
    if (nRetVal < 0) {
        TS_LOG_ERR("%s apk node para init error ret = %d\n", __func__, nRetVal);
    }

    nRetVal = mstar_apknode_create_procfs();
    if (nRetVal < 0) {
        TS_LOG_ERR("%s apk node create procfs error ret = %d\n", __func__, nRetVal);
    }

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    mstar_apknode_create_jni_msg();
#endif

    // Try to get chip type by SLAVE_I2C_ID_DBBUS(0x62) firstly.
    tskit_mstar_data->chip_type = mstar_get_chip_type();

    if (tskit_mstar_data->chip_type == 0xbf) {
        TS_LOG_INFO("%s: chip: MSG28XXA\n",__func__);
    }

    if (tskit_mstar_data->chip_type == 0) {
        tskit_mstar_data->chip_type = mstar_get_chip_type();
    }

    mstar_dev_hw_reset();

   // To make sure TP is attached on cell phone.
    if (tskit_mstar_data->chip_type != 0) {
        if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
            memset(&tskit_mstar_data->mutual_fw_info, 0x0, sizeof(MutualFirmwareInfo_t));
        }
        if (tskit_mstar_data->chip_type == CHIP_TYPE_MSG28XX || tskit_mstar_data->chip_type == CHIP_TYPE_MSG58XXA) {
            tskit_mstar_data->fw_mode = MSG28XX_FIRMWARE_MODE_DEMO_MODE;
        }
    } else {
        nRetVal = -ENODEV;
    }

    return nRetVal;
}

void mstar_mutex_init(void)
{
    mutex_init(&tskit_mstar_data->mutex_common);
    mutex_init(&tskit_mstar_data->mutex_protect);
    spin_lock_init(&tskit_mstar_data->irq_lock);
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 mstar_unregister_device(void)
{
    mstar_power_off();

    if (tskit_mstar_data->apk_info->touch_kset) {
        kset_unregister(tskit_mstar_data->apk_info->touch_kset);
        tskit_mstar_data->apk_info->touch_kset = NULL;
    }

    if (tskit_mstar_data->apk_info->touch_kobj) {
        kobject_put(tskit_mstar_data->apk_info->touch_kobj);
        tskit_mstar_data->apk_info->touch_kobj = NULL;
    }

    mstar_apknode_remove_procfs();

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    mstar_apknode_delete_jni_msg();
#endif

    return NO_ERR;
}
static int mstar_get_roi_data_form_fw(void)
{
    int i, ret = 0;
    u8 szTxData[2] = {0};
    szTxData[0] = 0x0E;
    TS_LOG_DEBUG("%s()into\n", __func__);
    ret = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, szTxData, 1);
    if (ret < 0) {
        TS_LOG_ERR("%s  write fingersense read data cmd failed\n", __func__);
    }

	mdelay(10);

    ret = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, tskit_mstar_data->roi_data, ROI_DATA_READ_LENGTH);
    if (ret < 0) {
        TS_LOG_ERR("%s  read fingersense data failed\n", __func__);
    }

    for(i = 0; i < ROI_DATA_READ_LENGTH; i++) {
        TS_LOG_DEBUG(" %02x ", tskit_mstar_data->roi_data[i]);
        if(i != 0 && (i % 16 == 0))
            TS_LOG_DEBUG("\n");
    }

    return ret;
}

static int mstar_get_gesture_code(u8 *pPacket, struct ts_fingers *info)
{
	int ret = NO_ERR;
	u8 nWakeupMode = 0;
	u8 bIsCorrectFormat = 0;
	u32 nReportGestureType = 0;
	u32 nGestureCount = 0;
	u32 nTmpBuffer[80] = { 0 };

	TS_LOG_DEBUG("%s: p[0]=%x, p[1]=%x, p[2]=%x, p[3]=%x, p[4]=%x, p[5]=%x \n",
		 __func__, pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

	if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06
		&& pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP) {
		nWakeupMode = pPacket[4];
		bIsCorrectFormat = 1;
	} else if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80
		 && pPacket[3] == PACKET_TYPE_GESTURE_INFORMATION) {

		u32 a = 0;
		u32 nTmpCount = 0;
		u32 nWidth = 0;
		u32 nHeight = 0;
		nWakeupMode = pPacket[4];
		bIsCorrectFormat = 1;

		for (a = 0; a < 6; a++) //header
		{
			tskit_mstar_data->gesture_data.log_info[nTmpCount] = pPacket[a];
			nTmpCount++;
		}

		for (a = 6; a < 126; a = a + 3) //parse packet to coordinate
		{
			u32 nTranX = 0;
			u32 nTranY = 0;

			mstar_gesture_convert_coordinate(&pPacket[a], &nTranX, &nTranY);
			tskit_mstar_data->gesture_data.log_info[nTmpCount] = nTranX;
			nTmpBuffer[nGestureCount * 2] = nTranX;
			nTmpCount++;
			tskit_mstar_data->gesture_data.log_info[nTmpCount] = nTranY;
			nTmpBuffer[nGestureCount * 2 + 1] = nTranY;
			nTmpCount++;
			nGestureCount++;
		}

		nWidth = (((pPacket[12] & 0xF0) << 4) | pPacket[13]);   //parse width & height
		nHeight = (((pPacket[12] & 0x0F) << 8) | pPacket[14]);

		TS_LOG_DEBUG("%s: Before convert [width,height]=[%d,%d]\n",__func__, nWidth, nHeight);

		if ((pPacket[12] == 0xFF) && (pPacket[13] == 0xFF) && (pPacket[14] == 0xFF)) {
			nWidth = 0;
			nHeight = 0;
		} else {
			nWidth = (nWidth * tskit_mstar_data->mstar_chip_data->x_max) / TPD_WIDTH;
			nHeight = (nHeight * tskit_mstar_data->mstar_chip_data->y_max) / TPD_HEIGHT;
			TS_LOG_DEBUG("%s: After convert [width,height]=[%d,%d]\n",__func__, nWidth, nHeight);
		}

		tskit_mstar_data->gesture_data.log_info[10] = nWidth;
		tskit_mstar_data->gesture_data.log_info[11] = nHeight;

		tskit_mstar_data->gesture_data.log_info[nTmpCount] = pPacket[126];   //Dummy
		nTmpCount++;
		tskit_mstar_data->gesture_data.log_info[nTmpCount] = pPacket[127];   //checksum
		nTmpCount++;
		TS_LOG_DEBUG("%s: gesture information mode Count = %d\n",__func__, nTmpCount);
	}

	if (bIsCorrectFormat) {
		TS_LOG_INFO("%s: nWakeupMode = 0x%x\n",__func__, nWakeupMode);
		switch (nWakeupMode) {
		case KEY_CODE_DOUBLE_CLICK:
			TS_LOG_INFO("%s: Light up screen by DOUBLE_CLICK gesture wakeup.\n",__func__);
			if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &&
				tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture) {
				nReportGestureType = TS_DOUBLE_CLICK;
				LOG_JANK_D(JLID_TP_GESTURE_KEY, "JL_TP_GESTURE_KEY");
				nGestureCount = LINEAR_LOCUS_NUM;
			}
			break;

		case KEY_CODE_M:
			TS_LOG_DEBUG("%s: GESTURE_WAKEUP_MODE_M_CHARACTER_FLAG.\n",__func__);
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_m) &&
				tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture) {
				nReportGestureType = TS_LETTER_m;
				nGestureCount = LETTER_LOCUS_NUM;
			}
			break;

		case KEY_CODE_W:
			TS_LOG_DEBUG("%s: GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG.\n",__func__);
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_w) &&
				tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture) {
				nReportGestureType = TS_LETTER_c;
				nGestureCount = LETTER_LOCUS_NUM;
			}
			break;

		case KEY_CODE_C:
			TS_LOG_DEBUG("%s: GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG.\n",__func__);

			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_c) &&
				tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture) {
				nReportGestureType = TS_LETTER_c;
				nGestureCount = LETTER_LOCUS_NUM;
			}
			break;

		case KEY_CODE_E:
			TS_LOG_DEBUG("%s: GESTURE_WAKEUP_MODE_E_CHARACTER_FLAG.\n",__func__);
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_e) &&
				tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture) {
				nReportGestureType = TS_LETTER_e;
				nGestureCount = LETTER_LOCUS_NUM;
			}
			break;
		default:
			TS_LOG_ERR("%s: Un-supported gesture wakeup mode. Please check your device driver code.\n",__func__);
			break;
		}
	} else {
		TS_LOG_ERR("%s: gesture wakeup packet format is incorrect. pPacket[0]=%x\n",__func__,pPacket[0]);
		ret = -1;
		goto out;
	}

	if (nReportGestureType != 0) {
		wake_lock_timeout(&tskit_mstar_data->mstar_chip_data->ts_platform_data->ts_wake_lock, 5 * HZ);
		if (true == tskit_mstar_data->mstar_chip_data->easy_wakeup_info.off_motion_on) {
			tskit_mstar_data->mstar_chip_data->easy_wakeup_info.off_motion_on = false;
			mstar_easy_wakeup_gesture_report_coordinate(nTmpBuffer, nGestureCount);
			info->gesture_wakeup_value = nReportGestureType;
			TS_LOG_DEBUG("%s: info->gesture_wakeup_value = %d\n", __func__,
					info->gesture_wakeup_value);
		}
	}

out:
	return ret;
}

static s32 mstar_mutual_parse_packet(u8 * pPacket, u16 nLength, MutualTouchInfo_t * pInfo, struct ts_fingers *info)
{
    u32 i, ret = 0;
    u8 nCheckSum = 0;
    u32 nX = 0, nY = 0;
    int touch_count = 0;
    static int pre_finger_num = 0;

    TS_LOG_DEBUG("%s: received raw data from touch panel as following:\n", __func__);
    TS_LOG_DEBUG("%s: pPacket[0]=%x pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x pPacket[8]=%x\n",__func__,
         pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7],
         pPacket[8]);

    if (tskit_mstar_data->apk_info->firmware_special_log_enable) {
        if (pPacket[0] == 0x2C) {
            // Notify android application to retrieve firmware specific debug value packet from device driver by sysfs.
            if (tskit_mstar_data->apk_info->touch_kobj != NULL) {
                char szRspFwSpecificLogPacket[512] = { 0 };
                char szValue[3] = { 0 };
                char *pEnvp[3];
                s32 nRetVal = 0;
                strcat(szRspFwSpecificLogPacket, "VALUE=");

                for (i = 0; i < nLength; i++) {
                    sprintf(szValue, "%02x", pPacket[i]);
                    strcat(szRspFwSpecificLogPacket, szValue);
                }

                pEnvp[0] = "STATUS=GET_FW_LOG";
                pEnvp[1] = szRspFwSpecificLogPacket;
                pEnvp[2] = NULL;
                TS_LOG_DEBUG("%s: pEnvp[1] = %s\n",__func__, pEnvp[1]);
                TS_LOG_DEBUG("%s: g_demo_packet[] = %s\n",__func__, pPacket);

                nRetVal = kobject_uevent_env(tskit_mstar_data->apk_info->touch_kobj, KOBJ_CHANGE, pEnvp);
                TS_LOG_DEBUG("%s: kobject_uevent_env() STATUS=GET_FW_LOG, nRetVal = %d\n",__func__, nRetVal);
            }

            return -1;
        }
    }

    nCheckSum = mstar_calculate_checksum(&pPacket[0], (nLength - 1));
    TS_LOG_DEBUG("%s: checksum : [%x] == [%x]? \n",__func__, pPacket[nLength - 1], nCheckSum);

	if((tskit_mstar_data->ges_self_debug == false) &&
		(tskit_mstar_data->fw_mode != MSG28XX_FIRMWARE_MODE_DEBUG_MODE)) {
		if (pPacket[nLength - 1] != nCheckSum) {
			TS_LOG_ERR("%s: WRONG CHECKSUM\n",__func__);
	#if defined (CONFIG_HUAWEI_DSM)
			if (!dsm_client_ocuppy(ts_dclient)) {
				dsm_client_record(ts_dclient, "Mstar mutual_parse_packet WRONG CHECKSUM\n");
				dsm_client_notify(ts_dclient,DSM_TP_FW_CRC_ERROR_NO);
			}
	#endif
			return -1;
		}
	}

	/* For gesture packet */
    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == true && !tskit_mstar_data->ges_self_debug) {
        if (tskit_mstar_data->gesture_data.wakeup_flag) {
			return mstar_get_gesture_code(pPacket, info);
        }
    }

    if (tskit_mstar_data->apk_info->firmware_log_enable) {
        if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x5A) {
            TS_LOG_ERR("%s: WRONG DEMO MODE HEADER.firmware_log_enable. pPacket[0]=%x \n",__func__,pPacket[0]);
            return -1;
        } else if (tskit_mstar_data->fw_mode == MSG28XX_FIRMWARE_MODE_DEBUG_MODE
               && (pPacket[0] != 0xA7 && pPacket[3] != PACKET_TYPE_TOOTH_PATTERN && pPacket[3] != PACKET_TYPE_CSUB_PATTERN
			   && pPacket[3] != PACKET_TYPE_FOUT_PATTERN && pPacket[3] != PACKET_TYPE_FREQ_PATTERN)) {
            TS_LOG_ERR("%s: WRONG DEBUG MODE HEADER\n",__func__);
            return -1;
        }
    } else {
        if (pPacket[0] != 0x5A) {
            TS_LOG_ERR("%s: WRONG DEMO MODE HEADER. pPacket[0]=%x \n",__func__,pPacket[0]);
            return -1;
        }
    }

    // Process raw data...
    if (pPacket[0] == 0x5A) {
        for (i = 0; i < MUTUAL_MAX_TOUCH_NUM; i++) {
            if ((pPacket[(4 * i) + 1] == 0xFF) && (pPacket[(4 * i) + 2] == 0xFF)
                && (pPacket[(4 * i) + 3] == 0xFF)) {
                continue;
            }

            nX = (((pPacket[(4 * i) + 1] & 0xF0) << 4) | (pPacket[(4 * i) + 2]));
            nY = (((pPacket[(4 * i) + 1] & 0x0F) << 8) | (pPacket[(4 * i) + 3]));

            pInfo->tPoint[pInfo->nCount].nX = nX * tskit_mstar_data->mstar_chip_data->x_max / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * tskit_mstar_data->mstar_chip_data->y_max / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = pPacket[4 * (i + 1)];
            pInfo->tPoint[pInfo->nCount].nId = i;

            info->fingers[i].status = TP_FINGER;
            info->fingers[i].x = nX * tskit_mstar_data->mstar_chip_data->x_max / TPD_WIDTH;
            info->fingers[i].y = nY * tskit_mstar_data->mstar_chip_data->y_max / TPD_HEIGHT;
            info->fingers[i].pressure = pPacket[4 * (i + 1)];

            pInfo->nCount++;
            touch_count = i + 1;
        }

        if (tskit_mstar_data->apk_info->firmware_log_enable) {
            // Notify android application to retrieve demo mode packet from device driver by sysfs.
            if (tskit_mstar_data->apk_info->touch_kobj != NULL) {
                char szRspDemoModePacket[512] = { 0 };
                char szValue[3] = { 0 };
                char *pEnvp[3];
                s32 nRetVal = 0;

                strcat(szRspDemoModePacket, "VALUE=");

                for (i = 0; i < nLength; i++) {
                    sprintf(szValue, "%02x", pPacket[i]);
                    strcat(szRspDemoModePacket, szValue);
                }

                pEnvp[0] = "STATUS=GET_DEMO_MODE_PACKET";
                pEnvp[1] = szRspDemoModePacket;
                pEnvp[2] = NULL;
                TS_LOG_DEBUG("%s: pEnvp[1] = %s\n",__func__, pEnvp[1]);  // TODO : add for debug

                nRetVal = kobject_uevent_env(tskit_mstar_data->apk_info->touch_kobj, KOBJ_CHANGE, pEnvp);
                TS_LOG_DEBUG("%s: kobject_uevent_env() STATUS=GET_DEMO_MODE_PACKET, nRetVal = %d\n",
                        __func__, nRetVal);
            }
        }
    } else if (pPacket[0] == 0xA7 && (pPacket[3] == PACKET_TYPE_TOOTH_PATTERN || pPacket[3] == PACKET_TYPE_CSUB_PATTERN ||
	           pPacket[3] == PACKET_TYPE_FOUT_PATTERN || pPacket[3] == PACKET_TYPE_FREQ_PATTERN)) {
        for (i = 0; i < MUTUAL_MAX_TOUCH_NUM; i++) {
            if ((pPacket[(3 * i) + 5] == 0xFF) && (pPacket[(3 * i) + 6] == 0xFF)
                && (pPacket[(3 * i) + 7] == 0xFF)) {
                continue;
            }

            nX = (((pPacket[(3 * i) + 5] & 0xF0) << 4) | (pPacket[(3 * i) + 6]));
            nY = (((pPacket[(3 * i) + 5] & 0x0F) << 8) | (pPacket[(3 * i) + 7]));

            pInfo->tPoint[pInfo->nCount].nX = nX * tskit_mstar_data->mstar_chip_data->x_max / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * tskit_mstar_data->mstar_chip_data->y_max / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;

            info->fingers[i].status = TP_FINGER;
            info->fingers[i].x = nX * tskit_mstar_data->mstar_chip_data->x_max / TPD_WIDTH;
            info->fingers[i].y = nY * tskit_mstar_data->mstar_chip_data->y_max / TPD_HEIGHT;
            info->fingers[i].pressure = pPacket[4 * (i + 1)];
            touch_count = i + 1;

            pInfo->nCount++;

        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.
        if (tskit_mstar_data->apk_info->touch_kobj != NULL) {
            char *pEnvp[2];
            s32 nRetVal = 0;

            memcpy(tskit_mstar_data->log_packet, tskit_mstar_data->debug_packet, nLength);  // Copy g_debug_packet to g_log_packet for avoiding the debug mode data which is received by MTPTool APK may be modified.

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";
            pEnvp[1] = NULL;

            nRetVal = kobject_uevent_env(tskit_mstar_data->apk_info->touch_kobj, KOBJ_CHANGE, pEnvp);
            TS_LOG_DEBUG("%s: kobject_uevent_env() STATUS=GET_DEBUG_MODE_PACKET, nRetVal = %d\n", __func__, nRetVal);
        }
    }
    if (tskit_mstar_data->roi_enabled) {
		TS_LOG_DEBUG("%s: pre_finger_num = %d, touch_count = %d\n",__func__,pre_finger_num,touch_count);
		if ((pre_finger_num != touch_count) && (touch_count <= 2) && (touch_count > 0)) {
			TS_LOG_INFO("%s: pre_finger_num = %d, touch_count = %d mstar_get_roi_data_form_fw() into\n",__func__,pre_finger_num,touch_count);
			mstar_get_roi_data_form_fw();
		}
    }
    pre_finger_num = touch_count;
    info->cur_finger_number = touch_count;
    return NO_ERR;
}

int mstar_enable_gesture_wakeup(u32 * pMode)
{
    u8 szTxData[4] = { 0 };
    u32 i = 0, count = 5;
    s32 rc = 0;
    struct ts_easy_wakeup_info *info = &tskit_mstar_data->mstar_chip_data->easy_wakeup_info;

    /*if the sleep_gesture_flag is ture,it presents that  the tp is at sleep state */
    if (tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == false ||
        true == info->easy_wakeup_flag) {
        TS_LOG_ERR("%s: easy_wakeup_flag is %d, do nothing\n", __func__, info->easy_wakeup_flag);
        return rc;
    }

    TS_LOG_INFO("%s: wakeup mode 0 = 0x%x\n", __func__, pMode[0]);
    TS_LOG_INFO("%s: wakeup mode 1 = 0x%x\n", __func__, pMode[1]);

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    szTxData[0] = 0x59;
    szTxData[1] = 0x00;
    szTxData[2] = ((pMode[1] & 0xFF000000) >> 24);
    szTxData[3] = ((pMode[1] & 0x00FF0000) >> 16);

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);   // delay 20ms
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], ARRAY_SIZE(szTxData));
        if (rc >= 0) {
            TS_LOG_INFO("%s: Enable gesture wakeup index 0 success\n",__func__);
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("%s: Enable gesture wakeup index 0 failed\n",__func__);
    }

    szTxData[0] = 0x59;
    szTxData[1] = 0x01;
    szTxData[2] = ((pMode[1] & 0x0000FF00) >> 8);
    szTxData[3] = ((pMode[1] & 0x000000FF) >> 0);

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);   // delay 20ms
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
        if (rc >= 0) {
            TS_LOG_INFO("%s: Enable gesture wakeup index 1 success\n",__func__);
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("%s: Enable gesture wakeup index 1 failed\n",__func__);
    }

    szTxData[0] = 0x59;
    szTxData[1] = 0x02;
    szTxData[2] = ((pMode[0] & 0xFF000000) >> 24);
    szTxData[3] = ((pMode[0] & 0x00FF0000) >> 16);

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);   // delay 20ms
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
        if (rc >= 0) {
            TS_LOG_INFO("%s: Enable gesture wakeup index 2 success\n",__func__);
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("%s: Enable gesture wakeup index 2 failed\n",__func__);
    }

    szTxData[0] = 0x59;
    szTxData[1] = 0x03;
    szTxData[2] = ((pMode[0] & 0x0000FF00) >> 8);
    szTxData[3] = ((pMode[0] & 0x000000FF) >> 0);

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);   // delay 20ms
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
        if (rc >= 0) {
            TS_LOG_INFO("%s: Enable gesture wakeup index 3 success\n",__func__);
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("%s: Enable gesture wakeup index 3 failed\n",__func__);
    }

    mutex_lock(&tskit_mstar_data->mutex_protect);
    tskit_mstar_data->gesture_data.wakeup_flag = TRUE;  // gesture wakeup is enabled
    mutex_unlock(&tskit_mstar_data->mutex_protect);
#else

    szTxData[0] = 0x58;
    szTxData[1] = ((pMode[0] & 0x0000FF00) >> 8);
    szTxData[2] = ((pMode[0] & 0x000000FF) >> 0);

    while (i < count) {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);   // delay 20ms
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, szTxData, 3);
        if (rc >= 0) {
            TS_LOG_INFO("%s: Enable gesture wakeup success\n",__func__);
            break;
        }

        i++;
    }
    if (i == count) {
        TS_LOG_ERR("%s: Enable gesture wakeup failed\n",__func__);
    }

    mutex_lock(&tskit_mstar_data->mutex_protect);
    tskit_mstar_data->gesture_data.wakeup_flag = TRUE;  // gesture wakeup is enabled
    mutex_unlock(&tskit_mstar_data->mutex_protect);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    info->easy_wakeup_flag = true;
    return rc;
}

static void mstar_put_device_outof_easy_wakeup(void)
{
    int retval = 0;
    struct ts_easy_wakeup_info *info = &tskit_mstar_data->mstar_chip_data->easy_wakeup_info;

    TS_LOG_INFO("%s: info->easy_wakeup_flag = %d\n",__func__, info->easy_wakeup_flag);

    if (false == info->easy_wakeup_flag) {
        return;
    }
    info->easy_wakeup_flag = false;
    return;
}

void mstar_disable_gesture_wakeup(void)
{
    mutex_lock(&tskit_mstar_data->mutex_protect);
    tskit_mstar_data->gesture_data.wakeup_flag = FALSE;  // gesture wakeup is disabled
    mutex_unlock(&tskit_mstar_data->mutex_protect);
}

static void mstar_gesture_convert_coordinate(u8 * pRawData, u32 * pTranX, u32 * pTranY)
{
    u32 nX;
    u32 nY;
#ifdef CONFIG_SWAP_X_Y
    u32 nTempX;
    u32 nTempY;
#endif

    nX = (((pRawData[0] & 0xF0) << 4) | pRawData[1]);   // parse the packet to coordinate
    nY = (((pRawData[0] & 0x0F) << 8) | pRawData[2]);


#ifdef CONFIG_SWAP_X_Y
    nTempY = nX;
    nTempX = nY;
    nX = nTempX;
    nY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
    nX = 2047 - nX;
#endif

#ifdef CONFIG_REVERSE_Y
    nY = 2047 - nY;
#endif

    /*
     * pRawData[0]~pRawData[2] : the point abs,
     * pRawData[0]~pRawData[2] all are 0xFF, release touch
     */
    if ((pRawData[0] == 0xFF) && (pRawData[1] == 0xFF) && (pRawData[2] == 0xFF)) {
        *pTranX = 0;    // final X coordinate
        *pTranY = 0;    // final Y coordinate
        TS_LOG_INFO("%s: gesture released, final coordinate\n",__func__);
    } else {
        /* one touch point */
        *pTranX = (nX * tskit_mstar_data->mstar_chip_data->x_max) / TPD_WIDTH;
        *pTranY = (nY * tskit_mstar_data->mstar_chip_data->y_max) / TPD_HEIGHT;
    }
}

static void mstar_easy_wakeup_gesture_report_coordinate(u32 * buf, u32 count)
{
    int x = 0;
    int y = 0, i = 0;

    if (count != 0) {
        if (count == 2) {
            for (i = 0; i < count; i++) {
                x = (s16) (buf[i * 2 + 0]);
                y = (s16) (buf[i * 2 + 1]);

                tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[i] = x << 16 | y;
                TS_LOG_DEBUG("easywake_position[%d] = 0x%04x\n", i,
                         tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[i]);
            }
        } else {
            /*1.beginning */
            x = (s16) (buf[0]);
            y = (s16) (buf[1]);
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[0] = x << 16 | y;

            /*2.end */
            x =  (s16)  buf[2];
            y =  (s16)  buf[3];
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[1] = x << 16 | y;

            /*3.top */
            x =  (s16)  buf[6];
            y =  (s16)  buf[7];
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[2] = x << 16 | y;

            /*4.leftmost */
            x =  (s16)  buf[8];
            y =  (s16)  buf[9];
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[3] = x << 16 | y;

            /*5.bottom */
            x =  (s16)  buf[10];
            y =  (s16)  buf[11];
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[4] = x << 16 | y;

            /*6.rightmost */
            x =  (s16)  buf[12];
            y =  (s16)  buf[13];
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easywake_position[5] = x << 16 | y;
        }
    }

}

static int mstar_get_glove_switch(unsigned char *mode)
{
    mstar_apknode_get_glove_info(mode);
    return NO_ERR;
}

static int mstar_set_glove_switch(unsigned char mode)
{
    int ret = 0;

    mstar_finger_touch_report_disable();
    if (mode == 1) {
        mstar_apknode_open_glove();
    } else {
        mstar_apknode_close_glove();
    }

    mstar_finger_touch_report_enable();
    return NO_ERR;
}

static unsigned char * mstar_roi_rawdata (void)
{
	u8 * rawdata_ptr = NULL;
	int i = 0;

	mutex_lock(&(tskit_mstar_data->mutex_common));

	for (i = 0; i < ROI_DATA_READ_LENGTH; i++)
		tskit_mstar_data->roi_data_send[i] = tskit_mstar_data->roi_data[i];

	mutex_unlock(&(tskit_mstar_data->mutex_common));
	return tskit_mstar_data->roi_data_send;
}

int mstar_into_fingersense_mode(bool status)
{
	int ret = 0;
	uint8_t cmd[2] = {0};
	int write_len = 2;

	TS_LOG_INFO("%s, status = %d\n", __func__, status);
	
	cmd[0] = 0x0F;
	if (status) {
		cmd[1] = 0x01;
	} else {
		cmd[1] = 0x00;
	}

	ret = mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, cmd, write_len);
	if(ret < 0){
		TS_LOG_ERR("%s: write data error, ret = %d\n", __func__, ret);
		return ret;
	}
	return NO_ERR;
}


static int mstar_set_roi_switch(u8 roi_switch)
{
	int error = 0;

	TS_LOG_INFO("%s:roi_switch = %d\n", __func__, roi_switch);

	if (tskit_mstar_data->fw_updating){
		TS_LOG_ERR("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}

	if (roi_switch < 0) {
		TS_LOG_ERR("%s: roi_switch value %d is invalid\n", __func__, roi_switch);
		return -EINVAL;
	}

	if (roi_switch) {
		error = mstar_into_fingersense_mode(true);
	}
	else {
		error = mstar_into_fingersense_mode(false);
	}
	if (error) {
		TS_LOG_ERR("%s: into fingersense mode error roi_switch  = %d",__func__, roi_switch);
	}
	tskit_mstar_data->roi_enabled = roi_switch;
	TS_LOG_DEBUG("%s: roi_switch value is %u\n",__func__, tskit_mstar_data->roi_enabled);
	return error;
}

static int mstar_get_roi_switch(u8 roi_switch)
{
	roi_switch = tskit_mstar_data->roi_enabled;
	return NO_ERR;
}


static int mstar_roi_switch(struct ts_roi_info *info)
{
	int retval = NO_ERR;

	if (!info) {
		TS_LOG_ERR("%s: ilitek_roi_switch: info is Null\n",__func__);
		retval = -ENOMEM;
		return retval;
	}
	if (tskit_mstar_data->avoid_roi_switch_flag){
		TS_LOG_DEBUG("%s: avoid_roi_switch,do nothing\n",__func__);
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
	case TS_ACTION_WRITE:
		retval = mstar_set_roi_switch(info->roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, ilitek_set_roi_switch faild\n",
				   __func__);
		}
		break;
	case TS_ACTION_READ:
		info->roi_switch = tskit_mstar_data->roi_enabled;
		break;
	default:
		TS_LOG_INFO("%s: invalid roi switch(%d) action: %d\n",
			    __func__,info->roi_switch, info->op_action);
		retval = -EINVAL;
		break;
	}
	return retval;
}

static int mstar_glove_switch(struct ts_glove_info *info)
{
    int ret = NO_ERR;

    if (!info) {
        TS_LOG_ERR("%s:info is null\n", __func__);
        ret = -ENOMEM;
        return ret;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        ret = mstar_get_glove_switch(&info->glove_switch);
        if (ret) {
            TS_LOG_ERR("%s:get glove switch fail,ret=%d\n", __func__, ret);
            return ret;
        } else {
            TS_LOG_INFO("%s:glove switch=%d\n", __func__, info->glove_switch);
        }

        break;
    case TS_ACTION_WRITE:
        TS_LOG_INFO("%s:glove switch=%d\n", __func__, info->glove_switch);
        ret = mstar_set_glove_switch(! !info->glove_switch);
        if (ret) {
            TS_LOG_ERR("%s:set glove switch fail, ret=%d\n", __func__, ret);
            return ret;
        }

        break;
    default:
        TS_LOG_ERR("%s:invalid op action:%d\n", __func__, info->op_action);
        return -EINVAL;
    }

    return NO_ERR;
}

static int mstar_holster_switch(struct ts_holster_info *info)
{
    return NO_ERR;
}
static int mstar_set_holster_switch(u8 holster_switch)
{
	return NO_ERR;
}
static int mstar_set_info_flag(struct ts_kit_device_data *info)
{
    return NO_ERR;
}

static int mstar_read_flash_finale_28xx(void)
{
     int ret = NO_ERR;

    // set read done
    ret = mstar_set_reg_16bit(0x1606, 0x02);
    if (ret < 0){
        TS_LOG_ERR("%s set read done error\n", __func__);
        return ret;
    }
    // unset info flag
    ret = mstar_set_reg_low_byte(0x1607, 0x00);
    if (ret < 0){
        TS_LOG_ERR("%s unset info flag error\n", __func__);
        return ret;
    }
    // clear addr
    ret = mstar_set_reg_16bit(0x1600, 0x00);
    if (ret < 0){
        TS_LOG_ERR("%s clear addr error\n", __func__);
        return ret;
    }
    return ret;
}

static int mstar_read_flash_init_28xx(u16 cayenne_address, int nBlockType)
{
    int ret = 0;

    // 0x1608 0x20
    ret = mstar_set_reg_16bit(0x1608, 0x20);
    if (ret < 0){
        TS_LOG_ERR("%s set reg (0x1608, 0x20)  error\n", __func__);
        return ret;
    }
    //wriu 0x1606 0x20
    ret = mstar_set_reg_16bit(0x1606, 0x20);
    if (ret < 0){
        TS_LOG_ERR("%s set reg (0x1606, 0x20) error\n", __func__);
        return ret;
    }
    //wriu read done
    ret = mstar_set_reg_16bit(0x1606, 0x02);
    if (ret < 0){
        TS_LOG_ERR("%s set reg read done error\n", __func__);
        return ret;
    }
    //set address
    ret = mstar_set_reg_16bit(0x1600, cayenne_address);
    if (ret < 0){
        TS_LOG_ERR("%s set reg set address error\n", __func__);
        return ret;
    }
    if (nBlockType == EMEM_TYPE_INFO_BLOCK) {
        //set Info Block
        ret = mstar_set_reg_low_byte((uint) 0x1607, (uint) 0x08);
        if (ret < 0){
            TS_LOG_ERR("%s set info block error\n", __func__);
            return ret;
        }
        //set Info Double Buffer
        ret = mstar_set_reg_low_byte((uint) 0x1604, (uint) 0x01);
        if (ret < 0){
            TS_LOG_ERR("%s set info double buffer error\n", __func__);
            return ret;
         }
    } else {
        //set Main Block
        ret = mstar_set_reg_low_byte((uint) 0x1607, (uint) 0x00);
        if (ret < 0){
            TS_LOG_ERR("%s set main block error\n", __func__);
            return ret;
         }
        //set Main Double Buffer
        ret = mstar_set_reg_low_byte((uint) 0x1604, (uint) 0x01);
        if (ret < 0){
            TS_LOG_ERR("%s set main double buffer error\n", __func__);
            return ret;
         }
    }
    // set FPGA flag
    ret = mstar_set_reg_low_byte(0x1610, 0x01);
    if (ret < 0){
        TS_LOG_ERR("%s set fpga flag error\n", __func__);
        return ret;
    }
    // set mode trigger
    if (nBlockType == EMEM_TYPE_INFO_BLOCK) {
        // set eflash mode to read mode
        // set info flag
        ret = mstar_set_reg_16bit(0x1606, 0x0801);
        if (ret < 0){
            TS_LOG_ERR("%s set info flah error\n", __func__);
            return ret;
         }
    } else {
        // set eflash mode to read mode
        ret = mstar_set_reg_16bit(0x1606, 0x0001);
        if (ret < 0){
            TS_LOG_ERR("%s set eflash mode to read mode error\n", __func__);
            return ret;
         }
    }
    return ret;
}


static int mstar_read_flash_RIU_28xx(u32 nAddr, int nBlockType, int nLength, u8 * pFlashData)
{
    int ret = 0;
    uint read_16_addr_a = 0, read_16_addr_c = 0;

    TS_LOG_DEBUG("  %s() nAddr:0x%x \n", __func__, nAddr);

    //set read address
    ret = mstar_set_reg_16bit(0x1600, nAddr);
    if (ret < 0){
        TS_LOG_ERR("%s set read address error\n", __func__);
        return ret;
    }
    //read 16+16 bits
    read_16_addr_a = mstar_get_reg_16bit(0x160a);
    read_16_addr_c = mstar_get_reg_16bit(0x160c);

    pFlashData[0] = (u8) (read_16_addr_a & 0xff);
    pFlashData[1] = (u8) ((read_16_addr_a >> 8) & 0xff);
    pFlashData[2] = (u8) (read_16_addr_c & 0xff);
    pFlashData[3] = (u8) ((read_16_addr_c >> 8) & 0xff);
    return NO_ERR;
}


static int mstar_read_flash_28xx(u32 nAddr, int nBlockType, int nLength, u8 * pFlashData)
{
    int ret = NO_ERR;
    u16 _28xx_addr = nAddr / 4;
    u32 addr_star, addr_end, addr_step;
    u32 read_byte = 0;

    addr_star = nAddr;
    addr_end = nAddr + nLength;

    if ((addr_star >= EMEM_SIZE_MSG28XX) || (addr_end > EMEM_SIZE_MSG28XX)) {
        TS_LOG_DEBUG(" %s : addr_start = 0x%x , addr_end = 0x%x   \n", __func__, addr_star, addr_end);
        return -1;
    }

    addr_step = 4;

    ret = mstar_read_flash_init_28xx(_28xx_addr, nBlockType);
    if (ret < 0) {
        TS_LOG_ERR("%s 28XX read flash init error ret = %d\n", __func__, ret);
        return ret;
    }
    for (addr_star = nAddr; addr_star < addr_end; addr_star += addr_step) {
        _28xx_addr = addr_star / 4;

        TS_LOG_DEBUG("  %s() _28xx_addr:0x%x addr_star:0x%x addr_end:%x nLength:%d pFlashData:%p \n",
                 __func__, _28xx_addr, addr_star, addr_end, nLength, pFlashData);
        mstar_read_flash_RIU_28xx(_28xx_addr, nBlockType, nLength, (pFlashData + read_byte));
        TS_LOG_DEBUG("  %s() pFlashData[%x]: %02x %02x %02x %02x read_byte:%d \n", __func__, addr_star,
                 pFlashData[read_byte], pFlashData[read_byte + 1], pFlashData[read_byte + 2],
                 pFlashData[read_byte + 3], read_byte);

        read_byte += 4;
    }

    ret = mstar_read_flash_finale_28xx();
    if (ret < 0) {
        TS_LOG_ERR("%s 28XX read flash finale error ret = %d\n", __func__, ret);
    }
    return ret;
}


int mstar_mp_read_flash(u8 nChipType, u32 nAddr, int nBlockType, int nLength, u8 * pFlashData)
{
    int ret = 0;

    TS_LOG_DEBUG("  %s()  nChipType 0x%x \n", __func__, nChipType);

    switch (nChipType) {

    case CHIP_TYPE_MSG28XX:
    case CHIP_TYPE_MSG28XXA:
        ret = mstar_read_flash_28xx(nAddr, nBlockType, nLength, pFlashData);
        if (ret < 0) {
            TS_LOG_ERR("%s 28xx read flash error ret = %d\n", __func__, ret);
        }
        break;

    default:
        break;
    }

    return ret;
}

static void mstar_get_project_info(void)
{
    int ret = 0;
    u8 temp_buf[4] = {0};

    ret = mstar_dbbus_enter();
    if (ret < 0) {
        TS_LOG_ERR("%s enter dbbus error ret = %d\n", __func__, ret);
    }
    ret = mstar_mp_read_flash(tskit_mstar_data->chip_type, MSTAR_PROJECT_ID_ADDR, EMEM_TYPE_INFO_BLOCK, 12, (u8*)tskit_mstar_data->project_id);
    if (ret < 0) {
        TS_LOG_ERR("%s read project id error ret = %d\n", __func__, ret);
    }
    tskit_mstar_data->project_id[MSTAR_PROJECT_ID_LEN-1] = '\0';
    TS_LOG_INFO("%s: project id: %s\n",__func__, tskit_mstar_data->project_id);

    if (tskit_mstar_data->support_get_tp_color) {
        ret = mstar_mp_read_flash(tskit_mstar_data->chip_type, MSTAR_COLOR_ID_ADDR, EMEM_TYPE_INFO_BLOCK, 4, temp_buf);
        if (ret < 0) {
            TS_LOG_ERR("%s golden sample version error ret = %d\n", __func__, ret);
        }
        cypress_ts_kit_color[0] = temp_buf[3];
        TS_LOG_INFO("%s: color id: 0x%x,0x%x\n",__func__, temp_buf[2], temp_buf[3]);
    }

    ret = mstar_mp_read_flash(tskit_mstar_data->chip_type, MSTAR_GOLDEN_VER_ADDR, EMEM_TYPE_INFO_BLOCK, 4, temp_buf);
    if (ret < 0) {
        TS_LOG_ERR("%s golden sample version error ret = %d\n", __func__, ret);
    }
    TS_LOG_INFO("%s: golden sample version: 0x%x,0x%x\n", 
		 __func__,temp_buf[2],  temp_buf[3]);

    ret = mstar_dbbus_exit();
    if (ret < 0) {
        TS_LOG_ERR("%s mstar_dbbus_exit error ret = %d\n", __func__, ret);
    }
    tskit_mstar_data->vendor_id = mstar_get_swid(EMEM_INFO);
    mstar_dev_hw_reset();
    TS_LOG_INFO("%s: module vendor id: 0x%x\n",__func__, tskit_mstar_data->vendor_id);
}

static int mstar_before_suspend(void)
{
	tskit_mstar_data->avoid_roi_switch_flag = 1;
    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status) {
        tskit_mstar_data->esd_enable = FALSE;
    }

	// check whether update frimware is finished
    if ( tskit_mstar_data->fw_updating) {
        TS_LOG_ERR("%s: Not allow to power on/off touch ic while update firmware.\n",__func__);
        //goto out;
    }

    switch (tskit_mstar_data->mstar_chip_data->easy_wakeup_info.sleep_mode) {
    case TS_POWER_OFF_MODE:
        break;

    case TS_GESTURE_MODE:
        if (tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == true) {
            if (tskit_mstar_data->gesture_data.wakeup_mode[0] != 0x00000000 || tskit_mstar_data->gesture_data.wakeup_mode[1] != 0x00000000) {
                enable_irq_wake(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_id);
                mstar_enable_gesture_wakeup(&tskit_mstar_data->gesture_data.wakeup_mode[0]);
                mdelay(20);
		TS_LOG_INFO("%s: Finsih gesture command after 20ms\n",__func__);
                // disable incorrect read to avoid i2c error during gesture suspend
                enable_irq(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_id);
                g_disable_suspend_touch = 1;
		TS_LOG_DEBUG("%s: set suspend touch as %d\n",__func__, g_disable_suspend_touch);
            }
        }
        break;

    default:
        break;
    }
    return NO_ERR;
}

int mstar_suspend(void)
{
    int ret = 0;

    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status) {
        tskit_mstar_data->esd_enable = FALSE;
    }

	// check whether update frimware is finished
    if ( tskit_mstar_data->fw_updating) {
        TS_LOG_ERR("%s: Not allow to power on/off touch ic while update firmware.\n",__func__);
        goto out;
    }

    switch (tskit_mstar_data->mstar_chip_data->easy_wakeup_info.sleep_mode) {
    case TS_POWER_OFF_MODE:
        ret = mstar_enter_sleep_mode();
        if(ret){
            TS_LOG_ERR("%s enter sleep mode error\n", __func__);
        }
        TS_LOG_INFO("%s: suspend end case TS_POWER_OFF_MODE",__func__);
        break;

    case TS_GESTURE_MODE:
        if (tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value == true) {
            TS_LOG_INFO("%s: wakeup_gesture_enable_info.switch_value = true\n",__func__);
            if (tskit_mstar_data->gesture_data.wakeup_mode[0] != 0x00000000 || tskit_mstar_data->gesture_data.wakeup_mode[1] != 0x00000000) {
                g_disable_suspend_touch = 0;
		TS_LOG_DEBUG("%s: set suspend touch as %d\n",__func__, g_disable_suspend_touch);
                disable_irq_nosync(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_id);
		TS_LOG_DEBUG("%s: Disable INT\n",__func__);
            }
            tskit_mstar_data->mstar_chip_data->easy_wakeup_info.off_motion_on = true;
            TS_LOG_INFO("%s: suspend end case TS_GESTURE_MODE",__func__);
        } else {
            ret = mstar_enter_sleep_mode();
            if(ret){
                TS_LOG_ERR("%s enter sleep mode error\n", __func__);
            }
        }
        break;

    default:
        break;
    }

out:
    return NO_ERR;
}

static int mstar_status_resume(void)
{
	int retval = 0;
	struct ts_feature_info *info = &tskit_mstar_data->mstar_chip_data->ts_platform_data->feature_info;

    if(info->holster_info.holster_supported){
		retval = mstar_set_holster_switch(info->holster_info.holster_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to set holster switch(%d), err: %d\n",__func__, info->holster_info.holster_switch, retval);
		}
	}

	if (info->roi_info.roi_supported) {
		retval = mstar_set_roi_switch(info->roi_info.roi_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s, synaptics_set_roi_switch faild\n",
				   __func__);
		}
	}

      if(info->glove_info.glove_supported){
		retval = mstar_set_glove_switch(info->glove_info.glove_switch);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to set glove switch(%d), err: %d\n",
				   __func__, info->glove_info.glove_switch, retval);
		}
	}

	TS_LOG_INFO("%s: glove_switch (%d), holster switch(%d), roi_switch(%d) \n",__func__,
			   info->glove_info.glove_switch,info->holster_info.holster_switch,info->roi_info.roi_switch);

	return retval;
}

/*  do some thing after power on. */
int mstar_after_resume(void *feature_info)
{
    int ret = NO_ERR;

   // check whether update frimware is finished
    if ( tskit_mstar_data->fw_updating) {
        TS_LOG_INFO("%s: Not allow to power on/off touch ic while update firmware.\n",__func__);
        goto out;
    }

    switch (tskit_mstar_data->mstar_chip_data->easy_wakeup_info.sleep_mode) {
		case TS_POWER_OFF_MODE:
			TS_LOG_INFO("%s: TS_POWER_OFF_MODE, do nothing\n",__func__);
			break;

		case TS_GESTURE_MODE:
			if (tskit_mstar_data->gesture_data.wakeup_flag) {
				TS_LOG_INFO("%s: Gesture mode, chip_type = 0x%x\n",__func__, tskit_mstar_data->chip_type);
				g_disable_suspend_touch = 0;
				mstar_disable_gesture_wakeup();
			}
			break;
		default:
			TS_LOG_INFO("%s: Unknown mode\n",__func__);
			break;
    }

	mstar_apknode_function_control();

    if (tskit_mstar_data->apk_info->firmware_log_enable) {
        // mark this function call for avoiding device driver may spend longer time to resume from suspend state.
        mstar_restore_fw_to_log_data();
    }

	ret = mstar_status_resume();
	if(ret < 0){
		TS_LOG_ERR("%s: failed to resume status ret = %d\n",__func__, ret);
		return -EINVAL;
	}

    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status)
        tskit_mstar_data->esd_enable = TRUE;
out:
    return ret;
}

int mstar_resume(void)
{
    int ret = NO_ERR;

    mstar_exit_sleep_mode();
	tskit_mstar_data->avoid_roi_switch_flag = 0;
    switch (tskit_mstar_data->mstar_chip_data->easy_wakeup_info.sleep_mode) {
		case TS_POWER_OFF_MODE:
			TS_LOG_INFO("%s: TS_POWER_OFF_MODE, do nothing\n",__func__);
			break;
		case TS_GESTURE_MODE:
			TS_LOG_INFO("%s: Gesture mode\n",__func__);
			mstar_put_device_outof_easy_wakeup();
			break;
		default:
			TS_LOG_ERR("%s: no resume mode\n",__func__);
			break;
    }

    return NO_ERR;
}

static int mstar_wakeup_gesture_enable_switch(struct ts_wakeup_gesture_enable_info *info)
{
    return NO_ERR;
}

static int mstar_get_brightness_info(void)
{
    int error = NO_ERR;

    return error;
}

static int mstar_get_lcd_panel_info_from_lcdkit(void)
{
	char *lcd_type = NULL;

	lcd_type = trans_lcd_panel_name_to_tskit();
	if(!lcd_type){
		TS_LOG_ERR("%s: Get lcd panel type from lcdkit fail!\n", __func__);
		return -EINVAL ;
	}
	strncpy(tskit_mstar_data->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("lcd_panel_info = %s.\n", tskit_mstar_data->lcd_panel_info);
	return 0;
}

static int  get_lcd_module_name(void)
{
	char temp[LCD_PANEL_INFO_MAX_LEN] = {0};
	int i = 0;
	strncpy(temp, tskit_mstar_data->lcd_panel_info, LCD_PANEL_INFO_MAX_LEN-1);
	for(i=0;i<(LCD_PANEL_INFO_MAX_LEN -1) && (i < (MAX_STR_LEN -1));i++){
		if(temp[i] == '_'){
			break;
		}
		tskit_mstar_data->lcd_module_name[i] = tolower(temp[i]);
	}
	TS_LOG_INFO("lcd_module_name = %s.\n", tskit_mstar_data->lcd_module_name);
	return 0;
}

int mstar_fw_update_boot(char *file_name)
{
    int ret = NO_ERR;
    char fw_version[32] = {0};

    mstar_fw_update_swid_entry();
    ret = mstar_get_customer_fw_ver(fw_version);

    if(ret < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, ret);
		goto out;
    }

	sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
	snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);

out:
    return ret;
}

static int mstar_fw_update_sd(void)
{
    int ret = NO_ERR;
    char fw_version[32] = {0};

    ret = mstar_fw_update_sdcard(FIRMWARE_FILE_PATH_FOR_HW, 0);
    if (ret < 0) {
        TS_LOG_ERR("%s: Failed to update fw sd err: %d\n",__func__, ret);
		goto out;
    }

	ret = mstar_get_customer_fw_ver(fw_version);
	    if(ret < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, ret);
		goto out;
	}

	sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
	snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);

out:
    return ret;
}

int mstar_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
    struct algo_param *algo_p = NULL;
    struct ts_fingers *info = NULL;
    int ret = NO_ERR	;
    unsigned long nIrqFlag;
    algo_p = &out_cmd->cmd_param.pub_params.algo_param;
    info = &algo_p->info;

    out_cmd->command = TS_INPUT_ALGO;
	algo_p->algo_order = TS_ALGO_FUNC_0;

    if (!tskit_mstar_data->mp_testing) {
        mstar_read_touch_data(info);
    }
    return ret;
}

int mstar_chip_reset(void)
{
    mstar_dev_hw_reset();
    return 0;
}

static int mstar_i2c_communicate_check(struct ts_kit_platform_data *mstar_pdata)
{
    int i = 0;
    int ret = NO_ERR;
    u8 cmd = 0;

    for (i = 0; i < MSTAR_DETECT_I2C_RETRY_TIMES; i++) {
        cmd = 0x37;
        ret = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &cmd, 1);
        if (ret < 0) {
            TS_LOG_ERR("%s:mstar chip id read fail, ret=%d, i=%d\n", __func__, ret, i);
            msleep(50);
        } else {
            TS_LOG_INFO("%s:mstar chip id read pass\n", __func__);
            return NO_ERR;
        }
    }

    return ret;
}

static int mstar_regulator_get(void)
{
	if(tskit_mstar_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			tskit_mstar_data->vdda = regulator_get(&tskit_mstar_data->mstar_dev->dev,"mstar_vci");
			if (IS_ERR(tskit_mstar_data->vdda)) {
				TS_LOG_ERR("%s, regulator tp vdda not used\n",__func__);
				return -EINVAL;
			}
		}

		if (1 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
			tskit_mstar_data->vddd = regulator_get(&tskit_mstar_data->mstar_dev->dev,"mstar_vddio");
			if (IS_ERR(tskit_mstar_data->vddd)) {
				TS_LOG_ERR("%s, regulator tp vddd not used\n",__func__);
				goto ts_vddio_out;
			}
		}
	} else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	return NO_ERR;

ts_vddio_out:
	if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type)
		regulator_put(tskit_mstar_data->vdda);
	return -EINVAL;
}

static void mstar_regulator_put(void)
{
	if(tskit_mstar_data->self_ctrl_power == 1){
		if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vdda)) {
				regulator_put(tskit_mstar_data->vdda);
			}
		}
		if (1 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vddd)) {
				regulator_put(tskit_mstar_data->vddd);
			}
		}
	}
}


static int mstar_set_voltage(void)
{
	int rc = NO_ERR;

	if(tskit_mstar_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		if(1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type){
			/*set voltage for vddd and vdda*/
			rc = regulator_set_voltage(tskit_mstar_data->vdda, tskit_mstar_data->mstar_chip_data->regulator_ctr.vci_value, tskit_mstar_data->mstar_chip_data->regulator_ctr.vci_value);
			if(rc < 0){
				TS_LOG_ERR("%s, failed to set parade vdda to %dV, rc = %d\n", __func__, rc,tskit_mstar_data->mstar_chip_data->regulator_ctr.vci_value);
				return rc;
			}
		}

		if (1 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type){ 
			rc = regulator_set_voltage(tskit_mstar_data->vddd, tskit_mstar_data->mstar_chip_data->regulator_ctr.vddio_value, tskit_mstar_data->mstar_chip_data->regulator_ctr.vddio_value);
			if(rc < 0){
				TS_LOG_ERR("%s, failed to set parade vddd to %dV, rc = %d\n", __func__, rc,tskit_mstar_data->mstar_chip_data->regulator_ctr.vddio_value);
				return rc;
			}
		}
	} else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}

	return rc;
}

static int mstar_gpio_request(void)
{
	int retval = NO_ERR;

	if (0 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
		retval = gpio_request(tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl, "parade_vci_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR
			    ("SFT:Ok;  ASIC: Real ERR----unable to request vci_gpio_ctrl2:%d\n",
			     tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl);
			return -1;
		}
	}
	if (0 ==  tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
		retval = gpio_request(tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl, "parade_vddio_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR
			    ("SFT:Ok;  ASIC: Real ERR----unable to request vddio_gpio_ctrl:%d\n",
			     tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl);
			goto ts_vddio_out;
		}
	}
	return retval;
	
ts_vddio_out:
	if (0 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) 
		gpio_free(tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl);	
	return -1;
}

static void mstar_gpio_free(void)
{
	/*0 is power supplied by gpio, 1 is power supplied by ldo */
	if (0 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
		if(0 <= tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl)
		gpio_free(tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl);
	}
	if (0 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
		if (0 <= tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl)
		gpio_free(tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl);
	}
}

static void mstar_power_on_gpio_set(void)
{
	gpio_direction_input(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_gpio);
	gpio_direction_output(tskit_mstar_data->mstar_chip_data->ts_platform_data->reset_gpio, 1);
}

static int mstar_power_on(void)
{
	int rc = NO_ERR;

	if(tskit_mstar_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		/*set voltage for vddd and vdda 0 - gpio control  1 - ldo  2 - boost & gpio*/
		if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vdda)) {
				TS_LOG_INFO("%s: vdda enable is called\n",__func__);
				rc = regulator_enable(tskit_mstar_data->vdda);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to enable parade vdda, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		} else if(0 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			TS_LOG_INFO("%s vdda switch gpio on\n", __func__);
			gpio_direction_output(tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl, 1);//TODO: need related with dts
		}

		if (1 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vddd)) {
				TS_LOG_INFO("vdda enable is called\n");
				rc = regulator_enable(tskit_mstar_data->vddd);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to enable parade vddd, rc = %d\n", __func__, rc);
					goto ts_vddd_out;
				}
			}
		}
		if(0 == tskit_mstar_data->mstar_chip_data->vddio_gpio_type) {
			TS_LOG_INFO("%s vddd switch gpio on\n", __func__);
			gpio_direction_output(tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl, 1);//TODO: need related with dts
		}
	} else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	mstar_power_on_gpio_set();
	return rc;

ts_vddd_out:
	if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type)
		regulator_disable(tskit_mstar_data->vdda);
	return -1;
}

static int mstar_power_off(void)
{
	int rc = NO_ERR;

	if(tskit_mstar_data->self_ctrl_power == 1){
		TS_LOG_INFO("%s, power control by touch ic\n",__func__);
		/*set voltage for vddd and vdda*/
		if (1 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vdda)) {
				TS_LOG_INFO("vdda disable is called\n");
				rc = regulator_disable(tskit_mstar_data->vdda);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to disable parade vdda, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		}
		else if(0 == tskit_mstar_data->mstar_chip_data->vci_regulator_type) {
			TS_LOG_INFO("%s vdda switch gpio off\n", __func__);
			gpio_direction_output(tskit_mstar_data->mstar_chip_data->vci_gpio_ctrl, 0);//TODO: need related with dts
		}

		if (1 == tskit_mstar_data->mstar_chip_data->vddio_regulator_type) {
			if (!IS_ERR(tskit_mstar_data->vddd)) {
				TS_LOG_INFO("vdda disable is called\n");
				rc = regulator_disable(tskit_mstar_data->vddd);
				if (rc < 0) {
					TS_LOG_ERR("%s, failed to disable parade vddd, rc = %d\n", __func__, rc);
					return -1;
				}
			}
		}
		if(0 == tskit_mstar_data->mstar_chip_data->vddio_gpio_type) {
			TS_LOG_INFO("%s vddd switch gpio off\n", __func__);
			gpio_direction_output(tskit_mstar_data->mstar_chip_data->vddio_gpio_ctrl, 0);//TODO: need related with dts
		}
	}
	else{
		TS_LOG_INFO("%s, power control by LCD, nothing to do\n",__func__);
	}
	return rc;
}

static int mstar_pinctrl_init(void)
{
	int ret = NO_ERR;

	tskit_mstar_data->pctrl= devm_pinctrl_get(&tskit_mstar_data->mstar_dev->dev);
	if (IS_ERR(tskit_mstar_data->pctrl)) {
		TS_LOG_ERR("%s: failed to devm pinctrl get\n",__func__);
		ret = -EINVAL;
		return ret;
	}

	tskit_mstar_data->pins_default =
	    pinctrl_lookup_state(tskit_mstar_data->pctrl, "default");
	if (IS_ERR(tskit_mstar_data->pins_default)) {
		TS_LOG_ERR("%s: failed to pinctrl lookup state default\n",__func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	tskit_mstar_data->pins_idle = pinctrl_lookup_state(tskit_mstar_data->pctrl, "idle");
	if (IS_ERR(tskit_mstar_data->pins_idle)) {
		TS_LOG_ERR("%s: failed to pinctrl lookup state idle\n",__func__);
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return ret;

err_pinctrl_put:
	devm_pinctrl_put(tskit_mstar_data->pctrl);
	return ret;
}

static void mstar_pinctrl_release(void)
{
	if (tskit_mstar_data->pctrl){
		devm_pinctrl_put(tskit_mstar_data->pctrl);
	}
}

static int mstar_pinctrl_select_normal(void)
{
	int retval = NO_ERR;

	if (tskit_mstar_data->pctrl == NULL || tskit_mstar_data->pins_default == NULL) {
		TS_LOG_ERR("%s: pctrl or pins_default is NULL.\n", __func__);
		return -1;
	}

	retval = pinctrl_select_state(tskit_mstar_data->pctrl, tskit_mstar_data->pins_default);
	if (retval < 0) {
		TS_LOG_ERR("%s: set pinctrl normal error.\n", __func__);
	}

	return retval;
}

static int mstar_chip_detect(struct ts_kit_platform_data *platform_data)
{
    int rc = NO_ERR;

    if ((!platform_data) && (!platform_data->ts_dev)) {
        TS_LOG_ERR("%s device, platform_data&&ts_dev is NULL \n", __func__);
        rc = -ENOMEM;
        goto exit;
    }
   
    tskit_mstar_data->mstar_chip_data->ts_platform_data = platform_data;
    tskit_mstar_data->mstar_dev = platform_data->ts_dev;
    tskit_mstar_data->mstar_dev->dev.of_node = platform_data->chip_data->cnode ;

    tskit_mstar_data->dev = &(platform_data->client->dev);
    tskit_mstar_data->mstar_chip_data->is_i2c_one_byte = 0;
    tskit_mstar_data->mstar_chip_data->is_new_oem_structure = 0;
    tskit_mstar_data->mstar_chip_data->is_parade_solution = 0;

    rc = mstar_parse_dts(tskit_mstar_data->mstar_dev->dev.of_node, tskit_mstar_data->mstar_chip_data);
    if(rc != 0) {
        TS_LOG_ERR("%s, error mstar_parse_dts\n", __func__);
        goto mstar_dts_failed;
    }

    rc = mstar_pinctrl_init();
    if (rc < 0) {
        TS_LOG_ERR("%s: mstar_pinctrl_init error\n", __func__);
    }

	rc = mstar_regulator_get();
	if(rc != 0) {
		TS_LOG_ERR("%s, error request power vdda vddd\n", __func__);
		goto regulator_err;
	}

	rc = mstar_gpio_request();
	if(rc != 0) {
		TS_LOG_ERR("%s, error request gpio for vci and vddio\n", __func__);
		goto gpio_err;
	}

    if(tskit_mstar_data->pinctrl_set == 1) {
        rc = mstar_pinctrl_select_normal();
        if(rc < 0){
            TS_LOG_ERR("%s, mstar_pinctrl_select_normal failed\n", __func__); 
			goto pinctrl_get_err;
        }
    }

    mstar_mutex_init();
    gpio_direction_output(tskit_mstar_data->mstar_chip_data->ts_platform_data->reset_gpio, 0);
    gpio_direction_output(tskit_mstar_data->mstar_chip_data->ts_platform_data->irq_gpio, 0);
    msleep(50);

 	rc = mstar_set_voltage();
	if (rc < 0) {
		TS_LOG_ERR("%s, failed to set voltage, rc = %d\n", __func__, rc);
		goto pinctrl_select_error;
	}
	rc = mstar_power_on();
	if (rc < 0) {
		TS_LOG_ERR("%s, failed to enable power, rc = %d\n", __func__, rc);
		goto pinctrl_select_error;
	}

    rc = mstar_chip_reset();
    if (rc) {
        TS_LOG_ERR("%s:hardware reset fail, rc=%d\n", __func__, rc);
        goto exit_free_power;
    }

    rc = mstar_i2c_communicate_check(platform_data);
    if (rc < 0) {
        TS_LOG_ERR("%s:not find mstar device, rc=%d\n", __func__, rc);
        goto exit_free_power;
    } else {
        TS_LOG_INFO("%s:find mstar device\n", __func__);

        strncpy(tskit_mstar_data->mstar_chip_data->chip_name, MSTAR_CHIP_NAME, strlen(MSTAR_CHIP_NAME)+1);
    }

    TS_LOG_INFO("%s:mstar chip detect success\n", __func__);


    return NO_ERR;

exit_free_power:
    mstar_power_off();
pinctrl_select_error:
pinctrl_get_err:
	mstar_gpio_free();
gpio_err:
	mstar_regulator_put();
regulator_err:
mstar_dts_failed:
    mstar_pinctrl_release();
    if(tskit_mstar_data->mstar_chip_data) {
        kfree(tskit_mstar_data->mstar_chip_data);
        tskit_mstar_data->mstar_chip_data = NULL;
    }

    if (tskit_mstar_data) {
        kfree(tskit_mstar_data);
        tskit_mstar_data = NULL;
    }
exit:
    TS_LOG_INFO("%s:mstar chip detect fail\n", __func__);
    return rc;
}

static int mstar_param_init(void)
{
    int ret = NO_ERR;
    char vendor_name[MSTAR_VENDOR_NAME_LEN] = {0};
    char fw_version[32] = {0};

    mstar_get_project_info();

    ret = mstar_get_vendor_name_from_dts(tskit_mstar_data->project_id, vendor_name, MSTAR_VENDOR_NAME_LEN);
    if (ret) {
        TS_LOG_ERR("%s:read vendor name fail by project id\n", __func__);
        if (MSTAR_VENDOR_ID_HLT == tskit_mstar_data->vendor_id) {
            strncpy(vendor_name, MSTAR_VENDOR_NAME_HLT, MSTAR_VENDOR_NAME_LEN);
        } else {
            TS_LOG_ERR("%s:read vendor name fail, ret=%d\n", __func__, ret);
        }
    }
    else{
        strncpy(tskit_mstar_data->mstar_chip_data->module_name, vendor_name, MAX_STR_LEN-1);
    }
    ret = mstar_get_platform_fw_ver(&tskit_mstar_data->fw_data.platform_inter_ver);
    if(ret < 0){
		TS_LOG_ERR("%s:read platform fw ver fail, ret=%d\n", __func__, ret);
    }
    ret = mstar_get_customer_fw_ver(fw_version);
    if(ret < 0){
		TS_LOG_ERR("%s:read fw_ver fail, ret=%d\n", __func__, ret);
    }else{
		sprintf(tskit_mstar_data->fw_data.cust_ver,"%s",fw_version);
		snprintf(tskit_mstar_data->mstar_chip_data->version_name,MAX_STR_LEN-1,"%s",fw_version);
    }

    if(tskit_mstar_data->lcd_panel_name_from_lcdkit){
        ret = mstar_get_lcd_panel_info_from_lcdkit();
        if(ret < 0){
            TS_LOG_ERR("%s: get lcd panel info from lcdkit fail!\n ",__func__);
        }
    }
    if(tskit_mstar_data->fw_only_depend_on_lcd){
        ret = get_lcd_module_name();
        if(ret < 0){
            TS_LOG_ERR("%s: get lcd module name fail!\n ",__func__);
        }
    }
    return ret;
}

static int mstar_init_chip(void)
{
    int ret = 0;

    ret = mstar_chip_detect_init();
    if (ret == -ENODEV) {
        mstar_unregister_device();
        goto out;
    }

    tskit_mstar_data->mstar_chip_data->is_in_cell = false;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.sleep_mode = TS_POWER_OFF_MODE;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_gesture = false;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.easy_wakeup_flag = false;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.palm_cover_flag = false;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.palm_cover_control = false;
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.off_motion_on = false;
    tskit_mstar_data->esd_enable = TRUE;
    tskit_mstar_data->finger_touch_disable = FALSE;
	tskit_mstar_data->ges_self_debug = false;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE  // support at most 64 types of gesture wakeup mode
	tskit_mstar_data->gesture_data.wakeup_mode[0] = 0xFFFFFFFF;
	tskit_mstar_data->gesture_data.wakeup_mode[1] = 0xFFFFFFFF ;
#else // support at most 16 types of gesture wakeup mode
	tskit_mstar_data->gesture_data.wakeup_mode[0] = 0x0000FFFF;
	tskit_mstar_data->gesture_data.wakeup_mode[1] = 0x00000000;
#endif
    tskit_mstar_data->int_flag = TRUE;
    tskit_mstar_data->fw_update_data.update_retry_cont = UPDATE_FIRMWARE_RETRY_COUNT;
    ret = mstar_param_init();
    if (ret) {
        TS_LOG_ERR("%s:init param fail, ret=%d\n", __func__, ret);
        goto out;
    }

    TS_LOG_INFO("%s:init chip success.\n", __func__);
    return NO_ERR;

out:
    TS_LOG_ERR("%s: init chip error.\n", __func__);
    return ret;
}

static void mstar_shutdown(void)
{
    return;
}

static int mstar_input_config(struct input_dev *input_dev)
{
    tskit_mstar_data->input = input_dev;
    set_bit(EV_SYN, input_dev->evbit);
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(EV_ABS, input_dev->evbit);
    set_bit(BTN_TOUCH, input_dev->keybit);
    set_bit(BTN_TOOL_FINGER, input_dev->keybit);
    set_bit(TS_PALM_COVERED, input_dev->keybit);
    set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
    set_bit(TS_LETTER_c, input_dev->keybit);
    set_bit(TS_LETTER_e, input_dev->keybit);
    set_bit(TS_LETTER_m, input_dev->keybit);
    set_bit(TS_LETTER_w, input_dev->keybit);
    set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, tskit_mstar_data->mstar_chip_data->x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, tskit_mstar_data->mstar_chip_data->y_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

#if TYPE_B_PROTOCOL
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 0))
	input_mt_init_slots(input_dev, 10, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, 10);
#endif
#endif

#ifdef REPORT_2D_W
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif
    return NO_ERR;
}

static int mstar_wrong_touch(void)
{
    int rc = NO_ERR;

    mutex_lock(&tskit_mstar_data->mutex_common);
    tskit_mstar_data->mstar_chip_data->easy_wakeup_info.off_motion_on = true;
    mutex_unlock(&tskit_mstar_data->mutex_common);

    return rc;
}

static int mstar_irq_top_half(struct ts_cmd_node *cmd)
{
    cmd->command = TS_INT_PROCESS;
    tskit_mstar_data->esd_check = FALSE;
    return NO_ERR;
}

static int mstar_chip_get_info(struct ts_chip_info_param *info)
{
    sprintf(info->fw_vendor, "%s", tskit_mstar_data->fw_data.cust_ver);
    strncpy(info->ic_vendor, MSTAR_CHIP_NAME, CHIP_INFO_LENGTH * 2);
    strncpy(info->mod_vendor, tskit_mstar_data->mstar_chip_data->module_name, CHIP_INFO_LENGTH);

    return NO_ERR;
}

int mstar_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
    int ret = NO_ERR;
    struct ts_kit_device_data *dev_data = NULL;

    dev_data = tskit_mstar_data->mstar_chip_data;
    if (!info) {
        TS_LOG_ERR("%s:info is Null\n", __func__);
        return -ENOMEM;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        memcpy(info->tp_test_type, dev_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
        TS_LOG_INFO("%s:test_type=%s\n", __func__, info->tp_test_type);
        break;
    case TS_ACTION_WRITE:
        break;
    default:
        TS_LOG_ERR("%s:invalid op action:%d\n", __func__, info->op_action);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int mstar_reset_device(void)
{
    mstar_dev_hw_reset();
    return NO_ERR;
}

static int mstar_esdcheck_func(void)
{
    if (tskit_mstar_data->mstar_chip_data->need_wd_check_status)
        mstar_esd_check();
    return NO_ERR;
}

int mstar_charger_switch(struct ts_charger_info *info)
{
	return NO_ERR;
}

struct ts_device_ops ts_mstar_ops = {
    .chip_detect = mstar_chip_detect,
    .chip_init = mstar_init_chip,
    .chip_get_brightness_info = mstar_get_brightness_info,
    .chip_input_config = mstar_input_config,
    .chip_irq_top_half = mstar_irq_top_half,
    .chip_irq_bottom_half = mstar_irq_bottom_half,
    .chip_fw_update_boot = mstar_fw_update_boot,
    .chip_fw_update_sd = mstar_fw_update_sd,
    .chip_get_info = mstar_chip_get_info,
    .chip_get_capacitance_test_type = mstar_chip_get_capacitance_test_type,
    .chip_set_info_flag = mstar_set_info_flag,
    .chip_before_suspend = mstar_before_suspend,
    .chip_suspend = mstar_suspend,
    .chip_resume = mstar_resume,
    .chip_after_resume = mstar_after_resume,
    .chip_wakeup_gesture_enable_switch = mstar_wakeup_gesture_enable_switch,
    .chip_get_rawdata = mstar_get_raw_data,
#ifdef MSTAR_ROI_ENABLE
    .chip_roi_rawdata = mstar_roi_rawdata,
    .chip_roi_switch = mstar_roi_switch,
#endif
    .chip_glove_switch = mstar_glove_switch,
    .chip_shutdown = mstar_shutdown,
    .chip_holster_switch = mstar_holster_switch,
    .chip_reset = mstar_reset_device,
    .chip_check_status = mstar_esdcheck_func,
    .chip_charger_switch = mstar_charger_switch,
    .chip_wrong_touch = mstar_wrong_touch,
};

static int __init mstar_ts_module_init(void)
{
    int ret = NO_ERR;
    bool found = false;
    struct device_node *child = NULL;
    struct device_node *root = NULL;

    tskit_mstar_data = kzalloc(sizeof(struct mstar_core_data), GFP_KERNEL);
    if (NULL == tskit_mstar_data) {
        TS_LOG_ERR("%s:alloc mem for device data fail\n", __func__);
        ret = -ENOMEM;
        return ret;
    }
    tskit_mstar_data->mstar_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!tskit_mstar_data->mstar_chip_data ) {
    	TS_LOG_ERR("Failed to alloc mem for struct synaptics_chip_data\n");
       ret =  -ENOMEM;
       goto error_exit;
    }

    root = of_find_compatible_node(NULL, NULL, HUAWEI_TS_KIT);
    if (!root) {
        TS_LOG_ERR("%s:find_compatible_node error\n", __func__);
        ret = -EINVAL;
        goto error_exit;
    }

    for_each_child_of_node(root, child) {
        if (of_device_is_compatible(child, MSTAR_CHIP_NAME)) {
            found = true;
            break;
        }
    }

    if (!found) {
        TS_LOG_ERR("%s:device tree node not found, name=%s\n", __func__, MSTAR_CHIP_NAME);
        ret = -EINVAL;
        goto error_exit;
    }

    tskit_mstar_data->mstar_chip_data->cnode = child;
    tskit_mstar_data->mstar_chip_data->ops = &ts_mstar_ops;
    ret = huawei_ts_chip_register(tskit_mstar_data->mstar_chip_data);
    if (ret) {
        TS_LOG_ERR("%s:chip register fail, ret=%d\n", __func__, ret);
        goto error_exit;
    }

    TS_LOG_INFO("%s: load driver success\n", __func__);
    return NO_ERR;

error_exit:
    if(tskit_mstar_data->mstar_chip_data) {
		kfree(tskit_mstar_data->mstar_chip_data);
		tskit_mstar_data->mstar_chip_data = NULL;/*Fix the DTS parse error cause panic bug*/
    }

    if (tskit_mstar_data) {
		kfree(tskit_mstar_data);
		tskit_mstar_data = NULL;
    }
    TS_LOG_ERR("%s: load driver fail\n", __func__);
    return ret;
}

static void __exit mstar_ts_module_exit(void)
{
    return;
}

late_initcall(mstar_ts_module_init);
module_exit(mstar_ts_module_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
