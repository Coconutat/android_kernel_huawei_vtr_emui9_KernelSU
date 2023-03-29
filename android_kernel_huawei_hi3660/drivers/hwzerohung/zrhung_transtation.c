/**********************************************************
 * Filename:    zrhung_transtation.c
 *
 * Discription: kernel process of hung event
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: fanghua(00396017) zhangliang(00175161)
 *
**********************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <uapi/linux/uio.h>
#include <asm/ioctls.h>
#ifdef CONFIG_HISILICON_PLATFORM
#include <linux/hisi/hisi_bootup_keypoint.h>
#endif

#include <linux/errno.h>
#include "zrhung_lastword.h"
#include "zrhung_transtation.h"
#include <chipset_common/hwzrhung/zrhung.h>
#include "zrhung_common.h"
#ifdef CONFIG_ARCH_QCOM
#define wp_get_sochalt(x)
#define get_sr_position_from_fastboot(x, y)
#else
#include "watchpoint/zrhung_wp_sochalt.h"
#endif

#ifdef GTEST
#define static
#define inline
#endif

//#define HTRANS_DEBUG

#define HTRANS_RECORD_EVENT_MAGIC			0x676e7548		// "Hung"

#define ADDED_HEADER_SIZE					(sizeof(zrhung_trans_event)-sizeof(zrhung_write_event))

#define HTRANS_WRITE_EVENT_BUF_SIZE_MAX 	(16*1024)
#define HTRANS_TRANS_EVENT_BUF_SIZE_MAX 	(HTRANS_WRITE_EVENT_BUF_SIZE_MAX + ADDED_HEADER_SIZE)
#define HTRANS_RING_BUF_SIZE_MAX 			(HTRANS_TOTAL_BUF_SIZE - 1024)
#define SR_POSITION_LEN                                (64)

typedef enum HTRANS_STAGE {
	HTRANS_STAGE_HTRANS_NOT_INIT,
	HTRANS_STAGE_LASTWORD_NOT_INIT,
	HTRANS_STAGE_LASTWORD_READING,
	HTRANS_STAGE_LASTWORD_FINISHED
} HTRANS_STAGE;

static uint8_t g_buf[HTRANS_WRITE_EVENT_BUF_SIZE_MAX];

/*
 * zrhung_write_event: event data from watch point
 * zrhung_read_event: event data to hung guard. it would add some msg based on zrhung_write_event
 * zrhung_trans_event: internal event data, it would include zrhung_read_event
 */
typedef struct{
	uint32_t 		magic;
	uint16_t 		len;
	uint16_t 		wp_id;
	uint32_t 		timestamp;
	uint16_t 		no;
	uint16_t 		reserved;
	uint32_t 		pid;
	uint32_t 		tgid;
	uint16_t 		cmd_len; 	// end with '\0', = cmd actual len + 1
	uint16_t 		msg_len; 	// end with '\0', = msg actual len + 1
	char info[0];				// <cmd buf>0<buf>0
} zrhung_read_event;

typedef struct {
	uint32_t		magic;
	uint16_t		len;
	uint16_t		reserved;
	zrhung_read_event	evt;
} zrhung_trans_event;

typedef struct {
	uint32_t	w;
	uint32_t	r;
} zrhung_trans_pos;

static DEFINE_SPINLOCK(htrans_lock);
static DEFINE_MUTEX(htrans_mutex);
static wait_queue_head_t	htrans_wq;
static char* 				htrans_buf;			// 64K
static zrhung_trans_pos *		htrans_pos;			// in the end of htrans_buf
static uint16_t 			htrans_no;
static HTRANS_STAGE			htrans_stage;

static inline void htrans_print_event(char* msg, zrhung_trans_event* te)
{
#ifdef HTRANS_DEBUG
	HTRANS_INFO("(%s) (w:%d, r:%d)(magic:0x%x, len: %d), (magic:0x%x, len:%d, wp_id:%d, timestamp:%d, no:%d, pid:%d, tgid:%d, cmd_len:%d, msg_len:%d)\n",
		msg, htrans_pos->w, htrans_pos->r, te->magic, te->len, te->evt.magic, te->evt.len,
		te->evt.wp_id, te->evt.timestamp, te->evt.no, te->evt.pid, te->evt.tgid, te->evt.cmd_len, te->evt.msg_len);
#endif
}

