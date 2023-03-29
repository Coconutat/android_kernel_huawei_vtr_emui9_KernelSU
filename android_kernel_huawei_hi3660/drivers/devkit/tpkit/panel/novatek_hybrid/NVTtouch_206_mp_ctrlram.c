/* drivers/input/touchscreen/nt11206/NVTtouch_206_mp_ctrlram.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 4343 $
 * $Date: 2016-04-26 18:59:34 +0800 (Tue, 26 Apr 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include "NVTtouch_207.h"

#include <../../huawei_ts_kit.h>
#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
#include "../../huawei_ts_kit_algo.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#if NVT_HYBRID_TOUCH_MP

#define NVT_206_IC_TX_CFG_SIZE 20
#define NVT_206_IC_RX_CFG_SIZE 33


#define NVT_206_NORMAL_MODE 0x00
#define NVT_206_TEST_MODE_1 0x21
#define NVT_206_TEST_MODE_2 0x22

#define NVT_206_FILE_LEN 128
#define NVT_206_PAGE_SIZE 8192

#define NVT_206_RAW_PIPE0_ADDR  0x10528
#define NVT_206_RAW_PIPE1_ADDR  0x13528
#define NVT_206_DIFF_PIPE0_ADDR 0x10A50
#define NVT_206_DIFF_PIPE1_ADDR 0x13A50

#define NVT_206_RAW_BTN_PIPE0_ADDR  0x10F78
#define NVT_206_RAW_BTN_PIPE1_ADDR  0x13F78
#define NVT_206_BASELINE_BTN_ADDR   0x14100
#define NVT_206_DIFF_BTN_PIPE0_ADDR 0x10F80
#define NVT_206_DIFF_BTN_PIPE1_ADDR 0x13F80

#define NVT_206_SHORT_RXRX_FILE		"/sdcard/ShortTestRX-RX.csv"
#define NVT_206_SHORT_TXRX_FILE		"/sdcard/ShortTestTX-RX.csv"
#define NVT_206_SHORT_TXTX_FILE		"/sdcard/ShortTestTX-TX.csv"
#define NVT_206_OPEN_TEST_FILE 		"/sdcard/OpenTest.csv"


#define NVT_206_MP_CRITERIA_GOLDEN_FILE "/product/etc/firmware/ts/cpn_MP_Criteria_Golden.csv"
#define NVT_206_CTRLRAM_SHORT_RXRX_FILE "/product/etc/firmware/ts/cpn_CtrlRAM_Short_RXRX.csv"
#define NVT_206_CTRLRAM_SHORT_RXRX1_FILE "/product/etc/firmware/ts/cpn_CtrlRAM_Short_RXRX1.csv"
#define NVT_206_CTRLRAM_SHORT_TXRX_FILE "/product/etc/firmware/ts/cpn_CtrlRAM_Short_TXRX.csv"
#define NVT_206_CTRLRAM_SHORT_TXTX_FILE "/product/etc/firmware/ts/cpn_CtrlRAM_Short_TXTX.csv"
#define NVT_206_CTRLRAM_OPEN_MUTUAL_FILE "/product/etc/firmware/ts/cpn_CtrlRAM_Open_Mutual.csv"

static int32_t NVT_206_Tolerance_Postive_Short = 900;
static int32_t NVT_206_Tolerance_Negative_Short = -900;
static int32_t NVT_206_DiffLimitG_Postive_Short = 900;
static int32_t NVT_206_DiffLimitG_Negative_Short = -900;
static int32_t NVT_206_Tolerance_Postive_Mutual = 900;
static int32_t NVT_206_Tolerance_Negative_Mutual = -900;
static int32_t NVT_206_DiffLimitG_Postive_Mutual = 900;
static int32_t NVT_206_DiffLimitG_Negative_Mutual = -900;
static int32_t NVT_206_Rawdata_Limit_Postive_Short_RXRX = 4000000;
static int32_t NVT_206_Rawdata_Limit_Negative_Short_RXRX = -4000000;
static int32_t NVT_206_Rawdata_Limit_Postive_Short_TXRX = 4000000;
static int32_t NVT_206_Rawdata_Limit_Negative_Short_TXRX = -4000000;
static int32_t NVT_206_Rawdata_Limit_Postive_Short_TXTX = 4000000;
static int32_t NVT_206_Rawdata_Limit_Negative_Short_TXTX = -4000000;
static int32_t NVT_206_RXRX_isDummyCycle = 0;
static int32_t NVT_206_RXRX_Dummy_Count = 1;
static int32_t NVT_206_RXRX_isReadADCCheck = 1;
static int32_t NVT_206_RXRX_TestTimes = 0;
static int32_t NVT_206_RXRX_Dummy_Frames = 0;
static int32_t NVT_206_TXRX_isDummyCycle = 0;
static int32_t NVT_206_TXRX_Dummy_Count = 1;
static int32_t NVT_206_TXRX_isReadADCCheck = 1;
static int32_t NVT_206_TXRX_TestTimes = 0;
static int32_t NVT_206_TXRX_Dummy_Frames = 0;
static int32_t NVT_206_TXTX_isDummyCycle = 0;
static int32_t NVT_206_TXTX_Dummy_Count = 1;
static int32_t NVT_206_TXTX_isReadADCCheck = 1;
static int32_t NVT_206_TXTX_TestTimes = 0;
static int32_t NVT_206_TXTX_Dummy_Frames = 0;
static int32_t NVT_206_Mutual_isDummyCycle = 0;
static int32_t NVT_206_Mutual_Dummy_Count = 1;
static int32_t NVT_206_Mutual_isReadADCCheck = 1;
static int32_t NVT_206_Mutual_TestTimes = 0;
static int32_t NVT_206_Mutual_Dummy_Frames = 0;

static uint8_t NVT_206_AIN_RX[NVT_206_IC_RX_CFG_SIZE] = {29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 30, 31, 32};
static uint8_t NVT_206_AIN_TX[NVT_206_IC_TX_CFG_SIZE] = {19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
static uint8_t NVT_206_AIN_RX_Order[NVT_206_IC_RX_CFG_SIZE] = {0};
static uint8_t NVT_206_AIN_TX_Order[NVT_206_IC_TX_CFG_SIZE] = {0};

static int32_t NVT_206_BoundaryShort_RXRX[40] = {0};

static int32_t NVT_206_BoundaryShort_TXRX[40*40] = {0};

static int32_t NVT_206_BoundaryShort_TXTX[40] = {0};

static int32_t NVT_206_BoundaryOpen[40 * 40] = {0};

#define NVT_206_MaxStatisticsBuf 100
#define NVT_206_MmiTestResult 100
#define TP_MMI_RESULT_LEN TS_RAWDATA_RESULT_MAX

static int64_t NVT_206_StatisticsNum[NVT_206_MaxStatisticsBuf];
static int64_t NVT_206_StatisticsSum[NVT_206_MaxStatisticsBuf];
static int64_t nvt_206_golden_Ratio[40 * 40] = {0};
static uint8_t NVT_206_RecordResult_FWMutual[40*40] = {0};
static uint8_t NVT_206_RecordResultShort_RXRX[40] = {0};
static uint8_t NVT_206_RecordResultShort_TXRX[40 * 40] = {0};
static uint8_t NVT_206_RecordResultShort_TXTX[40] = {0};
static uint8_t NVT_206_RecordResultOpen[40 * 40] = {0};
static uint8_t NVT_206_RecordResultPixelRaw[40 * 40] = {0};
static uint8_t NVT_206_RecordResult_FW_Diff[40 * 40] = {0};

static int32_t NVT_206_TestResult_FWMutual = 0;
static int32_t NVT_206_TestResult_Short_RXRX = 0;
static int32_t NVT_206_TestResult_Short_TXRX = 0;
static int32_t NVT_206_TestResult_Short_TXTX = 0;
static int32_t NVT_206_TestResult_Open = 0;
static int32_t NVT_206_TestResult_PixelRaw = 0;
static int32_t NVT_206_TestResult_Noise = 0;
static int32_t NVT_206_TestResult_FW_Diff = 0;

static int32_t nvt_206_rawdata_fwMutual[40 * 40] = {0};
static int32_t nvt_206_rawdata_diff[40 * 40] = {0};
static int32_t nvt_206_rawdata_short_rxrx[40] = {0};
static int32_t nvt_206_rawdata_short_rxrx0[40] = {0};
static int32_t nvt_206_rawdata_short_rxrx1[40] = {0};
static int32_t nvt_206_rawdata_short_txrx[40 * 40] = {0};
static int32_t nvt_206_rawdata_short_txtx[40] = {0};
static int32_t nvt_206_rawdata_open_raw1[40 * 40] = {0};
static int32_t nvt_206_rawdata_open_raw2[40 * 40] = {0};
static int32_t nvt_206_rawdata_open[40 * 40] = {0};
static int64_t NVT_206_PixelRawCmRatio[40 * 40] = {0};
static int32_t NVT_206_PixelRawCmRatioMax[40 * 40] = {0};
static int32_t NVT_206_PixelRaw_Diff = 6000;
static int32_t NVT_206_Lmt_FW_Diff_P = 500000;
static int32_t NVT_206_Lmt_FW_Diff_N = -500000;
static int32_t NVT_206_BoundaryDiff[40 * 40] = {0};
static int32_t NVT_206_Lmt_FW_Rawdata_P[40 * 40] = {
	6773,6682,6643,6552,6526,6487,6435,6370,6422,6344,6539,6552,6552,6501,6565,6474,6526,6513,6669,6747,2184,2171,2145,2132,2119,2054,2041,2041,2041,2028,
	2145,2145,2145,2128,2119,2054,2080,2080,2132,2145,4264,4264,4290,4264,4264,4186,4199,4186,4225,4199,4355,4342,4355,4342,4316,4251,4264,4225,4290,4226,
	4628,4537,4550,4524,4511,4433,4446,4420,4446,4425,4602,4563,4576,4576,4550,4485,4498,4485,4537,4576,1443,1404,1417,1404,1391,1313,1313,1313,1313,1313,
	1443,1443,1430,1430,1434,1326,1339,1339,1352,1378,1113,1075,1080,1067,1073,980,987,978,996,969,1096,1097,1099,1088,1092,993,1000,982,1017,1011,
	1095,1066,1069,1061,1065,989,993,984,995,975,1100,1132,1135,1125,1132,1026,1035,1002,1015,1039,1534,1508,1508,1508,1495,1417,1417,1417,1417,1417,
	1534,1547,1547,1547,1560,1451,1456,1443,1443,1495,4381,4329,4342,4316,4316,4238,4251,4225,4264,4251,4368,4355,4381,4381,4368,4299,4303,4290,4329,4368,
	4342,4277,4277,4261,4251,4173,4186,4173,4199,4186,4290,4277,4316,4303,4290,4225,4238,4225,4251,4277,1482,1469,1456,1456,1443,1365,1365,1366,1365,1365,
	1469,1495,1508,1500,1508,1404,1404,1404,1404,1430,1022,1001,1000,992,995,923,924,914,922,910,1023,1044,1057,1047,1054,957,965,943,957,975,
	989,982,978,975,976,905,905,897,901,891,1023,1023,1037,1030,1040,939,945,948,944,962,1417,1404,1404,1391,1391,1313,1313,1313,1313,1313,
	1417,1443,1456,1448,1456,1352,1352,1352,1352,1391,3913,3874,3887,3861,3861,3783,3796,3783,3809,3809,3913,3900,3926,3926,3913,3845,3848,3848,3861,3900,
	3744,3679,3692,3679,3666,3591,3601,3588,3614,3601,3718,3718,3744,3757,3731,3757,3770,3718,3770,3809,1339,1326,1326,1313,1313,1236,1236,1232,1230,1230,
	1339,1378,1378,1391,1391,1378,1391,1339,1378,1404,987,988,984,980,983,909,910,897,906,896,1008,1024,1036,1034,1043,1032,1039,984,1034,1058,
	853,859,857,848,852,783,785,779,784,774,858,897,902,909,915,902,905,845,892,914,1206,1231,1226,1219,1217,1140,1141,1139,1135,1134,
	1213,1260,1265,1271,1273,1257,1266,1209,1260,1261,3250,3263,3276,3250,3250,3172,3185,3172,3198,3185,3263,3296,3315,3315,3302,3328,3328,3263,3341,3328,
	3159,3120,3133,3120,3120,3041,3055,3042,3068,3055,3172,3185,3211,3211,3198,3224,3224,3159,3224,3237,1217,1197,1193,1188,1186,1110,1110,1108,1105,1104,
	1206,1251,1256,1253,1265,1247,1257,1201,1236,1265,900,878,872,865,868,794,797,789,794,785,888,928,931,931,946,930,933,880,913,949,
	881,855,850,846,852,776,780,771,776,771,862,904,906,900,920,904,909,868,884,920,1171,1145,1140,1135,1131,1062,1060,1058,1057,1057,
	1153,1188,1191,1187,1199,1183,1195,1153,1178,1216,2743,2704,2704,2691,2691,2613,2626,2626,2639,2639,2743,2743,2769,2769,2756,2782,2782,2730,2795,2808,
	2652,2626,2652,2626,2639,2574,2561,2553,2587,2600,2769,2743,2782,2782,2769,2795,2782,2730,2769,2808,1153,1145,1147,1140,1141,1076,1066,1062,1063,1073,
	1158,1193,1199,1193,1193,1186,1196,1152,1157,1195,933,922,931,923,928,867,874,872,876,880,956,988,985,978,997,983,995,976,950,993,
};

static int32_t NVT_206_Lmt_FW_Rawdata_N[40 * 40] = {
	3647,3598,3577,3528,3514,3493,3465,3430,3458,3416,3521,3528,3528,3501,3535,3486,3514,3507,3591,3633,1176,1169,1155,1148,1141,1106,1099,1099,1099,1092,
	1155,1155,1155,1146,1141,1106,1120,1120,1148,1155,2296,2296,2310,2296,2296,2254,2261,2254,2275,2261,2345,2338,2345,2338,2324,2289,2296,2275,2310,2276,
	2492,2443,2450,2436,2429,2387,2394,2380,2394,2383,2478,2457,2464,2464,2450,2415,2422,2415,2443,2464,777,756,763,756,749,707,707,707,707,707,
	777,777,770,770,772,714,721,721,728,742,599,579,582,575,578,528,531,526,536,522,590,591,592,586,588,535,538,529,547,545,
	589,574,575,571,573,533,535,530,536,525,592,610,611,606,610,552,557,540,547,559,826,812,812,812,805,763,763,763,763,763,
	826,833,833,833,840,781,784,777,777,805,2359,2331,2338,2324,2324,2282,2289,2275,2296,2289,2352,2345,2359,2359,2352,2315,2317,2310,2331,2352,
	2338,2303,2303,2295,2289,2247,2254,2247,2261,2254,2310,2303,2324,2317,2310,2275,2282,2275,2289,2303,798,791,784,784,777,735,735,736,735,735,
	791,805,812,808,812,756,756,756,756,770,550,539,538,534,536,497,498,492,496,490,551,562,569,564,568,515,519,508,515,525,
	533,529,526,525,526,487,487,483,485,480,551,551,559,554,560,505,509,510,508,518,763,756,756,749,749,707,707,707,707,707,
	763,777,784,780,784,728,728,728,728,749,2107,2086,2093,2079,2079,2037,2044,2037,2051,2051,2107,2100,2114,2114,2107,2071,2072,2072,2079,2100,
	2016,1981,1988,1981,1974,1933,1939,1932,1946,1939,2002,2002,2016,2023,2009,2023,2030,2002,2030,2051,721,714,714,707,707,666,666,664,662,662,
	721,742,742,749,749,742,749,721,742,756,531,532,530,528,529,489,490,483,488,482,543,552,558,557,561,556,559,530,557,570,
	459,463,461,456,459,421,423,419,422,417,462,483,486,489,493,486,487,455,480,492,650,663,660,657,655,614,615,613,611,610,
	653,678,681,685,685,677,682,651,678,679,1750,1757,1764,1750,1750,1708,1715,1708,1722,1715,1757,1775,1785,1785,1778,1792,1792,1757,1799,1792,
	1701,1680,1687,1680,1680,1637,1645,1638,1652,1645,1708,1715,1729,1729,1722,1736,1736,1701,1736,1743,655,645,643,640,638,598,598,596,595,594,
	650,673,676,675,681,671,677,647,666,681,484,473,470,466,468,428,429,425,428,423,478,500,501,501,510,501,503,474,491,511,
	475,461,458,456,459,418,420,415,418,415,464,487,488,484,496,487,489,468,476,496,631,617,614,611,609,572,571,570,569,569,
	621,640,641,639,645,637,643,621,634,655,1477,1456,1456,1449,1449,1407,1414,1414,1421,1421,1477,1477,1491,1491,1484,1498,1498,1470,1505,1512,
	1428,1414,1428,1414,1421,1386,1379,1375,1393,1400,1491,1477,1498,1498,1491,1505,1498,1470,1491,1512,621,617,617,614,615,580,574,572,573,578,
	624,643,645,643,643,638,644,620,623,643,503,496,501,497,500,467,470,470,472,474,515,532,531,526,537,529,536,526,512,535,
};

struct nvt_206_test_cmd {
	uint32_t addr;
	uint8_t len;
	uint8_t data[64];
};

struct nvt_206_test_cmd *nvt_206_short_test_rxrx = NULL;
struct nvt_206_test_cmd *nvt_206_short_test_txrx = NULL;
struct nvt_206_test_cmd *nvt_206_short_test_txtx = NULL;
struct nvt_206_test_cmd *nvt_206_open_test = NULL;
static int32_t nvt_206_short_test_rxrx_num = 0;
static int32_t nvt_206_short_test_txrx_cmd_num = 0;
static int32_t nvt_206_short_test_txtx_cmd_num = 0;
static int32_t nvt_206_open_test_cmd_num = 0;

extern struct nvt_hybrid_ts_data *nvt_hybrid_ts;
extern char nvt_hybrid_product_id[];
extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];
extern void nvt_hybrid_hw_reset(void);
extern int32_t nvt_hybrid_ts_i2c_read(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern int32_t nvt_hybrid_ts_i2c_write(struct i2c_client *client, uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern void nvt_hybrid_sw_reset_idle(void);
extern void nvt_hybrid_bootloader_reset(void);
extern int32_t nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RST_COMPLETE_STATE check_reset_state);
extern int32_t nvt_hybrid_clear_fw_status(void);
extern int32_t nvt_hybrid_check_fw_status(void);
extern void nvt_hybrid_change_mode(uint8_t mode);
extern int8_t nvt_hybrid_get_fw_info(void);
extern uint8_t nvt_hybrid_get_fw_pipe(void);
extern void nvt_hybrid_read_mdata(uint32_t xdata_addr, uint32_t xdata_btn_addr);
extern void nvt_hybrid_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num);
extern int32_t NVT_Hybrid_Init_BootLoader(void);
typedef enum mp_criteria_item {
	Tol_Pos_Short,
	Tol_Neg_Short,
	DifLimG_Pos_Short,
	DifLimG_Neg_Short,
	Tol_Pos_Mutual,
	Tol_Neg_Mutual,
	DifLimG_Pos_Mutual,
	DifLimG_Neg_Mutual,
	Raw_Lim_Pos_Short_RXRX,
	Raw_Lim_Neg_Short_RXRX,
	Raw_Lim_Pos_Short_TXRX,
	Raw_Lim_Neg_Short_TXRX,
	Raw_Lim_Pos_Short_TXTX,
	Raw_Lim_Neg_Short_TXTX,
	Open_rawdata,
	RXRX_rawdata,
	TXRX_rawdata,
	TXTX_rawdata,
	Test_Cmd_Short_RXRX,
	Test_Cmd_Short_TXRX,
	Test_Cmd_Short_TXTX,
	Test_Cmd_Open,
	IC_CMD_SET_RXRX_isDummyCycle,
	IC_CMD_SET_RXRX_Dummy_Count,
	IC_CMD_SET_RXRX_isReadADCCheck,
	IC_CMD_SET_RXRX_TestTimes,
	IC_CMD_SET_RXRX_Dummy_Frames,
	IC_CMD_SET_TXRX_isDummyCycle,
	IC_CMD_SET_TXRX_Dummy_Count,
	IC_CMD_SET_TXRX_isReadADCCheck,
	IC_CMD_SET_TXRX_TestTimes,
	IC_CMD_SET_TXRX_Dummy_Frames,
	IC_CMD_SET_TXTX_isDummyCycle,
	IC_CMD_SET_TXTX_Dummy_Count,
	IC_CMD_SET_TXTX_isReadADCCheck,
	IC_CMD_SET_TXTX_TestTimes,
	IC_CMD_SET_TXTX_Dummy_Frames,
	IC_CMD_SET_Mutual_isDummyCycle,
	IC_CMD_SET_Mutual_Dummy_Count,
	IC_CMD_SET_Mutual_isReadADCCheck,
	IC_CMD_SET_Mutual_TestTimes,
	IC_CMD_SET_Mutual_Dummy_Frames,
	MP_Cri_Item_Last
} nvt_206_mp_criteria_item_e;

typedef enum test_cmd_item {
	cmd_item_addr,
	cmd_item_len,
	cmd_item_data,
	cmd_item_last
} nvt_206_test_cmd_item_e;

typedef enum ctrl_ram_item {
	CtrlRam_REGISTER,
	CtrlRam_START_ADDR,
	CtrlRam_TABLE_ADDR,
	CtrlRam_UC_ADDR,
	CtrlRam_TXMODE_ADDR,
	CtrlRam_RXMODE_ADDR,
	CtrlRam_CCMODE_ADDR,
	CtrlRam_OFFSET_ADDR,
	CtrlRam_ITEM_LAST
} nvt_206_ctrl_ram_item_e;

static char mmitest_result[NVT_206_MmiTestResult] = {0};/*store mmi test result*/

