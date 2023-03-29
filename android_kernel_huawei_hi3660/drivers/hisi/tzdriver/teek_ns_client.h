

#ifndef _TEEK_NS_CLIENT_H_
#define _TEEK_NS_CLIENT_H_

#include <linux/mutex.h>
#include <linux/list.h>

#include "teek_client_type.h"
#include "tc_ns_client.h"

/*#define TC_DEBUG*/

#define TC_NS_CLIENT_IOC_MAGIC  't'
#define TC_NS_CLIENT_DEV            "tc_ns_client"
#define TC_NS_CLIENT_DEV_NAME   "/dev/tc_ns_client"

#ifdef TC_DEBUG
#define TCDEBUG(fmt, args...) pr_info("%s(%i, %s): " fmt, \
	 __func__, current->pid, current->comm, ## args)
#else
#define TCDEBUG(fmt, args...)
#endif

#ifdef TC_VERBOSE
#define TCVERBOSE(fmt, args...) pr_debug("%s(%i, %s): " fmt, \
	__func__, current->pid, current->comm, ## args)
#else
#define TCVERBOSE(fmt, args...)
#endif

#define TCERR(fmt, args...) pr_err("%s(%i, %s): " fmt, \
	__func__, current->pid, current->comm, ## args)

/*#define TC_IPI_DEBUG*/

#ifdef TC_IPI_DEBUG
#define TC_TIME_DEBUG(fmt, args...) pr_info("%s(%i, %s): " fmt "\n", \
	__func__, current->pid, current->comm, ## args)
#else
#define TC_TIME_DEBUG(fmt, args...)
#endif

#ifdef CONFIG_SECURE_EXTENSION
#define TC_ASYNC_NOTIFY_SUPPORT
#endif

#define EXCEPTION_MEM_SIZE (8*1024)	/*mem for exception handling*/

#define TSP_REQUEST			0xB2000008
#define TSP_RESPONSE			0xB2000009
#define TSP_REE_SIQ			0xB200000A
#define TSP_CRASH			0xB200000B

#define TSP_PREEMPTED		0xB2000005

#define TC_CALL_GLOBAL		0x01
#define TC_CALL_SYNC		0x02
#define TC_CALL_LOGIN           0x04

#define TEE_REQ_FROM_USER_MODE   0x0
#define TEE_REQ_FROM_KERNEL_MODE 0x1

/* Max sizes for login info buffer comming from teecd */
#define MAX_PACKAGE_NAME_LEN 255
/* The apk certificate format is as follows:
 * modulus_size(4bytes) ||modulus buffer(256 bytes)
 * || exponent size(4 bytes) || exponent buffer(1 bytes)
 */
#define MAX_PUBKEY_LEN 512

struct tag_TC_NS_Shared_MEM;

struct TC_NS_DEV_List {
	unsigned int dev_file_cnt;
	struct mutex dev_lock;
	struct list_head dev_file_list;
};
extern struct TC_NS_DEV_List g_tc_ns_dev_list;

typedef struct tag_TC_NS_DEV_File {
	unsigned int dev_file_id;
	unsigned int service_cnt;
	unsigned int shared_mem_cnt;
	struct mutex service_lock;
	struct mutex shared_mem_lock;
	struct list_head shared_mem_list;
	struct list_head head;
	struct list_head services_list;
	/* Device is linked to call from kernel */
	uint8_t kernel_api;
	/* client login info provided by teecd, can be either package name and public
	 * key or uid(for non android services/daemons) */
	/* login information can only be set once, dont' allow subsequent calls */
	bool login_setup;
	uint32_t pkg_name_len;
	uint8_t pkg_name[MAX_PACKAGE_NAME_LEN];
	uint32_t pub_key_len;
	uint8_t pub_key[MAX_PUBKEY_LEN];
	int load_app_flag;
} TC_NS_DEV_File;

typedef union {
	struct {
		unsigned int buffer;
		unsigned int size;
	} memref;
	struct {
		unsigned int a;
		unsigned int b;
	} value;
} TC_NS_Parameter;

