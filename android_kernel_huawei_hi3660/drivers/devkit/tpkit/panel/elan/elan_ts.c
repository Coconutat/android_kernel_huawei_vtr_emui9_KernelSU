#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/ctype.h>
#include <linux/atomic.h>
#include <linux/of_gpio.h>
#include "elan_ts.h"
#include "elan_mmi.h"
#include "../../huawei_ts_kit_api.h"
#include "../../../lcdkit/lcdkit1.0/include/lcdkit_panel.h"

static int elan_ktf_chip_detect(struct ts_kit_platform_data *platform_data);
static int elan_ktf_init_chip(void);
static int elan_ktf_input_config(struct input_dev *input_dev);
static int elan_ktf_irq_top_half(struct ts_cmd_node *cmd);
static int elan_ktf_irq_bottom_half(struct ts_cmd_node *in_cmd,struct ts_cmd_node *out_cmd);
static int elan_ktf_fw_update_boot(char *file_name);
static int elan_ktf_fw_update_sd(void);
static int elan_ktf_hw_reset(void);
static int elan_ktf_core_suspend(void);
static int elan_ktf_core_resume(void);
static void elan_chip_shutdown(void);
static int elan_ktf_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data);
static int elan_config_gpio(void);
static int elan_before_suspend(void);
static int elan_ktf_pen_config(struct input_dev *input_dev );
static int elan_read_tx_rx(void);
static int elan_ktf_get_rawdata(struct ts_rawdata_info* info, struct ts_cmd_node* out_cmd);
static int i2c_communicate_check(void);
static int elan_ktf_get_info(struct ts_chip_info_param* info);
static int elan_read_fw_info(void);
static int elan_chip_get_capacitance_test_type(struct ts_test_type_info *info);
static int rawdata_proc_elan_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size);
static atomic_t last_pen_inrange_status = ATOMIC_INIT(TS_PEN_OUT_RANGE); //remember the last pen inrange status
extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];
struct elan_ktf_ts_data *elan_ts=NULL;
enum TP_MODE
{
	TP_NORMAL,//normal mode
	TP_RECOVER,//update fw fail mode
	TP_MODULETEST,//mmi test mode
	TP_FWUPDATA,//update fw mode
};

/*fw debug tool use*/
#ifdef ELAN_IAP
#define ELAN_IOCTLID	0xD0
#define IOCTL_RESET  _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK  _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_FW_VER  _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_IAP_MODE_UNLOCK  _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  _IOR(ELAN_IOCTLID, 13, int)
#endif

struct ts_device_ops ts_kit_elan_ops = {
	.chip_parse_config = elan_ktf_parse_dts,
	.chip_detect = elan_ktf_chip_detect,
	.chip_init = elan_ktf_init_chip,
	.chip_input_config = elan_ktf_input_config,
	.chip_input_pen_config = elan_ktf_pen_config,
	.chip_irq_top_half = elan_ktf_irq_top_half,
	.chip_irq_bottom_half = elan_ktf_irq_bottom_half,
	.chip_suspend = elan_ktf_core_suspend,
	.chip_resume = elan_ktf_core_resume,
	.chip_hw_reset = elan_ktf_hw_reset,
	.chip_fw_update_boot = elan_ktf_fw_update_boot,
	.chip_fw_update_sd = elan_ktf_fw_update_sd,
	.chip_before_suspend = elan_before_suspend,
	.chip_get_rawdata = elan_ktf_get_rawdata,
	.chip_get_info = elan_ktf_get_info,
	.chip_get_capacitance_test_type = elan_chip_get_capacitance_test_type,
	.chip_special_rawdata_proc_printf = rawdata_proc_elan_printf,
	.chip_shutdown = elan_chip_shutdown,
 };

static int tp_module_test_init(struct ts_rawdata_info* info)
{
	int ret=0;
	if (!info) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	ret=i2c_communicate_check();
	if (ret) {
		TS_LOG_ERR("[elan]:i2c Connet Test Fail!\n");
		strncat(info->result,"0F-",strlen("0F-"));
		return ret;
	}else{
		TS_LOG_INFO("[elan]:i2c Connet Test Pass!\n");
		strncat(info->result,"0P-",strlen("0P-"));
	}

	ret=alloc_data_buf();
	if (ret) {
		TS_LOG_ERR("[elan]:alloc data buf fail!\n");
		return ret;
	}

	info->buff[0]=elan_ts->rx_num;
	info->buff[1]=elan_ts->tx_num;
	info->used_size += 2;
	ret = disable_finger_report();
	if (ret) {
		TS_LOG_ERR("[elan]:disable_finger_report fail!\n");
		return ret;
	}
	ret=elan_get_set_opcmd();
	if (ret) {
		TS_LOG_ERR("[elan]:set op cmd Fail!\n");
		return ret;
	}
	ret=elan_read_ph();
	if (ret) {
		TS_LOG_ERR("[elan]:read ph value Fail!\n");
		return ret;
	}
	ret=elan_calibration_base();
	if (ret) {
		TS_LOG_ERR("[elan]:elan_calibration_base Fail!\n");
		return ret;
	}
	return NO_ERR;
}

static int elan_ktf_get_rawdata(struct ts_rawdata_info* info, struct ts_cmd_node* out_cmd)
{
	int ret=0;
	bool txrx_open_test=true;
	char noise_test_result[ResultMaxLen]={0};
	char txrx_open_test_result[ResultMaxLen]={0};
	char txrx_short_test_result[ResultMaxLen]={0};
	char adc_mean_low_boundary_test_result[ResultMaxLen]={0};

	TS_LOG_INFO("--->>elan module test start<<---\n");
	if ((!info)||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]:%s,info is null\n",__func__);
		return -EINVAL;
	}
	if (atomic_read(&elan_ts->tp_mode)!=TP_NORMAL) {
		TS_LOG_ERR("[elan]:tp is not in normal mode\n");
		return -EINVAL;
	} else {
		atomic_set(&elan_ts->tp_mode,TP_MODULETEST);
	}
	wake_lock(&elan_ts->wake_lock);
	memset(info->result,0,sizeof(info->result));

	/*elan mmi step 1 i2c test and alloc data buf*/
	ret=tp_module_test_init(info);
	if (ret) {
		TS_LOG_ERR("[elan]:tp_module_test_init Fail!\n");
		goto TEST_EXIT;
	}

	/*elan mmi step 2 get noise and test*/
	ret = get_noise_test_data(info);
	if (ret) {
		TS_LOG_ERR("[elan]:Noise Test Fail!\n");
		strncat(noise_test_result,"3F-",sizeof(noise_test_result));
	} else {
		TS_LOG_INFO("[elan]:Noise Test Pass!\n");
		strncat(noise_test_result,"3P-",sizeof(noise_test_result));
	}

	/*elan mmi step 3 rx open test*/
	ret=elan_rx_open_check(info);
	if(ret<0){
		TS_LOG_ERR("[elan]:get normaL adc fail\n");
		goto TEST_EXIT;
	}else if(ret==1){
		txrx_open_test=false;
	}

	/*elan mmi step 4 normal adc mean test*/
	ret=elan_mean_value_check(adc_mean_low_boundary_test_result,info);
	if(ret){
		TS_LOG_ERR("[elan]:get normaL adc[2] fail\n");
		goto TEST_EXIT;
	}

	/*elan mmi step 5 tx open test*/
	ret = elan_tx_open_check();
	if (ret) {
		TS_LOG_ERR("[elan]:tx open test fail!\n");
		txrx_open_test=false;
	} else {
		TS_LOG_INFO("[elan]:tx open test pass!\n");
	}
	if(txrx_open_test){
		strncat(txrx_open_test_result,"2P-",sizeof(txrx_open_test_result));
	} else {
		strncat(txrx_open_test_result,"2F-",sizeof(txrx_open_test_result));
	}

	/*elan mmi step 6 txrx short test*/
	ret=elan_txrx_short_check(txrx_short_test_result,info);
	if(ret){
		TS_LOG_ERR("[elan]:get tx rx short data fail\n");
		goto TEST_EXIT;
	}

TEST_EXIT:
	strncat(info->result,adc_mean_low_boundary_test_result,strlen(adc_mean_low_boundary_test_result));
	strncat(info->result,txrx_open_test_result,strlen(txrx_open_test_result));
	strncat(info->result,noise_test_result,strlen(noise_test_result));
	strncat(info->result,txrx_short_test_result,strlen(txrx_short_test_result));
	strncat(info->result,ELAN_KTF_NAME,sizeof(ELAN_KTF_NAME));
	strncat(info->result,"_",sizeof("_"));
	strncat(info->result,elan_ts->elan_chip_data->chip_name,sizeof(elan_ts->elan_chip_data->chip_name));
	strncat(info->result,"_",sizeof("_"));
	strncat(info->result,elan_ts->project_id,sizeof(elan_ts->project_id));
	free_data_buf();
	wake_unlock(&elan_ts->wake_lock);
	atomic_set(&elan_ts->tp_mode, TP_NORMAL);
	elan_ktf_hw_reset();
	TS_LOG_ERR("[elan]%s,end!,%s\n", __func__,info->result);
	return NO_ERR;
}

