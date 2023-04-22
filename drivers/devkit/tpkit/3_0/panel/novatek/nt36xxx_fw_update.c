#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
//#include <linux/fs.h>
#include <asm/uaccess.h>
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif


#include "nt36xxx.h"

#define I2C_BLDR_Address 0x01
#define FW_BIN_SIZE_116KB 118784
#define FW_BIN_SIZE FW_BIN_SIZE_116KB
#define FW_BIN_VER_OFFSET 0x1A000
#define FW_BIN_VER_BAR_OFFSET 0x1A001
#define FW_BIN_CHIP_ID_OFFSET 0x1A00E
#define FW_BIN_CHIP_ID 8
#define FLASH_SECTOR_SIZE 4096
#define SIZE_64KB 65536
#define BLOCK_64KB_NUM 4

#if defined (CONFIG_HUAWEI_DSM)
enum FW_uptate_state
{
	Nova_Init_BootLoader_fail = 0,
	Nova_Resume_PD_fail,
	Erase_Flash_fail,
	Write_Flash_fail,
	Verify_Flash_fail,
	TS_UPDATE_STATE_UNDEFINE = 255,
};
#endif


struct nvt_ts_firmware {
	size_t size;
	u8 *data;
};
static struct nvt_ts_firmware nvt_ts_fw_entry;
static struct nvt_ts_firmware *fw_entry = &nvt_ts_fw_entry;

static const struct firmware *fw_entry_boot = NULL;
int g_nava_sd_force_update = 0;

extern char novatek_kit_project_id[];
extern uint8_t nvt_fw_ver;
extern struct nvt_ts_data *nvt_ts;

