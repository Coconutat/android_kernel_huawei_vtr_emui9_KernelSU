/**********************************************************
 * Filename:	imonitor_events.c
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "huawei_platform/log/hw_log.h"
#include "huawei_platform/log/log_exception.h"

#include "huawei_platform/log/imonitor.h"
#include "huawei_platform/log/imonitor_events.h"

#define BIT_TYPE (1)
#define TINYINT_TYPE (2)
#define SMALLINT_TYPE (3)
#define INT_TYPE (4)
#define FLOAT_TYPE (5)
#define DATETIME_TYPE (6)
#define CLASS_TYPE (7)
#define BASE_TYPE_MAX CLASS_TYPE
#define VARCHAR_TYPE (100)

#define BIT_TYPE_LEN (1)
#define TINYINT_TYPE_LEN (1)
#define SMALLINT_TYPE_LEN (2)
#define INT_TYPE_LEN (4)
#define FLOAT_TYPE_LEN (4)
#define DATETIME_TYPE_LEN (20)

#define MASK_OFFSET_GET_OFFSET(m) ((m) & (unsigned int)0x7FFFFFFF)
#define MASK_OFFSET_SET_MASK(m) ((m) | ((unsigned int)1 << 31))
#define MASK_OFFSET_GET_MASK(m) ((m) >> 31)

#define MAX_PATH_NUMBER (10)
#define MAX_PATH_LEN (256)
#define MAX_STR_LEN (10*1024)

/*64K is max length of /dev/hwlog_exception*/
#define EVENT_INFO_BUF_LEN (64*1024)
#define EVENT_INFO_PACK_BUF_LEN (2*1024)

#define HWLOG_TAG	imonitor
HWLOG_REGIST();

#define BUF_POINTER_FORWARD \
	do { \
		if (tmplen < len) { \
			tmp += tmplen; \
			len -= tmplen; \
		} else { \
			hwlog_err("string over length"); \
			tmp += len; \
			len = 0; \
		} \
	} while (0)

struct imonitor_param;
struct imonitor_param {
	int key;
	char *key_v2;
	char *value;
	struct imonitor_param *next;
};

/*event obj struct*/
struct imonitor_eventobj {
	unsigned int eventid;
	const unsigned short *desc;
	unsigned int params_count;
	char api_version;

	/*record params linked list*/
	struct imonitor_param *head;
	/*time*/
	long long time;
	/*dynamic file path should be packed and uploaded*/
	char *dynamic_path[MAX_PATH_NUMBER];
	/*
	 * dynamic file path should be packed and uploaded, deleted
	 * after packing and uploading
	 */
	char *dynamic_path_delete[MAX_PATH_NUMBER];
};

#define CHECK_V1_API(obj) \
do { \
	if (obj->api_version == 0) { \
		obj->api_version = 1; \
	} else if (obj->api_version == 2) { \
		hwlog_err("cannot use v1 api (%s) after v2 api used", __FUNCTION__); \
		return -1; \
	} \
}while(0)

#define CHECK_V2_API(obj) \
do { \
	if (obj->api_version == 0) { \
		obj->api_version = 2; \
	} else if (obj->api_version == 1) { \
		hwlog_err("cannot use v2 api (%s) after v1 api used", __FUNCTION__); \
		return -1; \
	} \
}while(0)

#ifndef IMONITOR_NEW_API
int imonitor_set_param_integer(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value);

int imonitor_set_param_string(struct imonitor_eventobj *eventobj,
		unsigned short paramid, const char* value);
#endif

#ifndef IMONITOR_NEW_API_V2
int imonitor_set_param_integer_v2(struct imonitor_eventobj *eventobj,
		const char* param, long value);

int imonitor_set_param_string_v2(struct imonitor_eventobj *eventobj,
		const char* param, const char* value);

int imonitor_unset_param_v2(struct imonitor_eventobj *eventobj,
		const char* param);
#endif

