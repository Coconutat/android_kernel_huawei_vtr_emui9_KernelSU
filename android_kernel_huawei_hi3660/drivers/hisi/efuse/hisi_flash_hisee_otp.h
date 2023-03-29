/*
 * hisi_flash_hisee_otp.h
 *
 * provides interfaces for writing hisee otp
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * chenli <chenli24@huawei.com>
 *
 */

#ifndef	__HISI_FLASH_HISEE_OTP_H
#define	__HISI_FLASH_HISEE_OTP_H

#ifdef CONFIG_HISI_HISEE
extern void creat_flash_otp_thread(void);
extern void creat_flash_otp_init(void);
#else
static inline void creat_flash_otp_thread(void)
{
	return OK;
}

static inline void creat_flash_otp_init(void)
{
	return OK;
}
#endif

#endif
