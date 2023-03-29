/*
 * blackbox header file (blackbox: kernel run data recorder.)
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BB_PRINTER_H__
#define __BB_PRINTER_H__

#include <linux/types.h>

#define BB_PRINT_PN(args...)    pr_info(args);
#define BB_PRINT_ERR(args...)   pr_err("<bbox fail>"args);
#define BB_PRINT_DBG(args...)   pr_debug(args);
#define BB_PRINT_START(args...) \
	pr_info(">>>>>enter blackbox %s: %.4d.\n", __func__, __LINE__);
#define BB_PRINT_END(args...)   \
	pr_info("<<<<<exit  blackbox %s: %.4d.\n", __func__, __LINE__);

void rdr_print_all_ops(void);
void rdr_print_all_exc(void);

#endif /* End #define __BB_PRINTER_H__ */