static int imonitor_convert_string(struct imonitor_eventobj *eventobj, char **pbuf);
static struct imonitor_param* create_imonitor_param(void);
static void destroy_imonitor_param(struct imonitor_param* p);
static struct imonitor_param* get_imonitor_param(struct imonitor_param *head, int key, const char* key_s);
static void del_imonitor_param(struct imonitor_eventobj *obj, int key, const char* key_s);
static void add_imonitor_param(struct imonitor_eventobj *obj, struct imonitor_param *param);

static struct imonitor_param* create_imonitor_param(void)
{
	struct imonitor_param* param = (struct imonitor_param *)vmalloc(sizeof(struct imonitor_param));
	if (param == NULL) {
		return NULL;
	}
	param->key = -1;
	param->key_v2 = NULL;
	param->value = NULL;
	param->next = NULL;
	return param;
}

static void destroy_imonitor_param(struct imonitor_param* p)
{
	if (p == NULL) {
		return;
	}
	if (p->value != NULL) {
		vfree(p->value);
	}
	if (p->key_v2 != NULL) {
		kfree(p->key_v2);
	}
	vfree(p);
}

static struct imonitor_param* get_imonitor_param(struct imonitor_param *head, int key, const char* key_s)
{
	struct imonitor_param *p = head;
	while (p != NULL) {
		if (key_s != NULL) {
			if (strcmp(p->key_v2, key_s) == 0) {
				return p;
			}
		} else if (p->key == key) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

static void del_imonitor_param(struct imonitor_eventobj *obj, int key, const char* key_s)
{
	struct imonitor_param *prev = NULL;
	struct imonitor_param *p = obj->head;
	while (p != NULL) {
		int is_found = 0;
		if (key_s != NULL) {
			if (strcmp(p->key_v2, key_s) == 0) {
				is_found = 1;
			}
		} else if (p->key == key) {
			is_found = 1;
		}
		if (is_found) {
			if (prev == NULL) {
				obj->head = p->next;
			} else {
				prev->next = p->next;
			}
			destroy_imonitor_param(p);
			break;
		}
		prev = p;
		p = p->next;
	}
}

static void add_imonitor_param(struct imonitor_eventobj *obj, struct imonitor_param *param)
{
	if (obj->head == NULL) {
		obj->head = param;
	} else {
		struct imonitor_param *p = obj->head;
		while (p->next != NULL) {
			p = p->next;
		}
		p->next = param;
	}
}

struct imonitor_eventobj *imonitor_create_eventobj(unsigned int eventid)
{
	/*Look for module*/
	unsigned int i;

	const struct imonitor_module_index *module_index = NULL;

	const struct imonitor_event_index *event_index = NULL;

	struct imonitor_eventobj *eventobj = NULL;

	for (i = 0; i < imonitor_modules_count; i++) {
		if (eventid >= imonitor_modules_table[i].min_eventid
		&& eventid <= imonitor_modules_table[i].max_eventid) {
			module_index = &(imonitor_modules_table[i]);
			break;
		}
	}

	if (NULL == module_index) {
		//hwlog_err("Invalid eventid: %d", eventid);
	} else {
		/*Look for eventid description*/
		for (i = 0; i < module_index->events_count; i++) {
			if (eventid == module_index->events[i].eventid) {
				event_index = &(module_index->events[i]);
				break;
			}
		}
	}
	if (NULL == event_index) {
		//hwlog_err("Invalid eventid: %d", eventid);
	}

	/*combined event obj struct*/
	eventobj = vmalloc(sizeof(struct imonitor_eventobj));
	if (NULL == eventobj)
		return NULL;
	eventobj->eventid = eventid;
	eventobj->api_version = 0;
	hwlog_info("imonitor_create_eventobj: %d", eventid);

	/*Initial path NULL*/
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		eventobj->dynamic_path[i] = NULL;
		eventobj->dynamic_path_delete[i] = NULL;
	}

	/*Time*/
	eventobj->time = 0;

	/*below are parameters related*/
	if (NULL == event_index || 0 == event_index->params_count) {/*no parameters for this event*/
		eventobj->params_count = 0;
		eventobj->desc = NULL;
		eventobj->head = NULL;
		return (void *)eventobj;
	}

	eventobj->params_count = event_index->params_count;
	eventobj->desc = event_index->desc;
	eventobj->head = NULL;

	return (void *)eventobj;
}
EXPORT_SYMBOL(imonitor_create_eventobj);

int imonitor_set_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value)
{
	unsigned short type;
	char *param;

	if (NULL == eventobj) {
		hwlog_err("Bad param for imonitor_set_param");
		return -EINVAL;
	}

	if (paramid >= eventobj->params_count) {
		hwlog_err("Bad param for imonitor_set_param, paramid: %d",
					paramid);
		return -EINVAL;
	}

	type = eventobj->desc[paramid];
	switch (type) {
	case BIT_TYPE:
		if (((char)value) > 1) {
			hwlog_err("Invalid value for bit type, value: %d",
						(int)value);
			return -EINVAL;
		}
		/*go through down*/
	case TINYINT_TYPE:
	case SMALLINT_TYPE:
	case INT_TYPE:
		imonitor_set_param_integer(eventobj, paramid, value);
		break;
	case FLOAT_TYPE:
		/* kernel not support float */
		break;
	case DATETIME_TYPE:
		if (0 == value)
			hwlog_err("Invalid value for datetime type");
		else {
			int len = DATETIME_TYPE_LEN;
			param = (char*)vmalloc(len);
			if (param == NULL) {
				break;
			}
			strncpy((char *)param, (char *)value, len);
			param[len - 1] = '\0';
			imonitor_set_param_string(eventobj, paramid, param);
			vfree(param);
		}
		break;
	case CLASS_TYPE:
		/* kernel not support class */
		break;
	default:
		if (type > BASE_TYPE_MAX && type <= VARCHAR_TYPE) {
			hwlog_err("type error: %d", type);
			return -EINVAL;
		}
		if (0 == value)
			hwlog_err("Invalid value for varchar type");
		else {
			int len = type - VARCHAR_TYPE;
			if (len <= 0) {
				break;
			}
			param = (char*)vmalloc(len);
			if (param == NULL) {
				break;
			}
			strncpy((char *)param, (char *)value, len);
			param[len - 1] = '\0';
			imonitor_set_param_string(eventobj,paramid, param);
			vfree(param);
		}
		break;
	}
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param);

