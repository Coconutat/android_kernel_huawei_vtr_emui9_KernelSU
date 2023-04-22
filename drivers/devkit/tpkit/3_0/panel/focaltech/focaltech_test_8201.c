#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "focaltech_test.h"
#include "focaltech_core.h"
#include "focaltech_flash.h"
#include "../../huawei_ts_kit.h"


#define TX_8201_NUM_MAX         60
#define RX_8201_NUM_MAX         60
#define DEVICE_MODE_ADDR        0x00
#define FTS_REG_DATA_TYPE       0x01
#define MAX_ADC_NUM_8201                4050
#define REG_TX_NUM          0x02
#define REG_RX_NUM          0x03
#define REG_CB_ADDR_H           0x18
#define REG_CB_ADDR_L           0x19
#define REG_RAW_BUF0            0x6A
#define REG_CB_BUF0         0x6E
#define REG_CLB             0x04
#define REG_8201_LCD_NOISE_FRAME        0x12
#define REG_8201_LCD_NOISE_START        0X11
#define REG_8201_LCD_NOISE_VALUE        0X80
#define REG_VREF                        0x86
#define REG_DATA_TYPE           0x06
#define DATA_TYPE_RAW_DATA      0xAD
#define VREF_2V                         0x01
#define DEBUG_DETA_DIFF_DATA        0x01
#define DEBUG_DATA_RAW_DATA     0x00
#define REG_CA_FILTER                   0x5E
#define CA_FILTER_UPPER_LIMIT_VALUE     0x64
#define IS_DIFFER_MODE                  0x01
#define START_LCD_NOISE_VALUE           0X01
#define REG_RAWDATA_LINE_ADDR           0x01
#define TEST_TIMEOUT            99
#define LEFT_SHIFT_FOUR         32
#define PERCENTAGE_DATA         100
#define RAWDATA_LINE_ADDR_CLEAR         0xAD
#define FTS_CALIBRATION_DISABLE_REG     0xEE
#define REG_ADC_SCAN            0x0F
#define REG_ADC_SCAN_STATUS     0x10
#define I2C_DEFAULT_MASTER_ADDR         0x70
#define I2C_DEFAULT_SLAVE_ADDR          0x72
#define REG_ADC_DATA_ADDR       0x89
#define REG_CS_TYPE                     0X26
#define REG_MASTER_CHX                  0X50
#define REG_MASTER_CHY                  0X51
#define REG_GIP                 0x20
#define MAX_CB_VALUE            200
#define TP_TEST_FAILED_REASON_LEN           20
#define REG_SLAVE_CHX                   0X52
#define REG_SLAVE_CHY                   0X53
#define ERROR_CODE_OK           0x00
#define DELAY_TIME_OF_CALIBRATION       30

#define LCD_TEST_FRAMENUM       50
#define REG_LCD_NOISE_NUMBER        0X13
#define PRE             1
#define LCD_NOISE_DATA_PROCESS      0
#define REG_LCD_NOISE_DATA_READY    0x00
#define REG_BUF_OFFSET                  0X17
#define I2C_MASTER_VALUE                0X00
#define I2C_SLAVE_VALUE                 0X0C
#define REG_I2C_ADDR                    0X81
#define LCD_TEST_FRAMENUM_8201 50
#define I_IS_OPEN 1
static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
struct focal_test_params *param = NULL;
static int  tp_cap_test_status = TEST_SUCCESS;

#define CB_INCREASE_TIMEOUT				40
#define FTS_CB_INCREASE_TEST_DELAY_TIME	50
#define REG_8201_CB_INCREASE_LEVEL		0X9A
#define REG_8201_CB_INCREASE_START	    0X99
#define START_CB_INCREASE_VALUE	        0X55
#define CB_INCREASE_FINISH_VALUE	    0XAA
#define CB_INCREASE_RESET	            0X5A

enum CS_TYPE {
	CS_TWO_SINGLE_MASTER = 1,      /* double chip master*/
	CS_TWO_SINGLE_SLAVE = 0,       /*double chip slave*/
	CS_SINGLE_CHIP = 3,            /*signel chip*/
};
enum CS_DIRECTION {
	CS_LEFT_RIGHT = 0x00,         /*left right direction*/
	CS_UP_DOWN    = 0x40,         /*up down direction*/
};
enum CD_S0_ROLE {
	CS_S0_AS_MASTER = 0x00,      /*S0 as master*/
	CS_S0_AS_SLAVE  = 0x80,      /*S0 as slave*/
};
struct cs_info_packet {
	enum CS_TYPE             cs_type;        /*chip type*/
	enum CS_DIRECTION   cs_direction;       /*chip direction*/
	enum CD_S0_ROLE     cs_s0_role;        /*S0 is master or slave*/
	int                 cs_master_addr;     /*Master IIC Address*/
	int                 cs_slave_addr;      /*Slave IIC Address*/
	int                 cs_master_tx;       /*Master Tx*/
	int                 cs_master_rx;       /*Master Rx*/
	int                 cs_slave_tx;        /*Slave Tx*/
	int                 cs_slave_rx;        /*Slave Rx*/
};
struct focal_test_8201 {
	struct cs_info_packet *cs_info;
	int  current_slave_addr;

};
struct cs_chip_addr_mgr {
	struct focal_test_8201 *m_parent;
	u8 slave_addr;
};

static int focal_start_adc_scan(void);
static int focal_8201_get_adc_data(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y);
static int focal_get_adc_result(int *data, size_t size);
static int focal_get_adc_scan_result(void);
static int focal_read_noise(u8 *raw_data, size_t size);
static int focal_get_noise_format(unsigned int chl_x , unsigned int chl_y, int *data, size_t size);
static int focal_8201_get_short_circuit_data(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y);
static int focal_8201_get_open_test_data(struct focal_test_8201 *ft8201, int *data, unsigned int size, unsigned int chl_x, unsigned int chl_y);
static int focal_8201_get_cb_data_format(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y);
static int focal_8201_get_raw_data_format(struct focal_test_8201 *ft8201, unsigned int chl_x , unsigned int chl_y, int *data, size_t size);
static int focal_8201_get_noise_format(struct focal_test_8201 *ft8201, unsigned int chl_x , unsigned int chl_y, int *data, size_t size);
static int focal_8201_chip_clb(struct focal_test_8201 *ft8201);
static int focal_8201_enter_work(struct focal_test_8201 *ft8201);
static int focal_8201_enter_factory(struct focal_test_8201 *ft8201);
static int focal_8201_start_scan(struct focal_test_8201 *ft8201);
static int focal_8201_write_reg(struct focal_test_8201 *ft8201, u8 addr, u8 value);
static int focal_8201_set_slave_addr(struct focal_test_8201 *ft8201,  u8 slave_addr);
static int focal_start_scan(void);
static int focal_chip_clb(void);
static int focal_read_raw_data(u8 data_type, u8 *raw_data, size_t size);

static int focal_read_channel_x(u8 *chl_x);
static int focal_read_channel_y(u8 *chl_y);
static int focal_get_cb_data(int offset, u8 *data, size_t data_size);
static int focal_get_raw_data_format(unsigned int chl_x, unsigned int chl_y, int *data, size_t size);
static void focal_print_test_data(char *msg, int row, int col, int max,
								  int min, int value);

static void focal_put_test_result(struct focal_test_params *params,
								  struct ts_rawdata_info *info, struct focal_test_result *test_results[],
								  int size);

static int focal_alloc_test_container(struct focal_test_result **result,
									  size_t data_size);
static void focal_free_test_container(struct focal_test_result *result);

static int focal_get_channel_form_ic(struct focal_test_params *params);
static int focal_enter_work(void);
static int focal_enter_factory(void);
static int focal_get_cb_data_format(int *data, size_t size, unsigned int chl_x, unsigned int chl_y);
static int focal_8201_raw_data_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
									struct focal_test_result **result, char test_num);
static int focal_8201_cb_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
							  struct focal_test_result **result, char test_num);
static int focal_8201_open_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
								struct focal_test_result **result, char test_num);
static int focal_8201_short_circuit_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
		struct focal_test_result **result, char test_num);
static int focal_8201_noise_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
									 struct focal_test_result **result, char test_num);
static int focal_8201_get_channel_num(u8 *channel_x, u8 *channel_y);
static int focal_8201_init_test_prm(struct focal_platform_data *pdata,
	struct focal_test_params *params,struct ts_rawdata_info *info);
static int focal_cb_uniformity_test(struct focal_test_params *params,
									struct focal_test_result *srcresult, struct focal_test_result **result, char test_num);
static int focal_8201_cb_increase_test(struct focal_test_8201 *ft8201, struct focal_test_params *params,
									   struct focal_test_result **result, char test_num);

static int focal_get_max(int a, int b)
{
	return a > b ? a : b;
}
static int focal_abs(int value)
{
	return value < 0 ? -value : value;
}
static void init_current_address(struct focal_test_8201 *ft8201)
{
	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		return;
	}
	ft8201->current_slave_addr = I2C_DEFAULT_MASTER_ADDR;
}
static struct cs_info_packet *get_cs_info(struct focal_test_8201 *ft8201)
{
	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		return NULL;
	}
	return ft8201->cs_info;
}
static int  get_current_addr(struct focal_test_8201 *ft8201)
{
	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		return -ENODEV;
	}
	return ft8201->current_slave_addr;
}
static int get_cs_type(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_type;
}
static int get_cs_direction(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_direction;
}
static int get_s0_role(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_s0_role;
}
static void set_master_addr(struct cs_info_packet *cspt,  u8 master )
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_master_addr = master;
}
static int get_master_addr(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_master_addr;
}
static void set_slave_addr(struct cs_info_packet *cspt,  u8 slave )
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_slave_addr = slave;
}
static int get_slave_addr(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_slave_addr;
}
static void set_master_tx(struct cs_info_packet *cspt,  u8 tx )
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_master_tx = tx;
}
static int get_master_tx(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_master_tx;
}
static void set_master_rx(struct cs_info_packet *cspt,  u8 rx )
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_master_rx = rx;
}
static int get_master_rx(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_master_rx;
}
static void set_slave_tx(struct cs_info_packet *cspt,  u8 tx )
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_slave_tx = tx;
}
static int get_slave_tx(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_slave_tx;
}
static void set_slave_rx(struct cs_info_packet *cspt, u8 rx)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return;
	}
	cspt->cs_slave_rx = rx;
}
static int get_slave_rx(struct cs_info_packet *cspt)
{
	if (!cspt) {
		TS_LOG_ERR("%s: cspt is Null\n", __func__);
		return -EINVAL;
	}
	return cspt->cs_slave_rx;
}
static void initialize(struct cs_info_packet *cspt,  u8 reg_cfg )
{
	cspt->cs_type          = (enum CS_TYPE)( reg_cfg & 0x3F );
	cspt->cs_direction     = (enum CS_DIRECTION)( reg_cfg & 0x40 );
	cspt->cs_s0_role        = (enum CD_S0_ROLE)(reg_cfg & 0x80);
	cspt->cs_master_addr    = I2C_DEFAULT_MASTER_ADDR;
	cspt->cs_slave_addr     = I2C_DEFAULT_SLAVE_ADDR;
	cspt->cs_master_tx      = 0;
	cspt->cs_master_rx      = 0;
	cspt->cs_slave_tx       = 0;
	cspt->cs_slave_rx       = 0;
}
static void init_info_packet(struct cs_info_packet *cspt)
{
	cspt->cs_type          = CS_SINGLE_CHIP;
	cspt->cs_direction     = CS_UP_DOWN;/*ft8201 is up down cascade connection*/
	cspt->cs_s0_role        = CS_S0_AS_MASTER;
	cspt->cs_master_addr    = I2C_DEFAULT_MASTER_ADDR;/*master ic(up ic) addr,same to device i2c addr 0x70*/
	cspt->cs_slave_addr     = I2C_DEFAULT_SLAVE_ADDR;/*i2c slave ic(down ic) addr 0x72 */
	cspt->cs_master_tx      = 0;
	cspt->cs_master_rx      = 0;
	cspt->cs_slave_tx       = 0;
	cspt->cs_slave_rx       = 0;
}

