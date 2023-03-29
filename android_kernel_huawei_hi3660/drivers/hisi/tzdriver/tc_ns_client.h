

#ifndef _TC_NS_CLIENT_H_
#define _TC_NS_CLIENT_H_

#include "teek_client_type.h"

#ifdef SECURITY_AUTH_ENHANCE
#define SCRAMBLING_KEY_LEN    4
#define TOKEN_BUFFER_LEN    42   /* token(32byte) + timestamp(8byte) + kernal_api(1byte) + sync(1byte)*/
#define TIMESTAMP_BUFFER_INDEX    32
#define KERNAL_API_INDEX    40
#define SYNC_INDEX    41
#define TIMESTAMP_LEN_DEFAULT \
	((KERNAL_API_INDEX) - (TIMESTAMP_BUFFER_INDEX))
#define KERNAL_API_LEN \
	((TOKEN_BUFFER_LEN) - (KERNAL_API_INDEX))
#define TIMESTAMP_SAVE_INDEX    16
#endif

#ifndef ZERO_SIZE_PTR
#define ZERO_SIZE_PTR ((void *)16)
#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= (unsigned long)ZERO_SIZE_PTR)
#endif

typedef struct {
	__u32 method;
	__u32 mdata;
} TC_NS_ClientLogin;

typedef union {
	struct {
		__u64 buffer;
		__u64 offset;
		__u64 size_addr;
	} memref;
	struct {
		__u64 *a_addr;
		__u64 *b_addr;
	} value;
} TC_NS_ClientParam;

typedef struct {
	__u32 code;
	__u32 origin;
} TC_NS_ClientReturn;

typedef struct {
	unsigned char uuid[16];
	__u32 session_id;
	__u32 cmd_id;
	TC_NS_ClientReturn returns;
	TC_NS_ClientLogin login;
	TC_NS_ClientParam params[4];
	__u32 paramTypes;
	__u8 started;
#ifdef SECURITY_AUTH_ENHANCE
	void* teec_token;
#endif
  	__u32 callingPid;
} TC_NS_ClientContext;

typedef struct {
	uint32_t seconds;
	uint32_t millis;
} TC_NS_Time;

struct load_app_ioctl_struct {
	TEEC_UUID uuid;
	uint32_t file_size;
	union {
		char *file_buffer;
		unsigned long long file_addr;
	};
};

#define	vmalloc_addr_valid(kaddr) \
	(((void *)(kaddr) >= (void *)VMALLOC_START) && \
	((void *)(kaddr) < (void *)VMALLOC_END))

#define	modules_addr_valid(kaddr) \
	(((void *)(kaddr) >= (void *)MODULES_VADDR) && \
	((void *)(kaddr) < (void *)MODULES_END))

#define TST_CMD_01 (1)
#define TST_CMD_02 (2)
#define TST_CMD_03 (3)
#define TST_CMD_04 (4)
#define TST_CMD_05 (5)

#define MAX_SHA_256_SZ 32

#define TC_NS_CLIENT_IOCTL_SES_OPEN_REQ \
	 _IOW(TC_NS_CLIENT_IOC_MAGIC, 1, TC_NS_ClientContext)
#define TC_NS_CLIENT_IOCTL_SES_CLOSE_REQ \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 2, TC_NS_ClientContext)
#define TC_NS_CLIENT_IOCTL_SEND_CMD_REQ \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 3, TC_NS_ClientContext)
#define TC_NS_CLIENT_IOCTL_SHRD_MEM_RELEASE \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 4, unsigned int)
#define TC_NS_CLIENT_IOCTL_WAIT_EVENT \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 5, unsigned int)
#define TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 6, unsigned int)
#define TC_NS_CLIENT_IOCTL_REGISTER_AGENT \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 7, unsigned int)
#define TC_NS_CLIENT_IOCTL_UNREGISTER_AGENT \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 8, unsigned int)
#define TC_NS_CLIENT_IOCTL_LOAD_APP_REQ \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 9, struct load_app_ioctl_struct)
#define TC_NS_CLIENT_IOCTL_NEED_LOAD_APP \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 10, TC_NS_ClientContext)
#define TC_NS_CLIENT_IOCTL_ALLOC_EXCEPTING_MEM \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 12, unsigned int)
#define TC_NS_CLIENT_IOCTL_CANCEL_CMD_REQ \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 13, TC_NS_ClientContext)
#define TC_NS_CLIENT_IOCTL_LOGIN \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 14, int)
#define TC_NS_CLIENT_IOCTL_TST_CMD_REQ \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 15, int)
#define TC_NS_CLIENT_IOCTL_TUI_EVENT \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 16, int)
#define TC_NS_CLIENT_IOCTL_SYC_SYS_TIME \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 17, TC_NS_Time)
#define TC_NS_CLIENT_IOCTL_SET_NATIVE_IDENTITY \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 18, int)
#define TC_NS_CLIENT_IOCTL_LOAD_TTF_FILE_AND_NOTCH_HEIGHT \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 19, unsigned int)
#define TC_NS_CLIENT_IOCTL_LOW_TEMPERATURE_MODE\
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 20, unsigned int)

#ifdef CONFIG_HISI_CLANG
#define TZ_CRYPTO_SHASH_DESCSIZE    128
#endif
#endif