static void goto_next_line(char **ptr)
{
	if(NULL == ptr) {
		TS_LOG_ERR("%s: ptr is Null \n", __func__);
		return;
	}
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n');
	*ptr = *ptr + 1;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from = NULL;
	char *copy_to = NULL;
	if(NULL == dest || NULL == src) {
		TS_LOG_ERR("%s: dest is Null or src is Null\n", __func__);
		return;
	}
	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';
}

static void str_low(char *str)
{
	unsigned int i=0;
	if(NULL == str) {
		TS_LOG_ERR("%s: str is Null\n", __func__);
		return;
	}
	for (i = 0; i < strlen(str); i++)
		if ((str[i] >= 65) && (str[i] <= 90))
			str[i] += 32;
}

static unsigned long str_to_hex(char *p)
{
	unsigned long hex = 0;
	unsigned long length = 0;
	unsigned long shift = 0;
	unsigned char dig = 0;
	if(NULL == p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return hex;
	}
	length = strlen(p);
	str_low(p);

	if (length == 0)
		return 0;

	do {
		dig = p[--length];
		dig = dig < 'a' ? (dig - '0') : (dig - 'a' + 0xa);
		hex |= ((unsigned long)dig << shift);
		shift += 4;
	} while (length);
	return hex;
}

int32_t parse_mp_criteria_item(char **ptr, const char *item_string, int32_t *item_value)
{
	char *tmp = NULL;
	if(NULL == item_string || NULL == item_value ||NULL == ptr) {
		TS_LOG_ERR("%s: item_string is Null,or item_value is null,or ptr is null\n", __func__);
		return -1;
	}
	tmp = strstr(*ptr, item_string);
	if (tmp == NULL) {
		TS_LOG_ERR( "%s not found\n", item_string);
		return -1;
	} else {
		*ptr = tmp;
		goto_next_line(ptr);
		sscanf(*ptr, "%d,", item_value);
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen load MP criteria and golden function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t nvt_206_load_mp_default_criteria(nvt_206_mp_criteria_item_e mp_criteria_item,char *ptr )
{
	if (mp_criteria_item == Tol_Pos_Short) {
	parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Short:", &NVT_206_Tolerance_Postive_Short);
	TS_LOG_INFO( "NVT_206_Tolerance_Postive_Short = %d\n", NVT_206_Tolerance_Postive_Short);
	} else if (mp_criteria_item == Tol_Neg_Short) {
	parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Short:", &NVT_206_Tolerance_Negative_Short);
	TS_LOG_INFO( "NVT_206_Tolerance_Negativee_Short = %d\n", NVT_206_Tolerance_Negative_Short);
	} else if (mp_criteria_item == DifLimG_Pos_Short) {
	parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Short:", &NVT_206_DiffLimitG_Postive_Short);
	TS_LOG_INFO( "NVT_206_DiffLimitG_Postive_Short = %d\n", NVT_206_DiffLimitG_Postive_Short );
	} else if (mp_criteria_item == DifLimG_Neg_Short) {
	parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Short:", &NVT_206_DiffLimitG_Negative_Short);
	TS_LOG_INFO( "NVT_206_DiffLimitG_Negative_Short = %d\n", NVT_206_DiffLimitG_Negative_Short);
	} else if (mp_criteria_item == Tol_Pos_Mutual) {
	parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Mutual:", &NVT_206_Tolerance_Postive_Mutual);
	TS_LOG_INFO( "NVT_206_Tolerance_Postive_Mutual = %d\n", NVT_206_Tolerance_Postive_Mutual);
	} else if (mp_criteria_item == Tol_Neg_Mutual) {
	parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Mutual:", &NVT_206_Tolerance_Negative_Mutual);
	TS_LOG_INFO( "NVT_206_Tolerance_Negative_Mutual = %d\n", NVT_206_Tolerance_Negative_Mutual);
	} else if (mp_criteria_item == DifLimG_Pos_Mutual) {
	parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Mutual:", &NVT_206_DiffLimitG_Postive_Mutual);
	TS_LOG_INFO( "NVT_206_DiffLimitG_Postive_Mutual = %d\n", NVT_206_DiffLimitG_Postive_Mutual);
	} else if (mp_criteria_item == DifLimG_Neg_Mutual) {
	parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Mutual:", &NVT_206_DiffLimitG_Negative_Mutual);
	TS_LOG_INFO( "NVT_206_DiffLimitG_Negative_Mutual = %d\n", NVT_206_DiffLimitG_Negative_Mutual);
	} else if (mp_criteria_item == Raw_Lim_Pos_Short_RXRX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_RXRX:", &NVT_206_Rawdata_Limit_Postive_Short_RXRX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Postive_Short_RXRX = %d\n", NVT_206_Rawdata_Limit_Postive_Short_RXRX);
	} else if (mp_criteria_item == Raw_Lim_Neg_Short_RXRX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_RXRX:", &NVT_206_Rawdata_Limit_Negative_Short_RXRX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Negative_Short_RXRX = %d\n", NVT_206_Rawdata_Limit_Negative_Short_RXRX);
	} else if (mp_criteria_item == Raw_Lim_Pos_Short_TXRX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_TXRX:", &NVT_206_Rawdata_Limit_Postive_Short_TXRX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Postive_Short_TXRX = %d\n", NVT_206_Rawdata_Limit_Postive_Short_TXRX);
	} else if (mp_criteria_item == Raw_Lim_Neg_Short_TXRX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_TXRX:", &NVT_206_Rawdata_Limit_Negative_Short_TXRX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Negative_Short_TXRX = %d\n", NVT_206_Rawdata_Limit_Negative_Short_TXRX);
	} else if (mp_criteria_item == Raw_Lim_Pos_Short_TXTX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_TXTX:", &NVT_206_Rawdata_Limit_Postive_Short_TXTX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Postive_Short_TXTX = %d\n", NVT_206_Rawdata_Limit_Postive_Short_TXTX);
	} else if (mp_criteria_item == Raw_Lim_Neg_Short_TXTX) {
	parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_TXTX:", &NVT_206_Rawdata_Limit_Negative_Short_TXTX);
	TS_LOG_INFO( "NVT_206_Rawdata_Limit_Negative_Short_TXTX = %d\n", NVT_206_Rawdata_Limit_Negative_Short_TXTX);
	} else if (mp_criteria_item == IC_CMD_SET_RXRX_isDummyCycle) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_isDummyCycle:", &NVT_206_RXRX_isDummyCycle);
		TS_LOG_INFO( "NVT_206_RXRX_isDummyCycle = %d\n", NVT_206_RXRX_isDummyCycle);
	} else if (mp_criteria_item == IC_CMD_SET_RXRX_Dummy_Count) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_Dummy_Count:", &NVT_206_RXRX_Dummy_Count);
		TS_LOG_INFO( "NVT_206_RXRX_Dummy_Count = %d\n", NVT_206_RXRX_Dummy_Count);
	} else if (mp_criteria_item == IC_CMD_SET_RXRX_isReadADCCheck) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_isReadADCCheck:", &NVT_206_RXRX_isReadADCCheck);
		TS_LOG_INFO( "NVT_206_RXRX_isReadADCCheck = %d\n", NVT_206_RXRX_isReadADCCheck);
	} else if (mp_criteria_item == IC_CMD_SET_RXRX_TestTimes) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_TestTimes:", &NVT_206_RXRX_TestTimes);
		TS_LOG_INFO( "NVT_206_RXRX_TestTimes = %d\n", NVT_206_RXRX_TestTimes);
	} else if (mp_criteria_item == IC_CMD_SET_RXRX_Dummy_Frames) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_Dummy_Frames:", &NVT_206_RXRX_Dummy_Frames);
		TS_LOG_INFO( "NVT_206_RXRX_Dummy_Frames = %d\n", NVT_206_RXRX_Dummy_Frames);
	} else if (mp_criteria_item == IC_CMD_SET_TXRX_isDummyCycle) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_isDummyCycle:", &NVT_206_TXRX_isDummyCycle);
		TS_LOG_INFO( "NVT_206_TXRX_isDummyCycle = %d\n", NVT_206_TXRX_isDummyCycle);
	} else if (mp_criteria_item == IC_CMD_SET_TXRX_Dummy_Count) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_Dummy_Count:", &NVT_206_TXRX_Dummy_Count);
		TS_LOG_INFO( "NVT_206_TXRX_Dummy_Count = %d\n", NVT_206_TXRX_Dummy_Count);
	} else if (mp_criteria_item == IC_CMD_SET_TXRX_isReadADCCheck) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_isReadADCCheck:", &NVT_206_TXRX_isReadADCCheck);
		TS_LOG_INFO( "NVT_206_TXRX_isReadADCCheck = %d\n", NVT_206_TXRX_isReadADCCheck);
	} else if (mp_criteria_item == IC_CMD_SET_TXRX_TestTimes) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_TestTimes:", &NVT_206_TXRX_TestTimes);
		TS_LOG_INFO( "NVT_206_TXRX_TestTimes = %d\n", NVT_206_TXRX_TestTimes);
	} else if (mp_criteria_item == IC_CMD_SET_TXRX_Dummy_Frames) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_Dummy_Frames:", &NVT_206_TXRX_Dummy_Frames);
		TS_LOG_INFO( "NVT_206_TXRX_Dummy_Frames = %d\n", NVT_206_TXRX_Dummy_Frames);
	} else if (mp_criteria_item == IC_CMD_SET_TXTX_isDummyCycle) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_isDummyCycle:", &NVT_206_TXTX_isDummyCycle);
		TS_LOG_INFO( "NVT_206_TXTX_isDummyCycle = %d\n", NVT_206_TXTX_isDummyCycle);
	} else if (mp_criteria_item == IC_CMD_SET_TXTX_Dummy_Count) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_Dummy_Count:", &NVT_206_TXTX_Dummy_Count);
		TS_LOG_INFO( "NVT_206_TXTX_Dummy_Count = %d\n", NVT_206_TXTX_Dummy_Count);
	} else if (mp_criteria_item == IC_CMD_SET_TXTX_isReadADCCheck) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_isReadADCCheck:", &NVT_206_TXTX_isReadADCCheck);
		TS_LOG_INFO( "NVT_206_TXTX_isReadADCCheck = %d\n", NVT_206_TXTX_isReadADCCheck);
	} else if (mp_criteria_item == IC_CMD_SET_TXTX_TestTimes) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_TestTimes:", &NVT_206_TXTX_TestTimes);
		TS_LOG_INFO( "NVT_206_TXTX_TestTimes = %d\n", NVT_206_TXTX_TestTimes);
	} else if (mp_criteria_item == IC_CMD_SET_TXTX_Dummy_Frames) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_Dummy_Frames:", &NVT_206_TXTX_Dummy_Frames);
		TS_LOG_INFO( "NVT_206_TXTX_Dummy_Frames = %d\n", NVT_206_TXTX_Dummy_Frames);
	} else if (mp_criteria_item == IC_CMD_SET_Mutual_isDummyCycle) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_isDummyCycle:", &NVT_206_Mutual_isDummyCycle);
		TS_LOG_INFO( "NVT_206_Mutual_isDummyCycle = %d\n", NVT_206_Mutual_isDummyCycle);
	} else if (mp_criteria_item == IC_CMD_SET_Mutual_Dummy_Count) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_Dummy_Count:", &NVT_206_Mutual_Dummy_Count);
		TS_LOG_INFO( "NVT_206_Mutual_Dummy_Count = %d\n", NVT_206_Mutual_Dummy_Count);
	} else if (mp_criteria_item == IC_CMD_SET_Mutual_isReadADCCheck) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_isReadADCCheck:", &NVT_206_Mutual_isReadADCCheck);
		TS_LOG_INFO( "NVT_206_Mutual_isReadADCCheck = %d\n", NVT_206_Mutual_isReadADCCheck);
	} else if (mp_criteria_item == IC_CMD_SET_Mutual_TestTimes) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_TestTimes:", &NVT_206_Mutual_TestTimes);
		TS_LOG_INFO( "NVT_206_Mutual_TestTimes = %d\n", NVT_206_Mutual_TestTimes);
	} else if (mp_criteria_item == IC_CMD_SET_Mutual_Dummy_Frames) {
		parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_Dummy_Frames:", &NVT_206_Mutual_Dummy_Frames);
		TS_LOG_INFO( "NVT_206_Mutual_Dummy_Frames = %d\n", NVT_206_Mutual_Dummy_Frames);
	}
	return 0;
}
int32_t nvt_206_load_mp_criteria_Short_RXRX(char *ptr )
{
	int32_t retval=0;
	char *tok_ptr = NULL;
	int i = 0;
	uint32_t j = 0;
	char *token = NULL;
	char *test_cmd_buf = NULL;
	uint32_t test_cmd_num = 0;
	nvt_206_test_cmd_item_e cmd_item;
	TS_LOG_INFO( "%s: load test_cmd_short_test_rxrx:\n", __func__);
	if(NULL==ptr){
		TS_LOG_ERR( "%s: ptr=NULL\n", __func__);
		retval = -15;
		return retval;
	}
	if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_rxrx:", &test_cmd_num) < 0) {
		TS_LOG_ERR( "%s: load test_cmd_short_test_rxrx failed\n", __func__);
		retval = -15;
		return retval;
	}
	TS_LOG_INFO( "test_cmd_num = %d\n", test_cmd_num);
	test_cmd_buf = (char *)kzalloc(8192, GFP_KERNEL);
	if(test_cmd_buf == NULL) {
		TS_LOG_ERR("kzalloc test_cmd_buf failed, line:%d\n", __LINE__);
		return -1;
	}
	nvt_206_short_test_rxrx_num = test_cmd_num;
	if (nvt_206_short_test_rxrx) {
		vfree(nvt_206_short_test_rxrx);
		nvt_206_short_test_rxrx = NULL;
	}
	nvt_206_short_test_rxrx = vmalloc(test_cmd_num * sizeof(struct nvt_206_test_cmd));
	if(nvt_206_short_test_rxrx == NULL) {
		TS_LOG_ERR("vmalloc nvt_206_short_test_rxrx failed, line:%d\n", __LINE__);
		kfree(test_cmd_buf);
		test_cmd_buf = NULL;
		return -1;
	}
	// load test commands
	for (i = 0; i < (int)test_cmd_num; i++) {
	goto_next_line(&ptr);
	copy_this_line(test_cmd_buf, ptr);
	cmd_item = cmd_item_addr;
	tok_ptr = test_cmd_buf;
	j = 0;
	while((token = strsep(&tok_ptr,", \t\r\0"))) {
	if (strlen(token) == 0)
		continue;
	if (cmd_item == cmd_item_addr) {
		nvt_206_short_test_rxrx[i].addr = str_to_hex(token + 2);
		cmd_item++;
	} else if (cmd_item == cmd_item_len) {
		nvt_206_short_test_rxrx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
		cmd_item++;
	} else {
		nvt_206_short_test_rxrx[i].data[j] = (uint8_t) str_to_hex(token + 2);
		j++;
	}
	}
	if (j != nvt_206_short_test_rxrx[i].len)
	TS_LOG_ERR( "command data and length loaded not match!!!\n");
	}
	kfree(test_cmd_buf);
	test_cmd_buf = NULL;
	return retval;
}
int32_t nvt_206_load_mp_criteria_Short_TXRX(char *ptr )
{
	int32_t retval=0;
	char *tok_ptr = NULL;
	int i = 0;
	uint32_t j = 0;
	char *token = NULL;
	char *test_cmd_buf = NULL;
	int32_t test_cmd_num = 0;
	nvt_206_test_cmd_item_e cmd_item;
	if(NULL==ptr){
		TS_LOG_ERR( "%s: ptr=NULL\n", __func__);
		retval = -16;
		return retval;
	}
	TS_LOG_INFO( "%s: load test_cmd_short_test_txrx:\n", __func__);
	if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_txrx:", &test_cmd_num) < 0) {
		TS_LOG_ERR( "%s: load test_cmd_short_test_txrx failed!\n", __func__);
		retval = -16;
		return retval;
	}
	TS_LOG_INFO( "test_cmd_num = %d\n", test_cmd_num);
	test_cmd_buf = (char *)kzalloc(8192, GFP_KERNEL);
	if(test_cmd_buf == NULL) {
		TS_LOG_ERR("kzalloc test_cmd_buf failed, line:%d\n", __LINE__);
		return -1;
	}
	nvt_206_short_test_txrx_cmd_num = test_cmd_num;
	if (nvt_206_short_test_txrx) {
		vfree(nvt_206_short_test_txrx);
		nvt_206_short_test_txrx = NULL;
	}
	nvt_206_short_test_txrx = vmalloc(test_cmd_num * sizeof(struct nvt_206_test_cmd));
	if(nvt_206_short_test_txrx == NULL) {
		TS_LOG_ERR("kzalloc nvt_206_short_test_txrx failed, line:%d\n", __LINE__);
		kfree(test_cmd_buf);
		test_cmd_buf = NULL;
		return -1;
	}
	// load test commands
	for (i = 0; i < (int)test_cmd_num; i++) {
		goto_next_line(&ptr);
		copy_this_line(test_cmd_buf, ptr);
		cmd_item = cmd_item_addr;
		tok_ptr = test_cmd_buf;
		j = 0;
		while((token = strsep(&tok_ptr,", \t\r\0"))) {
			if (strlen(token) == 0)
				continue;
			if (cmd_item == cmd_item_addr) {
				nvt_206_short_test_txrx[i].addr = str_to_hex(token + 2);
				cmd_item++;
			} else if (cmd_item == cmd_item_len) {
				nvt_206_short_test_txrx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
				cmd_item++;
			} else {
				nvt_206_short_test_txrx[i].data[j] = (uint8_t) str_to_hex(token + 2);
				j++;
			}
		}
		if (j != nvt_206_short_test_txrx[i].len)
			TS_LOG_ERR( "command data and length loaded not match!!!\n");
	}
	kfree(test_cmd_buf);
	test_cmd_buf = NULL;
	return retval;
}
int32_t nvt_206_load_mp_criteria_Short_TXTX(char *ptr )
{
   	int32_t retval=0;
	char *tok_ptr = NULL;
	int i = 0;
	uint32_t j = 0;
	char *token = NULL;
	char *test_cmd_buf = NULL;
	int test_cmd_num = 0;
	nvt_206_test_cmd_item_e cmd_item;
	TS_LOG_INFO( "%s: load test_cmd_short_test_txtx:\n", __func__);
	if(NULL==ptr){
		TS_LOG_ERR( "%s: ptr=NULL\n", __func__);
		retval = -17;
		return retval;
	}
	if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_txtx:", &test_cmd_num) < 0) {
		TS_LOG_ERR( "%s: load test_cmd_short_test_txtx failed!\n", __func__);
		retval = -17;
		return retval;
	}
	TS_LOG_INFO( "test_cmd_num = %d\n", test_cmd_num);
	test_cmd_buf = (char *)kzalloc(8192, GFP_KERNEL);
	if(test_cmd_buf == NULL) {
		TS_LOG_ERR("kzalloc test_cmd_buf failed, line:%d\n", __LINE__);
		return -1;
	}
	nvt_206_short_test_txtx_cmd_num = test_cmd_num;
	if (nvt_206_short_test_txtx) {
		vfree(nvt_206_short_test_txtx);
		nvt_206_short_test_txtx = NULL;
	}
	nvt_206_short_test_txtx = vmalloc(test_cmd_num * sizeof(struct nvt_206_test_cmd));
	if(nvt_206_short_test_txtx == NULL) {
		TS_LOG_ERR("vmalloc nvt_206_short_test_txtx failed, line:%d\n", __LINE__);
		kfree(test_cmd_buf);
		test_cmd_buf = NULL;
		return -1;
	}
	// load test commands
	for (i = 0; i < test_cmd_num; i++) {
		goto_next_line(&ptr);
		copy_this_line(test_cmd_buf, ptr);
		cmd_item = cmd_item_addr;
		tok_ptr = test_cmd_buf;
		j = 0;
		while((token = strsep(&tok_ptr,", \t\r\0"))) {
			if (strlen(token) == 0)
				continue;
			if (cmd_item == cmd_item_addr) {
				nvt_206_short_test_txtx[i].addr = str_to_hex(token + 2);
				cmd_item++;
			} else if (cmd_item == cmd_item_len) {
				nvt_206_short_test_txtx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
				cmd_item++;
			} else {
				nvt_206_short_test_txtx[i].data[j] = (uint8_t) str_to_hex(token + 2);
				j++;
			}
		}
		if (j != nvt_206_short_test_txtx[i].len)
			TS_LOG_ERR( "command data and length loaded not match!!!\n");
	}
	kfree(test_cmd_buf);
	test_cmd_buf = NULL;
	return retval;
}
int32_t nvt_206_load_mp_criteria_Open(char *ptr )
{
   	int32_t retval=0;
	char *tok_ptr = NULL;
	int i = 0;
	uint32_t j = 0;
	char *token = NULL;
	char *test_cmd_buf = NULL;
	int test_cmd_num = 0;
	nvt_206_test_cmd_item_e cmd_item;
	TS_LOG_INFO( "%s: load test_cmd_open_test:\n", __func__);
	if(NULL==ptr){
		TS_LOG_ERR( "%s: ptr=NULL\n", __func__);
		retval = -18;
		return retval;
	}
	if (parse_mp_criteria_item(&ptr, "test_cmd_open_test:", &test_cmd_num) < 0) {
		TS_LOG_ERR( "%s: load test_cmd_open_test failed!\n", __func__);
		retval = -18;
		return retval;
	}
	TS_LOG_INFO( "test_cmd_num = %d\n", test_cmd_num);
	test_cmd_buf = (char *)kzalloc(8192, GFP_KERNEL);
	if(test_cmd_buf == NULL) {
		TS_LOG_ERR("kzalloc test_cmd_buf failed, line:%d\n", __LINE__);
		return -1;
	}
	nvt_206_open_test_cmd_num = test_cmd_num;
	if (nvt_206_open_test) {
		vfree(nvt_206_open_test);
		nvt_206_open_test = NULL;
	}
	nvt_206_open_test = vmalloc(test_cmd_num * sizeof(struct nvt_206_test_cmd));
	if(nvt_206_open_test == NULL) {
		TS_LOG_ERR("vmalloc nvt_206_open_test failed, line:%d\n", __LINE__);
		kfree(test_cmd_buf);
		test_cmd_buf = NULL;
		return -1;
	}
	// load test commands
	for (i = 0; i < test_cmd_num; i++) {
		goto_next_line(&ptr);
		copy_this_line(test_cmd_buf, ptr);
		cmd_item = cmd_item_addr;
		tok_ptr = test_cmd_buf;
		j = 0;
		while((token = strsep(&tok_ptr,", \t\r\0"))) {
			if (strlen(token) == 0)
				continue;
			if (cmd_item == cmd_item_addr) {
				nvt_206_open_test[i].addr = str_to_hex(token + 2);
				cmd_item++;
			} else if (cmd_item == cmd_item_len) {
				nvt_206_open_test[i].len = (uint8_t) simple_strtol(token, NULL, 10);
				cmd_item++;
			} else {
				nvt_206_open_test[i].data[j] = (uint8_t) str_to_hex(token + 2);
				j++;
			}
		}
		if (j != nvt_206_open_test[i].len)
			TS_LOG_ERR( "command data and length loaded not match!!!\n");
	}
	kfree(test_cmd_buf);
	test_cmd_buf = NULL;
	return retval;
}

