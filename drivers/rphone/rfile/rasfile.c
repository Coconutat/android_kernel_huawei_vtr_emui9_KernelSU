#include "../rasbase/rasbase.h"
#include "../rasbase/rasprobe.h"
#include "../rasbase/rasproc.h"
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/statfs.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/kthread.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#define MNT_PARENT(mnt)  (mnt->mnt_parent)
#define MNT_POINT(mnt)   (mnt->mnt_mountpoint)
#else
#include "proc/internal.h"
#include "mount.h"
#define MNT_PARENT(mnt)  (real_mount(mnt)->mnt_parent ? \
(&real_mount(mnt)->mnt_parent->mnt):0)
#define MNT_POINT(mnt)   (real_mount(mnt)->mnt_mountpoint)
#endif

#define FILE_DEN(fp) (fp->f_path.dentry)
#define FILE_MNT(fp) (fp->f_path.mnt)

enum ChfilType {
	CHFILETYPE_DEADLOCK = 3,
	CHFILETYPE_UNREAD,
	CHFILETYPE_UNWRITE,
	CHFILETYPE_UNRW,
	CHFILETYPE_UNRM,
	CHFILETYPE_FULL,
	CHFILETYPE_ZERO,
	CHFILETYPE_BROKEN,
	CHFILETYPE_UNRENAME,
	CHFILETYPE_HUNMOUNT,
	CHFILETYPE_HUNMOUNTALL,
	CHFILETYPE_UNMOUNTBUSY,
	CHFILETYPE_DEPLENISH,
	CHFILETYPE_READONLY,
	CHFILETYPE_UNOPEN,
	CHFILETYPE_UNEXEC,
	CHFILETYPE_UNMOUNT,
	CHFILETYPE_UNUMOUNT,
	CHFILETYPE_MISSED,
};

struct ChfilImpl {
	struct file *fp;
	struct dentry *den;
	struct vfsmount *mnt;
	char *path;
	long long size;
	enum ChfilType ct;
};

struct Chfil {
	struct ChfilImpl impl[16];
	struct RasProc *proc;
	rwlock_t rwk;
};

struct log_switch {
	int enable;
/*records the operations in the file or
 directory included by the dentry*/
	struct dentry *den;
	struct vfsmount *mnt;
};

static struct Chfil chfil;
static struct log_switch log_sw;
static inline int chfilimpl_equalsb(struct ChfilImpl *ci,
				    struct super_block *sb)
{
	return ci->den->d_sb == sb;
}
static inline int chfilimpl_equal(struct ChfilImpl *ci, struct dentry *den)
{
	return ci->den == den;
}

static int check_used(void *dt, struct ChfilImpl *ci)
{
	return (0 != ci->ct || 0 != ci->den) ? 1 : 0;
}

static int handle_restore_func(void *data)
{
	struct ChfilImpl *ci = data;

	ras_sleep(ci->size * 1000);
	write_lock(&chfil.rwk);
	memset(ci, 0, sizeof(struct ChfilImpl));
	write_unlock(&chfil.rwk);
	return 0;
}

static int handle_restore(struct ChfilImpl *ci)
{
	struct task_struct *tsk;

	tsk = kthread_create(handle_restore_func, ci, "_restore");
	ras_retn_if(IS_ERR(tsk), PTR_ERR(tsk));
	wake_up_process(tsk);
	return 0;
}

static inline int _chfil_fill(struct Chfil *chf, const char *name,
			      long long size, struct file *flp,
			      struct dentry *den, int ct)
{
	struct ChfilImpl *ci = 0;
	int i, len = ARRAY_SIZE(chf->impl);

	for (i = 0; i < len; i++) {
		if (!check_used(0, &chf->impl[i])) {
			if (NULL == ci)
				ci = &chf->impl[i];
		} else if (CHFILETYPE_HUNMOUNTALL == ct
			   && ct == chf->impl[i].ct) {
			ci = &chf->impl[i];
			break;
		} else if (chfilimpl_equal(&chf->impl[i], den)) {
			ci = &chf->impl[i];
			break;
		}
	}

