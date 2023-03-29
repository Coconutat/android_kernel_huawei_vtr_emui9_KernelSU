#ifndef _HISI_CALLBACK_H_
#define _HISI_CALLBACK_H_

#include <mali_kbase.h>
#ifdef CONFIG_HISI_LAST_BUFFER
#include <last_buffer/mali_kbase_hisi_lb_callback.h>
#endif

#define KBASE_PM_TIME_SHIFT			8


/* This struct holds all of the callbacks of hisi platform. */
struct kbase_hisi_callbacks {
	void (* cl_boost_init)(void *dev);
	void (* cl_boost_update_utilization)(void *dev, void *atom, u64 microseconds_spent);

#ifdef CONFIG_HISI_LAST_BUFFER
	struct kbase_hisi_lb_callbacks *lb_cbs;
#endif
};

uintptr_t gpu_get_callbacks(void);

#endif