int32_t nvt_206_load_mp_criteria(void)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs={0};
	char file_path[NVT_206_FILE_LEN] = NVT_206_MP_CRITERIA_GOLDEN_FILE;
	int32_t read_ret = 0;
	char *ptr = NULL;
	nvt_206_mp_criteria_item_e mp_criteria_item = Tol_Pos_Short;
	uint32_t i = 0;
	uint32_t j = 0;
	char tx_data[1024] = {0};
	char *token = fbufp;
	char *tok_ptr = fbufp;
	size_t offset = 0;
	int8_t skip_TXn = 1;
	int m=0;
	TS_LOG_INFO( "%s:++\n", __func__);
	org_fs = get_fs();
	/*lint -save -e* */
	set_fs(KERNEL_DS);
	/*lint -restore*/

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		TS_LOG_ERR( "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	if (fp->f_op && fp->f_op->read) {
		struct kstat stat;

		retval = vfs_stat(file_path, &stat);

		fbufp = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
		if (!fbufp) {
			TS_LOG_ERR( "%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}

		if ((!retval) && (read_ret = fp->f_op->read(fp, fbufp, stat.size, &fp->f_pos) > 0)) {
			fbufp[stat.size] = '\n';
			ptr = fbufp;
			while ( ptr && (ptr < (fbufp + stat.size))) {
				 if (mp_criteria_item == Open_rawdata) {
					TS_LOG_INFO( "%s: load golden Open rawdata:\n", __func__);
					ptr = strstr(ptr, "Open rawdata:");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden Open rawdata failed!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden Open rawdata failed!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
						ptr = strstr(ptr, "TX");
						if (ptr == NULL) {
							TS_LOG_ERR( "%s: load golden Open rawdata failed!\n", __func__);
							retval = -7;
							goto exit_free;
						}
						// parse data after TXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 1024);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						j = 0;
						skip_TXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_TXn == 1) {
								skip_TXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							m=i * nvt_hybrid_ts->ain_rx_num + j;
							NVT_206_BoundaryOpen[m] = (int32_t) simple_strtol(token, NULL, 10);
							j++;
						}
						// go forward
						ptr = ptr + offset;
					}
				} else if (mp_criteria_item == RXRX_rawdata) {

					TS_LOG_INFO( "%s: load golden RX-RX rawdata:\n", __func__);
					ptr = strstr(ptr, "RX-RX rawdata:");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden RX-RX rawdata failed!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden RX-RX rawdata failed!\n", __func__);
						retval = -9;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					// copy this line to tx_data buffer
					memset(tx_data, 0, 1024);
					copy_this_line(tx_data, ptr);
					offset = strlen(tx_data);
					tok_ptr = tx_data;
					j = 0;
					while ((token = strsep(&tok_ptr,", \t\r\0"))) {
						if (strlen(token) == 0)
							continue;
						NVT_206_BoundaryShort_RXRX[j] = (int32_t) simple_strtol(token, NULL, 10);
						j++;
					}
					// go forward
					ptr = ptr + offset;
				} else if (mp_criteria_item == TXRX_rawdata) {
					TS_LOG_INFO( "%s: load golden TX-RX rawdata:\n", __func__);
					ptr = strstr(ptr, "TX-RX rawdata:");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden TX-RX rawdata failed!\n", __func__);
						retval = -10;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden TX-RX rawdata failed!\n", __func__);
						retval = -11;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
						ptr = strstr(ptr, "TX");
						if (ptr == NULL) {
							TS_LOG_ERR( "%s: load golden TX-RX rawdata failed!\n", __func__);
							retval = -12;
							goto exit_free;
						}
						// parse data after TXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 1024);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						j = 0;
						skip_TXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_TXn == 1) {
								skip_TXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							m=i * nvt_hybrid_ts->ain_rx_num + j;
							NVT_206_BoundaryShort_TXRX[m] = (int32_t) simple_strtol(token, NULL, 10);
							j++;
						}
						// go forward
						ptr = ptr + offset;
					}

				} else if (mp_criteria_item == TXTX_rawdata) {

					TS_LOG_INFO( "%s: load golden TX-TX rawdata:\n", __func__);
					ptr = strstr(ptr, "TX-TX rawdata:");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden TX-TX rawdata failed!\n", __func__);
						retval = -13;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "TX1");
					if (ptr == NULL) {
						TS_LOG_ERR( "%s: load golden TX-TX rawdata failed!\n", __func__);
						retval = -14;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					// copy this line to tx_data buffer
					memset(tx_data, 0, 1024);
					copy_this_line(tx_data, ptr);
					offset = strlen(tx_data);
					tok_ptr = tx_data;
					j = 0;
					while ((token = strsep(&tok_ptr,", \t\r\0"))) {
						if (strlen(token) == 0)
							continue;
						NVT_206_BoundaryShort_TXTX[j] = (int32_t) simple_strtol(token, NULL, 10);
						//printk("%5d, ", NVT_206_BoundaryShort_TXTX[j]);
						j++;
					}
					// go forward
					ptr = ptr + offset;

				} else if(mp_criteria_item == Test_Cmd_Short_RXRX) {
					retval=nvt_206_load_mp_criteria_Short_RXRX(ptr);
					if(retval!=0) {
						goto exit_free;
					}
				} else if(mp_criteria_item == Test_Cmd_Short_TXRX) {
					retval=nvt_206_load_mp_criteria_Short_TXRX(ptr);
					if(retval!=0) {
						goto exit_free;
					}
				} else if(mp_criteria_item == Test_Cmd_Short_TXTX) {
					retval=nvt_206_load_mp_criteria_Short_TXTX(ptr);
					if(retval!=0) {
						goto exit_free;
					}
				} else if(mp_criteria_item == Test_Cmd_Open) {
					retval=nvt_206_load_mp_criteria_Open(ptr);
					if(retval!=0) {
						goto exit_free;
					}
				}
				else
				{
					nvt_206_load_mp_default_criteria(mp_criteria_item,ptr);
				}
				mp_criteria_item++;
				if (mp_criteria_item == MP_Cri_Item_Last) {
					TS_LOG_INFO( "Load MP criteria and golden samples finished.\n");
					break;
				}
			}
		} else {
			TS_LOG_ERR( "%s: retval=%d,read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
			retval = -3;
			goto exit_free;
		}
	} else {
		TS_LOG_ERR( "%s: retval = %d\n", __func__, retval);
		retval = -4;  // Read Failed
		goto exit_free;
	}

