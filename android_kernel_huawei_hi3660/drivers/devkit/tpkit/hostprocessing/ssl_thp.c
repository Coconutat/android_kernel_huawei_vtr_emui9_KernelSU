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
#include <linux/spi/spi.h>
#include "huawei_thp.h"
#include <linux/time.h>
#include <linux/syscalls.h>

#define SSL_IC_NAME "ssl_thp"
#define THP_SSL_DEV_NODE_NAME "ssl_thp"

#define MXT680U2_FAMILY_ID  166
#define MXT680U2_VARIANT_ID  22

#define MXT_WAKEUP_TIME 10

// opcodes
#define SPI_WRITE_REQ    0x01
#define SPI_WRITE_OK     0x81
#define SPI_WRITE_FAIL   0x41
#define SPI_READ_REQ     0x02
#define SPI_READ_OK      0x82
#define SPI_READ_FAIL    0x42
#define SPI_INVALID_REQ  0x04
#define SPI_INVALID_CRC  0x08
#define SPI_APP_HEADER_LEN     6
#define SPI_BOOTL_HEADER_LEN   2
#define T117_BYTES_READ_LIMIT    1505 // 7 x 215 (T117 size)
#define WRITE_DUMMY_BYTE    400
#define READ_DUMMY_BYTE     400
#define FRAME_READ_DUMMY_BYTE    400

#define SPI_APP_DATA_MAX_LEN     64
#define SPI_APP_BUF_SIZE_WRITE    (SPI_APP_HEADER_LEN + SPI_APP_DATA_MAX_LEN + WRITE_DUMMY_BYTE)
#define SPI_APP_BUF_SIZE_READ    (2*SPI_APP_HEADER_LEN + T117_BYTES_READ_LIMIT + FRAME_READ_DUMMY_BYTE)
static unsigned int gs_thp_udfp_stauts = 0;

