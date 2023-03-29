#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
/* add for CMA malloc framebuffer */
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <linux/kmemleak.h>
#include <linux/hisi/hisi_ion.h>

#include "teek_client_constants.h"	/*lint !e451 */
#include "agent.h"
#include "mem.h"
#include "teek_ns_client.h"
#include "tui.h"
#include "smc.h"
#include "tc_ns_client.h"
#include "hisi_fb.h"
#include "libhwsecurec/securec.h"
#include "tc_ns_log.h"
#include "mailbox_mempool.h"
#include <linux/hisi/hisi_powerkey_event.h>
#include "mem.h"




static void tui_poweroff_work_func(struct work_struct *work);
static ssize_t tui_status_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf);
static void tui_msg_del(char *name);
static DECLARE_DELAYED_WORK(tui_poweroff_work, tui_poweroff_work_func);

static struct kobject *tui_kobj;
static struct kobj_attribute tui_attribute =
	__ATTR(c_state, 0440, tui_status_show, NULL);
static struct attribute *attrs[] = {
	&tui_attribute.attr,
	NULL,
};

static struct attribute_group tui_attr_group = {
	.attrs = attrs,
};

static struct task_struct *tui_task;
static struct tui_ctl_shm *tui_ctl;
static TC_NS_Shared_MEM *tui_tc_shm = NULL;

static spinlock_t tui_msg_lock;
static struct list_head tui_msg_head;

/*lint -e551 -esym(551,*) */
static atomic_t tui_state = ATOMIC_INIT(TUI_STATE_UNUSED);
/*lint -e551 +esym(551,*) */
static atomic_t tui_pid_state = ATOMIC_INIT(TUI_PID_CLEAR);

DEFINE_MUTEX(tui_drv_lock);
static struct list_head tui_drv_head = LIST_HEAD_INIT(tui_drv_head);

static unsigned int tui_attached_device;
static unsigned int tui_pid;

static wait_queue_head_t tui_state_wq;
static int tui_state_flag;
static int normal_load_flag;

static wait_queue_head_t tui_msg_wq;
static int tui_msg_flag;
static struct hisi_fb_data_type *dss_fd;

#define TUI_DSS_NAME "DSS"
#define TUI_GPIO_NAME "fff0d000.gpio"
#define TUI_TP_NAME "tp"
#define TUI_FP_NAME "fp"

#define TTF_NORMAL_BUFF_SIZE		(4 * 1024 * 1024)
#define TTF_NORMAL_FILE_PATH	"/vendor/etc/DroidSansFallbackTui.ttf"
#define TTF_UNUSUAL_BUFF_SIZE	(18 * 1024 * 1024)
#define TTF_UNUSUAL_FILE_PATH	"/system/fonts/NotoSansCJK-Regular.ttc"
#define DRIVER_NUM 4

/* 2M memory size is 2^21 */
#define ALIGN_SIZE  21
#define ALIGN_M  (1<<21)

/*do fp init(disable fp irq) before gpio init in order not response
* sensor in normal world(when gpio secure status is set)*/
static char *init_driver[DRIVER_NUM] = {TUI_DSS_NAME, TUI_TP_NAME,
					TUI_FP_NAME, TUI_GPIO_NAME
				       };
static char *deinit_driver[DRIVER_NUM] = {TUI_DSS_NAME, TUI_GPIO_NAME,
					  TUI_FP_NAME, TUI_TP_NAME
					 };
#define TIME_OUT_FOWER_ON 100

#define DOWN_VAL 22  //4M
#define UP_VAL       27  //64M
#define COLOR_TYPE	4  /*ARGB*/
#define BUFFER_NUM 2

/*tui-need-memory is calculated dynamically according to the screen resolution*/
struct tui_mem {
	unsigned int tui_addr_size;
	unsigned int tui_addr;
	unsigned int tui_addr_h;
	struct device *tui_dev;
	char *tui_virt;
};

struct ttf_mem {
	unsigned int ttf_addr_h;
	unsigned int ttf_addr_l;
	char *ttf_buff_virt;
	unsigned int ttf_file_size;
};

static struct tui_mem g_tui_mem;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
extern void create_mapping_late(phys_addr_t phys, unsigned long virt,
		phys_addr_t size, pgprot_t prot);
#endif

typedef struct tui_memory {
	struct ion_handle *tui_ion_handle;
	ion_phys_addr_t tui_ion_phys_addr;
	void *tui_ion_virt_addr;
	size_t len;
} tui_ion_mem;



static tui_ion_mem normal_font_mem;
static tui_ion_mem unusual_font_mem;

unsigned int get_frame_size(unsigned int num)
{
	if (num%ALIGN_M)
		return (((num >> ALIGN_SIZE) + 1) << ALIGN_SIZE);
	else
		return num;
}

unsigned int get_tui_size(unsigned int num)
{
	unsigned int i;
	for (i = DOWN_VAL; i < UP_VAL; i++)
		if (!(num >> i))
			break;
	return (unsigned int)1 << i; /*lint !e701 */
}

static size_t get_tui_font_file_size(ttf_type type)
{
	int ret;
	struct kstat ttf_file_stat;
	mm_segment_t old_fs;

	old_fs = get_fs(); /*lint !e501 */
	set_fs(KERNEL_DS); /*lint !e501 */
	/*get ttf file size*/
	if (normal == type) {
		ret = vfs_stat(TTF_NORMAL_FILE_PATH, &ttf_file_stat);
	} else {
		ret = vfs_stat(TTF_UNUSUAL_FILE_PATH, &ttf_file_stat);
	}
	if (ret < 0) {
		tloge("Failed to get ttf extend file size, ret is %d\n", ret);
		set_fs(old_fs);
		return 0;
	}

	set_fs(old_fs);

	return ttf_file_stat.size;

}

