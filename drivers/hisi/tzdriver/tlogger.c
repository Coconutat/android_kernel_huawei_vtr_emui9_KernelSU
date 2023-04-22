
/*******************************************************************************
 * This source code has been made available to you by HUAWEI on an
 * AS-IS basis. Anyone receiving this source code is licensed under HUAWEI
 * copyrights to use it in any way he or she deems fit, including copying it,
 * modifying it, compiling it, and redistributing it either with or without
 * modifications. Any person who transfers this source code or any derivative
 * work must include the HUAWEI copyright notice and this paragraph in
 * the transferred software.
 ******************************************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <linux/delay.h>
#include <asm/ioctls.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/syscalls.h>
#include "tee_rdr_register.h"
#include <securec.h>
#include "tc_ns_log.h"

/* ----------------------------------------------- */
/* for log item -----------------------------------*/
#define LOG_ITEM_MAGIC	0x5A5A
#define LOG_ITEM_LEN_ALIGN 64
#define LOG_ITEM_MAX_LEN 1024

#define LOG_READ_STATUS_ERROR 0x000FFFF

enum {
	LOG_OPER_NEW,
	LOG_OPER_READ,
};
/* 64字节头部 + 用户日志 */
typedef struct {
	uint8_t		never_used[32];

	uint16_t	magic;
	uint16_t	log_buffer_crc;

	uint32_t	serial_no;

	int16_t		log_real_len;	/* 日志的实际长度 */
	uint16_t	log_buffer_len; /* 日志buffer的长度 32字节的倍数 */

	uint8_t		uuid[16];

	uint8_t		reserved[3];
	uint8_t		new_line;		/* 为\n换行符，在bbox.bin中比较容易查看日志 */


	uint8_t		log_buffer[0];
}LOG_ITEM_T;

LOG_ITEM_T *m_logitem;


/* ================================================= */

/* ------------------------------------------------- */
/* --- for rdr mem --------------------------------- */

#define TEMP_RDR_MEM_SIZE   (10*1024)


/* 日志的标签区， 共64字节 + 156字节的版本信息 */
typedef struct {
	uint16_t	crc;
	uint16_t	reserved0;

	uint32_t	last_pos;		/* 当前日志的末尾位置， 下一次写的开始位置 */
	uint32_t	write_loops;	/* 循环写入，每写满一次从头重写的时候， 此值+1， 第一遍写入的时候为0 */
	uint32_t	log_level;		/* 日志级别 */
	uint32_t	reserved1;

	uint32_t	reserved2[4];
	uint32_t	reserved3[4];

	uint32_t	reserved4[2];
	uint32_t	max_len;

	uint8_t		version_info[156];  /* 在第20个字节开始有4个字节用于TEE INOUT STATUS */

}LOG_BUFFER_FLAG_T;

typedef struct {
	LOG_BUFFER_FLAG_T	flag;
	uint8_t		buffer_start[0];
}LOG_BUFFER_T;

static LOG_BUFFER_T *m_logbuffer;
uint32_t g_lastread_offset = 0;


/* =================================================== */


/*#define TC_DEBUG*/
#define LOGGER_LOG_TEEOS               "hisi_teelog"	/* tee os log  */

#define __TEELOGGERIO	0xBE	/* for ioctl */
/* get tee verison */
#define TEELOGGER_GET_VERSION		_IOR(__TEELOGGERIO, 5, char[256])

/* set the log reader pos to current pos */
#define TEELOGGER_SET_READERPOS_CUR		_IO(__TEELOGGERIO, 6)
#define TEELOGGER_SET_TLOGCAT_STAT _IO(__TEELOGGERIO, 7)
#define TEELOGGER_GET_TLOGCAT_STAT _IO(__TEELOGGERIO, 8)
int g_tlogcat_f = 0;

#define LOG_PATH_TEE_LOG_FILE	"/data/log/tee/last_teemsg"

