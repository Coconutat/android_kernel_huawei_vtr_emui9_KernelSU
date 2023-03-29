#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <uapi/linux/uio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <chipset_common/hwzrhung/zrhung.h>
#include "../zrhung_common.h"
#include "zrhung_wp_sochalt.h"

static int is_soc_halt;
static int is_longpress;

#define SOCHALT_INFO         "BR_PRESS_10S"
#define ZRHUNG_POWERKEY_LONGPRESS_EVENT "AP_S_PRESS6S"
#define SR_POSITION_KEYWORD  "sr position:"
#define FASTBOOT_LOG_PATH    "/proc/balong/log/fastboot_log"
#define BUFFER_SIZE_FASTBOOT (4 * 1024) //4KB buffer for reading fastboot log

static int __init wp_reboot_reason_cmdline(char *reboot_reason_cmdline)
{
	if (!strcmp(reboot_reason_cmdline, SOCHALT_INFO)) {  /*lint !e421*/
		printk(KERN_ERR "%s %d: LONGPRESS10S_EVENT happen! should report zerohung. \n", __FUNCTION__, __LINE__);
		#ifndef CONFIG_MTK_PLATFORM
		printk(KERN_ERR "%s %d: LONGPRESS10S_EVENT happen! should report zerohung. not mtk platform \n", __FUNCTION__, __LINE__);
		is_soc_halt = 1;
		#endif
	}
	if (!strcmp(reboot_reason_cmdline, ZRHUNG_POWERKEY_LONGPRESS_EVENT)) { /*lint !e421*/
		printk(KERN_ERR "%s %d: LONGPRESS6S_EVENT happen! should report zerohung. \n", __FUNCTION__, __LINE__);
		is_longpress = 1;
	}

	return 0;
}

early_param("reboot_reason", wp_reboot_reason_cmdline);

void zrhung_get_longpress_event(void)
{
	if (is_soc_halt || is_longpress) {
		printk(KERN_ERR "%s %d: POWERKEY_LONGPRESS_EVENT send to zerohung. \n", __FUNCTION__, __LINE__);
		zrhung_send_event(ZRHUNG_EVENT_LONGPRESS, NULL, NULL);
	}
}

int wp_get_sochalt(zrhung_write_event* we)
{
	if (!we) {
		printk(KERN_ERR "%s %d: param error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	memset(we, 0, sizeof(*we));

	if (is_soc_halt) {
		we->magic = MAGIC_NUM;
		we->len = sizeof(*we);
		we->wp_id = ZRHUNG_WP_SR;

		printk(KERN_ERR "%s %d: soc halt\n", __FUNCTION__, __LINE__);
	}

	return 0;
}

void get_sr_position_from_fastboot(char *dst, unsigned int max_dst_size)
{
	mm_segment_t old_fs;
	char * reading_buf = NULL;
	char *plog = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	int fd = sys_open(FASTBOOT_LOG_PATH, O_RDONLY, 0);
	if ( fd < 0 ){
		printk(KERN_ERR "%s %d: fail to open fastbootlog\n", __FUNCTION__, __LINE__);
		goto __out ;
	}

	reading_buf = (char *)kzalloc(BUFFER_SIZE_FASTBOOT + 1, GFP_KERNEL);
	if ( NULL == reading_buf ) {
		printk(KERN_ERR "%s %d: alloc fail\n", __FUNCTION__, __LINE__);
		goto __out;
	}
	while ( sys_read( fd, reading_buf, BUFFER_SIZE_FASTBOOT ) >0  ){
		if ( (plog = strstr(reading_buf, SR_POSITION_KEYWORD) ) != NULL ){
			if ( (BUFFER_SIZE_FASTBOOT - (plog - reading_buf)) < max_dst_size  ){
				strncpy(dst, plog, BUFFER_SIZE_FASTBOOT - (plog - reading_buf));
				break;
			}

			unsigned int i = 0;
			while ( plog[i] != '\r' && plog[i] != '\n' && i < max_dst_size ){
				dst[i] = plog[i];
				i ++;
			}
			dst[max_dst_size -1] = 0;
			break ;
		}
	}

__out:
	if (fd >= 0){
		sys_close(fd);
	}

	if (reading_buf != NULL){
		kfree(reading_buf);
	}

	set_fs(old_fs);
	return ;
}
