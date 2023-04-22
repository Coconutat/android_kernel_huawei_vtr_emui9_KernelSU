#include "../rasbase/rasbase.h"
#include "../rasbase/rasproc.h"
#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/delay.h>
#ifdef CONFIG_FB_MSM
#include <uapi/linux/msm_mdp.h>
#else

#if defined(CONFIG_FB_3630) || defined(CONFIG_FB_3635)
	#include <drivers/video/hisi/hi3630/hisi_dss.h>
#elif defined(CONFIG_FB_3650) || defined(CONFIG_FB_6250)
	#include <drivers/video/hisi/hi3650/hisi_dss.h>
#else
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 0, 0)
		#include <drivers/video/hisi/dss/hisi_dss.h>
	#else
		#include <drivers/video/fbdev/hisi/dss/hisi_dss.h>
	#endif
#endif
#endif
#define RFB_FRAME_RATE_MSEC 16
#define RFB_FRAME_RATE_USEC 700
static int rfb_dbg;
static int rfb_exit;
enum fault_type {
	FAULT_NONE = 0,
	FAULT_FRAME_DELAY,	/*read delay, care or no care address */
	FAULT_FRAME_LOST_EQ,
	FAULT_FRAME_LOST_ASC,
	FAULT_FRAME_LOST_DESC,
	FAULT_FRAME_FAILED
};

struct fault_ops {
	const char *name;
	enum fault_type type;
};
static int rasfb_ioctl(struct fb_info *info, unsigned int cmd,
		       unsigned long arg);

/* the stubs is the indexs of probe_manager, start from 1 not 0.*/
static struct fault_ops fault_ops_array[] = {
	{.name = "delay", .type = FAULT_FRAME_DELAY},
	{.name = "lost_eq", .type = FAULT_FRAME_LOST_EQ},
	{.name = "lost_asc", .type = FAULT_FRAME_LOST_ASC},
	{.name = "lost_desc", .type = FAULT_FRAME_LOST_DESC},
	{.name = "failed", .type = FAULT_FRAME_FAILED},
};

/*fault impl*/
struct fault_impl {
	enum fault_type type;
	enum fault_type input_type;
	int delay;
	int udelay;
	int frame;
	int count;
	int match;
	int display;
	int lost;
	int step;
};
#define FAULT_MAX 32

struct fault_list {
	atomic_t use_res;
	struct fault_impl impl[FAULT_MAX];

	int (*fb_ioctl[FB_MAX])(struct fb_info *info, unsigned int cmd,
				 unsigned long arg);

	struct fb_ops *fbops[FB_MAX];
	int res[FB_MAX];
	rwlock_t rwk;
};

/*record the faults which was injected.*/
static struct fault_list fault_injected;
static int fault_restore_all(void)
{
	int i = 0;
	int ret = 0;

	write_lock(&fault_injected.rwk);
	for (i = 0; i < ARRAY_SIZE(fault_injected.fbops); i++) {
		if (fault_injected.fbops[i] != NULL) {
			if (rfb_dbg)
				pr_err("rfb_dbg:restore %p=>%p,i=%d\n",
				       fault_injected.fbops[i]->fb_ioctl,
				       fault_injected.fb_ioctl[fault_injected.
							       res[i]], i);
			fault_injected.fbops[i]->fb_ioctl =
			    fault_injected.fb_ioctl[fault_injected.res[i]];
		}
	}
	write_unlock(&fault_injected.rwk);
	rfb_exit = 1;
	return ret;
}

static struct fault_ops *get_fault_ops(enum fault_type type)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops_array); i++) {
		if (fault_ops_array[i].type == type)
			return &fault_ops_array[i];
	}
	return NULL;
}

static const char *type2name(enum fault_type type)
{
	struct fault_ops *ops = get_fault_ops(type);

	if (!ops)
		return "fault_unknow";
	return ops->name;
}

static enum fault_type name2type(const char *name)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops_array); i++) {
		if (strcmp(name, fault_ops_array[i].name) == 0)
			return fault_ops_array[i].type;
	}
	return FAULT_NONE;
}

static int rasio_args_set(struct fault_impl *fault_args, char *prm)
{
	if (0 == prm)
		return 0;
	rasbase_set_func(fault_args, type, prm, name2type);
	rasbase_set(fault_args, delay, prm);
	rasbase_set(fault_args, display, prm);
	rasbase_set(fault_args, lost, prm);
	rasbase_set(fault_args, step, prm)
	    return 0;
}