#define ROOT_UID                0
#define SYSTEM_GID              1000

/**
 * struct logger_log - represents a specific log, such as 'main' or 'radio'
 * @buffer:	The actual ring buffer
 * @misc:	The "misc" device representing the log
 * @wq:		The wait queue for @readers
 * @readers:	This log's readers
 * @mutex:	The mutex that protects the @buffer
 * @w_off:	The current write head offset
 * @head:	The head, or location that readers start reading at.
 * @size:	The size of the log
 * @logs:	The list of log channels
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
struct logger_log {
	unsigned char *buffer;
	struct miscdevice misc;
	wait_queue_head_t wq;
	struct list_head readers;
	struct mutex mutex;
	struct list_head logs;
};

static LIST_HEAD(m_log_list);

/**
 * struct logger_reader - a logging device open for reading
 * @log:	The associated log
 * @list:	The associated entry in @logger_log's list
 * @r_off:	The current read head offset.
 * @r_all:	Reader can read all entries
 * @r_ver:	notice:read the whole ring-buffer times, not version
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
 #define READ_FAIL_MAX_TIMES	3
struct logger_reader {
	struct logger_log *log;
	struct list_head list;
	uint32_t	r_off;	/* 当前读到的位置， 下一次再读的开始位置 */
	uint32_t	r_loops;
	uint32_t	r_sn;
	uint32_t	r_failtimes;
	uint32_t	r_from_cur;
	uint32_t	r_is_tlogf;
	bool r_all;
	unsigned int r_ver;
};

unsigned int m_rdr_mem_len = 0;
uint32_t g_tlogcat_count = 0;

