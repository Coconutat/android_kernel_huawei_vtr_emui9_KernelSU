#ifndef BATT_INFO_H
#define BATT_INFO_H

#include <asm/current.h>
#include <linux/compiler-gcc.h>
#include <linux/stddef.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/reboot.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/of.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <uapi/asm-generic/ioctl.h>
#include <linux/power/hisi/hisi_battery_data.h>
#include <linux/power/hisi/coul/hisi_coul_event.h>
#include <linux/notifier.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <huawei_platform/power/power_mesg.h>

enum IC_CR{
    IC_PASS = 0,
    IC_FAIL_UNMATCH,    //IC information is not right for product
    IC_FAIL_UNKOWN,     //IC unkown(communication error)
    IC_FAIL_MEM_STATUS, //IC memory check failed
};

enum KEY_CR{
    KEY_PASS = 0,
    KEY_UNREADY,        //key is under checking
    KEY_FAIL_UNMATCH,   //key is unmatch
    KEY_FAIL_TIMEOUT,   //checking key timeout
};

#define SN_CR_ALLOW_MAX_NUM                             SN_FAIL_NV_TIMEOUT

typedef struct {
    unsigned char *datum;
    unsigned int len;
    unsigned int ic_type;
} mac_resource;

typedef enum {
    LOCAL_IC_TYPE = 0,
    MAXIM_SHA256_TYPE,
} batt_ic_type;

enum BATT_MATCH_TYPE{
    BATTERY_REMATCHABLE = 0,
    BATTERY_UNREMATCHABLE,
};

typedef enum {
    BATT_CHARGE_CYCLES = 0,
    BATT_MATCH_ABILITY,
} batt_safe_info_type;

typedef struct {
    batt_ic_type (*get_ic_type)(void);
    int (*get_ic_id)(const unsigned char **ic, unsigned int *ic_len);
    int (*get_batt_sn)(const unsigned char **sn, unsigned int *sn_len_bits, unsigned char *sn_version);
    int (*check_ic_status)(enum IC_CR *result);
    int (*certification)(void *data, int data_len, enum KEY_CR *result, int type);
    int (*get_ct_src)(mac_resource *mac_res, unsigned int type);
    int (*set_batt_safe_info)(batt_safe_info_type type, void *value);
    int (*get_batt_safe_info)(batt_safe_info_type type, void *value);
}batt_ct_ops;

typedef struct {
    struct list_head node;
    int (*ct_ops_register)(batt_ct_ops *);
    void (*ic_memory_release)(void);
} ct_ops_reg_list;

extern struct list_head batt_ct_head;

/* return macro */
#define BATTERY_DRIVER_FAIL                         1
#define BATTERY_DRIVER_SUCCESS                      0

/* IC memory protection mode */
#define READ_PROTECTION                             0x80
#define WRITE_PROTECTION                            0x40
#define EPROM_EMULATION_MODE                        0x20
#define AUTHENTICATION_PROTECTION                   0x10
#define NO_PROTECTION                               0x00

/* sys node information show macro */
#define BATT_ID_PRINT_SIZE_PER_CHAR                 3

/* NVME macro */
#define MAX_SN_LEN_BITS                             32
#define NV_VERSION_INDEX                            0

/* Battery maxium current&voltage initialization value */
#define MAX_CHARGE_CURRENT                          10000
#define MAX_CHARGE_VOLTAGE                          10000

#define HEXBITS                                     4
#define BYTEBITS                                    8
#define BYTE_HIGH_N_BITS(n)                         ((0x00ff)>>(8-n)<<(8-n))

#define MAC_RESOURCE_TPYE0                          0
#define MAC_RESOURCE_TPYE1                          1

#define BATTERY_CELL_FACTORY                        4
#define BATTERY_PACK_FACTORY                        5

#define ERR_NO_STRING_SIZE                          128
#define DSM_BATTERY_DETECT_BUFFSIZE                 1024

/* NULL pointer process*/
#define BATTERY_DRIVE_NULL_POINT_RETURN(x)                                              \
    do{                                                                                 \
        if(!x){                                                                         \
            hwlog_err("NULL point: "#x", found in %s.",__func__);       \
            return BATTERY_DRIVER_FAIL;                                                 \
        }                                                                               \
    }while(0)

/* dts read property error process*/
#define BATTERY_DRIVE_DTS_READ_ERROR_RETURN(x,y)                                                 \
    do{                                                                                 \
        if(x){                                                                        \
            hwlog_err("DTS do not have "y", needed in %s.",__func__);   \
            return BATTERY_DRIVER_FAIL;                                                 \
        }                                                                               \
    }while(0)

enum BATT_INFO_TYPE{
    DMD_INVALID = 0,
    DMD_ROM_ID_ERROR,
    DMD_IC_STATE_ERROR,
    DMD_IC_KEY_ERROR,
    DMD_OO_UNMATCH,
    DMD_OBD_UNMATCH,
    DMD_OBT_UNMATCH,
    DMD_NV_ERROR,
    DMD_SERVICE_ERROR,
};

enum {
    IC_DMD_GROUP = 0,
    KEY_DMD_GROUP,
    SN_DMD_GROUP,
    BATT_INFO_DMD_GROUPS,
};

typedef struct {
    enum IC_CR ic_status;
    enum KEY_CR key_status;
    enum SN_CR sn_status;
    enum CHECK_MODE check_mode;
} battery_check_result;

int get_battery_type(unsigned char *name);

#endif
