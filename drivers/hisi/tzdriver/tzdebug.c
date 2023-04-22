#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <stdarg.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/mm.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

#include "tc_ns_log.h"
#include "securec.h"
#include "teek_ns_client.h"
#include "smc.h"
#include "teek_client_api.h"
#include "teek_client_constants.h"
#include "mailbox_mempool.h"
#include "dynamic_mem.h"

static struct dentry *tz_dbg_dentry;

extern void tzdebug_archivelog(void);
extern void wakeup_tc_siq(void);
extern void tz_log_write(void);
extern int release_configid_mem(uint32_t configid);
extern int register_to_tee(struct dynamic_mem_item* mem_item);
typedef void (*tzdebug_opt_func)(char* param);
struct opt_ops {
	char* name;
	tzdebug_opt_func func;
};
static int send_dump_mem(void)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret = 0;
	struct mb_cmd_pack *mb_pack;
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		return -ENOMEM;
	}
	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_DUMP_MEMINFO;
	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_NONE;
	ret = TC_NS_SMC(&smc_cmd, 0);
	mailbox_free(mb_pack);
	if (ret) {
	    tloge("send_dump_mem failed.\n");
	}
	tz_log_write();
	return ret;
}


static void archivelog(char* param)
{
	(void)param;
	tzdebug_archivelog();
}
static void tzdump(char* param)
{
	(void)param;
	show_cmd_bitmap();
	wakeup_tc_siq();
}
static void tzmemdump(char* param)
{
	(void)param;
	(void)send_dump_mem();
}
static void tzlogwrite(char* param)
{
	(void)param;
	(void)tz_log_write();
}
static void tzhelp(char* param);

static struct opt_ops optArr[]={
	 {"help",tzhelp},
	 {"archivelog",archivelog},
	 {"dump",tzdump},
	 {"memdump",tzmemdump},
	 {"logwrite",tzlogwrite},
};
static void tzhelp(char* param)
{
	uint32_t i;
	(void)param;
	for( i=0;i< sizeof(optArr)/sizeof(struct opt_ops);i++) {
		tloge("cmd:%s\n",optArr[i].name);
	}
}

static ssize_t tz_dbg_opt_write(struct file *filp,
                               const char __user *ubuf, size_t cnt,
                               loff_t *ppos)
{
	char buf[128] = {0};
	char* value;
	char* p;
	int i;
	if (!ubuf || !filp || !ppos)
		return -EINVAL;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	if (cnt>0 && buf[cnt-1]=='\n') {
		buf[cnt-1] = 0;
	}
	value = buf;
	p = strsep(&value, ":");
	if ( p == NULL ) {
		return -EINVAL;
	}
	for( i=0;i< sizeof(optArr)/sizeof(struct opt_ops);i++) {
		if(!strncmp(p, optArr[i].name, strlen(optArr[i].name)) && strlen(p) == strlen(optArr[i].name)) {
			optArr[i].func(value);
			return cnt;
		}
	}
	return -EFAULT;
}


static const struct file_operations tz_dbg_opt_fops = {
	.owner = THIS_MODULE,
	.write = tz_dbg_opt_write,
};

static int __init tzdebug_init(void)
{
	tz_dbg_dentry = debugfs_create_dir("tzdebug", NULL);
	if ( !tz_dbg_dentry)
		return 0;
	debugfs_create_file("opt", 0220, tz_dbg_dentry,NULL, &tz_dbg_opt_fops);
	return 0;
}
static void __exit tzdebug_exit(void)
{
	if (!tz_dbg_dentry)
		return;
	debugfs_remove_recursive(tz_dbg_dentry);
	tz_dbg_dentry = NULL;
}
module_init(tzdebug_init);
module_exit(tzdebug_exit);