/*
 * file_get_log - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 *	1) Need to quickly obtain the associated log during an I/O operation
 *	2) Readers need to maintain state (logger_reader)
 *	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->logger_reader->logger_log.
 * For a writer, we don't want to maintain a logger_reader, so we just go
 * file->logger_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct logger_log *file_get_log(struct file *file)
{
	struct logger_reader *reader = NULL;

	if (file == NULL)
		return NULL;

	reader = (struct logger_reader *)file->private_data;
	if (reader == NULL)
		return NULL;

	return reader->log;

}


static uint16_t logitem_calc_crc16(uint8_t *pChar, int32_t lCount)
{
	uint16_t usCrc;
	uint16_t usTmp ;
	uint8_t *pTmp ;

	if (NULL == pChar)
		return 0;

	usCrc = 0 ;
	pTmp = pChar ;

	while (--lCount >= 0) {
		usCrc = usCrc ^ ((uint16_t)(*pTmp++) << 8);

		for (usTmp = 0 ; usTmp < 8 ; ++usTmp) {
			if (usCrc & 0x8000) {
				usCrc = (usCrc << 1) ^ 0x1021 ;
			} else {
				usCrc = usCrc << 1 ;
			}
		}
	}

	return (usCrc & 0xFFFF) ;
}

static struct logger_log *g_log;
/*
 * logger_read - our log's read() method
 *
 * Behavior:
 *
 *	- O_NONBLOCK works
 *	- If there are no log entries to read, blocks until log is written to
 *	- Atomically reads exactly one log entry
 *
 * Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static LOG_ITEM_T*	logitem_getnext(uint8_t	*buffer_start, uint32_t max_len, uint32_t readpos, uint32_t scope_len, uint32_t *pos)
{
	uint32_t i = 0;
	LOG_ITEM_T	*logitem_next = NULL;
	uint32_t 	item_max_size = 0;
	uint16_t	item_crc;

	if (!buffer_start)
		return NULL;

	if ((readpos + scope_len) > max_len)
		return NULL;

	while ((i + sizeof(LOG_ITEM_T) + LOG_ITEM_LEN_ALIGN) <= scope_len) {

		if (pos) {
			*pos = readpos + i;
		}

		logitem_next = (LOG_ITEM_T	*)(buffer_start + readpos + i);

		item_max_size = (scope_len - i) > LOG_ITEM_MAX_LEN ? LOG_ITEM_MAX_LEN : (scope_len - i);

		if (logitem_next->magic == LOG_ITEM_MAGIC
			&& logitem_next->log_buffer_len > 0
			&& logitem_next->log_real_len > 0
			&& logitem_next->log_buffer_len % LOG_ITEM_LEN_ALIGN == 0
			&& logitem_next->log_real_len <= logitem_next->log_buffer_len
			&& (logitem_next->log_buffer_len - logitem_next->log_real_len) < LOG_ITEM_LEN_ALIGN
			&& logitem_next->log_buffer_len + sizeof(LOG_ITEM_T) <= item_max_size) {

			item_crc = logitem_calc_crc16((uint8_t *)&logitem_next->serial_no, logitem_next->log_real_len + sizeof(LOG_ITEM_T) - offsetof(LOG_ITEM_T, serial_no));
			if (item_crc == logitem_next->log_buffer_crc) {

				break;
			} else {
				tlogd("crc error\n");
			}


		}

		i += LOG_ITEM_LEN_ALIGN;
		logitem_next = NULL;

	}


	return logitem_next;
}


static uint32_t logitem_parse(char __user *buf, size_t count, uint8_t	*buffer_start, uint32_t max_len, uint32_t start_pos, uint32_t end_pos, uint32_t *read_off, uint32_t *userbuffer_left)
{
	LOG_ITEM_T	*logitem_next = NULL;
	uint32_t 	buf_left, buf_written, item_len;
	int ret = 0;

	buf_written = 0;
	buf_left    = count;

	if (userbuffer_left)
		*userbuffer_left = 1;

	if ( NULL == buf || NULL == buffer_start)
		return buf_written;

	while (start_pos < end_pos) {

		logitem_next = logitem_getnext(buffer_start, max_len, start_pos, end_pos - start_pos, &start_pos);

		if (logitem_next == NULL)
			break;

		/* copy to user */

		item_len = logitem_next->log_buffer_len + sizeof(LOG_ITEM_T);

		if (buf_left < item_len) {
			if (userbuffer_left)
				*userbuffer_left = 0;
			break;
		}

		start_pos += item_len;

		ret = copy_to_user(buf + buf_written, (void *)logitem_next, item_len);
		if (ret != 0) {
			tloge("copy failed ret %d, item_len %d \n", ret, item_len); /*lint !e559 */
		}

		buf_written += item_len;
		buf_left    -= item_len;

	}

	if (read_off)
		*read_off = start_pos;

	return buf_written;
}
static ssize_t tlogger_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	struct logger_reader *reader;
	struct logger_log *log;
	LOG_BUFFER_T		*logbuffer = NULL;
	ssize_t ret = 0;
	uint32_t 	buf_written, userbuffer_left;
	uint32_t	log_last_pos;
	errno_t ret_s;
	uint16_t	item_crc;
	uint32_t log_buffer_maxlen = m_rdr_mem_len - sizeof(LOG_BUFFER_T);

	LOG_BUFFER_FLAG_T	bufferflag;


	if (count < LOG_ITEM_MAX_LEN)
		return -EINVAL;

	if (log_buffer_maxlen > 0x100000)
		return  -EINVAL;

	if (NULL == file || NULL == buf)
		return  -EINVAL;

	reader = file->private_data;
	if (NULL == reader)
		return  -EINVAL;

	log = reader->log;
	if (log == NULL)
		return  -EINVAL;


	logbuffer = (LOG_BUFFER_T*)log->buffer;

	if (NULL == logbuffer)
		return	-EINVAL;

	__asm__ volatile ("isb");
	__asm__ volatile ("dsb sy");

	mutex_lock(&log->mutex);
	ret_s = memcpy_s(&bufferflag, sizeof(bufferflag), &logbuffer->flag, sizeof(logbuffer->flag));
	mutex_unlock(&log->mutex);

	if (ret_s != EOK) {
		tloge("memcpy failed %d\n", ret_s);
		ret = -EAGAIN;
		return ret;
	}

	/* check the buffer flag crc */

	item_crc = logitem_calc_crc16((uint8_t *)&bufferflag + sizeof(bufferflag.crc), sizeof(LOG_BUFFER_T) - sizeof(bufferflag.crc));
	if (item_crc != bufferflag.crc) {
		tloge("buffer flag check %x %x\n", item_crc, bufferflag.crc);
		ret = 0;
		return LOG_READ_STATUS_ERROR;
	}

	log_last_pos = bufferflag.last_pos;

	if (log_last_pos == reader->r_off && bufferflag.write_loops == reader->r_loops) {
		ret = 0;
		return ret;
	}

	if (bufferflag.max_len < log_last_pos || bufferflag.max_len > log_buffer_maxlen) {

		tloge("invalid data maxlen %x  pos %x \n", bufferflag.max_len , log_last_pos);
		ret = -EFAULT;
		return ret;
	}

	if (reader->r_off > bufferflag.max_len) {
		tloge("invalid data roff %x maxlen %x\n", reader->r_off , bufferflag.max_len);
		ret = -EFAULT;

		return ret;
	}

	/* 2. 再看上次读到的位置
		如果上次读的位置序列号比当前最小的小， 说明没有读完被覆盖了， 则从当前最小的开始读。
		否则按照上次读的位置接着读。
	*/
	buf_written = 0;
	userbuffer_left = 0;

	tlogd("read start \n");


	if (bufferflag.write_loops == reader->r_loops) {

		buf_written = logitem_parse(buf, count, logbuffer->buffer_start, bufferflag.max_len, reader->r_off, log_last_pos, &reader->r_off, &userbuffer_left);

	} else {


		if (bufferflag.write_loops > (reader->r_loops +1)
			|| ((bufferflag.write_loops == (reader->r_loops + 1) ) && (reader->r_off < log_last_pos))) {

			reader->r_off = log_last_pos;
			reader->r_loops = bufferflag.write_loops - 1;

		}


		/* */

		buf_written = logitem_parse(buf, count, logbuffer->buffer_start, bufferflag.max_len, reader->r_off, bufferflag.max_len, &reader->r_off, &userbuffer_left);

		if (count > buf_written && userbuffer_left) {

			buf_written += logitem_parse(buf + buf_written, count - buf_written, logbuffer->buffer_start, bufferflag.max_len, 0, log_last_pos, &reader->r_off, &userbuffer_left);
			reader->r_loops = bufferflag.write_loops;
		}

	}


	if (buf_written == 0) {
		ret = LOG_READ_STATUS_ERROR;
	} else {
		ret = buf_written;
		tlogd("read length %d\n", buf_written);

		g_lastread_offset = reader->r_off;
	}

	return ret;
}