static inline uint32_t htrans_gettime(void)
{
	uint32_t second;
	struct timeval tv;
	memset(&tv, 0, sizeof(struct timeval));
	do_gettimeofday(&tv);
	second = (uint32_t)tv.tv_sec;

	return second;
}

static inline uint32_t htrans_get_pos(uint32_t pos, uint32_t len)
{
	uint32_t new_pos = pos + len;

	if (new_pos >= HTRANS_RING_BUF_SIZE_MAX) {
		new_pos -= HTRANS_RING_BUF_SIZE_MAX;
	}

	return new_pos;
}

static int htrans_write_from_kernel(uint32_t offset, void* k_data, uint32_t len)
{
	uint32_t part_len;

	if (NULL == k_data
		|| len > HTRANS_TRANS_EVENT_BUF_SIZE_MAX
		|| offset >= HTRANS_RING_BUF_SIZE_MAX) {

		HTRANS_ERROR("offset: %d, dst:%p, len:0x%x\n", offset, k_data, len);

		return -1;
	}

	part_len = min((size_t)len, (size_t)(HTRANS_RING_BUF_SIZE_MAX - offset));
	memcpy(htrans_buf + offset, k_data, part_len);

	if (part_len < len) {
		memcpy(htrans_buf, k_data + part_len, len - part_len);
	}

	return 0;
}

static inline int htrans_is_full(uint32_t len)
{
	if (htrans_pos->w > htrans_pos->r) {
		return (htrans_pos->w + len - htrans_pos->r >= HTRANS_RING_BUF_SIZE_MAX);
	} else if (htrans_pos->w < htrans_pos->r) {
		return (htrans_pos->w + len >= htrans_pos->r);
	} else {
		return 0;
	}
}

static inline int htrans_is_empty(void)
{
	return (htrans_pos->w == htrans_pos->r);
}

static int htrans_create_trans_event(zrhung_trans_event* te, zrhung_write_event* we)
{
	if (!te || !we) {
		HTRANS_ERROR("param error\n");
		return -1;
	}

	te->magic = HTRANS_RECORD_EVENT_MAGIC;
	te->len = we->len + ADDED_HEADER_SIZE;

	te->evt.magic = we->magic;
	te->evt.len = we->len + sizeof(zrhung_read_event) - sizeof(zrhung_write_event);
	te->evt.wp_id = we->wp_id;
	te->evt.timestamp = htrans_gettime();
	te->evt.no = htrans_no++;
	te->evt.pid = current->pid;
	te->evt.tgid = current->tgid;
	te->evt.cmd_len = we->cmd_len;
	te->evt.msg_len = we->msg_len;

	return 0;
}

static int htrans_read_trans_event(zrhung_trans_event* te, uint32_t pos)
{
	uint32_t part_len;

	if (!te || pos >= HTRANS_RING_BUF_SIZE_MAX) {
		HTRANS_ERROR("param error\n");
		return -1;
	}

	memset(te, 0, sizeof(zrhung_trans_event));

	// copy data to te
	part_len = min((size_t)sizeof(zrhung_trans_event), (size_t)(HTRANS_RING_BUF_SIZE_MAX - pos));
	memcpy(te, htrans_buf + pos, part_len);

	if (part_len < sizeof(zrhung_trans_event)) {
		memcpy((char*)te + part_len, htrans_buf, sizeof(zrhung_trans_event) - part_len);
	}

	// check te data
	if (te->magic != HTRANS_RECORD_EVENT_MAGIC
		|| te->len > HTRANS_TRANS_EVENT_BUF_SIZE_MAX
		|| te->len <= sizeof(zrhung_trans_event)
		|| te->evt.len > HTRANS_WRITE_EVENT_BUF_SIZE_MAX
		|| te->evt.len <= sizeof(zrhung_read_event)) {

		HTRANS_ERROR("check tmp buffer failed\n");
		return -1;
	}

	return 0;
}

static int htrans_check_valid(void)
{
	uint32_t r = htrans_pos->r, w = htrans_pos->w;
	zrhung_trans_event te = {0};

	if (w >= HTRANS_RING_BUF_SIZE_MAX || r >= HTRANS_RING_BUF_SIZE_MAX) {
		HTRANS_ERROR("w:0x%xr:0x%x\n", w, r);
		return 0;
	}

	while (r > w) {
		if (htrans_read_trans_event(&te, r)) {
			HTRANS_ERROR("htrans_read_trans_event error\n");
			return 0;
		}

		r = htrans_get_pos(r, te.len);
	}

	while (r < w) {
		if (htrans_read_trans_event(&te, r)) {
			HTRANS_ERROR("htrans_read_trans_event error\n");
			return 0;
		}

		r = htrans_get_pos(r, te.len);
	}

	return (r == w);
}

