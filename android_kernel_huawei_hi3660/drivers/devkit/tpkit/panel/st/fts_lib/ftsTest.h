/*

**************************************************************************
**                        STMicroelectronics 		                **
**************************************************************************
**                        marco.cali@st.com				**
**************************************************************************
*                                                                        *
*               	FTS API for MP test				 *
*                                                                        *
**************************************************************************
**************************************************************************

*/


#include "ftsSoftware.h"
#include "../../../huawei_ts_kit.h"

#define LIMITS_FILE							"stm_fts_production_limits.csv"


#define WAIT_FOR_FRESH_FRAMES				100 //ms
#define WAIT_AFTER_SENSEOFF				50 //ms

#define TIMEOUT_ITO_TEST_RESULT				200 //ms
#define TIMEOUT_INITIALIZATION_TEST_RESULT		5000 //ms

 //LABELS PRODUCTION TEST LIMITS FILE
#define MS_STRENGTH_MIN_MAX			"MS_STRENGTH_DATA_MIN_MAX"
#define MS_RAW_MIN_MAX				"MS_RAW_DATA_MIN_MAX"
#define MS_RAW_GAP				"MS_RAW_DATA_GAP"
#define MS_RAW_GAP_CHECK_AFTER_CAL "MS_RAW_GAP_CHECK_AFTER_CAL"
#define MS_CX1_MIN_MAX				"MS_TOUCH_ACTIVE_CX1_MIN_MAX"
#define MS_CX2_MAP_MIN				"MS_TOUCH_ACTIVE_CX2_MIN"
#define MS_CX2_MAP_MAX				"MS_TOUCH_ACTIVE_CX2_MAX"
#define MS_CX2_ADJH_MAP_MAX			"MS_TOUCH_ACTIVE_CX2_ADJ_HORIZONTAL"
#define MS_CX2_ADJV_MAP_MAX			"MS_TOUCH_ACTIVE_CX2_ADJ_VERTICAL"
#define MS_TOTAL_CX_MAP_MIN			"MS_TOUCH_ACTIVE_TOTAL_CX_MIN"
#define MS_TOTAL_CX_MAP_MAX			"MS_TOUCH_ACTIVE_TOTAL_CX_MAX"
#define MS_TOTAL_CX_ADJH_MAP_MAX	"MS_TOUCH_ACTIVE_TOTAL_CX_ADJ_HORIZONTAL"
#define MS_TOTAL_CX_ADJV_MAP_MAX	"MS_TOUCH_ACTIVE_TOTAL_CX_ADJ_VERTICAL"
#define SS_RAW_FORCE_MIN_MAX		"SS_RAW_DATA_FORCE_MIN_MAX"
#define SS_RAW_SENSE_MIN_MAX		"SS_RAW_DATA_SENSE_MIN_MAX"
#define SS_PRX_FORCE_MIN_MAX		"SS_PRX_DATA_FORCE_MIN_MAX"
#define SS_PRX_SENSE_MIN_MAX		"SS_PRX_DATA_SENSE_MIN_MAX"
#define SS_COM_SENSE_DATA_MIN_MAX		"SS_COM_SENSE_DATA_MIN_MAX"


#define SS_RAW_FORCE_GAP		"SS_RAW_DATA_FORCE_GAP"
#define SS_RAW_SENSE_GAP		"SS_RAW_DATA_SENSE_GAP"
#define SS_IX1_FORCE_MIN_MAX		"SS_TOUCH_ACTIVE_IX1_FORCE_MIN_MAX"
#define SS_IX1_SENSE_MIN_MAX		"SS_TOUCH_ACTIVE_IX1_SENSE_MIN_MAX"
#define SS_CX1_FORCE_MIN_MAX		"SS_TOUCH_ACTIVE_CX1_FORCE_MIN_MAX"
#define SS_CX1_SENSE_MIN_MAX		"SS_TOUCH_ACTIVE_CX1_SENSE_MIN_MAX"
#define SS_IX2_FORCE_MAP_MIN		"SS_TOUCH_ACTIVE_IX2_FORCE_MIN"
#define SS_IX2_FORCE_MAP_MAX		"SS_TOUCH_ACTIVE_IX2_FORCE_MAX"
#define SS_IX2_SENSE_MAP_MIN		"SS_TOUCH_ACTIVE_IX2_SENSE_MIN"
#define SS_IX2_SENSE_MAP_MAX		"SS_TOUCH_ACTIVE_IX2_SENSE_MAX"
#define SS_IX2_FORCE_ADJV_MAP_MAX	"SS_TOUCH_ACTIVE_IX2_ADJ_VERTICAL"
#define SS_IX2_SENSE_ADJH_MAP_MAX	"SS_TOUCH_ACTIVE_IX2_ADJ_HORIZONTAL"
#define SS_CX2_FORCE_MAP_MIN		"SS_TOUCH_ACTIVE_CX2_FORCE_MIN"
#define SS_CX2_FORCE_MAP_MAX		"SS_TOUCH_ACTIVE_CX2_FORCE_MAX"
#define SS_CX2_SENSE_MAP_MIN		"SS_TOUCH_ACTIVE_CX2_SENSE_MIN"
#define SS_CX2_SENSE_MAP_MAX		"SS_TOUCH_ACTIVE_CX2_SENSE_MAX"
#define SS_CX2_FORCE_ADJV_MAP_MAX	"SS_TOUCH_ACTIVE_CX2_ADJ_VERTICAL"
#define SS_CX2_SENSE_ADJH_MAP_MAX	"SS_TOUCH_ACTIVE_CX2_ADJ_HORIZONTAL"

