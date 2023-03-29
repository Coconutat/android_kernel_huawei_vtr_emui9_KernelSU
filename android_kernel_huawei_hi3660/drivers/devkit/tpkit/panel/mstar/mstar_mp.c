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
 * Author: Dicky Chiang
 * Maintain: Luca Hsu, Tigers Huang
 */

#include "mstar_mp.h"
#include "mstar_common.h"

#define RUN_OPEN_TEST   0
#define RUN_SHORT_TEST  1

#define PARSER_MAX_CFG_BUF          (512*2)
#define PARSER_MAX_KEY_NUM          (600*2)

#define PARSER_MAX_KEY_NAME_LEN     100
#define PARSER_MAX_KEY_VALUE_LEN    3000

#define M_CFG_SSL                                 '['
#define M_CFG_SSR                                 ']'
#define M_CFG_NIS                                 ':'
#define M_CFG_NTS                                 '#'
#define M_CFG_EQS                                 '='

int run_type = 0;
typedef struct _ST_INI_FILE_DATA
{
    char pSectionName[PARSER_MAX_KEY_NAME_LEN];
    char pKeyName[PARSER_MAX_KEY_NAME_LEN];
    char pKeyValue[PARSER_MAX_KEY_VALUE_LEN];
    int iSectionNameLen;
    int iKeyNameLen;
    int iKeyValueLen;
}ST_INI_FILE_DATA;

static ST_INI_FILE_DATA * ms_ini_file_data = NULL;
//ST_INI_FILE_DATA ms_ini_file_data[PARSER_MAX_KEY_NUM];

struct tp_mp_cmd
{
    int addr;
    int data;
};


static MutualMpTest_t *mp_test_data;
static MutualMpTestResult_t *mp_test_result;
static struct mp_main_func *p_mp_func;

static void mp_kfree(void **mem) {
    if(*mem != NULL) {
        kfree(*mem);
        *mem = NULL;
    }
}

static u8 check_thrs(s32 nValue, s32 nMax, s32 nMin) {
    return ((nValue <= nMax && nValue >= nMin) ? 1 : 0);
}

void mp_dump_data(void *pBuf, u16 nLen, int nDataType, int nCarry, int nChangeLine, const char *desp, int level)
{
    int i;
    char szStrTmp[10] = {0};
    char szStrBuf[512] = {0};
    u8 * pU8Buf = NULL;
    s8 * pS8Buf = NULL;
    u16 * pU16Buf = NULL;
    s16 * pS16Buf = NULL;
    u32 * pU32Buf = NULL;
    s32 * pS32Buf = NULL;

    if(nDataType == 8)
        pU8Buf = (u8 *)pBuf;
    else if(nDataType == -8)
        pS8Buf = (s8 *)pBuf;
    else if(nDataType == 16)
        pU16Buf = (u16 *)pBuf;
    else if(nDataType == -16)
        pS16Buf = (s16 *)pBuf;
    else if(nDataType == 32)
        pU32Buf = (u32 *)pBuf;
    else if(nDataType == -32)
        pS32Buf = (s32 *)pBuf;

    TS_LOG_DEBUG("%s: Dump %s data :\n", __func__, desp);

    for(i=0; i < nLen; i++)
    {
        if(nCarry == 16)
        {
            if(nDataType == 8)
                snprintf(szStrTmp,sizeof(szStrTmp), "%02X ", pU8Buf[i]);
            else if(nDataType == -8)
                snprintf(szStrTmp,sizeof(szStrTmp), "%02X ", pS8Buf[i]);
            else if(nDataType == 16)
                snprintf(szStrTmp,sizeof(szStrTmp), "%04X ", pU16Buf[i]);
            else if(nDataType == -16)
                snprintf(szStrTmp,sizeof(szStrTmp), "%04X ", pS16Buf[i]);
            else if(nDataType == 32)
                snprintf(szStrTmp,sizeof(szStrTmp), "%08X ", pU32Buf[i]);
            else if(nDataType == -32)
                snprintf(szStrTmp,sizeof(szStrTmp), "%08X ", pS32Buf[i]);
        }
        else if(nCarry == 10)
        {
            if(nDataType == 8)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6u ", pU8Buf[i]);
            else if(nDataType == -8)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6d ", pS8Buf[i]);
            else if(nDataType == 16)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6u ", pU16Buf[i]);
            else if(nDataType == -16)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6d ", pS16Buf[i]);
            else if(nDataType == 32)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6u ", pU32Buf[i]);
            else if(nDataType == -32)
                snprintf(szStrTmp,sizeof(szStrTmp), "%6d ", pS32Buf[i]);
        }

        strcat(szStrBuf, szStrTmp);
        memset(szStrTmp, 0, 10);
        if(i%nChangeLine==nChangeLine-1){
	    if(level == 0)
               TS_LOG_DEBUG(KERN_CONT"%s\n", szStrBuf);
	    else if(level == 1)
		TS_LOG_INFO("%s\n", szStrBuf);
            memset(szStrBuf, 0, 512);
        }
    }
}

int msg28xx_check_mp_switch(void)
{
    u32 nRegData = 0;
    int nTimeOut = 280;
    int nT = 0;

    do {
        nRegData = mstar_get_reg_16bit(0x1402);
        msleep(15);
        nT++;
        if (nT > nTimeOut) {
            return -1;
        }
        TS_LOG_DEBUG("%s: nT = %d, nRegData = 0x%04x\n",__func__, nT, nRegData);
    } while (nRegData != 0x7447);

    return 0;
}

int msg30xx_check_mp_switch(void)
{
    u16 nRegData = 0;
    int nTimeOut = 280;
    int nT = 0;

    do {
        nRegData = mstar_get_reg_16bit_by_addr(0x1401, ADDRESS_MODE_16BIT);
        mdelay(15);
        nT++;
        if (nT > nTimeOut) {
            return -1;
        }
        TS_LOG_DEBUG("%s: nT = %d, nRegData = 0x%04x\n",__func__, nT, nRegData);
    } while (nRegData != 0x7447);

    return 0;
}

int msg28xx_enter_mp_mode(void)
{
    int i, rc = NO_ERR;
    struct tp_mp_cmd tmc[5] = {0};

    tmc[0].addr = 0x3C60;
    tmc[0].data = 0xAA55;

    tmc[1].addr = 0x1402;
    tmc[1].data = 0x7474;

    tmc[2].addr = 0x1E06;
    tmc[2].data = 0x0000;

    tmc[3].addr = 0x1E06;
    tmc[3].data = 0x0001;

    rc = mstar_mp_stop_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_mp_stop_mcu error\n", __func__);
        return rc;
    }
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
		mdelay(10);
	}
	else{
		mdelay(100);
	}

    for(i = 0; i < ARRAY_SIZE(tmc); i++) {
        rc = mstar_set_reg_16bit(tmc[i].addr, tmc[i].data);
        if (rc < 0){
            TS_LOG_ERR("%s mstar_set_reg_16bit(0x%x, 0x%x) error\n",
                        __func__, tmc[i].addr, tmc[i].data);
            return rc;
        }
    }

    rc = mstar_mp_start_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_mp_start_mcu error\n", __func__);
        return rc;
    }
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG28XXA){
		mdelay(300);
	}

    return 0;
}

int msg30xx_enter_mp_mode(void)
{
    int i, rc = NO_ERR;
    struct tp_mp_cmd tmc[4] = {0};

    tmc[0].addr = 0x3C60;
    tmc[0].data = 0xAA55;

    tmc[1].addr = 0x1402;
    tmc[1].data = 0x7474;

    tmc[2].addr = 0x1E06;
    tmc[2].data = 0x0000;

    tmc[3].addr = 0x1E06;
    tmc[3].data = 0x0001;

    rc = mstar_mp_stop_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_mp_stop_mcu error\n", __func__);
        return rc;
    }

	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
		mdelay(10);
	}
	else{
		mdelay(100);
	}

    for(i = 0; i < ARRAY_SIZE(tmc); i++) {
        rc = mstar_set_reg_16bit(tmc[i].addr, tmc[i].data);
        if (rc < 0){
            TS_LOG_ERR("%s mstar_set_reg_16bit(0x%x, 0x%x) error\n",
                        __func__, tmc[i].addr, tmc[i].data);
            return rc;
        }
    }

    rc = mstar_mp_start_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_mp_start_mcu() error\n", __func__);
        return rc;
    }
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG28XXA){
		mdelay(300);
	}

    return 0;
}

int msg30xx_dbbus_set_default(void)
{
    int i, rc = NO_ERR;
    struct tp_mp_cmd tmc[8] = {0};

    tmc[0].data = 0x7F;
    tmc[1].data = 0x51;
    tmc[2].data = 0x81;
    tmc[3].data = 0x83;
    tmc[4].data = 0x84;
    tmc[5].data = 0x35;
    tmc[6].data = 0x7E;
    tmc[7].data = 0x34;

    rc = mstar_set_reg_low_byte(0, 0);
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_set_reg_low_byte error\n", __func__);
        return rc;
    }

    for(i = 0; i < ARRAY_SIZE(tmc); i++) {
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &tmc[i].data, 1);
        if (rc < 0) {
            TS_LOG_ERR("%s write 0x%x error\n", __func__, tmc[i].data);
            return rc;
        }
    }

    return rc;
}

int msg30xx_dbbus_read_dqmen_start(void)
{
    int i, rc = NO_ERR;
    struct tp_mp_cmd tmc[6] = {0};

    tmc[0].data = 0x7F;
    tmc[1].data = 0x52;
    tmc[2].data = 0x80;
    tmc[3].data = 0x82;
    tmc[4].data = 0x85;
    tmc[5].data = 0x35;

    rc = mstar_set_reg_16bit_off(0x0F50, BIT1);
    if (rc < 0) {
        TS_LOG_ERR("%s write mstar_set_reg_16bit_off error\n", __func__);
        return rc;
    }

    for(i = 0; i < ARRAY_SIZE(tmc); i++) {
        rc = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &tmc[i].data, 1);
        if (rc < 0) {
            TS_LOG_ERR("%s write 0x%x error\n", __func__, tmc[i].data);
            return rc;
        }
    }

    return rc;
}

int msg30xx_dbbus_read_dqmem_end(void)
{
    int rc = NO_ERR;

    rc = msg30xx_dbbus_set_default();
    if (rc < 0) {
        TS_LOG_ERR("%s msg30xx_dbbus_set_default error\n", __func__);
        return rc;
    }

    rc = mstar_set_reg_16bit_on(0x0F50, BIT1);
    if (rc < 0) {
        TS_LOG_ERR("%s mstar_set_reg_16bit_on(0x0F50, BIT1) error\n", __func__);
        return rc;
    }
    return rc;
}


int mstar_dbbus_exit(void)
{
    int rc = NO_ERR;

    rc = mstar_dbbus_iic_not_use_bus();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_iic_not_use_bus error\n", __func__);
        return rc;
    }

    rc = mstar_dbbus_not_stop_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_not_stop_mcu error\n", __func__);
        return rc;
    }

    rc = mstar_dbbus_exit_serial_debug();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_exit_serial_debug error\n", __func__);
        return rc;
    }

    return rc;
}

int mstar_dbbus_enter(void)
{
    int rc = NO_ERR;

    rc = mstar_dbbus_enter_serial_debug();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_enter_serial_debug error\n", __func__);
        return rc;
    }

    rc = mstar_dbbus_stop_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_stop_mcu error\n", __func__);
        return rc;
    }

    rc = mstar_dbbus_iic_use_bus();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_iic_use_bus error\n", __func__);
        return rc;
    }

    rc = mstar_dbbus_iic_reshape();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_dbbus_iic_reshape error\n", __func__);
        return rc;
    }

    return rc;
}

int mstar_mp_start_mcu(void)
{
    return mstar_set_reg_low_byte(0x0FE6, 0x00);   //bank:mheg5, addr:h0073
}

int mstar_mp_stop_mcu(void)
{
    return mstar_set_reg_low_byte(0x0FE6, 0x01);   //bank:mheg5, addr:h0073
}

int msg30xx_covert_Rvalue(int dump_time, s32 deltaR)
{
    s32 result = 0;

    if (deltaR >= IIR_MAX)
        return 0;
    if (deltaR == 0)
        deltaR = 1;

    if(deltaR > -1000)
        result = (int)(73318 * dump_time * 1000) / (55 * (abs(deltaR) - 0));
    else
        result = (int)(53248 * dump_time * 1000) / (55 * (abs(deltaR) - 0));

    return result;
}
int msg2846a_covert_Rvalue(int dump_time, s32 deltaR)
{
    s32 result = 0;

    if (deltaR >= IIR_MAX)
        return 0;
    if (deltaR == 0)
        deltaR = 1;

    if(deltaR >= 0)
        result = (int)(58654 * dump_time * 1000) / (55 * (abs(deltaR) - 0));
    else
        result = (int)(42598 * dump_time * 1000) / (55 * (abs(deltaR) - 0));

    return result;
}

int msg28xx_covert_Rvalue(s32 deltaR)
{
    s32 result = 0;

    if (deltaR >= IIR_MAX)
        return 0;
    if (deltaR == 0)
        deltaR = 1;
    if(deltaR > -1000)
        result = (int)(187541 * 20) / ((abs(deltaR) - 0));
    else
        result = (int)(100745 * 20) / (((deltaR) - 0));

    return result;
}

u16 msg28xx_read_test_pins(u16 itemID, int *testPins)
{
    int nSize = 0, i;
    char str[512] = {0};
    int _gGRtestPins[13] = {0};

    switch (itemID)
    {
        case 1:
        case 11:
            ms_get_ini_data("SHORT_TEST_N1","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_N1 nSize = %d\n", nSize);
            break;
        case 2:
        case 12:
            ms_get_ini_data("SHORT_TEST_N2","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_N2 nSize = %d\n", nSize);
            break;
        case 3:
        case 13:
            ms_get_ini_data("SHORT_TEST_S1","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_S1 nSize = %d\n", nSize);
            break;
        case 4:
        case 14:
            ms_get_ini_data("SHORT_TEST_S2","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_S2 nSize = %d\n", nSize);
            break;

        case 5:
        case 15:
            if (ms_get_ini_data("SHORT_TEST_N3", "MUX_MEM_20_3E", str) != 0) {
                ms_get_ini_data("SHORT_TEST_N3","TEST_PIN",str);
                nSize = ms_ini_split_int_array(str, testPins);
                TS_LOG_DEBUG("SHORT_TEST_N3 nSize = %d\n", nSize);
            }
            else if (ms_get_ini_data("SHORT_TEST_S3", "MUX_MEM_20_3E", str) != 0) {
                ms_get_ini_data("SHORT_TEST_S3","TEST_PIN",str);
                nSize = ms_ini_split_int_array(str, testPins);
                TS_LOG_DEBUG("SHORT_TEST_S3 nSize = %d\n", nSize);
            }
            else if (ms_get_ini_data("SHORT_TEST_GR", "MUX_MEM_20_3E", str) != 0) {
                if (mp_test_data->sensorInfo.numGr == 0)
                    nSize = 0;
                else
                    nSize = mp_test_data->sensorInfo.numGr;

                for (i = 0; i < sizeof(_gGRtestPins) / sizeof(_gGRtestPins[0]); i++)
                    testPins[i] = _gGRtestPins[i];
                TS_LOG_DEBUG("SHORT_TEST_GR nSize = %d\n", nSize);
            }

            break;

        case 0:
            default:
            return NO_ERR;
    }

    for (i = nSize; i < MAX_CHANNEL_NUM_28XX; i++) {
        testPins[i] = PIN_NO_ERROR;
    }

    return nSize;
}

u16 msg30xx_read_test_pins(u16 itemID, int *testPins)
{
    int nSize = 0, i;
    char str[512] = {0};
    int _gMsg30xxGRtestPins[14] = {0};

    switch (itemID)
    {
        case 0:
        case 10:
            ms_get_ini_data("SHORT_TEST_N1","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_N1 nSize = %d\n", nSize);
            break;
        case 1:
        case 11:
            ms_get_ini_data("SHORT_TEST_N2","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_N2 nSize = %d\n", nSize);
            break;
        case 2:
        case 12:
            ms_get_ini_data("SHORT_TEST_S1","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_S1 nSize = %d\n", nSize);
            break;
        case 3:
        case 13:
            ms_get_ini_data("SHORT_TEST_S2","TEST_PIN",str);
            nSize = ms_ini_split_int_array(str, testPins);
            TS_LOG_DEBUG("SHORT_TEST_S2 nSize = %d\n", nSize);
            break;

        case 4:
        case 14:
            if (ms_get_ini_data("SHORT_TEST_N3", "MUX_MEM_20_3E", str) != 0) {
                ms_get_ini_data("SHORT_TEST_N3","TEST_PIN",str);
                nSize = ms_ini_split_int_array(str, testPins);
                TS_LOG_DEBUG("SHORT_TEST_N3 nSize = %d\n", nSize);
            }
            else if (ms_get_ini_data("SHORT_TEST_S3", "MUX_MEM_20_3E", str) != 0) {
                ms_get_ini_data("SHORT_TEST_S3","TEST_PIN",str);
                nSize = ms_ini_split_int_array(str, testPins);
                TS_LOG_DEBUG("SHORT_TEST_S3 nSize = %d\n", nSize);
            }
            else if (ms_get_ini_data("SHORT_TEST_GR", "MUX_MEM_20_3E", str) != 0) {
                if (mp_test_data->sensorInfo.numGr == 0)
                    nSize = 0;
                else
                    nSize = mp_test_data->sensorInfo.numGr;

                for (i = 0; i < sizeof(_gMsg30xxGRtestPins) / sizeof(_gMsg30xxGRtestPins[0]); i++)
                    testPins[i] = _gMsg30xxGRtestPins[i];
                TS_LOG_DEBUG("SHORT_TEST_GR nSize = %d\n", nSize);
            }
            break;

        default:
            return 0;
    }

    for (i = nSize; i < MAX_CHANNEL_NUM_30XX; i++) {
        testPins[i] = PIN_NO_ERROR;
    }

    return nSize;
}

s32 msg28xx_short_test_judge(int *deltac_data, u8 nItemID, s8 *TestFail, u16 TestFail_check[][MAX_MUTUAL_NUM])
{
    int nRet = 1, i, count_test_pin = 0;
    int testPins[MAX_CHANNEL_NUM_28XX] = {0};

    msg28xx_read_test_pins(nItemID, testPins);

    for (i = 0; i < sizeof(testPins) / sizeof(testPins[0]); i++) {
        if (testPins[i] != PIN_NO_ERROR)
            count_test_pin++;
    }

    for (i = 0; i < count_test_pin; i++) {
        TestFail_check[nItemID][i] = testPins[i];
        if (0 == check_thrs(deltac_data[i + (nItemID - 1) * 13],
                            mp_test_data->sensorInfo.thrsICpin,
                            -mp_test_data->sensorInfo.thrsICpin))
        {
            TestFail[nItemID] = 1;
            mp_test_result->nShortResult = ITO_TEST_FAIL;
            nRet = 0;
        }
    }

    return nRet;
}

s32 msg30xx_short_test_judge(int *deltac_data, u8 nItemID, s8 *TestFail, u16 TestFail_check[][MAX_MUTUAL_NUM])
{
    int nRet = 1, i, count_test_pin = 0;
    int testPins[MAX_CHANNEL_NUM_30XX] = {0};

    msg30xx_read_test_pins(nItemID, testPins);

    for(i = 0; i < ARRAY_SIZE(testPins); i++)
        TS_LOG_DEBUG("%s: testPins[%d] = %d\n",__func__, i , testPins[i]);

    for (i = 0; i < sizeof(testPins) / sizeof(testPins[0]); i++) {
        if (testPins[i] != PIN_NO_ERROR)
            count_test_pin++;
    }

    for (i = 0; i < count_test_pin; i++) {
        TestFail_check[nItemID][i] = testPins[i];
        if (0 == check_thrs(deltac_data[i + nItemID * 14],
                            mp_test_data->sensorInfo.thrsICpin,
                            -mp_test_data->sensorInfo.thrsICpin))
        {
            TestFail[nItemID] = 1;
            mp_test_result->nShortResult = ITO_TEST_FAIL;
            nRet = 0;
        }
    }

    return nRet;
}

