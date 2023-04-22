/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>


/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);/*lint !e651 !e708 !e570 !e64 !e785 */

static unsigned bufsiz = 256*1024;
module_param(bufsiz, uint, S_IRUGO);
/*lint -e753 -esym(753,*) */
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");
/*lint -e753 +esym(753,*) */
/*-------------------------------------------------------------------------*/

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	struct spi_device *spi;

	spin_lock_irq(&spidev->spi_lock);
	spi = spidev->spi;
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
			status = (int)message->actual_length;

	return status;
}

static inline ssize_t
spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->tx_buffer,
			.len		= (unsigned)len,
			.speed_hz	= spidev->speed_hz,
		};/*lint !e785 */
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->rx_buffer,
			.len		= (unsigned)len,
			.speed_hz	= spidev->speed_hz,
		};/*lint !e785 */
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	status = spidev_sync_read(spidev, count);/*lint !e838 */
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, spidev->rx_buffer, (unsigned long)status);
		if (missing == (unsigned long)status)
			status = -EFAULT;
		else
			status = (unsigned long)status - missing;/*lint !e713 */
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}/*lint !e715 */

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;
	unsigned long		missing;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->tx_buffer, buf, count);
	if (missing == 0)
		status = spidev_sync_write(spidev, count);
	else
		status = -EFAULT;
	mutex_unlock(&spidev->buf_lock);

	return status;
}/*lint !e715 */

static int spidev_message(struct spidev_data *spidev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total, tx_total, rx_total;
	u8			*tx_buf, *rx_buf;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc((size_t)n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	tx_buf = spidev->tx_buffer;
	rx_buf = spidev->rx_buffer;
	total = 0;
	tx_total = 0;
	rx_total = 0;
	dev_dbg(&spidev->spi->dev,
		"spidev_message: n_xfers: %d\n", n_xfers);/*lint !e774 */
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		/* Since the function returns the total length of transfers
		 * on success, restrict the total to positive int values to
		 * avoid the return value looking like an error.  Also check
		 * each transfer length to avoid arithmetic overflow.
		 */
		if (total > INT_MAX || k_tmp->len > INT_MAX) {
			status = -EMSGSIZE;
			dev_dbg(&spidev->spi->dev,
				"spidev_message: total is more: %d\n", total);/*lint !e774 */
			goto done;
		}
		dev_dbg(&spidev->spi->dev,
			"spidev_message: rx_buf: %08x, tx_buf: %08x\n",
			(u32)u_tmp->rx_buf, (u32)u_tmp->tx_buf);/*lint !e774 */

		if (u_tmp->rx_buf) {
			/* this transfer needs space in RX bounce buffer */
			rx_total += k_tmp->len;
			if (rx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->rx_buf = rx_buf;
			if (!access_ok(VERIFY_WRITE, (u8 __user *)
						(uintptr_t) u_tmp->rx_buf,
						u_tmp->len)) {/*lint !e530 !e529 */
				dev_dbg(&spidev->spi->dev,
					"spidev_message: access_ok error\n");/*lint !e774 */
				goto done;
			}
			rx_buf += k_tmp->len;
		}
		if (u_tmp->tx_buf) {
			/* this transfer needs space in TX bounce buffer */
			tx_total += k_tmp->len;
			if (tx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->tx_buf = tx_buf;
			if (copy_from_user(tx_buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					(unsigned long)u_tmp->len)) {
				dev_dbg(&spidev->spi->dev,
					"spidev_message: copy_from_user error\n");/*lint !e774 */
				goto done;
			}
			tx_buf += k_tmp->len;
		}

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->tx_nbits = u_tmp->tx_nbits;
		k_tmp->rx_nbits = u_tmp->rx_nbits;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
		if (!k_tmp->speed_hz)
			k_tmp->speed_hz = spidev->speed_hz;
#ifdef VERBOSE
		dev_dbg(&spidev->spi->dev,
			"  xfer len %u %s%s%s%dbits %u usec %uHz n:%d\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : spidev->spi->max_speed_hz,
			n);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = (int)spidev_sync(spidev, &msg);
	if (status < 0) {
		dev_dbg(&spidev->spi->dev, "spidev_sync error with status: %d\n", status);/*lint !e774 */
		goto done;
	}

	/* copy any rx data out of bounce buffer */
	rx_buf = spidev->rx_buffer;
	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, rx_buf,
					(unsigned long)u_tmp->len)) {
				dev_dbg(&spidev->spi->dev, "__copy_to_user error\n");/*lint !e774 */
				status = -EFAULT;
				goto done;
			}
			rx_buf += u_tmp->len;
		}
	}
	status = (int)total;

