/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <mali_kbase.h>
#include "mali_kbase_hisi_callback.h"

#ifdef CONFIG_HISI_LAST_BUFFER
extern hisi_lb_callbacks lb_callbacks;
#endif

#ifdef MALI_HISI_CL_BOOST
void gpu_cl_boost_init(void *dev)
{
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	atomic_set(&kbdev->pm.backend.metrics.time_compute_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_vertex_jobs, 0);
	atomic_set(&kbdev->pm.backend.metrics.time_fragment_jobs, 0);
}

void gpu_cl_boost_update_utilization(void *dev, void *atom, u64 microseconds_spent)
{
	struct kbase_jd_atom *katom;
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	katom = (struct kbase_jd_atom *)atom;
	KBASE_DEBUG_ASSERT(katom != NULL);

	if (katom->core_req & BASE_JD_REQ_ONLY_COMPUTE)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_compute_jobs);
	else if (katom->core_req & BASE_JD_REQ_FS)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_fragment_jobs);
	else if (katom->core_req & BASE_JD_REQ_CS)
		atomic_add((microseconds_spent >> KBASE_PM_TIME_SHIFT), &kbdev->pm.backend.metrics.time_vertex_jobs);
}
#endif

struct kbase_hisi_callbacks hisi_callbacks = {
#ifdef MALI_HISI_CL_BOOST
	.cl_boost_init = gpu_cl_boost_init,
	.cl_boost_update_utilization = gpu_cl_boost_update_utilization,
#else
	.cl_boost_init = NULL,
	.cl_boost_update_utilization = NULL,
#endif

#ifdef CONFIG_HISI_LAST_BUFFER
	.lb_cbs = &lb_callbacks,
#endif
};

uintptr_t gpu_get_callbacks(void)
{
	return ((uintptr_t)&hisi_callbacks);
}


