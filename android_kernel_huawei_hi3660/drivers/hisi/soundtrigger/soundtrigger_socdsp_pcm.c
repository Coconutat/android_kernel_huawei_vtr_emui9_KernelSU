#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/errno.h>
#include "hifi_lpp.h"
#include "mlib_ringbuffer.h"
#include "../../sound/soc/hisilicon/hisi_pcm_hifi.h"

#define FAST_BUFFER_SIZE  (HISI_AP_AUDIO_WAKEUP_RINGBUFEER_SIZE)
#define FAST_SOCDSP_UPLOAD_SIZE (61440) //16K*1ch*2byte*1920ms
#define RINGBUF_FRAME_LEN	(640)   //16k*1ch*2byte*20ms
#define RINGBUF_FRAME_COUNT (100)
#define RINGBUF_HEAD_SIZE  (20)
#define NORMAL_BUFFER_SIZE (RINGBUF_FRAME_LEN*RINGBUF_FRAME_COUNT + RINGBUF_HEAD_SIZE)

enum fast_transfer_state {
	FAST_TRANS_NOT_COMPLETE = 0,
	FAST_TRANS_COMPLETE,
};

enum fast_read_state {
	FAST_READ_NOT_COMPLETE = 0,
	FAST_READ_COMPLETE,
};

struct soundtrigger_socdsp_pcm {
	RingBuffer *normal_ring_buffer;
	char *fast_buffer;
	char *elapsed_buffer;
	int fast_complete_flag; /* fast tansmit complete flag */
	unsigned int fast_len;
	int fast_read_complete_flag; /* flag to decide whether HAL read cpmlete */
};
struct soundtrigger_socdsp_pcm g_socdsp_pcm;

static int soundtrigger_socdsp_pcm_get_element(char *element)
{
	int ret = 0;
	if (g_socdsp_pcm.normal_ring_buffer == NULL)
		return -EFAULT;

	if (!RingBuffer_IsEmpty(g_socdsp_pcm.normal_ring_buffer)) {
		ret = RingBuffer_Get(g_socdsp_pcm.normal_ring_buffer, element);
		if (ret != RINGBUF_FRAME_LEN)
			return -EFAULT;
	}

	return ret;
}

static ssize_t soundtrigger_socdsp_pcm_read(struct file *file, char __user *buf,
								   size_t count, loff_t *ppos)
{
	int rest_bytes = 0;
	int read_bytes = 0;
	size_t max_read_len = 0;
	char element[RINGBUF_FRAME_LEN];
	if (buf == NULL || g_socdsp_pcm.fast_buffer == NULL) {
		pr_err("%s buf is null", __FUNCTION__);
		return -EFAULT;
	}

	if (count < RINGBUF_FRAME_LEN) {
		pr_err("%s count is err %zu", __FUNCTION__, count);
		return -EFAULT;
	}

	max_read_len = count >= FAST_SOCDSP_UPLOAD_SIZE ? FAST_SOCDSP_UPLOAD_SIZE : count;
	if (g_socdsp_pcm.fast_read_complete_flag == FAST_READ_NOT_COMPLETE) {
		if (g_socdsp_pcm.fast_complete_flag == FAST_TRANS_COMPLETE) {
			if (max_read_len > g_socdsp_pcm.fast_len) {
				max_read_len = g_socdsp_pcm.fast_len;
				pr_info("fastlen less than 64K %u", g_socdsp_pcm.fast_len);
			}
			rest_bytes = copy_to_user(buf, g_socdsp_pcm.fast_buffer, max_read_len);
			if (rest_bytes) {
				pr_err("copy to user fail\n");
				return -EFAULT;
			}
			g_socdsp_pcm.fast_read_complete_flag = FAST_READ_COMPLETE;
			return max_read_len;
		} else {
			return -EFAULT;
		}
	} else {
		read_bytes = soundtrigger_socdsp_pcm_get_element(element);
		if (read_bytes > 0) {
			rest_bytes = copy_to_user(buf, element, read_bytes);
			if (rest_bytes) {
				pr_err("copy to user fail\n");
				return -EFAULT;
			}
		} else {
			return -EFAULT;
		}
		return read_bytes;
	}
}

static const struct file_operations soundtrigger_socdsp_pcm_read_fops = {
	.owner = THIS_MODULE,
	.read = soundtrigger_socdsp_pcm_read,
};

static struct miscdevice soundtrigger_socdsp_pcm_drv_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "soundtrigger_socdsp_pcm_drv",
	.fops = &soundtrigger_socdsp_pcm_read_fops,
};