done:
	kfree(k_xfers);
	return status;
}

static struct spi_ioc_transfer *
spidev_get_ioc_message(unsigned int cmd, struct spi_ioc_transfer __user *u_ioc,
		unsigned *n_ioc)
{
	struct spi_ioc_transfer	*ioc;
	u32	tmp;

	/* Check type, command number and direction */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC
			|| _IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))/*lint !e84 */
			|| _IOC_DIR(cmd) != _IOC_WRITE)/*lint !e845 */
		return ERR_PTR((long)-ENOTTY);

	tmp = _IOC_SIZE(cmd);
	if ((tmp % sizeof(struct spi_ioc_transfer)) != 0)
		return ERR_PTR((long)-EINVAL);
	*n_ioc = tmp / sizeof(struct spi_ioc_transfer);
	if (*n_ioc == 0)
		return NULL;

	/* copy into scratch area */
	ioc = kmalloc((size_t)tmp, GFP_KERNEL);
	if (!ioc)
		return ERR_PTR((long)-ENOMEM);
	if (__copy_from_user(ioc, u_ioc, tmp)) {/*lint !e747 */
		kfree(ioc);
		return ERR_PTR((long)-EFAULT);
	}
	return ioc;
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));/*lint !e530 !e529 */
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));/*lint !e530 !e529 */
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:/*lint !e30 !e142 */
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);/*lint !e1058 !e1514 !e866 !e1564  !e774 !e529 !e734 */
		break;
	case SPI_IOC_RD_MODE32:/*lint !e30 !e142 */
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u32 __user *)arg);/*lint !e1058 !e1514 !e866 !e1564  !e774 !e529 */
		break;
	case SPI_IOC_RD_LSB_FIRST:/*lint !e30 !e142 */
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);/*lint !e1058 !e1514 !e866 !e1564  !e774 !e529 */
		break;
	case SPI_IOC_RD_BITS_PER_WORD:/*lint !e30 !e142 */
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);/*lint !e1058 !e1514 !e866 !e1564  !e774 !e529 */
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:/*lint !e30 !e142 */
		retval = __put_user(spidev->speed_hz, (__u32 __user *)arg);/*lint !e1058 !e1514 !e866 !e1564  !e774 !e529 */
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:/*lint !e30 !e142 */
	case SPI_IOC_WR_MODE32:/*lint !e30 !e142 */
		if (cmd == SPI_IOC_WR_MODE)
			retval = __get_user(tmp, (u8 __user *)arg);/*lint !e866 !e1564 !e774 !e50 !e530 */
		else
			retval = __get_user(tmp, (u32 __user *)arg);/*lint !e866 !e1564 !e774 !e50 !e530 */
		if (retval == 0) {/*lint !e774 */
			u32	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;/*lint !e734 */
			retval = spi_setup(spi);
			if (retval < 0)/*lint !e774 */
				spi->mode = save;/*lint !e734 */
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);/*lint !e774 */
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:/*lint !e30 !e142 */
		retval = __get_user(tmp, (__u8 __user *)arg);/*lint !e866 !e1564 !e774 !e50 !e530 */
		if (retval == 0) {/*lint !e774 */
			u32	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;/*lint !e734 */
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');/*lint !e774 */
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:/*lint !e30 !e142 */
		retval = __get_user(tmp, (__u8 __user *)arg);/*lint !e866 !e1564 !e774 !e50 !e530 */
		if (retval == 0) {/*lint !e774 */
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;/*lint !e734 */
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);/*lint !e774 */
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:/*lint !e30 !e142 */
		retval = __get_user(tmp, (__u32 __user *)arg);/*lint !e866 !e1564 !e774 !e50 !e530 */
		if (retval == 0) {/*lint !e774 */
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval >= 0)
				spidev->speed_hz = tmp;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);/*lint !e774 */
			spi->max_speed_hz = save;
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		/* Check message and copy into scratch area */
		ioc = spidev_get_ioc_message(cmd,
				(struct spi_ioc_transfer __user *)arg, &n_ioc);
		if (IS_ERR(ioc)) {
			retval = PTR_ERR(ioc);/*lint !e712 */
			break;
		}
		if (!ioc)
			break;	/* n_ioc is also 0 */

		/* translate to spi_message, execute */
		retval = spidev_message(spidev, ioc, n_ioc);
		kfree(ioc);
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