int imonitor_set_param_integer(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value)
{
	struct imonitor_param *param;
	if (NULL == eventobj) {
		hwlog_err("Bad param for imonitor_set_param_integer");
		return -EINVAL;
	}
	CHECK_V1_API(eventobj);
	param = get_imonitor_param(eventobj->head, (int)paramid, NULL);
	if (param == NULL) {
		param = create_imonitor_param();
		if (param == NULL) {
			return -ENOMEM;
		}
		param->key = paramid;
		add_imonitor_param(eventobj, param);
	}
	if (param->value != NULL) {
		vfree(param->value);
	}
	param->value = (char*)vmalloc(21);
	if (param->value == NULL) {
		return -ENOMEM;
	}
	snprintf(param->value, 21, "%d", (int)value);
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param_integer);

int imonitor_set_param_string(struct imonitor_eventobj *eventobj,
		unsigned short paramid, const char* value)
{
	struct imonitor_param *param;
	int len;
	if (NULL == eventobj || NULL == value) {
		hwlog_err("Bad param for imonitor_set_param_string");
		return -EINVAL;
	}
	CHECK_V1_API(eventobj);
	param = get_imonitor_param(eventobj->head, (int)paramid, NULL);
	if (param == NULL) {
		param = create_imonitor_param();
		if (param == NULL) {
			return -ENOMEM;
		}
		param->key = paramid;
		add_imonitor_param(eventobj, param);
	}
	if (param->value != NULL) {
		vfree(param->value);
	}
	len = strlen(value);
	if (len > MAX_STR_LEN) {
		// prevent length larger than MAX_STR_LEN
		len = MAX_STR_LEN;
	}
	param->value = vmalloc(len + 1);
	if (param->value == NULL) {
		return -ENOMEM;
	}
	strncpy(param->value, value, len);
	param->value[len] = '\0';
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param_string);

