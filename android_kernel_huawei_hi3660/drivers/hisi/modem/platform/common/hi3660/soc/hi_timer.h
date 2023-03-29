#ifndef __HI_TIMER_H__
#define __HI_TIMER_H__ 
#ifndef HI_SET_GET
#define HI_SET_GET(a0,a1,a2,a3,a4) 
#endif
#define HI_TIMER_LOADCOUNT_OFFSET (0x0)
#define HI_TIMER_LOADCOUNT_H_OFFSET (0xB0)
#define HI_TIMER_CURRENTVALUE_OFFSET (0x4)
#define HI_TIMER_CURRENTVALUE_H_OFFSET (0xB4)
#define HI_TIMER_CONTROLREG_OFFSET (0x8)
#define HI_TIMER_EOI_OFFSET (0xC)
#define HI_TIMER_INTSTATUS_OFFSET (0x10)
#define HI_TIMERS_INTSTATUS_OFFSET (0xA0)
#define HI_TIMERS_EOI_OFFSET (0xA4)
#define HI_TIMERS_RAWINTSTATUS_OFFSET (0xA8)
#ifndef __ASSEMBLY__
typedef union
{
    struct
    {
        unsigned int timern_loadcount_l : 32;
    } bits;
    unsigned int u32;
}HI_TIMER_LOADCOUNT_T;
typedef union
{
    struct
    {
        unsigned int timern_loadcount_h : 32;
    } bits;
    unsigned int u32;
}HI_TIMER_LOADCOUNT_H_T;
typedef union
{
    struct
    {
        unsigned int timern_currentvalue_l : 32;
    } bits;
    unsigned int u32;
}HI_TIMER_CURRENTVALUE_T;
typedef union
{
    struct
    {
        unsigned int timern_currentvalue_h : 32;
    } bits;
    unsigned int u32;
}HI_TIMER_CURRENTVALUE_H_T;
typedef union
{
    struct
    {
        unsigned int timern_en : 1;
        unsigned int timern_mode : 1;
        unsigned int timern_int_mask : 1;
        unsigned int reserved_1 : 1;
        unsigned int timern_en_ack : 1;
        unsigned int reserved_0 : 27;
    } bits;
    unsigned int u32;
}HI_TIMER_CONTROLREG_T;
typedef union
{
    struct
    {
        unsigned int timern_eoi : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_TIMER_EOI_T;
typedef union
{
    struct
    {
        unsigned int timern_int_status : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_TIMER_INTSTATUS_T;
typedef union
{
    struct
    {
        unsigned int timer1_int_status : 1;
        unsigned int timer2_int_status : 1;
        unsigned int timer3_int_status : 1;
        unsigned int timer4_int_status : 1;
        unsigned int timer5_int_status : 1;
        unsigned int timer6_int_status : 1;
        unsigned int timer7_int_status : 1;
        unsigned int timer8_int_status : 1;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_TIMERS_INTSTATUS_T;
typedef union
{
    struct
    {
        unsigned int reserved : 32;
    } bits;
    unsigned int u32;
}HI_TIMERS_EOI_T;
typedef union
{
    struct
    {
        unsigned int timer1_raw_int_status : 1;
        unsigned int timer2_raw_int_status : 1;
        unsigned int timer3_raw_int_status : 1;
        unsigned int timer4_raw_int_status : 1;
        unsigned int timer5_raw_int_status : 1;
        unsigned int timer6_raw_int_status : 1;
        unsigned int timer7_raw_int_status : 1;
        unsigned int timer8_raw_int_status : 1;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_TIMERS_RAWINTSTATUS_T;
#if 0
HI_SET_GET(hi_timer_loadcount_timern_loadcount_l,timern_loadcount_l,HI_TIMER_LOADCOUNT_T,HI_TIMER_BASE_ADDR, HI_TIMER_LOADCOUNT_OFFSET)
HI_SET_GET(hi_timer_loadcount_h_timern_loadcount_h,timern_loadcount_h,HI_TIMER_LOADCOUNT_H_T,HI_TIMER_BASE_ADDR, HI_TIMER_LOADCOUNT_H_OFFSET)
HI_SET_GET(hi_timer_currentvalue_timern_currentvalue_l,timern_currentvalue_l,HI_TIMER_CURRENTVALUE_T,HI_TIMER_BASE_ADDR, HI_TIMER_CURRENTVALUE_OFFSET)
HI_SET_GET(hi_timer_currentvalue_h_timern_currentvalue_h,timern_currentvalue_h,HI_TIMER_CURRENTVALUE_H_T,HI_TIMER_BASE_ADDR, HI_TIMER_CURRENTVALUE_H_OFFSET)
HI_SET_GET(hi_timer_controlreg_timern_en,timern_en,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_controlreg_timern_mode,timern_mode,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_controlreg_timern_int_mask,timern_int_mask,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_controlreg_reserved_1,reserved_1,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_controlreg_timern_en_ack,timern_en_ack,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_controlreg_reserved_0,reserved_0,HI_TIMER_CONTROLREG_T,HI_TIMER_BASE_ADDR, HI_TIMER_CONTROLREG_OFFSET)
HI_SET_GET(hi_timer_eoi_timern_eoi,timern_eoi,HI_TIMER_EOI_T,HI_TIMER_BASE_ADDR, HI_TIMER_EOI_OFFSET)
HI_SET_GET(hi_timer_eoi_reserved,reserved,HI_TIMER_EOI_T,HI_TIMER_BASE_ADDR, HI_TIMER_EOI_OFFSET)
HI_SET_GET(hi_timer_intstatus_timern_int_status,timern_int_status,HI_TIMER_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMER_INTSTATUS_OFFSET)
HI_SET_GET(hi_timer_intstatus_reserved,reserved,HI_TIMER_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMER_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer1_int_status,timer1_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer2_int_status,timer2_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer3_int_status,timer3_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer4_int_status,timer4_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer5_int_status,timer5_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer6_int_status,timer6_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer7_int_status,timer7_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_timer8_int_status,timer8_int_status,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_intstatus_reserved,reserved,HI_TIMERS_INTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_INTSTATUS_OFFSET)
HI_SET_GET(hi_timers_eoi_reserved,reserved,HI_TIMERS_EOI_T,HI_TIMER_BASE_ADDR, HI_TIMERS_EOI_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer1_raw_int_status,timer1_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer2_raw_int_status,timer2_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer3_raw_int_status,timer3_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer4_raw_int_status,timer4_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer5_raw_int_status,timer5_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer6_raw_int_status,timer6_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer7_raw_int_status,timer7_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_timer8_raw_int_status,timer8_raw_int_status,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
HI_SET_GET(hi_timers_rawintstatus_reserved,reserved,HI_TIMERS_RAWINTSTATUS_T,HI_TIMER_BASE_ADDR, HI_TIMERS_RAWINTSTATUS_OFFSET)
#endif
#endif
#endif