	if (0 == ci)	/*find the position to inject*/
		return -ENOMEM;
	ci->ct = ct;
	ci->size = size;
	switch (ct) {
	case CHFILETYPE_DEPLENISH:
		handle_restore(ci);	/*fd deplenish resotre auto */
		break;
	case CHFILETYPE_UNMOUNT:
		ras_retn_if(ras_malloc((void **)&ci->path,
		 strlen(name)+1), -ENOMEM);
		strcpy(ci->path, name);
		break;
	case CHFILETYPE_UNUMOUNT:
		if (flp) {
			ci->den = FILE_DEN(flp);
			ci->mnt = FILE_MNT(flp);
			fput(flp);	/*if hold file umount return busy*/
		}
		break;
	default:
		if (flp) {
			ci->fp = flp;
			ci->den = den;
			ci->mnt = FILE_MNT(flp);
		}
		break;
	}
	return 0;

}
static int chfil_get(char *name, struct file **ppflp, struct dentry **ppden)
{
	struct dentry *den;
	struct file *flp = filp_open(name, O_RDONLY, 0);

	ras_retn_if(IS_ERR(flp), PTR_ERR(flp));
	den = FILE_DEN(flp);
	if (S_ISBLK(den->d_inode->i_mode)) {
		struct super_block *sb;
		struct block_device *bdev = lookup_bdev(name);

		if (IS_ERR(bdev)) {
			fput(flp);
			ras_retn(PTR_ERR(bdev));
		}
		sb = get_super(bdev);
		if (0 == sb)
			ras_retn(-EIO);
		den = sb->s_root;
		drop_super(sb);
	}
	*ppflp = flp;
	*ppden = den;
	return 0;
}
static int chfil_fill(struct Chfil *chf, char *name, long long size, int ct)
{
	int ret = -EINVAL;
	struct dentry *den = 0;
	struct file *flp = 0;

	switch (ct) {
	case CHFILETYPE_UNMOUNT:	/*don't need open the file*/
		break;
	case CHFILETYPE_DEPLENISH:
		ras_retn_iferr(ras_atoll(&size, name, strlen(name), 0));
		size = size < 10 ? 10 : size;/*at least 10s restore */
		break;
	default:
		ras_retn_iferr(chfil_get(name, &flp, &den));
		break;
	}

	write_lock(&chf->rwk);
	ret = _chfil_fill(chf, name, size, flp, den, ct);
	write_unlock(&chf->rwk);
	ras_retn(ret);
}

static inline int chfilimpl_dtor(struct ChfilImpl *impl)
{
	if (impl->fp)
		fput(impl->fp);
	if (impl->path)
		ras_free(impl->path);
	memset(impl, 0, sizeof(struct ChfilImpl));
	return 0;
}

static inline int _chfil_restore(struct Chfil *chf, struct file *flp,
				 struct dentry *den)
{
	int i, len = ARRAY_SIZE(chf->impl);

	for (i = 0; i < len; i++) {
		if (chfilimpl_equal(&chf->impl[i], den))
			ras_retn(chfilimpl_dtor(&chf->impl[i]));
	}
	ras_retn(-ENOENT);
}

static int _chfil_cancel(struct Chfil *chf, long long ll)
{
	if (ll >= ARRAY_SIZE(chf->impl))
		ras_retn(-EINVAL);
	ras_retn(chfilimpl_dtor(&chf->impl[ll]));
}

static int chfil_cancel(struct Chfil *chf, char *name)
{
	int ret;
	long long ll = 0;

	ras_retn_iferr(ras_atoll(&ll, name, strlen(name), 0));
	write_lock(&chf->rwk);
	ret = _chfil_cancel(chf, ll);
	write_unlock(&chf->rwk);
	ras_retn(ret);
}

static int chfil_restore(struct Chfil *chf, char *name)
{
	int ret;
	struct dentry *den = 0;
	struct file *flp = 0;

	ras_retn_iferr(chfil_get(name, &flp, &den));
	write_lock(&chf->rwk);
	ret = _chfil_restore(chf, flp, den);
	write_unlock(&chf->rwk);
	fput(flp);
	ras_retn(ret);
}

