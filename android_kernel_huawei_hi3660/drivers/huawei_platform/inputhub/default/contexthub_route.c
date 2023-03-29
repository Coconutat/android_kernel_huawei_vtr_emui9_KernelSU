/*
 *  drivers/misc/inputhub/inputhub_route.c
 *  Sensor Hub Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: qindiwen <inputhub@huawei.com>
 *
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/suspend.h>
#include <linux/fb.h>
#include <linux/rtc.h>
#include <huawei_platform/log/log_exception.h>
#include <linux/wakelock.h>
#include <linux/hisi/hisi_syscounter.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include "protocol.h"
#include "contexthub_route.h"
#include "sensor_config.h"
#include "contexthub_recovery.h"
#include "contexthub_pm.h"
#include <huawei_platform/inputhub/motionhub.h>
#include <huawei_platform/inputhub/sensorhub.h>
#include <huawei_platform/log/imonitor.h>

#ifdef CONFIG_CONTEXTHUB_SHMEM
#include "shmem.h"
#endif

#define ROUTE_BUFFER_MAX_SIZE (1024 * 128)
#define SENSOR_DROP_IMONITOR_ID 936005000
#ifdef TIMESTAMP_SIZE
#undef TIMESTAMP_SIZE
#define TIMESTAMP_SIZE (8)
#endif

int step_ref_cnt;
static int iom3_timeout = 2000;
int g_iom3_state = IOM3_ST_NORMAL;
int resume_skip_flag;
int get_airpress_data;
int get_temperature_data;
int fobidden_comm; /*once this flag set to 1, ap/iom3 will not transfer commd */
int ext_hall_type = 0;
struct sensor_status sensor_status;
spinlock_t	fsdata_lock;
bool fingersense_data_ready;
bool fingersense_data_intrans;        /* the data is on the way */
s16 fingersense_data[FINGERSENSE_DATA_NSAMPLES] = { 0 };

static struct type_record type_record;
static struct wake_lock wlock;
static struct mutex mutex_write_cmd;
static struct mutex mutex_write_adapter;
static struct mutex mutex_unpack;
static uint16_t tranid;
static struct mcu_notifier mcu_notifier = { LIST_HEAD_INIT(mcu_notifier.head) };
static int ps_value; /*save ps value for phonecall dmd*/
static int tof_value = 0;
static int als_retry_cnt = ALS_RESET_COUNT;
static int ps_retry_cnt = PS_RESET_COUNT;
static int64_t sensors_tm[TAG_SENSOR_END] = { 0, };
static spinlock_t ref_cnt_lock;
static uint64_t als_last_reset_period = 0;
static uint64_t ps_last_reset_period = 0;
static uint32_t valid_step_count;
static uint32_t recovery_step_count;
static uint32_t valid_floor_count = 0;
static uint32_t recovery_floor_count = 0;
static struct workqueue_struct *mcu_aod_wq;
static int adapt_ext_hall_index = 0;
static DEFINE_MUTEX(mutex_update);

extern struct CONFIG_ON_DDR *pConfigOnDDr;
extern wait_queue_head_t iom3_rec_waitq;
extern struct ipc_debug ipc_debug_info;
extern volatile int hall_value;
extern struct completion iom3_reboot;
extern int stop_auto_accel;
extern int stop_auto_als;
extern int stop_auto_ps;
extern rproc_id_t ipc_ap_to_iom_mbx;

#ifdef CONFIG_HUAWEI_DSM
extern struct dsm_client *shb_dclient;
#endif
extern unsigned int sensor_read_number[];
extern int iom3_power_state;
extern struct completion iom3_resume_mini;
extern struct completion iom3_resume_all;
extern atomic_t iom3_rec_state;
extern uint32_t need_reset_io_power;
extern uint8_t tag_to_hal_sensor_type[TAG_SENSOR_END];

extern int ak8789_register_report_data(int ms);
extern int color_sensor_enable(bool enable);
extern void fingerprint_ipc_cgbe_abort_handle(void);

extern int hall_number ;
static struct inputhub_route_table package_route_tbl[] = {
	{ROUTE_SHB_PORT, {NULL, 0}, {NULL, 0}, {NULL, 0}, __WAIT_QUEUE_HEAD_INITIALIZER(package_route_tbl[0].read_wait)},
	{ROUTE_MOTION_PORT, {NULL, 0}, {NULL, 0}, {NULL, 0}, __WAIT_QUEUE_HEAD_INITIALIZER(package_route_tbl[1].read_wait)},
	{ROUTE_CA_PORT, {NULL,0}, {NULL,0}, {NULL,0}, __WAIT_QUEUE_HEAD_INITIALIZER(package_route_tbl[2].read_wait)},
	{ROUTE_FHB_PORT, {NULL,0}, {NULL,0}, {NULL,0}, __WAIT_QUEUE_HEAD_INITIALIZER(package_route_tbl[3].read_wait)},
	{ROUTE_FHB_UD_PORT, {NULL,0}, {NULL,0}, {NULL,0}, __WAIT_QUEUE_HEAD_INITIALIZER(package_route_tbl[4].read_wait)},
};

static struct {
	int ext_hall_adapt;
	int ext_hall_value[HALL_ONE_DATA_NUM];
} ext_hall_table[] = {
    { 0, {1, 0, 2, -1}},
};

bool really_do_enable_disable(int *ref_cnt, bool enable, int bit)
{
	bool ret = false;
	unsigned long flags = 0;
	if ((bit < 0) || (bit >= sizeof(int32_t) << 3)) {
		hwlog_err("bit %d out of range in %s.\n", bit, __func__);
		return false;
	}

	spin_lock_irqsave(&ref_cnt_lock, flags);
	if (enable) {
		ret = (0 == *ref_cnt);
		*ref_cnt |= 1 << bit;
	} else {
		*ref_cnt &= ~(1 << bit);
		ret = (0 == *ref_cnt);
	}
	/*ret = (0 == (enable ? (*ref_cnt)++ : --*ref_cnt));*/
	spin_unlock_irqrestore(&ref_cnt_lock, flags);
	return ret;
}

static int inputhub_route_item(unsigned short port, struct inputhub_route_table **route_item)
{
	int i;
	for (i = 0; i < sizeof(package_route_tbl) / sizeof(package_route_tbl[0]); ++i) {
		if (port == package_route_tbl[i].port) {
			*route_item = &package_route_tbl[i];
			return 0;
		}
	}

	hwlog_err("unknown port: %d in %s.\n", port, __func__);
	return -EINVAL;

}

int inputhub_route_open(unsigned short port)
{
	int ret;
	char *pos;
	struct inputhub_route_table *route_item;

	hwlog_info("%s\n", __func__);
	ret = inputhub_route_item(port, &route_item);
	if (ret < 0)
		return -EINVAL;

	if (route_item->phead.pos) {
		hwlog_err("port:%d was already opened in %s.\n", port, __func__);
		return -EINVAL;
	}

	pos = vzalloc(ROUTE_BUFFER_MAX_SIZE);
	if (!pos)
		return -ENOMEM;

	route_item->phead.pos = pos;
	route_item->pWrite.pos = pos;
	route_item->pRead.pos = pos;
	route_item->phead.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->pWrite.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->pRead.buffer_size = 0;
	return 0;
}

void inputhub_route_close(unsigned short port)
{
	int ret;
	struct inputhub_route_table *route_item;

	hwlog_info("%s\n", __func__);
	ret = inputhub_route_item(port, &route_item);
	if (ret < 0)
		return;

	if (route_item->phead.pos)
		vfree(route_item->phead.pos);

	route_item->phead.pos = NULL;
	route_item->pWrite.pos = NULL;
	route_item->pRead.pos = NULL;
}

static inline bool data_ready(struct inputhub_route_table *route_item, struct inputhub_buffer_pos *reader)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	*reader = route_item->pRead;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return reader->buffer_size > 0;
}

ssize_t inputhub_route_read(unsigned short port, char __user *buf, size_t count)
{
	struct inputhub_route_table *route_item;
	struct inputhub_buffer_pos reader;
	char *buffer_end;
	unsigned int full_pkg_length;
	unsigned int tail_half_len;
	unsigned long flags = 0;

	if (inputhub_route_item(port, &route_item) != 0) {
		hwlog_err("inputhub_route_item failed in %s\n", __func__);
		return 0;
	}

	buffer_end = route_item->phead.pos + route_item->phead.buffer_size;

	/*woke up by signal*/
	if (wait_event_interruptible(route_item->read_wait, data_ready(route_item, &reader)) != 0)
		return 0;

	if (reader.buffer_size > ROUTE_BUFFER_MAX_SIZE) {
		hwlog_err("error reader.buffer_size = %d in port %d!\n", (int)reader.buffer_size, (int)port);
		goto clean_buffer;
	}

	if (buffer_end - reader.pos >= LENGTH_SIZE) {
		full_pkg_length = *((unsigned int *)reader.pos);
		reader.pos += LENGTH_SIZE;
		if (reader.pos == buffer_end)
			reader.pos = route_item->phead.pos;
	} else {
		tail_half_len = buffer_end - reader.pos;
		memcpy(&full_pkg_length, reader.pos, tail_half_len);
		memcpy((char *)&full_pkg_length + tail_half_len, route_item->phead.pos, LENGTH_SIZE - tail_half_len);
		reader.pos = route_item->phead.pos + (LENGTH_SIZE - tail_half_len);
	}

	if (full_pkg_length + LENGTH_SIZE > reader.buffer_size || full_pkg_length > count) {
		hwlog_err("full_pkg_length = %u is too large in port %d!\n", full_pkg_length, (int)port);
		goto clean_buffer;
	}

	if (buffer_end - reader.pos >= full_pkg_length) {
		if (0 == copy_to_user(buf, reader.pos, full_pkg_length)) {
			reader.pos += full_pkg_length;
			if (reader.pos == buffer_end)
				reader.pos = route_item->phead.pos;
		} else {
			hwlog_err("copy to user failed\n");
			return 0;
		}
	} else {
		tail_half_len = buffer_end - reader.pos;
		if ((0 == copy_to_user(buf, reader.pos, tail_half_len)) &&
		    (0 == copy_to_user(buf + tail_half_len, route_item->phead.pos, (full_pkg_length - tail_half_len)))) {
			reader.pos = route_item->phead.pos + (full_pkg_length -tail_half_len);
		} else {
			hwlog_err("copy to user failed\n");
			return 0;
		}
	}
	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->pRead.pos = reader.pos;
	route_item->pRead.buffer_size -= (full_pkg_length + LENGTH_SIZE);
	if ((route_item->pWrite.buffer_size > ROUTE_BUFFER_MAX_SIZE)
	    || (route_item->pWrite.buffer_size + (full_pkg_length + LENGTH_SIZE) > ROUTE_BUFFER_MAX_SIZE)) {
		hwlog_err("%s:%d write buffer error buffer_size=%u pkg_len=%u\n",
			  __func__, __LINE__, route_item->pWrite.buffer_size, full_pkg_length);
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		goto clean_buffer;
	} else {
		route_item->pWrite.buffer_size += (full_pkg_length + LENGTH_SIZE);
	}
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return full_pkg_length;

clean_buffer:
	hwlog_err("now we will clear the receive buffer in port %d!\n", (int)port);
	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	route_item->pRead.pos = route_item->pWrite.pos;
	route_item->pWrite.buffer_size = ROUTE_BUFFER_MAX_SIZE;
	route_item->pRead.buffer_size = 0;
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	return 0;
}

