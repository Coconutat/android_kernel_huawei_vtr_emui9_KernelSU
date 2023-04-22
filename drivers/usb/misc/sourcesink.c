#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/usb.h>

#ifndef D
#define D(format, arg...) pr_info("[sourcesink][%s]" format, __func__, ##arg)
#endif

struct sourcesink {
	struct usb_interface *intf;
	struct usb_device *udev;

	struct dentry *debugfs_root;

	int control_rw_result;
	struct mutex control_rw_lock;

	int isoc_test_result;
	struct mutex isoc_test_lock;
	struct urb *isoc_in_urb;
	struct urb *isoc_out_urb;
};

/* return 1 sucess, 0 failed */
static int parse_test_arg(char *buf, int buf_len, char *arg_name, int *arg_value)
{
	char *p;
	int val;

	p = strnstr(buf, arg_name, buf_len);
	if (!p)
		return 0;

#define MAX_PARAM_NAME_LEN 16
	p += strnlen(arg_name, MAX_PARAM_NAME_LEN);

	if (*p == '\0')
		return 0;

	if (sscanf(p, "%d", &val) != 1)
		return 0;

	*arg_value = val;

	return 1;
}

static int sourcesink_bulk_tx_show(struct seq_file *s, void *_unused)
{
	seq_printf(s, "Usage:\n"
		      "echo \"tx_len=XX,tx_pattern=XX\" > /d/sourcesink-0/bulk_xfer\n"
		      "echo \"rx_len=XX,rx_pattern=XX\" > /d/sourcesink-0/bulk_xfer\n");

	return 0;
}

static int sourcesink_bulk_tx_open(struct inode *inode, struct file *file)
{
	return single_open(file, sourcesink_bulk_tx_show, inode->i_private);
}

static void reinit_write_data(int pattern, char *buf, int len,
		int max_packet_size)
{
	int i;

	switch (pattern) {
	case 0:
		memset(buf, 0, len);
		break;
	case 1:
		for (i = 0; i < len; i++)
			*buf++ = (u8) ((i % max_packet_size) % 63);
		break;
	default:
		break;
	}
}

static int check_read_data(int pattern, char *buf, int actual_len,
		int max_packet_size)
{
	int		i;

	if (pattern == 2)
		return 0;

	for (i = 0; i < actual_len; i++, buf++) {
		switch (pattern) {

		/* all-zeroes has no synchronization issues */
		case 0:
			if (*buf == 0)
				continue;
			break;

		/* "mod63" stays in sync with short-terminated transfers,
		 * OR otherwise when host and gadget agree on how large
		 * each usb transfer request should be.  Resync is done
		 * with set_interface or set_config.  (We *WANT* it to
		 * get quickly out of sync if controllers or their drivers
		 * stutter for any reason, including buffer duplication...)
		 */
		case 1:
			if (*buf == (u8)((i % max_packet_size) % 63))
				continue;
			break;
		default:
			break;
		}
		D("bad OUT byte, buf[%d] = %d\n", i, *buf);
		return -EINVAL;
	}
	return 0;
}

static int set_bulk_alt(struct sourcesink *ss,
		struct usb_host_endpoint **out_ep,
		struct usb_host_endpoint **in_ep)
{
	struct usb_interface	*intf = ss->intf;
	struct usb_host_interface *altsetting = NULL;
	struct usb_host_endpoint	*bulk_out_ep = NULL;
	struct usb_host_endpoint	*bulk_in_ep = NULL;
	unsigned i;
	int ret;

	/* find the interface which contain bulk ep. */
	for (i = 0; i < intf->num_altsetting; i++) {
		if (intf->altsetting[i].desc.bNumEndpoints == 2) {
			altsetting = &intf->altsetting[i];
			break;
		}
	}

	if (!altsetting) {
		D("can't find bulk altsetting\n");
		return -ENODEV;
	}

	if (ss->intf->cur_altsetting != altsetting) {
		D("usb_set_interface: interface %d, altsetting %d\n",
				altsetting->desc.bInterfaceNumber,
				altsetting->desc.bAlternateSetting);
		ret = usb_set_interface(ss->udev, altsetting->desc.bInterfaceNumber,
				altsetting->desc.bAlternateSetting);
		if (ret < 0) {
			D("set interface failed\n");
			return ret;
		}
	}

