#include<linux/kernel.h>
#include<linux/module.h>
#include<huawei_platform/log/log_exception.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/aio.h>
#include<linux/types.h>
#include<linux/version.h>
#include<uapi/linux/uio.h>

#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG	log_exception
HWLOG_REGIST();

static int CHECK_CODE = 0x7BCDABCD;

/**
*  tag: the tag of this command
*  msg: concrete command string to write to /dev/log/exception
*  return: on success return the bytes writed successfully, on error return <0
*
*/
int log_to_exception(char* tag, char* msg)
{
	mm_segment_t oldfs;
	struct file *filp;
	unsigned char prio_err = 6;	//ANDROID_ERROR
	int ret = 0;
	struct iovec vec[5];
	unsigned long vcount = 0;

	if (NULL == tag || NULL == msg) {
		hwlog_err("%s: arguments invalidate\n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s: exception tag '%s' msg '%s'", __func__, tag, msg);

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(LOG_EXCEPTION_FS, O_RDWR, 0);

	if (!filp || IS_ERR(filp)) {
		hwlog_err("%s: access '%s' fail", __func__, LOG_EXCEPTION_FS);
		set_fs(oldfs);
		return -ENODEV;
	}

	vcount = 0;
	vec[vcount].iov_base = &CHECK_CODE;
	vec[vcount++].iov_len  = sizeof(CHECK_CODE);
	vec[vcount].iov_base = &prio_err;
	vec[vcount++].iov_len = 1;
	vec[vcount].iov_base = tag;
	vec[vcount++].iov_len = strlen(tag)+1;
	vec[vcount].iov_base = msg;
	vec[vcount++].iov_len = strlen(msg)+1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	ret = vfs_writev(filp, vec, vcount, &filp->f_pos, 0);
#else
	ret = vfs_writev(filp, vec, vcount, &filp->f_pos);
#endif
	if (ret < 0) {
		hwlog_err("%s: write '%s' fail %d\n", __func__, LOG_EXCEPTION_FS, ret);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -EIO;
	}

	filp_close(filp, NULL);
	set_fs(oldfs);

	return ret;
}
EXPORT_SYMBOL(log_to_exception);

int logbuf_to_exception(char category, int level, char log_type, char sn, void *msg, int msglen)
{
	mm_segment_t oldfs;
	struct file *filp;
	int ret = 0;
	struct idapheader idaphdr;
	struct iovec vec[5];
	unsigned long vcount = 0;

	if (NULL == msg || msglen < 0) {
		hwlog_err("%s: arguments invalidate\n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s: exception msg '%s'", __func__, (char *)msg);

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(LOG_EXCEPTION_FS, O_RDWR, 0);

	if (!filp || IS_ERR(filp)) {
		hwlog_err("%s: access '%s' fail", __func__, LOG_EXCEPTION_FS);
		set_fs(oldfs);
		return -ENODEV;
	}

	idaphdr.level = level;
	idaphdr.category = category;
	idaphdr.log_type = log_type;
	idaphdr.sn = sn;
	vcount = 0;
	vec[vcount].iov_base = &CHECK_CODE;
	vec[vcount++].iov_len  = sizeof(CHECK_CODE);
	vec[vcount].iov_base = (void*)&idaphdr;;
	vec[vcount++].iov_len = sizeof(idaphdr);
	vec[vcount].iov_base = msg;
	vec[vcount++].iov_len = msglen;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	ret = vfs_writev(filp, vec, vcount, &filp->f_pos, 0);
#else
	ret = vfs_writev(filp, vec, vcount, &filp->f_pos);
#endif
	if (ret < 0) {
		hwlog_err("%s: write '%s' fail %d\n", __func__, LOG_EXCEPTION_FS, ret);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -EIO;
	}

	filp_close(filp, NULL);
	set_fs(oldfs);

	return ret;
}
EXPORT_SYMBOL(logbuf_to_exception);

