/*
 * Copyright 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Authors:
 *   Alexander Yashchenko <a.yashchenko@samsung.com>
 *   Sergei Rogachev <s.rogachev@samsung.com>
 *
 * This file is part of GMC (graphical memory compression) framework.
 *
 * GMC is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Midgard GMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMC. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gmc.c - implementation of generic interface for communication with user
 * space daemon that can be used to implement 'native' memory compression
 * facilities in GPU kernel driver.
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/threads.h>
#include <linux/gmc_storage.h>
#include <linux/gmc.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include "securec.h"

#define GMC_DEBUGFS_BUFMAX 1024
#define GMC_STORAGE_COUNT	128

static atomic_t gmc_device_number = ATOMIC_INIT(0);
static struct proc_dir_entry *gmc_root_dentry = NULL;

struct gmc_private
{
	long 	pid;
	gmc_op 	op;
	long		compress_size;
	struct gmc_device *device;
	struct work_struct gmc_work;
};

static int gmc_alloc_device_number(void)
{
	return atomic_add_return(1, &gmc_device_number) - 1;
}

static int read_pid(const char __user *ubuf, size_t len, loff_t *offp, long *pid, long *compress_size)
{
	char *buf = NULL;
	char *p = NULL;
	/*
	 * The variable pid here is signed long to satisfy requirements of
	 * the function kstrtol(). pid_t is defined in the kernel as int, thus
	 * the variable pid here has a higher value domain.
	 */
	long pid_value = 0,value = 0;
	int ret = 0;

	if(len == 0 || len > PAGE_SIZE)
		return -EINVAL;

	buf = kmalloc(len + 1, GFP_KERNEL);
	if(buf == NULL)
		return -ENOMEM;

	if ((ubuf != NULL) && copy_from_user(buf, ubuf, len)){
		ret = -EFAULT;
		goto out;
	}

	buf[len] = '\0';

	p = strstr(buf, " ");
	if(p != NULL){
		*p = '\0';
		p++;
	}

	if (kstrtol(&buf[0], 10, &pid_value) != 0){
		ret = -EINVAL;
		goto out;
	}

	if ((pid_value < 0) || (pid_value > PID_MAX_DEFAULT)){
		ret = -EINVAL;
		goto out;
	}

	*pid = pid_value;

	if(p && kstrtol(p, 10, &value) != 0)
		value = 0;

	if(compress_size != NULL){
		*compress_size = value;
		if(value < 0)
			*compress_size = 0;
	}

out:
	kfree(buf);
	return ret;
}

static int gmc_decompress_pid(struct gmc_device *device, unsigned long pid)
{
	if (!device->ops) {
		pr_info("gmc operations have not been registered\n");
		return -EINVAL;
	}

	if (!device->ops->decompress_kctx)
		return -EINVAL;

	return device->ops->decompress_kctx(pid, device);
}

static int gmc_compress_pid(struct gmc_device *device, unsigned long pid,long compress_size)
{
	if (!device->ops) {
		pr_info("gmc operations have not been registered\n");
		return -EINVAL;
	}

	if (!device->ops->compress_kctx)
		return -EINVAL;

	return device->ops->compress_kctx(pid, device,compress_size);
}

void gmc_work(struct work_struct *work)
{
	int ret = 0;
	struct gmc_private *gmc_private = container_of(work, struct gmc_private, gmc_work);
	if(GMC_COMPRESS == gmc_private->op)
		ret = gmc_compress_pid(gmc_private->device,(pid_t)gmc_private->pid,gmc_private->compress_size);
	else if(GMC_DECOMPRESS == gmc_private->op)
		ret = gmc_decompress_pid(gmc_private->device,(pid_t)gmc_private->pid);

	if(ret)
		printk(KERN_ERR"gmc_work op %d on pid %ld return error number %d \n",gmc_private->op,gmc_private->pid,ret);

	kfree(gmc_private);
}

