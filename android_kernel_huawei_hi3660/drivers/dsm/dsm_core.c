

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/log/hw_log.h>
#include "dsm_core.h"

/* int debug_output = 1; */
#define HWLOG_TAG	DSM
HWLOG_REGIST();

#define DSM_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define DSM_LOG_ERR(x...)		({\
						if(printk_ratelimit()) \
							_hwlog_err(HWLOG_TAG, ##x);\
					})
#define DSM_LOG_DEBUG(x...)	_hwlog_debug(HWLOG_TAG, ##x)
#ifndef DSM_MINOR
#define DSM_MINOR		254    /* DSM */
#endif

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static struct dsm_msgq g_dsm_msgq;
#endif

static struct dsm_server g_dsm_server;
static struct work_struct dsm_work;

/*
*FUNCTION: dsm_update_client_vendor_info
*
*DESCRIPTION:
*	This API is used to update the variables 'ic_name' & 'module_name' of the struct dsm_client
*
*RETRUN:
*	SUCCESS:	0
*	FAILED:		-1
*/
int dsm_update_client_vendor_info(struct dsm_dev *dev)
{
	int client_num = 0;
	int is_found = -1;
	int retval = -1;

	if (dev == NULL || dev->name == NULL) {
		DSM_LOG_ERR("dev or dev->name is NULL\n");
		goto out;
	}

	mutex_lock(&g_dsm_server.mtx_lock);
	for(client_num = 0; client_num < g_dsm_server.client_count; client_num++) {
		is_found = strncmp(g_dsm_server.client_list[client_num]->client_name, dev->name, CLIENT_NAME_LEN);
		if(!is_found) {
			if(dev->ic_name && strncmp(g_dsm_server.client_list[client_num]->ic_name,
					dev->ic_name, DSM_MAX_IC_NAME_LEN)) {
				strncpy(g_dsm_server.client_list[client_num]->ic_name, dev->ic_name, DSM_MAX_IC_NAME_LEN-1);
				g_dsm_server.client_list[client_num]->ic_name[DSM_MAX_IC_NAME_LEN-1] = '\0';
			}
			if(dev->module_name && strncmp(g_dsm_server.client_list[client_num]->module_name,
					dev->module_name, DSM_MAX_MODULE_NAME_LEN)) {
				strncpy(g_dsm_server.client_list[client_num]->module_name, dev->module_name, DSM_MAX_MODULE_NAME_LEN-1);
				g_dsm_server.client_list[client_num]->module_name[DSM_MAX_MODULE_NAME_LEN-1] = '\0';
			}
			retval = 0;
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
out:
	return retval;
}

struct dsm_client *dsm_register_client(struct dsm_dev *dev)
{
	int i;
	int conflict = -1;
	struct dsm_client *ptr = NULL;

	if (g_dsm_server.server_state != DSM_SERVER_INITED) {
		DSM_LOG_ERR("dsm server uninited\n");
		goto out;
	}

	if (dev == NULL || dev->buff_size > DSM_EXTERN_CLIENT_MAX_BUF_SIZE) {
		DSM_LOG_ERR("dsm_dev is NULL or buffer size is too big\n");
		goto out;
	}

	smp_rmb();
	mutex_lock(&g_dsm_server.mtx_lock);
	if (g_dsm_server.client_count < CLIENT_SIZE) {
		ptr = (struct dsm_client *)vzalloc((sizeof(struct dsm_client)+dev->buff_size));
		if (!ptr) {
			mutex_unlock(&g_dsm_server.mtx_lock);
			DSM_LOG_ERR("clients malloc failed\n");
			goto out;
		}

		for (i = 0; i < CLIENT_SIZE; i++) {
			if (!test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]))
				break;
			if (dev->name) {
				conflict = strncmp(g_dsm_server.client_list[i]->client_name, dev->name,
					CLIENT_NAME_LEN);
				if (!conflict) {
					DSM_LOG_ERR("new client %s conflict with No.%d client %s\n",
						dev->name, i, g_dsm_server.client_list[i]->client_name);
					break;
				}
			} else {
				mutex_unlock(&g_dsm_server.mtx_lock);
				DSM_LOG_ERR("Please specify the dsm device name!\n");
				vfree(ptr);
				ptr = NULL;
				goto out;
			}
		}

		if (i < CLIENT_SIZE && conflict) {
			strncpy(ptr->client_name, dev->name, CLIENT_NAME_LEN-1);
			ptr->client_name[CLIENT_NAME_LEN-1] = '\0';
			if (dev->device_name) {
				strncpy(ptr->device_name, dev->device_name, DSM_MAX_DEVICE_NAME_LEN-1);
				ptr->device_name[DSM_MAX_DEVICE_NAME_LEN-1] = '\0';
			}
			if (dev->ic_name) {
				strncpy(ptr->ic_name, dev->ic_name, DSM_MAX_IC_NAME_LEN-1);
				ptr->ic_name[DSM_MAX_IC_NAME_LEN-1] = '\0';
			}
			if (dev->module_name) {
				strncpy(ptr->module_name, dev->module_name, DSM_MAX_MODULE_NAME_LEN-1);
				ptr->module_name[DSM_MAX_MODULE_NAME_LEN-1] = '\0';
			}

			ptr->client_id = i;
			ptr->cops = dev->fops;
			ptr->buff_size = dev->buff_size;
			init_waitqueue_head(&ptr->waitq);
			g_dsm_server.client_list[i] = ptr;
			set_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]);
			g_dsm_server.client_count++;
			DSM_LOG_INFO("client %s register success!\n", ptr->client_name);
			smp_wmb();
		} else {
			DSM_LOG_ERR("clients register failed, index %d, conflict %d\n", i, conflict);
			vfree(ptr);
			ptr = NULL;
		}
	} else {
		DSM_LOG_INFO("clients has full\n");
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
out:
	return ptr;
}

