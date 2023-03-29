/* Himax Android Driver Sample Code for Himax HX83102B chipset
*
* Copyright (C) 2017 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_ic.h"

#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif

#define HIMAX_ROI
#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#ifdef HX_TP_SYS_DIAG
	static int  touch_monitor_stop_flag = 0;
	uint8_t hx_nc_diag_coor[HX_RECEIVE_BUF_MAX_SIZE] = {0};
#endif
#endif

#ifdef HX_ESD_WORKAROUND
static uint8_t ESD_RESET_ACTIVATE = 1;
static int g_zero_event_count = 0;
#endif

#define RETRY_TIMES 200
#define HIMAX_VENDER_NAME  "himax"

uint8_t *mutual_iir1 = NULL;
uint8_t *mutual_dc1 = NULL;
char himax_nc_project_id[HX_PROJECT_ID_LEN+1]={"999999999"};

static uint8_t HW_NC_RESET_ACTIVATE  = 1;
static uint8_t EN_NoiseFilter	= 0x00;
static uint8_t Last_EN_NoiseFilter = 0x00;
static int hx_real_point_num = 0;

static uint32_t hx_id_name[ID_NAME_LEN] = {0};
static uint32_t hx_id_addr[ID_ADDR_LEN] = {0};
static uint32_t hx_flash_addr[FLASH_ADDR_LEN] = {0};
static unsigned char tmp_roi_data[ROI_DATA_READ_LENGTH] = {0};

static int gest_pt_cnt = 0;
static int gest_pt_x[GEST_PT_MAX_NUM] = {0};
static int gest_pt_y[GEST_PT_MAX_NUM] = {0};
static int gest_start_x = 0;
static int gest_start_y = 0;
static int gest_end_x = 0;
static int gest_end_y = 0;
static int gest_most_left_x = 0;
static int gest_most_left_y = 0;
static int gest_most_right_x = 0;
static int gest_most_right_y = 0;
static int gest_most_top_x = 0;
static int gest_most_top_y = 0;
static int gest_most_bottom_x=0;
static int gest_most_bottom_y=0;
static int16_t *self_data = NULL;
static int16_t *mutual_data = NULL;
static int HX_TOUCH_INFO_POINT_CNT = 0;

extern bool DSRAM_Flag;
extern int Flash_Size;
extern int Selftest_flag;

struct himax_ts_data *g_himax_nc_ts_data = NULL;
static struct mutex wrong_touch_lock;
static int himax_palm_switch(struct ts_palm_info *info);
int himax_nc_input_config(struct input_dev* input_dev); //himax_input_register(struct himax_ts_data *ts);
static void himax_set_smwp_enable(uint8_t smwp_enable);
extern int himax_nc_factory_start(struct himax_ts_data *ts,struct ts_rawdata_info *info_top);
extern int i2c_himax_nc_read(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
extern int i2c_himax_nc_write(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
extern void himax_nc_rst_gpio_set(int pinnum, uint8_t value);
extern void himax_nc_register_read(uint8_t *read_addr, int read_length, uint8_t *read_data);
extern void himax_flash_write_burst(uint8_t * reg_byte, uint8_t * write_data);
extern void himax_burst_enable(uint8_t auto_add_4_byte);
extern int himax_nc_write_read_reg(uint8_t *tmp_addr,uint8_t *tmp_data,uint8_t hb,uint8_t lb);
extern void himax_register_write(uint8_t *write_addr, int write_length, uint8_t *write_data);
int himax_read_project_id(void);
extern void hx_nc_setMutualNewBuffer(void);
extern int16_t *hx_nc_getMutualOldBuffer(void);
extern void hx_nc_setSelfBuffer(void);
extern int16_t *hx_nc_getMutualNewBuffer(void);


int himax_rw_reg_reformat_com(int reg_addr, int reg_data, uint8_t *addr_buf, uint8_t *data_buf )
{
	if (NULL==addr_buf && NULL==data_buf)
	{
		return HX_ERR;
	}
	addr_buf[3] = (uint8_t)((reg_addr >>24) & 0x000000FF);
	addr_buf[2] = (uint8_t)((reg_addr >>16) & 0x000000FF);
	addr_buf[1] = (uint8_t)((reg_addr >>  8) & 0x000000FF);
	addr_buf[0] = (uint8_t)(reg_addr & 0x000000FF);

	data_buf[3] = (uint8_t)((reg_data >>24) & 0x000000FF);
	data_buf[2] = (uint8_t)((reg_data >>16) & 0x000000FF);
	data_buf[1] = (uint8_t)((reg_data >>  8) & 0x000000FF);
	data_buf[0] = (uint8_t)(reg_data & 0x000000FF);

	TS_LOG_DEBUG("%s  addr_buf[3~1] = %2X  %2X  %2X  %2X \n",__func__,addr_buf[3], addr_buf[2], addr_buf[1], addr_buf[0]);
	TS_LOG_DEBUG("%s  data_buf[3~1] = %2X  %2X  %2X  %2X \n",__func__,data_buf[3], data_buf[2], data_buf[1], data_buf[0]);

	return NO_ERR;

}
int himax_rw_reg_reformat(int reg_data,  uint8_t *data_buf )
{
	if (NULL==data_buf)
	{
		return HX_ERR;
	}
	data_buf[3] = (uint8_t)((reg_data >>24) & 0x000000FF);
	data_buf[2] = (uint8_t)((reg_data >>16) & 0x000000FF);
	data_buf[1] = (uint8_t)((reg_data >>  8) & 0x000000FF);
	data_buf[0] = (uint8_t)(reg_data & 0x000000FF);

	TS_LOG_DEBUG("%s  data_buf[3~1] = %2X  %2X  %2X  %2X \n",__func__,data_buf[3], data_buf[2], data_buf[1], data_buf[0]);

	return NO_ERR;
}
#ifdef ROI
static int himax_knuckle(int hx_touch_info_size, uint8_t *buf)
{
	int i = DATA_INIT;

	TS_LOG_INFO("%s: Entering!\n", __func__);

	if(buf[hx_touch_info_size + IDX_PKG_NUM] == 0x01) {
		TS_LOG_INFO("processing 1st package!\n");
		for(i = 0 ; i < SIZE_1ST_PKG;i++) {
			tmp_roi_data[i] = buf[hx_touch_info_size + SIZE_HX_HEADER + i];
		}
	} else if(buf[hx_touch_info_size + IDX_PKG_NUM] == 0x02){
		TS_LOG_INFO("processing 2nd package!\n");
		for(i = 0 ; i < SIZE_2ND_PKG;i++) {
			tmp_roi_data[SIZE_1ST_PKG + i] = buf[hx_touch_info_size + SIZE_HX_HEADER + SIZE_HX_HEADER + i];
		}
	} else {
		TS_LOG_ERR("Package number fail = %d!\n", buf[hx_touch_info_size + IDX_PKG_NUM]);
	}

	if(buf[hx_touch_info_size + IDX_PKG_NUM] == 0x02) {
		/* report to the upper layer or assign to huawei's struct*/
		memcpy(g_himax_nc_ts_data->roi_data,tmp_roi_data,ROI_DATA_READ_LENGTH);
		for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
			TS_LOG_INFO("0x%02X,", g_himax_nc_ts_data->roi_data[i]);
			if( i >0 && (i % 16 == 15)) {
				printk("\n");
			}
		}
			printk("\n");
	}
	TS_LOG_INFO("%s: End!\n", __func__);
	return NO_ERR;

}


static unsigned char *himax_roi_rawdata(void)
{
	TS_LOG_INFO("%s:return roi data\n",__func__);
    return g_himax_nc_ts_data->roi_data;
}

static int himax_roi_switch(struct ts_roi_info *info)
{
    int i = 0;

    if (IS_ERR_OR_NULL(info)) {
        TS_LOG_ERR("info invaild\n");
        return -EINVAL;
    }

    switch (info->op_action) {
    case TS_ACTION_READ:
        if (HX_ROI_EN_PSD == hx_nc_getDiagCommand()) {
			info->roi_switch = HX_ROI_ENABLE;
        }
		else
		{
			info->roi_switch = HX_ROI_DISABLE;
		}
        break;
    case TS_ACTION_WRITE:
		if (!!info->roi_switch)
			g_diag_command = HX_ROI_EN_PSD;
		else
			g_diag_command = HX_ROI_DISABLE;

		himax_nc_diag_register_set((uint8_t)g_diag_command);

        if(!info->roi_switch){
            for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
                g_himax_nc_ts_data->roi_data[i] = 0;
            }
        }
        break;
    default:
        TS_LOG_ERR("op action invalid : %d\n", info->op_action);
        return -EINVAL;
    }

    return 0;
}
#endif

static int hmx_wakeup_gesture_enable_switch(struct ts_wakeup_gesture_enable_info *info)
{
	return NO_ERR;
}
static int himax_get_glove_switch(u8 *glove_switch)
{
	int ret = 0;
	u8 glove_value = 0;
	u8 glove_enable_addr = 0;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	himax_rw_reg_reformat(ADDR_GLOVE_EN, tmp_addr);
	himax_nc_register_read(tmp_addr,FOUR_BYTE_CMD,tmp_data);

	if ( tmp_data[3]==0xA5 && tmp_data[2]==0x5A &&
	     tmp_data[1]==0xA5 && tmp_data[0]==0x5A )
	{
		glove_value = GLOVE_EN;
	}

	*glove_switch = glove_value;

	TS_LOG_INFO("%s:glove value=%d\n", __func__, *glove_switch);
	return ret;
}

static int himax_set_glove_switch(u8 glove_switch)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t back_data[4] = {0};
	uint8_t retry_cnt = 0;

	TS_LOG_INFO("%s glove_switch :%d .\n",__func__,glove_switch);
	do//Enable:0x10007F10 = 0xA55AA55A
	{
		if(glove_switch)
		{
			himax_rw_reg_reformat_com(ADDR_GLOVE_EN,DATA_GLOVE_EN,tmp_addr,tmp_data);
			himax_flash_write_burst( tmp_addr, tmp_data);
		}
		else
		{
			himax_rw_reg_reformat_com(ADDR_GLOVE_EN,(int)glove_switch,tmp_addr,tmp_data);
			himax_flash_write_burst( tmp_addr, tmp_data);
		}
		back_data[3] = tmp_data[3];
		back_data[2] = tmp_data[2];
		back_data[1] = tmp_data[1];
		back_data[0] = tmp_data[0];
		himax_nc_register_read( tmp_addr, FOUR_BYTE_CMD, tmp_data);
		retry_cnt++;
	}while((tmp_data[3] != back_data[3] || tmp_data[2] != back_data[2] || tmp_data[1] != back_data[1]  || tmp_data[0] != back_data[0] ) && retry_cnt < MAX_RETRY_CNT);
	TS_LOG_INFO("%s end.\n",__func__);
	return NO_ERR;
}
static int himax_glove_switch(struct ts_glove_info *info)
{
	int ret = 0;

	if (!info) {
		TS_LOG_ERR("%s:info is null\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	switch (info->op_action) {
	case TS_ACTION_READ:
		ret = himax_get_glove_switch(&info->glove_switch);
		if (ret) {
			TS_LOG_ERR("%s:get glove switch fail,ret=%d\n",
				__func__, ret);
			return ret;
		} else {
			TS_LOG_INFO("%s:glove switch=%d\n",
				__func__, info->glove_switch);
		}

		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("%s:glove switch=%d\n",
			__func__, info->glove_switch);
		ret = himax_set_glove_switch(!!info->glove_switch);
		if (ret) {
			TS_LOG_ERR("%s:set glove switch fail, ret=%d\n",
				__func__, ret);
			return ret;
		}

		break;
	default:
		TS_LOG_ERR("%s:invalid op action:%d\n",
			__func__, info->op_action);
		return -EINVAL;
	}

	return 0;
}

static int himax_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	if((NULL == info)||(NULL == out_cmd)) {
		return HX_ERROR;
	}
	TS_LOG_INFO("%s: Entering\n",__func__);
	retval = himax_nc_factory_start(g_himax_nc_ts_data, info);
	TS_LOG_INFO("%s: End\n",__func__);

	return retval;
}
static int himax_get_capacitance_test_type(struct ts_test_type_info *info)
{
	struct ts_kit_device_data *chip_data = NULL;
	chip_data = g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->chip_data;

	TS_LOG_INFO("%s enter\n", __func__);
	if (!info){
		TS_LOG_ERR("%s\n", __func__);
		return INFO_FAIL;
	}
	memcpy(info->tp_test_type, chip_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
	TS_LOG_INFO("%s:test_type=%s\n", __func__, info->tp_test_type);
	return NO_ERR;
}

static int himax_irq_top_half(struct ts_cmd_node *cmd)
{
	 cmd->command = TS_INT_PROCESS;
	 TS_LOG_DEBUG("%s\n", __func__);
	 return NO_ERR;
}
int himax_nc_input_config(struct input_dev* input_dev)//himax_input_register(struct himax_ts_data *ts)
{
	TS_LOG_INFO("%s: called\n", __func__);
	if(NULL == input_dev)
	{
		return HX_ERROR;
	}
	g_himax_nc_ts_data->input_dev = input_dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	TS_LOG_INFO("input_set_abs_params: min_x %d, max_x %d, min_y %d, max_y %d\n",
		g_himax_nc_ts_data->pdata->abs_x_min, g_himax_nc_ts_data->pdata->abs_x_max, g_himax_nc_ts_data->pdata->abs_y_min, g_himax_nc_ts_data->pdata->abs_y_max);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,g_himax_nc_ts_data->pdata->abs_x_min, g_himax_nc_ts_data->pdata->abs_x_max, g_himax_nc_ts_data->pdata->abs_x_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,g_himax_nc_ts_data->pdata->abs_y_min, g_himax_nc_ts_data->pdata->abs_y_max, g_himax_nc_ts_data->pdata->abs_y_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,g_himax_nc_ts_data->pdata->abs_pressure_min, g_himax_nc_ts_data->pdata->abs_pressure_max, g_himax_nc_ts_data->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,0,g_himax_nc_ts_data->nFinger_support, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,g_himax_nc_ts_data->pdata->abs_pressure_min, g_himax_nc_ts_data->pdata->abs_pressure_max, g_himax_nc_ts_data->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,g_himax_nc_ts_data->pdata->abs_width_min, g_himax_nc_ts_data->pdata->abs_width_max, g_himax_nc_ts_data->pdata->abs_pressure_fuzz, 0);
	return NO_ERR;

}
static int himax_reset_device(void)
{
	int retval = NO_ERR;
	retval = himax_nc_HW_reset(HX_LOADCONFIG_DISABLE, HX_INT_DISABLE);
	return retval;
}
static void calcDataSize(uint8_t finger_num)
{
	struct himax_ts_data *ts_data = g_himax_nc_ts_data;
	ts_data->coord_data_size = HX_COORD_BYTE_NUM * finger_num;// 1 coord 4 bytes.
	ts_data->area_data_size = ((finger_num / HX_COORD_BYTE_NUM) + (finger_num % HX_COORD_BYTE_NUM ? 1 : 0)) * HX_COORD_BYTE_NUM;  // 1 area 4 finger ?
	ts_data->raw_data_frame_size = HX_RECEIVE_BUF_MAX_SIZE - ts_data->coord_data_size - ts_data->area_data_size - 4 - 4 - 1;
	//check if devided by zero
	if (ts_data->raw_data_frame_size == 0)
	{
		TS_LOG_ERR("%s devided by zero.");
		return;
	}
	ts_data->raw_data_nframes  = ((uint32_t)ts_data->x_channel * ts_data->y_channel + ts_data->x_channel + ts_data->y_channel) / ts_data->raw_data_frame_size +
						(((uint32_t)ts_data->x_channel * ts_data->y_channel + ts_data->x_channel + ts_data->y_channel) % ts_data->raw_data_frame_size)? 1 : 0;
	TS_LOG_INFO("%s: coord_data_size: %d, area_data_size:%d, raw_data_frame_size:%d, raw_data_nframes:%d", __func__,
				ts_data->coord_data_size, ts_data->area_data_size, ts_data->raw_data_frame_size, ts_data->raw_data_nframes);
}
static void calculate_point_number(void)
{
	HX_TOUCH_INFO_POINT_CNT = HX_NC_MAX_PT * HX_COORD_BYTE_NUM;

	if ( (HX_NC_MAX_PT % 4) == 0)
		HX_TOUCH_INFO_POINT_CNT += (HX_NC_MAX_PT / HX_COORD_BYTE_NUM) * HX_COORD_BYTE_NUM;
	else
		HX_TOUCH_INFO_POINT_CNT += ((HX_NC_MAX_PT /HX_COORD_BYTE_NUM) +1) * HX_COORD_BYTE_NUM;
}
#define HX_NC_RX_ABN_NUM_40	40
#define HX_NC_TX_ABN_NUM_20	20
#define HX_NC_PT_ABN_NUM_10	10
#define HX_NC_RES_ABN_NUM_2000	2000
static void himax_touch_information(void)
{
	uint8_t cmd[4] ={0};
	uint8_t cmd1[4] ={0};
	char data[12] = {0};
	char data1[12] = {0};
	int retry = 20;
	int reload_status = 0;

	if(IC_NC_TYPE == HX_83102B_SERIES_PWON ||
		IC_NC_TYPE == HX_83112A_SERIES_PWON)
	{
		while(reload_status == 0)
		{
			himax_rw_reg_reformat(ADDR_SWITCH_FLASH_RLD_STS, cmd1);
			himax_nc_register_read(cmd1, FOUR_BYTE_CMD, data1);
			himax_rw_reg_reformat(ADDR_SWITCH_FLASH_RLD, cmd);
			himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);

			if( (data1[1]==0x3A && data1[0]==0xA3) || (data[1]==0x72 && data[0]==0xC0))
			{
				TS_LOG_INFO("reload OK! \n");
				reload_status = 1;
				break;
			}
			else if(retry == 0)
			{
				TS_LOG_INFO("reload 20 times! fail \n");
				break;
			}
			else
			{
				retry --;
				msleep(HX_SLEEP_10MS);
				TS_LOG_INFO("reload fail ,delay 10ms retry=%d\n",retry);
			}
		}
		TS_LOG_INFO("%s : data[0]=0x%2.2X,data[1]=0x%2.2X,data[2]=0x%2.2X,data[3]=0x%2.2X\n",__func__,data[0],data[1],data[2],data[3]);
		TS_LOG_INFO("reload_status=%d\n",reload_status);

		himax_rw_reg_reformat(ADDR_TXRX_INFO, cmd);
		himax_nc_register_read(cmd, 2*FOUR_BYTE_CMD, data);
		HX_NC_RX_NUM	= data[2];
		HX_NC_TX_NUM	= data[3];
		HX_NC_MAX_PT	= data[4];


		himax_rw_reg_reformat(ADDR_XY_RVRS, cmd);
		himax_nc_register_read(cmd,FOUR_BYTE_CMD, data);

		if((data[1] & 0x04) == 0x04)
		{
			HX_NC_XY_REVERSE= true;
		}
		else
		{
			HX_NC_XY_REVERSE=false;
		}

		himax_rw_reg_reformat(ADDR_TP_RES, cmd);
		himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);
		HX_NC_Y_RES = data[0] * 256 + data[1];
		HX_NC_X_RES = data[2] * 256 + data[3];

		if(IC_NC_TYPE == HX_83102B_SERIES_PWON){
			if (HX_NC_RX_NUM > HX_NC_RX_ABN_NUM_40)
				HX_NC_RX_NUM= HX83102_RX_NUM;
			if (HX_NC_TX_NUM > HX_NC_TX_ABN_NUM_20)
				HX_NC_TX_NUM = HX83102_TX_NUM;
			if (HX_NC_MAX_PT > HX_NC_PT_ABN_NUM_10)
				HX_NC_MAX_PT= HX83102_MAX_PT;
			if (HX_NC_Y_RES > HX_NC_RES_ABN_NUM_2000)
				HX_NC_Y_RES = HX83102_Y_RES;
			if (HX_NC_X_RES > HX_NC_RES_ABN_NUM_2000)
				HX_NC_X_RES = HX83102_X_RES;
		} else if (IC_NC_TYPE == HX_83112A_SERIES_PWON){
			if (HX_NC_RX_NUM > HX_NC_RX_ABN_NUM_40)
				HX_NC_RX_NUM= HX83112_RX_NUM;
			if (HX_NC_TX_NUM > HX_NC_TX_ABN_NUM_20)
				HX_NC_TX_NUM = HX83112_TX_NUM;
			if (HX_NC_MAX_PT > HX_NC_PT_ABN_NUM_10)
				HX_NC_MAX_PT= HX83112_MAX_PT;
			if (HX_NC_Y_RES > HX_NC_RES_ABN_NUM_2000)
				HX_NC_Y_RES = HX83112_Y_RES;
			if (HX_NC_X_RES > HX_NC_RES_ABN_NUM_2000)
				HX_NC_X_RES = HX83112_X_RES;
		}