static int htrans_read_one_event(void __user *u_data)
{
	uint32_t pos, part_len;
	zrhung_trans_event te = {0};

	if (htrans_is_empty()) {
		HTRANS_ERROR("buffer empty\n");
		return -1;
	}

	// read trans_event
	if (htrans_read_trans_event(&te, htrans_pos->r)) {
		HTRANS_ERROR("htrans_read_trans_event error\n");
		return -1;
	}

	// get read_event pos
	pos = htrans_get_pos(htrans_pos->r, sizeof(zrhung_trans_event)-sizeof(zrhung_read_event));

	// copy data to user
	part_len = min((size_t)te.evt.len, (size_t)(HTRANS_RING_BUF_SIZE_MAX - pos));
	if (copy_to_user(u_data, htrans_buf + pos, part_len)) {
		HTRANS_ERROR("copy_to_user error\n");
		return -1;
	}

	if (part_len < te.evt.len) {
		if (copy_to_user(u_data + part_len, htrans_buf, te.evt.len - part_len)) {
			HTRANS_ERROR("copy_to_user error\n");
			return -1;
		}
	}

	// change r
	htrans_pos->r = htrans_get_pos(htrans_pos->r, te.len);

	htrans_print_event("read_event", &te);

	return 0;
}

static long htrans_write_event_internal(void* kernel_event)
{
	uint32_t len, new_w;
	long ret = 0;
	zrhung_write_event *we = kernel_event;
	zrhung_trans_event te = {0};
	unsigned long flags;

	if (!kernel_event) {
		HTRANS_ERROR("param error\n");
		return -ECANCELED;
	}


	// can not start if last word was not read back
	if (HTRANS_STAGE_LASTWORD_FINISHED != htrans_stage) {
		HTRANS_ERROR("lastword not read\n");
		return -ECANCELED;
	}

	// change the write event to trans event
	htrans_create_trans_event(&te, we);

	spin_lock_irqsave(&htrans_lock, flags);
	// check whether buffer if full
	if (htrans_is_full(te.len)) {
		HTRANS_ERROR("[hung buffer full] wp_id: %u, pid: %u, tgid: %u, no: %u, len: %u, timestamp: %u\n",
			te.evt.wp_id, te.evt.pid, te.evt.tgid, te.evt.no, we->len, te.evt.timestamp);

		ret = -ECANCELED;
		goto end;
	}

	// write head into buffer
	htrans_write_from_kernel(htrans_pos->w, (char*)&te, sizeof(te));

	// write data into buffer
	new_w = htrans_get_pos(htrans_pos->w, sizeof(te));
	len = we->len - sizeof(zrhung_write_event);
	htrans_write_from_kernel(new_w, kernel_event + sizeof(zrhung_write_event), len);

	// move w
	htrans_pos->w = htrans_get_pos(htrans_pos->w, te.len);

	spin_unlock_irqrestore(&htrans_lock, flags);

	htrans_print_event("write_event", &te);

	wake_up_interruptible(&htrans_wq);

	return 0;

end:
	spin_unlock_irqrestore(&htrans_lock, flags);
	return ret;
}

long htrans_write_event(void __user *argp)
{
	zrhung_write_event *we;
	int ret;

	if(!argp)
		return -EINVAL;

	if(mutex_lock_interruptible(&htrans_mutex))
		return -1;

	we = (zrhung_write_event *)g_buf; 

	if (copy_from_user((void*)we, argp, sizeof(zrhung_write_event))) {
		HTRANS_ERROR("can not copy data from user space\n");
		mutex_unlock(&htrans_mutex);	/*lint !e455 */
		return -1;
	}

	if (we->len <= sizeof(zrhung_write_event)
			|| we->len > HTRANS_WRITE_EVENT_BUF_SIZE_MAX) {
		HTRANS_ERROR("length wrong: %d\n", we->len);
		mutex_unlock(&htrans_mutex);	/*lint !e455 */
		return -1;
	}

	if (copy_from_user(g_buf+sizeof(zrhung_write_event),
				argp+sizeof(zrhung_write_event),
				we->len-sizeof(zrhung_write_event))) {
		HTRANS_ERROR("can not copy data from user space\n");
		mutex_unlock(&htrans_mutex);	/*lint !e455 */
		return -1;
	}

	ret = htrans_write_event_internal(g_buf);

	mutex_unlock(&htrans_mutex);	/*lint !e455 */

	return ret;
}

