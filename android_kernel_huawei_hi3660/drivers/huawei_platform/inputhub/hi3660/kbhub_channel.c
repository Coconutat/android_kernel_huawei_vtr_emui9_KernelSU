/*
 *  inputhub/kbhub_channel.c
 *  Single wire UART Kyeboard  Channel driver
 *
 *  Copyright (C) 2017 Huawei, Inc.
 *  Author: @
 *  Date:   2017.8.10
 *
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "inputhub_route.h"
#include "inputhub_bridge.h"
#include "protocol.h"

/* ioctl cmd define */
#define KBHBIO                         0xB1

#define KBHB_IOCTL_START            _IOW(KBHBIO, 0x01, short)
#define KBHB_IOCTL_STOP             _IOW(KBHBIO, 0x02, short)
#define KBHB_IOCTL_ATTR_START       _IOW(KBHBIO, 0x03, short)
#define KBHB_IOCTL_ATTR_STOP        _IOW(KBHBIO, 0x04, short)
#define KBHB_IOCTL_INTERVAL_SET     _IOW(KBHBIO, 0x05, short)
#define KBHB_IOCTL_CMD              _IOW(KBHBIO, 0x06, short)


typedef enum {
    KB_TYPE_START = 0x0,
    KB_TYPE_UART_RUN,
    KB_TYPE_UART_STOP,
    KB_TYPE_UART_RESTART,
    KB_TYPE_END,
} kb_type_t;

typedef struct {
    pkt_header_t hd;
    kb_type_t cmd;
} pkt_kb_data_req_t;

#define KBHUB_REPORT_DATA_SIZE   (16)
typedef struct {
    unsigned int sub_cmd;
    int report_len;
    uint8_t report_data[KBHUB_REPORT_DATA_SIZE];
} kb_outreport_t;

typedef struct {
    int         fhb_ioctl_app_cmd;
    int         ca_type;
    int         tag;
    obj_cmd_t   cmd;
} kb_cmd_map_t ;

static bool kbchannel_status = false;
#ifdef CONFIG_HW_SW_DEVICES
extern int  Process_KBChannelData(char *data, int count);
extern void Notify_KB_DoDetect(void);
extern int sw_get_statue(void) ;
#else
int  Process_KBChannelData(char *data, int count)
{
    return -1;
}
void Notify_KB_DoDetect(void) { }
int sw_get_statue(void)
{
    return  -1 ;
}
#endif

extern int write_customize_cmd_noresp(int tag, int cmd, const void* data,
                                      int length);

extern int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length,
        struct read_info *rd);
extern bool really_do_enable_disable(int *ref_cnt, bool enable, int bit);

static int kb_ref_cnt = 0;
static const kb_cmd_map_t kb_cmd_map_tab[] = {
    {KBHB_IOCTL_START,  -1, TAG_KB, CMD_KB_OPEN_REQ },
    {KBHB_IOCTL_STOP,   -1, TAG_KB, CMD_KB_CLOSE_REQ},
    {KBHB_IOCTL_CMD,    -1, TAG_KB, CMD_KB_EVENT_REQ},
};

extern  struct wake_lock wlock;
extern int iom3_power_state;
extern int g_iom3_state;