// TOTAL SS
#define SS_TOTAL_IX_FORCE_MAP_MIN		"SS_TOUCH_ACTIVE_TOTAL_IX_FORCE_MIN"
#define SS_TOTAL_IX_FORCE_MAP_MAX		"SS_TOUCH_ACTIVE_TOTAL_IX_FORCE_MAX"
#define SS_TOTAL_IX_SENSE_MAP_MIN		"SS_TOUCH_ACTIVE_TOTAL_IX_SENSE_MIN"
#define SS_TOTAL_IX_SENSE_MAP_MAX		"SS_TOUCH_ACTIVE_TOTAL_IX_SENSE_MAX"
#define SS_TOTAL_IX_FORCE_ADJV_MAP_MAX	"SS_TOUCH_ACTIVE_TOTAL_IX_ADJ_VERTICAL"
#define SS_TOTAL_IX_SENSE_ADJH_MAP_MAX	"SS_TOUCH_ACTIVE_TOTAL_IX_ADJ_HORIZONTAL"
#define SS_TOTAL_CX_FORCE_MAP_MIN		"SS_TOUCH_ACTIVE_TOTAL_CX_FORCE_MIN"
#define SS_TOTAL_CX_FORCE_MAP_MAX		"SS_TOUCH_ACTIVE_TOTAL_CX_FORCE_MAX"
#define SS_TOTAL_CX_SENSE_MAP_MIN		"SS_TOUCH_ACTIVE_TOTAL_CX_SENSE_MIN"
#define SS_TOTAL_CX_SENSE_MAP_MAX		"SS_TOUCH_ACTIVE_TOTAL_CX_SENSE_MAX"
#define SS_TOTAL_CX_FORCE_ADJV_MAP_MAX	"SS_TOUCH_ACTIVE_TOTAL_CX_ADJ_VERTICAL"
#define SS_TOTAL_CX_SENSE_ADJH_MAP_MAX	"SS_TOUCH_ACTIVE_TOTAL_CX_ADJ_HORIZONTAL"

//KEYS
#define MS_KEY_RAW_MIN_MAX			"MS_KEY_RAW_DATA_MIN_MAX"
#define MS_KEY_CX1_MIN_MAX			"MS_KEY_CX1_MIN_MAX"
#define MS_KEY_CX2_MAP_MIN			"MS_KEY_CX2_MIN"
#define MS_KEY_CX2_MAP_MAX			"MS_KEY_CX2_MAX"
#define MS_KEY_TOTAL_CX_MAP_MIN		"MS_KEY_TOTAL_CX_MIN"
#define MS_KEY_TOTAL_CX_MAP_MAX		"MS_KEY_TOTAL_CX_MAX"

//CONSTANT TOTAL IX
#define SS_IX1_FORCE_W                      "IX1_FORCE_W"
#define SS_IX2_FORCE_W                      "IX2_FORCE_W"
#define SS_IX1_SENSE_W                      "IX1_SENSE_W"
#define SS_IX2_SENSE_W                      "IX2_SENSE_W"


#define SCAN_EN_MS_SCR		0x0001
#define SCAN_EN_MS_KEY		0x0002
#define SCAN_EN_MS_MRN		0x0004
#define SCAN_EN_SS_HVR		0x0008
#define SCAN_EN_SS_TCH		0x0010
#define SCAN_EN_SS_KEY		0x0020
#define SCAN_EN_MS_SCR_SEC	0x0040
#define SCAN_EN_MS_SCR_LP	0x0100
#define SCAN_EN_MS_SIDE_TCH	0x0200

