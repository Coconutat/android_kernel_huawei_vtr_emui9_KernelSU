/*
 * root_scan.h
 *
 * used for root scan triggering
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * likun <quentin.lee@huawei.com>
 *
 */

#ifndef _ROOT_SCAN_H_
#define _ROOT_SCAN_H_


#define D_RSOPID_MASK_NONE 0	/* check none items */
#define D_RSOPID_KCODE     (1 << 0)	/* check integrity of kernel code */
#define D_RSOPID_SYS_CALL  (1 << 1)	/* check system call table*/
#define D_RSOPID_SE_HOOKS  (1 << 2)	/* check seLinux hooks */
#define D_RSOPID_SE_STATUS (1 << 3)	/* check seLinux enforce status */
#define D_RSOPID_RRPOCS    (1 << 4)	/* check root processes */
#define D_RSOPID_SETID     (1 << 5) /* check setid */

#ifdef CONFIG_HW_ROOT_SCAN
int rscan_trigger(void);
#else
static inline int rscan_trigger(void)
{
	return 0;
}
#endif

/*

* root_scan_pause - for kernel space pause root-scan

* Description: pause root scan for kernel hot fix.

* support kcode, syscall, sehooks, selinux status scan pause.

* @result, after calling the pause fuction, root scan will be suspended

*    until using resume function

* @return:

*     Overall root status, return 0 if pause root success.

*     Otherwise if device has rooted, return an interger that each

*     of its bit corresponds to its bitmasks

*/

#ifdef CONFIG_HW_ROOT_SCAN
int root_scan_pause(unsigned int op_mask, void *reserved);
#else
static inline int root_scan_pause(unsigned int op_mask, void *reserved)
{
	return 0;
}
#endif

/*

* root_scan_resume - for kernel space resume root scan

* Description: pause and resume should be pairing

* @result, after using the interfase, root scan will init scan,and resume.

* @error_code, 0:success, others:error

* kcode resume error     1 << 0

* sys_call error             1 << 1

* se_hook error               1 << 2

* se_status error            1 << 3

*/

#ifdef CONFIG_HW_ROOT_SCAN
int root_scan_resume(unsigned int op_mask, void *reserved);
#else
static inline int root_scan_resume(unsigned int op_mask, void *reserved)
{
	return 0;
}
#endif

#endif
