#ifndef __SW_CORE_H__
#define __SW_CORE_H__


/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/completion.h>

 extern int g_SW_LOGlevel;

/*****************************************************************************
  2 Define macro
*****************************************************************************/

#define SUCCESS                         (0)
#define FAILURE                         (1)


#define DEBUG_SKB_BUFF_LEN          (2048)
#define PUBLIC_BUF_MAX              (8*1024)



 /* define for tx and rx packet queue */
#define TX_HIGH_QUEUE               (1)
#define TX_LOW_QUEUE                (2)

#define TX_DATA_QUEUE                (3)
#define RX_DATA_QUEUE                (4)

#define PROTO_CMD_KEY_NORAML     (unsigned char)(3)
#define PROTO_CMD_MOUSE		     (unsigned char)(2)
#define PROTO_CMD_KEY_CONSUMER   (unsigned char)(5)
#define PROTO_CMD_KEY_CONSUMER_1 (unsigned char)(0)
#define PROTO_CMD_HANDSHAKE		 (unsigned char)(9)
#define PROTO_CMD_WORKMODE		 (unsigned char)(0xFE)
#define PROTO_CMD_DISCONNECT	 (unsigned char)(0xFF)

/*
  PCMODE  -- keyborad HID devices attach
  PADMODE -- keyboard HID devices detach
*/
#define KBMODE_PCMODE    0
#define KBMODE_PADMODE   1

#define KBSTATE_ONLINE    1
#define KBSTATE_OFFLINE   0
 /*****************************************************************************
  3 STRUCT define
*****************************************************************************/
  struct sw_device;

  struct hw_sw_core_data {

	 struct sk_buff_head tx_data_seq;
	 struct sk_buff_head rx_data_seq;
	 struct work_struct      events;

	 void  *pm_data;

	 struct sw_device* device;
	 struct sw_device* mouse_device;
	 struct sw_device* key_device;

	 u16 kb_state;
	 u8 mainver;
	 u8 subver;
	 u32 vendor;
	 u32 product;

 };


 struct hw_sw_disc_data {
	 struct platform_device *pm_pdev;

	 struct hw_sw_core_data *core_data;

	 int kb_int_gpio;
	 int kb_int_irq;
	 bool irq_enabled;
	 spinlock_t irq_enabled_lock;
	 struct work_struct irq_work;

	 int kb_tx_gpio;
	 int kb_vdd_ctrl;
	 int kb_detect_adc;

	 struct wake_lock  kb_wakelock;
	struct delayed_work kb_channel_work;
	struct completion kb_comm_complete;

	 int kb_online;
	 int start_detect;

	 int fb_state;
	 int wait_fb_on;
 };




 /************************sw Bus Driver/Device*************************/
#define SW_ANY_ID				(~0)
#define SW_BUS_ANY				0xffff
#define SW_GROUP_ANY			0x0000
#define SW_TYPE_ANY			    0x0000

#define SW_HID_TYPE             0x0001

struct sw_device_id {
	u16 bus;
	u16 group;
	u32 vendor;
	u32 product;
	u32 type;
};

struct sw_driver {
	 char *name;
	 const struct sw_device_id *id_table;
	 int (*probe)(struct sw_device *dev);
	 void (*disconnect)(struct sw_device *dev);
	 /* private: */
	struct device_driver driver;
 };


struct sw_device {
	 struct hw_sw_core_data*  port;
	 struct device dev;						/* device */
	 u16 bus;								/* BUS ID */
	 u16 group;								/* Report group */
	 u32 vendor;							/* Vendor ID */
	 u32 product;							/* Product ID */
	 u32 version;							/* HID version */
	 u32 type;
	 struct sw_driver *drv;	/* accessed from interrupt, must be protected by serio->lock and serio->sem */
	 struct mutex drv_mutex;		/* protects sw->drv so attributes can pin driver */

	 /* Report descriptor */
	 u8 *rd_data;
	 u32 rd_size;
	 void *context;			/* (in) context for hid */
	 /* worker for registering and unregistering hid devices */
	 struct delayed_work uart_work;
 };


 /*****************************************************************************
  Function declare
*****************************************************************************/
#ifdef __cplusplus
#if __cplusplus
 extern "C" {
#endif
#endif

extern int (*sw_tty_recv)(void *, const u8 *, int);
extern int sw_get_core_reference(struct hw_sw_core_data **core_data);

extern void sw_kfree_skb(struct hw_sw_core_data *ps_core_d, u8 type);



/*
 * use these in module_init()/module_exit()
 * and don't forget MODULE_DEVICE_TABLE(sw, ...)
 */
extern int sw_register_driver(struct sw_driver *, struct module *,
			       const char *);

/* use a define to avoid include chaining to get THIS_MODULE & friends */
#define sw_register(driver) \
	sw_register_driver(driver, THIS_MODULE, KBUILD_MODNAME)

extern void sw_deregister(struct sw_driver *);


extern void sw_hid_input_report(struct sw_device* dev,u8 *data, int size);

//extern int  Process_KBChannelData(char *data, int count);
extern struct sw_device *sw_allocate_device(void);


extern int create_hid_devices(struct hw_sw_core_data* core_data,struct platform_device *dev);

extern void sw_recv_data_frame(struct hw_sw_core_data* core_data,struct sk_buff *skb);

extern int sw_check_recv_data(const u8 *data, int count);

extern int hkadc_detect_kb(int kb_detect_adc);

extern int sw_relese_hid_devices(struct hw_sw_core_data* core_data);

#ifdef __cplusplus
#if __cplusplus
        }
#endif
#endif

#endif /* SW_CORE_H */