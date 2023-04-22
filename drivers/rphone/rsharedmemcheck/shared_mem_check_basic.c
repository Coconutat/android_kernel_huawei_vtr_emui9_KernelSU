/* this file need not to be modified when new memmory free func is added to the injection list*/

#include "shared_memory_check.h"

char single_filler_page[PAGE_SIZE];
char filler_pages_with_order_4[PAGE_SIZE * (1 << 4)];
char filler_pages_with_order_8[PAGE_SIZE * (1 << 8)];

DEFINE_SPINLOCK(sm_list_lock);

int  total_delay_size[CUR_SM_DELAY_TYPE_NR] ={
	0,     //current delay memeory size for ION SYSTEM HEAP
	0,     //current delay memory size for remap pfn range
};
int total_num = 0;

struct transaction_node head = {
	.probe_type = -1,
};


struct timer_list* main_timer;



void remark_shared_memory_page(struct page* page) {

	if (page == NULL) {
		printk("[SharedMemCheck]remark shared memory page fail because of null pointer.\n");
		return;
	}

	page->flags |= HIT_MASK;

	return;
}

int is_shared_memory_page(struct page* page) {
	if (page == NULL) {
		printk("[SharedMemCheck]check flag of shared memory page fail because of null pointer.\n");
		return false;
	} else {
		return (page->flags & HIT_MASK) == HIT_MASK;
	}
}

void clear_shared_memory_remark(struct page* page) {
	if (page == NULL) {
		printk("[SharedMemCheck]check flag of shared memory page fail because of null pointer.\n");
        return;
	} else {
        page->flags = page->flags & (~HIT_MASK);
	}
	
}

//memory check func that check each byte.
int memory_check_base_on_byte(char* buffer, int size) {
	int i;
	for(i = 0; i < size; i++) {
		if(likely(buffer[i] == FILLER)) {
			continue;
		} else {
            return -1;
		}
    }
	return 0;
}

//memory check func that check each page
int memory_check_base_on_page(char *buffer, int size, int order) {
	int i;
	int step = PAGE_SIZE;
	char* filler_buffer = single_filler_page;
	if (order >= 4 && order < 8) {
		step = PAGE_SIZE * (1<<4);
		filler_buffer = filler_pages_with_order_4;
	} else if(order >= 8) {
		step = PAGE_SIZE * (1<<8);
		filler_buffer = filler_pages_with_order_8;
	}
	
	for(i = 0; i < size; i += step) {
		if(likely(memcmp(filler_buffer, buffer + i, step)) == 0) {
			continue;
		} else {
			return -1;
		}
    }
	return 0;
}

/***************************************************************************************
  definition of fault injection.Normally two types of func should be injected: 
    1. function that we find and remark shared memory by setting flag.
    2. function that shared memory is being freed.Delay it.
  **************************************************************************************/
rasprobe_entry_define(ion_page_pool_free_immediate);
rasprobe_entry_define(__free_pages);
rasprobe_entry_define(remap_pfn_range);

static struct rasprobe *probes[] = {
	&rasprobe_name(ion_page_pool_free_immediate),
	&rasprobe_name(__free_pages),
	&rasprobe_name(remap_pfn_range),
};


//check the delayed memmory list
void check_my_list(void* args) {
	struct transaction_node *node, *next;
	if(list_empty(&(head.list))) {
		printk("list empty.\n");
	}

	list_for_each_entry_safe(node, next, &(head.list), list) {
		if(jiffies - node->expires >= 2*HZ) {
			switch(node->probe_type) {
                default:
					//printk("[call timer in check list]node addr:%lx   probe_type:%d    para0:%lx    para1 addr:%lx, para1 value:%d\n", (unsigned long)node, node->probe_type, (unsigned long)(node->para0), (unsigned long)(node->para1), *((int *)(node->para1)));
					spin_lock(&sm_list_lock);
					list_del(&(node->list));
					total_delay_size[node->probe_type] -= 4096 << (*((int *)(node->para1)));
					total_num--;
					spin_unlock(&sm_list_lock);
                    node->function(node);
					kfree(node->para1);
				    kfree(node);
					break;
			}
		} else {
			break;
		}
	}
	mod_timer(main_timer, jiffies + HZ);
	return;
}


static int tool_init(void)
{
	INIT_LIST_HEAD(&(head.list));

	/*3. initialize probes and interface*/
    total_num = 0;
	memset(single_filler_page, FILLER, PAGE_SIZE);
	memset(filler_pages_with_order_4, FILLER, PAGE_SIZE * (1 << 4));
	memset(filler_pages_with_order_8, FILLER, PAGE_SIZE * (1 << 8));
	rasprobe_name(ion_page_pool_free_immediate).maxactive = 4;
    printk("[memcheck init]total_delay_size[ION SYS]:%d, total_delay_size[REMAP_PAGES]:%d, total_num:%d\n", total_delay_size[0], total_delay_size[1], total_num);
	ras_retn_iferr(register_rasprobes(probes, ARRAY_SIZE(probes)));
	main_timer = (struct timer_list *)kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	init_timer(main_timer);
	main_timer->expires = jiffies + 2*HZ;
	main_timer->function = check_my_list;
	main_timer->data = 0;
	add_timer(main_timer);

	return 0;
}

static void clear_before_module_exit(void) {
	struct transaction_node* node, *next;
	printk("[memcheck exit]memcheck exit\n");
	del_timer(main_timer);
	list_for_each_entry_safe(node, next, &(head.list), list) {
		switch(node->probe_type) {
			case MEM_ION_SYSTEM_HEAP:
	        case MEM_REMAP_PAGES:
					printk("timer func: free_pages  probe_type:%d    para0:%lx    para1 addr:%lx, para1 value:%d\n", node->probe_type, (unsigned long)node->para0, (unsigned long)node->para1, *((int *)(node->para1)));
					list_del(&(node->list));
					node->function(node);
					kfree((unsigned int *)(node->para1));
				    kfree(node);
					break;
				default: break;
			}
	}
}

static void tool_exit(void)
{
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	clear_before_module_exit();
}

module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("Delayed memory free test.");
MODULE_LICENSE("GPL");
MODULE_VERSION("V001R001C151-1.0");
