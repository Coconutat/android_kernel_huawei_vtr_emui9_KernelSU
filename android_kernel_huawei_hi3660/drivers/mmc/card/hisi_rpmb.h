
#ifndef __HISI_RPMB_H__
#define __HISI_RPMB_H__

#include <linux/mmc/core.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#define SCSI_RPMB_COMMAND_LENGTH 12
#define MAX_SENSE_BUFFER_LENGTH 16
#define MAX_DXFER_TIME_OUT_MILL 1000
#define FROM_KERNEL_ADDRESS 1

#define UFS_OP_SECURITY_PROTOCOL_IN (0xA2)
#define UFS_OP_SECURITY_PROTOCOL_OUT (0xB5)
#define SECURITY_PROTOCOL (0xEC)
#define WAIT_KEY_FRAME_TIMES 300

/*get key from HISEE*/
#define KEY_NOT_READY 0
#define KEY_READY 1
#define KEY_REQ_FAILED 2

#define RPMB_DRIVER_IS_NOT_READY 0
#define RPMB_DRIVER_IS_READY 1

#define RPMB_DEVICE_IS_NOT_READY 0
#define RPMB_DEVICE_IS_READY 1
#define WAIT_INIT_TIMES 3000
#define WAIT_CONFIG_TIME 5000

/* not defined yet */
#define RPMB_SVC_OK 0x0
#define RPMB_NUM_CALLS 0xF

/* ARM RPMB Service Calls version numbers */
#define RPMB_SVC_VERSION_MAJOR 0x0
#define RPMB_SVC_VERSION_MINOR 0x1

/* SMC function IDs for RPMB Service queries */

/* General and test func */
#define RPMB_SVC_CALL_COUNT 0xc6000000
#define RPMB_SVC_UID 0xc6000001
#define RPMB_SVC_VERSION 0xc6000002

#define RPMB_SVC_TEST 0xc6000003

#define RPMB_COMMON_HANDLER_MASK 0xFF00
#define RPMB_COMMON_HANDLER_VALUE 0xFF00
#define is_common_handler(_smc_fid)                                            \
	(((RPMB_COMMON_HANDLER_MASK) & (_smc_fid)) ==                          \
	 (RPMB_COMMON_HANDLER_VALUE))

#define RPMB_SVC_REQUEST_ADDR 0xc600FF04
#define RPMB_SVC_SET_KEY 0xc600FF05
#define RPMB_SVC_REQUEST_DONE 0xc600FF06
#define RPMB_SVC_GET_DEV_VER 0xc600FFFC

#define RPMB_SVC_SECURE_OS_INFO 0x8600FF10

/* debug command */
#define RPMB_SVC_READ 0xc600FFF1
#define RPMB_SVC_WRITE 0xc600FFF2
#define RPMB_SVC_COUNTER 0xc600FFF3
#define RPMB_SVC_FORMAT 0xc600FFF4
#define RPMB_SVC_WRITE_CAPABILITY	 0xc600FFF6
#define RPMB_SVC_READ_CAPABILITY 0xc600FFF7
#define RPMB_SVC_PARTITION 0xc600FFF8
#define RPMB_SVC_MULTI_KEY 0xc600FFF9
#define RPMB_SVC_CONFIG_VIEW 0xc600FFFA
#define RPMB_SVC_MULTI_KEY_STATUS 0xc600FFFB
/* Request codes */
#define RPMB_REQ_KEY 1
#define RPMB_REQ_WCOUNTER 2
#define RPMB_REQ_WRITE_DATA 3
#define RPMB_REQ_READ_DATA 4
#define RPMB_REQ_STATUS 5

/* Response code */
#define RPMB_RESP_KEY 0x0100
#define RPMB_RESP_WCOUNTER 0x0200
#define RPMB_RESP_WRITE_DATA 0x0300
#define RPMB_RESP_READ_DATA 0x0400

/* Error codes */
#define RPMB_OK 0
#define RPMB_ERR_GENERAL 1
#define RPMB_ERR_AUTH 2
#define RPMB_ERR_COUNTER 3
#define RPMB_ERR_ADDRESS 4
#define RPMB_ERR_WRITE 5
#define RPMB_ERR_READ 6
#define RPMB_ERR_KEY 7
#define RPMB_ERR_CNT_EXPIRED 0x80
#define RPMB_ERR_MSK 0x7

