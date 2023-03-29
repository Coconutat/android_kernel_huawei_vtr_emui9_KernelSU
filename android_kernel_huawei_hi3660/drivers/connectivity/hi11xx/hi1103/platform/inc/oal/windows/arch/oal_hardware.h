

#ifndef __OAL_WINDOWS_HARDWARE_H__
#define __OAL_WINDOWS_HARDWARE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "oal_util.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
typedef oal_uint8 (*oal_irq_handler_t)(oal_int32, oal_void *);

/*BEGIN:Added by zhouqingsong/2012/2/15 for SD5115V100*/
#define OAL_HI_TIMER_REG_BASE                 (0x10105000)

#define OAL_HI_TIMER_NUM                    2
#define OAL_HI_TIMER_ENABLE                 1
#define OAL_HI_TIMER_DISABLE                0
#define OAL_HI_TIMER_INT_DISABLE            1
#define OAL_HI_TIMER_INT_CLEAR              0
#define OAL_HI_TIMER_DEFAULT_PERIOD         1

#define OAL_HI_TIMER_IRQ_NO                 80            /*5113 : 5   5115:80*/

#define OAL_HI_TIMER_FREE_MODE              0         /* 1101测试新增 */
#define OAL_HI_TIMER_CYCLE_MODE             1
#define OAL_HI_TIMER_SIZE_32_BIT            1
#define OAL_HI_TIMER_WRAPPING               0
#define OAL_HI_TIMER_INT_OCCUR              1
#define OAL_HI_TIMER_INT_VALID              0x01
#define OAL_HI_TIMER_NO_DIV_FREQ            0x0

#define OAL_HI_SC_REG_BASE                  (0x10100000)
#define OAL_HI_SC_CTRL                      (OAL_HI_SC_REG_BASE + 0x0000)

#define OAL_IRQ_ENABLE     1  /* 可以中断 */
#define OAL_IRQ_FORBIDDEN  0  /* 禁止中断 */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#define WL_WAKE_HOST   0
#define WL_SLEEP_GPIO  0
#define WL_PCIE_RESET  0

#define OAL_IRQF_TRIGGER_NONE               0
#define OAL_IRQF_NO_SUSPEND                 0
#define OAL_IRQF_SHARED                     0
#define OAL_IRQF_TRIGGER_RISING             0
#define OAL_IRQF_TRIGGER_FALLING            0

#define OAL_IRQF_TRIGGER_HIGH               0
#define OAL_IRQF_TRIGGER_LOW                0

#define OAL_IRQ_HANDLED                     0
#endif

#define WL_SAW_SEL0    0xFFFF
#define WL_SAW_SEL1    0xFFFF

typedef oal_uint32                          oal_irqreturn_t;

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    OAL_5115TIMER_ONE,
    OAL_5115TIMER_SEC,

    OAL_5115TIMER_BUTT
}oal_5115timer_enum;
typedef oal_uint8 oal_5115timer_enum_uint8;


typedef enum
{
    OAL_5115IRQ_ADRDSH = 0,
    OAL_5115IRQ_ADUDSH = 1,
    OAL_5115IRQ_ADRT = 2,
    OAL_5115IRQ_DCSWR = 3,
    OAL_5115IRQ_DTSPl0 = 4,
    OAL_5115IRQ_DTSPl1 = 5,
    OAL_5115IRQ_DDERTF = 6,
    OAL_5115IRQ_DMSC = 7,
    OAL_5115IRQ_DRMP = 8,
    OAL_5115IRQ_DTOWT = 9,
    OAL_5115IRQ_FEPE = 10,
    OAL_5115IRQ_FES = 11,
    OAL_5115IRQ_FEA = 12,
    OAL_5115IRQ_FEF = 13,
    OAL_5115IRQ_HIIA = 14,
    OAL_5115IRQ_HRI = 15,
    OAL_5115IRQ_MFSC = 16,
    OAL_5115IRQ_OIS = 17,
    OAL_5115IRQ_OMSNA = 18,
    OAL_5115IRQ_OMSNF = 19,
    OAL_5115IRQ_OSLIS = 20,
    OAL_5115IRQ_WH5RW = 21,
    OAL_5115IRQ_WH5RR = 22,
    OAL_5115IRQ_MPP = 23,
    OAL_5115IRQ_OPCCEN  = 24,
    OAL_5115IRQ_HIRCI = 25,
    OAL_5115IRQ_PROFILING = 26,

    OAL_5115IRQ_BUTT
}oal_5115irq_enum;
typedef oal_uint8 oal_5115irq_enum_uint8;

typedef oal_int32         (*oal_dbac_isr_func)(int);

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef DWORD_PTR           oal_cpumask;
typedef HANDLE              oal_irq_num;
typedef oal_uint32          (*oal_irq_intr_func)(oal_void *);
#define OAL_SA_SHIRQ       0x00000080  /* 中断类型 */