static int tui_mem_alloc(struct device *class_dev)
{
	u64 len;
	int ret;
	struct device_node *node;
	const char *status = NULL;
	char *buff_virt;
	phys_addr_t buff_phy = 0;

	if (NULL == class_dev)
		return -1;
	of_dma_configure(class_dev,NULL);
	ret = dma_set_mask_and_coherent(class_dev, DMA_BIT_MASK(64));
	if(ret < 0){
		TCERR("dma set mask and coherent failed\n");
		return ret;
	}
	ret = of_reserved_mem_device_init(class_dev);
	if (ret < 0) {
		TCERR("reserve tui mem failed\n");
		return ret;
	}

	node = of_find_node_by_path("/reserved-memory/tui-mem");
	if (!node) {
		tloge("not find tui mem\n");
		return -1;
	}

	ret = of_property_read_string(node, "status", &status);
	if (ret) {
		tloge("get status error\n");
		return -1;
	}

	if (status && strncmp(status, "ok", strlen("ok")) != 0) {
		tloge("status is error\n");
		return -1;
	}

	if (of_property_read_u64(node, "size", &len)) {
		tloge("get tui-mem size error");
		return -1;
	}

	buff_virt = (void *)dma_alloc_coherent(class_dev, len,
					       &buff_phy, GFP_KERNEL);
	if (NULL == buff_virt) {
		tloge("tui dma_alloc_coherent failed\n");
		return -1;
	}

	__dma_map_area(phys_to_virt(buff_phy), len, DMA_BIDIRECTIONAL);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range((phys_addr_t)buff_phy,
			(unsigned long)phys_to_virt(buff_phy), (phys_addr_t)len,
			__pgprot(PROT_DEVICE_nGnRE));
#else
	create_mapping_late((phys_addr_t)buff_phy,
			(unsigned long)phys_to_virt(buff_phy), (phys_addr_t)len,
			__pgprot(PROT_DEVICE_nGnRE));
#endif

	kmemleak_ignore(phys_to_virt(buff_phy));

	flush_tlb_all();

	g_tui_mem.tui_addr = buff_phy;
	g_tui_mem.tui_addr_h = buff_phy >> 32;
	g_tui_mem.tui_virt = buff_virt;
	g_tui_mem.tui_addr_size = len;
	g_tui_mem.tui_dev = class_dev;

	return 0;
}

static void tui_mem_free(void)
{
	phys_addr_t buff_phy_addr;
	if (!g_tui_mem.tui_addr)
		return;
	buff_phy_addr = (phys_addr_t)g_tui_mem.tui_addr_h << 32 | g_tui_mem.tui_addr; /*lint !e63 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(buff_phy_addr,
			    (unsigned long)phys_to_virt(buff_phy_addr), /*lint !e571 */
			    (phys_addr_t)g_tui_mem.tui_addr_size,
			    PAGE_KERNEL);
#else
	create_mapping_late(buff_phy_addr,
			    (unsigned long)phys_to_virt(buff_phy_addr), /*lint !e571 */
			    (phys_addr_t)g_tui_mem.tui_addr_size,
			    __pgprot(PAGE_KERNEL));
#endif
	dma_free_coherent(g_tui_mem.tui_dev,
			  g_tui_mem.tui_addr_size, g_tui_mem.tui_virt, buff_phy_addr);
}

static unsigned int get_frame_addr(void)
{
	u64 num = 1;
	int screen_r;

	if (!dss_fd)
		return 0;

	screen_r = dss_fd->panel_info.xres *
		   dss_fd->panel_info.yres * COLOR_TYPE * BUFFER_NUM;
	if (g_tui_mem.tui_addr_size < (get_frame_size(screen_r))) {
		tloge("tui memory is not enough size=0x%x screen=(%d*%d)\n",
		      g_tui_mem.tui_addr_size, dss_fd->panel_info.xres, dss_fd->panel_info.yres);
		return 0;
	} else {
		tlogd("tui memory addr=0x%x 0x%x size=0x%x screen=(%d*%d)\n",  g_tui_mem.tui_addr_h, g_tui_mem.tui_addr,
		      g_tui_mem.tui_addr_size, dss_fd->panel_info.xres, dss_fd->panel_info.yres);
		return g_tui_mem.tui_addr;
	}
}

void free_frame_addr(void)
{
}
/*if we enable 960 platform, we should revserve TTF_NORMAL_BUFF_SIZE + (dss_fd->panel_info.xres *
 *              dss_fd->panel_info.yres*COLOR_TYPE*BUFFER_NUM), the ttf_buff_offset should be 4M-algin*/
static int get_tui_font_mem(tui_ion_mem *tui_font_mem)
{
	unsigned int ttf_buff_offset;
	tui_font_mem->len = TTF_NORMAL_BUFF_SIZE;

	ttf_buff_offset= get_tui_size(dss_fd->panel_info.xres *
		dss_fd->panel_info.yres*COLOR_TYPE*BUFFER_NUM);

	if (g_tui_mem.tui_addr_size < TTF_NORMAL_BUFF_SIZE + ttf_buff_offset) {
		tloge("tui mem is less than fonts+framebuffer memory 0x%x\n",g_tui_mem.tui_addr_size);
		return -1;
	}

	tui_font_mem->tui_ion_phys_addr = ((phys_addr_t)g_tui_mem.tui_addr_h << 32) | (g_tui_mem.tui_addr + ttf_buff_offset);
	tui_font_mem->tui_ion_virt_addr = g_tui_mem.tui_virt + ttf_buff_offset;

	return 0;
}

static void free_tui_font_mem(ttf_type type)
{
	return;
}


int TC_NS_register_tui_font_mem(tui_ion_mem *tui_font_mem, size_t font_file_size, ttf_type type)
{
	TC_NS_SMC_CMD smc_cmd = {0};
	int ret;
	struct mb_cmd_pack *mb_pack;

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		return -ENOMEM;
	}

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = (unsigned int)virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	if (normal == type) {
		smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_TTF_MEM;
	} else {
		smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_UNUSUAL_TTF_MEM;
	}

	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_VALUE_INPUT | TEE_PARAM_TYPE_VALUE_INPUT << 4;
	mb_pack->operation.params[0].value.a = (uint32_t)(tui_font_mem->tui_ion_phys_addr  & 0xFFFFFFFF);
	mb_pack->operation.params[0].value.b = (uint32_t)(tui_font_mem->tui_ion_phys_addr >> 32);
	mb_pack->operation.params[1].value.a = font_file_size;
	smc_cmd.operation_phys = (unsigned int)virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/

	ret = (int)TC_NS_SMC(&smc_cmd, 0);
	if (ret) {
	    tloge("Send ttf mem info failed. 0x%x\n",ret);
	}
	mailbox_free(mb_pack);

	return ret;
}

