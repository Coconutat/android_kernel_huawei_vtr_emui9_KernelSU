#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include "huawei_thp.h"

#include <linux/time.h>
#include <linux/syscalls.h>

#define GOODIX_IC_NAME "goodix"
#define THP_GOODIX_DEV_NODE_NAME "goodix_thp"

#define MODULE_INFO_READ_RETRY   5

#define GOODIX_FRAME_ADDR_DEFAULT 0xAFB0
#define GOODIX_CMD_ADDR_DEFAULT   0x6F68
#define GOODIX_MODULE_INFO_ADDR   0x452C
#define GOODIX_PROJECT_ID_ADDR    0xBDB4
#define GOODIX_AFE_INFO_ADDR      0x6D20
#define GOODIX_MAX_AFE_LEN        300
#define GOODIX_FRAME_LEN_OFFSET   20
#define GOODIX_FRAME_ADDR_OFFSET  22
#define GOODIX_CMD_ADDR_OFFSET    102

#define CMD_ACK_BUF_OVERFLOW    0x01
#define CMD_ACK_CHECKSUM_ERROR  0x02
#define CMD_ACK_BUSY            0x04
#define CMD_ACK_OK              0x80
#define CMD_ACK_IDLE            0xFF

#define CMD_FRAME_DATA          0x90
#define CMD_HOVER               0x91
#define CMD_FORCE               0x92
#define CMD_SLEEP               0x96
#define CMD_PT_MODE		0x05

#define PT_MODE         0

#define FEATURE_ENABLE          1
#define FEATURE_DISABLE         0

#define SPI_FLAG_WR		 0xF0
#define SPI_FLAG_RD		 0xF1
#define MASK_8BIT 		 0xFF

#define GOODIX_MASK_ID           "NOR_G1"
#define GOODIX_MASK_ID_LEN       6

#pragma pack(1)
struct goodix_module_info {
	u8 mask_id[6];
	u8 mask_vid[3];
	u8 pid[8];
	u8 vid[4];
	u8 sensor_id;
	u8 reserved[49];
	u8 checksum;
};
#pragma pack()

struct goodix_module_info module_info;
static int goodix_frame_addr = 0;
static int goodix_frame_len = 0;
static int goodix_cmd_addr = 0;
static unsigned int gs_thp_udfp_stauts = 0;

