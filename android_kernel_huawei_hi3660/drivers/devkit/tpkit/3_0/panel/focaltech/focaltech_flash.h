

#ifndef __FOCALTECH_FLASH_H__
#define __FOCALTECH_FLASH_H__

#include "focaltech_core.h"
#include "focaltech_dts.h"

#define FTS_CMD_READ_FLASH		0x03
#define FTS_CMD_READ_PRAM		0x85

#define FTS_BOOT_VENDER_ID_ADDR1	0x00
#define FTS_BOOT_VENDER_ID_ADDR2	0x04
#define FTS_BOOT_PROJ_CODE_ADDR1	0x00
#define FTS_BOOT_PROJ_CODE_ADDR2	0x20

#define FTS_REG_RESET_FW		0x07

#define FTS_UPGRADE_AA			0xAA
#define FTS_UPGRADE_55			0x55

#define FTS_REG_CHIP_ID_H		0xA3
#define FTS_REG_CHIP_ID_L		0x9F
#define FTS_REG_FW_VER			0xA6

#define FTS_REG_VENDOR_ID		0xA8
#define LEN_FLASH_ECC_MAX		0xFFFE
#define FTS_8201_LEN_FLASH_ECC_MAX  (96*1024)
#define FT8006U_LEN_FLASH_ECC_MAX		(96*1024)
#define FTS_REG_PROJ_CODE		0x99
#define FTS_REG_OFFSET_CODE		0x90

#define FTS_MAX_PRAM_SIZE		0x10000

#define FTS_REG_SOFT_RESET_FC		0xFC
#define FTS_REG_BOOT_CHIP_ID		0x90

#define FTS_CMD_WRITE_PRAM		0xAE
#define FTS_CMD_WRITE_FLASH		0xBF

#define FTS_CMD_START_APP		0x08

#define FTS_CMD_REBOOT_APP		0x07

#define FTS_CMD_SET_MODE		0x09
#define FTS_UPGRADE_MODE		0X0B
#define FTS_CMD_ERASURE_APP		0x61
#define FTS_CMD_GET_STATUS		0x6A

#define FTS_CMD_CALC_CRC		0x64
#define FTS_CMD_SET_CALC_ADDR		0x65
#define FTS_CMD_READ_CRC		0x66

#define FTS_REG_FLASH_TYPE		0x05
#define FTS_FLASH_TYPE_WINBOND		0x81
#define FTS_FLASH_TYPE_FT5003		0x80
#define FTS_FLASH_MAX_LEN_WINBOND	(1024*60)
#define FTS_FLASH_MAX_LEN_FT5003	(1024*64)
#define FTS_8201_FLASH_MAX_LEN	    (1024*96)
#define FTS_ERASURE_OK_STATUS		0xF0AA
#define FTS_ECC_OK_STATUS		0xF055

#define FTS_MODEL_SET_NO		2
#define FTS_FW_NAME_LEN			64
#define FTS_WORK_MODE_ADDR		0x00
#define FTS_WORK_MODE_VALUE		0x00

#define FTS_REG_SLEEP			0xA5

#define FTS_FW_IMG_ADDR_VER		0x10E
#define FTS_FW_IMG_ADDR_VER_OFFSET_5X46		2
#define FTS_FW_IC_ADDR_START		0x1000
#define FTS_8201_FW_IC_ADDR_START		0x5000
#define FT8006U_FW_IC_ADDR_START		0x2000
#define FTS_FW_WRITE_STATUS_ADDR_START		0x1000
#define FTS_FW_SEND_LENGTH	0xB0
#define FTS_RETRY_TIMES	10

#define FTS_CHANGE_I2C_HID2STD_ADDR1		0xEB
#define FTS_CHANGE_I2C_HID2STD_ADDR2		0xAA
#define FTS_CHANGE_I2C_HID2STD_ADDR3		0x09
#define FTS_CHANGE_I2C_HID2STD_STATUS	0x08

#define PRAM_NAME_DEFAULT_LEN		64

#define FTS_SLEEP_TIME_1		1
#define FTS_SLEEP_TIME_5		5

#define AL2_FCS_COEF					((1 << 15) + (1 << 10) + (1 << 3))
#define FTS_ROMBOOT_CMD_ECC				0xCC
#define FTS_ROMBOOT_CMD_ECC_LEN			7
#define FTS_ROMBOOT_CMD_ECC_FINISH		0xCE
#define FTS_ROMBOOT_CMD_ECC_READ		0xCD

#define FTS_ROMBOOT_CMD_GET_STA			0xF2
#define PROJECT_ID_ADDR					0x007812

struct focal_delay_time {
	int hard_reset_delay;
	int erase_min_delay;
	int calc_crc_delay;
	int reboot_delay;
	int erase_query_delay;

	int write_flash_query_times;
	int read_ecc_query_times;
	int erase_flash_query_times;
	int upgrade_loop_times;
};

int focal_flash_init(struct i2c_client *client);
int focal_flash_exit(void);
int focal_hardware_reset(int model);
int focal_read_vendor_id_from_pram(struct focal_platform_data *focal_pdata,
	u8 *vendor_id);
int focal_read_project_id_from_app(u8 *buf, size_t size);
int focal_read_project_id_from_pram(struct focal_platform_data *focal_pdata,
	char *project_id, size_t size);
bool focal_read_pram_package(u32 offset, u8 *buf, u16 size);
int focal_enter_work_model(void);
int focal_enter_rom_update_model_by_software(struct focal_platform_data *focal_pdata);
int focal_read_chip_id_(u32 *chip_id);
int focal_firmware_auto_update(struct focal_platform_data *focal_pdata,
	const char *product_name);
int focal_firmware_manual_update(struct focal_platform_data *focal_pdata,
	const char *fw_name);
int focal_read_chip_id_from_rom(struct focal_platform_data *focal_pdata,
	u32 *chip_id);
int focal_get_ic_firmware_version(u8 *ver);
int focal_read_project_id(struct focal_platform_data *focal_pdata, char *project_id,
	size_t size);
int focal_read_chip_id_from_app(u32 *chip_id);
int focal_read_chip_id(struct focal_platform_data *focal_pdata,
	u32 *chip_id);
int focal_read_vendor_id(struct focal_platform_data *focal_pdata, u8 *vendor_id);
int focal_flash_upgrade_with_bin_file(struct focal_platform_data *focal_pdata,
	char *fw_name);
int focal_software_reset_to_bootloader(struct focal_platform_data *focal_pdata);
#endif

