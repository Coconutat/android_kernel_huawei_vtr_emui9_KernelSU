/**********************************************************
 * Filename:    imonitor.h
 *
 * Discription: Interfaces for generate imonitor event struct
				and convert it to regular string which can be
				analysed by imonitor engine
 *
 * Copyright: (C) 2016 huawei.
 *
 * Author: yaomanhai(00303692)
 *
**********************************************************/
#ifndef _IMONITOR_H
#define _IMONITOR_H
#include "imonitor_keys.h"

#define IMONITOR_NEW_API
#define IMONITOR_NEW_API_V2

#ifdef __cplusplus
extern "C" {
#endif

struct imonitor_eventobj;

struct imonitor_eventobj *imonitor_create_eventobj(unsigned int eventid);

/* deprecated */
int imonitor_set_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value);

#ifdef IMONITOR_NEW_API
int imonitor_set_param_integer(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value);

int imonitor_set_param_string(struct imonitor_eventobj *eventobj,
		unsigned short paramid, const char* value);
#endif

int imonitor_unset_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid);

#ifdef IMONITOR_NEW_API_V2
int imonitor_set_param_integer_v2(struct imonitor_eventobj *eventobj,
		const char* param, long value);

int imonitor_set_param_string_v2(struct imonitor_eventobj *eventobj,
		const char* param, const char* value);

int imonitor_unset_param_v2(struct imonitor_eventobj *eventobj,
		const char* param);
#endif

int imonitor_set_time(struct imonitor_eventobj *eventobj,
		long long seconds);

int imonitor_add_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path);

int imonitor_add_and_del_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path);

int imonitor_send_event(struct imonitor_eventobj *eventobj);

void imonitor_destroy_eventobj(struct imonitor_eventobj *eventobj);

#ifdef __cplusplus
}
#endif
#endif