#define SAVE_FLAG_RETRY		3

#define FTS_LIMIT_FILE_NAME_MAX_LEN 64

typedef struct {
	int MutualRaw;
        int MutualRawGap;
	int MutualCx1;
	int MutualCx2;
	int MutualCx2Adj;
	int MutualCxTotal;
	int MutualCxTotalAdj;

	int MutualKeyRaw;
	int MutualKeyCx1;
	int MutualKeyCx2;
	int MutualKeyCxTotal;

	int SelfForceRaw;
        int SelfForceRawGap;
	int SelfForceIx1;
	int SelfForceIx2;
	int SelfForceIx2Adj;
	int SelfForceIxTotal;
	int SelfForceIxTotalAdj;
	int SelfForceCx1;
	int SelfForceCx2;
	int SelfForceCx2Adj;
	int SelfForceCxTotal;
	int SelfForceCxTotalAdj;

	int SelfSenseRaw;
        int SelfSenseRawGap;
	int SelfSenseIx1;
	int SelfSenseIx2;
	int SelfSenseIx2Adj;
	int SelfSenseIxTotal;
	int SelfSenseIxTotalAdj;
	int SelfSenseCx1;
	int SelfSenseCx2;
	int SelfSenseCx2Adj;
	int SelfSenseCxTotal;
	int SelfSenseCxTotalAdj;

} TestToDo;

#define ST_NP_TEST_RES_BUF_LEN 50

typedef struct {
	bool I2c_Check;
	bool Init_Res;
	bool MutualRawRes;
	bool MutualRawResGap;
	bool MutualCx2Res;
	bool MutualCx2Adj;
	bool MutualStrengthRes;
	bool SelfForceRawRes;
	bool SelfForceIxTotalRes;
	bool SelfSenseRawRes;
	bool SelfSenseStrengthData;
	bool SelfForceStrengthData;
	bool SelfSenseIxTotalRes;
	bool ITO_Test_Res;
	bool SelfSenseData;
	char mutal_raw_res_buf[ST_NP_TEST_RES_BUF_LEN];
	char mutal_noise_res_buf[ST_NP_TEST_RES_BUF_LEN];
	char mutal_cal_res_buf[ST_NP_TEST_RES_BUF_LEN];

} TestResult;

#define	MUTUALRAWTYPE 0x01
#define SELFFORCERAWTYPE 0x02
#define SELFSENSERAWTYPE 0x03
#define STRENGTHTYPE 0x04
#define MUTUALCOMPENSATIONTYPE 0x05
#define SSFORCEPRXTYPE 0x06
#define SSSENSEPRXTYPE 0x07
#define SSSENSEDATATYPE 0x08


extern chipInfo ftsInfo;

int computeAdjHoriz(short* data, int row, int column, u8**result);
int computeAdjHorizTotal(short* data, int row, int column, u16** result);
int computeAdjVert(short* data, int row, int column, u8**result);
int computeAdjVertTotal(short* data, int row, int column, u16**result);
int computeTotal(short* data, u8 main, int row, int column, int m, int n, u16**result);
int checkLimitsMinMax(short *data, int row, int column, int min, int max);
int checkLimitsMap(short *data, int row, int column, int *min, int *max);
int checkLimitsMapTotal(short *data, int row, int column, int *min, int *max);
int checkLimitsMapAdj(short *data, int row, int column, int *max);
int checkLimitsMapAdjTotal(short *data, int row, int column, int *max);
int production_test_ito(TestResult *result);
int production_test_initialization(u32 signature,TestResult *result);
int ms_compensation_tuning(void);
int ss_compensation_tuning(void);
int lp_timer_calibration(void);
int save_cx_tuning(void);
int production_test_splitted_initialization(int saveToFlash);
int production_test_main(char * pathThresholds, int stop_on_fail, int saveInit,TestToDo *todo, u32 signature,struct ts_rawdata_info *info,TestResult *result);
int production_test_ms_raw(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info, TestResult *result);
int production_test_ms_cx(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info, TestResult *result);
int production_test_ss_raw(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info, TestResult *result);
int production_test_data(char *path_limits, int stop_on_fail, TestToDo *todo,struct ts_rawdata_info *info, TestResult *result);
int production_test_ms_key_cx(char *path_limits, int stop_on_fail, TestToDo *todo);
int production_test_ms_key_raw(char *path_limits);
int save_mp_flag(u32 signature);
int parseProductionTestLimits(char * path, char *label, int **data, int *row, int *column);
int readLine(char *data, char *line, int size, int *n);
int fts_calibrate(int calibrate_type);

