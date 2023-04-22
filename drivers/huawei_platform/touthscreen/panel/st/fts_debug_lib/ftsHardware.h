


//DUMMY BYTES DATA
#define DUMMY_HW_REG				1 
#define DUMMY_FRAMEBUFFER			1
#define DUMMY_MEMORY				1

//DIGITAL CHIP INFO
#define DCHIP_ID_0					0x39
#define DCHIP_ID_1					0x6C
#define DCHIP_ID_ADDR				0x0007
#define DCHIP_FW_VER_ADDR			0x000A
#define DCHIP_FW_VER_BYTE			2

//CHUNKS
#define READ_CHUNK					2*1024
#define WRITE_CHUNK					2*1024
#define MEMORY_CHUNK				2*1024

//PROTOCOL INFO
#define I2C_SAD						0x49
#define I2C_INTERFACE				//comment if the chip use SPI
#define ICR_ADDR					0x0024
#define ICR_SPI_VALUE				0x02

//SYSTEM RESET INFO
#define SYSTEM_RESET_ADDRESS		0x0023
#define SYSTEM_RESET_VALUE			0x01

//INTERRUPT INFO
#define IER_ADDR					0x001C
#define IER_ENABLE					0x41
#define IER_DISABLE					0x00

//FLASH COMAND
#define FLASH_CMD_BURN				0xF2
#define FLASH_CMD_ERASE				0xF3
#define FLASH_CMD_READSTATUS		0xF4
#define FLASH_CMD_UNLOCK			0xF7
#define FLASH_CMD_WRITE_LOWER_64	0xF0
#define FLASH_CMD_WRITE_UPPER_64	0xF1

//FLASH UNLOCK PARAMETER
#define FLASH_UNLOCK_CODE0			0x74
#define FLASH_UNLOCK_CODE1			0x45

//FLASH ADDRESS
#define FLASH_ADDR_SWITCH_CMD		0x00010000
#define FLASH_ADDR_CODE				0x00000000
#define FLASH_ADDR_CONFIG			0x0001E800
#define FLASH_ADDR_CX				0x0001F000

//SIZES FW, CODE, CONFIG, MEMH
#define FW_SIZE						(int)(128*1024)
#define FW_CODE_SIZE				(int)(122*1024)
#define FW_CONFIG_SIZE				(int)(2*1024)
#define FW_CX_SIZE					(int)(FW_SIZE-FW_CODE_SIZE-FW_CONFIG_SIZE)
#define FW_VER_MEMH_BYTE1			193
#define FW_VER_MEMH_BYTE0			192
#define FW_OFF_CONFID_MEMH_BYTE1	2
#define FW_OFF_CONFID_MEMH_BYTE0	1

//FIFO
#define FIFO_EVENT_SIZE				8
#define FIFO_DEPTH					32

#define FIFO_CMD_READONE			0x85
#define FIFO_CMD_READALL			0x86
#define FIFO_CMD_LAST				0x87
#define FIFO_CMD_FLUSH				0xA1


//CONSTANT TOTAL IX AND CX
#define SS_FORCE_IX1_WEIGHT			2
#define SS_FORCE_IX2_WEIGHT			1
#define SS_SENSE_IX1_WEIGHT			2
#define SS_SENSE_IX2_WEIGHT			1

#define CX1_WEIGHT					300
#define CX2_WEIGHT					75

//OP CODES FOR MEMORY (based on protocol)

#define FTS_CMD_HW_REG_R			0xB6
#define FTS_CMD_HW_REG_W			0xB6
#define FTS_CMD_FRAMEBUFFER_R		0xD0
#define FTS_CMD_FRAMEBUFFER_W		0xD0




