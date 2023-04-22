#ifndef _PROXY_HCD_H_
#define _PROXY_HCD_H_

#include <linux/list.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/hcd.h>

#include "client-ref.h"
#include "proxy-hcd-stat.h"

#define PHCD_MAX_XFER_LEN 8192

struct urb_msg {
	__u64 urb_magic;
	__u32 slot_id;
	__u32 pipe;

	__s32 status;			/* (return) non-ISO status */
	__u32 transfer_flags;		/* (in) URB_SHORT_NOT_OK | ...*/
	__u32 transfer_buffer_length;	/* (in) data buffer length */
	__u32 actual_length;		/* (return) actual transfer length */
	__s32 interval;			/* (modify) transfer interval */

	void *share_buf;
	dma_addr_t share_buf_dma;

	__u8 *buf;
} /*__attribute__ ((packed))*/;

#define PHCD_URB_ENQUEUEING (1 << 1)
struct proxy_hcd_urb {
	struct list_head urb_list;
	struct urb *urb;
	struct urb_msg *urb_msg;

	unsigned long flags;
};

struct proxy_hcd_client;

struct proxy_hcd_client_ops {
	int (*alloc_dev)(struct proxy_hcd_client *client, int *slot_id);
	void (*free_dev)(struct proxy_hcd_client *client, int slot_id);

	int (*address_device)(struct proxy_hcd_client *client, int slot_id);
	int (*enable_device)(struct proxy_hcd_client *client, int slot_id,
			int speed, int ep0_maxpks);
	int (*reset_device)(struct proxy_hcd_client *client, int slot_id);
	int (*update_device)(struct proxy_hcd_client *client, int slot_id,
			struct usb_ext_cap_descriptor *ext_cap);

	int (*add_endpoint)(struct proxy_hcd_client *client, int slot_id,
			struct usb_endpoint_descriptor *desc);
	int (*drop_endpoint)(struct proxy_hcd_client *client, int slot_id,
			struct usb_endpoint_descriptor *desc);
	int (*check_bandwidth)(struct proxy_hcd_client *client, int slot_id);
	int (*reset_bandwidth)(struct proxy_hcd_client *client, int slot_id);

	int (*hub_control)(struct proxy_hcd_client *, struct usb_ctrlrequest *, char *);

	int (*urb_enqueue) (struct proxy_hcd_client *, struct urb_msg *);
	int (*urb_dequeue) (struct proxy_hcd_client *, struct urb_msg *);
};

struct proxy_hcd_client {
#define PROXY_HCD_CLIENT_NAME_LEN 32
	char name[PROXY_HCD_CLIENT_NAME_LEN];
	struct list_head node;
	struct proxy_hcd_client_ops *ops;

	int (*client_init)(struct proxy_hcd_client *client);
	void (*client_exit)(struct proxy_hcd_client *client);
	int (*client_suspend)(struct proxy_hcd_client *client);
	void (*client_resume)(struct proxy_hcd_client *client);

	void *phcd;
	void *client_priv;
	struct client_ref client_ref;
};

#define PROXY_HCD_MAX_PORTS 1
#define PROXY_HCD_DEV_MAX_EPS 31

struct proxy_hcd_ep {
	struct list_head urb_list;
	int added;
	struct proxy_hcd_urb_stat urb_stat;
};

struct proxy_hcd_usb_device {
	struct usb_device *udev;
	struct proxy_hcd_ep phcd_eps[PROXY_HCD_DEV_MAX_EPS];

	struct proxy_hcd_usb_device_stat stat;
};

#define PHCD_PORT_STATUS_MASK (USB_PORT_STAT_CONNECTION		\
			| USB_PORT_STAT_ENABLE			\
			/* | USB_PORT_STAT_SUSPEND */		\
			/* | USB_PORT_STAT_OVERCURRENT */	\
			| USB_PORT_STAT_RESET			\
			/* | USB_PORT_STAT_L1 */		\
			/* | USB_PORT_STAT_POWER */		\
			| USB_PORT_STAT_LOW_SPEED		\
			| USB_PORT_STAT_HIGH_SPEED		\
			/* | USB_PORT_STAT_TEST */		\
			/* | USB_PORT_STAT_INDICATOR */)