int imonitor_unset_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid)
{
	if (NULL == eventobj) {
		hwlog_err("Bad param for imonitor_unset_param");
		return -EINVAL;
	}

	CHECK_V1_API(eventobj);
	del_imonitor_param(eventobj, (int)paramid, NULL);
	return 0;
}
EXPORT_SYMBOL(imonitor_unset_param);

int imonitor_set_param_integer_v2(struct imonitor_eventobj *eventobj,
		const char* param, long value)
{
	struct imonitor_param *i_param;
	if (NULL == eventobj || NULL == param) {
		hwlog_err("Bad param for imonitor_set_param_integer_v2");
		return -EINVAL;
	}
	CHECK_V2_API(eventobj);
	i_param = get_imonitor_param(eventobj->head, -1, param);
	if (i_param == NULL) {
		i_param = create_imonitor_param();
		if (i_param == NULL) {
			return -ENOMEM;
		}
		i_param->key_v2 = kstrdup(param, GFP_ATOMIC);
		add_imonitor_param(eventobj, i_param);
	}
	if (i_param->value != NULL) {
		vfree(i_param->value);
	}
	i_param->value = (char*)vmalloc(21);
	if (i_param->value == NULL) {
		return -ENOMEM;
	}
	snprintf(i_param->value, 21, "%d", (int)value);
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param_integer_v2);

int imonitor_set_param_string_v2(struct imonitor_eventobj *eventobj,
		const char* param, const char* value)
{
	struct imonitor_param *i_param;
	int len;
	if (NULL == eventobj || NULL == param || NULL == value) {
		hwlog_err("Bad param for imonitor_set_param_string_v2");
		return -EINVAL;
	}
	CHECK_V2_API(eventobj);
	i_param = get_imonitor_param(eventobj->head, -1, param);
	if (i_param == NULL) {
		i_param = create_imonitor_param();
		if (i_param == NULL) {
			return -ENOMEM;
		}
		i_param->key_v2 = kstrdup(param, GFP_ATOMIC);
		add_imonitor_param(eventobj, i_param);
	}
	if (i_param->value != NULL) {
		vfree(i_param->value);
	}
	len = strlen(value);
	if (len > MAX_STR_LEN) {
		// prevent length larger than MAX_STR_LEN
		len = MAX_STR_LEN;
	}
	i_param->value = vmalloc(len + 1);
	if (i_param->value == NULL) {
		return -ENOMEM;
	}
	strncpy(i_param->value, value, len);
	i_param->value[len] = '\0';
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param_string_v2);

int imonitor_unset_param_v2(struct imonitor_eventobj *eventobj,
		const char* param)
{
	if (NULL == eventobj || NULL == param) {
		hwlog_err("Bad param for imonitor_unset_param_v2");
		return -EINVAL;
	}

	CHECK_V2_API(eventobj);
	del_imonitor_param(eventobj, -1, param);
	return 0;
}
EXPORT_SYMBOL(imonitor_unset_param_v2);

int imonitor_set_time(struct imonitor_eventobj *eventobj, long long seconds)
{
	if (NULL == eventobj || 0 == seconds) {
		hwlog_err("Bad param for imonitor_set_time");
		return -EINVAL;
	}
	eventobj->time = seconds;
	return 0;
}
EXPORT_SYMBOL(imonitor_set_time);

static int add_path(char **pool, const char *path)
{
	int i;

	if (NULL == path || 0 == path[0]) {
		hwlog_err("Bad param add_path");
		return -EINVAL;
	}

	if (strlen(path) > MAX_PATH_LEN) {
		hwlog_err("file path over max: %d", MAX_PATH_LEN);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL != pool[i])
			continue;
		pool[i] = kstrdup(path, GFP_ATOMIC);
		break;
	}

	if (MAX_PATH_NUMBER == i) {
		hwlog_err("Too many pathes");
		return -EINVAL;
	}
	return 0;
}