static int update_cs_info(struct focal_test_8201 *ft8201)
{
	int ret = 0;
	u8 reg_val = 0;
	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		ret = -EINVAL;
		return ret;
	}
	/*init cs_info_packet info*/
	ret = focal_read_reg(REG_CS_TYPE, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read chip cs type fail, ret=%d\n", __func__, ret);
		return ret;
	}
	initialize(ft8201->cs_info, reg_val );
	/*set master tx nu*/
	ret = focal_read_reg(REG_MASTER_CHX, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read reg master chx fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_master_tx(ft8201->cs_info,  reg_val );
	/*set master rx nu*/
	ret = focal_read_reg(REG_MASTER_CHY, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read reg master chy fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_master_rx(ft8201->cs_info,  reg_val );
	/*set slave tx nu*/
	ret = focal_read_reg(REG_SLAVE_CHX, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read reg slave chx fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_slave_tx(ft8201->cs_info,  reg_val );
	/*set slave rx nu*/
	ret = focal_read_reg(REG_SLAVE_CHY, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read reg slave chy fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_slave_rx(ft8201->cs_info, reg_val);
	/*read master i2c addr value from i2c register,set cs_info*/
	ret = focal_write_reg( REG_BUF_OFFSET, I2C_MASTER_VALUE);
	if (ret) {
		TS_LOG_ERR("%s:write reg buf offset fail, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = focal_read_reg(REG_I2C_ADDR, &reg_val );
	if (ret) {
		TS_LOG_ERR("%s:read reg i2c addr fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_master_addr(ft8201->cs_info,  reg_val);
	/*read slave i2c addr value from i2c register,set cs_info*/
	ret = focal_write_reg(REG_BUF_OFFSET, I2C_SLAVE_VALUE);
	if (ret) {
		TS_LOG_ERR("%s:write reg buf offset fail, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = focal_read_reg(REG_I2C_ADDR, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s:read reg i2c addr fail, ret=%d\n", __func__, ret);
		return ret;
	}
	set_slave_addr(ft8201->cs_info, reg_val);

	TS_LOG_DEBUG("cs_type = %d, cs_direction = %d, cs_s0_role = %d, cs_master_addr = 0x%x, cs_slave_addr =  0x%x,\
				 cs_master_tx = %d, cs_master_rx = %d, cs_slave_tx = %d, cs_slave_rx = %d. \n",
				 ft8201->cs_info->cs_type,
				 ft8201->cs_info->cs_direction,
				 ft8201->cs_info->cs_s0_role,
				 ft8201->cs_info->cs_master_addr,
				 ft8201->cs_info->cs_slave_addr,
				 ft8201->cs_info->cs_master_tx,
				 ft8201->cs_info->cs_master_rx,
				 ft8201->cs_info->cs_slave_tx,
				 ft8201->cs_info->cs_slave_rx
				);
	return ret;

}
static bool chip_has_two_heart(struct focal_test_8201 *ft8201)
{
	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		return false;
	}
	/*judge two heart chip,return true*/
	if (get_cs_type(ft8201->cs_info) == CS_SINGLE_CHIP)
		return false;
	return true;
}
static void cs_chip_addr_mgr_init(struct cs_chip_addr_mgr *mgr, struct focal_test_8201 *parent)
{
	if ((!mgr) || (!parent)) {
		TS_LOG_ERR("%s: mgr or parent is Null\n", __func__);
		return;
	}
	mgr->m_parent = parent;
	mgr->slave_addr = get_current_addr(parent);

}

static void cs_chip_addr_mgr_exit(struct cs_chip_addr_mgr *mgr)
{
	if (!mgr) {
		TS_LOG_ERR("%s: mgr is Null\n", __func__);
		return;
	}
	if (mgr->slave_addr != get_current_addr(mgr->m_parent)) {
		focal_8201_set_slave_addr(mgr->m_parent, mgr->slave_addr );
	}
}
static int focal_8201_set_slave_addr(struct focal_test_8201 *ft8201,  u8 slave_addr)
{
	unsigned char value = 0;
	int ret = ERROR_CODE_OK;
	int tmp = 0;

	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 is Null\n", __func__);
		return -ENODEV;
	}
	ft8201->current_slave_addr = slave_addr;
	tmp = g_focal_dev_data->ts_platform_data->client->addr;

	TS_LOG_INFO("Original i2c addr 0x%x ", g_focal_dev_data->ts_platform_data->client->addr);
	TS_LOG_INFO("CurrentAddr 0x%x ", (slave_addr >> 1));//0x70 to 0x38,or 0x72 to 0x39
	if (g_focal_dev_data->ts_platform_data->client->addr != (slave_addr >> 1)) {
		g_focal_dev_data->ts_platform_data->client->addr = (slave_addr >> 1);
		TS_LOG_INFO("Change i2c addr 0x%x to 0x%x", tmp, g_focal_dev_data->ts_platform_data->client->addr);
	}

	/*debug start*/
	ret = focal_read_reg(DEVICE_MODE_ADDR, &value);
	if (ret != ERROR_CODE_OK) {
		TS_LOG_ERR("[focal] focal_read_reg Error, code: %d ",  ret);
	} else {
		TS_LOG_INFO("[focal] focal_read_reg successed, Addr: 0x20, value: 0x%02x ",  value);
	}
	/*debug end*/

	return 0;
}

static void work_as_master(struct cs_chip_addr_mgr *mgr)
{
	struct cs_info_packet *pcs = NULL;
	if (!mgr) {
		TS_LOG_ERR("%s: mgr is Null\n", __func__);
		return;
	}
	pcs = get_cs_info(mgr->m_parent);
	/*confirm crrent work mode on master chip,if not on master chip,set as master mode*/
	if (get_master_addr(pcs) != get_current_addr(mgr->m_parent)) {
		focal_8201_set_slave_addr(mgr->m_parent, get_master_addr(pcs) );
	}
}

static void work_as_slave(struct cs_chip_addr_mgr *mgr)
{
	struct cs_info_packet *pcs = NULL;
	if (!mgr) {
		TS_LOG_ERR("%s: mgr is Null\n", __func__);
		return;
	}
	pcs = get_cs_info(mgr->m_parent);

	if (get_slave_addr(pcs) != get_current_addr(mgr->m_parent)) {
		focal_8201_set_slave_addr(mgr->m_parent, get_slave_addr(pcs));
	}
}

static void cat_single_to_one_screen(struct focal_test_8201 *ft8201, int *buffer_master, int *buffer_slave, int *buffer_cated)
{
	int left_channel_num = 0, right_channel_num = 0;
	int up_channel_num = 0, down_channel_num = 0;
	unsigned char split_rx = 0;
	unsigned char split_tx = 0;
	int row = 0, col = 0;
	int relative_rx = 0;
	int relative_tx = 0;
	int total_rx = 0;
	int total_tx = 0;
	int  *buffer_left = NULL;
	int  *buffer_right = NULL;
	int  *buffer_up = NULL;
	int  *buffer_down = NULL;
	int cated_index = 0;
	int right_index = 0;
	int left_index = 0;
	int down_index = 0;
	int up_index = 0;

	if (!ft8201 || !buffer_master || !buffer_slave || !buffer_cated) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return;
	}

	/*make sure chip is double chip or not*/
	if (CS_SINGLE_CHIP == get_cs_type(ft8201->cs_info))  return;

	/*left right direction*/
	if (CS_LEFT_RIGHT == get_cs_direction(ft8201->cs_info)) {
		if (CS_S0_AS_MASTER == get_s0_role(ft8201->cs_info)) {
			split_rx =  get_master_rx(ft8201->cs_info);
			buffer_left  = buffer_master;
			buffer_right = buffer_slave;
			left_channel_num  = get_master_tx(ft8201->cs_info) * get_master_rx(ft8201->cs_info);
			right_channel_num = get_slave_tx(ft8201->cs_info) *  get_slave_rx(ft8201->cs_info);
		} else {
			split_rx = get_slave_rx(ft8201->cs_info);
			buffer_left  = buffer_slave;
			buffer_right = buffer_master;
			left_channel_num  = get_slave_tx(ft8201->cs_info) * get_slave_rx(ft8201->cs_info);
			right_channel_num = get_master_tx(ft8201->cs_info) *  get_master_rx(ft8201->cs_info);
		}
		for ( row = 0; row < get_master_tx(ft8201->cs_info); ++row) {
			for ( col = 0; col < get_master_rx(ft8201->cs_info) + get_slave_rx(ft8201->cs_info); ++col) {
				relative_rx = col - split_rx;
				total_rx = get_master_rx(ft8201->cs_info) + get_slave_rx(ft8201->cs_info);
				cated_index = row * total_rx + col;
				right_index = row * (total_rx - split_rx) + relative_rx;
				left_index = row * split_rx + col;
				if (col >= split_rx) {
					/*right data*/
					buffer_cated[cated_index] = buffer_right[right_index];
				} else {
					/*left data*/
					buffer_cated[cated_index] = buffer_left[left_index];
				}
			}
		}
		//memcpy( buffer_cated + get_master_tx(ft8201->cs_info) * (get_master_rx(ft8201->cs_info) + get_slave_rx(ft8201->cs_info)),  buffer_left + left_channel_num, 6 );
		//memcpy( buffer_cated + get_master_tx(ft8201->cs_info) * (get_master_rx(ft8201->cs_info) + get_slave_rx(ft8201->cs_info)) + 6,  buffer_right + right_channel_num, 6 );
	}

	else if (CS_UP_DOWN == get_cs_direction(ft8201->cs_info)) {
		if (CS_S0_AS_MASTER == get_s0_role(ft8201->cs_info)) {
			split_tx = get_master_tx(ft8201->cs_info);
			buffer_up  = buffer_master;
			buffer_down = buffer_slave;
			up_channel_num  = get_master_tx(ft8201->cs_info) * get_master_rx(ft8201->cs_info);
			down_channel_num = get_slave_tx(ft8201->cs_info) *  get_slave_rx(ft8201->cs_info);
		} else {
			split_tx = get_slave_tx(ft8201->cs_info);
			buffer_up  = buffer_slave;
			buffer_down = buffer_master;
			up_channel_num  = get_slave_tx(ft8201->cs_info) * get_slave_rx(ft8201->cs_info);
			down_channel_num = get_master_tx(ft8201->cs_info) *  get_master_rx(ft8201->cs_info);
		}
		for ( row = 0; row < get_master_tx(ft8201->cs_info) + get_slave_tx(ft8201->cs_info); ++row) {
			for ( col = 0; col < get_master_rx(ft8201->cs_info); ++col) {
				relative_tx = row - split_tx;
				total_tx = get_master_tx(ft8201->cs_info) + get_slave_tx(ft8201->cs_info);
				cated_index = row * get_master_rx(ft8201->cs_info) + col;
				down_index = relative_tx * get_master_rx(ft8201->cs_info) + col;
				up_index = row * get_master_rx(ft8201->cs_info) + col;
				if (row >= split_tx) {
					//down data
					buffer_cated[cated_index] = buffer_down[down_index];
				} else {
					//up data
					buffer_cated[cated_index] = buffer_up[up_index];
				}
			}
		}

		//memcpy( buffer_cated + (get_master_tx(ft8201->cs_info) + get_slave_tx(ft8201->cs_info)) * get_master_rx(ft8201->cs_info),  buffer_up + up_channel_num, 6 );
		//memcpy( buffer_cated + (get_master_tx(ft8201->cs_info) + get_slave_tx(ft8201->cs_info)) * get_master_rx(ft8201->cs_info) + 6,  buffer_down + down_channel_num, 6 );
	}

}
static int focal_8201_write_reg(struct focal_test_8201 *ft8201, u8 addr, u8 value)
{
	int ret = ERROR_CODE_OK;
	if (!ft8201) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -ENODEV;
	}
	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		/* double chip deal */
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_write_reg(addr, value);
		/*move to slave addr,work on slave ic*/
		work_as_slave(&mgr);
		ret = focal_write_reg(addr, value);
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_start_scan(void)
{
	int i = 0;
	int ret = 0;
	int query_delay = 0;
	int max_query_times = 0;

	u8 reg_val = 0x00;
	/*read from 0x00 back 0x40*/
	ret = focal_read_reg(DEVICE_MODE_ADDR, &reg_val);
	if (ret) {
		TS_LOG_ERR("%s: read device mode fail, ret=%d\n",
				__func__, ret);
		return ret;
	}
	/*0x40 |0x80,write 1 to bit 7,write value to 0x00,start scan*/
	reg_val |= 0x80;
	ret = focal_write_reg(DEVICE_MODE_ADDR, reg_val);
	if (ret) {
		TS_LOG_ERR("%s:set device mode fail, ret=%d\n", __func__, ret);
		return ret;
	}

	query_delay = FTS_SCAN_QUERY_DELAY;
	max_query_times = FTS_SCAN_MAX_TIME / query_delay;

	msleep(query_delay);
	for (i = 0; i < max_query_times; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &reg_val);
		if (ret) {
			TS_LOG_ERR("%s: read device model fail, ret=%d\n",
					__func__, ret);
			msleep(query_delay);
			continue;
		}
		/*if scan finished,back bit 7  is 0*/
		if ((reg_val >> 7) != 0) {
			TS_LOG_INFO("%s:device is scan, retry=%d\n",
						__func__, i);
			msleep(query_delay);
		} else {
			TS_LOG_INFO("%s:device scan finished, retry=%d\n",
						__func__, i);
			return 0;
		}
	}

	TS_LOG_ERR("%s:device scan timeout\n", __func__);
	return -ETIMEDOUT;
}

/* auto clb */
static int focal_chip_clb(void)
{
	int ret = 0;
	u8 reg_data = 0;
	u8 retry_count = 50;

	/*  start auto clb write 4 to REG_CLB*/
	ret = focal_write_reg(REG_CLB, 4);
	if (ret) {
		TS_LOG_ERR("%s:write clb reg fail, ret=%d\n", __func__, ret);
		return ret;
	}
	msleep(100);

	while (retry_count--) {
		/*write 0x40 to 0x00,factory mode*/
		ret = focal_write_reg(DEVICE_MODE_ADDR, 0x04 << 4);
		if (ret) {
			TS_LOG_ERR("%s:write dev model reg fail, ret=%d\n",
					__func__, ret);
			return ret;
		}

		ret = focal_read_reg(0x04, &reg_data);
		if (ret) {
			TS_LOG_ERR("%s:read 0x04 reg fail, ret=%d\n",
					__func__, ret);
			return ret;
		}
		/*firmware back to 0x02,clb sucess*/
		if (reg_data == 0x02)
			return 0;
		else
			msleep(100);
	}

	return -ETIMEDOUT;
}

static int focal_read_raw_data(u8 data_type, u8 *raw_data, size_t size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;

	u8 cmd = 0;
	if (!raw_data) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	pkg_count = size / BYTES_PER_TIME;
	if (size % BYTES_PER_TIME != 0)
		pkg_count += 1;

	ret = focal_write_reg(FTS_REG_DATA_TYPE, data_type);
	if (ret) {
		TS_LOG_ERR("%s:write data type to ret, ret=%d\n",
				__func__, ret);
		return ret;
	}
	msleep(10);

	cmd = REG_RAW_BUF0;

	for (i = 0; i < pkg_count; i++) {
		/*
		 * compute pkg_size value
		 * if the last package is not equal to BYTES_PER_TIME,
		 * the package size will be set to (size % BYTES_PER_TIME)
		 */
		if ((size % BYTES_PER_TIME != 0) && (i + 1 == pkg_count))
			pkg_size = size % BYTES_PER_TIME;
		else
			pkg_size = BYTES_PER_TIME;

		/* first package shuold write cmd to ic */
		if (i == 0)
			ret = focal_read(&cmd, 1,   raw_data, pkg_size);
		else
			ret = focal_read_default(raw_data + readed_count,
									 pkg_size);

		if (ret) {
			TS_LOG_ERR("%s:read raw data fail, ret=%d\n",
					__func__, ret);
			return ret;
		}

		readed_count += pkg_size;
	}

	return 0;
}

/* get panelrows */
static int focal_read_channel_x(u8 *channel_x)
{
	if (!channel_x) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	return focal_read_reg(REG_TX_NUM, channel_x);
}

/* get panelcols */
static int focal_read_channel_y(u8 *channel_y)
{
	if (!channel_y) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	return focal_read_reg(REG_RX_NUM, channel_y);
}

static int focal_get_cb_data(
	int offset,
	u8 *data,
	size_t data_size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;
	u8 cmd[3] = {0};

	if (!data) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	pkg_count = data_size / BYTES_PER_TIME;
	if (0 != (data_size % BYTES_PER_TIME))
		pkg_count += 1;

	cmd[0] = REG_CB_BUF0;
	for (i = 0; i < pkg_count; i++) {

		if ((i + 1 == pkg_count) && (data_size % BYTES_PER_TIME != 0))
			pkg_size = data_size % BYTES_PER_TIME;
		else
			pkg_size = BYTES_PER_TIME;

		/* addr high 8 bits,CB adrr offset */
		cmd[1] = (offset + readed_count) >> 8;
		ret = focal_write_reg(REG_CB_ADDR_H, cmd[1]);
		if (ret) {
			TS_LOG_ERR("%s:write cb addr h fail, ret=%d\n",
					   __func__, ret);
			return ret;
		}

		/* addr low 8 bits,B adrr offset */
		cmd[2] = (offset + readed_count) & 0xff;
		ret = focal_write_reg(REG_CB_ADDR_L, cmd[2]);
		if (ret) {
			TS_LOG_ERR("%s:write cb addr l fail, ret=%d\n",
					__func__, ret);
			return ret;
		}

		ret = focal_read(cmd, 1, data + readed_count, pkg_size);
		if (ret) {
			TS_LOG_ERR("%s:read cb data fail, ret=%d\n",
					__func__, ret);
			return ret;
		}

		readed_count += pkg_size;
	}

	return 0;
}

static int focal_get_raw_data_format(unsigned int chl_x , unsigned int chl_y, int *data, size_t size)
{
	unsigned int i = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	int ret = 0;
	int raw_data_size = 0;
	int hig_inx = 0;
	int low_inx = 0;
	int cur_inx = 0;

	short   raw_data_value = 0;
	u8 *original_raw_data = NULL;

	if (!data || (0 == size) || (0 == chl_x) || (0 == chl_y)) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}
	raw_data_size = size * 2;//one data need 2 byte

	original_raw_data = kzalloc(raw_data_size, GFP_KERNEL);
	if (!original_raw_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	/*read original value from register 0xad*/
	ret = focal_read_raw_data(DATA_TYPE_RAW_DATA, original_raw_data, raw_data_size);

	if (ret) {
		TS_LOG_ERR("%s:read raw data fail, ret=%d\n", __func__, ret);
		goto exit;
	}
	/*high 8  bit and low 8bit compose ont data*/
	for (i = 0; i < size; i++) {
		hig_inx = i * 2;
		low_inx = hig_inx + 1;
		raw_data_value = (original_raw_data[hig_inx] << 8);
		raw_data_value |= original_raw_data[low_inx];
		CurTx = i % chl_y;
		CurRx = i / chl_y;
		cur_inx = CurTx * chl_x + CurRx;
		data[cur_inx] = raw_data_value;
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d,raw_data_value= %d \n", __func__, CurTx, CurRx, i, raw_data_value);
	}

exit:
	kfree(original_raw_data);
	original_raw_data = NULL;
	return ret;
}

static void focal_print_test_data(
	char *msg,
	int row,
	int col,
	int max,
	int min,
	int value)
{

	TS_LOG_ERR("%s:%s,data[%d, %d]=%d, range=[%d, %d]\n",
				__func__, msg, row, col, value, min, max);
}

static int focal_get_int_average(int *p, size_t len)
{
	long long sum = 0;
	size_t i = 0;

	if (!p)
		return 0;

	for (i = 0; i < len; i++)
		sum += p[i];

	if (len != 0)
		return (int)div_s64(sum, len);
	else
		return 0;
}

static int focal_get_int_min(int *p, size_t len)
{
	int min = 0;
	size_t i = 0;

	if (!p || len <= 0)
		return 0;

	min = p[0];
	for (i = 0; i < len; i++)
		min = min > p[i] ? p[i] : min;

	return min;
}

static int focal_get_int_max(int *p, size_t len)
{
	int max = 0;
	size_t i = 0;

	if (!p || len <= 0)
		return 0;

	max = p[0];
	for (i = 0; i < len; i++)
		max = max < p[i] ? p[i] : max;

	return max;
}

static int focal_get_statistics_data(
	int *data, size_t data_size, char *result, size_t res_size)
{
	int avg = 0;
	int min = 0;
	int max = 0;

	if (!data) {
		TS_LOG_ERR("%s:data is null\n", __func__);
		return -ENODATA;
	}

	if (!result) {
		TS_LOG_ERR("%s:result is null\n", __func__);
		return -ENODATA;
	}

	if (data_size <= 0 || res_size <= 0) {
		TS_LOG_ERR("%s:%s, data_size=%ld, res_size=%ld\n", __func__,
				"input parameter is illega",
				data_size, res_size);
		return -EINVAL;
	}
	avg = focal_get_int_average(data, data_size);
	min = focal_get_int_min(data, data_size);
	max = focal_get_int_max(data, data_size);

	memset(result, 0, res_size);
	return snprintf(result, res_size, "[%4d,%4d,%4d]", avg, max, min);
}


static void focal_put_device_info(struct ts_rawdata_info *info)
{
	struct focal_platform_data *focal_pdata = NULL;

	if (!info) {
		TS_LOG_ERR("%s:data is null\n", __func__);
		return;
	}

	focal_pdata = focal_get_platform_data();

	/* put ic data */
	focal_strncat(info->result, "-ft", TS_RAWDATA_RESULT_MAX);
	focal_strncatint(info->result, focal_pdata->chip_id, "%X",
					 TS_RAWDATA_RESULT_MAX);

	focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	focal_strncat(info->result, focal_pdata->vendor_name,
				  TS_RAWDATA_RESULT_MAX);

	focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	focal_strncatint(info->result, focal_pdata->fw_ver, "%d",
					 TS_RAWDATA_RESULT_MAX);
	focal_strncat(info->result, ";", TS_RAWDATA_RESULT_MAX);
}

static void focal_put_test_result(
	struct focal_test_params *params,
	struct ts_rawdata_info *info,
	struct focal_test_result *test_results[],
	int size)
{
	int i = 0;
	size_t j = 0;
	int buff_index = 0;
	char statistics_data[FTS_STATISTICS_DATA_LEN] = {0};

	if (!info || !params) {
		TS_LOG_ERR("%s:info  or params is null\n", __func__);
		return;
	}
	/* put test result */
	for (i = 0; i < size; i++) {

		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
						__func__, i);
			focal_strncat(info->result, "FF", TS_RAWDATA_RESULT_MAX);

			continue;
		}

		focal_strncat(info->result, test_results[i]->result_code,
					  TS_RAWDATA_RESULT_MAX);
		if (i != size - 1)
			focal_strncat(info->result, "-", TS_RAWDATA_RESULT_MAX);
	}

	/* put statistics data */
	for (i = 0; i < size; i++) {
		if (!test_results[i]) {
			TS_LOG_INFO("%s:test result is null, index=%d\n",
						__func__, i);
			continue;
		}

		focal_get_statistics_data(test_results[i]->values,
								  test_results[i]->size,
								  statistics_data,
								  FTS_STATISTICS_DATA_LEN);

		focal_strncat(info->result, statistics_data,
					  TS_RAWDATA_RESULT_MAX);
	}

	/* put test failed reason */
	if (tp_cap_test_status) { //TEST_SUCCESS = 0
		focal_strncat(info->result, tp_test_failed_reason, TP_TEST_FAILED_REASON_LEN);
	}
	focal_put_device_info(info);
	/* put test data */
	memset(info->buff, 0, TS_RAWDATA_BUFF_MAX);
	info->buff[buff_index++] = params->channel_x_num;
	info->buff[buff_index++] = params->channel_y_num;
	for (i = 0; i < size; i++) {
		if (I_IS_OPEN != i) {/*open cb normal all are 63,so proc printf remain max min average*/
			if (!test_results[i]) {
				TS_LOG_INFO("%s:test result is null, index=%d\n",
							__func__, i);
				continue;
			}

			for (j = 0; j < test_results[i]->size; j++) {

				if (buff_index >= TS_RAWDATA_BUFF_MAX) {
					TS_LOG_INFO("%s:buff is full, len=%d\n",
								__func__, buff_index);
					break;
				}

				info->buff[buff_index++] = test_results[i]->values[j];
			}
		}
	}

	info->used_size = buff_index;

}

static int focal_alloc_test_container(
	struct focal_test_result **result,
	size_t data_size)
{

	*result = kzalloc(sizeof(struct focal_test_result), GFP_KERNEL);
	if (!*result)
		return -ENOMEM;

	(*result)->size = data_size;
	(*result)->values = kzalloc(data_size * sizeof(int), GFP_KERNEL);
	if (!((*result)->values)) {
		kfree(*result);
		*result = NULL;
		return -ENOMEM;
	}

	return 0;
}

static void focal_free_test_container(struct focal_test_result *result)
{

	if (result) {
		if (result->values)
			kfree(result->values);
		kfree(result);
	}
}

static int focal_get_channel_form_ic(struct focal_test_params *params)
{
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;

	if (!params) {
		TS_LOG_ERR("%s:params is null\n", __func__);
		return -EINVAL;
	}
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory model fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}

	ret = focal_8201_get_channel_num(&chl_x, &chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get channel num fail, ret=%d\n", __func__, ret);
	} else {
		params->channel_x_num = chl_x;
		params->channel_y_num = chl_y;
		params->key_num = 0;// no visual key num
		TS_LOG_INFO("%s:channel_x=%d, channel_y=%d,params->key_num=%d\n",
					__func__, chl_x, chl_y, params->key_num);
	}

	/*
	 * Deleted fts_enter_work here, then need enter factory mode again
	 */

	return 0;
}