struct mxt_info
{
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_object
{
	u8 type;
	u16 start_address;
	u8 size_minus_one;
	u8 instances_minus_one;
	u8 num_report_ids;
} __packed;

#define MXT_OBJECT_START     0x07 // after struct mxt_info
#define MXT_INFO_CHECKSUM_SIZE  3 // after list of struct mxt_object
#define GETPDSDATA_COMMAND	0x81
#define MXT_T6_DIAGNOSTIC_OFFSET 0x05

#define READ_ID_RETRY_TIMES 3

#define SSL_T117_ADDR 117
#define SSL_T7_ADDR 7
#define SSL_T6_ADDR 6
#define SSL_T37_ADDR 37
#define SSL_T118_ADDR 118

#define SSL_SYNC_DATA_REG_DEFALUT_ADDR  238
#define THP_BOOTLOADER_SPI_FREQ_HZ	100000U
#define BOOTLOADER_READ_CNT 2
#define BOOTLOADER_READ_LEN 1
#define BOOTLOADER_CRC_ERROR_CODE 0x60
#define BOOTLOADER_WAITING_CMD_MODE_0 0xC0
#define BOOTLOADER_WAITING_CMD_MODE_1 0xE0

#define PDS_HEADER_OFFSET 4
#define PROJECTID_ARR_LEN 32
#define SSL_T7_COMMAMD_LEN 2


static uint16_t T117_address = 0;
static uint16_t T7_address = 0;
static uint16_t T6_address = 0;
static uint16_t T37_address = 0;
static uint16_t T118_address = 0;
static u8 is_bootloader_mode =0;

static u8 get_crc8_iter(u8 crc, u8 data)
{
	static const u8 crcpoly = 0x8c;
	u8 index = 8;
	u8 fb = 0;
	do{
		fb = (crc ^ data) & 0x01;
		data >>= 1;
		crc >>= 1;
		if (fb){
			crc ^= crcpoly;
		}
	} while (--index);
	return crc;
}

static u8 get_header_crc(u8 *p_msg)
{
	u8 calc_crc = 0;
	int i = 0;
	if (!p_msg ) {
		THP_LOG_ERR("%s: point null\n", __func__);
		return -EINVAL;
	}
	for (; i < SPI_APP_HEADER_LEN-1; i++){
		calc_crc = get_crc8_iter(calc_crc, p_msg[i]);
	}
	return calc_crc;
}

static void spi_prepare_header(u8 *header,
                               u8 opcode,
                               u16 start_register,
                               u16 count)
{
	if (!header) {
		THP_LOG_ERR("%s: point null\n", __func__);
		return;
	}

	header[0] = opcode;
	header[1] = start_register & 0xff;
	header[2] = start_register >> 8;
	header[3] = count & 0xff;
	header[4] = count >> 8;
	header[5] = get_header_crc(header);
	return ;
}


static int mxt_bootloader_read(struct thp_device *tdev ,struct spi_device *client, unsigned char *buf, int count)
{
	static const char op_header[] = {0x01, 0x00}; /* Require SPI LSB 0x01 for read op */
	int ret_val = 0;
	struct spi_message spimsg;
	struct spi_transfer spitr;

	if (!tdev || !client || !buf || !tdev->tx_buff || !tdev->rx_buff ) {
		THP_LOG_ERR("%s: point null\n", __func__);
		return -EINVAL;
	}
	spi_message_init(&spimsg);
	memset(&spitr, 0,  sizeof(struct spi_transfer));

	spitr.tx_buf = tdev->tx_buff;
	spitr.rx_buf = tdev->rx_buff;
	spitr.len = sizeof(op_header) + count;
	memcpy(tdev->tx_buff, op_header, sizeof(op_header));
	spi_message_add_tail(&spitr, &spimsg);
	ret_val = spi_sync(client, &spimsg);
	if (ret_val < 0){
		THP_LOG_ERR("%s: Error reading from spi (%d)\n", __func__, ret_val);
		return ret_val;
	}
	memcpy(buf, tdev->rx_buff + sizeof(op_header), count);
	return ret_val;

}

static int mxt_wait_for_chg_is_low(struct thp_device *tdev)
{
	int ret_val = -1;
	int count_timeout = 100; // 100 msec

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	while (0 != gpio_get_value(tdev->gpios->irq_gpio) && count_timeout > 0){
		count_timeout--;
		mdelay(1); // 1 msec
	}
	ret_val = count_timeout > 0 ? 0 : -1;
	return ret_val;
}


static int mxt_check_is_bootloader(struct thp_device *tdev)
{
	u8 status_byte = 0;
	int ret_value = 0;
	int bootloader_read_cnt = BOOTLOADER_READ_CNT;
	int max_thp_spi_clock = 0;
	if (!tdev) {
			THP_LOG_ERR("%s: tdev null\n", __func__);
			return -EINVAL;
		}
	max_thp_spi_clock = tdev->thp_core->sdev->max_speed_hz;
	THP_LOG_INFO("max thp spi clock= %d\n", max_thp_spi_clock);
	tdev->thp_core->sdev->max_speed_hz = THP_BOOTLOADER_SPI_FREQ_HZ;
	THP_LOG_INFO("set max thp spi clock= %d\n", tdev->thp_core->sdev->max_speed_hz);

	do{
		if(mxt_wait_for_chg_is_low(tdev) <0){
			THP_LOG_ERR("%s: CHG doesn't change to LOW\n", __func__);
		}else{
			thp_bus_lock();
			mxt_bootloader_read(tdev ,tdev->thp_core->sdev, &status_byte, BOOTLOADER_READ_LEN);
			thp_bus_unlock();
			THP_LOG_INFO("the bootloader status byte is %d\n", status_byte);
			if(status_byte == BOOTLOADER_CRC_ERROR_CODE || \
				status_byte == BOOTLOADER_WAITING_CMD_MODE_0 ||\
				status_byte == BOOTLOADER_WAITING_CMD_MODE_1 ){  //0x60: CRC error  0xE0: waiting cmd mode
				THP_LOG_INFO("****bootloader mode found - status byte is 0x%x\n", status_byte);
				ret_value =1;
				break;
			}
		}
	}while(bootloader_read_cnt-- >0);
	tdev->thp_core->sdev->max_speed_hz = max_thp_spi_clock;

	return ret_value;
}


static int __mxt_read_reg(struct thp_device *tdev ,struct spi_device *client, u16 start_register, u16 len, u8 *val)
{
	u8 attempt = 0;
	int ret_val = -EINVAL,i = 0;
	int DUMMY_BYTE = 0,dummy_offset = 0;
	u8 *rx_buf = NULL;
	u8 *tx_buf = NULL;

	struct spi_message  spimsg;
	struct spi_transfer spitr;
	if (!tdev || !client || !val || !tdev->rx_buff || !tdev->tx_buff) {
		THP_LOG_ERR("%s: tdev or client or val null\n", __func__);
		return -EINVAL;
	}

	rx_buf = tdev->rx_buff;
	tx_buf = tdev->tx_buff;

	if(len == T117_BYTES_READ_LIMIT)
		DUMMY_BYTE = FRAME_READ_DUMMY_BYTE;
	else
		DUMMY_BYTE = READ_DUMMY_BYTE;

	do{
		attempt++;
		if (attempt > 1){
			if (attempt > 5){
				THP_LOG_ERR("%s: Too many Retries\n", __func__);
				return -EIO;
			}
			if(len < T117_BYTES_READ_LIMIT)
				mdelay(MXT_WAKEUP_TIME);   //SPI2 need some delay time
			else
				return  -EINVAL;
		}

		memset(tx_buf, 0xFF, 2*SPI_APP_HEADER_LEN + DUMMY_BYTE);
		spi_prepare_header(tx_buf, SPI_READ_REQ, start_register, len);

		spi_message_init(&spimsg);
		memset(&spitr, 0,  sizeof(struct spi_transfer));
		spitr.tx_buf = tx_buf;
		spitr.rx_buf = rx_buf;
		spitr.len = 2*SPI_APP_HEADER_LEN + DUMMY_BYTE + len;
		spi_message_add_tail(&spitr, &spimsg);

		ret_val = spi_sync(client, &spimsg);

		if (ret_val < 0){
			THP_LOG_ERR("%s: Error reading from spi (%d)\n", __func__, ret_val);
			return ret_val;
		}

		for(i = 0; i < DUMMY_BYTE; i++){
			if(SPI_READ_OK == rx_buf[SPI_APP_HEADER_LEN+i]){
				dummy_offset = i + SPI_APP_HEADER_LEN;
				if(dummy_offset > READ_DUMMY_BYTE/2){
					THP_LOG_INFO("Found read dummy offset %d\n", dummy_offset);
				}
			break;
			}
		}
		if(dummy_offset == 0){
			THP_LOG_ERR("***mxt register read - cannot find dummy byte offset- read address =%d\n", start_register);
			if(len == T117_BYTES_READ_LIMIT){
				return -EINVAL;
			}
		}else{
			if(tx_buf[1] != rx_buf[1 + dummy_offset] || tx_buf[2] != rx_buf[2 + dummy_offset]){
				THP_LOG_ERR("%s: Unexpected address %d != %d reading from spi\n", __func__, rx_buf[1+dummy_offset] | (rx_buf[2+dummy_offset] << 8), start_register);
				if(len < T117_BYTES_READ_LIMIT)
					dummy_offset = 0;  //normal register read retry
				else					//T117 should not retry
					return -EINVAL;
			}else if (tx_buf[3] != rx_buf[3 + dummy_offset] || tx_buf[4] != rx_buf[4 + dummy_offset]){
				THP_LOG_ERR("%s: Unexpected count %d != %d reading from spi\n", __func__,rx_buf[3+dummy_offset] | (rx_buf[4+dummy_offset] << 8), len);
				if(len < T117_BYTES_READ_LIMIT)
					dummy_offset = 0;  //normal register read retry
				else					//T117 should not retry
					return -EINVAL;
			}
		}
	}while (get_header_crc(rx_buf + dummy_offset) != rx_buf[SPI_APP_HEADER_LEN-1+dummy_offset] || dummy_offset ==0 );
	memcpy(val, rx_buf + SPI_APP_HEADER_LEN + dummy_offset, len);
	return 0;
}

static int __mxt_write_reg(struct thp_device *tdev,struct spi_device *client, u16 start_register, u16 len, const u8 *val)
{
	int i = 0, ret_val= -EINVAL, attempt=0;
	struct spi_message  spimsg;
	struct spi_transfer spitr;

	u8 *rx_buf = NULL;
	u8 *tx_buf = NULL;
	int DUMMY_BYTE = WRITE_DUMMY_BYTE, dummy_offset = 0;

	if (!tdev || !client || !val || !tdev->rx_buff || !tdev->tx_buff) {
		THP_LOG_ERR("%s: point is  null\n", __func__);
		return -EINVAL;
	}
	rx_buf = tdev->rx_buff;
	tx_buf = tdev->tx_buff;

	do{
		attempt++;
		if (attempt > 1){
			if (attempt > 5){
				THP_LOG_ERR("Too many spi write Retries\n");
				return -EIO;
			}
			THP_LOG_INFO("__mxt_write_reg--retry %d after write fail\n", attempt-1);
			mdelay(MXT_WAKEUP_TIME);
		}

		/* WRITE SPI_WRITE_REQ */

		memset(tx_buf, 0xFF, 2*SPI_APP_HEADER_LEN + DUMMY_BYTE);
		spi_prepare_header(tx_buf, SPI_WRITE_REQ, start_register, len);
		memcpy(tx_buf + SPI_APP_HEADER_LEN, val, len);
		spi_message_init(&spimsg);
		memset(&spitr, 0,  sizeof(struct spi_transfer));
		spitr.tx_buf = tx_buf;
		spitr.rx_buf = rx_buf;
		spitr.len = 2*SPI_APP_HEADER_LEN + DUMMY_BYTE +len;
		spi_message_add_tail(&spitr, &spimsg);

		thp_bus_lock();
		ret_val = spi_sync(client, &spimsg);
		thp_bus_unlock();

		if (ret_val < 0){
			THP_LOG_ERR("%s: Error writing to spi\n", __func__);
			return ret_val;
		}

		for(i=0; i< DUMMY_BYTE; i++){
			if(SPI_WRITE_OK == rx_buf[SPI_APP_HEADER_LEN+i]){
				dummy_offset = i+SPI_APP_HEADER_LEN;
				if(dummy_offset>WRITE_DUMMY_BYTE/2){
					THP_LOG_INFO("Found big write dummy offset %d\n", dummy_offset);
				}
				break;
			}
		}

		if(dummy_offset){
			if (tx_buf[1] != rx_buf[1+dummy_offset] || tx_buf[2] != rx_buf[2+dummy_offset]){
				THP_LOG_ERR("Unexpected address %d != %d reading from spi", rx_buf[1+dummy_offset] | (rx_buf[2+dummy_offset] << 8), start_register);
				dummy_offset = 0;
			}else if (tx_buf[3] != rx_buf[3+dummy_offset] || tx_buf[4] != rx_buf[4+dummy_offset]){
				THP_LOG_ERR("Unexpected count %d != %d reading from spi", rx_buf[3+dummy_offset] | (rx_buf[4+dummy_offset] << 8), len);
				dummy_offset = 0;
			}
		}else{
			THP_LOG_ERR("mxt write --Cannot found write dummy offset address = %d\n", start_register);
		}
	}while (get_header_crc(rx_buf+dummy_offset) != rx_buf[SPI_APP_HEADER_LEN-1+dummy_offset] || dummy_offset ==0 );
	return 0;
}




static int mxt_read_blks(struct thp_device *tdev,struct spi_device *client, u16 start, u16 count,
				u8 *buf, u16 override_limit)
{
	u16 offset = 0;
	int ret_val = 0;
	u16 size = 0;
	if (!tdev || !client || !buf ) {
		THP_LOG_ERR("%s: point is  null\n", __func__);
		return -EINVAL;
	}

	thp_bus_lock();
	while (offset < count){
		if (0 == override_limit){
			size = min(SPI_APP_DATA_MAX_LEN, count - offset);
		}else{
			size = min(override_limit, count - offset);
		}
		ret_val = __mxt_read_reg(tdev,client,(start + offset),size,(buf + offset));
		if (ret_val){
			break;
		}

		offset += size;
	}
	thp_bus_unlock();

	return ret_val;
}


static int mxt_write_blks(struct thp_device *tdev,struct spi_device *client, u16 start, u16 count, u8 *buf)
{
	u16 offset = 0;
	int ret_val = 0;
	u16 size = 0;
	if (!tdev || !client || !buf ) {
		THP_LOG_ERR("%s: point is  null\n", __func__);
		return -EINVAL;
	}

	while (offset < count){
		size = min(SPI_APP_DATA_MAX_LEN, count - offset);

		ret_val = __mxt_write_reg(tdev,client,start + offset,size,buf + offset);
		if (ret_val){
			break;
		}
		offset += size;
	}

    return ret_val;
}

static int thp_ssl_init(struct thp_device *tdev)
{
	int rc = -EINVAL;
	struct thp_core_data *cd = NULL;
	struct device_node *ssl_node = NULL;

	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	cd = tdev->thp_core;
	ssl_node = of_get_child_by_name(cd->thp_node,THP_SSL_DEV_NODE_NAME);
	if (!ssl_node){
		THP_LOG_INFO("%s: syna dev not config in dts\n", __func__);
		return -ENODEV;
	}

	THP_LOG_INFO("%s >>>\n", __func__);

	rc = thp_parse_spi_config(ssl_node, cd);
	if (rc)
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);