extern int32_t novatek_ts_kit_read(uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t novatek_ts_kit_write(uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_kit_bootloader_reset(void);
extern int32_t nvt_kit_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern void nvt_kit_sw_reset_idle(void);
extern int8_t nvt_kit_get_fw_info(void);
extern int32_t novatek_kit_read_projectid(void);
extern void nvt_stop_crc_reboot(void);

/*******************************************************
Description:
	Novatek touchscreen request update firmware function.

return:
	Executive outcomes. 0---succeed. -1,-22---failed.
*******************************************************/
static int32_t update_firmware_request(char *filename)
{
	int32_t ret = 0;

	if (NULL == filename) {
		return -1;
	}

	TS_LOG_INFO("%s: filename is %s\n", __func__, filename);

	ret = request_firmware(&fw_entry_boot, filename, &nvt_ts->ts_dev->dev);
	if (ret) {
		TS_LOG_ERR("%s: firmware load failed, ret=%d\n", __func__, ret);
		return ret;
	}
	fw_entry->size = fw_entry_boot->size;
	fw_entry->data = (u8 *)fw_entry_boot->data;
	TS_LOG_ERR("%s: fw_entry->size=%lu\n", __func__, fw_entry->size);

	// check bin file size (116kb)
	if (fw_entry->size > FW_BIN_SIZE) {
		TS_LOG_ERR("%s: bin file size not match. (%zu)\n", __func__, fw_entry->size);
		return -EINVAL;
	}

	// check if FW version add FW version bar equals 0xFF
	if (*(fw_entry->data + FW_BIN_VER_OFFSET) + *(fw_entry->data + FW_BIN_VER_BAR_OFFSET) != 0xFF) {
		TS_LOG_ERR("%s: bin file FW_VER + FW_VER_BAR should be 0xFF!\n", __func__);
		TS_LOG_ERR("%s: FW_VER=0x%02X, FW_VER_BAR=0x%02X\n", __func__, *(fw_entry->data+FW_BIN_VER_OFFSET), *(fw_entry->data+FW_BIN_VER_BAR_OFFSET));
		return -EINVAL;
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen release update firmware function.

return:
	n.a.
*******************************************************/
static void update_firmware_release(void)
{
	if (fw_entry_boot) {
		release_firmware(fw_entry_boot);
	}
	fw_entry_boot=NULL;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware version function.

return:
	Executive outcomes. 0---need update. 1---need not
	update.
*******************************************************/
static int32_t Check_FW_Ver(void)
{
	uint8_t buf[16] = {0};
	int32_t ret = 0;

	//write i2c index to 0x11E00
	buf[0] = 0xFF;
	buf[1] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (nvt_ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = novatek_ts_kit_write(I2C_BLDR_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: i2c write error!(%d)\n", __func__, ret);
		return ret;
	}

	//read Firmware Version
	buf[0] = 0x78;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = novatek_ts_kit_read(I2C_BLDR_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: i2c read error!(%d)\n", __func__, ret);
		return ret;
	}

	TS_LOG_INFO("IC FW Ver = 0x%02X, FW Ver Bar = 0x%02X\n", buf[1], buf[2]);
	TS_LOG_INFO("Bin FW Ver = 0x%02X, FW ver Bar = 0x%02X\n",
			fw_entry->data[FW_BIN_VER_OFFSET], fw_entry->data[FW_BIN_VER_BAR_OFFSET]);

	// check IC FW_VER + FW_VER_BAR equals 0xFF or not, need to update if not
	if ((buf[1] + buf[2]) != 0xFF) {
		TS_LOG_ERR("%s: IC FW_VER + FW_VER_BAR not equals to 0xFF!\n", __func__);
		return 0;
	}

	// compare IC and binary FW version
	if (buf[1] >= fw_entry->data[FW_BIN_VER_OFFSET])
		return 1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen resume from deep power down function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Nova_Resume_PD(void)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	// Resume Command
	buf[0] = 0x00;
	buf[1] = 0xAB;
	ret = novatek_ts_kit_write(I2C_HW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Write Enable error!!(%d)\n", __func__, ret);
		return ret;
	}
	msleep(10);

	// Check 0xAA (Resume Command)
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Resume Command) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != 0xAA) {
		TS_LOG_ERR("%s: Check 0xAA (Resume Command) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}
	msleep(10);

	TS_LOG_INFO("Resume PD OK\n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware checksum function.

return:
	Executive outcomes. 0---checksum not match.
	1---checksum match. -1--- checksum read failed.
*******************************************************/
static int32_t Check_CheckSum(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = nvt_ts->mmap->READ_FLASH_CHECKSUM_ADDR;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	ret = Nova_Resume_PD();
	if(ret)
	{
		TS_LOG_INFO("Nova_Resume_PD failed ret = %d", ret);
	}

	fw_bin_size = fw_entry->size;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			// Calculate WR_Filechksum of each 64KB block
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = novatek_ts_kit_write(I2C_HW_Address, buf, 7);
			if (ret < 0) {
				TS_LOG_ERR("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
				if (ret < 0) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}
			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = novatek_ts_kit_write(I2C_BLDR_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = novatek_ts_kit_read(I2C_BLDR_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				TS_LOG_ERR("RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				TS_LOG_ERR("%s: firmware checksum not match!!\n", __func__);
				return 0;
			}
		}
	}

	TS_LOG_INFO("%s: firmware checksum match\n", __func__);
	return 1;
}

/*******************************************************
Description:
	Novatek touchscreen initial bootloader and flash
	block function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Nova_Init_BootLoader(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;

	// SW Reset & Idle
	nvt_kit_sw_reset_idle();

	// Initiate Flash Block
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = I2C_FW_Address;
	ret = novatek_ts_kit_write(I2C_HW_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: Inittial Flash Block error!!(%d)\n", __func__, ret);
		return ret;
	}
	msleep(5);

	// Check 0xAA (Initiate Flash Block)
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Inittial Flash Block) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != 0xAA) {
		TS_LOG_ERR("%s: Check 0xAA (Inittial Flash Block) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}

	TS_LOG_INFO("Init OK \n");
	msleep(20);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen erase flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t Erase_Flash(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = NO_ERR;
	int32_t count = NO_ERR;
	int32_t i = NO_ERR;
	int32_t Flash_Address = NO_ERR;
	int32_t retry = NO_ERR;

	// Write Enable
	buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
	buf[1] = NVTTDDI_ZERO_SIX_CMD;
	ret = novatek_ts_kit_write(I2C_HW_Address, buf, NVTTDDI_TWO_BYTES_LENGTH);
	if (ret < NO_ERR) {
		TS_LOG_ERR("%s: Write Enable (for Write Status Register) error!!(%d)\n", __func__, ret);
		return ret;
	}
	mdelay(NVTTDDI_DELAY_10_MS);
	// Check 0xAA (Write Enable)
	buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
	buf[1] = NVTTDDI_DOUBLE_ZERO_CMD;
	ret = novatek_ts_kit_read(I2C_HW_Address, buf, NVTTDDI_TWO_BYTES_LENGTH);
	if (ret < NO_ERR) {
		TS_LOG_ERR("%s: Check 0xAA (Write Enable for Write Status Register) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != NVTTDDI_DOUBLE_A_CMD) {
		TS_LOG_ERR("%s: Check 0xAA (Write Enable for Write Status Register) error!! status=0x%02X\n", __func__, buf[1]);
		return NVTTDDI_ERR;
	}
	mdelay(NVTTDDI_DELAY_10_MS);

	// Write Status Register
	buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
	buf[1] = NVTTDDI_ZERO_ONE_CMD;
	buf[2] = NVTTDDI_DOUBLE_ZERO_CMD;
	ret = novatek_ts_kit_write(I2C_HW_Address, buf, NVTTDDI_THREE_BYTES_LENGTH);
	if (ret < 0) {
		TS_LOG_ERR("%s: Write Status Register error!!(%d)\n", __func__, ret);
		return ret;
	}
	mdelay(NVTTDDI_DELAY_10_MS);
	// Check 0xAA (Write Status Register)
	buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
	buf[1] = NVTTDDI_DOUBLE_ZERO_CMD;
	ret = novatek_ts_kit_read(I2C_HW_Address, buf, NVTTDDI_TWO_BYTES_LENGTH);
	if (ret < NO_ERR) {
		TS_LOG_ERR("%s: Check 0xAA (Write Status Register) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != NVTTDDI_DOUBLE_A_CMD) {
		TS_LOG_ERR("%s: Check 0xAA (Write Status Register) error!! status=0x%02X\n", __func__, buf[1]);
		return NVTTDDI_ERR;
	}
	mdelay(NVTTDDI_DELAY_10_MS);

	// Read Status
	retry = NO_ERR;
	while (1) {
		mdelay(NVTTDDI_DELAY_30_MS);
		buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
		buf[1] = NVTTDDI_ZERO_FIVE_CMD;
		ret = novatek_ts_kit_write(I2C_HW_Address, buf, NVTTDDI_TWO_BYTES_LENGTH);
		if (ret < NO_ERR) {
			TS_LOG_ERR("%s: Read Status (for Write Status Register) error!!(%d)\n", __func__, ret);
			return ret;
		}

		// Check 0xAA (Read Status)
		buf[0] = NVTTDDI_DOUBLE_ZERO_CMD;
		buf[1] = NVTTDDI_DOUBLE_ZERO_CMD;
		buf[2] = NVTTDDI_DOUBLE_ZERO_CMD;
		ret = novatek_ts_kit_read(I2C_HW_Address, buf, NVTTDDI_THREE_BYTES_LENGTH);
		if (ret < NO_ERR) {
			TS_LOG_ERR("%s: Check 0xAA (Read Status for Write Status Register) error!!(%d)\n", __func__, ret);
			return ret;
		}
		if ((buf[1] == NVTTDDI_DOUBLE_A_CMD) && (buf[2] == NVTTDDI_DOUBLE_ZERO_CMD)) {
			break;
		}
		retry++;
		if (unlikely(retry > NVTTDDI_RETRY_5_TIMES)) {
			TS_LOG_ERR("%s: Check 0xAA (Read Status for Write Status Register) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
			return NVTTDDI_ERR;
		}
	}

	if (fw_entry->size % FLASH_SECTOR_SIZE)
		count = fw_entry->size / FLASH_SECTOR_SIZE + 1;
	else
		count = fw_entry->size / FLASH_SECTOR_SIZE;

	for(i = 0; i < count; i++) {
		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = novatek_ts_kit_write(I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Write Enable error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		mdelay(10);

		// Check 0xAA (Write Enable)
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Check 0xAA (Write Enable) error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		if (buf[1] != 0xAA) {
			TS_LOG_ERR("%s: Check 0xAA (Write Enable) error!! status=0x%02X\n", __func__, buf[1]);
			return -1;
		}
		mdelay(10);

		Flash_Address = i * FLASH_SECTOR_SIZE;

		// Sector Erase
		buf[0] = 0x00;
		buf[1] = 0x20;    // Command : Sector Erase
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		ret = novatek_ts_kit_write(I2C_HW_Address, buf, 5);
		if (ret < 0) {
			TS_LOG_ERR("%s: Sector Erase error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		mdelay(20);

		retry = 0;
		while (1) {
			// Check 0xAA (Sector Erase)
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Sector Erase) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Sector Erase) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
			mdelay(10);
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(30);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = novatek_ts_kit_write(I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Status error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = novatek_ts_kit_read(I2C_HW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if ((buf[1] == 0xAA) && (buf[2] == 0x00)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s:Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
	}

	TS_LOG_INFO("Erase OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen write flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
#define NVT_ONE_PAGE_SIZE 256
static int32_t Write_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = nvt_ts->mmap->RW_FLASH_DATA_ADDR;
	uint32_t Flash_Address = 0;
	int32_t i = 0, j = 0, k = 0;
	uint8_t tmpvalue = 0;
	int32_t count = 0;
	int32_t ret = 0;
	int32_t retry = 0;

	// change I2C buffer index
	buf[0] = 0xFF;
	buf[1] = XDATA_Addr >> 16;
	buf[2] = (XDATA_Addr >> 8) & 0xFF;
	ret = novatek_ts_kit_write(I2C_BLDR_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: change I2C buffer index error!!(%d)\n", __func__, ret);
		return ret;
	}

	if (fw_entry->size % NVT_ONE_PAGE_SIZE)
		count = fw_entry->size / NVT_ONE_PAGE_SIZE + 1;
	else
		count = fw_entry->size / NVT_ONE_PAGE_SIZE;

	for (i = 0; i < count; i++) {
		Flash_Address = i * NVT_ONE_PAGE_SIZE;

		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = novatek_ts_kit_write(I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Write Enable error!!(%d)\n", __func__, ret);
			return ret;
		}
		udelay(100);

		// Write Page : 256 bytes
		for (j = 0; j < min(fw_entry->size - i * NVT_ONE_PAGE_SIZE, (size_t)NVT_ONE_PAGE_SIZE); j += 32) {
			buf[0] = (XDATA_Addr + j) & 0xFF;
			for (k = 0; k < 32; k++) {
				buf[1 + k] = fw_entry->data[Flash_Address + j + k];
			}
			ret = novatek_ts_kit_write(I2C_BLDR_Address, buf, 33);
			if (ret < 0) {
				TS_LOG_ERR("%s: Write Page error!!(%d), j=%d\n", __func__, ret, j);
				return ret;
			}
		}
		if (fw_entry->size - Flash_Address >= NVT_ONE_PAGE_SIZE)
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (NVT_ONE_PAGE_SIZE -1);
		else
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (fw_entry->size - Flash_Address - 1);

		for (k = 0;k < min(fw_entry->size - Flash_Address,(size_t)NVT_ONE_PAGE_SIZE); k++)
			tmpvalue += fw_entry->data[Flash_Address + k];

		tmpvalue = NVT_ONE_PAGE_SIZE - tmpvalue ;

		// Page Program
		buf[0] = 0x00;
		buf[1] = 0x02;
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		buf[5] = 0x00;
		buf[6] = min(fw_entry->size - Flash_Address,(size_t)NVT_ONE_PAGE_SIZE) - 1;
		buf[7] = tmpvalue;
		ret = novatek_ts_kit_write(I2C_HW_Address, buf, 8);
		if (ret < 0) {
			TS_LOG_ERR("%s: Page Program error!!(%d), i=%d\n", __func__, ret, i);
			return ret;
		}

		// Check 0xAA (Page Program)
		retry = 0;
		while (1) {
			mdelay(3);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Page Program error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (buf[1] == 0xAA || buf[1] == 0xEA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Page Program) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			TS_LOG_ERR("%s: Page Program error!! i=%d\n", __func__, i);
			return -3;
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(2);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = novatek_ts_kit_write(I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Status error!!(%d)\n", __func__, ret);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = novatek_ts_kit_read(I2C_HW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (((buf[1] == 0xAA) && (buf[2] == 0x00)) || (buf[1] == 0xEA)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			TS_LOG_ERR("%s: Page Program error!! i=%d\n", __func__, i);
			return -4;
		}

		TS_LOG_INFO("Programming...%2d%%\r", ((i * 100) / count));
	}

	TS_LOG_INFO("Programming...%2d%%\r", 100);
	TS_LOG_INFO("Program OK         \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen verify checksum of written
	flash function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t Verify_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = nvt_ts->mmap->READ_FLASH_CHECKSUM_ADDR;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			// Calculate WR_Filechksum of each 64KB block
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = novatek_ts_kit_write(I2C_HW_Address, buf, 7);
			if (ret < 0) {
				TS_LOG_ERR("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = novatek_ts_kit_read(I2C_HW_Address, buf, 2);
				if (ret < 0) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}
			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = novatek_ts_kit_write(I2C_BLDR_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = novatek_ts_kit_read(I2C_BLDR_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				TS_LOG_ERR("Verify Fail%d!!\n", i);
				TS_LOG_ERR("RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				return -1;
			}
		}
	}

	TS_LOG_INFO("Verify OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t Update_Firmware(void)
{
	int32_t ret = 0;

	//---Stop CRC check to prevent IC auto reboot---
	nvt_stop_crc_reboot();

	// Step 1 : initial bootloader
	ret = Nova_Init_BootLoader();
	if (ret) {
		return ret;
	}

	// Step 2 : Resume PD
	ret = Nova_Resume_PD();
	if (ret) {
		nvt_ts->chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = Nova_Resume_PD_fail;
		TS_LOG_ERR("%s:Nova_Resume_PD fail\n",__func__);
		return ret;
	}

	// Step 3 : Erase
	ret = Erase_Flash();
	if (ret) {
		nvt_ts->chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = Erase_Flash_fail;
		TS_LOG_ERR("%s:Erase_Flash fail\n",__func__);
		return ret;
	}

	// Step 4 : Program
	ret = Write_Flash();
	if (ret) {
		nvt_ts->chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = Write_Flash_fail;
		TS_LOG_ERR("%s:Write_Flash fail\n",__func__);
		return ret;
	}

	// Step 5 : Verify
	ret = Verify_Flash();
	if (ret) {
		nvt_ts->chip_data->ts_platform_data->dsm_info.constraints_UPDATE_status = Verify_Flash_fail;
		TS_LOG_ERR("%s:Verify_Flash fail\n",__func__);
		return ret;
	}

	//Step 6 : Bootloader Reset
	nvt_kit_bootloader_reset();
	nvt_kit_check_fw_reset_state(RESET_STATE_INIT);

	return ret;
}

int32_t nvt_kit_fw_update_boot(char *file_name)
{
	int32_t ret = 0;

	TS_LOG_INFO("%s: file_name=%s\n", __func__, file_name);

	TS_LOG_INFO("%s: current fw version=%02x\n", __func__, nvt_fw_ver);

	ret = update_firmware_request(file_name);
	if (ret) {
		TS_LOG_ERR("%s: update_firmware_request failed. (%d)\n", __func__, ret);
		return 0;
	}

	nvt_kit_sw_reset_idle();
	if (!g_nava_sd_force_update){
		//---prevent last time write step was discontinued by unpredictable power off, Taylor 20160715---
		ret = Check_CheckSum();
		//--------------------------------------------------------------------------------
		 if (((ret == 0) && (Check_FW_Ver() == 0))||(ret < 0)) {	// (ic fw ver check failed) && (bin fw version > ic fw version)
			TS_LOG_INFO("%s: firmware version not match or checksum failed.updata fw\n", __func__);
			ret = Update_Firmware();
			if(ret) {
				TS_LOG_INFO("%s: updata firmware failed dmd report.\n", __func__);
			}
		} else {
			TS_LOG_INFO("%s: read firmware checksum match or IC FW version bigger, no need update.\n",__func__);
			// Bootloader Reset
			ret = 0;
			nvt_kit_bootloader_reset();
			nvt_kit_check_fw_reset_state(RESET_STATE_INIT);
		}
	}else{
		g_nava_sd_force_update = 0;

		TS_LOG_INFO("%s: fw force update\n", __func__);
		ret = Update_Firmware();
	}

	update_firmware_release();

	//get project id and fw version
	novatek_kit_read_projectid();
	nvt_kit_get_fw_info();
	TS_LOG_INFO("%s: new fw version=%02x\n", __func__, nvt_fw_ver);

	return ret;
}
