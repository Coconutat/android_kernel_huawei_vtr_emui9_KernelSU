/*
 * hisi_ufs_ioctl.c - block layer implementation of the sg v4 interface
 *
 * Copyright (C) 2004 Jens Axboe <axboe@suse.de> SUSE Labs
 * Copyright (C) 2004 Peter M. Jones <pjones@redhat.com>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License version 2.  See the file "COPYING" in the main directory of this
 *  archive for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/blkdev.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/percpu.h>
#include <linux/uio.h>
#include <linux/idr.h>
#include <linux/bsg.h>
#include <linux/slab.h>

#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/sg.h>

struct blk_scsi_device {
	struct request_queue *queue;
	spinlock_t lock;
	struct list_head busy_list;
	struct list_head done_list;
	struct hlist_node dev_list;
	atomic_t ref_count;
	int queued_cmds;
	int done_cmds;
	wait_queue_head_t wq_done;
	wait_queue_head_t wq_free;
	char name[20];
	int max_queue;
	unsigned long flags;
};

static long hisi_blk_kern_copy_data(void *to, const void *from, unsigned long n)
{
	memcpy(to, from, n);
	return 0;
}
static int hisi_blk_kern_fill_sgv4_hdr_rq(struct request_queue *q,
					  struct request *rq,
					  struct sg_io_v4 *hdr)
{
	if (hdr->request_len > BLK_MAX_CDB) {
		rq->cmd = kzalloc((size_t)hdr->request_len, GFP_KERNEL);
		if (!rq->cmd)
			return -ENOMEM;
	}

	if (hisi_blk_kern_copy_data((void *)rq->cmd,
				    (void *)(unsigned long)hdr->request,
				    (unsigned long)hdr->request_len))
		return -EFAULT;

	/*
	 * fill in request structure
	 */
	rq->cmd_len = (unsigned short)hdr->request_len;

	rq->timeout = (unsigned int)msecs_to_jiffies(hdr->timeout);
	if (!rq->timeout)
		rq->timeout = q->sg_timeout;
	if (!rq->timeout)
		rq->timeout = BLK_DEFAULT_SG_TIMEOUT;
	if (rq->timeout < BLK_MIN_SG_TIMEOUT)
		rq->timeout = BLK_MIN_SG_TIMEOUT;

	return 0;
}

/*
 * Check if sg_io_v4 from kernel is allowed and valid
 */
