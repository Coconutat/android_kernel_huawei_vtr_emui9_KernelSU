#include "../rasbase/rasbase.h"
#include "../rasbase/rasprobe.h"
#include <linux/kernel.h>
#include <asm/page.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/statfs.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/random.h>
#define MSEC(time) (time*HZ/1000)
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/mmdebug.h>
#include <linux/kasan.h>


#define FILLER          0x5a
#define HIT_MASK        0x2000000000000000

#define CUR_SM_DELAY_TYPE_NR     2
#define MAX_DELAY_SIZE_FOR_ION_SYSTEM_HEAP     (1 << 23)    //8M
#define MAX_DELAY_SIZE_FOR_REMAP_PAGES         (1 << 26)    //64M

enum mem_probe_type {
	MEM_ION_SYSTEM_HEAP,
	MEM_REMAP_PAGES,
};

extern int total_delay_size[CUR_SM_DELAY_TYPE_NR];

struct transaction_node {
	unsigned long expires;
	void  (*function)(struct transaction_node *);
	void* para0;     //strore the first parameter of timer->function.
	void* para1;     //store the second param of timer->function
	char probe_type;
	struct list_head list;
};

extern spinlock_t sm_list_lock;
extern struct transaction_node head;
extern int total_num;

//API for share memory flag
void remark_shared_memory_page(struct page* page);
int is_shared_memory_page(struct page* page);
void clear_shared_memory_remark(struct page* page);


//memory check func that check each byte.
int memory_check_base_on_byte(char* buffer, int size);

//memory check func that check each page
int memory_check_base_on_page(char *buffer, int size, int order);


//recheck func for the injected memory free func
void __free_pages_recheck(struct transaction_node* node);

//remark point for shared memory flag. the 61th bit of page->flags is used as shared memory flag.
int rasprobe_handler_entry(ion_page_pool_free_immediate)(struct rasprobe_instance *ri, struct pt_regs *regs);
int rasprobe_handler_entry(__free_pages) (struct rasprobe_instance *ri, struct pt_regs *regs);
int rasprobe_handler_entry(remap_pfn_range)(struct rasprobe_instance *ri, struct pt_regs* regs);

//check the delayed memmory list
void check_my_list(void* args);