exit_free:
	set_fs(org_fs);

	if (fbufp) {
		kfree(fbufp);
	}
	if (fp)
		filp_close(fp, NULL);

	TS_LOG_INFO( "%s:--, retval=%d\n", __func__, retval);
	return retval;
}

static void nvt_206_cal_ain_order(void)
{
	uint32_t i = 0;

	for (i = 0; i < NVT_206_IC_RX_CFG_SIZE; i++) {
		if (NVT_206_AIN_RX[i] == 0xFF)
			continue;
		NVT_206_AIN_RX_Order[NVT_206_AIN_RX[i]] = (uint8_t)i;
	}

	for (i = 0; i < NVT_206_IC_TX_CFG_SIZE; i++) {
		if (NVT_206_AIN_TX[i] == 0xFF)
			continue;
		NVT_206_AIN_TX_Order[NVT_206_AIN_TX[i]] = (uint8_t)i;
	}

	TS_LOG_INFO("NVT_206_AIN_RX_Order:\n");
	for (i = 0; i < NVT_206_IC_RX_CFG_SIZE; i++)
		printk("%d, ", NVT_206_AIN_RX_Order[i]);
	printk("\n");

	TS_LOG_INFO("NVT_206_AIN_TX_Order:\n");
	for (i = 0; i < NVT_206_IC_TX_CFG_SIZE; i++)
		printk("%d, ", NVT_206_AIN_TX_Order[i]);
	printk("\n");
}

static void nvt_206_ctrlram_fill_cmd_table(char **ptr, struct nvt_206_test_cmd *test_cmd_table, uint32_t *ctrlram_cmd_table_idx, uint32_t *ctrlram_cur_addr)
{
	char ctrlram_data_buf[64] = {0};
	uint32_t ctrlram_data = 0;
	if(NULL == ptr ||NULL == test_cmd_table ||NULL == ctrlram_cmd_table_idx ||NULL == ctrlram_cur_addr ){
		TS_LOG_ERR("%s: ptr is Null,or test_cmd_table is null,or ctrlram_cmd_table_idx is null,or ctrlram_cur_addr is null\n", __func__);
		return;
	}
	goto_next_line(ptr);
	copy_this_line(ctrlram_data_buf, *ptr);
	while (strlen(ctrlram_data_buf) && (ctrlram_data_buf[0] != '\r') && (ctrlram_data_buf[0] != '\n')) {
		ctrlram_data = (uint32_t) str_to_hex(ctrlram_data_buf);
		test_cmd_table[*ctrlram_cmd_table_idx].addr = *ctrlram_cur_addr;
		test_cmd_table[*ctrlram_cmd_table_idx].len = 4;
		test_cmd_table[*ctrlram_cmd_table_idx].data[0] = (uint8_t) ctrlram_data & 0xFF;
		test_cmd_table[*ctrlram_cmd_table_idx].data[1] = (uint8_t) ((ctrlram_data & 0xFF00) >> 8);
		test_cmd_table[*ctrlram_cmd_table_idx].data[2] = (uint8_t) ((ctrlram_data & 0xFF0000) >> 16);
		test_cmd_table[*ctrlram_cmd_table_idx].data[3] = (uint8_t) ((ctrlram_data & 0xFF000000) >> 24);
		*ctrlram_cmd_table_idx = *ctrlram_cmd_table_idx + 1;
		*ctrlram_cur_addr = *ctrlram_cur_addr + 4;
		goto_next_line(ptr);
		copy_this_line(ctrlram_data_buf, *ptr);
	}
}

