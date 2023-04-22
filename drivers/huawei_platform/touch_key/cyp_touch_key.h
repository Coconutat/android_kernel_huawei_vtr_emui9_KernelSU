

#ifndef _CYTTSP_BUTTON_H_
#define _CYTTSP_BUTTON_H_

#include <linux/types.h>

/* operate command list */
#define CYTTSP_CMD_VERIFY_CHKSUM		0x31
#define CYTTSP_CMD_GET_FLASH_SIZE		0x32
#define CYTTSP_CMD_GET_APP_STATUS     	0x33
#define CYTTSP_CMD_ERASE_ROW			0x34
#define CYTTSP_CMD_CLEAR_BUF			0x35
#define CYTTSP_CMD_SET_ACTIVE_APP      	0x36
#define CYTTSP_CMD_SEND_DATA			0x37
#define CYTTSP_CMD_ENTER_BTLD			0x38
#define CYTTSP_CMD_PROGRAM_ROW			0x39
#define CYTTSP_CMD_VERIFY_ROW			0x3A
#define CYTTSP_CMD_EXIT_BTLD			0x3B
#define CYTTSP_ENABLE_CAP_TEST			0x4E
#define CYTTSP_NEW_VERSION_APP			0x34
/* register map list */
#define CYTTSP_POLLUTION_SLAVE_ADDR		0x27
#define CYTTSP_DEFAULT_BL_ADDR			0x28
#define CYTTSP_NEW_BL_ADDR				0x56
#define CYTTSP_REG_TOUCHMODE			0x00
#define CYTTSP_REG_WORKMODE				0x01
#define CYTTSP_REG_RESET				0x02
#define CYTTSP_REG_MOD_IDAC_0			0x08
#define CYTTSP_REG_MOD_IDAC_1			0x09
#define CYTTSP_REG_COMP_IDAC_0			0x0B
#define CYTTSP_REG_COMP_IDAC_1			0x0C
#define CYTTSP_REG_F_THRESHOLD_0		0x0E
#define CYTTSP_REG_F_THRESHOLD_1		0x10
#define CYTTSP_REG_N_THRESHOLD_0		0x14
#define CYTTSP_REG_N_THRESHOLD_1		0x16
#define CYTTSP_REG_BASELINE				0x1A
#define CYTTSP_REG_CAP_EN				0x1B
#define CYTTSP_REG_LOCKDOWN_INFO		0x63
#define CYTTSP_REG_HWVERSION			0x48
#define CYTTSP_REG_RAW_COUNT_0			0x4B
#define CYTTSP_REG_RAW_COUNT_1			0x4D
#define CYTTSP_REG_BASELINE_B0			0x51
#define CYTTSP_REG_BASELINE_B1			0x53
#define CYTTSP_REG_INVALID				0xFF
#define CYTTSP_SILICON_VER1				0x40
#define CYTTSP_BTLD_VER1				0x44
#define CYTTSP_BTLD_VER2				0x45
#define CYTTSP_BUTTON_FW_VER1			0x46
#define CYTTSP_BUTTON_FW_VER2			0x47
#define CYTTSP_RAW_COUNT_BTN0_L			0x4B
#define CYTTSP_RAW_COUNT_BTN0_H			0x4C
#define CYTTSP_RAW_COUNT_BTN1_L			0x4D
#define CYTTSP_RAW_COUNT_BTN1_H			0x4E
#define CYTTSP_SIGNAL_BTN0_L			0x57
#define CYTTSP_SIGNAL_BTN0_H			0x58
#define CYTTSP_SIGNAL_BTN1_L			0x59
#define CYTTSP_SIGNAL_BTN1_H			0x5A
#define CYTTSP_SPEED_MODE				0x5D
#define CYTTSP_CP_BTN0					0x5E
#define CYTTSP_CP_BTN1					0x5F
#define CYTTSP_CMOD						0x62
#define CYTTSP_CMOD_CHARGING			0x6C
#define CYTTSP_BTN0_RAW_START			0x66
#define CYTTSP_BTN1_RAW_START			0x68
#define CYTTSP_BTN0_RAW_CNT_START		0x4B
#define CYTTSP_BTN1_RAW_CNT_START		0x4D
//proc node
#define CYTTSP_TRIGGER_MEASURE_CP		0x4E
#define CYTTSP_DEFAULT_NOISE_LIMIT		0x28
#define CYTTSP_DEFAULT_IDAC_LIMIT_MAX	0xFFFF
#define CYTTSP_DEFAULT_IDAC_LIMIT_MIN	0x00
#define CYTTSP_DEFAULT_CMOD_LIMIT_MAX	0xFFFF
#define CYTTSP_DEFAULT_CMOD_LIMIT_MIN	0x00
#define CYTTSP_DEFAULT_CMOD_CHARGING_LIMIT	0xFFFF
#define CYTTSP_DEFAULT_SUPPORT_MMI		0x00
#define CYTTSP_DEFAULT_SUPPORT_RUNNING	0x00
#define CYTTSP_RAW_DATA_MIN				0x00
#define CYTTSP_RAW_DATA_MAX				0xFFFF
#define CYTTSP_CP_NUM					2
#define CYTTSP_CP_TEST_NUM				6
#define CYTTSP_RAW_DATA_NUM				10
#define CYTTSP_REASON_LENGTH			200
#define CYTTSP_READ_CP_DELAY			500
#define CYTTSP_READ_CMOD_DELAY			1000
//mmi
#define CYTTSP_TOTAL_KEY_NUM			2
#define CYTTSP_READ_NOISE_TIMES			10
#define CYTTSP_SUPPORT_FUNC				"1"
#define CYTTSP_NOT_SUPPORT_FUNC			"0"
#define CYTTSP_I2C_ERR					"reason: i2c err;"
#define CYTTSP_PARAM_NULL				"reason: param null;"
#define CYTTSP_I2C_OK					"i2c ok;"
#define CYTTSP_I2C_MMI_ERR				"-0F-1F-2F"
#define CYTTSP_READ_REG_ERR				"reason:read reg err;"
#define CYTTSP_WRITE_REG_ERR			"reason:write reg err;"
#define CYTTSP_NULL_DATA			"null"
#define CYTTSP_PROBE_FAILED			"reason:probe failed, i2c err!"
#define CYTTSP_IDAC_REG_FAILED			"reason:read idac reg err;"
#define CYTTSP_REASON_OF_TEST_ONE		0
#define CYTTSP_REASON_OF_TEST_TWO		1
#define CYTTSP_REASON_OF_TEST_THREE		2
#define CYTTSP_REASON_OF_TEST_FOUR		3
#define CYTTSP_REASON_OF_TEST_FIVE		4
#define CYTTSP_REASON_OF_TEST_SIX		5
#define CYTTSP_MMI_TEST_ONE				0
#define CYTTSP_MMI_TEST_TWO				1
#define CYTTSP_MMI_TEST_THREE			2
#define CYTTSP_MMI_TEST_FOUR			3
#define CYTTSP_MMI_TEST_FIVE			4
#define CYTTSP_MMI_TEST_SIX				5
#define CYTTSP_MMI_NEXT_TEST			1
#define CYTTSP_PARAM_NULL				1
#define CYTTSP_IDAC_NUM					2
#define CYTTSP_CMOD_NUM					2
#define CYTTSP_IDAC_MIN					0
#define CYTTSP_IDAC_MAX					1
#define CYTTSP_CMOD_MIN					0
#define CYTTSP_CMOD_MAX					1
#define CYTTSP_TEST_MAX					5
#define CYTTSP_TEST_MIN					0