int rawdata_proc_elan_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size)
{
	int index = 0;
	int index1 = 0;

	if((0 == range_size) || (0 == row_size) || (!info)) {
		TS_LOG_ERR("%s  range_size OR row_size is 0 OR info is NULL\n", __func__);
		return -EINVAL;
	}

	for (index = 0; row_size * index + 2 < (2+row_size*range_size*3); index++) {
		if (0 == index) {
			seq_printf(m, "noisedata begin\n");	/*print the title */
		}
		for (index1 = 0; index1 < row_size; index1++) {
			seq_printf(m, "%d,", info->buff[2 + row_size * index + index1]);	/*print oneline */
		}
		/*index1 = 0;*/
		seq_printf(m, "\n ");

		if ((range_size - 1) == index) {
			seq_printf(m, "noisedata end\n");
			seq_printf(m, "normal adc begin\n");
		} else if ((range_size*2 - 1) == index){
			seq_printf(m, "normal adc end\n");
			seq_printf(m, "normal adc2 begin\n");
		}
	}
	seq_printf(m, "normal adc2 end\n");
	if (info->used_size > (2+row_size*range_size*3))
	{
		seq_printf(m, "rx short begin\n");
		for (index1 = 0;index1 < (row_size + range_size)*2; index1++)
		{
			seq_printf(m, "%d,", info->buff[2 + row_size * range_size*3 + index1]);
			if (index1 == (row_size - 1)) {
				seq_printf(m, "\n ");
				seq_printf(m, "rx short end\n");
				seq_printf(m, "tx short begin\n");
			} else if (index1 == (row_size + range_size - 1)) {
				seq_printf(m, "\n ");
				seq_printf(m, "tx short end\n");
				seq_printf(m, "rx short2 begin\n");
			} else if (index1 == (row_size*2 + range_size - 1)) {
				seq_printf(m, "\n ");
				seq_printf(m, "rx short2 end\n");
				seq_printf(m, "tx short2 begin\n");
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "tx short2 end\n");
	}
	return NO_ERR;
}

static int elan_chip_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int error = NO_ERR;
	struct elan_ktf_ts_data *data = elan_ts;

	if (!info || !data|| !data->elan_chip_data) {
		TS_LOG_ERR("[elan],%s:info=%ld\n", __func__, PTR_ERR(info));
		error = -ENOMEM;
		return error;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type, data->elan_chip_data->tp_test_type,
		       TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("read_chip_get_test_type=%s, \n",
			    info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid status: %s", info->tp_test_type);
		error = -EINVAL;
		break;
	}
	return error;
}

#ifdef ELAN_IAP
static int elan_iap_open(struct inode *inode, struct file *filp)
{
	TS_LOG_INFO("[elan]into elan_iap_open\n");
	if (!elan_ts) {
		TS_LOG_INFO("elan_ts is NULL\n");
	}
	return NO_ERR;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	int ret=0;
	char *tmp=NULL;
	if(!buff)
	{
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EFAULT;
	}
	TS_LOG_INFO("[elan]into elan_iap_write\n");
	if (count > MAX_WRITE_LEN) {
		count = MAX_WRITE_LEN;
	}
	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL) {
		TS_LOG_ERR("[elan]%s:fail to kmalloc\n",__func__);
		return -ENOMEM;
	}
	if (copy_from_user(tmp, buff, count)) {
		TS_LOG_ERR("[elan]%s:fail to copy_from_user\n",__func__);
		return -EFAULT;
	}

	ret = elan_i2c_write(tmp, ELAN_SEND_DATA_LEN);
	if (ret) {
		TS_LOG_ERR("[elan]%s:fail to i2c_write,ret=%d\n",__func__,ret);
		return -EINVAL;
	}
	kfree(tmp);
	tmp = NULL;
	return (ret <0) ? ret : count;
}

static ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	char *tmp=NULL;
	int ret=0;
	if (!buff) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EFAULT;
	}
	TS_LOG_INFO("[elan]into elan_iap_read\n");
	if (count > MAX_WRITE_LEN) {
		count = MAX_WRITE_LEN;
	}
	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL) {
		TS_LOG_ERR("[elan]%s:fail to kmalloc\n",__func__);
		return -ENOMEM;
	}
	ret = elan_i2c_read(NULL,0, tmp, count);
	if (ret >= 0) {
		ret = copy_to_user(buff, tmp, count);
		if (ret) {
			TS_LOG_ERR("[elan]%s:copy_to_user fail\n",__func__);
			return -ENOMEM;
		}
	}
	kfree(tmp);
	tmp=NULL;
	return (ret <0) ? ret : count;
}

static long elan_iap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int __user *ip = (int __user *)arg;
	TS_LOG_INFO("[elan]:%s\n",__func__);
	if ((!elan_ts)||(!elan_ts->elan_chip_client)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EFAULT;
	}
	switch (cmd) {
 		case IOCTL_RESET:
			elan_ktf_hw_reset();
			break;
		case IOCTL_IAP_MODE_LOCK:
			atomic_set(&elan_ts->tp_mode,TP_FWUPDATA);
			TS_LOG_INFO("[elan]:disable irq=%d\n",elan_ts->irq_id);
			disable_irq(elan_ts->irq_id);
			break;
		case IOCTL_FW_VER:
			return elan_ts->fw_ver;
			break;
		case IOCTL_X_RESOLUTION:
			return elan_ts->finger_x_resolution;
			break;
		case IOCTL_Y_RESOLUTION:
			return elan_ts->finger_y_resolution;
			break;
		case IOCTL_FW_ID:
			return elan_ts->fw_id;
			break;
		case IOCTL_IAP_MODE_UNLOCK:
			atomic_set(&elan_ts->tp_mode,TP_NORMAL);
			TS_LOG_INFO("[elan]:enable irq\n");
			enable_irq(elan_ts->irq_id);
			break;
		case IOCTL_I2C_INT:
			put_user(gpio_get_value(elan_ts->int_gpio), ip);
			break;
		default:
			TS_LOG_INFO("[elan]:unknow ioctl cmd!\n");
	}
	return NO_ERR;
}

static const struct file_operations elan_touch_fops = {
	.owner = THIS_MODULE,
	.open = elan_iap_open,
	.write = elan_iap_write,
	.read = elan_iap_read,
	.compat_ioctl = elan_iap_ioctl,
	.unlocked_ioctl = elan_iap_ioctl,
};
#endif

static void elants_reset_pin_low(void)
{
	gpio_set_value(elan_ts->reset_gpio, 0);
	return;
}

static void elants_reset_pin_high(void)
{
	gpio_set_value(elan_ts->reset_gpio, 1);
	return;
}

static int elan_ktf_get_info(struct ts_chip_info_param* info)
{
	struct elan_ktf_ts_data *ts = elan_ts;
	if (!info||!ts||!ts->elan_chip_data) {
		TS_LOG_ERR("[elan]arg is NULL,%s\n",__func__);
		return -EINVAL;
	}
	snprintf(info->ic_vendor, sizeof(info->chip_name),ts->elan_chip_data->chip_name);
	sprintf(info->fw_vendor, "0x%04x", ts->fw_ver);
	snprintf(info->mod_vendor,sizeof(info->mod_vendor), ts->project_id);
	return NO_ERR;
}