static ssize_t gmc_compress_write(struct file *file, const char __user *ubuf,
	size_t len, loff_t *offp)
{
	struct gmc_device *device = (struct gmc_device *)(PDE_DATA(file->f_inode));
	int ret;
	long pid = 0,compress_size=0;
	struct gmc_private *gmc_work_data = NULL;

	if (device == NULL)
		return -EPERM;

	ret = read_pid(ubuf, len, offp,&pid,&compress_size);
	if (IS_ERR_VALUE(ret))
		return ret;

	gmc_work_data = kzalloc(sizeof(*gmc_work_data), GFP_KERNEL);
	if(gmc_work_data == NULL)
		return -ENOMEM;

	gmc_work_data->pid = pid;
	gmc_work_data->compress_size = compress_size;
	gmc_work_data->op = GMC_COMPRESS;
	gmc_work_data->device = device;
	INIT_WORK(&gmc_work_data->gmc_work, gmc_work);

	queue_work(system_unbound_wq, &gmc_work_data->gmc_work);

	return len;//lint !e429
}

static ssize_t gmc_decompress_write(struct file *file, const char __user *ubuf,
	size_t len, loff_t *offp)
{
	struct gmc_device *device = (struct gmc_device *)(PDE_DATA(file->f_inode));
	int ret;
	long pid = 0;
	struct gmc_private *gmc_work_data = NULL;

	if (device == NULL)
		return -EPERM;

	ret = read_pid(ubuf, len, offp,&pid, NULL);
	if (IS_ERR_VALUE(ret))
		return ret;

	gmc_work_data = kzalloc(sizeof(*gmc_work_data), GFP_KERNEL);
	if(gmc_work_data == NULL)
		return -ENOMEM;

	gmc_work_data->pid = pid;
	gmc_work_data->compress_size = 0;
	gmc_work_data->op = GMC_DECOMPRESS;
	gmc_work_data->device = device;
	INIT_WORK(&gmc_work_data->gmc_work, gmc_work);

	queue_work(system_unbound_wq, &gmc_work_data->gmc_work);

	return len;//lint !e429
}

#ifdef GPU_GMC_DEBUG
int gmc_meminfo_open(struct inode *in, struct file *file)
{
	struct gmc_device *device = (struct gmc_device *)(PDE_DATA(file->f_inode));
	if (device == NULL)
		return -EPERM;

	if (!device->ops->compress_kctx)
		return -EINVAL;

	return device->ops->meminfo_open(in, file);
}
#endif

