#include <huawei_platform/power/power_mesg.h>

#ifdef  HWLOG_TAG
#undef  HWLOG_TAG
#endif

#define HWLOG_TAG POWER_MESG
HWLOG_REGIST();

/* board runtime */
static BLOCKING_NOTIFIER_HEAD(br_note_head);
static int board_runtime_needed = 0;
static int board_runtime_now  = 0;
static int board_runtime_step = 0;
static int board_runtime_free = 0;

static ssize_t runtime_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d %d %d", board_runtime_now,
                    board_runtime_step, board_runtime_free);
}
static ssize_t runtime_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
    if(count == sizeof(board_runtime_now)) {
        memcpy(&board_runtime_now, buf, count);
        goto update_borad_runtime;
    }

#ifdef POWER_MESG_DEBUG
    if(buf[count - 1] == '\n' && !kstrtoint(buf, 10, &board_runtime_now)) {
        goto update_borad_runtime;
    }
#endif

    hwlog_err("Illegal write data found in %s.", __func__);
    return -EINVAL;

update_borad_runtime:
    blocking_notifier_call_chain(&br_note_head, 0, &board_runtime_now);
    return count;
}

const static DEVICE_ATTR_RW(runtime);

int board_runtime_register(int rt_step, int free_rt, struct notifier_block *nb)
{
    if(rt_step > 0 && free_rt > 0) {
        board_runtime_needed = 1;
        if(rt_step < board_runtime_step || board_runtime_step == 0) {
            board_runtime_step = rt_step;
        }
        if(free_rt > board_runtime_free) {
            board_runtime_free = free_rt;
        }
        blocking_notifier_chain_cond_register(&br_note_head, nb);
        return 0;
    }
    hwlog_err("illegal para found in %s.(step:%d, free:%d)\n", __func__, rt_step, free_rt);
    return -1;
}

static int board_runtime_init(void)
{
    if(board_runtime_needed) {
        if( create_attr_for_power_mesg(&dev_attr_runtime) ) {
            hwlog_err("attribute runtime created failed in %s.\n", __func__);
            return -1;
        }
    }
    return 0;
}

/* power mesg */
static struct device *power_mesg = NULL;

int power_easy_send(power_mesg_node_t *node, unsigned char cmd,
                    unsigned char version, void *data, unsigned len)
{
    return power_genl_easy_send(node, cmd, version, data, len);
}

int power_easy_node_register(power_genl_easy_node_t *node)
{
    return power_genl_easy_node_register(node);
}

static struct device * get_power_mesg_device(void)
{
    struct class *hw_power;
    if(!power_mesg) {
        hw_power = hw_power_get_class();
        if(hw_power) {
            power_mesg = device_create(hw_power, NULL, 0, NULL, "power_mesg");
        }
    }
    return power_mesg;
}

int create_attr_for_power_mesg(const struct device_attribute *attr)
{
    struct device * dev;
    dev = get_power_mesg_device();
    if(dev) {
        return device_create_file(dev, attr);
    }
    hwlog_err("get power mesg device failed.\n");
    return -1;
}

int __init power_msg_init(void)
{
    struct device * dev;
    hwlog_info("power mesg driver init...\n");
    if(power_genl_init()) {
        hwlog_info("power genl probe failed, no attrs created.\n");
    }
    if(board_runtime_init()) {
        hwlog_info("board runtime probe failed, no attrs created.\n");
    }
    return 0;
}

void __exit power_msg_exit(void)
{
    hwlog_info("power generic netlink driver exit...\n");
}

late_initcall(power_msg_init);
module_exit(power_msg_exit);

MODULE_LICENSE("GPL");