/**
 * func - unregister dsm_client form server
 * @dsm_client: the client has registered in server
 * @dev: the device which used for creating dsm_client.
 *
 *		find out the same name of dev->name in the server's client_list, and clear the bit, free the dsm_client.
 * NOTE:
 */
void dsm_unregister_client (struct dsm_client *dsm_client,struct dsm_dev *dev)
{
	int i;
	int conflict;

	if(NULL == dsm_client || NULL == dev || NULL == dev->name){
		pr_info("pointer is NULL, please check the parameters!\n");
		return;
	}
	mutex_lock(&g_dsm_server.mtx_lock);
	for(i=0; i<CLIENT_SIZE; i++){
		/* find the client and free it */
		conflict = strncmp(g_dsm_server.client_list[i]->client_name, dev->name, CLIENT_NAME_LEN);
		if(!conflict){
			__clear_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]);
			g_dsm_server.client_list[i] = NULL;
			g_dsm_server.client_count--;
			vfree(dsm_client);
			dsm_client = NULL;
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
}

inline int dsm_client_ocuppy(struct dsm_client *client)
{
	int ret = -1;
	if (client) {
		smp_rmb();
		ret = test_and_set_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	}
	return ret;
}

inline int dsm_client_unocuppy(struct dsm_client *client)
{
	int ret = -1;
	if (client) {
		clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
		smp_wmb();
		ret = 0;
	}
	return ret;
}

void dsm_client_notify(struct dsm_client *client, int error_no)
{
	if (client) {
		client->error_no = error_no;
		set_bit(CBUFF_READY_BIT, &client->buff_flag);
		set_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[client->client_id]);
		smp_wmb();
		queue_work(g_dsm_server.dsm_wq, &dsm_work);
	}
	return;
}

