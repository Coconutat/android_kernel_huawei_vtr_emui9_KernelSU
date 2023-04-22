/*
* File:   fusb3601_global.h
*/

#ifndef FUSB3601_GLOBAL_H_
#define FUSB3601_GLOBAL_H_

#include <linux/i2c.h>                      /* i2c_client, spinlock_t */
#include <linux/hrtimer.h>                  /* hrtimer */
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include "FSCTypes.h"                       /* FUSB3601 custom types */

#include "../core/port.h"

#ifdef FSC_DEBUG
#include <linux/debugfs.h>
#endif /* FSC_DEBUG */
#include <linux/kthread.h>

struct fusb3601_chip
{
    struct mutex lock;                      /* Synchronization lock */
    struct wake_lock fusb3601_wakelock;      /* Wake lock */
    struct semaphore suspend_lock;

#ifdef FSC_DEBUG
    FSC_U8 dbgTimerTicks;                   /* Count of timer ticks */
    FSC_U8 dbgTimerRollovers;               /* Timer tick counter rollovers */
    FSC_U8 dbgSMTicks;                      /* Count of state machine ticks */
    FSC_U8 dbgSMRollovers;                  /* State machine tick rollovers */
    FSC_S32 dbg_gpio_StateMachine;          /* SM Debug Toggle GPIO */
    FSC_BOOL dbg_gpio_StateMachine_value;   /* Value of SM Debug Toggle */
    char HostCommBuf[FSC_HOSTCOMM_BUFFER_SIZE]; /* Buffer used with HostComm */

    struct dentry *debugfs_parent;          /* Parent node for DebugFS usage */
#endif /* FSC_DEBUG */

    /* Internal config data */
    FSC_S32 InitDelayMS;                    /* Time to wait before init'ing */
    FSC_S32 numRetriesI2C;                  /* # of times to retry I2C rd/wr */

    /* I2C */
    struct i2c_client* client;              /* I2C client provided by kernel */
    FSC_BOOL use_i2c_blocks;                /* True if I2C_FUNC_SMBUS_I2C_BLOCK
                                             * is supported */
    /* GPIO */
    FSC_S32 gpio_VBus5V;                    /* VBus 5V GPIO pin */
    FSC_BOOL gpio_VBus5V_value;             /* True if ON, False otherwise */

    FSC_S32 gpio_IntN;                      /* INT_N GPIO pin */
    FSC_S32 gpio_IntN_irq;                  /* IRQ assigned to INT_N GPIO pin */

	FSC_S32 gpio_Vconn;                    /* Vconn GPIO pin */
    FSC_BOOL gpio_Vconn_value;             /* True if ON, False otherwise */

    /* dts */
    int use_super_switch_cutoff_wired_channel;

    /* Timers */
    struct hrtimer sm_timer;                /* High-res timer for the SM */
    struct hrtimer timer_force_timeout;

    /* Work struct for bottom half state machine processing */
	struct workqueue_struct *highpri_wq;
	FSC_BOOL queued;
    struct work_struct sm_worker;

    /* Port structure */
    struct Port port;
    struct work_struct fusb3601_probe_work;
    struct work_struct fusb3601_set_cc_mode_work;
    CCOrientation orientation;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
    struct dual_role_phy_desc *dual_desc;
    struct dual_role_phy_instance *dual_role;
#endif
    struct kthread_worker set_drp_worker;
    struct kthread_work set_drp_work;
    struct task_struct *set_drp_worker_task;
};

extern struct fusb3601_chip* get_chip;

/* Getter for the global chip structure */
struct fusb3601_chip* fusb3601_GetChip(void);

/* Setter for the global chip structure */
void fusb3601_SetChip(struct fusb3601_chip* newChip);

#endif /* FUSB3601_GLOBAL_H */