static struct {
	const int len;
	const char *op;
	enum ChfilType ct;
	int ret;
	int val;
} chfil_ops[] = {
	{.len = 8, .op = "deadlock", .ret = EDEADLK,
	.val = 0, .ct = CHFILETYPE_DEADLOCK},
	{.len = 6, .op = "unread", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNREAD},
	{.len = 7, .op = "unwrite", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNWRITE},
	{.len = 4, .op = "unrw", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNRW},
	{.len = 4, .op = "unrm", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNRM},
	{.len = 4, .op = "full", .ret = ENOSPC,
	.val = 0, .ct = CHFILETYPE_FULL},
	{.len = 6, .op = "broken", .ret = 0,
	.val = 0, .ct = CHFILETYPE_BROKEN},
	{.len = 4, .op = "zero", .ret = 0,
	.val = 0, .ct = CHFILETYPE_ZERO},
	{.len = 4, .op = "unrn", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNRENAME},
	{.len = 4, .op = "humt", .ret = 0,
	.val = 0, .ct = CHFILETYPE_HUNMOUNT},
	{.len = 7, .op = "humtall", .ret = 0,
	.val = 0, .ct = CHFILETYPE_HUNMOUNTALL},
	{.len = 7, .op = "umtbusy", .ret = 0,
	.val = 0, .ct = CHFILETYPE_UNMOUNTBUSY},
	{.len = 9, .op = "deplenish", .ret = EMFILE,
	.val = 0, .ct = CHFILETYPE_DEPLENISH},
	{.len = 8, .op = "readonly", .ret = EROFS,
	.val = 0, .ct = CHFILETYPE_READONLY},
	{.len = 6, .op = "unopen", .ret = EACCES,
	.val = 0, .ct = CHFILETYPE_UNOPEN},
	{.len = 6, .op = "unexec", .ret = EPERM,
	.val = 0, .ct = CHFILETYPE_UNEXEC},
	{.len = 7, .op = "unmount", .ret = ENOTBLK,
	.val = 0, .ct = CHFILETYPE_UNMOUNT},
	{.len = 8, .op = "unumount", .ret = EPERM,
	.val = 0, .ct = CHFILETYPE_UNUMOUNT},
	{.len = 6, .op = "missed", .ret = ENOENT,
	.val = 0, .ct = CHFILETYPE_MISSED},
	};

static int chfilimpl_geturn(const struct ChfilImpl *impl)
{
	int i, len = ARRAY_SIZE(chfil_ops);

	for (i = 0; i < len; i++) {
		if (impl->ct == chfil_ops[i].ct)
			return -chfil_ops[i].ret;
	}
	return 0;
}

static inline int chfil_addsize(struct Chfil *chf, char *cmd, int len,
				char *name)
{
	long long size = 0;

	ras_retn_iferr(ras_atoll(&size, cmd, len, 0));
	ras_retn(chfil_fill(chf, name, size, 0));
}

static int chfil_log(char *name)
{
	struct dentry *den = 0;
	struct file *flp = 0;

	ras_retn_iferr(chfil_get(name, &flp, &den));
	log_sw.den = FILE_DEN(flp);
	log_sw.mnt = FILE_MNT(flp);
	log_sw.enable = 1;
	return 0;
}

const char *type2name(enum ChfilType why)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(chfil_ops); i++) {
		if (chfil_ops[i].ct == why)
			return chfil_ops[i].op;
	}
	return " ";
}

#define logger_unblocked(pid, pname, path, operater, fault)/*to do*/
void file_ops_tracer(struct task_struct *who, struct dentry *den,
	struct vfsmount *mnt,
	const char *what, enum ChfilType why)
{
	struct path f_path;
	char path_buff[1024];
	char *path;

	if (!log_sw.enable || !mnt || !den)
		return;

	f_path.mnt = mnt;
	f_path.dentry = den;
	path = d_path(&f_path, path_buff, sizeof(path_buff));

	if (IS_ERR(path))
		return;
#ifdef __LOG_UNBLOCKED__
	logger_unblocked(
	who->pid,
	who->comm,
	path,
	what,
	type2name(why));
#else/*just printk*/
	pr_err("file_ops_tracer:%s, %s, %s\n",
	path,
	what,
	type2name(why));
#endif
}

static int dentry_include(struct dentry *dentry_src, struct dentry *dentry_dest)
{
	if (!dentry_src || !dentry_dest)
		return 0;
	while (dentry_src && (dentry_src != dentry_src->d_parent)) {
		ras_retn_if((dentry_src == dentry_dest), 1);
		dentry_src = dentry_src->d_parent;
	}
	ras_retn(dentry_src == dentry_dest);
}

void file_key_ops_tracer(struct task_struct *who, struct dentry *den,
	const char *what)
{
	if (!log_sw.enable || !dentry_include(den, log_sw.den))
		return;
	file_ops_tracer(who, den, log_sw.mnt, what, 0);
}

typedef int (*checker) (void *, struct ChfilImpl *);
typedef long long (*operater) (void *, const struct ChfilImpl *);

static int check_equal(void *dt, struct ChfilImpl *ci)
{
	struct dentry *den = dt;

	return (0 != ci->den && chfilimpl_equal(ci, den)) ? 1 : 0;
}

static int check_include(void *data, struct ChfilImpl *ci)
{
	if (ci->den == 0)
		return 0;
	return dentry_include((struct dentry *)data, ci->den);
}

static long long should_fail(checker ck, operater op, void *data)
{
	int i = 0;
	long long ret = 0;
	int len = ARRAY_SIZE(chfil.impl);

	for (i = 0; i < len; i++) {	/*no lock operation */
		if (!ck(data, &chfil.impl[i]))
			continue;
		ret = op(data, &chfil.impl[i]);
		if (0 != ret)
			break;
	}
	return ret;
}

