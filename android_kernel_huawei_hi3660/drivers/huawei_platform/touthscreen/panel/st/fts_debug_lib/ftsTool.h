#define TIMEOUT_RESOLUTION		10								//ms
#define GENERAL_TIMEOUT			50*TIMEOUT_RESOLUTION			//ms

#define SYSTEM_RESET_RETRY		3

#define B2_RETRY				2

int readB2(u16 address, u8* outBuf, int len);
int readB2U16(u16 address, u8* outBuf, int byteToRead);
char* printHex(char* label, u8* buff, int count);
int pollForEvent(int * event_to_search, int event_bytes, u8* readData, int time_to_wait);
int disableInterrupt(void);
int enableInterrupt(void);
int u8ToU16(u8* src, u16* dst);
int u8ToU16_le(u8* src, u16* dst);
int u8ToU16n(u8* src, int src_length, u16* dst);
int u16ToU8(u16 src, u8* dst);
int u16ToU8_le(u16 src, u8* dst);
int u16ToU8_be(u16 src, u8* dst);
int u16ToU8n(u16* src, int src_length, u8* dst);
int attempt_function(int(*code)(void), unsigned long wait_before_retry, int retry_count);
int system_reset(void);
int senseOn(void);
int senseOff(void);
void print_frame_short(char *label,short **matrix, int row, int column);
short** array1dTo2d_short(short* data, int size, int columns);
u8** array1dTo2d_u8(u8* data, int size, int columns);
void print_frame_u8(char *label, u8 **matrix, int row, int column);
void print_frame_u32(char *label, u32 **matrix, int row, int column);
void print_frame_int(char *label, int **matrix, int row, int column);
int cleanUp(int enableTouch);
int flushFIFO(void);
