#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/usb.h>

#include "hifi-usb.h"

static char *hibernation_policy_name[] = {
	"allow",
	"forbid",
	"force",
};

static int hibernation_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	struct hifi_usb_proxy *hifi_usb = s->private;
	int complete_urb_count;
	struct list_head *pos;

	if (!hifi_usb)
		return -ENOENT;

	mutex_lock(&hifi_usb->msg_lock);

	complete_urb_count = 0;
	list_for_each(pos, &hifi_usb->complete_urb_list) {
		complete_urb_count++;
	}

	seq_printf(s, "hibernation_policy: %s\n"
			"hibernation_support: %s\n"
			"hibernation_state: %s\n"
			"hibernation_ctrl: 0x%x\n"
			"hibernation_count: %d\n"
			"revive_time: %dms\n"
			"max_revive_time: %dms\n"
			"hid_key_pressed: %d\n"
			"complete_urb: %d\n",
			hibernation_policy_name[hifi_usb->hibernation_policy],
			hifi_usb->hibernation_support ? "support" : "not support",
			hifi_usb->hibernation_state ? "hibernated" : "running",
			hifi_usb->hibernation_ctrl,
			hifi_usb->hibernation_count,
			hifi_usb->revive_time,
			hifi_usb->max_revive_time,
			hifi_usb->hid_key_pressed,
			complete_urb_count);
	mutex_unlock(&hifi_usb->msg_lock);

	return 0;
}
static int hibernation_open(struct inode *inode, struct file *file)
{
	return single_open(file, hibernation_show, inode->i_private);
}

static ssize_t hibernation_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	size_t len;
	char buf[32] = {0};
	struct seq_file *s = file->private_data;
	struct hifi_usb_proxy *hifi_usb = s->private;

	if (!hifi_usb)
		return -ENOENT;

	len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[len] = '\0';

	mutex_lock(&hifi_usb->msg_lock);

	if (!strncmp(buf, "allow", sizeof("allow") - 1))
		hifi_usb->hibernation_policy = HIFI_USB_HIBERNATION_ALLOW;
	else if (!strncmp(buf, "forbid", sizeof("forbid") - 1))
		hifi_usb->hibernation_policy = HIFI_USB_HIBERNATION_FORBID;
	else if (!strncmp(buf, "force", sizeof("force") - 1)) {
		hifi_usb->hibernation_policy = HIFI_USB_HIBERNATION_FORCE;
		hifi_usb->hibernation_support = 1;
	}

	mutex_unlock(&hifi_usb->msg_lock);

	return count;
}

static const struct file_operations hibernation_fops = {
	.open			= hibernation_open,
	.write			= hibernation_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int never_use_hifi_usb_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	seq_printf(s, "If you want never use hifi usb, "
			"write \"1\" to this file.\n"
			"If you don't want never use hifi usb, "
			"write \"0\" to this file.\n");
	return 0;
}

static int never_use_hifi_usb_open(struct inode *inode, struct file *file)
{
	return single_open(file, never_use_hifi_usb_show, inode->i_private);
}

static ssize_t never_use_hifi_usb_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	size_t len;
	char buf[32] = {0};

	len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (buf[0] == '1')
		never_use_hifi_usb(1);
	else if (buf[0] == '0')
		never_use_hifi_usb(0);

	return count;
}

static const struct file_operations never_use_hifi_usb_fops = {
	.open			= never_use_hifi_usb_open,
	.write			= never_use_hifi_usb_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int always_use_hifi_usb_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	seq_printf(s, "If you want always use hifi usb, "
			"write \"1\" to this file.\n"
			"If you don't want always use hifi usb, "
			"write \"0\" to this file.\n");
	return 0;
}

static int always_use_hifi_usb_open(struct inode *inode, struct file *file)
{
	return single_open(file, always_use_hifi_usb_show, inode->i_private);
}

static ssize_t always_use_hifi_usb_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	size_t len;
	char buf[32] = {0};

	len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (buf[0] == '1')
		always_use_hifi_usb(1);
	else if (buf[0] == '0')
		always_use_hifi_usb(0);

	return count;
}

