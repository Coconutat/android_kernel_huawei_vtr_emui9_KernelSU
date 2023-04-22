/*
 *
 * Copyright (C), 2001-2021, Huawei Tech. Co., Ltd.
 *
 * File Name     : hisi_ini.h
 * Version	     : Initial Draft
 * Created	     : 2012/8/26
 * Last Modified :
 * Description   : hisi_ini.c header file
 * Function List :
 * History 	     :
 * 1.Date		 : 2013/10/21
 *   Modification: Created file
 *
 */

#ifndef __HISI_INI_H__
#define __HISI_INI_H__

/*
 * 1 Other Header File Including
 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "hw_bfg_ps.h"
#include "bfgx_user_ctrl.h"
#endif
#include "plat_type.h"


/*
 * 2 Macro Definition
 */
#define INI_TIME_TEST

#define INI_MIN(_A, _B) (((_A) < (_B))? (_A) : (_B))
#define INI_DEBUG(fmt, arg...) do { \
    if (PLAT_LOG_DEBUG <= g_plat_loglevel_etc) \
    {\
        /*lint -e515*/ \
        printk(KERN_DEBUG "INI_DRV:D]%s:%d]"fmt"\n",__FUNCTION__,__LINE__, ##arg); \
        /*lint +e515*/ \
    }\
} while(0)

#define INI_INFO(fmt, arg...) do { \
    if (PLAT_LOG_INFO <= g_plat_loglevel_etc) \
    {\
        /*lint -e515*/ \
        printk(KERN_DEBUG "INI_DRV:D]%s:%d]"fmt"\n",__FUNCTION__,__LINE__,##arg); \
        /*lint +e515*/ \
    }\
} while(0)

#define INI_WARNING(fmt, arg...) do { \
    if (PLAT_LOG_WARNING <= g_plat_loglevel_etc) \
    {\
        /*lint -e515*/ \
        printk(KERN_WARNING "INI_DRV:W]%s:%d]"fmt"\n",__FUNCTION__,__LINE__,##arg); \
        /*lint +e515*/ \
    }\
} while(0)

#define INI_ERROR(fmt, arg...) do { \
    if (PLAT_LOG_ERR <= g_plat_loglevel_etc) \
    {\
        /*lint -e515*/ \
        printk(KERN_ERR "INI_DRV:E]%s:%d]"fmt"\n\n\n",__FUNCTION__,__LINE__,##arg); \
        /*lint +e515*/ \
    }\
} while(0)

#define STR_COUNTRY_CODE    ("country_code")

#define INI_KERNEL_READ_LEN             (512)
#define STR_CMP_BYTE(dest, str, local)  (dest[local] == str[local])

#define HISI_CUST_NVRAM_READ     (1)
#define HISI_CUST_NVRAM_WRITE    (0)
#define HISI_CUST_NVRAM_NUM      (340)
#define HISI_CUST_NVRAM_LEN      (104)
#define HISI_CUST_NVRAM_NAME     "WINVRAM"

#define INI_MODU_WIFI           (0x101)
#define INI_MODU_GNSS           (0x102)
#define INI_MODU_BT             (0x103)
#define INI_MODU_FM             (0x104)
#define INI_MODU_PLAT           (0x105)
#define INI_MODU_HOST_VERSION   (0x106)
#define INI_MODU_WIFI_MAC       (0x107)
#define INI_MODU_COEXIST        (0x108)
#define INI_MODU_DEV_WIFI       (0x109)
#define INI_MODU_DEV_GNSS       (0x10A)
#define INI_MODU_DEV_BT         (0x10B)
#define INI_MODU_DEV_FM         (0x10C)
#define INI_MODU_DEV_BFG_PLAT   (0x10D)

#define CUST_MODU_NVRAM         (0x200)

#define INI_MODU_POWER_FCC      (0xe1)
#define INI_MODU_POWER_ETSI     (0xe2)
#define INI_MODU_POWER_JP       (0xe3)
#define INI_MODU_INVALID        (0xff)

