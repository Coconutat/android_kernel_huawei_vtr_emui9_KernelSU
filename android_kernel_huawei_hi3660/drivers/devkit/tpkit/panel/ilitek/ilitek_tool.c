#include "ilitek_ts.h"
#ifdef TOOL
#ifdef TOOL
struct dev_data {
	dev_t devno;
	struct cdev cdev;
	struct class *class;
};
 struct dev_data dev_ilitek;
 struct proc_dir_entry * proc_ilitek;
 int create_tool_node(void);
 int remove_tool_node(void);
extern int driver_information[];
extern char Report_Flag;
extern volatile char int_Flag;
extern volatile char update_Flag;
extern int update_timeout;
extern char EXCHANG_XY;
extern char REVERT_X;
extern char REVERT_Y;
extern char DBG_FLAG,DBG_COR;
extern struct i2c_data * ilitek_data;
#endif
extern u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL;
#define ILITEK_IOCTL_BASE                       100
#define ILITEK_IOCTL_I2C_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 0,unsigned char*)
#define ILITEK_IOCTL_I2C_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 2, unsigned char*)
#define ILITEK_IOCTL_I2C_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 3, int)
#define ILITEK_IOCTL_USB_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 4, unsigned char*)
#define ILITEK_IOCTL_USB_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 5, int)
#define ILITEK_IOCTL_USB_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 6, unsigned char*)
#define ILITEK_IOCTL_USB_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 7, int)

#define ILITEK_IOCTL_DRIVER_INFORMATION		    _IOWR(ILITEK_IOCTL_BASE, 8, int)
#define ILITEK_IOCTL_USB_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 9, int)

#define ILITEK_IOCTL_I2C_INT_FLAG	            _IOWR(ILITEK_IOCTL_BASE, 10, int)
#define ILITEK_IOCTL_I2C_UPDATE                 _IOWR(ILITEK_IOCTL_BASE, 11, int)
#define ILITEK_IOCTL_STOP_READ_DATA             _IOWR(ILITEK_IOCTL_BASE, 12, int)
#define ILITEK_IOCTL_START_READ_DATA            _IOWR(ILITEK_IOCTL_BASE, 13, int)
#define ILITEK_IOCTL_GET_INTERFANCE				_IOWR(ILITEK_IOCTL_BASE, 14, int)//default setting is i2c interface
#define ILITEK_IOCTL_I2C_SWITCH_IRQ				_IOWR(ILITEK_IOCTL_BASE, 15, int)

#define ILITEK_IOCTL_UPDATE_FLAG				_IOWR(ILITEK_IOCTL_BASE, 16, int)
#define ILITEK_IOCTL_I2C_UPDATE_FW				_IOWR(ILITEK_IOCTL_BASE, 18, int)
#define ILITEK_IOCTL_RESET				_IOWR(ILITEK_IOCTL_BASE, 19, int)
#define ILITEK_IOCTL_INT_STATUS						_IOWR(ILITEK_IOCTL_BASE, 20, int)
#ifdef DEBUG_NETLINK
#define ILITEK_IOCTL_DEBUG_SWITCH						_IOWR(ILITEK_IOCTL_BASE, 21, int)
extern bool debug_flag;
#endif

static int ilitek_file_open(struct inode *inode, struct file *filp)
{
	ilitek_data->apk_use= true;
	TS_LOG_INFO("%s apk_use = %d\n",__func__, ilitek_data->apk_use);
	return 0;
}

static int ilitek_i2c_calibration(size_t count)
{
	int ret;
	unsigned char buffer[128]={0};

	buffer[0] = ILITEK_TP_CMD_ERASE_BACKGROUND;
	ret = ilitek_i2c_write(buffer, 1);
	if(ret < 0){
		TS_LOG_ERR("%s, i2c erase background, failed\n", __func__);
	}
	else{
		TS_LOG_INFO("%s, i2c erase background, success\n", __func__);
	}

	buffer[0] = ILITEK_TP_CMD_CALIBRATION;
	msleep(2000);
	ret = ilitek_i2c_write(buffer, 1);
	if(ret < 0){
		TS_LOG_ERR("%s, i2c calibration failed\n", __func__);
	}
	msleep(1000);
	return ret;
}