/*its role is same to logger_aio_write*/
void tz_log_write(void)
{
	LOG_BUFFER_T		*logbuffer = NULL;

	if (g_log == NULL)
		return;

	logbuffer = (LOG_BUFFER_T*)g_log->buffer;

	if (NULL == logbuffer)
			return;

	if (g_lastread_offset != logbuffer->flag.last_pos) {
		tlogd("tz_log_write wake up start \n");
		wake_up_interruptible(&g_log->wq);
	}

	return;

}

static struct logger_log *get_log_from_minor(int minor)
{
	struct logger_log *log;

	list_for_each_entry(log, &m_log_list, logs) {
		if (log->misc.minor == minor)
			return log;
	}
	return NULL;
}

/*
 * logger_open - the log's open() file operation
 *
 * Note how near a no-op this is in the write-only case. Keep it that way!
 */
static int tlogger_open(struct inode *inode, struct file *file)
{
	struct logger_log *log;
	int ret;
	struct logger_reader *reader;

	tlogd("open logger_open ++\n");
	/*not support seek */
	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;
	tlogd("Before get_log_from_minor\n");
	log = get_log_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	reader = kmalloc(sizeof(struct logger_reader), GFP_KERNEL);
	if (!reader)
		return -ENOMEM;

	reader->log = log;
	reader->r_all = true;
	reader->r_off = 0;
	reader->r_loops = 0;
	reader->r_sn  = 0;
	reader->r_failtimes = 0;
	reader->r_is_tlogf = 0;
	reader->r_from_cur = 0;

	INIT_LIST_HEAD(&reader->list);

	mutex_lock(&log->mutex);
	list_add_tail(&reader->list, &log->readers);
	g_tlogcat_count++;

	mutex_unlock(&log->mutex);

	file->private_data = reader;


	tlogd("tlogcat count %d\n", g_tlogcat_count);

	return 0;
}

