#ifndef _HIFI_USB_STAT_H_
#define _HIFI_USB_STAT_H_

#include <linux/atomic.h>

struct hifi_usb_stats {
	unsigned stat_send_msg;
	unsigned stat_receive_msg;
	unsigned stat_wait_msg_timeout;

	atomic_t stat_urb_enqueue_msg;
	unsigned stat_urb_complete_msg;
	atomic_t stat_urb_dequeue_msg;

	unsigned stat_hub_status_change_msg;
};

#define HIFI_USB_STAT_FUNC(_stat);						\
	static inline void hifi_usb ## _stat(struct hifi_usb_stats *stats)	\
	{									\
		stats->_stat++;							\
	}

#define HIFI_USB_STAT_ATOMIC_FUNC(_stat);					\
	static inline void hifi_usb ## _stat(struct hifi_usb_stats *stats)	\
	{									\
		atomic_inc(&stats->_stat);					\
	}

HIFI_USB_STAT_FUNC(stat_send_msg);
HIFI_USB_STAT_FUNC(stat_receive_msg);
HIFI_USB_STAT_FUNC(stat_wait_msg_timeout);
HIFI_USB_STAT_ATOMIC_FUNC(stat_urb_enqueue_msg);
HIFI_USB_STAT_FUNC(stat_urb_complete_msg);
HIFI_USB_STAT_ATOMIC_FUNC(stat_urb_dequeue_msg);
HIFI_USB_STAT_FUNC(stat_hub_status_change_msg);

#define HIFI_USB_STAT(_stat, _stats)					\
	hifi_usb ## _stat(_stats)

#endif /* _HIFI_USB_STAT_H_ */