static int elan_config_gpio(void)
{
	int err=NO_ERR;
	if (!elan_ts) {
		TS_LOG_ERR("[elan]arg is NULL,%s\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("[elan]:%s enter\n", __func__);
	if (gpio_is_valid(elan_ts->int_gpio)) {
		err = gpio_direction_input(elan_ts->int_gpio);
		if (err) {
			TS_LOG_ERR("[elan]:unable to set direction for gpio [%d]\n",elan_ts->int_gpio);
			return -EINVAL;
		}
	} else {
		TS_LOG_INFO("[elan]:int gpio not provided\n");
	}

	if (gpio_is_valid(elan_ts->reset_gpio))
	{
		err = gpio_direction_output(elan_ts->reset_gpio,0);
		if (err) {
			TS_LOG_ERR("[elan]:unable to set direction for gpio [%d]\n",elan_ts->reset_gpio);
			return -EINVAL;
		}
	} else {
		TS_LOG_ERR("[elan]:int gpio not provided\n");
	}
	return NO_ERR;
}

static inline int elan_ktf_finger_parse_xy(uint8_t *data,uint16_t *x, uint16_t *y)
{
	if (!data||!x||!y) {
		TS_LOG_ERR("[elan]:%s,null point\n",__func__);
		return -EINVAL;
	}
	*x = data[FingerX_Point_HByte];
	*x <<= 8;
	*x |= data[FingerX_Point_LByte];

	*y = data[FingerY_Point_HByte];
	*y <<= 8;
	*y |= data[FingerY_Point_LByte];

	return NO_ERR;
}

static inline int elan_ktf_pen_parse_xy(uint8_t *data,  uint16_t *x, uint16_t *y, uint16_t *p)
{
	if (!data||!x||!y||!p) {
		TS_LOG_ERR("[elan]:%s,null point\n",__func__);
		return -EINVAL;
	}
	*x = data[PenX_Point_HByte];
	*x <<= 8;
	*x |= data[PenX_Point_LByte];

	*y = data[PenY_Point_HByte];
	*y <<= 8;
	*y |= data[PenY_Point_LByte];

	*p = data[Pen_Press_HByte];
	*p <<= 8;
	*p |= data[Pen_Press_LByte];
	return NO_ERR;
}

int elan_i2c_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len)
{
	int rc = 0;
	if ((!elan_ts)||(!elan_ts->elan_chip_data)||(!elan_ts->elan_chip_client)||(!elan_ts->elan_chip_client->bops)) {
		TS_LOG_ERR("[elan]arg is NULL,%s\n",__func__);
		return -EINVAL;
	}
	mutex_lock(&elan_ts->ktf_mutex);
	if (!reg_addr) {
		elan_ts->elan_chip_data->is_i2c_one_byte = true;
	} else {
		elan_ts->elan_chip_data->is_i2c_one_byte = false;
	}
	rc = elan_ts->elan_chip_client->bops->bus_read(reg_addr, reg_len, buf, len);
	if (rc) {
		TS_LOG_ERR("[elan]:%s,fail read	rc=%d\n", __func__, rc);
	}
	mutex_unlock(&elan_ts->ktf_mutex);
	return rc;
}

int elan_i2c_write(u8* buf, u16 length)
{
	int rc = 0;
	if ((!elan_ts)||(!elan_ts->elan_chip_client)||(!elan_ts->elan_chip_client->bops)) {
		TS_LOG_ERR("[elan]arg is NULL,%s\n",__func__);
		return -EINVAL;
	}
	mutex_lock(&elan_ts->ktf_mutex);
	rc =elan_ts->elan_chip_client->bops->bus_write(buf,length);
	if (rc) {
		TS_LOG_ERR("[elan]:%s,fail write rc=%d\n", __func__, rc);
	}
	mutex_unlock(&elan_ts->ktf_mutex);
	return rc;
}

EXPORT_SYMBOL(elan_i2c_read);
EXPORT_SYMBOL(elan_i2c_write);

static int i2c_communicate_check(void)
{
	int rc = NO_ERR;
	int i=0;
	u8 cmd_id[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0xf0,0x00,0x01};
	u8 buf[ELAN_RECV_DATA_LEN] = {0};
	for (i = 0; i < I2C_RW_TRIES; i ++)
	{
		rc = elan_i2c_read(cmd_id,sizeof(cmd_id), buf, sizeof(buf));
		if (rc) {
			TS_LOG_ERR("[elan]:%s, Failed to read tp info,i = %d, rc = %d\n", __func__, i, rc);
			msleep(50);//IC need
		} else {
			TS_LOG_INFO("[elan]:%s, i2c communicate check success,buf[0]=%x\n", \
				__func__,buf[0]); //hid head recv data len
			return NO_ERR;
		}
	}

	return rc;
}

static int elan_ktf_ts_recv_data(u8 *pbuf)
{
	int ret=0,fingernum=0;
	int recv_count=0;
	int recv_count_max=0;
	u8 *buf=pbuf;
	u8 data_buf[REPORT_DATA_LEN]={0};
	if ((!buf)||(!elan_ts)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	ret = elan_i2c_read(NULL,0,buf,REPORT_DATA_LEN);
	if (ret) {
		TS_LOG_ERR("[elan]elan_i2c_read Fail!ret=%d\n",ret);
		return ret;
	}
	if (buf[REPORT_ID_BYTE]==FINGER_REPORT_ID) {
		fingernum = buf[CUR_FINGER_NUM_BYTE];
		if(fingernum > MAX_FINGER_SIZE)
		{
			TS_LOG_ERR("[elan]:invalid finger num\n");
			return -EINVAL;
		}
		elan_ts->cur_finger_num = fingernum;
		recv_count_max = (fingernum/2) + ((fingernum%2) != 0);
		for(recv_count = 1;recv_count < recv_count_max;recv_count++)
		{
			ret = elan_i2c_read(NULL,0,data_buf,REPORT_DATA_LEN);
			if (ret) {
				TS_LOG_ERR("[elan]elan_i2c_read Fail!ret=%d\n",ret);
				return ret;
			}
			memcpy(buf+(REPORT_DATA_LEN-1)*recv_count-POINT_HEAD_LEN*(recv_count-1),data_buf+POINT_HEAD_LEN,REPORT_DATA_LEN-POINT_HEAD_LEN);//not Out of bounds 
		}
	}
	return NO_ERR;
}

static void parse_fingers_point(struct ts_fingers *pointinfo,u8 *pbuf)
{
	int i=0;
	int fid=0;
	int idx=3;//point  start byte
	uint16_t x =0, y =0;
	int lcm_max_x=0;
	int lcm_max_y=0;
	if ((!pointinfo)||(!pbuf)||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return ;
	}
	lcm_max_x=elan_ts->elan_chip_data->x_max;
	lcm_max_y=elan_ts->elan_chip_data->y_max;
	memset(pointinfo,0,sizeof(struct ts_fingers));
	for(i=0;i<elan_ts->cur_finger_num;i++)
	{
		if((pbuf[idx]&0x3)!=0x0)	//bit0 tip bit1 range
		{
			fid=(pbuf[idx]>>2)&0x3f;	//fingerid bit 2-7
			elan_ktf_finger_parse_xy(pbuf+idx, &y, &x);
			if ((elan_ts->finger_x_resolution>0)&&(elan_ts->finger_y_resolution>0)) {
				x=x*lcm_max_x/elan_ts->finger_x_resolution;
				y=y*lcm_max_y/elan_ts->finger_y_resolution;
			}
			pointinfo->fingers[fid].status = TS_FINGER_PRESS;
			pointinfo->fingers[fid].x = lcm_max_x - x;
			pointinfo->fingers[fid].y = y;
			pointinfo->fingers[fid].major = Finger_Major;
			pointinfo->fingers[fid].minor = Finger_Minor;
			pointinfo->fingers[fid].pressure = Finger_Pressure;
		}
		idx += Value_Offset;
	}
}

static void parse_pen_point(struct ts_pens *pointinfo,u8 *pbuf, struct ts_cmd_node *out_cmd)
{
	uint16_t x =0, y =0,p=0;
	int pen_down = pbuf[3]&0x03;   //pbuf[3] bit 0,1 tip and inrange
	int lcm_max_x=0;
	int lcm_max_y=0;
	if ((!pointinfo)||(!pbuf)||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return ;
	}
	lcm_max_x=elan_ts->elan_chip_data->x_max;
	lcm_max_y=elan_ts->elan_chip_data->y_max;
	if (pen_down) {
		elan_ktf_pen_parse_xy(pbuf, &y, &x, &p);
		if ((elan_ts->pen_x_resolution>0)&&(elan_ts->pen_y_resolution>0)) {
			x=x*lcm_max_x/elan_ts->pen_x_resolution;
			y=y*lcm_max_y/elan_ts->pen_y_resolution;
		}
		pointinfo->tool.tip_status = pen_down>>1;
		pointinfo->tool.x = lcm_max_x - x;
		pointinfo->tool.y = y;
		pointinfo->tool.pressure = p;
		pointinfo->tool.pen_inrange_status=pen_down&0x1;
		if(!elan_ts->pen_detected){
			TS_LOG_INFO("[elan]:pen is detected!\n");
			elan_ts->pen_detected = true;
		}
	} else {
		pointinfo->tool.tip_status = 0;
		pointinfo->tool.pen_inrange_status=0;
		TS_LOG_INFO("[elan]:pen is exit!\n");
		elan_ts->pen_detected = false;
	}
	return;
}

static int check_fw_status(void)
{
	int ret = 0;
	u8 checkstatus[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x18};
	u8 buff[ELAN_RECV_DATA_LEN] = {0};
	if (!elan_ts) {
		TS_LOG_ERR("[elan]:%s,elan ts is null\n",__func__);
		return -EINVAL;
	}
	ret = elan_i2c_read(checkstatus,sizeof(checkstatus),buff,sizeof(buff));
	if (ret) {
		TS_LOG_ERR("[elan]:i2c read tp mode fail!ret=%d\n",ret);
		return -EINVAL;
	} else {
		TS_LOG_INFO("[elan]:Tp mode check:%x,%x,%x,%x,%x\n", \
			buff[0],buff[1],buff[2],buff[3],buff[4]);//0-3 hid head 4 th fw status buff
	}

	if (buff[TP_NORMAL_DATA_BYTE]==TP_NORMAL_DATA) {
		atomic_set(&elan_ts->tp_mode,TP_NORMAL);
		return 1;
	} else if (buff[TP_RECOVER_DATA_BYTE]==TP_RECOVER_DATA) {
		atomic_set(&elan_ts->tp_mode,TP_RECOVER);
		return 0;
	} else {
		TS_LOG_ERR("[elan]:unknow mode!\n");
		return -EINVAL;
	}
	return NO_ERR;
}

/*enter updata fw mode*/
static int enter_iap_mode(void)
{
	int ret = 0;
	u8 flash_key[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xc0,0xe1,0x5a};
	u8 isp_cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x00,0x12,0x34};
	u8 check_addr[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x01,0x10};
	u8 buff[ELAN_RECV_DATA_LEN] = {0};
	TS_LOG_INFO("[elan]:%s!!\n",__func__);
	if (!elan_ts) {
		TS_LOG_ERR("[elan]:%s elan_ts is null\n",__func__);
		return -EINVAL;
	}
	ret = elan_i2c_write(flash_key, sizeof(flash_key));
	if (ret) {
		TS_LOG_ERR("[elan]:%s,send flash key fail!ret=%d\n",__func__,ret);
		return -EINVAL;
	}
	if (atomic_read(&elan_ts->tp_mode)==TP_NORMAL) {
		ret = elan_i2c_write(isp_cmd, sizeof(isp_cmd));
		if (ret) {
			TS_LOG_ERR("[elan]:%s,send iap cmd fail!ret=%d\n",__func__,ret);
			return -EINVAL;
		}
	}

	ret = elan_i2c_read(check_addr,sizeof(check_addr), buff,sizeof(buff));
	if (ret) {
		TS_LOG_ERR("[elan]:%s,i2c read data fail!ret=%d\n",__func__,ret);
		return -EINVAL;
	} else {
		TS_LOG_INFO("[elan]:addr data:%x,%x,%x,%x,%x,%x,%x\n", \
			buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],buff[6]);//0-3th hid head 4-6th fw status buff
	}
	return NO_ERR;
}