static int thp_goodix_spi_read(struct thp_device *tdev, unsigned int addr,
			   u8 *data, unsigned int len)
{
	struct spi_device *spi = NULL;
	u8 *rx_buf = NULL;
	u8 *tx_buf = NULL;
	u8 start_cmd_buf[3];
	struct spi_transfer xfers[2];
	struct spi_message spi_msg;
	int ret = 0;

	if (!tdev || !data || !tdev->tx_buff || !tdev->rx_buff || !tdev->thp_core || !tdev->thp_core->sdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	spi = tdev->thp_core->sdev;
	rx_buf = tdev->rx_buff;
	tx_buf = tdev->tx_buff;

	spi_message_init(&spi_msg);
	memset(xfers, 0, sizeof(xfers));

	start_cmd_buf[0] = SPI_FLAG_WR; //0xF0 start write flag
	start_cmd_buf[1] = (addr >> 8) & MASK_8BIT;
	start_cmd_buf[2] = addr & MASK_8BIT;

	xfers[0].tx_buf = start_cmd_buf;
	xfers[0].len = 3;
	xfers[0].cs_change = 1;
	spi_message_add_tail(&xfers[0], &spi_msg);

	tx_buf[0] = SPI_FLAG_RD; //0xF1 start read flag
	xfers[1].tx_buf = tx_buf;
	xfers[1].rx_buf = rx_buf;
	xfers[1].len = len + 1;
	xfers[1].cs_change = 1;
	spi_message_add_tail(&xfers[1], &spi_msg);
	thp_bus_lock();
	ret = thp_spi_sync(spi, &spi_msg);
	thp_bus_unlock();
	if (ret < 0) {
		THP_LOG_ERR("Spi transfer error:%d\n",ret);
		return ret;
	}
	memcpy(data, &rx_buf[1], len);

	return ret;
}

static int thp_goodix_spi_write(struct thp_device *tdev,
		unsigned int addr, u8 *data, unsigned int len)
{
	struct spi_device *spi = NULL;
	u8 *tx_buf = NULL;
	struct spi_transfer xfers;
	struct spi_message spi_msg;
	int ret = 0;

	if (!tdev || !data || !tdev->tx_buff || !tdev->thp_core || !tdev->thp_core->sdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	spi = tdev->thp_core->sdev;
	tx_buf = tdev->tx_buff;

	spi_message_init(&spi_msg);
	memset(&xfers, 0, sizeof(xfers));

	tx_buf[0] = SPI_FLAG_WR; //0xF1 start read flag
	tx_buf[1] = (addr >> 8) & MASK_8BIT;
	tx_buf[2] = addr & MASK_8BIT;
	memcpy(&tx_buf[3], data, len);
	xfers.tx_buf = tx_buf;
	xfers.len = len + 3;
	xfers.cs_change = 1;
	spi_message_add_tail(&xfers, &spi_msg);
	thp_bus_lock();
	ret = thp_spi_sync(spi, &spi_msg);
	thp_bus_unlock();
	if (ret < 0) {
		THP_LOG_ERR("Spi transfer error:%d\n",ret);
	}

	return ret;
}

static int thp_goodix_switch_cmd(struct thp_device *tdev, u8 cmd, u8 status)
{
	int ret;
	u8 cmd_buf[4] = {0};
	u8 cmd_ack = 0;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	cmd_buf[0] = cmd;
	cmd_buf[1] = status;
	cmd_buf[2] = 0 - cmd_buf[1] - cmd_buf[0]; /* checksum */
	cmd_buf[3] = 0;
	ret = thp_goodix_spi_write(tdev, goodix_cmd_addr, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		THP_LOG_ERR("%s: failed send command, ret %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = thp_goodix_spi_read(tdev, goodix_cmd_addr + 4, &cmd_ack, 1);
	if (ret < 0) {
		THP_LOG_ERR("%s: failed read command ack info, ret %d\n", __func__, ret);
		return -EINVAL;
	}

	if (cmd_ack != CMD_ACK_OK) {
		THP_LOG_ERR("%s: command does't work, command state ack info 0x%x\n",
				__func__, cmd_ack);
		return -EINVAL;
	}

	return 0;
}

static int thp_goodix_init(struct thp_device *tdev)
{
	int rc = 0;
	struct thp_core_data *cd = NULL;
	struct device_node *goodix_node = NULL;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	cd = tdev->thp_core;
	goodix_node = of_get_child_by_name(cd->thp_node,
						THP_GOODIX_DEV_NODE_NAME);

	THP_LOG_INFO("%s: called\n", __func__);

	if (!goodix_node) {
		THP_LOG_INFO("%s: goodix dev not config in dts\n", __func__);
		return -ENODEV;
	}

	rc = thp_parse_spi_config(goodix_node, cd);
	if (rc)
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);

	rc = thp_parse_timing_config(goodix_node, &tdev->timing_config);
	if (rc)
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);

	rc = thp_parse_feature_config(goodix_node, cd);
	if (rc)
		THP_LOG_ERR("%s: feature_config fail\n", __func__);

	return 0;
}

static u8 checksum_u8(u8 *data, u32 size)
{
	u8 checksum = 0;
	u32 i;
	int zero_count = 0;

	for (i = 0; i < size; i++) {
		checksum += data[i];
		if (!data[i])
			zero_count++;
	}
	return zero_count == size ? 0xFF : checksum;
}

static int thp_goodix_communication_check(struct thp_device *tdev)
{
	int ret;
	int len;
	int retry;
	u8 temp_buf[GOODIX_MASK_ID_LEN + 1] = {0};
	u8 checksum = 0;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	len = sizeof(module_info);
	memset(&module_info, 0, len);

	for (retry = 0; retry < MODULE_INFO_READ_RETRY; retry++) {
		ret = thp_goodix_spi_read(tdev, GOODIX_MODULE_INFO_ADDR,
				(u8 *)&module_info, len);
		print_hex_dump(KERN_INFO, "[I/THP] goodix module info: ", DUMP_PREFIX_NONE,
				32, 3, (u8 *)&module_info, len, 0);
		checksum = checksum_u8((u8 *)&module_info, len);
		if (!ret && !checksum)
			break;

		msleep(10);
	}

	THP_LOG_INFO("hw info: ret %d, checksum 0x%x, retry %d\n",ret,
		checksum, retry);
	if (retry == MODULE_INFO_READ_RETRY) {
		THP_LOG_ERR("%s: failed read module info\n", __func__);
		return -EINVAL;
	}

	if (memcmp(module_info.mask_id, GOODIX_MASK_ID, sizeof(GOODIX_MASK_ID))) {
		memcpy(temp_buf, module_info.mask_id, GOODIX_MASK_ID_LEN);
		THP_LOG_ERR("%s: invalied mask id %s != %s\n", __func__,
			    temp_buf, GOODIX_MASK_ID);
		return -EINVAL;
	}

	THP_LOG_INFO("%s: communication check passed\n", __func__);
	memcpy(temp_buf, module_info.mask_id, GOODIX_MASK_ID_LEN);
	temp_buf[GOODIX_MASK_ID_LEN] = '\0';
	THP_LOG_INFO("MASK_ID %s : ver %*ph\n", temp_buf,
			sizeof(module_info.mask_vid), module_info.mask_vid);
	THP_LOG_INFO("PID %s : ver %*ph\n", module_info.pid,
			sizeof(module_info.vid), module_info.vid);
	return 0;
}

static int goodix_power_init(void)
{
	int ret;

	ret = thp_power_supply_get(THP_VCC);
	ret |= thp_power_supply_get(THP_IOVDD);
	if (ret) {
		THP_LOG_ERR("%s: fail to get power\n", __func__);
	}

	return 0;
}

static void goodix_power_release(void)
{
	thp_power_supply_put(THP_VCC);
	thp_power_supply_put(THP_IOVDD);
	return;
}

static int goodix_power_on(struct thp_device *tdev)
{
	int ret;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	THP_LOG_INFO("%s:called\n", __func__);
	gpio_set_value(tdev->gpios->cs_gpio, GPIO_HIGH);

	ret = thp_power_supply_ctrl(THP_VCC, THP_POWER_ON, 0);
	ret |= thp_power_supply_ctrl(THP_IOVDD, THP_POWER_ON, 1);
	if (ret) {
		THP_LOG_ERR("%s:power ctrl fail\n", __func__);
	}
	gpio_set_value(tdev->gpios->rst_gpio, GPIO_HIGH);
	return ret;
}

static int goodix_power_off(struct thp_device *tdev)
{
	int ret;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.suspend_reset_after_delay_ms);//1ms

	ret = thp_power_supply_ctrl(THP_IOVDD, THP_POWER_OFF, 0);
	ret |= thp_power_supply_ctrl(THP_VCC, THP_POWER_OFF, 0);
	if (ret) {
		THP_LOG_ERR("%s:power ctrl fail\n", __func__);
	}
	gpio_set_value(tdev->gpios->cs_gpio, GPIO_LOW);

	return ret;
}

static void thp_goodix_timing_work(struct thp_device *tdev)
{
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	THP_LOG_INFO("%s:called,do hard reset\n", __func__);
	gpio_direction_output(tdev->gpios->rst_gpio, THP_RESET_HIGH);
	thp_do_time_delay(tdev->timing_config.boot_reset_after_delay_ms);
}

static int checksum_16(u8 *data, int size)
{
	int i;
	int non_zero_count = 0;
	u32 checksum = 0;

	for (i = 0; i < size - 4; i += 2) {
		checksum += (data[i] << 8) | data[i + 1];
		if (data[i] || data[i + 1])
			non_zero_count++;
	}

	checksum += (data[i] << 24) + (data[i + 1] << 16) +
		    (data[i + 2] << 8) + data[i + 3];

	return non_zero_count ? checksum : 0xFF;
}

static int thp_goodix_get_afe_info(struct thp_device *tdev)
{
	int ret;
	int afe_data_len = 0;
	u8 buf[2] = {0};
	u8 afe_data_buf[GOODIX_MAX_AFE_LEN] = {0};
	u8 debug_buf[108] = {0};
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	ret = thp_goodix_spi_read(tdev, GOODIX_AFE_INFO_ADDR,
			buf, sizeof(buf));
	if (ret) {
		THP_LOG_ERR("%s: failed read afe data length, ret %d\n", __func__, ret);
		return ret;
	}

	afe_data_len = (buf[0] << 8) | buf[1];
	/* data len must be equal or less than GOODIX_MAX_AFE_LEN */
	if ((afe_data_len == 0) || (afe_data_len > GOODIX_MAX_AFE_LEN)
		|| (afe_data_len & 0x1)) {
		THP_LOG_ERR("%s: invalied afe_data_len %d\n", __func__, afe_data_len);

		ret = thp_goodix_spi_read(tdev, GOODIX_AFE_INFO_ADDR,
				debug_buf, sizeof(debug_buf));
		THP_LOG_ERR("debug_buf[0-20] %*ph\n", 20, debug_buf);
		THP_LOG_ERR("debug_buf[20-40] %*ph\n", 20, debug_buf + 20);
		THP_LOG_ERR("debug_buf[40-60] %*ph\n", 20, debug_buf + 40);
		THP_LOG_ERR("debug_buf[60-80] %*ph\n", 20, debug_buf + 60);
		THP_LOG_ERR("debug_buf[80-100] %*ph\n", 20, debug_buf + 80);

		goodix_frame_addr = GOODIX_FRAME_ADDR_DEFAULT;
		goodix_frame_len = 1500;
		goodix_cmd_addr = GOODIX_CMD_ADDR_DEFAULT;
		THP_LOG_INFO("use default addr info\n");
		return -EINVAL;
	}

	ret = thp_goodix_spi_read(tdev, GOODIX_AFE_INFO_ADDR + 2,
			afe_data_buf, afe_data_len);
	if (ret) {
		THP_LOG_ERR("%s: failed read afe data, ret %d\n", __func__, ret);
		return ret;
	}

	if (checksum_16(afe_data_buf, afe_data_len)) {
		THP_LOG_ERR("%s: afe data checksum error, checksum %d\n", __func__,
			checksum_16(afe_data_buf, afe_data_len));
		return -EINVAL;
	}

     	goodix_frame_addr = (afe_data_buf[GOODIX_FRAME_ADDR_OFFSET] << 8) +
				afe_data_buf[GOODIX_FRAME_ADDR_OFFSET + 1];
     	goodix_frame_len = (afe_data_buf[GOODIX_FRAME_LEN_OFFSET] << 8) +
				afe_data_buf[GOODIX_FRAME_LEN_OFFSET + 1];
     	goodix_cmd_addr = (afe_data_buf[GOODIX_CMD_ADDR_OFFSET] << 8) +
				afe_data_buf[GOODIX_CMD_ADDR_OFFSET + 1];

	THP_LOG_INFO("%s: frame addr 0x%x, len %d, cmd addr 0x%x\n", __func__,
		goodix_frame_addr, goodix_frame_len, goodix_cmd_addr);
	return 0;
}

int thp_goodix_chip_detect(struct thp_device *tdev)
{
	int ret = 0;

	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	goodix_power_init();
	ret = goodix_power_on(tdev);
	if (ret) {
		THP_LOG_ERR("%s: power on failed\n", __func__);
	}

	thp_goodix_timing_work(tdev);

	if (thp_goodix_communication_check(tdev)) {
		THP_LOG_ERR("%s:communication check fail\n", __func__);
		goodix_power_off(tdev);
		goodix_power_release();
		return -ENODEV;
	}

	if (thp_goodix_get_afe_info(tdev))
		THP_LOG_ERR("%s: failed get afe addr info\n", __func__);
	return 0;
}

static int thp_goodix_get_frame(struct thp_device *tdev,
			char *buf, unsigned int len)
{
	if (!tdev || !buf) {
		THP_LOG_INFO("%s: input dev null\n", __func__);
		return -ENOMEM;
	}

	if (!len) {
		THP_LOG_INFO("%s: read len illegal\n", __func__);
		return -ENOMEM;
	}

	return thp_goodix_spi_read(tdev, goodix_frame_addr, buf, len);
}

static int thp_goodix_resume(struct thp_device *tdev)
{
	int ret = 0;
	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	if (gs_thp_udfp_stauts) {
		THP_LOG_INFO("enable frame data report\n");
		ret =  thp_goodix_switch_cmd(tdev, CMD_FRAME_DATA, FEATURE_ENABLE);
		if (ret)
			THP_LOG_ERR("failed enable frame data report\n");
	} else if(is_pt_test_mode(tdev)){
		gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
		thp_do_time_delay(tdev->timing_config.resume_reset_after_delay_ms);//1ms
		gpio_set_value(tdev->gpios->rst_gpio, GPIO_HIGH);
	}else{
		goodix_power_on(tdev);
	}

	return ret;
}

static int thp_goodix_suspend(struct thp_device *tdev)
{
	int ret = 0;

	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	gs_thp_udfp_stauts = thp_get_status(THP_STAUTS_UDFP);
	if (gs_thp_udfp_stauts) {//To avoid THP_STAUTS_UDFP be changed in suspend status.
		THP_LOG_INFO("disable frame data report\n");
		ret =  thp_goodix_switch_cmd(tdev, CMD_FRAME_DATA, FEATURE_DISABLE);
		if (ret)
			THP_LOG_ERR("failed disable frame data report\n");

	} else if(is_pt_test_mode(tdev)){
		THP_LOG_INFO("%s: suspend PT mode \n", __func__);
		ret =  thp_goodix_switch_cmd(tdev, CMD_PT_MODE, PT_MODE);
		if (ret)
			THP_LOG_ERR("failed enable PT mode\n");
	}else{
		goodix_power_off(tdev);
		THP_LOG_INFO("enter poweroff mode\n");
	}

	return ret;
}

static int goodix_get_project_id(struct thp_device *tdev, char *buf, unsigned int len)
{

	char proj_id[THP_PROJECT_ID_LEN + 1] = {0};
	int ret = 0;
	if (!tdev || !buf) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	ret = thp_goodix_spi_read(tdev, GOODIX_PROJECT_ID_ADDR, proj_id, THP_PROJECT_ID_LEN);
	if(ret){
		THP_LOG_ERR("Project_id Read ERR\n");
	}
	proj_id[THP_PROJECT_ID_LEN] = '\0';
	THP_LOG_INFO("PROJECT_ID[0-9] %*ph\n", 10, proj_id);

	if (is_valid_project_id(proj_id)) {
		strncpy(buf, proj_id, len);
	} else {
		THP_LOG_ERR("%s:get project id fail\n", __func__);
		return -EIO;
	}
	return 0;
}

static void thp_goodix_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	kfree(tdev->tx_buff);
	kfree(tdev->rx_buff);
	kfree(tdev);
	return;
}

