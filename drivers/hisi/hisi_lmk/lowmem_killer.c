#define pr_fmt(fmt) "hisi_lowmem: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/swap.h>

#include "lowmem_killer.h"
#define CREATE_TRACE_POINTS
#include "lowmem_trace.h"

#include <linux/version.h>
#define ZONE_DMA_LOWMEM_RATIO 30

static int nzones = -1;

static struct zone *dma_zone;

static inline void hisi_lowmem_init(void)
{
	int n = 0;
	struct zone *zone;

	for_each_populated_zone(zone) {
		if (n == 0)
			dma_zone = zone;
		n++;
	}
	nzones = n;
}

int hisi_lowmem_tune(int *other_free, int *other_file,
		     struct shrink_control *sc)
{
	if (nzones == -1)
		hisi_lowmem_init();

	if ((nzones > 1) && (sc->gfp_mask & __GFP_DMA)) {
		int zone_free, zone_file;

		/*lint -save -e834 */
		zone_free = (int)zone_page_state(dma_zone, NR_FREE_PAGES)
			- (int)zone_page_state(dma_zone, NR_FREE_CMA_PAGES)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
			- (int)dma_zone->totalreserve_pages;
#else
			- (int)dma_zone->zone_pgdat->totalreserve_pages;
#endif
		zone_file = (int)zone_page_state(dma_zone, NR_FILE_PAGES)
			- (int)zone_page_state(dma_zone, NR_SHMEM)
			- (int)zone_page_state(dma_zone, NR_SWAPCACHE);
		trace_lowmem_tune(nzones, sc->gfp_mask, *other_free,
				  *other_file, zone_free, zone_file);
		*other_free = zone_free * 100 / ZONE_DMA_LOWMEM_RATIO;
		*other_file = zone_file * 100 / ZONE_DMA_LOWMEM_RATIO;
		/*lint -restore */
		return 2;
	}

	if (!(sc->gfp_mask & ___GFP_CMA)) {
		int nr_free_cma;

		nr_free_cma = (int)global_page_state(NR_FREE_CMA_PAGES);
		trace_lowmem_tune(nzones, sc->gfp_mask, *other_free,
				  *other_file, -nr_free_cma, 0);
		*other_free -= nr_free_cma;
		return 1;
	}

	return 0;
}