static int get_ack_data(void)
{
	int res=0;
	u8 buff[ELAN_RECV_DATA_LEN] = {0};
	u8 ack_buf[2]={IC_ACK,IC_ACK};
	res = elan_i2c_read(NULL,0, buff, sizeof(buff));
	if (res) {
		TS_LOG_ERR("[elan]:read ack data fail!res=%d\n",res);
		return -EINVAL;
	} else {
		TS_LOG_INFO("[elan]:ack data:%x,%x,%x,%x,%x,%x\n", \
			buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);	//0-3 hid head data 4&5th ic ack
	}
	res = memcmp(buff+4,ack_buf,sizeof(ack_buf));	//4-5th ack (0xaa)
	if (res) {
		TS_LOG_ERR("[elan]:ack not right!\n");
		return -EINVAL;
	}
	return NO_ERR;
}

static int SendFinishCmd(void)
{
	int len = 0;
	u8 finish_cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x1A};
	len = elan_i2c_write(finish_cmd, sizeof(finish_cmd));
	if (len) {
		TS_LOG_ERR("[elan]:send updata fw finish cmd fail!len=%d\n",len);
	}
	return len;
}

static int elan_ktf_ts_calibrate(void)
{
	int ret;
	u8 flash_key[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0xc0,0xe1,0x5a};
	u8 cal_cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x29,0x00,0x01};
	ret = elan_i2c_write(flash_key,sizeof(flash_key));
	if (ret) {
		TS_LOG_ERR("[elan]:send flash key cmd fail!ret=%d\n",ret);
		return -EINVAL;
	}
	ret = elan_i2c_write(cal_cmd,sizeof(cal_cmd));
	if (ret) {
		TS_LOG_ERR("[elan]:send calibrate cmd fail!ret=%d\n",ret);
		return -EINVAL;
	}
	return NO_ERR;
}

static int hid_write_page(int pagenum,const u8* fwdata )
{
	int write_times=0;
	int ipage=0;
	int offset=0;
	int byte_count=0;
	int curIndex = 0;
	int res=0;
	int lastdatalen=0;
	const u8 *szBuff = NULL;
	u8 write_buf[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x21,0x00,0x00,0x28};
	u8 cmd_iap_write[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x22};
	int lastpages=(int)(pagenum/WritePages)*WritePages+1;
	if (!fwdata) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	TS_LOG_DEBUG("[elan]:lastpages=%d\n",lastpages);
	for(ipage=1;ipage<=pagenum;ipage+=WritePages)
	{
		offset=0;
		if (ipage==lastpages) {
			write_times=(int)((pagenum-lastpages+1)*FwPageSize/WriteDataValidLen)+
				((pagenum-lastpages+1)*FwPageSize%WriteDataValidLen != 0);
			lastdatalen=(pagenum-lastpages+1)*FwPageSize%WriteDataValidLen;
		} else {
			write_times=(int)(WritePages*FwPageSize/WriteDataValidLen)+(WritePages*FwPageSize%WriteDataValidLen!=0);
			lastdatalen=WritePages*FwPageSize%WriteDataValidLen;
		}
		TS_LOG_DEBUG("[elan]:update fw write_times=%d,lastdatalen=%d\n",write_times,lastdatalen);

		for(byte_count=1;byte_count<=write_times;byte_count++){
			szBuff = fwdata + curIndex;
			write_buf[OFFSET_LBYTE] = offset & 0x00ff;
			write_buf[OFFSET_HBYTE] = offset >> 8;

			if (byte_count!=write_times) {
				write_buf[WriteDataValidLen_Byte] = WriteDataValidLen;
				offset += WriteDataValidLen;
				curIndex =  curIndex + WriteDataValidLen;
				memcpy(write_buf+9,szBuff,WriteDataValidLen); //0-8 is write head
			} else {
				write_buf[WriteDataValidLen_Byte] = lastdatalen;
				curIndex =  curIndex + lastdatalen;
				memcpy(write_buf+9,szBuff,lastdatalen);	//0-8 is write head
			}
			res=elan_i2c_write(write_buf, ELAN_SEND_DATA_LEN);
			if (res) {
				TS_LOG_ERR("[elan]:updata fw write page fail!res=%d\n",res);
				return -EINVAL;
			}
		}
		msleep(10);// fw updata need
		res = elan_i2c_write(cmd_iap_write,ELAN_SEND_DATA_LEN);
		if (res) {
			TS_LOG_ERR("[elan]:write cmd_iap_write fail,res=%d\n",res);
			return -EINVAL;
		}
		msleep(200);// fw updata need
		res = get_ack_data();
		if (res) {
			TS_LOG_ERR("[elan]get_ack_data Fail\n");
			return -EINVAL;
		}
		mdelay(10);	// fw updata need
	}
	res = SendFinishCmd();
	if (res) {
		TS_LOG_ERR("[elan]SendFinishCmd Fail\n");
		return -EINVAL;
	}
	return NO_ERR;
}