#define spase_used 0
#define spase_lose 1
#define ULLMAX ((unsigned long long)(-1))
static unsigned long long chfil_injected_size(struct super_block *sb, int mode)
{
	int i, len;
	unsigned long long ret = 0;
	unsigned long long size = 0;

	read_lock(&chfil.rwk);
	for (i = 0, len = ARRAY_SIZE(chfil.impl); i < len; i++) {
		if (0 == chfil.impl[i].den
		    || !chfilimpl_equalsb(&chfil.impl[i], sb))
			continue;
		if (CHFILETYPE_FULL == chfil.impl[i].ct
			&& mode == spase_used) {/*no space left by used */
			ret = ULLMAX;
			break;
		}
		if (0 != chfil.impl[i].ct)
			continue;

		if (mode == 0) {	/*space used increased */
			size = chfil.impl[i].size > 0 ? chfil.impl[i].size : 0;
		} else {	/*space lose */

			size =
			    chfil.impl[i].size <
			    0 ? (0 - chfil.impl[i].size) : 0;
		}

		if (ULLMAX - ret > size) {
			ret += size;
		} else {
			ret = ULLMAX;
			break;
		}
	}
	read_unlock(&chfil.rwk);
	return ret;
}

static unsigned long long get_block_free_size(struct super_block *sb)
{
	struct kstatfs fs_state;

	if (sb == 0)
		return 0;
	memset(&fs_state, 0, sizeof(struct kstatfs));
	if (sb->s_op->statfs(sb->s_root, &fs_state) != 0)
		return -1;
	return (unsigned long long)(fs_state.f_bavail * fs_state.f_bsize);
}

static int rasprobe_handler(vfs_statfs) (struct rasprobe_instance *ri,
					 struct pt_regs *regs) {
	struct RasRegs *rd = (struct RasRegs *)ri->data;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	struct super_block *sb = ((struct dentry *)rd->args[0])->d_sb;
#else
	struct super_block *sb = ((struct path *)rd->args[0])->dentry->d_sb;
#endif
	unsigned long long blks = 0;
	unsigned long long size_free = 0;
	unsigned long long size_add = chfil_injected_size(sb, spase_used);
	unsigned long long size_sub = chfil_injected_size(sb, spase_lose);
	struct kstatfs *kst = rd->args[1];

	if (0 == kst)
		return 0;
	if (size_add == 0 && size_sub == 0)
		return 0;
	size_free = get_block_free_size(sb);

	/*1. reduce the total size of block*/
	if (size_sub < size_free) {
		blks = ras_div(size_sub, kst->f_bsize);
		kst->f_bfree -= blks;
		kst->f_bavail -= blks;
		kst->f_blocks -= blks;
	} else {
		kst->f_bfree = 0;
		kst->f_bavail = 0;
		blks = ras_div(size_free, kst->f_bsize);
		kst->f_blocks -= blks;
	}
	size_free -= size_sub;
	if (size_free <= 0)
		return 0;

	/*2. increase the total size of block  */
	if (size_add < size_free) {
		blks = ras_div(size_add, kst->f_bsize);
		kst->f_bfree -= blks;
		kst->f_bavail -= blks;
	} else {
		kst->f_bfree = 0;
		kst->f_bavail = 0;
	}
	return 0;
}

static long long fail_null_file(void *data, const struct ChfilImpl *ci)
{
	return (ci->ct == 0 && ci->size == 0) ? 1 : 0;
}

static long long fail_size_file(void *data, const struct ChfilImpl *ci)
{
	struct dentry *den = data;
	unsigned long long size_free = 0;

	if (ci->ct == 0 && ci->size <= 0)
		return 0;
	size_free = get_block_free_size(den->d_sb);
	return (ci->size < size_free) ? ci->size : size_free;
}