static int32_t nvt_206_load_mp_ctrl_ram(char *file_path, struct nvt_206_test_cmd *test_cmd_table, uint32_t *ctrlram_test_cmds_num)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs={0};
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	nvt_206_ctrl_ram_item_e nvt_206_ctrl_ram_item = CtrlRam_REGISTER;
	char ctrlram_data_buf[64] = {0};
	uint32_t ctrlram_cur_addr = 0;
	uint32_t ctrlram_cmd_table_idx = 0;
	uint32_t ctrlram_data = 0;
	uint32_t TXMODE_Table_Addr = 0;
	uint32_t RXMODE_Table_Addr = 0;
	uint32_t CCMODE_Table_Addr = 0;
	uint32_t OFFSET_Table_Addr = 0;

	TS_LOG_INFO("%s:++\n", __func__);
	if(NULL == file_path ||NULL == test_cmd_table ||NULL == ctrlram_test_cmds_num){
		TS_LOG_ERR("%s: file_path is Null,or test_cmd_table is Null, or ctrlram_test_cmds_num is Null,\n", __func__);
		return -ENOMEM;
	}
	org_fs = get_fs();
	/*lint -save -e* */
	set_fs(KERNEL_DS);
	/*lint -restore*/
	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		TS_LOG_ERR("%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)kzalloc((unsigned int) stat.size + 1, __GFP_NOFAIL | GFP_KERNEL);
		if (!fbufp) {
			TS_LOG_ERR("%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}
		memset((void *)fbufp,0,(unsigned int)stat.size + 1);
		read_ret = (int) vfs_read(fp, (char __user *)fbufp, (size_t) stat.size, &pos);
		if (read_ret > 0) {
			fbufp[stat.size] = '\0';
			ptr = fbufp;

			while ( ptr && (ptr < (fbufp + stat.size))) {
				if (nvt_206_ctrl_ram_item == CtrlRam_REGISTER) {
					uint32_t ctrlram_table_number = 0;
					uint32_t ctrlram_start_addr = 0;
					ptr = strstr(ptr, "REGISTER");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: REGISTER not found!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					ctrlram_table_number = (uint32_t) str_to_hex(ctrlram_data_buf);
					ctrlram_table_number = ((ctrlram_table_number & 0x3F000000) >> 24);
					TS_LOG_INFO("ctrlram_table_number=%08X\n", ctrlram_table_number);
					test_cmd_table[0].addr = 0x1F200;
					test_cmd_table[0].len = 3;
					test_cmd_table[0].data[2] = (uint8_t)ctrlram_table_number;

					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					ctrlram_start_addr = (uint32_t) str_to_hex(ctrlram_data_buf);
					ctrlram_start_addr = ((ctrlram_start_addr & 0xFFFF0000) >> 16) | 0x10000;
					TS_LOG_INFO("start_addr_table=%08X\n", ctrlram_start_addr);
					test_cmd_table[0].data[0] = (uint8_t)(ctrlram_start_addr & 0xFF);
					test_cmd_table[0].data[1] = (uint8_t)((ctrlram_start_addr & 0xFF00) >> 8);

					ctrlram_cur_addr = ctrlram_start_addr;
				} else if (nvt_206_ctrl_ram_item == CtrlRam_START_ADDR) {
					uint32_t table1_addr = 0;
					ptr = strstr(ptr, "START_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: START_ADDR not found!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					table1_addr = (uint32_t) str_to_hex(ctrlram_data_buf);
					table1_addr = table1_addr & 0xFFFF;
					TS_LOG_INFO("table1_addr=%08X\n", table1_addr);
					test_cmd_table[1].addr = ctrlram_cur_addr;
					test_cmd_table[1].len = 4;
					test_cmd_table[1].data[0] = (uint8_t)(table1_addr & 0xFF);
					test_cmd_table[1].data[1] = (uint8_t)((table1_addr & 0xFF00) >> 8);
					test_cmd_table[1].data[2] = 0x00;
					test_cmd_table[1].data[3] = 0x00;

					ctrlram_cmd_table_idx = 2;
					ctrlram_cur_addr = ctrlram_cur_addr + 4;
				} else if (nvt_206_ctrl_ram_item == CtrlRam_TABLE_ADDR) {
					uint32_t table_addr_offset = 0;
					ptr = strstr(ptr, "TABLE_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: TABLE_ADDR not found!\n", __func__);
						retval = -7;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					while (strlen(ctrlram_data_buf) && (ctrlram_data_buf[0] != '\r') && (ctrlram_data_buf[0] != '\n')) {
						ctrlram_data = (uint32_t) str_to_hex(ctrlram_data_buf);
						if (table_addr_offset == 0x3C) {
							TXMODE_Table_Addr = (ctrlram_data & 0xFFFF) | 0x10000;
							TS_LOG_INFO("%s: TXMODE_Table_Addr = 0x%08X\n", __func__, TXMODE_Table_Addr);
							RXMODE_Table_Addr = ((ctrlram_data & 0xFFFF0000) >> 16) | 0x10000;
							TS_LOG_INFO("%s: RXMODE_Table_Addr = 0x%08X\n", __func__, RXMODE_Table_Addr);
						}
						if (table_addr_offset == 0x40) {
							CCMODE_Table_Addr = (ctrlram_data & 0xFFFF) | 0x10000;
							TS_LOG_INFO("%s: CCMODE_Table_Addr = 0x%08X\n", __func__, CCMODE_Table_Addr);
							OFFSET_Table_Addr = ((ctrlram_data & 0xFFFF0000) >> 16) | 0x10000;
							TS_LOG_INFO("%s: OFFSET_Table_Addr = 0x%08X\n", __func__, OFFSET_Table_Addr);
						}
						test_cmd_table[ctrlram_cmd_table_idx].addr = ctrlram_cur_addr;
						test_cmd_table[ctrlram_cmd_table_idx].len = 4;
						test_cmd_table[ctrlram_cmd_table_idx].data[0] = (uint8_t) ctrlram_data & 0xFF;
						test_cmd_table[ctrlram_cmd_table_idx].data[1] = (uint8_t) ((ctrlram_data & 0xFF00) >> 8);
						test_cmd_table[ctrlram_cmd_table_idx].data[2] = (uint8_t) ((ctrlram_data & 0xFF0000) >> 16);
						test_cmd_table[ctrlram_cmd_table_idx].data[3] = (uint8_t) ((ctrlram_data & 0xFF000000) >> 24);
						ctrlram_cmd_table_idx = ctrlram_cmd_table_idx + 1;
						ctrlram_cur_addr = ctrlram_cur_addr + 4;
						table_addr_offset = table_addr_offset + 4;
						goto_next_line(&ptr);
						copy_this_line(ctrlram_data_buf, ptr);
					}
				} else if (nvt_206_ctrl_ram_item == CtrlRam_UC_ADDR) {
					ptr = strstr(ptr, "UC_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: UC_ADDR not found!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					nvt_206_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (nvt_206_ctrl_ram_item == CtrlRam_TXMODE_ADDR) {
					if (TXMODE_Table_Addr != ctrlram_cur_addr) {
						TS_LOG_ERR("%s: TXMODE Table Address not match!\n", __func__);
						retval = -9;
						goto exit_free;
					}
					ptr = strstr(ptr, "TXMODE_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: TXMODE_ADDR not found!\n", __func__);
						retval = -10;
						goto exit_free;
					}
					nvt_206_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (nvt_206_ctrl_ram_item == CtrlRam_RXMODE_ADDR) {
					if (RXMODE_Table_Addr != ctrlram_cur_addr) {
						TS_LOG_ERR("%s: RXMODE Table Address not match!\n", __func__);
						retval = -11;
						goto exit_free;
					}
					ptr = strstr(ptr, "RXMODE_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: RXMODE_ADDR not found!\n", __func__);
						retval = -12;
						goto exit_free;
					}
					nvt_206_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (nvt_206_ctrl_ram_item == CtrlRam_CCMODE_ADDR) {
					if (CCMODE_Table_Addr != ctrlram_cur_addr) {
						TS_LOG_ERR("%s: CCMODE Table Address not match!\n", __func__);
						retval = -13;
						goto exit_free;
					}
					ptr = strstr(ptr, "CCMODE_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: CCMODE_ADDR not found!\n", __func__);
						retval = -14;
						goto exit_free;
					}
					nvt_206_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (nvt_206_ctrl_ram_item == CtrlRam_OFFSET_ADDR) {
					if (OFFSET_Table_Addr != ctrlram_cur_addr) {
						TS_LOG_ERR("%s: OFFSET Table Address not match!\n", __func__);
						retval = -15;
						goto exit_free;
					}
					ptr = strstr(ptr, "OFFSET_ADDR");
					if (ptr == NULL) {
						TS_LOG_ERR("%s: OFFSET_ADDR not found!\n", __func__);
						retval = -16;
						goto exit_free;
					}
					nvt_206_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				}

				nvt_206_ctrl_ram_item++;
				if (nvt_206_ctrl_ram_item == CtrlRam_ITEM_LAST) {
					*ctrlram_test_cmds_num = ctrlram_cmd_table_idx;
					TS_LOG_INFO("%s: ctrlram_test_cmds_num = %d\n", __func__, *ctrlram_test_cmds_num);
					TS_LOG_INFO("%s: Load control ram items finished\n", __func__);
					retval = 0;
					break;
				}
			}

        } else {
            TS_LOG_ERR("%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
            retval = -3;
            goto exit_free;
        }
    } else {
        TS_LOG_ERR("%s: failed to get file stat, retval = %d\n", __func__, retval);
        retval = -4;  // Read Failed
        goto exit_free;
    }

exit_free:
    set_fs(org_fs);
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}
	if (fp) {
		filp_close(fp, NULL);
	//	fp = NULL;
	}

    TS_LOG_INFO("%s:--, retval=%d\n", __func__, retval);
    return retval;
}

/*******************************************************
Description:
	Novatek touchscreen set ADC operation function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_206_set_adc_oper(int32_t isReadADCCheck)
{
	int32_t i2cRet = 0;
	uint8_t buf[4] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---write i2c cmds to set ADC operation---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF2;
	i2cRet = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if(i2cRet < 0) {
		TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
		return i2cRet;
	}

	//---write i2c cmds to set ADC operation---
	buf[0] = 0x10;
	buf[1] = 0x01;
	i2cRet = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
	if(i2cRet < 0) {
		TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
		return i2cRet;
	}
	if (isReadADCCheck) {
		for (i = 0; i < retry; i++) {
			//---read ADC status---
			buf[0] = 0x10;
			i2cRet = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2);
			if(i2cRet < 0) {
				TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
				return i2cRet;
			}

			if (buf[1] == 0x00)
				break;

			msleep(10);
		}

		if (i >= retry) {
			TS_LOG_ERR("%s: Failed!\n", __func__);
			return -1;
		} else {
			return i2cRet;
		}
	} else {
		return i2cRet;
	}
}

/*******************************************************
Description:
	Novatek touchscreen write test commands function.

return:
	n.a.
*******************************************************/
static void nvt_206_write_test_cmd(struct nvt_206_test_cmd *cmds, int32_t cmd_num)
{
	int32_t i = 0;
	int32_t j = 0;
	uint8_t buf[64]={0};
	int m=0;
	if (NULL == cmds) {
		TS_LOG_ERR("%s: cmdsis Null\n", __func__);
		return;
	}
	for (i = 0; i < cmd_num; i++) {
		//---set xdata index---
		buf[0] = 0xFF;
		buf[1] = ((cmds[i].addr >> 16) & 0xFF);
		buf[2] = ((cmds[i].addr >> 8) & 0xFF);
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);

		//---write test cmds---
		buf[0] = (cmds[i].addr & 0xFF);
		for (j = 0; j < cmds[i].len; j++) {
			m=1 + j;
			buf[m] = cmds[i].data[j];
		}
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 1 + cmds[i].len);


	}
	return;
}

#define ABS(x)	(((x) < 0) ? -(x) : (x))
/*******************************************************
Description:
	Novatek touchscreen read short test RX-RX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_206_read_short_rxrx(void)
{
	int ret = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[128] = {0};
	char ctrlram_short_rxrx_file[128] = NVT_206_CTRLRAM_SHORT_RXRX_FILE;
	char ctrlram_short_rxrx1_file[128] = NVT_206_CTRLRAM_SHORT_RXRX1_FILE;
	struct nvt_206_test_cmd *ctrlram_short_rxrx_cmds = NULL;
	struct nvt_206_test_cmd *ctrlram_short_rxrx1_cmds = NULL;
	int32_t ctrlram_short_rxrx_cmds_num = 0;
	int32_t ctrlram_short_rxrx1_cmds_num = 0;
	struct nvt_206_test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	int32_t sh = 0;
	int32_t sh1 = 0;

	TS_LOG_INFO( "%s:++\n", __func__);
	// Load CtrlRAM for rxrx
	ctrlram_short_rxrx_cmds = (struct nvt_206_test_cmd *)vmalloc( 512 * sizeof(struct nvt_206_test_cmd));
	if(ctrlram_short_rxrx_cmds == NULL) {
		TS_LOG_ERR("vmalloc ctrlram_short_rxrx_cmds failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}
	if (nvt_206_load_mp_ctrl_ram(ctrlram_short_rxrx_file, ctrlram_short_rxrx_cmds, &ctrlram_short_rxrx_cmds_num) < 0) {
		TS_LOG_ERR( "%s: load control ram %s failed!\n", __func__, ctrlram_short_rxrx_file);
		ret = -EAGAIN;
		goto exit;
	}
	ctrlram_short_rxrx_cmds[1].data[2] = 0x18;

	// Load CtrlRAM for rxrx1
	ctrlram_short_rxrx1_cmds = (struct nvt_206_test_cmd *)vmalloc( 512 * sizeof(struct nvt_206_test_cmd));
	if(ctrlram_short_rxrx1_cmds == NULL) {
		TS_LOG_ERR("vmalloc ctrlram_short_rxrx1_cmds failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}
	if (nvt_206_load_mp_ctrl_ram(ctrlram_short_rxrx1_file, ctrlram_short_rxrx1_cmds, &ctrlram_short_rxrx1_cmds_num) < 0) {
		TS_LOG_ERR("%s: load control ram %s failed!\n", __func__, ctrlram_short_rxrx1_file);
		ret = -EAGAIN;
		goto exit;
	}
	ctrlram_short_rxrx1_cmds[1].data[2] = 0x18;

	for (j = 0; j < NVT_206_RXRX_TestTimes; j++) {
		//---Reset IC & into idle---
		nvt_hybrid_sw_reset_idle();
		msleep(100);
		// WTG Disable
		nvt_206_write_test_cmd(&cmd_WTG_Disable, 1);

		// CtrlRAM test commands
		nvt_206_write_test_cmd(ctrlram_short_rxrx_cmds, ctrlram_short_rxrx_cmds_num);

		// Other test commands
		nvt_206_write_test_cmd(nvt_206_short_test_rxrx, nvt_206_short_test_rxrx_num);


		if (NVT_206_RXRX_isDummyCycle) {
			for (k = 0; k < NVT_206_RXRX_Dummy_Count; k++) {
				if (nvt_206_set_adc_oper(NVT_206_RXRX_isReadADCCheck) != 0) {
					ret = -EAGAIN;
					goto exit;
				}
			}
		}
		if (nvt_206_set_adc_oper(NVT_206_RXRX_isReadADCCheck) != 0) {
			ret = -EAGAIN;
			goto exit;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_RX_CFG_SIZE * 2 + 1);
		for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
			nvt_206_rawdata_short_rxrx0[i] = (int16_t)(buf[NVT_206_AIN_RX_Order[i] * 2 + 1] + 256 * buf[NVT_206_AIN_RX_Order[i] * 2 + 2]);
			printk("%5d, ", nvt_206_rawdata_short_rxrx0[i]);
		}
		printk("\n");

		//---Reset IC & into idle---
		nvt_hybrid_sw_reset_idle();
		msleep(100);
		// WTG Disable
		nvt_206_write_test_cmd(&cmd_WTG_Disable, 1);
		// CtrlRAM test commands
		nvt_206_write_test_cmd(ctrlram_short_rxrx1_cmds, ctrlram_short_rxrx1_cmds_num);
		// Other test commands
		nvt_206_write_test_cmd(nvt_206_short_test_rxrx, nvt_206_short_test_rxrx_num);


		if (NVT_206_RXRX_isDummyCycle) {
			for (k = 0; k < NVT_206_RXRX_Dummy_Count; k++) {
				if (nvt_206_set_adc_oper(NVT_206_RXRX_isReadADCCheck) != 0) {
					ret = -EAGAIN;
					goto exit;
				}
			}
		}
		if (nvt_206_set_adc_oper(NVT_206_RXRX_isReadADCCheck) != 0) {
			ret = -EAGAIN;
			goto exit;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = 0x00;
		nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_RX_CFG_SIZE * 2 + 1);
		for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
			nvt_206_rawdata_short_rxrx1[i] = (int16_t)(buf[NVT_206_AIN_RX_Order[i] * 2 + 1] + 256 * buf[NVT_206_AIN_RX_Order[i] * 2 + 2]);
			printk("%5d, ", nvt_206_rawdata_short_rxrx1[i]);
		}
		printk("\n");

		for (i = 0; i < nvt_hybrid_ts->ain_rx_num; i++) {
			sh = nvt_206_rawdata_short_rxrx0[i];
			sh1 = nvt_206_rawdata_short_rxrx1[i];
			if (ABS(sh) < ABS(sh1))
				sh = sh1;
			if (j == NVT_206_RXRX_Dummy_Frames) {
				nvt_206_rawdata_short_rxrx[i] = sh;
			} else if (j > NVT_206_RXRX_Dummy_Frames) {
				nvt_206_rawdata_short_rxrx[i] += sh;
				nvt_206_rawdata_short_rxrx[i] /= 2;
			}
		}
	} // NVT_206_RXRX_TestTimes

exit:
	if (ctrlram_short_rxrx_cmds) {
		vfree(ctrlram_short_rxrx_cmds);
		ctrlram_short_rxrx_cmds = NULL;
	}

	if (ctrlram_short_rxrx1_cmds) {
		vfree(ctrlram_short_rxrx1_cmds);
		ctrlram_short_rxrx1_cmds = NULL;
	}

	TS_LOG_INFO("%s:--\n", __func__);
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen read short test TX-RX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_206_read_short_txrx(void)
{
	int32_t ret = 0;
	int32_t i2cRet = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[128] = {0};
	char ctrlram_short_txrx_file[128] = NVT_206_CTRLRAM_SHORT_TXRX_FILE;
	struct nvt_206_test_cmd *ctrlram_short_txrx_cmds = NULL;
	uint32_t ctrlram_short_txrx_cmds_num = 0;
	struct nvt_206_test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	uint8_t *rawdata_buf = NULL;
	int16_t sh = 0;
	int m=0;
	int n=0;
	TS_LOG_INFO( "%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_short_txrx_cmds = (struct nvt_206_test_cmd *)vmalloc( 512 * sizeof(struct nvt_206_test_cmd));
	if(ctrlram_short_txrx_cmds == NULL) {
		TS_LOG_ERR("vmalloc ctrlram_short_txrx_cmds failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}
	if (nvt_206_load_mp_ctrl_ram(ctrlram_short_txrx_file, ctrlram_short_txrx_cmds, &ctrlram_short_txrx_cmds_num) < 0) {
		TS_LOG_ERR( "%s: load control ram %s failed!\n", __func__, ctrlram_short_txrx_file);
		ret = -EAGAIN;
		goto exit;
	}
	ctrlram_short_txrx_cmds[1].data[2] = 0x18;

	//---Reset IC & into idle---
	nvt_hybrid_sw_reset_idle();
	msleep(100);
	// WTG Disable
	nvt_206_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_206_write_test_cmd(ctrlram_short_txrx_cmds, ctrlram_short_txrx_cmds_num);
	// Other test commands
	nvt_206_write_test_cmd(nvt_206_short_test_txrx, nvt_206_short_test_txrx_cmd_num);

	rawdata_buf = (uint8_t *) kzalloc(NVT_206_IC_TX_CFG_SIZE * NVT_206_IC_RX_CFG_SIZE * 2, GFP_KERNEL);
	if(rawdata_buf == NULL) {
		TS_LOG_ERR("kzalloc rawdata_buf failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}

	if (NVT_206_TXRX_isDummyCycle) {
		for (k = 0; k < NVT_206_TXRX_Dummy_Count; k++) {
			if (nvt_206_set_adc_oper(NVT_206_TXRX_isReadADCCheck) != 0) {
				ret = -EAGAIN;
				goto exit;
			}
		}
	}

	for (k = 0; k < NVT_206_TXRX_TestTimes; k++) {
		if (nvt_206_set_adc_oper(NVT_206_TXRX_isReadADCCheck) != 0) {
			ret = -EAGAIN;
			goto exit;
		}

		for (i = 0; i < NVT_206_IC_TX_CFG_SIZE; i++) {
			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = (uint8_t)(((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			i2cRet = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if(i2cRet < 0) {
				TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
				ret = i2cRet;
				goto i2c_err;
			}
			//---read data---
			buf[0] = (uint8_t)((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF);
			i2cRet = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_RX_CFG_SIZE * 2 + 1);
			if(i2cRet < 0) {
				TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
				ret = i2cRet;
				goto i2c_err;
			}
			m=i * NVT_206_IC_RX_CFG_SIZE * 2;
			memcpy(rawdata_buf +m , buf + 1, NVT_206_IC_RX_CFG_SIZE * 2);
		}
		for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
			for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
				sh = (int16_t)(rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2] + 256 * rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2 + 1]);
				m=i * nvt_hybrid_ts->ain_rx_num + j;
				if ( k == NVT_206_TXRX_Dummy_Frames) {
					nvt_206_rawdata_short_txrx[m] = sh;
				} else if (k > NVT_206_TXRX_Dummy_Frames) {
					nvt_206_rawdata_short_txrx[m] = nvt_206_rawdata_short_txrx[m] + sh;
					nvt_206_rawdata_short_txrx[m] = nvt_206_rawdata_short_txrx[m] / 2;
				}
			}
		}
	} // NVT_206_TXRX_TestTimes


	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
		for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
			m=i * nvt_hybrid_ts->ain_rx_num + j;
			printk("%5d, ", nvt_206_rawdata_short_txrx[m]);
			m=7 * (i * nvt_hybrid_ts->ain_rx_num + j) + i * 2;
			n=i * nvt_hybrid_ts->ain_rx_num + j;
		}
		printk("\n");
		m=7 * (i * nvt_hybrid_ts->ain_rx_num + j) + i * 2;
	}
	printk("\n");
exit:
i2c_err:
	if (rawdata_buf) {
		kfree(rawdata_buf);
		rawdata_buf = NULL;
	}
	if (ctrlram_short_txrx_cmds) {
		vfree(ctrlram_short_txrx_cmds);
		ctrlram_short_txrx_cmds = NULL;
	}
	TS_LOG_INFO( "%s:--\n", __func__);
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen read short test TX-TX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_206_read_short_txtx(void)
{
	int32_t ret = 0;
	int32_t i2cRet = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint8_t buf[128] = {0};

	char ctrlram_short_txtx_file[128] = NVT_206_CTRLRAM_SHORT_TXTX_FILE;
	struct nvt_206_test_cmd *ctrlram_short_txtx_cmds = NULL;
	uint32_t ctrlram_short_txtx_cmds_num = 0;
	struct nvt_206_test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	int16_t sh = 0;
	int m=0;
	TS_LOG_INFO( "%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_short_txtx_cmds = (struct nvt_206_test_cmd *)vmalloc( 512 * sizeof(struct nvt_206_test_cmd));
	if(ctrlram_short_txtx_cmds == NULL) {
		TS_LOG_ERR("vmalloc ctrlram_short_txtx_cmds failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}
	if (nvt_206_load_mp_ctrl_ram(ctrlram_short_txtx_file, ctrlram_short_txtx_cmds, &ctrlram_short_txtx_cmds_num) < 0) {
		TS_LOG_ERR( "%s: load control ram %s failed!\n", __func__, ctrlram_short_txtx_file);
		ret = -EAGAIN;
		goto exit;
	}
	ctrlram_short_txtx_cmds[1].data[2] = 0x18;

	//---Reset IC & into idle---
	nvt_hybrid_sw_reset_idle();
	msleep(100);
	// WTG Disable
	nvt_206_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_206_write_test_cmd(ctrlram_short_txtx_cmds, ctrlram_short_txtx_cmds_num);
	// Other test commands
	nvt_206_write_test_cmd(nvt_206_short_test_txtx, nvt_206_short_test_txtx_cmd_num);

	if (NVT_206_TXTX_isDummyCycle) {
		for (k = 0; k < NVT_206_TXTX_Dummy_Count; k++) {
			if (nvt_206_set_adc_oper(NVT_206_TXTX_isReadADCCheck) != 0) {
				ret = -EAGAIN;
				goto exit;
			}
		}
	}

	for (k = 0; k < NVT_206_TXTX_TestTimes; k++) {
		if (nvt_206_set_adc_oper(NVT_206_TXTX_isReadADCCheck) != 0) {
			ret = -EAGAIN;
			goto exit;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		i2cRet = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
		if(i2cRet < 0) {
			TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
			ret = i2cRet;
			goto i2c_err;
		}
		//---read data---
		buf[0] = 0x00;
		i2cRet = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_TX_CFG_SIZE * 4 + 1);
		if(i2cRet < 0) {
			TS_LOG_ERR("%s,%d, i2c err\n", __func__, __LINE__);
			ret = i2cRet;
			goto i2c_err;
		}
		for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
			if (NVT_206_AIN_TX_Order[i] < 10)
				sh = (int16_t)(buf[NVT_206_AIN_TX_Order[i] * 4 + 1] + 256 * buf[NVT_206_AIN_TX_Order[i] * 4 + 2]);
			else
				sh = (int16_t)(buf[NVT_206_AIN_TX_Order[i] * 4 + 2 + 1] + 256 * buf[NVT_206_AIN_TX_Order[i] * 4 + 2 + 2]);

			if (k == NVT_206_TXTX_Dummy_Frames) {
				nvt_206_rawdata_short_txtx[i] = sh;
			} else if (k > NVT_206_TXTX_Dummy_Frames) {
				nvt_206_rawdata_short_txtx[i] = nvt_206_rawdata_short_txtx[i] + sh;
				nvt_206_rawdata_short_txtx[i] = nvt_206_rawdata_short_txtx[i] / 2;
			}
		}
	} // NVT_206_TXTX_TestTimes



	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < (int)nvt_hybrid_ts->ain_tx_num; i++) {
		printk("%5d, ", nvt_206_rawdata_short_txtx[i]);
		m=7 * i;
	}
	printk("\n");
exit:
i2c_err:
	if (ctrlram_short_txtx_cmds) {
		vfree(ctrlram_short_txtx_cmds);
	}
	TS_LOG_INFO( "%s:--\n", __func__);
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen raw data report upper and lower test function.

return:
	Executive outcomes. 0--passwd. negative--failed.
*******************************************************/
static int32_t nvt_206_rawdata_up_low(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_len, uint8_t y_len, int32_t Upper_Lmt[], int32_t Lower_Lmt[])
{
	int32_t retval = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t m=0;
	int32_t n=0;
	//---Check Lower & Upper Limit---
	for (j = 0 ; j < y_len ; j++) {
		for (i = 0 ; i < x_len ; i++) {
			n=j * x_len + i;
			if(rawdata[n] > Upper_Lmt[n]) {
				m=j * x_len + i;
				RecordResult[m] |= 0x01;
				retval = -1;
			}

			if(rawdata[n] < Lower_Lmt[n]) {
				m=j * x_len + i;
				RecordResult[m] |= 0x02;
				retval = -1;
			}
		}
	}

	//---Return Result---
	return retval;
}

/*******************************************************
Description:
	Novatek touchscreen calculate square root function.

return:
	Executive outcomes. square root of input
*******************************************************/
static uint32_t nvt_206_sqrt(uint32_t sqsum)
{
	uint32_t sq_rt = 0;

	uint32_t g0 = 0;
	uint32_t g1 = 0;
	uint32_t g2 = 0;
	uint32_t g3 = 0;
	uint32_t g4 = 0;
	uint32_t seed = 0;
	uint32_t next = 0;
	uint32_t step = 0;

	g4 =  sqsum / 100000000;
	g3 = (sqsum - g4 * 100000000) / 1000000;
	g2 = (sqsum - g4 * 100000000 - g3 * 1000000) / 10000;
	g1 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000) / 100;
	g0 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000 - g1 * 100);

	next = g4;
	step = 0;
	seed = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = seed * 10000;
	next = (next - (seed * step)) * 100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return sq_rt;
}