static const struct file_operations always_use_hifi_usb_fops = {
	.open			= always_use_hifi_usb_open,
	.write			= always_use_hifi_usb_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int urb_bufs_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	struct hifi_usb_proxy *hifi_usb = s->private;
	struct urb_buffers *bufs;

	if (!hifi_usb)
		return -ENOENT;

	bufs = &hifi_usb->urb_bufs;

	seq_printf(s, "num %d, len %d, bitmap 0x%lx\n", bufs->urb_buf_num,
			bufs->urb_buf_len, bufs->urb_buf_bitmap);

	return 0;
}

static int urb_bufs_open(struct inode *inode, struct file *file)
{
	return single_open(file, urb_bufs_show, inode->i_private);
}

static const struct file_operations urb_bufs_fops = {
	.open			= urb_bufs_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int hifi_usb_stat_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	struct hifi_usb_proxy *hifi_usb = s->private;
	struct hifi_usb_stats *stats;

	if (!hifi_usb)
		return -ENOENT;

	stats = &hifi_usb->stat;

	seq_printf(s, "msg: send %d, receive %d, timeout %d\n",
			stats->stat_send_msg, stats->stat_receive_msg,
			stats->stat_wait_msg_timeout);

	seq_printf(s, "urb: enqueue %d, complete %d, dequeue %d\n",
			atomic_read(&stats->stat_urb_enqueue_msg),
			stats->stat_urb_complete_msg,
			atomic_read(&stats->stat_urb_dequeue_msg));

	seq_printf(s, "hub status change %d\n", stats->stat_hub_status_change_msg);

	return 0;
}

static int hifi_usb_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, hifi_usb_stat_show, inode->i_private);
}

static const struct file_operations hifi_usb_stat_fops = {
	.open			= hifi_usb_stat_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

int hifi_usb_write_reg(unsigned int addr, unsigned int val)
{
	int ret;
	struct hifi_usb_test_msg *mesg;

	mesg = kzalloc(sizeof(*mesg), GFP_KERNEL);
	if (!mesg)
		return -ENOMEM;

	mesg->msg_id = ID_AP_HIFI_USB_TEST;
	mesg->msg_type = AP_HIFI_USB_TEST_REG;
	mesg->data_len = sizeof(struct hifi_usb_test_reg_data);
	mesg->reg.flags = HIFI_USB_TEST_REG_WRITE;
	mesg->reg.addr = addr;
	mesg->reg.val = val;

	ret = hifi_usb_send_mailbox(mesg, sizeof(*mesg));
	if (ret)
		pr_err("[%s]send mailbox to hifi failed\n", __func__);

	kfree(mesg);

	return ret;
}

int hifi_usb_read_reg(unsigned int addr)
{
	int ret;
	struct hifi_usb_test_msg *mesg;

	mesg = kzalloc(sizeof(*mesg), GFP_KERNEL);
	if (!mesg)
		return -ENOMEM;

	mesg->msg_id = ID_AP_HIFI_USB_TEST;
	mesg->msg_type = AP_HIFI_USB_TEST_REG;
	mesg->data_len = sizeof(struct hifi_usb_test_reg_data);
	mesg->reg.flags = HIFI_USB_TEST_REG_READ;
	mesg->reg.addr = addr;

	ret = hifi_usb_send_mailbox(mesg, sizeof(*mesg));
	if (ret)
		pr_err("[%s]send mailbox to hifi failed\n", __func__);

	kfree(mesg);

	return ret;
}

static int send_print_debug_msg(unsigned int flags)
{
	int ret;
	struct hifi_usb_test_msg *mesg;

	mesg = kzalloc(sizeof(*mesg), GFP_KERNEL);
	if (!mesg)
		return -ENOMEM;

	mesg->msg_id = ID_AP_HIFI_USB_TEST;
	mesg->msg_type = AP_HIFI_USB_TEST_DEBUG;
	mesg->data_len = sizeof(struct hifi_usb_test_debug_print_data);
	mesg->debug.flags = flags;

	ret = hifi_usb_send_mailbox(mesg, sizeof(*mesg));
	if (ret)
		pr_err("[%s]send mailbox to hifi failed\n", __func__);

	kfree(mesg);

	return ret;
}

static int hifi_print_dbg_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	seq_printf(s, "Usage: echo <value> >/d/usb/hifiusb/print_dbg\n"
		      "<value> = \n"
		      "print_usb_debug   -> 0x1\n"
		      "print_xhci_debug  -> 0x2\n"
		      "------------------------\n"
		      "bitwise OR is supported\n"
		      "------------------------\n"
		      );

	return 0;
}