/************************************************************************
* name: focal_8201_get_channel_num
* brief:  get num of ch_x, ch_y and key
* input: none
* output: none
* return: comm code. code = 0x00 is ok, else fail.
***********************************************************************/
static int focal_8201_get_channel_num(u8 *channel_x, u8 *channel_y)
{
	int ret = 0;
	u8 chl_x = 0;
	u8 chl_y = 0;

	if (!channel_x || !channel_y) {
		TS_LOG_ERR("%s:channel_x or channel_y is null\n", __func__);
		return -EINVAL;
	}
	/* get channel x num */
	ret = focal_read_channel_x(&chl_x);
	if (ret) {
		TS_LOG_ERR("%s:get chennel x from ic fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}

	/* get channel y num */
	ret = focal_read_channel_y(&chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get chennel y from ic fail, ret=%d\n",
				   __func__, ret);
		return ret;
	}

	if (chl_x <= 0 || chl_x > TX_8201_NUM_MAX) {
		TS_LOG_ERR("%s:channel x value out of range, value = %d\n",
				__func__, chl_x);
		return -EINVAL;
	}

	if (chl_y <= 0 || chl_y > RX_8201_NUM_MAX) {
		TS_LOG_ERR("%s:channel y value out of range, value = %d\n",
				__func__, chl_y);
		return -EINVAL;
	}

	*channel_x = chl_x;
	*channel_y = chl_y;

	return 0;
}

static int focal_enter_work(void)
{
	int i = 0;
	int ret = 0;
	u8 ret_val = 0;

	TS_LOG_INFO("%s: enter work\n", __func__);

	for (i = 0; i < MAX_RETRY_TIMES; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &ret_val);
		if (ret) {
			TS_LOG_ERR("%s: read DEVICE_MODE_ADDR reg failed\n",
					__func__);
			return ret;
		}
		if (((ret_val >> 4) & 0x07) != DEVICE_MODE_WORK) {
			ret = focal_write_reg(DEVICE_MODE_ADDR, DEVICE_MODE_WORK);
			if (!ret) {
				msleep(50);
				continue;
			} else {
				TS_LOG_ERR("%s: change work mode failed\n",
						__func__);
				break;
			}
		} else {
			TS_LOG_INFO("%s: change work mode success\n", __func__);
			break;
		}
	}

	return ret;
}