static long long chfil_missed(void *data, const struct ChfilImpl *impl)
{
	if (CHFILETYPE_MISSED == impl->ct)
		ras_retn(chfilimpl_geturn(impl));
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
static int rasprobe_handler(vfs_getattr) (struct rasprobe_instance *ri,
					  struct pt_regs *regs) {
#else
static int rasprobe_handler(vfs_getattr_nosec) (struct rasprobe_instance *ri,
					  struct pt_regs *regs) {
#endif
	int ret = 0;
	long long size = 0;
	struct kstat *kst = NULL;
	struct dentry *den = NULL;
	struct RasRegs *rd = (struct RasRegs *)ri->data;

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0)
	struct path *pth = (struct path *)rd->args[0];

	den = pth->dentry;
	kst = rd->args[1];
#else
	den = (struct dentry *)rd->args[1];
	kst = rd->args[2];
#endif

	if (0 == kst)
		return 0;
	size = should_fail(check_equal, fail_size_file, den);
	if (size > 0) {
		kst->blocks += ras_div(size, kst->blksize);
		kst->size += size;
	} else if (should_fail(check_include, fail_null_file, den)) {
		kst->size = 0;
		kst->blocks = 0;
	}
	ret = should_fail(check_include, chfil_missed, den);
	if (ret)
		rasprobe_seturn(regs, ret);

	return 0;
}

static long long chfil_unlink(void *data, const struct ChfilImpl *impl)
{
	if (CHFILETYPE_UNRM == impl->ct || CHFILETYPE_READONLY == impl->ct)
		ras_retn(chfilimpl_geturn(impl));

	return 0;
}
static int rasprobe_handler(security_inode_unlink) (struct rasprobe_instance
			*ri, struct pt_regs *regs) {
	if (0 == regs_return_value(regs)) {
		int ret;
		struct RasRegs *rr = (struct RasRegs *)ri->data;
		struct dentry *den = rr->args[1];

		ret = should_fail(check_include, chfil_unlink, den);
		if (0 != ret)
			rasprobe_seturn(regs, ret);

		file_key_ops_tracer(current, den, "delete");
	}
	return 0;
}
static int rasprobe_handler(security_inode_rmdir) (struct rasprobe_instance *
						   ri, struct pt_regs *regs) {
	return rasprobe_handler(security_inode_unlink) (ri, regs);
}
static long long chfil_create(void *data, const struct ChfilImpl *impl)
{
	if ((CHFILETYPE_UNWRITE == impl->ct) || (CHFILETYPE_UNRW == impl->ct)
	    || (CHFILETYPE_READONLY == impl->ct)) {
		ras_retn(chfilimpl_geturn(impl));
	}
	return 0;
}
static int rasprobe_handler(security_inode_create) (struct rasprobe_instance *
						    ri, struct pt_regs *regs) {
	if (0 == regs_return_value(regs)) {
		int ret;
		struct RasRegs *rr = (struct RasRegs *)ri->data;
		struct dentry *den = rr->args[1];

		file_key_ops_tracer(current, den, "create");
		ret = should_fail(check_include, chfil_create, den);
		if (0 != ret)
			rasprobe_seturn(regs, ret);
	}
	return 0;
}
static int rasprobe_handler(security_inode_mkdir) (struct rasprobe_instance *
						   ri, struct pt_regs *regs) {
	return rasprobe_handler(security_inode_create) (ri, regs);
}

static long long chfil_unwrite(void *data, const struct ChfilImpl *impl)
{
	if (impl->ct == CHFILETYPE_UNRW ||
	    impl->ct == CHFILETYPE_UNWRITE ||
	    impl->ct == CHFILETYPE_DEADLOCK ||
	    impl->ct == CHFILETYPE_FULL || impl->ct == CHFILETYPE_READONLY) {
		return chfilimpl_geturn(impl);
	}
	return 0;
}

static long long chfil_unread(void *data, const struct ChfilImpl *impl)
{
	if (impl->ct == CHFILETYPE_UNRW ||
	    impl->ct == CHFILETYPE_UNREAD || impl->ct == CHFILETYPE_DEADLOCK) {
		return chfilimpl_geturn(impl);
	}
	return 0;
}

static long long chfil_rebuild(void *data, const struct ChfilImpl *impl)
{
	if (impl->ct == CHFILETYPE_ZERO || impl->ct == CHFILETYPE_MISSED ||
	    impl->ct == CHFILETYPE_BROKEN || fail_null_file(data, impl)) {
		memset((void *)impl, 0x00, sizeof(struct ChfilImpl));
		return 1;
	}
	return 0;
}
static int rasprobe_handler(rw_verify_area) (struct rasprobe_instance *ri,
					     struct pt_regs *regs) {
	int ret = 0;
	struct RasRegs *rd = (void *)ri->data;
	struct file *flp = (struct file *)rd->args[1];
	struct dentry *den = FILE_DEN(flp);
	struct super_block *sb = den->d_sb;
	loff_t pos = *(loff_t *)rd->args[2];

	if (regs_return_value(regs) < 0)
		return 0;	/*nothing read return directly */
	if (WRITE == (long)rd->args[0]) {
		unsigned long long size_add =
		    chfil_injected_size(sb, spase_used);
		unsigned long long size_sub =
		    chfil_injected_size(sb, spase_lose);
		unsigned long long size =
		    (ULLMAX - size_add <
		     size_sub) ? ULLMAX : size_add + size_sub;

		ret = should_fail(check_include, chfil_unwrite, den);
		if (0 != ret) {
			rasprobe_seturn(regs, ret);
			return 0;
		}

		if (0 != size && sb->s_op->statfs) {
			struct kstatfs buf;

			memset(&buf, 0, sizeof(buf));
			if (0 == sb->s_op->statfs(sb->s_root, &buf) &&
			    size >= buf.f_bfree * buf.f_bsize) {
				rasprobe_seturn(regs, -ENOSPC);
				return 0;
			}
		}
		if (0 == pos) {/*record write from head*/
			file_key_ops_tracer(current, den, "write");
			/*cancel the borken fault when new written*/
			should_fail(check_equal, chfil_rebuild, den);
		}
	} else if (READ == (long)rd->args[0]) {
		if (0 == pos)/*record read from head*/
			file_key_ops_tracer(current, den, "read");

		ret = should_fail(check_include, chfil_unread, den);
		if (0 != ret) {
			rasprobe_seturn(regs, ret);
			return 0;
		}

		if (should_fail(check_include, fail_null_file, den))
			rasprobe_seturn(regs, 0);
	}

	return 0;
}

static long long chfil_rename(void *data, const struct ChfilImpl *impl)
{
	if (CHFILETYPE_UNRENAME == impl->ct
	    || (CHFILETYPE_READONLY == impl->ct)) {
		ras_retn(chfilimpl_geturn(impl));
	}
	return 0;
}

static int rasprobe_handler(security_inode_rename) (struct rasprobe_instance *
						    ri, struct pt_regs *regs) {
	if (0 == regs_return_value(regs)) {
		int ret;
		struct RasRegs *rr = (struct RasRegs *)ri->data;
		struct dentry *den = rr->args[1];

		ret = should_fail(check_include, chfil_rename, den);
		if (0 != ret)
			rasprobe_seturn(regs, ret);
	}
	return 0;
}

static long long fail_read(void *data, const struct ChfilImpl *ci)
{
	if (CHFILETYPE_BROKEN == ci->ct || CHFILETYPE_ZERO == ci->ct)
		return ci->ct;

	return 0;
}

static int rasprobe_handler(vfs_read) (struct rasprobe_instance *ri,
				       struct pt_regs *regs) {
	int ret = 0;
	int cnt = regs_return_value(regs);
	struct RasRegs *rd = (void *)ri->data;
	struct file *fp = (struct file *)rd->args[0];
	char __user *buf_user = (char __user *)rd->args[1];

	if (cnt <= 0)
		return 0;
	ret = should_fail(check_include, fail_read, FILE_DEN(fp));
	if (CHFILETYPE_BROKEN == ret)
		ras_retn_if(clear_user(buf_user, ras_div(cnt, 2)), 0);
	if (CHFILETYPE_ZERO == ret)
		ras_retn_if(clear_user(buf_user, cnt), 0);
	return 0;
}

static long long fail_umount(void *data, const struct ChfilImpl *impl)
{
	struct vfsmount *mnt = data;

	if (CHFILETYPE_UNUMOUNT == impl->ct && mnt == impl->mnt)
		return chfilimpl_geturn(impl);

	if (CHFILETYPE_HUNMOUNTALL == impl->ct ||
	    (CHFILETYPE_HUNMOUNT == impl->ct && mnt == impl->mnt)) {
		while (impl->ct)
			ras_sleep(20);
	}
	return 0;
}

static int rasprobe_handler(security_sb_umount) (struct rasprobe_instance *ri,
						 struct pt_regs *regs) {
	int ret = 0;
	struct RasRegs *rd = (void *)ri->data;

	ret = should_fail(check_used, fail_umount, rd->args[0]);
	if (0 != ret)
		rasprobe_seturn(regs, ret);

	return 0;
}

static long long fail_mount(void *data, const struct ChfilImpl *impl)
{
	return (CHFILETYPE_UNMOUNT == impl->ct
		&& NULL != impl->path
		&& strcmp((char *)data, impl->path) == 0)
		 ? chfilimpl_geturn(impl):0;
}

static int rasprobe_handler(security_sb_mount) (struct rasprobe_instance *ri,
						struct pt_regs *regs) {
	int ret = 0;
	struct RasRegs *rd = (void *)ri->data;

	ret = should_fail(check_used, fail_mount, rd->args[0]);
	if (ret)
		rasprobe_seturn(regs, ret);

	return 0;
}

static long long fail_deplenish(void *data, const struct ChfilImpl *impl)
{
	return (CHFILETYPE_DEPLENISH == impl->ct) ? 1:0;
}

static long long chfil_unopen(void *data, const struct ChfilImpl *impl)
{
	if (CHFILETYPE_UNOPEN == impl->ct)
		ras_retn(chfilimpl_geturn(impl));

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
static int rasprobe_handler(security_file_open) (struct rasprobe_instance *ri,
						 struct pt_regs *regs)
#else
static int rasprobe_handler(security_dentry_open) (struct rasprobe_instance *ri,
							struct pt_regs *regs)
#endif
{
	int ret = 0;
	struct RasRegs *rd = (void *)ri->data;
	struct file *fp = rd->args[0];

	if (IS_ERR(fp))
		return 0;
	if (should_fail((checker) fail_deplenish,
		fail_deplenish, NULL)) {	/*file handler deplenished */
		rasprobe_seturn(regs, -EMFILE);
		return 0;
	}

	ret = should_fail(check_include, chfil_unopen, FILE_DEN(fp));
	if (0 != ret) {		/*can't open file */
		rasprobe_seturn(regs, ret);
		return 0;
	}

	if (fp->f_mode & FMODE_WRITE) {
		ret = should_fail(check_include, chfil_unwrite, FILE_DEN(fp));
		if (0 != ret) {	/*can't write */
			rasprobe_seturn(regs, ret);
			return 0;
		}
	}

	if ((fp->f_flags & O_CREAT) == 0) {
		ret = should_fail(check_include, chfil_missed, FILE_DEN(fp));
		if (0 != ret) {	/*can't write */
			rasprobe_seturn(regs, ret);
			return 0;
		}
	}
	return 0;
}

static long long chfil_unexec(void *data, const struct ChfilImpl *impl)
{
	if (CHFILETYPE_UNEXEC == impl->ct || CHFILETYPE_READONLY == impl->ct)
		ras_retn(chfilimpl_geturn(impl));

	return 0;
}

static int rasprobe_handler(do_filp_open) (struct rasprobe_instance *ri,
					   struct pt_regs *regs) {
	int ret = 0;
	struct file *fp = (struct file *)regs_return_value(regs);

	if (IS_ERR(fp))
		return 0;
	if (fp->f_flags & FMODE_EXEC) {
		ret = should_fail(check_include, chfil_unexec, FILE_DEN(fp));
		if (0 != ret) {
			fput(fp);
			rasprobe_seturn(regs, ret);
		}
	}
	return 0;
}

int show_name(struct seq_file *m, struct vfsmount *mnt, struct dentry *den)
{
	if (0 == den || 0 == mnt)
		return -1;

	if (den == den->d_parent) {
		if (mnt == MNT_PARENT(mnt)) {
			seq_puts(m, "/");/*for root path*/
			return 0;
		}
		return show_name(m, MNT_PARENT(mnt), MNT_POINT(mnt));
	}

	if (show_name(m, mnt, den->d_parent) == 0)
		seq_printf(m, "%s", den->d_name.name);
	else
		seq_printf(m, "/%s", den->d_name.name);
	return -1;
}

#include <linux/security.h>

static inline int chfile_check(void)
{
	struct inode *dir;
	struct dentry *den = 0;
	int mode = 0;
	struct kstatfs *kfs = 0;
	struct kstat *ks = 0;
	struct file *f = 0;
	/*loff_t *ppos = 0;
	size_t count = 0;*/
	struct path *pth = 0;
	struct vfsmount *mnt = 0;

	security_inode_mkdir(dir, den, mode);
	security_inode_create(dir, den, mode);
	security_inode_rmdir(dir, den);
	security_inode_unlink(dir, den);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	vfs_statfs(den, kfs);
#else
	{
		vfs_statfs(pth, kfs);
	}
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0)
	vfs_getattr(pth, ks);
	mnt = 0;
#else
	vfs_getattr(mnt, den, ks);
#endif
	/*rw_verify_area(mode, f, ppos, count);*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	security_file_open(f, NULL);
#else
	security_dentry_open(f, NULL);
#endif
	return 0;
}
rasprobe_define(security_inode_create);
rasprobe_define(security_inode_rmdir);
rasprobe_define(security_inode_unlink);
rasprobe_define(security_inode_mkdir);
rasprobe_define(vfs_statfs);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
rasprobe_define(vfs_getattr);
#else
rasprobe_define(vfs_getattr_nosec);
#endif
rasprobe_define(rw_verify_area);
rasprobe_define(vfs_read);
rasprobe_define(security_inode_rename);
rasprobe_define(security_sb_umount);
rasprobe_define(security_sb_mount);
rasprobe_define(do_filp_open);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
rasprobe_define(security_file_open);
#else
rasprobe_define(security_dentry_open);
#endif

static struct rasprobe *probes[] = {
	&rasprobe_name(security_inode_create),
	&rasprobe_name(security_inode_rmdir),
	&rasprobe_name(security_inode_unlink),
	&rasprobe_name(security_inode_mkdir),
	&rasprobe_name(vfs_statfs),
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	&rasprobe_name(vfs_getattr),
#else
	&rasprobe_name(vfs_getattr_nosec),
#endif
	&rasprobe_name(rw_verify_area),
	&rasprobe_name(vfs_read),
	&rasprobe_name(security_inode_rename),
	&rasprobe_name(security_sb_umount),
	&rasprobe_name(security_sb_mount),
	&rasprobe_name(do_filp_open),
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	&rasprobe_name(security_file_open),
#else
	&rasprobe_name(security_dentry_open),
#endif
};

static int cmd_main(void *data, int argc, char *args[])
{
	int i, len, cnt;
	char *cmd;
	char *name;
	struct Chfil *chf = data;

	ras_retn_if(2 > argc, -EINVAL);
	cmd = args[0];
	name = args[1];
	ras_retn_if(0 == cmd || 0 == name, -EINVAL);
	len = strlen(cmd);

	for (i = 0, cnt = ARRAY_SIZE(chfil_ops); i < cnt; i++) {
		ras_retn_if(!strncmp(chfil_ops[i].op, cmd, chfil_ops[i].len),
			    chfil_fill(chf, name, chfil_ops[i].val,
				       chfil_ops[i].ct));
	}

	ras_retn_if((3 == len) && (0 == strncmp("log", cmd, len)),
		chfil_log(name));
	ras_retn_if((6 == len) && (0 == strncmp("cancel", cmd, len)),
		chfil_cancel(chf, name));
	ras_retn_if((7 == len) && (0 == strncmp("restore", cmd, len)),
		chfil_restore(chf, name));
	ras_retn(chfil_addsize(chf, cmd, len, name));

}

static int proc_ops_show(rfile) (struct seq_file *m, void *v)
{
	struct ChfilImpl *impl = NULL;
	struct Chfil *chf = m->private;
	int i, len = ARRAY_SIZE(chf->impl);
	int j, olen = ARRAY_SIZE(chfil_ops);

	read_lock(&chf->rwk);
	for (i = 0; i < len; i++) {
		impl = &chf->impl[i];
		if (!check_used(0, impl))
			continue;
		for (j = 0; j < olen; j++) {
			if (impl->ct == chfil_ops[j].ct) {
				seq_printf(m, "%d\t%s\t", i, chfil_ops[j].op);
				j = -1;
				break;
			}
		}
		if (-1 != j)
			seq_printf(m, "%d\t%lld\t", i, impl->size);
		if (impl->path)
			seq_printf(m, "%s", impl->path);
		else if (impl->fp)
			show_name(m, FILE_MNT(impl->fp), FILE_DEN(impl->fp));
		else if (impl->den)	/*never use dentry if fp unholded.*/
			show_name(m, impl->mnt, impl->den);
		seq_puts(m, "\n");
	}
	read_unlock(&chf->rwk);
	if (log_sw.enable) {
		seq_puts(m, "\n Log path :");
		show_name(m, log_sw.mnt, log_sw.den);
		seq_puts(m, "\n");
	}
/*	miss_show(probes, ARRAY_SIZE(probes), m);*/
	return 0;
}