static int elan_firmware_update(const struct firmware *fw )
{
	int res=0;
	int retry_num=0;
	const u8 *fw_data=NULL;
	int fw_size=0;
	int PageNum=0;
	if(!fw)
	{
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	fw_data = fw->data;
	fw_size = fw->size;
	PageNum = (fw_size/sizeof(uint8_t)/FwPageSize);
	TS_LOG_INFO("[elan]:fw pagenum=%d\n",PageNum);
retry_updata:
	elan_ktf_hw_reset();
	res = check_fw_status();
	if(res<0)
	{
		TS_LOG_ERR("[elan]:tp is on unknow mode!\n");
		return -EINVAL;
	}
	res=enter_iap_mode();
	if (res) {
		TS_LOG_ERR("[elan]:enter updata fw mode fail!\n");
		return -EINVAL;
	}
	msleep(10);	// fw updata need
	res=hid_write_page(PageNum,fw_data);
	if (res) {
		retry_num++;
		if (retry_num >= Fw_Update_Retry) {
			TS_LOG_ERR("[elan]:retry updata fw fail!\n");
			return -EINVAL;
		} else {
			TS_LOG_INFO("[elan]:fw updata fail!,retry num=%d\n",retry_num);
			goto retry_updata;
		}
	}
	msleep(10);// fw updata finish need
	elan_ktf_hw_reset();
	res=elan_read_fw_info();
	if (res) {
		TS_LOG_ERR("[elan]:%s,read fw info fail\n",__func__);
	}
	res=elan_read_tx_rx();
	if(res==NO_ERR)
	{
		elan_ts->finger_y_resolution = (elan_ts->rx_num - 1)*FINGER_OSR;
		elan_ts->finger_x_resolution = (elan_ts->tx_num - 1)*FINGER_OSR;
		elan_ts->pen_y_resolution = (elan_ts->rx_num - 1)*PEN_OSR;
		elan_ts->pen_x_resolution = (elan_ts->tx_num - 1)*PEN_OSR;
	}
	res = elan_ktf_ts_calibrate();
	if (res) {
		TS_LOG_ERR("[elan]:%s,calibrate fail\n",__func__);
	}
	return NO_ERR;
}

static int elan_get_lcd_panel_info(void)
{
	struct device_node *dev_node = NULL;
	char *lcd_type = NULL;

	dev_node = of_find_compatible_node(NULL, NULL, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
	if (!dev_node) {
		TS_LOG_ERR("[elan]:%s: NOT found device node[%s]!\n", __func__, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
		return -EINVAL;
	}

	lcd_type = (char*)of_get_property(dev_node, "lcd_panel_type", NULL);
	if(!lcd_type){
		TS_LOG_ERR("[elan]:%s: Get lcd panel type faile!\n", __func__);
		return -EINVAL ;
	}

	strncpy(elan_ts->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("[elan]:lcd_panel_info = %s.\n", elan_ts->lcd_panel_info);

	return 0;
}

static int  elan_get_lcd_module_name(void)
{
	char temp[LCD_PANEL_INFO_MAX_LEN] = {0};
	int i = 0;

	strncpy(temp, elan_ts->lcd_panel_info, LCD_PANEL_INFO_MAX_LEN-1);
	for(i=0;i<(LCD_PANEL_INFO_MAX_LEN-1) && (i < (MAX_STR_LEN-1));i++)
	{
		if(temp[i] == '_') {
			break;
		}
		elan_ts->lcd_module_name[i] = tolower(temp[i]);
	}
	TS_LOG_INFO("[elan]:lcd_module_name = %s.\n", elan_ts->lcd_module_name);

	return 0;
}

static int elan_ktf_fw_update_sd(void)
{
	char filename[MAX_NAME_LEN]={0};
	if ((!elan_ts)||(!elan_ts->elan_chip_client)) {
		TS_LOG_ERR("[elan]elan_ts is NULL\n");
		return -EINVAL;
	}

	strncat(filename,elan_ts->elan_chip_client->product_name,sizeof(elan_ts->elan_chip_client->product_name));
	strncat(filename,"_",strlen("_"));
	strncat(filename,ELAN_IC_NAME,strlen(ELAN_IC_NAME));

	TS_LOG_INFO("[elan]:%s, enter\n", __func__);
	elan_ts->sd_fw_updata=true;
	elan_ktf_fw_update_boot(filename);
	elan_ts->sd_fw_updata=false;
	TS_LOG_INFO("[elan]:%s, end\n", __func__);
	return NO_ERR;
}

static int elan_ktf_fw_update_boot(char *file_name)
{
	int err = NO_ERR;
	int New_Fw_Ver=0;
	char file_path[MAX_NAME_LEN]={0};
	const  struct firmware *fw_entry = NULL;
	const u8 *fw_data=NULL;
	if ((!file_name)||(!elan_ts)||(!elan_ts->elan_dev)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("[elan]:%s: enter!\n", __func__);
	err = elan_get_lcd_panel_info();
	if (err < 0) {
		TS_LOG_ERR("[elan]:elan_get_lcd_panel_info fail\n");
		return -EINVAL;
	}
	elan_get_lcd_module_name();
	strncat(file_name,elan_ts->project_id,sizeof(elan_ts->project_id));
	strncat(file_name,"_",strlen("_"));
	strncat(file_name,elan_ts->lcd_module_name,strlen(elan_ts->lcd_module_name));
	strncat(file_name,FW_SUFFIX,strlen(FW_SUFFIX));
	snprintf(file_path,strlen(file_name)+strlen("ts/")+1,"ts/%s",file_name);
	TS_LOG_INFO("[elan]:start to request firmware %s,%s\n", file_name,file_path);

	err = request_firmware(&fw_entry,file_path,&elan_ts->elan_dev->dev);
	if (err < 0) {
		TS_LOG_ERR("[elan]:%s %d: Fail request firmware %s, retval = %d\n", \
					__func__, __LINE__, file_path, err);
		goto exit;
	}
	fw_data = fw_entry->data;
	New_Fw_Ver=fw_data[FWVER_HIGH_BYTE_IN_EKT]<<8|fw_data[FWVER_LOW_BYTE_IN_EKT];
	TS_LOG_INFO("[elan]:new fw ver=%x\n",New_Fw_Ver);

	if(New_Fw_Ver!=(elan_ts->fw_ver)||elan_ts->sd_fw_updata)
	{
		atomic_set(&elan_ts->tp_mode,TP_FWUPDATA);
		wake_lock(&elan_ts->wake_lock);
		err=elan_firmware_update(fw_entry);
		if (err) {
			TS_LOG_ERR("[elan]:updata fw fail!\n");
			atomic_set(&elan_ts->tp_mode,TP_RECOVER);
		} else {
			TS_LOG_DEBUG("[elan]:updata fw success!\n");
			atomic_set(&elan_ts->tp_mode,TP_NORMAL);
		}
		wake_unlock(&elan_ts->wake_lock);
	} else {
		TS_LOG_INFO("[elan]:fw ver is new don't need updata!\n");
	}
	TS_LOG_INFO("[elan]:%s: end!\n", __func__);
	release_firmware(fw_entry);
exit:
	return NO_ERR;
}

static int elan_ktf_parse_dts_power(struct device_node *device, struct ts_kit_device_data *chip_data)
{
	int slave_addr=0;
	int ret=NO_ERR;
	if ((!device)||(!chip_data)||(!elan_ts)||(!elan_ts->elan_chip_data)||(!elan_ts->elan_chip_client->client)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	ret = of_property_read_u32(device,ELAN_SLAVE_ADDR,&slave_addr);
	if (ret) {
		elan_ts->elan_chip_client->client->addr = ELAN_I2C_ADRR;
	} else {
		elan_ts->elan_chip_client->client->addr = slave_addr;
	}
	TS_LOG_INFO("[elan]:tp ic slave_addr=%x\n",slave_addr);
	/*vci_power_type*/
	ret = of_property_read_u32(device, "vci_power_type",&chip_data->vci_regulator_type);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define vci_power_type in Dts\n");
	}
	TS_LOG_INFO("[elan]:%s, regulator_ctr.vci_regulator_type = %d \n", __func__, chip_data->vci_regulator_type);

	/*vddio_power_type*/
	ret = of_property_read_u32(device, "vddio_power_type",&chip_data->vddio_regulator_type);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define vddio_power_type in Dts\n");
	}
	TS_LOG_INFO("[elan]:%s,vddio_regulator_type = %d \n", __func__, chip_data->vddio_regulator_type);
	/*vci_ldo_value*/
	ret = of_property_read_u32(device, "vci_ldo_value",&chip_data->regulator_ctr.vci_value);
	if (ret) {
		TS_LOG_ERR("[elan]:get vci_ldo_value failed\n");
	}
	TS_LOG_INFO("[elan]:%s, regulator_ctr.vci_value = %d \n", __func__, chip_data->regulator_ctr.vci_value);

	/*vddio_ldo_need_set*/
	ret = of_property_read_u32(device, "vddio_ldo_need_set",&chip_data->regulator_ctr.need_set_vddio_value);
	if (ret) {
		TS_LOG_ERR("[elan]:get vddio_ldo_need_set failed\n");
	}
	TS_LOG_INFO("[elan]:%s, regulator_ctr.need_set_vddio_value = %d \n", __func__, chip_data->regulator_ctr.need_set_vddio_value);
	if (chip_data->regulator_ctr.need_set_vddio_value) {
		/*vddio_ldo_value*/
		ret = of_property_read_u32(device, "vddio_ldo_value",&chip_data->regulator_ctr.vddio_value);
		if (ret) {
			TS_LOG_ERR("[elan]:get vddio_ldo_value failed\n");
		}
		TS_LOG_INFO("[elan]:%s, regulator_ctr.vddio_value = %d \n", __func__, chip_data->regulator_ctr.vddio_value);
	}
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		elan_ts->elan_chip_data->vci_gpio_ctrl = of_get_named_gpio(device, "vci_gpio_enable", 0);
	}

	if (VDD_GPIO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		elan_ts->elan_chip_data->vddio_gpio_ctrl =of_get_named_gpio(device, "vddio_gpio_enable", 0);
	}

	return NO_ERR;
}

static int elan_ktf_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data)
{
	int ret = NO_ERR;
	char *chipname = NULL;
	TS_LOG_INFO("[elan]:%s: call!\n", __func__);
	if ((!device)||(!chip_data)||(!elan_ts)||(!elan_ts->elan_chip_client)) {
		TS_LOG_ERR("[elan]:%s,null point\n",__func__);
		return -EINVAL;
	}
	mutex_init(&chip_data->device_call_lock);
	elan_ts->elan_chip_data->is_direct_proc_cmd=false;

	ret = of_property_read_u32(device, "provide_panel_id_suppot", &chip_data->provide_panel_id_support);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define provide_panel_id_suppot in Dts\n");
	} else {
		TS_LOG_INFO("[elan]:provide_panel_id_support = %d\n", chip_data->provide_panel_id_support);
	}

	ret = of_property_read_u32(device,"elan,irq_config",&chip_data->irq_config);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define irq_config in Dts\n");
	} else {
		TS_LOG_INFO("[elan]:get elan irq_config = %d\n", chip_data->irq_config);
	}

	ret = of_property_read_u32(device, "rawdata_timeout", &chip_data->rawdata_get_timeout);
	if (ret) {
		chip_data->rawdata_get_timeout=ELAN_GET_RAWDATA_TIMEOUT;
		TS_LOG_ERR("[elan]:Not define rawdata_get_timeout in Dts\n");
	}

	ret = of_property_read_u32(device, "elan,abs_max_x", &chip_data->x_max);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define abs_max_x in Dts\n");
	}

	ret = of_property_read_u32(device, "elan,abs_max_y", &chip_data->y_max);
	if (ret) {
		TS_LOG_ERR("[elan]:Not define abs_max_y in Dts\n");
	}
	//pen support
	ret = of_property_read_u32(device, "ktf_pen_support", (u32*)&elan_ts->elan_chip_client->feature_info.pen_info.pen_supported);
	if (ret) {
		TS_LOG_ERR("[elan]:get pen supported failed\n");
		elan_ts->elan_chip_client->feature_info.pen_info.pen_supported= 1;
	}
	TS_LOG_INFO("[elan]:pen supported value is  0x%x, (1:support, 0: not supported)\n", \
		elan_ts->elan_chip_client->feature_info.pen_info.pen_supported);

	ret = of_property_read_string(device, "chip_name", (const char**)&chipname);
	if (ret) {
		strncpy(chip_data->chip_name,ELAN_IC_NAME,strlen(ELAN_IC_NAME)+1);
		TS_LOG_ERR("[elan]:Not define chipname in Dts,use default\n");
	} else {
		strncpy(chip_data->chip_name, chipname, CHIP_NAME_LEN);
	}
	TS_LOG_INFO("[elan]:get elan_chipname = %s\n",chip_data->chip_name);

	ret = of_property_read_string(device, "tp_test_type",(const char **)&chip_data->tp_test_type);
	if (ret) {
		TS_LOG_INFO("[elan]:get device tp_test_type not exit,use default value\n");
		strncpy(chip_data->tp_test_type,"Normalize_type:judge_different_reslut",TS_CAP_TEST_TYPE_LEN);
	}

	ret = of_property_read_u32(device, ELAN_TP_IC_TYPE, &chip_data->ic_type);
	if (ret) {
		chip_data->ic_type = ONCELL;
		TS_LOG_INFO("%s,not define device ic_type in dts\n",__func__);
	}
	g_tskit_ic_type = chip_data->ic_type;
	TS_LOG_INFO("[elan]:%s,get g_tskit_ic_type = %d.\n",__func__, g_tskit_ic_type);
	return NO_ERR;
}

