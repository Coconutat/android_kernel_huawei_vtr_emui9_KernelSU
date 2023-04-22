

#ifndef _SMC_H_
#define _SMC_H_

#include <linux/of_device.h>
#include "teek_client_constants.h"
#include "teek_ns_client.h"

enum TC_NS_CMD_type {
	TC_NS_CMD_TYPE_INVALID = 0,
	TC_NS_CMD_TYPE_NS_TO_SECURE,
	TC_NS_CMD_TYPE_SECURE_TO_NS,
	TC_NS_CMD_TYPE_SECURE_TO_SECURE,
	TC_NS_CMD_TYPE_SECURE_CONFIG = 0xf,
	TC_NS_CMD_TYPE_MAX
};

#ifdef CONFIG_TEE_SMP
struct pending_entry {
	atomic_t users;
	pid_t pid;
	wait_queue_head_t wq;
	atomic_t run;
	struct list_head list;
};
#endif

extern struct device_node *np;

int smc_init_data(struct device *class_dev);
void smc_free_data(void);
unsigned int TC_NS_SMC(TC_NS_SMC_CMD *cmd, uint8_t flags);
unsigned int TC_NS_SMC_WITH_NO_NR(TC_NS_SMC_CMD *cmd, uint8_t flags);
int teeos_log_exception_archive(unsigned int eventid, const char* exceptioninfo);

#ifdef CONFIG_TEE_SMP
int init_smc_svc_thread(void);
int smc_wakeup_ca(pid_t ca);
int smc_wakeup_broadcast(void);
int smc_shadow_exit(pid_t ca);
int smc_queue_shadow_worker(uint64_t target);
void fiq_shadow_work_func(uint64_t target);
struct pending_entry *find_pending_entry(pid_t pid);
void foreach_pending_entry(void (*func)(struct pending_entry *));
void put_pending_entry(struct pending_entry *pe);
void show_cmd_bitmap(void);
static inline int switch_low_temperature_mode(unsigned int mode)
{
	return -EINVAL;
}
#else
/* Post command to TrustedCore without waiting for answer or result */
unsigned int TC_NS_POST_SMC(TC_NS_SMC_CMD *cmd);
void tc_smc_wakeup(void);
static inline int init_smc_svc_thread(void)
{
	return 0;
}
static inline void smc_wakeup_ca(void) {}
static inline void show_cmd_bitmap(void) {}
#endif
#endif