EXPORT_SYMBOL_GPL(inputhub_route_read);

static int64_t getTimestamp(void)
{
	struct timespec ts;
	get_monotonic_boottime(&ts);
	/*timevalToNano*/
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static inline void write_to_fifo(struct inputhub_buffer_pos *pwriter,
				 char *const buffer_begin, char *const buffer_end, char *buf, int count)
{
	int cur_to_tail_len = buffer_end - pwriter->pos;

	if (cur_to_tail_len >= count) {
		memcpy(pwriter->pos, buf, count);
		pwriter->pos += count;
		if (buffer_end == pwriter->pos) {
			pwriter->pos = buffer_begin;
		}
	} else {
		memcpy(pwriter->pos, buf, cur_to_tail_len);
		memcpy(buffer_begin, buf + cur_to_tail_len, count - cur_to_tail_len);
		pwriter->pos = buffer_begin + (count - cur_to_tail_len);
	}
}

t_ap_sensor_ops_record all_ap_sensor_operations[TAG_SENSOR_END] = {
#ifdef CONFIG_HUAWEI_HALL_INPUTHUB
	[TAG_HALL] = {
		.work_on_ap = true,
		.ops = {.setdelay = ak8789_register_report_data},
	},
	[TAG_EXT_HALL] = {
		.work_on_ap = true,
		.ops = {.setdelay = ak8789_register_report_data},
	},
#endif
#ifdef CONFIG_SENSORS_COLOR_AP
	[TAG_COLOR] = {
		.work_on_ap = true,
		.ops = {//.setdelay = ams_tcs3430_setdelay,
				.enable = color_sensor_enable},
	},
#endif
};

int register_ap_sensor_operations(int tag, sensor_operation_t *ops)
{
	if (!(TAG_SENSOR_BEGIN <= tag && tag < TAG_SENSOR_END)) {
		hwlog_err("tag %d range error in %s\n", tag, __func__);
		return -EINVAL;
	}

	if (!all_ap_sensor_operations[tag].work_on_ap) {
		memcpy(&all_ap_sensor_operations[tag].ops, ops, sizeof(sensor_operation_t));
		all_ap_sensor_operations[tag].work_on_ap = true;
	} else {
		hwlog_warn("tag %d has registered already in %s\n", tag, __func__);
	}

	return 0;
}

int unregister_ap_sensor_operations(int tag)
{
	if (!(TAG_SENSOR_BEGIN <= tag && tag < TAG_SENSOR_END)) {
		hwlog_err("tag %d range error in %s\n", tag, __func__);
		return -EINVAL;
	}
	memset(&all_ap_sensor_operations[tag], 0, sizeof(all_ap_sensor_operations[tag]));
	return 0;
}

static void sensor_get_data(struct sensor_data *data)
{
	struct t_sensor_get_data *get_data = NULL;
	if (NULL == data ||(!(TAG_SENSOR_BEGIN <= data->type && data->type < TAG_SENSOR_END))) {
		return;
	}

	get_data = &sensor_status.get_data[data->type];
	if (atomic_cmpxchg(&get_data->reading, 1, 0)) {
		memcpy(&get_data->data, data, sizeof(get_data->data));
		complete(&get_data->complete);
	}
}

int report_sensor_event(int tag, int value[], int length)
{
	struct sensor_data event;

	if ((!(TAG_SENSOR_BEGIN <= tag && tag < TAG_SENSOR_END)) || (length > sizeof(event.value))) {
		hwlog_err("para error (tag : %d), (length : %d) in %s\n", tag, length, __func__);
		return -EINVAL;
	}

	event.type = tag_to_hal_sensor_type[tag];
	event.length = length;
	memcpy(&event.value, value, length);
	sensor_get_data(&event);

	return inputhub_route_write(ROUTE_SHB_PORT, (char *)&event,
		event.length + OFFSET_OF_END_MEM(struct sensor_data, length));
}

static int adapt_hall_value(int value)
{
	if(adapt_ext_hall_index >= ARRAY_SIZE(ext_hall_table)){
		return -EPERM;
	}
	if(value > MAX_EXT_HALL_VALUE){
		return -EPERM;
	}
	return ext_hall_table[adapt_ext_hall_index].ext_hall_value[value];
}

int ap_hall_report(int value)
{
	int double_hall_value[3] = {0};
	int ext_hall_value[HALL_DATA_NUM] = {0};
	hall_value = value;
	double_hall_value[0] = value;
	if(hall_number == 2){
		double_hall_value[1] = 2;
	}else{
		ext_hall_value[0] = SLIDE_HALL_TYPE;
		ext_hall_value[1] = adapt_hall_value(value);
	}
	//return diff sensor type
	report_sensor_event(TAG_HALL, double_hall_value, sizeof(double_hall_value));
	report_sensor_event(TAG_EXT_HALL, ext_hall_value, sizeof(ext_hall_value));

	return 0;
}
int ap_color_report(int value[], int length)
{
	return report_sensor_event(TAG_COLOR, value, length);
}

int thp_prox_event_report(int value[], int length)
{
	if (value == NULL)
		return -EINVAL;

	return 0;
}

bool ap_sensor_enable(int tag, bool enable)
{
	bool work_on_ap = false;

	if (tag < TAG_SENSOR_BEGIN ||tag >= TAG_SENSOR_END)
		return false;

	work_on_ap = all_ap_sensor_operations[tag].work_on_ap;
	if (work_on_ap) {	/*leave this code for furture use*/
		if (all_ap_sensor_operations[tag].ops.enable) {
			all_ap_sensor_operations[tag].ops.enable(enable);
		}
	}

	return work_on_ap;
}

bool ap_sensor_setdelay(int tag, int ms)
{
	bool work_on_ap;

	if (tag < TAG_SENSOR_BEGIN ||tag>=TAG_SENSOR_END)
		return false;

	work_on_ap = all_ap_sensor_operations[tag].work_on_ap;
	if (work_on_ap) {
		if (all_ap_sensor_operations[tag].ops.setdelay) {
			all_ap_sensor_operations[tag].ops.setdelay(ms);
		}
	}
	return work_on_ap;
}

ssize_t inputhub_route_write_batch(unsigned short port, char *buf, size_t count, int64_t timestamp)
{
	struct inputhub_route_table *route_item;
	struct inputhub_buffer_pos writer;
	char *buffer_begin, *buffer_end;
	t_head header;
	unsigned long flags = 0;

	if (inputhub_route_item(port, &route_item) != 0) {
		hwlog_err("inputhub_route_item failed in %s port = %d!\n", __func__, (int)port);
		return 0;
	}
	header.timestamp = timestamp;

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	writer = route_item->pWrite;

	if (writer.buffer_size < count + HEAD_SIZE) {
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		/*hwlog_err("inputhub_route_write_batch failed buffer not enough!\n");*/
		return 0;
	}

	buffer_begin = route_item->phead.pos;
	buffer_end = route_item->phead.pos + route_item->phead.buffer_size;
	if (UINT_MAX - count < sizeof(int64_t)) {
		hwlog_err("inputhub_route_write_batch :count is too large :%zd!\n", count);
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		return 0;
	} else {
		header.pkg_length = count + sizeof(int64_t);
	}
	write_to_fifo(&writer, buffer_begin, buffer_end, header.effect_addr, HEAD_SIZE);
	write_to_fifo(&writer, buffer_begin, buffer_end, buf, count);

	route_item->pWrite.pos = writer.pos;
	route_item->pWrite.buffer_size -= (count + HEAD_SIZE);
	if ((UINT_MAX - route_item->pRead.buffer_size) < (count + HEAD_SIZE)) {
		hwlog_err("inputhub_route_write_batch:pRead :count is too large :%zd!\n", count);
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		return 0;
	} else {
		route_item->pRead.buffer_size += (count + HEAD_SIZE);
	}
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	atomic_set(&route_item->data_ready, 1);
	wake_up_interruptible(&route_item->read_wait);

	return (count + HEAD_SIZE);
}

ssize_t inputhub_route_write(unsigned short port, char *buf, size_t count)
{
	struct inputhub_route_table *route_item;
	struct inputhub_buffer_pos writer;
	char *buffer_begin, *buffer_end;
	t_head header;
	unsigned long flags = 0;

	if (inputhub_route_item(port, &route_item) != 0) {
		hwlog_err("inputhub_route_item failed in %s port = %d!\n", __func__, (int)port);
		return 0;
	}
	header.timestamp = getTimestamp();

	spin_lock_irqsave(&route_item->buffer_spin_lock, flags);
	writer = route_item->pWrite;

	if (writer.buffer_size < count + HEAD_SIZE) {
		spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
		return 0;
	}

	buffer_begin = route_item->phead.pos;
	buffer_end = route_item->phead.pos + route_item->phead.buffer_size;
	header.pkg_length = count + sizeof(int64_t);
	write_to_fifo(&writer, buffer_begin, buffer_end, header.effect_addr, HEAD_SIZE);
	write_to_fifo(&writer, buffer_begin, buffer_end, buf, count);

	route_item->pWrite.pos = writer.pos;
	route_item->pWrite.buffer_size -= (count + HEAD_SIZE);
	route_item->pRead.buffer_size += (count + HEAD_SIZE);
	spin_unlock_irqrestore(&route_item->buffer_spin_lock, flags);
	atomic_set(&route_item->data_ready, 1);
	wake_up_interruptible(&route_item->read_wait);

	return (count + HEAD_SIZE);
}
EXPORT_SYMBOL_GPL(inputhub_route_write);

static const pkt_header_t *normalpack(const char *buf, unsigned int length)
{
	const pkt_header_t *head = (const pkt_header_t *)buf;
	static struct link_package linker = { -1 };	/*init partial_order to -1 to aviod lost first pkt*/

	/*try to judge which pkt it is*/
	if ((TAG_BEGIN <= head->tag && head->tag < TAG_END)
	    && (head->length <= (MAX_PKT_LENGTH_AP - sizeof(pkt_header_t)))) {
		linker.total_pkg_len = head->length + OFFSET_OF_END_MEM(pkt_header_t, length);
		if (linker.total_pkg_len > (int)length) {	/*need link other partial packages*/
			linker.partial_order = head->partial_order;	/*init partial_order*/
			if (length <= sizeof(linker.link_buffer)) {
				memcpy(linker.link_buffer, buf, length);	/*save first partial package*/
				linker.offset = length;
			} else {
				goto error;
			}
			goto receive_next;	/*receive next partial package*/
		} else {
			return head;	/*full pkt*/
		}
	} else if (TAG_END == head->tag) {	/*check if partial_order effective*/
		pkt_part_header_t *partial = (pkt_part_header_t *) buf;
		if (partial->partial_order == (uint8_t) (linker.partial_order + 1)) {	/*type must keep same with partial->partial_order, because integer promote*/
			int partial_pkt_data_length = length - sizeof(pkt_part_header_t);
			if (linker.offset + partial_pkt_data_length <= sizeof(linker.link_buffer)) {
				++linker.partial_order;
				memcpy(linker.link_buffer + linker.offset, buf + sizeof(pkt_part_header_t), partial_pkt_data_length);
				linker.offset += partial_pkt_data_length;
				if (linker.offset >= linker.total_pkg_len) {	/*link finished*/
					return (pkt_header_t *) linker.link_buffer;
				} else {
					goto receive_next;	/*receive next partial package*/
				}
			}
		}
	}

error:				/*clear linker info when error*/
	++ipc_debug_info.pack_error_cnt;
	linker.partial_order = -1;
	linker.total_pkg_len = 0;
	linker.offset = 0;
receive_next:
	return NULL;
}

static int inputhub_mcu_send(const char* buf, unsigned int length)
{
	mbox_msg_len_t len = 0;
	int ret = -1;

	peri_used_request();
	len = (length + sizeof(mbox_msg_t) - 1) / (sizeof(mbox_msg_t));
	ret = RPROC_SYNC_SEND(ipc_ap_to_iom_mbx, (mbox_msg_t*) buf, len, NULL, 0);
	if (ret) {
	    hwlog_err("RPROC_SYNC_SEND return %d.\n", ret);
	    return -1;
	}
	peri_used_release();
	return ret;
}

static const pkt_header_t *pack(const char *buf, unsigned int length, bool *is_notifier)
{
	const pkt_header_t *head = normalpack(buf, length);
#ifdef CONFIG_CONTEXTHUB_SHMEM
	if(head && (head->tag == TAG_SHAREMEM))
	{
		if (head->cmd == CMD_SHMEM_AP_RECV_REQ) {
			head = shmempack(buf, length);
		} else if (head->cmd == CMD_SHMEM_AP_SEND_RESP) {
			shmem_send_resp(head);
			*is_notifier = true;
		}
	}
#endif
	return head;
}

static int unpack(const void *buf, unsigned int length)
{
	int ret = 0;
	static int partial_order;

	mutex_lock(&mutex_unpack);
	((pkt_header_t *) buf)->partial_order = partial_order++;	/*inc partial_order in header*/
	if (length <= MAX_SEND_LEN) {
		ret = inputhub_mcu_send((const char *)buf, length);
		goto out;
	} else {
		char send_partial_buf[MAX_SEND_LEN];
		unsigned int send_cnt = 0;

		/*send head*/
		ret = inputhub_mcu_send((const char *)buf, MAX_SEND_LEN);
		if (ret != 0)
			goto out;

		((pkt_part_header_t *) send_partial_buf)->tag = TAG_END;
		for (send_cnt = MAX_SEND_LEN; send_cnt < length;
		     send_cnt += (MAX_SEND_LEN - sizeof(pkt_part_header_t))) {
			((pkt_part_header_t *) send_partial_buf)->partial_order = partial_order++;	/*inc partial_order in partial pkt*/
			memcpy(send_partial_buf + sizeof(pkt_part_header_t),
			       (const char *)buf + send_cnt, min(MAX_SEND_LEN - sizeof(pkt_part_header_t), (unsigned long)(length - send_cnt)));
			ret = inputhub_mcu_send(send_partial_buf, MAX_SEND_LEN);
			if (ret != 0)
				goto out;
		}
	}

out:
	mutex_unlock(&mutex_unpack);
	return ret;
}

static struct mcu_event_wait_list {
	spinlock_t slock;
	struct list_head head;
} mcu_event_wait_list;

void init_wait_node_add_list(struct mcu_event_waiter *waiter, t_match match,
			     void *out_data, int out_data_len, void *priv)
{
	unsigned long flags = 0;

	waiter->match = match;
	init_completion(&waiter->complete);
	waiter->out_data = out_data;
	waiter->out_data_len = out_data_len;
	waiter->priv = priv;

	spin_lock_irqsave(&mcu_event_wait_list.slock, flags);
	list_add(&waiter->entry, &mcu_event_wait_list.head);
	spin_unlock_irqrestore(&mcu_event_wait_list.slock, flags);
}

void list_del_mcu_event_waiter(struct mcu_event_waiter *self)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&mcu_event_wait_list.slock, flags);
	list_del(&self->entry);
	spin_unlock_irqrestore(&mcu_event_wait_list.slock, flags);
}