typedef struct tag_TC_NS_Login {
	unsigned int method;
	unsigned int mdata;
} TC_NS_Login;

typedef struct tag_TC_NS_Operation {
	unsigned int paramTypes;
	TC_NS_Parameter params[4];
	unsigned int    buffer_h_addr[4];
	struct tag_TC_NS_Shared_MEM *sharemem[4];
	void *mb_buffer[4];
} TC_NS_Operation;

typedef struct tag_TC_NS_Temp_Buf {
	void *temp_buffer;
	unsigned int size;
} TC_NS_Temp_Buf;

typedef struct  tag_TC_NS_SMC_CMD {
	unsigned int uuid_phys;
	unsigned int uuid_h_phys;
	unsigned int cmd_id;
	unsigned int dev_file_id;
	unsigned int context_id;
	unsigned int agent_id;
	unsigned int operation_phys;
	unsigned int operation_h_phys;
	unsigned int login_method;
	unsigned int login_data_phy;
	unsigned int login_data_h_addr;
	unsigned int login_data_len;
	unsigned int err_origin;
	unsigned int ret_val;
	unsigned int event_nr;
	unsigned int remap;
	unsigned int uid;
#ifdef CONFIG_TEE_SMP
	unsigned int ca_pid;
#endif
#ifdef SECURITY_AUTH_ENHANCE
	unsigned int token_phys;
	unsigned int token_h_phys;
	unsigned int pid;
	unsigned int params_phys;
	unsigned int params_h_phys;
	unsigned int eventindex;	//tee audit event index for upload
#endif
#ifdef CONFIG_TEE_CFC_ABI
	unsigned int real_pid;
#endif
	bool started;
#ifdef SECURITY_AUTH_ENHANCE
	unsigned int chksum;
#endif
}__attribute__((__packed__)) TC_NS_SMC_CMD;

typedef struct tag_TC_NS_Shared_MEM {
	void *kernel_addr;
	void *user_addr;
	unsigned int len;
	bool from_mailbox;
	struct list_head head;
	atomic_t usage;
        atomic_t offset;
} TC_NS_Shared_MEM;

typedef struct tag_TC_NS_Service {
	unsigned char uuid[16];
	struct mutex session_lock;
	struct list_head head;
	struct list_head session_list;
	atomic_t usage;
} TC_NS_Service;

/**
 * @brief
 */
struct TC_wait_data {
	wait_queue_head_t send_cmd_wq;
	int send_wait_flag;
};

#ifdef SECURITY_AUTH_ENHANCE
/* Using AES-CBC algorithm to encrypt communication between secure world and
   normal world.
 */
#define CIPHER_KEY_BYTESIZE 32   /* AES-256 key size */
#define IV_BYTESIZE   16  /* AES-CBC encryption initialization vector size */
#define CIPHER_BLOCK_BYTESIZE 16 /* AES-CBC cipher block size */
#define SCRAMBLING_NUMBER 3
#define CHKSUM_LENGTH  (sizeof(TC_NS_SMC_CMD) - sizeof(uint32_t))

#define HASH_PLAINTEXT_SIZE (MAX_SHA_256_SZ + sizeof(struct encryption_head))
#define HASH_PLAINTEXT_ALIGNED_SIZE \
	ALIGN(HASH_PLAINTEXT_SIZE, CIPHER_BLOCK_BYTESIZE)

enum SCRAMBLING_ID {
	SCRAMBLING_TOKEN = 0,
	SCRAMBLING_OPERATION = 1,
	SCRAMBLING_CMDCRC = 2,
	SCRAMBLING_MAX = SCRAMBLING_NUMBER
};

struct session_crypto_info {
	uint8_t key[CIPHER_KEY_BYTESIZE]; /* AES-256 key */
	uint8_t iv[IV_BYTESIZE]; /* AES-CBC encryption initialization vector */
};