static int hifi_print_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, hifi_print_dbg_show, inode->i_private);
}

static ssize_t hifi_print_dbg_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	int ret;
	unsigned int flags;

	ret = kstrtouint_from_user(ubuf, count, 0, &flags);
	if (ret < 0) {
		pr_err("Invalid value for print_dbg\n");
		return ret;
	}

	flags &= USB_PRINT_DEBUG_MASK;
	ret = send_print_debug_msg(flags);
	if (ret < 0) {
		pr_err("set print_dbg to hifi usb failed ret %d\n", ret);
		return ret;
	}

	return count;
}

static const struct file_operations hifi_print_dbg_fops = {
	.open			= hifi_print_dbg_open,
	.write			= hifi_print_dbg_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static unsigned char hifi_usb_sr_state;

static int hifi_usb_sr_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	struct hifi_usb_proxy *hifi_usb = s->private;
	struct hifi_usb_test_msg mesg;
	char *state;
	int ret;

	if (!hifi_usb)
		return -ENOENT;

	if (!hifi_usb->runstop) {
		seq_printf(s, "shutdown\n");
		return 0;
	}

	mutex_lock(&hifi_usb->msg_lock);

	mesg.msg_id = ID_AP_HIFI_USB_TEST;
	mesg.msg_type = AP_HIFI_USB_TEST_SR;
	mesg.data_len = sizeof(struct hifi_usb_test_sr_data);
	mesg.sr_control.action = USB_TEST_INQUIRE_SR_STATE;

	hifi_usb_sr_state = USB_TEST_SR_STATE_UNKNOWN;

	init_completion(&hifi_usb->msg_completion);

	ret = hifi_usb_send_mailbox(&mesg, sizeof(mesg));
	if (ret)
		pr_err("[%s]send mailbox to hifi failed\n", __func__);
	else {
		ret = wait_for_completion_timeout(&hifi_usb->msg_completion,
						HIFI_USB_MSG_TIMEOUT);
		if (ret == 0)
			pr_err("[%s]wait for sr state timeout\n", __func__);
	}

	mutex_unlock(&hifi_usb->msg_lock);

	if (hifi_usb_sr_state == USB_TEST_SR_STATE_SUSPENDED)
		state = "suspended";
	else if (hifi_usb_sr_state == USB_TEST_SR_STATE_RUNNING)
		state = "running";
	else
		state = "unknown";

	seq_printf(s, "%s\n", state);

	return 0;
}
static int hifi_usb_sr_open(struct inode *inode, struct file *file)
{
	return single_open(file, hifi_usb_sr_show, inode->i_private);
}

static ssize_t hifi_usb_sr_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	size_t len;
	char buf[32] = {0};
	int ret;
	struct hifi_usb_test_msg mesg = {0};
	struct seq_file *s = file->private_data;
	struct hifi_usb_proxy *hifi_usb = s->private;

	if (!hifi_usb)
		return -ENOENT;

	if (!hifi_usb->runstop)
		return -EPERM;

	len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[len] = '\0';

	mesg.msg_id = ID_AP_HIFI_USB_TEST;
	mesg.msg_type = AP_HIFI_USB_TEST_SR;
	mesg.data_len = sizeof(struct hifi_usb_test_sr_data);

	if (!strncmp(buf, "do_suspend", sizeof("do_suspend") - 1))
		mesg.sr_control.action = USB_TEST_SUSPEND;
	else if (!strncmp(buf, "do_resume", sizeof("do_resume") - 1))
		mesg.sr_control.action = USB_TEST_RESUME;

	ret = hifi_usb_send_mailbox(&mesg, sizeof(mesg));
	if (ret < 0) {
		pr_err("%s: error ret %d\n", __func__, ret);
		return ret;
	}

	return count;
}

