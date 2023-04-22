#ifndef __HISI_HISEE_FS_H__
#define __HISI_HISEE_FS_H__

#define HISEE_IMAGE_PARTITION_NAME  "hisee_img"
#define HISEE_IMAGE_A_PARTION_NAME   "hisee_img_a"
#define HISEE_IMG_PARTITION_SIZE     (4 * SIZE_1M)
#define HISEE_SW_VERSION_MAGIC_VALUE (0xa5a55a5a)

#define HISEE_ENCOS_PARTITION_NAME  "hisee_encos"

#define HISEE_MIN_MISC_IMAGE_NUMBER  (1)
#ifdef CONFIG_HICOS_MISCIMG_PATCH
#define HISEE_MAX_MISC_IMAGE_NUMBER  (5)
#define HISEE_MISC_NO_UPGRADE_NUMBER (2) /*contains: cos patch and misc_version img*/
#else
#define HISEE_MAX_MISC_IMAGE_NUMBER  (5)
#define HISEE_MISC_NO_UPGRADE_NUMBER (1)
#endif
#define HISEE_SMX_MISC_IMAGE_NUMBER  (10)

#define HISEE_FS_PARTITION_NAME         "/mnt/hisee_fs/"
#define HISEE_COS_FLASH_IMG_NAME        "cos_flash.img"
#define HISEE_COS_FLASH_IMG_FULLNAME    "/mnt/hisee_fs/cos_flash.img"
#define HISEE_CASD_IMG_FULLNAME         "/mnt/hisee_fs/casd.img"


/* Hisee module specific error code*/
#define HISEE_COS_VERIFICATITON_ERROR  (-1002)
#define HISEE_IMG_PARTITION_MAGIC_ERROR  (-1003)
#define HISEE_IMG_PARTITION_FILES_ERROR  (-1004)
#define HISEE_IMG_SUB_FILE_NAME_ERROR    (-1005)
#define HISEE_SUB_FILE_SIZE_CHECK_ERROR   (-1006)
#define HISEE_SUB_FILE_OFFSET_CHECK_ERROR (-1007)
#define HISEE_IMG_SUB_FILE_ABSENT_ERROR  (-1008)
#define HISEE_FS_SUB_FILE_ABSENT_ERROR   (-1009)

#define HISEE_OPEN_FILE_ERROR    (-2000)
#define HISEE_READ_FILE_ERROR    (-2001)
#define HISEE_WRITE_FILE_ERROR   (-2002)
#define HISEE_CLOSE_FILE_ERROR   (-2003)
#define HISEE_LSEEK_FILE_ERROR   (-2004)
#define HISEE_OUTOF_RANGE_FILE_ERROR   (-2005)
#define HISEE_ACCESS_FILE_ERROR   (-2006)
#define HISEE_READ_COSID_ERROR    (-2007)

#define HISEE_FS_MALLOC_ERROR          (-3000)
#define HISEE_FS_PATH_ABSENT_ERROR     (-3001)
#define HISEE_FS_OPEN_PATH_ERROR       (-3002)
#define HISEE_FS_COUNT_FILES_ERROR     (-3003)
#define HISEE_FS_PATH_LONG_ERROR       (-3004)
#define HISEE_FS_READ_FILE_ERROR       (-3005)

#define HISEE_ENCOS_HEAD_INIT_ERROR    (-9000)
#define HISEE_COS_IMG_ID_ERROR   (-9001)
#define HISEE_ENCOS_PARTITION_MAGIC_ERROR    (-9002)
#define HISEE_ENCOS_PARTITION_FILES_ERROR    (-9003)
#define HISEE_ENCOS_PARTITION_SIZE_ERROR    (-9004)
#define HISEE_ENCOS_SUBFILE_SIZE_CHECK_ERROR    (-9005)
#define HISEE_ENCOS_OPEN_FILE_ERROR    (-9006)
#define HISEE_ENCOS_FIND_PTN_ERROR    (-9007)
#define HISEE_ENCOS_LSEEK_FILE_ERROR    (-9008)
#define HISEE_ENCOS_WRITE_FILE_ERROR    (-9009)
#define HISEE_ENCOS_READ_FILE_ERROR    (-9010)
#define HISEE_ENCOS_ACCESS_FILE_ERROR    (-9011)
#define HISEE_ENCOS_CHECK_HEAD_ERROR    (-9012)
#define HISEE_ENCOS_NAME_NULL_ERROR    (-9013)
#define HISEE_ENCOS_SYNC_FILE_ERROR    (-9014)