int msg28xx_short_judge(int *deltac_data)
{
    int i, j;
    int *senseR = NULL, sense = mp_test_data->sensorInfo.numSen;
    int *driveR = NULL, drive = mp_test_data->sensorInfo.numDrv;
    int *GRR = NULL, gr = mp_test_data->sensorInfo.numGr;
    int thrs = 0;
    u32 nRetVal = 1;
    int datalen = MAX_CHANNEL_NUM_28XX;
    int *deltaC = NULL;
    int sum = 0, cunt = 0, avg = 0, convert[MAX_CHANNEL_NUM_28XX] = {0};

    deltaC = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    senseR = kcalloc(sense, sizeof(int), GFP_KERNEL);
    driveR = kcalloc(drive, sizeof(int), GFP_KERNEL);
    GRR = kcalloc(gr, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(senseR) || ERR_ALLOC_MEM(driveR) || ERR_ALLOC_MEM(GRR)) {
        TS_LOG_ERR("%s: Failed to allocate senseR/driveR/GRR mem\n",__func__);
        nRetVal = -1;
        goto ITO_TEST_END;
    }

    thrs = msg28xx_covert_Rvalue(mp_test_data->sensorInfo.thrsICpin);

    for (i = 0; i < sense; i++) senseR[i] = thrs;
    for (i = 0; i < drive; i++) driveR[i] = thrs;
    for (i = 0; i < gr; i++) GRR[i] = thrs;

    for (i = 1; i < 6; i++)  {//max 6 subframe
        for (j = 0; j < 13; j++) {// max 13 AFE
            if (((i-1) * 13 + j) < MAX_CHANNEL_NUM_28XX)
                mp_test_result->pShortFailChannel[(i-1) * 13 + j] = (u32)PIN_UN_USE;
            mp_test_result->normalTestFail_check[i][j] = PIN_UN_USE;
        }
    }

    memset(mp_test_result->normalTestFail, 0, TEST_ITEM_NUM * sizeof(s8));

    for (i = 0; i < datalen; i+=2) // 2 AFE * 28 subframe,but AFE0 un-use
    {
        deltaC[i] = deltac_data[i+1];
        deltaC[i + 1] = deltac_data[i];

        if ((deltaC[i] > -1000) && (deltaC[i] < 1000))
        {
            sum += deltaC[i];
            cunt++;
        }

        if ((deltaC[i+1] > -1000) && (deltaC[i+1] < 1000))
        {
            sum += deltaC[i + 1];
            cunt++;
        }
    }
    if (cunt != 0)
    {
        avg = (int)(sum / cunt);

        for (i = 0; i < datalen; i++)
        {
            deltac_data[i] = deltaC[i] - avg;
        }
    }

    for(i = 0; i < datalen; i++) {

        convert[i] = msg28xx_covert_Rvalue(deltac_data[i]);

        if(mp_test_data->PAD2Sense != NULL) {
            for(j = 0; j < p_mp_func->sense_len; j++) {

                if(i == mp_test_data->PAD2Sense[j]) {

                    mp_test_result->pShortRData[j] = convert[i];
                    mp_test_result->pShortResultData[j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[j] = ITO_TEST_FAIL;
                    }
                }
            }

        }

        if(mp_test_data->PAD2Drive != NULL) {

            for(j = 0; j < p_mp_func->drive_len; j++) {

                if(i == mp_test_data->PAD2Drive[j]) {

                    mp_test_result->pShortRData[p_mp_func->sense_len + j] = convert[i];
                    mp_test_result->pShortResultData[p_mp_func->sense_len + j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[p_mp_func->sense_len + j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[p_mp_func->sense_len + j] = -1;
                    }
                }
            }
        }

        if(mp_test_data->PAD2GR != NULL) {

            for(j = 0; j < p_mp_func->gr_len; j++) {

                if(i == mp_test_data->PAD2GR[j]) {

                    mp_test_result->pShortRData[p_mp_func->sense_len + p_mp_func->drive_len + j] = convert[i];
                    mp_test_result->pShortResultData[p_mp_func->sense_len + p_mp_func->drive_len + j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[p_mp_func->sense_len + p_mp_func->drive_len + j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[p_mp_func->sense_len + p_mp_func->drive_len + j] = -1;
                    }
                }
            }
        }
    }


ITO_TEST_END:
    mp_kfree(&deltaC);
    mp_kfree(&senseR);
    mp_kfree(&driveR);
    mp_kfree(&GRR);
    TS_LOG_DEBUG("Short Judge done \n");
    return nRetVal;
}

int msg30xx_short_judge(int *deltac_data)
{
    int i, j;
    int *senseR = NULL, sense = mp_test_data->sensorInfo.numSen;
    int *driveR = NULL, drive = mp_test_data->sensorInfo.numDrv;
    int *GRR = NULL, gr = mp_test_data->sensorInfo.numGr;
    int thrs = 0, dump_time;
    u32 nRetVal = 1;
    int datalen1 = 0;
    int *deltaC = NULL;
    int sum = 0, cunt = 0, avg = 0, convert[MAX_AFENUMs_30XX * MAX_SUBFRAMES_30XX] = {0};

    deltaC = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

    senseR = kcalloc(sense, sizeof(int), GFP_KERNEL);
    driveR = kcalloc(drive, sizeof(int), GFP_KERNEL);
    GRR = kcalloc(gr, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(senseR) || ERR_ALLOC_MEM(driveR) || ERR_ALLOC_MEM(GRR)) {
        TS_LOG_ERR("%s: Failed to allocate senseR/driveR/GRR mem\n",__func__);
        nRetVal = -1;
        goto ITO_TEST_END;
    }
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    dump_time = mp_test_data->SHORT_Dump2 - mp_test_data->SHORT_Dump1;
	    dump_time = (dump_time * 4 * 2 *100) / 5166;
	    TS_LOG_DEBUG("%s, dump_time = %d, thrsICpin = %d\n", __func__, dump_time, mp_test_data->sensorInfo.thrsICpin);
	    thrs = msg2846a_covert_Rvalue(dump_time, mp_test_data->sensorInfo.thrsICpin);
    }
    else{
	dump_time = (((mp_test_data->SHORT_Dump1 + 1) * 4 * 2)*100) / 5166;
        TS_LOG_DEBUG("%s, dump_time = %d, short_thrs = %d\n", __func__, dump_time, mp_test_data->sensorInfo.thrsShort);
        thrs = msg30xx_covert_Rvalue(dump_time, mp_test_data->sensorInfo.thrsShort);
    }

    for (i = 0; i < sense; i++) senseR[i] = thrs;

    for (i = 0; i < drive; i++) driveR[i] = thrs;

    for (i = 0; i < gr; i++) GRR[i] = thrs;

    for (i = 0; i < 6; i++)  {//max 6 subframe
        for (j = 0; j < 14; j++) {// max 14 AFE
            if ((i * 14 + j) < MAX_CHANNEL_NUM_30XX)
                mp_test_result->pShortFailChannel[i * 14 + j] = (u32)PIN_UN_USE;
            mp_test_result->normalTestFail_check[i][j] = PIN_UN_USE;
        }
    }

    memset(mp_test_result->normalTestFail, 0, TEST_ITEM_NUM * sizeof(s8));

    datalen1 = MAX_AFENUMs_30XX * MAX_SUBFRAMES_30XX;
    for (i = 0; i < datalen1; i++) // 2 AFE * 28 subframe,but AFE0 un-use
    {
        deltaC[i] = deltac_data[i];

        if ((deltaC[i] > -1000) && (deltaC[i] < 1000))
        {
            sum += deltaC[i];
            cunt++;
        }
    }

    if (cunt != 0)
    {
        avg = (int)(sum / cunt);

        TS_LOG_DEBUG("%s: avg = %d, sum = %d, cunt = %d\n",__func__, avg, sum, cunt);

        for (i = 0; i < datalen1; i++)
        {
            deltac_data[i] = deltaC[i] - avg;
        }
    }

    for(i = 0; i < datalen1; i++) {

        convert[i] = msg30xx_covert_Rvalue(dump_time, deltac_data[i]);

        if(mp_test_data->PAD2Sense != NULL) {
            for(j = 0; j < p_mp_func->sense_len; j++) {

                if(i == mp_test_data->PAD2Sense[j]) {

                    mp_test_result->pShortRData[j] = convert[i];
                    mp_test_result->pShortResultData[j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[j] = ITO_TEST_FAIL;
                        TS_LOG_ERR("short test fail pin[%d]: %d", j, deltac_data[i]);
                    }
                }
            }

        }
        if(mp_test_data->PAD2Drive != NULL) {

            for(j = 0; j < p_mp_func->drive_len; j++) {

                if(i == mp_test_data->PAD2Drive[j]) {

                    mp_test_result->pShortRData[p_mp_func->sense_len + j] = convert[i];
                    mp_test_result->pShortResultData[p_mp_func->sense_len + j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[p_mp_func->sense_len + j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[p_mp_func->sense_len + j] = -1;
                        TS_LOG_ERR("short ratio fail pin[%d]: %d", p_mp_func->sense_len + j, deltac_data[i]);
                    }
                }
            }
        }

        if(mp_test_data->PAD2GR != NULL) {

            for(j = 0; j < p_mp_func->gr_len; j++) {

                if(i == mp_test_data->PAD2GR[j]) {

                    mp_test_result->pShortRData[p_mp_func->sense_len + p_mp_func->drive_len + j] = convert[i];
                    mp_test_result->pShortResultData[p_mp_func->sense_len + p_mp_func->drive_len + j] = deltac_data[i];

                    if (convert[i] >= 10000)
                        mp_test_result->pShortRData[p_mp_func->sense_len + p_mp_func->drive_len + j] = 10000;

                    if (0 == check_thrs(deltac_data[i],
                                        mp_test_data->sensorInfo.thrsICpin,
                                        -mp_test_data->sensorInfo.thrsICpin))
                    {
                        nRetVal = -1;
                        mp_test_result->nShortResult = ITO_TEST_FAIL;
                        mp_test_result->pShortFailChannel[p_mp_func->sense_len + p_mp_func->drive_len + j] = -1;
                        TS_LOG_ERR("short ratio fail pin[%d]: %d", p_mp_func->sense_len + p_mp_func->drive_len + j, deltac_data[i]);
                    }
                }
            }
        }
    }
    mp_dump_data(deltac_data, datalen1, -32, 10, 10, "short data", 0);
ITO_TEST_END:
    mp_kfree(&senseR);
    mp_kfree(&driveR);
    mp_kfree(&GRR);
    mp_kfree(&deltaC);
     TS_LOG_DEBUG("Short Judge done \n");
    return nRetVal;
}

int msg28xx_open_judge(int *deltac_data, int deltac_size)
{
    u16 i, j, k;
    u16 nRowNum = 0, nColumnNum = 0;
    u16 nCSub, nCfb;
    s32 nRetVal = 1, bg_per_csub;
    int ratioAvg = 0, ratioAvg_max = 0, ratioAvg_min = 0, passCount = 0;
    int ratioAvg_border = 0, ratioAvg_border_max = 0, ratioAvg_border_min = 0, passCount1 = 0;
    int ratioAvg_move = 0, ratioAvg_border_move = 0;
    int sense_num = 0, drv_num = 0, *result_buf = NULL;
    s8 nNormalTestResult[2] = {0};

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    result_buf = mp_test_result->pOpenResultData;
    nCSub = mp_test_data->Open_test_csub;

    /* if open mode is sine mode, Csub must be zero. */
    if ((mp_test_data->Open_mode == 1))
        nCSub = 0;

    if (!mp_test_data->Open_test_cfb)
        nCfb = 2;
    else
        nCfb = mp_test_data->Open_test_cfb;

    bg_per_csub = (int)(92012 / (11 * nCfb));

    for (i = 0; i < sense_num * drv_num; i++)
    {
        if (deltac_data[i] > 31000)
            return -1;

        result_buf[i] = bg_per_csub * nCSub - deltac_data[i];

        // For mutual key, last column if not be used, show number "one".
        if ((mp_test_data->Mutual_Key == 1 || mp_test_data->Mutual_Key == 2) && (mp_test_data->sensorInfo.numKey != 0))
        {
            if (mp_test_data->Pattern_type == 5)
            {
                // KEY_CH = 1, it mean keys in same drive. Current one key project only KEY_CH = 1 type.
                if (mp_test_data->sensorInfo.KEY_CH != mp_test_data->sensorInfo.numKey)
                {
                    if (!((i + 1) % drv_num))
                    {
                        result_buf[i] = NULL_DATA;
                        for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                            if ((i + 1) / drv_num == mp_test_data->KeySen[k])
                            {
                                if(mp_test_data->Pattern_model == 1)
                                    result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                                else
                                    result_buf[i] = 1673 * nCSub - deltac_data[i];
                            }
                    }
                }
                else
                {
                    if (i > ((sense_num - 1) * drv_num - 1))
                    {
                        result_buf[i] = NULL_DATA;
                        for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                            if (((i + 1) - (sense_num - 1) * drv_num) == mp_test_data->KeySen[k])
                            {
                                if(mp_test_data->Pattern_model == 1)
                                    result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                                else
                                    result_buf[i] = 1673 * nCSub - deltac_data[i];
                            }
                    }
                }
            }
            else
            {
                if ((sense_num < drv_num) && ((i + 1) % drv_num == 0))
                {
                    result_buf[i] = NULL_DATA;
                    for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                        if ((i + 1) / drv_num == mp_test_data->KeySen[k])
                        {
                            result_buf[i] = 1673 * nCSub - deltac_data[i];
                        }
                }

                if ((sense_num > drv_num) && (i > (sense_num - 1) * drv_num - 1))
                {
                    result_buf[i] = NULL_DATA;
                    for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                    {
                        if (((i + 1) - (sense_num - 1) * drv_num) == mp_test_data->KeySen[k])
                        {
                            result_buf[i] = 1673 * nCSub - deltac_data[i];
                        }
                    }
                }
            }
        }
    }

    memset(mp_test_result->normalTestFail_check_Deltac, 0xFFFF, MAX_MUTUAL_NUM);
    memset(mp_test_result->normalTestFail_check_Ratio, 0xFFFF, MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio,0,MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio_border,0,MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio_move,0,MAX_MUTUAL_NUM);

    nRowNum = drv_num;
    nColumnNum = sense_num;

    for (k = 0; k < deltac_size; k++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[k]) {
            if (k == 0)
                nNormalTestResult[0] = 1;   // no golden sample
            break;
        }

        if (result_buf[k] != NULL_DATA)
        {
            mp_test_result->ratio[k] = (result_buf[k] * 1000) / mp_test_data->Goldensample_CH_0[k];

            if (0 == check_thrs(result_buf[k], mp_test_data->Goldensample_CH_0_Max[k], mp_test_data->Goldensample_CH_0_Min[k]))
            {
                nNormalTestResult[0] = 1;
                mp_test_result->nNormalTestResultCheck[k] = (u16)(((k / drv_num) + 1) * 100 + ((k % drv_num) + 1));
            }
            else
            {
                mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;
                if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey == 0) && ((k % drv_num == 0 )||((k + 1) % drv_num == 0)))
                {
                    ratioAvg_border += mp_test_result->ratio[k];
                    passCount1 += 1;
                }
                else if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey != 0) && ((k % drv_num == 0) || ((k + 2) % drv_num == 0)))
                {
                    ratioAvg_border += mp_test_result->ratio[k];
                    passCount1 += 1;
                }
                else
                {
                    ratioAvg += mp_test_result->ratio[k];
                    passCount += 1;
                }
            }
        }
        else
        {
            mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;
        }
        mp_test_result->normalTestFail_check_Deltac[k] = mp_test_result->nNormalTestResultCheck[k];
    }

    TS_LOG_DEBUG("  Msg28xx Open Test# mp_test_result->normalTestFail_check_Deltac Channel  \n");

    ratioAvg_max = (int)(100000 + (mp_test_data->ToastInfo.persentDC_VA_Ratio * 1000) + (mp_test_data->ToastInfo.persentDC_VA_Ratio_up * 1000)) / 100;
    ratioAvg_min = (int)(100000 - (mp_test_data->ToastInfo.persentDC_VA_Ratio * 1000)) / 100;

    ratioAvg_border_max=(int)(100000 + (mp_test_data->ToastInfo.persentDC_Border_Ratio * 1000) + (mp_test_data->ToastInfo.persentDC_VA_Ratio_up * 1000)) / 100;
    ratioAvg_border_min = (int)(100000 - (mp_test_data->ToastInfo.persentDC_Border_Ratio * 1000)) / 100;

    if (passCount != 0)
    {
        if (passCount1 != 0)
        {
            ratioAvg_border_move = ratioAvg_border / passCount1;

            ratioAvg_move = ratioAvg / passCount;

            for (i = 0; i < MAX_MUTUAL_NUM; i++)
            {
                if ((mp_test_data->sensorInfo.numKey == 0) && ((i % drv_num == 0) || ((i + 1) % drv_num == 0)))
                {
                    mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_border_move + 1000;
                }
                else if ((mp_test_data->sensorInfo.numKey != 0) && ((i % drv_num == 0) || ((i + 2) % drv_num == 0)))
                {
                      mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_border_move + 1000;
                }
                else
                {
                    mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_move + 1000;
                }

            }
        }
        else
        {
            ratioAvg_move = ratioAvg / passCount;

            for (i = 0; i < MAX_MUTUAL_NUM; i++)
            {
               mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_move + 1000;
            }
        }
    }
    else
    {
        memcpy(mp_test_result->ratio, mp_test_result->ratio_move, MAX_MUTUAL_NUM);
    }

    /* Based on the size of deltac_data for th loop */
    for (j = 0; j < deltac_size; j++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[j]) {
            if (j == 0)
                nNormalTestResult[1] = 1;   // no golden sample
            break;
        }

        if (PIN_NO_ERROR == mp_test_result->nNormalTestResultCheck[j])
        {
            if (result_buf[j] != NULL_DATA)
            {
                if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey == 0) && ((j % drv_num == 0) || ((j + 1) % drv_num == 0)))
                {
                    if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min))
                    {
                        nNormalTestResult[1] = 1;
                        mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    }
                    else
                    {
                        mp_test_result->nNormalTestResultCheck[j] = PIN_NO_ERROR;
                    }
                }
                else if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey != 0) && ((j % drv_num == 0) || ((j + 2) % drv_num == 0)))
                {
                    if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min))
                    {
                        nNormalTestResult[1] = 1;
                        mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    }
                    else
                    {
                        mp_test_result->nNormalTestResultCheck[j] = PIN_NO_ERROR;
                    }
                }
                else
                {
                    if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_max, ratioAvg_min))
                    {
                        nNormalTestResult[1] = 1;
                        mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    }
                    else
                    {
                        mp_test_result->nNormalTestResultCheck[j] = PIN_NO_ERROR;
                    }
                }
            }
            else
            {
                mp_test_result->nNormalTestResultCheck[j] = PIN_NO_ERROR;
            }
        }
        else
        {
            mp_test_result->normalTestFail_check_Ratio[j] = mp_test_result->nNormalTestResultCheck[j];
            continue;
        }
        mp_test_result->normalTestFail_check_Ratio[j] = mp_test_result->nNormalTestResultCheck[j];
    }

    for (k = 0; k < MAX_MUTUAL_NUM; k++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[k])
        {
            mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;
            mp_test_result->normalTestFail_check_Deltac[k] = PIN_NO_ERROR;
            mp_test_result->normalTestFail_check_Ratio[k] = PIN_NO_ERROR;
        }
    }

    if ((nNormalTestResult[0] != 0) || (nNormalTestResult[1] != 0))
        nRetVal = -1;

    for (i = 0; i < 2; i++)
    {
        mp_test_result->pCheck_Fail[i] = nNormalTestResult[i];
    }
    for (i = 0; i < MAX_MUTUAL_NUM; i++)    // reduce memory operation instead of memcpy
    {
        mp_test_result->pOpenFailChannel[i] = mp_test_result->normalTestFail_check_Deltac[i];
        mp_test_result->pOpenRatioFailChannel[i] = mp_test_result->normalTestFail_check_Ratio[i];
        mp_test_result->pGolden_CH_Max_Avg[i] = mp_test_result->ratio_move[i];
    }

    return nRetVal;
}

int msg28xx_uniformity_judge(int *deltac_data, int deltac_size)
{
    int ret = ITO_TEST_FAIL;

    TS_LOG_ERR("%s: doesn't support this test item \n", __func__);

    return ret;
}

