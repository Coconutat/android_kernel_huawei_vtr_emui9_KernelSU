

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define LOG_USERTYPE_NAME "log-usertype"

struct proc_dir_entry *log_usertype2 = NULL;

static unsigned int log_usertype_flag2 = 0;

unsigned int get_logusertype_flag2(void)
{
    return log_usertype_flag2;
}
EXPORT_SYMBOL(get_logusertype_flag2);

static void set_logusertype_flag2(int value)
{
    log_usertype_flag2 = value;
}

static int logusertype_info_show2(struct seq_file *m, void *v)
{
    seq_printf(m,"%d\n", log_usertype_flag2);
    return 0;
}
ssize_t logusertype_write_proc2(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
    int ret = -EINVAL;
    char tmp;

    if (count > 2) { //only support receive '0' to '9' and add '\0' in the end
        return ret;
    }

    if (get_logusertype_flag2() > 0) { //log_usertype_flag should be assigned only once
        return ret;
    }

    if (copy_from_user(&tmp, buffer, 1)) { //should ignore character '\n'
        return -EFAULT;
    }

    if (tmp >= '1' && tmp <= '9') {
        set_logusertype_flag2((int)(tmp-'0'));
    }
    return 1;
}

static int logusertype_open2(struct inode *inode, struct file *file)
{
    return single_open(file, logusertype_info_show2, NULL);
}


static const struct file_operations logusertype_proc_fops2 = {
    .open		= logusertype_open2,
    .read		= seq_read,
    .write		= logusertype_write_proc2,
    .llseek		= seq_lseek,
    .release	= single_release,
};

/*lint -save -e* */
static int __init logusertype_proc_init2(void) {
    proc_create(LOG_USERTYPE_NAME, 0600, NULL, &logusertype_proc_fops2);
    return 0;
}

module_init(logusertype_proc_init2);
/*lint -restore*/