static int copy_tui_font_file(size_t font_file_size, void *font_virt_addr, ttf_type type)
{
	struct file *filep;
	mm_segment_t old_fs;
	loff_t pos = 0;
	unsigned int count;
	int ret = 0;
	tloge("tui copy_ttf_file---\n");

	if (normal == type) {
		filep = filp_open(TTF_NORMAL_FILE_PATH, O_RDONLY, 0);
	} else {
		filep = filp_open(TTF_UNUSUAL_FILE_PATH, O_RDONLY, 0);
	}

	if(IS_ERR(filep) || !filep) {
		tloge("Failed to open ttf file.\n");
		return -1;
	}

	old_fs = get_fs(); /*lint !e501 */
	set_fs(KERNEL_DS); /*lint !e501 */

	count = (unsigned int)vfs_read(filep, (char __user *)font_virt_addr, font_file_size, &pos);

	if (font_file_size != count) {
		tloge("read ttf file failed.\n");
		ret = -1;
	}

	set_fs(old_fs);
	filp_close(filep, 0);

	return ret;
}

int load_tui_font_file(ttf_type type, unsigned int arg)
{
	int ret = 0;
	size_t tui_font_file_size = 0;
	tui_ion_mem *tui_ttf_mem;

	if (normal == type) {
		if (normal_load_flag) {
			tloge("normal tui font file has been loaded.\n");
			return -1;
		}

		normal_font_mem.len = TTF_NORMAL_BUFF_SIZE;
		ret = get_tui_font_mem(&normal_font_mem);
		tui_ttf_mem = &normal_font_mem;
	} else {
		unusual_font_mem.len = TTF_UNUSUAL_BUFF_SIZE;
		ret = get_tui_font_mem(&unusual_font_mem);
		tui_ttf_mem = &unusual_font_mem;
	}

	if(ret) {
		tloge("Failed to get tui font memory.\n");
		return -1;
	}

	tui_font_file_size = get_tui_font_file_size(type);
	if (0 == tui_font_file_size || tui_font_file_size > tui_ttf_mem->len) {
		free_tui_font_mem(type);
		tloge("Failed to get the tui font file size or the tui_font_file_size is too big.\n");
		return -1;
	}

	ret = copy_tui_font_file(tui_font_file_size, tui_ttf_mem->tui_ion_virt_addr, type);
	if (ret < 0) {
		free_tui_font_mem(type);
		tloge("Failed to do ttf file copy\n");
		return -1;
	}

	__dma_map_area(tui_ttf_mem->tui_ion_virt_addr, tui_ttf_mem->len, DMA_BIDIRECTIONAL);
	flush_tlb_all();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(tui_ttf_mem->tui_ion_phys_addr,
			(unsigned long)phys_to_virt(tui_ttf_mem->tui_ion_phys_addr), (phys_addr_t)tui_ttf_mem->len,
			__pgprot(PROT_DEVICE_nGnRE));
#else
	create_mapping_late(tui_ttf_mem->tui_ion_phys_addr,
			(unsigned long)phys_to_virt(tui_ttf_mem->tui_ion_phys_addr), (phys_addr_t)tui_ttf_mem->len,
			__pgprot(PROT_DEVICE_nGnRE));
#endif

	ret = TC_NS_register_tui_font_mem(tui_ttf_mem, tui_font_file_size, type);
	if (ret != 0) {
		free_tui_font_mem(type);
		tloge("Failed to do ttf file register ret is 0x%x\n", ret);
		return -1;
	}

	if (normal == type) {
		normal_load_flag = 1;
	}

	return ret;
}

int register_tui_driver(tui_drv_init fun, const char *name,
			void *pdata)
{
	struct tui_drv_node *tui_drv, *pos;
	int rc;

	/* Return error if name is invalid */
	if (!name || !fun) {
		TCERR("name or func is null");
		return -EINVAL;
	}

	if (!strncmp(name, TUI_DSS_NAME, (size_t)TUI_DRV_NAME_MAX)) {
		if (!pdata)
			return -1;
		else
			dss_fd = (struct hisi_fb_data_type *)pdata;
	}

	if (!strncmp(name, TUI_TP_NAME, (size_t)TUI_DRV_NAME_MAX) && !pdata)
		return -1;

	mutex_lock(&tui_drv_lock);

	/* name should not have been registered */
	list_for_each_entry(pos, &tui_drv_head, list) {
		if (!strncmp(pos->name, name, TUI_DRV_NAME_MAX - 1)) {
			tloge("this drv(%s) have registered\n", name);
			mutex_unlock(&tui_drv_lock);
			return -EINVAL;
		}
	}
	mutex_unlock(&tui_drv_lock);

	/* Allocate memory for tui_drv */
	tui_drv = kzalloc(sizeof(struct tui_drv_node), GFP_KERNEL);
	if (!tui_drv)
		return -ENOMEM;

	/* Assign content for tui_drv */
	tui_drv->init_func = fun;
	tui_drv->pdata = pdata;

	rc = strncpy_s(tui_drv->name, TUI_DRV_NAME_MAX - 1, name, TUI_DRV_NAME_MAX - 1);
	if (rc != 0) {
		tloge("strncpy_s error\n");
		kfree(tui_drv);
		return -1;
	}

	INIT_LIST_HEAD(&tui_drv->list);

	/* Link the new tui_drv to the list */
	mutex_lock(&tui_drv_lock);
	list_add_tail(&tui_drv->list, &tui_drv_head);
	mutex_unlock(&tui_drv_lock);

	return 0; /*lint !e429 */
}
EXPORT_SYMBOL(register_tui_driver);

void unregister_tui_driver(const char *name)
{
	struct tui_drv_node *pos=NULL, *tmp;

	/* Return error if name is invalid */
	if (!name) {
		TCERR("name is null");
		return;
	}

	mutex_lock(&tui_drv_lock);
	list_for_each_entry_safe(pos, tmp, &tui_drv_head, list) { /*lint !e64 !e826 !e530*/
		if (!strncmp(pos->name, name, TUI_DRV_NAME_MAX)) {/*lint !e413 */
			list_del(&pos->list); /*lint !e413 */
			kfree(pos);
			break;
		}
	}
	mutex_unlock(&tui_drv_lock);
}
EXPORT_SYMBOL(unregister_tui_driver);