struct resource {
	oal_ulong start;
	oal_ulong end;
	const char *name;
	unsigned long flags;
	struct resource *parent, *sibling, *child;
};
typedef struct resource oal_resource;

typedef struct
{
	volatile oal_uint32 ul_timerx_load;
	volatile oal_uint32 ul_timerx_value;
	volatile oal_uint32 ul_timerx_control;
	volatile oal_uint32 ul_timerx_intclr;
	volatile oal_uint32 ul_timerx_ris;
	volatile oal_uint32 ul_timerx_mis;
	volatile oal_uint32 ul_timerx_bgload;
	volatile oal_uint32 ul_reserve;
} oal_hi_timerx_reg_stru;
/*timer控制寄存器*/
typedef union
{
	volatile oal_uint32 ul_value;
	struct
	{
		volatile oal_uint32 ul_oneshot: 1;                 /*选择计数模式 0：回卷计数 1：一次性计数*/
		volatile oal_uint32 ul_timersize: 1;               /*16bit|32bit计数操作模式 0：16bit 1：32bit*/
		volatile oal_uint32 ul_timerpre: 2;                /*预分频因子 00：不分频 01：4级分频 10：8级分频 11：未定义，设置相当于分频因子10*/
		volatile oal_uint32 ul_reserved0: 1;               /*保留位*/
		volatile oal_uint32 ul_intenable: 1;               /*中断屏蔽位 0：屏蔽 1：不屏蔽*/
		volatile oal_uint32 ul_timermode: 1;               /*计数模式 0：自由模式 1：周期模式*/
		volatile oal_uint32 ul_timeren: 1;                 /*定时器使能位 0：禁止 1：使能*/
		volatile oal_uint32 ul_reserved1: 24;              /*保留位*/
	} bits_stru;
} oal_hi_timer_control_union;

/*timer2_3寄存器*/
typedef struct
{
	oal_hi_timerx_reg_stru ast_timer[2];
} oal_hi_timer_reg_stru;

typedef struct
{
    oal_hi_timer_control_union  u_timerx_config;
}oal_hi_timerx_config_stru;

/* PCI驱动相关Win32封装 */
typedef struct
{
    oal_uint32  vendor;         /* Vendor and device ID or PCI_ANY_ID */
    oal_uint32  device;
    oal_uint32  subvendor;      /* Subsystem ID's or PCI_ANY_ID */
    oal_uint32  subdevice;
    oal_uint32  class_;
    oal_uint32  class_mask;
    oal_uint32  driver_data;    /* data private to the driver */
}oal_pci_device_id_stru;

typedef struct
{
    oal_uint16  vendor;
    oal_uint16	device;
    oal_uint32  irq;
}oal_pci_dev_stru;

typedef struct
{
    oal_int8                         *pc_name;
    OAL_CONST oal_pci_device_id_stru *id_table;
    oal_int32 (*probe)(oal_pci_dev_stru *dev, OAL_CONST oal_pci_device_id_stru *id);
    void      (*remove)(oal_pci_dev_stru *dev);
}oal_pci_driver_stru;

/* 中断设备结构体 */
typedef struct
{
    oal_uint32              ul_irq;                  /* 中断号 */
    oal_int32               l_irq_type;             /* 中断类型标志 */
    oal_void               *p_drv_arg;              /* 中断处理函数参数 */
    oal_int8               *pc_name;                /* 中断设备名字 只为界面友好 */
    oal_irq_intr_func       p_irq_intr_func;        /* 中断处理函数地址 */
}oal_irq_dev_stru;

typedef oal_uint8   oal_hi_timerx_index_enum_uint8;

typedef enum
{
    HI5115_TIMER_INDEX_0 = 0,
    HI5115_TIMER_INDEX_1,
    HI5115_TIMER_INDEX_2,
    HI5115_TIMER_INDEX_3,
    HI5115_TIMER_INDEX_BUTT
}oal_hi_timerx_index_enum;

#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
typedef struct
{
    oal_void (*resume)(oal_void);
}oal_pm_syscore_ops;

#endif
/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

OAL_INLINE oal_void  oal_irq_enable(oal_void)
{
}


OAL_INLINE oal_void  oal_irq_disable(oal_void)
{
}


OAL_INLINE oal_void  oal_irq_free(oal_irq_dev_stru *st_osdev)
{
}


OAL_INLINE oal_int32  oal_irq_setup(oal_irq_dev_stru *st_osdev)
{
    return 0;
}


OAL_INLINE oal_void  oal_irq_trigger(oal_uint8 uc_cpuid)
{

}


OAL_INLINE oal_void  oal_irq_save(oal_uint *pui_flags, oal_uint32 ul_type)
{
    if (OAL_IRQ_FORBIDDEN == *pui_flags)
    {
        return;
    }

    *pui_flags =  OAL_IRQ_FORBIDDEN;
}


