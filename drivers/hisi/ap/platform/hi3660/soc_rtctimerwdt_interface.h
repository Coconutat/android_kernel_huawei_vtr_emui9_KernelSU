#ifndef __SOC_RTCTIMERWDT_INTERFACE_H__
#define __SOC_RTCTIMERWDT_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_RTCTIMERWDT_RTC_DR_ADDR(base) ((base) + (0x0000))
#define SOC_RTCTIMERWDT_RTC_MR_ADDR(base) ((base) + (0x0004))
#define SOC_RTCTIMERWDT_RTC_LR_ADDR(base) ((base) + (0x0008))
#define SOC_RTCTIMERWDT_RTC_CR_ADDR(base) ((base) + (0x000C))
#define SOC_RTCTIMERWDT_RTC_IMSC_ADDR(base) ((base) + (0x0010))
#define SOC_RTCTIMERWDT_RTCRIS_ADDR(base) ((base) + (0x0014))
#define SOC_RTCTIMERWDT_RTCMIS_ADDR(base) ((base) + (0x0018))
#define SOC_RTCTIMERWDT_RTCICR_ADDR(base) ((base) + (0x001C))
#define SOC_RTCTIMERWDT_TIMER0_LOAD_ADDR(base) ((base) + (0x0000))
#define SOC_RTCTIMERWDT_TIMER1_LOAD_ADDR(base) ((base) + (0x0020))
#define SOC_RTCTIMERWDT_TIMER0_VALUE_ADDR(base) ((base) + (0x0004))
#define SOC_RTCTIMERWDT_TIMER1_VALUE_ADDR(base) ((base) + (0x0024))
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_ADDR(base) ((base) + (0x0008))
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_ADDR(base) ((base) + (0x0028))
#define SOC_RTCTIMERWDT_TIMER0_INTCLR_ADDR(base) ((base) + (0x000C))
#define SOC_RTCTIMERWDT_TIMER1_INTCLR_ADDR(base) ((base) + (0x002C))
#define SOC_RTCTIMERWDT_TIMER0_RIS_ADDR(base) ((base) + (0x0010))
#define SOC_RTCTIMERWDT_TIMER1_RIS_ADDR(base) ((base) + (0x0030))
#define SOC_RTCTIMERWDT_TIMER0_MIS_ADDR(base) ((base) + (0x0014))
#define SOC_RTCTIMERWDT_TIMER1_MIS_ADDR(base) ((base) + (0x0034))
#define SOC_RTCTIMERWDT_TIMER0_BGLOAD_ADDR(base) ((base) + (0x0018))
#define SOC_RTCTIMERWDT_TIMER1_BGLOAD_ADDR(base) ((base) + (0x0038))
#define SOC_RTCTIMERWDT_WDLOAD_ADDR(base) ((base) + (0x0000))
#define SOC_RTCTIMERWDT_WDVALUE_ADDR(base) ((base) + (0x0004))
#define SOC_RTCTIMERWDT_WDCONTROL_ADDR(base) ((base) + (0x0008))
#define SOC_RTCTIMERWDT_WDINTCLR_ADDR(base) ((base) + (0x000C))
#define SOC_RTCTIMERWDT_WDRIS_ADDR(base) ((base) + (0x0010))
#define SOC_RTCTIMERWDT_WDMIS_ADDR(base) ((base) + (0x0014))
#define SOC_RTCTIMERWDT_WDLOCK_ADDR(base) ((base) + (0x0C00))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcdr : 32;
    } reg;
} SOC_RTCTIMERWDT_RTC_DR_UNION;
#endif
#define SOC_RTCTIMERWDT_RTC_DR_rtcdr_START (0)
#define SOC_RTCTIMERWDT_RTC_DR_rtcdr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcmr : 32;
    } reg;
} SOC_RTCTIMERWDT_RTC_MR_UNION;
#endif
#define SOC_RTCTIMERWDT_RTC_MR_rtcmr_START (0)
#define SOC_RTCTIMERWDT_RTC_MR_rtcmr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtclr : 32;
    } reg;
} SOC_RTCTIMERWDT_RTC_LR_UNION;
#endif
#define SOC_RTCTIMERWDT_RTC_LR_rtclr_START (0)
#define SOC_RTCTIMERWDT_RTC_LR_rtclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtccr : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_RTC_CR_UNION;
#endif
#define SOC_RTCTIMERWDT_RTC_CR_rtccr_START (0)
#define SOC_RTCTIMERWDT_RTC_CR_rtccr_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcimsc : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_RTC_IMSC_UNION;
#endif
#define SOC_RTCTIMERWDT_RTC_IMSC_rtcimsc_START (0)
#define SOC_RTCTIMERWDT_RTC_IMSC_rtcimsc_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_RTCRIS_UNION;
#endif
#define SOC_RTCTIMERWDT_RTCRIS_rtcris_START (0)
#define SOC_RTCTIMERWDT_RTCRIS_rtcris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_RTCMIS_UNION;
#endif
#define SOC_RTCTIMERWDT_RTCMIS_rtcmis_START (0)
#define SOC_RTCTIMERWDT_RTCMIS_rtcmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int rtcicr : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_RTCICR_UNION;
#endif
#define SOC_RTCTIMERWDT_RTCICR_rtcicr_START (0)
#define SOC_RTCTIMERWDT_RTCICR_rtcicr_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxload : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_LOAD_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_LOAD_timerxload_START (0)
#define SOC_RTCTIMERWDT_TIMER0_LOAD_timerxload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxload : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_LOAD_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_LOAD_timerxload_START (0)
#define SOC_RTCTIMERWDT_TIMER1_LOAD_timerxload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxvalue : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_VALUE_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_VALUE_timerxvalue_START (0)
#define SOC_RTCTIMERWDT_TIMER0_VALUE_timerxvalue_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxvalue : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_VALUE_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_VALUE_timerxvalue_START (0)
#define SOC_RTCTIMERWDT_TIMER1_VALUE_timerxvalue_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int oneshot : 1;
        unsigned int reserved_0 : 1;
        unsigned int timerpre : 2;
        unsigned int reserved_1 : 1;
        unsigned int internable : 1;
        unsigned int timermode : 1;
        unsigned int timeren : 1;
        unsigned int reserved_2 : 24;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_CONTROL_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_oneshot_START (0)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_oneshot_END (0)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timerpre_START (2)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timerpre_END (3)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_internable_START (5)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_internable_END (5)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timermode_START (6)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timermode_END (6)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timeren_START (7)
