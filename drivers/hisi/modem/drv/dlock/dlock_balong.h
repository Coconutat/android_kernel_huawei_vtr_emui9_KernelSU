
#ifndef __DLOCK_BALONG_H__
#define __DLOCK_BALONG_H__

#ifdef __cplusplus
    extern "C" {
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "mdrv_errno.h"
#include "bsp_om.h"
#include "bsp_om_enum.h"
#include "osl_types.h"
#include "osl_malloc.h"
#include <linux/interrupt.h>

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define dlock_ok (0)
#define dlock_error (-1)
#define dlock_print(fmt, ...)   (bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_DLOCK, "[dlock]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define dlock_malloc(size)      osl_malloc(size)
#define dlock_free(ptr)         osl_free(ptr)
#define SYSCTRL_ADDR_SIZE      (0x1000)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
enum
{
    DLOCK_DISABLE = 0,
    DLOCK_ENABLE = 1
};

/*****************************************************************************
  3 STRUCT定义
*****************************************************************************/
struct mst_port_info
{
    u32 mst_port;                           /*mst最小端口号值*/
    char mst_name[32];                      /*与mst最小端口号对应的mst_name*/
};

struct mst_id_info
{
    u32 id;                                 /*mst_id值*/
    char id_mst_name[32];                   /*与mst_id对应的mst_name*/
};

struct bus_reset_info
{
    u32 pd_reset_reg[3];                    /*pd区复位寄存器偏移地址、起始位、终止位*/
    u32 mdm_reset_reg[3];                   /*mdm区复位寄存器偏移地址、起始位、终止位*/
    u32 pcie_reset_reg[3];                  /*pcie区复位寄存器偏移地址、起始位、终止位*/
    u32 cnt_div_num_reg[3];                 /*死锁计数分频系数寄存器偏移地址、起始位、终止位*/
    void* pd_vir_addr;
    void* mdm_vir_addr;
    void* pcie_vir_addr;
    char version[32];
};

struct bus_dlock_info
{
    u32 slv_port_reg[3];                    /*死锁slave端口号偏移地址、起始位、终止位*/
    u32 wr_reg[3];                          /*死锁读写指示寄存器偏移地址、起始位、终止位*/
    u32 addr_reg[3];                        /*死锁地址寄存器偏移地址、起始位、终止位*/
    u32 mst_port_reg[3];                    /*死锁master最小端口号偏移地址、起始位、终止位*/
    u32 mst_id_reg[3];                      /*死锁mst_id偏移地址、起始位、终止位*/
    u32 mst_port_num;                       /*需要记录mst_name的端口个数*/
    u32 mst_id_num;                         /*需要记录mst_name的mst_id个数*/
    u32 mst_port_id;                        /*需要查看mst_id的mst端口号*/
    struct mst_port_info *mst_port_info;    /*需要记录mst_name的mst端口信息*/
    struct mst_id_info *mst_id_info;        /*需要记录mst_name的mst_id信息*/
};

struct bus_state
{
    void* bus_vir_addr;                     /*总线虚拟基地址*/
    char bus_name[32];                      /*总线名称*/
    u32 bus_state_reg[3];                   /*总线状态寄存器偏移地址、起始位、终止位*/
    u32 clear_irq_reg[3];
};

struct bus_state_info
{
    struct bus_state bus_state;             /*总线状态寄存器信息*/
    struct bus_dlock_info bus_dlock_info;   /*总线死锁dlock寄存器信息*/
};

struct bus_info
{
    u32 bus_num;                            /*需要记录的死锁总线个数*/
    struct bus_state_info *bus_state_info;  /*总线死锁寄存器信息*/
};

#ifdef __cplusplus
}
#endif

#endif



