#ifndef __ERECOVERY_H
#define __ERECOVERY_H

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROCESSNAME_LENGTH 128
#define MAX_FINGERPRINT_LENGTH 16
#define MAX_REASON_LENGTH 64
#define MAX_RESERVED_LENGTH 128
#define ERECOVERY_CMD_LEN_MAX (512)
#define ERECOVERY_MSG_LEN_MAX (15 * 1024)
#define ERECOVERY_CMD_INVALID (0xFF)

typedef struct {
    long erecovery_id;
    long fault_id;
    long pid;
    char process_name[MAX_PROCESSNAME_LENGTH];
    char fingerprint[MAX_FINGERPRINT_LENGTH];
    long time;
    long state;
    long result;
    char reason[MAX_REASON_LENGTH];
    char Reserved[MAX_RESERVED_LENGTH];
} erecovery_eventobj;

typedef struct {
    uint32_t magic;
    erecovery_eventobj ere_obj;
} erecovery_write_event;

long erecovery_report(erecovery_eventobj* eventdata);
long erecovery_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#ifdef __cplusplus
}
#endif
#endif
