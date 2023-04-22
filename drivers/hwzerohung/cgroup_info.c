#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cgroup.h>
#include <linux/kernfs.h>
#include <linux/kobject.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

static DEFINE_MUTEX(cgroup_info_mutex);

#define FREEZER_PATH "/dev/frz"
#define TASKS_NAME "tasks"
#define STATUS_NAME "freezer.state"

#define STORE_BUFFER_SIZE_MIN 2
#define STORE_BUFFER_SIZE_MAX 100

#define FILE_BUFFER_SIZE 128
#define STATUS_BUFFER_SIZE FILE_BUFFER_SIZE
#define TASK_BUFFER_SIZE 1
#define TASK_PID_MAX_SIZE 8
#define CHECK_STATUS(src, dst) strncmp(src, #dst, strlen(#dst))

#define BUFFER_SIZE     1024

static char file_buf[FILE_BUFFER_SIZE];

enum MSG_TYPE {
	TO_USER = 0,
	TO_MODULE,
};

static ssize_t info_print_wrapper(int type, char **pbuf, const char *fmt, ...)
{
	ssize_t size = 0;
	va_list args;

	va_start(args, fmt);
	if (type == TO_USER) {
		size = vsprintf(*pbuf, fmt, args);
		*pbuf += size;
	} else if (type == TO_MODULE) {
		vprintk_emit(0, LOGLEVEL_ERR, NULL, 0, fmt, args);
	}
	va_end(args);
	return size;
}

enum FREEZER_STATUS {
	THAWED = 0,
	FREEZING,
	FROZEN,
	DEFAULT,
};

struct task_node {
	struct list_head list;
	pid_t pid;
};

struct cgroup_info_node {
	struct list_head list;
	char *path;
	char *name;
	int status;
	int task_count;
	struct list_head task_list;
	/* list for directory traverse */
	struct list_head queue_list;
};

static int cgroup_info_list_size;
static struct list_head cgroup_info_list;

static void task_list_reset(struct list_head *src)
{
	if (NULL == src)
		return;
	struct task_node *tcur = NULL;
	struct task_node *ttmp = NULL;

	list_for_each_entry_safe(tcur, ttmp, src, list) {
		list_del(&tcur->list);
		kfree(tcur);
	}
}

static void cgroup_info_list_reset(struct list_head *src)
{
	if (NULL == src)
		return;
	struct cgroup_info_node *ccur = NULL;
	struct cgroup_info_node *ctmp = NULL;

	cgroup_info_list_size = 0;

	list_for_each_entry_safe(ccur, ctmp, src, list) {
		printk("[cgroup info] visit one node.\n");
		if (ccur == NULL) {
			return;
		}
		list_del(&ccur->list);
		if (ccur->name != NULL)
			kfree(ccur->name);
		if (ccur->path != NULL)
			kfree(ccur->path);
		task_list_reset(&ccur->task_list);
		kfree(ccur);
	}
	return;
}

static struct file *file_open(const char *path, int flags, int rights)
{
	mm_segment_t oldfs;
	struct file *filp = NULL;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp))
		return NULL;
	return filp;
}

static void file_close(struct file *file)
{
	if (file)
		filp_close(file, NULL);
}

static int file_read(struct file *file, unsigned char *data, unsigned int size, unsigned long long pos)
{
	mm_segment_t oldfs;
	int ret = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(file, data, size, &pos);
	set_fs(oldfs);
	return ret;
}

static char *file_fullname_generate(const char *path, const char *name)
{
	if (!path || !name)
		return NULL;
	size_t path_len = strlen(path);
	size_t name_len = strlen(name);
	size_t fullname_len = path_len + name_len + 2;

	if (fullname_len > PATH_MAX)
		return NULL;
	char *fullname = kzalloc(fullname_len, GFP_KERNEL);

	if (!fullname)
		return NULL;
	snprintf(fullname, fullname_len, "%s/%s", path, name);
	return fullname;
}

