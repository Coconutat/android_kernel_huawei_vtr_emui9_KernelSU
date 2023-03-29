/*
 *  wac_test.h :
 *  Header File of wacom_test.c
 */

#ifndef WAC_TEST_H
#define WAC_TEST_H



//#include "wacom.h"

//#define WACOM_API

//--Type definitions
typedef __u8 u8;	// one byte
typedef __u16 u16;	// two bytes
typedef __u32 u32;	// four bytes
typedef __s32 S32;
/*
typedef unsigned char u8;			// use u8
typedef unsigned short u16;			// use u16
typedef unsigned long u32;
*/
typedef bool BOOL;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef __s32 TBL_CELL;

//
#define	MAX_RX			(72)	// Max Rx port
#define	MAX_TX			(46)	// Max Tx port
//


//	Function declarations
int wacom_touch_short_test(int *result);
int wacom_touch_open_test(char *i2c_device, int *result);
int wacom_touch_open_short_test(char *i2c_device, int *result);
        // i2c_device:  ( IN),	the device file of wacom_touch_device  for example “/dev/i2c-1”
        // result:      (OUT),  the result code as defined below (ERR_CODE_NO_ERR...)
        // return Value:        0 for OK, others for return code defined as below

int wacom_touch_getTXRX(int *outTX, int *outRX);
int wacom_get_touch_rawdata(S32 dataArray[MAX_TX][MAX_RX]);
		// dataArray:   ( IN),	is a pointer to __s32 rawData[MAX_TX][MAX_RX]

int wacom_get_touch_actdata(S32 dataArray[MAX_TX][MAX_RX]);
		// dataArray:   ( IN),	is a pointer to __s32 actualData[MAX_TX][MAX_RX]

int wacom_check_actdata(int top, int right, int bottom, int left, int threshold);
		// top, right, bottom, left : the ignorleine of each edge
		// threshold : the value for judge max allowed diff of max -min value of each RX
       // return Value:        0 for OK, ERR_CODE_RAW_CALTHR when has error
// WACOM_API
//
// Check result Error code  (All minus)
#define ERR_CODE_NO_ERR		0
#define WACOM_ERR_TBASE     -600
//
#define	ERR_CODE_SHORT_TT	(WACOM_ERR_TBASE-20)
#define	ERR_CODE_SHORT_RR	(WACOM_ERR_TBASE-21)
//
#define	ERR_CODE_FW_VER		(WACOM_ERR_TBASE-1)
#define	ERR_CODE_FW_VID		(WACOM_ERR_TBASE-2)
#define	ERR_CODE_FW_PID		(WACOM_ERR_TBASE-3)
//
#define	ERR_CODE_COM        (WACOM_ERR_TBASE-10)
#define	ERR_CODE_SHORT_TR   (WACOM_ERR_TBASE-22)
#define	ERR_CODE_SHORT_TG	(WACOM_ERR_TBASE-23)
#define	ERR_CODE_SHORT_TV	(WACOM_ERR_TBASE-24)
#define	ERR_CODE_SHORT_RG	(WACOM_ERR_TBASE-25)
#define	ERR_CODE_SHORT_RV	(WACOM_ERR_TBASE-26)
//
#define	ERR_CODE_OPEN_TX	(WACOM_ERR_TBASE-30)
#define	ERR_CODE_OPEN_RX	(WACOM_ERR_TBASE-31)
//
#define ERR_CODE_RAW_UPPER	(WACOM_ERR_TBASE-40)
#define ERR_CODE_RAW_LOWER	(WACOM_ERR_TBASE-41)
#define	ERR_CODE_RAW_CALTHR	(WACOM_ERR_TBASE-42)
#define ERR_CODE_RAW_GAP	(WACOM_ERR_TBASE-43)	// gap check
#define ERR_CODE_RAW_GAP_V	(WACOM_ERR_TBASE-44)	// gap vertical check
//
//

typedef enum
{
	ERR_COM = 0,
	ERR_FWCHECK,
	//
	ERR_SHORT,
	ERR_SHORT_TX,
	ERR_SHORT_TX_GND,
	ERR_SHORT_TX_VDD,
	ERR_SHORT_RX,
	ERR_SHORT_RX_GND,
	ERR_SHORT_RX_VDD,
	ERR_SHORT_TX_RX,
	//
	ERR_OPEN,
	ERR_OPEN_TX,
	ERR_OPEN_RX,
	//
	ERR_RAW,
	ERR_RAW_UPPER,
	ERR_RAW_LOWER,
	ERR_RAW_CALTHR,
	ERR_RAW_GAP,
	ERR_RAW_GAP_V,
	//
	ERR_RESULT,
	ERR_END,

} emError;