/* rpmb non-standard err code */
#define  RPMB_EXCEED_PART     0xFF01
#define  RPMB_UNKNOWN_PART    0xFF02
#define  RPMB_EXCEED_BUF      0xFF03
#define  RPMB_ERR_MMC_ERR     0xFF04
#define  RPMB_ERR_BLKDEV      0xFF05
#define  RPMB_ERR_MEMALOC     0xFF06
#define  RPMB_ERR_INIT        0xFF07
#define  RPMB_ERR_IOCTL       0xFF08
#define  RPMB_ERR_SET_KEY     0xFF09
#define  RPMB_ERR_GET_COUNT   0xFF0A
#define  RPMB_ERR_TIMEOUT     0xFF0B
#define  RPMB_ERR_DEV_VER     0xFF0C
/* Sizes of RPMB data frame */
#define RPMB_SZ_STUFF 196
#define RPMB_SZ_MAC 32
#define RPMB_SZ_DATA 256
#define RPMB_SZ_NONCE 16

#define SHA256_BLOCK_SIZE 64
#define RPMB_IOCTL_RETRY_TIMES 3
/* Error messages */
static const char *const rpmb_err_msg[] = {
	"",
	"General failure",
	"Authentication failure",
	"Counter failure",
	"Address failure",
	"Write failure",
	"Read failure",
	"Authentication key not yet programmed",
};

/* Structure of RPMB data frame. */
struct rpmb_frame {
	unsigned char stuff[RPMB_SZ_STUFF];
	unsigned char mac[RPMB_SZ_MAC];
	unsigned char data[RPMB_SZ_DATA];
	unsigned char nonce[RPMB_SZ_NONCE];
	unsigned int write_counter;
	unsigned short address;
	unsigned short block_count;
	unsigned short result;
	unsigned short request;
};

enum rpmb_state {
	RPMB_STATE_IDLE,

	RPMB_STATE_KEY,

	RPMB_STATE_RD,

	RPMB_STATE_CNT,

	RPMB_STATE_WR_CNT,
	RPMB_STATE_WR_DATA,
};

struct request_info {
	unsigned int func_id;
	unsigned int blks;
	enum rpmb_state state;
	uint8_t rpmb_region_num;
	struct _current_rqst {
		unsigned int offset; /* offset upon to request_info.base */
		unsigned int blks;   /* current request size */
	} current_rqst;
};

#define RPMB_BLK_SZ (512)
#define MAX_RPMB_WRITE_FRAME (32)
#define MAX_RPMB_READ_FRAME (32)
#define MAX_RPMB_FRAME (32)
#define MAX_HAMC_BUF_SZ (10 * 1024)
#define RPMB_TIMEOUT_TIME_IN_KERNEL 800000000/*may be in kernel we think 800ms is abnormal*/
#define RPMB_MARK_EXIST_STATUS 0x5A5A

struct rpmb_request {
	struct rpmb_frame frame[MAX_RPMB_FRAME];
	unsigned char hmac_buf[MAX_HAMC_BUF_SZ];
	struct rpmb_frame status_frame;
	struct rpmb_frame key_frame;
	struct rpmb_frame error_frame;
	struct request_info info;
	uint16_t key_frame_status;
	uint16_t rpmb_request_status;
	uint16_t rpmb_exception_status;
	enum rpmb_version dev_ver;
	struct _rpmb_debug {
		uint64_t partition_size;
		uint64_t result;
		uint64_t test_time;
		uint64_t test_hisee_atf_read_time;
		uint64_t test_hisee_atf_write_time;
		uint64_t test_hisee_atf_counter_time;
		uint32_t key_num;
		uint32_t func_id;
		uint16_t start;
		uint16_t block_count;
		uint16_t write_value;
		uint16_t read_check;
		uint8_t capability_times;
		uint8_t multi_region_num;
		uint8_t partition_id;
	} rpmb_debug;
};

#endif /* __HISI_RPMB_H__ */