int dsm_client_record(struct dsm_client *client, const char *fmt, ...)
{
	va_list ap;
	int size = 0;
	size_t avail = 0;

	if (!client) {
		DSM_LOG_ERR("%s no client to record\n", __func__);
		goto out;
	}

	if (client->buff_size - 1 <= client->used_size) {
		DSM_LOG_ERR("%s no buffer to record\n", __func__);
		goto out;
	}

	avail = client->buff_size - client->used_size - 1;

	va_start(ap, fmt);
	size = vsnprintf((char*)&client->dump_buff[client->used_size], avail, fmt, ap);
	va_end(ap);
	if(size < 0) {
		DSM_LOG_ERR("%s:record buffer failed!\n", __func__);
		goto out;
	}

	client->used_size += size;
	if(client->used_size >= client->buff_size) {
		client->used_size = client->buff_size - 1;
	}
out:
	return size;
}

int dsm_client_copy(struct dsm_client *client, void *src, int sz)
{
	int size = 0;

	if (!client) {
		DSM_LOG_ERR("%s no client to record\n", __func__);
		goto out;
	}

	if ((client->used_size + sz) > client->buff_size) {
		DSM_LOG_ERR("%s no enough buffer to record\n", __func__);
		goto out;
	}

	size = sz;
	memcpy(&client->dump_buff[client->used_size], src, size);
	client->used_size += size;

out:
	return size;
}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static unsigned int dsm_kfifo_get_data_len(void)
{
	unsigned int fifo_data_len = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&g_dsm_msgq.fifo_lock, flags);
	fifo_data_len = kfifo_len(&g_dsm_msgq.msg_fifo);
	spin_unlock_irqrestore(&g_dsm_msgq.fifo_lock, flags);

	return fifo_data_len;
}

static void dsm_kfifo_reset(void)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&g_dsm_msgq.fifo_lock, flags);
	kfifo_reset(&g_dsm_msgq.msg_fifo);
	spin_unlock_irqrestore(&g_dsm_msgq.fifo_lock, flags);
}

static bool dsm_is_errno_in_da_range(int error_no)
{
	return (error_no >= DA_MIN_ERROR_NO && error_no <= DA_MAX_ERROR_NO);
}

int dsm_client_copy_ext(struct dsm_client *client, void *src, int sz)
{
	int size = 0;

	if (!client || !src || sz <= 0) {
		DSM_LOG_ERR("%s invlaid params\n", __func__);
		goto out;
	}

	if ((dsm_kfifo_get_data_len() + sz) > MSGQ_SIZE) {
		DSM_LOG_ERR("%s no enough space in msgQ, [used_size] - %lu bytes\n",
			__func__, dsm_kfifo_get_data_len());
		goto out;
	}

	size = kfifo_in_locked(&g_dsm_msgq.msg_fifo, src, sz,
		&g_dsm_msgq.fifo_lock);
	if (size > 0)
		DSM_LOG_INFO("%s finish putting %d bytes into msgQ\n",
			client->client_name, size);

out:
	return size;
}
#endif

static inline int dsm_client_readable(struct dsm_client *client)
{
	int ret = 0;
	if (client) {
		smp_rmb();
		ret = test_bit(CBUFF_READY_BIT, &client->buff_flag);
	}
	return ret;
}

static inline int dsm_atoi(const char *p)
{
	int val = 0;

	if (!p)
		return -1;

	while (isdigit(*p))
		val = val*10 + (*p++ - '0');

	return val;
}

static inline char *dsm_strtok(char *string_org, const char *demial)
{
	static unsigned char *last;
	unsigned char *str;
	const unsigned char *ctrl = (const unsigned char *)demial;
	unsigned char map[32];
	int count;

	if (!demial) {
		return last;
	}

	for (count = 0; count < 32; count++)
		map[count] = 0;

	do {
		map[*ctrl >> 3] |= (1 << (*ctrl & 7));
	} while (*ctrl++);

	if (string_org)
		str = (unsigned char *)string_org;
	else
		str = last;

	while ((map[*str >> 3] & (1 << (*str & 7)))  && *str)
		str++;

	string_org = (char *)str;
	for (; *str; str++) {
		if (map[*str >> 3] & (1 << (*str & 7))) {
			*str++ = '\0';
			break;
		}
	}
	last = str;
	if (string_org == (char *)str)
		return NULL;
	else
		return string_org;
}