static int rasio_args_parse(struct fault_impl *fault_args, int args,
			    char *argv[])
{
	int i;

	fault_args->step = 1;
	for (i = 0; i < args; i++)
		ras_retn_iferr(rasio_args_set(fault_args, argv[i]));

	fault_args->input_type = fault_args->type;
	if (rfb_dbg)
		pr_err("rfb_dbg:input_type=%d,delay=%d,display=%d,lost=%d,s\n",
		       fault_args->input_type, fault_args->delay,
		       fault_args->display, fault_args->lost);
	ras_retn_if((fault_args->input_type == FAULT_NONE), -EINVAL);
	ras_retn_if((fault_args->input_type == FAULT_FRAME_DELAY
		     && fault_args->delay == 0), -EINVAL);
	ras_retn_if((fault_args->input_type >= FAULT_FRAME_LOST_EQ
		     && fault_args->input_type <= FAULT_FRAME_LOST_DESC
		     && fault_args->lost == 0), -EINVAL);

	if (fault_args->type != FAULT_FRAME_FAILED) {
		fault_args->type = FAULT_FRAME_DELAY;
		if (fault_args->input_type != FAULT_FRAME_DELAY) {
			fault_args->delay =
			    (fault_args->lost * RFB_FRAME_RATE_MSEC) +
			    ((fault_args->lost * RFB_FRAME_RATE_USEC) / 1000);
			fault_args->udelay =
			    (fault_args->lost * RFB_FRAME_RATE_USEC % 1000);
		} else
			fault_args->lost = 1;
	}
	return 0;
}

void replace_ioctl(struct fault_impl *fault)
{
	int i = 0, j = 0, bFind = 0, index = -1;

	write_lock(&fault_injected.rwk);
	for (i = 0; i < FB_MAX; i++) {
		if (registered_fb[i] == NULL)
			continue;

		bFind = 0;
		index = -1;
		for (j = 0; j < ARRAY_SIZE(fault_injected.fbops); j++) {
			if (fault_injected.fbops[j] == NULL && index == -1)
				index = j;

			if (fault_injected.fbops[j] != NULL
			    && registered_fb[i]->fbops ==
			    fault_injected.fbops[j]) {
				bFind = 1;
				fault_injected.fb_ioctl[i] =
				    fault_injected.fb_ioctl[fault_injected.
							    res[j]];
				if (rfb_dbg)
					pr_err("rfb_dbg:i=%d,j=%d,%d,%p,node=%d\n",
					     i, j, fault_injected.res[j],
					     fault_injected.fb_ioctl[i],
					     registered_fb[i]->node);
				break;
			}
		}
		if (bFind)
			continue;
		if (registered_fb[i]->fbops->fb_ioctl) {
			if (rfb_dbg)
				pr_err("rfb_dbg:replace_ioctl,i=%d,%d,%p-%p,%d\n",
				     i, index, rasfb_ioctl,
				     registered_fb[i]->fbops->fb_ioctl,
				     registered_fb[i]->node);
			fault_injected.fb_ioctl[i] =
			    registered_fb[i]->fbops->fb_ioctl;
			fault_injected.fbops[index] = registered_fb[i]->fbops;
			fault_injected.res[index] = i;
			registered_fb[i]->fbops->fb_ioctl = rasfb_ioctl;
		}
	}
	write_unlock(&fault_injected.rwk);
}

/*Conver args to faults, then inject and manage them.*/
static struct fault_impl *args2fault(int args, char *argv[])
{
	int i;
	struct fault_impl *fault_cur = NULL;
	struct fault_impl fault_args;

	memset(&fault_args, 0, sizeof(fault_args));

	/*1. check the args */
	if (args < 1)
		return NULL;
	if (rasio_args_parse(&fault_args, args, argv))
		return NULL;

	/*2. manage the faults */
	write_lock(&fault_injected.rwk);
	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		if (fault_cur == NULL
		    && fault_injected.impl[i].type == FAULT_NONE) {
			fault_cur = &fault_injected.impl[i];
		}
		/*check fault exist, check type/pdi/disk */
		if (fault_injected.impl[i].type == fault_args.type) {
			fault_cur = NULL;
			break;
		}
	}

	/*3. got it and inject it. */
	if (fault_cur)
		memcpy(fault_cur, &fault_args, sizeof(struct fault_impl));
	write_unlock(&fault_injected.rwk);

	if (fault_cur && fault_cur->type != FAULT_FRAME_FAILED)
		replace_ioctl(fault_cur);
	return fault_cur;
}

struct fault_impl *fault_include(enum fault_type type)
{
	int i = 0;
	struct fault_impl *fault = NULL;

	read_lock(&fault_injected.rwk);
	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		if (fault_injected.impl[i].type == type) {
			fault = &fault_injected.impl[i];
			break;
		}
	}
	read_unlock(&fault_injected.rwk);

	return fault;
}

struct fault_impl *get_fault(enum fault_type type)
{
	struct fault_impl *fault = NULL;

	fault = fault_include(type);
	if (!fault)
		return NULL;

	fault->count++;
	if (rfb_dbg)
		pr_err("rfb_dbg:count=%d,step=%d,frame=%d,lost=%d,display=%d\n",
		       fault->count, fault->step, fault->frame, fault->lost,
		       fault->display);
	if (fault->count < fault->step)
		return NULL;