struct session_secure_info {
	uint32_t challenge_word;
	uint32_t scrambling[SCRAMBLING_NUMBER];
	struct session_crypto_info crypto_info;
};

#define MAGIC_SIZE 16
#define MAGIC_STRING "Trusted-magic"

/* One encrypted block, which is aligned with CIPHER_BLOCK_BYTESIZE bytes
 * Head + Payload + Padding
 */
struct encryption_head {
	int8_t magic[MAGIC_SIZE];
	uint32_t crc;
	uint32_t payload_len;
};

struct session_secure_params {
	struct encryption_head head;
	union {
		struct {
			uint32_t challenge_word;
		} ree2tee;
		struct {
			uint32_t scrambling[SCRAMBLING_NUMBER];
			struct session_crypto_info crypto_info;
		} tee2ree;
	} payload;
};
#endif

#ifdef SECURITY_AUTH_ENHANCE
typedef struct tag_TC_NS_Token {
	/* 42byte, token_32byte + timestamp_8byte + kernal_api_1byte + sync_1byte*/
	uint8_t *token_buffer;
} TC_NS_Token;
#endif

typedef struct tag_TC_NS_Session {
	unsigned int session_id;
	struct list_head head;
	struct TC_wait_data wait_data;
	struct mutex ta_session_lock;
#ifdef SECURITY_AUTH_ENHANCE
	/* Session secure enhanced information */
	struct session_secure_info secure_info;
	TC_NS_Token tc_ns_token;
#endif
	atomic_t usage;
} TC_NS_Session;

static inline void get_service_struct(struct tag_TC_NS_Service *service)
{
	if (service)
		atomic_inc(&service->usage);
}

static inline void put_service_struct(struct tag_TC_NS_Service *service)
{
	if (service) {
		if (atomic_dec_and_test(&service->usage))
			kfree(service);
	}
}

static inline void get_session_struct(struct tag_TC_NS_Session *session)
{
	if (session)
		atomic_inc(&session->usage);
}

static inline void put_session_struct(struct tag_TC_NS_Session *session)
{
	if (session) {
		if (atomic_dec_and_test(&session->usage)) {
#ifdef SECURITY_AUTH_ENHANCE
			if (session->tc_ns_token.token_buffer) {
				kfree(session->tc_ns_token.token_buffer);
				session->tc_ns_token.token_buffer = NULL;
				(void)session->tc_ns_token.token_buffer; /* avoid Codex warning */
			}
#endif
			kfree(session);
		}
	}
}

TC_NS_Service *tc_find_service(struct list_head *services, unsigned char *uuid);
TC_NS_Session *tc_find_session(struct list_head *session_list,
			       unsigned int session_id);

#ifdef SECURITY_AUTH_ENHANCE
int set_encryption_head(struct encryption_head *head,
			const uint8_t *data,
			uint32_t len);
int generate_encrypted_session_secure_params(uint8_t *enc_secure_params,
	size_t enc_params_size);
#define ENCRYPT 1
#define DECRYPT 0

int crypto_session_aescbc_key256(uint8_t *in, uint32_t in_len,
                                 uint8_t *out, uint32_t out_len,
                                 const uint8_t *key, uint8_t *iv,
                                 uint32_t mode);
int crypto_aescbc_cms_padding(uint8_t *plaintext, uint32_t plaintext_len,
                              uint32_t payload_len);
#endif

int TC_NS_ClientOpen(TC_NS_DEV_File **dev_file, uint8_t kernel_api);
int TC_NS_ClientClose(TC_NS_DEV_File *dev, int flag);
int is_agent_alive(unsigned int agent_id);

int TC_NS_OpenSession(TC_NS_DEV_File *dev_file, TC_NS_ClientContext *context);
int TC_NS_CloseSession(TC_NS_DEV_File *dev_file,
		       TC_NS_ClientContext *context);
int TC_NS_Send_CMD(TC_NS_DEV_File *dev_file, TC_NS_ClientContext *context);

uint32_t TC_NS_get_uid(void);

#endif
