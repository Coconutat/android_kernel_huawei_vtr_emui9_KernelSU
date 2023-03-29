/*
 * upload_double_free.h
 *
 * the upload_double_free.h for kernel module using.
 *
 * chenli <chenli45@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#ifndef _UPLOAD_DOUBLE_FREE_H
#define _UPLOAD_DOUBLE_FREE_H

/*
 * upload_double_free_log - upload stack trace infomation when double free happen
 * @s, the kmem_cache happen double free
 * @add_info, the add_info wanted to upload
 * @return: void
 */

#ifdef CONFIG_HW_DOUBLE_FREE_DYNAMIC_CHECK
void upload_double_free_log(struct kmem_cache *s, char *add_info);
#else
static inline void upload_double_free_log(struct kmem_cache *s, char *add_info)
{
}
#endif

#endif