/*******************************************************
Description:
	Novatek touchscreen read open test raw data function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_206_read_open(void)
{
	int32_t ret = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[128]={0};
	char ctrlram_open_mutual_file[128] = NVT_206_CTRLRAM_OPEN_MUTUAL_FILE;
	struct nvt_206_test_cmd *ctrlram_open_mutual_cmds = NULL;
	int32_t ctrlram_open_mutual_cmds_num = 0;
	struct nvt_206_test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	uint8_t *rawdata_buf = NULL;
	int16_t sh1 = 0;
	int16_t sh2 = 0;
	int m=0;
	TS_LOG_INFO("%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_open_mutual_cmds = (struct nvt_206_test_cmd *)vmalloc( 512 * sizeof(struct nvt_206_test_cmd));
	if(ctrlram_open_mutual_cmds == NULL) {
		TS_LOG_ERR("vmalloc ctrlram_open_mutual_cmds failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}
	memset(ctrlram_open_mutual_cmds, 0, 512 * sizeof(struct nvt_206_test_cmd));

	if (nvt_206_load_mp_ctrl_ram(ctrlram_open_mutual_file, ctrlram_open_mutual_cmds, &ctrlram_open_mutual_cmds_num) < 0) {
		TS_LOG_ERR("%s: load control ram %s failed!\n", __func__, ctrlram_open_mutual_file);
		ret = -EAGAIN;
		goto exit;
	}
	ctrlram_open_mutual_cmds[1].data[2] = 0x18;
	//---Reset IC & into idle---
	nvt_hybrid_sw_reset_idle();
	msleep(100);
	// WTG Disable
	nvt_206_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_206_write_test_cmd(ctrlram_open_mutual_cmds, ctrlram_open_mutual_cmds_num);
	// Other test commands
	nvt_206_write_test_cmd(nvt_206_open_test, nvt_206_open_test_cmd_num);

	rawdata_buf = (uint8_t *) kzalloc(NVT_206_IC_TX_CFG_SIZE * NVT_206_IC_RX_CFG_SIZE * 2, GFP_KERNEL);
	if(rawdata_buf == NULL) {
		TS_LOG_ERR("kzalloc rawdata_buf failed, line:%d\n", __LINE__);
		ret = -EAGAIN;
		goto exit;
	}

	if (NVT_206_Mutual_isDummyCycle) {
		for (k = 0; k < NVT_206_Mutual_Dummy_Count; k++) {
			if (nvt_206_set_adc_oper(NVT_206_Mutual_isReadADCCheck) != 0) {
				ret = -EAGAIN;
				goto exit;
			}
		}
	}

	for (k = 0; k < NVT_206_Mutual_TestTimes; k++) {
		if (nvt_206_set_adc_oper(NVT_206_Mutual_isReadADCCheck) != 0) {
			ret = -EAGAIN;
			goto exit;
		}

		for (i = 0; i < NVT_206_IC_TX_CFG_SIZE; i++) {

			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = 0x00 + (uint8_t)(((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			//---read data---
			buf[0] = (uint8_t)((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF);
			nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_RX_CFG_SIZE * 2 + 1);
			m= i * NVT_206_IC_RX_CFG_SIZE * 2;
			memcpy(rawdata_buf +m, buf + 1, NVT_206_IC_RX_CFG_SIZE * 2);
		}
		printk("nvt_206_rawdata_open_raw1: k=%d, NVT_206_Mutual_Dummy_Frames=%d\n", k, NVT_206_Mutual_Dummy_Frames);
		for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
			for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
				sh1 = (int16_t)(rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2] + 256 * rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2 + 1]);
				m=i * nvt_hybrid_ts->ain_rx_num + j;
				if (k == NVT_206_Mutual_Dummy_Frames) {
					nvt_206_rawdata_open_raw1[m] = sh1;
				} else if (k > NVT_206_Mutual_Dummy_Frames) {
					nvt_206_rawdata_open_raw1[m] = nvt_206_rawdata_open_raw1[m] + sh1;
					nvt_206_rawdata_open_raw1[m] = nvt_206_rawdata_open_raw1[m] / 2;
				}
			}
		}

		for (i = 0; i < NVT_206_IC_TX_CFG_SIZE; i++) {
			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = 0x30 + (uint8_t)(((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			//---read data---
			buf[0] = (uint8_t)((i * NVT_206_IC_RX_CFG_SIZE * 2) & 0xFF);
			nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, NVT_206_IC_RX_CFG_SIZE * 2 + 1);
			m=  i * NVT_206_IC_RX_CFG_SIZE * 2;
			memcpy(rawdata_buf +m, buf + 1, NVT_206_IC_RX_CFG_SIZE * 2);
		}
		for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
			for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
				sh2 = (int16_t)(rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2] + 256 * rawdata_buf[(NVT_206_AIN_TX_Order[i] * NVT_206_IC_RX_CFG_SIZE + NVT_206_AIN_RX_Order[j]) * 2 + 1]);
				m=i * nvt_hybrid_ts->ain_rx_num + j;
				if (k == NVT_206_Mutual_Dummy_Frames) {
					nvt_206_rawdata_open_raw2[m] = sh2;
				} else if (k > NVT_206_Mutual_Dummy_Frames) {
					nvt_206_rawdata_open_raw2[m] = nvt_206_rawdata_open_raw2[m] + sh2;
					nvt_206_rawdata_open_raw2[m] = nvt_206_rawdata_open_raw2[m] / 2;
				}
			}
		}
	} // NVT_206_Mutual_TestTimes

	//--IQ---
	for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
		for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
			m=i * nvt_hybrid_ts->ain_rx_num + j;
			nvt_206_rawdata_open[m] =(int32_t) nvt_206_sqrt((uint32_t) nvt_206_rawdata_open_raw1[m] * nvt_206_rawdata_open_raw1[m] + nvt_206_rawdata_open_raw2[m] * nvt_206_rawdata_open_raw2[m]);
		}
	}



	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < nvt_hybrid_ts->ain_tx_num; i++) {
		for (j = 0; j < nvt_hybrid_ts->ain_rx_num; j++) {
			m=i * nvt_hybrid_ts->ain_rx_num + j;
			printk("%5d, ", nvt_206_rawdata_open[m]);
			m=7 * (i * nvt_hybrid_ts->ain_rx_num + j) + i * 2;
		}
		printk("\n");
		m=7 * (i * nvt_hybrid_ts->ain_rx_num + j) + i * 2;
	}
	printk("\n");
exit:
	if (rawdata_buf) {
		kfree(rawdata_buf);
		rawdata_buf = NULL;
	}
	if (ctrlram_open_mutual_cmds) {
		vfree(ctrlram_open_mutual_cmds);
		ctrlram_open_mutual_cmds = NULL;
	}
	TS_LOG_INFO("%s:--\n", __func__);
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen calculate G Ratio and Normal
	function.

return:
	Executive outcomes. 0---succeed. 1---failed.
*******************************************************/
static int32_t NVT_206_Test_CaluateGRatioAndNormal(int32_t boundary[], int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int64_t tmpValue = 0;
	int64_t MaxSum = 0;
	int32_t SumCnt = 0;
	int64_t MaxNum = 0;
	int32_t MaxIndex = 0;
	int64_t Max = -99999999;
	int64_t Min =  99999999;
	int64_t offset = 0;
	int32_t Data = 0; // double
	int64_t StatisticsStep=0;
	int m=0;
	int n=0;

	//--------------------------------------------------
	//1. (Testing_CM - Golden_CM ) / Testing_CM
	//--------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			m=j * x_len + i;
			Data = rawdata[m];
			if (Data == 0)
				Data = 1;
			nvt_206_golden_Ratio[m] = Data - boundary[m];
			nvt_206_golden_Ratio[m] = div_s64((nvt_206_golden_Ratio[m] * 1000), Data); // *1000 before division
		}
	}

	//--------------------------------------------------------
	// 2. Mutual_GoldenRatio*1000
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			m=j * x_len + i;
			nvt_206_golden_Ratio[m] *= 1000;
		}
	}

	//--------------------------------------------------------
	// 3. Calculate StatisticsStep
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			m=j * x_len + i;
			if (Max < nvt_206_golden_Ratio[m])
				Max = (int32_t)nvt_206_golden_Ratio[m];
			if (Min > nvt_206_golden_Ratio[m])
				Min = (int32_t)nvt_206_golden_Ratio[m];
		}
	}

	offset = 0;
	if (Min < 0) { // add offset to get erery element Positive
		offset = 0 - Min;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				m=j * x_len + i;
				nvt_206_golden_Ratio[m] += offset;
			}
		}
		Max += offset;
	}
	StatisticsStep = div_s64(Max, NVT_206_MaxStatisticsBuf);
	StatisticsStep += 1;
	if (StatisticsStep < 0) {
		TS_LOG_ERR("%s: FAIL! (StatisticsStep < 0)\n", __func__);
		return 1;
	}

	//--------------------------------------------------------
	// 4. Start Statistics and Average
	//--------------------------------------------------------
	memset(NVT_206_StatisticsSum, 0, sizeof(int64_t) * NVT_206_MaxStatisticsBuf);
	memset(NVT_206_StatisticsNum, 0, sizeof(int64_t) * NVT_206_MaxStatisticsBuf);
	for (i = 0; i < NVT_206_MaxStatisticsBuf; i++) {
		NVT_206_StatisticsSum[i] = 0;
		NVT_206_StatisticsNum[i] = 0;
	}
	for (j = 0; j < y_len; j++) {
		for(i = 0; i < x_len; i++) {
			m=j * x_len + i;
			tmpValue = nvt_206_golden_Ratio[m];
			tmpValue = div_s64(tmpValue ,StatisticsStep);
			NVT_206_StatisticsNum[tmpValue] += 2;
			NVT_206_StatisticsSum[tmpValue] += (2 * nvt_206_golden_Ratio[m]);

			if ((tmpValue + 1) < NVT_206_MaxStatisticsBuf) {
				m=tmpValue + 1;
				n=j * x_len + i;
				NVT_206_StatisticsNum[m] += 1;
				NVT_206_StatisticsSum[m] += nvt_206_golden_Ratio[n];
			}

			if ((tmpValue - 1) >= 0) {
				m=tmpValue - 1;
				n=j * x_len + i;
				NVT_206_StatisticsNum[m] += 1;
				NVT_206_StatisticsSum[m] += nvt_206_golden_Ratio[n];
			}
		}
	}
	//Find out Max Statistics
	MaxNum = 0;
	for (k = 0; k < NVT_206_MaxStatisticsBuf; k++) {
		if (MaxNum < NVT_206_StatisticsNum[k]) {
			MaxSum = NVT_206_StatisticsSum[k];
			MaxNum = NVT_206_StatisticsNum[k];
			MaxIndex = k;
		}
	}

	//Caluate Statistics Average
	if (MaxSum > 0 ) {
		if (NVT_206_StatisticsNum[MaxIndex] != 0) {
			tmpValue = (int64_t)div_s64(NVT_206_StatisticsSum[MaxIndex], NVT_206_StatisticsNum[MaxIndex]) * 2;
			SumCnt += 2;
		}

		if ((MaxIndex + 1) < (NVT_206_MaxStatisticsBuf)) {
			m=MaxIndex + 1;
			if (NVT_206_StatisticsNum[m] != 0) {
				tmpValue += (int64_t)div_s64(NVT_206_StatisticsSum[m], NVT_206_StatisticsNum[m]);
				SumCnt++;
			}
		}

		if ((MaxIndex - 1) >= 0) {
			m=MaxIndex - 1;
			if (NVT_206_StatisticsNum[m] != 0) {
				tmpValue += (int64_t)div_s64(NVT_206_StatisticsSum[m], NVT_206_StatisticsNum[m]);
				SumCnt++;
			}
		}

		if (SumCnt > 0)
			tmpValue = div_s64(tmpValue, SumCnt);
	} else { // Too Separately
		NVT_206_StatisticsSum[0] = 0;
		NVT_206_StatisticsNum[0] = 0;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				m=j * x_len + i;
				NVT_206_StatisticsSum[0] += (int64_t)nvt_206_golden_Ratio[m];
				NVT_206_StatisticsNum[0]++;
			}
		}
		tmpValue = div_s64(NVT_206_StatisticsSum[0], NVT_206_StatisticsNum[0]);
	}
	//----------------------------------------------------------
	//----------------------------------------------------------
	//----------------------------------------------------------
	tmpValue -= offset;
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			m=j * x_len + i;
			nvt_206_golden_Ratio[m] -= offset;
			nvt_206_golden_Ratio[m] = nvt_206_golden_Ratio[m] - tmpValue;
			nvt_206_golden_Ratio[m] = div_s64(nvt_206_golden_Ratio[m], 1000);
		}
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t NVT_206_RawDataTest_Sub(int32_t boundary[], int32_t rawdata[], uint8_t RecordResult[],
								uint8_t x_ch, uint8_t y_ch,
								int32_t Tol_P, int32_t Tol_N,
								int32_t Dif_P, int32_t Dif_N,
								int32_t Rawdata_Limit_Postive, int32_t Rawdata_Limit_Negative)
{
	int32_t i = 0;
	int32_t j = 0;
	int iArrayIndex = 0;
	int32_t iBoundary = 0;
	int32_t iTolLowBound = 0;
	int32_t iTolHighBound = 0;
	bool isAbsCriteria = false;
	bool isPass = true;

	if ((Rawdata_Limit_Postive != 0) || (Rawdata_Limit_Negative != 0))
		isAbsCriteria = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			iBoundary = boundary[iArrayIndex];

			RecordResult[iArrayIndex] = 0x00; // default value for PASS

			if (isAbsCriteria) {
				iTolLowBound = Rawdata_Limit_Negative;
				iTolHighBound = Rawdata_Limit_Postive;
			} else {
				if (iBoundary > 0) {
					iTolLowBound = (iBoundary * (1000 + Tol_N));
					iTolHighBound = (iBoundary * (1000 + Tol_P));
				} else {
					iTolLowBound = (iBoundary * (1000 - Tol_N));
					iTolHighBound = (iBoundary * (1000 - Tol_P));
				}
			}

			if((rawdata[iArrayIndex] * 1000) > iTolHighBound)
				RecordResult[iArrayIndex] |= 0x01;

			if((rawdata[iArrayIndex] * 1000) < iTolLowBound)
				RecordResult[iArrayIndex] |= 0x02;
		}
	}

	if (!isAbsCriteria) {
		NVT_206_Test_CaluateGRatioAndNormal(boundary, rawdata, x_ch, y_ch);

		for (j = 0; j < y_ch; j++) {
			for (i = 0; i < x_ch; i++) {
				iArrayIndex = j * x_ch + i;

				if (nvt_206_golden_Ratio[iArrayIndex] > Dif_P)
					RecordResult[iArrayIndex] |= 0x04;

				if (nvt_206_golden_Ratio[iArrayIndex] < Dif_N)
					RecordResult[iArrayIndex] |= 0x08;
			}
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex=j * x_ch + i;
			if (RecordResult[iArrayIndex] != 0) {
				isPass = false;
				TS_LOG_ERR("Tx:%d, Rx:%d result check failed\n", i, j);
				break;
			}
		}
	}
	if(!isPass) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t NVT_206_PixelRawTest_Sub(int32_t rawdata[], int64_t PixelRawCmRatio[], int32_t PixelRawCmRatioMax[], uint8_t RecordResult[],
								uint8_t x_ch, uint8_t y_ch, int32_t PixelRaw_Diff)
{
	int32_t i = 0;
	int j = 0;
	int iArrayIndex = 0;
	int32_t nvt_up = 0;
	int32_t nvt_down = 0;
	int32_t right = 0;
	int32_t left = 0;
	int64_t tmpRatio[4] = {0};
	bool isPass = true;
	int m=0;
	TS_LOG_INFO("%s:++\n", __func__);


	// rawdata to Cm(fF)
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			RecordResult[iArrayIndex] = 0x00;
			PixelRawCmRatio[iArrayIndex] = (int64_t)rawdata[iArrayIndex];
			PixelRawCmRatio[iArrayIndex] = div_s64(PixelRawCmRatio[iArrayIndex] * 9567, 10000);
		}
	}

	for (j = 0; j < y_ch; j++) {
		if (j == 0) {
			nvt_up = 0;
			nvt_down = j + 1;
		} else if (j == y_ch - 1) {
			nvt_up = j - 1;
			nvt_down = y_ch -1;
		} else {
			nvt_up = j - 1;
			nvt_down = j + 1;
		}
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;

			if (i == 0) {
				left = 0;
				right = i + 1;
			} else if (i == x_ch - 1) {
				left = i - 1;
				right = x_ch - 1;
			} else {
				left = i - 1;
				right = i + 1;
			}
			m=nvt_up * x_ch + i;
			tmpRatio[0] = ABS(PixelRawCmRatio[m] - PixelRawCmRatio[iArrayIndex]);
			m=nvt_down * x_ch + i;
			tmpRatio[1] = ABS(PixelRawCmRatio[m] - PixelRawCmRatio[iArrayIndex]);
			m=j * x_ch + left;
			tmpRatio[2] = ABS(PixelRawCmRatio[m] - PixelRawCmRatio[iArrayIndex]);
			m=j * x_ch + right;
			tmpRatio[3] = ABS(PixelRawCmRatio[m] - PixelRawCmRatio[iArrayIndex]);
			PixelRawCmRatioMax[iArrayIndex] = (int32_t) MAX(MAX(tmpRatio[0], tmpRatio[1]), MAX(tmpRatio[2], tmpRatio[3]));
		}
	}


	printk("%s:\n", __func__);
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			printk("%5d, ", PixelRawCmRatioMax[iArrayIndex]);
			m=7 * iArrayIndex + j * 2;
		}
		printk("\n");
		m=7 * (iArrayIndex + 1) + j * 2;
	}
	printk("\n");

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			if (PixelRawCmRatioMax[iArrayIndex] > PixelRaw_Diff) {
				RecordResult[iArrayIndex] |= 0x01;
				isPass = false;
				TS_LOG_INFO("%s: break at x_ch=%d, y_ch=%d\n", __func__, i, j);
				break;
			}
		}
	}
	TS_LOG_INFO("%s:--\n", __func__);
	if(!isPass) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}

}

