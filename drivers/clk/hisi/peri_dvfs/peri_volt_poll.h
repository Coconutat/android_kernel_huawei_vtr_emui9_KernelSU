/*
 * Copyright (C) 2015
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PERIVOLT_POLL_EXTERN_H
#define __PERIVOLT_POLL_EXTERN_H
#include "peri_volt_internal.h"

enum {
	PERI_VOLT_0 = 0, /*0.7v*/
	PERI_VOLT_1,
	PERI_VOLT_2, /*0.8v*/
};

struct peri_volt_poll *peri_volt_poll_get(unsigned int dev_id, const char *name);
unsigned int peri_get_volt(struct peri_volt_poll *pvp);
int peri_set_volt(struct peri_volt_poll *pvp, unsigned int volt);
int peri_poll_stat(struct peri_volt_poll *pvp);
int peri_get_temperature(struct peri_volt_poll *pvp);

#endif /* __PERIVOLT_POLL_INTERNAL_H */