static int hisi_blk_kern_validate_sgv4_hdr(struct sg_io_v4 *hdr,
					   int *rw)
{
	int ret = 0;

	if (hdr->guard != 'Q')
		return -EINVAL;

	switch (hdr->protocol) {
	case BSG_PROTOCOL_SCSI:
		switch (hdr->subprotocol) {
		case BSG_SUB_PROTOCOL_SCSI_CMD:
		case BSG_SUB_PROTOCOL_SCSI_TRANSPORT:
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}

	*rw = hdr->dout_xfer_len ? WRITE : READ;
	return ret;
}

/*
 * map sg_io_v4 to a request.
 */
static struct request *hisi_blk_kern_map_hdr(struct blk_scsi_device *bd,
					     struct sg_io_v4 *hdr,
					     u8 *sense)
{
	struct request_queue *q = bd->queue;
	struct request *rq = NULL, *next_rq = NULL;
	int ret, rw;
	unsigned int dxfer_len;
	void *dxferp = NULL;
	struct bsg_class_device *bcd = &q->bsg_dev;

	/* if the LLD has been removed then the bsg_unregister_queue will
	 * eventually be called and the class_dev was freed, so we can no
	 * longer use this request_queue. Return no such address.
	 */
	if (!bcd->class_dev)
		return ERR_PTR(-ENXIO);

	ret = hisi_blk_kern_validate_sgv4_hdr(hdr, &rw);
	if (ret)
		return ERR_PTR(ret);

	/*
	 * map scatter-gather elements separately and string them to request
	 */
	rq = blk_get_request(q, rw, GFP_KERNEL);
	if (IS_ERR(rq))
		return rq;
	blk_rq_set_block_pc(rq);

	ret = hisi_blk_kern_fill_sgv4_hdr_rq(q, rq, hdr);
	if (ret)
		goto out;

	if (rw == WRITE && hdr->din_xfer_len) {
		if (!test_bit(QUEUE_FLAG_BIDI, &q->queue_flags)) {
			ret = -EOPNOTSUPP;
			goto out;
		}

		next_rq = blk_get_request(q, READ, GFP_KERNEL);
		if (IS_ERR(next_rq)) {
			ret = PTR_ERR(next_rq);
			next_rq = NULL;
			goto out;
		}
		rq->next_rq = next_rq;
		next_rq->cmd_type = rq->cmd_type;

		dxferp = (void __user *)(unsigned long)hdr->din_xferp;
		ret = blk_rq_map_kern(q, next_rq, dxferp, hdr->din_xfer_len,
				      GFP_KERNEL);
		if (ret)
			goto out;
	}

	if (hdr->dout_xfer_len) {
		dxfer_len = hdr->dout_xfer_len;
		dxferp = (void *)(unsigned long)hdr->dout_xferp;
	} else if (hdr->din_xfer_len) {
		dxfer_len = hdr->din_xfer_len;
		dxferp = (void *)(unsigned long)hdr->din_xferp;
	} else
		dxfer_len = 0;

	if (dxfer_len) {
		ret = blk_rq_map_kern(q, rq, dxferp, dxfer_len, GFP_KERNEL);
		if (ret)
			goto out;
	}

	rq->sense = sense;
	rq->sense_len = 0;

	return rq;
out:
	if (rq->cmd != rq->__cmd)
		kfree(rq->cmd);
	blk_put_request(rq);
	if (next_rq) {
		blk_put_request(next_rq);
	}
	return ERR_PTR(ret);
}

static int hisi_blk_kern_complete_hdr_rq(struct request *rq,
					 struct sg_io_v4 *hdr)
{
	int ret = 0;

	/*
	 * fill in all the output members
	 */
	hdr->device_status = rq->errors & 0xff;
	hdr->transport_status = host_byte(rq->errors);
	hdr->driver_status = driver_byte(rq->errors);
	hdr->info = 0;
	if (hdr->device_status || hdr->transport_status || hdr->driver_status)
		hdr->info |= SG_INFO_CHECK;
	hdr->response_len = 0;

	if (rq->sense_len && hdr->response) {
		unsigned int len = min_t(unsigned int, hdr->max_response_len,
				rq->sense_len);

		ret = hisi_blk_kern_copy_data(
			(void *)(unsigned long)hdr->response, rq->sense, (unsigned long)len);
		if (!ret)
			hdr->response_len = len;
		else
			ret = -EFAULT;
	}

	if (rq->next_rq) {
		hdr->dout_resid = (__s32)rq->resid_len;
		hdr->din_resid = (__s32)rq->next_rq->resid_len;
		blk_put_request(rq->next_rq);
	} else if (rq_data_dir(rq) == READ)
		hdr->din_resid = (__s32)rq->resid_len;
	else
		hdr->dout_resid = (__s32)rq->resid_len;

	/*
	 * If the request generated a negative error number, return it
	 * (providing we aren't already returning an error); if it's
	 * just a protocol response (i.e. non negative), that gets
	 * processed above.
	 */
	if (!ret && rq->errors < 0)
		ret = rq->errors;

	if (rq->cmd != rq->__cmd)
		kfree(rq->cmd);
	blk_put_request(rq);

	return ret;
}

long blk_scsi_kern_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	int *uarg = (int *)arg;
	int ret;
	struct request *rq;
	struct sg_io_v4 hdr;
	int at_head;
	u8 sense[SCSI_SENSE_BUFFERSIZE];
	struct blk_scsi_device *bd;
	struct file *pfile = fget(fd);
	if (pfile == NULL)
		return -EFAULT;
	bd = (struct blk_scsi_device *)pfile->private_data;
	switch (cmd) {
	case SG_IO:
		if (hisi_blk_kern_copy_data(&hdr, uarg, sizeof(hdr)))
			return -EFAULT;

		rq = hisi_blk_kern_map_hdr(bd, &hdr, sense);

		if (IS_ERR(rq))
			return PTR_ERR(rq);

		at_head = (0 == (hdr.flags & BSG_FLAG_Q_AT_TAIL));
		blk_execute_rq(bd->queue, NULL, rq, at_head);
		ret = hisi_blk_kern_complete_hdr_rq(rq, &hdr);

		if (hisi_blk_kern_copy_data(uarg, &hdr, sizeof(hdr)))
			return -EFAULT;

		break;
	/*
	* block device ioctls
	 */
	default:
		ret = -ENOTTY;

	}
	fput(pfile);
	return ret;
}