static inline int copy_int_to_user(void __user *argp, int val)
{
	int ret;
	int size;
	char buff[UINT_BUF_MAX] = {0};

	size = snprintf(buff, UINT_BUF_MAX, "%d\n", val);
	ret = copy_to_user(argp, buff, size);
	DSM_LOG_DEBUG("%s result %d\n", __func__, ret);
	return ret;
}

struct dsm_client *dsm_find_client(char *cname)
{
	int i;
	struct dsm_client *client = NULL;
	if(NULL == cname || !strlen(cname))
	{
		DSM_LOG_ERR("cname is NULL\n");
		goto out;
	}
	if (g_dsm_server.server_state != DSM_SERVER_INITED) {
		goto out;
	}

	mutex_lock(&g_dsm_server.mtx_lock);
	smp_rmb();
	for (i = 0; i < CLIENT_SIZE; i++) {
		if ((test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i])) && (!strncasecmp(
			g_dsm_server.client_list[i]->client_name, cname, CLIENT_NAME_LEN))){
			client = g_dsm_server.client_list[i];
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	DSM_LOG_DEBUG("cname: %s find %s\n", cname, client ? "success" : "failed");

out:
	return client;
}

static inline void dsm_client_set_idle(struct dsm_client *client)
{
	client->used_size = 0;
	client->read_size = 0;
	client->error_no = 0;
	memset(client->dump_buff, 0, client->buff_size);
	clear_bit(CBUFF_READY_BIT, &client->buff_flag);
	clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	clear_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[client->client_id]);
	smp_wmb();
	return;
}

static inline void dsm_bind_client(struct dsm_client *client)
{
	return;
}


static inline void dsm_unbind_client(struct dsm_client *client)
{
	return;
}

static void dsm_work_func(struct work_struct *work)
{
	int i;
	struct dsm_client *client;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	mutex_lock(&g_dsm_server.mtx_lock);
	smp_rmb();
	for (i = 0; i < CLIENT_SIZE; i++) {
		if (test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i])) {
			DSM_LOG_DEBUG("No.%d client name %s flag 0x%lx\n", i,
				g_dsm_server.client_list[i]->client_name, g_dsm_server.client_flag[i]);
			if (!test_and_clear_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[i]))
				continue;
			client = g_dsm_server.client_list[i];
			wake_up_interruptible_all(&client->waitq);
			DSM_LOG_INFO("%s finish notify\n", client->client_name);
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	DSM_LOG_DEBUG("%s exit\n", __func__);

	return;
}

static void dsm_get_report_info(char* string_buff)
{
	char client_name[CLIENT_NAME_LEN] = {'\0'};
	int size, error_no = 0;
	struct dsm_client *client = NULL;
	char *buff = NULL;
	char *ptr = NULL;

	buff = string_buff;

	/*get client name*/
	ptr = dsm_strtok(buff, ",");
	if (ptr) {
		size = strlen(ptr);
		size = (size < CLIENT_NAME_LEN) ? size : (CLIENT_NAME_LEN - 1);
		memcpy(client_name, ptr, size);
	}
	/*get error no*/
	ptr = dsm_strtok(NULL, ",");
	if (ptr) {
		error_no = dsm_atoi(ptr);
	}

	/*get notify content*/
	ptr = dsm_strtok(NULL, NULL);
	DSM_LOG_INFO("client name - %s, error no - %d\n", client_name, error_no);
	if (ptr) {
		DSM_LOG_INFO("content - %s\n", ptr);
	}

	client = dsm_find_client(client_name);

	if (client && (!dsm_client_ocuppy(client))) {
		DSM_LOG_DEBUG("dsm write find client - %s\n", client_name);
		if (ptr)
			dsm_client_copy(client, ptr, strlen(ptr));
		dsm_client_notify(client, error_no);
	} else {
		DSM_LOG_INFO("dsm notify can't find client - %s\n", client_name);
	}

}