void msg30xx_get_csub_cfb(u16 *nCSub,u16 *nCfb)
{
    /* if open mode is sine mode, Csub must be zero. */
    if (mp_test_data->Open_mode == 1)
        *nCSub = 0;

    if (!mp_test_data->Open_test_cfb)
        *nCfb = 2;
    else
        *nCfb = mp_test_data->Open_test_cfb;
}

int judge_nNormalTestResult(s8 *testresult)
{
    int nRetVal = ITO_TEST_OK;
    int i = 0;

    if ((testresult[0] != 0) || (testresult[1] != 0))
        nRetVal = ITO_TEST_FAIL;

    for (i = 0; i < 2; i++)
    {
        mp_test_result->pCheck_Fail[i] = testresult[i];
    }

    return nRetVal;
}

void msg30xx_open_judge_calc_ratio_avg(int passCount, int passCount1, int ratioAvg, int ratioAvg_border)
{
    int i, sense_num, drv_num;
    int ratioAvg_border_move = 0, ratioAvg_move = 0;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;

    /* Calculate the average of ratio */
    if (passCount != 0)
    {
        ratioAvg_move = ratioAvg / passCount;

        if (passCount1 != 0)
        {
            ratioAvg_border_move = ratioAvg_border / passCount1;

            for (i = 0; i < MAX_MUTUAL_NUM; i++)
            {
                if ((mp_test_data->sensorInfo.numKey == 0) && ((i % drv_num == 0) || ((i + 1) % drv_num == 0)))
                {
                    mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_border_move + 1000;
                }
                else if ((mp_test_data->sensorInfo.numKey != 0) && ((i % drv_num == 0) || ((i + 2) % drv_num == 0)))
                {
                      mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_border_move + 1000;
                }
                else
                {
                    mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_move + 1000;
                }

            }
        }
        else
        {
            for (i = 0; i < MAX_MUTUAL_NUM; i++)
            {
               mp_test_result->ratio_move[i] = mp_test_result->ratio[i] - ratioAvg_move + 1000;
            }
        }
    }
    else
    {
        memcpy(mp_test_result->ratio, mp_test_result->ratio_move, MAX_MUTUAL_NUM);
    }
}

s8 msg30xx_open_judge_ratio(int deltac_size, int *by_pass_buf, int *result_buf)
{
    int j, sense_num = 0, drv_num = 0;
    int ratioAvg_max, ratioAvg_min, ratioAvg_border_max, ratioAvg_border_min;
    s8 ret = 0;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;

    ratioAvg_max = (int)(100000 + (mp_test_data->ToastInfo.persentDC_VA_Ratio * 1000) + (mp_test_data->ToastInfo.persentDC_VA_Ratio_up * 1000)) / 100;
    ratioAvg_min = (int)(100000 - (mp_test_data->ToastInfo.persentDC_VA_Ratio * 1000)) / 100;

    ratioAvg_border_max=(int)(100000 + (mp_test_data->ToastInfo.persentDC_Border_Ratio * 1000) + (mp_test_data->ToastInfo.persentDC_VA_Ratio_up * 1000)) / 100;
    ratioAvg_border_min = (int)(100000 - (mp_test_data->ToastInfo.persentDC_Border_Ratio * 1000)) / 100;

    /* Judge ratio */
    for (j = 0; j < deltac_size; j++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[j]) {
            if (j == 7 || j == 8) // bypass node
                continue;

            if (j == 0)
                ret = 1;   // no golden sample
            break;
        }

        if (result_buf[j] != NULL_DATA)
        {
            if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey == 0) && ((j % drv_num == 0) || ((j + 1) % drv_num == 0)))
            {
                if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min) && by_pass_buf[j] == NORMAL_JUDGE)
                {
                    ret = 1;
                    mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    TS_LOG_ERR("1 open ratio fail [%d][%d]: %d, ratio_Max:%d , ratio_Min:%d\n", j/drv_num, j%drv_num, mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min);
                }
            }
            else if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey != 0) && ((j % drv_num == 0) || ((j + 2) % drv_num == 0)))
            {
                if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min) && by_pass_buf[j] == NORMAL_JUDGE)
                {
                    ret = 1;
                    mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    TS_LOG_ERR("2 open ratio fail [%d][%d]: %d, ratio_Max:%d , ratio_Min:%d\n", j/drv_num, j%drv_num, mp_test_result->ratio_move[j], ratioAvg_border_max, ratioAvg_border_min);
                }
            }
            else
            {
                if (0 == check_thrs(mp_test_result->ratio_move[j], ratioAvg_max, ratioAvg_min) && by_pass_buf[j] == NORMAL_JUDGE)
                {
                    ret = 1;
                    mp_test_result->nNormalTestResultCheck[j] = (u16)(((j / drv_num) + 1) * 100 + ((j % drv_num) + 1));
                    TS_LOG_ERR("3 open ratio fail [%d][%d]: %d, ratio_Max:%d , ratio_Min:%d\n", j/drv_num, j%drv_num, mp_test_result->ratio_move[j], ratioAvg_max, ratioAvg_min);
                }
            }
        }
        mp_test_result->normalTestFail_check_Ratio[j] = mp_test_result->nNormalTestResultCheck[j];
    }

    return ret;
}

int msg30xx_open_judge(int *deltac_data, int deltac_size)
{
    u16 i, k;
    u16 nRowNum = 0, nColumnNum = 0;
    u16 nCSub, nCfb;
    s32 nRetVal = 1, bg_per_csub;
    s8  nNormalTestResult[2] = {0};
    int passCount = 0, passCount1 = 0, ratioAvg = 0, ratioAvg_border = 0;
    int sense_num = 0, drv_num = 0, *result_buf = NULL,*by_pass_buf = NULL;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    result_buf = mp_test_result->pOpenResultData;
    by_pass_buf = kcalloc(deltac_size, sizeof(int), GFP_KERNEL);
    nCSub = mp_test_data->Open_test_csub;

    msg30xx_get_csub_cfb(&nCSub,&nCfb);

    bg_per_csub = (int)(92012 / (11 * nCfb));

    /* Calculate DeltaC's value */
    for (i = 0; i < sense_num * drv_num; i++)
    {
        if (deltac_data[i] > 31000)
            return -1;

        result_buf[i] = bg_per_csub * nCSub - deltac_data[i];

        // For mutual key, last column if not be used, show number "one".
        if ((mp_test_data->Mutual_Key == 1 || mp_test_data->Mutual_Key == 2) && (mp_test_data->sensorInfo.numKey != 0))
        {
            if (mp_test_data->Pattern_type == 5)
            {
                // KEY_CH = 1, it mean keys in same drive. Current one key project only KEY_CH = 1 type.
                if (mp_test_data->sensorInfo.KEY_CH != mp_test_data->sensorInfo.numKey)
                {
                    if (!((i + 1) % drv_num))
                    {
                        result_buf[i] = NULL_DATA;
                        for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                            if ((i + 1) / drv_num == mp_test_data->KeySen[k])
                                    result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                    }
                }
                else
                {
                    if (i > ((sense_num - 1) * drv_num - 1))
                    {
                        result_buf[i] = NULL_DATA;
                        for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                            if (((i + 1) - (sense_num - 1) * drv_num) == mp_test_data->KeySen[k])
                                    result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                    }
                }
            }
            else
            {
                if ((sense_num < drv_num) && ((i + 1) % drv_num == 0))
                {
                    result_buf[i] = NULL_DATA;
                    for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                        if ((i + 1) / drv_num == mp_test_data->KeySen[k])
                                result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                }

                if ((sense_num > drv_num) && (i > (sense_num - 1) * drv_num - 1))
                {
                    result_buf[i] = NULL_DATA;
                    for (k = 0; k < mp_test_data->sensorInfo.numKey; k++)
                    {
                        if (((i + 1) - (sense_num - 1) * drv_num) == mp_test_data->KeySen[k])
                                result_buf[i] = bg_per_csub * nCSub - deltac_data[i];
                    }
                }
            }
        }
    }

    memset(by_pass_buf, NORMAL_JUDGE, deltac_size);

#ifdef ENABLE_BY_PASS_CHANNEL
    by_pass_buf[ADDR(0,0)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR(0,6)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR(0,7)]= NO_JUDGE;
    by_pass_buf[ADDR(0,8)]= NO_JUDGE;
    by_pass_buf[ADDR(0,9)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR(0,(drv_num-1))]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR(1,7)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR(1,8)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR((sense_num-1),0)]= NO_RATIO_JUDGE;
    by_pass_buf[ADDR((sense_num-1),(drv_num-1))]= NO_RATIO_JUDGE;
#endif

    memset(mp_test_result->normalTestFail_check_Deltac, 0xFFFF, MAX_MUTUAL_NUM);
    memset(mp_test_result->normalTestFail_check_Ratio, 0xFFFF, MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio,0,MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio_border,0,MAX_MUTUAL_NUM);
    memset(mp_test_result->ratio_move,0,MAX_MUTUAL_NUM);

    nRowNum = drv_num;
    nColumnNum = sense_num;

    /* Judge deltaC with range */
    for (k = 0; k < deltac_size; k++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[k]) {
            if (k == 7 || k == 8) // bypass node
                continue;

            if (k == 0) // no golden sample
                nNormalTestResult[0] = 1;
            break;
        }

        if (result_buf[k] != NULL_DATA)
        {
            mp_test_result->ratio[k] = (result_buf[k] * 1000) / mp_test_data->Goldensample_CH_0[k];

            if (0 == check_thrs(result_buf[k], mp_test_data->Goldensample_CH_0_Max[k], mp_test_data->Goldensample_CH_0_Min[k]) && by_pass_buf[k] != NO_JUDGE)
            {
                nNormalTestResult[0] = 1;
                mp_test_result->nNormalTestResultCheck[k] = (u16)(((k / drv_num) + 1) * 100 + ((k % drv_num) + 1));
                TS_LOG_ERR("open fail channel[%d][%d]: %d, Golden_Max:%d, , Golden_Min:%d\n", k/drv_num, k%drv_num, result_buf[k], mp_test_data->Goldensample_CH_0_Max[k], mp_test_data->Goldensample_CH_0_Min[k]);
            }
            else
            {
                    mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;

                    if (by_pass_buf[k] != NORMAL_JUDGE)
                        continue;

                    if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey == 0) && ((k % drv_num == 0 )||((k + 1) % drv_num == 0)))
                    {
                        ratioAvg_border += mp_test_result->ratio[k];
                        passCount1 += 1;
                    }
                    else if ((mp_test_data->Pattern_type == 3) && (mp_test_data->sensorInfo.numKey != 0) && ((k % drv_num == 0) || ((k + 2) % drv_num == 0)))
                    {
                        ratioAvg_border += mp_test_result->ratio[k];
                        passCount1 += 1;
                    }
                    else
                    {
                        ratioAvg += mp_test_result->ratio[k];
                        passCount += 1;
                    }

            }
        }
        else
        {
            mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;
        }
        mp_test_result->normalTestFail_check_Deltac[k] = mp_test_result->nNormalTestResultCheck[k];
    }

    msg30xx_open_judge_calc_ratio_avg(passCount, passCount1, ratioAvg, ratioAvg_border);

    memset(mp_test_result->nNormalTestResultCheck, PIN_NO_ERROR, MAX_MUTUAL_NUM);

    nNormalTestResult[1] = msg30xx_open_judge_ratio(deltac_size, by_pass_buf, result_buf);

    for (k = 0; k < MAX_MUTUAL_NUM; k++)
    {
        if (0 == mp_test_data->Goldensample_CH_0[k])
        {
            mp_test_result->nNormalTestResultCheck[k] = PIN_NO_ERROR;
            mp_test_result->normalTestFail_check_Deltac[k] = PIN_NO_ERROR;
            mp_test_result->normalTestFail_check_Ratio[k] = PIN_NO_ERROR;
        }
    }

    nRetVal =  judge_nNormalTestResult(nNormalTestResult);

    TS_LOG_INFO("%s : DeltaC: nNormalTestResult[0] = %d\n", __func__, nNormalTestResult[0]);
    TS_LOG_INFO("%s : Ratio : nNormalTestResult[1] = %d\n", __func__, nNormalTestResult[1]);

    for (i = 0; i < MAX_MUTUAL_NUM; i++)    // reduce memory operation instead of memcpy
    {
        mp_test_result->pOpenFailChannel[i] = mp_test_result->normalTestFail_check_Deltac[i];
        mp_test_result->pOpenRatioFailChannel[i] = mp_test_result->normalTestFail_check_Ratio[i];
        mp_test_result->pGolden_CH_Max_Avg[i] = mp_test_result->ratio_move[i];
    }
    mp_dump_data(mp_test_result->pOpenResultData, sense_num * drv_num, -32, 10, drv_num, "Open data", 0);
    mp_dump_data(mp_test_result->ratio, sense_num * drv_num, -32, 10, drv_num, "Open ratio data", 0);
    mp_kfree(&by_pass_buf);
    return nRetVal;
}

int msg2846a_uniformity_delta_left_right(int *condition_buf ,int *deltac_data,int index,int judge)
{
    int ret = NO_ERR, temp;

    if((index+1)%mp_test_data->sensorInfo.numDrv==1 || (index+1)%mp_test_data->sensorInfo.numDrv==0|| (index+2)%mp_test_data->sensorInfo.numDrv==0){
        return  PIN_NO_ERROR;
    }else if((condition_buf[index] == condition_buf[index+1]) && condition_buf[index]!= NO_JUDGE){
        temp = UNIF_ABS(deltac_data[index],deltac_data[index+1]);
    if(temp <= judge)
        ret= temp;
    else
        ret =0-temp;
    }else{
        return	PIN_NO_ERROR;
    }

    return ret;
}
int msg30xx_uniformity_delta_left_right(int *condition_buf ,int *deltac_data,int index,int judge)
{
    int ret = NO_ERR, temp;

    if((index+1)%mp_test_data->sensorInfo.numDrv==1 ||
            (index+1)%mp_test_data->sensorInfo.numDrv==0||
            (index+2)%mp_test_data->sensorInfo.numDrv==0)
    {
        return  PIN_NO_ERROR;
    }
    else if(condition_buf[index] == NO_JUDGE ||  condition_buf[index+1] ==NO_JUDGE)
    {
        return  PIN_NO_ERROR;
    }
    else
    {
        temp = UNIF_ABS(deltac_data[index],deltac_data[index+1]);
        if(temp <= judge)
                ret= temp;
        else
                ret =0-temp;
    }

    return ret;
}

int msg2846a_uniformity_delta_up_down(int *condition_buf,int  *deltac_data,int index, int va_cut,int judge)
{
    int ret = NO_ERR, temp;

    if((index /mp_test_data->sensorInfo.numDrv == 0) ||
        (index /mp_test_data->sensorInfo.numDrv > mp_test_data->sensorInfo.numSen-3)||
        (index /mp_test_data->sensorInfo.numDrv == va_cut)){
        ret=  PIN_NO_ERROR;
    }else if((condition_buf[index] == condition_buf[index+mp_test_data->sensorInfo.numDrv]) && (condition_buf[index] != NO_JUDGE) && (condition_buf[index] != NOTCH_JUDGE)){
        temp =UNIF_ABS(deltac_data[index],deltac_data[index+mp_test_data->sensorInfo.numDrv]);
        if( temp <= judge)
            ret= temp;
        else
            ret = 0-temp;
    }else{
        ret = PIN_NO_ERROR;
    }

    return ret;
}

int msg30xx_uniformity_delta_up_down(int *condition_buf,int  *deltac_data,int index, int va_cut,int judge)
{
    int ret = NO_ERR, temp;

    if((index /mp_test_data->sensorInfo.numDrv == 0) ||
            (index /mp_test_data->sensorInfo.numDrv > mp_test_data->sensorInfo.numSen-3)||
            (index /mp_test_data->sensorInfo.numDrv == va_cut))

    {
        ret=  PIN_NO_ERROR;
    }
    else if(condition_buf[index] == NO_JUDGE || condition_buf[index+mp_test_data->sensorInfo.numDrv] == NO_JUDGE)
    {
            ret=  PIN_NO_ERROR;
    }
    else
    {
        temp =UNIF_ABS(deltac_data[index],deltac_data[index+mp_test_data->sensorInfo.numDrv]);
        if( temp <= judge)
                ret= temp;
        else
                ret = 0-temp;
    }

    return ret;
}

int msg30xx_uniformity_bd_ratio_checck(int data, u32 ratio_max, u32 ratio_min)
{
    int ret = NO_ERR;

    if(data == -1)
        ret = -1;
    else if(data <= ratio_max && data >= ratio_min)
        ret =  data;
    else
        ret = -data;

    return  ret;
}

void msg2846a_uniformity_check_border_ratio(int *border_ratio)
{
    int i, j, index, sense_num, drv_num, ratio_max, ratio_min, va_cut;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    va_cut = 13;

    /* Check border ratio */
    for(i = 0, index = 0; i < 5; i++){
        for(j = 0; j < sense_num; j++, index++){
            if(i == 0){// top
                ratio_max = mp_test_data->uniformity_ratio_max.bd_top;
                ratio_min = mp_test_data->uniformity_ratio_min.bd_top;
            }else if(i == 1){//down
                ratio_max = mp_test_data->uniformity_ratio_max.bd_bottom;
                ratio_min = mp_test_data->uniformity_ratio_min.bd_bottom;
            }else if(i == 2){//left
                if(j <= va_cut){
                    ratio_max = mp_test_data->uniformity_ratio_max.bd_l_top;
                    ratio_min = mp_test_data->uniformity_ratio_min.bd_l_top;
                }else{
                    ratio_max = mp_test_data->uniformity_ratio_max.bd_l_bottom;
                    ratio_min = mp_test_data->uniformity_ratio_min.bd_l_bottom;
                }
            }else if(i == 3){//right
                if(j <= va_cut){
                    ratio_max = mp_test_data->uniformity_ratio_max.bd_r_top;
                    ratio_min = mp_test_data->uniformity_ratio_min.bd_r_top;
                }else{
                    ratio_max = mp_test_data->uniformity_ratio_max.bd_r_bottom;
                    ratio_min = mp_test_data->uniformity_ratio_min.bd_r_bottom;
                }
            }else if(i == 4){//notch
                ratio_max = mp_test_data->uniformity_ratio_max.notch;
                ratio_min = mp_test_data->uniformity_ratio_min.notch;
            }
            border_ratio[i*sense_num+j] = msg30xx_uniformity_bd_ratio_checck(border_ratio[i*sense_num+j],ratio_max,ratio_min);
            if(border_ratio[i*sense_num+j]	< 0 && border_ratio[i*sense_num+j] != -1)
            mp_test_result->uniformity_check_fail[2] = ITO_TEST_FAIL;
        }
    }

    TS_LOG_DEBUG("%s: border_ratio\n",__func__);
    for(i = 0, index = 0; i <5 ; i++){
         for(j = 0; j <sense_num ; j++, index ++){
            TS_LOG_DEBUG("%d,\n",border_ratio[i*sense_num+j]);
        }
    }
}

void msg30xx_uniformity_check_border_ratio(int *border_ratio)
{
    int i, j, index, sense_num, drv_num;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;

    /* Check border ratio */
    for(i = 0, index = 0; i < 4; i++)
    {
         for(j = 0; j < sense_num; j++, index++)
        {
            if(i == 0&& j >=10)
                border_ratio[i*sense_num+j] = msg30xx_uniformity_bd_ratio_checck(border_ratio[i*sense_num+j],border_ratio[4*sense_num+j],border_ratio[5*sense_num+j]);
            else
                border_ratio[i*sense_num+j] = msg30xx_uniformity_bd_ratio_checck(border_ratio[i*sense_num+j],mp_test_data->bd_va_ratio_max,mp_test_data->bd_va_ratio_min);

            if(border_ratio[i*sense_num+j]  < 0 && border_ratio[i*sense_num+j]  != -1) {
                mp_test_result->uniformity_check_fail[2] = ITO_TEST_FAIL;
                TS_LOG_ERR("uniformity bd-ratio fail\n");
            }
        }
    }

}