enum test_item {
	REAL_RX_NUM,
	REAL_TX_NUM,
	TEST_COUNT,
	ACTUAL_DATA_IGNORE_TOP,
	ACTUAL_DATA_IGNORE_BOTTOM,
	ACTUAL_DATA_IGNORE_LEFT,
	ACTUAL_DATA_IGNORE_RIGHT,
	ACTUAL_DATA_LIMIT,
	RAW_DATA_LIMIT,
	RAW_DATA_REFERENCE,
	GAP_HORIZONTAL,
	GAP_VERTICAL,
	MAX_CASE_CNT,
};



//-----------------------------------------
//	Feature report definitions
//-----------------------------------------
#define	FEATURE_REPID_REPORT_TYPE	(0x03)	// Configuration of touch report type
#define	REPSIZE_REPORT_TYPE		    (64)	// report type
#define	REPTYPE_NORMAL			(0x00)	// normal format
#define	REPTYPE_WACOM           (0x01)  // wacom format
//-----------------------------------------
//#define REPORT_CMD_TOUCH_QUERY	(0x04)	// Query of touch info.
//#define REPSIZE_TOUCH_QUERY		(16)
//-----------------------------------------
#define	FEATURE_REPID_MNT		    (0x09)	// Maintenance Mode
#define	REPSIZE_MNT			        (64)
//-----------------------------------------
#define	FEATURE_REPID_CUSTOM_CMD	(0x0D)	// Custom Command for customer
#define	CUSTOM_CMD_TPKIT		(0x20)
#define	REPSIZE_CUSTOM_CMD_G11	(8)
#define	TPKIT_CMD_RESET			(0x01)	// TPKIT_CMD_GENERAL
#define	TPKIT_CMD_RAW_CHECK		(0x02)
#define	TPKIT_CMD_GET_ACTDATA	(0x03)	// TPKIT_CMD_CALIB_CHECK
#define	TPKIT_CMD_SHORT_CHECK	(0x04)
//-----------------------------------------

#define MAX_FILE_NAME_LENGTH 128

#pragma pack(push,1)
//	Structure of TMP_DATA data
typedef union
{
	UCHAR	b[2];
	USHORT	uw;
	short	sw;
} __attribute__((packed)) TMP_DATA;
#pragma pack(pop)

//	Structure of Panel Info
struct PANEL_INFO
{
	UINT	x;
	UINT	y;
	UINT	pitch;
	UINT	log_x;
	UINT	log_y;
};

//	Structure of PinCheck (Info and state)
struct PIN_CHECK
{
	INT Error[4];
	struct
	{
		INT state;	// 2:pass, 3:fail
		INT vdd;	// 0:pass, 1:fail
		INT gnd;	// 0:pass, 1:fail
	} Rx[MAX_RX];
	struct
	{
		INT state;	// 2:pass, 3:fail
		INT vdd;	// 0:pass, 1:fail
		INT gnd;	// 0:pass, 1:fail
	} Tx[MAX_TX + 2];///each rx use one bit,8 means one byte, add 2 to adjust TX num is  multiple of 8
};

// Calibration threshold check
struct wacom_calInfoStruct {
	struct {
		int	pos;
		S32	val;	// original double value
	} min;
	struct  {
		int	pos;
		S32	val;
	} max;
	S32 dif;
	int err;
};

////	Structure of Lookup-Table//
typedef union{
	UCHAR b[2];
	USHORT w ;
	struct {
	USHORT pin : 7 ;
	USHORT rsv0 : 1 ;
	USHORT en : 1 ;
	USHORT rsv1 : 7 ;
	} u ;
} XLUT;

typedef union{
	UCHAR b[2];
	USHORT w;
	struct {
		USHORT pin : 6;
		USHORT rsv0 : 2;
		USHORT en : 2;
		USHORT rsv1 : 6;
	} u;
} YLUT;
//AAA//

//	Structure of ini
struct MY_INI
{
	//	[Raw Check] & Calibration judge parameter and flags
	int rx_num;
	int tx_num;
	INT data_count;	            // DATA_COUNT 		get raw data data_count times and average it
	BOOL raw_log_only;	        // RAW_LOG_ONLY	raw_check just log
	// calibration check flags
	INT cal_thr;                // CAL_THRESHOLD
	INT calth_ignoreline[4];    // CALTH_IGNORELINE
	BOOL cal_diff;	            // CAL_DIFF
	// gap check flags
	BOOL gap_check;             // GAP_CHECK
	BOOL gap_log_only;          // lGAP_LOG_ONLY 	log only, no judge
	BOOL gap_vertical;          // GAP_VERTICAL
	INT gap_count;              // GAP_COUNT
	INT ignore_count;           // IGNORE_COUNT
	BOOL rescue;                // TOUCH_CALIB_IGNORE

	XLUT xlut[MAX_RX];
	YLUT ylut[MAX_TX];
	//
	// Table for save input reference table
	//
	int *m_ref;
	int *m_upr;
	int *m_lwr;
	int *m_gap;
	int *m_gav;
};

#endif //WAC_TEST_H