	/* find isoc endpoint */
	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		if (usb_endpoint_is_bulk_out(&altsetting->endpoint[i].desc))
			bulk_out_ep = &altsetting->endpoint[i];
		if (usb_endpoint_is_bulk_in(&altsetting->endpoint[i].desc))
			bulk_in_ep = &altsetting->endpoint[i];

	}

	if (!bulk_out_ep || !bulk_in_ep) {
		D("can't find bulk ep\n");
		return -ENODEV;
	}

	*out_ep = bulk_out_ep;
	*in_ep = bulk_in_ep;

	return 0;
}

static int submit_bulk_test(struct usb_device *udev,
		struct usb_host_endpoint *ep, int xfer_len, int pattern)
{
	int ret, pipe, actual_len;
	char *buf;
	int is_out = usb_endpoint_dir_out(&ep->desc);
	int max_packet_size = le16_to_cpu(ep->desc.wMaxPacketSize);

	buf = kzalloc(xfer_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* set out data */
	if (is_out)
		reinit_write_data(pattern, buf, xfer_len, max_packet_size);

	if (is_out)
		pipe = usb_sndbulkpipe(udev, ep->desc.bEndpointAddress); /*lint !e648 */
	else
		pipe = usb_rcvbulkpipe(udev, ep->desc.bEndpointAddress); /*lint !e648 */

	ret = usb_bulk_msg(udev, pipe, buf, xfer_len, &actual_len, 5000);
	if (ret < 0) {
		D("usb_bulk_msg tx ret %d\n", ret);
		kfree(buf);
		return ret;
	}

	D("Sourcesink bulk test %s xfer %d\n",
			is_out ? "OUT" : "IN", actual_len);
	actual_len = actual_len > xfer_len ? xfer_len : actual_len;

	/* check in data */
	if (!is_out) {
		ret = check_read_data(pattern, buf, actual_len, max_packet_size);
		if (ret < 0) {
			D("usb_bulk_msg rx data error!\n");
		}
	}

	kfree(buf);
	return 0;
}

#define MAX_BULK_TX_LEN (4096)
#define MAX_BULK_RX_LEN (4096)
#define MAX_BULT_TEST_PATTERN (2)

static int parse_bulk_tx_len(char *cmd, int cmd_len)
{
	int ret, value, tx_len;

	ret = parse_test_arg(cmd, cmd_len, "tx_len=", &value);
	if ((ret) && (value > 0) && (value <= MAX_BULK_TX_LEN))
		tx_len = value;
	else
		tx_len = 0;

	return tx_len;
}
static int parse_bulk_tx_pattern(char *cmd, int cmd_len)
{
	int ret, value;
	int tx_pattern = 0;

	ret = parse_test_arg(cmd, cmd_len, "tx_pattern=", &value);
	if ((ret) && (value >= 0) && (value <= MAX_BULT_TEST_PATTERN))
		tx_pattern = value;
	else
		tx_pattern = 0;

	return tx_pattern;
}
static int parse_bulk_rx_len(char *cmd, int cmd_len)
{
	int ret, value, rx_len;

	ret = parse_test_arg(cmd, cmd_len, "rx_len=", &value);
	if ((ret) && (value > 0) && (value <= MAX_BULK_RX_LEN))
		rx_len = value;
	else
		rx_len = 0;

	return rx_len;
}
static int parse_bulk_rx_pattern(char *cmd, int cmd_len)
{
	int ret, value;
	int rx_pattern = 0;

	ret = parse_test_arg(cmd, cmd_len, "rx_pattern=", &value);
	if ((ret) && (value >= 0) && (value <= MAX_BULT_TEST_PATTERN))
		rx_pattern = value;
	else
		rx_pattern = 0;

	return rx_pattern;
}

static ssize_t sourcesink_bulk_tx_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct sourcesink	*ss = s->private;
	struct usb_host_endpoint	*bulk_out_ep = NULL;
	struct usb_host_endpoint	*bulk_in_ep = NULL;
	char cmd[32];
	int cmd_len;
	int ret, tx_len, rx_len;
	int tx_pattern = 0;
	int rx_pattern = 0;

	D("+\n");

	cmd_len = min_t(size_t, sizeof(cmd) - 1, count);
	if (copy_from_user(&cmd, ubuf, cmd_len))
		return -EFAULT;

	cmd[cmd_len] = 0;

	D("cmd %s\n", cmd);

	tx_len = parse_bulk_tx_len(cmd, cmd_len);
	if (tx_len)
		tx_pattern = parse_bulk_tx_pattern(cmd, cmd_len);