struct cgroup_info_readdir_callback {
	struct dir_context ctx;
	int result;
	unsigned is_dir;
};

static int cgroup_info_filldir(struct dir_context *ctx, const char *name, int namelen,
			loff_t offset, u64 ino, unsigned d_type)
{
	struct cgroup_info_readdir_callback *buf =
		container_of(ctx, struct cgroup_info_readdir_callback, ctx);

	if (buf->result)
		return -EINVAL;

	int name_buf_len = namelen + 1;

	if (name_buf_len > FILE_BUFFER_SIZE) {
		buf->result = -ENOMEM;
		return -ENOMEM;
	}
	buf->result++;
	buf->is_dir = d_type;
	strncpy(file_buf, name, name_buf_len);

	return 0;
}

static int file_read_dir(struct file *file, int *is_dir)
{
	int ret = 0;

	struct cgroup_info_readdir_callback buf = {
		.ctx.actor = cgroup_info_filldir,
	};
	if (!file)
		return -EBADF;
	ret = iterate_dir(file, &buf.ctx);
	if (buf.result)
		ret = buf.result;
	*is_dir = buf.is_dir;
	return ret;
}



static ssize_t tasks_read_and_save(struct cgroup_info_node *n, struct file *file)
{
	ssize_t ret = 0;
	ssize_t total = 0;
	int i = 0;
	int task_pid_index = 0;
	char task_pid[TASK_PID_MAX_SIZE + 1];
    char* buffer = (char *)kzalloc(BUFFER_SIZE, GFP_KERNEL);
	struct task_node *ttmp;
	if (NULL == buffer) {
	  return -ENOMEM;
	}
	total = file_read(file, buffer, BUFFER_SIZE, 0);
	if (total < 0) {
		kfree(buffer);
		return -ENOMEM;
	}

	for (; i < total; i++) {
		if (buffer[i] != '\n') {
			//get each pid
		  	task_pid[task_pid_index] = buffer[i];
			task_pid_index++;
			if (task_pid_index > TASK_PID_MAX_SIZE) {
				kfree(buffer);
				return -ENOMEM;
			}
		} else {
			//push pid to task list
			ttmp = (struct task_node *)kzalloc(sizeof(struct task_node), GFP_KERNEL);
			task_pid[task_pid_index] = '\0';
			if (NULL == ttmp) {
				kfree(buffer);
				return -ENOMEM;
			}

			ret = sscanf(task_pid, "%d\0", &(ttmp->pid));
			if (ret < 0) {
				kfree(ttmp);
				kfree(buffer);
				return -ENOMEM;
			}
			
		    list_add_tail(&ttmp->list, &n->task_list);
		    n->task_count++;
			task_pid_index = 0;
		}
	}
	kfree(buffer);
	return total;
}

static ssize_t status_read_and_save(struct cgroup_info_node *n, struct file *file)
{
	ssize_t ret = 0;

	ret = file_read(file, file_buf, STATUS_BUFFER_SIZE, ret);
	if (ret > 0) {
		if (!CHECK_STATUS(file_buf, THAWED))
			n->status = THAWED;
		else if (!CHECK_STATUS(file_buf, FREEZING))
			n->status = FREEZING;
		else if (!CHECK_STATUS(file_buf, FROZEN))
			n->status = FROZEN;
		else
			n->status = DEFAULT;
	}
	return ret;
}