static ssize_t gmc_storage_stat_read(struct file *file, char __user *ubuf,
				size_t len, loff_t *offp)
{
	ssize_t ret;
	ssize_t out_offset;
	const ssize_t out_count = GMC_DEBUGFS_BUFMAX;
	struct gmc_device *device = (struct gmc_device *)(PDE_DATA(file->f_inode));
	char *buf = NULL;

	if (device == NULL)
		return -EPERM;

	buf = kmalloc(out_count, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	out_offset = 0;
	out_offset += snprintf_s(buf + out_offset, out_count, GMC_STORAGE_COUNT,
			"ComprSize:    %10llu kB\n",
			(unsigned long long) atomic64_read(
				&device->storage->stat.compr_data_size) / 1024);
	out_offset += snprintf_s(buf + out_offset, out_count - out_offset, GMC_STORAGE_COUNT,
			"StoredSize:   %10llu kB\n",
			(unsigned long long) atomic64_read(
				&device->storage->stat.nr_pages) * 4);
	out_offset += snprintf_s(buf + out_offset, out_count - out_offset, GMC_STORAGE_COUNT,
			"ZeroedSize:   %10llu kB\n",
			(unsigned long long) atomic64_read(
				&device->storage->stat.nr_zero_pages) * 4);

	/*
	 * In case of an error, the following function returns a negative error
	 * code which is propagated upwards the call stack, thus the read system
	 * call will return a positive number of copied symbols or an error.
	 */
	ret = simple_read_from_buffer(ubuf, len, offp, buf, out_offset);
	kfree(buf);

	return ret;
}

static const struct file_operations gmc_compress_fops = {
	.open = simple_open,
        .llseek = no_llseek,
	.write = gmc_compress_write
};

static const struct file_operations gmc_decompress_fops = {
	.open = simple_open,
        .llseek = no_llseek,
	.write = gmc_decompress_write
};

#ifdef GPU_GMC_DEBUG
static const struct file_operations gmc_storage_stat_fops = {
	.open = simple_open,
	.read = gmc_storage_stat_read,
	.llseek = no_llseek
};

static const struct file_operations gmc_memory_info_fops = {
       .open = gmc_meminfo_open,
       .read = seq_read,
       .llseek = seq_lseek,
       .release = single_release,
};
#endif


static int gmc_fs_init(void)
{
	gmc_root_dentry = proc_mkdir("gmc", NULL);
	if (gmc_root_dentry == NULL) {
		pr_err("Unable to create gmc proc directory\n");
		return -EINVAL;
	}

	return 0;
}

/*
 * 6 characters of the "device" word, maximum 10 digits in unsigned int, zero
 * symbol and padding up to 32 bytes.
 */
#define GMC_DIRNAME_LENGTH 32

/**
 * gmc_register_device() - register a new graphical device in the GMC subsystem.
 *
 * @gmc_operations: a pointer to the gmc_operations structure provided by the
 * user of the GMC subsystem.
 * @gmc_device: a pointer to generic gmc device, built in platform device
 * Returns 0 on success and error code if something is wrong.
 */
int gmc_register_device(struct gmc_ops *gmc_operations, struct gmc_device *device)
{
	char dirname[GMC_DIRNAME_LENGTH] = {0};
	struct gmc_storage *storage;
	struct proc_dir_entry *device_dir_dentry;

	int id,i,array_size = 0;
	int err = -EINVAL;
	int ret = -1;

	/*
	 * This data structure describes files associated with some particular
	 * device registered in the GMC subsystem. This data is used by a
	 * generic piece of code below to create necessary debugfs dentries in
	 * the GMC file hierarchy.
	 */
	struct {
		const char                   *name;
		const struct file_operations *fops_p;
	} files[] = {
		{"compress",     &gmc_compress_fops},
		{"decompress",   &gmc_decompress_fops},
#if GPU_GMC_DEBUG
		{"storage_stat", &gmc_storage_stat_fops},
		{"memory_info", &gmc_memory_info_fops},
#endif
	};

	BUILD_BUG_ON(ARRAY_SIZE(files) > GMC_FS_MAX_DENTRIES);

	id = gmc_alloc_device_number();
	ret = snprintf_s(dirname, GMC_DIRNAME_LENGTH,  GMC_DIRNAME_LENGTH - 1, "device%d", id);
	if (ret == -1) {
		pr_err("GMC register device snprintf_s failed\n");
		return -EINVAL;
	}
	dirname[GMC_DIRNAME_LENGTH - 1] = '\0';

	storage = gmc_storage_create();
	if (storage == NULL) {
		pr_err("Unable to create a storage for the device.\n");
		err = -ENOMEM;
		goto error_out;
	}

	device->storage = storage;
	device->ops = gmc_operations;

	/* Lazy creation of the root GMC dentry. */
	if (gmc_root_dentry == NULL) {
		if (gmc_fs_init())
			goto error_destroy_storage;
	}

	device_dir_dentry = proc_mkdir(dirname, gmc_root_dentry);
	if (device_dir_dentry == NULL) {
		pr_err("Unable to create gmc device proc directory\n");
		goto error_cleanup_debugfs;
	}

	/*
	 * Create necessary dentries, pass a pointer to the device
	 * structure to initialize the private fields of the corresponding
	 * inodes.
	 */
	array_size = ARRAY_SIZE(files);
	for (i = 0; i < array_size; i++) {
		struct proc_dir_entry *dir_entry = proc_create_data(files[i].name,S_IWUSR|S_IRUSR|S_IWGRP,device_dir_dentry,files[i].fops_p, device);
		if (dir_entry == NULL) {
			pr_err("Unable to create %s file\n", files[i].name);
			goto error_cleanup_debugfs;
		}

		device->fs.dentries[i] = dir_entry;
	}

	return 0;

error_cleanup_debugfs:
	for (i = 0; i < array_size; i++)
		remove_proc_entry(files[i].name,device->fs.dentries[i]);
	proc_remove(device_dir_dentry);
	proc_remove(gmc_root_dentry);
error_destroy_storage:
	gmc_storage_destroy(storage);
error_out:
	return err;
}
EXPORT_SYMBOL(gmc_register_device);

MODULE_AUTHOR("Sergei Rogachev <s.rogachev@samsung.com>");
MODULE_AUTHOR("Yashchenko Alexander <a.yashchenko@samsung.com>");
MODULE_DESCRIPTION("GPU memory compression infrastructure");
MODULE_LICENSE("GPL");
