#include "gtx8.h"

#define GOODIX_TOOLS_NAME		"gtp_tools"
#define GOODIX_TS_IOC_MAGIC		'G'
#define NEGLECT_SIZE_MASK		(~(_IOC_SIZEMASK << _IOC_SIZESHIFT))

#define GTP_IRQ_ENABLE	_IO(GOODIX_TS_IOC_MAGIC, 0)
#define GTP_DEV_RESET	_IO(GOODIX_TS_IOC_MAGIC, 1)
#define GTP_SEND_COMMAND (_IOW(GOODIX_TS_IOC_MAGIC, 2, u8) & NEGLECT_SIZE_MASK)
#define GTP_SEND_CONFIG	(_IOW(GOODIX_TS_IOC_MAGIC, 3, u8) & NEGLECT_SIZE_MASK)
#define GTP_ASYNC_READ	(_IOR(GOODIX_TS_IOC_MAGIC, 4, u8) & NEGLECT_SIZE_MASK)
#define GTP_SYNC_READ	(_IOR(GOODIX_TS_IOC_MAGIC, 5, u8) & NEGLECT_SIZE_MASK)
#define GTP_ASYNC_WRITE	(_IOW(GOODIX_TS_IOC_MAGIC, 6, u8) & NEGLECT_SIZE_MASK)
#define GTP_READ_CONFIG	(_IOW(GOODIX_TS_IOC_MAGIC, 7, u8) & NEGLECT_SIZE_MASK)
#define GTP_ESD_ENABLE	_IO(GOODIX_TS_IOC_MAGIC, 8)
#define GTP_CLEAR_RAWDATA_FLAG	_IO(GOODIX_TS_IOC_MAGIC, 9)

#define GOODIX_TS_IOC_MAXNR			10
#define GOODIX_TOOLS_MAX_DATA_LEN	4096
#define I2C_MSG_HEAD_LEN				20

#define GTX8_TOOLS_CLOSE  0
#define GTX8_TOOLS_OPEN  1
static atomic_t open_flag = ATOMIC_INIT(GTX8_TOOLS_CLOSE);

static int init_cfg_data(struct gtx8_ts_config *cfg, void __user *arg)
{
	int ret = NO_ERR;
	u32 reg_addr = 0, length = 0;
	u8 i2c_msg_head[I2C_MSG_HEAD_LEN] = {0};

	cfg->initialized = 0;
	mutex_init(&cfg->lock);
	ret = copy_from_user(&i2c_msg_head, arg, I2C_MSG_HEAD_LEN);
	if (ret) {
		TS_LOG_ERR("Copy data from user failed\n");
		return -EFAULT;
	}

	reg_addr = i2c_msg_head[0] + (i2c_msg_head[1] << 8)
			+ (i2c_msg_head[2] << 16) + (i2c_msg_head[3] << 24);
	length = i2c_msg_head[4] + (i2c_msg_head[5] << 8)
			+ (i2c_msg_head[6] << 16) + (i2c_msg_head[7] << 24);
	if (length > GOODIX_CFG_MAX_SIZE) {
		TS_LOG_ERR("%s: Invalied config length:%d\n",__func__, length);
		return -EFAULT;
	}
	ret = copy_from_user(cfg->data, (u8 *)arg + I2C_MSG_HEAD_LEN, length);
	if (ret) {
		ret = -EFAULT;
		TS_LOG_ERR("Copy data from user failed\n");
		goto err_out;
	}
	cfg->reg_base = reg_addr;
	cfg->length = length;
	strlcpy(cfg->name, "tools-send-cfg", sizeof(cfg->name));
	cfg->delay = 50;
	cfg->initialized = true;
	return 0;

err_out:
	return ret;
}

/* if success return config data length */
static int read_config_data(void __user *arg)
{
	int ret = NO_ERR;
	u32 reg_addr = 0, length = 0;
	u8 i2c_msg_head[I2C_MSG_HEAD_LEN] = {0};
	u8 *tmp_buf = NULL;

	ret = copy_from_user(&i2c_msg_head, arg, I2C_MSG_HEAD_LEN);
	if (ret) {
		TS_LOG_ERR("Copy data from user failed\n");
		return -EFAULT;
	}

	reg_addr = i2c_msg_head[0] + (i2c_msg_head[1] << 8)
		   + (i2c_msg_head[2] << 16) + (i2c_msg_head[3] << 24);
	length = i2c_msg_head[4] + (i2c_msg_head[5] << 8)
		 + (i2c_msg_head[6] << 16) + (i2c_msg_head[7] << 24);
	if (length > GOODIX_CFG_MAX_SIZE) {
		TS_LOG_ERR("%s: Invalied config length:%d\n",__func__, length);
		return -EFAULT;
	}

	TS_LOG_INFO("read config,reg_addr=0x%x, length=%d\n", reg_addr, length);

	tmp_buf = kzalloc(length, GFP_KERNEL);
	if (!tmp_buf) {
		TS_LOG_ERR("failed alloc memory\n");
		return -ENOMEM;
	}

	/* if reg_addr == 0, read config data with specific flow */
	if (!reg_addr) {
		if (gtx8_ts->ops.read_cfg)
			ret = gtx8_ts->ops.read_cfg(tmp_buf, 0);
		else
			ret = -EINVAL;
	} else {
		ret = gtx8_ts->ops.read_trans((u16)reg_addr, tmp_buf, length);
		if (!ret)
			ret = length;
	}
	if (ret <= 0)
		goto err_out;

	if (copy_to_user((u8 *)arg + I2C_MSG_HEAD_LEN, tmp_buf, ret)) {
			ret = -EFAULT;
			TS_LOG_ERR("Copy_to_user failed\n");
	}

err_out:
	if(tmp_buf) {
		kfree(tmp_buf);
		tmp_buf = NULL;
	}
	return ret;
}