#ifdef CONFIG_COMPAT
static long
spidev_compat_ioc_message(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct spi_ioc_transfer __user	*u_ioc;
	int				retval = 0;
	struct spidev_data		*spidev;
	struct spi_device		*spi;
	unsigned			n_ioc, n;
	struct spi_ioc_transfer		*ioc;

	u_ioc = (struct spi_ioc_transfer __user *) compat_ptr(arg);/*lint !e712 !e747 */
	if (!access_ok(VERIFY_READ, u_ioc, _IOC_SIZE(cmd)))/*lint !e530 !e529 */
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* SPI_IOC_MESSAGE needs the buffer locked "normally" */
	mutex_lock(&spidev->buf_lock);

	/* Check message and copy into scratch area */
	ioc = spidev_get_ioc_message(cmd, u_ioc, &n_ioc);
	if (IS_ERR(ioc)) {
		retval = PTR_ERR(ioc);/*lint !e712 */
		goto done;
	}
	if (!ioc)
		goto done;	/* n_ioc is also 0 */

	/* Convert buffer pointers */
	for (n = 0; n < n_ioc; n++) {
		ioc[n].rx_buf = (uintptr_t) compat_ptr(ioc[n].rx_buf);/*lint !e712 !e747 */
		ioc[n].tx_buf = (uintptr_t) compat_ptr(ioc[n].tx_buf);/*lint !e712 !e747 */
	}

	/* translate to spi_message, execute */
	retval = spidev_message(spidev, ioc, n_ioc);
	kfree(ioc);

done:
	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	dev_dbg(&spi->dev, "spidev_ioctl: retval:%d\n", retval);/*lint !e774 */

	return retval;
}

static long
spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) == SPI_IOC_MAGIC
			&& _IOC_NR(cmd) == _IOC_NR(SPI_IOC_MESSAGE(0))/*lint !e84 */
			&& _IOC_DIR(cmd) == _IOC_WRITE)/*lint !e845 */
		return spidev_compat_ioc_message(filp, cmd, arg);

	return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));/*lint !e712 !e747 */
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {/*lint !e64 !e826 */
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	if (!spidev->tx_buffer) {
		spidev->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);/*lint !e747 */
		if (!spidev->tx_buffer) {
				dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");/*lint !e774 */
				status = -ENOMEM;
			goto err_find_dev;
			}
		}

	if (!spidev->rx_buffer) {
		spidev->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);/*lint !e747 */
		if (!spidev->rx_buffer) {
			dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");/*lint !e774 */
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	spidev->users++;
	filp->private_data = spidev;
	nonseekable_open(inode, filp);

	mutex_unlock(&device_list_lock);
	return 0;

err_alloc_rx_buf:
	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;
err_find_dev:
	mutex_unlock(&device_list_lock);
	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	spidev->users--;
	if (!spidev->users) {
		int		dofree;

		kfree(spidev->tx_buffer);
		spidev->tx_buffer = NULL;

		kfree(spidev->rx_buffer);
		spidev->rx_buffer = NULL;

		spin_lock_irq(&spidev->spi_lock);
		if (spidev->spi)
			spidev->speed_hz = spidev->spi->max_speed_hz;

		/* ... after we unbound from the underlying device? */
		dofree = (spidev->spi == NULL);
		spin_unlock_irq(&spidev->spi_lock);

		if (dofree)
			kfree(spidev);
	}
	mutex_unlock(&device_list_lock);

	return 0;
}/*lint !e715 */

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,/*lint !e64 */
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	.llseek =	no_llseek,
};/*lint !e785 */

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