void msg30xx_uniformity_calc_data(int *d_data, int d_size, int *c_buf, MpUniformityPart uni_cont)
{
    int i, j, index, sense_num, drv_num, judge;
    int va_cut ;
    int *delta_LR_buf = NULL , *delta_UD_buf = NULL;
    int *data = NULL, *ratio = NULL, *num = NULL;

    /* Calculation judge data */
    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    delta_LR_buf = mp_test_result->pdelta_LR_buf;
    delta_UD_buf = mp_test_result->pdelta_UD_buf;
    data = &mp_test_result->uniformity_judge.bd_top;
    ratio = &mp_test_data->uniformity_ratio.bd_top;
    num = &uni_cont.bd_top;

    if (tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A) {
        va_cut = 13;
    } else {
        va_cut = 15;
    }

    memset(delta_LR_buf, NULL_DATA, d_size * sizeof(int));
    memset(delta_UD_buf, NULL_DATA, d_size * sizeof(int));

    for(i = 0; i <sizeof(MpUniformityPart)/sizeof(u32); i++)
            data[i] = ((data[i] * ratio[i]) / num[i] + 50) / 100;

   for(i = 0, index = 0; i < sense_num ; i++)
    {
        for(j = 0; j < drv_num ; j++, index++)
        {
            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                if(c_buf[index] == NOTCH_JUDGE){
                    judge = mp_test_result->uniformity_judge.notch;
                }
        }
        /* set judge data */
            if(i==0)
            {
                    judge = mp_test_result->uniformity_judge.bd_top;
            }
            else if (i == sense_num-1)//down
            {
                    judge =  mp_test_result->uniformity_judge.bd_bottom;
            }
            else if(j == 0 &&  i <= va_cut)//left top
            {
                    judge = mp_test_result->uniformity_judge.bd_l_top;
            }
            else if(j == 0 &&  i > va_cut)//left down
            {
                    judge = mp_test_result->uniformity_judge.bd_l_bottom;
            }
            else if(j == drv_num-1 &&   i <= va_cut) //right top
            {
                    judge = mp_test_result->uniformity_judge.bd_r_top;
            }
            else if(j == drv_num-1 &&   i > va_cut)//right down
            {
                    judge = mp_test_result->uniformity_judge.bd_r_bottom;
            }
            else if ( i <= va_cut)
            {
                    judge = mp_test_result->uniformity_judge.va_top;
            }
            else
            {
                    judge = mp_test_result->uniformity_judge.va_bottom;
            }

            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                /* left to right */
                delta_LR_buf[index] = msg2846a_uniformity_delta_left_right(c_buf, d_data, index, judge);
            }else{
                /* left to right */
                delta_LR_buf[index] = msg30xx_uniformity_delta_left_right(c_buf, d_data, index, judge);
            }
            if(delta_LR_buf[index] < 0 && delta_LR_buf[index] != PIN_NO_ERROR) {
                mp_test_result->uniformity_check_fail[0] = ITO_TEST_FAIL;
            }
            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                /* up-down */
                delta_UD_buf[index] = msg2846a_uniformity_delta_up_down(c_buf, d_data, index, va_cut, judge);
            }else{
                /* up-down */
                delta_UD_buf[index] = msg30xx_uniformity_delta_up_down(c_buf, d_data, index, va_cut, judge);
            }
            if(delta_UD_buf[index] < 0 && delta_UD_buf[index] != PIN_NO_ERROR) {
                TS_LOG_ERR("%s: Failed : delta_UD_buf[%d] = %d  \n", __func__, index, delta_UD_buf[index]);
                mp_test_result->uniformity_check_fail[1] = ITO_TEST_FAIL;
           }
        }
    }

    TS_LOG_DEBUG("%s: judge condition\n",__func__);
    for(i=0; i<sizeof(MpUniformityPart)/sizeof(u32); i++)
    {
        TS_LOG_DEBUG(KERN_CONT"judge = %d,%d,%d\n",data[i],ratio[i],num[i]);
    }
}

void msg2846a_uniformity_sum_data(int *d_data, int d_size, int *c_buf, int *border_ratio, MpUniformityPart *uni_cont)
{
    int i, j, index, va_cut ;
    int sense_num, drv_num;
    int *golden = NULL;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    golden = mp_test_data->Goldensample_CH_0;
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        va_cut = 13;
    }else{
        va_cut = 15;
    }
    for(i = 0, index = 0; i < sense_num ; i++)
    {
        for(j = 0; j < drv_num ; j++, index ++)
        {
            mp_test_result->puniformity_deltac[index] = 0 - d_data[index];

            /* Sum judge data */
            if(c_buf[index] != NO_JUDGE ){
            if(c_buf[index] == NOTCH_JUDGE){
                mp_test_result->uniformity_judge.notch += golden[index];
                uni_cont->notch++;
            if(d_data[index+drv_num]==0)
                d_data[index+drv_num]++;
                border_ratio[4*sense_num+j] = ((100* d_data[index])/d_data[index+drv_num]);
            }else if(i==0){//top
                mp_test_result->uniformity_judge.bd_top += golden[index];
                uni_cont->bd_top++;
            if(d_data[index+drv_num]==0)
                d_data[index+drv_num]++;
                border_ratio[j] = ((100* d_data[index])/d_data[index+drv_num]);
            }else if (i == sense_num-1){//down
                    mp_test_result->uniformity_judge.bd_bottom += golden[index];
                    uni_cont->bd_bottom++;
                     if(d_data[index-drv_num]==0)
                            d_data[index-drv_num]++;

                     border_ratio[sense_num+j]=((100*d_data[index])/d_data[index-drv_num]);
                }
                else if(j == 0 &&  i <= va_cut)//left top
                {
                    mp_test_result->uniformity_judge.bd_l_top += golden[index];
                    uni_cont->bd_l_top++;
                    if(d_data[index+1] == 0)
                            d_data[index+1]++;

                    border_ratio[2*sense_num+i]=((100*d_data[index])/d_data[index+1]);
                }
                else if(j == 0 &&  i > va_cut)//left down
                {
                    mp_test_result->uniformity_judge.bd_l_bottom += golden[index];
                    uni_cont->bd_l_bottom++;
                    if(d_data[index+1] == 0)
                            d_data[index+1]++;

                    border_ratio[2*sense_num+i]=((100*d_data[index])/d_data[index+1]);
                }
                else if(j == drv_num-1 &&   i <= va_cut) //right top
                {
                    mp_test_result->uniformity_judge.bd_r_top += golden[index];
                    uni_cont->bd_r_top++;
                    if(d_data[index-1] == 0)
                            d_data[index-1]++;

                    border_ratio[3*sense_num+i]= ( (100*d_data[index])/d_data[index-1]);
                }
                else if(j == drv_num-1 &&   i > va_cut)//right down
                {
                    mp_test_result->uniformity_judge.bd_r_bottom += golden[index];
                    uni_cont->bd_r_bottom++;
                    if(d_data[index-1] == 0)
                            d_data[index-1]++;

                    border_ratio[3*sense_num+i]= ((100*d_data[index])/d_data[index-1]);
                }
                else if ( i <= va_cut)
                {
                    mp_test_result->uniformity_judge.va_top += golden[index];
                    uni_cont->va_top++;
                }
                else
                {
                    mp_test_result->uniformity_judge.va_bottom += golden[index];
                    uni_cont->va_bottom++;
                }
            }
        }
    }
}
void msg30xx_uniformity_sum_data(int *d_data, int d_size, int *c_buf, int *border_ratio, MpUniformityPart *uni_cont)
{
    int i, j, index, va_cut ;
    int sense_num, drv_num;
    int *golden = NULL;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    golden = mp_test_data->Goldensample_CH_0;
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        va_cut = 13;
    }else{
        va_cut = 15;
    }
    for(i = 0, index = 0; i < sense_num ; i++)
    {
         for(j = 0; j < drv_num; j++, index ++)
        {
            mp_test_result->puniformity_deltac[index] = 0 - d_data[index];

            /* Sum judge data */
            if(c_buf[index] != NO_JUDGE )
            {
                if(i==0)//top
                {
                    mp_test_result->uniformity_judge.bd_top += golden[index];
                    uni_cont->bd_top++;

                    if(golden[index+drv_num]==0)
                            golden[index+drv_num]++;

                    border_ratio[4*sense_num+j] = ((100 * golden[index])/(golden[index+drv_num]))-100+mp_test_data->bd_va_ratio_max;
                    border_ratio[5*sense_num+j] = ((100 * golden[index])/(golden[index+drv_num]))-100+mp_test_data->bd_va_ratio_min;

                    if(d_data[index+drv_num]==0)
                            d_data[index+drv_num]++;

                    border_ratio[j] = ((100* d_data[index])/d_data[index+drv_num]);
                }
                else if (i == sense_num-1)//down
                {
                    mp_test_result->uniformity_judge.bd_bottom += golden[index];
                    uni_cont->bd_bottom++;
                     if(d_data[index-drv_num]==0)
                            d_data[index-drv_num]++;

                     border_ratio[sense_num+j]=((100*d_data[index])/d_data[index-drv_num]);
                }
                else if(j == 0 &&  i <= va_cut)//left top
                {
                    mp_test_result->uniformity_judge.bd_l_top += golden[index];
                    uni_cont->bd_l_top++;
                    if(d_data[index+1] == 0)
                            d_data[index+1]++;

                    border_ratio[2*sense_num+i]=((100*d_data[index])/d_data[index+1]);
                }
                else if(j == 0 &&  i > va_cut)//left down
                {
                    mp_test_result->uniformity_judge.bd_l_bottom += golden[index];
                    uni_cont->bd_l_bottom++;
                    if(d_data[index+1] == 0)
                            d_data[index+1]++;

                    border_ratio[2*sense_num+i]=((100*d_data[index])/d_data[index+1]);
                }
                else if(j == drv_num-1 &&   i <= va_cut) //right top
                {
                    mp_test_result->uniformity_judge.bd_r_top += golden[index];
                    uni_cont->bd_r_top++;
                    if(d_data[index-1] == 0)
                            d_data[index-1]++;

                    border_ratio[3*sense_num+i]= ( (100*d_data[index])/d_data[index-1]);
                }
                else if(j == drv_num-1 &&   i > va_cut)//right down
                {
                    mp_test_result->uniformity_judge.bd_r_bottom += golden[index];
                    uni_cont->bd_r_bottom++;
                    if(d_data[index-1] == 0)
                            d_data[index-1]++;

                    border_ratio[3*sense_num+i]= ((100*d_data[index])/d_data[index-1]);
                }
                else if ( i <= va_cut)
                {
                    mp_test_result->uniformity_judge.va_top += golden[index];
                    uni_cont->va_top++;
                }
                else
                {
                    mp_test_result->uniformity_judge.va_bottom += golden[index];
                    uni_cont->va_bottom++;
                }
            }
        }
    }
}

int msg30xx_uniformity_judge(int *deltac_data, int deltac_size)
{
    int drv_num, sense_num, node_num = deltac_size;
    int *condition_buf = NULL, *border_ratio_buf = NULL;
    MpUniformityPart  uniformity_cont;

    sense_num = mp_test_data->sensorInfo.numSen;
    drv_num = mp_test_data->sensorInfo.numDrv;
    border_ratio_buf = mp_test_result->pborder_ratio_buf;

    condition_buf = kcalloc(node_num, sizeof(int), GFP_KERNEL);
    memset(condition_buf, NULL_DATA, node_num * sizeof(int));
    memset(border_ratio_buf, -1, 6 * sense_num * sizeof(int));
    memset(&uniformity_cont.bd_top, 0, sizeof(MpUniformityPart));
    memset(&mp_test_result->uniformity_judge.bd_top, 0, sizeof(MpUniformityPart));

#ifdef ENABLE_BY_PASS_CHANNEL
    condition_buf[ADDR(0,0)]= NO_JUDGE;
    condition_buf[ADDR(0,6)]= NO_JUDGE;
    condition_buf[ADDR(0,7)]= NO_JUDGE;
    condition_buf[ADDR(0,8)]= NO_JUDGE;
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        condition_buf[ADDR(0,5)]= NO_JUDGE;
        condition_buf[ADDR(1,6)]= NOTCH_JUDGE;
        condition_buf[ADDR(1,7)]= NOTCH_JUDGE;
    }else{
        condition_buf[ADDR(0,9)]= NO_JUDGE;
        condition_buf[ADDR(1,7)]= NO_JUDGE;
        condition_buf[ADDR(1,8)]= NO_JUDGE;
    }
    condition_buf[ADDR(0,(drv_num-1))]= NO_JUDGE;
    condition_buf[ADDR((sense_num-1),0)]= NO_JUDGE;
    condition_buf[ADDR((sense_num-1),(drv_num-1))]= NO_JUDGE;
#endif

    mp_test_result->uniformity_check_fail[0] = ITO_TEST_OK;
    mp_test_result->uniformity_check_fail[1] = ITO_TEST_OK;
    mp_test_result->uniformity_check_fail[2] = ITO_TEST_OK;


    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        msg2846a_uniformity_sum_data(deltac_data, node_num, condition_buf, border_ratio_buf, &uniformity_cont);
    }else{
        msg30xx_uniformity_sum_data(deltac_data, node_num, condition_buf, border_ratio_buf, &uniformity_cont);
    }

    msg30xx_uniformity_calc_data(deltac_data, node_num, condition_buf, uniformity_cont);
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        msg2846a_uniformity_check_border_ratio(border_ratio_buf);
    }else{
        msg30xx_uniformity_check_border_ratio(border_ratio_buf);
    }
    mp_kfree(&condition_buf);

    if(mp_test_result->uniformity_check_fail[0] == ITO_TEST_OK &&
        mp_test_result->uniformity_check_fail[1] == ITO_TEST_OK &&
        mp_test_result->uniformity_check_fail[2] == ITO_TEST_OK)
        return ITO_TEST_OK;
    else {
        TS_LOG_INFO("%s : uniformity_check_fail[0] = %d\n", __func__, mp_test_result->uniformity_check_fail[0]);
        TS_LOG_INFO("%s : uniformity_check_fail[1] = %d\n", __func__, mp_test_result->uniformity_check_fail[1]);
        TS_LOG_INFO("%s : uniformity_check_fail[2] = %d\n", __func__, mp_test_result->uniformity_check_fail[2]);
        return ITO_TEST_FAIL;
    }
}

static int new_clear_switch_status(void)
{
    int rc = NO_ERR;

    rc = mstar_set_reg_16bit(0x1402, 0xFFFF);
    if(rc < 0) {
        TS_LOG_ERR("%s: set reg failed\n",__func__);
    }

    return rc;
}

static int new_polling_data_ready(void)
{
    int ret = NO_ERR;
    int timer = 500;
    int delay_time = 20;
    u16 RegData = 0;

    while(RegData != 0x7744) {
        RegData = mstar_get_reg_16bit_by_addr(0x1401, ADDRESS_MODE_16BIT);
        TS_LOG_DEBUG("%s TIMER = %d, RegData = 0x%04x\n",__func__, timer, RegData);
        mdelay(delay_time);
        timer--;
        if(timer < 0)
         break;
    }

    if(timer <= 0)
        ret = -1;

    return ret;
}