static int add_tui_msg(int type, int val, void *data)
{
	struct tui_msg_node *tui_msg = NULL;
	unsigned long flags;

	/* Return error if pdata is invalid */
	if (!data) {
		TCERR("data is null");
		return -EINVAL;
	}

	/* Allocate memory for tui_msg */
	tui_msg = kzalloc(sizeof(*tui_msg), GFP_KERNEL);
	if (!tui_msg)
		return -ENOMEM;

	/* Assign the content of tui_msg */
	tui_msg->type = type;
	tui_msg->val = val;
	tui_msg->data = data;
	INIT_LIST_HEAD(&tui_msg->list);

	/* Link the new tui_msg to the list */
	spin_lock_irqsave(&tui_msg_lock, flags);
	list_add_tail(&tui_msg->list, &tui_msg_head);
	tui_msg_flag = 1;
	spin_unlock_irqrestore(&tui_msg_lock, flags);

	return 0; /*lint !e429 */
}
/*WARNING: Too many leading tabs - consider code refactoring*/
/* secure : 0-unsecure, 1-secure */
static int init_tui_driver(int secure)
{
	struct tui_drv_node *pos;
	char *drv_name = NULL;
	char **drv_array = deinit_driver;
	int count = 0;
	int i = 0;
	if (!dss_fd)
		return -1;
	if (secure)
		drv_array = init_driver;
	while (i < DRIVER_NUM) {
		drv_name = drv_array[i];
		i++;
		mutex_lock(&tui_drv_lock);

		/* Search all the tui_drv in their list */
		list_for_each_entry(pos, &tui_drv_head, list) {
			if (!strncmp(drv_name, pos->name, TUI_DRV_NAME_MAX)) {
				/* The names match. */

				// cppcheck-suppress *
				if (!strncmp(TUI_TP_NAME, pos->name, TUI_DRV_NAME_MAX)) {
					/* If the name is "tp", assign pos->pdata to tui_ctl */
					tui_ctl->n2s.tp_info =
						virt_to_phys(pos->pdata);
					tui_ctl->n2s.tp_info_h_addr =
						virt_to_phys(pos->pdata) >> 32;
				}
				if (pos->init_func) {
					// cppcheck-suppress *
					if (!strncmp(TUI_DSS_NAME, pos->name,
						     TUI_DRV_NAME_MAX) && secure) {
						tloge("init_tui_driver wait power on status---\n");
						while (!dss_fd->panel_power_on &&
						       count < TIME_OUT_FOWER_ON) {
							count++;
							msleep(20);
						}
						if (TIME_OUT_FOWER_ON == count) {
							/* Time out. So return error. */
							mutex_unlock(&tui_drv_lock);
							tloge("wait status time out\n");
							return -1;
						}
						spin_lock(&tui_msg_lock);
						tui_msg_del(TUI_DSS_NAME);
						spin_unlock(&tui_msg_lock);
					}
					if (!secure) {
						tlogd("drv(%s) state=%d,%d\n",
							pos->name, secure, pos->state);
						if (0 == pos->state)
							continue;
						if (pos->init_func(pos->pdata, secure)) {
							/* Process init_func() fail */
							pos->state = -1;
							//return -1;
						}
						/* set secure state will be proceed in tui msg */
						pos->state = 0;
					} else {
						tlogd("init tui drv(%s) state=%d\n",
							pos->name, secure);
						/*when init, tp and dss should be async*/
						if (pos->init_func(pos->pdata, secure)) {
							pos->state = -1;
							mutex_unlock(&tui_drv_lock);
							return -1;
						} else {
							// cppcheck-suppress *
							if (strncmp(TUI_DSS_NAME, pos->name, TUI_DRV_NAME_MAX))
								pos->state = 1;
						}
					}
				}
			}
		}
		mutex_unlock(&tui_drv_lock);
	}

	return 0;
}

/* Only after all drivers cfg ok or some one failed, it need
 * to add_tui_msg.
 * ret val:	 1 - all cfg ok
 *			 0 - cfg is not complete, or have done
 *			-1 - cfg failed
 *			-2 - invalid name
 */
static int tui_cfg_filter(const char *name, bool ok)
{
	struct tui_drv_node *pos;
	char find = 0;
	int lock_flag = 0;

	/* Return error if name is invalid */
	if (!name) {
		TCERR("name is null");
		return -2;
	}

	/* some drivers may call send_tui_msg_config at the end
	 * of drv_init_func which had got the lock.
	 */
	if (mutex_is_locked(&tui_drv_lock))
		lock_flag = 1;
	if (!lock_flag)
		mutex_lock(&tui_drv_lock);
	list_for_each_entry(pos, &tui_drv_head, list) {
		if (!strncmp(pos->name, name, TUI_DRV_NAME_MAX)) {
			find = 1;
			if (ok)
				pos->state = 1;
			else {
				if (!lock_flag)
					mutex_unlock(&tui_drv_lock);
				return -1; /*lint !e454 !e456 */
			}
		}
	}
	if (!lock_flag)
		mutex_unlock(&tui_drv_lock);

	if (!find) /*lint !e456 */
		return -2; /*lint !e454 */

	return 1; /*lint !e454 */
}

enum poll_class {
	CLASS_POLL_CONFIG,
	CLASS_POLL_RUNNING,
	CLASS_POLL_COMMON,
	CLASS_POLL_INVALID
};

static inline enum poll_class tui_poll_class(int event_type)
{
	enum poll_class class = CLASS_POLL_INVALID;

	switch (event_type) {
	case TUI_POLL_CFG_OK:
	case TUI_POLL_CFG_FAIL:
	case TUI_POLL_RESUME_TUI:
		class = CLASS_POLL_CONFIG;
		break;
	case TUI_POLL_TP:
	case TUI_POLL_TICK:
	case TUI_POLL_DELAYED_WORK:
	case TUI_POLL_PAUSE_TUI:
		class = CLASS_POLL_RUNNING;
		break;
	case TUI_POLL_CANCEL:
		class = CLASS_POLL_COMMON;
		break;
	default:
		break;
	}
	return class;
}

int send_tui_msg_config(int type, int val, void *data)
{/*lint !e31 */
	int ret;

	if (type >= TUI_POLL_MAX  || type < 0 || !data) {
		tloge("invalid tui event type\n");
		return -EINVAL;
	}

	/* The tui_state should be CONFIG */
	if (atomic_read(&tui_state) != TUI_STATE_CONFIG) {
		tloge("failed to send tui msg(%s)\n",
		      poll_event_type_name[type]);
		return -EINVAL;
	}

	if (CLASS_POLL_RUNNING == tui_poll_class(type)) {
		tloge("invalid tui event type(%s) in config state\n",
		      poll_event_type_name[type]);
		return -EINVAL;
	}

	tlogd("send config event type %s(%s)\n",
	      poll_event_type_name[type], (char *)data);

	if (TUI_POLL_CFG_OK == type || TUI_POLL_CFG_FAIL == type) {
		int cfg_ret;

		cfg_ret = tui_cfg_filter((const char *)data,
					 TUI_POLL_CFG_OK == type);
		tlogd("tui driver(%s) cfg ret = %d\n", (char *)data, cfg_ret);
		if (-2 == cfg_ret)
			return -EINVAL;
	}

	ret = add_tui_msg(type, val, data);
	if (ret) {
		tloge("add tui msg ret=%d\n", ret);
		return ret;
	}

	tlogd("add config msg type %s\n", poll_event_type_name[type]);

	/* wake up tui kthread */
	wake_up(&tui_msg_wq);

	return 0;
}