#define HISEE_MULTICOS_PARAM_ERROR (-18000)
#define HISEE_MULTICOS_READ_UPGRADE_ERROR (-18001)
#define HISEE_MULTICOS_WRITE_UPGRADE_ERROR (-18002)
#define HISEE_MULTICOS_COSID_INVALID_ERROR (-18003)
#define HISEE_MULTICOS_COS_FLASH_FILE_ERROR (-18004)
#define HISEE_MULTICOS_POWERON_UPGRADE_ERROR (-18005)
#define HISEE_MULTICOS_POWEROFF_ERROR        (-18006)
#define HISEE_MULTICOS_IMG_UPGRADE_ERROR     (-18007)
#define HISEE_MULTICOS_COS_FLASH_SIZE_ERROR  (-18008)
#define HISEE_MULTICOS_COS_ID_IS_NOT_EXIST_ERROR  (-18009)

/* hisee image info */
#define HISEE_IMG_MAGIC_LEN		    (4)
#define HISEE_IMG_MAGIC_VALUE		"inse"
#define HISEE_IMG_TIME_STAMP_LEN	(20)
#define HISEE_IMG_DATA_LEN	        (4)
#define HISEE_IMG_SUB_FILE_LEN	    (4)
#define HISEE_IMG_HEADER_LEN        (HISEE_IMG_MAGIC_LEN + HISEE_IMG_TIME_STAMP_LEN + HISEE_IMG_DATA_LEN + HISEE_IMG_SUB_FILE_LEN)

#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
#define HISEE_IMG_SUB_FILE_MAX		(50)
#else
#define HISEE_IMG_SUB_FILE_MAX		(12)
#endif

#define HISEE_IMG_SUB_FILE_NAME_LEN	(8)
#define HISEE_IMG_SUB_FILE_OFFSET_LEN (4)
#define HISEE_IMG_SUB_FILE_DATA_LEN (4)
#define HISEE_IMG_SUB_FILES_LEN     (HISEE_IMG_SUB_FILE_NAME_LEN + HISEE_IMG_SUB_FILE_OFFSET_LEN + HISEE_IMG_SUB_FILE_DATA_LEN)
#define HISEE_IMG_SLOADER_NAME		"SLOADER"
#define HISEE_IMG_COS_NAME		"COS"
#define HISEE_IMG_COS1_NAME		"COS1"
#define HISEE_IMG_COS2_NAME		"COS2"
#define HISEE_IMG_COS3_NAME		"COS3"
#define HISEE_IMG_OTP_NAME		"OTP"
#define HISEE_IMG_OTP0_NAME		"OTP0"
#define HISEE_IMG_OTP1_NAME		"OTP1"
#define HISEE_IMG_MISC_NAME		"MISC"

#ifdef CONFIG_HISEE_NEW_COS_VERSION_HEADER
#define HISEE_COS_VERSION_OFFSET  (16)
#else
#define HISEE_COS_VERSION_OFFSET  (12)
#endif

#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
#ifdef CONFIG_HISEE_NEW_COSID_HEADER
#define HISEE_COS_ID_OFFSET       (12)
#else
#define HISEE_COS_ID_OFFSET       (20)
#endif
#endif

#define HISEE_MAX_RPMB_COS_NUMBER (3)
#define HISEE_MIN_RPMB_COS_NUMBER (1)

#define HISEE_MAX_EMMC_COS_NUMBER (5)
#define HISEE_MIN_EMMC_COS_NUMBER (0)

#define HISEE_MAX_COS_IMAGE_NUMBER_RESERVED (8)
#define HISEE_MAX_COS_IMAGE_NUMBER  (HISEE_MAX_RPMB_COS_NUMBER + HISEE_MAX_EMMC_COS_NUMBER)
#define HISEE_MIN_COS_IMAGE_NUMBER  (1)

#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
#define HISEE_SUPPORT_COS_FILE_NUMBER (HISEE_MAX_COS_IMAGE_NUMBER)
#define HISEE_MAX_SW_VERSION_NUMBER  (HISEE_SUPPORT_COS_FILE_NUMBER)
#define HISEE_COS_EXIST              (0xB43C5A5A)
#else
#define HISEE_SUPPORT_COS_FILE_NUMBER (HISEE_MIN_COS_IMAGE_NUMBER)
#define HISEE_MAX_SW_VERSION_NUMBER  (4)
#endif
#define HISEE_FACTORY_TEST_DEFAULT_COS_FILE_NUMBER  (HISEE_MIN_COS_IMAGE_NUMBER)

