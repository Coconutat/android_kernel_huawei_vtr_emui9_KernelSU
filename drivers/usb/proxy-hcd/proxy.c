
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ch9.h>
#include <linux/usb/ch11.h>

#include "proxy-hcd.h"
#include "client-ref.h"

#ifdef DEBUG
#define DBG(format, arg...) pr_info("[proxy][DBG][%s]" format, __func__, ##arg)
#else
#define DBG(format, arg...)
#endif
#define INFO(format, arg...) pr_info("[proxy][INFO][%s]" format, __func__, ##arg)
#define ERR(format, arg...) pr_err("[proxy][ERR][%s]" format, __func__, ##arg)

void proxy_port_status_change(struct proxy_hcd_client *client, __u32 port_bitmap)
{
	struct proxy_hcd *phcd;
	struct usb_hcd *hcd;
	unsigned long flags;

	INFO("+\n");

	phcd = client_to_phcd(client);

	spin_lock_irqsave(&phcd->lock, flags); /*lint !e666 */
	phcd->port_bitmap |= port_bitmap;

	hcd = phcd->hcd;
	if (!hcd) {
		/*
		 * The proxy-hcd module is loaded earlier than xhci module.
		 * This will never hanppen.
		 */
		spin_unlock_irqrestore(&phcd->lock, flags);
		WARN_ON(1);
		return;
	}

	if (hcd->state == HC_STATE_SUSPENDED) {
		DBG("hcd suspended, resume root hub\n");
		usb_hcd_resume_root_hub(hcd);
	}
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	spin_unlock_irqrestore(&phcd->lock, flags);

	usb_hcd_poll_rh_status(hcd);

	INFO("-\n");
}

void proxy_port_disconnect(struct proxy_hcd_client *client, __u32 port_bitmap)
{
	struct proxy_hcd *phcd;
	struct usb_hcd *hcd;
	unsigned long flags;

	INFO("+\n");

	phcd = client_to_phcd(client);

	mutex_lock(&phcd->mutex);
	spin_lock_irqsave(&phcd->lock, flags); /*lint !e666 */
	phcd->port_bitmap = port_bitmap;

	hcd = phcd->hcd;
	if (!hcd) {
		/*
		 * The proxy-hcd module is loaded earlier than xhci module.
		 * This will never hanppen.
		 */
		spin_unlock_irqrestore(&phcd->lock, flags);
		mutex_unlock(&phcd->mutex);
		WARN_ON(1);
		return;
	}

	if (hcd->state == HC_STATE_SUSPENDED) {
		DBG("hcd suspended, resume root hub\n");
		usb_hcd_resume_root_hub(hcd);
	}
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	spin_unlock_irqrestore(&phcd->lock, flags);

	usb_hcd_poll_rh_status(hcd);
	mutex_unlock(&phcd->mutex);

	INFO("-\n");
}

int proxy_urb_complete(struct proxy_hcd_client *client, struct urb_msg *urb_msg)
{
	struct proxy_hcd *phcd = client_to_phcd(client);
	int ret;

	ret = phcd_urb_complete(phcd, urb_msg);
	if (ret) {
		ERR("phcd_urb_complete failed!\n");
		return ret;
	}

	/*
	 * only ref_put when urb_complete sucess, because the urb maybe
	 * dequeued already
	 */
	client_ref_put(&client->client_ref);
	return 0;
}

int proxy_urb_enqueue(struct proxy_hcd *phcd, struct urb_msg *phcd_urb)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->urb_enqueue)
		ret = client->ops->urb_enqueue(client, phcd_urb);
	else
		ret = -ENODEV;

	if (ret) /*lint !e644 */
		/* enqueue urb failed, won't a urb completion, put ref */
		client_ref_put(&client->client_ref);

	return ret;
}

int proxy_urb_dequeue(struct proxy_hcd *phcd, struct urb_msg *phcd_urb)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->urb_enqueue)
		ret = client->ops->urb_dequeue(client, phcd_urb);

	client_ref_put(&client->client_ref);

	return ret;
}


int proxy_hub_control(struct proxy_hcd *phcd, __u16 typeReq, __u16 wValue,
		__u16 wIndex, char *buf, __u16 wLength)
{
	struct proxy_hcd_client *client = phcd->client;
	struct usb_ctrlrequest cmd;
	int ret = 0;

	might_sleep();

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	cmd.bRequestType = (__u8)((typeReq >> 8) & 0xff);
	cmd.bRequest = (__u8)(typeReq & 0xff);
	cmd.wValue = wValue; /*lint !e63 */
	cmd.wIndex = wIndex; /*lint !e63 */
	cmd.wLength = wLength; /*lint !e63 */

	ret = client->ops->hub_control(client, &cmd, buf);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_alloc_dev(struct proxy_hcd *phcd, int *slot_id)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->alloc_dev)
		ret = client->ops->alloc_dev(client, slot_id);

	client_ref_put(&client->client_ref);

	return ret;
}

void proxy_free_dev(struct proxy_hcd *phcd, int slot_id)
{
	struct proxy_hcd_client *client = phcd->client;

	if (!client_ref_tryget_live(&client->client_ref))
		return;

	if (client->ops->free_dev)
		client->ops->free_dev(client, slot_id);

	client_ref_put(&client->client_ref);
}

int proxy_add_endpoint(struct proxy_hcd *phcd, struct usb_device *udev,
				struct usb_host_endpoint *ep)
{
	struct proxy_hcd_client *client = phcd->client;
	int slot_id = udev->slot_id;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev != udev)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->add_endpoint)
		ret = client->ops->add_endpoint(client, slot_id, &ep->desc);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_drop_endpoint(struct proxy_hcd *phcd, struct usb_device *udev,
			struct usb_host_endpoint *ep)
{
	struct proxy_hcd_client *client = phcd->client;
	int slot_id = udev->slot_id;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev != udev)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->drop_endpoint)
		ret = client->ops->drop_endpoint(client, slot_id, &ep->desc);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_check_bandwidth(struct proxy_hcd *phcd, int slot_id)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->check_bandwidth)
		ret = client->ops->check_bandwidth(client, slot_id);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_reset_bandwidth(struct proxy_hcd *phcd, int slot_id)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->reset_bandwidth)
		ret = client->ops->reset_bandwidth(client, slot_id);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_address_device(struct proxy_hcd *phcd, int slot_id)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->address_device)
		ret = client->ops->address_device(client, slot_id);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_enable_device(struct proxy_hcd *phcd, int slot_id, int speed, int ep0_maxpks)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->enable_device)
		ret = client->ops->enable_device(client, slot_id, speed, ep0_maxpks);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_reset_device(struct proxy_hcd *phcd, int slot_id)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->reset_device)
		ret = client->ops->reset_device(client, slot_id);

	client_ref_put(&client->client_ref);

	return ret;
}

int proxy_update_device(struct proxy_hcd *phcd, int slot_id,
		struct usb_ext_cap_descriptor *ext_cap)
{
	struct proxy_hcd_client *client = phcd->client;
	int ret = 0;

	if (!phcd->phcd_udev.udev || phcd->phcd_udev.udev->slot_id != slot_id)
		return -EINVAL;

	if (!client_ref_tryget_live(&client->client_ref))
		return -ESHUTDOWN;

	if (client->ops->update_device)
		ret = client->ops->update_device(client, slot_id, ext_cap);

	client_ref_put(&client->client_ref);

	return ret;

}
