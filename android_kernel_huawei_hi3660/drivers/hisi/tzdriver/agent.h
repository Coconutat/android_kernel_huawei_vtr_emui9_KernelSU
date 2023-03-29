

#ifndef _AGENT_H_
#define _AGENT_H__
#include <linux/fs.h>
#include "teek_ns_client.h"

#define AGENT_FS_ID 0x46536673	/*FSfs*/
#define AGENT_MISC_ID 0x4d495343	/*MISC*/
#define TEE_RPMB_AGENT_ID 0x4abe6198	/*RPMB*/
#define AGENT_SOCKET_ID 0x69e85664	/*socket*/
#ifdef CONFIG_TEE_SMP
/* if a shadow task does a agent request */
#define SHADOW_AGENT_FLAG          (1<<31)

/* ssa and rpmb agent use */
#define SS_AGENT_MSG               11723
#define MT_AGENT_MSG               11724

#define AGENT_MSG_VAL(msg) (msg&(~SHADOW_AGENT_FLAG))
#endif
struct __smc_event_data *find_event_control(unsigned int agent_id);

/*for secure agent*/
struct __smc_event_data {
	unsigned int agent_id;
	int agent_alive;
	struct mutex work_lock;
	wait_queue_head_t wait_event_wq;
	int ret_flag;
	wait_queue_head_t send_response_wq;
	int send_flag;
	struct list_head head;
	TC_NS_SMC_CMD cmd;
	TC_NS_DEV_File *owner;
	TC_NS_Shared_MEM *buffer;
	atomic_t usage;
#ifdef CONFIG_TEE_SMP
	wait_queue_head_t ca_pending_wq;
	atomic_t ca_run;
#endif
};

struct tee_agent_kernel_ops {
	const char *agent_name;	/* MUST NOT be NULL*/
	unsigned int agent_id;	/* MUST NOT be zero*/
	int (*tee_agent_init)(struct tee_agent_kernel_ops *agent_instance);
	int (*tee_agent_run)(struct tee_agent_kernel_ops *agent_instance);
	/* MUST NOT be NULL*/
	int (*tee_agent_work)(struct tee_agent_kernel_ops *agent_instance);
	int (*tee_agent_stop)(struct tee_agent_kernel_ops *agent_instance);
	int (*tee_agent_exit)(struct tee_agent_kernel_ops *agent_instance);
	int (*tee_agent_crash_work)(struct tee_agent_kernel_ops *
					agent_instance,
					TC_NS_ClientContext *context,
					unsigned int dev_file_id);
	struct task_struct *agent_thread;
	void *agent_data;
	TC_NS_Shared_MEM *agent_buffer;
	struct list_head list;
};

static inline void get_agent_event(struct __smc_event_data *event_data)
{
	if (event_data)
		atomic_inc(&event_data->usage);
}

static inline void put_agent_event(struct __smc_event_data *event_data)
{
	if (event_data) {
		if (atomic_dec_and_test(&event_data->usage))
			kfree(event_data);
	}
}

int agent_init(void);

int agent_exit(void);

int agent_process_work(TC_NS_SMC_CMD *smc_cmd, unsigned int agent_id);

int is_agent_alive(unsigned int agent_id);

int TC_NS_set_nativeCA_hash(unsigned long arg);

int TC_NS_register_agent(TC_NS_DEV_File *dev_file,
								 unsigned int agent_id,
								 TC_NS_Shared_MEM *shared_mem);
int TC_NS_unregister_agent(unsigned int agent_id);
int TC_NS_unregister_agent_client(TC_NS_DEV_File *dev_file);

unsigned int TC_NS_incomplete_proceed(TC_NS_SMC_CMD *smc_cmd,
								unsigned int agent_id,
								uint8_t flags);

int TC_NS_wait_event(unsigned int agent_id);

int TC_NS_send_event_response(unsigned int agent_id);
void TC_NS_send_event_response_all(void);

int TC_NS_sync_sys_time(TC_NS_Time *tc_ns_time);

int tee_agent_clear_work(TC_NS_ClientContext *context, unsigned int dev_file_id);
int tee_agent_kernel_register(struct tee_agent_kernel_ops *new_agent);

bool TC_NS_is_system_agent_client(TC_NS_DEV_File *dev_file);

extern int mmc_blk_ioctl_rpmb_cmd(enum func_id id,
						struct block_device *bdev,
						struct mmc_blk_ioc_rpmb_data *idata);

extern struct mmc_card *get_mmc_card(struct block_device *bdev);

extern int check_ext_agent_access(struct task_struct *ca_task);
char *get_process_path(struct task_struct *task, char *tpath);
#ifdef CONFIG_HISI_MMC_SECURE_RPMB
extern int rpmb_agent_register(void);
#endif

#endif /*_AGENT_H_*/