static void wake_up_mcu_event_waiter(const pkt_header_t *head)
{
	unsigned long flags = 0;
	struct mcu_event_waiter *pos, *n;

	spin_lock_irqsave(&mcu_event_wait_list.slock, flags);
	list_for_each_entry_safe(pos, n, &mcu_event_wait_list.head, entry) {
		if (pos->match && pos->match(pos->priv, head)) {
			if (pos->out_data != NULL) {
				memcpy(pos->out_data, head, pos->out_data_len);
			}
			complete(&pos->complete);
			/*to support diffrent task wait for same event, here we don't break;*/
		}
	}
	spin_unlock_irqrestore(&mcu_event_wait_list.slock, flags);
}

static int inputhub_mcu_write_cmd_nolock(const void *buf, unsigned int length)
{
	int ret = 0;
	pkt_header_t *pkt = (pkt_header_t *) buf;
	((pkt_header_t *) buf)->resp = RESP;
	if (!WAIT_FOR_MCU_RESP_AFTER_SEND(buf, unpack(buf, length), 2000)) {
		hwlog_err("wait for tag:%s:%d\tcmd:%d resp timeout in %s\n", obj_tag_str[pkt->tag], pkt->tag, pkt->cmd, __func__);
		ret = -1;
	}

	return ret;
}

char *obj_tag_str[] = {
	[TAG_ACCEL] = "TAG_ACCEL",
	[TAG_GYRO] = "TAG_GYRO",
	[TAG_MAG] = "TAG_MAG",
	[TAG_ALS] = "TAG_ALS",
	[TAG_PS] = "TAG_PS",
	[TAG_LINEAR_ACCEL] = "TAG_LINEAR_ACCEL",
	[TAG_GRAVITY] = "TAG_GRAVITY",
	[TAG_ORIENTATION] = "TAG_ORIENTATION",
	[TAG_ROTATION_VECTORS] = "TAG_ROTATION_VECTORS",
	[TAG_PRESSURE] = "TAG_PRESSURE",
	[TAG_HALL] = "TAG_HALL",
	[TAG_MAG_UNCALIBRATED] = "TAG_MAG_UNCALIBRATED",
	[TAG_GAME_RV] = "TAG_GAME_RV",
	[TAG_GYRO_UNCALIBRATED] = "TAG_GYRO_UNCALIBRATED",
	[TAG_SIGNIFICANT_MOTION] = "TAG_SIGNIFICANT_MOTION",
	[TAG_STEP_DETECTOR] = "TAG_STEP_DETECTOR",
	[TAG_STEP_COUNTER] = "TAG_STEP_COUNTER",
	[TAG_GEOMAGNETIC_RV] = "TAG_GEOMAGNETIC_RV",
	[TAG_HANDPRESS] = "TAG_HANDPRESS",
	[TAG_CAP_PROX] = "TAG_CAP_PROX",
	[TAG_FINGERSENSE] = "TAG_FINGERSENSE",
	[TAG_PHONECALL] = "TAG_PHONECALL",
	[TAG_CONNECTIVITY] = "TAG_CONNECTIVITY",
	[TAG_CA] = "TAG_CA",
	[TAG_OIS] = "TAG_OIS",
	[TAG_TP] = "TAG_TP",
	[TAG_SPI] = "TAG_SPI",
	[TAG_I2C] = "TAG_I2C",
	[TAG_RGBLIGHT] = "TAG_RGBLIGHT",
	[TAG_BUTTONLIGHT] = "TAG_BUTTONLIGHT",
	[TAG_BACKLIGHT] = "TAG_BACKLIGHT",
	[TAG_VIBRATOR] = "TAG_VIBRATOR",
	[TAG_SYS] = "TAG_SYS",
	[TAG_LOG] = "TAG_LOG",
	[TAG_MOTION] = "TAG_MOTION",
	[TAG_LOG_BUFF] = "TAG_LOG_BUFF",
	[TAG_PDR] = "TAG_PDR",
	[TAG_AR] = "TAG_AR",
	[TAG_FP] = "TAG_FP",
	[TAG_KEY] = "TAG_KEY",
	[TAG_TILT_DETECTOR] = "TAG_TILT_DETECTOR",
	[TAG_FAULT] = "TAG_FAULT",
	[TAG_MAGN_BRACKET] = "TAG_MAGN_BRACKET",
	[TAG_RPC]= "TAG_RPC",
	[TAG_AGT]="TAG_AGT",
	[TAG_COLOR]="TAG_COLOR",
	[TAG_FP_UD] = "TAG_FP_UD",
	[TAG_ACCEL_UNCALIBRATED] = "TAG_ACCEL_UNCALIBRATED",
	[TAG_TOF]="TAG_TOF",
	[TAG_DROP] = "TAG_DROP",
	[TAG_EXT_HALL] = "TAG_EXT_HALL",
	[TAG_END] = "TAG_END",
};

static bool is_extend_step_counter_cmd(const pkt_header_t *pkt)
{
	bool ret = false;

	if (pkt->tag != TAG_STEP_COUNTER)
		return false;

	switch (pkt->cmd) {
	case CMD_CMN_OPEN_REQ:
	case CMD_CMN_CLOSE_REQ:
		/*could not judge which type step counter in open protocol*/
		break;
	case CMD_CMN_CONFIG_REQ:
		ret = (TYPE_EXTEND == ((pkt_cmn_motion_req_t *) pkt)->app_config[1]);
		break;

	case CMD_CMN_INTERVAL_REQ:
		ret = (TYPE_EXTEND == ((pkt_cmn_interval_req_t *) pkt)->param.reserved[0]);
		break;

	default:
		break;
	}

	return ret;
}