#define HISEE_MAX_MISC_ID_NUMBER     (HISEE_MAX_MISC_IMAGE_NUMBER * HISEE_SUPPORT_COS_FILE_NUMBER)

#define HISEE_COS_VERSION_STORE_SIZE  (32)
#define HISEE_UPGRADE_STORE_SIZE      (32)
#define HISEE_MISC_VERSION_STORE_SIZE  (32)

#define MAX_PATH_NAME_LEN    (128)
#define HISEE_FILESYS_DIR_ENTRY_SIZE    (1024)
#define HISEE_FILESYS_DEFAULT_MODE       0660  /* default mode when creating a file or dir if user doesn't set mode*/
#define HISEE_U32_MAX_VALUE             (0xffffffffu)

typedef union _TIMESTAMP_INFO {
	struct timestamp {
		unsigned char second;
		unsigned char minute;
		unsigned char hour;
		unsigned char day;
		unsigned char month;
		unsigned char padding;
		unsigned short year;
	} timestamp;
	unsigned long value;
} timestamp_info;

/* hisee_img partition struct */
typedef struct _IMG_FILE_INFO {
	char name[HISEE_IMG_SUB_FILE_NAME_LEN];
	unsigned int offset;
	unsigned int size;
} img_file_info;

/* multi cos image upgrade information struct */
typedef struct _UPGRADE_TIMESTAMP_INFO {
	timestamp_info img_timestamp;
	unsigned int reserved[2];
} upgrade_timestamp_info;


/* multi cos image upgrade information struct,72 bytes */
typedef struct _MULTICOS_UPGRADE_INFO {
	/*store image upgrade sw version*/
	unsigned char sw_upgrade_version[HISEE_MAX_COS_IMAGE_NUMBER_RESERVED];
	/*store image upgrade timestamp version*/
	upgrade_timestamp_info sw_upgrade_timestamp[HISEE_MAX_COS_IMAGE_NUMBER_RESERVED];
} multicos_upgrade_info;


typedef struct _HISEE_IMG_HEADER {
	char magic[HISEE_IMG_MAGIC_LEN];
	char time_stamp[HISEE_IMG_TIME_STAMP_LEN];
	unsigned int total_size;
	unsigned int file_cnt;
	img_file_info file[HISEE_IMG_SUB_FILE_MAX];

	unsigned int sw_version_cnt[HISEE_MAX_SW_VERSION_NUMBER];
	multicos_upgrade_info cos_upgrade_info;
	unsigned int misc_image_cnt[HISEE_SUPPORT_COS_FILE_NUMBER];
	unsigned int emmc_cos_cnt;
	unsigned int rpmb_cos_cnt;
#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
	unsigned int is_cos_exist[HISEE_SUPPORT_COS_FILE_NUMBER];
#endif
} hisee_img_header;


#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
#define HISEE_ENCOS_MAGIC_LEN 8
#define HISEE_ENCOS_SUB_FILE_MAX    HISEE_MAX_EMMC_COS_NUMBER
#define HISEE_ENCOS_SUB_FILES_LEN 16
#define HISEE_ENCOS_SUB_FILE_NAME_LEN 8
#define HISEE_ENCOS_SUB_FILE_LEN (HISEE_MAX_IMG_SIZE)
#define HISEE_ENCOS_MAGIC_VALUE "encd_cos"
#define HISEE_ENCOS_TOTAL_FILE_SIZE (HISEE_ENCOS_SUB_FILE_MAX * HISEE_ENCOS_SUB_FILE_LEN)

typedef struct _ENCOS_FILE_INFO {
	char name[HISEE_ENCOS_SUB_FILE_NAME_LEN];
	unsigned int offset;
	unsigned int size;
} encos_file_info;

typedef struct _HISEE_ENCOS_HEADER {
	char magic[HISEE_ENCOS_MAGIC_LEN];
	unsigned int total_size;
	unsigned int file_cnt;
	encos_file_info file[HISEE_ENCOS_SUB_FILE_MAX];
} hisee_encos_header;
#endif