#define INI_STR_WIFI_NORMAL     ("[HOST_WIFI_NORMAL]")
#define INI_STR_GNSS_NORMAL     ("[HOST_GNSS_NORMAL]")
#define INI_STR_BT_NORMAL       ("[HOST_BT_NORMAL]")
#define INI_STR_FM_NORMAL       ("[HOST_FM_NORMAL]")
#define INI_STR_PLAT            ("[HOST_PLAT]")
#define INI_STR_WIFI_MAC        ("[HOST_WIFI_MAC]")
#define INT_STR_HOST_VERSION    ("[HOST_VERSION]")
#define INI_STR_COEXIST         ("[HOST_COEXIST]")
#define INI_STR_DEVICE_WIFI     ("[DEVICE_WIFI]")
#define INI_STR_DEVICE_GNSS     ("[DEVICE_GNSS]")
#define INI_STR_DEVICE_BT       ("[DEVICE_BT]")
#define INI_STR_DEVICE_FM       ("[DEVICE_FM]")
#define INI_STR_DEVICE_BFG_PLAT ("[DEVICE_BFG_PLAT]")
#define INI_STR_POWER_FCC               ("[HOST_WIFI_POWER_FCC]")
#define INI_STR_POWER_ETSI              ("[HOST_WIFI_POWER_ETSI]")
#define INI_STR_POWER_JP                ("[HOST_WIFI_POWER_JP]")
#define INI_STR_POWER_COMMON            ("[HOST_WIFI_POWER_COMMON]")

#define INI_INIT_MUTEX(mutex)   mutex_init(mutex)
#define INI_MUTEX_LOCK(mutex)   mutex_lock(mutex)
#define INI_MUTEX_UNLOCK(mutex) mutex_unlock(mutex)

#define INI_STR_MODU_LEN        (40)
#define MAX_READ_LINE_NUM       (192)
#define INI_FILE_PATH_LEN       (128)
#define INI_READ_VALUE_LEN      (64)
#define INI_VERSION_STR_LEN     (32)

#define INI_SUCC                (0)
#define INI_FAILED              (-1)

#define INI_FILE_TIMESPEC_UNRECONFIG (0)
#define INI_FILE_TIMESPEC_RECONFIG   (BIT0)
#define INI_NVRAM_RECONFIG           (BIT1)
#define INF_FILE_GET_CTIME(file_dentry) ((file_dentry)->d_inode->i_ctime.tv_sec)

/*
 * 4 Message Header Definition
 */


/*
 * 5 Message Definition
 */


/*
 * 6 STRUCT Type Definition
 */
typedef struct ini_board_vervion
{
	unsigned char board_version[INI_VERSION_STR_LEN];
}INI_BOARD_VERSION_STRU;

typedef struct ini_param_vervion
{
	unsigned char param_version[INI_VERSION_STR_LEN];
}INI_PARAM_VERSION_STRU;

typedef struct file INI_FILE;

/*
 * 7 Global Variable Declaring
 */
extern char g_ini_file_name_etc[INI_FILE_PATH_LEN];
extern INI_BOARD_VERSION_STRU g_board_version_etc;
extern INI_PARAM_VERSION_STRU g_param_version_etc;

/*
 * 8 UNION Type Definition
 */


/*
 * 9 OTHERS Definition
 */


/*
 * 10 Function Declare
 */
extern int32 get_cust_conf_int32_etc(int32 tag_index, int8 * puc_var, int32 *pul_value);
extern int32 get_cust_conf_string_etc(int32 tag_index, int8* puc_var, int8 *puc_value, uint32 size);
extern int32 find_download_channel_etc(uint8* buff, int8 * puc_var);
extern int32 read_conf_from_nvram_etc(uint8 * pc_out, uint32 size, uint32 nv_number,  const char* nv_name);
extern int32 ini_cfg_init_etc(void);
extern void  ini_cfg_exit_etc(void);
extern int32 ini_file_check_conf_update(void);
extern int8 *get_str_from_file(int8 *pc_file_path, const int8 *pc_mask_str, const int8 *pc_target_str);
#endif