/*To keep same with mcu, to activate sensor need open first and then setdelay*/
static void update_sensor_info(const pkt_header_t *pkt)
{
	if (TAG_SENSOR_BEGIN <= pkt->tag && pkt->tag < TAG_SENSOR_END) {
		mutex_lock(&mutex_update);
		if (CMD_CMN_OPEN_REQ == pkt->cmd) {
			sensor_status.opened[pkt->tag] = 1;
		} else if (CMD_CMN_CLOSE_REQ == pkt->cmd) {
			sensor_status.opened[pkt->tag] = 0;
			sensor_status.status[pkt->tag] = 0;
			sensor_status.delay[pkt->tag] = 0;
		} else if (CMD_CMN_INTERVAL_REQ == pkt->cmd) {
			sensor_status.delay[pkt->tag] = ((const pkt_cmn_interval_req_t *)pkt)->param.period;
			sensor_status.status[pkt->tag] = 1;
			sensor_status.batch_cnt[pkt->tag] = ((const pkt_cmn_interval_req_t *)pkt)->param.batch_count;
		}
		mutex_unlock(&mutex_update);
	}
}

int inputhub_mcu_write_cmd(const void *buf, unsigned int length)
{
	bool is_in_recovery = false;
	int ret = 0;

	if (length > MAX_PKT_LENGTH) {
		hwlog_err("length = %d is too large in %s\n", (int)length, __func__);
		return -EINVAL;
	}
	if (TAG_SENSOR_BEGIN > ((pkt_header_t *) buf)->tag || ((pkt_header_t *) buf)->tag >=TAG_END) {
		hwlog_err("tag = %d is wrong in %s\n", ((pkt_header_t *) buf)->tag, __func__);
		return -EINVAL;
	}
	mutex_lock(&mutex_write_cmd);
	if (g_iom3_state == IOM3_ST_NORMAL) {	/*false - send direct*/
	} else if (g_iom3_state == IOM3_ST_RECOVERY) {	/*true - only update*/
		is_in_recovery = true;
	} else if (g_iom3_state == IOM3_ST_REPEAT) {	/*IOM3_ST_REPEAT...BLOCK IN HERE, WAIT FOR REPEAT COMPLETE*/
		hwlog_err("wait for iom3 recovery complete. tag %d cmd %d.\n", ((pkt_header_t *)buf)->tag, ((pkt_header_t *)buf)->cmd);
		mutex_unlock(&mutex_write_cmd);
		wait_event(iom3_rec_waitq, (g_iom3_state == IOM3_ST_NORMAL));
		hwlog_err("wakeup for iom3 recovery complete\n");
		mutex_lock(&mutex_write_cmd);
	} else {
		hwlog_err("unknown iom3 state %d\n", g_iom3_state);
	}
	if (true == is_in_recovery) {
		mutex_unlock(&mutex_write_cmd);
		goto update_info;
	}
	((pkt_header_t *) buf)->tranid = tranid++;
	ret = unpack(buf, length);
	mutex_unlock(&mutex_write_cmd);

update_info:
	update_current_app_status(((pkt_header_t *) buf)->tag, ((pkt_header_t *) buf)->cmd);
	if (!is_extend_step_counter_cmd(((const pkt_header_t *)buf))) {
		update_sensor_info(((const pkt_header_t *)buf));
	}
	if (ret && (false == is_in_recovery)) {
		iom3_need_recovery(SENSORHUB_MODID, SH_FAULT_IPC_TX_TIMEOUT);
	}
	return 0;
}

/*use lock for all command to avoid conflict*/
static int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length, struct read_info *rd)
{
	int ret = 0;
	unsigned long flags = 0;
	int retry_count = 2;

	mutex_lock(&mutex_write_adapter);
	peri_used_request();
	if (NULL == rd) {
		ret = inputhub_mcu_write_cmd(buf, length);
	} else {
		mutex_lock(&type_record.lock_mutex);
		spin_lock_irqsave(&type_record.lock_spin, flags);
		type_record.pkt_info = ((pkt_header_t *) buf);
		type_record.rd = rd;
		spin_unlock_irqrestore(&type_record.lock_spin, flags);
		while (retry_count--) {	/*send retry 3 times*/
			/*send data to mcu*/
			reinit_completion(&type_record.resp_complete);
			if (inputhub_mcu_write_cmd(buf, length)) {
				hwlog_err("send cmd to mcu failed in %s, retry %d\n", __func__, retry_count);
				ret = -1;
			} else if (!wait_for_completion_timeout(&type_record.resp_complete, msecs_to_jiffies(iom3_timeout))) {
				hwlog_err("wait for response timeout in %s, retry %d. tag %d cmd %d. g_iom3_state %d.\n",
				     __func__, retry_count, ((pkt_header_t *) buf)->tag, ((pkt_header_t *) buf)->cmd, g_iom3_state);
				if (retry_count == 0) {
					iom3_need_recovery(SENSORHUB_MODID, SH_FAULT_IPC_RX_TIMEOUT);
				}
				ret = -1;
			} else {
				ret = 0;	/*send success & response success*/
				break;
			}
		}

		/*clear info*/
		spin_lock_irqsave(&type_record.lock_spin, flags);
		type_record.pkt_info = NULL;
		type_record.rd = NULL;
		spin_unlock_irqrestore(&type_record.lock_spin, flags);
		mutex_unlock(&type_record.lock_mutex);
	}

	peri_used_release();
	mutex_unlock(&mutex_write_adapter);

	return ret;
}

int send_app_config_cmd_with_resp(int tag, void *app_config, bool use_lock)
{
	pkt_cmn_motion_req_t i_pkt;

	struct read_info rd;
	memset(&rd, 0, sizeof(rd));

	i_pkt.hd.tag = tag;
	i_pkt.hd.cmd = CMD_CMN_CONFIG_REQ;
	i_pkt.hd.resp = RESP;
	i_pkt.hd.length = sizeof(i_pkt.app_config);
	memcpy(i_pkt.app_config, app_config, sizeof(i_pkt.app_config));

	if (use_lock) {
		return inputhub_mcu_write_cmd_adapter(&i_pkt, sizeof(i_pkt), &rd);
	} else {
		return inputhub_mcu_write_cmd_nolock(&i_pkt, sizeof(i_pkt));
	}
}

int send_app_config_cmd(int tag, void *app_config, bool use_lock)
{
	pkt_cmn_motion_req_t i_pkt;

	i_pkt.hd.tag = tag;
	i_pkt.hd.cmd = CMD_CMN_CONFIG_REQ;
	i_pkt.hd.resp = NO_RESP;
	i_pkt.hd.length = sizeof(i_pkt.app_config);
	memcpy(i_pkt.app_config, app_config, sizeof(i_pkt.app_config));

	if (use_lock) {
		return inputhub_mcu_write_cmd_adapter(&i_pkt, sizeof(i_pkt), NULL);
	} else {
		return inputhub_mcu_write_cmd_nolock(&i_pkt, sizeof(i_pkt));
	}
}

static int inputhub_sensor_enable_internal(int tag, bool enable, bool is_lock)
{
	if (tag < TAG_BEGIN || tag >=TAG_END) {
		hwlog_err("NULL pointer param in %s or tag %d is error.\n", __func__, tag);
		return -EINVAL;
	}

	if (ap_sensor_enable(tag, enable)) {
		return 0;
	}

	if (enable) {
		pkt_header_t pkt = (pkt_header_t) {
			.tag = tag,
			.cmd = CMD_CMN_OPEN_REQ,
			.resp = NO_RESP,
			.length = 0
		};
		hwlog_info("open sensor %s (tag:%d)!\n", obj_tag_str[tag] ? obj_tag_str[tag] : "TAG_UNKNOWN", tag);
		if (is_lock)
			return inputhub_mcu_write_cmd_adapter(&pkt, sizeof(pkt), NULL);
		else
			return inputhub_mcu_write_cmd_nolock(&pkt, sizeof(pkt));
	} else {
		pkt_header_t pkt = (pkt_header_t) {
			.tag = tag,
			.cmd = CMD_CMN_CLOSE_REQ,
			.resp = NO_RESP,
			.length = 0
		};
		hwlog_info("close sensor %s (tag:%d)!\n", obj_tag_str[tag] ? obj_tag_str[tag] : "TAG_UNKNOWN", tag);
		if (is_lock)
			return inputhub_mcu_write_cmd_adapter(&pkt, sizeof(pkt), NULL);
		else
			return inputhub_mcu_write_cmd_nolock(&pkt, sizeof(pkt));
	}
}

static int inputhub_sensor_setdelay_internal(int tag, interval_param_t *param, bool use_lock)
{
	pkt_cmn_interval_req_t pkt;

	if (NULL == param || (tag < TAG_BEGIN || tag >=TAG_END)) {
		hwlog_err("NULL pointer param in %s or tag %d is error.\n", __func__, tag);
		return -EINVAL;
	}

	if (ap_sensor_setdelay(tag, param->period))
		return 0;

	memset(&pkt, 0, sizeof(pkt));
	pkt.hd.tag = tag;
	pkt.hd.cmd = CMD_CMN_INTERVAL_REQ;
	pkt.hd.resp = NO_RESP;
	pkt.hd.length = sizeof(pkt.param);
	memcpy(&pkt.param, param, sizeof(pkt.param));
	hwlog_info("set sensor %s (tag:%d) delay %d ms!\n",
		   obj_tag_str[tag] ? obj_tag_str[tag] : "TAG_UNKNOWN", tag, param->period);
	if (use_lock) {
		return inputhub_mcu_write_cmd_adapter(&pkt, sizeof(pkt), NULL);
	} else {
		return inputhub_mcu_write_cmd_nolock(&pkt, sizeof(pkt));
	}
}

int inputhub_sensor_enable_nolock(int tag, bool enable)
{
	return inputhub_sensor_enable_internal(tag, enable, false);
}

int inputhub_sensor_setdelay_nolock(int tag, interval_param_t *interval_param)
{
	return inputhub_sensor_setdelay_internal(tag, interval_param, false);
}

int inputhub_sensor_enable(int tag, bool enable)
{
	return inputhub_sensor_enable_internal(tag, enable, true);
}
int inputhub_sensor_enable_stepcounter(bool enable, type_step_counter_t steptype)
{
	uint8_t app_config[16] = { 0, };

	hwlog_info("TAG_STEP_COUNTER %d, enabe:%d in %s!\n", TAG_STEP_COUNTER, enable, __func__);
	if (!enable) {
		app_config[0] = enable;
		app_config[1] = steptype;
		send_app_config_cmd(TAG_STEP_COUNTER, app_config, true);
	}
	if (really_do_enable_disable(&step_ref_cnt, enable, steptype)) {
		return inputhub_sensor_enable_internal(TAG_STEP_COUNTER, enable, true);
	}
	return 0;
}

