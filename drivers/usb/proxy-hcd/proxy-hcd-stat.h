#ifndef _PROXY_HCD_STAT_H_
#define _PROXY_HCD_STAT_H_

#include <linux/time.h>

struct proxy_hcd_usb_device_stat {
	unsigned long stat_urb_complete_pipe_err;
};

static inline void urb_complete_pipe_err_add_stat(struct proxy_hcd_usb_device_stat *stat)
{
	stat->stat_urb_complete_pipe_err++;
}

struct proxy_hcd_urb_stat {
	unsigned long		stat_urb_enqueue;
	unsigned long		stat_urb_dequeue;
	unsigned long		stat_urb_enqueue_fail;
	unsigned long		stat_urb_dequeue_fail;
	unsigned long		stat_urb_dequeue_giveback;
	unsigned long		stat_urb_giveback;
	unsigned long		stat_urb_complete;
	unsigned long		stat_urb_complete_fail;
};

static inline void urb_enqueue_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_enqueue++;
}

static inline void urb_dequeue_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_dequeue++;
}

static inline void urb_enqueue_fail_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_enqueue_fail++;
}

static inline void urb_dequeue_fail_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_dequeue_fail++;
}

static inline void urb_dequeue_giveback_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_dequeue_giveback++;
}

static inline void urb_giveback_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_giveback++;
}

static inline void urb_complete_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_complete++;
}

static inline void urb_complete_fail_add_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_complete_fail++;
}

static inline void reset_urb_stat(struct proxy_hcd_urb_stat *stat)
{
	stat->stat_urb_enqueue = 0;
	stat->stat_urb_dequeue = 0;
	stat->stat_urb_enqueue_fail = 0;
	stat->stat_urb_dequeue_fail = 0;
	stat->stat_urb_dequeue_giveback = 0;
	stat->stat_urb_giveback = 0;
	stat->stat_urb_complete = 0;
	stat->stat_urb_complete_fail = 0;
}

struct proxy_hcd_stat {
	unsigned int stat_alloc_dev;
	unsigned int stat_free_dev;

	unsigned int stat_hub_control;
	unsigned int stat_hub_status_data;

	unsigned long last_hub_control_time;
	unsigned long last_hub_status_data_time;

	unsigned int stat_bus_suspend;
	unsigned int stat_bus_resume;
};

static inline void alloc_dev_add_stat(struct proxy_hcd_stat *stat)
{
	stat->stat_alloc_dev++;
}

static inline void free_dev_add_stat(struct proxy_hcd_stat *stat)
{
	stat->stat_free_dev++;
}

static inline void hub_control_add_stat(struct proxy_hcd_stat *stat)
{
	struct timespec uptime;
	stat->stat_hub_control++;
	get_monotonic_boottime(&uptime);
	stat->last_hub_control_time = (unsigned long)uptime.tv_sec;
}

static inline void hub_status_data_add_stat(struct proxy_hcd_stat *stat)
{
	struct timespec uptime;

	stat->stat_hub_status_data++;
	get_monotonic_boottime(&uptime);
	stat->last_hub_status_data_time = (unsigned long)uptime.tv_sec;
}

static inline void bus_suspend_add_stat(struct proxy_hcd_stat *stat)
{
	stat->stat_bus_suspend++;
}

static inline void bus_resume_add_stat(struct proxy_hcd_stat *stat)
{
	stat->stat_bus_resume++;
}

static inline void reset_proxy_hcd_stat(struct proxy_hcd_stat *stat)
{
	stat->stat_alloc_dev = 0;
	stat->stat_free_dev = 0;
	stat->stat_hub_control = 0;
	stat->stat_hub_status_data = 0;
	stat->last_hub_control_time = 0;
	stat->last_hub_status_data_time = 0;
	stat->stat_bus_suspend = 0;
	stat->stat_bus_resume = 0;
}

#endif /* _PROXY_HCD_STAT_H_ */