long htrans_write_event_kernel(void *argp)
{
	return htrans_write_event_internal(argp);
}

long htrans_read_event(void __user *argp)
{
	long ret = 0;

	if (!argp) {
		HTRANS_ERROR("param error\n");
		return -ECANCELED;
	}

	if(mutex_lock_interruptible(&htrans_mutex)) {
		return -1;
	}

	if (htrans_is_empty()) {
		HTRANS_INFO("buffer empty\n");
		mutex_unlock(&htrans_mutex);	/*lint !e455 */
		wait_event_interruptible(htrans_wq, htrans_pos->w != htrans_pos->r);
		if(mutex_lock_interruptible(&htrans_mutex)) {
			return -1;
		}
	}

	if (HTRANS_STAGE_LASTWORD_FINISHED != htrans_stage) {
		HTRANS_ERROR("lastword not read\n");
		ret = -ECANCELED;
		goto end;
	}

	if (htrans_read_one_event(argp)) {
		HTRANS_ERROR("buffer empty\n");
		ret = -ECANCELED;
		goto end;
	}

end:
	mutex_unlock(&htrans_mutex);	/*lint !e455 */
	return ret;
}

int htrans_get_sochalt(void __user*argp)
{
	zrhung_write_event we = {0};
	zrhung_read_event *re = NULL;
	char sr_position_msg[SR_POSITION_LEN] = {0};
	static int is_sochalt_read = 0;

	// read soc halt watchpoint
	if (is_sochalt_read) {
		HTRANS_INFO("already read\n");
		return -1;
	}

	is_sochalt_read = 1;

	wp_get_sochalt(&we);
	if (0 == we.wp_id || 0 == we.len) {
		HTRANS_INFO("no soc halt\n");
		return -1;
	}

	memset(sr_position_msg, 0, sizeof(sr_position_msg));
	get_sr_position_from_fastboot(sr_position_msg,sizeof(sr_position_msg));

	re = (zrhung_read_event*)kzalloc(sizeof(zrhung_read_event) + strlen(sr_position_msg) + 2, GFP_KERNEL);
	if (NULL == re) {
		HTRANS_ERROR("alloc fail\n");
		return -1;
	}

	re->magic = we.magic;
	re->len = sizeof(zrhung_read_event) + strlen(sr_position_msg) + 2;
	re->wp_id = we.wp_id;
	re->timestamp = htrans_gettime();
	re->no = 0;
	re->pid = 0;
	re->tgid = 0;
	re->cmd_len = 1;
	re->msg_len = strlen(sr_position_msg) + 1;

	if ( strlen(sr_position_msg) > 0){
		strncpy((char *)re + sizeof(zrhung_read_event) + re->cmd_len, sr_position_msg, strlen(sr_position_msg));
	}

	if (copy_to_user(argp, re, re->len)) {
		HTRANS_ERROR("copy_to_user error\n");
		kfree(re);
		return -1;
	}
	HTRANS_INFO("soc halt\n");

	kfree(re);
	return 0;
}