int send_kb_cmd_internal(int tag, obj_cmd_t cmd, kb_type_t type, bool use_lock)
{
    pkt_header_t hpkt;
    pkt_cmn_interval_req_t i_pkt;
    pkt_cmn_close_req_t c_pkt;
    struct read_info rd;
    memset(&rd, 0, sizeof(rd));
    memset(&i_pkt, 0, sizeof(i_pkt));
    memset(&c_pkt, 0, sizeof(c_pkt));
    if (CMD_CMN_OPEN_REQ == cmd) {
        kbchannel_status = true;
        if (really_do_enable_disable(&kb_ref_cnt, true, type)) {
            hpkt.tag = TAG_KB;
            hpkt.cmd = CMD_CMN_OPEN_REQ;
            hpkt.resp = RESP;//NO_RESP
            hpkt.length = 0;
            if (use_lock) {
                inputhub_mcu_write_cmd_adapter(&hpkt, sizeof(hpkt), &rd);
            } else {
                inputhub_mcu_write_cmd_nolock(&hpkt, sizeof(hpkt));
            }
            i_pkt.hd.tag = tag;
            i_pkt.hd.cmd = CMD_CMN_INTERVAL_REQ;
            i_pkt.hd.resp = RESP;
            i_pkt.hd.length = sizeof(i_pkt.param);
            if (use_lock) {
                inputhub_mcu_write_cmd_adapter(&i_pkt, sizeof(i_pkt), &rd);
            } else {
                inputhub_mcu_write_cmd_nolock(&i_pkt, sizeof(i_pkt));
            }
            hwlog_info("kb: %s:CMD_CMN_OPEN_REQ cmd:%d SUCC [%x]!\n",__func__, cmd,kb_ref_cnt);
        } else {
            hwlog_info("kb: %s:CMD_CMN_OPEN_REQ cmd:%d FAIL [%x]!\n",__func__, cmd,kb_ref_cnt);
        }
    } else if ( CMD_CMN_CLOSE_REQ == cmd) {
        kbchannel_status = false;
        if (really_do_enable_disable(&kb_ref_cnt, false, type)) {
            c_pkt.hd.tag = TAG_KB;
            c_pkt.hd.cmd = CMD_CMN_CLOSE_REQ;
            c_pkt.hd.resp = RESP;
            c_pkt.hd.length = sizeof(c_pkt.close_param);
            if (use_lock) {
                inputhub_mcu_write_cmd_adapter(&c_pkt, sizeof(c_pkt), &rd);
            } else {
                inputhub_mcu_write_cmd_nolock(&c_pkt, sizeof(c_pkt));
            }
            hwlog_info("kb: %s: CMD_CMN_CLOSE_REQ cmd:%d SUCC !\n", __func__, cmd);
        } else {
            hwlog_info("kb: %s: CMD_CMN_CLOSE_REQ cmd:%d FAIL !\n", __func__, cmd);
        }
    } else {
        hwlog_err("kb: %s: unknown cmd!\n", __func__);
        return -EINVAL;
    }
    return 0;
}

int send_kb_cmd(unsigned int cmd, unsigned long arg)
{
    void __user* argp = (void __user*)arg;
    int argvalue = 0;
    int i;
    hwlog_info("kb: %s enter!\n", __func__);
    for (i = 0; i < sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]); i++) {
        if (kb_cmd_map_tab[i].fhb_ioctl_app_cmd == cmd) {
            break;
        }
    }
    if (sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]) == i) {
        hwlog_err("kb: %s unknown cmd %d in parse_ca_cmd!\n", __func__,cmd);
        return -EFAULT;
    }
    if (copy_from_user(&argvalue, argp, sizeof(argvalue))) {
        hwlog_err("kb: %s copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    if (!
        (KB_TYPE_START <= argvalue
         && argvalue < KB_TYPE_END)) {
        hwlog_err("error kb type %d in %s\n", argvalue, __func__);
        return -EINVAL;
    }
    return send_kb_cmd_internal(kb_cmd_map_tab[i].tag, kb_cmd_map_tab[i].cmd, argvalue, true);//true
}

void enable_kb_when_recovery_iom3(void)
{
    kb_ref_cnt = 0;
    hwlog_info("kb: %s enter!\n", __func__);
    //notify sensorhub start work
    if(kbchannel_status)
    {
       hwlog_info("kb: %s enable kbchannel !\n", __func__);
       send_kb_cmd_internal(TAG_KB, CMD_KB_OPEN_REQ , 0, false);
    }
}

void disable_kb_when_sysreboot(void)
{
    hwlog_info("kb: %s enter!\n", __func__);
}


