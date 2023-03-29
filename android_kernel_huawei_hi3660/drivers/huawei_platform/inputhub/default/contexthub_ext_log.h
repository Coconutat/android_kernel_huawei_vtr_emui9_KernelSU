/**********************************************************
 * Filename:  inputhub_ext_log.h
 *
 * Discription: some functions of sensorhub power
 *
 * Owner:DIVS_SENSORHUB
 *
**********************************************************/
#ifndef __IOMCU_EXT_LOG_H
#define __IOMCU_EXT_LOG_H

struct inputhub_ext_notifier_node {
	struct list_head entry;
	int tag;
	int (*notify)(const pkt_header_t* data);
};

struct inputhub_ext_log_notifier {
	struct list_head head;
	spinlock_t lock;
};

typedef struct {
	pkt_header_t hd;
	uint8_t tag;
	uint8_t data[];
} ext_logger_req_t;

typedef struct {
	uint8_t type;
	uint16_t len;
	uint8_t data[];
} pedo_ext_logger_req_t;

int inputhub_ext_log_init(void);
int is_inputhub_ext_log_notify(const pkt_header_t* head);
int inputhub_ext_log_register_handler(int tag, int (*notify)(const pkt_header_t* head));

#endif