static ssize_t cgroup_info_read_and_save(struct cgroup_info_node *n, const char *file_name)
{
	ssize_t ret = 0;
	char *fullname = NULL;
	struct file *file = NULL;

	if (!n || !file_name) {
		ret = -EINVAL;
		goto out;
	}
	fullname = file_fullname_generate(n->path, file_name);
	if (!fullname) {
		ret = -ENOMEM;
		goto out;
	}
	file = file_open(fullname, O_RDONLY, NULL);
	if (!file) {
		ret = -ENOENT;
		goto out;
	}
	if (!strcmp(file_name, TASKS_NAME)) {
		ret = tasks_read_and_save(n, file);
		if (ret < 0)
			goto out;
	} else if (!strcmp(file_name, STATUS_NAME)) {
		ret = status_read_and_save(n, file);
		if (ret < 0)
			goto out;
	} else {
		ret = -EINVAL;
	}
out:
	if (file)
		file_close(file);
	if (fullname)
		kfree(fullname);
	if (ret < 0 && n)
		task_list_reset(&n->task_list);
	return ret;
}

static int cgroup_info_node_init(struct cgroup_info_node *n,
			const char *path_buf, const char *name_buf)
{
	int ret = 0;

	if (!n || !path_buf || !name_buf) {
		ret = -EINVAL;
		goto out;
	}
	size_t name_buf_len = strlen(name_buf) + 1;

	n->name = kzalloc(name_buf_len, GFP_KERNEL);
	if (!n->name) {
		ret = -ENOMEM;
		goto out;
	}
	strncpy(n->name, name_buf, name_buf_len);

	n->path = file_fullname_generate(path_buf, name_buf);
	if (!n->path) {
		ret = -ENOMEM;
		goto out;
	}
	n->status = DEFAULT;
	n->task_count = 0;
	INIT_LIST_HEAD(&n->task_list);
	cgroup_info_list_size++;
out:
	return ret;
}

static void free_cgroup_info_node(struct cgroup_info_node* node) {
	if (NULL == node) {
		return;
	}

	if (NULL != node->path) {
		kfree(node->path);
	}

	if (NULL != node->name) {
		kfree(node->name);
	}
    task_list_reset(&node->task_list);
	kfree(node);
}

static int cgroup_info_get(void)
{
	int ret = -1;
	struct cgroup_subsys *ss = NULL;

	INIT_LIST_HEAD(&cgroup_info_list);
	if (cgroup_subsys_enabled(freezer_cgrp_subsys)) {
		char *path_buf = NULL;
		char *file = NULL;
		struct list_head cgroup_info_queue;
		size_t cgroup_info_queue_size = 0;
		unsigned is_dir = 0;
		struct cgroup_info_node *node = NULL;

		path_buf = kzalloc(PATH_MAX, GFP_KERNEL);
		if (!path_buf) {
			ret = -ENOMEM;
			goto out;
		}
		strcpy(path_buf, FREEZER_PATH);
		INIT_LIST_HEAD(&cgroup_info_queue);
		file = file_open(path_buf, O_RDONLY, NULL);
		if (!file) {
			ret = -ENOENT;
			goto out;
		}
		do {
			if (cgroup_info_queue_size > 0) {
				node = list_first_entry(&cgroup_info_queue, struct cgroup_info_node, queue_list);
				strcpy(path_buf, node->path);
				file_close(file);
				file = file_open(path_buf, O_RDONLY, NULL);
				if (!file) {
					ret = -ENOENT;
					goto out;
				}
				list_del(&node->queue_list);
				cgroup_info_queue_size--;
			}
			while ((ret = file_read_dir(file, &is_dir)) > 0) {
				if (is_dir == DT_DIR) {
					if (!strcmp(file_buf, ".") || !strcmp(file_buf, ".."))
						continue;
					node = kzalloc(sizeof(struct cgroup_info_node), GFP_KERNEL);
					if (!node) {
						ret = -ENOMEM;
						goto out;
					}
					ret = cgroup_info_node_init(node, path_buf, file_buf);
					if (ret < 0) {
						free_cgroup_info_node(node);
						goto out;
					}
					ret = cgroup_info_read_and_save(node, STATUS_NAME);
					if (ret < 0) {
						free_cgroup_info_node(node);
						goto out;
					}
					ret = cgroup_info_read_and_save(node, TASKS_NAME);
					if (ret < 0) {
						free_cgroup_info_node(node);
						goto out;
					}
					list_add_tail(&node->queue_list, &cgroup_info_queue);
					list_add_tail(&node->list, &cgroup_info_list);
					cgroup_info_queue_size++;
				}
			}
			if (ret < 0){
				goto out;	
			}
		} while (cgroup_info_queue_size > 0);
out:
		file_close(file);
		if (ret < 0)
			cgroup_info_list_reset(&cgroup_info_list);
		if (path_buf != NULL)
			kfree(path_buf);
	} else {
		ret = -ENODEV;
	}
	return ret;
}