typedef enum _HISEE_IMAGE_A_ACCESS_TYPE_ {
	SW_VERSION_READ_TYPE = 0,
	SW_VERSION_WRITE_TYPE,
	COS_UPGRADE_RUN_READ_TYPE = 2,
	COS_UPGRADE_RUN_WRITE_TYPE,
	MISC_VERSION_READ_TYPE = 4,
	MISC_VERSION_WRITE_TYPE,
	COS_UPGRADE_INFO_READ_TYPE = 6,
	COS_UPGRADE_INFO_WRITE_TYPE,
} hisee_image_a_access_type;

#define HISEE_IS_WRITE_ACCESS(access_type)    \
		((SW_VERSION_WRITE_TYPE == (access_type)) \
		|| (COS_UPGRADE_RUN_WRITE_TYPE == (access_type)) \
		|| (MISC_VERSION_WRITE_TYPE == (access_type)) \
		|| (COS_UPGRADE_INFO_WRITE_TYPE == (access_type)))

typedef enum  _HISEE_IMG_FILE_TYPE {
	SLOADER_IMG_TYPE = 0,
	COS_IMG_TYPE,
#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
	COS1_IMG_TYPE,
	COS2_IMG_TYPE,
	COS3_IMG_TYPE,
	COS4_IMG_TYPE,
	COS5_IMG_TYPE,
	COS6_IMG_TYPE,
	COS7_IMG_TYPE,
#endif
	OTP_IMG_TYPE,
	OTP1_IMG_TYPE,
	MISC0_IMG_TYPE,
	MISC1_IMG_TYPE,
	MISC2_IMG_TYPE,
	MISC3_IMG_TYPE,
	MISC4_IMG_TYPE,
#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
	MISC5_IMG_TYPE,
	MISC6_IMG_TYPE,
	MISC7_IMG_TYPE,
	MISC8_IMG_TYPE,
	MISC9_IMG_TYPE,
	MISC10_IMG_TYPE,
	MISC11_IMG_TYPE,
	MISC12_IMG_TYPE,
	MISC13_IMG_TYPE,
	MISC14_IMG_TYPE,
	MISC15_IMG_TYPE,
	MISC16_IMG_TYPE,
	MISC17_IMG_TYPE,
	MISC18_IMG_TYPE,
	MISC19_IMG_TYPE,
	COS_FLASH_IMG_TYPE,/*make sure COS_FLASH_IMG_TYPE as the last vaild IMG_TYPE*/
#endif
	MAX_IMG_TYPE,
} hisee_img_file_type;


typedef struct _COSIMAGE_VERSION_INFO_ {
	unsigned int magic;
	unsigned char img_version_num[HISEE_MAX_SW_VERSION_NUMBER];
	timestamp_info img_timestamp;
} cosimage_version_info;

#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
/* To adapt to previous version, the img_version which is stored
 * in hisee_img partition is divided into two part. */
typedef struct _HISEE_PARTITION_VERSION_INFO_ {
	unsigned int magic;
	unsigned char img_version_num[HISEE_MAX_SW_VERSION_NUMBER / 2];
	timestamp_info img_timestamp;
	unsigned char img_version_num1[HISEE_MAX_SW_VERSION_NUMBER / 2];
} hisee_partition_version_info;
#endif


int write_hisee_otp_value(hisee_img_file_type otp_img_index);
int hisee_parse_img_header(char *buffer);
int filesys_hisee_read_image(hisee_img_file_type type, char *buffer);
int hisee_read_file(const char *fullname, char *buffer, size_t offset, size_t size);
int filesys_read_img_from_file(const char *filename, char *buffer, size_t *file_size, size_t max_read_size);
int hisee_write_file(const char *fullname, char *buffer, size_t size);
int hisee_get_partition_path(char full_path[MAX_PATH_NAME_LEN]);

#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
extern hisee_encos_header g_hisee_encos_header;
int hisee_encos_header_init(void);
int hisee_encos_read(char *data_buf, unsigned int size, unsigned int cos_id);
int hisee_encos_write(char *data_buf, unsigned int size, unsigned int cos_id);
int filesys_rm_cos_flash_file(void);
int check_cos_flash_file_exist(unsigned int *exist_flg);
#endif

int access_hisee_image_partition(char *data_buf, hisee_image_a_access_type access_type);
void parse_timestamp(const char timestamp_str[HISEE_IMG_TIME_STAMP_LEN], timestamp_info *timestamp_value);

#endif