/* mask value list */
#define CYTTSP_NORMAL_MODE				0x00
#define CYTTSP_DEEPSLEEP_MODE			0x01
#define CYTTSP_STS_SUCCESS				0x00
#define CYTTSP_BUTTON_OP_MODE			0x00
#define CYTTSP_PACKET_START				0x01
#define CYTTSP_PACKET_END				0x17
#define CYTTSP_MAX_PAYLOAD_LEN			0x15
#define CYTTSP_RESP_HEADER_LEN			0x04
#define CYTTSP_RESP_TAIL_LEN			0x03
#define CYTTSP_ENTER_BTLD_RESP_LEN		8
#define CYTTSP_GET_FLASHSZ_RESP_LEN		4
#define CYTTSP_8BITS_SHIFT				8
#define CYTTSP_16BITS_SHIFT				16
#define CYTTSP_8BITS_MASK				0xFF
#define CYTTSP_16BITS_MASK				0xFF00
#define CYTTSP_16BITS_FULL_MASK			0xFFFF

/* delay count list */
#define CYTTSP_WRITE_DELAY_COUNT		25
#define CYTTSP_WAKE_DELAY_COUNT 		100
#define CYTTSP_CAP_TEST_DELAY_COUNT		500
#define CYTTSP_FWUP_DELAY_COUNT 		1000

/* shift value list */
#define CYTTSP_GLOVE_MODE_SHIFT			2
#define CYTTSP_FW_VER_SIZE				2
#define CYTTSP_BTLD_VER_SIZE			2
#define CYTTSP_RAW_DATA_SIZE			2
#define CYTTSP_CAP_DATA_SIZE			2
#define CYTTSP_SILICON_VER_SIZE			4

/* magic number */
#define CYTTSP_PROGRAM_DATA_LEN			4
#define CYTTSP_AVAILABLE_DATA_LEN		70
#define CYTTSP_FW_ORG_DATA_LEN			512