	rx_len = parse_bulk_rx_len(cmd, cmd_len);

	if (rx_len)
		rx_pattern = parse_bulk_rx_pattern(cmd, cmd_len);

	if (!tx_len && !rx_len) {
		D("not invalid argument!\n");
		return -EINVAL;
	}

	ret = set_bulk_alt(ss, &bulk_out_ep, &bulk_in_ep);
	if (ret < 0) {
		D("get bulk ep failed %d\n", ret);
		return ret;
	}

	if (tx_len) {
		ret = submit_bulk_test(ss->udev, bulk_out_ep, tx_len, tx_pattern);
		if (ret)
			D("bulk out test failed %d\n", ret);
	}

	if (rx_len) {
		ret = submit_bulk_test(ss->udev, bulk_in_ep, rx_len, rx_pattern);
		if (ret)
			D("bulk in test failed %d\n", ret);
	}
	D("-\n");

	return count;
}

static const struct file_operations sourcesink_bulk_tx_fops = {
	.open			= sourcesink_bulk_tx_open,
	.write			= sourcesink_bulk_tx_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

struct isoc_test_case {
	int pipe;

	__u16 urb_num;
	__u16 pkt_num;
	__u16 pkt_len;
};

static int sourcesink_isoc_show(struct seq_file *s, void *_unused)
{
	struct sourcesink	*ss = s->private;
	mutex_lock(&ss->isoc_test_lock);
	seq_printf(s, "%d\n", ss->isoc_test_result);
	mutex_unlock(&ss->isoc_test_lock);
	return 0;
}

static int sourcesink_isoc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sourcesink_isoc_show, inode->i_private);
}

static void sourcesink_isoc_irq(struct urb *urb)
{
	struct sourcesink *ss = urb->context;
	D("[%s] sourcesink_isoc_irq called, actual_length %d\n",
		dev_name(&ss->intf->dev), urb->actual_length);
	usb_put_urb(urb);
}

static int set_isoc_alt(struct sourcesink *ss)
{
	struct usb_interface *intf = ss->intf;
	struct usb_host_interface *altsetting = NULL;
	struct usb_host_endpoint *isoc_in_ep = NULL;
	struct usb_host_endpoint *isoc_out_ep = NULL;
	unsigned i, j;
	int ret;

	/* find the altsetting which contain isoc ep. */
	for (i = 0; i < intf->num_altsetting; i++) {
		altsetting = &intf->altsetting[i];
		for (j = 0; j < altsetting->desc.bNumEndpoints; j++) {
			if (usb_endpoint_is_isoc_in(&altsetting->endpoint[j].desc))
				isoc_in_ep = &altsetting->endpoint[j];
			if (usb_endpoint_is_isoc_out(&altsetting->endpoint[j].desc))
				isoc_out_ep = &altsetting->endpoint[j];
		}
		if (isoc_in_ep || isoc_out_ep)
			break;
	}

	if ((!isoc_in_ep) && (!isoc_out_ep)) {
		D("can't find isoc altsetting\n");
		return -1;
	}

	D("usb_set_interface: interface %d, altsetting %d\n", /*[false alarm]:it is a false alarm*/
			altsetting->desc.bInterfaceNumber,
			altsetting->desc.bAlternateSetting);
	ret = usb_set_interface(ss->udev, altsetting->desc.bInterfaceNumber,
			altsetting->desc.bAlternateSetting);
	if (ret) {
		D("set interface failed\n");
		return ret;
	}

	return 0;
}

