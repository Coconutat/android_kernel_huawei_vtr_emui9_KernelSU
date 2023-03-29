/*
 * urb buffers allocator
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/slab.h>

#include "hifi_lpp.h"
#include "hifi-usb-urb-buf.h"

#define HIFI_USB_SHARE_BUFFER_ADDR HIFI_USB_DRIVER_SHARE_MEM_ADDR
#define HIFI_USB_SHARE_BUFFER_LEN HIFI_USB_DRIVER_SHARE_MEM_SIZE

void free_urb_buf(struct urb_buffers *urb_bufs, void *addr)
{ /*lint !e49 !e601 */
	unsigned int index;

	index = (addr - urb_bufs->urb_buf[0]) / urb_bufs->urb_buf_len; /*lint !e409 */
	if (index < urb_bufs->urb_buf_num) /*lint !e574 */
		clear_bit((int)index, &urb_bufs->urb_buf_bitmap);
	else
		WARN_ON(1);
}

void *alloc_urb_buf(struct urb_buffers *urb_bufs, dma_addr_t *dma_addr) /*lint !e49 !e601 */
{ /*lint !e49 !e601 */
	unsigned int index;

	while (1) {
		index = (unsigned int)find_first_zero_bit(&urb_bufs->urb_buf_bitmap,
				urb_bufs->urb_buf_num);
		if (unlikely(index >= urb_bufs->urb_buf_num))
			return NULL;

		if (!test_and_set_bit(index, &urb_bufs->urb_buf_bitmap))
			break;
	}

	*dma_addr = urb_bufs->urb_buf_dma[index]; /*lint !e63 !e409 */
	return urb_bufs->urb_buf[index]; /*lint !e409 */
}

int urb_buf_init(struct urb_buffers *urb_bufs)
{
	unsigned int i;

	urb_bufs->urb_buf_num = URB_BUF_NUM;
	urb_bufs->urb_buf_len = HIFI_USB_SHARE_BUFFER_LEN / URB_BUF_NUM;

	if ((urb_bufs->urb_buf_num * urb_bufs->urb_buf_len)
				> HIFI_USB_SHARE_BUFFER_LEN) {
		urb_bufs->urb_buf_num = HIFI_USB_SHARE_BUFFER_LEN / urb_bufs->urb_buf_len;
	}

	if (urb_bufs->urb_buf_num > (sizeof(urb_bufs->urb_buf_bitmap) * 8)) {
		urb_bufs->urb_buf_num = sizeof(urb_bufs->urb_buf_bitmap) * 8;
	}

	pr_info("HIFI_USB_SHARE_BUFFER_ADDR 0x%lx\n", (long unsigned int)HIFI_USB_SHARE_BUFFER_ADDR);

	urb_bufs->urb_buf_dma[0] = 0; /* caution: this value is offset */ /*lint !e63 !e409 */
	urb_bufs->urb_buf[0] = ioremap(HIFI_USB_SHARE_BUFFER_ADDR, HIFI_USB_SHARE_BUFFER_LEN); /*lint !e569 !e598 !e648 */
	if (!urb_bufs->urb_buf[0])
		return -ENOMEM;

	for (i = 1; i < urb_bufs->urb_buf_num; i++) {
		urb_bufs->urb_buf_dma[i] = urb_bufs->urb_buf_dma[i - 1] + urb_bufs->urb_buf_len; /*lint !e63 !e409 */
		urb_bufs->urb_buf[i] = urb_bufs->urb_buf[i - 1] + urb_bufs->urb_buf_len;
	}

	urb_bufs->urb_buf_bitmap = 0;

	return 0;
}

void urb_buf_destroy(struct urb_buffers *urb_bufs)
{
	urb_bufs->urb_buf_bitmap = ~0UL;
	iounmap(urb_bufs->urb_buf[0]);
}

void urb_buf_reset(struct urb_buffers *urb_bufs)
{
	urb_bufs->urb_buf_bitmap = 0;
}