void soundtrigger_socdsp_pcm_flag_init(void)
{
	g_socdsp_pcm.fast_complete_flag = FAST_TRANS_NOT_COMPLETE;
	g_socdsp_pcm.fast_len = 0;
	g_socdsp_pcm.fast_read_complete_flag = FAST_READ_NOT_COMPLETE;
}

int soundtrigger_socdsp_pcm_fastbuffer_filled(unsigned int fast_len)
{
	if (g_socdsp_pcm.fast_buffer == NULL)
		return -ENOMEM;
	g_socdsp_pcm.fast_len = fast_len;
	g_socdsp_pcm.fast_complete_flag = FAST_TRANS_COMPLETE;
	return 0;
}

char* soundtrigger_socdsp_pcm_fastbuffer_get(void)
{
	return g_socdsp_pcm.fast_buffer;
}

int soundtrigger_socdsp_pcm_elapsed(unsigned int start, int buffer_len)
{
	if ((buffer_len != RINGBUF_FRAME_LEN) || (start > RINGBUF_FRAME_LEN))
		return -EFAULT;

	if (g_socdsp_pcm.fast_buffer == NULL || g_socdsp_pcm.elapsed_buffer == NULL || g_socdsp_pcm.normal_ring_buffer == NULL)
		return -ENOMEM;

	RingBuffer_Put(g_socdsp_pcm.normal_ring_buffer, (void*)(g_socdsp_pcm.elapsed_buffer + start));

	return 0;
}

int soundtrigger_socdsp_pcm_init(void)
{
	int ret = 0;

	ret = misc_register(&soundtrigger_socdsp_pcm_drv_device);
	if (ret) {
		pr_err("%s misc_register fail\n", __FUNCTION__);
		return -EFAULT;
	}

	g_socdsp_pcm.normal_ring_buffer = kzalloc(NORMAL_BUFFER_SIZE, GFP_ATOMIC);
	if (g_socdsp_pcm.normal_ring_buffer == NULL) {
		pr_err("%s normal_ring_buffer malloc fail\n", __FUNCTION__);
		misc_deregister(&soundtrigger_socdsp_pcm_drv_device);
		return -ENOMEM;
	}

	g_socdsp_pcm.fast_buffer = ioremap_wc(HISI_AP_AUDIO_WAKEUP_RINGBUFFER_ADDR, HISI_AP_AUDIO_WAKEUP_RINGBUFEER_SIZE);
	if (g_socdsp_pcm.fast_buffer == NULL) {
		pr_err("%s fast buffer ioremap fail\n", __FUNCTION__);
		misc_deregister(&soundtrigger_socdsp_pcm_drv_device);
		kfree(g_socdsp_pcm.normal_ring_buffer);
		g_socdsp_pcm.normal_ring_buffer = NULL;
		return -EFAULT;
	}

	g_socdsp_pcm.elapsed_buffer = ioremap_wc(HISI_AP_AUDIO_WAKEUP_CAPTURE_ADDR, HISI_AP_AUDIO_WAKEUP_CAPTURE_SIZE);
	if (g_socdsp_pcm.elapsed_buffer == NULL) {
		pr_err("%s elapsed_buffer ioremap fail\n", __FUNCTION__);
		misc_deregister(&soundtrigger_socdsp_pcm_drv_device);
		iounmap(g_socdsp_pcm.fast_buffer);
		g_socdsp_pcm.fast_buffer = NULL;
		kfree(g_socdsp_pcm.normal_ring_buffer);
		g_socdsp_pcm.normal_ring_buffer = NULL;
		return -EFAULT;
	}

	RingBuffer_Init(g_socdsp_pcm.normal_ring_buffer, RINGBUF_FRAME_LEN, RINGBUF_FRAME_COUNT);

	return 0;
}

int soundtrigger_socdsp_pcm_deinit(void)
{
	if (g_socdsp_pcm.normal_ring_buffer != NULL) {
		kfree(g_socdsp_pcm.normal_ring_buffer);
		g_socdsp_pcm.normal_ring_buffer = NULL;
	}

	if (g_socdsp_pcm.fast_buffer != NULL) {
		iounmap(g_socdsp_pcm.fast_buffer);
		g_socdsp_pcm.fast_buffer = NULL;
	}

	if (g_socdsp_pcm.elapsed_buffer != NULL) {
		iounmap(g_socdsp_pcm.elapsed_buffer);
		g_socdsp_pcm.elapsed_buffer = NULL;
	}

	misc_deregister(&soundtrigger_socdsp_pcm_drv_device);

	return 0;
}