int kernel_send_kb_cmd(unsigned int cmd,int val)
{
    int i=0;
    hwlog_info("kb: %s enter!\n", __func__);
    for (i = 0; i < sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]); i++) {
        if (kb_cmd_map_tab[i].fhb_ioctl_app_cmd == cmd) {
            break;
        }
    }
    if (sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]) == i) {
        hwlog_err("kb: %s unknown cmd %d in parse_ca_cmd!\n", __func__,cmd);
        return -EFAULT;
    }

    if (!
        (KB_TYPE_START <= val
         && val < KB_TYPE_END)) {
        hwlog_err("error kb type %d in %s\n", val, __func__);
        return -EINVAL;
    }
    return send_kb_cmd_internal(kb_cmd_map_tab[i].tag, kb_cmd_map_tab[i].cmd, val, true);
}

int kernel_send_kb_report_event(unsigned int cmd, void* buffer,int size)
{
    int ret = 0;
    int i=0;

    write_info_t pkg_ap;
    read_info_t pkg_mcu;
    kb_outreport_t  outreport;
    hwlog_info("kb: %s enter!\n", __func__);

    if(size>KBHUB_REPORT_DATA_SIZE) {
        return -EFAULT;
    }

    for (i = 0; i < sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]); i++) {
        if (kb_cmd_map_tab[i].fhb_ioctl_app_cmd == cmd) {
            break;
        }
    }
    if (sizeof(kb_cmd_map_tab) / sizeof(kb_cmd_map_tab[0]) == i) {
        hwlog_err("kb: %s unknown cmd %d in parse_ca_cmd!\n", __func__,cmd);
        return -EFAULT;
    }


    memset(&pkg_ap, 0, sizeof(pkg_ap));
    memset(&pkg_mcu, 0, sizeof(pkg_mcu));
    memset(&outreport, 0, sizeof(outreport));

    pkg_ap.cmd = CMD_CMN_CONFIG_REQ ;
    pkg_ap.tag =kb_cmd_map_tab[i].tag;
    outreport.sub_cmd = kb_cmd_map_tab[i].cmd;
    outreport.report_len=size;
    memcpy(outreport.report_data,buffer,size);
    pkg_ap.wr_buf = &outreport;
    pkg_ap.wr_len = sizeof(outreport);;
    if (g_iom3_state == IOM3_ST_RECOVERY || iom3_power_state == ST_SLEEP) {
        ret = write_customize_cmd_noresp(pkg_ap.tag,
                                         pkg_ap.cmd,
                                         pkg_ap.wr_buf,
                                         pkg_ap.wr_len);
    } else
        ret = write_customize_cmd(&pkg_ap, &pkg_mcu);

    if (ret < 0) {
        hwlog_err("err. write cmd\n");
        return -1;
    }

    if (0 != pkg_mcu.errno) {
        hwlog_info("mcu err \n");
        return -1;
    }

    return 0;

}

void kbhb_notify_mcu_state(sys_status_t status)
{
    if(status == ST_MCUREADY) {
        Notify_KB_DoDetect();
    }
}


extern volatile int hall_value;

int kbhb_get_hall_value(void)
{
   int val = hall_value;
   //hwlog_info("[HALL]%s  value = %d \n", __func__,val);
   return val;
}

#ifdef USE_KBHB_DEVICE
/*******************************************************************************************
Function:       kbhb_read
Description:   read /dev/kbhub
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         no
Return:         length of read data
*******************************************************************************************/
static ssize_t kbhb_read(struct file *file, char __user *buf, size_t count,
                         loff_t *pos)
{
    hwlog_info("%s go!\n", __func__); 
    return 0;
}

/*******************************************************************************************
Function:       kbhb_write
Description:   write to /dev/kbhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, const char __user *data, size_t len, loff_t *ppos
Output:         no
Return:         length of write data
*******************************************************************************************/
static ssize_t kbhb_write(struct file *file, const char __user *data,
                          size_t len, loff_t *ppos)
{
    hwlog_info("%s need to do...\n", __func__);

    return len;
}

/*******************************************************************************************
Function:       kbhb_ioctl
Description:   ioctrl function to /dev/kbhub, do open, close kb, or set interval and attribute to kb
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, unsigned int cmd, unsigned long arg
            cmd indicates command, arg indicates parameter
Output:         no
Return:         result of ioctrl command, 0 successed, -ENOTTY failed
*******************************************************************************************/
static long kbhb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case KBHB_IOCTL_START:
            break;
        case KBHB_IOCTL_STOP:
            break;
        case KBHB_IOCTL_CMD:
            break;
        default:
            hwlog_err("%s unknown cmd : %d\n", __func__, cmd);
            return -ENOTTY;
    }
    hwlog_err("%s  cmd : %d\n", __func__, cmd); 

    return 0;
}