#ifdef CONFIG_OF
/*lint -e528 -esym(528,*) */
static const struct of_device_id spidev_dt_ids[] = {
/*lint -e528 +esym(528,*) */
#ifndef CONFIG_HISI_SPI
	{ .compatible = "rohm,dh2228fv" },
	{ .compatible = "lineartechnology,ltc2488" },
#else
	{ .compatible = "arm,pl022" },/*lint !e785 */
#endif
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
#endif

#ifdef CONFIG_ACPI

/* Dummy SPI devices not to be used in production systems */
#define SPIDEV_ACPI_DUMMY	1

static const struct acpi_device_id spidev_acpi_ids[] = {
	/*
	 * The ACPI SPT000* devices are only meant for development and
	 * testing. Systems used in production should have a proper ACPI
	 * description of the connected peripheral and they should also use
	 * a proper driver instead of poking directly to the SPI bus.
	 */
	{ "SPT0001", SPIDEV_ACPI_DUMMY },
	{ "SPT0002", SPIDEV_ACPI_DUMMY },
	{ "SPT0003", SPIDEV_ACPI_DUMMY },
	{},
};
MODULE_DEVICE_TABLE(acpi, spidev_acpi_ids);

static void spidev_probe_acpi(struct spi_device *spi)
{
	const struct acpi_device_id *id;

	if (!has_acpi_companion(&spi->dev))
		return;

	id = acpi_match_device(spidev_acpi_ids, &spi->dev);
	if (WARN_ON(!id))
		return;

	if (id->driver_data == SPIDEV_ACPI_DUMMY)
		dev_warn(&spi->dev, "do not use this driver in production systems!\n");
}
#else
static inline void spidev_probe_acpi(struct spi_device *spi) {}
#endif

/*-------------------------------------------------------------------------*/

static int spidev_probe(struct spi_device *spi)
{
	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;

	/*
	 * spidev should never be referenced in DT without a specific
	 * compatible string, it is a Linux implementation thing
	 * rather than a description of the hardware.
	 */
#ifndef CONFIG_HISI_SPI
	if (spi->dev.of_node && !of_match_device(spidev_dt_ids, &spi->dev)) {
		dev_err(&spi->dev, "buggy DT: spidev listed directly in DT\n");
		WARN_ON(spi->dev.of_node &&
			!of_match_device(spidev_dt_ids, &spi->dev));
	}
#endif

	spidev_probe_acpi(spi);

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);/*lint !e747 */
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);/*lint !e712 !e72 */
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");/*lint !e774 */
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);/*lint !e712 !e747 */
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	spidev->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);

	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