static const struct file_operations hifi_usb_sr_fops = {
	.open			= hifi_usb_sr_open,
	.write			= hifi_usb_sr_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

void hifi_usb_handle_test_mesg(struct hifi_usb_proxy *hifi_usb,
		struct hifi_usb_test_msg *mesg)
{
	if (mesg->msg_type == AP_HIFI_USB_TEST_SR) {
		hifi_usb_sr_state = mesg->sr_control.state;
		complete(&hifi_usb->msg_completion);
	} else {
		pr_err("[%s]ilegal msg_type\n", __func__);
	}
}

/* hifi usb fault inject */

enum inject_point {
	INJECT_AT_NULL,
	INJECT_AT_STARTSTOP,
	INJECT_AT_HCD_MESG,
	INJECT_AT_COMPLETE,
	INJECT_END,
};

enum fault_type {
	NONE_FAULT,
	RESET_HIFI,
	MISS_IPC,
	FAULT_END,
};

struct fault_inject_point {
	char *name;
	size_t len;
	enum inject_point point;
	u32 data;
};

const struct fault_inject_point fault_inject_points_table[] = {
	{"start", 		sizeof("start") - 1, 			INJECT_AT_STARTSTOP, 1},
	{"alloc_dev", 		sizeof("alloc_dev") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_ALLOC_DEV},
	{"free_dev", 		sizeof("free_dev") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_FREE_DEV},
	{"enable_dev", 		sizeof("enable_dev") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_ENABLE_DEV},
	{"reset_dev", 		sizeof("reset_dev") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_RESET_DEV},
	{"address_dev", 	sizeof("address_dev") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_ADDRESS_DEV},
	{"urb_enqueue", 	sizeof("urb_enqueue") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_URB_ENQUEUE},
	{"urb_dequeue", 	sizeof("urb_dequeue") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_URB_DEQUEUE},
	{"add_endpoint", 	sizeof("add_endpoint") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_ADD_ENDPOINT},
	{"drop_endpoint", 	sizeof("drop_endpoint") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_DROP_ENDPOINT},
	{"check_bandwidth", 	sizeof("check_bandwidth") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_CHECK_BANDWIDTH},
	{"reset_bandwidth", 	sizeof("reset_bandwidth") - 1, 		INJECT_AT_HCD_MESG, AP_HIFI_USB_RESET_BANDWIDTH},
	{"complete_ctrl_xfer", 	sizeof("complete_ctrl_xfer") - 1, 	INJECT_AT_COMPLETE, PIPE_CONTROL},
	{"complete_int_xfer", 	sizeof("complete_int_xfer") - 1, 	INJECT_AT_COMPLETE, PIPE_INTERRUPT},
};

static int hifi_test_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	seq_printf(s, "Usage:\n"
		      "1. echo \"start_hifi_usb\" >/d/usb/hifiusb/hifi_test\n"
		      "2. echo \"reset_hifi @fun\" >/d/usb/hifiusb/hifi_test\n"
		      "3. echo \"miss_ipc @fun\" >/d/usb/hifiusb/hifi_test\n"
		      "4. echo \"hc_died\" >/d/usb/hifiusb/hifi_test\n"
		      "-----------------------------\n"
		      "fun list:\n"
		      "start\n"
		      "alloc_dev\n"
		      "free_dev\n"
		      "enable_dev\n"
		      "reset_dev\n"
		      "address_dev\n"
		      "urb_enqueue\n"
		      "urb_dequeue\n"
		      "add_endpoint\n"
		      "drop_endpoint\n"
		      "check_bandwidth\n"
		      "reset_bandwidth\n"
		      "complete_ctrl_xfer\n"
		      "complete_int_xfer\n"
		      "-----------------------------\n");

	return 0;
}

static int hifi_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, hifi_test_show, inode->i_private);
}

static int get_inject_point_data(char *cmd, size_t len,
		enum inject_point *point, u32 *data)
{
	char *fun_name;
	unsigned i;

	if (!cmd || !point || !data)
		return -EINVAL;

	fun_name = strnchr(cmd, len, '@');
	if (!fun_name) {
		pr_err("Unable to locate @ in cmd\n");
		return -EINVAL;
	}

	fun_name++;
	if ((size_t)(fun_name - cmd) >= len)
		return -EINVAL;
	
	for (i = 0;
	     i < sizeof(fault_inject_points_table) / sizeof(struct fault_inject_point);
	     i++) {
		if (!strncmp(fun_name, fault_inject_points_table[i].name,
			fault_inject_points_table[i].len)) {
			*point = fault_inject_points_table[i].point;
			*data = fault_inject_points_table[i].data;
			return 0;
		}
	}

	return -EOPNOTSUPP;
}

