/****************************************************
 *   Copyright(c)1988-2017, Huawei Tech. Co., Ltd.
 *               All Rights Reserved.
 *   Author      :   gaolin ID:00196511  
 *   Version     :   0.0.1
 *   Create time :   2017/07/26 11:18:23
 *   Description :   use -finstrument-functions
 *   Others      :
 ***************************************************/
#ifdef _PRE_DEBUG_PROFILING

/*  usage:
 *  1. set DBG_DO_PROFILING=y  in env.config
 *  2. write a check function(my_check for example) return 1 if ERROR happened, else return 0
     !!!!WARNING: a. my_check cannot call any functions profiled(inline functions included),
                     or cpu will be stucked
                  b. my_check must be registered AFTER objects being checked have been inited
     int __attribute__((__no_instrument_function__)) my_check(long this_func, long call_func, long direction)
     {
         if (....)
         {
            return 1;   // HAPPEN
         }
         return 0;  // NOT HAPPEN
     }

    3. register check function at proper code point
         __cyg_profile_func_register(my_check);

 *  4. compile and run! do not upload code to repo.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <asm/uaccess.h>

// use /proc/check to switch check
//#define  USE_PROC

#define  CFG_REPORT_CNT 3

static int cnt = 0;
static int pro_enable = 1; // enable check as defualt, or you need to use proc

typedef int (*cyg_check_hook_t)(long this_func, long call_func, long direction);
static cyg_check_hook_t pst_cyg_check_hook = NULL;

#ifdef USE_PROC
static int inited = 0;
static struct proc_dir_entry *proc_entry;
ssize_t write_info( struct file *filp, const char __user *buff,unsigned long len, void *data );
int __attribute__((__no_instrument_function__)) init_proc( void )
{
    proc_entry = create_proc_entry( "pcheck", 0644, NULL );

    if (proc_entry == NULL)
    {
        printk(KERN_INFO "pcheck could not be created\n");
    }
    else
    {
        proc_entry->read_proc = NULL;
        proc_entry->write_proc = write_info;
    }

    return 0;
}

ssize_t __attribute__((__no_instrument_function__))write_info( struct file *filp, const char __user *buff, unsigned long len, void *data )
{
    pro_enable = !pro_enable;
    printk("check enable=%d\n", pro_enable);
    return len;
}
#endif

void __attribute__((__no_instrument_function__))check(long this_func, long call_func, long direction)
{
    if (pro_enable && pst_cyg_check_hook)
    {
        if (pst_cyg_check_hook(this_func, call_func, direction))
        {
            if(cnt < CFG_REPORT_CNT)
            {
                printk("raised!!!this_func=%pf call_func=%pf dir=[%s]\n", 
                        (void *)this_func, (void *)call_func, direction ? "OUT": "IN");
                dump_stack();
                cnt++;
            }
        }
    }
}

void __attribute__((__no_instrument_function__)) __cyg_profile_func_enter(void *this_func, void *call_site)
{
#ifdef USE_PROC
    if (!inited) {
        inited = 1;
        init_proc();
    }
#endif
    check((long)this_func, (long)call_site, 0);
}
void __attribute__((__no_instrument_function__)) __cyg_profile_func_exit(void *this_func, void *call_site)
{
    check((long)this_func, (long)call_site, 1);
}
void __attribute__((__no_instrument_function__)) __cyg_profile_func_register(cyg_check_hook_t hook)
{
    pst_cyg_check_hook = hook;
}
EXPORT_SYMBOL(__cyg_profile_func_enter);
EXPORT_SYMBOL(__cyg_profile_func_exit);
EXPORT_SYMBOL(__cyg_profile_func_register);
#endif
