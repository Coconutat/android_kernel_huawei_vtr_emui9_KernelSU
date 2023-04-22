


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
//#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/tcp.h>

#include <net/sock.h>

#include <net/tcp.h>
#include <net/inet_sock.h>

#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#include <linux/kthread.h>  
/******************************************************************************
   2 宏定义
******************************************************************************/
#define DOZABLE_NAME    "fw_dozable"
#define WLAN_NAME       "wlan0"
#define ITEM_COUNT_MAX_VALUE    (1<<5) //32
#define ITEM_COUNT_MASK         (ITEM_COUNT_MAX_VALUE-1)
#define NETBIOS_PORT            (137)
#define SPECIAL_PORT_NUM        (0)

#define LOG_TAG					"wifi_filter"

#define DEBUG					1
#define INFO					1

#define FILTER_LOGD(fmt, ...) \
	do { \
		if (DEBUG) { \
			printk(KERN_DEBUG "["LOG_TAG"] %s: "fmt"\n", __func__, ##__VA_ARGS__); \
		} \
	} while (0)

#define FILTER_LOGI(fmt, ...) \
	do { \
		if (INFO) { \
			printk(KERN_INFO "["LOG_TAG"] %s: "fmt"\n", __func__, ##__VA_ARGS__); \
		} \
	} while (0)

#define FILTER_LOGE(fmt, ...) \
	do { \
		printk(KERN_ERR "["LOG_TAG"] %s: "fmt"\n", __func__, ##__VA_ARGS__); \
	} while (0)
/*****************************************************************************
  3 函数声明
*****************************************************************************/
static int hw_add_filter_items(hw_wifi_filter_item *items, int count);
static int hw_clear_filter_item(void);

static inline struct ipt_entry *
get_entry(const void *base, unsigned int offset)
{
	return (struct ipt_entry *)(base + offset);
}

static inline const struct xt_entry_target *
ipt_get_target_c(const struct ipt_entry *e)
{
	return ipt_get_target((struct ipt_entry *)e);
}
/******************************************************************************
   4 私有定义
******************************************************************************/
static struct task_struct *WifiFilterThread = NULL;
static wait_queue_head_t mythread_wq;
static int wake_up_condition = 0;
static bool bIsSupportWifiFilter = false;
static bool bDozeEnable = false;
static struct hw_wlan_filter_ops gWlanFilterOps;
static hw_wifi_filter_item g_filter_items[ITEM_COUNT_MAX_VALUE];
static int g_filter_item_index = SPECIAL_PORT_NUM;
static int g_filter_item_count = SPECIAL_PORT_NUM;
/******************************************************************************
   5 全局变量定义
******************************************************************************/

/******************************************************************************
   6 函数实现
******************************************************************************/

int hw_register_wlan_filter(struct hw_wlan_filter_ops *ops)
{
    if ( NULL == ops ) {
        return -1;
    }
    bIsSupportWifiFilter = true;
    gWlanFilterOps.add_filter_items     = ops->add_filter_items;
    gWlanFilterOps.clear_filters        = ops->clear_filters;
    gWlanFilterOps.get_filter_pkg_stat  = ops->get_filter_pkg_stat;
    gWlanFilterOps.set_filter_enable    = ops->set_filter_enable;
    return 0;
}
EXPORT_SYMBOL(hw_register_wlan_filter);

int hw_unregister_wlan_filter()
{
    gWlanFilterOps.add_filter_items     = NULL;
    gWlanFilterOps.clear_filters        = NULL;
    gWlanFilterOps.get_filter_pkg_stat  = NULL;
    gWlanFilterOps.set_filter_enable    = NULL;
    return 0;
}
EXPORT_SYMBOL(hw_unregister_wlan_filter);

static void get_chainname(
    unsigned int hook,
    const struct xt_table_info *private,
    const struct ipt_entry *e,
    const char **chainname )
{
	const struct ipt_entry *root;
    const struct ipt_entry *iter;
    const struct xt_standard_target *t;

    if (NULL == private || NULL == e) {
        return;
    }

    root        = get_entry(private->entries, private->hook_entry[hook]);

    xt_entry_foreach(iter, root, private->size - private->hook_entry[hook])
    {
        t = (void *)ipt_get_target_c(iter);
        if (strcmp(t->target.u.kernel.target->name, XT_ERROR_TARGET) == 0)
        {
            *chainname = t->target.data;
	    } 
        else if (iter == e)
	    {
	        break;
        }
        else
        {
        }
    }

}

static bool is_in_items_array(hw_wifi_filter_item *item)
{
    int i;

    for(i = 0; i < g_filter_item_count; i++)
    {
        g_filter_items[i].filter_cnt = 1;
        if (item->port == g_filter_items[i].port)
        {
            return true;
        }
    }
    return false;
}

void get_wifi_filter_info(struct sk_buff *skb)
{
    hw_wifi_filter_item items;
    hw_wifi_filter_item special_item;
    const struct iphdr *ip;

    ip = ip_hdr(skb);
    if (NULL == ip) {
        FILTER_LOGE("ip is null");
        return;
    }

    if (ip->protocol == IPPROTO_TCP) {
        FILTER_LOGD("tcp dest=%d, source=%d",ntohs(tcp_hdr(skb)->dest),ntohs(tcp_hdr(skb)->source));
        items.port = tcp_hdr(skb)->dest;
        items.protocol = IPPROTO_TCP;
        items.filter_cnt = 0;
    } else if (ip->protocol == IPPROTO_UDP) {
        FILTER_LOGD("udp dest=%d,source=%d",ntohs(udp_hdr(skb)->dest), ntohs(udp_hdr(skb)->source));
        items.port = udp_hdr(skb)->dest;
        items.protocol = IPPROTO_UDP;
        items.filter_cnt = 0;
    } else {
        printk("other protocol");
        return;
    }
    if (is_in_items_array(&items)) {
        FILTER_LOGD("port %d has exist",ntohs(items.port));
        return;
    }
    memcpy(&g_filter_items[g_filter_item_index],&items,sizeof(hw_wifi_filter_item));
    g_filter_item_index++;
    if (g_filter_item_count < ITEM_COUNT_MAX_VALUE)
    {
        g_filter_item_count = g_filter_item_index;
    }

    if (g_filter_item_index >= ITEM_COUNT_MAX_VALUE)
    {
        g_filter_item_index = SPECIAL_PORT_NUM;
    }

    wake_up_condition = 1;
    wake_up_interruptible(&mythread_wq);
}

void get_filter_info(
    struct sk_buff *skb,
    const struct nf_hook_state *state,
    unsigned int hook,
    const struct xt_table_info *private,
    const struct ipt_entry *e)
{
    const char *indev = "";

    if (!bIsSupportWifiFilter) {
        FILTER_LOGD("wifi filter is not support");
        return;
    }
    if (!bDozeEnable){
        FILTER_LOGD("doze is not enable");
        return;
    }
    if (NULL == state) {
        FILTER_LOGE("state is null");
        return;
    }
    indev = state->in ? state->in->name : "";
    if ( (strcmp(indev, WLAN_NAME) != 0)&&(strcmp(indev,"")!=0) ) {
    	 FILTER_LOGD("indevl name %s", indev);
        return;
    }
    
    if (NULL == skb) {
        FILTER_LOGE("skb is null");
        return;
    }

    get_wifi_filter_info(skb);
}

void get_filter_infoEx(struct sk_buff *skb)
{

    if (!bIsSupportWifiFilter) {
        FILTER_LOGD("wifi filter is not support");
        return;
    }
    if (!bDozeEnable){
        FILTER_LOGD("doze is not enable");
        return;
    }
    if (NULL == skb) {
        FILTER_LOGE("skb is null");
        return;
    }
	
    if (NULL == skb->dev) {
        FILTER_LOGE("skb->dev is null");
        return;
    }

    if ( (strcmp(skb->dev->name, WLAN_NAME)!=0)&&(strcmp(skb->dev->name,"")!=0) ){
    	 FILTER_LOGD("indevl name %s", skb->dev->name);
        return;
    }

    get_wifi_filter_info(skb);
}


static int hw_add_filter_items(hw_wifi_filter_item *items, int count)
{
    if ( NULL == gWlanFilterOps.add_filter_items )
    {
        return -1;
    }
    FILTER_LOGD("hw_add_filter_items count=%d", count);

    return gWlanFilterOps.add_filter_items(items, count);
}

static int hw_clear_filter_item(void)
{
    if ( NULL == gWlanFilterOps.clear_filters )
    {
        return -1;
    }

    g_filter_item_count = SPECIAL_PORT_NUM;
    g_filter_item_index = SPECIAL_PORT_NUM;
    FILTER_LOGD("hw_clear_filter_item");
    return gWlanFilterOps.clear_filters();
}

int hw_set_net_filter_enable(int enable)
{
    int count = 0;
    int i;
    FILTER_LOGD("enable=%d", enable);
    bDozeEnable = (bool)enable;
    if ( NULL == gWlanFilterOps.set_filter_enable )
    {
        return -1;
    }

    if (!enable) {
        if(gWlanFilterOps.get_filter_pkg_stat != NULL)
        {
            gWlanFilterOps.get_filter_pkg_stat(&g_filter_items[0],ITEM_COUNT_MAX_VALUE,&count);
            FILTER_LOGD("item count=%d",count);
            for (i = 0; i < count; i++)
            {
                FILTER_LOGD("i = %d,filter packet count=%d",i,g_filter_items[i].filter_cnt);
            }
        }
        hw_clear_filter_item();
    }
    
    return gWlanFilterOps.set_filter_enable(enable);
}
EXPORT_SYMBOL(hw_set_net_filter_enable);

static int wifi_filter_threadfn(void *data)  
{  
    while(1)
    {
        wait_event_interruptible(mythread_wq, wake_up_condition);
        wake_up_condition = 0;
        FILTER_LOGD("filter thread\n");
        hw_add_filter_items(&g_filter_items[0], g_filter_item_count);
    }
    return 0;  
}  

static int __init init_kthread(void)  
{
    int err;
    init_waitqueue_head(&mythread_wq); 
    WifiFilterThread = kthread_run(wifi_filter_threadfn,NULL,"wifi_filter_thread");
    if (IS_ERR(WifiFilterThread))
    {
        FILTER_LOGE("create new kernel thread failed");
        err = PTR_ERR(WifiFilterThread);
        WifiFilterThread = NULL;
        return err;
    }
    return 0;  
}  
  
static void __exit exit_kthread(void)  
{  
    if(WifiFilterThread)  
    {  
        FILTER_LOGI("stop MyThread");  
        kthread_stop(WifiFilterThread);  
    }  
}  
  
module_init(init_kthread);  
module_exit(exit_kthread);  
  
  
MODULE_AUTHOR("z00220931");  
MODULE_LICENSE("GPL");
