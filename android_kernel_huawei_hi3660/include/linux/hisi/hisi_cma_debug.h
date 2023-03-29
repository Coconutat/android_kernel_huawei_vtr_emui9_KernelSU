#ifndef _LINUX_HISI_CMD_DEBUG_H
#define _LINUX_HISI_CMD_DEBUG_H

static inline unsigned int cma_bitmap_used(struct cma *cma)
{
	unsigned int used;

	used = (unsigned int)bitmap_weight(cma->bitmap, (unsigned int)cma->count);

	return used << cma->order_per_bit;
}

static inline unsigned long int cma_bitmap_maxchunk(struct cma *cma)
{
	unsigned long maxchunk = 0;
	unsigned long start, end = 0;

	for (;;) {
		start = find_next_zero_bit(cma->bitmap, cma->count, end);
		if (start >= cma->count)
			break;
		end = find_next_bit(cma->bitmap, cma->count, start);
		maxchunk = max(end - start, maxchunk);
	}

	return (maxchunk << cma->order_per_bit);
}

static inline void dump_cma_page_flags(unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);

	pr_info("[%s] PFN %lu Block %lu Flags %s%s%s%s%s%s%s%s%s%s%s%s flags 0x%lx page_count(page) %d page_mapcount(page) %d\n",
			__func__,
			pfn,
			pfn >> pageblock_order,
			PageLocked(page)	? "K" : " ",
			PageError(page)		? "E" : " ",
			PageReferenced(page)	? "R" : " ",
			PageUptodate(page)	? "U" : " ",
			PageDirty(page)		? "D" : " ",
			PageLRU(page)		? "L" : " ",
			PageActive(page)	? "A" : " ",
			PageSlab(page)		? "S" : " ",
			PageWriteback(page)	? "W" : " ",
			PageCompound(page)	? "C" : " ",
			PageSwapCache(page)	? "B" : " ",
			PageMappedToDisk(page)	? "M" : " ",
			page->flags,
			page_count(page),
			PageSlab(page) ? 0 : page_mapcount(page));
}

static inline void dump_cma_page(struct cma *cma, size_t count,
			unsigned long mask, unsigned long offset,
			unsigned long bitmap_maxno, unsigned long bitmap_count)
{
	unsigned int used = 0;
	unsigned long maxchunk = 0;
	unsigned long bitmap_no;
	unsigned long start = 0;
	struct page *page;
	unsigned long pfn, pfn_end;

	mutex_lock(&cma->lock);
	used = cma_bitmap_used(cma);
	maxchunk = cma_bitmap_maxchunk(cma);
	pr_info("total %lu KB mask 0x%lx used %u KB maxchunk %lu KB alloc %lu KB\n",
			cma->count << (PAGE_SHIFT - 10), mask,
			used << (PAGE_SHIFT - 10),
			maxchunk << (PAGE_SHIFT - 10),
			count << (PAGE_SHIFT - 10));

	for (;;) {
		bitmap_no = bitmap_find_next_zero_area_off(cma->bitmap,
			bitmap_maxno, start, bitmap_count, mask, offset);
		if (bitmap_no >= bitmap_maxno)
			break;

		pfn = cma->base_pfn + (bitmap_no << cma->order_per_bit);
		pfn_end = pfn + count;
		while (pfn < pfn_end) {
			if (!pfn_valid_within(pfn)) {
				pfn++;
				continue;
			}
			page = pfn_to_page(pfn);
			if (PageBuddy(page)) {
				pfn += 1 << page_order(page);
			} else {
				dump_cma_page_flags(pfn);
				break;
			}
		}

		/* try again with a bit different memory target */
		start = bitmap_no + mask + 1;
	}
	mutex_unlock(&cma->lock);
}

#endif