/* read data from i2c asynchronous,
** success return bytes read, else return <= 0
*/
static int async_read(void __user *arg)
{
	u8 *databuf = NULL;
	int ret = NO_ERR;
	u32 reg_addr = 0;
	u32 length = 0;
	u8 i2c_msg_head[I2C_MSG_HEAD_LEN];

	ret = copy_from_user(&i2c_msg_head, arg, I2C_MSG_HEAD_LEN);
	if (ret) {
		ret = -EFAULT;
		return ret;
	}

	reg_addr = i2c_msg_head[0] + (i2c_msg_head[1] << 8)
			+ i2c_msg_head[2] + (i2c_msg_head[3] << 8);
	length = i2c_msg_head[4] + (i2c_msg_head[5] << 8)
			+ (i2c_msg_head[6] << 16) + (i2c_msg_head[7] << 24);
	if (length > GOODIX_TOOLS_MAX_DATA_LEN) {
		TS_LOG_ERR("%s: Invalied data length:%d\n",__func__, length);
		return -EFAULT;
	}

	databuf = kzalloc(length, GFP_KERNEL);
	if (!databuf) {
			TS_LOG_ERR("Alloc memory failed\n");
			return -ENOMEM;
	}

	if (!gtx8_ts->ops.read_trans((u16)reg_addr, databuf, length)) {
		if (copy_to_user((u8 *)arg + I2C_MSG_HEAD_LEN, databuf, length)) {
			ret = -EFAULT;
			TS_LOG_ERR("Copy_to_user failed\n");
		} else {
			ret = length;
		}
	} else {
		ret = -EBUSY;
		TS_LOG_ERR("Read i2c failed\n");
	}

	if(databuf) {
		kfree(databuf);
		databuf = NULL;
	}
	return ret;
}

/* write data to i2c asynchronous,
** success return bytes write, else return <= 0
*/
static int async_write(void __user *arg)
{
	u8 *databuf = NULL;
	int ret = 0;
	u32 reg_addr = 0;
	u32 length = 0;
	u8 i2c_msg_head[I2C_MSG_HEAD_LEN];

	ret = copy_from_user(&i2c_msg_head, arg, I2C_MSG_HEAD_LEN);
	if (ret) {
		TS_LOG_ERR("Copy data from user failed\n");
		return -EFAULT;
	}

	reg_addr = i2c_msg_head[0] + (i2c_msg_head[1] << 8)
			+ i2c_msg_head[2] + (i2c_msg_head[3] << 8);
	length = i2c_msg_head[4] + (i2c_msg_head[5] << 8)
			+ (i2c_msg_head[6] << 16) + (i2c_msg_head[7] << 24);

	if (length > GOODIX_TOOLS_MAX_DATA_LEN) {
		TS_LOG_ERR("%s: Invalied data length:%d\n",__func__, length);
		return -EFAULT;
	}
	databuf = kzalloc(length, GFP_KERNEL);
	if (!databuf) {
			TS_LOG_ERR("Alloc memory failed\n");
			return -ENOMEM;
	}

	ret = copy_from_user(databuf, (u8 *)arg + I2C_MSG_HEAD_LEN, length);
	if (ret) {
		ret = -EFAULT;
		TS_LOG_ERR("Copy data from user failed\n");
		goto err_out;
	}

	if (gtx8_ts->ops.write_trans((u16)reg_addr, databuf, length)) {
		ret = -EBUSY;
		TS_LOG_ERR("Write data to device failed\n");
	} else {
		ret = length;
	}

err_out:
	if(databuf) {
		kfree(databuf);
		databuf = NULL;
	}
	return ret;
}

static int goodix_tools_open(struct inode *inode, struct file *filp)
{
	TS_LOG_INFO("tools open\n");
	if(GTX8_TOOLS_CLOSE == atomic_cmpxchg(&open_flag ,GTX8_TOOLS_CLOSE ,GTX8_TOOLS_OPEN)){
		TS_LOG_INFO("%s: open sucess\n", __func__);
		gtx8_ts->tool_esd_disable = 1;
		return NO_ERR;
	}else{
		TS_LOG_ERR("tools already open!\n");
		return -ERESTARTSYS;
	}
}

