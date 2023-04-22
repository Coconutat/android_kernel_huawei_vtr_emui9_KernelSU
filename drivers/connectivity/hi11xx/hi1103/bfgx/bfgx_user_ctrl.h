

#ifndef __BFGX_USER_CTRL_H__
#define __BFGX_USER_CTRL_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include "plat_type.h"
#include "hisi_customize_wifi.h"
#include "hisi_ini.h"
/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define DEV_SW_MARK_STR_BFGX            ("@DEV_SW_VERSION_BFGX")
#define DEV_SW_MARK_STR_WIFI            ("@DEV_SW_VERSION_WIFI")
#define DEV_SW_VERSION_HEAD5BYTE        ("HI1103")

#define USERCTL_KFREE(p)    do{\
                                if (likely(NULL != p))\
                                {\
                                    kfree(p);\
                                    p = NULL;\
                                }\
                            }while(0)

enum PLAT_LOGLEVLE{
	PLAT_LOG_ALERT = 0,
	PLAT_LOG_ERR = 1,
	PLAT_LOG_WARNING = 2,
	PLAT_LOG_INFO = 3,
	PLAT_LOG_DEBUG = 4,
};

enum UART_DUMP_CTRL{
    UART_DUMP_CLOSE =0,
    UART_DUMP_OPEN=1,
};

enum BUG_ON_CTRL{
    BUG_ON_DISABLE = 0,
    BUG_ON_ENABLE  = 1,
};

enum ROTATE_STATE
{
    ROTATE_FINISH     = 0,
    ROTATE_NOT_FINISH = 1,
};
/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

typedef struct
{
    wait_queue_head_t dump_type_wait;
    struct sk_buff_head dump_type_queue;
    atomic_t  rotate_finish_state;
}DUMP_CMD_QUEUE;

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/
extern struct kobject *g_sysfs_hi110x_bfgx_etc;
extern int32 g_plat_loglevel_etc;
extern int32 g_bug_on_enable_etc;

#ifdef PLATFORM_DEBUG_ENABLE
extern int32 g_uart_rx_dump_etc;
#endif

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern int32 bfgx_user_ctrl_init_etc(void);
extern void bfgx_user_ctrl_exit_etc(void);
#ifndef HI110X_HAL_MEMDUMP_ENABLE
extern int32 plat_send_rotate_cmd_2_app_etc(uint32 which_dump);
extern void plat_wait_last_rotate_finish_etc(void);
extern void plat_rotate_finish_set_etc(void);
#endif
extern void plat_exception_dump_file_rotate_init_etc(void);

#endif