/* Send tui event by smc_cmd */
int tui_send_event(int event, unsigned int value)
{
	if (!dss_fd)
		return -1;
	if ((atomic_read(&tui_state) != TUI_STATE_UNUSED
	    && dss_fd->panel_power_on) || TUI_POLL_NOTCH == event) {
		TC_NS_SMC_CMD smc_cmd = { 0 };
		uint32_t uid;
		struct mb_cmd_pack *mb_pack;
		int ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		kuid_t kuid;

		kuid = current_uid();	/*lint !e666 !e667 !e64 */
		uid = kuid.val;
#else
		uid = current_uid();
#endif
		if (uid > 1000 || ((TUI_POLL_CANCEL != event) && (TUI_POLL_NOTCH != event))) {
			tloge("no permission to send msg\n");
			return -1;
		}

		mb_pack = mailbox_alloc_cmd_pack();
		if (!mb_pack) {
			tloge("alloc cmd pack failed\n");
			return -1;
		}

		mb_pack->uuid[0] = 1;
		smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
		smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
		switch(event){
			case TUI_POLL_CANCEL:
				smc_cmd.cmd_id = GLOBAL_CMD_ID_TUI_EXCEPTION;
				break;
			case TUI_POLL_NOTCH:
				smc_cmd.cmd_id = GLOBAL_CMD_ID_TUI_NOTCH;
				break;
			default:
				tloge("invalid event type : %d.\n",event);
				break;
		}
		smc_cmd.agent_id = event;
		smc_cmd.uid = uid;
		smc_cmd.ret_val = value;

		ret = TC_NS_SMC(&smc_cmd, 0);
		mailbox_free(mb_pack);

		return ret;
	} else {
		tlogd("tui unused no need send tui event!\n");
		return 0;
	}
}

static void tui_poweroff_work_func(struct work_struct *work)
{
	tui_send_event(TUI_POLL_CANCEL,0);
}

void tui_poweroff_work_start(void)
{
	tlogd("tui_poweroff_work_start----------\n");
	if (!dss_fd)
		return;
	if (atomic_read(&tui_state) != TUI_STATE_UNUSED
	    && dss_fd->panel_power_on) {
		tlogd("come in tui_poweroff_work_start state=%d--\n",
		      atomic_read(&tui_state));
		queue_work(system_wq, &tui_poweroff_work.work);
	}
}

static void wait_tui_msg(void)
{
	if (wait_event_interruptible(tui_msg_wq, tui_msg_flag))
		tloge("get tui state is interrupted\n");
}

static int valid_msg(int msg_type)
{
	switch (msg_type) {
	case TUI_POLL_RESUME_TUI:
		if (TUI_STATE_RUNNING == atomic_read(&tui_state))
			return 0;
		break;
	case TUI_POLL_CANCEL:
		if (TUI_STATE_UNUSED == atomic_read(&tui_state))
			return 0;
		break;
	default:
		break;
	}

	return 1;
}
/*
 * 1: init ok
 * 0: still do init
 * -1: init failed
 */
static int get_cfg_state(char *name)
{
	struct tui_msg_node *tui_msg;

	/* Return error if name is invalid */
	if (!name) {
		TCERR("name is null");
		return -1;
	}

	list_for_each_entry(tui_msg, &tui_msg_head, list) {
		/* Names match */
		if (!strncmp(tui_msg->data, name, TUI_DRV_NAME_MAX)) {
			if (TUI_POLL_CFG_OK == tui_msg->type)
				return 1;
			else if (TUI_POLL_CFG_FAIL == tui_msg->type)
				return -1;
			else
				TCERR("other state\n");
		}
	}

	return 0;
}
static void tui_msg_del(char *name)
{
	struct tui_msg_node *tui_msg=NULL, *tmp;

	/* Return error if name is invalid */
	if (!name) {
		TCERR("name is null");
		return;
	}

	list_for_each_entry_safe(tui_msg, tmp, &tui_msg_head, list) { /*lint !e64 !e826 !e530 !e516 */
		/* Names match */
		if (!strncmp(tui_msg->data, name, TUI_DRV_NAME_MAX)) { /*lint !e413 */
			list_del(&tui_msg->list); /*lint !e413 */
			kfree(tui_msg);
		}
	}
}
#define DSS_CONFIG_INDEX (1)
#define TP_CONFIG_INDEX (2)
static void process_tui_msg(void)
{
	int val = 0;
	int type = TUI_POLL_CFG_OK;

fetch_msg:
	spin_lock(&tui_msg_lock);
	if (DSS_CONFIG_INDEX == tui_ctl->s2n.value) {
		/* Wait, until DSS init finishs */
		while (0 == get_cfg_state(TUI_DSS_NAME)) {
			tlogd("waiting for dss tui msg\n");
			tui_msg_flag = 0;
			spin_unlock(&tui_msg_lock);
			wait_tui_msg();
			tlogd("get dss init ok tui msg\n");
			spin_lock(&tui_msg_lock);
		}
		if (-1 == get_cfg_state(TUI_DSS_NAME)) {
			tloge("dss init failed\n");
			type = TUI_POLL_CFG_FAIL;
		}
		/* Delete DSS msg from tui_msg_head */
		tui_msg_del(TUI_DSS_NAME);
	} else if (TP_CONFIG_INDEX == tui_ctl->s2n.value) {
		while (0 == get_cfg_state(TUI_TP_NAME)) {
			tlogd("waiting for tp tui msg\n");
			tui_msg_flag = 0;
			spin_unlock(&tui_msg_lock);
			wait_tui_msg();
			tlogd("get tp init ok tui msg\n");
			spin_lock(&tui_msg_lock);
		}
		if (-1 == get_cfg_state(TUI_TP_NAME)) {
			tloge("tp failed to do init\n");
			type = TUI_POLL_CFG_FAIL;
			tui_msg_del(TUI_TP_NAME);
			goto next;
		}
		tui_msg_del(TUI_TP_NAME);
		tlogd("tp/gpio/fp is config result:type = 0x%x\n", type);
	} else {
		tloge("wait others dev\n");
	}
next:
	spin_unlock(&tui_msg_lock);

	/* pre-process tui poll event if needed */
	switch (type) {
	case TUI_POLL_CFG_OK:
		if (DSS_CONFIG_INDEX == tui_ctl->s2n.value) {

			tui_ctl->n2s.addr = get_frame_addr();
			tui_ctl->n2s.addr_h = g_tui_mem.tui_addr_h;
			tloge("old tui reserve memory h:l=0x%x 0x%x\n",tui_ctl->n2s.addr_h, tui_ctl->n2s.addr);
			if (0 == tui_ctl->n2s.addr)
				val = -1;
		}
		break;
	default:
		break;
	}

	tui_ctl->n2s.event_type = type;
	tui_ctl->n2s.value = val;

	if (!valid_msg(tui_ctl->n2s.event_type)) {
		tlogd("refetch tui msg\n");
		goto fetch_msg;
	}
}