static int goodix_tools_release(struct inode *inode, struct file *filp)
{
	TS_LOG_INFO("tools release\n");
	atomic_set(&open_flag, GTX8_TOOLS_CLOSE);
	gtx8_ts->rawdiff_mode = 0;
	gtx8_ts->tool_esd_disable = 0;
	return 0;
}

/**
 * goodix_tools_ioctl - ioctl implementation
 *
 * @filp: Pointer to file opened
 * @cmd: Ioctl opertion command
 * @arg: Command data
 * Returns >=0 - succeed, else failed
 */
 #define TOOL_CMD_LEN 2
static long goodix_tools_ioctl(struct file *filp, unsigned int cmd,
					unsigned long arg)
{
	u8 temp_cmd[TOOL_CMD_LEN] = {0};
	struct gtx8_ts_config *temp_cfg = NULL;
	int ret = NO_ERR;

	if (_IOC_TYPE(cmd) != GOODIX_TS_IOC_MAGIC) {
		TS_LOG_ERR("Bad magic num:%c\n", _IOC_TYPE(cmd));
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > GOODIX_TS_IOC_MAXNR) {
		TS_LOG_ERR("Bad cmd num:%d > %d",
		       _IOC_NR(cmd), GOODIX_TS_IOC_MAXNR);
		return -ENOTTY;
	}

	switch (cmd & NEGLECT_SIZE_MASK) {
	case GTP_IRQ_ENABLE:
		TS_LOG_INFO("Unsupport irq operation\n");
		ret = 0;
		break;
	case GTP_ESD_ENABLE:
		TS_LOG_INFO("Unsupport esd operation\n");
		ret = 0;
		break;
	case GTP_DEV_RESET:
		gtx8_ts->ops.chip_reset();
		break;
	case GTP_SEND_COMMAND:
		ret = copy_from_user(temp_cmd, (void __user *)arg, TOOL_CMD_LEN);
		if (ret) {
			ret = -EINVAL;
			goto err_out;
		}

		ret = gtx8_ts->ops.send_cmd(temp_cmd[0], temp_cmd[1], GTX8_NOT_NEED_SLEEP);
		if (ret) {
			TS_LOG_ERR("Send command failed\n");
			ret = -EAGAIN;
		}
		break;
	case GTP_SEND_CONFIG:
		temp_cfg = kzalloc(sizeof(struct gtx8_ts_config), GFP_KERNEL);
		if (temp_cfg == NULL) {
			TS_LOG_ERR("Memory allco err\n");
			ret = -ENOMEM;
			goto err_out;
		}

		ret = init_cfg_data(temp_cfg, (void __user *)arg);
		if (!ret && gtx8_ts->ops.send_cfg) {
			ret = gtx8_ts->ops.send_cfg(temp_cfg);
			if (ret) {
				TS_LOG_ERR("Failed send config\n");
				ret = -EAGAIN;
			} else {
				TS_LOG_INFO("Send config success\n");
				ret = NO_ERR;
			}
		}
		break;
	case GTP_READ_CONFIG:
		ret = read_config_data((void __user *)arg);
		if (ret > 0)
			TS_LOG_INFO("success read config:len=%d\n", ret);
		else
			TS_LOG_ERR("failed read config:ret=0x%x\n", ret);
		break;
	case GTP_ASYNC_READ:
		ret = async_read((void __user *)arg);
		if (ret < 0)
			TS_LOG_ERR("Async data read failed\n");
		break;
	case GTP_SYNC_READ:
		gtx8_ts->rawdiff_mode = 1;
		break;
	case GTP_ASYNC_WRITE:
		ret = async_write((void __user *)arg);
		if (ret < 0)
			TS_LOG_ERR("Async data write failed\n");
		break;
	case GTP_CLEAR_RAWDATA_FLAG:
		gtx8_ts->rawdiff_mode = 0;
		ret = 0;
		break;

	default:
		TS_LOG_INFO("Invalid cmd\n");
		ret = -ENOTTY;
		break;
	}

err_out:
	if (temp_cfg) {
		kfree(temp_cfg);
		temp_cfg = NULL;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long goodix_tools_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;
	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
}
#endif

static const struct file_operations goodix_tools_fops = {
	.owner		= THIS_MODULE,
	.open		= goodix_tools_open,
	.release	= goodix_tools_release,
	.unlocked_ioctl	= goodix_tools_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = goodix_tools_compat_ioctl,
#endif
};

static struct miscdevice goodix_tools_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= GOODIX_TOOLS_NAME,
	.fops	= &goodix_tools_fops,
};

int gtx8_init_tool_node(void)
{
	int ret = NO_ERR;

	ret = misc_register(&goodix_tools_miscdev);
	if (ret)
		TS_LOG_ERR("Debug tools miscdev register failed\n");


	return ret;
}

void gtx8_deinit_tool_node(void)
{
	misc_deregister(&goodix_tools_miscdev);
	TS_LOG_INFO("Goodix tools miscdev exit\n");
}