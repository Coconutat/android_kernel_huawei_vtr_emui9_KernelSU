#include "ftsSoftware.h"
#include <../../../huawei_touchscreen_chips.h>

//#define PATH_FILE_LIMITS					"/storage/emulated/0/Android/data/stm_fts_production_limits.csv"
#define LIMITS_FILE							"stm_fts_production_limits.csv"

#define WAIT_FOR_FRESH_FRAMES					50				//ms

#define TIMEOUT_ITO_TEST_RESULT					200				//ms
#define TIMEOUT_INITIALIZATION_TEST_RESULT		5000			//ms

#define PANEL_SENSELEN	27
#define PANEL_FORCELEN	15

//LABELS PRODUCTION TEST LIMITS FILE
#define MS_RAW_MIN_MAX				"MS_RAW_DATA_MIN_MAX"
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

#define	MUTUALRAWTYPE 0x01
#define SELFFORCERAWTYPE 0x02
#define SELFSENSERAWTYPE 0x03
#define STRENGTHTYPE 0x04

struct production_data_limit {
	int MutualRawMax;
	int MutualRawMin;
	int MutualCx2Max;
	int MutualCx2Min;
	int MutualRawGapLimit;

	int SelfForceRawMax;
	int SelfForceRawMin;
	int SelfForceIxTotalMax;
	int SelfForceIxTotalMin;

	int SelfSenseRawMax;
	int SelfSenseRawMin;
	int SelfSenseIxTotalMax;
	int SelfSenseIxTotalMin;

	int MutualStrengthMax;
	int MutualStrengthMin;

	int32_t* MutualRawMaxTab;
	int32_t* MutualRawMinTab;
	int32_t* SelfForceRawMaxTab;
	int32_t* SelfForceRawMinTab;
	int32_t* SelfSensorRawMaxTab;
	int32_t* SelfSensorRawMinTab;

	struct ts_rawdata_limit_tab limit_tab_data;
};

typedef struct {
	int MutualRaw;
	int MutualCx1;
	int MutualCx2;
	int MutualCxTotal;

	int SelfForceRaw;
	int SelfForceIx1;
	int SelfForceIx2;
	int SelfForceIxTotal;
	int SelfForceCx1;
	int SelfForceCx2;
	int SelfForceCxTotal;

	int SelfSenseRaw;
	int SelfSenseIx1;
	int SelfSenseIx2;
	int SelfSenseIxTotal;
	int SelfSenseCx1;
	int SelfSenseCx2;
	int SelfSenseCxTotal;
} TestToDo;

typedef struct {
	bool Init_Res;
	bool MutualRawRes;
	bool MutualRawResGap;
	bool MutualCx2Res;
	bool MutualStrengthRes;
	bool SelfForceRawRes;
	bool SelfForceIxTotalRes;
	bool SelfSenseRawRes;
	bool SelfSenseIxTotalRes;
	bool ITO_Test_Res;
	char max_min_aver_buf[50];
} TestResult;

int computeAdjHoriz(u8* data, int row, int column, u8**result);
int computeAdjHorizTotal(u16* data, int row, int column, u16** result);
int computeAdjVert(u8* data, int row, int column, u8**result);
int computeAdjVertTotal(u16* data, int row, int column, u16**result);
int computeTotal(u8* data, u8 main, int row, int column, int m, int n, u16**result);
int checkLimitsMinMax(short *data, int row, int column, int min, int max);
int checkLimitsMap(u8 *data, int row, int column, int *min, int *max);
int checkLimitsMapTotal(u16 *data, int row, int column, int *min, int *max);
int checkLimitsMapAdj(u8 *data, int row, int column, int *max);
int checkLimitsMapAdjTotal(u16 *data, int row, int column, int *max);
int production_test_main(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result);
int production_test_ms_raw(struct ts_rawdata_info *info, struct production_data_limit *limitdata, TestResult *result);
int production_test_ms_cx(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result);
int production_test_ss_raw(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result);
int production_test_ss_ix_cx(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result);
int production_test_data(struct ts_rawdata_info *info, int stop_on_fail, TestToDo *todo, struct production_data_limit *limitdata, TestResult *result);
