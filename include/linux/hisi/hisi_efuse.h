#ifndef _HISILICON_EFUSE_H_
#define _HISILICON_EFUSE_H_

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/string.h>


typedef enum tag_efuse_log_level {
	log_level_disable = 0,
	log_level_error = 1,
	log_level_warning,
	log_level_debug,
	log_level_info ,
	log_level_total = log_level_info
} efuse_log_level_t;

typedef enum tag_efuse_mem_attr {
	efuse_mem_attr_none = -1,
	efuse_mem_attr_huk = 0,
	efuse_mem_attr_scp,
	efuse_mem_attr_authkey,
	efuse_mem_attr_chipid,
	efuse_mem_attr_tsensor_calibration,
	efuse_mem_attr_huk_rd_disable,
	efuse_mem_attr_authkey_rd_disable,
	efuse_mem_attr_dbg_class_ctrl,
	efuse_mem_attr_dieid,
#ifdef CONFIG_HISI_DEBUG_FS
	efuse_mem_attr_sltfinishflag,
#endif
	efuse_mem_attr_max
} efuse_mem_attr_t;

typedef struct tag_efuse_attribution_info {
	u32 bits_width;
} efuse_attr_info_t;

typedef struct tag_efusec_data{
	u32 efuse_group_max;
	phys_addr_t  paddr;
	unsigned char *vaddr;
	s32 (*atf_fn)(u64, u64, u64, u64);
	efuse_attr_info_t efuse_attrs_from_dts[efuse_mem_attr_max];
	struct mutex efuse_mutex;
	u32 is_init_success;
} efusec_data_t;

#define OK                                 (0)     /* 成功 */
#define ERROR                              (-1)    /* 包括参数错误和不支持 两种错误情况*/
#define ERROR_EXIT_PD                      (-2)    /* 不能退出power down模式 */
#define ERROR_ENTER_PD                     (-3)    /* 不能退出power down模式 */
#define ERROR_APB_PGM_DIS                  (-4)    /* 当前eFusec不允许烧写 */
#define ERROR_EFUSEC_READ                  (-5)    /* 不能完成一次eFuse读操作 */
#define ERROR_PRE_WRITE                    (-6)    /* 未完成预烧写置位 */
#define ERROR_PG_OPERATION                 (-7)    /* 不能完成一次eFuse写操作 */


#define HISI_EFUSE_READ_CHIPID             (0x1000)
#define HISI_EFUSE_READ_DIEID              (0x2000)
#define HISI_EFUSE_WRITE_CHIPID            (0x3000)
#define HISI_EFUSE_READ_AUTHKEY            (0x4000)
#define HISI_EFUSE_WRITE_AUTHKEY           (0x5000)
#define HISI_EFUSE_READ_CHIPIDLEN          (0x6000)
#define HISI_EFUSE_WRITE_DEBUGMODE         (0x7000)
#define HISI_EFUSE_READ_DEBUGMODE          (0x8000)
#define HISI_EFUSE_READ_THERMAL            (0x9000)

#ifdef CONFIG_HISI_DEBUG_FS
#define HISI_EFUSE_TEST_WR                 (0xa001)
#define HISI_EFUSE_TEST_READ_CHIPID        (0xa002)
#define HISI_EFUSE_TEST_READ_DIEID         (0xa003)
#define HISI_EFUSE_TEST_READ_KCE           (0xa004)
#define HISI_EFUSE_TEST_WRITE_KCE          (0xa005)
#endif

#ifdef CONFIG_HISI_DEBUG_FS
#define HISI_EFUSE_WRITE_SLTFINISHFLAG     (0xb000)
#define HISI_EFUSE_READ_SLTFINISHFLAG      (0xb001)
#define EFUSE_SLTFINISHFLAG_LENGTH_BYTES   (4)
#endif

#define HISI_EFUSE_WRITE_DJTAGDEBUG        (0xc000)
#define HISI_EFUSE_READ_DJTAGDEBUG         (0xd000)
#define HISI_EFUSE_READ_DESKEW             (0xe000)
#define EFUSE_TIMEOUT_1000MS               (1000)
#define EFUSE_MODULE_INIT_SUCCESS          (0x12345678)

#define EFUSE_KCE_LENGTH_BYTES             (16)
#define EFUSE_HISEE_LENGTH_BYTES           (8)
#define EFUSE_DIEID_LENGTH_BYTES           (20)
#define EFUSE_CHIPID_LENGTH_BYTES          (8)
#define EFUSE_AUTHKEY_LENGTH_BYTES         (8)
#define EFUSE_SECDBG_LENGTH_BYTES          (4)
#define EFUSE_THERMAL_LENGTH_BYTES         (8)
#define EFUSE_FREQ_LENGTH_BYTES            (4)
#define EFUSE_DESKEW_LENGTH_BYTES          (1)

#ifdef CONFIG_HI3XXX_EFUSE
extern s32 get_efuse_dieid_value(unsigned char *buf, u32 u32Lenght, u32 timeout);
extern s32 get_efuse_chipid_value(unsigned char *buf, u32 u32Lenght, u32 timeout);
extern s32 get_efuse_thermal_value(unsigned char *buf, u32 size, u32 timeout);
extern s32 get_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout);
extern s32 set_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout);
extern s32 get_efuse_freq_value(unsigned char *buf, u32 size);
extern s32 get_efuse_kce_value(unsigned char *buf, u32 size, u32 timeout);
extern s32 set_efuse_kce_value(unsigned char *buf, u32 size, u32 timeout);
extern s32 get_efuse_deskew_value(unsigned char *buf, u32 size, u32 timeout);
#else
static inline s32 get_efuse_dieid_value(unsigned char *buf, u32 u32Lenght, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_chipid_value(unsigned char *buf, u32 u32Lenght, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_thermal_value(unsigned char *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 set_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_freq_value(unsigned char *buf, u32 size)
{
	return OK;
}

static inline s32 get_efuse_kce_value(unsigned char *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 set_efuse_kce_value(unsigned char *buf, u32 size, u32 timeout)
{
	return OK;
}
static inline s32 get_efuse_deskew_value(unsigned char *buf, u32 size, u32 timeout)
{
         return OK;
}
#endif

#ifdef CONFIG_HI3XXX_EFUSE
#define  EFUSE_DIEID_GROUP_START         (32)
#define  EFUSE_DIEID_GROUP_WIDTH         (5)
#define  EFUSE_CHIPID_GROUP_START        (57)
#define  EFUSE_CHIPID_GROUP_WIDTH        (2)
#define  EFUSE_KCE_GROUP_START           (28)
#define  EFUSE_KCE_GROUP_WIDTH           (4)
#endif

#endif