#define PHCD_PORT_STATUS_CHANGE_MASK ((USB_PORT_STAT_C_CONNECTION	\
				| USB_PORT_STAT_C_ENABLE		\
				/* | USB_PORT_STAT_C_SUSPEND */		\
				/* | USB_PORT_STAT_C_OVERCURRENT */	\
				| USB_PORT_STAT_C_RESET			\
				/* | USB_PORT_STAT_C_L1 */) << 16)


/* There is one xhci_hcd structure per controller */
struct proxy_hcd {
	struct usb_hcd			*hcd;	/* bound to a usb_hcd */
	struct proxy_hcd_client		*client;
	struct platform_device		*pdev;

	/* ports of hc */
	__u32				port_bitmap;
	__u32				port_status[PROXY_HCD_MAX_PORTS];
	unsigned int			port_count;

	/* support only one usb_device */
	struct proxy_hcd_usb_device	phcd_udev;
	spinlock_t			lock;
	struct mutex			mutex;

	struct proxy_hcd_stat		stat;
	struct dentry			*debugfs_root;
};

static inline struct proxy_hcd *hcd_to_phcd(struct usb_hcd *hcd)
{
	return *((struct proxy_hcd **) (hcd->hcd_priv));
}

static inline struct proxy_hcd *client_to_phcd(struct proxy_hcd_client *client)
{
	return (struct proxy_hcd *)client->phcd;
}

/*
 * proxy support
 */
int proxy_alloc_dev(struct proxy_hcd *phcd, int *slot_id);
void proxy_free_dev(struct proxy_hcd *phcd, int slot_id);
int proxy_add_endpoint(struct proxy_hcd *phcd, struct usb_device *udev,
				struct usb_host_endpoint *ep);
int proxy_drop_endpoint(struct proxy_hcd *phcd, struct usb_device *udev,
				struct usb_host_endpoint *ep);
int proxy_check_bandwidth(struct proxy_hcd *phcd, int slot_id);
int proxy_reset_bandwidth(struct proxy_hcd *phcd, int slot_id);
int proxy_address_device(struct proxy_hcd *phcd, int slot_id);
int proxy_enable_device(struct proxy_hcd *phcd, int slot_id, int speed, int ep0_maxpks);
int proxy_reset_device(struct proxy_hcd *phcd, int slot_id);

int proxy_urb_enqueue(struct proxy_hcd *phcd, struct urb_msg *phcd_urb);
int proxy_urb_dequeue(struct proxy_hcd *phcd, struct urb_msg *phcd_urb);

int proxy_update_device(struct proxy_hcd *phcd, int slot_id,
		struct usb_ext_cap_descriptor *ext_cap);
int proxy_hub_control(struct proxy_hcd *phcd, __u16 typeReq, __u16 wValue,
		__u16 wIndex, char *buf, __u16 wLength);


/*
 * proxy.c
 */
void proxy_port_status_change(struct proxy_hcd_client *client, __u32 port_bitmap);
void proxy_port_disconnect(struct proxy_hcd_client *client, __u32 port_bitmap);
int proxy_urb_complete(struct proxy_hcd_client *client, struct urb_msg *phcd_urb);


/*
 * proxy-hcd.c
 */
void phcd_update_port_status(struct proxy_hcd *phcd, __u16 port_index,
			__u32 status);
int phcd_urb_complete(struct proxy_hcd *phcd, struct urb_msg *urb_msg);
void phcd_mark_all_endpoint_dropped(struct proxy_hcd *phcd);
__u32 phcd_current_port_status(struct proxy_hcd *phcd);

int phcd_register_client(struct proxy_hcd_client *client);
void phcd_unregister_client(struct proxy_hcd_client *client);
void phcd_giveback_all_urbs(struct proxy_hcd_client *client);

enum hibernation_policy {
	HIFI_USB_HIBERNATION_ALLOW = 0,
	HIFI_USB_HIBERNATION_FORBID,
	HIFI_USB_HIBERNATION_FORCE,
};
enum hibernation_policy phcd_get_hibernation_policy(struct proxy_hcd_client *client);

/*
 * usbaudio-monirot.c
 */
bool stop_hifi_usb_when_non_usbaudio(struct usb_device *udev, int configuration);
bool is_huawei_usb_c_audio_adapter(struct usb_device *udev);

#endif
