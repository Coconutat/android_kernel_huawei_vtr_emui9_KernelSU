// Wacom I2C Firmware Flash Program
// Copyright (c) 2013 Tatsunosuke Tobita, Wacom. Co., Ltd.
#ifndef H_WAC_FLASH
#define H_WAC_FLASH


//---------------------------------------//
//--WACOM common-------------------------//
//---------------------------------------//
//--Type definitions
typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;


//-----Update program exit code----------//
//	return Code related to system definitions ( All minus )
//
#define WACOM_RET_OK             0               // Function OK
#define WACOM_EXIT_FAIL          -2
//
#define WACOM_ERRBASE            -700
#define WACOM_ERRBASE            -700
#define WACOM_ERR_NOMEM	         (WACOM_ERRBASE-1)	// No memory, allocation failed
#define WACOM_ERR_DEV_OPEN       (WACOM_ERRBASE-2)	// Device open error, No file
#define WACOM_ERR_DEV_WRITE      (WACOM_ERRBASE-3)	// Device Write error
#define WACOM_ERR_DEV_READ       (WACOM_ERRBASE-4)	// Device Read error
#define WACOM_ERR_DEV_IOCTL 	 (WACOM_ERRBASE-5)	// Device IO control error
#define WACOM_ERR_ERASE_FAIL     (WACOM_ERRBASE-6)	// Flash Erase fail
#define WACOM_ERR_OLD_VERSION    (WACOM_ERRBASE-7)	// Version is older
#define WACOM_ERR_PID_MISMATCH   (WACOM_ERRBASE-8)	// File PID is msimatch
#define WACOM_ERR_FW_FILE_CHECKSUM  (WACOM_ERRBASE-9)	// FW file Check sum error
#define WACOM_ERR_FW_FILE_SIZE   (WACOM_ERRBASE-10)	// FW file Size error
#define WACOM_ERR_FW_FILE_ADDR   (WACOM_ERRBASE-11)	// FW file Start Address error
#define WACOM_ERR_FW_FILE_ERROR  (WACOM_ERRBASE-12)	// FW file incorrect
#define WACOM_ERR_UBL_ERROR 	 (WACOM_ERRBASE-13)	// UBL generic error
#define WACOM_ERR_UBL_ADDR_ERR   (WACOM_ERRBASE-14)	// UBL address error
#define WACOM_ERR_UBL_COMMAND	 (WACOM_ERRBASE-15)	// UBL command fail
#define WACOM_ERR_UBL_TIMEOUT	 (WACOM_ERRBASE-16)	// UBL command time out fail
#define WACOM_ERR_UBL_SETFEATURE (WACOM_ERRBASE-17)	// UBL set feature error
#define WACOM_ERR_UBL_GETFEATURE (WACOM_ERRBASE-18)	// UBL get feature error
//---------------------------------------//



// HW pin id  //caoxiaoxing  fangzhongyi
#define HW_PINID_TRULY           0x03	// default, 2 bits all high
#define HW_PINID_MUTTO           0x02
#define HW_PINID_NOFUNCTION      0x00  // invalid FW, Only boot loader


#pragma pack(push,1)
// For wacom_get_TouchQuery
#define FEATURE_REPID_TOUCH_QUERY   (0x04)	// Query of touch info.
#define REPSIZE_TOUCH_QUERY		    (16)
//	size = 16Bytes --- Get ---
typedef struct{
	u16 lgmax_x;	// Logical Max X  5400
	u16 lgmax_y;	// Logical Max Y
	u16 phmax_x;	// Physical Max X
	u16 phmax_y;	// Physical Max Y
	u16 tracking_finger_max;	// Tracking Finger Max  5
	u16 product_version;	// Product Version   ------------------fw version
 	u8 minor_version;	// Minor Version
	u8 Reserverd_B1;	// 0
	u8 Reserverd_B2;	// Touch+panel HW pin ID (2 pins, default pull high), 0x02, 0x01, 0x00
} TOUCH_QUERY;
#pragma pack(pop)


//---------------------------------------//
//-----FW update operation code----------//
//---------------------------------------//
#define UBL_MAIN_ADDRESS        0x8000      // for g11t
#define UBL_MAIN_SIZE           0x2bfff     // For g11t
#define UBL_MAIN_SIZE_PLUS_ONE  (0x2bfff+1) // For g11t   176k
#define UBL_ROM_SIZE            0x30000     // Changed for merged G11T FW  192k

// Process code
#define UBL_NONE                0x00	// wait for user-command
#define UBL_WRITE               0x01	// regular writing
#define UBL_FORCEWRITE          0x02	// force-writing by ignoring device states
#define UBL_ENCRYPTWRITE        0x03	// to write secured encryption
#define UBL_BASEWRITE           0x04	// to write base-UBL
#define UBL_GETDATA             0x10	// to obtain basic information
#define UBL_CHECKSUM            0x11	// to check checksum
#define UBL_CONFIRM             0x12	// to notify UBL writing succeeded
#define UBL_WRITEEND            0x13	// to wait for UBL back after writing
#define UBL_WAIT                0x14	// to wait for user-process done
#define UBL_FILESEL             0x15	// in selecting files
#define UBL_RESET               0x20	// to reset by UBL booting
#define UBL_COMPVERSION         0x21	// returning firmware version
#define UBL_QUIT                0x22	// showing message
#define UBL_COMPLETE            0x23	// finishing consequence processes