	rc = thp_parse_timing_config(ssl_node, &tdev->timing_config);
	if (rc)
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);

	rc = thp_parse_feature_config(ssl_node, cd);
	if (rc)
		THP_LOG_ERR("%s: feature_config fail\n", __func__);

	rc = thp_parse_trigger_config(ssl_node, cd);
	if (rc)
		THP_LOG_ERR("%s: trigger_config fail\n", __func__);

	return 0;
}

static int thp_ssl_get_project_id(struct thp_device *tdev, char *buf, unsigned int len)
{
	int ret = -EINVAL, i = 0;
	char buff[PROJECTID_ARR_LEN] = {0};
	int8_t retry = READ_ID_RETRY_TIMES;
	unsigned char write_value = GETPDSDATA_COMMAND;
	struct thp_core_data *cd = NULL;
	memset(buff, 0, sizeof(buff));

	if (!tdev || !buf) {
		THP_LOG_ERR("%s: tdev or buf is  null\n", __func__);
		return -EINVAL;
	}
	cd = tdev->thp_core;
	if(is_bootloader_mode){
		//char *projectid = "P086811000";
		char *projectid = cd->project_id_dummy;
		memcpy(buf, projectid, len);
		THP_LOG_INFO("The project id is set to %s in bootloader mode ", projectid);
		return 0;
	}

	if(T37_address == 0 || T6_address == 0 || len > sizeof(buff) - PDS_HEADER_OFFSET - 1){
		THP_LOG_ERR( "Read project ID-address or read size check fail T37=[%d],T6=[%d],read len=[%d]\n", T37_address, T6_address, len);
		return ret;
	}
	do{
		ret = mxt_write_blks(tdev,tdev->thp_core->sdev, T6_address + MXT_T6_DIAGNOSTIC_OFFSET, 1, &write_value);
		if (ret != 0){
			THP_LOG_ERR( "Failed to send T6 diagnositic command");
		}
		mdelay(3 + READ_ID_RETRY_TIMES - retry);

		ret = mxt_read_blks(tdev,tdev->thp_core->sdev, T37_address, len + PDS_HEADER_OFFSET, buff, 0/*override_limit*/);
		for(i=0; i< len; i++){
			THP_LOG_INFO(" [%2x] ", buff[i]);
		}

		if (ret != 0 || buff[0] != 0x81 ||  buff[1] != 0 || buff[2] != 0x24 || buff[PDS_HEADER_OFFSET] == 0 ){
			THP_LOG_ERR( "Failed to read T37 data for project ID");
		}else{
			THP_LOG_INFO( "read T37 data for project ID..");
			break;
		}
	}while(retry-- > 0);

	if(retry < 0){
		THP_LOG_ERR( "read T37 data for projecd ID timeout");
		return -1;
	}
	memcpy(buf, buff + PDS_HEADER_OFFSET, len);
	THP_LOG_INFO("the project id is %s ", buff + PDS_HEADER_OFFSET);
	return 0;
}


