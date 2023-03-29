

#ifndef __LINUX_XHCI_LOCAL_MEM_H
#define __LINUX_XHCI_LOCAL_MEM_H

#include <linux/types.h>

struct xhci_hcd;
struct urb;

struct xhci_local_dma_manager {
	struct work_struct dma_free_wk;
	spinlock_t lock;
	struct list_head dma_free_list;
};

struct xhci_local_dma {
	struct list_head list;
	void *vaddr;
	dma_addr_t dma;
	size_t size;
};

#ifdef CONFIG_USB_DWC3_NYET_ABNORMAL
int xhci_create_dma_pool(struct xhci_hcd *xhci);
void xhci_destroy_dma_pool(struct xhci_hcd *xhci);
int xhci_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags);
void xhci_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb);
void xhci_dma_free_handler(struct work_struct *work);
#else
static inline int xhci_create_dma_pool(struct xhci_hcd *xhci) {return -EFAULT;}
static inline void xhci_destroy_dma_pool(struct xhci_hcd *xhci) {}
static inline int xhci_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
{
	return -EFAULT;
}
static inline void xhci_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb) {}
static inline void xhci_dma_free_handler(struct work_struct *work) {}
#endif

#endif /* __LINUX_XHCI_LOCAL_MEM_H */