// Returned values
#define UBL_OK                  0x00
#define UBL_ERROR               0x01
#define UBL_ERROR_OPEN          0x02
#define UBL_ERROR_DATACHECK     0x03
#define UBL_ERROR_TIMEOUT       0x04
#define UBL_ERROR_FILEERROR     0x05
#define UBL_ERROR_SETFEATURE    0x10
#define UBL_ERROR_GETFEATURE    0x11
#define UBL_ERROR_REPORTSIZE    0x20
#define UBL_CHECKSUM_NG         0x30
#define UBL_VERSION_NG          0x31
#define UBL_PID_NG              0x32

// In protected state
#define UBL_ALLPROTECT_STATE    0x0F
#define UBL_VERSION_OLD         0x53


// struct(s)
typedef struct{
	u8 command;
	u8 response;
	u32 progress;
	u16 vid;	// device vid
	u16 pid;	// device pid
	u16 version; // device version
//	u16 ret;
	u16 checksum;
	u16 mputype;
	u16 ubl_ver;
	u16 protect;
	u16 ubl_type;
} UBL_STATUS;//IC ÀïÃæµÄ

typedef struct{
	u16 process;
	//u8 data[UBL_MAIN_SIZE_PLUS_ONE];
	u8 *data;// use malloc to allocate memory
	u32 start_adrs;
	u32 max_addres;
	u32 size;
	u16 vid;
	u16 pid;
	u16 version;
	u16 checksum;
	u16 ret;
} UBL_PROCESS;//hex file information


//---------------------------------------//
//-----Boot loader operation code--------//
//---------------------------------------//
#define UBL_G11T_UBL_PID        0x0094      // Boot loader PID
#define UBL_CMD_SIZE_G11T       (256 + 1)	// with report id
#define UBL_RSP_SIZE_G11T   8   // (135 + 1)	// with report id
#define UBL_G11T_CMD_DATA_SIZE  128        // writing in 128 byte chunks
//
#define UBL_TIMEOUT_WRITE       100
#define UBL_RETRY_NUM           5
#define UBL_G11T_MODE_UBL       0x06
#define DEVICETYPE_UBL          0x02
//
#define DEVICETYPE_REPORT_ID    0x02
#define UBL_CMD_REPORT_ID       7
#define UBL_RSP_REPORT_ID       8

// bootloader commands
#define UBL_COM_WRITE		0x01
#define UBL_COM_EXIT		0x03
#define UBL_COM_GETBLVER	0x04
#define UBL_COM_GETMPUTYPE	0x05
#define UBL_COM_CHECKMODE	0x07
#define UBL_COM_FORCEWRITE	0x80
#define UBL_COM_WRITEPROTECT    0x81
#define UBL_COM_CHECKSUM	0x83
#define UBL_COM_ENCRYPTWRITE    0x84
#define UBL_COM_MATCHING	0x85
#define UBL_COM_CONFIRM		0x86
#define UBL_COM_ALLERASE	0x90
#define UBL_COM_BASEWRITE	0x91

// bootloader responses
#define UBL_RES_OK              0x00
#define UBL_RES_BUSY            0x80
#define UBL_RES_MCUTYPE_ERROR   0x0C
#define UBL_RES_PID_ERROR       0x0D
#define UBL_RES_VERSION_ERROR   0x0E

#define UBL_RES_BUSYBIT        0x80
#define UBL_RES_ERRORBIT      0x40


//related to get pinid
#define FW_BINDATA_ID1 0x00
#define FW_BINDATA_ID2 0x00
#define FW_BINDATA_ID3 0x0F
#define FW_BINDATA_ID4 0xD9

#define FW_VERSION_ONE_BYTE  0xFC
#define FW_VERSION_TWO_BYTE  0xFD
#define FW_VERSION_MORE_BYTE  0xFB

#define MAX_OFFSET_SIZE 512
#define FW_VERSION_EXTRA_OFFSET_BYTE_0  0x0C
#define FW_VERSION_EXTRA_OFFSET_BYTE_1	0x00
#define FW_VERSION_EXTRA_OFFSET_BYTE_2	0xFA

#define RECORD_TYPE_0 0
#define RECORD_TYPE_1 1
#define RECORD_TYPE_2 2
#define RECORD_TYPE_3 3
#define RECORD_TYPE_4 4
#define RECORD_TYPE_5 5

#define HEXADECIMAL 16

#pragma pack(push,1)
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u8 op[4];
} boot_cmd_header;


/*
* WRITE_FLASH - write flash memory
*/
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u32 addr;	/* address must be divisible by 2 */
	u8 size8;		/* size must be divisible by 8*/
	u8 data[UBL_CMD_SIZE_G11T - 1 - 3 - sizeof(u32)];
} boot_cmd_write_flash;	// must be 256+1 bytes


/*
 * ERASE_FLASH - erase flash memory
 */
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u8 blkNo;	/* block No. */
} boot_cmd_erase_flash;


typedef union
{
/*
 * data field is used to make all commands the same length
 */
	u8 data[UBL_CMD_SIZE_G11T];
	boot_cmd_header header;
	boot_cmd_write_flash write_flash;
	boot_cmd_erase_flash erase_flash;
} boot_cmd;

/*
 * common for all responses fields
 */
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u8 resp;
} boot_rsp_header;

/*
* WRITE_FLASH - write flash memory
*/
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u8 resp;
} boot_rsp_write_flash;

/*
* ERASE_FLASH - erase flash memory
*/
typedef struct
{
	u8 reportId;
	u8 cmd;		/* command code, see BOOT_xxx constants */
	u8 echo;		/* echo is used to link between command and response */
	u8 resp;
} boot_rsp_erase_flash;


typedef union
{
/*
 * data field is used to make all responses the same length
 */
	u8 data[UBL_RSP_SIZE_G11T];
	boot_rsp_header header;
	boot_rsp_write_flash write_flash;
	boot_rsp_erase_flash erase_flash;
} boot_rsp;
#pragma pack(pop)


#endif //H_WACFLASH