static int proc_ops_open(rfile) (struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rfile), PDE_DATA(inode));
}

static ssize_t proc_ops_write(rfile) (struct file *filp,
				      const char __user *bff, size_t count,
				      loff_t *data) {
	char buf_cmd[256] = { 0 };

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd,
		count, cmd_main, PDE_DATA(FILE_NODE(filp))));
	return count;
}

#define MODULE_NAME "rfile"
proc_ops_define(rfile);
static int rfile_init(void)
{
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	memset(&chfil, 0, sizeof(chfil));
	rwlock_init(&chfil.rwk);
	rasprobe_name(vfs_read).maxactive = 100;
	ras_retn_iferr(register_rasprobes(probes, ARRAY_SIZE(probes)));
	ras_retn_iferr(proc_init(MODULE_NAME, &proc_ops_name(rfile), &chfil));
	return 0;
}
static void rfile_exit(void)
{
	int i, len;

	write_lock(&chfil.rwk);
	for (i = 0, len = ARRAY_SIZE(chfil.impl); i < len; i++)
		chfilimpl_dtor(&chfil.impl[i]);
	write_unlock(&chfil.rwk);
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	proc_exit(MODULE_NAME);
}

module_init(rfile_init);
module_exit(rfile_exit);
MODULE_DESCRIPTION("Change the file system of linux.");
MODULE_LICENSE("GPL");
#ifndef RASFIRE_VERSION
#define RASFIRE_VERSION "V001R001C151-"
#endif
MODULE_VERSION(RASFIRE_VERSION "1.4");