static int submit_isoc_test(struct sourcesink *ss, struct isoc_test_case *test, bool is_in)
{
	struct usb_interface *intf = ss->intf;
	struct usb_host_interface *altsetting = intf->cur_altsetting;
	struct usb_host_endpoint *isoc_in_ep = NULL;
	struct usb_host_endpoint *isoc_out_ep = NULL;
	struct urb *isoc_urb;
	int i;
	int ret;

	D("pkt_num %d, pkt_len %d, urb_num %d\n", test->pkt_num, test->pkt_len, test->urb_num);
	D("cur_altsetting %d\n", intf->cur_altsetting->desc.bAlternateSetting);

	/* find isoc endpoint */
	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		if (usb_endpoint_is_isoc_in(&altsetting->endpoint[i].desc))
			isoc_in_ep = &altsetting->endpoint[i];
		if (usb_endpoint_is_isoc_out(&altsetting->endpoint[i].desc))
			isoc_out_ep = &altsetting->endpoint[i];

	}

	if ((!isoc_in_ep && is_in) || (!isoc_out_ep && !is_in)) {
		D("no isoc ep\n");
		return -ENOENT;
	}

	if (is_in) {
		D("isoc_in_ep 0x%02x\n", /*[false alarm]:it is a false alarm*/
				isoc_in_ep->desc.bEndpointAddress);/*lint !e613*/
		test->pipe = usb_rcvisocpipe(ss->udev,
				isoc_in_ep->desc.bEndpointAddress);/*lint !e613*/
	} else {
		D("isoc_out_ep 0x%02x\n", /*[false alarm]:it is a false alarm*/
				isoc_out_ep->desc.bEndpointAddress);/*lint !e613*/
		test->pipe = usb_sndisocpipe(ss->udev,
				isoc_out_ep->desc.bEndpointAddress);/*lint !e613*/
	}

	isoc_urb = usb_alloc_urb(1, GFP_KERNEL);
	if (!isoc_urb) {
		D("alloc urb failed\n");
		return -ENOMEM;
	}

	/* borrow the usb_fill_int_urb */
	usb_fill_int_urb(isoc_urb, ss->udev, test->pipe, test, sizeof(*test),
			sourcesink_isoc_irq, ss,
			is_in ? isoc_in_ep->desc.bInterval : isoc_out_ep->desc.bInterval);/*lint !e613*/ /*[false alarm]:it is a false alarm*/
	isoc_urb->number_of_packets = 1;
	isoc_urb->iso_frame_desc[0].length = sizeof(*test);

	ret = usb_submit_urb(isoc_urb, GFP_KERNEL);
	D("submit urb ret %d\n", ret);
	if (ret)
		usb_put_urb(isoc_urb);
	else {
		/* should wait for urb complete!!! */
		/* TODO: wait for urb complete. */
	}

	return 0;
}

#define MAX_TX_PKT_NUM 200
#define MAX_TX_PKT_LEN (1024 *3)
#define MAX_TX_URB_NUM 65535
#define MAX_RX_PKT_NUM 200
#define MAX_RX_PKT_LEN (1024 *3)
#define MAX_RX_URB_NUM 65535

static void parse_isoc_rx_test_args(struct isoc_test_case *rx_test,
					char *buf, int buf_len)
{
	int value;
	int ret;

	ret = parse_test_arg(buf, buf_len, "rx_pkt_num=", &value);
	if ((ret) && (value > 0) && (value < MAX_RX_PKT_NUM))
		rx_test->pkt_num = value;

	ret = parse_test_arg(buf, buf_len, "rx_pkt_len=", &value);
	if ((ret) && (value > 0) && (value <= MAX_TX_PKT_LEN))
		rx_test->pkt_len = value;

	ret = parse_test_arg(buf, buf_len, "rx_urb_num=", &value);
	if ((ret) && (value > 0) && (value <= MAX_TX_URB_NUM))
		rx_test->urb_num = value;
}

static void parse_isoc_tx_test_args(struct isoc_test_case *tx_test,
					char *buf, int buf_len)
{
	int value;
	int ret;

	ret = parse_test_arg(buf, buf_len, "tx_pkt_num=", &value);
	if ((ret) && (value > 0) && (value < MAX_TX_PKT_NUM))
		tx_test->pkt_num = value;

	ret = parse_test_arg(buf, buf_len, "tx_pkt_len=", &value);
	if ((ret) && (value > 0) && (value <= MAX_RX_PKT_LEN))
		tx_test->pkt_len = value;

	ret = parse_test_arg(buf, buf_len, "tx_urb_num=", &value);
	if ((ret) && (value > 0) && (value <= MAX_TX_URB_NUM))
		tx_test->urb_num = value;
}

static void parse_isoc_test_args(struct isoc_test_case *tx_test,
		struct isoc_test_case *rx_test, char *buf, int buf_len)
{

	tx_test->pkt_len = 0;
	tx_test->pkt_num = 0;
	tx_test->urb_num = MAX_TX_URB_NUM;

	rx_test->pkt_len = 0;
	rx_test->pkt_num = 0;
	rx_test->urb_num = MAX_TX_URB_NUM;

	D("buf %s\n", buf);
	parse_isoc_rx_test_args(rx_test, buf, buf_len);
	parse_isoc_tx_test_args(tx_test, buf, buf_len);
}