/*
 * logger_release - the log's release file operation
 *
 * Note this is a total no-op in the write-only case. Keep it that way!
 */
static int tlogger_release(struct inode *ignored, struct file *file)
{
	struct logger_reader *reader = NULL;
	struct logger_log *log = NULL;

	tlogd("logger_release ++\n");

	reader = file->private_data;
	if (reader == NULL){
		tloge("reader is null");
		return -1;
	}

	log = reader->log;
	if (log == NULL){
		tloge("log is null");
		return -1;
	}

	mutex_lock(&log->mutex);
	list_del(&reader->list);
	if (g_tlogcat_count >= 1)
		g_tlogcat_count--;
	mutex_unlock(&log->mutex);

	tlogi("logger_release r_is_tlogf-%u\n", reader->r_is_tlogf);
	if (reader->r_is_tlogf)
		g_tlogcat_f = 0;

	kfree(reader);

	tlogd("tlogcat count %d\n", g_tlogcat_count);
	return 0;
}

/*
 * logger_poll - the log's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int tlogger_poll(struct file *file, poll_table *wait)
{
	struct logger_reader *reader;
	struct logger_log *log;
	LOG_BUFFER_T		*logbuffer = NULL;

	unsigned int ret = POLLOUT | POLLWRNORM;

	tlogd("logger_poll ++\n");

	reader = (struct logger_reader *)file->private_data;
	if (NULL == reader) {
		tloge("the private data is null\n");
		return ret;
	}

	log = reader->log;
	if (log == NULL){
		tloge("log is null\n");
		return ret;
	}

	logbuffer = (LOG_BUFFER_T*)log->buffer;
	if (logbuffer == NULL){
		tloge("logbuffer is null\n");
		return ret;
	}

	poll_wait(file, &log->wq, wait);

	tlogd("poll after w_off_reader=%x reader->r_off=%x\n", logbuffer->flag.last_pos, reader->r_off);
	if (logbuffer->flag.last_pos != reader->r_off)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

static void tlogger_setreaderpos_cur(struct file *file)
{
	struct logger_reader *reader;

	struct logger_log *log;
	LOG_BUFFER_T		*logbuffer = NULL;

	if (NULL == file)
		return;

	reader = file->private_data;
	if (NULL == reader)
		return;

	log = reader->log;

	if (NULL == log)
		return;

	logbuffer = (LOG_BUFFER_T*)log->buffer;

	if (NULL == logbuffer) {
		return;
	}

	reader->r_from_cur = 1;

	reader->r_off = logbuffer->flag.last_pos;
	reader->r_loops = logbuffer->flag.write_loops;


}
static void tlogger_set_tlogcat_f(struct file *file)
{
	struct logger_reader *reader;

	if (NULL == file)
		return;

	reader = file->private_data;
	if (NULL == reader)
		return;

	reader->r_is_tlogf = 1;
	g_tlogcat_f = 1;

	tlogi("set tlogcat_f-%u\n", g_tlogcat_f);

	return;
}
static int tlogger_get_tlogcat_f(void)
{
	tlogi("get tlogcat_f-%u\n", g_tlogcat_f);
	return g_tlogcat_f;
}
static long tlogger_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	struct logger_log *log;
	long ret = -EINVAL;

	if (file == NULL)
		return -1;

	log	= file_get_log(file);

	if (m_logbuffer == NULL) {
		tloge("log buffer is null \n");
		return -1;
	}

	if (log == NULL){
		tloge("log is null \n");
		return -1;
	}

	tlogd("logger_ioctl start ++\n");
	mutex_lock(&log->mutex);

	switch (cmd) {

	case TEELOGGER_GET_VERSION:
		if (_IOC_DIR(cmd) & _IOC_READ) {
			ret = !access_ok(VERIFY_WRITE, (void __user *)arg, sizeof(m_logbuffer->flag.version_info));
			if (!ret) {
				ret = copy_to_user((void __user *)arg, (void *)m_logbuffer->flag.version_info, sizeof(m_logbuffer->flag.version_info));
				if (ret != 0) {
					tloge("ver copy failed ret %ld\n", ret);
				}

			}
		}
		break;
	case TEELOGGER_SET_READERPOS_CUR:
		tlogger_setreaderpos_cur(file);
		ret = 0;
		break;
	case TEELOGGER_SET_TLOGCAT_STAT:
		tlogger_set_tlogcat_f(file);
		ret = 0;
		break;
	case TEELOGGER_GET_TLOGCAT_STAT:
		ret = tlogger_get_tlogcat_f();
		break;
	default:
		tloge("ioctl error default\n");
		break;
	}

	mutex_unlock(&log->mutex);

	return ret;
}

static long tlogger_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	long ret = -ENOIOCTLCMD;

	tlogd("logger_compat_ioctl ++\n");
	arg = (unsigned long)compat_ptr(arg);
	ret = tlogger_ioctl(file, cmd, arg);
	return ret;
}

static const struct file_operations logger_fops = {
	.owner = THIS_MODULE,
	.read = tlogger_read,
	/*.write = logger_aio_write,*/
	.poll = tlogger_poll,
	.unlocked_ioctl = tlogger_ioctl,
	.compat_ioctl = tlogger_compat_ioctl,
	.open = tlogger_open,
	.release = tlogger_release,
};