#ifdef HX_EN_MUT_BUTTON
	himax_rw_reg_reformat(ADDR_BT_NUM, cmd);
	himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);
	HX_NC_BT_NUM = data[3];
#else
		if(IC_NC_TYPE == HX_83102B_SERIES_PWON)
			HX_NC_BT_NUM = HX83102_BT_NUM;
		else if (IC_NC_TYPE == HX_83112A_SERIES_PWON)
			HX_NC_BT_NUM = HX83112_BT_NUM;
#endif
	}
	else
	{
		HX_NC_RX_NUM				= 0;
		HX_NC_TX_NUM				= 0;
		HX_NC_BT_NUM				= 0;
		HX_NC_X_RES				= 0;
		HX_NC_Y_RES				= 0;
		HX_NC_MAX_PT				= 0;
		HX_NC_XY_REVERSE			= false;
	}

	hx_nc_setXChannel(HX_NC_RX_NUM); // X channel
	hx_nc_setYChannel(HX_NC_TX_NUM); // Y channel
	TS_LOG_INFO("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__, HX_NC_RX_NUM, HX_NC_TX_NUM, HX_NC_MAX_PT);
	TS_LOG_INFO("%s:HX_XY_REVERSE =%d,HX_Y_RES =%d,HX_X_RES=%d \n", __func__, HX_NC_XY_REVERSE, HX_NC_Y_RES,HX_NC_X_RES);
}
void himax_nc_get_information(void)
{
	TS_LOG_INFO("%s:enter\n", __func__);
	himax_touch_information();
	TS_LOG_INFO("%s:exit\n", __func__);
}
/*int_off:  false: before reset, need disable irq; true: before reset, don't need disable irq.*/
/*loadconfig:  after reset, load config or not.*/
int himax_nc_HW_reset(bool loadconfig, bool int_off)
{
	int retval = 0;
	HW_NC_RESET_ACTIVATE = 1;
	if(HX_NC_UPDATE_FLAG == UPDATE_ONGOING)
	{
		HX_NC_RESET_COUNT++;
		TS_LOG_INFO("HX still in updateing no reset ");
		return retval;
	}
	TS_LOG_INFO("%s: Now reset the Touch chip.\n", __func__);

	himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 0);
	msleep(RESET_LOW_TIME);
	himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 1);
	msleep(RESET_HIGH_TIME);

	if(loadconfig)
		retval = himax_nc_loadSensorConfig();
	if(retval < 0)
		return retval;
	return NO_ERR;
}
static bool himax_ic_package_check(void)
{
	int i = 0;
	int retry = 3;
	uint8_t ret_data = 0x00;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	for (i = 0; i < retry; i++)
	{
		// Product ID
		// Touch
		tmp_addr[3] = (uint8_t)hx_id_addr[3];
		tmp_addr[2] = (uint8_t)hx_id_addr[2];
		tmp_addr[1] = (uint8_t)hx_id_addr[1];
		tmp_addr[0] = (uint8_t)hx_id_addr[0];

		TS_LOG_INFO("%s:tmp_addr = %X,%X,%X,%X\n", __func__, tmp_addr[0],tmp_addr[1],tmp_addr[2],tmp_addr[3]);

		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		TS_LOG_INFO("%s:Read driver IC ID = %X,%X,%X\n", __func__, tmp_data[3],tmp_data[2],tmp_data[1]);

		if ((tmp_data[3] == (uint8_t)hx_id_name[0]) &&
		    (tmp_data[2] == (uint8_t)hx_id_name[1]) &&
		    (tmp_data[1] == (uint8_t)hx_id_name[2]))
		{
		  	IC_NC_CHECKSUM 	= HX_TP_BIN_CHECKSUM_CRC;

		  	//Himax: Set FW and CFG Flash Address
		  	NC_FW_VER_MAJ_FLASH_ADDR   	= hx_flash_addr[0];
		  	FW_VER_MAJ_FLASH_LENG   	= hx_flash_addr[1];
		  	NC_FW_VER_MIN_FLASH_ADDR   	= hx_flash_addr[2];
		  	FW_VER_MIN_FLASH_LENG  	 	= hx_flash_addr[3];
		  	NC_CFG_VER_MAJ_FLASH_ADDR 	= hx_flash_addr[4];
		  	CFG_VER_MAJ_FLASH_LENG 	 	= hx_flash_addr[5];
		  	NC_CFG_VER_MIN_FLASH_ADDR 	= hx_flash_addr[6];
		  	CFG_VER_MIN_FLASH_LENG 	 	= hx_flash_addr[7];
			NC_CID_VER_MAJ_FLASH_ADDR	= hx_flash_addr[8];
			CID_VER_MAJ_FLASH_LENG	 	= hx_flash_addr[9];
			NC_CID_VER_MIN_FLASH_ADDR	= hx_flash_addr[10];
			CID_VER_MIN_FLASH_LENG		= hx_flash_addr[11];
			//PANEL_VERSION_ADDR			= hx_flash_addr[12];
			//PANEL_VERSION_LENG			= hx_flash_addr[13];

			if((tmp_data[3] == HX_83102_ID_PART_1) &&
				(tmp_data[2] == HX_83102_ID_PART_2) &&
				(tmp_data[1] == HX_83102_ID_PART_3)) {
				IC_NC_TYPE         		= HX_83102B_SERIES_PWON;
				TS_LOG_INFO("Himax IC package HX83102 series \n");
			}
			else if((tmp_data[3] == HX_83112_ID_PART_1) &&
		   	    	(tmp_data[2] == HX_83112_ID_PART_2) &&
		   	    	(tmp_data[1] == HX_83112_ID_PART_3)){
				IC_NC_TYPE         		= HX_83112A_SERIES_PWON;
				TS_LOG_INFO("Himax IC package HX83112 series \n");
			}
			else
				TS_LOG_ERR("%s ID match fail!\n",__func__);
			ret_data = IC_PACK_CHECK_SUCC;

			break;
		}
		else
		{
			ret_data = IC_PACK_CHECK_FAIL;
			TS_LOG_ERR("%s:Read driver ID register Fail:\n", __func__);
		}
	}

	if(ret_data != IC_PACK_CHECK_SUCC)
		return IC_PACK_CHECK_FAIL;

	// 4. After read finish, set DDREG_Req = 0 (0x9000_0020 = 0x0000_0000) (Unlock register R/W from driver)
	himax_rw_reg_reformat_com(ADDR_SET_DDREG_REQ, DATA_INIT, &tmp_addr[0], &tmp_data[0]);
	himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);

	return ret_data;
}
void himax_nc_read_TP_info(void)
{
	uint8_t cmd[4] = {0};
	uint8_t data[64] = {0};

	//=====================================
	// Read FW version : 0x1000_7004  but 05,06 are the real addr for FW Version
	//=====================================

	himax_rw_reg_reformat(ADDR_READ_FW_VER, cmd);
	himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);

	g_himax_nc_ts_data->vendor_panel_ver =  data[0];
	g_himax_nc_ts_data->vendor_fw_ver = data[1] << 8 | data[2];

	TS_LOG_INFO("PANEL_VER : %X \n",g_himax_nc_ts_data->vendor_panel_ver);
	TS_LOG_INFO("FW_VER : %X \n",g_himax_nc_ts_data->vendor_fw_ver);

	himax_rw_reg_reformat(ADDR_READ_CONFIG_VER, cmd);
	himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);

	g_himax_nc_ts_data->vendor_config_ver = data[2] << 8 | data[3];

	TS_LOG_INFO("CFG_VER : %X \n",g_himax_nc_ts_data->vendor_config_ver);

	himax_rw_reg_reformat(ADDR_READ_CID_VER, cmd);
	himax_nc_register_read(cmd, FOUR_BYTE_CMD, data);

	g_himax_nc_ts_data->vendor_cid_maj_ver = data[2] ;
	g_himax_nc_ts_data->vendor_cid_min_ver = data[3];

	TS_LOG_INFO("CID_VER : %X \n",(g_himax_nc_ts_data->vendor_cid_maj_ver << 8 | g_himax_nc_ts_data->vendor_cid_min_ver));

}
#ifdef HX_ESD_WORKAROUND
static void ESD_HW_REST(void)
{
	if (self_test_nc_flag == 1 )
	{
		TS_LOG_INFO("In self test ,not  TP: ESD - Reset\n");
		return;
	}

	if(HX_NC_UPDATE_FLAG==UPDATE_ONGOING)
	{
		HX_NC_ESD_RESET_COUNT++;
		TS_LOG_INFO("HX still in updateing , no ESD reset");
		return;
	}
	ESD_RESET_ACTIVATE = 1;
	g_zero_event_count = 0;

	himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 0);
	msleep(RESET_LOW_TIME);
	himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 1);
	msleep(RESET_HIGH_TIME);

	TS_LOG_INFO("END_Himax TP: ESD - Reset\n");
}
#endif
static void himax_get_rawdata_from_event(int RawDataLen,int hx_touch_info_size, int mul_num, int sel_num,int index,uint8_t *buff )
{
	int i = 0;
	int temp1 = 0;
	int temp2 = 0;
	uint8_t *buf = NULL;

	if(NULL == buff) {
		return;
	}

	buf = kzalloc(sizeof(uint8_t)*(HX_RECEIVE_BUF_MAX_SIZE - hx_touch_info_size),GFP_KERNEL);
	if(NULL == buf) {
		return;
	}
	memcpy(buf, &buff[hx_touch_info_size], (HX_RECEIVE_BUF_MAX_SIZE - hx_touch_info_size));

	for (i = 0; i < RawDataLen; i++)
	{
		temp1 = index + i;
		if (temp1 < mul_num)//mutual
		{
			mutual_data[index + i] = buf[i*2 + 4 + 1]*256 + buf[i*2 + 4];
		}
		else//self
		{
			temp1 = i + index;
			temp2 = mul_num + sel_num;
			if (temp1 >= temp2)
			{
				break;
			}
			self_data[i+index-mul_num] = buf[i*2 + 4];	//4: RawData Header
			self_data[i+index-mul_num+1] = buf[i*2 + 4 + 1];
		}
	}

	if(NULL!=buf)
	{
		kfree(buf);
		buf =NULL;
	}

}
static int himax_start_get_rawdata_from_event(int hx_touch_info_size, int RawDataLen, uint8_t *buf)
{
	int i = 0;
	int index = 0;
	int check_sum_cal = 0;
	int mul_num = 0;
	int self_num = 0;
	int retval = NO_ERR;

	if((NULL == buf)||(hx_touch_info_size > HX_RECEIVE_BUF_MAX_SIZE - 3))
	{
		return HX_ERROR;
	}

	for (i = 0, check_sum_cal = 0; i < (HX_RECEIVE_BUF_MAX_SIZE - hx_touch_info_size); i=i+2)
	{
		check_sum_cal += (buf[i + hx_touch_info_size + 1]*256 + buf[i + hx_touch_info_size]);
	}

	if  (check_sum_cal % 0x10000 != 0)
	{
		TS_LOG_ERR("fail,  check_sum_cal: %d\n", check_sum_cal);
		retval = HX_ERROR;
		return retval;
	}

	mutual_data = hx_nc_getMutualBuffer();
	self_data       = hx_nc_getSelfBuffer();
	mul_num = hx_nc_getXChannel() * hx_nc_getYChannel();
	self_num = hx_nc_getXChannel() + hx_nc_getYChannel();
	//header format check
	if (buf[hx_touch_info_size] == 0x3A &&
		buf[hx_touch_info_size + 1] == 0xA3
		&& buf[hx_touch_info_size + 2] > 0)
	{
#ifdef ROI
		if( hx_nc_getDiagCommand() == HX_ROI_EN_PSD ) {
			himax_knuckle(hx_touch_info_size, buf);
		}
		else {
#endif
			RawDataLen = RawDataLen /2;
			index = (buf[hx_touch_info_size+2] - 1) * RawDataLen;
			himax_get_rawdata_from_event(RawDataLen, hx_touch_info_size,  mul_num, self_num, index, buf);
#ifdef ROI
		}
#endif
	}
	else
	{
		TS_LOG_INFO("[HIMAX TP MSG]%s: header format is wrong!\n", __func__);
		retval = HX_ERROR;
		return retval;
	}

	return retval;
}
#ifdef HX_ESD_WORKAROUND
static int himax_check_report_data_for_esd(int hx_touch_info_size, uint8_t *buf)
{
	int i = 0;
	int retval = 0;
	if(NULL == buf) {
		TS_LOG_ERR("himax_check_report_data_for_esd buf pointer NULL!\n");
		return HX_ERROR;
	}
	for(i = 0; i < hx_touch_info_size; i++)
	{
		if (buf[i] == ESD_EVENT_ALL_ZERO) //case 2 ESD recovery flow-Disable
		{
			retval = ESD_ALL_ZERO_BAK_VALUE;//if hand shanking fail,firmware error
		}
		else if(buf[i] == ESD_EVENT_ALL_ED)/*case 1 ESD recovery flow*/
		{
			retval = ESD_ALL_ED_BAK_VALUE;//ESD event,ESD reset
		}
		else
		{
			retval = 0;
			break;
		}
	}
	return retval;
}
#endif
static void himax_debug_level_print(int debug_mode,int status,int hx_touch_info_size,struct himax_touching_data hx_touching,uint8_t *buf)
{
	uint32_t m = (uint32_t)hx_touch_info_size;
	if(NULL == buf) {
		return;
	}
	switch(debug_mode)
	{
		case 0:
			for (hx_touching.loop_i = 0; hx_touching.loop_i < m ; hx_touching.loop_i++) {
				printk("0x%2.2X ", buf[hx_touching.loop_i]);
				if (hx_touching.loop_i % 8 == 7)
					printk("\n");
			}
			break;
		case 1:
			TS_LOG_INFO("Finger %d=> W:%d, Z:%d, F:%d, N:%d\n",
			hx_touching.loop_i + 1, hx_touching.w, hx_touching.w, hx_touching.loop_i + 1, EN_NoiseFilter);
			break;
		case 2:
			break;
		case 3:
			if(status == 0) //reporting down
			{
				if ((hx_touching.old_finger >> hx_touching.loop_i)  == 0)
				{
					if (g_himax_nc_ts_data->useScreenRes)
					{
							TS_LOG_INFO("Screen:F:%02d Down, W:%d, N:%d\n",
							hx_touching.loop_i+1,  hx_touching.w, EN_NoiseFilter);
					}
					else
					{
							TS_LOG_INFO("Raw:F:%02d Down, W:%d, N:%d\n",
							hx_touching.loop_i+1, hx_touching.w, EN_NoiseFilter);
					}
				}
			}
			else if(status == 1)  //reporting up
			{
				if ((hx_touching.old_finger >> hx_touching.loop_i)  == 1)
				{
					if (g_himax_nc_ts_data->useScreenRes)
					{
						TS_LOG_INFO("Screen:F:%02d Up, X:%d, Y:%d, N:%d\n",
						hx_touching.loop_i+1, (g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][0] * g_himax_nc_ts_data->widthFactor )>> SHIFTBITS,
						(g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][1] * g_himax_nc_ts_data->heightFactor) >> SHIFTBITS, Last_EN_NoiseFilter);
					}
					else{
						TS_LOG_INFO("Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",
						hx_touching.loop_i + 1, g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][0],
						g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][1], Last_EN_NoiseFilter);
					}
				}
			}
			else if(status == 2) //all leave event
			{
				for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_nc_ts_data->nFinger_support && (g_himax_nc_ts_data->debug_log_level & BIT(3)) > 0; hx_touching.loop_i++) {
					if (((g_himax_nc_ts_data->pre_finger_mask >>hx_touching.loop_i) & 1) == 1)
					{
						if (g_himax_nc_ts_data->useScreenRes) {
								TS_LOG_INFO("status:%X, Screen:F:%02d Up, X:%d, Y:%d, N:%d\n", 0,hx_touching.loop_i + 1, (g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][0] * g_himax_nc_ts_data->widthFactor )>> SHIFTBITS,
								(g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][1] * g_himax_nc_ts_data->heightFactor) >> SHIFTBITS, Last_EN_NoiseFilter);
						} else {
							TS_LOG_INFO("status:%X, Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",0, hx_touching.loop_i + 1, g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][0],g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][1], Last_EN_NoiseFilter);
						}
					}
				}
				g_himax_nc_ts_data->pre_finger_mask = 0;
			}
			break;
		default:
			break;
	}
}
static int himax_checksum_cal(int hx_touch_info_size, struct himax_touching_data hx_touching, uint8_t *buf)
{
	int checksum = 0;
	uint32_t m=(uint32_t)hx_touch_info_size;
	if(NULL == buf) {
		return  checksum;
	}
	for (hx_touching.loop_i = 0; hx_touching.loop_i < m; hx_touching.loop_i++)
		checksum += buf[hx_touching.loop_i];

	return checksum;
}
static void himax_parse_coords(int hx_touch_info_size,int hx_point_num,struct ts_fingers *info,struct himax_touching_data hx_touching,uint8_t *buf)
{
	int m=0;
	int m1=0;
	int m2=0;
	int base = 0;
	uint8_t coordInfoSize = g_himax_nc_ts_data->coord_data_size + g_himax_nc_ts_data->area_data_size + 4;

	TS_LOG_DEBUG("%s enter \n",__func__);

	if(NULL == buf||NULL == info) {
		return ;
	}

	if (hx_point_num != 0 )
	{
		hx_touching.old_finger = g_himax_nc_ts_data->pre_finger_mask;
		g_himax_nc_ts_data->pre_finger_mask = 0;
		hx_touching.finger_num =  buf[coordInfoSize - 4] & 0x0F;

		for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_nc_ts_data->nFinger_support; hx_touching.loop_i++)
		{
			base = hx_touching.loop_i * 4;//every finger coordinate need 4 bytes.
			m=base + 1;
			m1=base + 2;
			m2=base + 3;
			hx_touching.x = ((buf[base]) << 8) |(buf[m]);
			hx_touching.y = ((buf[m1]) << 8 )| (buf[m2]);
			hx_touching.w = 10;

			if(hx_touching.x  >= 0 && hx_touching.x  <= g_himax_nc_ts_data->pdata->abs_x_max && hx_touching.y  >= 0 && hx_touching.y  <= g_himax_nc_ts_data->pdata->abs_y_max)
			{
				if ((g_himax_nc_ts_data->debug_log_level & BIT(3)) > 0)//debug 3: print finger coordinate information
				{
					himax_debug_level_print(3,0,hx_touch_info_size,hx_touching,buf); //status = report down
				}

				info->fingers[hx_touching.loop_i].status = TS_FINGER_PRESS;
				info->fingers[hx_touching.loop_i].x = hx_touching.x;
				info->fingers[hx_touching.loop_i].y = hx_touching.y;
				info->fingers[hx_touching.loop_i].major = 255;
				info->fingers[hx_touching.loop_i].minor = 255;
				info->fingers[hx_touching.loop_i].pressure = hx_touching.w;

				if (!g_himax_nc_ts_data->first_pressed)
				{
					g_himax_nc_ts_data->first_pressed = 1;//first report
				}

				g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][0] = hx_touching.x;
				g_himax_nc_ts_data->pre_finger_data[hx_touching.loop_i][1] = hx_touching.y;

				if (g_himax_nc_ts_data->debug_log_level & BIT(1))
					himax_debug_level_print(1,0,hx_touch_info_size,hx_touching, buf);  //status useless

				g_himax_nc_ts_data->pre_finger_mask = g_himax_nc_ts_data->pre_finger_mask + (1 <<  hx_touching.loop_i);
			}
			else
			{
				if (hx_touching.loop_i == 0 && g_himax_nc_ts_data->first_pressed == 1)
				{
					g_himax_nc_ts_data->first_pressed = 2;
					TS_LOG_DEBUG("E1@%d, %d\n",
					g_himax_nc_ts_data->pre_finger_data[0][0] , g_himax_nc_ts_data->pre_finger_data[0][1]);
				}
				if ((g_himax_nc_ts_data->debug_log_level & BIT(3)) > 0)
				{
					himax_debug_level_print(3,1,hx_touch_info_size,hx_touching,buf); //status= report up
				}
			}
		}
	}
	else
	{
		for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_nc_ts_data->nFinger_support; hx_touching.loop_i++) {
				if (((g_himax_nc_ts_data->pre_finger_mask >> hx_touching.loop_i) & 1) == 1) {

					info->fingers[hx_touching.loop_i].status = TS_FINGER_RELEASE;
					info->fingers[hx_touching.loop_i].x = 0;
					info->fingers[hx_touching.loop_i].y = 0;
					info->fingers[hx_touching.loop_i].major = 0;
					info->fingers[hx_touching.loop_i].minor = 0;
					info->fingers[hx_touching.loop_i].pressure = 0;
				}
			}
		if (g_himax_nc_ts_data->pre_finger_mask > 0) {
			himax_debug_level_print(3, 3, hx_touch_info_size,hx_touching,buf);  //all leave event
		}

		if (g_himax_nc_ts_data->first_pressed == 1) {
			g_himax_nc_ts_data->first_pressed = 2;
			TS_LOG_DEBUG("E1@%d, %d\n",g_himax_nc_ts_data->pre_finger_data[0][0] , g_himax_nc_ts_data->pre_finger_data[0][1]);
		}

		if (g_himax_nc_ts_data->debug_log_level & BIT(1))
			TS_LOG_INFO("All Finger leave\n");
	}
}
static void himax_set_smwp_enable(uint8_t smwp_enable)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t back_data[4] = {0};
	uint8_t retry_cnt = 0;
	TS_LOG_INFO("%s enter.\n",__func__);
	do//Enable:0x10007F10 = 0xA55AA55A
	{
		if(smwp_enable)
		{
			himax_rw_reg_reformat_com(ADDR_SMWP_EN,DATA_SMWP_EN,tmp_addr,tmp_data);
			himax_flash_write_burst( tmp_addr, tmp_data);
			TS_LOG_INFO("smwp_enable.\n");
		}
		else
		{
			himax_rw_reg_reformat_com(ADDR_SMWP_EN,(int)smwp_enable,tmp_addr,tmp_data);
			himax_flash_write_burst( tmp_addr, tmp_data);
			TS_LOG_INFO("smwp_disable.\n");
		}
		back_data[3] = tmp_data[3];
		back_data[2] = tmp_data[2];
		back_data[1] = tmp_data[1];
		back_data[0] = tmp_data[0];
		himax_nc_register_read( tmp_addr, FOUR_BYTE_CMD, tmp_data);
		retry_cnt++;
	}while((tmp_data[3] != back_data[3] || tmp_data[2] != back_data[2] || tmp_data[1] != back_data[1]  || tmp_data[0] != back_data[0] ) && retry_cnt < MAX_RETRY_CNT);
}