static inline struct usb_host_endpoint *find_isoc_ep(struct usb_interface *intf, bool is_in)
{
	struct usb_host_endpoint *isoc_in_ep = NULL;
	struct usb_host_endpoint *isoc_out_ep = NULL;
	unsigned i;

	/* find isoc endpoint */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		if (usb_endpoint_is_isoc_in(&intf->cur_altsetting->endpoint[i].desc))
			isoc_in_ep = &intf->cur_altsetting->endpoint[i];
		if (usb_endpoint_is_isoc_out(&intf->cur_altsetting->endpoint[i].desc))
			isoc_out_ep = &intf->cur_altsetting->endpoint[i];

	}

	if (is_in)
		return isoc_in_ep;
	else
		return isoc_out_ep;
}

static unsigned long calc_interval(struct sourcesink *ss, bool is_in)
{
	struct usb_interface *intf = ss->intf;
	struct usb_host_endpoint *isoc_ep = NULL;
	unsigned long interval;

	/* find isoc endpoint */
	isoc_ep = find_isoc_ep(intf, is_in);
	if (!isoc_ep) {
		D("can't find %s isoc ep\n", is_in ? "in" : "out");
		return 0;
	}

	interval = 1UL << (isoc_ep->desc.bInterval - 1);/*lint !e613*/

	if (ss->udev->speed == USB_SPEED_FULL)
		interval *= 1000;
	else if (ss->udev->speed == USB_SPEED_HIGH)
		interval *= 125;

	return interval;
}

static unsigned int calc_mps(struct sourcesink *ss, bool is_in)
{
	struct usb_interface *intf = ss->intf;
	struct usb_host_endpoint *isoc_ep = NULL;

	/* find isoc endpoint */
	isoc_ep = find_isoc_ep(intf, is_in);
	if (!isoc_ep) {
		D("can't find %s isoc ep\n", is_in ? "in" : "out");
		return 0;
	}


	return le16_to_cpu(isoc_ep->desc.wMaxPacketSize);/*lint !e613*/
}

static void excute_isoc_test(struct sourcesink *ss,
		struct isoc_test_case *tx_test,	struct isoc_test_case *rx_test)
{
	int ret;
	unsigned long interval;
	unsigned delay_ms;
	unsigned tx_delay_ms = 0;
	unsigned rx_delay_ms = 0;

	if (rx_test->pkt_num != 0) {
		ret = submit_isoc_test(ss, rx_test, true);
		if (ret < 0) {
			ss->isoc_test_result = ret;
		}

		interval = calc_interval(ss, true);
		if (!interval) {
			ss->isoc_test_result = -EINVAL;
			return;
		}
		rx_delay_ms = (interval * rx_test->pkt_num * rx_test->urb_num)/1000;
	}

	if (tx_test->pkt_num != 0) {
		ret = submit_isoc_test(ss, tx_test, false);
		if (ret < 0) {
			ss->isoc_test_result = ret;
		}
		interval = calc_interval(ss, false);
		if (!interval) {
			ss->isoc_test_result = -EINVAL;
			return;
		}
		tx_delay_ms = (interval * tx_test->pkt_num * tx_test->urb_num)/1000;
	}

	delay_ms = (tx_delay_ms > rx_delay_ms) ? tx_delay_ms : rx_delay_ms;
	D("Wait %d ms for transfer complete.\n", delay_ms);
	msleep(delay_ms + 1000); /* wait more 1000ms */
}

struct isoc_test_case isoc_tcs[] = {
	/* pipe, urb_num, pkt_num, pkt_len */
	{0, 10000, 1, 64},
	{0, 10000, 1, 65},
	{0, 10000, 1, 512},
	{0, 10000, 1, 1023},
	{0, 10000, 1, 1024},

	{0, 2000, 5, 64},
	{0, 2000, 5, 65},
	{0, 2000, 5, 512},
	{0, 2000, 5, 1023},
	{0, 2000, 5, 1024},

	{0, 1000, 20, 64},
	{0, 1000, 20, 65},
	{0, 1000, 20, 512},
	{0, 1000, 20, 1023},
	{0, 1000, 20, 1024},

	{0, 0, 0, 0},
};