static int __init create_log(char *log_name, size_t addr, int size)
{
	int ret = 0;
	struct logger_log *log;
	unsigned char *buffer;

	buffer = (unsigned char *)addr; /*lint !e64 */

	if (buffer == NULL)
		return -ENOMEM;

	if(log_name == NULL)
		return -EINVAL;

	log = kzalloc(sizeof(struct logger_log), GFP_KERNEL);
	if (log == NULL) {
		ret = -ENOMEM;
		goto out_free_buffer;
	}
	log->buffer = buffer;
	log->misc.minor = MISC_DYNAMIC_MINOR;
	log->misc.name = kstrdup(log_name, GFP_KERNEL);
	if (log->misc.name == NULL) {
		ret = -ENOMEM;
		goto out_free_log;
	}
	log->misc.fops = &logger_fops;
	log->misc.parent = NULL;

	init_waitqueue_head(&log->wq);
	INIT_LIST_HEAD(&log->readers);
	mutex_init(&log->mutex);
	INIT_LIST_HEAD(&log->logs);
	list_add_tail(&log->logs, &m_log_list);
	/* finally, initialize the misc device for this log */
	ret = misc_register(&log->misc);
	if (unlikely(ret)) {
		tloge("failed to register misc device for log '%s'!\n", log->misc.name);
		goto out_free_log;
	}
	g_log = log;
	return 0;

out_free_log:
	if (log->misc.name) {
		kfree(log->misc.name);
	}
	kfree(log);

out_free_buffer:
	/*vfree(buffer); */
	return ret;
}