OAL_INLINE oal_void  oal_irq_restore(oal_uint *pui_flags, oal_uint32 ul_type)
{
    if (OAL_IRQ_ENABLE == *pui_flags)
    {
        return;
    }

    *pui_flags = OAL_IRQ_ENABLE;
}


OAL_INLINE oal_int32  oal_irq_set_affinity(oal_irq_num irq, oal_uint32 ul_cpu)
{
    BOOL        ret;
    oal_cpumask mask;

    mask = 1<<ul_cpu;

    ret = SetProcessAffinityMask(irq, mask);

    if (0 == ret)
    {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_is_valid(oal_int32 i_number)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_request(oal_uint32 ul_gpio, OAL_CONST oal_int8 *pc_label)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_void  oal_gpio_free(oal_uint32 ul_gpio)
{
    return;
}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_direction_input(oal_uint32 ul_gpio)
{
    return 0;

}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_direction_output(oal_uint32 ul_gpio, oal_int l_level)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_to_irq(oal_uint32 ul_gpio)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_int32  oal_request_irq(oal_uint32             ul_irq,
                                                     oal_irq_handler_t      p_handler,
                                                     oal_uint32             ul_flags,
                                                     OAL_CONST oal_int8    *p_name,
                                                     oal_void              *p_dev)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_int32  oal_gpio_get_value(oal_uint32 ul_gpio)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_void  oal_wifi_reg_on_pull_up(oal_int32 wifi_gpio_addr)
{
    return;
}


OAL_STATIC OAL_INLINE oal_void  oal_wifi_reg_on_pull_down(oal_int32 wifi_gpio_addr)
{
    return;
}


OAL_INLINE oal_void  oal_5115timer_init(oal_void)
{

}


OAL_INLINE oal_uint32  oal_5115timer_get_10ns(oal_void)
{
    return 1;
}


OAL_INLINE oal_void  oal_5115timer_exit(oal_void)
{
}

/* 创建一个新的被使用资源区 */
OAL_STATIC OAL_INLINE oal_uint32 oal_request_mem_region(oal_uint32 start, oal_uint32 n, oal_int8 *name)
{
    return start;
}

OAL_STATIC OAL_INLINE oal_uint32 oal_release_mem_region(oal_uint32 start, oal_uint32 n)
{
    return start;
}

OAL_STATIC OAL_INLINE oal_uint32 oal_ioremap(oal_uint32 cookie, oal_uint32 size)
{
    return cookie;
}


OAL_STATIC OAL_INLINE oal_void *oal_ioremap_nocache(oal_uint32 cookie, oal_uint32 size)
{
    return (void *)cookie;
}

OAL_STATIC OAL_INLINE oal_uint32 oal_iounmap(oal_void *cookie)
{
    return 0;
}


OAL_STATIC OAL_INLINE oal_void  oal_writel(oal_uint32 ul_val, oal_uint32 ul_addr)
{
    /* do nothing */
}


OAL_STATIC OAL_INLINE oal_uint32  oal_readl(oal_uint32 ul_addr)
{
    return ul_addr;
}

OAL_STATIC  OAL_INLINE oal_hi_timerx_reg_stru  *hi_timerx_request(oal_hi_timerx_index_enum_uint8  en_timer_idx)
{
    return   (oal_hi_timerx_reg_stru *)0xDEAD;
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_load(oal_hi_timerx_reg_stru  *pst_timer, oal_uint32 ul_load)
{
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_start(oal_hi_timerx_reg_stru *pst_timer)
{
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_stop(oal_hi_timerx_reg_stru *pst_timer)
{
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_release(oal_hi_timerx_reg_stru  *pst_timer)
{
}

OAL_STATIC  OAL_INLINE  oal_uint32    hi_timerx_read(oal_hi_timerx_reg_stru *pst_timer)
{
    return 0x0;
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_enable_intr(oal_hi_timerx_reg_stru *pst_timer)
{
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_disable_intr(oal_hi_timerx_reg_stru *pst_timer)
{
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_clear_intr(oal_hi_timerx_reg_stru *pst_timer)
{
}
OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8  hi_timerx_intr_hit(oal_hi_timerx_reg_stru *pst_timer)
{
    return OAL_TRUE;
}
OAL_STATIC  OAL_INLINE oal_void    hi_timerx_control(oal_hi_timerx_reg_stru        *pst_timer,
                                                  oal_hi_timerx_config_stru     *pst_config)
{
}

#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
OAL_STATIC OAL_INLINE oal_int32 oal_mdrv_timer_start(oal_uint32 ul_id, oal_dbac_isr_func p_func, oal_int32 l_arg, oal_uint32 ul_timer_value, oal_uint32 ul_mode, oal_uint32 ul_type)
{
    return 0;
}

OAL_STATIC OAL_INLINE oal_int32 oal_mdrv_timer_stop(oal_uint32 ul_id)
{
    return 0;
}
#endif

OAL_STATIC OAL_INLINE oal_void oal_hi_kernel_change_hw_rps_enable(oal_uint32 ul_action)
{
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_hardware.h */
