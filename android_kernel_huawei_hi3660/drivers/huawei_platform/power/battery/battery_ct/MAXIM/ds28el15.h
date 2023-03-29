#ifndef DS28EL15_H
#define DS28EL15_H

#include <huawei_platform/power/batt_info.h>
#include "maxim_onewire.h"
#include <linux/random.h>
#include <linux/wakelock.h>

typedef struct {
    struct platform_device *pdev;
    maxim_onewire_ic_des ic_des;
    struct attribute_group *attr_group;
    struct attribute_group **attr_groups;
    struct wake_lock write_lock;
} ds28el15_des;

/* MAC types */
#define DS28EL15_CT_MAC_PAGE0                       MAC_RESOURCE_TPYE0
#define DS28EL15_CT_MAC_PAGE1                       MAC_RESOURCE_TPYE1
#define DS28EL15_CT_MAC_SIZE                        128
#define DS28EL15_MAC_RES                            0x00
#define DS28EL15_SN_VERSION_INDEX                   32

/* IC memory protection mode */
#define DS28EL15_INFO_BLOCK_NUM                     4
#define DS28EL15_READ_PROTECTION                    0x80
#define DS28EL15_WRITE_PROTECTION                   0x40
#define DS28EL15_EPROM_EMULATION_MODE               0x20
#define DS28EL15_AUTHENTICATION_PROTECTION          0x10
#define DS28EL15_AUTH_WRITE_PROTECTION              (DS28EL15_WRITE_PROTECTION | DS28EL15_AUTHENTICATION_PROTECTION)
#define DS28EL15_PROTECTION_MASK                    0xf0
#define DS28EL15_EMPTY_PROTECTION                   0
#define DS28EL15_INFO_BLOCK0                        MAXIM_BLOCK0
#define DS28EL15_INFO_BLOCK1                        MAXIM_BLOCK1
#define DS28EL15_BATTERY_VENDOR_BLOCK               MAXIM_BLOCK2
#define DS28EL15_PCB_VENDOR_BLOCK                   MAXIM_BLOCK3

/* ds28el15 return value */
#define DS28EL15_SUCCESS                            0
#define DS28EL15_FAIL                               1

/* set sram for get mac retry */
#define SET_SRAM_RETRY                              4
#define GET_USER_MEMORY_RETRY                       8
#define GET_PERSONALITY_RETRY                       8
#define GET_ROM_ID_RETRY                            8
#define GET_BLOCK_STATUS_RETRY                      8
#define SET_BLOCK_STATUS_RETRY                      8
#define GET_MAC_RETRY                               8

#define CURRENT_DS28EL15_TASK                       0
#define BYTES2BITS(x)                               ((x)<<3)

#define SN_LENGTH_BITS                              108
#define PRINTABLE_SN_SIZE                           17
#define SN_CHAR_PRINT_SIZE                          11
#define SN_HEX_PRINT_SIZE                           5

#define IS_ODD(a)                                   ((a) & 0x1)
#define IS_EVEN(a)                                  (!IS_ODD(a))
#define NOT_MUTI8(a)                                ((a) & 0x7)
#define IS_MUTI8(a)                                 (!NOT_MUTI8(a))

/* onewire communication error process*/
#define DS28EL15_COMMUNICATION_INFO(x,y)    \
    do{ \
        if(x){  \
            hwlog_info(y" failed(%x) in %s.", x,__func__);  \
        }   \
    }while(0)

/* dts read property error process*/
#define DS28EL15_DTS_READ_ERROR_RETURN(x,y) \
    do{ \
        if(x){  \
            hwlog_err("DTS do not have "y", needed in %s.",__func__);   \
            return DS28EL15_FAIL;   \
        }   \
    }while(0)

/* NULL pointer process*/
#define DS28EL15_NULL_POINT_RETURN(x)   \
    do{ \
        if(!x){ \
            hwlog_err("NULL point: "#x", found in %s.",__func__);   \
            return DS28EL15_FAIL;   \
        }   \
    }while(0)

enum {
    SET_SRAM_INDEX = 0,
    GET_USER_MEMORY_INDEX,
    GET_BLOCK_STATUS_INDEX,
    SET_BLOCK_STATUS_INDEX,
    GET_PERSONALITY_INDEX,
    GET_ROM_ID_INDEX,
    GET_MAC_INDEX,
    __MAX_COM_ERR_NUM,
};

typedef struct {
    unsigned char *id_mask;
    unsigned char *id_example;
    unsigned char *id_chk;
} battery_constraint;

#endif
