#include "shared_memory_check.h"

char inject_func_name[2][40] = {
	{"ion_page_pool_free_immediate\0"},
	{"remap_pfn_range\0"},
};

//recek func for __free_pages after delay
void __free_pages_recheck(struct transaction_node* node) {
	void *addr;
	int order;
	char* buffer;
	int size;
	struct page* mpage;

	if (node == NULL){
		printk("[SharedMemCheck]para of __free_pages_recheck() is NULL.\n");
		return;
	}
	if (node->para0 == NULL){
		printk("[SharedMemCheck]para 0 struct transaction_node is NULL\n");
		return;
	}

	if (node->para1 == NULL){
		printk("[SharedMemCheck]para 1 struct transaction_node is NULL\n");
		return;
	}

	addr = node->para0;
	order = *((int *)(node->para1));
	if (order < 0) {
		printk("[SharedMemCheck]invalid order:%d\n",order);
		return;
	}

	buffer = (char *)addr;
	size = PAGE_SIZE << order;
	mpage = virt_to_page(addr);
	clear_shared_memory_remark(mpage);

	//check the memory
	if (memory_check_base_on_page(buffer, size, order) != 0) {
		panic("[Share Memory Check]:shared memory use after free!  [time]:%ld  [HZ]:%d  [inject func]:%s  [addr]:%lx  [size]:%d\n", jiffies - node->expires, HZ, inject_func_name[node->probe_type], (unsigned long)addr, size);	
	}

	//free the memory. use the impletement of __free_pages
    __free_pages(mpage, order);
}