int imonitor_add_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path)
{
	if (NULL == eventobj) {
		hwlog_err("Bad param imonitor_add_dynamic_path");
		return -EINVAL;
	}
	return add_path(eventobj->dynamic_path, path);
}
EXPORT_SYMBOL(imonitor_add_dynamic_path);

int imonitor_add_and_del_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path)
{
	if (NULL == eventobj) {
		hwlog_err("Bad param imonitor_add_and_del_dynamic_path");
		return -EINVAL;
	}
	return add_path(eventobj->dynamic_path_delete, path);
}
EXPORT_SYMBOL(imonitor_add_and_del_dynamic_path);

/* make string ":" to "::", ";" to ";;", and remove newline character
   for example: "abc:def;ghi" transfer to "abc::def;;ghi" */
static char *make_regular(char *value)
{
	int count = 0;
	int len = 0;
	char *temp, *regular, *regular_temp;

	temp = value;
	while (*temp != '\0') {
		if (*temp == ':')
			count++;
		else if (*temp == ';')
			count++;
		else if (*temp == '\n' || *temp == '\r')
			*temp = ' ';
		temp++;
		len++;
	}
	if (count == 0) {
		/* no need to transfer, just return old value */
		return value;
	}
	regular = vmalloc(len + count*2 + 1);
	if (regular == NULL) {
		return NULL;
	}
	regular_temp = regular;
	temp = value;
	while (*temp != 0) {
		if (*temp == ':' || *temp == ';')
			*regular_temp++ = *temp;
		*regular_temp++ = *temp;
		temp++;
	}
	*regular_temp = '\0';
	return regular;
}

static int imonitor_convert_string(struct imonitor_eventobj *eventobj, char **pbuf)
{
	int len;
	char *tmp;
	int tmplen;
	unsigned int i = 0;
	unsigned int keycount = 0;
	struct imonitor_param *p;
	char *buf = vmalloc(EVENT_INFO_BUF_LEN);
	if(NULL == buf)
	{
		goto ERRORDONE;
	}

	len = EVENT_INFO_BUF_LEN;
	tmp = buf;
	/*fill eventid*/
	tmplen = snprintf(tmp, len, "eventid %d", eventobj->eventid);
	BUF_POINTER_FORWARD;

	/*fill the path*/
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL == eventobj->dynamic_path[i])
			break;
		tmplen = snprintf(tmp, len, " -i %s",
					eventobj->dynamic_path[i]);
		BUF_POINTER_FORWARD;
	}
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL == eventobj->dynamic_path_delete[i])
			break;
		tmplen = snprintf(tmp, len, " -d %s",
					eventobj->dynamic_path_delete[i]);
		BUF_POINTER_FORWARD;
	}

	/*fill time*/
	if (eventobj->time) {
		tmplen = snprintf(tmp, len, " -t %lld",  eventobj->time);
		BUF_POINTER_FORWARD;
	}

	/*fill the param info*/
	keycount = 0;
	p = eventobj->head;
	while (p != NULL) {
		char *value, *regular_value;
		int need_free = 1;

		if (p->value == NULL) {
			p = p->next;
			continue;
		}
		if (0 == keycount) {
			tmplen = snprintf(tmp, len, " --extra ");
			BUF_POINTER_FORWARD;
		}
		keycount++;
		/*fill key*/
		if (p->key_v2 != NULL) {
			tmplen = snprintf(tmp, len, "%s:", p->key_v2);
		} else  {
			tmplen = snprintf(tmp, len, "%d:", p->key);
		}
		BUF_POINTER_FORWARD;
		/*fill value*/
		tmplen = 0;

		value = p->value;
		regular_value = make_regular(value);
		if (regular_value == NULL) {
			regular_value = "NULL";
			need_free = 0;
		}
		tmplen = snprintf(tmp, len, "%s;",
					regular_value);
		if (value != regular_value && need_free)
			vfree(regular_value);
		BUF_POINTER_FORWARD;
		p = p->next;
	}

	*pbuf = buf;
	return (EVENT_INFO_BUF_LEN - len);