static int freezer_info_print_messages(int type, char **pbuf)
{
	int ret = 0;

	mutex_lock(&cgroup_info_mutex);
	ret = cgroup_info_get();
	if (ret < 0) {
		info_print_wrapper(type, pbuf, "cgroup_info: can not get correct file information.\n");
		goto out;
	}
	info_print_wrapper(type, pbuf, "cgroup_info: total group number %d\n", cgroup_info_list_size);
	struct cgroup_info_node *ccur = NULL;

	info_print_wrapper(type, pbuf, "cgroup_info: non-empty groups: ");
	list_for_each_entry(ccur, &cgroup_info_list, list) {
		if (ccur->task_count > 0) {
			info_print_wrapper(type, pbuf, "%s[%d][ ", ccur->name, ccur->status);
			struct task_node *tcur = NULL;

			list_for_each_entry(tcur, &ccur->task_list, list) {
				info_print_wrapper(type, pbuf, "%d ", tcur->pid);
			}
			info_print_wrapper(type, pbuf, "]");
		}
	}
	info_print_wrapper(type, pbuf, "\n");
out:
	cgroup_info_list_reset(&cgroup_info_list);
	if (ret < 0)
		info_print_wrapper(type, pbuf, "cgroup_info: failed.\n");
	mutex_unlock(&cgroup_info_mutex);
	return ret;
}

/* to hungtask */
int freezer_info_show_messages(void)
{
	char *buf = NULL;

	return freezer_info_print_messages(TO_MODULE, buf);
}

static ssize_t freezer_info_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	const char *start = buf;

	freezer_info_print_messages(TO_USER, &buf);
	ssize_t size = (ssize_t) (buf - start);

	return size;
}

static ssize_t freezer_info_store(struct kobject *kobj,
				 struct kobj_attribute *attr, const char *buf,
				 size_t n)
{
	if (n < STORE_BUFFER_SIZE_MIN || n > STORE_BUFFER_SIZE_MAX)
		return -EINVAL;
	if (!buf)
		return -EINVAL;
	return (ssize_t) n;
}

/* freezer_subsys */
static struct kobj_attribute freezer_info = {
	.attr = {
		 .name = "freezer_info",
		 .mode = 0640,
		 },
	.show = freezer_info_show,
	.store = freezer_info_store,
};

static struct attribute *attrs[] = {
	&freezer_info.attr,
	NULL,
};

static struct attribute_group cgroup_info_attr_group = {
	.attrs = attrs,
};

struct kobject *cgroup_info_kobj;
int create_sysfs_cgroup_info(void)
{
	int ret = 0;

	while (kernel_kobj == NULL)
		msleep(1000);
	/*Create kobject cgroup_info at /sys/kernel/cgroup_info */
	cgroup_info_kobj = kobject_create_and_add("cgroup_info", kernel_kobj);
	if (!cgroup_info_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(cgroup_info_kobj, &cgroup_info_attr_group);
	if (ret)
		kobject_put(cgroup_info_kobj);
	return ret;
}

static int __init cgroup_info_init(void)
{
	int ret = 0;

	INIT_LIST_HEAD(&cgroup_info_list);
	ret = create_sysfs_cgroup_info();
	if (ret)
		pr_err("cgroup_info: create cgroup_info_init fail\n");
	return 0;
}
subsys_initcall(cgroup_info_init);