static int thp_ssl_update_obj_addr(struct thp_device *tdev)
{
	int ret_val = -EINVAL,i = 0;
	struct mxt_info mxtinfo ;
	u8 *buff = NULL;
	size_t curr_size = 0;
	struct mxt_object *object_table = NULL;
	struct mxt_object *object = NULL;
	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}

	curr_size = MXT_OBJECT_START;

	ret_val = mxt_read_blks(tdev,tdev->thp_core->sdev, 0, curr_size, (u8 *)&mxtinfo, 0);
	if (ret_val){
		THP_LOG_ERR("mxt_read_blks--info block reading error\n");
		ret_val = -ENOMEM;
		goto error_free;
	}

	curr_size += mxtinfo.object_num * sizeof(struct mxt_object) + MXT_INFO_CHECKSUM_SIZE;
	buff = (u8 *)kzalloc(curr_size, GFP_KERNEL);
	if(NULL == buff){
		THP_LOG_ERR("%s:buff alloc faild\n", __func__);
		ret_val = -ENOMEM;
		goto error_free;
	}

	ret_val = mxt_read_blks(tdev,tdev->thp_core->sdev, MXT_OBJECT_START, curr_size , buff ,0);

	if (ret_val){
		THP_LOG_ERR("mxt_read_blks--info block reading error\n");
		goto error_free;
	}

	T117_address = 0;
	object_table = (struct mxt_object *)(buff );
	for (i = 0; i < mxtinfo.object_num; i++){
		object = object_table + i;
		le16_to_cpus(&object->start_address);
		switch (object->type){
			case SSL_T117_ADDR:
				T117_address = object->start_address;
				break;
			case SSL_T7_ADDR:
				T7_address = object->start_address;
				break;
			case SSL_T118_ADDR:
				T118_address = object->start_address;
				break;
			default :
				THP_LOG_ERR("%s error: unkown start object",__func__);
				break;
		}
	}
	THP_LOG_INFO("T117_address updated [%d]\n", T117_address );