/*******************************************************************************************
Function:       kbhb_open
Description:   open to /dev/kbhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of open
*******************************************************************************************/
static int kbhb_open(struct inode *inode, struct file *file)
{
    hwlog_info("%s ok!\n", __func__);
    return 0;
}

/*******************************************************************************************
Function:       kbhb_release
Description:   releaseto /dev/kbhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of release
*******************************************************************************************/
static int kbhb_release(struct inode *inode, struct file *file)
{
    hwlog_info("%s ok!\n", __func__);
    return 0;
}

/*******************************************************************************************
Description:   file_operations to kb
*******************************************************************************************/
static const struct file_operations kbhb_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = kbhb_read,
    .write = kbhb_write,
    .unlocked_ioctl = kbhb_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = kbhb_ioctl,
#endif
    .open = kbhb_open,
    .release = kbhb_release,
};

/*******************************************************************************************
Description:   miscdevice to kb
*******************************************************************************************/
static struct miscdevice kbhub_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kbhub",
    .fops = &kbhb_fops,
};
#endif

/*******************************************************************************************
Function:       kb_report_callback
Description:    kb report function , register inputhub mcu event notifier
Data Accessed:  no
Data Updated:   no
Input:          const pkt_header_t *head
Output:
Return:        result of function, 0 successed, else false
*******************************************************************************************/
int kb_report_callback(const pkt_header_t *head)
{
    int ret = -1;
    int count = 0;
    char* kb_data = NULL;
    if(!head) {
        return -1;
    }
    kb_data = (char*)head + sizeof(pkt_header_t);

    count = kb_data[1];
    ret = Process_KBChannelData(kb_data,count);
    return ret;
}
/*******************************************************************************************
Function:       kbhub_init
Description:   apply kernel buffer, register kbhub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        result of function, 0 successed, else false
*******************************************************************************************/
static int __init kbhub_init(void)
{
    int ret;

    if (is_sensorhub_disabled())
        return -1;

    if (!getSensorMcuMode()) {
        hwlog_err("mcu boot fail,kbhub_init exit\n");
        return -1;
    }

    if(sw_get_statue() != 0 ) {
        hwlog_err("sw core not running,kbhub_init exit\n");
        return -1;
    }

    hwlog_info("%s start \n", __func__);

    ret = inputhub_route_open(ROUTE_KB_PORT);
    if (ret != 0) {
        hwlog_err("cannot open inputhub route err=%d\n", ret);
        return ret;
    }
#ifdef USE_KBHB_DEVICE
    ret = misc_register(&kbhub_miscdev);
    if (ret != 0) {
        hwlog_err("cannot register miscdev err=%d\n", ret);
        inputhub_route_close(ROUTE_KB_PORT);
        return ret;
    }
#endif
    register_mcu_event_notifier(TAG_KB, CMD_KB_REPORT_REQ,
                                kb_report_callback);

    hwlog_info("%s ok\n", __func__);
    return ret;
}

/*******************************************************************************************
Function:       kbhub_exit
Description:   release kernel buffer, deregister kbhub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        void
*******************************************************************************************/
static void __exit kbhub_exit(void)
{

    if(sw_get_statue() != 0 ) {
        return ;
    }
    inputhub_route_close(ROUTE_KB_PORT);
#ifdef USE_KBHB_DEVICE
    misc_deregister(&kbhub_miscdev);
#endif
    hwlog_info("exit %s\n", __func__);
}

//module_init(kbhub_init);
late_initcall_sync(kbhub_init);
module_exit(kbhub_exit);

MODULE_AUTHOR("KBHub <chenquanquan@huawei.com>");
MODULE_DESCRIPTION("KBHub driver");
MODULE_LICENSE("GPL");
