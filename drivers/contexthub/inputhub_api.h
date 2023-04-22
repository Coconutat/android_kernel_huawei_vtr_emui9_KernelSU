#ifndef __HISI_INPUTHUB_API_H__
#define __HISI_INPUTHUB_API_H__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>
#include "protocol.h"
#ifdef CONFIG_INPUTHUB_20
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "contexthub_recovery.h"
#else
#include "inputhub_route.h"
#include "inputhub_bridge.h"

extern int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length, read_info_t *rd);
extern int inputhub_mcu_write_cmd_nolock(const void *buf, unsigned int length);
extern int register_iom3_recovery_notifier(struct notifier_block *nb);
#endif

#define CONTEXTHUB_HEADER_SIZE (sizeof(pkt_header_t) + sizeof(unsigned int))

extern int register_mcu_event_notifier(int tag, int cmd, int (*notify)(const pkt_header_t *head));
extern int unregister_mcu_event_notifier(int tag, int cmd, int (*notify) (const pkt_header_t *head));
extern int getSensorMcuMode(void);
int send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, char  *buf, size_t count);
int send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, char  *buf, size_t count, struct read_info *rd);
int send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, char  *buf, size_t count);
#endif
