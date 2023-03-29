#ifndef _ALLOCPAGES_DELAYACCT_H
#define _ALLOCPAGES_DELAYACCT_H

#include <linux/sched.h>

#ifdef CONFIG_HW_MEMORY_MONITOR
#include <log/log_usertype/log-usertype.h>

extern void __delayacct_allocpages_start(void);
extern void __delayacct_allocpages_end(unsigned int order);

#ifdef CONFIG_TASK_DELAY_ACCT
static inline void delayacct_allocpages_start(void)
{
       if (current->delays && (BETA_USER == get_logusertype_flag()))
               __delayacct_allocpages_start();
}

static inline void delayacct_allocpages_end(unsigned int order)
{
       if (current->delays && (BETA_USER == get_logusertype_flag()))
               __delayacct_allocpages_end(order);
}

#else
static inline void delayacct_allocpages_start(void)
{}
static inline void delayacct_allocpages_end(unsigned int order)
{}

#endif /* CONFIG_TASK_DELAY_ACCT */

#endif /* CONFIG_HW_MEMORY_MONITOR */

#endif