error_free:
	if (NULL != buff){
		kfree(buff);
		buff = NULL;
	}
	return ret_val;
}

static int mxt_power_init(struct thp_device *tdev)
{
	int ret = -EINVAL;
	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	ret = thp_power_supply_get(THP_VCC);
	ret |= thp_power_supply_get(THP_IOVDD);
	if (ret) {
		THP_LOG_ERR("%s: fail to get power\n", __func__);
	}

	return 0;
}
static void mxt_power_release(struct thp_device *tdev)
{
	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	thp_power_supply_put(THP_VCC);
	thp_power_supply_put(THP_IOVDD);
	return;
}

static int mxt_power_on(struct thp_device *tdev)
{
	int ret = -EINVAL;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	gpio_direction_output(tdev->gpios->rst_gpio, GPIO_LOW);
	mdelay(1);
	ret = thp_power_supply_ctrl(THP_IOVDD, THP_POWER_ON, 1);
	ret |= thp_power_supply_ctrl(THP_VCC, THP_POWER_ON, 1);
	if (ret) {
		THP_LOG_ERR("%s:power on ctrl fail\n", __func__);
	}
	gpio_set_value(tdev->gpios->rst_gpio, GPIO_HIGH);
	mdelay(tdev->timing_config.boot_reset_after_delay_ms);
	return ret;
}