static void __auto_isoc_test(struct sourcesink *ss, bool is_in)
{
	int ret;
	unsigned long interval;
	unsigned int mps;
	unsigned delay_ms;
	int i;

	ret = set_isoc_alt(ss);
	if (ret) {
		ss->isoc_test_result = ret;
		return;
	}

	interval = calc_interval(ss, is_in);
	if (!interval) {
		ss->isoc_test_result = -EINVAL;
		return;
	}

	mps = calc_mps(ss, is_in);
	if (!mps) {
		ss->isoc_test_result = -EINVAL;
		return;
	}

	/* do transfer */
	for (i = 0; isoc_tcs[i].pkt_len != 0; i++) {
		struct isoc_test_case *isoc_tc = &isoc_tcs[i];

		if ((ss->udev->speed == USB_SPEED_FULL)
				&& (isoc_tc->pkt_len >= 1024))
			continue;

		/* receive buffer must no less than mps,
		 * transmit buffer must no more than mps. */
		if (is_in) {
			if (isoc_tc->pkt_len < mps)
				continue;
		} else {
			if (isoc_tc->pkt_len > mps)
				continue;
		}

		ret = submit_isoc_test(ss, isoc_tc, is_in);
		if (ret < 0) {
			ss->isoc_test_result = ret;
			return;
		}

		D("interval %ld ms, mps %d\n", interval/1000, mps);
		delay_ms = (interval * isoc_tc->pkt_num * isoc_tc->urb_num)/1000;
		D("Wait %d ms for transfer complete.\n", delay_ms);
		msleep(delay_ms + 1000); /* wait more 1000ms */
	}
}

static int auto_isoc_test(struct sourcesink *ss, char *buf, int buf_len)
{
	int value;
	int ret;
	int i;
	bool is_auto = false;

	ret = parse_test_arg(buf, buf_len, "auto_rx=", &value);
	if ((ret) && (value > 0)) {
		for (i = 0; i < value; i++) {
			__auto_isoc_test(ss, true);
		}
		is_auto = true;
	}

	ret = parse_test_arg(buf, buf_len, "auto_tx=", &value);
	if ((ret) && (value > 0)) {
		for (i = 0; i < value; i++) {
			__auto_isoc_test(ss, false);
		}
		is_auto = true;
	}

	if (is_auto)
		return 0;

	return -ENOTSUPP;
}

static ssize_t sourcesink_isoc_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sourcesink *ss = s->private;
	struct isoc_test_case tx_test, rx_test;
	char buf[64] = {0};
	int buf_len;
	int ret;


	D("+\n");

	buf_len = min_t(size_t, sizeof(buf) - 1, count);
	if (copy_from_user(&buf, ubuf, buf_len))
		return -EFAULT;

	mutex_lock(&ss->isoc_test_lock);

	ss->isoc_test_result = 0;

	/* try auto test */
	ret = auto_isoc_test(ss, buf, buf_len);
	if (ret != -ENOTSUPP)
		goto done;

	/* get the tx test case and rx test case */
	parse_isoc_test_args(&tx_test, &rx_test, buf, buf_len);

	/* set interface altsetting, to support isoc xfer */
	if ((rx_test.pkt_num != 0) || (tx_test.pkt_num != 0)) {
		ret = set_isoc_alt(ss);
		if (ret < 0) {
			ss->isoc_test_result = ret;
			goto done;
		}
	}

	excute_isoc_test(ss, &tx_test, &rx_test);

done:
	mutex_unlock(&ss->isoc_test_lock);
	D("-\n");

	return count;
}

static const struct file_operations sourcesink_isoc_fops = {
	.open			= sourcesink_isoc_open,
	.write			= sourcesink_isoc_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

#define SOURCESINK_CONTROL_WRITE_REQ 0x5b
#define SOURCESINK_CONTROL_READ_REQ 0x5c
#define SOURCESINK_CONTROL_TIMEOUT 5000
#define SOURCESINK_CONTROL_RW_REPEAT 5000

static int control_rw(struct sourcesink *ss, void *data, int size, bool r)
{
	struct usb_device *usb_dev = interface_to_usbdev(ss->intf);
	__u8 requesttype, request;
	int result;

	if ((size <= 0) || (size > 1024))
		return -EINVAL;

	if (r) {
		request = SOURCESINK_CONTROL_READ_REQ;
		requesttype = USB_DIR_IN | USB_TYPE_VENDOR;
	} else {
		request = SOURCESINK_CONTROL_WRITE_REQ;
		requesttype = USB_DIR_OUT | USB_TYPE_VENDOR;
	}

	result = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0), /*lint !e648 */
			request,
			requesttype,
			0,
			ss->intf->cur_altsetting->desc.bInterfaceNumber,
			data, size, SOURCESINK_CONTROL_TIMEOUT);
	if (result < 0) {
		dev_err(&ss->intf->dev, "%s: USB ctrl %s error: %d\n",
				__func__, (r ? "r" : "w"), result);
		return result;
	}

	return result;
}

