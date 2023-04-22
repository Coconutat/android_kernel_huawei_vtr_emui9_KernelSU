/*
 * hisi softtime driver.
 *
 * Copyright (C) 2015 huawei Ltd.
 * Author:lijiangxiong <lijingxiong@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define TIMERLOAD(n)                        ((n) << 5)
#define TIMERVALUE(n)                       (((n) << 5) + 0x04)
#define TIMERCTRL(n)                        (((n) << 5) + 0x08)
#define TIMERINTCLR(n)                      (((n) << 5) + 0x0C)
#define TIMERRIS(n)                         (((n) << 5) + 0x10)
#define TIMERMIS(n)                         (((n) << 5) + 0x14)
#define TIMERBGLOAD(n)                      (((n) << 5) + 0x18)

#define TIMER_EN_ACK                        (1<<4)
#define TIMER_INT_MASK                      (1<<2)
#define TIMER_MODE_PERIOD                   (1<<1)
#define HARD_TIMER_ENABLE                   1
#define HARD_TIMER_DISABLE                  0
#define TIMER_ONCE_COUNT                    0
#define TIMER_PERIOD_COUNT                  1

typedef void (*softtimer_func)(unsigned long);

struct softtimer_list
{
   softtimer_func func;
   unsigned long para;
   unsigned int timeout;
   unsigned int timer_type;

   struct list_head entry;
   unsigned int count_num;
   unsigned int is_running;
   unsigned int init_flags;
};

extern void hisi_softtimer_add(struct softtimer_list * timer);
extern int hisi_softtimer_delete(struct softtimer_list * timer);
extern int hisi_softtimer_create(struct softtimer_list *timer,softtimer_func func,
       unsigned long para, unsigned int timeout);
extern int hisi_softtimer_modify(struct softtimer_list *timer,unsigned int new_expire_time);