static int mxt_power_off(struct thp_device *tdev)
{
	int ret = -EINVAL;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
	ret = thp_power_supply_ctrl(THP_IOVDD, THP_POWER_OFF, 0);
	ret |= thp_power_supply_ctrl(THP_VCC, THP_POWER_OFF, 1);
	if (ret) {
		THP_LOG_ERR("%s:power off ctrl fail\n", __func__);
	}
	return ret;
}
//disable_frame = 1  ==> to disable frame data output
//disable_frame = 0  ==> to enable frame data output
//T118_CTRL.bit0: 0: disable frame data output, 1: enable frame data output
static int mxt_framedata_output_disable(struct thp_device *tdev, u8 disable_frame)
{
	int ret_val = -EINVAL;
	unsigned char T118_CTRL = 0;

	if(0==T118_address)
		return ret_val;

	ret_val = mxt_read_blks(tdev, tdev->thp_core->sdev,T118_address, 1, &T118_CTRL,0);

	if (ret_val != 0)
	{
	   THP_LOG_ERR( "mxt failed to read T118 cfg setting \n");
	   return ret_val;
	}

	T118_CTRL = disable_frame? (T118_CTRL&0xFE):(T118_CTRL| 0x01);
	ret_val = mxt_write_blks(tdev,tdev->thp_core->sdev, T118_address, 1, &T118_CTRL);

	if (ret_val != 0)
	{
	   THP_LOG_ERR( "mxt failed to send T118 frame data output on/off command[%d]",T118_CTRL);
	}

	THP_LOG_INFO( "T118 frame data control done[%d], T118_CTRL is [0x%x]\n", disable_frame, T118_CTRL);
	return ret_val;
}