static int elan_regulator_get(void)
{
	if((!elan_ts)||(!elan_ts->elan_chip_data))
	{
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("%s,\n",__func__);
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		elan_ts->vdda = regulator_get(&elan_ts->elan_dev->dev, "elan_vci");
		if (IS_ERR(elan_ts->vdda)) {
			TS_LOG_ERR("[elan]:%s, regulator tp vdda not used\n",__func__);
			return -EINVAL;
		}
	}
	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		elan_ts->vddd = regulator_get(&elan_ts->elan_dev->dev,"elan_vddio");
		if (IS_ERR(elan_ts->vddd)) {
			TS_LOG_ERR("[elan]:%s, regulator tp vddd not used\n",__func__);
			goto ts_vci_out;
		}
	}
	return NO_ERR;
ts_vci_out:
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if (!IS_ERR(elan_ts->vdda)) {
			regulator_put(elan_ts->vdda);
		}
	}
	return NO_ERR;
}

static int elan_power_gpio_request(void)
{
	int retval = NO_ERR;
	if ((!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("%s enter power_type =%d\n", __func__,elan_ts->elan_chip_data->vddio_regulator_type);
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		retval = gpio_request(elan_ts->elan_chip_data->vci_gpio_ctrl, "elan_vci_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR("[elan]:request vci gpio fail:vci_gpio=%d\n",elan_ts->elan_chip_data->vci_gpio_ctrl);
			return -EINVAL;
		}
	}
	if (VDD_GPIO_TYPE ==  elan_ts->elan_chip_data->vddio_regulator_type) {
		retval = gpio_request(elan_ts->elan_chip_data->vddio_gpio_ctrl, "elan_vddio_gpio_ctrl");
		if (retval) {
			TS_LOG_ERR("[elan]:unable to request vddio_gpio_ctrl:%d\n",
				  elan_ts->elan_chip_data->vddio_gpio_ctrl);
			goto ts_vddio_out;
		}
	}
	return NO_ERR;
ts_vddio_out:
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		gpio_free(elan_ts->elan_chip_data->vci_gpio_ctrl);
	}
	return -EINVAL;
}

static void elan_regulator_put(void)
{
	if ((!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return;
	}
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if (!IS_ERR(elan_ts->vdda)) {
			regulator_put(elan_ts->vdda);
		}
	}
	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (!IS_ERR(elan_ts->vddd)) {
			regulator_put(elan_ts->vddd);
		}
	}
}

static int elan_set_voltage(void)
{
	int rc = 0;
	if ((!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}

	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		/*set voltage for vddd and vdda*/
		rc = regulator_set_voltage(elan_ts->vdda, elan_ts->elan_chip_data->regulator_ctr.vci_value, \
		elan_ts->elan_chip_data->regulator_ctr.vci_value);
		if (rc < 0) {
			TS_LOG_ERR("[elan]:%s, failed to set elan vdda to %d, regulator_ctr.vci_value = %dV,p=%p\n", \
				__func__, rc,elan_ts->elan_chip_data->regulator_ctr.vci_value,elan_ts->vdda);
			return -EINVAL;
		}
	}

	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type){
		rc = regulator_set_voltage(elan_ts->vddd, elan_ts->elan_chip_data->regulator_ctr.vddio_value, \
			elan_ts->elan_chip_data->regulator_ctr.vddio_value);
		if (rc < 0) {
			TS_LOG_ERR("[elan]:%s, failed to set elan vddd to %dV, rc = %d\n", \
				__func__, rc,elan_ts->elan_chip_data->regulator_ctr.vddio_value);
			return -EINVAL;
		}
	}
	return NO_ERR;
}

static int elan_power_on(void)
{
	int rc = 0;
	TS_LOG_INFO("[elan]:%s, power control by touch ic\n",__func__);
	if ((!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
	atomic_set(&last_pen_inrange_status, TS_PEN_OUT_RANGE);
	(void)ts_event_notify(TS_PEN_OUT_RANGE); // notify pen out of range
	/*set voltage for vddd and vdda*/
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if (!IS_ERR(elan_ts->vdda)) {
			TS_LOG_INFO("[elan]:vdda enable is called\n");
			rc = regulator_enable(elan_ts->vdda);
			if (rc < 0) {
				TS_LOG_ERR("[elan]:%s, failed to enable elan vdda, rc = %d\n", __func__, rc);
				return -EINVAL;
			}
		}
	} else if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		TS_LOG_INFO("[elan]:%s vdda switch gpio on\n", __func__);
		gpio_direction_output(elan_ts->elan_chip_data->vci_gpio_ctrl, 1);
	}

	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (!IS_ERR(elan_ts->vddd)) {
			TS_LOG_INFO("[elan]:vdda enable is called\n");
			rc = regulator_enable(elan_ts->vddd);
			if (rc < 0) {
				TS_LOG_ERR("[elan]:%s, failed to enable elan vddd, rc = %d\n", __func__, rc);
				return -EINVAL;
			}
		}
	}
	if(VDD_GPIO_TYPE == elan_ts->elan_chip_data->vddio_gpio_type) {
		TS_LOG_INFO("[elan]:%s vddd switch gpio on\n", __func__);
		gpio_direction_output(elan_ts->elan_chip_data->vddio_gpio_ctrl, 1);
	}
	return NO_ERR;
}

static int elan_before_suspend(void)
{
	return NO_ERR;
}

static int elan_ktf_chip_detect(struct ts_kit_platform_data *platform_data)
{
 	int ret = NO_ERR;
	TS_LOG_INFO("[elan]:%s enter!\n", __func__);

	if (((!platform_data)||(!platform_data->ts_dev))||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]:%s device, ts_kit_platform_data *data or data->ts_dev is NULL \n", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	mutex_init(&elan_ts->ktf_mutex);
	elan_ts->elan_dev=platform_data->ts_dev;
	elan_ts->elan_dev->dev.of_node = elan_ts->elan_chip_data->cnode;
	elan_ts->elan_chip_client=platform_data;

	elan_ts->elan_chip_data->is_new_oem_structure = 0;
	elan_ts->elan_chip_data->is_parade_solution = 0;
	elan_ts->elan_chip_data->is_ic_rawdata_proc_printf = 1;
	elan_ts->reset_gpio=platform_data->reset_gpio;
	elan_ts->int_gpio=platform_data->irq_gpio;
	elan_ts->irq_id=gpio_to_irq(elan_ts->int_gpio);

	ret = elan_ktf_parse_dts_power(elan_ts->elan_chip_data->cnode,elan_ts->elan_chip_data);
	if (ret) {
		TS_LOG_ERR("[elan]:parse_dts for DT err\n");
		goto exit;
	}

	ret = elan_regulator_get();
	if (ret) {
		TS_LOG_ERR("[elan]:%s, error request power vdda vddd\n", __func__);
		goto exit;
	}

	ret = elan_power_gpio_request();
	if (ret) {
		TS_LOG_ERR("[elan]:%s, error request gpio for vci and vddio\n", __func__);
		goto regulator_err;
	}

	ret = elan_config_gpio();
	if (ret) {
		TS_LOG_ERR("[elan]:gpio config fail!ret=%d\n",ret);
		goto free_power_gpio;
	}

	ret = elan_set_voltage();
	if (ret) {
		TS_LOG_ERR("%s, failed to set voltage, rc = %d\n", __func__, ret);
		goto free_power_gpio;
	}

	ret = elan_power_on();
	if (ret) {
		TS_LOG_ERR("[elan]:%s, failed to enable power, rc = %d\n", __func__, ret);
		goto power_off;
	}
	msleep(1);//spec need
	elan_ktf_hw_reset();
	ret = i2c_communicate_check();
	if (ret) {
		TS_LOG_ERR("[elan]:%s:not find elan tp device, ret=%d\n", __func__, ret);
		goto power_off;
	} else {
		TS_LOG_INFO("[elan]:%s:find elan tp device\n", __func__);
	}
	TS_LOG_INFO("[elan]:%s:elan chip detect success\n", __func__);
	ret = elan_ktf_parse_dts(elan_ts->elan_chip_data->cnode,elan_ts->elan_chip_data);
	if (ret) {
		TS_LOG_ERR("[elan]:elan_ktf_parse_dts fail\n");
		goto power_off;
	}

	return NO_ERR;
power_off:
	elants_reset_pin_low();
	mdelay(2);//spec need
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if(elan_ts->elan_chip_data->vci_gpio_ctrl) {
			gpio_direction_output(elan_ts->elan_chip_data->vci_gpio_ctrl, 0);
		}
	}
	if (VDD_GPIO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (elan_ts->elan_chip_data->vddio_gpio_ctrl) {
			gpio_direction_output(elan_ts->elan_chip_data->vddio_gpio_ctrl, 0);
		}
	}
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if (!IS_ERR(elan_ts->vdda)) {
			regulator_disable(elan_ts->vdda);
		}
	}
	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (!IS_ERR(elan_ts->vddd)) {
			regulator_disable(elan_ts->vddd);
		}
	}