static int control_rw_test(struct sourcesink *ss, int size, bool r)
{
	char *buf;
	unsigned repeat = SOURCESINK_CONTROL_RW_REPEAT;
	unsigned i;
	int actual;
	int ret = 0;

	D("+\n");
	D("control %s %d test.\n", r ? "read" : "write", size);

	buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		D("no mem\n");
		return -ENOMEM;
	}

	if (!r)
		memset(buf, SOURCESINK_CONTROL_WRITE_REQ, size);

	for (i = 0; i < repeat; i++) {
		actual = control_rw(ss, buf, size, r);
		if (actual != size) {
			D("test %d: size %d, actual %d\n", i, size, actual);
			ret = -EPIPE;
			break;
		}
	}

	kfree(buf);
	D("-\n");
	return ret;
}

static int sourcesink_control_rw_show(struct seq_file *s, void *_unused)
{
	struct sourcesink	*ss = s->private;
	mutex_lock(&ss->control_rw_lock);
	seq_printf(s, "%d\n", ss->control_rw_result);
	mutex_unlock(&ss->control_rw_lock);
	return 0;
}

static int sourcesink_control_rw_open(struct inode *inode, struct file *file)
{
	return single_open(file, sourcesink_control_rw_show, inode->i_private);
}

static ssize_t sourcesink_control_rw_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos, bool is_r)
{
	struct seq_file		*s = file->private_data;
	struct sourcesink	*ss = s->private;
	int 			mps = ss->udev->ep0.desc.wMaxPacketSize;
	char			buf[32] = {0};
	int 			len;

	D("+\n");

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	mutex_lock(&ss->control_rw_lock);

	ss->control_rw_result = 0;

	if (sscanf(buf, "%d", &len) != 1) {
		D("parse len error!\n");
		ss->control_rw_result = -EINVAL;
		goto err;
	}
	D("len %d\n", len);
	if ((len < 0) || (len > 1024)) {
		D("ilegal len %d!\n", len);
		ss->control_rw_result = -EINVAL;
		goto err;
	}

	if ((mps <= 0) || (mps > 64)) {
		D("ilegal mps %d!\n", mps);
		ss->control_rw_result = -EINVAL;
		goto err;
	}

	if (len != 0)
		ss->control_rw_result = control_rw_test(ss, len, is_r);
	else {
		ss->control_rw_result = control_rw_test(ss, 1, is_r);
		if (ss->control_rw_result != 0)
			goto err;

		ss->control_rw_result = control_rw_test(ss, mps, is_r);
		if (ss->control_rw_result != 0)
			goto err;

		ss->control_rw_result = control_rw_test(ss, mps + 1, is_r);
		if (ss->control_rw_result != 0)
			goto err;

		ss->control_rw_result = control_rw_test(ss, 1024, is_r);
	}

	D("-\n");

err:
	mutex_unlock(&ss->control_rw_lock);
	return count;
}

static ssize_t sourcesink_control_r_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	D(":\n");
	return sourcesink_control_rw_write(file, ubuf, count, ppos, true);
}

static ssize_t sourcesink_control_w_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	D(":\n");
	return sourcesink_control_rw_write(file, ubuf, count, ppos, false);
}