static ssize_t dsm_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct dsm_client *client = file->private_data;
	size_t copy_size = 0;
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	size_t len = 0;
	int ret = 0;
	unsigned int copied = 0;
#endif

	DSM_LOG_DEBUG("%s enter\n", __func__);

	if (!client) {
		DSM_LOG_ERR("client not bind\n");
		goto out;
	}

	if (dsm_client_readable(client)) {
	#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
		if (dsm_is_errno_in_da_range(client->error_no)) {
			len = dsm_kfifo_get_data_len();
			DSM_LOG_DEBUG("[msgQ_len] - %lu bytes, [count] - %lu bytes\n",
				len, count);
			if (len > 0) {
				if (mutex_lock_interruptible(&g_dsm_msgq.read_lock)) {
					return -ERESTARTSYS;
				}
				ret = kfifo_to_user(&g_dsm_msgq.msg_fifo, buf, min(len, count), &copied);
				mutex_unlock(&g_dsm_msgq.read_lock);
				if (ret)
					DSM_LOG_ERR("copy msgQ to user failed, ret %d\n", ret);
				else
					copy_size = copied;

				if (copied == len) {
					DSM_LOG_DEBUG("all msgQ data is ready for reset\n");
					dsm_kfifo_reset();
				}
			} else {
				DSM_LOG_ERR("ignore coping to user as msgQ is empty\n");
			}
			dsm_client_set_idle(client);
			DSM_LOG_INFO("total %lu bytes read from msgQ to user\n", copy_size);
		} else {
	#endif
		copy_size = min(count, (client->used_size - client->read_size));
		if (copy_to_user(buf, &client->dump_buff[client->read_size], copy_size))
			DSM_LOG_ERR("copy to user failed\n");
		client->read_size += copy_size;
		if (client->read_size >= client->used_size)
			dsm_client_set_idle(client);
		DSM_LOG_DEBUG("%lu bytes read to user\n", copy_size);
	#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
		}
	#endif
	}

out:
	DSM_LOG_DEBUG("%s exit\n", __func__);
	return copy_size;
}

static ssize_t dsm_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char *buff = NULL;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	if (NULL == buf) {
		DSM_LOG_ERR("buf is null\n");
		goto out;
	}
	if ((count > DSM_MAX_LOG_SIZE) || (count < DSM_MIN_LOG_SIZE)) {
		DSM_LOG_ERR("count is %lu,out of range\n",count);
		goto out;
	}
	buff = (char *)kzalloc(count, GFP_KERNEL);
	if (!buff) {
		DSM_LOG_ERR("dsm write malloc failed\n");
		goto out;
	}

	if (copy_from_user(buff, buf, (count - 1))) {
		DSM_LOG_ERR("dsm write copy failed\n");
		goto out;
	}
	*(buff+count-1) = '\0';
	dsm_get_report_info(buff);

out:
	if (buff)
		kfree(buff);

	DSM_LOG_DEBUG("%s exit\n", __func__);
	return count;
}

static unsigned int dsm_poll(struct file *file, poll_table *wait)
{
	struct dsm_client *client = file->private_data;
	unsigned int mask = 0;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	if (!client) {
		DSM_LOG_ERR("dsm can't poll without client\n");
		goto out;
	}
	DSM_LOG_DEBUG("client name :%s\n", client->client_name);
	poll_wait(file, &client->waitq, wait);
	if (test_bit(CBUFF_READY_BIT, &client->buff_flag)) {
	#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
		if (dsm_is_errno_in_da_range(client->error_no)) {
			if (dsm_kfifo_get_data_len() > 0)
				mask = POLLIN | POLLRDNORM;
			goto out;
		}
	#endif
		mask = POLLIN | POLLRDNORM;
	}

out:
	DSM_LOG_DEBUG("%s exit, mask:%d\n", __func__, mask);
	return mask;
}

static int dsm_open(struct inode *inode, struct file *file)
{
	DSM_LOG_DEBUG("%s enter\n", __func__);
	file->private_data = NULL;
	DSM_LOG_DEBUG("%s exit\n", __func__);
	return 0;
}