free_power_gpio:
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if(elan_ts->elan_chip_data->vci_gpio_ctrl) {
			gpio_free(elan_ts->elan_chip_data->vci_gpio_ctrl);
		}
	}
	if (VDD_GPIO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (elan_ts->elan_chip_data->vddio_gpio_ctrl) {
			gpio_free(elan_ts->elan_chip_data->vddio_gpio_ctrl);
		}
	}
regulator_err:
	elan_regulator_put();
exit:
	TS_LOG_INFO("[elan]:%s:elan chip detect fail\n", __func__);
	if (elan_ts->elan_chip_data) {
		kfree(elan_ts->elan_chip_data);
		elan_ts->elan_chip_data = NULL;
	}

	if (elan_ts) {
		kfree(elan_ts);
		elan_ts = NULL;
	}
	return ret;
}

static int elan_project_color(void)
{
	int ret=0,i=0;
	u8 rsp_buf[ELAN_RECV_DATA_LEN]={0};
	u8 test_cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x55,0x55,0x55,0x55};
	u8 project_cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x06,0x59,0x00,0x80,0x80,0x00,0x40};
	if (!elan_ts) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
	ret = elan_i2c_write(test_cmd,sizeof(test_cmd));
	if (ret) {
		TS_LOG_ERR("[elan]:set test cmd fail!ret=%d\n",ret);
		return -EINVAL;
	}
	msleep(15);//IC need
	ret = elan_i2c_write(project_cmd,sizeof(project_cmd));
	if (ret) {
		TS_LOG_ERR("[elan]:elan_i2c_write project_cmd fail!ret=%d\n",ret);
		return -EINVAL;
	}
	for (i = 0;i < PROJECT_ID_POLL;i ++) {
		msleep(10);//IC need
		if (gpio_get_value(elan_ts->int_gpio) == 0) {
			TS_LOG_INFO("[elan]:int is low!i=%d",i);
			break;
		} else {
			TS_LOG_INFO("[elan]:int is high!");
		}
	}
	ret = elan_i2c_read(NULL,0,rsp_buf,ELAN_RECV_DATA_LEN);
	if (ret) {
		TS_LOG_ERR("[elan]:i2c read data fail!\n");
		return -EINVAL;
	}
	memcpy(elan_ts->project_id,rsp_buf+PROJECT_ID_INDEX,sizeof(elan_ts->project_id)-1);//reserved 1 byte
	memcpy(elan_ts->color_id,rsp_buf+COLOR_ID_INDEX,sizeof(elan_ts->color_id));
	TS_LOG_INFO("[elan]:project_id=%s,color_id=%x\n",elan_ts->project_id,elan_ts->color_id[0]);
	memcpy(elan_ts->elan_chip_data->module_name,elan_ts->project_id,sizeof(elan_ts->project_id)-1);//reserved 1 byte
	TS_LOG_INFO("[elan]:module_name=%s\n",elan_ts->elan_chip_data->module_name);
	cypress_ts_kit_color[0]=rsp_buf[COLOR_ID_INDEX];
	elan_ktf_hw_reset();
	return NO_ERR;
}

static int elan_read_fw_info(void)
{
	int ret=0;
	int highbyte=0,lowbyte=0;
	char ver_byte[2]={0};
	u8 cmd_ver[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x53,0x00,0x00,0x01}; /*Get firmware version*/
	u8 resp_buf[ELAN_RECV_DATA_LEN]={0};
	if ((!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
	ret = elan_i2c_read(cmd_ver, sizeof(cmd_ver),resp_buf, sizeof(resp_buf));
	if (ret) {
		TS_LOG_ERR("[elan]:get fw ver fail!ret=%d\n",ret);
		return -EINVAL;
	} else {
		TS_LOG_DEBUG("[elan]:buf[0]=%x,buf[1]=%x,buf[2]=%x,buf[3]=%x,buf[4]=%x\n", \
			resp_buf[0],resp_buf[1],resp_buf[2],resp_buf[3],resp_buf[4]);
	}
	highbyte=((resp_buf[FW_INFO_INDEX]&0x0f)<<4)|((resp_buf[FW_INFO_INDEX+1]&0xf0)>>4);
	lowbyte=((resp_buf[FW_INFO_INDEX+1]&0x0f)<<4)|((resp_buf[FW_INFO_INDEX+2]&0xf0)>>4);
	elan_ts->fw_ver=highbyte<<8|lowbyte;
	ver_byte[0]=(char)highbyte;
	ver_byte[1]=(char)lowbyte;
	snprintf(elan_ts->elan_chip_data->version_name,MAX_STR_LEN,"0x%02x%02x",ver_byte[0],ver_byte[1]);
	TS_LOG_INFO("[elan]:FW VER=0x%04x\n",elan_ts->fw_ver);
	return NO_ERR;
}

static int elan_read_tx_rx(void)
{
	int ret=0;
	u8 info_buff[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x5b,0x00,0x00,0x00,0x00,0x00};
	u8 resp_buf[ELAN_RECV_DATA_LEN]={0};
	if (!elan_ts) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}

	ret = elan_i2c_read(info_buff, sizeof(info_buff),resp_buf, sizeof(resp_buf));
	if (ret) {
		TS_LOG_ERR("[elan]:get tp tx rx num fail!ret=%d\n",ret);
		return -EINVAL;
	} else {
		TS_LOG_INFO("[elan]:buf[6]=%x,buf[7]=%x\n", resp_buf[RX_NUM_INDEX],resp_buf[TX_NUM_INDEX]);  //6th rx num byte 7th tx num byte
	}
	elan_ts->rx_num=resp_buf[RX_NUM_INDEX];
	elan_ts->tx_num=resp_buf[TX_NUM_INDEX];
	if(elan_ts->rx_num<=0||elan_ts->tx_num<=0){
		TS_LOG_ERR("[elan]:tp tx rx invalid\n");
		return -EINVAL;
	}
	TS_LOG_INFO("[elan]:rx=%d,tx=%d\n",elan_ts->rx_num, elan_ts->tx_num);
	return NO_ERR;
}

static int elan_ktf_init_chip(void)
{
	int ret=NO_ERR;
	TS_LOG_INFO("[ELAN]:%s\n", __func__);
	if ((!elan_ts)||(!elan_ts->elan_chip_client)) {
		TS_LOG_ERR("[elan]%s:elan_ts is NULL\n",__func__);
		return -EINVAL;
	}
#ifdef ELAN_IAP
	elan_ts->elan_device.minor = MISC_DYNAMIC_MINOR;
	elan_ts->elan_device.name = "elan-iap";
	elan_ts->elan_device.fops = &elan_touch_fops;
	ret = misc_register(&elan_ts->elan_device);
	if (ret < 0) {
		TS_LOG_ERR("[elan]:misc_register failed!!\n");
	} else {
	  	TS_LOG_INFO("[elan]:misc_register finished!!\n");
	}
#endif
	wake_lock_init(&elan_ts->wake_lock,WAKE_LOCK_SUSPEND,"elantp_wake_lock");
	ret=check_fw_status();
	if (ret<0) {
		TS_LOG_ERR("[elan]:ic is unknow mode\n");
		return ret;
	}
	ret=elan_project_color();
	if (ret) {
		TS_LOG_ERR("[elan]:read project id and color fail\n");
		return ret;
	}
	/*provide panel_id for sensor,panel_id is high byte *10 + low byte*/
	elan_ts->elan_chip_client->panel_id = (elan_ts->project_id[ELAN_PANEL_ID_START_BIT] - '0') * 10 + elan_ts->project_id[ELAN_PANEL_ID_START_BIT + 1] - '0';
	TS_LOG_INFO("%s: panel_id=%d\n", __func__, elan_ts->elan_chip_client->panel_id);
	if(atomic_read(&elan_ts->tp_mode)==TP_NORMAL)
	{
		ret=elan_read_fw_info();
		if (ret) {
			TS_LOG_ERR("[elan]:read tp fw fail!\n");
			return -EINVAL;
		}
		ret = elan_read_tx_rx();
		if (ret==NO_ERR) {
			elan_ts->finger_y_resolution = (elan_ts->rx_num - 1)*FINGER_OSR;
			elan_ts->finger_x_resolution = (elan_ts->tx_num - 1)*FINGER_OSR;
			elan_ts->pen_y_resolution = (elan_ts->rx_num - 1)*PEN_OSR;
			elan_ts->pen_x_resolution = (elan_ts->tx_num - 1)*PEN_OSR;
		} else {
			TS_LOG_ERR("[elan]:read tx rx num fail!\n");
			return -EINVAL;
		}
	}
	elan_ts->sd_fw_updata=false;
	elan_ts->pen_detected=false;
	return NO_ERR;
 }

static int elan_ktf_input_config(struct input_dev *input_dev)
{
	if ((!input_dev)||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]%s:arg is NULL\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("[ELAN]:%s enter\n", __func__);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, Finger_Major, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, Finger_Minor, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,0,	MAX_FINGER_SIZE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, elan_ts->elan_chip_data->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, elan_ts->elan_chip_data->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_FINGER_SIZE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, MAX_FINGER_PRESSURE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, MAX_FINGER_SIZE, 0, 0);
	return NO_ERR;
}