static void gest_pt_log_coordinate(int rx,int tx)
{
	gest_pt_x[gest_pt_cnt] = rx*HX_NC_X_RES/255;
	gest_pt_y[gest_pt_cnt] = tx*HX_NC_Y_RES/255;
}
static int easy_wakeup_gesture_report_coordinate(unsigned int reprot_gesture_point_num, struct ts_fingers *info,uint8_t* buf)
{
	int i = 0;
	int retval = 0;
	int tmp_max_x=0x00;
	int tmp_min_x=0xFFFF;
	int tmp_max_y=0x00;
	int tmp_min_y=0xFFFF;
	int gest_len = 0;
	int max_high_index = 0;
	int max_low_index = 0;
	int max_left_index = 0;
	int max_right_index = 0;

	if (reprot_gesture_point_num != 0)
      {
		/*
		 *The most points num is 6,point from 1(lower address) to 6(higher address) means:
		 *1.beginning 2.end 3.top 4.leftmost 5.bottom 6.rightmost
		 */
		if(buf[GEST_PTLG_ID_LEN] == GEST_PTLG_HDR_ID1 && buf[GEST_PTLG_ID_LEN+1] == GEST_PTLG_HDR_ID2)
		{
			gest_len = buf[GEST_PTLG_ID_LEN+2];
			i = 0;
			gest_pt_cnt = 0;
			while(i<(gest_len+1)/2)
                	{
				gest_pt_log_coordinate(buf[GEST_PTLG_ID_LEN+4+i*2],buf[GEST_PTLG_ID_LEN+4+i*2+1]);
				i++;
				TS_LOG_DEBUG("gest_pt_x[%d]=%d gest_pt_y[%d]=%d\n",gest_pt_cnt, gest_pt_x[gest_pt_cnt], gest_pt_cnt, gest_pt_y[gest_pt_cnt]);
				gest_pt_cnt +=1;
                	}
			if(gest_pt_cnt)
			{
				for(i=0; i<gest_pt_cnt; i++)
				{
					if(tmp_max_x<gest_pt_x[i])
					{
					    tmp_max_x=gest_pt_x[i];
					    max_right_index = i;
					}
					if(tmp_min_x>gest_pt_x[i])
					{
					    tmp_min_x=gest_pt_x[i];
					    max_left_index = i;
					}
					if(tmp_max_y<gest_pt_y[i])
					{
					    tmp_max_y=gest_pt_y[i];
					    max_high_index = i;
					}
					if(tmp_min_y>gest_pt_y[i])
					{
					    tmp_min_y=gest_pt_y[i];
					    max_low_index = i;
					}
				}
		//start
                gest_start_x=gest_pt_x[0];
                gest_start_y=gest_pt_y[0];
                //end
                gest_end_x=gest_pt_x[gest_pt_cnt-1];
                gest_end_y=gest_pt_y[gest_pt_cnt-1];
                //most_left
                gest_most_left_x = gest_pt_x[max_left_index];
                gest_most_left_y = gest_pt_y[max_left_index];
                //most_right
                gest_most_right_x = gest_pt_x[max_right_index];
                gest_most_right_y = gest_pt_y[max_right_index];
                //top
                gest_most_top_x = gest_pt_x[max_high_index];
                gest_most_top_y = gest_pt_x[max_high_index];
                //bottom
                gest_most_bottom_x = gest_pt_x[max_low_index];
                gest_most_bottom_y = gest_pt_x[max_low_index];

            }
		}
		TS_LOG_INFO("%s: gest_len = %d\n", __func__, gest_len);

		if(reprot_gesture_point_num == 2){
			TS_LOG_INFO("%s: Gesture Dobule Click \n", __func__);
			/*1.beginning 2.end */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[0] = gest_start_x << 16 | gest_start_y;
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[1] = gest_end_x << 16 | gest_end_y;
			return retval;
		}
		else{
	           	 /*1.begin */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[0] = gest_start_x << 16 | gest_start_y;
			TS_LOG_INFO("begin = 0x%08x,  begin_x= %d , begin_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[0], gest_start_x, gest_start_y);
	          	/*2.end */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[1] = gest_end_x << 16 | gest_end_y;
			TS_LOG_INFO("top = 0x%08x,  end_x= %d , end_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[1], gest_end_x, gest_end_y);
	           	 /*3.top */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[2] = gest_most_top_x << 16 | gest_most_top_y;
			TS_LOG_INFO("top = 0x%08x,  top_x= %d , top_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[2], gest_most_top_x, gest_most_top_y);
	           	 /*4.leftmost */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[3] = gest_most_left_x << 16 | gest_most_left_y;
			TS_LOG_INFO("leftmost = 0x%08x,  left_x= %d , left_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[3], gest_most_left_x, gest_most_left_y);
	           	 /*5.bottom */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[4] = gest_most_bottom_x << 16 | gest_most_bottom_y;
			TS_LOG_INFO("bottom = 0x%08x,  bottom_x= %d , bottom_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[4], gest_most_bottom_x, gest_most_bottom_x);
	           	 /*6.rightmost */
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[5] = gest_most_right_x << 16 | gest_most_right_y;
			TS_LOG_INFO("rightmost = 0x%08x,  right_x= %d , right_y= %d \n",
				g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.easywake_position[5], gest_most_right_x, gest_most_right_y);
		}

	}
	return retval;
}
static int hmx_check_key_gesture_report( struct ts_fingers *info, struct ts_easy_wakeup_info *gesture_report_info, unsigned char get_gesture_wakeup_data, uint8_t* buf)
{
	int retval = 0;
	unsigned int reprot_gesture_key_value = 0;
	unsigned int reprot_gesture_point_num = 0;

	TS_LOG_DEBUG("get_gesture_wakeup_data is %d \n",
		    get_gesture_wakeup_data);

	switch (get_gesture_wakeup_data) {
		case DOUBLE_CLICK_WAKEUP:
			if (IS_APP_ENABLE_GESTURE(GESTURE_DOUBLE_CLICK) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_DEBUG("@@@DOUBLE_CLICK_WAKEUP detected!@@@\n");
				reprot_gesture_key_value = TS_DOUBLE_CLICK;
				reprot_gesture_point_num = LINEAR_LOCUS_NUM;
		}
		break;
		case SPECIFIC_LETTER_C:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_c) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_DEBUG
				    ("@@@SPECIFIC_LETTER_c detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_c;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_E:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_e) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_DEBUG
				    ("@@@SPECIFIC_LETTER_e detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_e;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_M:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_m) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_DEBUG
				    ("@@@SPECIFIC_LETTER_m detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_m;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
			break;
		case SPECIFIC_LETTER_W:
			if (IS_APP_ENABLE_GESTURE(GESTURE_LETTER_w) &
			    gesture_report_info->easy_wakeup_gesture) {
				TS_LOG_DEBUG
				    ("@@@SPECIFIC_LETTER_w detected!@@@\n");
				reprot_gesture_key_value = TS_LETTER_w;
				reprot_gesture_point_num = LETTER_LOCUS_NUM;
			}
		break;
	default:
		TS_LOG_INFO("@@@unknow gesture detected!\n");
		return 1;
	}

	if (0 != reprot_gesture_key_value) {
		mutex_lock(&wrong_touch_lock);

		if (true == gesture_report_info->off_motion_on) {
			retval = easy_wakeup_gesture_report_coordinate(
								  reprot_gesture_point_num,
								  info,buf);
			if (retval < 0) {
				mutex_unlock(&wrong_touch_lock);
				TS_LOG_ERR
				    ("%s: report line_coordinate error!retval = %d\n",
				     __func__, retval);
				return retval;
			}

			info->gesture_wakeup_value = reprot_gesture_key_value;
			TS_LOG_DEBUG
			    ("%s: info->gesture_wakeup_value = %d\n",
			     __func__, info->gesture_wakeup_value);
		}
		mutex_unlock(&wrong_touch_lock);
	}
	return NO_ERR;
}
static int himax_parse_wake_event(uint8_t *buf,struct ts_fingers *info)
{
	int i = 0;
	int retval=0;
	int check_FC = 0;
	int gesture_flag = 0;
	unsigned char check_sum_cal = 0;

	struct ts_easy_wakeup_info *gesture_report_info = &g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info;
	TS_LOG_INFO("Himax gesture buf[0] = 0x%x buf[1] = 0x%x buf[2] = 0x%x buf[3] = 0x%x\n",buf[0],buf[1],buf[2],buf[3] );
	for(i=0;i<4;i++)
	{
		if (check_FC==0)
		{
			if((buf[0]!=0x00)&&((buf[0]<=0x0F)||(buf[0]==0x80)))
			{
				check_FC = 1;
				gesture_flag = buf[i];
			}
			else
			{
				check_FC = 0;
				TS_LOG_INFO("ID START at %x , value = %x skip the event\n", i, buf[i]);
				break;
			}
		}
		else
		{
			if(buf[i]!=gesture_flag)
			{
				check_FC = 0;
				TS_LOG_INFO("ID NOT the same %x != %x So STOP parse event\n", buf[i], gesture_flag);
				break;
			}
		}

		TS_LOG_INFO("0x%2.2X ", buf[i]);
		if (i % 8 == 7)
				TS_LOG_INFO("\n");
	}
	TS_LOG_INFO("Himax gesture_flag= %x\n",gesture_flag );
	TS_LOG_INFO("Himax check_FC is %d\n", check_FC);

	if (check_FC == 0)
		return 1;
	if(buf[4] != 0xCC ||
			buf[4+1] != 0x44)
		return 1;
	for(i=0;i<(4+4);i++)
	{
		check_sum_cal += buf[i];
	}
	if ((check_sum_cal != 0x00) )
	{
		TS_LOG_INFO(" %s : check_sum_cal: 0x%02X\n",__func__ ,check_sum_cal);
		return 1;
	}
	retval = hmx_check_key_gesture_report(info,gesture_report_info,gesture_flag,buf);

	return retval;
}
static bool himax_read_event_stack(uint8_t *buf, uint8_t length)
{
	uint8_t cmd[4] = {0};

	//  AHB_I2C Burst Read Off
	cmd[0] = DATA_BURST_READ_OFF;
	if ( i2c_himax_nc_write(ADDR_BURST_READ ,cmd, ONE_BYTE_CMD, sizeof(cmd), DEFAULT_RETRY_CNT) < 0)
	{
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return I2C_FAIL;
	}

	i2c_himax_nc_read(ADDR_READ_EVENT_STACK, buf, length, HX_RECEIVE_BUF_MAX_SIZE, DEFAULT_RETRY_CNT);

	//  AHB_I2C Burst Read On
	cmd[0] = DATA_BURST_READ_ON;
	if ( i2c_himax_nc_write(ADDR_BURST_READ ,cmd, ONE_BYTE_CMD, sizeof(cmd), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return I2C_FAIL;
	}
	return NO_ERR;
}
static int himax_irq_bottom_half(struct ts_cmd_node *in_cmd,struct ts_cmd_node *out_cmd)
{
	int m=0;
	int retval = 0;
	int RawDataLen = 0;
	int raw_cnt_max = 0;
	int raw_cnt_rmd = 0;
	int hx_touch_info_size = 0;
	static int iCount = 0;
	uint8_t buf[HX_RECEIVE_BUF_MAX_SIZE] = {0};
	unsigned char check_sum_cal = 0;

	struct algo_param *algo_p = NULL;
	struct ts_fingers *info = NULL;
	struct himax_touching_data hx_touching;

#ifdef HX_TP_SYS_DIAG
	uint8_t diag_cmd = 0;
#endif

	TS_LOG_DEBUG("%s:enter\n",__func__);

	if(NULL == in_cmd||NULL == out_cmd) {
		return HX_ERROR;
	}
	algo_p = &out_cmd->cmd_param.pub_params.algo_param;
	info = &algo_p->info;
	hx_touching.x = 0;
	hx_touching.y = 0;
	hx_touching.w = 0;
	hx_touching.finger_num = 0;
	hx_touching.old_finger = 0;
	hx_touching.loop_i = 0;

	raw_cnt_max = HX_NC_MAX_PT/4;//max point / 4
	raw_cnt_rmd = HX_NC_MAX_PT%4;

	if (raw_cnt_rmd != 0x00) //more than 4 fingers
	{
		RawDataLen = HX_RECEIVE_BUF_MAX_SIZE - ((HX_NC_MAX_PT+raw_cnt_max+3)*4) - 1;
		hx_touch_info_size = (HX_NC_MAX_PT+raw_cnt_max+2)*4;
	}

	else //less than 4 fingers
	{
		RawDataLen = HX_RECEIVE_BUF_MAX_SIZE - ((HX_NC_MAX_PT+raw_cnt_max+2)*4) - 1;
		hx_touch_info_size = (HX_NC_MAX_PT+raw_cnt_max+1)*4;
	}

	if(hx_touch_info_size > HX_RECEIVE_BUF_MAX_SIZE)
	{
		TS_LOG_ERR("%s:hx_touch_info_size larger than HX_RECEIVE_BUF_MAX_SIZE\n",__func__);
		goto err_no_reset_out;
	}

	TS_LOG_DEBUG("%s: hx_touch_info_size = %d\n",__func__, hx_touch_info_size);

	 if(atomic_read(&g_himax_nc_ts_data->suspend_mode)&&g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->chip_data->easy_wakeup_info.sleep_mode)
	  {
	  	/*increase wake_lock time to avoid system suspend.*/
		wake_lock_timeout(&g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->ts_wake_lock, TS_WAKE_LOCK_TIMEOUT);
		msleep(HX_SLEEP_200MS);
		retval = himax_read_event_stack(buf, HX_RECEIVE_BUF_MAX_SIZE);
		if(retval < 0)
		{
			TS_LOG_ERR("%s: can't read data from chip!\n", __func__);
		}
		//info->gesture_wakeup_value = himax_parse_wake_event(buf,info);
		retval = himax_parse_wake_event(buf,info);
		if(!retval)
			out_cmd->command = TS_INPUT_ALGO;

		return  retval;
	   }
	TS_LOG_DEBUG("%s: himax_parse_wake_event end\n",__func__);

#ifdef HX_TP_SYS_DIAG
	diag_cmd = hx_nc_getDiagCommand();
#ifdef HX_ESD_WORKAROUND
	if((diag_cmd) || (ESD_RESET_ACTIVATE) || (HW_NC_RESET_ACTIVATE))
#else
		if((diag_cmd) || (HW_NC_RESET_ACTIVATE))
#endif
		{
			retval = himax_read_event_stack(buf, HX_RECEIVE_BUF_MAX_SIZE);//diad cmd not 0, need to read 128.
		}
		else
		{
			if(touch_monitor_stop_flag != 0)
			{
				retval = himax_read_event_stack(buf, HX_RECEIVE_BUF_MAX_SIZE);
				touch_monitor_stop_flag-- ;
			}
			else
			{
				retval = himax_read_event_stack(buf, hx_touch_info_size);
			}
		}
	TS_LOG_DEBUG("%s: himax_read_event_stack: retval = %d \n",__func__, retval);//use for debug
	if (retval < 0)
#else
		if (himax_read_event_stack(buf, hx_touch_info_size)<0)
#endif
		{
			TS_LOG_ERR("%s: can't read data from chip!\n", __func__);
			iCount++;
			TS_LOG_ERR("%s: error count is %d !\n", __func__, iCount);
			if(iCount >= RETRY_TIMES)
			{
				iCount = 0;
				goto err_workqueue_out;
			}
			goto err_no_reset_out;
		}
		else
		{
		 	out_cmd->command = TS_INPUT_ALGO;

			if (g_himax_nc_ts_data->debug_log_level & BIT(0))
			{
				TS_LOG_INFO("%s: raw data:\n", __func__);
				himax_debug_level_print(0,0,hx_touch_info_size,hx_touching,buf);  //status uselss
			}
#ifdef HX_ESD_WORKAROUND
			check_sum_cal = himax_check_report_data_for_esd(hx_touch_info_size,buf);
#ifdef HX_TP_SYS_DIAG
			diag_cmd = hx_nc_getDiagCommand();
			if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_NC_RESET_ACTIVATE == 0
				&& diag_cmd == 0 && self_test_nc_flag == 0)  //ESD Check
#else
				if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_NC_RESET_ACTIVATE == 0
					&& self_test_nc_flag == 0)  //ESD Check
#endif
					{
						if (check_sum_cal == ESD_ALL_ZERO_BAK_VALUE)
						{
							TS_LOG_INFO("[HIMAX TP MSG]: ESD event checked - ALL Zero.\n");
							g_zero_event_count++;
							if(g_zero_event_count > 5)
								{
									ESD_HW_REST();
								}
							goto err_no_reset_out;
						}
						else if (check_sum_cal == ESD_ALL_ED_BAK_VALUE)
						{
							TS_LOG_INFO("[HIMAX TP MSG]: ESD event checked - ALL 0xED.\n");
							ESD_HW_REST();
							goto err_no_reset_out;
						}
					}
				else if (ESD_RESET_ACTIVATE)
				{
					ESD_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
					TS_LOG_INFO("[HIMAX TP MSG]:%s: Back from reset, ready to serve.\n", __func__);
					return retval;
				}
				else if (HW_NC_RESET_ACTIVATE)
#else
					if (HW_NC_RESET_ACTIVATE)
#endif
					{
						HW_NC_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
						TS_LOG_INFO("[HIMAX TP MSG]:%s: HW_RST Back from reset, ready to serve.\n", __func__);
						return retval;
					}

			check_sum_cal = himax_checksum_cal(hx_touch_info_size,hx_touching,buf);

			if (check_sum_cal != 0x00 )  //self_test_nc_flag == 1
			{
				TS_LOG_INFO("[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);
				iCount++;
				TS_LOG_ERR("%s: error count is %d !\n", __func__, iCount);
				if(iCount >= RETRY_TIMES)
				{
					iCount = 0;
					goto err_workqueue_out;
				}
				goto err_no_reset_out;
			}
		}

	/*touch monitor raw data fetch*/
#ifdef HX_TP_SYS_DIAG
	diag_cmd = hx_nc_getDiagCommand();
	if (diag_cmd >= 1 && diag_cmd <= 6)
	{
		if(himax_start_get_rawdata_from_event(hx_touch_info_size, RawDataLen, buf) == HX_ERROR)
			goto bypass_checksum_failed_packet;
	}
	else if (diag_cmd == 7)
	{
		memcpy(&(hx_nc_diag_coor[0]), &buf[0], HX_RECEIVE_BUF_MAX_SIZE);
	}

#endif
bypass_checksum_failed_packet:
	m = HX_TOUCH_INFO_POINT_CNT + 2;
	EN_NoiseFilter = ((buf[m]) >> 3);//HX_TOUCH_INFO_POINT_CNT: 52 ;
	EN_NoiseFilter = EN_NoiseFilter & 0x01;

	if (buf[HX_TOUCH_INFO_POINT_CNT] == 0xff)
		hx_real_point_num = 0;
	else
		hx_real_point_num = buf[HX_TOUCH_INFO_POINT_CNT] & 0x0f;//only use low 4 bits.

	/*Touch Point information*/
	himax_parse_coords(hx_touch_info_size,hx_real_point_num,info,hx_touching,buf);
	TS_LOG_DEBUG("%s: himax_parse_coords end \n", __func__);
	Last_EN_NoiseFilter = EN_NoiseFilter;

	iCount = 0;//I2C communication ok, checksum ok;
	return retval;

err_workqueue_out:
	TS_LOG_ERR("%s: Now reset the Touch chip.\n", __func__);

	himax_nc_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

err_no_reset_out:
	return NO_RESET_OUT;

}
static int himax_parse_specific_dts(struct himax_ts_data *ts, struct himax_i2c_platform_data *pdata)
{
	int retval = 0;
	int read_val = 0;
	int coords_size = 0;
	int length = 0;
	int i = 0;

	const char *tptesttype = NULL;
	uint32_t coords[HX_COORDS_MAX_SIZE] = {0};
	struct property *prop = NULL;
	struct device_node *dt = NULL;
	struct ts_kit_device_data *chip_data = NULL;

	if(NULL == ts || NULL == pdata) {
		return HX_ERR;
	}

	chip_data = g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->chip_data;

	/*parse IC feature*/
	dt = of_find_compatible_node(NULL, NULL, chip_data->chip_name);

	if(ts->power_support)
	{
		pdata->gpio_3v3_en = of_get_named_gpio(dt, "himax,vdd_ana-supply", 0);
		if (!gpio_is_valid(pdata->gpio_3v3_en))
		{
			TS_LOG_INFO("DT:gpio_3v3_en value is not valid\n");
		}
		pdata->gpio_1v8_en = of_get_named_gpio(dt, "himax,vcc_i2c-supply", 0);
		if (!gpio_is_valid(pdata->gpio_1v8_en))
		{
			TS_LOG_INFO("DT:pdata->gpio_1v8_en is not valid\n");
		}
		TS_LOG_INFO("DT:gpio_3v3_en=%d,gpio_1v8_en=%d\n",pdata->gpio_3v3_en,pdata->gpio_1v8_en);
	}
	if(ts->rst_support)
	{
		retval = of_property_read_u32(dt, "himax,reset_gpio", &pdata->reset_gpio);
		if (retval) {
			TS_LOG_ERR("Not define reset_gpio in Dts\n");
			return retval;
		}
		TS_LOG_INFO("get himax reset_gpio = %X\n", pdata->reset_gpio);
	}


	retval = of_property_read_u32(dt, "himax,irq_config", &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("Not define irq_config in Dts\n");
		return retval;
	}
	TS_LOG_INFO("get himax irq_config = %d\n", chip_data->irq_config);

	prop = of_find_property(dt, "himax,id-name", NULL);
	if (prop) {
		length = prop->length /((int) sizeof(uint32_t));
		TS_LOG_DEBUG("%s:id-name length = %d", __func__, length);
	}

	if (of_property_read_u32_array(dt, "himax,id-name", hx_id_name, length) == NO_ERR) {
		//ts->hx_id_name = hx_id_name;
		TS_LOG_INFO("DT-%s:id-name = %2X, %2X, %2X\n", __func__, hx_id_name[0],
				hx_id_name[1], hx_id_name[2]);
	}
	else
	{
		TS_LOG_ERR("Not define himax,id-name\n");
		return HX_ERR;
	}

	prop = of_find_property(dt, "himax,id-addr", NULL);
	if (prop) {
		length = prop->length /((int) sizeof(uint32_t));
		TS_LOG_DEBUG("%s:id-addr length = %d", __func__, length);
	}

	if (of_property_read_u32_array(dt, "himax,id-addr", hx_id_addr,length) == NO_ERR) {
		//ts->hx_id_addr = hx_id_addr;
		TS_LOG_INFO("DT-%s:id-addr = %2X, %2X, %2X, %2X\n", __func__, hx_id_addr[0],
				hx_id_addr[1], hx_id_addr[2], hx_id_addr[3]);
	}
	else
	{
		TS_LOG_ERR("Not define himax,id-addr\n");
		return HX_ERR;
	}


	prop = of_find_property(dt, "himax,flash-addr", NULL);
	if (prop) {
		length = prop->length /((int) sizeof(uint32_t));
		TS_LOG_DEBUG("%s:flash-addr length = %d", __func__, length);
	}

	if (of_property_read_u32_array(dt, "himax,flash-addr", hx_flash_addr,length) == NO_ERR) {
		for(i = 0; i < FLASH_ADDR_LEN; i += 7)
			TS_LOG_INFO("DT-%s:flash-addr = %d, %d, %d, %d, %d, %d,%d\n", __func__,
				hx_flash_addr[i],hx_flash_addr[i + 1], hx_flash_addr[i +2],hx_flash_addr[i +3],
				hx_flash_addr[i +4], hx_flash_addr[i +5],hx_flash_addr[i +6]);
	}
	else
	{
		TS_LOG_ERR("Not define flash-addr\n");
		return HX_ERR;
	}

	retval = of_property_read_u32(dt, "himax,p2p-test-en", &read_val);
	if (retval) {
		ts->p2p_test_sel= 0;
		TS_LOG_INFO("get device p2p_test_sel not exit,use default value.\n");
	}else {
		ts->p2p_test_sel = read_val;
		TS_LOG_INFO("get device p2p_test_sel:%d\n", read_val);
	}

	return NO_ERR;
}

static int himax_parse_project_dts(struct himax_ts_data *ts, struct himax_i2c_platform_data *pdata)
{
	int retval = 0;
	int read_val = 0;
	int coords_size = 0;
	int length = 0;
	int i = 0;


	const char *tptesttype = NULL;
	uint32_t coords[HX_COORDS_MAX_SIZE] = {0};
	const char *modulename = NULL;
	struct property *prop = NULL;
	struct device_node *dt = NULL;
	struct ts_kit_device_data *chip_data = NULL;

	if(NULL == ts || NULL == pdata) {
		return HX_ERR;
	}

	chip_data = g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->chip_data;

	/*parse module feature*/
	dt = of_find_compatible_node(NULL, NULL, himax_nc_project_id);

	retval = of_property_read_string(dt, "module",&modulename);

	if (retval) {
		//strncpy(chip_data->module_name, MODULE_NAME,strlen(MODULE_NAME)+1);
		TS_LOG_INFO("Not define module in Dts!\n");
	}
	else{
		strncpy(chip_data->module_name, modulename,strlen(modulename)+1);
	}
	TS_LOG_INFO("module_name: %s\n", chip_data->module_name);

	retval =of_property_read_string(dt, "tp_test_type", &tptesttype);
	if (retval) {
		TS_LOG_INFO("Not define device tp_test_type in Dts, use default\n");
		strncpy(chip_data->tp_test_type, "Normalize_type:judge_last_result", TS_CAP_TEST_TYPE_LEN);
	}
	else {
		snprintf(chip_data->tp_test_type, TS_CAP_TEST_TYPE_LEN, "%s", tptesttype);
	}
	TS_LOG_INFO("get tp test type = %s\n", chip_data->tp_test_type);

	prop = of_find_property(dt, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid panel coords size %d", __func__, coords_size);
	}

	if (of_property_read_u32_array(dt, "himax,panel-coords", coords, coords_size) == NO_ERR) {
		pdata->abs_x_min = coords[0], pdata->abs_x_max = coords[1];
		pdata->abs_y_min = coords[2], pdata->abs_y_max = coords[3];
		TS_LOG_INFO("DT-%s:panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
				pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	}
	else
	{
		pdata->abs_x_max = ABS_X_MAX_DEFAULT;
		pdata->abs_y_max = ABS_Y_MAX_DEFAULT;
	}

	prop = of_find_property(dt, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid display coords size %d", __func__, coords_size);
	}

	retval = of_property_read_u32_array(dt, "himax,display-coords", coords, coords_size);
	if (retval) {
		TS_LOG_DEBUG("%s:Fail to read display-coords %d\n", __func__, retval);
		return retval;
	}
	pdata->screenWidth  = coords[1];
	pdata->screenHeight = coords[3];

	TS_LOG_INFO("DT-%s:display-coords = (%d, %d)", __func__, pdata->screenWidth,
		pdata->screenHeight);

	retval = of_property_read_u32(dt, "himax,rawdata_timeout", &chip_data->rawdata_get_timeout);
	if (retval) {
		chip_data->rawdata_get_timeout = RAWDATA_GET_TIME_DEFAULT;
		TS_LOG_INFO("Not define chip rawdata limit time in Dts, use default\n");
	}
	TS_LOG_INFO("get chip rawdata limit time = %d\n", chip_data->rawdata_get_timeout);


	retval = of_property_read_u32(dt, "himax,trx_delta_test_support", &chip_data->trx_delta_test_support);
	if (retval) {
		TS_LOG_INFO("get device trx_delta_test_support null, use default\n");
		chip_data->trx_delta_test_support = 0;
		retval = 0;
	}
	else
	{
		TS_LOG_INFO("get device trx_delta_test_support = %d\n", chip_data->trx_delta_test_support);
	}

	retval = of_property_read_u32(dt, TEST_CAPACITANCE_VIA_CSVFILE, &read_val);
	if (retval) {
	ts->test_capacitance_via_csvfile = false;
	TS_LOG_INFO("get device test_capacitance_via_csvfile not exit,use default value.\n");
	}else {
	ts->test_capacitance_via_csvfile = read_val;
	TS_LOG_INFO("get device test_capacitance_via_csvfile:%d\n", read_val);
	}

	retval = of_property_read_u32(dt, CSVFILE_USE_SYSTEM_TYPE, &read_val);
	if (retval) {
		ts->csvfile_use_system = false;
		TS_LOG_INFO("get device csvfile_use_system not exit,use default value.\n");
	}else {
		ts->csvfile_use_system = read_val;
		TS_LOG_INFO("get device csvfile_use_system:%d\n", read_val);
	}

	return NO_ERR;
}

static int himax_parse_dts(struct himax_ts_data *ts, struct ts_kit_device_data *chip_data)
{
	int read_val = 0;
	int retval = NO_ERR;
	int coords_size = 0;
	uint32_t coords[HX_COORDS_MAX_SIZE] = {0};
	const char *projectid = NULL;
	const char *chipname = NULL;
	struct property *prop = NULL;
	struct device_node *device = NULL;

	device = ts->ts_dev->dev.of_node;

	TS_LOG_INFO("%s: parameter init begin\n", __func__);
	if(NULL == device||NULL == chip_data) {
		return HX_ERR;
	}

	retval = of_property_read_u32(device, "reg", &chip_data->slave_addr);
	if (retval) {
		chip_data->ts_platform_data->client->addr = SLAVE_I2C_ADRR;
		chip_data->slave_addr = SLAVE_I2C_ADRR;
		TS_LOG_INFO("Not define reg in Dts, use default\n");
	}
	chip_data->ts_platform_data->client->addr=(uint8_t)chip_data->slave_addr;
	TS_LOG_INFO("get himax reg = 0x%02x\n", chip_data->ts_platform_data->client->addr);

	retval =of_property_read_u32(device, "ic_type",&chip_data->ic_type);
	if (retval) {
		chip_data->ic_type = ONCELL;
		TS_LOG_ERR("Not define device ic_type in Dts\n");
	} else {
		g_tskit_ic_type = chip_data->ic_type;
	}
	TS_LOG_INFO("get g_tskit_ic_type = %d.\n", g_tskit_ic_type);

	retval =of_property_read_string(device, "chip_name", &chipname);
	if (retval) {
		//strncpy(chip_data->chip_name,STR_IC_NAME,strlen(STR_IC_NAME)+1);
		TS_LOG_INFO("Not define module in Dts!\n");
	}else{
		strncpy(chip_data->chip_name, chipname, CHIP_NAME_LEN);
	}
	TS_LOG_INFO("get himax_chipname = %s\n",chip_data->chip_name);

	retval = of_property_read_u32(device, "himax,power_support", &ts->power_support);
	if (retval) {
		ts->power_support= NOT_SUPPORT;
		TS_LOG_INFO("NOT support to parse the power dts!\n");
	}
	TS_LOG_INFO("himax,power_support = %d\n", ts->power_support);

	retval = of_property_read_u32(device, "himax,rst_support", &ts->rst_support);
	if (retval) {
		ts->rst_support= NOT_SUPPORT;
		TS_LOG_INFO("NOT support to parse the rst dts!\n");
	}
	TS_LOG_INFO("himax,rst_support = %d\n", ts->rst_support);

	retval = of_property_read_u32(device, "himax,re_send_cmd_support", &ts->re_send_cmd_support);
	if (retval) {
		ts->re_send_cmd_support = NOT_SUPPORT;
		TS_LOG_INFO("NOT support to parse the re_send_cmd dts!\n");
	}
	TS_LOG_INFO("himax,re_send_cmd_support = %d\n", ts->re_send_cmd_support);

	retval = of_property_read_u32(device, "himax,glove_supported", &chip_data->ts_platform_data->feature_info.glove_info.glove_supported);
	if (retval) {
		chip_data->ts_platform_data->feature_info.glove_info.glove_supported = NOT_SUPPORT;
		TS_LOG_INFO("NOT support to parse the glove dts!\n");
	}
	TS_LOG_INFO("himax,glove_support = %d\n", chip_data->ts_platform_data->feature_info.glove_info.glove_supported);

	retval = of_property_read_u32(device, "himax,gesture_supported", &chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value);
	if (retval) {
		chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value = NOT_SUPPORT;
		TS_LOG_INFO("NOT support to parse the gesture dts!\n");
	}
	TS_LOG_INFO("himax,gesture_supported = %d\n", chip_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value);

	prop = of_find_property(device, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid panel coords size %d", __func__, coords_size);
	}

	if (of_property_read_u32_array(device, "himax,panel-coords", coords, coords_size) == NO_ERR) {
		ts->pdata->abs_x_min = coords[0], ts->pdata->abs_x_max = coords[1];
		ts->pdata->abs_y_min = coords[2], ts->pdata->abs_y_max = coords[3];
		TS_LOG_INFO("DT-%s:panel-coords = %d, %d, %d, %d\n", __func__, ts->pdata->abs_x_min,
				ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
	}
	else
	{
		ts->pdata->abs_x_max = ABS_X_MAX_DEFAULT;
		ts->pdata->abs_y_max = ABS_Y_MAX_DEFAULT;
	}
	prop = of_find_property(device, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid display coords size %d", __func__, coords_size);
	}

	retval = of_property_read_u32_array(device, "himax,display-coords", coords, coords_size);
	if (retval) {
		TS_LOG_DEBUG("%s:Fail to read display-coords %d\n", __func__, retval);
		return retval;
	}
	ts->pdata->screenWidth  = coords[1];
	ts->pdata->screenHeight = coords[3];

	TS_LOG_INFO("DT-%s:display-coords = %d, %d", __func__, ts->pdata->screenWidth,
		ts->pdata->screenHeight);

	return NO_ERR;

}
static int himax_chip_detect(struct ts_kit_platform_data *platform_data)
{
	int err = NO_ERR;
	struct himax_ts_data *ts = NULL;
	struct himax_i2c_platform_data *pdata = NULL;
	TS_LOG_INFO("%s:called\n", __func__);

	if (!platform_data){
		TS_LOG_ERR("device, ts_kit_platform_data *platform_data or platform_data->ts_dev is NULL \n");
		err =  -EINVAL;
		goto out;
	}

	g_himax_nc_ts_data->ts_dev = platform_data->ts_dev;
	g_himax_nc_ts_data->ts_dev->dev.of_node = g_himax_nc_ts_data->tskit_himax_data->cnode ;
	g_himax_nc_ts_data->tskit_himax_data->ts_platform_data = platform_data;
	g_himax_nc_ts_data->tskit_himax_data->is_in_cell = false;
	g_himax_nc_ts_data->tskit_himax_data->is_i2c_one_byte = 0;
	g_himax_nc_ts_data->tskit_himax_data->is_new_oem_structure= 0;
	g_himax_nc_ts_data->dev = &(platform_data->client->dev);
	g_himax_nc_ts_data->firmware_updating = false;
	ts = g_himax_nc_ts_data;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		err = -ENOMEM;
		goto err_alloc_platform_data_fail;
	}
	ts->pdata = pdata;
	err = himax_parse_dts(ts,platform_data->chip_data);
	if(err)
	{
		TS_LOG_ERR("himax_parse_dts err:%d\n",err);
	}

	if (ts->ts_dev->dev.of_node) { /*DeviceTree Init Platform_data*/
		err = himax_parse_specific_dts(ts, pdata);
		if (err < 0) {
			TS_LOG_INFO(" %s himax_parse_specific_dts failed \n",__func__);
			goto err_parse_pdata_failed;
		}
	}
	pdata->gpio_reset = ts->tskit_himax_data->ts_platform_data->reset_gpio;
	TS_LOG_INFO("pdata->gpio_reset:%d\n",pdata->gpio_reset);
	pdata->gpio_irq = ts->tskit_himax_data->ts_platform_data->irq_gpio;
	TS_LOG_INFO("pdata->gpio_irq:%d\n",pdata->gpio_irq);
	ts->rst_gpio = pdata->gpio_reset;
	//config rst gpio in kernel and keep high
	err = gpio_direction_output(ts->rst_gpio, 1);
	if (err) {
		TS_LOG_ERR("%s:gpio direction output to 1 fail, err=%d\n",
				   __func__, err);
		return err;
	}
	err = himax_reset_device();
	if(err<0)
	{
		TS_LOG_ERR("himax_reset_device error\n");
		goto  err_ic_package_failed;
	}

	if (himax_ic_package_check() == IC_PACK_CHECK_FAIL) {
		TS_LOG_ERR("Himax chip does NOT EXIST");
		err = -ENOMEM;
		goto err_ic_package_failed;
	}
	//msleep();
	himax_read_project_id();

	if (ts->ts_dev->dev.of_node) { /*DeviceTree Init Platform_data*/
		err = himax_parse_project_dts(ts, pdata);
		if (err < 0) {
			TS_LOG_INFO(" %s himax_parse_project_dts failed \n",__func__);
			goto err_parse_project_dts_failed;
		}
	}
	return NO_ERR;

err_parse_project_dts_failed:

err_ic_package_failed:
err_dsm_register_failed:
err_parse_pdata_failed:
	kfree(pdata);
	pdata = NULL;
err_alloc_platform_data_fail:
out:
	TS_LOG_ERR("detect himax error\n");
	return err;
}

static int himax_init_chip(void)
{
	int err = NO_ERR;
	struct himax_ts_data *ts = NULL;

	TS_LOG_INFO("%s:called\n", __func__);

	ts = g_himax_nc_ts_data;

#ifdef  HX_TP_SYS_FLASH_DUMP
	ts->flash_wq = create_singlethread_workqueue("himax_flash_wq");
	if (!ts->flash_wq)
	{
		TS_LOG_ERR("%s: create flash workqueue failed\n", __func__);
		err = -ENOMEM;
		goto err_create_flash_wq_failed;
	}
	INIT_WORK(&ts->flash_work, himax_nc_ts_flash_work_func);
	hx_nc_setSysOperation(0);
	hx_nc_setFlashBuffer();
#endif

	himax_nc_read_TP_info();

	if (himax_nc_loadSensorConfig() < 0) {
		TS_LOG_ERR("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		err = -ENOMEM;
		goto err_detect_failed;
	}

	calculate_point_number();

	mutex_init(&wrong_touch_lock);

#ifdef HX_TP_SYS_DIAG
	hx_nc_setXChannel(HX_NC_RX_NUM); // X channel
	hx_nc_setYChannel(HX_NC_TX_NUM); // Y channel
	hx_nc_setMutualBuffer();
	if (hx_nc_getMutualBuffer() == NULL) {
		TS_LOG_ERR("%s: mutual buffer allocate fail failed\n", __func__);
		goto err_getMutualBuffer_failed;
	}
	hx_nc_setMutualNewBuffer();
	if (hx_nc_getMutualNewBuffer() == NULL) {
		TS_LOG_ERR("%s: New mutual buffer allocate fail failed\n", __func__);
		goto err_getMutualNewBuffer_failed;
	}
	hx_nc_setMutualOldBuffer();
	if (hx_nc_getMutualOldBuffer() == NULL) {
		TS_LOG_ERR("%s: Old mutual buffer allocate fail failed\n", __func__);
		goto err_getMutualOldBuffer_failed;
	}
	hx_nc_setSelfBuffer();
	if (hx_nc_getSelfBuffer() == NULL) {
		TS_LOG_ERR("%s: Old mutual buffer allocate fail failed\n", __func__);
		goto err_geSelf_failed;
	}
#endif

	ts->x_channel = HX_NC_RX_NUM;
	ts->y_channel = HX_NC_TX_NUM;
	ts->nFinger_support = HX_NC_MAX_PT;
	/*calculate the i2c data size*/
	calcDataSize(ts->nFinger_support);
	TS_LOG_INFO("%s: calcDataSize complete\n", __func__);
	ts->pdata->abs_pressure_min = 0;
	ts->pdata->abs_pressure_max = 200;
	ts->pdata->abs_width_min	= 0;
	ts->pdata->abs_width_max	= 200;
	ts->suspended	= false;

	atomic_set(&ts->suspend_mode, 0);

#ifdef HX_ESD_WORKAROUND
	ESD_RESET_ACTIVATE = 0;
#endif
	HW_NC_RESET_ACTIVATE = 0;

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
		himax_nc_touch_sysfs_init();
#endif
	TS_LOG_INFO("%s:sucess\n", __func__);
	return NO_ERR;

#ifdef  HX_TP_SYS_DIAG
err_create_wq_failed:
#endif
//#ifdef HX_RESEND_CMD
err_resend_cmd_wq_failed:
//#endif
#ifdef HX_TP_SYS_DIAG
	hx_nc_freeSelfBuffer();
err_geSelf_failed:
	hx_nc_freeMutualOldBuffer();
err_getMutualOldBuffer_failed:
	hx_nc_freeMutualNewBuffer();
err_getMutualNewBuffer_failed:
	hx_nc_freeMutualBuffer();
err_getMutualBuffer_failed:
#endif
err_detect_failed:

#ifdef  HX_TP_SYS_FLASH_DUMP
	hx_nc_freeFlashBuffer();
	destroy_workqueue(ts->flash_wq);
err_create_flash_wq_failed:
#endif
	return err;

}
static int himax_enter_sleep_mode(void)
{
	TS_LOG_INFO("%s: enter \n", __func__);
	TS_LOG_INFO("%s: exit \n", __func__);
	return NO_ERR;
}
static int himax_exit_sleep_mode(void)
{
	TS_LOG_INFO("%s: enter \n", __func__);
	TS_LOG_INFO("%s: exit \n", __func__);
	return NO_ERR;
}
static int himax_core_suspend(void)
{
	int retval = 0;
	struct himax_ts_data *ts = NULL;
	TS_LOG_INFO("%s: Enter suspended. \n", __func__);

	ts = g_himax_nc_ts_data;

	if(ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(atomic_read(&hmx_nc_mmi_test_status)) {
		TS_LOG_INFO("%s: tp fw is hmx_nc_mmi_test_status, return\n", __func__);
		return NO_ERR;
	}
	if(ts->suspended)
	{
		TS_LOG_INFO("%s: Already suspended. Skipped. \n", __func__);
		return SUSPEND_IN;
	}
	else
	{
		ts->suspended = true;
		TS_LOG_INFO("%s: enter \n", __func__);
	}

#ifdef HX_TP_SYS_FLASH_DUMP
	if (hx_nc_getFlashDumpGoing())
	{
		TS_LOG_INFO("[himax] %s: Flash dump is going, reject suspend\n",__func__);
		return SUSPEND_REJECT;
	}
#endif

	ts->first_pressed = 0;
	atomic_set(&ts->suspend_mode, 1);
	ts->pre_finger_mask = 0;
	if(true == ts->tskit_himax_data->ts_platform_data->feature_info.wakeup_gesture_enable_info.switch_value){
		if(ts->tskit_himax_data->ts_platform_data->chip_data->easy_wakeup_info.sleep_mode)
		{
			himax_set_smwp_enable(SMWP_ON);

			mutex_lock(&wrong_touch_lock);
			g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.off_motion_on = true;
			mutex_unlock(&wrong_touch_lock);
			TS_LOG_INFO("ENABLE gesture mode\n");
		}
		else{
			himax_set_smwp_enable(SMWP_OFF);
			retval = himax_enter_sleep_mode();
			if(retval<0){
			   TS_LOG_ERR("[himax] %s: himax_enter_sleep_mode fail!\n", __func__);
			   return retval;
			}
		}
	}

	TS_LOG_INFO("%s: exit \n", __func__);

	return NO_ERR;
}
static int himax_core_resume(void)
{
	struct himax_ts_data *ts;
	int retval=0;

	TS_LOG_INFO("%s: enter \n", __func__);

	ts = g_himax_nc_ts_data;
	struct ts_feature_info *info = &ts->tskit_himax_data->ts_platform_data->feature_info;

	if(ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(atomic_read(&hmx_nc_mmi_test_status)) {
		TS_LOG_INFO("%s: tp fw is hmx_nc_mmi_test_status, return\n", __func__);
		return NO_ERR;
	}

	if(ts->suspended)
	{
		TS_LOG_INFO("%s: will be resume \n", __func__);
	}
	else
	{
		TS_LOG_INFO("%s: Already resumed. Skipped. \n", __func__);
		return RESUME_IN;
	}

	if(ts->tskit_himax_data->ts_platform_data->chip_data->easy_wakeup_info.sleep_mode)
	{
		mutex_lock(&wrong_touch_lock);
		g_himax_nc_ts_data->tskit_himax_data->easy_wakeup_info.off_motion_on = false;
		mutex_unlock(&wrong_touch_lock);
		msleep(HX_SLEEP_5MS);
	}

	if(info->glove_info.glove_supported){
		retval = himax_set_glove_switch(info->glove_info.glove_switch);
		if (retval < 0) {
			TS_LOG_ERR("Failed to set glove switch(%d), err: %d\n",
				   info->glove_info.glove_switch, retval);
			return retval;
		}
	}

	retval = himax_exit_sleep_mode();
	if(retval < 0){
		TS_LOG_ERR("[himax] %s: himax_exit_sleep_mode fail!\n", __func__);
		return retval;
	}
	TS_LOG_INFO("%s: power on. \n", __func__);
	HW_NC_RESET_ACTIVATE = HW_RST_FLAT_ENABLE;
	atomic_set(&ts->suspend_mode, 0);

	ts->suspended = false;

	TS_LOG_INFO("%s: exit \n", __func__);

	return NO_ERR;
}

static void himax_power_off_gpio_set(void)
{

	TS_LOG_INFO("%s:enter\n", __func__);

	if (g_himax_nc_ts_data->pdata->gpio_reset >= 0) {
		gpio_free(g_himax_nc_ts_data->pdata->gpio_reset);
	}
	if (g_himax_nc_ts_data->pdata->gpio_3v3_en >= 0) {
		gpio_free(g_himax_nc_ts_data->pdata->gpio_3v3_en);
	}
	if (g_himax_nc_ts_data->pdata->gpio_1v8_en >= 0) {
		gpio_free(g_himax_nc_ts_data->pdata->gpio_1v8_en);
	}
	if (gpio_is_valid(g_himax_nc_ts_data->pdata->gpio_irq)) {
		gpio_free(g_himax_nc_ts_data->pdata->gpio_irq);
	}
	TS_LOG_INFO("%s:exit\n", __func__);

}

static int himax_power_off(void)
{
	int err = 0;
	TS_LOG_INFO("%s:enter\n", __func__);

	if (g_himax_nc_ts_data->pdata->gpio_3v3_en >= 0) {
		err = gpio_direction_output(g_himax_nc_ts_data->pdata->gpio_3v3_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				g_himax_nc_ts_data->pdata->gpio_3v3_en);
			return err;
		}
	}
	if (g_himax_nc_ts_data->pdata->gpio_1v8_en >= 0) {
		err = gpio_direction_output(g_himax_nc_ts_data->pdata->gpio_1v8_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				g_himax_nc_ts_data->pdata->gpio_1v8_en);
			return err;
		}
	}

	himax_power_off_gpio_set();
	return err;
}

static void himax_shutdown(void)
{
	TS_LOG_INFO("%s himax_shutdown call power off\n",__func__);
	himax_power_off();
	return;
}

static int himax_algo_cp(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info)
{
	int index = 0;
	int id = 0;
	TS_LOG_INFO("%s Enter",__func__);
	if((NULL == dev_data)||(NULL == in_info)||(NULL == out_info)) {
		return HX_ERROR;
	}
	for (index = 0, id = 0; index < TS_MAX_FINGER; index++, id++)
	{
		if (in_info->cur_finger_number == 0)
		{
			out_info->fingers[0].status = TS_FINGER_RELEASE;
			if (id >= 1)
				out_info->fingers[id].status = 0;
		}
		else
		{
			if (in_info->fingers[index].x != 0 ||in_info->fingers[index].y != 0)
			{
				if (HIMAX_EV_TOUCHDOWN == in_info->fingers[index].event
				    || HIMAX_EV_MOVE == in_info->fingers[index].event
				    || HIMAX_EV_NO_EVENT == in_info->fingers[index].event)
				{
					out_info->fingers[id].x = in_info->fingers[index].x;
					out_info->fingers[id].y = in_info->fingers[index].y;
					out_info->fingers[id].pressure = in_info->fingers[index].pressure;
					out_info->fingers[id].status = TS_FINGER_PRESS;
				}
				else if (HIMAX_EV_LIFTOFF == in_info->fingers[index].event)
				{
					out_info->fingers[id].status = TS_FINGER_RELEASE;
				}
				else
				{
					TS_LOG_INFO("%s  Nothing to do.",__func__);
				}
			}
			else
				out_info->fingers[id].status = 0;
		}
	}
	return NO_ERR;
}

static struct ts_algo_func himax_algo_f1 = {
	.algo_name = "himax_algo_cp",
	.chip_algo_func = himax_algo_cp,
};
static int himax_register_algo(struct ts_kit_device_data *chip_data)
{
	int retval =  -EIO;
	TS_LOG_INFO("%s: himax_reg_algo called\n", __func__);
	if(NULL == chip_data) {
		return retval;
	}
	retval = register_ts_algo_func(chip_data, &himax_algo_f1);

	return retval;
}
static int Himax_chip_check_status(void){
	TS_LOG_DEBUG("%s +\n", __func__);
	TS_LOG_DEBUG("%s -\n", __func__);
	return 0;
}

static int himax_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s Enter\n", __func__);
	if(NULL == info) {
		return HX_ERROR;
	}
#if 0
	if (himax_read_projectid() < 0) {
			TS_LOG_ERR("%s read project id error!\n", __func__);
		}
#endif
		snprintf(info->ic_vendor, HX_PROJECT_ID_LEN + 7, "himax-%s", himax_nc_project_id);
		snprintf(info->mod_vendor, MAX_STR_LEN, g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->chip_data->module_name);
		snprintf(info->fw_vendor, FW_VENDOR_MAX_STR_LEN, "%x.%x.%x.%x.%x",
		         g_himax_nc_ts_data->vendor_panel_ver,
		         g_himax_nc_ts_data->vendor_fw_ver,
		         g_himax_nc_ts_data->vendor_config_ver,
		         g_himax_nc_ts_data->vendor_cid_maj_ver,
	             g_himax_nc_ts_data->vendor_cid_min_ver);
    return retval;
}
bool himax_nc_sense_off(void)
{
	uint8_t cnt = 0;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	do
	{
		/* change for hx83112 start*/
		if (HX_83112A_SERIES_PWON==IC_NC_TYPE)	{
			//===========================================
			//  0x31 ==> 0x27
			//===========================================
			tmp_data[0] = DATA_SENSE_SWITCH_1_OFF;
			if (i2c_himax_nc_write(ADDR_SENSE_SWITCH_1, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
				TS_LOG_ERR("%s: i2c access fail!\n", __func__);
				return false;
			}
			//===========================================
			//  0x32 ==> 0x95
			//===========================================
			tmp_data[0] = DATA_SENSE_SWITCH_2_OFF;
			if (i2c_himax_nc_write(ADDR_SENSE_SWITCH_2, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
				TS_LOG_ERR("%s: i2c access fail!\n", __func__);
				return false;
			}

			//===========================================
			//  0x31 ==> 0x00
			//===========================================
			tmp_data[0] = RESERVED_VALUE;
			if (i2c_himax_nc_write(ADDR_SENSE_SWITCH_1, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
				TS_LOG_ERR("%s: i2c access fail!\n", __func__);
				return false;
			}
		}
		/* change for hx83112 end*/

		//===========================================
		//  0x31 ==> 0x27
		//===========================================
		tmp_data[0] = DATA_SENSE_SWITCH_1_OFF;
		if (i2c_himax_nc_write(ADDR_SENSE_SWITCH_1, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return false;
		}
		//===========================================
		//  0x32 ==> 0x95
		//===========================================
		tmp_data[0] = DATA_SENSE_SWITCH_2_OFF;
		if (i2c_himax_nc_write(ADDR_SENSE_SWITCH_2, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return false;
		}

		// ======================
		// Check enter_save_mode
		// ======================

		himax_rw_reg_reformat(ADDR_READ_MODE_CHK,tmp_addr);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		TS_LOG_INFO("%s: Check enter_save_mode data[0]=%X \n", __func__, tmp_data[0]);

		if (tmp_data[0] == ENTER_SAVE_MODE) {
			//=====================================
			// Reset TCON
			//=====================================

			himax_rw_reg_reformat_com(ADDR_RESET_TCON,DATA_INIT,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);
			msleep(HX_SLEEP_1MS);
			himax_rw_reg_reformat_com(ADDR_RESET_TCON,DATA_RESET_TCON,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);
			return true;
		}
		else
			msleep(HX_SLEEP_10MS);
	} while (cnt++ < 15);

	return false;
}
void himax_nc_sense_on(uint8_t FlashMode)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	int retry = 0;

	TS_LOG_INFO("Enter %s  \n", __func__);

	himax_nc_interface_on();

	do
	{
		himax_rw_reg_reformat_com(ADDR_SENSE_ON,DATA_SENSE_ON,tmp_addr,tmp_data);
		himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);

		tmp_addr[0] = 0xE4;
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);

		TS_LOG_INFO("%s:Read status from IC = %X,%X\n", __func__, tmp_data[0],tmp_data[1]);

	}while((tmp_data[1] != 0x01 || tmp_data[0] != 0x00) && retry++ < 5);

	if(retry >= 5)
	{
		TS_LOG_ERR("%s: Fail:\n", __func__);
		//===AHBI2C_SystemReset==========
		himax_rw_reg_reformat_com(ADDR_AHBI2C_SYSRST,DATA_AHBI2C_SYSRST,tmp_addr,tmp_data);
		himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);
	}
	else
	{
		TS_LOG_INFO("%s:OK and Read status from IC = %X,%X\n", __func__, tmp_data[0],tmp_data[1]);

		/* reset code*/
		tmp_data[0] = DATA_SENSE_SWITCH_ON;
		if ( i2c_himax_nc_write(ADDR_SENSE_SWITCH_1 ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0){
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		}
		if ( i2c_himax_nc_write(ADDR_SENSE_SWITCH_2 ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0){
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		}
		himax_rw_reg_reformat_com(ADDR_SENSE_ON,DATA_INIT,tmp_addr,tmp_data);
		himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);
	}

}

int himax_read_project_id(void)
{
	uint8_t *flash_tmp_buffer 	= NULL;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t buffer[256] = {0};
	uint32_t project_id_addr = 0x10000;/*60k*/
	uint32_t project_id_len = HX_PROJECT_ID_LEN;
	uint32_t flash_page_len = 0x1000;/*4k*/
	uint32_t temp_addr = 0;
	int cnt = 0, i = 0;

	flash_tmp_buffer = kzalloc(flash_page_len * sizeof(uint8_t), GFP_KERNEL);
	if (!flash_tmp_buffer)
		return HX_ERR;
	himax_nc_sense_off();
	himax_burst_enable(0);

	do
	{
		//===========================================
		// AHB address auto +4		: 0x0D ==> 0x11
		// Do not AHB address auto +4 : 0x0D ==> 0x10
		//===========================================
		tmp_data[0] = DATA_AHB_AUTO;
		if ( i2c_himax_nc_write(ADDR_AHB ,tmp_data, ONE_BYTE_CMD,sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0)
		{
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return I2C_FAIL;
		}
		// Check cmd

		i2c_himax_nc_read(ADDR_AHB, tmp_data,ONE_BYTE_CMD,sizeof(tmp_data),DEFAULT_RETRY_CNT);

		if (tmp_data[0] == DATA_AHB_AUTO)
		{
			break;
		}
		msleep(HX_SLEEP_1MS);
	} while (++cnt < 10);

	if (cnt > 0)
		TS_LOG_INFO("%s:Polling auto+4 mode: %d times", __func__,cnt);

	for (temp_addr = project_id_addr; temp_addr <project_id_addr + flash_page_len; temp_addr = temp_addr + NOR_READ_LENTH)
	{

		tmp_addr[0] = temp_addr % 0x100;
		tmp_addr[1] = (temp_addr >> 8) % 0x100;
		tmp_addr[2] = (temp_addr >> 16) % 0x100;
		tmp_addr[3] = temp_addr / 0x1000000;
		himax_nc_register_read(tmp_addr ,NOR_READ_LENTH,buffer);
		memcpy(&flash_tmp_buffer[temp_addr - project_id_addr],buffer,NOR_READ_LENTH);
	}
	for(i = 8; i <= project_id_len; i++)
		{
			TS_LOG_INFO("flash_buf[%d]=%x\n", i, flash_tmp_buffer[i]);
		}

	do
		{
		//===========================================
		// AHB address auto +4		: 0x0D ==> 0x11
		// Do not AHB address auto +4 : 0x0D ==> 0x10
		//===========================================
		tmp_data[0] = (DATA_AHB);
		if ( i2c_himax_nc_write(ADDR_AHB ,tmp_data, ONE_BYTE_CMD,sizeof(tmp_data),  DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return I2C_FAIL;
		}
		// Check cmd
		i2c_himax_nc_read(ADDR_AHB, tmp_data,ONE_BYTE_CMD,sizeof(tmp_data),DEFAULT_RETRY_CNT);

		if (tmp_data[0] == DATA_AHB)
		{
			break;
		}
		msleep(HX_SLEEP_1MS);
	} while (++cnt < 10);
	himax_nc_sense_on(0x01);

	TS_LOG_INFO("%s:project_id_len: %d\n", __func__,project_id_len);
#if 0
//debug log print
	for(i = 1; i <= project_id_len; i++)
	{
		TS_LOG_INFO("flash_buf[%d]=%x\n", i, flash_tmp_buffer[i]);
	}
//debug log print
#endif
	if((flash_tmp_buffer[8] != 0x00) && (flash_tmp_buffer[9] != 0x00) && (flash_tmp_buffer[10] != 0x00) && (flash_tmp_buffer[11] != 0x00)
		&&(flash_tmp_buffer[8] != 0xFF) && (flash_tmp_buffer[9] != 0xFF) && (flash_tmp_buffer[10] != 0xFF) && (flash_tmp_buffer[11] != 0xFF))
	strncpy(himax_nc_project_id, flash_tmp_buffer + 8, HX_PROJECT_ID_LEN); /*get project id from flash*/

    TS_LOG_INFO("%s:project id : %s\n", __func__, himax_nc_project_id);

	if(NULL!=flash_tmp_buffer)
	{
		kfree(flash_tmp_buffer);
		flash_tmp_buffer =NULL;
	}
	return NO_ERR;
}

void himax_nc_interface_on(void)
{
	int cnt = 0;
	uint8_t tmp_data[5] = {0};
	uint8_t tmp_buf[2] = {0};

	if (i2c_himax_nc_read(DUMMY_REGISTER, tmp_data, FOUR_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {	//Read a dummy register to wake up I2C.
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}

	do
	{
		//===========================================
		// Enable continuous burst mode : 0x13 ==> 0x31
		//===========================================
		tmp_data[0] = DATA_EN_BURST_MODE;
		if (i2c_himax_nc_write(ADDR_EN_BURST_MODE, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return;
		}
		//===========================================
		// AHB address auto +4		: 0x0D ==> 0x11
		// Do not AHB address auto +4 : 0x0D ==> 0x10
		//===========================================
		tmp_data[0] = DATA_AHB;
		if (i2c_himax_nc_write(ADDR_AHB ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return;
		}

		// Check cmd
		i2c_himax_nc_read(ADDR_EN_BURST_MODE, tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT);
		i2c_himax_nc_read(ADDR_AHB, tmp_buf, ONE_BYTE_CMD, sizeof(tmp_buf), DEFAULT_RETRY_CNT);

		if (tmp_data[0] == DATA_EN_BURST_MODE && tmp_buf[0] == DATA_AHB) {
			break;
		}
		msleep(HX_SLEEP_1MS);
	} while (++cnt < 10);

	if (cnt > 0)
		TS_LOG_INFO("%s:Polling burst mode: %d times", __func__,cnt);
}
static void himax_init_psl(void) //power saving level
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	//==============================================================
	// SCU_Power_State_PW : 0x9000_00A0 ==> 0x0000_0000 (Reset PSL)
	//==============================================================

	himax_rw_reg_reformat_com(ADDR_SCU_POWER_STATE,DATA_INIT,tmp_addr,tmp_data);
	himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);

	TS_LOG_INFO("%s: power saving level reset OK!\n",__func__);
}
static bool wait_wip(int Timing)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t in_buffer[10] = {0};
	int retry_cnt = 0;

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================

	himax_rw_reg_reformat_com(ADDR_SPI_FORMAT,DATA_SPI_FORMAT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	//in_buffer[0] = 0x01;
	do
	{
		//=====================================
		// SPI Transfer Control : 0x8000_0020 ==> 0x4200_0003
		//=====================================
		himax_rw_reg_reformat_com(ADDR_SPI_CONTROL,DATA_SPI_CONTROL,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		//=====================================
		// SPI Command : 0x8000_0024 ==> 0x0000_0005
		// read 0x8000_002C for 0x01, means wait success
		//=====================================
		himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_SPI_CMD,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_rw_reg_reformat_com(ADDR_SPI_RESD_STATUS,DATA_SPI_RESD_STATUS,tmp_addr,in_buffer);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, in_buffer);

		if ((in_buffer[0] & 0x01) == 0x00)
			return true;

		retry_cnt++;

		if (in_buffer[0] != 0x00 || in_buffer[1] != 0x00 || in_buffer[2] != 0x00 || in_buffer[3] != 0x00)
			TS_LOG_INFO("%s:Wait wip retry_cnt:%d, buffer[0]=%d, buffer[1]=%d, buffer[2]=%d, buffer[3]=%d \n", __func__,
			retry_cnt,in_buffer[0],in_buffer[1],in_buffer[2],in_buffer[3]);

		if (retry_cnt > 100) {
			TS_LOG_ERR("%s: Wait wip error!\n", __func__);
			return false;
		}
		msleep(Timing);
	}while ((in_buffer[0] & 0x01) == 0x01);
	return true;
}
bool himax_block_erase(int start_addr, int length)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint32_t page_prog_start = 0;

	himax_nc_interface_on();

	/* init psl */
	himax_init_psl();

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	himax_rw_reg_reformat_com(ADDR_SPI_FORMAT,DATA_SPI_FORMAT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	for (page_prog_start = start_addr; page_prog_start < start_addr + length; page_prog_start = page_prog_start + 0x10000)
		{
			//=====================================
			// Chip Erase
			// Write Enable : 	1. 0x8000_0020 ==> 0x4700_0000//control
			//				2. 0x8000_0024 ==> 0x0000_0006//WREN
			//=====================================
			himax_rw_reg_reformat_com(ADDR_SPI_CONTROL,DATA_WRITE_EN,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);

			himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_ERASE_PRE,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);

			//=====================================
			// Block Erase
			// Erase Command : 0x8000_0028 ==> 0x0000_0000 //SPI addr
			//				   0x8000_0020 ==> 0x6700_0000 //control
			//				   0x8000_0024 ==> 0x0000_00D8 //BE
			//=====================================
			//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
			//tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			himax_rw_reg_reformat_com(ADDR_SPI_CMD,page_prog_start,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);

			//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
			//tmp_data[3] = 0x67; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			himax_rw_reg_reformat_com(ADDR_SPI_CONTROL,DATA_BLK_WRITE_EN,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);

			//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
			//tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x52;
			himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_BLK_ERASE,tmp_addr,tmp_data);
			himax_flash_write_burst(tmp_addr, tmp_data);

			msleep(HX_SLEEP_1S);

			if (!wait_wip(100))
			{
				TS_LOG_ERR("%s:83102_Erase Fail\n", __func__);
				return false;
			}
		}
	return true;
}

static void himax_chip_erase(void)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	himax_nc_interface_on();

	/* init psl */
	himax_init_psl();

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	himax_rw_reg_reformat_com(ADDR_SPI_FORMAT,DATA_SPI_FORMAT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
	//				  2. 0x8000_0024 ==> 0x0000_0006
	//=====================================

	himax_rw_reg_reformat_com(ADDR_SPI_CONTROL,DATA_WRITE_EN,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_ERASE_PRE,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Erase Command : 0x8000_0024 ==> 0x0000_00C7
	//=====================================
	himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_ERASE,tmp_addr,tmp_data);
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xC7;
	himax_flash_write_burst(tmp_addr, tmp_data);

	msleep(HX_SLEEP_2S);

	if (!wait_wip(100))
		TS_LOG_ERR("%s:83102_Chip_Erase Fail\n", __func__);
}
static void himax_flash_programming(uint8_t *FW_content, int FW_Size)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int page_prog_start = 0;
	int program_length = 48;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t buring_data[256] = {0};

	TS_LOG_INFO("%s:enter \n", __func__);
	himax_nc_interface_on();
	//himax_burst_enable(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	himax_rw_reg_reformat_com(ADDR_SPI_FORMAT,DATA_SPI_FORMAT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);
	TS_LOG_INFO("%s:start \n", __func__);
	for (page_prog_start = 0; page_prog_start < FW_Size; page_prog_start = page_prog_start + 256) {
		//=====================================
		// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
		//				  2. 0x8000_0024 ==> 0x0000_0006
		//=====================================
		himax_rw_reg_reformat_com(ADDR_SPI_CONTROL,DATA_WRITE_EN,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_rw_reg_reformat_com(ADDR_SPI_WREN,DATA_ERASE_PRE,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		//=================================
		// SPI Transfer Control
		// Set 256 bytes page write : 0x8000_0020 ==> 0x610F_F000
		// Set read start address	: 0x8000_0028 ==> 0x0000_0000
		//=================================

		himax_rw_reg_reformat_com(ADDR_SPI_CONTROL, DATA_SET_PAGE_256,tmp_addr,tmp_data);
		// data bytes should be 0x6100_0000 + ((word_number)*4-1)*4096 = 0x6100_0000 + 0xFF000 = 0x610F_F000
		// Programmable size = 1 page = 256 bytes, word_number = 256 byte / 4 = 64
		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_rw_reg_reformat_com(ADDR_SET_READ_START, (page_prog_start & 0x00FFFFFF),tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		//=================================
		// Send 16 bytes data : 0x8000_002C ==> 16 bytes data
		//=================================
		himax_rw_reg_reformat(DATA_BURNING, buring_data);

		/// <------ bin file
		for (i = page_prog_start, j = 0; i < 16 + page_prog_start; i++, j++) {
			buring_data[j + 4] = FW_content[i];
		}

		if (i2c_himax_nc_write(ADDR_FLASH_BURNED, buring_data, 5 * FOUR_BYTE_CMD, sizeof(buring_data), DEFAULT_RETRY_CNT) < 0) {
			TS_LOG_ERR("%s: i2c access fail!\n", __func__);
			return;
		}
		//=================================
		// Write command : 0x8000_0024 ==> 0x0000_0002
		//=================================

		himax_rw_reg_reformat_com(ADDR_SPI_WREN, DATA_WRITE_CMD,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		//=================================
		// Send 240 bytes data : 0x8000_002C ==> 240 bytes data
		//=================================

		for (j = 0; j < 5; j++) {
			for (i = (page_prog_start + 16 + (j * 48)), k = 0; i < (page_prog_start + 16 + (j * 48)) + program_length; i++, k++) {
				buring_data[k + 4] = FW_content[i];
			}

			if (i2c_himax_nc_write(ADDR_FLASH_BURNED, buring_data, program_length + 4, sizeof(buring_data), DEFAULT_RETRY_CNT) < 0) {
				TS_LOG_ERR("%s: i2c access fail!\n", __func__);
				return;
			}
		}

		if (!wait_wip(1))
			TS_LOG_ERR("%s:83102_Flash_Programming Fail\n", __func__);
	}
	TS_LOG_INFO("%s:end \n", __func__);
}
static uint32_t himax_hw_check_CRC(uint8_t *start_addr, int reload_length)
{
	uint32_t result = 0;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	int cnt = 0;
	int length = reload_length / 4;
	int tmp_len = CRC_LEN;

	//CRC4 // 0x8005_0020 <= from, 0x8005_0028 <= 0x0099_length
	#define ADDR_CRC 0x80050020
	himax_rw_reg_reformat(ADDR_CRC,tmp_addr);
	himax_flash_write_burst(tmp_addr, start_addr);

	tmp_len = (tmp_len << 16) + (length & 0x0000FFFF);
	himax_rw_reg_reformat_com(ADDR_CRC_DATAMAXLEN_SET,tmp_len, tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	cnt = 0;
	himax_rw_reg_reformat(ADDR_CRC_STATUS_SET,tmp_addr);
	do
	{
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		TS_LOG_INFO("%s: waiting for hw ready  tmp_data[3]=%X, tmp_data[2]=%X, tmp_data[1]=%X, tmp_data[0]=%X  \n", __func__, tmp_data[3], tmp_data[2], tmp_data[1], tmp_data[0]);

		if ((tmp_data[0] & 0x01) != 0x01) {
			himax_rw_reg_reformat(ADDR_CRC_RESULT,tmp_addr);
			himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
			TS_LOG_INFO("%s: tmp_data[3]=%X, tmp_data[2]=%X, tmp_data[1]=%X, tmp_data[0]=%X  \n", __func__, tmp_data[3], tmp_data[2], tmp_data[1], tmp_data[0]);
			result = ((tmp_data[3] << 24) + (tmp_data[2] << 16) + (tmp_data[1] << 8) + tmp_data[0]);
			break;
		}
	} while (cnt++ < 100);

	if(cnt < 100)
		return result;
	else
		return FWU_FW_CRC_ERROR;
}
uint8_t himax_nc_calculateChecksum(bool change_iref)
{
	uint8_t CRC_result = 0;
	uint8_t tmp_data[4] = {0};

	himax_rw_reg_reformat(DATA_INIT, tmp_data);

	CRC_result = himax_hw_check_CRC(tmp_data, FW_SIZE_64k);

	return !CRC_result;
}
int hx_nc_fts_ctpm_fw_upgrade_with_fs(unsigned char *fw, int len, bool change_iref)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	int fw_update_ststus = DATA_INIT;

	HX_NC_UPDATE_FLAG = UPDATE_ONGOING;
	TS_LOG_INFO("%s: enter\n", __func__);


    TS_LOG_INFO("%s: The file size is %d bytes\n", __func__, len);

	himax_rw_reg_reformat_com(ADDR_AHBI2C_SYSRST,DATA_AHBI2C_SYSRST,tmp_addr,tmp_data);
	himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);

	himax_nc_sense_off();
	//himax_chip_erase();
	himax_block_erase(ADDR_FLASH_BURNED,FW_SIZE_64k);

	himax_flash_programming(fw, FW_SIZE_64k);

	himax_rw_reg_reformat(DATA_INIT,tmp_data);
	if(himax_hw_check_CRC(tmp_data, FW_SIZE_64k) == 0)
	{
		fw_update_ststus = UPDATE_PASS;
	}
	else
	{
		fw_update_ststus = UPDATE_FAIL;
	}
	//System reset
	himax_rw_reg_reformat_com(ADDR_AHBI2C_SYSRST,DATA_AHBI2C_SYSRST,tmp_addr,tmp_data);
	himax_register_write(tmp_addr, FOUR_BYTE_CMD, tmp_data);

	TS_LOG_INFO("%s: end\n", __func__);
	HX_NC_UPDATE_FLAG = UPDATE_DONE;

	return fw_update_ststus;

}
void himax_nc_flash_dump_func(uint8_t local_flash_command, int Flash_Size, uint8_t *flash_buffer)
{
	int i = 0;
	int page_prog_start = 0;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t out_buffer[20] = {0};
	uint8_t in_buffer[260] = {0};

	TS_LOG_INFO("%s: enter\n", __func__);

	himax_nc_sense_off();
	himax_burst_enable(0);

	/*  init psl */
	himax_init_psl();

	TS_LOG_INFO("%s: flash_dump start\n", __func__);

	/*=============Dump Flash Start=============*/
	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	himax_rw_reg_reformat_com(ADDR_SPI_FORMAT,DATA_SPI_FORMAT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < Flash_Size; page_prog_start = page_prog_start + 256)
	{
		//=================================
		// SPI Transfer Control
		// Set 256 bytes page read : 0x8000_0020 ==> 0x6940_02FF
		// Set read start address  : 0x8000_0028 ==> 0x0000_0000
		// Set command			   : 0x8000_0024 ==> 0x0000_003B
		//=================================
		himax_rw_reg_reformat_com(ADDR_SPI_CONTROL, DATA_SET_PAGE_256_READ,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_rw_reg_reformat_com(ADDR_SET_READ_START, (page_prog_start & 0x00FFFFFF),tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);
		himax_rw_reg_reformat_com(ADDR_SPI_WREN, DATA_WRITE_CMD_x3B,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		//==================================
		// AHB_I2C Burst Read
		// Set SPI data register : 0x8000_002C ==> 0x00
		//==================================
		himax_rw_reg_reformat(DATA_BURNING, out_buffer);
		i2c_himax_nc_write(ADDR_FLASH_BURNED ,out_buffer, FOUR_BYTE_CMD, sizeof(out_buffer), DEFAULT_RETRY_CNT);

		//==================================
		// Read access : 0x0C ==> 0x00
		//==================================
		out_buffer[0] = DATA_READ_ACCESS;
		i2c_himax_nc_write(ADDR_READ_ACCESS ,out_buffer, ONE_BYTE_CMD, sizeof(out_buffer), DEFAULT_RETRY_CNT);

		//==================================
		// Read 128 bytes two times
		//==================================
		i2c_himax_nc_read(DUMMY_REGISTER ,in_buffer, NOR_READ_LENTH, sizeof(in_buffer), DEFAULT_RETRY_CNT);
		for (i = 0; i < 128; i++)
			flash_buffer[i + page_prog_start] = in_buffer[i];

		i2c_himax_nc_read(DUMMY_REGISTER ,in_buffer, NOR_READ_LENTH, sizeof(in_buffer), DEFAULT_RETRY_CNT);
		for (i = 0; i < NOR_READ_LENTH; i++)
			flash_buffer[(i + NOR_READ_LENTH) + page_prog_start] = in_buffer[i];

		TS_LOG_INFO("%s:Verify Progress: %x\n", __func__, page_prog_start);
	}

	TS_LOG_INFO("%s:Dump Flash End\n", __func__);

	himax_nc_sense_on(0x01);
}

void himax_nc_get_DSRAM_data(uint8_t *info_data)
{
	int i = 0;
	int address = 0;
	int fw_run_flag = -1;
	int m_key_num = 0;
	int total_size_temp = 0;
	int total_read_times = 0;
	unsigned char tmp_addr[4] = {0};
	unsigned char tmp_data[4] = {0};
	uint8_t max_i2c_size = 128;
	uint8_t x_num = HX_NC_RX_NUM;
	uint8_t y_num = HX_NC_TX_NUM;
	int mutual_data_size = x_num * y_num * 2;
	int total_size =  (x_num * y_num + x_num + y_num) * 2 + 4;
	uint16_t check_sum_cal = 0;
	uint8_t temp_info_data_hx102b[(MUTUL_NUM_HX83102+ SELF_NUM_HX83102) *2 + 4 + 8] = {0}; //max mkey size = 8
	uint8_t temp_info_data_hx112a[(MUTUL_NUM_HX83112+ SELF_NUM_HX83112) *2 + 4 + 8] = {0};
	/*1. Read number of MKey R100070E8H to determin data size*/

	himax_rw_reg_reformat(ADDR_MKEY,tmp_addr);
	himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD,tmp_data);
	m_key_num = tmp_data[0] & 0x03;
	total_size += m_key_num*2;

	/* 2. Start DSRAM Rawdata and Wait Data Ready */

	himax_rw_reg_reformat_com(ADDR_DSRAM_START,DATA_DSRAM_START,tmp_addr,tmp_data);
	fw_run_flag = himax_nc_write_read_reg(tmp_addr,tmp_data,0xA5,0x5A);

	if(fw_run_flag < 0)
	{
		TS_LOG_INFO("%s Data NOT ready => bypass \n", __func__);
		return;
	}

	/* 3. Read RawData */
	total_size_temp = total_size;

	himax_rw_reg_reformat(ADDR_DSRAM_START,tmp_addr);

	if (total_size % max_i2c_size == 0)
	{
		total_read_times = total_size / max_i2c_size;
	}
	else
	{
		total_read_times = total_size / max_i2c_size + 1;
	}

	for (i = 0; i < (total_read_times); i++)
	{
		if ( total_size_temp >= max_i2c_size)
		{
			if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
				himax_nc_register_read(tmp_addr, max_i2c_size, &temp_info_data_hx102b[i*max_i2c_size]);
			else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
				himax_nc_register_read(tmp_addr, max_i2c_size, &temp_info_data_hx112a[i*max_i2c_size]);
			total_size_temp = total_size_temp - max_i2c_size;
		}
		else
		{
			TS_LOG_DEBUG("last total_size_temp=%d\n",total_size_temp);
			if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
				himax_nc_register_read(tmp_addr, total_size_temp % max_i2c_size, &temp_info_data_hx102b[i*max_i2c_size]);
			else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
				himax_nc_register_read(tmp_addr, total_size_temp % max_i2c_size, &temp_info_data_hx112a[i*max_i2c_size]);
		}

		address = ((i+1)*max_i2c_size);
		tmp_addr[1] = (uint8_t)((address>>8)&0x00FF);
		tmp_addr[0] = (uint8_t)((address)&0x00FF);
	}

	/* 4. FW stop outputing */
	TS_LOG_DEBUG("DSRAM_Flag=%d\n",DSRAM_Flag);
	if(DSRAM_Flag == false)
	{
		TS_LOG_DEBUG("Return to Event Stack!\n");
		himax_rw_reg_reformat_com(ADDR_DSRAM_START,DATA_INIT,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);
	}
	else
	{
		TS_LOG_DEBUG("Continue to SRAM!\n");
		himax_rw_reg_reformat_com(ADDR_DSRAM_START,DATA_RETURN_SRAM_EVENT,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);
	}

	/* 5. Data Checksum Check */
	for (i = 2; i < total_size; i=i+2)/* 2:PASSWORD NOT included */
	{
		if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
			check_sum_cal += (temp_info_data_hx102b[i+1]*256 + temp_info_data_hx102b[i]);
		else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
			check_sum_cal += (temp_info_data_hx112a[i+1]*256 + temp_info_data_hx112a[i]);
	}

	if (check_sum_cal % 0x10000 != 0)
	{
		TS_LOG_INFO("%s check_sum_cal fail=%2X \n", __func__, check_sum_cal);
		return;
	}
	else
	{
		if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
			memcpy(info_data, &temp_info_data_hx102b[4], mutual_data_size * sizeof(uint8_t));
		else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
			memcpy(info_data, &temp_info_data_hx112a[4], mutual_data_size * sizeof(uint8_t));
		TS_LOG_DEBUG("%s checksum PASS \n", __func__);
	}

}
void himax_nc_diag_register_set(uint8_t diag_command)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	TS_LOG_INFO("diag_command = %d\n", diag_command );

	himax_nc_interface_on();

	himax_rw_reg_reformat_com(ADDR_DIAG_REG_SET,(int)diag_command,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);

	himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
	TS_LOG_INFO("%s: tmp_data[3]=0x%02X,tmp_data[2]=0x%02X,tmp_data[1]=0x%02X,tmp_data[0]=0x%02X!\n",
	__func__,tmp_data[3],tmp_data[2],tmp_data[1],tmp_data[0]);

}
void himax_nc_reload_disable(int on)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	TS_LOG_INFO("%s:entering\n",__func__);

	if(on) {
		himax_rw_reg_reformat_com(ADDR_SWITCH_FLASH_RLD,DATA_DISABLE_FLASH_RLD,tmp_addr,tmp_data);
	} else {
		himax_rw_reg_reformat_com(ADDR_SWITCH_FLASH_RLD,DATA_INIT,tmp_addr,tmp_data);
	}

	himax_flash_write_burst(tmp_addr, tmp_data);

	TS_LOG_INFO("%s: setting OK!\n",__func__);
}

void himax_nc_idle_mode(int disable)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	TS_LOG_INFO("%s:entering\n",__func__);
	if(disable)
	{
		himax_rw_reg_reformat(ADDR_IDLE_MODE,tmp_addr);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);

		tmp_data[0] = tmp_data[0] & 0xF7;

		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		TS_LOG_INFO("%s:After turn ON/OFF IDLE Mode [0] = 0x%02X,[1] = 0x%02X,[2] = 0x%02X,[3] = 0x%02X\n", __func__,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);
	}
	else
		TS_LOG_INFO("%s: enable!\n",__func__);

}
int himax_nc_switch_mode(int mode)
{
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};
	uint8_t mode_wirte_cmd = 0;
	uint8_t mode_read_cmd = 0;
	int result = -1;
	int retry = 200;

	TS_LOG_INFO("%s: Entering\n",__func__);

	himax_nc_sense_off();

	/* clean up FW status */
	himax_rw_reg_reformat_com(ADDR_DSRAM_START,DATA_INIT,tmp_addr,tmp_data);
	himax_flash_write_burst(tmp_addr, tmp_data);
	//msleep(HX_SLEEP_30MS);

	if(mode == 0) /*normal mode*/
	{
		if (HX_83112A_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83112,DATA_NOR_MODE,tmp_addr,tmp_data);
		else if (HX_83102B_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83102,DATA_NOR_MODE,tmp_addr,tmp_data);

		mode_wirte_cmd = tmp_data[1];
		mode_read_cmd = tmp_data[0];
	}
	else	/*sorting mode*/
	{
		if (HX_83112A_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83112,DATA_SORT_MODE,tmp_addr,tmp_data);
		else if (HX_83102B_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83102,DATA_SORT_MODE,tmp_addr,tmp_data);

		mode_wirte_cmd = tmp_data[1];
		mode_read_cmd = tmp_data[0];
	}
	himax_flash_write_burst(tmp_addr, tmp_data);
	//msleep(HX_SLEEP_30MS);
	himax_nc_idle_mode(ON);

	// To stable the sorting
	if(mode)
	{
		if (HX_83112A_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83112,DATA_SORT_MODE_NFRAME,tmp_addr,tmp_data);
		else if (HX_83102B_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat_com(ADDR_MODE_SWITCH_HX83102,DATA_SORT_MODE_NFRAME,tmp_addr,tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);
		himax_nc_reload_disable(ON);
	}
	else
	{
		himax_rw_reg_reformat_com(ADDR_NFRAME_SEL,DATA_SET_IIR_FRM,tmp_addr,tmp_data);/*0x0A normal mode 10 frame*/
		/* N Frame Sorting*/
		himax_flash_write_burst( tmp_addr, tmp_data);
		himax_nc_idle_mode(ON);
		himax_nc_reload_disable(ON);
	}

	himax_nc_sense_on(0x01);
	TS_LOG_INFO("mode_wirte_cmd(0)=0x%2.2X,mode_wirte_cmd(1)=0x%2.2X\n",tmp_data[0],tmp_data[1]);
	while(retry!=0)
	{
		TS_LOG_INFO("[%d]Read 100007FC!\n",retry);
		if (HX_83112A_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat(ADDR_MODE_SWITCH_HX83112,tmp_addr);
		else if (HX_83102B_SERIES_PWON==IC_NC_TYPE)
			himax_rw_reg_reformat(ADDR_MODE_SWITCH_HX83102,tmp_addr);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		msleep(HX_SLEEP_100MS);
		TS_LOG_INFO("mode_read_cmd(0)=0x%2.2X,mode_read_cmd(1)=0x%2.2X\n",tmp_data[0],tmp_data[1]);
		if(tmp_data[0] == mode_read_cmd && tmp_data[1] == mode_read_cmd)
		{
			TS_LOG_INFO("Read OK!\n");
			result = NO_ERR;
			break;
		}

		himax_rw_reg_reformat(ADDR_READ_MODE_CHK,tmp_addr);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		if(tmp_data[0] == 0x00 && tmp_data[1] == 0x00 && tmp_data[2] == 0x00 && tmp_data[3] == 0x00)
		{
			TS_LOG_ERR("%s,: FW Stop!\n",__func__);
			break;
		}
		retry--;
	}

	if(result == NO_ERR)
	{
		if(mode == 0)
			return NORMAL_MODE;
		else
			return SORTING_MODE;
	}
	else
		return HX_ERR;    //change mode fail
}
void himax_nc_return_event_stack(void)
{
	int retry = 20;
	uint8_t tmp_addr[4] = {0};
	uint8_t tmp_data[4] = {0};

	TS_LOG_INFO("%s:entering\n",__func__);
	do{

		TS_LOG_INFO("%s,now %d times\n!",__func__,retry);

		himax_rw_reg_reformat_com(ADDR_DSRAM_START,DATA_INIT, tmp_addr, tmp_data);
		himax_flash_write_burst(tmp_addr, tmp_data);

		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);
		retry--;
		msleep(HX_SLEEP_10MS);

	}while((tmp_data[1] != 0x00 && tmp_data[0] != 0x00) && retry > 0);

	TS_LOG_INFO("%s: End of setting!\n",__func__);

}

struct ts_device_ops ts_kit_himax_nc_ops = {
	.chip_parse_config =  himax_parse_dts,
	.chip_detect = himax_chip_detect,
	.chip_init =  himax_init_chip,
	.chip_register_algo = himax_register_algo,
	.chip_input_config = himax_nc_input_config,
	.chip_irq_top_half =  himax_irq_top_half,
	.chip_irq_bottom_half =  himax_irq_bottom_half,
	.chip_suspend = himax_core_suspend,
	.chip_resume = himax_core_resume,
	.chip_reset= himax_reset_device,
	.chip_fw_update_boot = himax_nc_fw_update_boot,
	.chip_fw_update_sd = himax_nc_fw_update_sd,
	.chip_get_info = himax_chip_get_info,
	.chip_get_rawdata = himax_get_rawdata,
	.chip_get_capacitance_test_type = himax_get_capacitance_test_type,
	.chip_shutdown = himax_shutdown,/*NOT tested*/
	.chip_wakeup_gesture_enable_switch = hmx_wakeup_gesture_enable_switch,
	.chip_palm_switch = himax_palm_switch,
	.chip_check_status = Himax_chip_check_status,
	.chip_glove_switch = himax_glove_switch,
#ifdef ROI
	.chip_roi_rawdata = himax_roi_rawdata,
	.chip_roi_switch = himax_roi_switch,
#endif
 };
static int himax_palm_switch(struct ts_palm_info *info)
{
	return NO_ERR;
}
static int __init himax_module_init(void)
{
    bool found = false;
    struct device_node* child = NULL;
    struct device_node* root = NULL;
    int err = NO_ERR;

    TS_LOG_INFO("[HXTP] himax_module_init called here\n");
    g_himax_nc_ts_data = kzalloc(sizeof(struct himax_ts_data), GFP_KERNEL);
    if (!g_himax_nc_ts_data) {
		TS_LOG_ERR("Failed to alloc mem for struct g_himax_nc_ts_data\n");
       err =  -ENOMEM;
       goto himax_ts_data_alloc_fail;
    }
	g_himax_nc_ts_data->tskit_himax_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!g_himax_nc_ts_data->tskit_himax_data ) {
		TS_LOG_ERR("Failed to alloc mem for struct tskit_himax_data\n");
       err =  -ENOMEM;
       goto tskit_himax_data_alloc_fail;
    }

	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root)
	{
		TS_LOG_ERR("[HXTP]huawei_ts, find_compatible_node huawei,ts_kit error\n");
		err = -EINVAL;
		goto out;
	}
	/*find the chip node*/
	for_each_child_of_node(root, child)
	{
		if (of_device_is_compatible(child, HIMAX_VENDER_NAME))
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		TS_LOG_ERR(" not found chip himax child node  !\n");
		err = -EINVAL;
		goto out;
	}

    g_himax_nc_ts_data->tskit_himax_data->cnode = child;
    g_himax_nc_ts_data->tskit_himax_data->ops = &ts_kit_himax_nc_ops;

    err = huawei_ts_chip_register(g_himax_nc_ts_data->tskit_himax_data);
    if(err)
    {
		TS_LOG_ERR(" himax chip register fail !\n");
		goto out;
    }
    TS_LOG_INFO("himax chip_register sucess! teturn value=%d\n", err);
    return err;
out:
	if (g_himax_nc_ts_data->tskit_himax_data){
		kfree(g_himax_nc_ts_data->tskit_himax_data);
		g_himax_nc_ts_data->tskit_himax_data = NULL;
	}
tskit_himax_data_alloc_fail:
	if (g_himax_nc_ts_data){
		kfree(g_himax_nc_ts_data);
		g_himax_nc_ts_data = NULL;
	}
himax_ts_data_alloc_fail:

	return err;

}

static void __exit himax_module_exit(void)
{

	TS_LOG_INFO("himax_module_exit called here\n");
	hx_nc_freeMutualBuffer();
    	hx_nc_freeMutualNewBuffer();
    	hx_nc_freeMutualOldBuffer();
	hx_nc_freeSelfBuffer();
	hx_nc_freeFlashBuffer();

	if (g_himax_nc_ts_data->tskit_himax_data){
		kfree(g_himax_nc_ts_data->tskit_himax_data);
		g_himax_nc_ts_data->tskit_himax_data = NULL;
	}
	if (g_himax_nc_ts_data->pdata){
        kfree(g_himax_nc_ts_data->pdata);
        g_himax_nc_ts_data->pdata = NULL;
   	 }
	if (g_himax_nc_ts_data){
		kfree(g_himax_nc_ts_data);
		g_himax_nc_ts_data = NULL;
	}
   	return;
}


/*lint -save -e* */
late_initcall(himax_module_init);
module_exit(himax_module_exit);
/*lint -restore*/
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