static int focal_enter_factory(void)
{
	int i = 0;
	int ret = 0;
	u8 ret_val = 0;

	TS_LOG_INFO("%s: start change factory mode\n", __func__);
	for (i = 0; i < MAX_RETRY_TIMES; i++) {
		ret = focal_read_reg(DEVICE_MODE_ADDR, &ret_val);
		if (ret) {
			TS_LOG_ERR("%s: read DEVICE_MODE_ADDR error\n",
					__func__);
			return ret;
		}
		/* read addr 0x00 back 0x40,already in factory mode,or write 0x40 to addr ox00*/
		if (((ret_val >> 4) & 0x07) == 0x04) {
			TS_LOG_INFO("%s: change factory success\n", __func__);
			return 0;
		} else {
			ret = focal_write_reg(DEVICE_MODE_ADDR,
								DEVICE_MODE_FACTORY);
			if (!ret) {
				msleep(50);
				continue;
			} else {
				TS_LOG_ERR("%s: write reg failed\n",
						__func__);
				return ret;
			}
		}
	}

	TS_LOG_INFO("%s: end change factory mode\n", __func__);
	return ret;
}

static int focal_get_cb_data_format(int *data, size_t size, unsigned int chl_x, unsigned int chl_y)
{
	unsigned int i = 0;
	int ret = 0;
	size_t read_size = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	unsigned int Count = 0;
	char *cb_data = NULL;
	if (!data || (0 == size) || (0 == chl_x) || (0 == chl_y)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	read_size = size;
	cb_data = kzalloc(read_size, GFP_KERNEL);
	if (!cb_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	ret = focal_get_cb_data(0, cb_data, read_size);

	if (ret) {
		TS_LOG_ERR("%s: get cb data failed\n", __func__);
		goto free_cb_data;
	}

	memset(data, 0, size);


	for (i = 0; i < size; i++) {
		CurTx = i % chl_y; //size = chl_x*chl_y,because  params->key_num =0
		CurRx = i / chl_y;
		Count = CurTx * chl_x + CurRx;
		data[Count] = cb_data[i];
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n", __func__, CurTx, CurRx, i);
	}

free_cb_data:

	kfree(cb_data);
	cb_data = NULL;

	return ret;
}

static int focal_8201_start_scan(struct focal_test_8201 *ft8201)
{
	int ret = ERROR_CODE_OK;

	if (!ft8201) {
		TS_LOG_ERR("%s:ft8201 is null\n", __func__);
		return -ENODEV;
	}

	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		/* double chip deal */
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_start_scan();
		if (ret) {
			TS_LOG_ERR("%s: focal_start_scan fail !\n", __func__);
		}
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_8201_enter_factory(struct focal_test_8201 *ft8201)
{
	int ret = 0;

	if (!ft8201) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	/* Every time  enter the factory mode, the previous information is deleted*/
	memset(ft8201->cs_info, 0, sizeof(struct cs_info_packet));

	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:%d: focal_enter_factory fail\n",
			__func__,__LINE__);
		return ret;
	}

	/*Every time  update the information after enter  the factory mode*/
	ret = update_cs_info(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:%d:read chip cs type fail, ret=%d\n", __func__,__LINE__, ret);
	}

	return ret;
}
static int focal_8201_enter_work(struct focal_test_8201 *ft8201)
{
	int ret = 0;

	if (!ft8201) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		// double chip deal
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_enter_work();
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_8201_start_test_tp(
	struct focal_test_params *params,
	struct ts_rawdata_info *info)
{
	unsigned char cap_test_num = 0;
	int i = 0;
	int ret = 0;
	struct focal_test_result *test_results[FTS_8201_MAX_CAP_TEST_NUM] = {0};
	struct focal_test_8201  ft8201;

	TS_LOG_INFO("%s: start test tp\n",  __func__);
	if ((!params ) || !(info)) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}

	ft8201.cs_info =  kzalloc( sizeof(struct cs_info_packet), GFP_KERNEL);
	if (!ft8201.cs_info) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	init_info_packet(ft8201.cs_info);
	/*default addr 0x70*/
	init_current_address(&ft8201);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(&ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				__func__, ret);
	}
	/*first test is i2c test which is completed in  focal_8201_init_test_prm,so there count from 1*/
	/*  raw data test */
	ret = focal_8201_raw_data_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if(ret){
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/* open test */
	ret = focal_8201_open_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/*  short test */
	ret = focal_8201_short_circuit_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/*  cb data test */
	ret = focal_8201_cb_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/* cb uniformity test,use cb_test_result to compute cb uniformity */
	ret = focal_cb_uniformity_test(params, test_results[cap_test_num - 1], &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/* lcd_noise test */
	ret = focal_8201_noise_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;

	/* cb increase test */
	ret = focal_8201_cb_increase_test(&ft8201, params, &test_results[cap_test_num], cap_test_num + 1);
	if (ret) {
		tp_cap_test_status = SOFTWARE_REASON;
		strncpy(tp_test_failed_reason, "-software reason", TP_TEST_FAILED_REASON_LEN);
	}
	cap_test_num++;
	/*hand on test data and test result to kit info struct*/
	focal_put_test_result(params, info, test_results, cap_test_num);

	for (i = 0; i < cap_test_num; i++) {
		focal_free_test_container(test_results[i]);
		test_results[i] = NULL;
	}

	ret = focal_8201_enter_work(&ft8201);
	if (ret < 0) {
		TS_LOG_ERR("%s:enter work mode fail, ret=%d\n",
				__func__, ret);
	}

	if ( ft8201.cs_info != NULL) {
		kfree(ft8201.cs_info);
	}
	return ret;
}

/* auto clb */
static int focal_8201_chip_clb(struct focal_test_8201 *ft8201)
{
	int ret = 0;

	if (!ft8201) {
		TS_LOG_ERR("%s: ft8201 invalid !\n", __func__);
		return -ENODEV;
	}
	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		/* double chip deal*/
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_chip_clb();
		if (ret) {
			TS_LOG_ERR("%s:focal_chip_clb fail, ret=%d\n",
					__func__, ret);
		}
		/*move to slave addr,work on slave ic*/
		work_as_slave(&mgr);
		ret = focal_chip_clb();
		if (ret) {
			TS_LOG_ERR("%s:focal_chip_clb fail, ret=%d\n",
					__func__, ret);
		}
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_read_noise(u8 *raw_data, size_t size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;

	u8 cmd = 0;

	if (!raw_data) {
		TS_LOG_ERR("%s: raw_data invalid !\n", __func__);
		return -EINVAL;
	}
	pkg_count = size / BYTES_PER_TIME;
	if (size % BYTES_PER_TIME != 0)
		pkg_count += 1;

	cmd = REG_RAW_BUF0;
	for (i = 0; i < pkg_count; i++) {
		/*
		 * compute pkg_size value
		 * if the last package is not equal to BYTES_PER_TIME,
		 * the package size will be set to (size % BYTES_PER_TIME)
		 */
		if ((size % BYTES_PER_TIME != 0) && (i + 1 == pkg_count))
			pkg_size = size % BYTES_PER_TIME;
		else
			pkg_size = BYTES_PER_TIME;

		/* first package shuold write cmd to ic */
		if (i == 0)
			ret = focal_read(&cmd, 1,   raw_data, pkg_size);
		else
			ret = focal_read_default(raw_data + readed_count,
									 pkg_size);

		if (ret) {
			TS_LOG_ERR("%s:read raw data fail, ret=%d\n",
					   __func__, ret);
			return ret;
		}

		readed_count += pkg_size;
	}

	return 0;
}

static int focal_get_noise_format(unsigned int chl_x , unsigned int chl_y, int *data, size_t size)
{
	unsigned int i = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	int ret = 0;
	int raw_data_size = 0;
	short   raw_data_value = 0;
	u8 *original_raw_data = NULL;
	int high_index = 0;
	int low_index = 0;
	int cur_index = 0;

	if (!data || (0 == size) || (0 == chl_x) || (0 == chl_y)) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}
	raw_data_size = size * 2;//one data need 2 byte
	original_raw_data = kzalloc(raw_data_size, GFP_KERNEL);
	if (!original_raw_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	ret = focal_read_noise(original_raw_data, raw_data_size);
	if (ret) {
		TS_LOG_ERR("%s:read raw data fail, ret=%d\n", __func__, ret);
		goto exit;
	}

	for (i = 0; i < (unsigned int)size; i++) {
		high_index = i * 2;
		low_index = i * 2 + 1;
		raw_data_value = (original_raw_data[high_index] << 8);
		raw_data_value |= original_raw_data[low_index];
		CurTx = i % chl_y;
		CurRx = i / chl_y;
		cur_index = CurTx * chl_x + CurRx;
		data[cur_index] = raw_data_value;
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d,raw_data_value= %d \n", __func__, CurTx, CurRx, i, raw_data_value);
	}

exit:
	kfree(original_raw_data);
	original_raw_data = NULL;
	return ret;
}

static int focal_8201_get_noise_format(
	struct focal_test_8201 *ft8201,
	unsigned int chl_x , unsigned int chl_y,
	int *data, size_t size)
{
	int ret = 0;

	if (!ft8201 || !data) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -ENODEV;
	}

	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		/* Read rawdata hand over to the master  to complete it. */
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master or slave chip*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_get_noise_format(chl_x, chl_y, data, size);
		if (ret) {
			TS_LOG_ERR("%s:focal_get_noise_format fail, ret=%d\n",
					__func__, ret);
		}
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_8201_get_raw_data_format(
	struct focal_test_8201 *ft8201,
	unsigned int chl_x , unsigned int chl_y,
	int *data, size_t size)
{
	int ret = 0;
	if (!ft8201 || !data) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -ENODEV;
	}
	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		/* Read rawdata hand over to the master  to complete it. */
		struct cs_chip_addr_mgr mgr;
		/*get current addr 0x70,certain current work master or slave chip*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_get_raw_data_format(chl_x, chl_y, data, size);
		if (ret) {
			TS_LOG_ERR("%s:focal_get_raw_data_format fail, ret=%d\n",
					__func__, ret);
		}
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);
	}

	return ret;
}

static int focal_8201_raw_data_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int raw_data_min = 0;
	int raw_data_max = 0;
	int raw_data_size = 0;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s:  start,First test +:%d\n", __func__, test_num);
	if (!ft8201 || !params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	raw_data_min = params->threshold.raw_data_min;
	raw_data_max = params->threshold.raw_data_max;
	raw_data_size = chl_x * chl_y;

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; /*default result_code is failed*/
	result_code[2] = '\0';

	ret = focal_alloc_test_container(&test_result, raw_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc raw data container fail, ret=%d\n",
				   __func__, ret);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "raw_data_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				   __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	/* start scanning,fist two data is not stable,use third data */
	for (i = 0; i < 3; i++) {
		ret = focal_8201_start_scan(ft8201);
		if (ret) {
			TS_LOG_ERR("%s:scan fail, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}

		ret = focal_8201_get_raw_data_format(ft8201, chl_x, chl_y, test_result->values, test_result->size);
		if (ret) {
			TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
					__func__, ret);

			test_result->result = false;
			goto test_finish;
		}
	}
	/* compare raw data with shreshold */
	for (i = 0; i < raw_data_size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if ((test_result->values[i] < raw_data_min)
			|| (test_result->values[i] > raw_data_max)) {

			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("raw data test fail",
								i / chl_x,
								i % chl_x,
								raw_data_max,
								raw_data_min,
								test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:raw data test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:raw data test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	TS_LOG_INFO("%s:  end,First test +:%d\n", __func__, test_num);
	return ret;
}

static int focal_8201_get_cb_data_format(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y)
{
	int ret = 0;
	struct cs_chip_addr_mgr mgr;
	int *buffer_master = NULL;
	int *buffer_slave = NULL;
	int master_tx = 0;
	int master_rx = 0;
	int slave_tx = 0;
	int slave_rx = 0;
	int master_buf_size = 0;
	int slave_buf_size = 0;
	int master_buf_num = 0;
	int slave_buf_num = 0;
	if (!ft8201 || !data) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -ENODEV;
	}
	master_tx = get_master_tx(ft8201->cs_info);
	master_rx = get_master_rx(ft8201->cs_info);
	slave_tx = get_slave_tx(ft8201->cs_info);
	slave_rx = get_slave_rx(ft8201->cs_info);
	/*judge ic type is cascade connection*/
	if (chip_has_two_heart(ft8201)) {
		master_buf_size = sizeof(int)*(master_tx + 1) * master_rx;
		buffer_master = (int *)kzalloc(master_buf_size, GFP_KERNEL);
		if (!buffer_master) {
			TS_LOG_ERR("%s:alloc mem fail\n", __func__);
			return -ENOMEM;
		}
		slave_buf_size = sizeof(int)*(slave_tx + 1) * slave_rx;
		buffer_slave = (int *)kzalloc(slave_buf_size , GFP_KERNEL);
		if (!buffer_slave) {
			TS_LOG_ERR("%s:alloc mem fail\n", __func__);
			goto release_master;
		}
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		master_buf_num = master_tx * master_rx;
		ret = focal_get_cb_data_format(buffer_master, (unsigned int)master_buf_num, (unsigned int)master_tx, (unsigned int)master_rx);
		if (ret) {
			TS_LOG_ERR("%s:get cb data fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		/*move to slave addr,work on slave ic*/
		work_as_slave(&mgr);
		slave_buf_num = slave_tx * slave_rx;
		ret = focal_get_cb_data_format(buffer_slave, (unsigned int)slave_buf_num, (unsigned int)slave_tx, (unsigned int)slave_rx);
		if (ret) {
			TS_LOG_ERR("%s:get cb data fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		/*master ic data and slave ic data form 48*30 data array*/
		cat_single_to_one_screen(ft8201, buffer_master, buffer_slave, data);
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);

release_slave:
		if (buffer_slave)
			kfree(buffer_slave);
release_master:
		if (buffer_master)
			kfree(buffer_master);
	}

	return ret;
}

static int focal_8201_cb_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int cb_max = 0;
	int cb_min = 0;
	int data_size = 0;
	size_t cb_data_size = 0;
	int *cb_data = NULL;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: start,Fourth test ++++:%d\n", __func__, test_num);
	if (!ft8201 || !params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	cb_min = params->threshold.cb_test_min;
	cb_max = params->threshold.cb_test_max;
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;

	data_size = (chl_x * chl_y + params->key_num);
	cb_data_size = (size_t)(unsigned)(data_size);
	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, cb_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem for cb test result fail\n", __func__);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "cb_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				__func__, ret);
		test_result->result = false;
		goto test_finish;

	}
	/*cb test main function*/
	ret = focal_8201_get_cb_data_format(ft8201, test_result->values, cb_data_size, chl_x, chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get cb data fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/* compare raw data with shreshold */
	cb_data = test_result->values;
	for (i = 0; i < (int)cb_data_size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if (cb_data[i] < cb_min || cb_data[i] > cb_max) {

			test_result->result = false;
			tp_cap_test_status  = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("cb test failed",
								i / chl_x,
								i % chl_x,
								cb_max,
								cb_min,
								cb_data[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:cb test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:cb test fail!\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	TS_LOG_INFO("%s: end,Fourth test ++++:%d\n", __func__, test_num);
	return ret;
}

static int focal_cb_uniformity_test(
	struct focal_test_params *params,
	struct focal_test_result *srcresult, struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int cb_data_size = 0;
	int cb_uniformity_x = 0;
	int cb_uniformity_y = 0;
	int deviation = 0;
	int max = 0;
	int malloc_test_result = 0;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;
	int ver_index = 0;
	int cb_index = 0;
	TS_LOG_INFO("%s: start,Fifth test +++++:%d\n", __func__, test_num);
	if (!params || !result || (params->channel_x_num) <= 0 || (params->channel_y_num) <= 0) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}


	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	cb_data_size = chl_x * chl_y;
	/*broadwise direction and vertical direction need dounle malloc*/
	malloc_test_result = cb_data_size * 2;

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';

	ret = focal_alloc_test_container(&test_result, malloc_test_result);
	if (ret) {
		TS_LOG_ERR("%s:alloc cb uniformity data container fail, ret=%d\n",
				   __func__, ret);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "cb_uniformity_test", FTS_TEST_NAME_LEN - 1);
	/*compare array i and i-1 uniformity,broadwise direction*/
	for (i = 0; i < chl_x * chl_y ; i++) {
		if (i % chl_x == 0) {
			test_result->values[i] = 0;
		} else {
			deviation = focal_abs(srcresult->values[i] - srcresult->values[i - 1]);
			max = focal_get_max(srcresult->values[i], srcresult->values[i - 1]);
			if (0 == max) {
				TS_LOG_ERR("%s:CB x value max is zero,not normal value return fail\n",__func__);
				*result = test_result;
				return -EINVAL;
			}
			test_result->values[i] = 100 * deviation / max;
		}
	}
	/*compare array i and i-chx(i>chx) uniformity,vertical direction*/
	for (i = 0; i < chl_x * chl_y ; i++) {
		cb_index = i + cb_data_size;
		ver_index = i - chl_x;
		if (i < chl_x) {
			test_result->values[cb_index] = 0;
		} else {
			deviation = focal_abs(srcresult->values[i] - srcresult->values[ver_index]);
			max = focal_get_max(srcresult->values[i], srcresult->values[ver_index]);
			if (0 == max) {
				TS_LOG_ERR("%s:CB y value max is zero,not normal value return fail\n",__func__);
				*result = test_result;
				return -EINVAL;
			}
			test_result->values[cb_index] = 100 * deviation / max;
		}
	}

	cb_uniformity_x = params->threshold.cb_uniformity_x;
	cb_uniformity_y = params->threshold.cb_uniformity_y;
	/* broadwise direction,compare cb uniformity data with shreshold */
	for (i = 0; i < cb_data_size; i++) {
		if (test_result->values[i] > cb_uniformity_x) {
			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("cb uniformity x test fail",
								i / chl_x,
								i % chl_x,
								cb_uniformity_x,
								0,
								test_result->values[i]);
		}
	}
	/* vertical direction,compare raw data with shreshold */
	for (i = 0; i < cb_data_size; i++) {
		cb_index = i + cb_data_size;
		if (test_result->values[cb_index] > cb_uniformity_y) {
			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("cb uniformity y test fail",
								i / chl_x,
								i % chl_x,
								cb_uniformity_y,
								0,
								test_result->values[i]);
		}
	}

	//test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:cb uniformity test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:cb uniformity test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	TS_LOG_INFO("%s: end,Fifth test +++++:%d\n", __func__, test_num);
	return ret;
}

static int focal_8201_get_open_test_data(struct focal_test_8201 *ft8201, int *data, unsigned int size, unsigned int chl_x, unsigned int chl_y)
{
	unsigned int i = 0;
	int ret = 0;
	unsigned int read_size = 0;
	unsigned int CurTx = 0;
	unsigned int CurRx = 0;
	u8 reg_value = 0;
	u8 reg_data = 0;
	int *open_data = NULL;
	int cur_inx = 0;

	if (!ft8201 || !data || (0 == size) || (0 == chl_x) || (0 == chl_y)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	/*back-up REG_GIP(0x20)status value to reg_valu,after open test,write  back-up value to REG_GIP(0x20)*/
	ret = focal_read_reg(REG_GIP, &reg_value);
	if (ret) {
		TS_LOG_ERR("%s:read gip reg fail, ret=%d\n", __func__, ret);
		return ret;
	}
	/*back-up REG_VREF(0x86)status value to reg_valu,after open test,write back-up value to REG_VREF(0x86)*/
	ret = focal_read_reg(REG_VREF, &reg_data);
	if (ret) {
		TS_LOG_ERR("%s:read vref reg fail, ret=%d\n", __func__, ret);
		return ret;
	}

	/*the VREF_TP is set to 2V,wirte 0x01 to 0x86*/
	ret = focal_8201_write_reg(ft8201, REG_VREF, VREF_2V);
	if (ret) {
		TS_LOG_ERR("%s:write vref switch fail, ret=%d\n", __func__, ret);
		return ret;
	}
	msleep(50);
	/*The 0x20 register Bit4~Bit5 is set to 2b'10 (Source to GND),
	the value of other bit remains unchanged*/
	ret = focal_8201_write_reg(ft8201, REG_GIP, ((reg_value & 0xCF) + 0x20));
	if (ret) {
		TS_LOG_ERR("%s:write gip switch fail, ret=%d\n", __func__, ret);
		return ret;
	}
	msleep(50);

	/* auto clb */
	ret = focal_8201_chip_clb(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:clb fail, ret=%d\n", __func__, ret);
		return ret;
	}

	read_size = size;
	open_data = (int *)kzalloc(read_size * sizeof(int), GFP_KERNEL);
	if (!open_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}
	/*cb main function*/
	ret = focal_8201_get_cb_data_format(ft8201, open_data, (size_t)read_size, chl_x, chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get cb data fail, ret=%d\n", __func__, ret);
		goto free_open_data;
	}

	/* reset reg,back-up origin status value to 0x20*/
	ret = focal_8201_write_reg(ft8201, REG_GIP, reg_value);
	if (ret) {
		TS_LOG_ERR("%s:reset gip reg fail, ret=%d\n", __func__, ret);
		goto free_open_data;
	}
	/* reset reg,back-up origin status value to 0x86*/
	ret = focal_8201_write_reg(ft8201, REG_VREF, reg_data);
	if (ret) {
		TS_LOG_ERR("%s:reset vref reg fail, ret=%d\n", __func__, ret);
		goto free_open_data;
	}

	/* auto clb */
	ret = focal_8201_chip_clb(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:clb fail, ret=%d\n", __func__, ret);
		goto free_open_data;
	}
	for (i = 0; i < size; i++) {
		CurTx = i % chl_y ;
		CurRx = i / chl_y;
		cur_inx = CurTx * chl_x + CurRx;
		data[cur_inx] = open_data[i];;
		TS_LOG_DEBUG("%s:CurTx= %d , Rx =%d ,i =%d \n", __func__, CurTx, CurRx, i);
	}
free_open_data:
	kfree(open_data);

	return ret;
}

static int focal_8201_open_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	unsigned int chl_x = 0;
	unsigned int chl_y = 0;
	unsigned int read_size = 0;
	int panel_data_max = 0;
	int panel_data_min = 0;

	int *panel_data = NULL;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: start,Second test ++:%d\n", __func__, test_num);
	if (!ft8201 || !params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = (unsigned int)params->channel_x_num;
	chl_y = (unsigned int)params->channel_y_num;

	read_size = chl_x * chl_y + (unsigned int)params->key_num;

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, (size_t)read_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "open_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory model fail, ret=%d\n",
				   __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*open test main function*/
	ret = focal_8201_get_open_test_data(ft8201, test_result->values, read_size, chl_x, chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get open test data fail\n", __func__);
		test_result->result = false;
		goto test_finish;
	}
	/* compare raw data with shreshold */
	panel_data = test_result->values;
	panel_data_min = params->threshold.open_test_cb_min;
	panel_data_max = MAX_CB_VALUE;
	for (i = 0; i < (int)test_result->size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if ((panel_data[i] > panel_data_max)
			|| (panel_data[i] < panel_data_min)) {

			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("open test failed",
								i / (int)chl_x,
								i % (int)chl_x,
								panel_data_max,
								panel_data_min,
								panel_data[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:open test pass!\n", __func__);
	} else {
		TS_LOG_ERR("%s: open test fail!\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	TS_LOG_INFO("%s: end,Second test ++:%d\n", __func__, test_num);
	return ret;
}

static int focal_8201_get_short_circuit_data(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y)
{
	int i = 0;
	int ret = 0;
	int adc_data_tmp = 0;

	if (!ft8201 || !data || (0 == size) || (0 == chl_x) || (0 == chl_y)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}

	ret = focal_8201_get_adc_data(ft8201, data, size, chl_x, chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get adc data fail, ret=%d\n", __func__, ret);
		return ret;
	}

	/* data check */
	for (i = 0; i < (int)size; i++) {
		if (MAX_ADC_NUM_8201 <= data[i]) {
			TS_LOG_DEBUG("%s:%s,adc_data[%d]=%d\n", __func__,
						 "adc data out of range", i, data[i]);
			data[i] = MAX_ADC_NUM_8201;
		}
	}

	/* data exchange */
	for (i = 0; i < (int)size; i++) {
		adc_data_tmp = data[i] * 100 / (4095 - data[i]);
		data[i] = adc_data_tmp;
	}

	return 0;
}
static int focal_8201_noise_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int lcd_data_size = 0;
	int lcd_noise_max = 0;
	int lcd_noise_min = 0;
	int framenum = 0;
	unsigned char lcd_value = 0;
	unsigned char  oldmode = 0;
	unsigned char  newmode = 0;
	unsigned char regdata = 0;
	unsigned char dataready = 0;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: start,Sixth test ++++++:%d\n", __func__, test_num);
	if (!params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	lcd_data_size = (chl_x * chl_y + params->key_num);
	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';

	ret = focal_alloc_test_container(&test_result, lcd_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	TS_LOG_INFO("%s:focal_alloc_test_container\n", __func__);
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "noise_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	TS_LOG_INFO("%s:focal_enter_factory\n", __func__);
	/*read old data type,after lcd noise test,need write oldmode value to REG_DATA_TYPE*/
	ret = focal_read_reg(REG_DATA_TYPE, &oldmode);
	if (ret) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*read old data,after lcd noise test,need write regdata value to 0x5E*/
	ret = focal_read_reg(REG_CA_FILTER, &regdata);
	if (ret) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	/*set the upper limit of CA filter*/
	ret = focal_8201_write_reg(ft8201, REG_CA_FILTER, CA_FILTER_UPPER_LIMIT_VALUE);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*write diff mode*/
	ret = focal_8201_write_reg(ft8201, REG_DATA_TYPE, IS_DIFFER_MODE);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
	/*read new data type*/
	ret = focal_read_reg(REG_DATA_TYPE, &newmode);
	if (ret != ERROR_CODE_OK || newmode != IS_DIFFER_MODE) {
		TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*write test framenum*/
	framenum = LCD_TEST_FRAMENUM_8201;//50 is test framenum
	ret = focal_8201_write_reg(ft8201, REG_8201_LCD_NOISE_FRAME, framenum);//0x12
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*clean counter*/
	ret = focal_8201_write_reg(ft8201, REG_RAWDATA_LINE_ADDR, RAWDATA_LINE_ADDR_CLEAR);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*start lcd noise test*/
	ret = focal_8201_write_reg(ft8201, REG_8201_LCD_NOISE_START, START_LCD_NOISE_VALUE);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}

	TS_LOG_INFO("%s:start noise test\n", __func__);

	for ( i = 0; i <= TEST_TIMEOUT; i++) {
		msleep(FTS_LCD_NOISE_TEST_DELAY_TIME);
		/*wait lcd noise finished*/
		ret = focal_read_reg(REG_8201_LCD_NOISE_START, &dataready);//0x00
		if (ret) {
			TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
			goto test_finish;
		}
		if (0x00 == dataready) {
			break;
		}

		if (TEST_TIMEOUT == i) {
			TS_LOG_ERR("%s:noise test Time out, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}
	TS_LOG_INFO("%s:start read raw data i = %d\n", __func__, i);
	/*get noise data*/
	ret = focal_8201_get_noise_format(ft8201, chl_x, chl_y, test_result->values, test_result->size);
	if (ret) {
		TS_LOG_ERR("%s:get raw data fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*reset ic,write oldmode value back-up REG_DATA_TYPE status*/
	ret = focal_8201_write_reg(ft8201, REG_DATA_TYPE, oldmode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	/*reset ic,write regdata value back-up REG_CA_FILTER status */
	ret = focal_8201_write_reg(ft8201, REG_CA_FILTER, regdata);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	/*rest ic,stop lcd noise test*/
	ret = focal_8201_write_reg(ft8201, REG_8201_LCD_NOISE_START, 0x00);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	ret = focal_enter_work();
	if (ret) {
		TS_LOG_ERR("%s:enter work mode fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	msleep(100);
	/*read lcd noise value*/
	ret = focal_read_reg(REG_8201_LCD_NOISE_VALUE, &lcd_value);//0x80
	if (ret) {
		TS_LOG_ERR("%s :reed reg dats read  fail, ret=%d\n", __func__, ret);
		goto test_finish;
	}
	TS_LOG_INFO("get lcd reg value: %d\n", lcd_value);
	ret = focal_enter_factory();
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n", __func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*lcd_value * 32  touchscreen point threshold*/
	lcd_noise_max = (params->threshold.lcd_noise_max) * lcd_value * LEFT_SHIFT_FOUR / PERCENTAGE_DATA;
	/* compare raw data with shreshold */
	for (i = 0; i < lcd_data_size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if ((test_result->values[i] > lcd_noise_max ) || (test_result->values[i] < lcd_noise_min)) {
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			test_result->result = false;
			TS_LOG_ERR("%s:Noise test failured,data[%d] = %d", __func__, i, test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:noise test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:noise test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	ret = focal_8201_write_reg(ft8201, REG_DATA_TYPE, oldmode);
	if (ret) {
		TS_LOG_ERR("%s:write reg dats read  fail, ret=%d\n", __func__, ret);
		return ret;
	}
	TS_LOG_INFO("%s: end,Sixth test ++++++:%d\n", __func__, test_num);
	return ret;
}

static int focal_8201_cb_increase_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int cb_increase_level = 0;
	int cb_increase_min = 0;
	int cb_increase_size = 0;
	u8 dataready = 0;
	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: start, Seventh test +++++++:%d\n", __func__, test_num);
	if (!ft8201 || !params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;
	cb_increase_level = params->threshold.cb_increase_level;
	cb_increase_min = params->threshold.cb_increase_min;
	cb_increase_size = chl_x * chl_y;

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, cb_increase_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc cb increase data container fail, ret=%d\n",
				__func__, ret);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name, "cb_increase_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	ret = focal_write_reg(REG_8201_CB_INCREASE_LEVEL, cb_increase_level);
	if (ret) {
		TS_LOG_ERR("%s:write cb increase level reg fail, ret=%d\n", __func__, ret);
		return ret;
	}

	/*start cb increase test*/
	ret = focal_write_reg(REG_8201_CB_INCREASE_START, START_CB_INCREASE_VALUE);
	if (ret) {
		TS_LOG_ERR("%s:write cb increase start reg fail, ret=%d\n", __func__, ret);
		return ret;
	}
	/*set CB level,wait data ready */
	for ( i = 0; i <= CB_INCREASE_TIMEOUT; i++) {
		msleep(FTS_CB_INCREASE_TEST_DELAY_TIME);
		ret = focal_read_reg(REG_8201_CB_INCREASE_START, &dataready);
		if (ret) {
			TS_LOG_ERR("%s:read reg dats read  fail, ret=%d\n", __func__, ret);
			goto test_finish;
		}
		if (CB_INCREASE_FINISH_VALUE == dataready) {
			break;
		}
		if (CB_INCREASE_TIMEOUT == i) {
			TS_LOG_ERR("%s:cb increase test Time out, ret=%d\n", __func__, ret);
			test_result->result = false;
			goto test_finish;
		}
	}
	TS_LOG_INFO("%s:start read raw data i = %d\n", __func__, i);
	/*after set CB level,get rawdata diff data*/
	ret = focal_8201_get_raw_data_format(ft8201, chl_x, chl_y, test_result->values, test_result->size);
	if (ret) {
		TS_LOG_ERR("%s:get raw data fail, ret=%d\n",
				__func__, ret);

		test_result->result = false;
		goto test_finish;
	}

	/* compare cb increase data with shreshold */
	for (i = 0; i < cb_increase_size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if (test_result->values[i] < cb_increase_min) {

			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("cb increase test fail",
								i / chl_x,
								i % chl_x,
								0,
								cb_increase_min,
								test_result->values[i]);
		}
	}

test_finish:
	/*set CB level,wait data ready,reset rawdata buffer */
	ret = focal_write_reg(REG_8201_CB_INCREASE_START, CB_INCREASE_RESET);
	if (ret) {
		TS_LOG_ERR("%s:write reg fail, ret=%d\n", __func__, ret);
		return ret;
	}
	if (test_result->result) {

		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:cb increase test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:cb increase test fail\n", __func__);
	}
	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN - 1);
	*result = test_result;
	TS_LOG_INFO("%s: end, Seventh test +++++++:%d\n", __func__, test_num);
	return ret;
}

static int focal_8201_short_circuit_test(
	struct focal_test_8201 *ft8201,
	struct focal_test_params *params,
	struct focal_test_result **result, char test_num)
{
	int i = 0;
	int ret = 0;
	int chl_x = 0;
	int chl_y = 0;
	int adc_data_size = 0;
	int short_data_min = 0;

	char result_code[FTS_RESULT_CODE_LEN] = {0};
	struct focal_test_result *test_result = NULL;

	TS_LOG_INFO("%s: start,Third test +++:%d\n", __func__, test_num);
	if (!ft8201 || !params || !result || !(params->channel_x_num) || !(params->channel_y_num)) {
		TS_LOG_ERR("%s: parameters invalid !\n", __func__);
		return -EINVAL;
	}
	chl_x = params->channel_x_num;
	chl_y = params->channel_y_num;


	adc_data_size = (chl_x * chl_y + params->key_num);

	result_code[0] = test_num + '0';
	result_code[1] = 'F'; //default result_code is failed
	result_code[2] = '\0';
	ret = focal_alloc_test_container(&test_result, adc_data_size);
	if (ret) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		*result = NULL;
		return ret;
	}
	/*default value true*/
	test_result->result = true;
	strncpy(test_result->test_name,
			"short_circuit_test", FTS_TEST_NAME_LEN - 1);
	/*every test case must repeat confirm enter factory*/
	ret = focal_8201_enter_factory(ft8201);
	if (ret) {
		TS_LOG_ERR("%s:enter factory mode fail, ret=%d\n",
				__func__, ret);
		test_result->result = false;
		goto test_finish;
	}
	/*short data test function*/
	ret = focal_8201_get_short_circuit_data(ft8201, test_result->values, adc_data_size, chl_x, chl_y);
	if (ret) {
		TS_LOG_ERR("%s:get short circuit data fail, ret=%d\n",
				__func__, ret);
		test_result->result = false;
		goto test_finish;
	}

	/* compare raw data with shreshold */
	short_data_min = params->threshold.short_circuit_min;

	for (i = 0; i < adc_data_size; i++) {
		//TS_LOG_INFO("test_result->values[%d] = %d", i+1, test_result->values[i]);
		if (short_data_min > test_result->values[i]) {
			test_result->result = false;
			tp_cap_test_status = PANEL_REASON;
			strncpy(tp_test_failed_reason, "-panel_reason", TP_TEST_FAILED_REASON_LEN);
			focal_print_test_data("short circuit test fail:",
								i / chl_x,
								i % chl_x,
								0,
								short_data_min,
								test_result->values[i]);
		}
	}

test_finish:
	if (test_result->result) {
		result_code[1] = 'P'; //test pass
		TS_LOG_INFO("%s:short circuit test pass\n", __func__);
	} else {
		TS_LOG_ERR("%s:short circuit test fail\n", __func__);
	}

	strncpy(test_result->result_code, result_code, FTS_RESULT_CODE_LEN);
	*result = test_result;
	TS_LOG_INFO("%s: end,Third test +++:%d\n", __func__, test_num);
	return ret;
}


static int focal_start_adc_scan(void)
{
	int ret = 0;

	/* start adc sample */
	ret = focal_write_reg(REG_ADC_SCAN, 1);
	if (ret) {
		TS_LOG_ERR("%s:adc scan fail, ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}
static int focal_get_adc_scan_result(void)
{
	int i = 0;
	int ret = 0;
	int scan_query_times = 50;
	u8 reg_val = 0;

	for (i = 0; i < scan_query_times; i++) {
		ret = focal_read_reg(REG_ADC_SCAN_STATUS, &reg_val);
		if (!ret) {
			if (reg_val == 0) {
				TS_LOG_INFO("%s:adc scan success\n", __func__);
				return 0;
			} else {
				TS_LOG_INFO("%s:adc scan status:0x%02X\n",
							__func__, reg_val);
				msleep(50);
			}
		} else {
			return ret;
		}
	}

	TS_LOG_ERR("%s:adc scan timeout\n", __func__);
	return -ETIMEDOUT;

}
static int focal_get_adc_result(int *data, size_t size)
{
	int i = 0;
	int ret = 0;
	int pkg_size = 0;
	int pkg_count = 0;
	int readed_count = 0;
	int adc_data_size = 0;
	u8 cmd[2] = {0};
	u8 *adc_data = NULL;
	int adc_hig_inx = 0;
	int adc_low_inx = 0;

	if (!data) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}

	adc_data_size = size * 2;
	pkg_count = adc_data_size / BYTES_PER_TIME;
	if (adc_data_size % BYTES_PER_TIME != 0)
		pkg_count += 1;

	adc_data = kzalloc(adc_data_size, GFP_KERNEL);
	if (!adc_data) {
		TS_LOG_ERR("%s:alloc mem fail\n", __func__);
		return -ENOMEM;
	}

	cmd[0] = REG_ADC_DATA_ADDR;
	for (i = 0; i < pkg_count; i++) {

		/* if the last package is not full, set package size */
		if ((i + 1 == pkg_count)
			&& (adc_data_size % BYTES_PER_TIME != 0)) {

			pkg_size = adc_data_size % BYTES_PER_TIME;
		} else {
			pkg_size = BYTES_PER_TIME;
		}

		/* the first package should send a command and read */
		if (i == 0) {
			ret = focal_read(cmd, 1, adc_data, pkg_size);
		} else {
			ret = focal_read_default(adc_data + readed_count,
									 pkg_size);
		}

		if (ret) {
			TS_LOG_ERR("%s:read adc data fail, ret=%d\n",
					   __func__, ret);
			goto free_mem;
		}

		readed_count += pkg_size;
	}

	for (i = 0; i < (int)size; i++) {
		adc_hig_inx = i * 2;
		adc_low_inx = adc_hig_inx + 1;
		data[i] = (adc_data[adc_hig_inx] << 8) + adc_data[adc_low_inx];
		TS_LOG_DEBUG("%s:adc_data[%d] =%d \n", __func__, i + 1, data[i]);
	}

	ret = 0;
free_mem:
	kfree(adc_data);
	adc_data = NULL;
	return ret;
}

static int focal_8201_get_adc_data(struct focal_test_8201 *ft8201, int *data, size_t size, unsigned int chl_x, unsigned int chl_y)
{
	int ret = 0;
	struct cs_chip_addr_mgr mgr;
	int master_adc_num = 0;
	int slave_adc_num = 0;
	int *buffer_master = NULL;
	int *buffer_slave = NULL;
	int master_tx = 0;
	int master_rx = 0;
	int slave_tx = 0;
	int slave_rx = 0;
	int master_buf_size = 0;
	int slave_buf_size = 0;

	if (!ft8201 || !data) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -ENODEV;
	}
	master_tx = get_master_tx(ft8201->cs_info);
	master_rx = get_master_rx(ft8201->cs_info);
	slave_tx = get_slave_tx(ft8201->cs_info);
	slave_rx = get_slave_rx(ft8201->cs_info);
	/*judge ic type is cascade connection*/
	if ( chip_has_two_heart(ft8201) ) {
		master_buf_size = sizeof(int)*(master_tx + 1) * master_rx;
		buffer_master = (int *)kzalloc(master_buf_size, GFP_KERNEL);
		if (!buffer_master) {
			TS_LOG_ERR("%s:alloc mem fail\n", __func__);
			return -ENOMEM;
		}
		slave_buf_size = sizeof(int)*(slave_tx + 1) * slave_rx;
		buffer_slave = (int *)kzalloc(slave_buf_size, GFP_KERNEL);
		if (!buffer_slave) {
			TS_LOG_ERR("%s:alloc mem fail\n", __func__);
			goto release_master;
		}
		/*get current addr 0x70,certain current work master model*/
		cs_chip_addr_mgr_init(&mgr, ft8201);
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		ret = focal_start_adc_scan();
		if (ret) {
			TS_LOG_ERR("%s:start adc scan fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		/*move to master addr,work on master ic*/
		work_as_master(&mgr);
		master_adc_num = (master_tx * master_rx + 6);
		ret =  focal_get_adc_scan_result();
		if (ret) {
			TS_LOG_ERR("%s:get adc scan result fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		ret =  focal_get_adc_result(buffer_master, master_adc_num);
		if (ret) {
			TS_LOG_ERR("%s:get adc result fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		/*move to slave addr,work on slave ic*/
		work_as_slave(&mgr);
		slave_adc_num = (slave_tx * slave_rx + 6);
		ret = focal_get_adc_scan_result();
		if (ret) {
			TS_LOG_ERR("%s:get adc scan result fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		ret = focal_get_adc_result(buffer_slave, slave_adc_num);
		if (ret) {
			TS_LOG_ERR("%s:get adc result fail, ret=%d\n", __func__, ret);
			goto release_slave;
		}
		/*master ic data and slave ic data form 48*30 data array*/
		cat_single_to_one_screen(ft8201, buffer_master, buffer_slave, data);
		/*scan over,back to devault addr slave_addr*/
		cs_chip_addr_mgr_exit(&mgr);

release_slave:
		if (buffer_slave)
			kfree(buffer_slave);
release_master:
		if (buffer_master)
			kfree(buffer_master);
	}

	return ret;
}


static int focal_8201_init_test_prm(
	struct focal_platform_data *pdata,
	struct focal_test_params *params,
	struct ts_rawdata_info *info)
{
	int ret = 0;

	TS_LOG_INFO("%s: set param data called\n", __func__);
	if ((!pdata ) || !(params) || !(info)) {
		TS_LOG_ERR("%s: parameters invalid!\n", __func__);
		return -EINVAL;
	}
	/*first test term i2c test is here*/
	ret = focal_get_channel_form_ic(params);
	if (ret) {
		TS_LOG_ERR("%s:get channel num fail,I2C communication error! ret=%d\n", __func__, ret);
		focal_strncat(info->result, "0F-", TS_RAWDATA_RESULT_MAX);
		focal_strncat(info->result, "software reason ", TS_RAWDATA_RESULT_MAX);
		return ret;
	} else {
		TS_LOG_INFO("%s: i2c test pass.ret =%d\n", __func__,ret);
		focal_strncat(info->result, "0P-", TS_RAWDATA_RESULT_MAX);
	}
	/*threahold init in this function*/
	ret = focal_parse_cap_test_config(pdata, params);
	if (ret < 0) {
		TS_LOG_ERR("%s: analysis tp test data failed\n", __func__);
		return ret;
	}

	TS_LOG_INFO("%s: set param data success\n", __func__);

	return 0;
}

void focal_8201_prase_threshold_for_csv(const char *project_id,
	struct focal_test_threshold *threshold,
	struct focal_test_params *params)
{
	int ret = 0;
	char csv_path[CAP_TEST_BUF_SIZE] = {0};
	u32 data_buf[RAW_DATA_BUF_SIZE] = {0};
	char file_name[FTS_THRESHOLD_NAME_LEN] = {0};

	struct focal_platform_data *fts_pdata = focal_get_platform_data();

	TS_LOG_INFO("%s: TP test preparation\n", __func__);

	if (!project_id || !threshold || !params) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return;
	}
	/*full csv name*/
	snprintf(file_name, FTS_THRESHOLD_NAME_LEN, "%s_%s_%s_%s_limits.csv",
		g_focal_dev_data->ts_platform_data->product_name,
		FTS_CHIP_NAME, fts_pdata->project_id,
		fts_pdata->vendor_name);
	/*fimd csv file path*/
	snprintf(csv_path, sizeof(csv_path), "/odm/etc/firmware/ts/%s", file_name);
	TS_LOG_INFO("%s: csv_file_path =%s\n", __func__, csv_path);

	/*because cb data only have max/min data, so columns is 2/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, FTS_CB_TEST_CSV, data_buf, 1, 2);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, FTS_CB_TEST_CSV);
		return;
	}
	threshold->cb_test_min = data_buf[0];
	threshold->cb_test_max = data_buf[1];

	/*because short data only have min data, so columns is 1/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, DTS_SHORT_CIRCUIT_RES_MIN, data_buf, 1, 1);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_SHORT_CIRCUIT_RES_MIN);
		return;
	}
	threshold->short_circuit_min = data_buf[0];

	/*because lcd noise data only have max data, so columns is 1/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, DTS_LCD_NOISE_MAX, data_buf, 1, 1);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_LCD_NOISE_MAX);
		return;
	}
	threshold->lcd_noise_max = data_buf[0];

	/*because open data  only have max data, so columns is 1/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, FTS_OPEN_TEST_CSV, data_buf, 1, 1);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MIN);
		return;
	}
	threshold->open_test_cb_min = data_buf[0];

	/*because raw data only have max/min data, so columns is 2/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, FTS_RAWDATA_TEST_CSV, data_buf, 1, 2);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MIN);
		return;
	}
	threshold->raw_data_min = data_buf[0];
	threshold->raw_data_max = data_buf[1];

	/*because cb uniformity only have min for chx and chy data, so columns is 2/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, FTS_CB_UNIFORMITY_TEST_CSV, data_buf, 1, 2);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MIN);
		return;
	}
	threshold->cb_uniformity_x = data_buf[0];
	threshold->cb_uniformity_y = data_buf[1];

	/*because cb increase only have level and min  data, so columns is 2/rows is 1*/
	ret = ts_kit_parse_csvfile(csv_path, FTS_CB_INCREASE_CSV, data_buf, 1, 2);
	if (ret) {
		TS_LOG_ERR("%s: Failed get %s \n", __func__, DTS_OPEN_TEST_CB_MIN);
		return;
	}
	threshold->cb_increase_level = data_buf[0];
	threshold->cb_increase_min = data_buf[1];

	TS_LOG_INFO("%s:%s:%s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d, %s=%d\n",
		__func__, "cb test thresholds",
		"raw_data_min", threshold->raw_data_min,
		"raw_data_maxn", threshold->raw_data_max,
		"cb_test_min",  threshold->cb_test_min,
		"cb_test_maxn",  threshold->cb_test_max,
		"lcd_noise_max", threshold->lcd_noise_max,
		"short_circuit_min", threshold->short_circuit_min,
		"open_test_thresholdn",threshold->open_test_cb_min,
		"cb_uniformity_x", threshold->cb_uniformity_x,
		"cb_uniformity_yn", threshold->cb_uniformity_y,
		"cb_increase_level", threshold->cb_increase_level,
		"cb_increase_minn", threshold->cb_increase_min);
}

int focal_8201_get_raw_data(
	struct ts_rawdata_info *info,
	struct ts_cmd_node *out_cmd)
{
	int ret = 0;
	struct focal_platform_data *pdata = NULL;

	TS_LOG_INFO("%s:focal get rawdata called\n", __func__);

	if (!info || !out_cmd) {
		TS_LOG_ERR("%s: parameter error\n",  __func__);
		return -EINVAL;
	}

	pdata = focal_get_platform_data();

	memset(tp_test_failed_reason, 0, TP_TEST_FAILED_REASON_LEN);
	if (!param) {
		param = kzalloc(sizeof(struct focal_test_params), GFP_KERNEL);
		if (!param) {
			TS_LOG_ERR("%s:alloc mem for params fail\n", __func__);
			ret = -ENOMEM;
			goto err_alloc_mem;
		}
	}

	if (true == pdata->open_threshold_status) {
		ret = focal_8201_init_test_prm(pdata, param, info);
		if (ret < 0) {
			TS_LOG_ERR("%s:get param from dts fail, ret=%d", __func__, ret);
			ret  = 0;//use for printf I2c error information
			goto free_mem;
		}
		if (pdata->only_open_once_captest_threshold) {
			pdata->open_threshold_status = false;
		}
	}

	ret = focal_8201_start_test_tp(param, info);
	if (!ret) {
		TS_LOG_INFO("%s:tp test pass\n", __func__);
	}else{
		TS_LOG_ERR("%s:tp test fail, ret=%d\n", __func__, ret);
	}
	return ret;

free_mem:
	kfree(param);
	param = NULL;
	ret = focal_enter_work();
	if (ret < 0)
		TS_LOG_ERR("%s:enter work model fail, ret=%d\n", __func__, ret);
err_alloc_mem:

	return ret;
}