	if (fault->frame < fault->lost) {
		fault->frame++;
		fault->match++;
		fault->count = 0;
		return fault;
	} else if (fault->frame < fault->display + fault->lost) {
		fault->frame++;
		if (fault->frame == fault->display + fault->lost)
			fault->frame = 0;
	} else {
		fault->frame = 0;
	}
	return NULL;
}
static int should_delay_checkcmd(unsigned int cmd)
{
	switch (cmd) {
	#ifdef CONFIG_FB_MSM
	case MSMFB_DISPLAY_COMMIT:
	case MSMFB_OVERLAY_SET:
	#else
	case HISIFB_OV_ONLINE_PLAY:
	case HISIFB_OV_OFFLINE_PLAY:
	#endif
		return 1;
	default:
		break;
	}
	return 0;
}

void rasfb_delay(int delay, int udelay)
{
	while (delay > 0) {
		if (rfb_exit)
			break;
		if (delay >= 50)
			msleep(50);
		else
			msleep(delay);
		delay -= 50;
	}
	if (udelay)
		udelay(udelay);
}

static int rasfb_ioctl(struct fb_info *info, unsigned int cmd,
		       unsigned long arg)
{
	struct fault_impl *fault = NULL;
	int ret = -ENODEV;
	int delay = 0;
	int udelay = 0;

	atomic_inc(&(fault_injected.use_res));
	if (should_delay_checkcmd(cmd))
		fault = get_fault(FAULT_FRAME_DELAY);
	if (fault_injected.fb_ioctl[info->node])
		ret = fault_injected.fb_ioctl[info->node](info, cmd, arg);
	if (fault) {
		if (fault->input_type == FAULT_FRAME_LOST_ASC) {
			delay =
			    fault->frame * RFB_FRAME_RATE_MSEC +
			    (fault->frame * RFB_FRAME_RATE_USEC) / 1000;
			udelay = (fault->frame * RFB_FRAME_RATE_USEC) % 1000;
		} else if (fault->input_type == FAULT_FRAME_LOST_DESC) {
			delay =
			    (fault->lost + 1 -
			     fault->frame) * RFB_FRAME_RATE_MSEC +
			    ((fault->lost + 1 -
			      fault->frame) * RFB_FRAME_RATE_USEC) / 1000;
			udelay =
			    ((fault->lost + 1 -
			      fault->frame) * RFB_FRAME_RATE_USEC) % 1000;
		} else {
			delay = fault->delay;
			udelay = fault->udelay;
		}
		rasfb_delay(delay, udelay);
		if (rfb_dbg)
			pr_err("rfb_dbg:delay=%d,udelay=%d,frame=%d\n",
			       fault->delay, fault->udelay, fault->frame);
	}
	atomic_dec(&(fault_injected.use_res));
	return ret;
}

static int cmd_main(void *data, int argc, char *args[])
{
	struct fault_impl *fault;

	ras_retn_if(0 == args[0], -EINVAL);
	if (args[0] == NULL)
		return -EINVAL;

	ras_retn_if((strcmp(args[0], "clean") == 0), fault_restore_all());
	fault = args2fault(argc, args);
	ras_retn_if((!fault), -EINVAL);
	return 0;
}

static int proc_ops_show(rfb)(struct seq_file *m, void *v)
{
	int i = 0;
	char buf[256] = { 0 };
	struct fault_impl *impl = NULL;

	write_lock(&fault_injected.rwk);
	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		impl = &fault_injected.impl[i];
		if (impl->type == FAULT_NONE)
			continue;
		if (impl->input_type == FAULT_FRAME_DELAY) {
			snprintf(buf, sizeof(buf), "%d %s  %d  match(%d)\n", i,
				 type2name(impl->input_type), impl->delay,
				 impl->match);
		} else {
			snprintf(buf, sizeof(buf), "%d %s  %d  match(%d)\n", i,
				 type2name(impl->input_type), impl->lost,
				 impl->match);
		}
		seq_printf(m, "%s", buf);
	}

	write_unlock(&fault_injected.rwk);
	return 0;
}

static int proc_ops_open(rfb)(struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rfb), NULL);
}

static ssize_t proc_ops_write(rfb) (struct file *filp,
				    const char __user *bff, size_t count,
				    loff_t *data) {
	char buf_cmd[256];

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	memset(buf_cmd, 0, sizeof(buf_cmd));
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd, count, cmd_main, NULL));
	return count;
}

#define MODULE_NAME "rFb"
proc_ops_define(rfb);
module_param(rfb_dbg, int, 0644);
MODULE_PARM_DESC(rfb_dbg, "debug param!\n");
static int tool_init(void)
{
	ras_debugset(0);
	ras_retn_iferr(ras_check());
	memset(&fault_injected, 0, sizeof(struct fault_list));
	rwlock_init(&fault_injected.rwk);
	atomic_set(&(fault_injected.use_res), 0);
	ras_retn_iferr(proc_init
		       (MODULE_NAME, &proc_ops_name(rfb), &fault_injected));
	return 0;
}

static void tool_exit(void)
{
	int i = 0;

	proc_exit(MODULE_NAME);
	fault_restore_all();
	while (atomic_read(&(fault_injected.use_res)) > 0) {
		ras_sleep(100);
		i++;
	}
}

module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("Fb faults inject.");
MODULE_LICENSE("GPL");
MODULE_VERSION("V001R001C151-1.0");
