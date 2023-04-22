#ifndef ONEWIRE_COMMON_H
#define ONEWIRE_COMMON_H

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timex.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/batt_info.h>

/* onewire common interface */
typedef struct {
    unsigned int reset_init_low_ns;
    unsigned int reset_init_low_cycles;
    unsigned int reset_slave_response_delay_ns;
    unsigned int reset_slave_response_delay_cycles;
    unsigned int reset_hold_high_ns;
    unsigned int reset_hold_high_cycles;
    unsigned int write_init_low_ns;
    unsigned int write_init_low_cycles;
    unsigned int write_hold_ns;
    unsigned int write_hold_cycles;
    unsigned int write_residual_ns;
    unsigned int write_residual_cycles;
    unsigned int read_init_low_ns;
    unsigned int read_init_low_cycles;
    unsigned int read_residual_ns;
    unsigned int read_residual_cycles;
}onewire_time_request;

typedef struct {
    unsigned char (*reset)(void);
    unsigned char (*read_byte)(void);
    void (*write_byte)(const unsigned char);
    void (*set_time_request)(onewire_time_request *);
    void (*wait_for_ic)(unsigned int);
}onewire_phy_ops;

typedef struct{
    struct list_head node;
    int (*onewire_phy_register)(onewire_phy_ops *, unsigned int);
}ow_phy_reg_list;

extern struct list_head ow_phy_reg_head;

#define ONEWIRE_ILLEGAL_PARAM                   10
#define ONEWIRE_CRC16_FAIL                      9
#define ONEWIRE_CRC8_FAIL                       8
#define ONEWIRE_REGIST_FAIL                     7
#define ONEWIRE_OPS_REG_FAIL                    6
#define ONEWIRE_NO_SLAVE                        5
#define ONEWIRE_DTS_FAIL                        4
#define ONEWIRE_NULL_INPARA                     3
#define ONEWIRE_MEM_FAIL                        2
#define ONEWIRE_COM_FAIL                        1
#define ONEWIRE_SUCCESS                         0

#define FIND_IC_RETRY_NUM                       1

/* NULL point process*/
#define ONEWIRE_NULL_POINT(x)                                                       \
    do{                                                                             \
        if(!x){                                                                     \
            hwlog_err("NULL point: "#x", found in %s.",__func__);                   \
            return ONEWIRE_NULL_INPARA;                                             \
        }                                                                           \
    }while(0)

#define ONEWIRE_KALLOC_FAIL(x)                                                      \
    do{                                                                             \
        if(!x){                                                                     \
            hwlog_err("Kalloc for "#x" failed in %s.", __func__);                    \
            return ONEWIRE_MEM_FAIL;                                                \
        }                                                                           \
    }while(0)

#endif