static int dsm_close(struct inode *inode, struct file *file)
{
	struct dsm_client *client = file->private_data;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	if (client)
		dsm_unbind_client(client);
	DSM_LOG_DEBUG("%s exit\n", __func__);
	return 0;
}

static long dsm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dsm_client *client = (struct dsm_client *)file->private_data;
	long ret = 0;
	int error = 0;
	char buff[CLIENT_NAME_LEN] = {0};

	DSM_LOG_DEBUG("%s enter,\n", __func__);

	switch (cmd) {
	case DSM_IOCTL_GET_CLIENT_COUNT:
			mutex_lock(&g_dsm_server.mtx_lock);
			error = g_dsm_server.client_count;
			mutex_unlock(&g_dsm_server.mtx_lock);
			DSM_LOG_INFO("client count :%d\n", error);
			ret = copy_int_to_user(argp, error);
			break;
	case DSM_IOCTL_BIND:
			if (copy_from_user(buff, argp, CLIENT_NAME_LEN - 1)) {
				DSM_LOG_ERR("copy from user failed\n");
				ret = -EFAULT;
			} else {
				DSM_LOG_DEBUG("try bind client %s\n", buff);
				client = dsm_find_client(buff);
				if (client) {
					dsm_bind_client(client);
					file->private_data = (void *)client;
				} else {
					ret = -ENXIO;
				}
			}
			break;
	case DSM_IOCTL_POLL_CLIENT_STATE:
			if (client && client->cops && client->cops->poll_state) {
				error = client->cops->poll_state();
				DSM_LOG_INFO("poll %s state result :%d\n", client->client_name, error);
				ret = copy_int_to_user(argp, error);
			} else {
				DSM_LOG_ERR("dsm client not bound or poll not support\n");
				ret = -ENXIO;
			}
			break;
	case DSM_IOCTL_FORCE_DUMP:
			if (copy_from_user(buff, argp, UINT_BUF_MAX)) {
				DSM_LOG_ERR("copy from user failed\n");
				ret = -EFAULT;
			} else {
				if (client && client->cops && client->cops->dump_func) {
					if (!dsm_client_ocuppy(client)) {
						client->error_no = dsm_atoi(buff);
						client->used_size = client->cops->dump_func(client->error_no,
							(void *)client->dump_buff, (int)client->buff_size);
						set_bit(CBUFF_READY_BIT, &client->buff_flag);
					} else {
						DSM_LOG_INFO("client %s's buff ocuppy failed\n", client->client_name);
						ret = -EBUSY;
					}
				} else {
					DSM_LOG_ERR("dsm client not bound or dump not support\n");
					ret = -ENXIO;
				}
			}
			break;
	case DSM_IOCTL_GET_CLIENT_ERROR:
			if (client)
				ret = copy_int_to_user(argp, client->error_no);
			else{
				DSM_LOG_ERR("dsm find client failed\n");
				ret = -ENXIO;
			}
			break;
	case DSM_IOCTL_GET_DEVICE_NAME:
			if (client && (strlen(client->device_name) > 0)) {
				ret = copy_to_user(argp, client->device_name, DSM_MAX_DEVICE_NAME_LEN);
			} else {
				ret = -ENXIO;
			}
			break;
	case DSM_IOCTL_GET_IC_NAME:
			if (client && (strlen(client->ic_name) > 0)) {
				ret = copy_to_user(argp, client->ic_name, DSM_MAX_IC_NAME_LEN);
			} else {
				ret = -ENXIO;
			}
			break;
	case DSM_IOCTL_GET_MODULE_NAME:
			if (client && (strlen(client->module_name) > 0)) {
				ret = copy_to_user(argp, client->module_name, DSM_MAX_MODULE_NAME_LEN);
			} else {
				ret = -ENXIO;
			}
			break;
	default:
			DSM_LOG_ERR("unknown ioctl command :%d\n", cmd);
			ret = -EINVAL;
			break;
	}

	DSM_LOG_DEBUG("%s exit\n", __func__);
	return ret;
}