static LOG_ITEM_T*	lastmsg_logitem_getnext(uint8_t	*buffer_start, uint32_t readpos, int scope_len, uint32_t max_len)
{
	int i = 0;
	LOG_ITEM_T	*logitem_next = NULL;
	uint32_t 	item_max_size = 0;

	if (!buffer_start)
		return NULL;


	while (i <= scope_len &&
		((readpos + i + sizeof(LOG_ITEM_T)) < max_len)) {

		item_max_size = (uint32_t)((scope_len - i) > LOG_ITEM_MAX_LEN ? LOG_ITEM_MAX_LEN : (scope_len - i));
		logitem_next = (LOG_ITEM_T	*)(buffer_start + readpos + i);

		if (logitem_next && logitem_next->magic == LOG_ITEM_MAGIC
			&& logitem_next->log_buffer_len > 0
			&& logitem_next->log_real_len > 0
			&& logitem_next->log_buffer_len % LOG_ITEM_LEN_ALIGN == 0
			&& logitem_next->log_real_len <= logitem_next->log_buffer_len
			&& (logitem_next->log_buffer_len - logitem_next->log_real_len) < LOG_ITEM_LEN_ALIGN
			&& logitem_next->log_buffer_len + sizeof(LOG_ITEM_T) < item_max_size) {

			if ((readpos + i + sizeof(LOG_ITEM_T) + logitem_next->log_buffer_len) > max_len) {
				return NULL;
			}

			return logitem_next;
		}

		i += LOG_ITEM_LEN_ALIGN;

		logitem_next = NULL;
	}


	return NULL;
}

