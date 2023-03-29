

#ifdef CONFIG_USB_MASS_STORAGE_SUPPORT_MAC
/* usbsdms_read_toc_data1 rsp packet */
static u8 usbsdms_read_toc_data1[] = {
	0x00, 0x0A, 0x01, 0x01,
	0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00
};

/* usbsdms_read_toc_data1_format0000 rsp packet */
static u8 usbsdms_read_toc_data1_format0000[] = {
	0x00, 0x12, 0x01, 0x01,
	0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* the last four bytes:32MB */
	0x00, 0x14, 0xAA, 0x00, 0x00, 0x00, 0xFF, 0xFF
};

/* usbsdms_read_toc_data1_format0001 rsp packet */
static u8 usbsdms_read_toc_data1_format0001[] = {
	0x00, 0x0A, 0x01, 0x01,
	0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* usbsdms_read_toc_data2 rsp packet */
static u8 usbsdms_read_toc_data2[] = {
	0x00, 0x2e, 0x01, 0x01,
	0x01, 0x14, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x01, 0x14, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x01, 0x14, 0x00, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x3c,
	/* ^ CDROM size from this byte */
	0x01, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00
};

/* usbsdms_read_toc_data3 rsp packet */
static u8 usbsdms_read_toc_data3[] = {
	0x00, 0x12, 0x01, 0x01,
	0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* READ_TOC command structure */
typedef struct _usbsdms_read_toc_cmd_type {
	u8  op_code;
	u8  msf;	/* bit1 is MSF,
			 * 0: address format is LBA form
			 * 1: address format is MSF form */

	u8  format;	/* bit3~bit0,   MSF Field   Track/Session Number
			 * 0000b:       Valid       Valid as a Track Number
			 * 0001b:       Valid       Ignored by Drive
			 * 0010b:       Ignored     Valid as a Session Number
			 * 0011b~0101b: Ignored     Ignored by Drive
			 * 0110b~1111b: Reserved
			 */

	u8  reserved1;
	u8  reserved2;
	u8  reserved3;
	u8  session_num;	/* a specific session or a track */
	u8  allocation_length_msb;
	u8  allocation_length_lsb;
	u8  control;
} usbsdms_read_toc_cmd_type;

/* ------------------------------------------------------------
 * function      : static int do_read_toc(struct fsg_dev *fsg, struct fsg_buffhd *bh)
 * description   : response for command READ TOC
 * input         : struct fsg_dev *fsg, struct fsg_buffhd *bh
 * output        : none
 * return        : response data length
 * -------------------------------------------------------------
 */
static int do_read_toc(struct fsg_common *common, struct fsg_buffhd *bh)
{
	u8    *buf = (u8 *) bh->buf;
	usbsdms_read_toc_cmd_type *read_toc_cmd = NULL;
	unsigned long response_length = 0;
	u8 *response_ptr = NULL;

	read_toc_cmd = (usbsdms_read_toc_cmd_type *)common->cmnd;

	/* When TIME is set to one, the address fields in some returned
	 * data formats shall be in TIME form.
	 * 2 is time form mask.
	 */
	if (2 == read_toc_cmd->msf) {
		response_ptr = usbsdms_read_toc_data2;
		response_length = sizeof(usbsdms_read_toc_data2);
	} else if (0 != read_toc_cmd->allocation_length_msb) {
		response_ptr = usbsdms_read_toc_data3;
		response_length = sizeof(usbsdms_read_toc_data3);
	} else {
		/* When TIME is set to zero, the address fields in some returned
		 * data formats shall be in LBA form.
		 */
		if (0 == read_toc_cmd->format) {
			/* 0 is mean to valid as a Track Number */
			response_ptr = usbsdms_read_toc_data1_format0000;
			response_length = sizeof(usbsdms_read_toc_data1_format0000);
		} else if (1 == read_toc_cmd->format) {
			/* 1 is mean to ignored by Logical Unit */
			response_ptr = usbsdms_read_toc_data1_format0001;
			response_length = sizeof(usbsdms_read_toc_data1_format0001);
		} else {
			/* Valid as a Session Number */
			response_ptr = usbsdms_read_toc_data1;
			response_length = sizeof(usbsdms_read_toc_data1);
		}
	}

	memcpy(buf, response_ptr, response_length);

	if (response_length < common->data_size_from_cmnd)
		common->data_size_from_cmnd = response_length;

	common->data_size = common->data_size_from_cmnd;

	common->residue = common->usb_amount_left = common->data_size;

	return response_length;
}
#endif

#ifdef CONFIG_HISI_USB_CONFIGFS
extern struct device *create_function_device(char *name);

static ssize_t mass_storage_inquiry_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct usb_function_instance *fi = dev_get_drvdata(dev);
	struct fsg_opts *opts = fsg_opts_from_func_inst(fi);

	return snprintf(buf, PAGE_SIZE, "%s\n", opts->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct usb_function_instance *fi = dev_get_drvdata(dev);
	struct fsg_opts *opts = fsg_opts_from_func_inst(fi);
	int len;

	len = min(size, sizeof(opts->common->inquiry_string) - 1);

	strncpy(opts->common->inquiry_string, buf, len);

	opts->common->inquiry_string[len] = 0;

	return size;
}

static DEVICE_ATTR(inquiry_string, (S_IRUGO | S_IWUSR),
					mass_storage_inquiry_show,
					mass_storage_inquiry_store);

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
	NULL
};

static int create_mass_storage_device(struct usb_function_instance *fi)
{
	struct device *dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;

	dev = create_function_device("f_mass_storage");
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	attrs = mass_storage_function_attributes;
	if (attrs) {
		while ((attr = *attrs++) && !err)
			err = device_create_file(dev, attr);
		if (err) {
			device_destroy(dev->class, dev->devt);
			return -EINVAL;
		}
	}
	dev_set_drvdata(dev, fi);
	return 0;
}
#endif