static int thp_ssl_chip_detect(struct thp_device *tdev)
{
	int ret_val = -EINVAL, i = 0;
	struct mxt_info mxtinfo;
	u8 *buff = NULL;
	size_t curr_size = 0;
	struct mxt_object *object_table = NULL;
	int retry_time = READ_ID_RETRY_TIMES;
	struct mxt_object *object = NULL;

	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
	THP_LOG_INFO("%s: called\n", __func__);
	mxt_power_init(tdev);

	curr_size = MXT_OBJECT_START;
	mxt_power_on(tdev);

	do{
		ret_val = mxt_read_blks(tdev,tdev->thp_core->sdev, 0, sizeof(mxtinfo), (u8 *)&mxtinfo, 0/*override_limit*/);

		if (ret_val){
			THP_LOG_ERR("mxt_read_blks info- read device ID fails\n");
			continue;
		}

		if (mxtinfo.family_id != MXT680U2_FAMILY_ID || mxtinfo.variant_id != MXT680U2_VARIANT_ID){
			THP_LOG_ERR("%s: chip is not identified (%d, %d)\n", __func__, mxtinfo.family_id, mxtinfo.variant_id);
			THP_LOG_INFO("retry time is %d \n", retry_time );
		}else{
			THP_LOG_INFO("%s: Chip is identified OK!!!(%d, %d)\n", __func__, mxtinfo.family_id, mxtinfo.variant_id);
		}

	}while(retry_time-- > 0 && (mxtinfo.family_id != MXT680U2_FAMILY_ID));


	if (mxtinfo.family_id != MXT680U2_FAMILY_ID || mxtinfo.variant_id != MXT680U2_VARIANT_ID){
		if(mxt_check_is_bootloader(tdev)){
			is_bootloader_mode = 1;
			T117_address = SSL_SYNC_DATA_REG_DEFALUT_ADDR; 		//set default value
			return 0;
		}
		THP_LOG_ERR("chip is not identified, try to check with bootloader mode \n");
		return -ENODEV;
	}

	curr_size += mxtinfo.object_num * sizeof(struct mxt_object) + MXT_INFO_CHECKSUM_SIZE;
	buff = (u8 *)kzalloc(curr_size, GFP_KERNEL);

	if(NULL == buff){
		THP_LOG_ERR("%s:buff  alloc faild\n", __func__);
		ret_val = -ENOMEM;
		goto error_free;
	}
	ret_val = mxt_read_blks(tdev,tdev->thp_core->sdev,MXT_OBJECT_START,curr_size ,
							buff ,0/*override_limit*/);
	if (ret_val){
		THP_LOG_ERR("mxt_read_blks--info block reading error\n");
		goto error_free;
	}

	object_table = (struct mxt_object *)(buff);
	for (i = 0; i < mxtinfo.object_num; i++){
		object = object_table + i;
		// object start_address from little endian to local CPU endianness **IN PLACE**
		// IMPORTANT: this is only for the first loop through the object table
		le16_to_cpus(&object->start_address);
		switch (object->type){
			case SSL_T117_ADDR:
				T117_address = object->start_address;
				break;
			case SSL_T7_ADDR:
				T7_address = object->start_address;
				break;
			case SSL_T6_ADDR:
				T6_address = object->start_address;
				break;
			case SSL_T37_ADDR:
				T37_address = object->start_address;
				break;
			case SSL_T118_ADDR:
				T118_address = object->start_address;
				break;
			default :
				THP_LOG_ERR("%s error: unkown start object",__func__);
				break;
		}
	}

	if (0 == T117_address){
		T117_address = SSL_SYNC_DATA_REG_DEFALUT_ADDR;
		THP_LOG_ERR("Got T117_address fails, set to default value  %d\n", T117_address );
	}

error_free:
	if(buff != NULL){
		kfree(buff);
		buff = NULL;
	}
	return ret_val;
}