static const struct file_operations dsm_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.unlocked_ioctl	= dsm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dsm_ioctl,
#endif
	.open		= dsm_open,
	.release		= dsm_close,
	.read			= dsm_read,
	.write		= dsm_write,
	.poll			= dsm_poll,
};

/* cnotify format: |client name|,|error no|,|contents| */
static ssize_t dsm_cnotify_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char *strings = NULL;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	if (NULL == buf) {
		DSM_LOG_ERR("buf is null\n");
		goto out;
	}
	if ((count > DSM_MAX_LOG_SIZE) || (count < DSM_MIN_LOG_SIZE)) {
		DSM_LOG_ERR("count is %lu,out of range\n",count);
		goto out;
	}

	strings = (char *)kzalloc(count, GFP_KERNEL);
	if (!strings) {
		DSM_LOG_ERR("dsm write malloc failed\n");
		goto out;
	}

	memcpy(strings, buf, count-1);
	*(strings+count-1) = '\0';
	dsm_get_report_info(strings);

out:
	if (strings)
		kfree(strings);
	DSM_LOG_DEBUG("%s exit\n", __func__);
	return count;
}

static struct device_attribute dsm_interface_attrs[] = {
	__ATTR(client_notify, S_IWUSR, NULL, dsm_cnotify_store),
};

static struct miscdevice dsm_miscdev = {
	.minor		= DSM_MINOR,
	.name		= "dsm",
	.fops		= &dsm_fops,
};

static int __init dsm_init(void)
{
	int ret = -EIO;
	int i;

	memset(&g_dsm_server, 0, sizeof(struct dsm_server));
	g_dsm_server.server_state = DSM_SERVER_UNINIT;
	mutex_init(&g_dsm_server.mtx_lock);

	g_dsm_server.dsm_wq = create_singlethread_workqueue("dsm_wq");
	if (IS_ERR(g_dsm_server.dsm_wq)) {
		DSM_LOG_ERR("alloc workqueue failed\n");
		goto out;
	}

	INIT_WORK(&dsm_work, dsm_work_func);

	ret = misc_register(&dsm_miscdev);
	if (ret) {
		DSM_LOG_ERR("misc register failed\n");
		goto out;
	}

	g_dsm_server.server_state = DSM_SERVER_INITED;

	for (i = 0; i < ARRAY_SIZE(dsm_interface_attrs); i++) {
		ret = device_create_file(dsm_miscdev.this_device,
				&dsm_interface_attrs[i]);
		if (ret < 0)
			DSM_LOG_ERR("creating sysfs attribute %s failed: %d\n",
					dsm_interface_attrs[i].attr.name, ret);
	}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	memset(&g_dsm_msgq, 0, sizeof(struct dsm_msgq));
	spin_lock_init(&g_dsm_msgq.fifo_lock);
	mutex_init(&g_dsm_msgq.read_lock);
	if (kfifo_alloc(&g_dsm_msgq.msg_fifo, MSGQ_SIZE, GFP_KERNEL)) {
		DSM_LOG_ERR("alloc message queue failed\n");
		ret = -ENOMEM;
		goto out;
	}
#endif

out:
	DSM_LOG_INFO("%s called, ret %d\n", __func__, ret);
	return ret;
}

EXPORT_SYMBOL(dsm_client_ocuppy);
EXPORT_SYMBOL(dsm_register_client);
EXPORT_SYMBOL(dsm_unregister_client);
EXPORT_SYMBOL(dsm_client_record);
EXPORT_SYMBOL(dsm_client_notify);
EXPORT_SYMBOL(dsm_find_client);
EXPORT_SYMBOL(dsm_client_unocuppy);
EXPORT_SYMBOL(dsm_client_copy);
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
EXPORT_SYMBOL(dsm_client_copy_ext);
#endif
EXPORT_SYMBOL(dsm_update_client_vendor_info);

subsys_initcall(dsm_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Device state monitor");