static const struct file_operations sourcesink_control_read_fops = {
	.open			= sourcesink_control_rw_open,
	.write			= sourcesink_control_r_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static const struct file_operations sourcesink_control_write_fops = {
	.open			= sourcesink_control_rw_open,
	.write			= sourcesink_control_w_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int sourcesink_debugfs_init(struct sourcesink *ss)
{
	struct dentry		*root;
	struct dentry		*file;
	int			ret;
	char			name[32] = {0};

	D("+\n");

	/* Create dir for INTERFACE */
	snprintf(name, sizeof(name) - 1, "sourcesink-%d",
			ss->intf->cur_altsetting->desc.bInterfaceNumber);
	D("name %s\n", name);

	root = debugfs_create_dir(name, NULL);
	if (!root) {
		D("debugfs_create_dir failed\n");
		ret = -ENOMEM;
		goto err0;
	}

	D("create debugfs dir %s sucess.\n", name);
	ss->debugfs_root = root;

	/*
	 * For Control transfer
	 */
	mutex_init(&ss->control_rw_lock);

	/* Control read */
	file = debugfs_create_file("control_read", S_IRUGO | S_IWUSR, root,
			ss, &sourcesink_control_read_fops);
	if (!file) {
		D("create control_read failed\n");
		ret = -ENOMEM;
		goto err1;
	}

	/* Control write */
	file = debugfs_create_file("control_write", S_IRUGO | S_IWUSR, root,
			ss, &sourcesink_control_write_fops);
	if (!file) {
		D("create control_write failed\n");
		ret = -ENOMEM;
		goto err1;
	}

	/*
	 * For isoc transfer
	 */
	mutex_init(&ss->isoc_test_lock);

	file = debugfs_create_file("isoc_xfer", S_IRUGO | S_IWUSR, root,
			ss, &sourcesink_isoc_fops);
	if (!file) {
		D("create isoc_xfer failed\n");
		ret = -ENOMEM;
		goto err1;
	}

	/*
	 * For bulk transfer
	 */
	file = debugfs_create_file("bulk_xfer", S_IRUGO | S_IWUSR, root,
			ss, &sourcesink_bulk_tx_fops);
	if (!file) {
		D("create bulk_xfer failed\n");
		ret = -ENOMEM;
		goto err1;
	}

	D("-\n");
	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void sourcesink_debugfs_exit(struct sourcesink *ss)
{
	D("+\n");
	debugfs_remove_recursive(ss->debugfs_root);
	ss->debugfs_root = NULL;
	D("-\n");
}

static int usb_sourcesink_probe(struct usb_interface *intf,
			   const struct usb_device_id *usb_id)
{
	struct sourcesink *ss;
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *cur_alt;
	int ret;

	D("+\n");

	cur_alt = intf->cur_altsetting;

	D("bInterfaceNumber: %d\n", cur_alt->desc.bInterfaceNumber);
	D("bNumEndpoints: %d\n", cur_alt->desc.bNumEndpoints);
	D("num_altsetting: %d\n", intf->num_altsetting);

	ss = kzalloc(sizeof(*ss), GFP_KERNEL);
	if (!ss) {
		D("alloc ss failed!\n");
		return -ENOMEM;
	}

	ss->intf = intf;
	ss->udev = udev;
	usb_set_intfdata(intf, ss);

	ret = sourcesink_debugfs_init(ss);
	if (ret) {
		D("sourcesink_debugfs_init failed\n");
		usb_set_intfdata(intf, NULL);
		kfree(ss);
		return ret;
	}

	D("-\n");

	return 0;
}

static void usb_sourcesink_disconnect(struct usb_interface *intf)
{
	struct sourcesink *ss;

	D("+\n");

	ss = usb_get_intfdata(intf);
	if (!ss)
		return;

	sourcesink_debugfs_exit(ss);

	usb_set_intfdata(intf, NULL);
	kfree(ss);

	D("-\n");
}

#ifdef CONFIG_PM
static int usb_sourcesink_suspend(struct usb_interface *intf, pm_message_t message)
{
	D("+\n");
	D("-\n");
	return 0;
}

static int usb_sourcesink_resume(struct usb_interface *intf)
{
	D("+\n");
	D("-\n");
	return 0;
}

static int usb_sourcesink_reset_resume(struct usb_interface *intf)
{
	D("+\n");
	D("-\n");
	return 0;
}
#else
#define usb_sourcesink_suspend	NULL
#define usb_sourcesink_resume	NULL
#define usb_sourcesink_reset_resume	NULL
#endif /* CONFIG_PM */


static struct usb_device_id usb_sourcesink_ids [] = {
	{ .match_flags = (USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT
		| USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS),
	.idVendor = 0x12d1,
	.idProduct = 0xfffe, /* To avoid mismatch other functions, using a strange pid */
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 0 },
	{ }	/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, usb_sourcesink_ids);

/*
 * entry point for linux usb interface
 */

static struct usb_driver usb_sourcesink_driver = {
	.name =		"sourcesink",
	.probe =	usb_sourcesink_probe,
	.disconnect =	usb_sourcesink_disconnect,
	.suspend =	usb_sourcesink_suspend,
	.resume =	usb_sourcesink_resume,
	.reset_resume =	usb_sourcesink_reset_resume,
	.id_table =	usb_sourcesink_ids,
	.supports_autosuspend = 1,
};

/*lint -save -e64 */
module_usb_driver(usb_sourcesink_driver);
/*lint -restore */