int tlogger_store_lastmsg(void)
{
	struct file *filep = NULL;
	ssize_t write_len, item_len, total_len;
	uint32_t read_off;
	mm_segment_t old_fs;
	loff_t pos = 0;
	int ret = 0;
	errno_t ret_s;
	LOG_ITEM_T	*logitem_next;
	uint32_t log_buffer_maxlen = m_rdr_mem_len - sizeof(LOG_BUFFER_T);

	uint8_t *buffer = NULL;

	tlogd("filp_open last_teemsg\n");

	if (!g_tlogcat_count) {
		tlogd("tlogcat count  %d\n", g_tlogcat_count);
		return 0;
	}

	if (m_logbuffer == NULL)
		return 0;

	/* 先将日志从RDR中拷贝到内存中再解析 */
	if (log_buffer_maxlen > 0x100000)
		return 0;

	buffer = kmalloc(log_buffer_maxlen, GFP_KERNEL);
	if (buffer == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ret_s = memcpy_s(buffer, log_buffer_maxlen,  m_logbuffer->buffer_start, log_buffer_maxlen);
	if (ret_s != EOK) {
		tloge("memcpy failed %d\n", ret_s);
		ret = -EAGAIN;
		goto out;
	}

	/* 解析buffer， 并写入文件 */

	/* */

	/*exception handling, store trustedcore exception info to file */
	filep = filp_open(LOG_PATH_TEE_LOG_FILE, O_CREAT | O_RDWR | O_TRUNC , 0640);
	if (IS_ERR(filep)) {
		tloge("Failed to filp_open last_teemsg, err %ld\n", PTR_ERR(filep));
		ret = -1;
		filep = NULL;
		goto out;
	}

	tlogd("Succeed to filp_open last_teemsg\n");

	old_fs = get_fs(); /*lint !e501 */
	set_fs(KERNEL_DS); /*lint !e501 */

	ret = (int)sys_chown((const char __user *)LOG_PATH_TEE_LOG_FILE, ROOT_UID, SYSTEM_GID);
	if (ret) {
		tloge("Failed to chown last_teemsg\n");
		ret = -2;
		goto out2;
	}


	/* first write tee versino info */
	write_len = vfs_write(filep, m_logbuffer->flag.version_info, strlen(m_logbuffer->flag.version_info), &pos);
	if (write_len < 0) {
		tloge("Failed to write to last_teemsg version\n");

		ret = -4;
		goto out2;
	} else {
		tlogd("Succeed to Write to last_teemsg version, write_len=%ld\n",
			write_len);
	}


	read_off = 0;
	total_len = 0;
	logitem_next = lastmsg_logitem_getnext(buffer, read_off, LOG_ITEM_MAX_LEN, log_buffer_maxlen);

	while (logitem_next) {

		item_len = logitem_next->log_buffer_len + sizeof(LOG_ITEM_T);

		write_len = vfs_write(filep, logitem_next->log_buffer, logitem_next->log_real_len, &pos);
		if (write_len < 0) {
			tloge("Failed to write to last_teemsg %zd\n", write_len);
			ret = -6;
			goto out2;
		} else {
			tlogd("Succeed to Write to last_teemsg, write_len=%ld\n",
				write_len);
		}

		total_len += item_len;

		read_off = (uint8_t *)logitem_next - buffer + item_len;

		if (total_len >= log_buffer_maxlen)
			break;

		logitem_next = lastmsg_logitem_getnext(buffer, read_off, LOG_ITEM_MAX_LEN, log_buffer_maxlen);
	}
	pos = 0;

	ret = 0;

out2:
	set_fs(old_fs);

out:

	if (buffer) {
		kfree(buffer);
		buffer = NULL;
	}

	if (filep)
		filp_close(filep, 0);

	/* 触发写 teeos_log */
	tz_log_write();

	return ret;
}

static int __init tlogger_init(void)
{
	int ret;
	unsigned long rdr_mem_addr;

	m_rdr_mem_len	= TC_NS_get_rdr_mem_len();
	rdr_mem_addr 	= TC_NS_get_rdr_mem_addr();

	if (m_rdr_mem_len < TEMP_RDR_MEM_SIZE) {
		tloge("rdr mem init failed!!! rdr len is too small 0x%x\n", m_rdr_mem_len);
		return -1;
	}

	if (!rdr_mem_addr) {
		tloge("rdr mem init failed!!! rdr addr is 0\n");
		return -1;
	}

	m_logbuffer = (LOG_BUFFER_T *)hisi_bbox_map(rdr_mem_addr, m_rdr_mem_len);
	if(!m_logbuffer){
		return -ENOMEM;
	}

	m_logbuffer->flag.max_len 	= m_rdr_mem_len - sizeof(LOG_BUFFER_T);

	tloge("tlogcat verison 1.0.0\n");
	tlogd("11-29 tlogcat:start=0x%lx\t len=0x%x\n", (unsigned long)m_logbuffer->buffer_start, m_logbuffer->flag.max_len);
	ret = create_log(LOGGER_LOG_TEEOS, (size_t)m_logbuffer, sizeof(LOG_BUFFER_T) + m_logbuffer->flag.max_len);
	if(ret) {
		hisi_bbox_unmap((void *)m_logbuffer);
		m_logbuffer = NULL;
		m_rdr_mem_len = 0;
	}
	return ret;
}

static void __exit tlogger_exit(void)
{
	struct logger_log *current_log, *next_log;

	list_for_each_entry_safe(current_log, next_log, &m_log_list, logs) {
		/* we have to delete all the entry inside m_log_list */
		misc_deregister(&current_log->misc);
		/*vfree(current_log->buffer); */
		kfree(current_log->misc.name);
		list_del(&current_log->logs);
		kfree(current_log);
	}
	hisi_bbox_unmap(m_logbuffer);
	m_logbuffer = NULL;
	m_rdr_mem_len = 0;
}

device_initcall(tlogger_init);
module_exit(tlogger_exit);

MODULE_AUTHOR("z00202529");
MODULE_DESCRIPTION("TrustCore Logger");
MODULE_VERSION("1.00");