ERRORDONE:
	if (NULL != buf)
		vfree(buf);
	*pbuf = NULL;
	return 0;
}

static int imonitor_write_log_exception(char *str, const int strlen)
{
	char        tempchr;
	int         nLeftBufLen = strlen+1;
	char       *strptr = str;
	int         sentcnt = 0;

	while (nLeftBufLen > 0) {
		if (nLeftBufLen <= EVENT_INFO_PACK_BUF_LEN) {
			logbuf_to_exception(0, 0, IDAP_LOGTYPE_CMD, 0, strptr, nLeftBufLen);
			sentcnt++;
			nLeftBufLen = 0;
			break;
		} else {
			tempchr = strptr[EVENT_INFO_PACK_BUF_LEN-1];
			strptr[EVENT_INFO_PACK_BUF_LEN-1] = '\0';
			logbuf_to_exception(0, 0, IDAP_LOGTYPE_CMD, 1, strptr, EVENT_INFO_PACK_BUF_LEN);
			nLeftBufLen -= (EVENT_INFO_PACK_BUF_LEN-1);
			strptr += (EVENT_INFO_PACK_BUF_LEN-1);
			strptr[0] = tempchr;
			sentcnt++;
		}
	}

	return sentcnt;
}

static void imonitor_file_lock(struct file *filp, int cmd)
{
	struct file_lock *fl;

	fl = locks_alloc_lock();
	if (NULL == fl) {
		hwlog_err("imonitor_file_lock alloc error");
		return;
	}
	fl->fl_file = filp;
	fl->fl_owner = filp;
	fl->fl_pid = 0;
	fl->fl_flags = FL_FLOCK;
	fl->fl_type = cmd;
	fl->fl_end = OFFSET_MAX;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0))
	locks_lock_file_wait(filp, fl);
#else
	flock_lock_file_wait(filp, fl);
#endif
	locks_free_lock(fl);
}

int imonitor_send_event(struct imonitor_eventobj *obj)
{
	char   *str = NULL;
	int     buflen = 0;
	int     nSentPacket = 0;
	struct file *fp;

	if (NULL == obj) {
		hwlog_err("Bad param imonitor_send_event");
		return -EINVAL;
	}

	buflen = imonitor_convert_string(obj, &str);
	if (NULL == str)
		return -EINVAL;

	fp = filp_open("/dev/hwlog_exception", O_WRONLY, 0);
	if (IS_ERR(fp)) {
		hwlog_info("imonitor_send_event open fail");
		nSentPacket = imonitor_write_log_exception(str, buflen);
	} else {
		imonitor_file_lock(fp, F_WRLCK);
		nSentPacket = imonitor_write_log_exception(str, buflen);
		imonitor_file_lock(fp, F_UNLCK);
		filp_close(fp, 0);
	}
	hwlog_info("imonitor send event: %s", str);
	vfree(str);
	return nSentPacket;
}
EXPORT_SYMBOL(imonitor_send_event);

void imonitor_destroy_eventobj(struct imonitor_eventobj *eventobj)
{
	int i;
	struct imonitor_param *p;
	if (NULL == eventobj)
		return;

	p = eventobj->head;
	while (p != NULL) {
		struct imonitor_param *del = p;
		p = p->next;
		destroy_imonitor_param(del);
	}
	eventobj->head = NULL;

	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (eventobj->dynamic_path[i] != NULL) {
			kfree(eventobj->dynamic_path[i]);
			eventobj->dynamic_path[i] = NULL;
		}
		if (eventobj->dynamic_path_delete[i] != NULL) {
			kfree(eventobj->dynamic_path_delete[i]);
			eventobj->dynamic_path_delete[i] = NULL;
		}
	}

	vfree(eventobj);
}
EXPORT_SYMBOL(imonitor_destroy_eventobj);