static int thp_ssl_get_frame(struct thp_device *tdev, char *buf, unsigned int len)
{
	int ret_val = -EINVAL;

	if (!tdev || !buf) {
		THP_LOG_INFO("%s: input dev or buf null\n", __func__);
		return -ENOMEM;
	}

	if (!len) {
		THP_LOG_INFO("%s: read len illegal\n", __func__);
		return -ENOMEM;
	}
	ret_val = mxt_read_blks(tdev,tdev->thp_core->sdev, T117_address, len, buf, T117_BYTES_READ_LIMIT);
	return ret_val;
}

static int thp_ssl_resume(struct thp_device *tdev)
{
	int ret = 0;
	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}

	if (gs_thp_udfp_stauts) {
		THP_LOG_INFO("enable frame data report\n");
		ret = mxt_framedata_output_disable(tdev,false);
		if (ret)
			THP_LOG_ERR("failed enable frame data report\n");
	} else if(is_pt_test_mode(tdev)){
		gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
		thp_do_time_delay(tdev->timing_config.resume_reset_after_delay_ms);//1ms
		gpio_set_value(tdev->gpios->rst_gpio, GPIO_HIGH);
	}else{
		mxt_power_on(tdev);
	}
	return 0;
}

static int thp_ssl_suspend(struct thp_device *tdev)
{
	u8 T7_ACTIVE[SSL_T7_COMMAMD_LEN] = {8,8};
	int ret = 0;
	THP_LOG_INFO("%s: called\n", __func__);
	if (!tdev) {
		THP_LOG_ERR("%s: tdev is  null\n", __func__);
		return -EINVAL;
	}
	gs_thp_udfp_stauts = thp_get_status(THP_STAUTS_UDFP);
	if (gs_thp_udfp_stauts) {//To avoid THP_STAUTS_UDFP be changed in suspend status.
		THP_LOG_INFO("disable frame data report\n");
		ret = mxt_framedata_output_disable(tdev,true);
		if (ret)
			THP_LOG_ERR("failed disable frame data report\n");
	} else if(is_pt_test_mode(tdev)){
		THP_LOG_INFO("%s: suspend PT mode \n", __func__);
		if(T7_address != 0){
			ret = mxt_write_blks(tdev,tdev->thp_core->sdev, T7_address, SSL_T7_COMMAMD_LEN, &T7_ACTIVE);
			if (ret != 0){
				THP_LOG_ERR( "Failed to send T7 always active command");
				return -EINVAL;
			}
		return -EINVAL;
		}
	}else{
		gpio_set_value(tdev->gpios->rst_gpio, 0);
		gpio_set_value(tdev->gpios->cs_gpio, 0);
		mxt_power_off(tdev);
	}
	return 0;
}

static void thp_ssl_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	if (tdev){
		if(tdev->tx_buff){
			kfree(tdev->tx_buff);
			tdev->tx_buff = NULL;
		}
		if(tdev->rx_buff){
			kfree(tdev->rx_buff);
			tdev->rx_buff = NULL;
		}
		kfree(tdev);
		tdev = NULL;
	}
}

static int thp_ssl_afe_notify_callback(struct thp_device *tdev, unsigned long event)
{
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	return thp_ssl_update_obj_addr(tdev);
}


struct thp_device_ops ssl_dev_ops = {
	.init = thp_ssl_init,
	.detect = thp_ssl_chip_detect,
	.get_frame = thp_ssl_get_frame,
	.resume = thp_ssl_resume,
	.suspend = thp_ssl_suspend,
	.get_project_id = thp_ssl_get_project_id,
	.exit = thp_ssl_exit,
	.afe_notify = thp_ssl_afe_notify_callback,
};

static int __init thp_ssl_module_init(void)
{
	int rc;
	struct thp_device *dev;

	THP_LOG_INFO("%s: called \n", __func__);
	dev = kzalloc(sizeof(struct thp_device), GFP_KERNEL);
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

	dev->ic_name = SSL_IC_NAME;
	dev->ops = &ssl_dev_ops;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	} else
		THP_LOG_INFO("%s: register success\n", __func__);

	return rc;
err:
	thp_ssl_exit(dev);
	return rc;
}

static void __exit thp_ssl_module_exit(void)
{
	THP_LOG_ERR("%s: called \n", __func__);
};

module_init(thp_ssl_module_init);
module_exit(thp_ssl_module_exit);