static int send_faultinject_msg(enum fault_type fault, char *cmd, size_t len)
{
	struct hifi_usb_test_msg mesg;
	enum inject_point point;
	u32 data;
	int ret;

	ret = get_inject_point_data(cmd, len, &point, &data);
	if (ret < 0) {
		pr_err("[%s] get inject point failed\n", __func__);
		return ret;
	}

	mesg.msg_id = ID_AP_HIFI_USB_TEST;
	mesg.msg_type = AP_HIFI_USB_TEST_FAULTINJECT;
	mesg.data_len = sizeof(struct hifi_usb_faultinject_data);
	mesg.faultinject.fault = fault;
	mesg.faultinject.point = point;
	mesg.faultinject.data = data;

	ret = hifi_usb_send_mailbox(&mesg, sizeof(mesg));
	if (ret)
		pr_err("[%s] send mailbox failed ret %d\n", __func__, ret);

	return ret;
}

static ssize_t hifi_test_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct hifi_usb_proxy *hifi_usb = s->private;
	int ret;
	size_t len;
	char buf[32];

	if (!hifi_usb || !hifi_usb->runstop)
		return -EPERM;

	len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (!strncmp(buf, "start_hifi_usb", sizeof("start_hifi_usb") - 1)) {
		struct hifi_usb_runstop_msg mesg;

		mesg.mesg_id = ID_AP_HIFI_USB_RUNSTOP; /*lint !e63 */
		mesg.reserved = 0;/*lint !e63 */
		mesg.runstop = 1;/*lint !e63 */
		mesg.result = 0;/*lint !e63 */

		ret = hifi_usb_send_mailbox(&mesg, sizeof(mesg));
	} else if (!strncmp(buf, "reset_hifi", sizeof("reset_hifi") - 1)) {
		ret = send_faultinject_msg(RESET_HIFI, buf, len);
	} else if (!strncmp(buf, "miss_ipc", sizeof("miss_ipc") - 1)) {
		ret = send_faultinject_msg(MISS_IPC, buf, len);
	} else if (!strncmp(buf, "hc_died", sizeof("hc_died") - 1)) {
		struct hifi_usb_test_msg mesg;

		mesg.msg_id = ID_AP_HIFI_USB_TEST;
		mesg.msg_type = AP_HIFI_USB_TEST_HC_DIED;

		ret = hifi_usb_send_mailbox(&mesg, sizeof(mesg));
	} else
		return -EINVAL;

	if (ret < 0) {
		pr_err("[%s] execute test failed\n", __func__);
		return ret;
	}

	return count;
}

static const struct file_operations hifi_test_fops = {
	.open			= hifi_test_open,
	.write			= hifi_test_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

int hifi_usb_debugfs_init(struct hifi_usb_proxy *hifi_usb)
{
	struct dentry		*root;
	struct dentry		*file;

	root = debugfs_create_dir("hifiusb", usb_debug_root);
	if (IS_ERR_OR_NULL(root))
		return -ENOMEM;

	file = debugfs_create_file("print_dbg", S_IRUGO | S_IWUSR, root,
			hifi_usb, &hifi_print_dbg_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("hifi_test", S_IRUGO | S_IWUSR, root,
			hifi_usb, &hifi_test_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("sr", S_IRUSR | S_IWUSR, root,
			hifi_usb, &hifi_usb_sr_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("stat", S_IRUGO, root,
			hifi_usb, &hifi_usb_stat_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("urb_bufs", S_IRUGO, root,
			hifi_usb, &urb_bufs_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("always_use_hifi_usb", S_IRUGO | S_IWUSR, root,
			hifi_usb, &always_use_hifi_usb_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("never_use_hifi_usb", S_IRUGO | S_IWUSR, root,
			hifi_usb, &never_use_hifi_usb_fops);
	if (!file)
		goto err;

	file = debugfs_create_file("hibernation", S_IRUGO | S_IWUSR, root,
			hifi_usb, &hibernation_fops);
	if (!file)
		goto err;

	hifi_usb->debugfs_root = root;
	return 0;

err:
	debugfs_remove_recursive(root);
	return -ENOMEM;
}

void hifi_usb_debugfs_exit(struct hifi_usb_proxy *hifi_usb)
{
	if (!hifi_usb->debugfs_root)
		return;

	debugfs_remove_recursive(hifi_usb->debugfs_root);
}