static int init_tui_agent(void)
{
	int ret;

	tui_tc_shm = tc_mem_allocate((size_t)SZ_4K, true);
	if (IS_ERR(tui_tc_shm)) {
		tloge("failed to allocate 4k bytes for tui_ctl\n");
		tui_tc_shm = NULL;
		return -ENOMEM;
	}

	tui_ctl = tui_tc_shm->kernel_addr;
	ret = TC_NS_register_agent(NULL, TEE_TUI_AGENT_ID, tui_tc_shm);
	if (ret) {
		tloge("register tui agent failed\n");
		tc_mem_free(tui_tc_shm);
		tui_tc_shm = NULL;
		tui_ctl = NULL;
		return -EFAULT;
	}

	atomic_set(&tui_tc_shm->usage, 1);

	return 0;
}

static void exit_tui_agent(void)
{
	if (TC_NS_unregister_agent(TEE_TUI_AGENT_ID))
		tloge("unregister tui agent failed\n");

	put_sharemem_struct(tui_tc_shm); /* paired with init_tui_agent() */
	tui_tc_shm = NULL;
	tui_ctl = NULL;
}

static void set_tui_state(int state)
{
	if (state < TUI_STATE_UNUSED || state > TUI_STATE_ERROR) {
		tloge("state=%d is invalid\n",state);
		return;
	}
	if (atomic_read(&tui_state) != state) {
		atomic_set(&tui_state, state); /*lint !e1058 */
		tui_state_flag = 1;
		wake_up(&tui_state_wq);
	}
}

static void set_tui_pid(int state)
{
	if (state < TUI_PID_CLEAR || state > TUI_PID_CONFIG) {
		tloge("state=%d is invalid\n", state);
		return;
	}
	if (state == TUI_PID_CLEAR)
		atomic_set(&tui_pid_state, 0);
	if (state == TUI_PID_CONFIG){
		atomic_set(&tui_pid_state, tui_pid);
	}
}
int tui_pid_status(int pid_value)
{
	if (pid_value == atomic_read(&tui_pid_state))
		return 1;
	return 0;
}
static int agent_process_work_tui(void)
{
	struct __smc_event_data *event_data;

	/* TODO: needs lock */
	event_data = find_event_control(TEE_TUI_AGENT_ID);
	if ( NULL == event_data || 0 == event_data->agent_alive) {
		/*TODO: if return, the pending task in S can't be resumed!! */
		tloge("tui agent is not exist\n");
		put_agent_event(event_data);
		return TEEC_ERROR_GENERIC;/*lint !e570*/
	}

	isb();
	wmb();
	event_data->ret_flag = 1;
	/* Wake up tui agent that will process the command */
	wake_up(&event_data->wait_event_wq);

	tlogi("agent 0x%x request, goto sleep, pe->run=%d\n",
		TEE_TUI_AGENT_ID, atomic_read(&event_data->ca_run));
	wait_event(event_data->ca_pending_wq, atomic_read(&event_data->ca_run));
	atomic_set(&event_data->ca_run, 0);
	put_agent_event(event_data);

	return TEEC_SUCCESS;
}

