

#include "ftsHardware.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

//FTS FW COMAND
#define FTS_CMD_MS_MT_SENSE_OFF				0x92
#define FTS_CMD_MS_MT_SENSE_ON				0x93
#define FTS_CMD_SS_HOVER_OFF				0x94
#define FTS_CMD_SS_HOVER_ON					0x95
#define FTS_CMD_LP_TIMER_CALIB				0x97
#define FTS_CMD_FULL_INITIALIZATION			0xA5
#define FTS_CMD_ITO_CHECK					0xA7
#define FTS_CMD_REQU_FW_CONF				0xB2
#define FTS_CMD_REQU_COMP_DATA				0xB8



//Event ID 
#define EVENTID_NO_EVENT					0x00
#define EVENTID_ERROR_EVENT					0x0F
#define EVENTID_CONTROLLER_READY			0x10
#define EVENTID_FW_CONFIGURATION			0x12
#define EVENTID_COMP_DATA_READ				0x13
#define EVENTID_STATUS_UPDATE				0x16

//EVENT TYPE
#define EVENT_TYPE_ITO						0x05
#define EVENT_TYPE_FULL_INITIALIZATION		0x07


// CONFIG ID INFO	
#define CONFIG_ID_ADDR						0x0001
#define CONFIG_ID_BYTE						2


//ADDRESS OFFSET IN SYSINFO
#define ADDR_RAW_TOUCH						0x0000
#define ADDR_FILTER_TOUCH					0x0002
#define ADDR_NORM_TOUCH						0x0004
#define ADDR_CALIB_TOUCH					0x0006
#define ADDR_RAW_HOVER_FORCE				0x000A
#define ADDR_RAW_HOVER_SENSE				0x000C
#define ADDR_FILTER_HOVER_FORCE				0x000E
#define ADDR_FILTER_HOVER_SENSE				0x0010
#define ADDR_NORM_HOVER_FORCE				0x0012
#define ADDR_NORM_HOVER_SENSE				0x0014
#define ADDR_CALIB_HOVER_FORCE				0x0016
#define ADDR_CALIB_HOVER_SENSE				0x0018
#define ADDR_RAW_PRX_FORCE					0x001A
#define ADDR_RAW_PRX_SENSE					0x001C
#define ADDR_FILTER_PRX_FORCE				0x001E
#define ADDR_FILTER_PRX_SENSE				0x0020
#define ADDR_NORM_PRX_FORCE					0x0022
#define ADDR_NORM_PRX_SENSE					0x0024
#define ADDR_CALIB_PRX_FORCE				0x0026
#define ADDR_CALIB_PRX_SENSE				0x0028
#define ADDR_COMP_DATA						0x0050


//ADDRESS FW REGISTER
#define ADDR_SENSE_LEN						0x0014
#define ADDR_FORCE_LEN						0x0015


//B2 INFO
#define B2_DATA_BYTES						4
#define B2_CHUNK							(FIFO_DEPTH/2)*B2_DATA_BYTES			//number of bytes