static int thp_goodix_afe_notify_callback(struct thp_device *tdev, unsigned long event)
{
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	return thp_goodix_get_afe_info(tdev);
}

struct thp_device_ops goodix_dev_ops = {
	.init = thp_goodix_init,
	.detect = thp_goodix_chip_detect,
	.get_frame = thp_goodix_get_frame,
	.resume = thp_goodix_resume,
	.suspend = thp_goodix_suspend,
	.get_project_id = goodix_get_project_id,
	.exit = thp_goodix_exit,
	.afe_notify = thp_goodix_afe_notify_callback,
};

static int __init thp_goodix_module_init(void)
{
	int rc;

	struct thp_device *dev = kzalloc(sizeof(struct thp_device), GFP_KERNEL);
	if (!dev) {
		THP_LOG_ERR("%s: thp device malloc fail\n", __func__);
		return -ENOMEM;
	}

	dev->tx_buff = kzalloc(THP_MAX_FRAME_SIZE, GFP_KERNEL);
	dev->rx_buff = kzalloc(THP_MAX_FRAME_SIZE, GFP_KERNEL);
	if (!dev->tx_buff || !dev->rx_buff) {
		THP_LOG_ERR("%s: out of memory\n", __func__);
		rc = -ENOMEM;
		goto err;
	}

	dev->ic_name = GOODIX_IC_NAME;
	dev->dev_node_name = THP_GOODIX_DEV_NODE_NAME;
	dev->ops = &goodix_dev_ops;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	}

	return rc;
err:
	if(dev->tx_buff){
		kfree(dev->tx_buff);
		dev->tx_buff = NULL;
	}
	if(dev->rx_buff){
		kfree(dev->rx_buff);
		dev->rx_buff = NULL;
	}
	if(dev){
		kfree(dev);
		dev = NULL;
	}
	return rc;
}

static void __exit thp_goodix_module_exit(void)
{
		THP_LOG_INFO("%s: called \n", __func__);
};

module_init(thp_goodix_module_init);
module_exit(thp_goodix_module_exit);
MODULE_AUTHOR("goodix");
MODULE_DESCRIPTION("goodix driver");
MODULE_LICENSE("GPL");