int32_t nvt_ctrlram_read_raw(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	int y = 0;
	int iArrayIndex = 0;
	int m=0;
	TS_LOG_INFO("%s:++\n", __func__);
	if (NULL == xdata) {
		TS_LOG_ERR("%s: xdata is Null\n", __func__);
		return -ENOMEM;
	}

	if (nvt_hybrid_clear_fw_status()) {
		TS_LOG_ERR("%s: clear_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_change_mode(NVT_206_TEST_MODE_1);

	if (nvt_hybrid_check_fw_status()) {
		TS_LOG_ERR("%s: check_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_get_fw_info();

	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_206_RAW_PIPE0_ADDR, 0);
	else
		nvt_hybrid_read_mdata(NVT_206_RAW_PIPE1_ADDR, 0);

	nvt_hybrid_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex=y * x_num + x;
			xdata[iArrayIndex] =  (int16_t)xdata[iArrayIndex];
		}
	}

	nvt_hybrid_change_mode(NVT_206_NORMAL_MODE);
	//---for debug---
	printk("%s:\n", __func__);
	for (y = 0; y< nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = (int)(y* nvt_hybrid_ts->ain_rx_num + x);
			printk("%5d, ", xdata[iArrayIndex]);
			m=7 * iArrayIndex +y * 2;
		}
		printk("\n");
		m= 7 * (iArrayIndex + 1) +y* 2;
	}
	printk("\n");

	TS_LOG_INFO("%s:--\n", __func__);
	return 0;
}

int32_t nvt_ctrlram_read_diff(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int m=0;
	TS_LOG_INFO("%s:++\n", __func__);
	if (NULL == xdata) {
		TS_LOG_ERR("%s: xdata is Null\n", __func__);
		return -ENOMEM;
	}
	if (nvt_hybrid_clear_fw_status()) {
		TS_LOG_ERR("%s: clear_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_change_mode(NVT_206_TEST_MODE_1);

	if (nvt_hybrid_check_fw_status()) {
		TS_LOG_ERR("%s: check_fw_status err\n", __func__);
		return -EAGAIN;
	}

	nvt_hybrid_get_fw_info();
	if (nvt_hybrid_get_fw_pipe() == 0)
		nvt_hybrid_read_mdata(NVT_206_DIFF_PIPE0_ADDR, NVT_206_DIFF_BTN_PIPE0_ADDR);
	else
		nvt_hybrid_read_mdata(NVT_206_DIFF_PIPE1_ADDR, NVT_206_DIFF_BTN_PIPE1_ADDR);

	nvt_hybrid_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			m=y * x_num + x;
			xdata[m] =  (int16_t)xdata[m];
		}
	}

	TS_LOG_INFO("%s:change mode\n", __func__);
	nvt_hybrid_change_mode(NVT_206_NORMAL_MODE);
	TS_LOG_INFO("%s:--\n", __func__);

	return 0;
}

static int32_t nvt_206_read_noise(void)
{
	int32_t x = 0;
	int y = 0;
	int iArrayIndex = 0;
	int m=0;

	if (nvt_ctrlram_read_diff(nvt_206_rawdata_diff)) {
		return 1; // read data failed
	}



	//---for debug---
	printk("%s:\n", __func__);
	for (y = 0; y< nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex =y* nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d, ", nvt_206_rawdata_diff[iArrayIndex]);
			m=7 * iArrayIndex +y * 2;
		}
		printk("\n");
		m= 7 * (iArrayIndex + 1) +y* 2;
	}
	printk("\n");


	return 0;
}


/*******************************************************
Description:
	Novatek touchscreen calculate selftest data for huawei_touchscreen.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int16_t nvt_206_get_avg(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t sum=0;
	int i=0;

	if(NULL == p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return (int16_t) sum;
	}
	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		sum += p[i];

	return (int16_t) (sum / (x_ch * y_ch));
}


static int16_t nvt_206_get_max(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t max=INT_MIN;
	int i=0;

	if(NULL == p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return  (int16_t) max;
	}
	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		max = max > p[i] ? max : p[i];

	return (int16_t) max;
}

static int16_t nvt_206_get_min(int32_t *p, uint8_t x_ch, uint8_t y_ch)
{
	int32_t min=INT_MAX;
	int i=0;
	if(NULL == p) {
		TS_LOG_ERR("%s: p is Null\n", __func__);
		return (int16_t) min;
	}
	for (i = 0 ; i < (x_ch * y_ch) ; i++)
		min = min < p[i] ? min : p[i];

	return (int16_t) min;
}

static int nvt_206_mmi_add_static_data(void)
{
 	unsigned int i = 0;

	TS_LOG_ERR("%s enter\n", __func__);


	//1: RawData:	nvt_206_rawdata_fwMutual
	i= (unsigned int) strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_206_get_avg(nvt_206_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_max(nvt_206_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_min(nvt_206_rawdata_fwMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//2: TRx Delta(Pixel Raw):	NVT_206_PixelRawCmRatioMax
	i= (unsigned int) strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_206_get_avg(NVT_206_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_max(NVT_206_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_min(NVT_206_PixelRawCmRatioMax, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//3: Noise(Diff):	nvt_206_rawdata_diff
	i= (unsigned int) strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_206_get_avg(nvt_206_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_max(nvt_206_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_min(nvt_206_rawdata_diff, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	//4-1: Short:		nvt_206_rawdata_short_rxrx
	i=(unsigned int)  strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_206_get_avg(nvt_206_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1),
			nvt_206_get_max(nvt_206_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1),
			nvt_206_get_min(nvt_206_rawdata_short_rxrx, nvt_hybrid_ts->ain_rx_num, 1));

	//4-2: Open:		nvt_206_rawdata_open
	i=(unsigned int) strlen(mmitest_result);
	if  (i >= TP_MMI_RESULT_LEN) {
		return -EINVAL;
	}

	snprintf((mmitest_result + i), (TP_MMI_RESULT_LEN - i), "[%d,%d,%d]",
			nvt_206_get_avg(nvt_206_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_max(nvt_206_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num),
			nvt_206_get_min(nvt_206_rawdata_open, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num));

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen self-test criteria print
	function.

return:
	n.a.
*******************************************************/
static void nvt_206_print_criteria(void)
{
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t iArrayIndex = 0;

	TS_LOG_INFO("%s:++\n", __func__);

	//---PSConfig_Tolerance_Mutual---
	printk("NVT_206_Tolerance_Postive_Mutual: %d\n", NVT_206_Tolerance_Postive_Mutual);
	printk("NVT_206_Tolerance_Negative_Mutual: %d\n", NVT_206_Tolerance_Negative_Mutual);

	//---PSConfig_DiffLimitG_Mutual---
	printk("NVT_206_DiffLimitG_Postive_Mutual: %d\n", NVT_206_DiffLimitG_Postive_Mutual);
	printk("NVT_206_DiffLimitG_Negative_Mutual: %d\n", NVT_206_DiffLimitG_Negative_Mutual);

	//---NVT_206_PixelRaw_Diff---
	printk("NVT_206_PixelRaw_Diff: %d\n", NVT_206_PixelRaw_Diff);

	//---PSConfig_Rawdata_Limit_Short_RXRX---
	printk("NVT_206_Rawdata_Limit_Postive_Short_RXRX: %d\n", NVT_206_Rawdata_Limit_Postive_Short_RXRX);
	printk("NVT_206_Rawdata_Limit_Negative_Short_RXRX: %d\n", NVT_206_Rawdata_Limit_Negative_Short_RXRX);

	//---PS_Config_Lmt_FW_Diff---
	printk("NVT_206_Lmt_FW_Diff_P: %d\n", NVT_206_Lmt_FW_Diff_P);
	printk("NVT_206_Lmt_FW_Diff_N: %d\n", NVT_206_Lmt_FW_Diff_N);
	//---PS_Config_Lmt_FW_Rawdata---
	printk("NVT_206_Lmt_FW_Rawdata_P:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_206_Lmt_FW_Rawdata_P[iArrayIndex]);
		}
		printk("\n");
	}
	printk("NVT_206_Lmt_FW_Rawdata_N:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_206_Lmt_FW_Rawdata_N[iArrayIndex]);
		}
		printk("\n");
	}

	//---NVT_206_BoundaryOpen---
	printk("NVT_206_BoundaryOpen:\n");
	for (y = 0; y < nvt_hybrid_ts->ain_tx_num; y++) {
		for (x = 0; x < nvt_hybrid_ts->ain_rx_num; x++) {
			iArrayIndex = y * nvt_hybrid_ts->ain_rx_num + x;
			printk("%5d ", NVT_206_BoundaryOpen[iArrayIndex]);
		}
		printk("\n");
	}

	TS_LOG_INFO("%s:--\n", __func__);
}

