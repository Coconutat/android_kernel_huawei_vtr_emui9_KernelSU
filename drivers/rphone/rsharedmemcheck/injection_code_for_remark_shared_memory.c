#include "shared_memory_check.h"

int rasprobe_handler_entry(ion_page_pool_free_immediate)(struct rasprobe_instance *ri, struct pt_regs *regs) {
	struct RasRegs *rb;
	struct page* mpage;
	int order;
	char* addr;
	int size;
	struct transaction_node* node;

	rasprobe_entry(ri, regs);
	rb = (struct RasRegs *)ri->data;
	if (rb->args[1] == 0) {
		return 0;
	}

	mpage = (struct page *)rb->args[1];
	if (mpage == NULL){
		return 0;
	}

	if (is_shared_memory_page(mpage)) {
		clear_shared_memory_remark(mpage);
	}

	if (total_delay_size[MEM_ION_SYSTEM_HEAP] > MAX_DELAY_SIZE_FOR_ION_SYSTEM_HEAP) {
		return 0;
	}

	order = compound_order(mpage);
	if(order < 0) return 0;
	addr = page_address(mpage);
	if(addr == NULL) return 0;
	size = PAGE_SIZE << order;

	//register info to list
    node = (struct transaction_node *)kmalloc(sizeof(struct transaction_node), GFP_KERNEL);
    node->expires = jiffies;
	node->function = __free_pages_recheck;
	node->probe_type = MEM_ION_SYSTEM_HEAP;
    node->para0 = page_address(mpage);
    node->para1 = (void *)kmalloc(sizeof(unsigned int), GFP_KERNEL);
    *((unsigned int *)(node->para1)) = order;

	spin_lock(&sm_list_lock);
    list_add_tail(&(node->list), &(head.list));
	total_delay_size[MEM_ION_SYSTEM_HEAP] += size;
	total_num ++;
    spin_unlock(&sm_list_lock);

    //memory poison
	memset((char *)addr, FILLER, size);
	rasprobe_entry(ri, regs);

	//increase page->count so that this page will not be freed by __free_pages because of page->count > 0
    atomic_inc(&mpage->_count);

	return 0;
}

int rasprobe_handler_entry(remap_pfn_range)(struct rasprobe_instance *ri, struct pt_regs* regs) {
	struct RasRegs *rb;
	unsigned long pfn = 0;
	struct page* mpage;
	rasprobe_entry(ri, regs);
	rb = (struct RasRegs *)ri->data;

	if (total_delay_size[MEM_REMAP_PAGES] > MAX_DELAY_SIZE_FOR_REMAP_PAGES) {
		return 0;
	}

	if(rb->args[2] == 0) {
		printk("[SharedMemCheck]remap args is null in injection of remap_pfn_range:%lu\n", (unsigned long)(rb->args[2]));
		return 0;
	}
	pfn = (unsigned long)(rb->args[2]);

	if (pfn <= 0) {
		printk("[SharedMemCheck]error pfn in injection of remap_pfn_range:%lu\n",pfn);
		return 0;
	}
	mpage = pfn_to_page(pfn);
	if (mpage == NULL) {
		printk("[SharedMemCheck]error page address in injection of remap_pfn_range.\n");
		return 0;
		
	}

	//set shared_memmory mark to page flags
	remark_shared_memory_page(mpage);

	return 0;
}


int rasprobe_handler_entry(__free_pages) (struct rasprobe_instance *ri, struct pt_regs *regs) {
	int size = 0;
	struct RasRegs *rb;
	struct transaction_node* node;
	struct page* mpage;
	void* addr;
	int order;
	rasprobe_entry(ri, regs);
	rb = (struct RasRegs *)ri->data;
    if (rb->args[0] == 0) {
    	return 0;
	}
	mpage = (struct page *)rb->args[0];
	order = (unsigned int)(rb->args[1]);

	if (!is_shared_memory_page(mpage)) {
		return 0;
	}

	addr = page_address(mpage);
	if(addr == NULL || order < 0) return 0;
	size = PAGE_SIZE << order;

	//register info to list
    node = (struct transaction_node *)kmalloc(sizeof(struct transaction_node), GFP_KERNEL);
    node->expires = jiffies;
	node->function = __free_pages_recheck;
	node->probe_type = MEM_REMAP_PAGES;
    node->para0 = page_address(mpage);
    node->para1 = (void *)kmalloc(sizeof(unsigned int), GFP_KERNEL);
    *((unsigned int *)(node->para1)) = (unsigned int)(rb->args[1]);
    spin_lock(&sm_list_lock);
    list_add_tail(&(node->list), &(head.list));
	total_delay_size[MEM_REMAP_PAGES] += size;
	total_num ++;
    spin_unlock(&sm_list_lock);

    //memory poison
	memset((char *)addr, FILLER, size);
	rasprobe_entry(ri, regs);
    atomic_inc(&mpage->_count);
	return 0;
}