void do_ns_tui_release(void)
{ /*lint !e31 !e831 */
	if (atomic_read(&tui_state) != TUI_STATE_UNUSED) { /*lint !e529 !e438 */
	tui_ctl->s2n.command = TUI_CMD_FREE_UNUSUAL_TTF_MEM_AND_DISABLE;
	tui_ctl->s2n.ret = -1;
	tloge("exec tui do_ns_tui_release\n");
	if(agent_process_work_tui())
		tloge("wake up tui agent error\n");
	}
}
static int do_tui_unusual_ttf_work(void)
{
	int ret = 0;
	unsigned int arg = 0;
	switch (tui_ctl->s2n.command) {
	case TUI_CMD_FREE_UNUSUAL_TTF_MEM_AND_DISABLE:
		if (atomic_read(&tui_state) != TUI_STATE_UNUSED) {
			tlogd("tui disable\n");
			init_tui_driver(0);
			free_frame_addr();
			free_tui_font_mem(unusual);
			set_tui_state(TUI_STATE_UNUSED);
		}
		break;
	case TUI_CMD_LOAD_UNUSUAL_TTF:
		ret = load_tui_font_file(unusual, arg);
		if (!ret) {
			tlogd("=======suceed to load ttf\n");
			tui_ctl->n2s.event_type = TUI_POLL_CFG_OK;
		} else {
			tloge("Failed to load ttf ret is 0x%x\n", ret);
			tui_ctl->n2s.event_type = TUI_POLL_CFG_FAIL;
		}
		break;
	case TUI_CMD_FREE_UNUSUAL_TTF_MEM:
		free_tui_font_mem(unusual);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		tloge("get error unusual ttf tui command(0x%x)\n", tui_ctl->s2n.command);
		break;
	}
	return ret;
}
static int do_tui_config_work(void)
{
	int ret = 0;

	switch (tui_ctl->s2n.command) {
	case TUI_CMD_ENABLE:
		if (atomic_read(&tui_state) != TUI_STATE_CONFIG) {
			tlogd("tui enable\n");
			set_tui_state(TUI_STATE_CONFIG);
			/*do dss and tp init*/
			if (init_tui_driver(1)) {
				tui_ctl->s2n.ret = -1;
				set_tui_state(TUI_STATE_ERROR);
			}
		}
		set_tui_pid(TUI_PID_CONFIG);
		break;
	case TUI_CMD_DISABLE:
		if (atomic_read(&tui_state) != TUI_STATE_UNUSED) {
			tlogd("tui disable\n");
			init_tui_driver(0);
			free_frame_addr();
			set_tui_state(TUI_STATE_UNUSED);
		}
		set_tui_pid(TUI_PID_CLEAR);
		break;

	case TUI_CMD_PAUSE:
		if (atomic_read(&tui_state) != TUI_STATE_UNUSED) {
			tlogd("tui pause\n");
			init_tui_driver(0);
			set_tui_state(TUI_STATE_CONFIG);
		}
		break;
	case TUI_CMD_POLL:
		process_tui_msg();
		break;
	case TUI_CMD_DO_SYNC:
		/*if (tp_sync_func) {*/
		tlogd("enable tp irq cmd\n");
		/*		tp_sync_func();*/
		/*		tui_timeout_reset();*/
		/*}*/
		break;
	case TUI_CMD_SET_STATE:
		tlogd("tui set state %d\n", tui_ctl->s2n.value);
		set_tui_state(tui_ctl->s2n.value);
		break;
	case TUI_CMD_START_DELAY_WORK:
		tlogd("start delay work\n");
		break;
	case TUI_CMD_CANCEL_DELAY_WORK:
		tlogd("cancel delay work\n");
		break;
	case TUI_CMD_SEND_INPUT_WORK:
		tlogd("send input work\n");
		break;
	default:
		ret = -EINVAL;
		tloge("get error config tui command(0x%x)\n", tui_ctl->s2n.command);
		break;
	}
	return ret;
}
static int do_tui_work(void)
{
	int ret = 0;

	/* clear s2n cmd ret */
	tui_ctl->s2n.ret = 0;
	switch (tui_ctl->s2n.command) {
	case TUI_CMD_ENABLE:
	case TUI_CMD_DISABLE:
	case TUI_CMD_PAUSE:
	case TUI_CMD_POLL:
	case TUI_CMD_DO_SYNC:
	case TUI_CMD_SET_STATE:
	case TUI_CMD_START_DELAY_WORK:
	case TUI_CMD_CANCEL_DELAY_WORK:
	case TUI_CMD_SEND_INPUT_WORK:
		ret = do_tui_config_work();
		break;
	case TUI_CMD_LOAD_UNUSUAL_TTF:
	case TUI_CMD_FREE_UNUSUAL_TTF_MEM_AND_DISABLE:
	case TUI_CMD_FREE_UNUSUAL_TTF_MEM:
		ret = do_tui_unusual_ttf_work();
		break;
	default:
		ret = -EINVAL;
		tloge("get error tui command\n");
		break;
	}
	return ret;
}

void set_tui_caller_info(unsigned int devid, unsigned int pid)
{/*lint !e18 !e31 !e532 */
	tui_attached_device = devid;
	tui_pid = pid;
} /*lint !e533 */

unsigned int tui_attach_device(void)
{
	return tui_attached_device;
}

static int tui_kthread_work_fn(void *data)
{
	int ret;

	ret = init_tui_agent();
	if (ret)
		return ret;

	while (1) {
		/*tlogd("tui before sleep  1\n");*/
		TC_NS_wait_event(TEE_TUI_AGENT_ID);
		/*tlogd("tui after sleep	 2\n");*/

		if (kthread_should_stop())
			break;

		do_tui_work();

		if (TC_NS_send_event_response(TEE_TUI_AGENT_ID))
			tloge("send event response error\n");
	}

	exit_tui_agent();

	return 0;
}

#define READ_BUF 128
static ssize_t tui_dbg_state_read(struct file *filp, char __user *ubuf,
				  size_t cnt, loff_t *ppos)
{
	char buf[READ_BUF] = {0};
	unsigned int r;
	int ret;
	struct tui_drv_node *pos;

	if (!filp || !ubuf || !ppos)
		return -EINVAL;

	ret = snprintf_s(buf, READ_BUF, READ_BUF-1, "tui state:%s\n",
			 state_name[atomic_read(&tui_state)]);
	if (ret < 0)
		return -EINVAL;
	r = (unsigned int)ret;

	ret = snprintf_s(buf + r, READ_BUF - r, READ_BUF - r, "drv config state:");
	if (ret < 0)
		return -EINVAL;
	r += (unsigned int)ret;

	mutex_lock(&tui_drv_lock);
	list_for_each_entry(pos, &tui_drv_head, list) {
		ret = snprintf_s(buf + r, READ_BUF - r, READ_BUF - r, "%s-%s,", pos->name,
				1 == pos->state ? "ok" : "no ok");/* [false alarm]:buffer足够大不会越界  */
		if (ret < 0) {
			mutex_unlock(&tui_drv_lock);
			return -EINVAL;
		}
		r += (unsigned int)ret;
	}
	mutex_unlock(&tui_drv_lock);
	if (r < READ_BUF)
		buf[r-1]='\n';

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static const struct file_operations tui_dbg_state_fops = {
	.owner = THIS_MODULE,
	.read = tui_dbg_state_read,
};

static ssize_t tui_status_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	int r;

	if (!kobj || !attr || !buf)
		return -EINVAL;
	tui_state_flag = 0;
	r = wait_event_interruptible(tui_state_wq, tui_state_flag);
	if (r) {
		tloge("get tui state is interrupted\n");
		return r;
	}

	r = snprintf_s(buf, 32, 32, "%s", state_name[atomic_read(&tui_state)]);
	if (r < 0)
		return -1;

	return r;
}