static int new_get_raw_data(s16 *pRawData, u16 count, u8 dump_time)
{
    int i, j, rc = 0, offset;
    u8 *pShotOriData = NULL;
    s32 *pShotDataAll = NULL;
    u16 fout_base_addr = 0x0, RegData = 0;
    u16 touch_info_length = 35;
    u16 data_length = count;

    TS_LOG_DEBUG("  %s() data_length = %d  \n", __func__, data_length);

    if(mp_test_data->run_type == RUN_SHORT_TEST)
        touch_info_length = 0;

    /* one byte original data */
    pShotOriData = kcalloc(data_length * 2*2 + touch_info_length, sizeof(u8), GFP_KERNEL);
    if(ERR_ALLOC_MEM(pShotOriData)) {
        TS_LOG_ERR("%s: Failed to allocate pShotOriData mem\n",__func__);
        return -1;
    }

    /* two bytes combined by above */
    pShotDataAll = kcalloc(data_length*2, sizeof(s32), GFP_KERNEL);
    if(ERR_ALLOC_MEM(pShotDataAll)) {
        TS_LOG_ERR("%s: Failed to allocate pShotDataAll mem\n",__func__);
        return -1;
    }

    /* Read dump time coeff */
    if(mp_test_data->run_type == RUN_SHORT_TEST) {
        if(dump_time == mp_test_data->SHORT_Dump1) {
            mp_test_data->SHORT_Dump1 = mstar_get_reg_16bit_by_addr(0x1018, ADDRESS_MODE_16BIT);
            TS_LOG_DEBUG("%s: Dump1 = 0x%x\n",__func__, mp_test_data->SHORT_Dump1);
        }
        if(dump_time == mp_test_data->SHORT_Dump2) {
            mp_test_data->SHORT_Dump2 = mstar_get_reg_16bit_by_addr(0x1018, ADDRESS_MODE_16BIT);
            TS_LOG_DEBUG("%s: Dump2 = 0x%x\n",__func__, mp_test_data->SHORT_Dump2);
        }
    }

    /* Read DQ base */
    RegData = mstar_get_reg_16bit_by_addr(p_mp_func->fout_data_addr, ADDRESS_MODE_16BIT);
    fout_base_addr = (int)(RegData << 2);

    TS_LOG_DEBUG("fout_base_addr = 0x%x , data_length*2 = %d\n", fout_base_addr, data_length*2);

    if(fout_base_addr <= 0) {
        TS_LOG_ERR("%s: Failed to get fout_base_addr\n",__func__);
        return -1;
    }

    rc = mstar_mp_stop_mcu();
    if (rc < 0){
        TS_LOG_ERR("%s mstar_mp_stop_mcu error\n", __func__);
        return rc;
    }

    /* Read data segmentally */
    for(offset = 0; offset < data_length * 2 + touch_info_length; offset += MAX_I2C_TRANSACTION_LENGTH_LIMIT) {

        rc = msg30xx_dbbus_read_dqmen_start();
        if (rc < 0){
            TS_LOG_ERR("%s msg30xx_dbbus_read_dqmen_start error\n", __func__);
            return rc;
        }

    if(offset == 0 && (MAX_I2C_TRANSACTION_LENGTH_LIMIT < data_length * 2 + touch_info_length))
        {
            rc = mstar_reg_get_xbit_write_4byte_value(fout_base_addr + offset, pShotOriData + offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
          if (rc < 0){
              TS_LOG_ERR("%s mstar_reg_get_xbit_write_4byte_value error\n", __func__);
              return rc;
          }
        }
        else if(offset + MAX_I2C_TRANSACTION_LENGTH_LIMIT < data_length * 2 + touch_info_length)
        {
            rc = mstar_reg_get_xbit_write_4byte_value(fout_base_addr + offset, pShotOriData + offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
          if (rc < 0){
              TS_LOG_ERR("%s mstar_reg_get_xbit_write_4byte_value error\n", __func__);
              return rc;
          }
        }
        else
        {
            rc = mstar_reg_get_xbit_write_4byte_value(fout_base_addr + offset, pShotOriData + offset, data_length * 2 + touch_info_length - offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
          if (rc < 0){
              TS_LOG_ERR("%s mstar_reg_get_xbit_write_4byte_value error\n", __func__);
              return rc;
          }
        }

        rc = msg30xx_dbbus_read_dqmem_end();
        if (rc < 0){
            TS_LOG_ERR("%s msg30xx_dbbus_read_dqmem_end error\n", __func__);
            return rc;
        }
    }


    if(mp_test_data->run_type == RUN_SHORT_TEST) {
        for (j = 0; j < data_length; j++) {
            pShotDataAll[j] = (pShotOriData[2 * j] | pShotOriData[2 * j + 1 ] << 8);
        }
    } else {
        for (j = 0; j < data_length; j++) {
            pShotDataAll[j] = (pShotOriData[2 * j + touch_info_length + 1] | pShotOriData[2 * j + touch_info_length ] << 8);
        }
    }

    for (i = 0; i < data_length; i++) {
        pRawData[i] = pShotDataAll[i];
    }

    mp_kfree(&pShotOriData);
    mp_kfree(&pShotDataAll);
    mstar_mp_start_mcu();
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	if(mp_test_data->run_type == RUN_SHORT_TEST) {
	    mstar_dbbus_wait_mcu();
	    mstar_dbbus_iic_use_bus(); //0x35
	    mstar_dbbus_iic_reshape(); //0x71
	}
    }
    return 0;
}

static int new_get_deltac(int *delt, u8 dump_time)
{
    int i;
    s16 *pRawData = NULL, count = 0;

    pRawData = kcalloc(MAX_CHANNEL_SEN * MAX_CHANNEL_DRV*2, sizeof(s16), GFP_KERNEL);

    if(ERR_ALLOC_MEM(pRawData)) {
        TS_LOG_ERR("Failed to allocate pRawData mem \n");
        return -1;
    }

    if(mp_test_data->run_type == RUN_SHORT_TEST)
	{
        if(p_mp_func->chip_type == CHIP_TYPE_MSG28XX)
		{
            count = MAX_CHANNEL_NUM_28XX;
		}
        else
		{
            count = MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX;
		}

        if(new_get_raw_data(pRawData, count, dump_time) < 0)
		{
            TS_LOG_ERR("  Get DeltaC failed!  \n");
            return -1;
        }
    }
	else
	{
        count = mp_test_data->sensorInfo.numSen * mp_test_data->sensorInfo.numDrv + mp_test_data->sensorInfo.numSen + mp_test_data->sensorInfo.numDrv;
        if(new_get_raw_data(pRawData, count, dump_time) < 0)
		{
            TS_LOG_ERR("  Get DeltaC failed!  \n");
            return -1;
        }
    }

    for (i = 0; i < count; i++)
         delt[i] = pRawData[i];


    mp_kfree(&pRawData);
    return 0;
}

static int new_send_test_cmd(u16 fmode, int mode)
{
    int ret = NO_ERR;
    u8 cmd[8] = {0};
    u8 freq, freq1, csub, cfb, chargeP, short_charge, sensorpin;
    u8 post_idle, self_sample_hi, self_sample_lo, short_sample_hi, short_sample_lo;
    u16 chargeT, dumpT;
    int postidle;

    TS_LOG_DEBUG("%s: Mode = %d  \n",__func__, mode);

    freq = mp_test_data->Open_fixed_carrier;
    freq1 = mp_test_data->Open_fixed_carrier1;
    csub = mp_test_data->Open_test_csub;
    cfb = mp_test_data->Open_test_cfb;
    chargeP = (mp_test_data->Open_test_chargepump ? 0x02 : 0x00);
    chargeT = mp_test_data->OPEN_Charge;
    dumpT = mp_test_data->OPEN_Dump;
    post_idle = mp_test_data->post_idle;
    self_sample_lo = mp_test_data->self_sample_hi;
    self_sample_hi = mp_test_data->self_sample_lo;
    short_charge = mp_test_data->SHORT_Charge;
    short_sample_hi = (mp_test_data->SHORT_sample_number & 0xFF00) >> 8;
    short_sample_lo = (mp_test_data->SHORT_sample_number & 0x00FF);
    postidle = mp_test_data->SHORT_Postidle;
    sensorpin = mp_test_data->SHORT_sensor_pin_number;

    cmd[0] = 0xF1; /* Header */
    cmd[1] = mode;

    switch(cmd[1])
    {
        case TYPE_SHORT:
            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                if(mp_test_data->SHORT_Hopping)
                    cmd[1] = TYPE_SHORT_HOPPING;
	        cmd[2] = (u8)short_charge;
	        cmd[3] = (u8)fmode;
	        cmd[4] = (u8)postidle;
	        cmd[5] = sensorpin;
	        cmd[6] = short_sample_hi;
	        cmd[7] = short_sample_lo;
            }
	    else
	    {
	            cmd[2] = 0;
	            cmd[3] = 0;
	            cmd[4] = 0;
	            cmd[5] = 0;
	            cmd[6] = 0;
	            cmd[7] = 0;
	    }
        break;

        case TYPE_SELF:
            if (chargeT == 0 || dumpT == 0) {
                chargeT = 0x18;
                dumpT = 0x16;
            }
            cmd[2] = chargeT;
            cmd[3] = dumpT;
            cmd[4] = post_idle;
            cmd[5] = self_sample_hi;
            cmd[6] = self_sample_lo;
            cmd[7] = 0x0;
        break;

        case TYPE_OPEN:
            if(fmode == MUTUAL_SINE) {
                cmd[2] = (0x01 | chargeP);

                /* Open test by each frequency */
                cmd[3] = freq;
                cmd[4] = freq1;
                cmd[5] = 0x00;
            } else {
                cmd[2] = (0x00 | chargeP);

                if (chargeT == 0 || dumpT == 0) {
                    chargeT = 0x18;
                    dumpT = 0x16;
                }

                /* Switch cap mode */
                cmd[3] = chargeT;
                cmd[4] = dumpT;
                cmd[5] = post_idle; //postidle
            }

            cmd[6] = 0x0;   //Csub, default : 0
            cmd[7] = 0x0;
        break;

        default:
            TS_LOG_ERR("Mode is invalid\n");
            ret = -1;
        break;
    }

    mp_dump_data(cmd, ARRAY_SIZE(cmd), 8, 16, 16, "Test command", 0);

    /* Writting commands via SMBus */
    if(mstar_iic_write_data(SLAVE_I2C_ID_DWI2C, &cmd[0], ARRAY_SIZE(cmd)) < 0) {
        TS_LOG_ERR("Writting commands via SMBus error !!\n");
        return -1;
    }

    mdelay(5);

    if(new_polling_data_ready() < 0) {
        TS_LOG_ERR("New Flow polling data timout !!\n");
        return -1;
    }

    /* Clear MP mode */
    mstar_set_reg_16bit(0x1402, 0xFFFF);
    return ret;
}

static int new_flow_start_test(void)
{
    int ret = NO_ERR, deltac_size = 0;
    int i = 0;
    u16 fmode = MUTUAL_MODE;
    int *deltac_data = NULL, *short_deltac_data = NULL;
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        int get_deltac_flag = 0;
    }

    switch (mp_test_data->Open_mode) {
        case 0:
            fmode = MUTUAL_MODE;
            break;
        case 1:
        case 2:
            fmode = MUTUAL_SINE;
            break;
    }

    deltac_size = MAX_MUTUAL_NUM;

    deltac_data = kcalloc(deltac_size, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(deltac_data)) {
        TS_LOG_ERR("Failed to allocate deltac_data mem \n");
        ret = -1;
        goto out;
    }

    short_deltac_data = kcalloc(deltac_size, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(short_deltac_data)) {
        TS_LOG_ERR("Failed to allocate short_deltac_data mem \n");
        ret = -1;
        goto out;
    }
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    ret = p_mp_func->enter_mp_mode();
	    if(ret < 0) {
	        TS_LOG_ERR("Failed to enter MP mode\n");
	    }

	    ret = p_mp_func->check_mp_switch();
	    if(ret < 0) {
	        TS_LOG_ERR("*** Switch FW Mode Busy, return fail ***\n");
	    }

	    if(new_clear_switch_status() < 0) {
	        TS_LOG_ERR("*** Clear Switch status fail ***\n");
	        ret = -1;
	    }
	}

    if(mp_test_data->run_type == RUN_SHORT_TEST) {
        fmode = mp_test_data->SHORT_Dump1;
        ret = new_send_test_cmd(fmode, TYPE_SHORT);
        if(ret < 0) {
            TS_LOG_ERR("Send cmd busy\n");
            goto out;
        }
    } else {
        ret = new_send_test_cmd(fmode, TYPE_SELF);
            if(ret < 0) {
            pr_err("Send cmd busy\n");
            goto out;
        }

        ret = new_send_test_cmd(fmode, TYPE_OPEN);
            if(ret < 0) {
            TS_LOG_ERR("Send cmd busy\n");
            goto out;
        }
    }
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        if(mp_test_data->get_deltac_flag == 0 && run_type != RUN_SHORT_TEST){
            ret = new_get_deltac(deltac_data, fmode);
            mp_test_data->get_deltac_flag = 1;
            for(i = 0; i < deltac_size; i++){
                mp_test_result->pdeltac_buffer[i] = deltac_data[i];
            }
        }
    }else{
        /* Get DeltaC */
        ret = new_get_deltac(deltac_data, fmode);
		if(ret < 0) {
			goto out;
		}
    }
    /* If it's the short test, get deltac one more time */
    if (mp_test_data->run_type == RUN_SHORT_TEST &&
	tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A) {
         fmode = mp_test_data->SHORT_Dump2;
         ret = new_send_test_cmd(fmode, TYPE_SHORT);
         if(ret < 0) {
             TS_LOG_ERR("Send cmd busy\n");
             goto out;
        }

        new_get_deltac(short_deltac_data, fmode);
        for(i = 0; i < MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX; i++) {
            if(abs(deltac_data[i]) >= mp_test_data->SHORT_Fout_max_1) {
                deltac_data[i] = deltac_data[i];
            } else if(abs(short_deltac_data[i]) >= mp_test_data->SHORT_Fout_max_2) {
                deltac_data[i] = short_deltac_data[i];
            } else {
                deltac_data[i] = short_deltac_data[i] - deltac_data[i];
            }
        }
        mp_dump_data(deltac_data, MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX, -32, 10, 14, "short data", 0);
        mp_dump_data(deltac_data, MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX, -32, 10, 10, "short dump2 - dump1", 0);
    }


        /* Judge values */
        if(mp_test_data->run_type == RUN_SHORT_TEST)
        {
            mp_test_result->nShortResult = ITO_TEST_OK;
            ret = p_mp_func->short_judge(deltac_data);
            mp_dump_data(deltac_data, MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX, -32, 10, 14, "short judge data", 0);
        }else if(mp_test_data->run_type == RUN_OPEN_TEST){
            mp_test_result->nOpenResult = ITO_TEST_OK;
            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                ret = p_mp_func->open_judge(mp_test_result->pdeltac_buffer, deltac_size);
            }else{
                ret = p_mp_func->open_judge(deltac_data, deltac_size);
            }
        }else{
            mp_test_result->nUniformityResult = ITO_TEST_OK;
            if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
                ret = p_mp_func->uniformity_judge(mp_test_result->pdeltac_buffer, deltac_size);
            }else{
                ret = p_mp_func->uniformity_judge(deltac_data, deltac_size);
            }
        }
        TS_LOG_DEBUG("Judge return = %d \n", ret);

out:
    mp_kfree(&deltac_data);
    mp_kfree(&short_deltac_data);
    return ret;
}

int mp_new_flow_main(int item)
{
    int i, ret = 0;
    struct tp_mp_cmd tmc[3] = {0};

    mp_test_data->run_type = item;

    mstar_finger_touch_report_disable();
    mstar_dev_hw_reset();
    mdelay(10);

    /* Enter DBbus */
    ret = mstar_dbbus_enter();
    if(ret < 0) {
        TS_LOG_ERR("%s mstar_dbbus_enter error\n", __func__);
        goto out;
    }

    /* DBbus command required by FW */
    tmc[0].data = 0x80;
    tmc[1].data = 0x82;
    tmc[2].data = 0x84;

    for(i = 0; i < ARRAY_SIZE(tmc); i++) {
        ret = mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &tmc[i].data, 1);
        if(ret < 0) {
            TS_LOG_ERR("%s access_command(0x%x) error\n", __func__,tmc[i].data);
            goto out;
        }
    }

    ret = mstar_set_reg_16bit_off(0x1E08, BIT15);
    if(ret < 0) {
        TS_LOG_ERR("%s mstar_set_reg_16bit_off(0x1E08, BIT15) error\n", __func__);
        goto out;
    }
    ret = mstar_dbbus_i2c_response_ack();

    ret = p_mp_func->enter_mp_mode();
    if(ret < 0) {
        TS_LOG_ERR("Failed to enter MP mode\n");
        goto out;
    }

    ret = p_mp_func->check_mp_switch();
    if(ret < 0) {
        TS_LOG_ERR("  Switch FW Mode Busy, return fail  \n");
        goto out;
    }

    if(new_clear_switch_status() < 0) {
        TS_LOG_ERR("  Clear Switch status fail  \n");
        ret = -1;
        goto out;
    }

    ret = new_flow_start_test();

out:
    /* Exit DBbus */
    mstar_set_reg_16bit_on(0x1E04, BIT15);
    mstar_iic_write_data(SLAVE_I2C_ID_DBBUS, &tmc[2].data, 1);
    mstar_dbbus_exit();

    mstar_dev_hw_reset();
    mdelay(10);
    mstar_finger_touch_report_enable();
    return ret;
}

int mp_test_data_avg(int *data, int len)
{
    int i = 0, data_sum = 0;
    for(i = 0; i < len; i++)
        data_sum += data[i];
    return data_sum/len;
}

static void mp_calc_golden_range_new(int *goldenTbl,int weight_up,int weight_low, int *maxTbl, int *minTbl,int length)
{
    int i = 0;
    for(i = 0; i < length; i++)
    {
        maxTbl[i] = (goldenTbl[i] * (10000 + weight_up)) / 10000;
        minTbl[i] = (goldenTbl[i] * (10000 + weight_low)) / 10000;
    }
}

static void mp_calc_golden_range(int *goldenTbl, u16 weight, u16 golden_up, int *maxTbl, int *minTbl, int length)
{
	int i, value = 0, value_up = 0;
	int allnode_number = mp_test_data->sensorInfo.numDrv * mp_test_data->sensorInfo.numSen;
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    int j = 0, shift_value = 0, weight_up = golden_up, weight_low = 0, weight_notch_up = 0, weight_notch_low = 0;
	    int golden_Cbg = 0;
	    int notch_node[] = { 0, 6, 7, 8, 9, 15, 23, 24, 512, 527 };
	    bool notch_flag = false;

	    golden_Cbg = mp_test_data_avg(goldenTbl, allnode_number);

	    TS_LOG_DEBUG("%s: golden_Cbg = %d, allnode_number = %d\n",
	        __func__, golden_Cbg, allnode_number);

	    mp_dump_data(goldenTbl, allnode_number, -32, 10, 16, "goldenTbl", 0);

	    if(golden_Cbg == 0) {
	        shift_value = 0;
	    } else {
	        shift_value = (int)((((mp_test_data->Open_Swt * 10000) / golden_Cbg - 10000) * ( mp_test_data->Open_Cbg2deltac_ratio))/10);
	    }

	    if(mp_test_data->Open_Swt > golden_Cbg) {
	        TS_LOG_DEBUG("%s: shift_value = %d\n",__func__, shift_value);
	        weight_up = shift_value + mp_test_data->Open_Csig_Vari_Up * 100;
	        weight_low = shift_value + mp_test_data->Open_Csig_Vari_Low * 100;
	        weight_notch_up = shift_value + mp_test_data->Open_Csig_Vari_Notch_Up * 100;
	        weight_notch_low = shift_value + mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	    } else if(mp_test_data->Open_Swt < golden_Cbg) {
	        TS_LOG_DEBUG("%s: shift_value = %d\n",__func__, shift_value);
	        weight_up = mp_test_data->Open_Csig_Vari_Up * 100;
	        weight_low = shift_value + mp_test_data->Open_Csig_Vari_Low * 100;
	        weight_notch_up = mp_test_data->Open_Csig_Vari_Notch_Up * 100;
	        weight_notch_low = shift_value + mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	    } else {
	        TS_LOG_DEBUG("%s: shift_value = %d\n",__func__, shift_value);
	        weight_up = mp_test_data->Open_Csig_Vari_Up * 100;
	        weight_low = mp_test_data->Open_Csig_Vari_Low * 100;
	        weight_notch_up = mp_test_data->Open_Csig_Vari_Notch_Up * 100;
	        weight_notch_low = mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	    }

	    mp_calc_golden_range_new(goldenTbl, weight_up, weight_low, maxTbl, minTbl, length);

	    TS_LOG_DEBUG("%s: golden_Cbg = %d\n",__func__, golden_Cbg);
	    TS_LOG_DEBUG("%s: weight_up = %d\n",__func__, weight_up);
	    TS_LOG_DEBUG("%s: weight_low = %d\n",__func__, weight_low);
	    TS_LOG_DEBUG("%s: weight_notch_up = %d\n",__func__, weight_notch_up);
	    TS_LOG_DEBUG("%s: weight_notch_low = %d\n",__func__, weight_notch_low);

	    for (i = 0; i < length; i++) {
	        for(j = 0; j < sizeof(notch_node); j++) {
	            if(i == notch_node[j]) {
	                notch_flag = true;
	                break;
	            }
	        }

	        if(notch_flag) {
	            maxTbl[i] = abs(goldenTbl[i]) * (10000 + weight_notch_up) / 10000;
	            minTbl[i] = abs(goldenTbl[i]) * (10000 - weight_notch_low) / 10000;
	        } else {
	            value = (int)weight * abs(goldenTbl[i]) / 100;
	            value_up = (int)weight_up * abs(goldenTbl[i]) / 10000;

	            maxTbl[i] = goldenTbl[i] + value + value_up;
	            minTbl[i] = goldenTbl[i] - value;
	        }
	        notch_flag = false;
	    }
	}
	else
	{
	    for (i = 0; i < length; i++) {
	        value = (int)weight * abs(goldenTbl[i]) / 100;
	        value_up = (int)golden_up * abs(goldenTbl[i]) / 100;

	        maxTbl[i] = goldenTbl[i] + value + value_up;
	        minTbl[i] = goldenTbl[i] - value;
	    }
	}
    mp_dump_data(maxTbl, allnode_number, -32, 10, 16, "maxTbl", 0);
    mp_dump_data(minTbl, allnode_number, -32, 10, 16, "minTbl", 0);
}


/*
 * This parser accpets the size of 600*512 to store values from ini file.
 * It would be doubled in kernel as key and value seprately.
 * Strongly suggest that do not modify the size unless you know how much
 * size of your kernel stack is.
 */
static int isdigit_t(int x)
{
    if(((x <= '9') && (x >= '0')) || (x=='.'))
        return 1;
    else
        return 0;
}

static int isspace_t(int x)
{
    if(x==' '||x=='\t'||x=='\n'||x=='\f'||x=='\b'||x=='\r')
        return 1;
    else
        return 0;
}

static long atol_t(char *nptr)
{
    int c; /* current char */
    long total; /* current total */
    int sign; /* if ''-'', then negative, otherwise positive */
    /* skip whitespace */
    while ( isspace_t((int)(unsigned char)*nptr) )
        ++nptr;
    c = (int)(unsigned char)*nptr++;
    sign = c; /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++; /* skip sign */
    total = 0;
    while (isdigit_t(c)) {
        if(c == '.')
            c = (int)(unsigned char)*nptr++;

        total = 10 * total + (c - '0'); /* accumulate digit */
        c = (int)(unsigned char)*nptr++; /* get next char */
    }
    if (sign == '-')
        return -total;
    else
        return total; /* return result, negated if necessary */
}

int ms_atoi(char *nptr)
{
    return (int)atol_t(nptr);
}

int ms_ini_file_get_line(char *filedata, char *buffer, int maxlen)
{
    int i=0;
    int j=0;
    int iRetNum=-1;
    char ch1='\0';

    for(i=0, j=0; i<maxlen; j++) {
        ch1 = filedata[j];
        iRetNum = j+1;
        if(ch1 == '\n' || ch1 == '\r') //line end
        {
            ch1 = filedata[j+1];
            if(ch1 == '\n' || ch1 == '\r')
            {
                iRetNum++;
            }

            break;
        }else if(ch1 == 0x00)
        {
            iRetNum = -1;
            break; //file end
        }
        else
        {
            buffer[i++] = ch1;
        }
    }
    buffer[i] = '\0';

    return iRetNum;
}

static char *ms_ini_str_trim_r(char * buf)
{
    int len,i;
    char tmp[1024] = {0};

    len = strlen(buf);

    for(i = 0;i < len;i++) {
        if (buf[i] !=' ')
            break;
    }
    if (i < len) {
        strncpy(tmp,(buf+i),(len-i));
    }
    strncpy(buf,tmp,len);
    return buf;
}

static char *ms_ini_str_trim_l(char * buf)
{
    int len,i;
    char tmp[1024];

    memset(tmp, 0, sizeof(tmp));
    len = strlen(buf);

    memset(tmp,0x00,len);

    for(i = 0;i < len;i++) {
        if (buf[len-i-1] !=' ')
            break;
    }
    if (i < len) {
        strncpy(tmp,buf,len-i);
    }
    strncpy(buf,tmp,len);
    return buf;
}