#ifdef CONFIG_HISI_SPI
static const struct of_device_id spidev1_dt_ids[] = {
	{ .compatible = "spi_dev1"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev1_dt_ids);

static struct spi_driver spidev_spi_driver1 = {
	.driver = {
		.name =		"spi_dev1",
		.owner =	THIS_MODULE,/*lint !e84  !e64*/
		.of_match_table = of_match_ptr(spidev1_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev2_dt_ids[] = {
	{ .compatible = "spi_dev2"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev2_dt_ids);

static struct spi_driver spidev_spi_driver2 = {
	.driver = {
		.name =		"spi_dev2",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev2_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev3_dt_ids[] = {
	{ .compatible = "spi_dev3"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev3_dt_ids);

static struct spi_driver spidev_spi_driver3 = {
	.driver = {
		.name =		"spi_dev3",
		.owner =	THIS_MODULE,/*lint !e785 !e64 */
		.of_match_table = of_match_ptr(spidev3_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev10_dt_ids[] = {
	{ .compatible = "spi_dev10"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev10_dt_ids);

static struct spi_driver spidev_spi_driver10 = {
	.driver = {
		.name =		"spi_dev10",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev10_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev11_dt_ids[] = {
	{ .compatible = "spi_dev11"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev11_dt_ids);

static struct spi_driver spidev_spi_driver11 = {
	.driver = {
		.name =         "spi_dev11",
		.owner =        THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev11_dt_ids),
	},/*lint !e785 */
	.probe =        spidev_probe,
	.remove =       spidev_remove,

       /* NOTE:  suspend/resume methods are not necessary here.
	* We don't do anything except pass the requests to/from
	* the underlying controller.  The refrigerator handles
	* most issues; the controller driver handles the rest.
	*/
};

static const struct of_device_id spidev12_dt_ids[] = {
	{ .compatible = "spi_dev12"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev12_dt_ids);

static struct spi_driver spidev_spi_driver12 = {
	.driver = {
		.name =         "spi_dev12",
		.owner =        THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev12_dt_ids),
	},/*lint !e785 */
	.probe =        spidev_probe,
	.remove =       spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev13_dt_ids[] = {
	{ .compatible = "spi_dev13"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev13_dt_ids);

static struct spi_driver spidev_spi_driver13 = {
	.driver = {
		.name =         "spi_dev13",
		.owner =        THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev13_dt_ids),
	},/*lint !e785 */
	.probe =        spidev_probe,
	.remove =       spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};
static const struct of_device_id spidev21_dt_ids[] = {
	{ .compatible = "spi_dev21"},/*lint !e785 */
	{},/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, spidev21_dt_ids);

static struct spi_driver spidev_spi_driver21 = {
	.driver = {
		.name =		"spi_dev21",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev21_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev30_dt_ids[] = {
	{ .compatible = "spi_dev30"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev30_dt_ids);
static struct spi_driver spidev_spi_driver30 = {
	.driver = {
		.name =		"spi_dev30",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev30_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev31_dt_ids[] = {
	{ .compatible = "spi_dev31"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev31_dt_ids);
static struct spi_driver spidev_spi_driver31 = {
	.driver = {
		.name =		"spi_dev31",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev31_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev32_dt_ids[] = {
	{ .compatible = "spi_dev32"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev32_dt_ids);
static struct spi_driver spidev_spi_driver32 = {
	.driver = {
		.name =		"spi_dev32",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev32_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev33_dt_ids[] = {
	{ .compatible = "spi_dev33"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev33_dt_ids);
static struct spi_driver spidev_spi_driver33 = {
	.driver = {
		.name =		"spi_dev33",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev33_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev40_dt_ids[] = {
	{ .compatible = "spi_dev40"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev40_dt_ids);
static struct spi_driver spidev_spi_driver40 = {
	.driver = {
		.name =		"spi_dev40",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev40_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev41_dt_ids[] = {
	{ .compatible = "spi_dev41"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev41_dt_ids);
static struct spi_driver spidev_spi_driver41 = {
	.driver = {
		.name =		"spi_dev41",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev41_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev42_dt_ids[] = {
	{ .compatible = "spi_dev42"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev42_dt_ids);
static struct spi_driver spidev_spi_driver42 = {
	.driver = {
		.name =		"spi_dev42",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev42_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

static const struct of_device_id spidev43_dt_ids[] = {
	{ .compatible = "spi_dev43"},/*lint !e785 */
	{},/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, spidev43_dt_ids);
static struct spi_driver spidev_spi_driver43 = {
	.driver = {
		.name =		"spi_dev43",
		.owner =	THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(spidev43_dt_ids),
	},/*lint !e785 */
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};
#else
static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.of_match_table = of_match_ptr(spidev_dt_ids),
		.acpi_match_table = ACPI_PTR(spidev_acpi_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};
#endif

/*-------------------------------------------------------------------------*/

static int __init spidev_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);/*lint !e514 */
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");/*lint !e64 */
	if (IS_ERR(spidev_class)) {
#ifndef CONFIG_HISI_SPI
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
#endif
		return PTR_ERR(spidev_class);/*lint !e712 */
	}

#ifdef CONFIG_HISI_SPI
	status = spi_register_driver(&spidev_spi_driver1);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver1.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver2);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver2.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver3);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver3.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver10);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver10.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver11);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver11.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver12);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver12.driver.name);
	 }

	status = spi_register_driver(&spidev_spi_driver13);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver13.driver.name);
	 }

	status = spi_register_driver(&spidev_spi_driver21);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver21.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver30);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver30.driver.name);
	}


	status = spi_register_driver(&spidev_spi_driver31);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver31.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver32);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver32.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver33);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver33.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver40);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver40.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver41);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver41.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver42);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver42.driver.name);
	}

	status = spi_register_driver(&spidev_spi_driver43);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver43.driver.name);
	}
#else
	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}
#endif

	return status;
}
/*lint -e528 -esym(528,*) */
module_init(spidev_init);/*lint !e528 */
/*lint -e528 +esym(528,*) */
static void __exit spidev_exit(void)
{
#ifdef CONFIG_HISI_SPI
	spi_unregister_driver(&spidev_spi_driver1);
	spi_unregister_driver(&spidev_spi_driver2);
	spi_unregister_driver(&spidev_spi_driver3);
	spi_unregister_driver(&spidev_spi_driver10);
	spi_unregister_driver(&spidev_spi_driver11);
	spi_unregister_driver(&spidev_spi_driver12);
	spi_unregister_driver(&spidev_spi_driver13);
	spi_unregister_driver(&spidev_spi_driver21);
	spi_unregister_driver(&spidev_spi_driver30);
	spi_unregister_driver(&spidev_spi_driver31);
	spi_unregister_driver(&spidev_spi_driver32);
	spi_unregister_driver(&spidev_spi_driver33);
	spi_unregister_driver(&spidev_spi_driver40);
	spi_unregister_driver(&spidev_spi_driver41);
	spi_unregister_driver(&spidev_spi_driver42);
	spi_unregister_driver(&spidev_spi_driver43);

	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver1.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver2.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver3.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver10.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver11.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver12.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver13.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver21.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver30.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver31.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver32.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver33.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver40.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver41.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver42.driver.name);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver43.driver.name);
#else
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
#endif
}
/*lint -e528 -esym(528,*) */
module_exit(spidev_exit);
/*lint -e528 +esym(528,*) */

/*lint -e753 -esym(753,*) */
MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp!eng.it>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
/*lint -e753 +esym(753,*) */