static int elan_ktf_pen_config(struct input_dev *input_dev )
{
	struct input_dev *input =  input_dev;
	if ((!input_dev)||(!elan_ts)||(!elan_ts->elan_chip_data)) {
		TS_LOG_ERR("[elan]:%s,date is null\n",__func__);
		return -EINVAL;
	}
	TS_LOG_INFO("[elan]:elan_pen_input_config called\n");

	input->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	__set_bit(ABS_X, input->absbit);
	__set_bit(ABS_Y, input->absbit);
	//__set_bit(BTN_TOOL_RUBBER, input->keybit);
	//__set_bit(BTN_STYLUS, input->keybit);
	//__set_bit(BTN_STYLUS2, input->keybit);
	__set_bit(BTN_TOUCH, input->keybit);
	__set_bit(BTN_TOOL_PEN, input->keybit);
	__set_bit(INPUT_PROP_DIRECT, input->propbit);
	input_set_abs_params(input, ABS_X, 0, elan_ts->elan_chip_data->x_max, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, elan_ts->elan_chip_data->y_max, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, MAX_PEN_PRESSURE, 0, 0);
	//input_set_abs_params(input, ABS_TILT_X, 0, 90, 0, 0);
	//input_set_abs_params(input, ABS_TILT_Y, 0,90, 0, 0);
	return NO_ERR;
}

static int elan_ktf_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	return NO_ERR;
}

static int elan_ktf_irq_bottom_half(struct ts_cmd_node *in_cmd,struct ts_cmd_node *out_cmd)
{
	int ret=0;
	u8 buf[TEN_FINGER_DATA_LEN] = { 0 };
	struct algo_param *algo_p = NULL;
	struct ts_fingers *info = NULL;
	struct ts_pens* pens = NULL;
	if (!out_cmd) {
		TS_LOG_ERR("[elan]:%s out cmd is null\n",__func__);
		return -EINVAL;
	}

	algo_p = &out_cmd->cmd_param.pub_params.algo_param;
	info = &algo_p->info;
	ret = elan_ktf_ts_recv_data(buf);
	if (ret) {
		TS_LOG_ERR("[elan]:recv data fail\n");
		return ret;
	}

	switch(buf[REPORT_ID_BYTE])
	{
		case FINGER_REPORT_ID:
			out_cmd->command = TS_INPUT_ALGO;
			parse_fingers_point(info,buf);
			break;
		case PEN_REPORT_ID:
			pens = &out_cmd->cmd_param.pub_params.report_pen_info;
			pens->tool.tool_type = BTN_TOOL_PEN;
			out_cmd->command = TS_REPORT_PEN;
			parse_pen_point(pens, buf, out_cmd);
			if (pens->tool.pen_inrange_status != atomic_read(&last_pen_inrange_status)){
				atomic_set(&last_pen_inrange_status, pens->tool.pen_inrange_status);
				if (TS_PEN_OUT_RANGE < pens->tool.pen_inrange_status) {
					(void)ts_event_notify(TS_PEN_IN_RANGE);
				} else {
					(void)ts_event_notify(TS_PEN_OUT_RANGE);
				}
			}
			break;
		default:
			TS_LOG_INFO("[elan]:unknow report id:%0x,%0x,%0x,%0x\n",buf[0],buf[2],buf[4],buf[5]);  //0th len 2th id 4,5th other
	}
	return NO_ERR;
}

static int elan_ktf_hw_reset(void)
{
	TS_LOG_INFO("[elan]:%s\n", __func__);
	elants_reset_pin_high();
	msleep(5);//ic reset need
	elants_reset_pin_low();
	msleep(10);//ic reset need
	elants_reset_pin_high();
	msleep(300); //ic reset need
	return NO_ERR;
}

static int elan_ktf_ts_set_power_state(int state)
{
	int ret = 0;
	u8 cmd[ELAN_SEND_DATA_LEN] = {0x04,0x00,0x23,0x00,0x03,0x00,0x04,0x54,0x50,0x00,0x01};
	cmd[SUSPEND_OR_REPORT_BYTE] |= (state << 3);
	ret = elan_i2c_write(cmd, sizeof(cmd));
	if (ret) {
		TS_LOG_ERR("[elan] %s: i2c_master_send failed!ret=%d\n", __func__,ret);
		return -EINVAL;
	}
	return NO_ERR;
}

static int elan_ktf_core_suspend(void)
{
	int rc =NO_ERR;
	if(!elan_ts){
		TS_LOG_ERR("[elan]:%s,elan_ts is null\n",__func__);
		return -EINVAL;
	}
	if(atomic_read(&elan_ts->tp_mode)==TP_NORMAL)
	{
		TS_LOG_INFO("[elan] %s: enter\n", __func__);
		rc = elan_ktf_ts_set_power_state(0); //suspend
		if (rc) {
			TS_LOG_ERR("[elan]:suspend fail\n");
		}
	}
	return rc;
}

static int elan_ktf_core_resume(void)
{
	int rc = NO_ERR;
	if (!elan_ts) {
		TS_LOG_ERR("[elan]:%s,elan_ts is null\n",__func__);
		return -EINVAL;
	}
	if(atomic_read(&elan_ts->tp_mode)==TP_NORMAL)
	{
		TS_LOG_INFO("[elan]:%s: enter\n", __func__);
		rc = elan_ktf_ts_set_power_state(1);  //1 resume
		if (rc) {
			elan_ktf_hw_reset();
		}
	}
	return rc;
}

static void elan_chip_shutdown(void)
{
	TS_LOG_INFO("[elan]:%s: enter\n", __func__);
	elants_reset_pin_low();
	mdelay(2);//spec need
	if (VCI_GPIO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if(elan_ts->elan_chip_data->vci_gpio_ctrl) {
			gpio_direction_output(elan_ts->elan_chip_data->vci_gpio_ctrl, 0);
			gpio_free(elan_ts->elan_chip_data->vci_gpio_ctrl);
		}
	}
	if (VDD_GPIO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (elan_ts->elan_chip_data->vddio_gpio_ctrl) {
			gpio_direction_output(elan_ts->elan_chip_data->vddio_gpio_ctrl, 0);
			gpio_free(elan_ts->elan_chip_data->vddio_gpio_ctrl);
		}
	}
	if (VCI_LDO_TYPE == elan_ts->elan_chip_data->vci_regulator_type) {
		if (!IS_ERR(elan_ts->vdda)) {
			regulator_disable(elan_ts->vdda);
		}
	}
	if (VDD_LDO_TYPE == elan_ts->elan_chip_data->vddio_regulator_type) {
		if (!IS_ERR(elan_ts->vddd)) {
			regulator_disable(elan_ts->vddd);
		}
	}
	elan_regulator_put();

	return;
}
static int __init elan_ktf_ts_init(void)
{
	bool found = false;
	struct device_node* child = NULL;
	struct device_node* root = NULL;
	int error = NO_ERR;

	TS_LOG_INFO("[elan]:%s enter!\n",__func__);
	elan_ts = kzalloc(sizeof(struct elan_ktf_ts_data), GFP_KERNEL);
	if (!elan_ts) {
		TS_LOG_ERR("[elan]:Failed to alloc mem for struct elan_ktf_ts_data!\n");
		error =  -ENOMEM;
		goto err;
	}
	elan_ts->elan_chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!elan_ts->elan_chip_data) {
		TS_LOG_ERR("[elan]:Failed to alloc mem for struct elan_chip_data\n");
		error =  -ENOMEM;
		goto free_elan_ts;
	}

	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root) {
		TS_LOG_ERR("[elan]:huawei_ts, find_compatible_node huawei,ts_kit error\n");
		error = -EINVAL;
		goto exit;
	}

	for_each_child_of_node(root, child)  //find the chip node
	{
		if (of_device_is_compatible(child, ELAN_KTF_NAME)) {
			found = true;
			break;
		}
	}
	if (!found) {
		TS_LOG_ERR("[elan]:not found chip elan child node!\n");
		error = -EINVAL;
		goto exit;
	}

	elan_ts->elan_chip_data->cnode = child;
	elan_ts->elan_chip_data->ops = &ts_kit_elan_ops;

	error = huawei_ts_chip_register(elan_ts->elan_chip_data);
	if (error) {
		TS_LOG_ERR("[elan]: chip register fail !\n");
		goto exit;
	}
	TS_LOG_INFO("[elan]: chip_register! err=%d\n", error);
	return error;
exit:
	if(elan_ts->elan_chip_data) {
		kfree(elan_ts->elan_chip_data);
		elan_ts->elan_chip_data = NULL;
	}

free_elan_ts:
	if (elan_ts) {
		kfree(elan_ts);
		elan_ts = NULL;
	}
err:
	return error;
}

static void __exit elan_ktf_ts_exit(void)
{
	return;
}

late_initcall(elan_ktf_ts_init);
module_exit(elan_ktf_ts_exit);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