static void ms_init_key_data(void)
{
    int i = 0;

    mp_test_data->ini_items = 0;

    for(i = 0; i < PARSER_MAX_KEY_NUM; i++) {
        memset(ms_ini_file_data[i].pSectionName, 0, PARSER_MAX_KEY_NAME_LEN);
        memset(ms_ini_file_data[i].pKeyName, 0, PARSER_MAX_KEY_NAME_LEN);
        memset(ms_ini_file_data[i].pKeyValue, 0, PARSER_MAX_KEY_VALUE_LEN);
        ms_ini_file_data[i].iSectionNameLen = 0;
        ms_ini_file_data[i].iKeyNameLen = 0;
        ms_ini_file_data[i].iKeyValueLen = 0;
    }
}

static int ms_ini_get_key_data(char *filedata)
{
    int i, res = 0, n = 0, dataoff = 0, iEqualSign = 0;
    char *ini_buf = NULL, *tmpSectionName = NULL;

    if (filedata == NULL) {
        TS_LOG_ERR("INI filedata is NULL\n");
        res = -EINVAL;
        goto out;
    }

    ini_buf = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
    if (ERR_ALLOC_MEM(ini_buf)) {
        TS_LOG_ERR("Failed to allocate ini_buf memory, %ld\n", PTR_ERR(ini_buf));
        res = -ENOMEM;
        goto out;
    }

    tmpSectionName = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
    if (ERR_ALLOC_MEM(tmpSectionName)) {
        TS_LOG_ERR("Failed to allocate tmpSectionName memory, %ld\n", PTR_ERR(tmpSectionName));
        res = -ENOMEM;
        goto out;
    }

    while(1)
    {
        if(mp_test_data->ini_items > PARSER_MAX_KEY_NUM) {
            TS_LOG_ERR("MAX_KEY_NUM: Out Of Length\n");
            goto out;
        }

        n = ms_ini_file_get_line(filedata+dataoff, ini_buf, PARSER_MAX_CFG_BUF);

        if (n < 0) {
            TS_LOG_ERR("End of Line\n");
            goto out;
        }

        dataoff += n;

        n = strlen(ms_ini_str_trim_l(ms_ini_str_trim_r(ini_buf)));
        if(n == 0 || ini_buf[0] == M_CFG_NTS)
            continue;

        /* Get section names */
        if(n > 2 && ((ini_buf[0] == M_CFG_SSL && ini_buf[n-1] != M_CFG_SSR))) {
            TS_LOG_ERR("Bad Section:%s\n\n", ini_buf);
            res = -EINVAL;
            goto out;
        }

        if(ini_buf[0] == M_CFG_SSL)
        {
            ms_ini_file_data[mp_test_data->ini_items].iSectionNameLen = n-2;
            if(PARSER_MAX_KEY_NAME_LEN < ms_ini_file_data[mp_test_data->ini_items].iSectionNameLen) {
                TS_LOG_ERR("MAX_KEY_NAME_LEN: Out Of Length\n");
                res = -1;
                goto out;
            }

            ini_buf[n-1] = 0x00;
            strcpy((char *)tmpSectionName, ini_buf+1);
            TS_LOG_DEBUG("Section Name:%s, Len:%d\n", tmpSectionName, n-2);
            continue;
        }

        /* copy section's name without square brackets to its real buffer */
        strcpy( ms_ini_file_data[mp_test_data->ini_items].pSectionName, tmpSectionName);
        ms_ini_file_data[mp_test_data->ini_items].iSectionNameLen = strlen(tmpSectionName);

        iEqualSign = 0;
        for(i=0; i < n; i++){
            if(ini_buf[i] == M_CFG_EQS ) {
                iEqualSign = i;
                break;
            }
        }

        if(0 == iEqualSign)
            continue;

        /* Get key names */
        ms_ini_file_data[mp_test_data->ini_items].iKeyNameLen = iEqualSign;
        if(PARSER_MAX_KEY_NAME_LEN < ms_ini_file_data[mp_test_data->ini_items].iKeyNameLen) {
                TS_LOG_ERR("MAX_KEY_NAME_LEN: Out Of Length\n\n");
                res = -1;
                goto out;
        }

        memcpy(ms_ini_file_data[mp_test_data->ini_items].pKeyName,
            ini_buf, ms_ini_file_data[mp_test_data->ini_items].iKeyNameLen);

        /* Get a value with its key */
        ms_ini_file_data[mp_test_data->ini_items].iKeyValueLen = n-iEqualSign-1;
        if(PARSER_MAX_KEY_VALUE_LEN < ms_ini_file_data[mp_test_data->ini_items].iKeyValueLen) {
                TS_LOG_DEBUG("MAX_KEY_VALUE_LEN: Out Of Length\n\n");
                res = -1;
                goto out;
        }

        memcpy(ms_ini_file_data[mp_test_data->ini_items].pKeyValue,
            ini_buf+ iEqualSign+1, ms_ini_file_data[mp_test_data->ini_items].iKeyValueLen);

        TS_LOG_DEBUG("%s = %s\n", ms_ini_file_data[mp_test_data->ini_items].pKeyName,
            ms_ini_file_data[mp_test_data->ini_items].pKeyValue);

        mp_test_data->ini_items++;
    }

out:
    mp_kfree(&ini_buf);
    mp_kfree(&tmpSectionName);
    return res;
}

int ms_ini_split_u16_array(char * key, u16 * pBuf)
{
    char * s = key;
    char * pToken;
    int nCount = 0;
    int res;
    long s_to_long = 0;

    if(isspace_t((int)(unsigned char)*s) == 0)
    {
        while((pToken = strsep(&s, ",")) != NULL){
            res = kstrtol(pToken, 0, &s_to_long);
            if(res == 0)
                pBuf[nCount] = s_to_long;
            else
                TS_LOG_DEBUG("convert string too long, res = %d\n", res);
            nCount++;
        }
    }
    return nCount;
}

int ms_ini_split_golden(int *pBuf, int line)
{
    char *pToken = NULL;
    int nCount = 0;
    int res;
    int num_count = 0;
    long s_to_long = 0;
    char szSection[100] = {0};
    char str[100] = {0};
    char *s = NULL;

    while(num_count < line)
    {
        sprintf(szSection, "Golden_CH_%d", num_count);
        TS_LOG_DEBUG("szSection = %s \n",szSection);
        ms_get_ini_data("RULES", szSection, str);
        s = str;
        while((pToken = strsep(&s, ",")) != NULL)
        {
            res = kstrtol(pToken, 0, &s_to_long);
            if(res == 0)
                pBuf[nCount] = s_to_long;
            else
                TS_LOG_DEBUG("convert string too long, res = %d\n", res);
            nCount++;
        }
        num_count++;
    }
    return nCount;
}

int ms_ini_split_u8_array(char * key, u8 * pBuf)
{
    char * s = key;
    char * pToken;
    int nCount = 0;
    int res;
    long s_to_long = 0;

    if(isspace_t((int)(unsigned char)*s) == 0)
    {
        while((pToken = strsep(&s, ".")) != NULL){
            res = kstrtol(pToken, 0, &s_to_long);
            if(res == 0)
                pBuf[nCount] = s_to_long;
            else
                TS_LOG_DEBUG("convert string too long, res = %d\n", res);
            nCount++;
        }
    }
    return nCount;
}

int ms_ini_split_int_array(char * key, int * pBuf)
{
    char * s = key;
    char * pToken;
    int nCount = 0;
    int res;
    long s_to_long = 0;

    if(isspace_t((int)(unsigned char)*s) == 0)
    {
        while((pToken = strsep(&s, ",")) != NULL)
        {
            res = kstrtol(pToken, 0, &s_to_long);
            if(res == 0)
                pBuf[nCount] = s_to_long;
            else
                TS_LOG_DEBUG("convert string too long, res = %d\n", res);
            nCount++;
        }
    }
    return nCount;
}

int ms_ini_get_key(char * section, char * key, char * value)
{
    int i = 0;
    int ret = -1;
    int len = 0;

    len = strlen(key);

    for(i = 0; i < mp_test_data->ini_items; i++) {
            if(strncmp(section, ms_ini_file_data[i].pSectionName,
                 ms_ini_file_data[i].iSectionNameLen) != 0)
                 continue;

            if(strncmp(key, ms_ini_file_data[i].pKeyName, len) == 0) {
                memcpy(value, ms_ini_file_data[i].pKeyValue, ms_ini_file_data[i].iKeyValueLen);
                TS_LOG_DEBUG("key = %s, value:%s\n",ms_ini_file_data[i].pKeyName, value);
                ret = 0;
                break;
            }
    }

    return ret;
}

int ms_get_ini_data(char *section, char *ItemName, char *returnValue)
{
    char value[512] = {0};
    int len = 0;

    if(returnValue == NULL) {
        TS_LOG_ERR("returnValue as NULL in function\n");
        return 0;
    }

    if(ms_ini_get_key(section, ItemName, value) < 0)
    {
        sprintf(returnValue, "%s", value);
        return 0;
    } else {
        len = sprintf(returnValue, "%s", value);
    }

    return len;
}

int mp_parse(char *path)
{
    int res = -1, fsize = 0, retry = 0;
    char *tmp = NULL;
    struct file *f = NULL;
    mm_segment_t old_fs;
    loff_t pos = 0;
    struct kstat stat;

    TS_LOG_DEBUG("path = %s vmalloc\n", path);
	for(retry = 0; retry < 3; retry ++)
	{
	    f = filp_open(path, O_RDONLY, 0);
	    if(IS_ERR_OR_NULL(f)) {
	        TS_LOG_ERR("Failed to open the file at %ld. retry = %d\n", PTR_ERR(f), retry);

	    }
		else
		{
			TS_LOG_DEBUG("Open ini file sucesss\n");
			break;
		}
		if(retry == 2)
			return -ENOENT;
	}
    res = vfs_stat(path, &stat);
    if(res) {
        TS_LOG_ERR("%s, failed to get file stat.\n", __func__);
        res = -ENOENT;
        goto out;
    }
    fsize = stat.size;//f->f_inode->i_size;
    TS_LOG_DEBUG("ini size = %d\n", fsize);
    if(fsize <= 0) {
        filp_close(f, NULL);
        return -EINVAL;
    }

    tmp = vmalloc(fsize + 1);
    if(ERR_ALLOC_MEM(tmp)) {
        TS_LOG_ERR("Failed to allocate ini data \n");
        return -ENOMEM;
    }
    memset(tmp, 0, fsize + 1);
    /* ready to map user's memory to obtain data by reading files */
    old_fs = get_fs();
    set_fs(get_ds());
    vfs_read(f, tmp, fsize, &pos);
    set_fs(old_fs);
    ms_init_key_data();

    res = ms_ini_get_key_data(tmp);

    if (res < 0) {
        TS_LOG_ERR("Failed to get physical ini data, res = %d\n", res);
        goto out;
    }

    TS_LOG_DEBUG("Parsing INI file doen\n");

out:
    filp_close(f, NULL);
    if(!ERR_ALLOC_MEM(tmp)) {
        vfree(tmp);
        tmp = NULL;
    }
    return res;
}

static int mp_load_ini(char * pFilePath)
{
    int res = 0, nSize = 0;
    char *token = NULL, str[512]={0};
    long s_to_long = 0;
    int ana_count = 0;
    u8 ana_ver[100] = {0};
    int mapping_barker_ini = 0;

    mp_test_data = kcalloc(1, sizeof(*mp_test_data), GFP_KERNEL);
    mp_test_result = kcalloc(1, sizeof(*mp_test_result), GFP_KERNEL);
    if(ERR_ALLOC_MEM(mp_test_result) || ERR_ALLOC_MEM(mp_test_data)) {
        TS_LOG_ERR("Failed to allocate mp_test mem \n");
        return -1;
    }
    if(mp_parse(pFilePath) < 0) {
         TS_LOG_ERR("Failed to parse file = %s\n", pFilePath);
         return -1;
    }

    TS_LOG_DEBUG("Parsed %s successfully!\n", pFilePath);
    token = kmalloc(100, GFP_KERNEL);
    ms_get_ini_data("INFOMATION", "MAPPING_BARKER_INI",str);
    mapping_barker_ini = ms_atoi(str);
    TS_LOG_DEBUG(" mapping_barker_ini = %d \n", mapping_barker_ini);

    ms_get_ini_data("UI_CONFIG","OpenTest",str);
    mp_test_data->UIConfig.bOpenTest = ms_atoi(str);
    ms_get_ini_data("UI_CONFIG","ShortTest",str);
    mp_test_data->UIConfig.bShortTest = ms_atoi(str);
    ms_get_ini_data("UI_CONFIG","WpTest",str);
    mp_test_data->UIConfig.bWpTest = ms_atoi(str);
    ms_get_ini_data("UI_CONFIG","UniformityTest",str);
    mp_test_data->UIConfig.bUniformityTest = ms_atoi(str);

    mp_test_data->ana_version = kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);
    if(ERR_ALLOC_MEM(mp_test_data->ana_version)) {
        pr_err("Failed to allocate Ana mem \n");
        return -1;
    }

    ms_get_ini_data("UI_CONFIG","ANAGEN_VER", str);
    strcpy(mp_test_data->ana_version, str);
    ana_count = ms_ini_split_u8_array(mp_test_data->ana_version, ana_ver);
    TS_LOG_DEBUG("Ana count = %d , mem = %p\n", ana_count, mp_test_data->ana_version);

    ms_get_ini_data("SENSOR","DrvNum",str);
    mp_test_data->sensorInfo.numDrv = ms_atoi(str);
    ms_get_ini_data("SENSOR","SenNum",str);
    mp_test_data->sensorInfo.numSen = ms_atoi(str);
    ms_get_ini_data("SENSOR","KeyNum",str);
    mp_test_data->sensorInfo.numKey = ms_atoi(str);
    ms_get_ini_data("SENSOR","KeyLine",str);
    mp_test_data->sensorInfo.numKeyLine = ms_atoi(str);
    ms_get_ini_data("SENSOR","GrNum",str);
    mp_test_data->sensorInfo.numGr = ms_atoi(str);

    ms_get_ini_data("OPEN_TEST_N","CSUB_REF",str);
    mp_test_data->Open_test_csub = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N","CFB_REF",str);
    mp_test_data->Open_test_cfb = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N","OPEN_MODE",str);
    mp_test_data->Open_mode = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N","FIXED_CARRIER",str);
    mp_test_data->Open_fixed_carrier = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N","FIXED_CARRIER1",str);
    mp_test_data->Open_fixed_carrier1 = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N","CHARGE_PUMP",str);
    mp_test_data->Open_test_chargepump = ms_atoi(str);
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    ms_get_ini_data("OPEN_TEST_N","CBG_SWT",str);
	    mp_test_data->Open_Swt = ms_atoi(str);
	    ms_get_ini_data("OPEN_TEST_N","CBG2DELTAC_RATIO",str);
	    mp_test_data->Open_Cbg2deltac_ratio = ms_atoi(str);
	    ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_UP",str);
	    mp_test_data->Open_Csig_Vari_Up = ms_atoi(str);
	    ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_LOW",str);
	    mp_test_data->Open_Csig_Vari_Low = ms_atoi(str);
	    ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_NOTCH_UP",str);
	    mp_test_data->Open_Csig_Vari_Notch_Up = ms_atoi(str);
	    ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_NOTCH_LOW",str);
	    mp_test_data->Open_Csig_Vari_Notch_Low = ms_atoi(str);
	}
    ms_get_ini_data("INFOMATION","MutualKey",str);
    mp_test_data->Mutual_Key = ms_atoi(str);
    ms_get_ini_data("INFOMATION","Pattern_type",str);
    mp_test_data->Pattern_type = ms_atoi(str);
    ms_get_ini_data("INFOMATION","1T2R_MODEL",str);
    mp_test_data->Pattern_model = ms_atoi(str);

    ms_get_ini_data("RULES","DC_Range",str);
    mp_test_data->ToastInfo.persentDC_VA_Range = ms_atoi(str);
    ms_get_ini_data("RULES","DC_Range_UP",str);
    mp_test_data->ToastInfo.persentDC_VA_Range_up = ms_atoi(str);
    ms_get_ini_data("RULES","DC_Ratio",str);
    mp_test_data->ToastInfo.persentDC_VA_Ratio = ms_atoi(str);
    ms_get_ini_data("RULES","DC_Ratio_UP",str);
    mp_test_data->ToastInfo.persentDC_VA_Ratio_up = ms_atoi(str);
    ms_get_ini_data("RULES","DC_Border_Ratio",str);
    mp_test_data->ToastInfo.persentDC_Border_Ratio = ms_atoi(str);
    ms_get_ini_data("RULES","opentestmode",str);
    mp_test_data->Open_test_mode = ms_atoi(str);
    ms_get_ini_data("RULES","shorttestmode",str);
    mp_test_data->Short_test_mode = ms_atoi(str);

    ms_get_ini_data("BASIC","DEEP_STANDBY",str);
    mp_test_data->deep_standby = ms_atoi(str);

    ms_get_ini_data("BASIC","DEEP_STANDBY_TIMEOUT",str);
    mp_test_data->deep_standby_timeout = ms_atoi(str);

    if ((mp_test_data->Mutual_Key == 1) && (mp_test_data->Mutual_Key == 2)) {
        ms_get_ini_data("SENSOR","KEY_CH",str);
        mp_test_data->sensorInfo.KEY_CH = ms_atoi(str);
    }

    ms_get_ini_data("OPEN_TEST_N", "KEY_SETTING_BY_FW", str);
    mp_test_data->Open_KeySettingByFW = ms_atoi(str);
    ms_get_ini_data("OPEN_TEST_N", "INVERT_MODE", str);
    mp_test_data->inverter_mode = ms_atoi(str);

    ms_get_ini_data("CDTIME", "OPEN_CHARGE", str);
    mp_test_data->OPEN_Charge = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->OPEN_Charge = s_to_long;

    ms_get_ini_data("CDTIME", "OPEN_DUMP", str);
    mp_test_data->OPEN_Dump = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->OPEN_Dump = s_to_long;
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    ms_get_ini_data("CDTIME", "SHORT_POSTIDLE", str);
	    mp_test_data->SHORT_Postidle = ms_atoi(str);
	    strcpy(token, str);
	    res = kstrtol((const char *)token, 0, &s_to_long);
	    if(res == 0)
	        mp_test_data->SHORT_Postidle = s_to_long;
	}
    ms_get_ini_data("CDTIME", "SHORT_Charge", str);
    mp_test_data->SHORT_Charge = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Charge = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_Dump1", str);
    mp_test_data->SHORT_Dump1 = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Dump1 = s_to_long;

