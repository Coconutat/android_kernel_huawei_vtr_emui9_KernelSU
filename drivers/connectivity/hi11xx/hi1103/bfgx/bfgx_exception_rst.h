

#ifndef __BFGX_EXCEPTION_RST_H__
#define __BFGX_EXCEPTION_RST_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_exception_rst.h"

/*****************************************************************************
  2 Define macro
*****************************************************************************/
/*plat cfg cmd */
#define PLAT_CFG_IOC_MAGIC                     'z'
#define PLAT_DFR_CFG_CMD                       _IOW(PLAT_CFG_IOC_MAGIC, PLAT_DFR_CFG, int)
#define PLAT_BEATTIMER_TIMEOUT_RESET_CFG_CMD   _IOW(PLAT_CFG_IOC_MAGIC, PLAT_BEATTIMER_TIMEOUT_RESET_CFG, int)
#define PLAT_BFGX_CALI_CMD                     _IOW(PLAT_CFG_IOC_MAGIC, PLAT_BFGX_CALI, int)
#ifndef HI110X_HAL_MEMDUMP_ENABLE
#define PLAT_DUMP_FILE_READ_CMD                _IOW(PLAT_CFG_IOC_MAGIC, PLAT_DUMP_FILE_READ, int)
#define PLAT_DUMP_ROTATE_FINISH_CMD            _IOW(PLAT_CFG_IOC_MAGIC, PLAT_DUMP_ROTATE_FINISH, int)

/*hal excp cmd*/
/*plat cfg cmd */
#define DFR_HAL_IOC_MAGIC                     'e'
#define DFR_HAL_GNSS_CFG_CMD                   _IOW(DFR_HAL_IOC_MAGIC, DFR_HAL_GNSS, int)
#define DFR_HAL_BT_CFG_CMD                     _IOW(DFR_HAL_IOC_MAGIC, DFR_HAL_BT, int)
#define DFR_HAL_FM_CFG_CMD                     _IOW(DFR_HAL_IOC_MAGIC, DFR_HAL_FM, int)

enum DFR_HAL_EM
{
    DFR_HAL_GNSS                           = 0,
    DFR_HAL_BT                             = 1,
    DFR_HAL_FM                             = 2,
    DFR_HAL_BOTT,
};
#endif
#define PLAT_EXCEPTION_ENABLE                  (1)
#define PLAT_EXCEPTION_DISABLE                 (0)

#define DUMP_BCPU_MEM_BUFF_LEN                 (256*4)
#define DUMP_BCPU_REG_BUFF_LEN                 (256*4)
#define WAKEUP_RETRY_TIMES                     (3)
#define WLAN_OPEN_BEFORE_DUMP                  (1)
#define WLAN_CLOSE_BEFROE_DUMP                 (0)
#define HI110X_DFR_WAIT_SSI_IDLE_MS           (5000)

#ifdef HISI_TOP_LOG_DIR
#define BFGX_DUMP_PATH HISI_TOP_LOG_DIR"/hi110x/memdump"
#define WIFI_DUMP_PATH HISI_TOP_LOG_DIR"/wifi/memdump"
#else
#define BFGX_DUMP_PATH "/data/memdump"
#define WIFI_DUMP_PATH "/data/memdump"
#endif
#ifdef HI110X_HAL_MEMDUMP_ENABLE
#define PLAT_BFGX_EXCP_CFG_IOC_MAGIC            'b'
#define DFR_HAL_GNSS_CFG_CMD                   _IOW(PLAT_BFGX_EXCP_CFG_IOC_MAGIC, DFR_HAL_GNSS, int)
#define DFR_HAL_BT_CFG_CMD                     _IOW(PLAT_BFGX_EXCP_CFG_IOC_MAGIC, DFR_HAL_BT, int)
#define DFR_HAL_FM_CFG_CMD                     _IOW(PLAT_BFGX_EXCP_CFG_IOC_MAGIC, DFR_HAL_FM, int)
#define DFR_HAL_IR_CFG_CMD                     _IOW(PLAT_BFGX_EXCP_CFG_IOC_MAGIC, DFR_HAL_IR, int)
#define PLAT_BFGX_DUMP_FILE_READ_CMD           _IOW(PLAT_BFGX_EXCP_CFG_IOC_MAGIC, PLAT_BFGX_DUMP_FILE_READ, int)