#define MSG_BUF 512
static ssize_t tui_dbg_msg_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char buf[MSG_BUF] = {0};
	unsigned int r;
	int ret;
	int i;
	struct tui_drv_node *pos;

	if (!filp || !ubuf || !ppos)
		return -EINVAL;

	ret = snprintf_s(buf, MSG_BUF, MSG_BUF-1, "%s", "event format: event_type:val\n"
			 "event type:\n");
	if (ret < 0)
		return -EINVAL;
	r = (unsigned int)ret;

	/* event type list */
	for (i = 0; i < TUI_POLL_MAX - 1; i++) {
		ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r, "%s, ",
				 poll_event_type_name[i]);/* [false alarm]:buffer足够大不会越界 */
		if (ret < 0)
			return -EINVAL;
		r += (unsigned int)ret;
	}

	ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r, "%s\n", poll_event_type_name[i]);
	if (ret < 0)
		return -EINVAL;
	r += (unsigned int)ret;

	/* cfg drv type list */
	ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r, "val type for %s or %s:\n",
			poll_event_type_name[TUI_POLL_CFG_OK],
			poll_event_type_name[TUI_POLL_CFG_FAIL]);
	if (ret < 0)
		return -EINVAL;
	r += (unsigned int)ret;

	mutex_lock(&tui_drv_lock);
	list_for_each_entry(pos, &tui_drv_head, list) {
		ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r, "%s,", pos->name);
		if (ret < 0) {
			mutex_unlock(&tui_drv_lock);
			return -EINVAL;
		}
		r += (unsigned int)ret;
	}
	mutex_unlock(&tui_drv_lock);
	if (r < MSG_BUF)
		buf[r - 1] = '\n';

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t tui_dbg_msg_write(struct file *filp,
				 const char __user *ubuf, size_t cnt,
				 loff_t *ppos)
{
	char buf[64];
	int i;
	int event_type = -1;
	char *tokens, *begins;
	int ret;

	if (!ubuf || !filp || !ppos)
		return -EINVAL;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	begins = buf;

	/* event type */
	tokens = strsep(&begins, ":");
	if (!tokens)
		return -EFAULT;

	tlogd("1: tokens:%s\n", tokens);
	for (i = 0; i < TUI_POLL_MAX; i++) {
		if (!strncmp(tokens, poll_event_type_name[i],
			     strlen(poll_event_type_name[i]))) {
			event_type = i;
			break;
		}
	}

	/* only for tp and cancel  */
	if (event_type != TUI_POLL_TP  && event_type != TUI_POLL_CANCEL)
		return -EFAULT;
	/* drv type */
	tokens = strsep(&begins, ":");
	if (!tokens)
		return -EFAULT;
	tlogd("2: tokens:%s\n", tokens);
	if (TUI_POLL_TP == event_type) {
		long value = 0;
		int base = 10;

		/* simple_strtol is obsolete, use kstrtol instead*/
		ret = kstrtol(tokens, base, &value);
		if (ret)
			return -EFAULT;
		tui_ctl->n2s.status = value;

		tokens = strsep(&begins, ":");
		if (!tokens)
			return -EFAULT;
		ret = kstrtol(tokens, base, &value);
		if (ret)
			return -EFAULT;
		tui_ctl->n2s.x = value;

		tokens = strsep(&begins, ":");
		if (!tokens)
			return -EFAULT;
		ret = kstrtol(tokens, base, &value);
		if (ret)
			return -EFAULT;
		tui_ctl->n2s.y = value;
	}
	tlogd("status=%d x=%d y=%d\n",
	      tui_ctl->n2s.status, tui_ctl->n2s.x, tui_ctl->n2s.y);

	if (tui_send_event(event_type,0))
		return -EFAULT;

	*ppos += cnt;

	return cnt;
}

static const struct file_operations tui_dbg_msg_fops = {
	.owner = THIS_MODULE,
	.read = tui_dbg_msg_read,
	.write = tui_dbg_msg_write,
};

static struct dentry *dbg_dentry;


static int tui_powerkey_notifier_call(struct notifier_block *powerkey_nb, unsigned long event, void *data)
{
	if (event == HISI_PRESS_KEY_DOWN) {
		tui_poweroff_work_start();
	}else if (event == HISI_PRESS_KEY_UP) {
	}else if (event == HISI_PRESS_KEY_1S) {
	}else if (event == HISI_PRESS_KEY_6S) {
	} else if (event == HISI_PRESS_KEY_8S) {
	} else if (event == HISI_PRESS_KEY_10S) {
	} else {
		pr_err("[%s]invalid event %ld !\n", __func__, event);
	}
	return 0;
}
static struct notifier_block tui_powerkey_nb;
int register_tui_powerkeyListener(void)
{
	tui_powerkey_nb.notifier_call = tui_powerkey_notifier_call;
	return hisi_powerkey_register_notifier(&tui_powerkey_nb);
}
int unregister_tui_powerkeyListener(void)
{
	tui_powerkey_nb.notifier_call = tui_powerkey_notifier_call;
	return hisi_powerkey_unregister_notifier(&tui_powerkey_nb);
}



int __init init_tui(struct device *class_dev)
{
	int retval;
	struct sched_param param;
	param.sched_priority = MAX_RT_PRIO - 1;

	if (!class_dev)
		return -1;


	retval = tui_mem_alloc(class_dev);
	if (retval) {
		tloge("tui alloc fail\n");
		return -ENOMEM;
	}


	tui_task = kthread_create(tui_kthread_work_fn, NULL, "tuid");
	if (IS_ERR(tui_task)) { /*lint !e413 !e516 */
		tui_mem_free();
		return PTR_ERR(tui_task);
	}

	sched_setscheduler_nocheck(tui_task, SCHED_FIFO, &param);
	get_task_struct(tui_task);

	wake_up_process(tui_task);

	INIT_LIST_HEAD(&tui_msg_head);
	spin_lock_init(&tui_msg_lock);

	init_waitqueue_head(&tui_state_wq);
	init_waitqueue_head(&tui_msg_wq);
	dbg_dentry = debugfs_create_dir("tui", NULL);
	debugfs_create_file("d_state", 0440, dbg_dentry,
			    NULL, &tui_dbg_state_fops);
	tui_kobj = kobject_create_and_add("tui", kernel_kobj);
	if (!tui_kobj) {
		tloge("tui kobj create error\n");
		retval =  -ENOMEM;
		goto error2;
	}
	retval = sysfs_create_group(tui_kobj, &tui_attr_group);

	if (retval) {
		tloge("sysfs_create_group error\n");
		goto error1;
	}

	retval = register_tui_powerkeyListener();
	if (retval != 0) {
		tloge("tui register failed.\n");
		goto error1;
	}
	return 0;
error1:
	kobject_put(tui_kobj);
error2:
	kthread_stop(tui_task);
	tui_mem_free();
	return retval;

}

void tui_exit(void)
{ /*lint !e18 !e31 !e532 */
	if(unregister_tui_powerkeyListener() < 0)
	{
		tloge("tui power key unregister failed.\n");
	}
	tui_mem_free();

	kthread_stop(tui_task);
	put_task_struct(tui_task);
	debugfs_remove(dbg_dentry);
	sysfs_remove_group(tui_kobj, &tui_attr_group);
	kobject_put(tui_kobj);
} /*lint !e533 */