static int ilitek_i2c_calibration_status(size_t count)
{
	int ret;
	unsigned char buffer[128]={0};
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ret = ilitek_i2c_write(buffer, 1);
	if(ret < 0){
		TS_LOG_ERR("%s, i2c calibration status failed\n", __func__);
	}
	msleep(500);
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ilitek_i2c_write_and_read(buffer, 1, 10, buffer, 1);
	TS_LOG_INFO("%s, i2c calibration status:0x%X\n",__func__,buffer[0]);
	ret=buffer[0];
	return ret;
}

static ssize_t ilitek_file_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char buffer[128]={0};

	// before sending data to touch device, we need to check whether the device is working or not

	// check the buffer size whether it exceeds the local buffer size or not
	if(count > 128){
		TS_LOG_ERR("%s, buffer exceed 128 bytes\n", __func__);
		return -1;
	}

	ret = copy_from_user(buffer, buf, count-1);
	if(ret < 0){
		TS_LOG_ERR("%s, copy data from user space, failed", __func__);
		return -1;
	}

	if(strcmp(buffer, "calibrate") == 0){
		ret=ilitek_i2c_calibration(count);
		if(ret < 0){
			TS_LOG_ERR("%s, i2c send calibration command, failed\n", __func__);
		}
		else{
			TS_LOG_INFO("%s, i2c send calibration command, success\n", __func__);
		}
		ret=ilitek_i2c_calibration_status(count);
		if(ret == 0x5A){
			TS_LOG_INFO("%s, i2c calibration, success\n", __func__);
		}
		else if (ret == 0xA5){
			TS_LOG_ERR("%s, i2c calibration, failed\n", __func__);
		}
		else{
			TS_LOG_ERR("%s, i2c calibration, i2c protoco failed\n", __func__);
		}
		return count;
	}else if(strcmp(buffer, "dbg") == 0){
		DBG_FLAG=!DBG_FLAG;
		TS_LOG_INFO("%s, %s DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "dbgco") == 0){
		DBG_COR=!DBG_COR;
		TS_LOG_INFO("%s, %s DBG COORDINATE message(%X).\n",__func__,DBG_COR?"Enabled":"Disabled",DBG_COR);
	}else if(strcmp(buffer, "info") == 0){
		ilitek_i2c_read_tp_info();
	}else if(strcmp(buffer, "report") == 0){
		Report_Flag=!Report_Flag;
	}else if(strcmp(buffer, "chxy") == 0){
		EXCHANG_XY=!EXCHANG_XY;
	}else if(strcmp(buffer, "revx") == 0){
		REVERT_X=!REVERT_X;
	}else if(strcmp(buffer, "revy") == 0){
		REVERT_Y=!REVERT_Y;
	}else if(strcmp(buffer, "suspd") == 0){
		ilitek_suspend();
	}else if(strcmp(buffer, "resm") == 0){
		ilitek_resume();
	}
	else if(strcmp(buffer, "stop_report") == 0){
		ilitek_data->report_status = 0;
		TS_LOG_INFO("The report point function is disable.\n");
	}else if(strcmp(buffer, "start_report") == 0){
		ilitek_data->report_status = 1;
		TS_LOG_INFO("The report point function is enable.\n");
	}else if(strcmp(buffer, "update_flag") == 0){
		TS_LOG_INFO("update_Flag=%d\n",update_Flag);
	}else if(strcmp(buffer, "reset") == 0){
		TS_LOG_INFO("start reset\n");
		if(ilitek_data->reset_request_success)
			ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
		TS_LOG_INFO("end reset\n");
	}else if(strcmp(buffer, "irq_status") == 0){
		printk("%s, gpio_get_value(ilitek_data->irq_gpio) = %d.\n",__func__, gpio_get_value(ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio));
	}else if(strcmp(buffer, "roi_enable") == 0){
		ilitek_data->ilitek_roi_enabled = !(ilitek_data->ilitek_roi_enabled);
		TS_LOG_INFO("ilitek_data->ilitek_roi_enabled = %d \n", ilitek_data->ilitek_roi_enabled);
	}
#ifdef DEBUG_NETLINK
	else if(strcmp(buffer, "dbg_flag") == 0){
		debug_flag =!debug_flag;
		printk("%s, %s debug_flag message(%X).\n",__func__,debug_flag?"Enabled":"Disabled",debug_flag);
	}
	else if(strcmp(buffer, "enable") == 0){
		enable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		printk("%s, %s enable_irq(ilitek_data->client->irq)DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "disable") == 0){
		disable_irq(ilitek_data->ilitek_chip_data->ts_platform_data->irq_id);
		printk("%s, %s disable_irq(ilitek_data->client->irq) DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "debug_5") == 0){
		TOUCH_DRIVER_DEBUG_LOG_LEVEL = 5;
	}else if(strcmp(buffer, "debug_4") == 0){
		TOUCH_DRIVER_DEBUG_LOG_LEVEL = 4;
	}else if(strcmp(buffer, "debug_3") == 0){
		TOUCH_DRIVER_DEBUG_LOG_LEVEL = 3;
	}else if(strcmp(buffer, "debug_2") == 0){
		TOUCH_DRIVER_DEBUG_LOG_LEVEL = 2;
	}else if(strcmp(buffer, "debug_1") == 0){
		TOUCH_DRIVER_DEBUG_LOG_LEVEL = 1;
	}
#endif
	return -1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long ilitek_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int  ilitek_file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	static unsigned char buffer[512]={0};
	static int len = 0, i;
	int ret;

	switch(cmd){
		case ILITEK_IOCTL_I2C_WRITE_DATA:
			ret = copy_from_user(buffer, (unsigned char*)arg, len);
			if(ret < 0){
				TS_LOG_ERR("%s, copy data from user space, failed\n", __func__);
				return -1;
			}
			ret = ilitek_i2c_write(buffer, len);
			if(ret < 0){
				TS_LOG_ERR("%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_READ_DATA:
			ret = ilitek_i2c_read(buffer, 0, buffer, len);
			if(ret < 0){
				TS_LOG_ERR("%s, i2c read, failed\n", __func__);
				return -1;
			}
			ret = copy_to_user((unsigned char*)arg, buffer, len);

			if(ret < 0){
				TS_LOG_ERR("%s, copy data to user space, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_WRITE_LENGTH:
		case ILITEK_IOCTL_I2C_READ_LENGTH:
			len = arg;
			break;
		case ILITEK_IOCTL_DRIVER_INFORMATION:
			for(i = 0; i < 7; i++){
				buffer[i] = driver_information[i];
			}
			ret = copy_to_user((unsigned char*)arg, buffer, 7);
			break;
		case ILITEK_IOCTL_I2C_UPDATE:
			break;
		case ILITEK_IOCTL_I2C_INT_FLAG:
			if(update_timeout == 1){
				buffer[0] = int_Flag;
				ret = copy_to_user((unsigned char*)arg, buffer, 1);
				if(ret < 0){
					TS_LOG_ERR("%s, copy data to user space, failed\n", __func__);
					return -1;
				}
			}
			else
				update_timeout = 1;

			break;
		case ILITEK_IOCTL_START_READ_DATA:
			ilitek_data->stop_polling = 0;
			if((ilitek_data->ilitek_chip_data->ts_platform_data->irq_id) != 0 )
				ilitek_i2c_irq_enable();
			ilitek_data->report_status = 1;
			TS_LOG_INFO("The report point function is enable.\n");
			break;
		case ILITEK_IOCTL_STOP_READ_DATA:
			ilitek_data->stop_polling = 1;
			if((ilitek_data->ilitek_chip_data->ts_platform_data->irq_id) != 0 )
				ilitek_i2c_irq_disable();
			ilitek_data->report_status = 0;
			TS_LOG_INFO("The report point function is disable.\n");
			break;
		case ILITEK_IOCTL_RESET:
			ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
			break;
		case ILITEK_IOCTL_INT_STATUS:
			//put_user(gpio_get_value((ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio), (int *)arg);
			buffer[0] = gpio_get_value((ilitek_data->ilitek_chip_data->ts_platform_data->irq_gpio));
			ret = copy_to_user((unsigned char*)arg, buffer, 1);
			break;
#ifdef DEBUG_NETLINK
			case ILITEK_IOCTL_DEBUG_SWITCH:
				ret = copy_from_user(buffer, (unsigned char*)arg, 1);
				printk("ilitek The debug_flag = %d.\n", buffer[0]);
				if (buffer[0] == 0)
				{
					debug_flag = false;
				}
				else if (buffer[0] == 1)
				{
					debug_flag = true;
				}
				break;
#endif
		case ILITEK_IOCTL_I2C_SWITCH_IRQ:
			ret = copy_from_user(buffer, (unsigned char*)arg, 1);
			if (buffer[0] == 0)
			{
				if((ilitek_data->ilitek_chip_data->ts_platform_data->irq_id) != 0 ){
					TS_LOG_INFO("ilitek_i2c_irq_disable ready.\n");
					ilitek_i2c_irq_disable();
				}
			}
			else
			{
				if((ilitek_data->ilitek_chip_data->ts_platform_data->irq_id) != 0 ){
					TS_LOG_INFO("ilitek_i2c_irq_enable ready.\n");
					ilitek_i2c_irq_enable();
				}
			}
			break;
		case ILITEK_IOCTL_UPDATE_FLAG:
			update_timeout = 1;
			update_Flag = arg;
			TS_LOG_DEBUG("%s,update_Flag=%d\n",__func__,update_Flag);
			break;
		case ILITEK_IOCTL_I2C_UPDATE_FW:
			ret = copy_from_user(buffer, (unsigned char*)arg, 35);
			if(ret < 0){
				TS_LOG_ERR("%s, copy data from user space, failed\n", __func__);
				return -1;
			}
			int_Flag = 0;
			update_timeout = 0;
			//msgs[0].len = buffer[34];
			ret =ilitek_i2c_write(buffer, buffer[34]);
#ifndef CLOCK_INTERRUPT
			ilitek_i2c_irq_enable();
#endif
			if(ret < 0){
				TS_LOG_ERR("%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}

static ssize_t ilitek_file_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static int ilitek_file_close(struct inode *inode, struct file *filp)
{
	ilitek_data->apk_use= false;
	TS_LOG_INFO("%s apk_use = %d\n",__func__, ilitek_data->apk_use);
	return 0;
}

struct file_operations ilitek_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	.unlocked_ioctl = ilitek_file_ioctl,
#else
	.ioctl = ilitek_file_ioctl,
#endif
	.read = ilitek_file_read,
	.write = ilitek_file_write,
	.open = ilitek_file_open,
	.release = ilitek_file_close,
};

int create_tool_node(void) {
	int ret = 0;
	// allocate character device driver buffer
	ret = alloc_chrdev_region(&dev_ilitek.devno, 0, 1, ILITEK_FILE_DRIVER_NAME);
	if(ret){
		TS_LOG_ERR("%s, can't allocate chrdev\n", __func__);
		return ret;
	}
	TS_LOG_INFO("%s, register chrdev(%d, %d)\n", __func__, MAJOR(dev_ilitek.devno), MINOR(dev_ilitek.devno));

	// initialize character device driver
	cdev_init(&dev_ilitek.cdev, &ilitek_fops);
	dev_ilitek.cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_ilitek.cdev, dev_ilitek.devno, 1);
	if(ret < 0){
		TS_LOG_ERR("%s, add character device error, ret %d\n", __func__, ret);
		return ret;
	}
	dev_ilitek.class = class_create(THIS_MODULE, ILITEK_FILE_DRIVER_NAME);
	if(IS_ERR(dev_ilitek.class)){
		TS_LOG_ERR("%s, create class, error\n", __func__);
		return ret;
	}
	device_create(dev_ilitek.class, NULL, dev_ilitek.devno, NULL, "ilitek_ctrl");
	proc_ilitek = proc_create("ilitek_ctrl", 0666, NULL, &ilitek_fops);
	if(proc_ilitek == NULL) {
		TS_LOG_ERR("proc_create(ilitek_ctrl, 0666, NULL, &ilitek_fops) fail\n");
	}
	return 0;
}

int remove_tool_node(void)
{
	cdev_del(&dev_ilitek.cdev);
	unregister_chrdev_region(dev_ilitek.devno, 1);
	device_destroy(dev_ilitek.class, dev_ilitek.devno);
	class_destroy(dev_ilitek.class);
	if (proc_ilitek) {
		proc_remove(proc_ilitek);
	}
	return 0;
}
#endif