typedef enum PLAT_BFGX_EXCP_CMD_e
{
    DFR_HAL_GNSS                           = 0,
    DFR_HAL_BT                             = 1,
    DFR_HAL_FM                             = 2,
    DFR_HAL_IR                             = 3,
    PLAT_BFGX_DUMP_FILE_READ               = 4,

    PLAT_BFGX_EXCP_CMD_BOTT,
}plat_bfgx_excp_cmd_m;

#define PLAT_WIFI_EXCP_CFG_IOC_MAGIC                 'w'
#define PLAT_WIFI_DUMP_FILE_READ_CMD                _IOW(PLAT_WIFI_EXCP_CFG_IOC_MAGIC, PLAT_WIFI_DUMP_FILE_READ, int)
typedef enum PLAT_WIFI_EXCP_CMD_e
{
    DFR_HAL_WIFI                           = 0,
    PLAT_WIFI_DUMP_FILE_READ               = 1,
}plat_WIFI_excp_cmd_m;
#endif
/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/
typedef struct DUMP_MEM_RES
{
    uint8 *file_name;
    uint32 start_addr;
    uint32 align_type;
    uint32 men_len;
}exception_bcpu_dump_msg;
enum dump_msg_align_type
{
    ALIGN_1_BYTE    = 0,
    ALIGN_2_BYTE    = 1,
    ALIGN_4_BYTE    = 2,
};

typedef struct memdump_driver_s {
    struct sk_buff_head quenue;
    int32 is_open;
    int32 is_working;
    wait_queue_head_t dump_type_wait;
    struct sk_buff_head dump_type_queue;
}memdump_info_t;

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 全局变量定义
*****************************************************************************/
enum PLAT_CFG
{
    PLAT_DFR_CFG                           = 0,
    PLAT_BEATTIMER_TIMEOUT_RESET_CFG       = 1,
    PLAT_CFG_BUFF,

    PLAT_DUMP_FILE_READ                    = 100,
    PLAT_DUMP_ROTATE_FINISH                = 101,
    PLAT_BFGX_CALI                         = 102,
};

enum BFGX_DUMP_TYPE
{
    BFGX_PUB_REG                           = 0,
    BFGX_PRIV_REG                          = 1,
    BFGX_MEM                               = 2,

    SDIO_BFGX_MEM_DUMP_BOTTOM,
};


/*****************************************************************************
  6 EXTERN FUNCTION
*****************************************************************************/
extern void plat_dfr_cfg_set_etc(uint64 arg);
extern void plat_beatTimer_timeOut_reset_cfg_set_etc(uint64 arg);
extern int32 bfgx_store_stack_mem_to_file_etc(void);
extern int32 plat_get_excp_total_cnt(void);
extern int32 plat_get_dfr_sinfo(char* buf,int32 index);
extern int32 is_dfr_test_en(enum excp_test_cfg_em excp_cfg);
extern void set_excp_test_en(enum excp_test_cfg_em excp_cfg);
#ifdef HI110X_HAL_MEMDUMP_ENABLE
extern void plat_beatTimer_timeOusdio_read_device_memt_reset_cfg_set(uint64 arg);
extern void bfgx_memdump_finish_etc(void);
extern void wifi_memdump_finish_etc(void);
extern int32 wifi_notice_hal_memdump_etc(void);
extern int32 bfgx_notice_hal_memdump_etc(void);
extern int32 bfgx_memdump_quenue_clear_etc(void);
extern void wifi_memdump_quenue_clear_etc(void);
extern int32 bfgx_memdump_enquenue_etc(uint8 *buf_ptr, uint16 count);
extern int32 wifi_memdump_enquenue_etc(uint8 *buf_ptr, uint16 count);
extern memdump_info_t bcpu_memdump_cfg_etc;
extern memdump_info_t wcpu_memdump_cfg_etc;
#endif

#endif