#define SOC_RTCTIMERWDT_TIMER0_CONTROL_timeren_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int oneshot : 1;
        unsigned int timersize : 1;
        unsigned int timerpre : 2;
        unsigned int reserved_0 : 1;
        unsigned int internable : 1;
        unsigned int timermode : 1;
        unsigned int timeren : 1;
        unsigned int reserved_1 : 24;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_CONTROL_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_oneshot_START (0)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_oneshot_END (0)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timersize_START (1)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timersize_END (1)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timerpre_START (2)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timerpre_END (3)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_internable_START (5)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_internable_END (5)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timermode_START (6)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timermode_END (6)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timeren_START (7)
#define SOC_RTCTIMERWDT_TIMER1_CONTROL_timeren_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxintclr : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_INTCLR_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_INTCLR_timerxintclr_START (0)
#define SOC_RTCTIMERWDT_TIMER0_INTCLR_timerxintclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxintclr : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_INTCLR_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_INTCLR_timerxintclr_START (0)
#define SOC_RTCTIMERWDT_TIMER1_INTCLR_timerxintclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_RIS_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_RIS_timerxris_START (0)
#define SOC_RTCTIMERWDT_TIMER0_RIS_timerxris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_RIS_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_RIS_timerxris_START (0)
#define SOC_RTCTIMERWDT_TIMER1_RIS_timerxris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_MIS_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_MIS_timerxmis_START (0)
#define SOC_RTCTIMERWDT_TIMER0_MIS_timerxmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_MIS_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_MIS_timerxmis_START (0)
#define SOC_RTCTIMERWDT_TIMER1_MIS_timerxmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxbgload : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER0_BGLOAD_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER0_BGLOAD_timerxbgload_START (0)
#define SOC_RTCTIMERWDT_TIMER0_BGLOAD_timerxbgload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timerxbgload : 32;
    } reg;
} SOC_RTCTIMERWDT_TIMER1_BGLOAD_UNION;
#endif
#define SOC_RTCTIMERWDT_TIMER1_BGLOAD_timerxbgload_START (0)
#define SOC_RTCTIMERWDT_TIMER1_BGLOAD_timerxbgload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogload : 32;
    } reg;
} SOC_RTCTIMERWDT_WDLOAD_UNION;
#endif
#define SOC_RTCTIMERWDT_WDLOAD_wdogload_START (0)
#define SOC_RTCTIMERWDT_WDLOAD_wdogload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogvalue : 32;
    } reg;
} SOC_RTCTIMERWDT_WDVALUE_UNION;
#endif
#define SOC_RTCTIMERWDT_WDVALUE_wdogvalue_START (0)
#define SOC_RTCTIMERWDT_WDVALUE_wdogvalue_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int inten : 1;
        unsigned int resen : 1;
        unsigned int reserved : 30;
    } reg;
} SOC_RTCTIMERWDT_WDCONTROL_UNION;
#endif
#define SOC_RTCTIMERWDT_WDCONTROL_inten_START (0)
#define SOC_RTCTIMERWDT_WDCONTROL_inten_END (0)
#define SOC_RTCTIMERWDT_WDCONTROL_resen_START (1)
#define SOC_RTCTIMERWDT_WDCONTROL_resen_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogxintclr : 32;
    } reg;
} SOC_RTCTIMERWDT_WDINTCLR_UNION;
#endif
#define SOC_RTCTIMERWDT_WDINTCLR_wdogxintclr_START (0)
#define SOC_RTCTIMERWDT_WDINTCLR_wdogxintclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_WDRIS_UNION;
#endif
#define SOC_RTCTIMERWDT_WDRIS_wdogris_START (0)
#define SOC_RTCTIMERWDT_WDRIS_wdogris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_RTCTIMERWDT_WDMIS_UNION;
#endif
#define SOC_RTCTIMERWDT_WDMIS_wdogmis_START (0)
#define SOC_RTCTIMERWDT_WDMIS_wdogmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdoglock : 32;
    } reg;
} SOC_RTCTIMERWDT_WDLOCK_UNION;
#endif
#define SOC_RTCTIMERWDT_WDLOCK_wdoglock_START (0)
#define SOC_RTCTIMERWDT_WDLOCK_wdoglock_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