/*******************************************************
Description:
	Novatek touchscreen selftest function for huawei_touchscreen.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
#define NVT_206_TP_TEST_FAILED_REASON_LEN 20
static char selftest_failed_reason[NVT_206_TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
void nvt_FW_Rawdata_Test(struct ts_rawdata_info *info)
{
   	if (nvt_ctrlram_read_raw(nvt_206_rawdata_fwMutual) != 0) {
		NVT_206_TestResult_FWMutual = 1;
		TS_LOG_ERR("%s: nvt_ctrlram_read_raw ERROR! NVT_206_TestResult_FWMutual=%d\n", __func__, NVT_206_TestResult_FWMutual);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		NVT_206_TestResult_FWMutual = nvt_206_rawdata_up_low(nvt_206_rawdata_fwMutual, NVT_206_RecordResult_FWMutual, nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num,
													NVT_206_Lmt_FW_Rawdata_P, NVT_206_Lmt_FW_Rawdata_N);
		if (NVT_206_TestResult_FWMutual == -1){
			TS_LOG_ERR("%s: FW RAWDATA TEST FAIL! NVT_206_TestResult_FWMutual=%d\n", __func__, NVT_206_TestResult_FWMutual);
		} else {
			TS_LOG_INFO("%s: FW RAWDATA TEST PASS! NVT_206_TestResult_FWMutual=%d\n", __func__, NVT_206_TestResult_FWMutual);
		}
	}
}
void nvt_Noise_Test(struct ts_rawdata_info *info)
{
   	if (nvt_206_read_noise() != 0) {
		NVT_206_TestResult_Noise = 1;	// 1: ERROR
		NVT_206_TestResult_FW_Diff = 1;
		TS_LOG_ERR("%s: nvt_206_read_noise ERROR! NVT_206_TestResult_Noise=%d\n", __func__, NVT_206_TestResult_Noise);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		NVT_206_TestResult_FW_Diff = NVT_206_RawDataTest_Sub(NVT_206_BoundaryDiff, nvt_206_rawdata_diff, NVT_206_RecordResult_FW_Diff,
											nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num,
											0, 0,
											0, 0,
											NVT_206_Lmt_FW_Diff_P, NVT_206_Lmt_FW_Diff_N);
		if (NVT_206_TestResult_FW_Diff == -1) {
			NVT_206_TestResult_Noise = -1;
			TS_LOG_ERR("%s: NOISE TEST FAIL! NVT_206_TestResult_Noise=%d\n", __func__, NVT_206_TestResult_Noise);
		} else {
			NVT_206_TestResult_Noise = 0;
			TS_LOG_INFO("%s: NOISE TEST PASS! NVT_206_TestResult_Noise=%d\n", __func__, NVT_206_TestResult_Noise);
		}
	}
}
void nvt_Short_Test_TX_RX(struct ts_rawdata_info *info)
{
    	if (nvt_206_read_short_txrx() != 0) {
		NVT_206_TestResult_Short_TXRX = 1; // 1:ERROR
		TS_LOG_ERR("%s: nvt_206_read_open ERROR! NVT_206_TestResult_Short_TXRX=%d\n", __func__, NVT_206_TestResult_Short_TXRX);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_206_TestResult_Short_TXRX = NVT_206_RawDataTest_Sub(NVT_206_BoundaryShort_TXRX, nvt_206_rawdata_short_txrx, NVT_206_RecordResultShort_TXRX,
												nvt_hybrid_ts->ain_tx_num, nvt_hybrid_ts->ain_rx_num,
												NVT_206_Tolerance_Postive_Short, NVT_206_Tolerance_Negative_Short,
												NVT_206_DiffLimitG_Postive_Short, NVT_206_DiffLimitG_Negative_Short,
												NVT_206_Rawdata_Limit_Postive_Short_TXRX, NVT_206_Rawdata_Limit_Negative_Short_TXRX);
	}
}
void nvt_Short_Test_TX_TX(struct ts_rawdata_info *info)
{
   	if (nvt_206_read_short_txtx() != 0) {
		NVT_206_TestResult_Short_TXTX = 1; // 1:ERROR
		TS_LOG_ERR("%s: nvt_206_read_open ERROR! NVT_206_TestResult_Short_TXTX=%d\n", __func__, NVT_206_TestResult_Short_TXTX);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_206_TestResult_Short_TXTX = NVT_206_RawDataTest_Sub(NVT_206_BoundaryShort_TXTX, nvt_206_rawdata_short_txtx, NVT_206_RecordResultShort_TXTX,
												nvt_hybrid_ts->ain_tx_num, 1,
												NVT_206_Tolerance_Postive_Short, NVT_206_Tolerance_Negative_Short,
												NVT_206_DiffLimitG_Postive_Short, NVT_206_DiffLimitG_Negative_Short,
												NVT_206_Rawdata_Limit_Postive_Short_TXTX, NVT_206_Rawdata_Limit_Negative_Short_TXTX);
	}
}
int32_t nvt_ctrlram_selftest(struct ts_rawdata_info *info)
{
	unsigned long timer_start=jiffies, timer_end=0;
	int retry = 3;
	int i=0;
	int k=0;
	//int index = 0;
	int test_retry = 1;
	uint8_t buf[2] = {0};
	char test_0_result[4]={0};
	char test_1_result[4]={0};
	char test_2_result[4]={0};
	char test_3_result[4]={0};
	char test_4_result[4]={0};
	if (!info) {
		TS_LOG_ERR("%s: info is Null\n", __func__);
		return -ENOMEM;
	}
	NVT_206_TestResult_FWMutual =0;
	NVT_206_TestResult_Short_RXRX = 0;
	NVT_206_TestResult_Short_TXRX = 0;
	NVT_206_TestResult_Short_TXTX = 0;
	NVT_206_TestResult_Open = 0;
	NVT_206_TestResult_Noise = 0;
	NVT_206_TestResult_FW_Diff = 0;
	//NVT_206_TestResult_I2c = 0;
	nvt_206_short_test_rxrx = NULL;
	nvt_206_short_test_txrx = NULL;
	nvt_206_short_test_txtx = NULL;
	nvt_206_open_test = NULL;

	nvt_hybrid_ts->sensor_testing = true;
	memset(nvt_206_rawdata_short_rxrx, 0xFF, sizeof(nvt_206_rawdata_short_rxrx));
	memset(nvt_206_rawdata_short_txrx, 0xFF, sizeof(nvt_206_rawdata_short_txrx));
	memset(nvt_206_rawdata_short_txtx, 0xFF, sizeof(nvt_206_rawdata_short_txtx));
	memset(nvt_206_rawdata_open, 0xFF, sizeof(nvt_206_rawdata_open));
	memset(nvt_206_golden_Ratio, 0xff, sizeof(nvt_206_golden_Ratio));

	TS_LOG_INFO("%s nvt selftest start.\n",__func__);
	//---Test I2C Transfer---
	for(i = 0; i < retry; i++) {
		buf[0] = 0x00;
		if (nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 2) < 0) {
			TS_LOG_ERR("%s: I2C READ FAIL!, retry = %d\n", __func__, retry);
		} else {
			strncpy(test_0_result, "0P-",strlen("0P-"));
			break;
		}
	}

	if(i == retry) {
		TS_LOG_ERR("%s: I2C Check FAIL!\n", __func__);
		strncpy(test_0_result, "0F-",strlen("0F-"));
		goto err_nvt_i2c_read;
	}
	nvt_206_cal_ain_order();

	if(mutex_lock_interruptible(&nvt_hybrid_ts->mp_mutex)) {
		TS_LOG_ERR("%s: mutex_lock_interruptible FAIL!", __func__);
		goto err_mutex_lock_interruptible;
	}

	if (nvt_206_load_mp_criteria()) {
		TS_LOG_ERR("%s, nvt_load_mp_criteria failed\n", __func__);
		if(nvt_206_short_test_rxrx)
			vfree(nvt_206_short_test_rxrx);
		if(nvt_206_short_test_txrx)
			vfree(nvt_206_short_test_txrx);
		if(nvt_206_short_test_txtx)
			vfree(nvt_206_short_test_txtx);
		if(nvt_206_open_test)
			vfree(nvt_206_open_test);
		nvt_206_short_test_rxrx = NULL;
		nvt_206_short_test_txrx = NULL;
		nvt_206_short_test_txtx = NULL;
		nvt_206_open_test = NULL;
		mutex_unlock(&nvt_hybrid_ts->mp_mutex);
		return -EAGAIN;
	}

	//---print criteria---
	if(nvt_hybrid_ts->print_criteria == true) {
		nvt_206_print_criteria();
		nvt_hybrid_ts->print_criteria = false;
	}
	/*
	 * FW Rawdata Test
	 */
	 nvt_FW_Rawdata_Test(info);

	//--- Result for FW Rawdata---
	if (NVT_206_TestResult_FWMutual != 0) {
		strncpy(test_1_result, "1F-",strlen("1F-"));
	} else {
		strncpy(test_1_result, "1P-",strlen("1P-"));
	}

	/*
	 * Noise Test---
	 */
	 nvt_Noise_Test(info);

	//--- Result for Noise Test---
	if (NVT_206_TestResult_Noise != 0) {
		strncpy(test_3_result, "3F-",strlen("3F-"));
	} else {
		strncpy(test_3_result, "3P-",strlen("3P-"));
	}

	//---Reset IC & into idle---
	nvt_hybrid_hw_reset();
	nvt_hybrid_bootloader_reset();
	nvt_hybrid_sw_reset_idle();
	k = 0;

rxrx_test:
	//---Short Test RX-RX---
	if (nvt_206_read_short_rxrx() != 0) {
		NVT_206_TestResult_Short_RXRX = 1; // 1:ERROR
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_206_TestResult_Short_RXRX = NVT_206_RawDataTest_Sub(NVT_206_BoundaryShort_RXRX, nvt_206_rawdata_short_rxrx, NVT_206_RecordResultShort_RXRX,
												nvt_hybrid_ts->ain_rx_num, 1,
												NVT_206_Tolerance_Postive_Short, NVT_206_Tolerance_Negative_Short,
												NVT_206_DiffLimitG_Postive_Short, NVT_206_DiffLimitG_Negative_Short,
												NVT_206_Rawdata_Limit_Postive_Short_RXRX, NVT_206_Rawdata_Limit_Negative_Short_RXRX);
	}
	if (NVT_206_TestResult_Short_RXRX  == 0) {
		TS_LOG_INFO("%s, nvt_read_short_rxrx test pass\n", __func__);
	} else if (NVT_206_TestResult_Short_RXRX  == 1) {
		TS_LOG_INFO("%s, nvt_read_short_rxrx test failed\n", __func__);
		if(k < test_retry) {
			TS_LOG_INFO("rxrx try again:%d\n", i);
			nvt_hybrid_hw_reset();
			nvt_hybrid_bootloader_reset();
			nvt_hybrid_sw_reset_idle();
			k++;
			goto rxrx_test;
		}else {
			goto test_fail;
		}
	}
	k  = 0;

	TS_LOG_INFO("%s, nvt_read_short_rxrx test finish\n", __func__);

txrx_test:
	//---Short Test TX-RX---
	nvt_Short_Test_TX_RX(info);
	if (NVT_206_TestResult_Short_TXRX  == 0) {
		TS_LOG_INFO("%s, nvt_206_read_short_txrx test pass\n", __func__);
	} else if (NVT_206_TestResult_Short_TXRX  == 1) {
		TS_LOG_INFO("%s, nvt_206_read_short_txrx test failed\n", __func__);
		if(k < test_retry) {
			TS_LOG_INFO("txrx try again\n");
			nvt_hybrid_hw_reset();
			nvt_hybrid_bootloader_reset();
			nvt_hybrid_sw_reset_idle();
			k++;
			goto txrx_test;
		}else {
			goto test_fail;
		}
	}
	k = 0;
	TS_LOG_INFO("%s, nvt_206_read_short_txrx test finish\n", __func__);

txtx_test:
	/*
	 * Short Test TX-TX
	 */
	 nvt_Short_Test_TX_TX(info);
	if (NVT_206_TestResult_Short_TXTX  == 0) {
		TS_LOG_INFO("%s, nvt_206_read_short_txtx test pass\n", __func__);
	} else if (NVT_206_TestResult_Short_TXTX  == 1){
		TS_LOG_INFO("%s, nvt_206_read_short_txtx test failed\n", __func__);
		if(k < test_retry) {
			TS_LOG_INFO("txtx try again\n");
			nvt_hybrid_hw_reset();
			nvt_hybrid_bootloader_reset();
			nvt_hybrid_sw_reset_idle();
			k++;
			goto txtx_test;
		}else {
			goto test_fail;
		}
	}
	k = 0;
	TS_LOG_INFO("%s, nvt_206_read_short_txtx test finish\n", __func__);

	//---Reset IC & into idle---
	nvt_hybrid_hw_reset();
	nvt_hybrid_sw_reset_idle();
	msleep(100);


OPEN_test:
	/*
	 * Open Test---
	 */
	if (nvt_206_read_open() != 0) {
		NVT_206_TestResult_Open = 1;	// 1:ERROR
		TS_LOG_ERR("%s: nvt_206_read_open ERROR! NVT_206_TestResult_Open=%d\n", __func__, NVT_206_TestResult_Open);
		info->buff[0] = 0;
		info->buff[1] = 0;
		info->used_size = 0;
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		NVT_206_TestResult_Open = NVT_206_RawDataTest_Sub(NVT_206_BoundaryOpen, nvt_206_rawdata_open, NVT_206_RecordResultOpen, nvt_hybrid_ts->ain_tx_num, nvt_hybrid_ts->ain_rx_num,
											NVT_206_Tolerance_Postive_Mutual, NVT_206_Tolerance_Negative_Mutual,
											NVT_206_DiffLimitG_Postive_Mutual, NVT_206_DiffLimitG_Negative_Mutual,
											0, 0);
	}
	if (NVT_206_TestResult_Open  == 0) {
		TS_LOG_INFO("%s, nvt_read_open test pass\n", __func__);
	} else if (NVT_206_TestResult_Open  == 1) {
		TS_LOG_INFO("%s, nvt_read_open test failed\n", __func__);
		if(k < test_retry) {
			TS_LOG_INFO("open try again\n");
			nvt_hybrid_hw_reset();
			nvt_hybrid_bootloader_reset();
			nvt_hybrid_sw_reset_idle();
			k++;
			goto OPEN_test;
		}else {
			goto test_fail;
		}
	}

	//--- Result for Open & Short Test---
	if ((NVT_206_TestResult_Short_RXRX != 0) ||
		(NVT_206_TestResult_Short_TXRX != 0) ||
		(NVT_206_TestResult_Short_TXTX != 0) ||
		(NVT_206_TestResult_Open != 0)) {
		strncat(test_4_result, "4F", (strlen("4F")+1));
	} else {
		strncat(test_4_result, "4P", (strlen("4P")+1));
	}

	/*
	 * PixelRaw Test
	 */
	if (NVT_206_TestResult_Open == 1) {
		NVT_206_TestResult_PixelRaw = 1;
	} else {
		NVT_206_TestResult_PixelRaw = NVT_206_PixelRawTest_Sub(nvt_206_rawdata_open, NVT_206_PixelRawCmRatio, NVT_206_PixelRawCmRatioMax, NVT_206_RecordResultPixelRaw,
												nvt_hybrid_ts->ain_rx_num, nvt_hybrid_ts->ain_tx_num, NVT_206_PixelRaw_Diff);
	}

	if (NVT_206_TestResult_PixelRaw != 0) {
		strncat(test_2_result, "2F-", (strlen("2F-")+1));
	} else {
		strncat(test_2_result, "2P-", (strlen("2P-")+1));
	}
test_fail:
	TS_LOG_INFO("%s, nvt_read_open test finish\n", __func__);

	//---Reset IC---
	nvt_hybrid_hw_reset();
	nvt_hybrid_bootloader_reset();
	nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);

	if(nvt_206_short_test_rxrx)
		vfree(nvt_206_short_test_rxrx);
	if(nvt_206_short_test_txrx)
		vfree(nvt_206_short_test_txrx);
	if(nvt_206_short_test_txtx)
		vfree(nvt_206_short_test_txtx);
	if(nvt_206_open_test)
		vfree(nvt_206_open_test);
	nvt_206_short_test_rxrx = NULL;
	nvt_206_short_test_txrx = NULL;
	nvt_206_short_test_txtx = NULL;
	nvt_206_open_test = NULL;

	mutex_unlock(&nvt_hybrid_ts->mp_mutex);

	//---Copy Data to info->buff---
	info->buff[0] = nvt_hybrid_ts->ain_rx_num;
	info->buff[1] = nvt_hybrid_ts->ain_tx_num;
	info->used_size = (nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 3)  + 2;	// (Rawdata+Noise+Open) + 2

	memcpy(&info->buff[2], nvt_206_rawdata_fwMutual, (nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num*4));
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num + 2], nvt_206_rawdata_diff, (nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num*4));
	memcpy(&info->buff[nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num * 2 + 2], nvt_206_rawdata_open, (nvt_hybrid_ts->ain_rx_num * nvt_hybrid_ts->ain_tx_num*4));

err_mutex_lock_interruptible:
err_nvt_i2c_read:
	//---Check Fail Reason---
	if((NVT_206_TestResult_Short_RXRX == -1)
		|| (NVT_206_TestResult_Open == -1)
		|| (NVT_206_TestResult_PixelRaw == -1)
		|| (NVT_206_TestResult_Noise ==-1)
		|| (NVT_206_TestResult_FW_Diff == -1)
		|| (NVT_206_TestResult_FWMutual == -1))
		strncpy(selftest_failed_reason, "-panel_reason", NVT_206_TP_TEST_FAILED_REASON_LEN);

	//---String Copy---
	memset(mmitest_result, 0, sizeof(mmitest_result));
	strncat(mmitest_result, test_0_result, strlen(test_0_result));
	strncat(mmitest_result, test_1_result, strlen(test_1_result));
	strncat(mmitest_result, test_2_result, strlen(test_2_result));
	strncat(mmitest_result, test_3_result, strlen(test_3_result));
	strncat(mmitest_result, test_4_result, strlen(test_4_result));
	nvt_206_mmi_add_static_data();
	strncat(mmitest_result, ";", strlen(";"));

	if (0 == strlen(mmitest_result) || strstr(mmitest_result, "F")) {
		strncat(mmitest_result, selftest_failed_reason, strlen(selftest_failed_reason));
	}

	strncat(mmitest_result, "-novatek_", strlen("-novatek_"));
	strncat(mmitest_result, nvt_hybrid_product_id, NVT_HYBRID_PROJECT_ID_LEN);

	//---Copy String to Result---
	memcpy(info->result, mmitest_result, strlen(mmitest_result));

	nvt_hybrid_ts->sensor_testing = false;
	timer_end = jiffies;
	TS_LOG_INFO("%s: self test time:%d\n", __func__, jiffies_to_msecs(timer_end-timer_start));

	return NO_ERR;
}

int32_t nvt_206_read_tp_color(void)
{
	uint8_t buf[64] = {0};
	int retval = NO_ERR;
	char nvt_oncell_tp_color_buf[NVT_ONCELL_TP_COLOR_LEN+1]={0};
	nvt_hybrid_sw_reset_idle();
	// Step 1 : initial bootloader
	retval = NVT_Hybrid_Init_BootLoader();
	if (retval) {
		return retval;
	}
	// Step 2 : unlock
	buf[0] = 0x00;
	buf[1] = 0x35;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: write unlock error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);
	//Step 3 : Flash Read Command
	buf[0] = 0x00;
	buf[1] = 0x03;
	buf[2] = 0x01;
	buf[3] = 0xF0;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x20;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 7);
	if (retval < 0) {
		TS_LOG_ERR("%s: write Read Command error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);
	// Check 0xAA (Read Command)
	buf[0] = 0x00;
	buf[1] = 0x00;
	retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (retval < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!!(%d)\n", __func__, retval);
		return retval;
	}
	if (buf[1] != 0xAA) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}
	msleep(10);
	//Step 4 : Read Flash Data
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x40;
	retval = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_BLDR_Address, buf, 3);
	if (retval < 0) {
		TS_LOG_ERR("%s: change index error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);
	// Read Back
	buf[0] = 0x00;
	retval = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_BLDR_Address, buf, 33);
	if (retval < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Read Command) error!!(%d)\n", __func__, retval);
		return retval;
	}
	msleep(10);
	//buf[13:14]	=> novatek_tp color
	strncpy(nvt_oncell_tp_color_buf, &buf[13], NVT_ONCELL_TP_COLOR_LEN);
	cypress_ts_kit_color[0]=(u8)str_to_hex(nvt_oncell_tp_color_buf);
	nvt_hybrid_bootloader_reset();
	return retval;
}
#endif