int inputhub_sensor_setdelay(int tag, interval_param_t *interval_param)
{
	return inputhub_sensor_setdelay_internal(tag, interval_param, true);
}

int write_customize_cmd(const struct write_info *wr, struct read_info *rd, bool is_lock)
{
	char buf[MAX_PKT_LENGTH];

	if (NULL == wr) {
		hwlog_err("NULL pointer in %s\n", __func__);
		return -EINVAL;
	}

	if (wr->tag < TAG_BEGIN || wr->tag >= TAG_END) {
		hwlog_err("tag = %d error in %s\n", wr->tag, __func__);
		return -EINVAL;
	}
	if (wr->wr_len + sizeof(pkt_header_t) > MAX_PKT_LENGTH) {
		hwlog_err("-----------> wr_len = %d is too large in %s\n", wr->wr_len, __func__);
		return -EINVAL;
	}
	memset(&buf, 0, sizeof(buf));
	((pkt_header_t *) buf)->tag = wr->tag;
	((pkt_header_t *) buf)->cmd = wr->cmd;
	((pkt_header_t *) buf)->resp = ((rd != NULL) ? (RESP) : (NO_RESP));
	((pkt_header_t *) buf)->length = wr->wr_len;
	if (wr->wr_buf != NULL) {
		memcpy(buf + sizeof(pkt_header_t), wr->wr_buf, wr->wr_len);
	}
	if ((wr->tag == TAG_SHAREMEM) && (g_iom3_state == IOM3_ST_REPEAT || g_iom3_state == IOM3_ST_RECOVERY || iom3_power_state == ST_SLEEP))
	{
		return inputhub_mcu_write_cmd_nolock(buf, sizeof(pkt_header_t) + wr->wr_len);
	}
	if (is_lock)
		return inputhub_mcu_write_cmd_adapter(buf, sizeof(pkt_header_t) + wr->wr_len, rd);
	else
		return inputhub_mcu_write_cmd_nolock(buf, sizeof(pkt_header_t) + wr->wr_len);
}

static bool is_fingerprint_data_report(const pkt_header_t* head)
{
    return ((head->tag == TAG_FP) || (head->tag == TAG_FP_UD)) && (CMD_DATA_REQ == head->cmd);
}

static bool is_motion_data_report(const pkt_header_t *head)
{
	/*all sensors report data with command CMD_PRIVATE*/
	return (head->tag == TAG_MOTION) && (CMD_DATA_REQ == head->cmd);
}

static bool is_ca_data_report(const pkt_header_t *head)
{
	/*all sensors report data with command CMD_PRIVATE*/
	return (head->tag == TAG_CA) && (CMD_DATA_REQ == head->cmd);
}

static bool cmd_match(int req, int resp)
{
	return (req + 1) == resp;
}

static bool is_sensor_data_report(const pkt_header_t *head)
{
	/*all sensors report data with command CMD_PRIVATE*/
	return (TAG_SENSOR_BEGIN <= head->tag && head->tag < TAG_SENSOR_END)
	    && (CMD_DATA_REQ == head->cmd);
}

static bool is_fingersense_zaxis_data_report(const pkt_header_t *head)
{
	return (TAG_FINGERSENSE == head->tag) && (CMD_DATA_REQ == head->cmd);
}

static bool is_requirement_resp(const pkt_header_t *head)
{
	return (0 == (head->cmd & 1));	/*even cmds are resp cmd*/
}

static int report_resp_data(const pkt_subcmd_resp_t *head)
{
	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&type_record.lock_spin, flags);
	if (type_record.rd != NULL && type_record.pkt_info != NULL	/*check record info*/
	    && (cmd_match(type_record.pkt_info->cmd, head->hd.cmd))
	    && (type_record.pkt_info->tranid == head->hd.tranid)) {	/*rcv resp from mcu*/
		if (head->hd.length <= (MAX_PKT_LENGTH + sizeof(head->hd.errno) + sizeof(head->subcmd))) {	/*data length ok*/
			type_record.rd->errno = head->hd.errno;	/*fill errno to app*/
			type_record.rd->data_length = (head->hd.length - sizeof(head->hd.errno) - sizeof(head->subcmd));	/*fill data_length to app, data_length means data lenght below*/
			memcpy(type_record.rd->data, (char *)head + sizeof(pkt_subcmd_resp_t), type_record.rd->data_length);	/*fill resp data to app*/
		} else {	/*resp data too large*/
			type_record.rd->errno = -EINVAL;
			type_record.rd->data_length = 0;
			hwlog_err("data too large from mcu in %s\n", __func__);
		}
		complete(&type_record.resp_complete);
	}
	spin_unlock_irqrestore(&type_record.lock_spin, flags);

	return ret;
}

static void init_aod_workqueue(void)
{
	mcu_aod_wq = create_singlethread_workqueue("mcu_aod_workqueue");
}

int __weak dss_sr_of_sh_callback(const pkt_header_t *head)
{
	if (head)
		hwlog_debug("weak dss_sr_of_sh_callback:%d\n", head->cmd);

	return 0;
}

static void mcu_aod_notifier_handler(struct work_struct *work)
{
	/*find data according work*/
	struct mcu_notifier_work *p = container_of(work, struct mcu_notifier_work, worker);
	if (!p || !p->data) {
		hwlog_info("invalid data\n");
		if (p)
			kfree(p);

		return;
	}

	dss_sr_of_sh_callback(p->data);

	kfree(p->data);
	kfree(p);
}

static bool is_aod_notifier(const pkt_header_t *head)
{
	uint32_t *sub_cmd;
	struct mcu_notifier_work *notifier_work;

	if ((head->tag == TAG_AOD) && (CMD_DATA_REQ == head->cmd) && (head->length > 0)) {
		sub_cmd = (uint32_t *)(head + 1);
		if (SUB_CMD_AOD_DSS_ON_REQ == (*sub_cmd)
			|| SUB_CMD_AOD_DSS_OFF_REQ  == (*sub_cmd)) {

			if (head->length > MAX_PKT_LENGTH) {
				hwlog_err("is_aod_notifier:invalid data len\n");
				return true;
			}

			if (SUB_CMD_AOD_DSS_ON_REQ == (*sub_cmd)) {
				hwlog_info("SUB_CMD_AOD_DSS_ON_REQ\n");
				//make sure AOD DSS ON IPC can be handled
				wake_lock_timeout(&wlock, HZ / 2);
			} else {
				hwlog_info("SUB_CMD_AOD_DSS_OFF_REQ\n");
			}

			notifier_work = kzalloc(sizeof(struct mcu_notifier_work), GFP_ATOMIC);
			if (NULL == notifier_work)
				return true;

			notifier_work->data = kzalloc(head->length + sizeof(pkt_header_t), GFP_ATOMIC);
			if (NULL == notifier_work->data) {
				kfree(notifier_work);
				return true;
			}

			memcpy(notifier_work->data, head, sizeof(pkt_header_t) + head->length);
			INIT_WORK(&notifier_work->worker, mcu_aod_notifier_handler);
			queue_work(mcu_aod_wq, &notifier_work->worker);
		}
		return true;
	}
	return false;
}

static void init_mcu_notifier_list(void)
{
	INIT_LIST_HEAD(&mcu_notifier.head);
	spin_lock_init(&mcu_notifier.lock);
	mcu_notifier.mcu_notifier_wq = create_freezable_workqueue("mcu_event_notifier");
}

static void init_mcu_event_wait_list(void)
{
	INIT_LIST_HEAD(&mcu_event_wait_list.head);
	spin_lock_init(&mcu_event_wait_list.slock);
}

static void mcu_notifier_handler(struct work_struct *work)
{
	/*find data according work*/
	struct mcu_notifier_work *p = container_of(work, struct mcu_notifier_work, worker);

	/*search mcu_notifier, call all call_backs*/
	struct mcu_notifier_node *pos, *n;
	list_for_each_entry_safe(pos, n, &mcu_notifier.head, entry) {
		if ((pos->tag == ((const pkt_header_t *)p->data)->tag)
		    && (pos->cmd == ((const pkt_header_t *)p->data)->cmd)) {
			if (pos->notify != NULL) {
				pos->notify((const pkt_header_t *)p->data);
			}
		}
	}

	kfree(p->data);
	kfree(p);
}

static void mcu_notifier_queue_work(const pkt_header_t *head, void (*fn)(struct work_struct *work))
{
	struct mcu_notifier_work *notifier_work = kmalloc(sizeof(struct mcu_notifier_work), GFP_ATOMIC);
	if (NULL == notifier_work)
		return;
	memset(notifier_work, 0, sizeof(struct mcu_notifier_work));
	notifier_work->data = kmalloc(head->length + sizeof(pkt_header_t), GFP_ATOMIC);
	if (NULL == notifier_work->data) {
		kfree(notifier_work);
		return;
	}
	memset(notifier_work->data, 0, head->length + sizeof(pkt_header_t));
	memcpy(notifier_work->data, head, sizeof(pkt_header_t) + head->length);
	INIT_WORK(&notifier_work->worker, fn);
	queue_work(mcu_notifier.mcu_notifier_wq, &notifier_work->worker);
}