long htrans_read_lastword(void __user *argp)
{
	if (!argp) {
		HTRANS_ERROR("param error\n");
		return 1;
	}

	if(mutex_lock_interruptible(&htrans_mutex))
		return -1;

	if (HTRANS_STAGE_HTRANS_NOT_INIT == htrans_stage
		|| HTRANS_STAGE_LASTWORD_FINISHED == htrans_stage) {

		HTRANS_ERROR("stage error: %d\n", htrans_stage);
		mutex_unlock(&htrans_mutex);	/*lint !e455 */
		return 1;
	}

	// lastword not be read
	if (HTRANS_STAGE_LASTWORD_NOT_INIT == htrans_stage) {
		/* unlock before read data from dfx partition */
		if (hlastword_read(htrans_buf, HTRANS_TOTAL_BUF_SIZE)) {
			HTRANS_ERROR("hlastword_read error\n");
			goto finish;
		}

		HTRANS_INFO("r:0x%x, w:0x%x\n", htrans_pos->r, htrans_pos->w);

		// if events are not valid, return 1 to finish reading last word
		if (!htrans_check_valid()) {
			HTRANS_ERROR("htrans_check_valid error\n");
			goto finish;
		}

		htrans_stage = HTRANS_STAGE_LASTWORD_READING;
	}

	// read lastword first,
	// then read sochalt info,
	// otherwise, finish
	if (htrans_read_one_event(argp)) {
		if (htrans_get_sochalt(argp)) {
			goto finish;
		}
	}

	mutex_unlock(&htrans_mutex);	/*lint !e455 */

	return 0;

finish:
	memset(htrans_buf, 0, HTRANS_TOTAL_BUF_SIZE);
	hlastword_write(htrans_buf, HTRANS_TOTAL_BUF_SIZE);
	htrans_stage = HTRANS_STAGE_LASTWORD_FINISHED;
	mutex_unlock(&htrans_mutex);	/*lint !e455 */

	//get LONGPRESS event "AP_S_PRESS6S" from reboot_reason
	zrhung_get_longpress_event();

	// return 1 here to notify lastword reading finished
	return 1;
}

long htrans_save_lastword(void)
{
	long ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&htrans_lock, flags);

	if (HTRANS_STAGE_LASTWORD_FINISHED != htrans_stage) {
		HTRANS_ERROR("transtion not init\n");
		ret = -ECANCELED;
		spin_unlock_irqrestore(&htrans_lock, flags);
		goto end;
	}

	if (HTRANS_STAGE_HTRANS_NOT_INIT == htrans_stage) {
		spin_unlock_irqrestore(&htrans_lock, flags);
		goto end;
	}

	htrans_stage = HTRANS_STAGE_HTRANS_NOT_INIT;

	/* unlock spinlock before write lastword to disk */
	spin_unlock_irqrestore(&htrans_lock, flags);

	hlastword_write(htrans_buf, HTRANS_TOTAL_BUF_SIZE);

	/* change stage back to HTRANS_STAGE_LASTWORD_FINISHED */
	spin_lock_irqsave(&htrans_lock, flags);
	htrans_stage = HTRANS_STAGE_LASTWORD_FINISHED;
	spin_unlock_irqrestore(&htrans_lock, flags);

end:
	return ret;
}

long zrhung_save_lastword(void)
{
#ifdef CONFIG_DETECT_HUAWEI_HUNG_TASK
#ifdef CONFIG_HISILICON_PLATFORM
	if (STAGE_BOOTUP_END == get_boot_keypoint()) {
		return htrans_save_lastword();
	} else {
		return 0;
	}
#else
	return 0;
#endif
#endif
}
EXPORT_SYMBOL(zrhung_save_lastword);

static int __init htrans_init(void)
{
	int ret = 0;

	ret = hlastword_init();
	if (0 != ret) {
		HTRANS_ERROR("hlastword_init error\n");
		goto _error;
	}

	// malloc hungevent buffer
	htrans_buf = (char*)vmalloc(HTRANS_TOTAL_BUF_SIZE);
	if (IS_ERR_OR_NULL(htrans_buf)) {
		HTRANS_ERROR("transtion not init\n");
		ret = -ENOMEM;
		goto _error;
	}
	memset(htrans_buf, 0, HTRANS_TOTAL_BUF_SIZE);

	HTRANS_INFO("malloc buf: %p, size: 0x%x\n", htrans_buf, HTRANS_TOTAL_BUF_SIZE);

	// trans pos information is in the end of htrans_buf
	htrans_pos = (zrhung_trans_pos*)(htrans_buf+HTRANS_TOTAL_BUF_SIZE-sizeof(zrhung_trans_pos));
	HTRANS_INFO("pos: %p\n", htrans_pos);

	init_waitqueue_head(&htrans_wq);

	// set stage
	htrans_stage = HTRANS_STAGE_LASTWORD_NOT_INIT;

	return 0;

_error:
	if (htrans_buf) {
		vfree(htrans_buf);
	}

	return ret;
}

static void __exit htrans_exit(void)
{
	if (htrans_buf) {
		vfree(htrans_buf);
	}
}

module_init(htrans_init);
module_exit(htrans_exit);

MODULE_LICENSE("GPL");
