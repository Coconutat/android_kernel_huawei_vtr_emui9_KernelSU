/*
* File:   fusb30x_global.h
* Author: Tim Bremm <tim.bremm@fairchildsemi.com>
* Company: Fairchild Semiconductor
*
* Created on September 11, 2015, 15:28 AM
*/

#ifndef FUSB30X_TYPES_H
#define FUSB30X_TYPES_H

#include <linux/i2c.h>                              // i2c_client, spinlock_t
#include <linux/hrtimer.h>                          // hrtimer
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include "FSCTypes.h"                               // FUSB30x custom types
#include "../core/platform.h"

#ifdef FSC_DEBUG
#define FSC_HOSTCOMM_BUFFER_SIZE    64              // Length of the hostcomm buffer
#endif // FSC_DEBUG

struct fusb30x_chip                                 // Contains data required by this driver
{
    struct mutex lock;                              // Synchronization lock
    struct mutex thread_lock;                       // Prevent thread re-etrance
    struct wake_lock fusb302_wakelock;               // Wake lock

    struct notifier_block fsc_nb;					// Notifier block
    /* Vendor Info */
    FSC_U8 vconn_swap_to_on_supported;
    FSC_U8 vconn_swap_to_off_supported;
    FSC_U8 dp_enabled;
    FSC_U8 product_type_ama;
    FSC_U8 modal_operation_supported;
    FSC_U8 discover_mode_supported;
    FSC_U8 enter_mode_supported;
    FSC_U8 discover_svid_supported;
    FSC_U8 sink_pdo_number;

#ifdef FSC_DEBUG
    FSC_U8 dbgTimerTicks;                           // Count of timer ticks
    FSC_U8 dbgTimerRollovers;                       // Timer tick counter rollover counter
    FSC_U8 dbgSMTicks;                              // Count of state machine ticks
    FSC_U8 dbgSMRollovers;                          // State machine tick counter rollover counter
    FSC_S32 dbg_gpio_StateMachine;                  // Gpio that toggles every time the state machine is triggered
    FSC_BOOL dbg_gpio_StateMachine_value;           // Value of sm toggle state machine
    char HostCommBuf[FSC_HOSTCOMM_BUFFER_SIZE];     // Buffer used to communicate with HostComm
#endif // FSC_DEBUG

    /* Internal config data */
    FSC_S32 InitDelayMS;                            // Number of milliseconds to wait before initializing the fusb30x
    FSC_S32 numRetriesI2C;                          // Number of times to retry I2C reads/writes

    /* I2C */
    struct i2c_client* client;                      // I2C client provided by kernel
    FSC_BOOL use_i2c_blocks;                        // True if I2C_FUNC_SMBUS_I2C_BLOCK is supported

    /* GPIO */
    FSC_S32 gpio_VBus5V;                            // VBus 5V GPIO pin
    FSC_BOOL gpio_VBus5V_value;                     // true if active, false otherwise
    FSC_S32 gpio_VBusOther;                         // VBus other GPIO pin (eg. VBus 12V) (NOTE: Optional feature - if set to <0 during GPIO init, then feature is disabled)
    FSC_BOOL gpio_VBusOther_value;                  // true if active, false otherwise
    FSC_S32 gpio_IntN;                              // INT_N GPIO pin

#ifdef FSC_INTERRUPT_TRIGGERED
    FSC_S32 gpio_IntN_irq;                          // IRQ assigned to INT_N GPIO pin
#endif  // FSC_INTERRUPT_TRIGGERED

    /* Threads */
    struct delayed_work init_worker;                // Kicks off our runtime worker
    struct work_struct worker;                      // Main state machine actions

    /* Timers */
    struct hrtimer timer_state_machine;             // High-resolution timer for the state machine
    struct hrtimer timer_ccdebounce;
    struct hrtimer timer_pddebounce;
    struct hrtimer timer_statetimer;
    struct hrtimer timer_policystatetimer;
    struct hrtimer timer_noresponsetimer;
    struct hrtimer timer_loopresettimer;

    struct hrtimer timer_wake_unlock;
    struct hrtimer timer_force_timeout;
    struct hrtimer timer_vbus_timeout;
    struct task_struct *hard_reset_thread;
    struct kthread_worker hardreset_worker;
    struct kthread_work hardreset_work;
    struct task_struct *hardreset_worker_task;
    struct kthread_worker main_worker;
    struct kthread_work main_work;
    struct task_struct *main_worker_task;
    struct kthread_worker vbus_timeout_worker;
    struct kthread_work vbus_timeout_work;
    struct task_struct *vbus_timeout_worker_task;
    unsigned long flags;
    CC_ORIENTATION orientation;
    SourceOrSink    sourceOrSink;
    struct dual_role_phy_desc *dual_desc;
    struct dual_role_phy_instance *dual_role;

    struct kthread_worker set_drp_worker;
    struct kthread_work set_drp_work;
    struct task_struct *set_drp_worker_task;
};

extern struct fusb30x_chip* g_chip;

struct fusb30x_chip* fusb30x_GetChip(void);         // Getter for the global chip structure
void fusb30x_SetChip(struct fusb30x_chip* newChip); // Setter for the global chip structure

#endif /* FUSB30X_TYPES_H */