if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
    ms_get_ini_data("CDTIME", "SHORT_Dump2", str);
    mp_test_data->SHORT_Dump2 = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Dump2 = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_AVG_OK_COUNTS", str);
    mp_test_data->SHORT_AVG_count = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_AVG_count = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_CAPTURE_COUNTS", str);
    mp_test_data->SHORT_Capture_count = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Capture_count = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_FOUT_MAX_1", str);
    mp_test_data->SHORT_Fout_max_1 = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Fout_max_1 = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_FOUT_MAX_2", str);
    mp_test_data->SHORT_Fout_max_2 = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Fout_max_2 = s_to_long;

    ms_get_ini_data("CDTIME", "SHORT_HOPPING_EN", str);
    mp_test_data->SHORT_Hopping = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_Hopping = s_to_long;

    TS_LOG_INFO("%s:  MSTAR_2846A  \n", __func__ );
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_TOP_Ratio", str);
    mp_test_data->uniformity_ratio.bd_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_BOTTOM_Ratio", str);
    mp_test_data->uniformity_ratio.bd_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_UPPER_Ratio", str);
    mp_test_data->uniformity_ratio.bd_l_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_UPPER_Ratio", str);
    mp_test_data->uniformity_ratio.bd_r_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_LOWER_Ratio", str);
    mp_test_data->uniformity_ratio.bd_l_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_LOWER_Ratio", str);
    mp_test_data->uniformity_ratio.bd_r_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_Notch_Ratio", str);
    mp_test_data->uniformity_ratio.notch= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "VA_UPPER_Ratio", str);
    mp_test_data->uniformity_ratio.va_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "VA_LOWER_Ratio", str);
    mp_test_data->uniformity_ratio.va_bottom= ms_atoi(str);

    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Top_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Top_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Bottom_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Bottom_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_UPPER_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_l_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_UPPER_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_l_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_LOWER_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_l_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_LOWER_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_l_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_UPPER_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_r_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_UPPER_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_r_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_LOWER_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.bd_r_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_LOWER_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.bd_r_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Notch_Ratio_Max", str);
    mp_test_data->uniformity_ratio_max.notch= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Notch_Ratio_Min", str);
    mp_test_data->uniformity_ratio_min.notch= ms_atoi(str);

    ms_get_ini_data("SHORT_TEST_N", "SHORT_SENSOR_PIN_NUM", str);
    mp_test_data->SHORT_sensor_pin_number= ms_atoi(str);
    ms_get_ini_data("SHORT_TEST_N", "SHORT_SAMPLE_NUM", str);
    mp_test_data->OPEN_Dump = ms_atoi(str);
    strcpy(token, str);
    res = kstrtol((const char *)token, 0, &s_to_long);
    if(res == 0)
        mp_test_data->SHORT_sample_number = s_to_long;
}else{
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_TOP_Ratio", str);
    mp_test_data->uniformity_ratio.bd_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_BOTTOM_Ratio", str);
    mp_test_data->uniformity_ratio.bd_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_TOP_Ratio", str);
    mp_test_data->uniformity_ratio.bd_l_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_TOP_Ratio", str);
    mp_test_data->uniformity_ratio.bd_r_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_BOTTOM_Ratio", str);
    mp_test_data->uniformity_ratio.bd_l_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_BOTTOM_Ratio", str);
    mp_test_data->uniformity_ratio.bd_r_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "VA_TOP_Ratio", str);
    mp_test_data->uniformity_ratio.va_top= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "VA_BOTTOM_Ratio", str);
    mp_test_data->uniformity_ratio.va_bottom= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_VA_Ratio_Max", str);
    mp_test_data->bd_va_ratio_max= ms_atoi(str);
    ms_get_ini_data("UNIFORMITY_TEST_N", "BD_VA_Ratio_Min", str);
    mp_test_data->bd_va_ratio_min= ms_atoi(str);
}

    TS_LOG_DEBUG("ANAGEN_VER:    [%s]\n", mp_test_data->ana_version);
    TS_LOG_DEBUG("OpenTest:      [%d]\n", mp_test_data->UIConfig.bOpenTest);
    TS_LOG_DEBUG("ShortTest:      [%d]\n", mp_test_data->UIConfig.bShortTest);
    TS_LOG_DEBUG("WPTest:      [%d]\n", mp_test_data->UIConfig.bWpTest);
    TS_LOG_DEBUG("DrvNum:      [%d]\n", mp_test_data->sensorInfo.numDrv);
    TS_LOG_DEBUG("SenNum:      [%d]\n", mp_test_data->sensorInfo.numSen);
    TS_LOG_DEBUG("KeyNum:      [%d]\n", mp_test_data->sensorInfo.numKey);
    TS_LOG_DEBUG("KeyLine:      [%d]\n", mp_test_data->sensorInfo.numKeyLine);
    TS_LOG_DEBUG("DEEP_STANDBY = [%d] \n", mp_test_data->deep_standby);
    TS_LOG_DEBUG("GrNum:      [%d]\n", mp_test_data->sensorInfo.numGr);
    TS_LOG_DEBUG("CSUB_REF:      [%d]\n", mp_test_data->Open_test_csub);
    TS_LOG_DEBUG("CFB_REF:      [%d]\n", mp_test_data->Open_test_cfb);
    TS_LOG_DEBUG("OPEN_MODE:      [%d]\n", mp_test_data->Open_mode);
    TS_LOG_DEBUG("FIXED_CARRIER:      [%d]\n", mp_test_data->Open_fixed_carrier);
    TS_LOG_DEBUG("FIXED_CARRIER1:      [%d]\n", mp_test_data->Open_fixed_carrier1);
    TS_LOG_DEBUG("CHARGE_PUMP:      [%d]\n", mp_test_data->Open_test_chargepump);
    TS_LOG_DEBUG("MutualKey:      [%d]\n", mp_test_data->Mutual_Key);
    TS_LOG_DEBUG("KEY_CH:      [%d]\n", mp_test_data->sensorInfo.KEY_CH);
    TS_LOG_DEBUG("Pattern_type:      [%d]\n", mp_test_data->Pattern_type);
    TS_LOG_DEBUG("Pattern_model:      [%d]\n", mp_test_data->Pattern_model);
    TS_LOG_DEBUG("DC_Range:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Range);
    TS_LOG_DEBUG("DC_Ratio:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Ratio);
    TS_LOG_DEBUG("DC_Range_up:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Range_up);
    TS_LOG_DEBUG("DC_Ratio_up:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Ratio_up);
    TS_LOG_DEBUG("KEY_SETTING_BY_FW:      [%d]\n", mp_test_data->Open_KeySettingByFW);
    TS_LOG_DEBUG("INVERT_MODE:      [%d]\n", mp_test_data->inverter_mode);
    TS_LOG_DEBUG("DEEP_STANDBY_TIMEOUT:      [%d]\n", mp_test_data->deep_standby_timeout);
    TS_LOG_DEBUG("OPEN_CHARGE:      [%d]\n", mp_test_data->OPEN_Charge);
    TS_LOG_DEBUG("OPEN_DUMP:      [%d]\n", mp_test_data->OPEN_Dump);
    TS_LOG_DEBUG("SHORT_Charge:      [%d]\n", mp_test_data->SHORT_Charge);
    TS_LOG_DEBUG("SHORT_Dump1:      [%d]\n", mp_test_data->SHORT_Dump1);
    TS_LOG_DEBUG("BD_TOP_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_top);
    TS_LOG_DEBUG("BD_BOTTOM_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_bottom);

if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	TS_LOG_INFO("%s:  MSTAR_2846A  \n", __func__ );
    TS_LOG_DEBUG("CBG_SWT:      [%d]\n", mp_test_data->Open_Swt);
    TS_LOG_DEBUG("CBG2DELTAC_RATIO = [%d] \n", mp_test_data->Open_Cbg2deltac_ratio);
    TS_LOG_DEBUG("Csig_SUPPORT_VARI_UP:      [%d]\n", mp_test_data->Open_Csig_Vari_Up);
    TS_LOG_DEBUG("Csig_SUPPORT_VARI_LOW:      [%d]\n", mp_test_data->Open_Csig_Vari_Low);
    TS_LOG_DEBUG("Csig_SUPPORT_VARI_NOTCH_UP:      [%d]\n", mp_test_data->Open_Csig_Vari_Notch_Up);
    TS_LOG_DEBUG("Csig_SUPPORT_VARI_NOTCH_LOW:      [%d]\n", mp_test_data->Open_Csig_Vari_Notch_Low);
    TS_LOG_DEBUG("SHORT_SENSOR_PIN_NUM:      [%d]\n", mp_test_data->SHORT_sensor_pin_number);
    TS_LOG_DEBUG("SHORT_SAMPLE_NUM:      [%d]\n", mp_test_data->SHORT_sample_number);
    TS_LOG_DEBUG("SHORT_POSTIDLE:      [%d]\n", mp_test_data->SHORT_Postidle);
    TS_LOG_DEBUG("SHORT_FOUT_MAX_1:      [%d]\n", mp_test_data->SHORT_Fout_max_1);
    TS_LOG_DEBUG("SHORT_Dump2:      [%d]\n", mp_test_data->SHORT_Dump2);
    TS_LOG_DEBUG("SHORT_FOUT_MAX_2:      [%d]\n", mp_test_data->SHORT_Fout_max_2);
    TS_LOG_DEBUG("SHORT_HOPPING_EN:      [%d]\n", mp_test_data->SHORT_Hopping);
    TS_LOG_DEBUG("SHORT_CAPTURE_COUNTS:      [%d]\n", mp_test_data->SHORT_Capture_count);
    TS_LOG_DEBUG("SHORT_AVG_OK_COUNTS:      [%d]\n", mp_test_data->SHORT_AVG_count);
    TS_LOG_DEBUG("[UNIFORMITY_TEST_N]\n");
    TS_LOG_DEBUG("BD_L_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_top);
    TS_LOG_DEBUG("BD_R_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_top);
    TS_LOG_DEBUG("BD_L_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_bottom);
    TS_LOG_DEBUG("BD_R_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_bottom);
    TS_LOG_DEBUG("BD_Notch_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.notch);
    TS_LOG_DEBUG("VA_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_top);
    TS_LOG_DEBUG("VA_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_bottom);

    TS_LOG_DEBUG("BDVA_Top_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_top);
    TS_LOG_DEBUG("BDVA_Top_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_top);
    TS_LOG_DEBUG("BDVA_Bottom_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_bottom);
    TS_LOG_DEBUG("BDVA_Bottom_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_bottom);
    TS_LOG_DEBUG("BDVA_Left_UPPER_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_l_top);
    TS_LOG_DEBUG("BDVA_Left_UPPER_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_l_top);
    TS_LOG_DEBUG("BDVA_Left_LOWER_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_l_bottom);
    TS_LOG_DEBUG("BDVA_Left_LOWER_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_l_bottom);
    TS_LOG_DEBUG("BDVA_Right_UPPER_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_r_top);
    TS_LOG_DEBUG("BDVA_Right_UPPER_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_r_top);
    TS_LOG_DEBUG("BDVA_Right_LOWER_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.bd_r_bottom);
    TS_LOG_DEBUG("BDVA_Right_LOWER_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.bd_r_bottom);
    TS_LOG_DEBUG("BDVA_Notch_Ratio_Max:      [%d]\n", mp_test_data->uniformity_ratio_max.notch);
    TS_LOG_DEBUG("BDVA_Notch_Ratio_Min:      [%d]\n", mp_test_data->uniformity_ratio_min.notch);
}else{
    TS_LOG_DEBUG("BD_L_TOP_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_top);
    TS_LOG_DEBUG("BD_R_TOP_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_top);
    TS_LOG_DEBUG("BD_L_BOTTOM_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_bottom);
    TS_LOG_DEBUG("BD_R_BOTTOM_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_bottom);
    TS_LOG_DEBUG("VA_TOP_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_top);
    TS_LOG_DEBUG("VA_BOTTOM_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_bottom);
    TS_LOG_DEBUG("BD_VA_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max);
    TS_LOG_DEBUG("BD_VA_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min);
}

    if(mp_test_data->sensorInfo.numKey > 0) {
        ms_get_ini_data("SENSOR","KeyDrv_o",str);
        mp_test_data->sensorInfo.KeyDrv_o = ms_atoi(str);

        ms_get_ini_data("SENSOR","KEYSEN",str);
        mp_test_data->KeySen = kcalloc(mp_test_data->sensorInfo.numKey, sizeof(int), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->KeySen)) {
            TS_LOG_ERR("Failed to allocate mp_test_data->KeySen mem\n");
            return -1;
        }

        ms_ini_split_int_array(str, mp_test_data->KeySen);

        ms_get_ini_data("SENSOR","KEY_TYPE",str);
        mp_test_data->sensorInfo.key_type = kmalloc(64 * sizeof(char), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->sensorInfo.key_type)) {
            TS_LOG_ERR("Failed to allocate mp_test_data->sensorInfo.key_type mem\n");
            return -1;
        }

        strcpy(mp_test_data->sensorInfo.key_type, str);
    }

    mp_test_data->UIConfig.sSupportIC = kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);

    if(ERR_ALLOC_MEM(mp_test_data->UIConfig.sSupportIC)) {
        TS_LOG_ERR("Failed to allocate mp_test_data->UIConfig.sSupportIC mem\n");
        return -1;
    }

    memset(mp_test_data->UIConfig.sSupportIC, 0, FILENAME_MAX * sizeof(char));
    if(ms_get_ini_data("UI_CONFIG","SupportIC",str) != 0)
        strcpy(mp_test_data->UIConfig.sSupportIC, str);

    TS_LOG_DEBUG("SupportIC:      [%s]\n", mp_test_data->UIConfig.sSupportIC);

    mp_test_data->project_name = (char *)kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);
    if(ERR_ALLOC_MEM(mp_test_data->UIConfig.sSupportIC))
    {
        TS_LOG_ERR("Failed to allocate mp_test_data->project_name mem\n");
        return -1;
    }

    memset(mp_test_data->project_name, 0, FILENAME_MAX * sizeof(char));
    if(ms_get_ini_data("INFOMATION", "PROJECT", str) != 0)
        strcpy(mp_test_data->project_name, str);

    TS_LOG_DEBUG("PROJECT:      [%s]\n", mp_test_data->project_name);

    mp_test_data->Goldensample_CH_0 = (int *)kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0)) {
        TS_LOG_ERR("Failed to allocate mp_test_data->Goldensample_CH_0 mem\n");
        return -1;
    }

    nSize = ms_ini_split_golden(mp_test_data->Goldensample_CH_0, mp_test_data->sensorInfo.numSen);
    TS_LOG_DEBUG("The number of Golden line = %d\n",nSize);

    mp_test_data->Goldensample_CH_0_Max = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_data->Goldensample_CH_0_Max_Avg = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_data->Goldensample_CH_0_Min = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_data->Goldensample_CH_0_Min_Avg = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Max) || ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Max_Avg) ||
             ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Min)|| ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Min_Avg))
    {
        TS_LOG_ERR("Failed to allocate Goldensample mem\n");
        return -1;
    }

    if (mp_test_data->sensorInfo.numDrv && mp_test_data->sensorInfo.numSen) {

        mp_test_data->PAD2Drive = kmalloc(mp_test_data->sensorInfo.numDrv * sizeof(u16), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->PAD2Drive)) {
            TS_LOG_ERR("Failed to allocate PAD2Drive mem\n");
            return -1;
        }

        ms_get_ini_data("PAD_TABLE","DRIVE",str);
        TS_LOG_DEBUG("PAD_TABLE(DRIVE):      [%s]\n", str);
        p_mp_func->drive_len = ms_ini_split_u16_array(str, mp_test_data->PAD2Drive);

        mp_test_data->PAD2Sense = kmalloc(mp_test_data->sensorInfo.numSen * sizeof(u16), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->PAD2Sense)) {
            TS_LOG_ERR("Failed to allocate PAD2Sense mem\n");
            return -1;
        }

        ms_get_ini_data("PAD_TABLE","SENSE",str);
        TS_LOG_DEBUG("PAD_TABLE(SENSE):      [%s]\n", str);
        p_mp_func->sense_len = ms_ini_split_u16_array(str, mp_test_data->PAD2Sense);
    }

    if (mp_test_data->sensorInfo.numGr) {
        mp_test_data->PAD2GR = kmalloc(mp_test_data->sensorInfo.numGr * sizeof(u16), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->PAD2GR)) {
            TS_LOG_ERR("Failed to allocate PAD2GR mem\n");
            return -1;
        }

        ms_get_ini_data("PAD_TABLE","GR",str);
        TS_LOG_DEBUG("PAD_TABLE(GR):      [%s]\n", str);
        p_mp_func->gr_len = ms_ini_split_u16_array(str, mp_test_data->PAD2GR);
    }

    ms_get_ini_data("RULES","ICPINSHORT",str);
    mp_test_data->sensorInfo.thrsICpin = ms_atoi(str);
    TS_LOG_DEBUG("ICPINSHORT:      [%d]\n", mp_test_data->sensorInfo.thrsICpin);

    mp_kfree(&token);
    return 0;
}

static int mp_main_init_var(void)
{
    int i = 0;
    mp_test_result->i2c_status = NO_ERR;

    mp_calc_golden_range(mp_test_data->Goldensample_CH_0,
        mp_test_data->ToastInfo.persentDC_VA_Range, mp_test_data->ToastInfo.persentDC_VA_Range_up,
        mp_test_data->Goldensample_CH_0_Max, mp_test_data->Goldensample_CH_0_Min, MAX_MUTUAL_NUM);

    mp_test_result->nRatioAvg_max = (int) (100 + mp_test_data->ToastInfo.persentDC_VA_Ratio + mp_test_data->ToastInfo.persentDC_VA_Ratio_up) / 100;
    mp_test_result->nRatioAvg_min =(int) (100 - mp_test_data->ToastInfo.persentDC_VA_Ratio) / 100;
    mp_test_result->nBorder_RatioAvg_max = (int) (100 + mp_test_data->ToastInfo.persentDC_Border_Ratio + mp_test_data->ToastInfo.persentDC_VA_Ratio_up) / 100;
    mp_test_result->nBorder_RatioAvg_min = (int) (100 - mp_test_data->ToastInfo.persentDC_Border_Ratio) / 100;

    mp_test_result->pCheck_Fail =               kcalloc(TEST_ITEM_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pOpenResultData =           kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pOpenFailChannel =          kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pOpenRatioFailChannel =     kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

    mp_test_result->pGolden_CH =                kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pGolden_CH_Max =            kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pGolden_CH_Max_Avg =        kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pGolden_CH_Min =            kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pGolden_CH_Min_Avg =        kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

    mp_test_result->pShortFailChannel =         kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
    mp_test_result->pShortResultData =          kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
    mp_test_result->pICPinChannel =             kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
    mp_test_result->pICPinShortFailChannel =    kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
    mp_test_result->pICPinShortResultData =     kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);

    mp_test_result->pICPinShortRData =          kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
    mp_test_result->pShortRData =               kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);

    mp_test_result->nNormalTestResultCheck =         kcalloc(MAX_MUTUAL_NUM, sizeof(u16), GFP_KERNEL);
    mp_test_result->normalTestFail_check_Deltac =          kcalloc(MAX_MUTUAL_NUM, sizeof(u16), GFP_KERNEL);
    mp_test_result->normalTestFail_check_Ratio =             kcalloc(MAX_MUTUAL_NUM, sizeof(u16), GFP_KERNEL);
    mp_test_result->ratio_border =    kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->ratio_move =     kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

    mp_test_result->ratio =          kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->normalTestFail_check =               kcalloc(TEST_ITEM_NUM, sizeof(u16 *), GFP_KERNEL);

    mp_test_result->puniformity_deltac = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pdelta_LR_buf = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    mp_test_result->pdelta_UD_buf = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
	    mp_test_result->pborder_ratio_buf = kcalloc(7*mp_test_data->sensorInfo.numSen, sizeof(int), GFP_KERNEL);
	    mp_test_result->pdeltac_buffer = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	}
	else{
		mp_test_result->pborder_ratio_buf = kcalloc(6 * mp_test_data->sensorInfo.numSen, sizeof(int), GFP_KERNEL);
	}

    if (!ERR_ALLOC_MEM(mp_test_result->normalTestFail_check))
    {
        for (i = 0; i < TEST_ITEM_NUM; i++)
        {
            mp_test_result->normalTestFail_check[i] = kcalloc(MAX_MUTUAL_NUM, sizeof(u16), GFP_KERNEL);
                if(ERR_ALLOC_MEM(mp_test_result->normalTestFail_check[i]))
                {
                    TS_LOG_ERR("Failed to allocate test result two dimen normalTestFail_check mem\n");
                    return -1;
                }
        }
    }

    /* Check allocated memory status  */
    if(ERR_ALLOC_MEM(mp_test_result->nNormalTestResultCheck) || ERR_ALLOC_MEM(mp_test_result->normalTestFail_check_Deltac) ||
             ERR_ALLOC_MEM(mp_test_result->normalTestFail_check_Ratio) || ERR_ALLOC_MEM(mp_test_result->ratio_border ) ||
             ERR_ALLOC_MEM(mp_test_result->ratio_move) || ERR_ALLOC_MEM(mp_test_result->ratio))
    {
        TS_LOG_ERR("Failed to allocate test result mem\n");
        return -1;
    }

    if(ERR_ALLOC_MEM(mp_test_result->pCheck_Fail) || ERR_ALLOC_MEM(mp_test_result->pOpenResultData) ||
             ERR_ALLOC_MEM(mp_test_result->pOpenFailChannel) || ERR_ALLOC_MEM(mp_test_result->pOpenRatioFailChannel))
    {
        TS_LOG_ERR("Failed to allocate channels' mem\n");
        return -1;
    }

    if(ERR_ALLOC_MEM(mp_test_result->pGolden_CH) || ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Max) ||
        ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Max_Avg)|| ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Min) ||
        ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Min_Avg))
    {
        TS_LOG_ERR("Failed to allocate pGolden_CH' mem\n");
        return -1;
    }

    if(ERR_ALLOC_MEM(mp_test_result->pShortFailChannel) || ERR_ALLOC_MEM(mp_test_result->pShortResultData) ||
        ERR_ALLOC_MEM(mp_test_result->pICPinChannel)|| ERR_ALLOC_MEM(mp_test_result->pICPinShortFailChannel) ||
        ERR_ALLOC_MEM(mp_test_result->pICPinShortResultData))
    {
        TS_LOG_ERR("Failed to allocate pShortFailChannel' mem\n");
        return -1;
    }

    if(ERR_ALLOC_MEM(mp_test_result->pICPinShortRData) || ERR_ALLOC_MEM(mp_test_result->pShortRData))
    {
        TS_LOG_ERR("Failed to allocate pICPinShortRData' mem\n");
        return -1;
    }
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        if(ERR_ALLOC_MEM(mp_test_result->puniformity_deltac) || ERR_ALLOC_MEM(mp_test_result->pdelta_LR_buf) ||
	        ERR_ALLOC_MEM(mp_test_result->pdelta_UD_buf) || ERR_ALLOC_MEM(mp_test_result->pborder_ratio_buf) ||
	        ERR_ALLOC_MEM(mp_test_result->pdeltac_buffer))
		{
	        TS_LOG_ERR("[MS2846]Failed to allocate UniformityTest mem\n");
	        return -1;
	    }
	}
    else
    {
	    if(ERR_ALLOC_MEM(mp_test_result->puniformity_deltac) || ERR_ALLOC_MEM(mp_test_result->pdelta_LR_buf) ||
	        ERR_ALLOC_MEM(mp_test_result->pdelta_UD_buf) || ERR_ALLOC_MEM(mp_test_result->pborder_ratio_buf))
	    {
	        TS_LOG_ERR("[MS2856A]Failed to allocate UniformityTest mem\n");
	        return -1;
	    }
	}

    return NO_ERR;
}

static int mp_start_test(void)
{
    int i, res = 0, retry = 0;
    u8 szRxData[4] = {0};
    TS_LOG_DEBUG("================*** 0.0.0.1 ***==============\n");
    mstar_finger_touch_report_disable();
    mstar_dev_hw_reset();

    /*i2c test*/
    res = mstar_iic_read_data(SLAVE_I2C_ID_DWI2C, szRxData, 4);
    if (res < 0){
        TS_LOG_ERR("%s I2C test error\n", __func__);
        mp_test_result->i2c_status = ERROR_I2C_READ;
        goto out;
    }
    else {
        mp_test_result->i2c_status = NO_ERR;
    }
    /* Open Test */
    if(mp_test_data->UIConfig.bOpenTest == 1)
    {
        TS_LOG_DEBUG("*** Running Open Test ***\n");
        for(retry = 0; retry < MP_RETRY_TEST_TIME; retry++)
        {
            mp_test_result->nOpenResult = mp_new_flow_main(RUN_OPEN_TEST);
            if(mp_test_result->nOpenResult ==  ITO_TEST_OK)
            {
                mp_test_result->nDeltacResult = ITO_TEST_OK;
                break;
            }
            TS_LOG_ERR("MP OPEN TEST Fail, Retry %d\n", retry + 1);
        }

        if(retry >= 3) {
            mp_test_result->nDeltacResult = ITO_TEST_FAIL;
            mp_test_result->nOpenResult = ITO_TEST_FAIL;
        }

        for (i = 0; i < MAX_MUTUAL_NUM; i++)
        {
            mp_test_result->pGolden_CH[i] = mp_test_data->Goldensample_CH_0[i];
            mp_test_result->pGolden_CH_Max[i] = mp_test_data->Goldensample_CH_0_Max[i];
            mp_test_result->pGolden_CH_Min[i] = mp_test_data->Goldensample_CH_0_Min[i];
        }
    }
    else
    {
        mp_test_result->nOpenResult = ITO_NO_TEST;
    }

    /* Short Test */
    if(mp_test_data->UIConfig.bShortTest == 1)
    {
        for(retry = 0; retry < MP_RETRY_TEST_TIME; retry++)
        {
            TS_LOG_DEBUG("*** Running Short Test ***\n");
            mp_test_result->nShortResult = mp_new_flow_main(RUN_SHORT_TEST);
            if(mp_test_result->nShortResult ==  ITO_TEST_OK)
            {
                break;
            }
            TS_LOG_ERR("MP SHORT TEST Fail, Retry %d\n", retry + 1);
        }

        if(retry >= 3) {
            mp_test_result->nShortResult = ITO_TEST_FAIL;
        }
    }
    else
    {
        mp_test_result->nShortResult = ITO_NO_TEST;
    }

    /* Uniformity Test */
    if(mp_test_data->UIConfig.bUniformityTest == 1)
    {
        for(retry = 0; retry < MP_RETRY_TEST_TIME; retry++)
        {
            TS_LOG_DEBUG("*** Running Uniformity Test ***\n");
            mp_test_result->nUniformityResult = mp_new_flow_main(RUN_UNIFORMITY_TEST);
            if(mp_test_result->nUniformityResult ==  ITO_TEST_OK)
            {
                break;
            }
            TS_LOG_ERR("MP UNIFORMITY TEST Fail, Retry %d\n", retry + 1);
        }

        if(retry >= 3) {
            mp_test_result->nUniformityResult = ITO_TEST_FAIL;
        }
    }
    else
    {
            mp_test_result->nUniformityResult = ITO_NO_TEST;
    }

    /* Return final result */
    if(mp_test_result->nOpenResult == ITO_NO_TEST)
    {
        res = mp_test_result->nOpenResult;
    }
    else if(mp_test_result->nShortResult == ITO_NO_TEST)
    {
        res = mp_test_result->nShortResult;
    }
    else if(mp_test_result->nUniformityResult == ITO_NO_TEST)
    {
        res = mp_test_result->nUniformityResult;
    }
    else
    {
        if(mp_test_result->nShortResult == ITO_TEST_OK &&
            mp_test_result->nOpenResult == ITO_TEST_OK &&
            mp_test_result->nUniformityResult == ITO_TEST_OK)
            res = ITO_TEST_OK;
        else
            res = ITO_TEST_FAIL;
    }

    TS_LOG_INFO("Final Result(%d): Short = %d, Open = %d, Uniformity = %d\n",res, mp_test_result->nShortResult,
            mp_test_result->nOpenResult, mp_test_result->nUniformityResult);

out:
    mstar_dev_hw_reset();
    mdelay(300);
    mstar_finger_touch_report_enable();
    return res;
}

void mp_main_free(void)
{
    int i = 0;

    if (!ERR_ALLOC_MEM(mp_test_data)) {
        mp_kfree(&mp_test_data->ana_version);
        if(mp_test_data->sensorInfo.numKey > 0)
        {
            mp_kfree(&mp_test_data->KeySen);
            mp_kfree(&mp_test_data->sensorInfo.key_type);
            TS_LOG_DEBUG("MEM free mp_test_data->KeySen\n");
            TS_LOG_DEBUG("MEM free mp_test_data->sensorInfo.key_type\n");
        }
        mp_kfree(&mp_test_data->UIConfig.sSupportIC );
        mp_kfree(&mp_test_data->project_name);
        mp_kfree(&mp_test_data->Goldensample_CH_0 );
        mp_kfree(&mp_test_data->Goldensample_CH_0_Max);
        mp_kfree(&mp_test_data->Goldensample_CH_0_Max_Avg);
        mp_kfree(&mp_test_data->Goldensample_CH_0_Min);
        mp_kfree(&mp_test_data->Goldensample_CH_0_Min_Avg);
        mp_kfree(&mp_test_data->PAD2Drive);
        mp_kfree(&mp_test_data->PAD2Sense);
        if (mp_test_data->sensorInfo.numGr)
        {
            mp_kfree(&mp_test_data->PAD2GR);
            TS_LOG_DEBUG("MEM free mp_test_data->PAD2GR\n");
        }
    }

    if (!ERR_ALLOC_MEM(mp_test_result)) {
        mp_kfree(&mp_test_result->pCheck_Fail);
        mp_kfree(&mp_test_result->pOpenResultData);
        mp_kfree(&mp_test_result->pOpenFailChannel);
        mp_kfree(&mp_test_result->pOpenRatioFailChannel);

        mp_kfree(&mp_test_result->pGolden_CH);
        mp_kfree(&mp_test_result->pGolden_CH_Max);
        mp_kfree(&mp_test_result->pGolden_CH_Max_Avg);
        mp_kfree(&mp_test_result->pGolden_CH_Min);
        mp_kfree(&mp_test_result->pGolden_CH_Min_Avg);

        mp_kfree(&mp_test_result->pShortFailChannel);
        mp_kfree(&mp_test_result->pShortResultData);
        mp_kfree(&mp_test_result->pShortRData);

        mp_kfree(&mp_test_result->pICPinChannel);
        mp_kfree(&mp_test_result->pICPinShortFailChannel);
        mp_kfree(&mp_test_result->pICPinShortResultData);
        mp_kfree(&mp_test_result->pICPinShortRData);

        mp_kfree(&mp_test_result->nNormalTestResultCheck);
        mp_kfree(&mp_test_result->normalTestFail_check_Deltac);
        mp_kfree(&mp_test_result->normalTestFail_check_Ratio);

        mp_kfree(&mp_test_result->ratio_border);
        mp_kfree(&mp_test_result->ratio_move);
        mp_kfree(&mp_test_result->ratio);
        if (!ERR_ALLOC_MEM(mp_test_result->normalTestFail_check)) {
               for (i = 0; i < TEST_ITEM_NUM; i++)
               {
                   mp_kfree(&mp_test_result->normalTestFail_check[i]);
               }
        }
        mp_kfree(&mp_test_result->normalTestFail_check);

        mp_kfree(&mp_test_result->puniformity_deltac);
        mp_kfree(&mp_test_result->pdelta_LR_buf);
        mp_kfree(&mp_test_result->pdelta_UD_buf);
        mp_kfree(&mp_test_result->pborder_ratio_buf);
        if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
               mp_kfree(&mp_test_result->pdeltac_buffer);
        }
    }

       if (!ERR_ALLOC_MEM(ms_ini_file_data)) {
           vfree(ms_ini_file_data);
           ms_ini_file_data = NULL;
    }
    mp_kfree(&mp_test_data);
    mp_kfree(&mp_test_result);
    mp_kfree(&p_mp_func);
}

char *mstar_strncat(unsigned char *dest, char *src, size_t dest_size)
{
    size_t dest_len = 0;
    char *start_index = NULL;
    dest_len = strnlen(dest, dest_size);
    start_index = dest + dest_len;
    return strncat(&dest[dest_len], src, dest_size - dest_len - 1);
}

void mstar_set_sensor_test_result(struct ts_rawdata_info *info)
{
    if (!ERR_ALLOC_MEM(mp_test_result)){
        if (mp_test_result->i2c_status == NO_ERR) {
            mstar_strncat(info->result, MSTAR_CONNECT_TEST_PASS, TS_RAWDATA_RESULT_MAX);
        } else {
            mstar_strncat(info->result, MSTAR_CONNECT_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
        }
        if (mp_test_result->nDeltacResult == ITO_TEST_OK) {
            mstar_strncat(info->result, MSTAR_ALLNODE_TEST_PASS, TS_RAWDATA_RESULT_MAX);
        } else {
            mstar_strncat(info->result, MSTAR_ALLNODE_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
        }
        if (mp_test_result->nOpenResult == ITO_TEST_OK) {
            mstar_strncat(info->result, MSTAR_OPEN_PASS, TS_RAWDATA_RESULT_MAX);
        } else {
            mstar_strncat(info->result, MSTAR_OPEN_FAIL, TS_RAWDATA_RESULT_MAX);
        }
        if (mp_test_result->nShortResult == ITO_TEST_OK) {
            mstar_strncat(info->result, MSTAR_SHORT_TEST_PASS, TS_RAWDATA_RESULT_MAX);
        } else {
            mstar_strncat(info->result, MSTAR_SHORT_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
        }
        if (mp_test_result->nUniformityResult == ITO_TEST_OK) {
            mstar_strncat(info->result, MSTAR_UNIFORMITY_TEST_PASS, TS_RAWDATA_RESULT_MAX);
        } else {
            mstar_strncat(info->result, MSTAR_UNIFORMITY_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
        }
    }else{
            TS_LOG_ERR("%s, Failed to allocate mem force to set test fail\n", __func__);
            mstar_strncat(info->result, MSTAR_CONNECT_TEST_PASS, TS_RAWDATA_RESULT_MAX);
            mstar_strncat(info->result, MSTAR_ALLNODE_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
            mstar_strncat(info->result, MSTAR_OPEN_FAIL, TS_RAWDATA_RESULT_MAX);
            mstar_strncat(info->result, MSTAR_SHORT_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
            mstar_strncat(info->result, MSTAR_UNIFORMITY_TEST_FAIL, TS_RAWDATA_RESULT_MAX);
    }
	mstar_strncat(info->result, MSTAR_MP_TEST_END, TS_RAWDATA_RESULT_MAX);
}

void mp_send_data2kit(struct ts_rawdata_info *info)
{
    int buff_index = 0;
    int i = 0;
    int sense_num = 0;
    int drv_num = 0;

    mstar_set_sensor_test_result(info);
    if (!ERR_ALLOC_MEM(mp_test_result)){
        sense_num = mp_test_data->sensorInfo.numSen;
        drv_num = mp_test_data->sensorInfo.numDrv;
    }

    info->buff[buff_index++] = drv_num;
    info->buff[buff_index++] = sense_num;

    if ((!ERR_ALLOC_MEM(mp_test_result)) && (!ERR_ALLOC_MEM(mp_test_result->pOpenResultData)) &&
        (!ERR_ALLOC_MEM(mp_test_result->pGolden_CH)) && (!ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Max_Avg)) &&
        (!ERR_ALLOC_MEM(mp_test_result->pShortResultData)) && (!ERR_ALLOC_MEM(mp_test_result->puniformity_deltac)) &&
        (!ERR_ALLOC_MEM(mp_test_result->pdelta_LR_buf)) && (!ERR_ALLOC_MEM(mp_test_result->pdelta_UD_buf)) &&
        (!ERR_ALLOC_MEM(mp_test_result->pborder_ratio_buf))
        ) {
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pOpenResultData[i];
        }
        if(mp_test_result->nOpenResult== -1)
		{
            mp_dump_data(mp_test_result->pOpenResultData, drv_num * sense_num, -32, 10, drv_num, "pOpenResultData", 1);
		}
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pGolden_CH[i];
        }
        if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG28XXA){
            for (i = 0; i < drv_num * sense_num; i++) {
                info->buff[buff_index++] = mp_test_result->pGolden_CH_Max_Avg[i];
            }
        }
        for (i = 0; i < drv_num + sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pShortResultData[i];
        }
	    if(mp_test_result->nShortResult == -1)
		{
			mp_dump_data(mp_test_result->pShortResultData, MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX, -32, 10, 14, "pShortResultData", 1);
        }
		/* Uniformity deltac */
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->puniformity_deltac[i];
        }
        /* Uniformity Left-Right */
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pdelta_LR_buf[i];
        }
        /* Uniformity Down-Up */
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pdelta_UD_buf[i];
        }
        /* Uniformity border ratio */
        for (i = 0; i < 6 * sense_num; i++) {
            info->buff[buff_index++] = mp_test_result->pborder_ratio_buf[i];
        }
    }else{
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num + sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < drv_num * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
        for (i = 0; i < 6 * sense_num; i++) {
            info->buff[buff_index++] = 0;
        }
    }

    info->used_size = buff_index;
    TS_LOG_INFO("%s, buff_index = %d\n", __func__, buff_index);
}

int startMPTest(int nChipType, char *pFilePath)
{
    int res = 0;

    TS_LOG_INFO("nChipType = 0x%x\n",nChipType);
    TS_LOG_INFO("iniPath = %s\n", pFilePath);

    mutex_lock(&tskit_mstar_data->mutex_common);
    tskit_mstar_data->mp_testing = true;
    mutex_unlock(&tskit_mstar_data->mutex_common);

    /* Init main structure members */
    p_mp_func = kmalloc(sizeof(struct mp_main_func), GFP_KERNEL);
    if(ERR_ALLOC_MEM(p_mp_func)) {
        TS_LOG_ERR("Failed to allocate mp_func mem\n");
        res = -ENOMEM;
        goto out;
    }

    p_mp_func->chip_type = nChipType;

    if (nChipType == CHIP_TYPE_MSG58XXA) {
        p_mp_func->check_mp_switch = msg30xx_check_mp_switch;
        p_mp_func->enter_mp_mode = msg30xx_enter_mp_mode;
        p_mp_func->open_judge = msg30xx_open_judge;
        p_mp_func->short_judge = msg30xx_short_judge;
        p_mp_func->uniformity_judge = msg30xx_uniformity_judge;
        p_mp_func->max_channel_num = MAX_CHANNEL_NUM_30XX;
        p_mp_func->fout_data_addr = 0x1361;
    } else if (nChipType == CHIP_TYPE_MSG28XX) {
        p_mp_func->check_mp_switch = msg28xx_check_mp_switch;
        p_mp_func->enter_mp_mode = msg28xx_enter_mp_mode;
        p_mp_func->open_judge = msg28xx_open_judge;
        p_mp_func->short_judge = msg28xx_short_judge;
        p_mp_func->uniformity_judge = msg28xx_uniformity_judge;
        p_mp_func->max_channel_num = MAX_CHANNEL_NUM_28XX;
        p_mp_func->fout_data_addr = 0x136E;
    } else {
        TS_LOG_ERR("New MP Flow doesn't support this chip type\n");
        res = -1;
        goto out;
    }

    TS_LOG_DEBUG("sizeof(ST_INI_FILE_DATA) = %d\n", sizeof(ST_INI_FILE_DATA));
    ms_ini_file_data = (ST_INI_FILE_DATA *)vmalloc(PARSER_MAX_KEY_NUM * sizeof(ST_INI_FILE_DATA));
    if (ERR_ALLOC_MEM(ms_ini_file_data))
    {
        res = -1;
        TS_LOG_ERR("Failed to allocate  ms_ini_file_data mem\n");
        goto out;
    }

    /* Parsing ini file and prepare to run MP test */
    res = mp_load_ini(pFilePath);
    if(res < 0) {
        TS_LOG_ERR("Failed to load ini\n");
        goto out;
    }
    res = mp_main_init_var();
    if(res < 0)
    {
        res = ALLOCATE_ERROR;//allocate err
       goto out;
    }
    if(tskit_mstar_data->mstar_chip_data->ic_type == CHIP_TYPE_MSG2846A){
        mp_test_data->get_deltac_flag = 0;
        TS_LOG_INFO("%s : MSTAR_2846A \n", __func__);
    }else{
        TS_LOG_INFO("%s : MSTAR_30XX[2856] \n", __func__);
    }
    res = mp_start_test();

    /*
     * We deleted this function because client doesn't need it.
     * You can compare original version if you want to check printed data.
     */
    //mp_save_result(res);

out:
    mutex_lock(&tskit_mstar_data->mutex_common);
    tskit_mstar_data->mp_testing = false;;
    mutex_unlock(&tskit_mstar_data->mutex_common);
    return res;
}

MutualMpTestResult_t *mstar_get_mp_test_info()
{
    return mp_test_result;
}
int mstar_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
    int ret = 0;
    char ini_path[INI_FILE_PATH_LENS] = {0};
    char file_name[INI_FILE_NAME_LENS] = {0};

    if(tskit_mstar_data->fw_only_depend_on_lcd){
        snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_settings.ini",
        tskit_mstar_data->mstar_chip_data->ts_platform_data->product_name,
        MSTAR_CHIP_NAME,
        tskit_mstar_data->project_id,
        tskit_mstar_data->lcd_module_name);
    }else{
        snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_settings.ini",
        tskit_mstar_data->mstar_chip_data->ts_platform_data->product_name,
        MSTAR_CHIP_NAME,
        tskit_mstar_data->project_id,
        tskit_mstar_data->mstar_chip_data->module_name);
    }
    snprintf(ini_path, sizeof(ini_path), "%s%s", INI_PATH_PERF_ODM, file_name);
    ret = startMPTest(tskit_mstar_data->chip_type, ini_path);
    if (ret != 1) {
        TS_LOG_ERR("%s: MP Test Fail\n", __func__);
    } else {
        TS_LOG_ERR("%s: MP Test Pass\n", __func__);
    }

    mp_send_data2kit(info);
    mp_main_free();
    return 0;
}