static bool is_mcu_notifier(const pkt_header_t *head)
{
	struct mcu_notifier_node *pos, *n;
	unsigned long flags = 0;
	spin_lock_irqsave(&mcu_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &mcu_notifier.head, entry) {
		if ((pos->tag == head->tag) && (pos->cmd == head->cmd))
		{
			spin_unlock_irqrestore(&mcu_notifier.lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&mcu_notifier.lock, flags);
	return false;
}

static bool is_mcu_wakeup(const pkt_header_t *head)
{
	if ((TAG_SYS == head->tag) && (CMD_SYS_STATUSCHANGE_REQ == head->cmd)
	    && (ST_WAKEUP == ((pkt_sys_statuschange_req_t *) head)->status)) {
		return true;
	}
	return false;
}

static bool is_mcu_resume_mini(const pkt_header_t *head)
{
	if ((TAG_SYS == head->tag) && (CMD_SYS_STATUSCHANGE_REQ == head->cmd)
	    	&& ST_MINSYSREADY == ((pkt_sys_statuschange_req_t *) head)->status
	    	&& IOM3_RECOVERY_MINISYS != atomic_read(&iom3_rec_state)) {
		return true;
	}
	return false;
}

static bool is_mcu_resume_all(const pkt_header_t *head)
{
	if ((TAG_SYS == head->tag) && (CMD_SYS_STATUSCHANGE_REQ == head->cmd)
	    	&& ST_MCUREADY == ((pkt_sys_statuschange_req_t *) head)->status
	    	&& IOM3_RECOVERY_MINISYS != atomic_read(&iom3_rec_state)) {
		return true;
	}
	return false;
}

int register_mcu_event_notifier(int tag, int cmd, int (*notify) (const pkt_header_t *head))
{
	struct mcu_notifier_node *pnode, *n;
	int ret = 0;
	unsigned long flags = 0;

	if ((!(TAG_BEGIN <= tag && tag < TAG_END)) || (NULL == notify))
		return -EINVAL;

	spin_lock_irqsave(&mcu_notifier.lock, flags);
	/*avoid regist more than once*/
	list_for_each_entry_safe(pnode, n, &mcu_notifier.head, entry) {
		if ((tag == pnode->tag) && (cmd == pnode->cmd) && (notify == pnode->notify)) {
			hwlog_warn("tag = %d, cmd = %d, notify = %pK has already registed in %s\n!", tag, cmd, notify, __func__);
			goto out;	/*return when already registed*/
		}
	}

	/*make mcu_notifier_node*/
	pnode = kmalloc(sizeof(struct mcu_notifier_node), GFP_ATOMIC);
	if (NULL == pnode) {
		ret = -ENOMEM;
		goto out;
	}
	memset(pnode, 0, sizeof(struct mcu_notifier_node));
	pnode->tag = tag;
	pnode->cmd = cmd;
	pnode->notify = notify;

	/*add to list*/
	list_add(&pnode->entry, &mcu_notifier.head);
out:
	spin_unlock_irqrestore(&mcu_notifier.lock, flags);
	return ret;
}

int unregister_mcu_event_notifier(int tag, int cmd, int (*notify) (const pkt_header_t *head))
{
	struct mcu_notifier_node *pos, *n;
	unsigned long flags = 0;

	if ((!(TAG_BEGIN <= tag && tag < TAG_END)) || (NULL == notify))
		return -EINVAL;

	spin_lock_irqsave(&mcu_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &mcu_notifier.head, entry) {
		if ((tag == pos->tag) && (cmd == pos->cmd) && (notify == pos->notify)) {
			list_del(&pos->entry);
			kfree(pos);
			break;
		}
	}
	spin_unlock_irqrestore(&mcu_notifier.lock, flags);
	return 0;
}

static void step_counter_data_process(pkt_step_counter_data_req_t *head)
{
	int standard_data_len = sizeof(head->step_count);
	valid_step_count = recovery_step_count + head->step_count;
	head->total_step_count = head->step_count = valid_step_count;
	valid_floor_count = recovery_floor_count + head->total_floor_ascend;
	head->total_floor_ascend = valid_floor_count;

	if ((head->record_count > 0) && (head->record_count != EXT_PEDO_VERSION_2) ) {//extend step counter data structure changed
		int extend_effect_len = head->hd.length + sizeof(pkt_header_t) - OFFSET(pkt_step_counter_data_req_t, begin_time); /*skip offset of begin_time*/
		char motion_data[extend_effect_len + 1];	/*reserve 1 byte for motion type*/
		motion_data[0] = MOTIONHUB_TYPE_HW_STEP_COUNTER;	/*add motion type*/
		memcpy(motion_data + 1, &head->begin_time, extend_effect_len);	/*the offset rely on sizeof enum motion_type_t of mcu, now it is 1, we suggest motion_type_t use uint8_t, because sizeof(enum) may diffrernt between AP and mcu;*/
		inputhub_route_write(ROUTE_MOTION_PORT, motion_data, extend_effect_len + 1);	/*report extend step counter date to motion HAL*/
	}

	hwlog_info("convert to standard step counter data to sensor event buffer\n");
	head->hd.length = standard_data_len;	/*avoid report extend data to sensor HAL, convert to standard step counter data, just report member step_count to sensor HAL*/
}

void save_step_count(void)
{
	/*save step count when iom3 recovery*/
	recovery_step_count = valid_step_count;
	recovery_floor_count = valid_floor_count;
}

static bool is_dmd_log_data_report(const pkt_header_t *head)
{
	/*all sensors report data with command CMD_PRIVATE*/
	pkt_dmd_log_report_req_t *buf = (pkt_dmd_log_report_req_t *) head;
	return (TAG_LOG == head->tag) && (CMD_LOG_REPORT_REQ == head->cmd) && (buf->level == LOG_LEVEL_FATAL);
}

static bool is_additional_info_report(const pkt_header_t *head)
{
	return (CMD_CMN_CONFIG_REQ == head->cmd) && (SUB_CMD_ADDITIONAL_INFO == ((pkt_subcmd_req_t *)head)->subcmd);
}
static int sensor_need_reset_power(pkt_dmd_log_report_req_t* pkt)
{
    struct timespec ts;
    int ret = 0;

    if (!need_reset_io_power)
    { goto OUT; }

    get_monotonic_boottime(&ts);

    if ((pkt->dmd_id == DSM_SHB_ERR_MCU_ALS ) && (pkt->dmd_case == DMD_CASE_ALS_NEED_RESET_POWER ))
    {
        pkt->hd.tag = TAG_ALS;
        if ((ts.tv_sec - als_last_reset_period) > RESET_REFRESH_PERIOD)
        {
            als_last_reset_period = ts.tv_sec;
            als_retry_cnt = ALS_RESET_COUNT;
        }

        if (als_retry_cnt)
        {
            als_retry_cnt--;
            ret = 1;
        }
        else
        {
            hwlog_err("als abnormal exceed reset limit\n");
        }
    }
    if ((pkt->dmd_id == DSM_SHB_ERR_MCU_PS ) && (pkt->dmd_case == DMD_CASE_PS_NEED_RESET_POWER ))
    {
        pkt->hd.tag = TAG_PS;
        if ((ts.tv_sec - ps_last_reset_period) > RESET_REFRESH_PERIOD)
        {
            ps_last_reset_period = ts.tv_sec;
            ps_retry_cnt = PS_RESET_COUNT;
        }

        if (ps_retry_cnt)
        {
            ps_retry_cnt--;
            ret = 1;
        }
        else
        {
            hwlog_err("ps abnormal exceed reset limit\n");
        }
    }

OUT:
     return ret;
}

#ifdef CONFIG_HUAWEI_DSM
extern struct dsm_dev dsm_sensorhub;
static void update_client_info(uint8_t dmd_case)
{
	switch(dmd_case) {
		case 0x68:
		case 0x69:
			dsm_sensorhub.ic_name = "AG_BOSCH_INV";
			break;
		case 0x6A:
		case 0x6B:
			dsm_sensorhub.ic_name = "AG_LSM6DS";
			break;
		case 0x0C:
			dsm_sensorhub.ic_name = "MAG_AKM";
			break;
		case 0x0D:
			dsm_sensorhub.ic_name = "MAG_AKM09911";
			break;
		case 0x2E:
			dsm_sensorhub.ic_name = "MAG_YAS537";
			break;
		case 0x38:
		case 0x39:
			dsm_sensorhub.ic_name = "ALS_BH1745";
			break;
		case 0x52:
			dsm_sensorhub.ic_name = "ALS_APDS9251";
			break;
		case 0x53:
			dsm_sensorhub.ic_name = "PS_APDS9110";
			break;
		case 0x1E:
			dsm_sensorhub.ic_name = "PS_PA224";
			break;
		case 0x5C:
			dsm_sensorhub.ic_name = "AIR_LPS22BH";
			break;
		case 0x5D:
			dsm_sensorhub.ic_name = "AIR_LPS_BM";
			break;
		case 0x76:
		case 0x77:
			dsm_sensorhub.ic_name = "AIR_BMP380";
			break;
		case 0x28:
			dsm_sensorhub.ic_name = "SAR_SX9323";
			break;
		case 0x2C:
			dsm_sensorhub.ic_name = "SAR_ADUX1050";
			break;
		case 0x49:
			dsm_sensorhub.ic_name = "TP_FTM4CD56D_ALS_TMD3702";
			break;
		case 0x41:
			dsm_sensorhub.ic_name = "TOF_TFM8701";
			break;
		case 0x29:
			dsm_sensorhub.ic_name = "TOF_GP2AP02VT00F";
			break;

		default:
			hwlog_err("update_client_info uncorrect dmd case %x.\n", dmd_case);
			return;
	}
	hwlog_info("update_client_info ic name is %s.\n", dsm_sensorhub.ic_name);
	dsm_update_client_vendor_info(&dsm_sensorhub);
}
#endif
static void iom7_dmd_log_handle(struct work_struct *work)
{
	struct iom7_log_work *iom7_log = container_of(work, struct iom7_log_work, log_work.work);
	pkt_dmd_log_report_req_t *pkt = (pkt_dmd_log_report_req_t *) iom7_log->log_p;

	hwlog_info("iom7_dmd_log_handle, dmd id is %d.\n", pkt->dmd_id);

#ifdef CONFIG_HUAWEI_DSM
	if (!dsm_client_ocuppy(shb_dclient)) {
		if (pkt->dmd_id == DSM_SHB_ERR_MCU_I2C_ERR || pkt->dmd_id == DSM_SHB_ERR_PS_OIL_POLLUTION)
			update_client_info(pkt->dmd_case);
		dsm_client_record(shb_dclient, "dmd_case = %d", pkt->dmd_case);
		dsm_client_notify(shb_dclient, pkt->dmd_id);
	}
#endif

	if(sensor_need_reset_power(pkt))
	{
	    hwlog_err(" %s reset sensorhub\n",obj_tag_str[pkt->hd.tag]);
	    iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_RESET);
	}

	kfree(iom7_log->log_p);
	kfree(iom7_log);
}

static int report_sensor_event_batch(int tag, int value[], int length, uint64_t timestamp)
{
	struct sensor_data event;
	int64_t ltimestamp = 0;
	struct sensor_data_xyz *sensor_batch_data = (struct sensor_data_xyz *)value;
	if ((!(TAG_FLUSH_META <= tag && tag < TAG_SENSOR_END)) || (length > sizeof(event.value))) {
		hwlog_err("para error (tag : %d), (length : %d) in %s\n", tag, length, __func__);
		return -EINVAL;
	}
	if (TAG_FLUSH_META != tag) {
		ltimestamp = timestamp;
		event.type = tag_to_hal_sensor_type[tag];
		event.length = length;
		memcpy(&event.value, (char *)&(sensor_batch_data->x), event.length);
		sensor_get_data(&event);
	} else {
		ltimestamp = 0;
		event.type = tag_to_hal_sensor_type[tag];
		event.length = 4;
		event.value[0] = tag_to_hal_sensor_type[((pkt_header_t *) value)->tag];
	}
	return inputhub_route_write_batch(ROUTE_SHB_PORT, (char *)&event,
					  event.length + OFFSET_OF_END_MEM(struct sensor_data, length), ltimestamp);
}

#ifdef CONFIG_HW_TOUCH_KEY
extern int touch_key_report_from_sensorhub(int key, int value);
#else
int touch_key_report_from_sensorhub(int key, int value)
{
	return 0;
}
#endif
#ifndef CONFIG_HISI_SYSCOUNTER
int syscounter_to_timespec64(u64 syscnt, struct timespec64 *ts)
{
	return -1;
}
#endif
static int64_t get_sensor_syscounter_timestamp(pkt_batch_data_req_t* sensor_event)
{
    int64_t timestamp;

    timestamp = getTimestamp();

    if (sensor_event->data_hd.data_flag & DATA_FLAG_VALID_TIMESTAMP)
    {
        timestamp = sensor_event->data_hd.timestamp;
    }

    hwlog_debug("sensor %d origin tick %lld transfer timestamp %lld.\n", sensor_event->data_hd.hd.tag, sensor_event->data_hd.timestamp, timestamp);
    return timestamp;
}

static void process_ps_report(const pkt_header_t* head)
{
	pkt_batch_data_req_t* sensor_event = (pkt_batch_data_req_t*) head;

	ps_value = sensor_event->xyz[0].x;
	if(sensor_event->xyz[0].x != 0) {
		wake_lock_timeout(&wlock, HZ);
    		hwlog_info("Kernel get far event!pdata=%d\n", sensor_event->xyz[0].y);
	} else
		hwlog_info("Kernel get near event!!!!pdata=%d\n", sensor_event->xyz[0].y);
}

static void process_tof_report(const pkt_header_t* head)
{
	pkt_batch_data_req_t* sensor_event = NULL;

	if(head == NULL){
		return;
	}
	sensor_event = (pkt_batch_data_req_t*) head;
	tof_value = sensor_event->xyz[0].x;
	//wake_lock_timeout(&wlock, HZ);
	//hwlog_info("Kernel get tof event!!!!data=%d, %d, %d\n", sensor_event->xyz[0].x ,sensor_event->xyz[0].y ,sensor_event->xyz[0].z);
}

static void process_phonecall_report(const pkt_header_t* head)
{
	pkt_batch_data_req_t* sensor_event = (pkt_batch_data_req_t*) head;

	wake_lock_timeout(&wlock, HZ);
	hwlog_info("Kernel get phonecall event! %d %d %d\n", sensor_event->xyz[0].x, sensor_event->xyz[0].y, sensor_event->xyz[0].z);
	if (sensor_event->xyz[0].y == 1 && ps_value != 0)
	{
	    hwlog_info("ps don't get the point!\n");
	}
}

static void process_step_counter_report(const pkt_header_t* head)
{
	wake_lock_timeout(&wlock, HZ);
	hwlog_info("Kernel get pedometer event!\n");
	step_counter_data_process((pkt_step_counter_data_req_t *) head);
	report_sensor_event(head->tag, (int*)(&((pkt_step_counter_data_req_t*) head)->step_count), head->length);
}

static int process_drop_report(const pkt_drop_data_req_t* head)
{
	struct imonitor_eventobj* obj = NULL;
	int ret = 0, yaw = 0, speed = 0, shell = 0, film = 0;

	if (!head) {
		hwlog_err("%s para error\n", __func__);
		return -1;
	}

	hwlog_info("Kernel get drop type %d, initial_speed %d, height %d, angle_pitch %d, angle_roll %d, impact %d\n",
			   head->data.type, head->data.initial_speed, head->data.height, head->data.angle_pitch, head->data.angle_roll, head->data.material);

	obj = imonitor_create_eventobj(SENSOR_DROP_IMONITOR_ID);

	if (!obj) {
		hwlog_err("%s imonitor_create_eventobj failed\n", __func__);
		return -1;
	}

	ret += imonitor_set_param_integer_v2(obj, "Type", (long)(head->data.type));
	ret += imonitor_set_param_integer_v2(obj, "InitSpeed", (long)(head->data.initial_speed));
	ret += imonitor_set_param_integer_v2(obj, "Height", (long)(head->data.height));
	ret += imonitor_set_param_integer_v2(obj, "Pitch", (long)(head->data.angle_pitch));
	ret += imonitor_set_param_integer_v2(obj, "Roll", (long)(head->data.angle_roll));
	ret += imonitor_set_param_integer_v2(obj, "Yaw", (long)(head->data.material));
	ret += imonitor_set_param_integer_v2(obj, "Material", (long)(yaw));
	ret += imonitor_set_param_integer_v2(obj, "Speed", (long)(speed));
	ret += imonitor_set_param_integer_v2(obj, "Shell", (long)(shell));
	ret += imonitor_set_param_integer_v2(obj, "Film", (long)(film));

	if (ret) {
		imonitor_destroy_eventobj(obj);
		hwlog_err("%s imonitor_set_param failed, ret %d\n", __func__, ret);
		return ret;
	}

	ret = imonitor_send_event(obj);

	if (ret < 0) {
		hwlog_err("%s imonitor_send_event failed, ret %d\n", __func__, ret);
	}

	imonitor_destroy_eventobj(obj);
	return ret;
}

static int process_sensors_report(const pkt_header_t* head)
{
	pkt_batch_data_req_t* sensor_event = (pkt_batch_data_req_t*) head;

	switch(head->tag) {
		case TAG_TILT_DETECTOR:
			wake_lock_timeout(&wlock, HZ);
            		hwlog_info("Kernel get TILT_DETECTOR event!=%d\n", sensor_event->xyz[0].x);
			break;
		case TAG_PS:
			process_ps_report(head);
			if (unlikely(stop_auto_ps)) {
				hwlog_info("%s not report ps_data for dt\n", __func__);
				return -1;
			}
			break;
		case TAG_TOF:
			process_tof_report(head);
			//hwlog_info("%s not report tof_data for dt\n", __func__);
			break;
		case TAG_CAP_PROX:
			hwlog_debug("TAG_CAP_PROX!!!!data[0]=%d,data[1]=%d,data[2]=%d.\n",
				sensor_event->xyz[0].x,sensor_event->xyz[0].y,sensor_event->xyz[0].z);
			break;
		case TAG_ACCEL:
			if (unlikely(stop_auto_accel)) {
				hwlog_info("%s not report accel_data for dt\n", __func__);
            			return -1;
			}
			break;
		case TAG_ALS:
			if (unlikely(stop_auto_als)) {
				hwlog_info("%s not report als_data for dt\n", __func__);
            			return -1;
			}
			break;
		case TAG_PRESSURE:
			hwlog_debug("pressure x %x y %x z %x.\n", sensor_event->xyz[0].x, sensor_event->xyz[0].y, sensor_event->xyz[0].z);
            		get_airpress_data = sensor_event->xyz[0].x;
            		get_temperature_data = sensor_event->xyz[0].y;
			break;
		case TAG_CA:
			hwlog_info("ca tag=%d:data length:%d, [data0:%d][data1:%d][data2:%d]",
                       			head->tag, head->length, sensor_event->xyz[0].x, sensor_event->xyz[0].y, sensor_event->xyz[0].y);
			break;
		case TAG_PHONECALL:
			process_phonecall_report(head);
			break;
		case TAG_MAGN_BRACKET:
			hwlog_info("Kernel get magn bracket event %d\n", ((pkt_magn_bracket_data_req_t *)head)->status);
			break;
		case TAG_RPC:
			hwlog_debug("TAG_RPC!data[0]=%d,data[1]=%d,data[2]=%d.\n",
				sensor_event->xyz[0].x,sensor_event->xyz[0].y,sensor_event->xyz[0].z);
			break;
		case TAG_DROP:
			process_drop_report((pkt_drop_data_req_t*)head);
			break;
		default:
			break;
	}
	return 0;
}

static void inputhub_process_sensor_report(const pkt_header_t* head)
{
        uint64_t delta = 0, i = 0;
        int64_t timestamp = 0;
        int64_t head_timestamp = 0;
        int16_t flush_flag = 0;
        pkt_batch_data_req_t* sensor_event = (pkt_batch_data_req_t*) head;

        if (TAG_STEP_COUNTER == head->tag) {  	/*extend step counter date*/
		process_step_counter_report(head);
		flush_flag = ((pkt_step_counter_data_req_t*) head)->data_flag & FLUSH_END;
		if (flush_flag == 1)
			goto flush_event;
		else
			return;
        }

        sensor_read_number[head->tag] += sensor_event->data_hd.cnt;
	timestamp = get_sensor_syscounter_timestamp(sensor_event);
	if ((sensor_event->data_hd.hd.tag == 1) || (sensor_event->data_hd.hd.tag == 2))
		hwlog_debug("sensor %d origin tick %lld transfer timestamp %lld.\n", sensor_event->data_hd.hd.tag, sensor_event->data_hd.timestamp, timestamp);

        if(timestamp <= sensors_tm[head->tag]) {
            timestamp = sensors_tm[head->tag] + 1;
        }

        if (sensor_event->data_hd.cnt < 1) {
		goto flush_event;
        } else if (sensor_event->data_hd.cnt > 1) {
		delta = (uint64_t)(sensor_event->data_hd.sample_rate) * 1000000;
		head_timestamp = timestamp - (sensor_event->data_hd.cnt - 1) * (int64_t)delta;
		if (head_timestamp <= sensors_tm[head->tag]) {
			delta = (timestamp - sensors_tm[head->tag]) / sensor_event->data_hd.cnt;
			timestamp = sensors_tm[head->tag] + delta;
		} else {
			timestamp = head_timestamp;
		}
		for (i = 0; i < sensor_event->data_hd.cnt; i++) {
			report_sensor_event_batch(head->tag, (int*)((char*)head + OFFSET(pkt_batch_data_req_t, xyz)
		                                     +  i *  sensor_event->data_hd.len_element), sensor_event->data_hd.len_element, timestamp);
			timestamp += delta;
		}
		timestamp -= delta;
		sensors_tm[head->tag] = timestamp;
		goto flush_event;
        }

	if (process_sensors_report(head) < 0)
		return;
        report_sensor_event_batch(head->tag, (int*)(sensor_event->xyz), sensor_event->data_hd.len_element, timestamp);
	sensors_tm[head->tag] = timestamp;
flush_event:
	if ((sensor_event->data_hd.data_flag & FLUSH_END) || flush_flag == 1) {
		report_sensor_event_batch(TAG_FLUSH_META, (int*)head, sizeof(pkt_header_t), 0);
        }
}

static void inputhub_process_sensor_report_notifier_handler(struct work_struct *work)
{
	/*find data according work*/
	struct mcu_notifier_work *p = container_of(work, struct mcu_notifier_work, worker);
	inputhub_process_sensor_report((const pkt_header_t *)p->data);
	kfree(p->data);
	kfree(p);
}

static int mcu_reboot_callback(const pkt_header_t *head)
{
	hwlog_err("%s\n", __func__);
	complete(&iom3_reboot);
	return 0;
}

static void update_fingersense_zaxis_data(s16 *buffer, int nsamples)
{
	unsigned long flags = 0;

	if (nsamples < 0) {
		hwlog_err("The second parameter of %s is wrong(negative number)\n", __func__);
		return;
	}
	spin_lock_irqsave(&fsdata_lock, flags);
	memcpy(fingersense_data, buffer, min(nsamples, FINGERSENSE_DATA_NSAMPLES) * sizeof(*buffer));
	fingersense_data_ready = true;
	fingersense_data_intrans = false;
	spin_unlock_irqrestore(&fsdata_lock, flags);
}

static void inputhub_process_additional_info_report(const pkt_header_t* head)
{
	int64_t timestamp = 0;
	pkt_additional_info_req_t* addi_info = NULL;
	struct sensor_data event;
	int info_len = 0;

	addi_info = (pkt_additional_info_req_t*)head;
        timestamp = getTimestamp();
        if (head->tag >= TAG_SENSOR_END) {
            hwlog_err("%s head->tag = %d\n", __func__, head->tag);
            return;
        }
        if (addi_info->serial == 1) {//create a begin event
            event.type = SENSORHUB_TYPE_ADDITIONAL_INFO;
            event.serial = 0;
            event.sensor_type = tag_to_hal_sensor_type[head->tag];
            event.data_type = AINFO_BEGIN;
            event.length = 12;
            inputhub_route_write_batch(ROUTE_SHB_PORT, (char*)&event,
				event.length + OFFSET_OF_END_MEM(struct sensor_data, length), timestamp);
            hwlog_info("###report sensor type %d first addition info event!\n", event.sensor_type);
        }

        info_len = head->length - 8;
        event.type = SENSORHUB_TYPE_ADDITIONAL_INFO;
        event.serial = addi_info->serial;
        event.sensor_type = tag_to_hal_sensor_type[head->tag];
        event.data_type = addi_info->type;
        event.length = info_len + 12;
        memcpy(event.info, addi_info->data_int32, info_len);
        inputhub_route_write_batch(ROUTE_SHB_PORT, (char*)&event,
			event.length + OFFSET_OF_END_MEM(struct sensor_data, length), timestamp);
        hwlog_info("report sensor type %d addition info: %d !\n", event.sensor_type, event.info[0]);

        if (addi_info->end == 1) {
            event.type = SENSORHUB_TYPE_ADDITIONAL_INFO;
            event.serial = ++addi_info->serial;
            event.sensor_type = tag_to_hal_sensor_type[head->tag];
            event.data_type = AINFO_END;
            event.length = 12;
            inputhub_route_write_batch(ROUTE_SHB_PORT, (char*)&event,
				event.length + OFFSET_OF_END_MEM(struct sensor_data, length), timestamp);
            hwlog_info("***report sensor_type %d end addition info event!***\n", event.sensor_type);
        }
}

static int inputhub_process_dmd_log_report(const pkt_header_t* head)
{
	struct iom7_log_work* work;

	hwlog_info("[iom7]dmd_log_data_report");
        work = kmalloc(sizeof(struct iom7_log_work), GFP_ATOMIC);
        if (!work)
        { return -ENOMEM; }

        memset(work, 0, sizeof(struct iom7_log_work));
        work->log_p = kmalloc(head->length + sizeof(pkt_header_t), GFP_ATOMIC);
        if (!work->log_p) {
            kfree(work);
            return -ENOMEM;
        }
        memset(work->log_p, 0, head->length + sizeof(pkt_header_t));
        memcpy(work->log_p, head, head->length + sizeof(pkt_header_t));
        INIT_DELAYED_WORK(&(work->log_work), iom7_dmd_log_handle);
        queue_delayed_work(system_power_efficient_wq, &(work->log_work), msecs_to_jiffies(250));
	return 0;
}

static int inputhub_process_fingerprint_report(const pkt_header_t* head)
{
	char* fingerprint_data = NULL;
	const fingerprint_upload_pkt_t* fingerprint_data_upload = (const fingerprint_upload_pkt_t*)head;

	if (NULL == fingerprint_data_upload) {
		hwlog_err("%s fingerprint_data_upload is NULL\n", __func__);
		return -EINVAL;
	}

	wake_lock_timeout(&wlock, 2 * HZ);

	hwlog_info("fingerprint: %s: tag = %d, data:%d\n", __func__, fingerprint_data_upload->fhd.hd.tag, fingerprint_data_upload->data);
	fingerprint_data = (char*)fingerprint_data_upload + sizeof(pkt_common_data_t);

	fingerprint_ipc_cgbe_abort_handle();

	if (TAG_FP == fingerprint_data_upload->fhd.hd.tag)
	{
		return inputhub_route_write(ROUTE_FHB_PORT, fingerprint_data, sizeof(fingerprint_data_upload->data));
	} else {
		return inputhub_route_write(ROUTE_FHB_UD_PORT, fingerprint_data, sizeof(fingerprint_data_upload->data));
	}
}

static int inputhub_process_motion_report(const pkt_header_t* head)
{
	char* motion_data = (char*)head + sizeof(pkt_common_data_t);

	if ((((int)motion_data[0]) == MOTIONHUB_TYPE_TAKE_OFF) || (((int)motion_data[0]) == MOTIONHUB_TYPE_PICKUP))
        {
            wake_lock_timeout(&wlock, HZ);
            hwlog_err("%s weaklock HZ motiontype = %d \n", __func__, motion_data[0]);
        }

	hwlog_info("%s : motiontype = %d motion_result = %d, motion_status = %d\n", __func__, motion_data[0],motion_data[1],motion_data[2]);
	return inputhub_route_write(ROUTE_MOTION_PORT, motion_data, head->length-(sizeof(pkt_common_data_t) - sizeof(pkt_header_t)));
}

int inputhub_route_recv_mcu_data(const char *buf, unsigned int length)
{
	const pkt_header_t* head = (const pkt_header_t*)buf;
	bool is_notifier = false;

	head = pack(buf, length, &is_notifier);

	if (NULL == head)
		{ return 0; }	/*receive next partial package.*/

	if (head->tag < TAG_BEGIN || head->tag >= TAG_END)
	{
		hwlog_err("---------------------->head value : tag=%#.2x, cmd=%#.2x, length=%#.2x in %s\n",
		head->tag, head->cmd, head->length, __func__);
		return -EINVAL;
	}


	++ipc_debug_info.event_cnt[head->tag];
	wake_up_mcu_event_waiter(head);

	if (is_mcu_resume_mini(head)) {
		hwlog_err("%s MINI ready\n", __func__);
		resume_skip_flag = RESUME_MINI;
		barrier();
		complete(&iom3_resume_mini);
	}
	if (is_mcu_resume_all(head)) {
		hwlog_err("%s ALL ready\n", __func__);
		complete(&iom3_resume_all);
	}
	if (is_mcu_wakeup(head)) {
		if (RESUME_MINI != resume_skip_flag) {
			resume_skip_flag = RESUME_SKIP;
			barrier();
			complete(&iom3_resume_mini);
			complete(&iom3_resume_all);
		}
		mcu_reboot_callback(head);
	}
	if (is_aod_notifier(head)) {
		is_notifier = true;
	}
	if (is_mcu_notifier(head)) {
		mcu_notifier_queue_work(head, mcu_notifier_handler);
		is_notifier = true;
	}
	if (is_fingerprint_data_report(head)) {
		return inputhub_process_fingerprint_report(head);
	} else if (is_fingersense_zaxis_data_report(head)) {
		pkt_fingersense_data_report_req_t* zaxis_report = (pkt_fingersense_data_report_req_t*) head;
		update_fingersense_zaxis_data(zaxis_report->zaxis_data, zaxis_report->hd.length /sizeof(s16));
	} else if (is_sensor_data_report(head)) {

		if(head->tag == TAG_PS){
			hwlog_info("hold lock  avoid system suspend for ps");
			//hold wakelock 1ms avoid system suspend
			wake_lock_timeout(&wlock, 1);
		}
	#ifdef CONFIG_CONTEXTHUB_SHMEM
		mcu_notifier_queue_work(head, inputhub_process_sensor_report_notifier_handler);
	#else
		inputhub_process_sensor_report(head);
	#endif
	} else if (is_additional_info_report(head)) {
		inputhub_process_additional_info_report(head);
	} else if (is_dmd_log_data_report(head)) {
		inputhub_process_dmd_log_report(head);
	} else if (is_requirement_resp(head)) {
		return report_resp_data((const pkt_subcmd_resp_t*)head);
	} else if (is_motion_data_report(head)) {
		return inputhub_process_motion_report(head);
	} else if (is_ca_data_report(head)) {
		char* ca_data = (char*)head + sizeof(pkt_common_data_t);
		return inputhub_route_write(ROUTE_CA_PORT, ca_data, head->length);
	} else if (head->tag == TAG_KEY) {
		uint16_t key_datas[12] = { 0 };
		pkt_batch_data_req_t *key_data = (pkt_batch_data_req_t *)head;
		memcpy(key_datas, key_data->xyz, sizeof(key_datas));
		hwlog_info("status:%2x\n", key_datas[0]);
		touch_key_report_from_sensorhub((int)key_datas[0], 0);
	} else {
		if (!is_notifier) {
			hwlog_err("--------->tag = %d, cmd = %d is not implement!\n", head->tag, head->cmd);
			fobidden_comm = 1;
			return -EINVAL;
		}
	}
	return 0;
}

static void init_locks(void)
{
	int i;
	for (i = 0; i < sizeof(package_route_tbl) / sizeof(package_route_tbl[0]); ++i) {
		spin_lock_init(&package_route_tbl[i].buffer_spin_lock);
	}
	mutex_init(&mutex_write_cmd);
	mutex_init(&mutex_write_adapter);
	mutex_init(&mutex_unpack);
	init_completion(&type_record.resp_complete);
	mutex_init(&type_record.lock_mutex);
	spin_lock_init(&type_record.lock_spin);
	spin_lock_init(&ref_cnt_lock);
	/* Initialize wakelock */
	wake_lock_init(&wlock, WAKE_LOCK_SUSPEND, "sensorhub");
}

void inputhub_route_init(void)
{
	init_locks();
	init_mcu_notifier_list();
	init_mcu_event_wait_list();
	init_aod_workqueue();
#ifdef CONFIG_CONTEXTHUB_SHMEM
	if(contexthub_shmem_init())
	    hwlog_err("failed to init shmem\n");
	else
	    hwlog_info("shmem ipc init done\n");
#endif
}

static void close_all_ports(void)
{
	int i;
	for (i = 0; i < sizeof(package_route_tbl) / sizeof(package_route_tbl[0]); ++i) {
		inputhub_route_close(package_route_tbl[i].port);
	}
}

void inputhub_route_exit(void)
{
	/*close all ports*/
	close_all_ports();
	wake_lock_destroy(&wlock);
}