#define CYTTSP_SYS_PARAM_READ			0
#define CYTTSP_SYS_PARAM_WRITE			1
#define CYTTSP_SYS_PARAM_REG_MAX		107
#define CYTTSP_SYS_PARAM_REG_MIN		0
#define CYTTSP_SYS_PARAM_VALUE_MAX		255
#define CYTTSP_SYS_PARAM_VALUE_MIN		0
#define CYTTSP_SYS_PARAM_LEN_ZERO		0
#define CYTTSP_SYS_PARAM_LEN_ONE		1
#define CYTTSP_SYS_PARAM_LEN_TWO		2
#define CYTTSP_I2C_BUF_SIZE 			2
#define CYTTSP_SENDND_COMMAND_BUF_SIZE	512
#define CYTTSP_SENDND_COMMAND_RESP_SIZE	50
#define CYTTSP_DEALY_AFTER_RESET		50
#define CYTTSP_NEED_UPDATE_SIZE 		2
#define CYTTSP_BTLD_CMD_BUFF			5
#define CYTTSP_LEN_OF_REG				4
#define CYTTSP_RESET_CMD				0x3D
#define CYTTSP_REG_TOUCHMODE_STR		"0x00"
#define CYTTSP_REG_RESET_STR			"0x02"
#define CYTTSP_ERR_CP_RET				0
#define CYTTSP_ERR_NOISE_RET			0
#define CYTTSP_KEY1						0
#define CYTTSP_KEY2						1
#define CYTTSP_NOISE_TEST_DELAY			20

/* DMD define */
#define CYTTSP_DSM_BUFFER_SIZE			1024
#define CYTTSP_DSM_TOUCH_KEY			"dsm_touch_key"
#define CYTTSP_DSM_DEVICE_NAME			"TOUCHK_KEY"
#define CYTTSP_DSM_IC_NAME			"cypress"
#define CYTTSP_DSM_MODULE_NAME			"NNN"
#define CYTTSP_DSM_EN_DISABLE_ERR		"failed to enable or disable ic, in dsm"
#define CYTTSP_DSM_FW_UPDATE_ERR		"failed to update fw"
#define CYTTSP_DSM_IRQ_I2C_ERR			"failed to read i2c in irq handler"
#define CYTTSP_DSM_PROBE_I2C_ERR_1		"fail to read btld slave addr, button's not on site"
#define CYTTSP_DSM_PROBE_I2C_ERR_2		"fail to switch to bootloader"
#define CYTTSP_DSM_PROBE_I2C_ERR_3		"failed to wake ic from sleep mode "

#define CYTTSP_INPUT_DEVICE_NAME		"touch_key"

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#ifndef TURN_ON
#define TURN_ON 1
#endif
#ifndef TURN_OFF
#define TURN_OFF 0
#endif

#ifndef PROC_ON
#define PROC_ON 1
#endif
#ifndef PROC_OFF
#define PROC_OFF 0
#endif

#ifndef I2C_NAME_SIZE
#define I2C_NAME_SIZE	20
#endif
#ifndef CYTTSP_I2C_DEFAULT_ADDR
#define CYTTSP_I2C_DEFAULT_ADDR 0x27
#endif

#define CYP_TOUCH_KEY_COMPATIBLE_ID	"huawei,cyp_touch_key"
#define CYP_DEFAULT_VDD_VOL 3300000

struct cyttsp_config_info {
	unsigned char panel_id;
	const char* fw_info;
};

struct cyttsp_button_platform_data {
	int irq_gpio;
	unsigned int irq_num;
	unsigned int irq_gpio_flags;
	unsigned long irq_flags;
	unsigned int vdd;
	unsigned long long vdd_vol;
	const char *input_name;
	int nbuttons;
	int *key_code;
	unsigned char button_status_reg;
	unsigned char bl_addr;
	unsigned char work_mode_reg;
	int default_config;
	int config_array_size;
	struct cyttsp_config_info *config_array;
	int cp_limit[CYTTSP_CP_NUM];
	int cypress_store_cp[CYTTSP_CP_NUM];
	int cypress_store_noise[CYTTSP_CP_NUM];
	unsigned int idac_limit[CYTTSP_IDAC_NUM];
	unsigned int cmod_limit[CYTTSP_CMOD_NUM];
	unsigned int cmod_charging_limit;
	unsigned int noise_limit;
	unsigned int if_support_mmi;
	unsigned int if_support_running;
};

struct cyttsp_button_data {
	struct i2c_client *client;
	struct cyttsp_button_platform_data *pdata;
	struct input_dev *input_dev;
	bool dbg_dump;
	unsigned long key_status;
	bool enable;
	unsigned char bl_addr;
	unsigned char app_addr;
#ifdef CONFIG_FB
	struct notifier_block fb_notif;
#endif
	struct notifier_block glove_mode_notif;
	bool glove_mode;
	unsigned char fw_version[CYTTSP_FW_VER_SIZE];
	struct work_struct touchkey_dsm_work_struct;
};

#endif
