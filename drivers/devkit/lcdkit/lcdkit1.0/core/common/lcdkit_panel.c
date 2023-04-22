#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "lcdkit_parse.h"
#ifndef CONFIG_ARCH_QCOM
#include "hisi_fb.h"
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory
#endif
#include <huawei_ts_kit.h>

struct lcdkit_adapt_func lcdkit_adapt_func_array[] = {
    {"JDI_NT35696 5.1' CMD TFT 1920 x 1080", lcdkit_jdi_nt35696_5p5_gram_check_show, lcdkit_jdi_nt35696_5p5_reg_read_show},//checksum show
    {"JDI_2LANE_NT36860 5.88' CMD TFT 1440 x 2560", NULL, lcdkit_jdi_nt36860_5p88_reg_read_show}, // gamma calibration show
    {"JDI_2LANE_NT36860 CUT2 5.88' CMD TFT 1440 x 2560", NULL, lcdkit_jdi_nt36860_5p88_reg_read_show}, // gamma calibration show
    {"JDI_NT36860 5.88' CMD TFT 1440 x 2560", NULL, lcdkit_jdi_nt36860_5p88_reg_read_show}, // gamma calibration show
};
uint32_t checksum_start = LCDKIT_CHECKSUM_END;
struct lcdkit_esd_error_info g_esd_error_info;
extern int lcdkit_brightness_ddic_info;
#define MIN_BL_RESUME_TIMMER 1
char lcdkit_panel_name[LCDKIT_MAX_PANEL_NAME_LEN] = {0};
/***********************************************************
  *function: LCD resoure init, include gpio,regulator,xml parse and so on.
  *input:
  *@np: this node is used to parse DTS file.
  *output: get the pinfo struct
*/
void lcdkit_init(struct device_node* np, void* pdata)
{
    lcdkit_parse_panel_dts(np);
    lcdkit_parse_platform_dts(np, pdata);
    lcdkit_info_init(pdata);

    lcdkit_info.panel_infos.display_on_need_send = false;
    lcdkit_info.panel_infos.bl_is_shield_backlight = false;
    lcdkit_info.panel_infos.bl_is_start_second_timer = false;
}

/***********************************************************
  *function: send display on cmds of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@cmds:  the display on code
*/
void lcdkit_on_cmd(void* pdata, struct lcdkit_dsi_panel_cmds* cmds)
{
    lcdkit_dsi_tx(pdata, cmds);
}

/***********************************************************
  *function:  send display off cmds of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@cmds:  the display off code
*/
void lcdkit_off_cmd(void* pdata, struct lcdkit_dsi_panel_cmds* cmds)
{
    lcdkit_dsi_tx(pdata, cmds);
}

/***********************************************************
  *function: this function is used to get the lcdkit_panel_data struct.
*/
struct lcdkit_panel_data* lcdkit_get_panel_info()
{
    return &lcdkit_info;
}

/***********************************************************
  *function: this function is used to get the support flag.
*/
uint32_t lcdkit_get_dbc_set_boost_ctrl_flag(void)
{
    LCDKIT_INFO("bl_dbc_set_boost_ctrl_flag=%u\n",lcdkit_info.panel_infos.bl_dbc_set_boost_ctrl_flag);
    return lcdkit_info.panel_infos.bl_dbc_set_boost_ctrl_flag;
}

/***********************************************************
  *function:  this function is used to get the type of LCD
  *output:
  *@buf: get the type of lcd
*/

static ssize_t lcdkit_model_show(char* buf)
{
    char* lcd_model = lcdkit_info.panel_infos.panel_name;

    if (lcdkit_info.panel_infos.lcd_version_support) {
       return snprintf(buf, PAGE_SIZE, "%s\n", lcdkit_panel_name);
    }

#ifndef CONFIG_ARCH_QCOM
    if ((!runmode_is_factory()) && (NULL != lcdkit_info.panel_infos.panel_model)){
#else
	if (NULL != lcdkit_info.panel_infos.panel_model){
#endif
        lcd_model = lcdkit_info.panel_infos.panel_model;
    }

    if ((NULL == lcd_model) || (NULL == buf)){
        LCDKIT_ERR("NULL_PTR ERROR!\n");
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%s\n", lcd_model);
}

/***********************************************************
  *function:  this function is used to get the type of LCD
  *output:
  *@buf: get the type of lcd
*/

static ssize_t lcdkit_display_type_show(char* buf)
{
    int lcd_display_type = 0;

    if (NULL == buf){
        LCDKIT_ERR("NULL_PTR ERROR!\n");
        return -EINVAL;
    }

    lcd_display_type = lcdkit_is_cmd_panel() ? 1 : 0;
    return snprintf(buf, PAGE_SIZE, "%d\n", lcd_display_type);
}

/***********************************************************
  *function:  this function is used to get the panel information of LCD
  *output:
  *@buf: get max nit of the backlight
**********************************************************/
#define BL_MIN (0)
#define BL_MAX (256)
#define BL_NIT (400)
#define BL_REG_NOUSE_VALUE (128)

static ssize_t lcdkit_panel_info_show(char* buf)
{
    ssize_t ret = 0;
    u32 bl_max_nit = 0;

    lcdkit_update_lcd_brightness_info();
    if (lcdkit_info.panel_infos.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC && lcdkit_brightness_ddic_info > BL_MIN && lcdkit_brightness_ddic_info < BL_MAX){
        bl_max_nit = (lcdkit_brightness_ddic_info < BL_REG_NOUSE_VALUE) ? (lcdkit_brightness_ddic_info + lcdkit_info.panel_infos.get_blmaxnit_offset) : (lcdkit_brightness_ddic_info + lcdkit_info.panel_infos.get_blmaxnit_offset - 1);
    } else if(lcdkit_info.panel_infos.get_blmaxnit_type == GET_BLMAXNIT_FROM_TP && g_ts_kit_lcd_brightness_info) {
        bl_max_nit = g_ts_kit_lcd_brightness_info;
    } else {
        bl_max_nit = lcdkit_info.panel_infos.bl_max_nit;
    }

    if (lcdkit_is_lcd_panel())
    {
        LCDKIT_INFO("is lcd panel!\n");
        ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,blmax_nit_actual:%d,blmax_nit_standard:%d,lcdtype:%s,\n",
                       lcdkit_info.panel_infos.bl_level_max, lcdkit_info.panel_infos.bl_level_min,\
                       bl_max_nit, lcdkit_info.panel_infos.bl_max_nit,"LCD");
    }
    else if (lcdkit_is_oled_panel())
    {
        ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,blmax_nit_actual:%d,blmax_nit_standard:%d,lcdtype:%s,\n",
                       lcdkit_info.panel_infos.bl_level_max,lcdkit_info.panel_infos.bl_level_min,\
                       bl_max_nit, lcdkit_info.panel_infos.bl_max_nit,  "AMOLED");
    }

    return ret;
}

/***********************************************************
  *function: this function is used to get the cabc mode of LCD
  *output:
  *@buf: get the cabc mode of lcd
*/
static ssize_t lcdkit_cabc_mode_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.cabc_mode);
}

/***********************************************************
 *function: this function is used to set the cabc mode of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@buf: the value of cabc mode to be set
*/
static ssize_t lcdkit_cabc_mode_store(void* pdata, const char* buf)
{
    int ret = 0;
    unsigned long val = 0;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("invalid data!\n");
        return -EINVAL;
    }

    switch (val)
    {
        case CABC_OFF :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.cabc_off_cmds);
            break;

        case CABC_UI_MODE :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.cabc_ui_cmds);
            break;

        case CABC_STILL_MODE:
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.cabc_still_cmds);
            break;

        case CABC_MOVING_MODE :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.cabc_moving_cmds);
            break;

        case CABC_GAMING_MODE :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.cabc_gaming_cmds);
            break;

        default:
            return -EINVAL;
    }

    lcdkit_info.panel_infos.cabc_mode = (int)val;
    LCDKIT_INFO("cabc_mode = %d\n", lcdkit_info.panel_infos.cabc_mode);

    return ret;
}

/***********************************************************
  *function: this function is used to get the inversion mode of LCD
  *output:
  *@buf: get the inversion mode of lcd
*/
static ssize_t lcdkit_inversion_mode_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.inversion_mode);
}

/***********************************************************
  *function: this function is used to set the inversion mode of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@buf: the value of inversion mode to be set
*/
static ssize_t lcdkit_inversion_mode_store(void* pdata, const char* buf)
{
    ssize_t ret = 0;
    unsigned long val = 0;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("invalid data!\n");
        return -EINVAL;
    }

    //switch dot and column inversion
    switch (val)
    {
        case COLUMN_INVERSION :
            lcdkit_switch_hs_lp(pdata, 1);
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.column_inversion_cmds);
            lcdkit_switch_hs_lp(pdata, 0);
            break;

        case DOT_INVERSION :
            lcdkit_switch_hs_lp(pdata, 1);
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.dot_inversion_cmds);
            lcdkit_switch_hs_lp(pdata, 0);
            break;

        default :
            return -EINVAL;
    }

    lcdkit_info.panel_infos.inversion_mode = (int)val;
    LCDKIT_INFO("inversion_mode = %d\n", lcdkit_info.panel_infos.inversion_mode);

    return ret;
}

/***********************************************************
  *function: this function is used to get the scan mode of LCD
  *output:
  *@buf: get the scan mode of lcd.
*/
static ssize_t lcdkit_scan_mode_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.scan_mode);
}

/***********************************************************
  *function: this function is used to set the inversion mode of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@buf: the value of scan mode to be set
*/
static ssize_t lcdkit_scan_mode_store(void* pdata,  const char* buf)
{
    int ret = 0;
    unsigned long val = 0;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("invalid data!\n");
        return -EINVAL;
    }

    //switch revert and scan mode
    switch (val)
    {
        case FORWORD_SCAN:
            lcdkit_switch_hs_lp(pdata, 1);
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.forword_scan_cmds);
            lcdkit_switch_hs_lp(pdata, 0);
            break;

        case REVERT_SCAN :
            lcdkit_switch_hs_lp(pdata, 1);
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.revert_scan_cmds);
            lcdkit_switch_hs_lp(pdata, 0);
            break;

        default:
            return -EINVAL;
    }

    lcdkit_info.panel_infos.scan_mode = (int)val;
    LCDKIT_INFO("scan_mode = %d\n", lcdkit_info.panel_infos.scan_mode);
    return ret;
}

/***********************************************************
  *function: this function is used to check the status reg of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *output:
   *@buf: output the check result, "OK" means SUCCESS, "FAIL" means FAIL
*/
#define LCDKIT_REG_CHECK_NUM    5
static ssize_t lcdkit_check_reg_show(void* pdata, char* buf)
{
    int ret = 0;
    uint32_t read_value[MAX_REG_READ_COUNT] = {0};
    int i = 0;
    char* expect_ptr = NULL;
    unsigned int count;

    if (NULL == lcdkit_info.panel_infos.check_reg_value.buf)
    {
        LCDKIT_ERR("check_reg_value buf is null pointer \n");
        return -EINVAL;
    }

    for (count = 0; count < LCDKIT_REG_CHECK_NUM; count++)
    {
        ret = 0;
        expect_ptr = lcdkit_info.panel_infos.check_reg_value.buf;
        lcdkit_dsi_rx(pdata, read_value, 1, &lcdkit_info.panel_infos.check_reg_cmds);

        for (i = 0; i < lcdkit_info.panel_infos.check_reg_cmds.cmd_cnt; i++)
        {
            if ((char)read_value[i] != expect_ptr[i])
            {
                ret = 1;
                LCDKIT_INFO("read_value[%u] = 0x%x, but expect_ptr[%u] = 0x%x!\n",
                    i, read_value[i], i, expect_ptr[i]);
                break;
            }
            LCDKIT_INFO("read_value[%u] = 0x%x same with expect value!\n",
                    i, read_value[i]);
        }

        if (0 == ret)
        {
            break;
        }
    }

    if (0 == ret)
    {
        ret = snprintf(buf, PAGE_SIZE, "OK\n");
    }
    else
    {
        ret = snprintf(buf, PAGE_SIZE, "ERROR\n");
    }

    return ret;
}
static void lcdkit_clear_esd_error_info(void)
{
	memset(&g_esd_error_info, 0, sizeof(g_esd_error_info));
	return;
}
static void lcdkit_record_esd_error_info(int read_reg_index, int read_reg_val, int expect_reg_val)
{
	int reg_index = g_esd_error_info.esd_error_reg_num;

	if ((reg_index + 1) <= MAX_REG_READ_COUNT){
		g_esd_error_info.esd_reg_index[reg_index] = read_reg_index;
		g_esd_error_info.esd_error_reg_val[reg_index] = read_reg_val;
		g_esd_error_info.esd_expect_reg_val[reg_index] = expect_reg_val;
		g_esd_error_info.esd_error_reg_num++;
	}
	return;
}

static int lcd_kit_judge_esd(unsigned char type, unsigned char read_val, unsigned char expect_val)
{
	int ret = 0;

	switch (type) {
	case ESD_UNEQUAL:
		if (read_val != expect_val) {
			ret = 1;
		}
		break;
	case ESD_EQUAL:
		if (read_val == expect_val) {
			ret = 1;
		}
		break;
	case ESD_BIT_VALID:
		if (read_val & expect_val) {
			ret = 1;
		}
		break;
	default:
		if (read_val != expect_val) {
			ret = 1;
		}
		break;
	}
	return ret;
}

/***********************************************************
  *function: this function is used to check the esd status reg of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@cmds: the esd reg to be check.
  *output:
*/
static ssize_t lcdkit_check_esd(void* pdata)
{
    ssize_t ret = 0;
    uint32_t read_value[MAX_REG_READ_COUNT] = {0};
    int i = 0;
    char expect_value = 0;
    char judge_type = 0;
    uint32_t* expect_ptr = NULL;
    int clear_esd_info_flag = FALSE;

    if (lcdkit_dbg.g_esd_debug.esd_enable == 1)
    {
        ret = lcdkit_esd_debug(pdata);
        return ret;
    }

    if (lcdkit_dbg.g_esd_debug.esd_recover_test)
    {
        return -EINVAL;
    }

    if (lcdkit_info.panel_infos.display_on_in_backlight
        && lcdkit_info.panel_infos.display_on_need_send)
    {
        LCDKIT_INFO("display on not set, ESD no need to check\n");
        return 0;
    }

    if (lcdkit_info.panel_infos.panel_chip_esd_disable)
    {
        LCDKIT_DEBUG("panel esd is disable\n");
        return 0;
    }

    if (NULL == lcdkit_info.panel_infos.esd_value.buf)
    {
        LCDKIT_ERR("esd_value cmd buf is null pointer\n");
        return 0;
    }


    expect_ptr = lcdkit_info.panel_infos.esd_value.buf;

    ret = lcdkit_dsi_rx(pdata, read_value, 1, &lcdkit_info.panel_infos.esd_cmds);
    if (!ret)
    {
        for (i = 0; i < lcdkit_info.panel_infos.esd_cmds.cmd_cnt; i++)
        {
            judge_type = (expect_ptr[i] >> 8) & 0xFF;
            expect_value = expect_ptr[i] & 0xFF;
            if (lcd_kit_judge_esd(judge_type, read_value[i], expect_value)) {
                if (FALSE == clear_esd_info_flag) {
					lcdkit_clear_esd_error_info();
					clear_esd_info_flag = TRUE;
                }
                lcdkit_record_esd_error_info(i, (int)read_value[i], expect_ptr[i]);
                LCDKIT_INFO("read_value[%u] = %d, but expect_ptr[%u] = %d!\n", i, read_value[i], i, expect_ptr[i]);
                ret = 1;
                break;
            }
        }
    }

    if((!ret) && lcdkit_info.panel_infos.use_second_ic){
        expect_ptr = lcdkit_info.panel_infos.esd_value.buf;

        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.esd_curic_cmds);
        ret = lcdkit_dsi_rx(pdata, read_value, 1, &lcdkit_info.panel_infos.esd_cmds);
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.esd_oriic_cmds);

        if (!ret)
        {
            for (i = 0; i < lcdkit_info.panel_infos.esd_cmds.cmd_cnt; i++)
            {
                if (read_value[i] != expect_ptr[i])
                {
					if (FALSE == clear_esd_info_flag) {
						lcdkit_clear_esd_error_info();
						clear_esd_info_flag = TRUE;
					}
					lcdkit_record_esd_error_info(i, (int)read_value[i], expect_ptr[i]);
                    LCDKIT_INFO("read_value[%u] = %d, but expect_ptr[%u] = %d!\n", i, read_value[i], i, expect_ptr[i]);
                    ret = 1;
                    break;
                }
            }
        }
    }
	LCDKIT_DEBUG("clear_esd_info_flag=%d\n",clear_esd_info_flag);
    return ret;
}

/***********************************************************
  *function: this function is used to check the gram status reg of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *output:
  *@buf: output the check result, "OK" means SUCCESS, "FAIL" means FAIL
*/
static ssize_t lcdkit_gram_check_show(void* pdata, char* buf)
{
     int ret = 0, i = 0, checksum_result = 0;
    uint32_t rd[LCDKIT_CHECKSUM_SIZE] = {0};
    char checksum_read[] = {0x00};
    struct lcdkit_dsi_cmd_desc packet_size_set_cmd = {LCDKIT_DTYPE_MAX_PKTSIZE, 1, 0, 0, 10, LCDKIT_WAIT_TYPE_US, 1, NULL};
    struct lcdkit_dsi_cmd_desc check_reg_dsi_cmd = {DTYPE_DCS_READ, 1, 0, 0, 10, LCDKIT_WAIT_TYPE_US, 1, checksum_read};
    struct lcdkit_dsi_panel_cmds lcd_check_reg = {NULL, 0, &check_reg_dsi_cmd, 1, 0, 0};
    char read_reg = 0;
    int reg_count = 0;

    LCDKIT_INFO("enter \n");

    switch (lcdkit_info.panel_infos.checksum_pic_num){
        case TEST_PIC_0:
        case TEST_PIC_1:
        case TEST_PIC_2:
            LCDKIT_INFO("start check gram checksum:[%d]\n", lcdkit_info.panel_infos.checksum_pic_num);
            break;
        default:
            LCDKIT_ERR("gram checksum pic num unknown(%d)\n", lcdkit_info.panel_infos.checksum_pic_num);
            //unknown pic, return fail
            ret = snprintf(buf, PAGE_SIZE, "1\n");
            return ret;
    }
    for (i = 0; i < sizeof(lcdkit_adapt_func_array)/sizeof(lcdkit_adapt_func_array[0]); i++) {
        if (!strncmp(lcdkit_info.panel_infos.panel_name, lcdkit_adapt_func_array[i].name, strlen(lcdkit_adapt_func_array[i].name))) {
            if (lcdkit_adapt_func_array[i].lcdkit_gram_check_show) {
                ret = lcdkit_adapt_func_array[i].lcdkit_gram_check_show(pdata, buf);
                LCDKIT_INFO("%s\n", buf);
                return ret;
            }
        }
    }

    lcdkit_mipi_dsi_max_return_packet_size(pdata, &packet_size_set_cmd);
    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_enter_cmds);
    reg_count = lcdkit_info.panel_infos.checksum_cmds.cmd_cnt;
    read_reg = lcdkit_info.panel_infos.checksum_cmds.cmds[reg_count - 1].payload[0];
    for (i = 0; i < LCDKIT_CHECKSUM_SIZE; i++)
    {
        char *data = lcd_check_reg.cmds->payload;
        *data = read_reg + i;

        lcdkit_dsi_rx(pdata, (rd + i), 0, &lcd_check_reg);
        if (rd[i] != lcdkit_info.panel_infos.checksum_value.arry_data[lcdkit_info.panel_infos.checksum_pic_num].buf[i])
        {
            checksum_result++;
        }
    }
    LCDKIT_INFO("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n", \
            rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);

    if (lcdkit_info.panel_infos.checksum_second_part_support) {
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_enter_second_part_cmds);
        for (i = 0; i < LCDKIT_CHECKSUM_SIZE; i++)
        {
            char *data = lcd_check_reg.cmds->payload;
            *data = read_reg + i;
            lcdkit_dsi_rx(pdata, (rd + i), 0, &lcd_check_reg);
            if (rd[i] != lcdkit_info.panel_infos.checksum_value.arry_data[lcdkit_info.panel_infos.checksum_pic_num].buf[i + LCDKIT_CHECKSUM_SIZE])
            {
                checksum_result++;
            }
        }
        LCDKIT_INFO("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n", \
                rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);
    }

    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_exit_cmds);

    if (checksum_result){
        LCDKIT_ERR("checksum_result:%d\n", checksum_result);
        checksum_result = 1;
    }

    ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);
    if (checksum_result && 2 == lcdkit_info.panel_infos.checksum_pic_num){
        checksum_start = LCDKIT_CHECKSUM_END;
    }

    if (LCDKIT_CHECKSUM_END == checksum_start){
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_disable_cmds);
        lcdkit_effect_switch_ctrl(pdata, 0);
        LCDKIT_INFO("gram checksum end, disable checksum.\n");
    }

    return ret;
}

static ssize_t lcdkit_gram_check_store(void* pdata, const char* buf)
{
     ssize_t ret = 0;
    unsigned long val = 0;
    static int pic_4_check_count = 0;
    int pic_index = 0;

    ret = strict_strtoul(buf, 0, &val);
    if (ret)
    {
        return ret;
    }
    LCDKIT_INFO("val=%ld\n", val);

    pic_index = val - 2;
    if (LCDKIT_CHECKSUM_END == checksum_start){
        /* The first pic num 2 used to instruct that checksum start */
        if (TEST_PIC_0 == pic_index){
            LCDKIT_INFO("start gram checksum...\n");
            checksum_start = LCDKIT_CHECKSUM_START;
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.checksum_enable_cmds);
            lcdkit_effect_switch_ctrl(pdata, 1);
            lcdkit_info.panel_infos.checksum_pic_num = 0xff;
            LCDKIT_INFO("Enable checksum\n");
            return ret;
        }else{
            LCDKIT_INFO("gram checksum not start, and first cmd not start:2\n");
            return -1;
        }
    }else{
        if (TEST_PIC_2 == pic_index) pic_4_check_count++;
        if (CHECKSUM_CHECKCOUNT == pic_4_check_count){
            LCDKIT_INFO("gram checksum pic %ld test 5 times, set checkout end\n", val);
            checksum_start = LCDKIT_CHECKSUM_END;
            pic_4_check_count = 0;
        }
    }

    switch(pic_index){
        case TEST_PIC_0:
        case TEST_PIC_1:
        case TEST_PIC_2:
            LCDKIT_INFO("gram checksum set pic [%d]\n", pic_index);
            lcdkit_info.panel_infos.checksum_pic_num = pic_index;
            break;
        default:
            LCDKIT_INFO("gram checksum set pic [%d] unknown\n", pic_index);
            lcdkit_info.panel_infos.checksum_pic_num = 0xff;
            break;
    }

    return ret;
}

static ssize_t lcdkit_dynamic_sram_check_show(void* pdata, char* buf)
{
    int ret = 0, i = 0, checksum_result = 1;
    uint32_t rd[LCDKIT_CHECKSUM_SIZE] = {0};
    struct lcdkit_dsi_cmd_desc packet_size_set_cmd = {LCDKIT_DTYPE_MAX_PKTSIZE, 1, 0, 0, 10, LCDKIT_WAIT_TYPE_US, 1, NULL};

    lcdkit_mipi_dsi_max_return_packet_size(pdata, &packet_size_set_cmd);
    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.dynamic_sram_check_enter_cmds);

    for (i = 0; i < LCDKIT_CHECKSUM_SIZE; i++)
    {
        char* data = lcdkit_info.panel_infos.checksum_cmds.cmds[i].payload;
        *data = 0x73 + i;

        lcdkit_dsi_rx(pdata, (rd + i), 0, &lcdkit_info.panel_infos.dynamic_sram_check_cmds);

        if (rd[i] != lcdkit_info.panel_infos.dynamic_sram_check_value.arry_data[lcdkit_info.panel_infos.dynamic_sram_check_pic_num].buf[i])
        {
            LCDKIT_ERR("rd[%d] = 0x%x, expect checksum_value[%d]= 0x%x\n", i, rd[i], i, lcdkit_info.panel_infos.dynamic_sram_check_value.arry_data[lcdkit_info.panel_infos.dynamic_sram_check_pic_num].buf[i]);
            checksum_result = 0;
            break;
        }
    }

    ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);
    LCDKIT_INFO("%d %d %d %d %d %d %d %d\n", \
                rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);

    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.dynamic_sram_check_exit_cmds);

    return ret;
}

static ssize_t lcdkit_dynamic_sram_check_store(void* pdata, const char* buf)
{
    ssize_t ret = 0;

    if ((!*buf) || (!strchr("yY1nN0", *buf)))
    {
        LCDKIT_ERR("Input param is error(valid: yY1nN0): %s. \n", buf);
        return -EINVAL;
    }

    lcdkit_info.panel_infos.dynamic_sram_check_pic_num = (int)(buf[1] - '0');
    lcdkit_info.panel_infos.dynamic_sram_check_support = !!strchr("yY1", *buf);

    if ( lcdkit_info.panel_infos.dynamic_sram_check_support == true )
    {
        if (lcdkit_info.panel_infos.dynamic_sram_check_pic_num < LCDKIT_CHECKSUM_PIC_NUM)
        {
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.dynamic_sram_check_enable_cmds);

        }
        else
        {
            LCDKIT_ERR("Input param is invalid: %s, pic_num=%d. \n", buf,  lcdkit_info.panel_infos.dynamic_sram_check_pic_num);
            return -EINVAL;
        }

        LCDKIT_DEBUG("Enable dyanmic_sram_check!\n");
    }
    else
    {
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.dynamic_sram_check_disable_cmds);
        LCDKIT_DEBUG("Disable dyanmic_sram_check!\n");
    }

    return ret;
}

static ssize_t lcdkit_ce_mode_show(void* pdata, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.ce_mode);
}

static ssize_t lcdkit_ce_mode_store(void* pdata, const char* buf, size_t count)
{
    int ret = 0;
    unsigned long val = 0;
    int flag=-1;

    ret = strict_strtoul(buf, 0, &val);
    if(ret)
       return ret;

    flag=(int)val;

    switch(flag)
    {
        case CE_OFF:
            LCDKIT_INFO("ce mode = %d!\n", flag);
            lcdkit_dsi_tx(pdata,&lcdkit_info.panel_infos.ce_off_cmds);
            break;

        case CE_SRGB_MODE:
            LCDKIT_INFO("ce mode = %d!\n", flag);
            lcdkit_dsi_tx(pdata,&lcdkit_info.panel_infos.ce_srgb_cmds);
            break;
        case CE_USER_MODE:
            LCDKIT_INFO("ce mode = %d!\n", flag);
            lcdkit_dsi_tx(pdata,&lcdkit_info.panel_infos.ce_user_cmds);
            break;
        case CE_VIVID_MODE:
            LCDKIT_INFO("ce mode = %d!\n", flag);
            lcdkit_dsi_tx(pdata,&lcdkit_info.panel_infos.ce_vivid_cmds);
            break;
        default:
            LCDKIT_INFO("wrong flag!\n");
            break;
    }

    lcdkit_info.panel_infos.ce_mode = flag;

    return count;

}

static ssize_t lcdkit_sleep_ctrl_show(char* buf)
{
    return  snprintf(buf, PAGE_SIZE, "enable_PT_test=%d, PT_test_support=%d\n",
                     lcdkit_info.panel_infos.enable_PT_test, lcdkit_info.panel_infos.PT_test_support);
}

static ssize_t lcdkit_sleep_ctrl_store(const char* buf)
{
    int ret = 0;
    unsigned long val = 0;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("invalid parameter!\n");
        return ret;
    }

    if ( lcdkit_info.panel_infos.PT_test_support )
    {
        lcdkit_info.panel_infos.enable_PT_test = val;
    }

    if (lcdkit_info.panel_infos.enable_PT_test == 2)
    {
        g_tskit_pt_station_flag = 1;    //used for pt  current test, tp sleep
    }
    else
    {
        g_tskit_pt_station_flag = 0;    //used for pt  current test, tp power off
    }

    LCDKIT_INFO("exit!\n");
    return ret;

}

static ssize_t lcdkit_lp2hs_mipi_check_show(char* buf)
{
    if (lcdkit_info.panel_infos.g_lp2hs_mipi_check_result)
    {
        return snprintf(buf, PAGE_SIZE, "OK\n");
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "FAIL\n");
    }
}

static ssize_t lcdkit_lp2hs_mipi_check_store(void* pdata, const char* buf)
{
    int ret = 0;
    unsigned long val = 0;
    int flag = -1;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    { return ret; }

    flag = (int)val;

    if (flag == LP2HS_MIPI_TEST_OFF)
    {
        lcdkit_info.panel_infos.lp2hs_mipi_check_support = false;
        lcdkit_info.panel_infos.g_lp2hs_mipi_check_result = false;
        lcdkit_info.panel_infos.lcd_self_testing = false;
        LCDKIT_INFO("lp2hs_mipi test OFF\n");
    }
    else  if (flag == LP2HS_MIPI_TEST_ON)
    {
        lcdkit_info.panel_infos.lp2hs_mipi_check_support = true;
        lcdkit_info.panel_infos.g_lp2hs_mipi_check_result = true;
        lcdkit_info.panel_infos.lcd_self_testing = true;
        LCDKIT_INFO("lp2hs_mipi test ON\n");
    }

    return ret;

}

static ssize_t lcdkit_bist_check_show(void* pdata, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

/*mipi detect*/
/*function:
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *output:
  *@buf: output the test result,OK means success, FAIL means fail.
*/
static ssize_t lcdkit_mipi_detect_show(void* pdata, char* buf)
{
    int i = 0;
    ssize_t ret = 0;
    uint32_t read_value[MAX_REG_READ_COUNT] = {0};
    char* expect_ptr = NULL;
    char* mask = NULL;

    expect_ptr = lcdkit_info.panel_infos.mipi_detect_value.buf;
    mask = lcdkit_info.panel_infos.mipi_detect_mask.buf;

    if (NULL == expect_ptr)
    {
        LCDKIT_ERR("mipi_detect_value buf is null pointer \n");
        return -EINVAL;
    }

    if (NULL == mask)
    {
        LCDKIT_ERR("mipi_detect_mask buf is null pointer \n");
        return -EINVAL;
    }

    lcdkit_dsi_rx(pdata, read_value, 0, &lcdkit_info.panel_infos.mipi_detect_cmds);

    for (i = 0; i < lcdkit_info.panel_infos.mipi_detect_cmds.cmd_cnt; i++)
    {
        if (read_value[i] != (expect_ptr[i] & mask[i] ))
        {
            ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
            LCDKIT_ERR("read_value[%d] = %d, expect_ptr[%d]=%d ", i, read_value[i], i, expect_ptr[i]);
            ret = 1;
            break;
        }
    }

    if (!ret)
    {
        ret = snprintf(buf, PAGE_SIZE, "OK\n");
    }

    return ret;
}

static ssize_t lcdkit_hkadc_debug_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.hkadc_buf * 4);
}

static ssize_t lcdkit_hkadc_debug_store(const char* buf)
{
    ssize_t ret = 0;
    int channel = 0;

    ret = sscanf(buf, "%u", &channel);

    if (ret <= 0)
    {
        LCDKIT_ERR("ivalid parameter!\n");
        return ret;
    }

    lcdkit_info.panel_infos.hkadc_buf = adc_get_value(channel);
    return ret;
}

static ssize_t lcdkit_pcd_errflag_check(char* buf)
{
	int pcd_gpio = 0;
	int errflag_gpio = 0;
	int rc = 0;
	ssize_t ret = 0;
	u8 result_value = 0;

	if (lcdkit_info.panel_infos.gpio_pcd != 0) {
		rc = gpio_request(lcdkit_info.panel_infos.gpio_pcd,
			GPIO_LCDKIT_PCD_NAME);
		if (rc != 0) {
			result_value = PCD_ERRFLAG_FAIL;
			LCDKIT_ERR("gpio_pcd[%d] request failed\n",
				lcdkit_info.panel_infos.gpio_pcd);
			goto err_out;
		}
		rc = gpio_direction_input(lcdkit_info.panel_infos.gpio_pcd);
		if (rc != 0) {
			gpio_free(lcdkit_info.panel_infos.gpio_pcd);
			result_value = PCD_ERRFLAG_FAIL;
			LCDKIT_ERR("gpio_err_flag[%d] direction set fail!\n",
				lcdkit_info.panel_infos.gpio_pcd);
			goto err_out;
		}
		pcd_gpio = gpio_get_value(lcdkit_info.panel_infos.gpio_pcd);
		gpio_free(lcdkit_info.panel_infos.gpio_pcd);
	}
	if (lcdkit_info.panel_infos.gpio_err_flag != 0) {
		rc = gpio_request(lcdkit_info.panel_infos.gpio_err_flag,GPIO_LCDKIT_ERRFLAG_NAME);
		if (rc != 0) {
			result_value = PCD_ERRFLAG_FAIL;
			LCDKIT_ERR("gpio_err_flag[%d] request fail!\n",lcdkit_info.panel_infos.gpio_err_flag);
			goto err_out;
		}
		rc = gpio_direction_input(lcdkit_info.panel_infos.gpio_err_flag);
		if (rc != 0) {
			gpio_free(lcdkit_info.panel_infos.gpio_err_flag);
			result_value = PCD_ERRFLAG_FAIL;
			LCDKIT_ERR("gpio_err_flag[%d] direction set fail!\n",lcdkit_info.panel_infos.gpio_err_flag);
			goto err_out;
		}
		errflag_gpio = gpio_get_value(lcdkit_info.panel_infos.gpio_err_flag);
		gpio_free(lcdkit_info.panel_infos.gpio_err_flag);
	}
	LCDKIT_INFO("pcd_gpio[%d]= %d, errflag_gpio[%d]= %d \n",
		lcdkit_info.panel_infos.gpio_pcd,pcd_gpio,
		lcdkit_info.panel_infos.gpio_err_flag,errflag_gpio);
	if ((pcd_gpio == GPIO_LOW_PCDERRFLAG) && (errflag_gpio == GPIO_LOW_PCDERRFLAG)) {
		result_value = PCD_ERR_FLAG_SUCCESS; // PCD_ERR_FLAG_SUCCESS
	} else if ((pcd_gpio == GPIO_HIGH_PCDERRFLAG) && (errflag_gpio == GPIO_LOW_PCDERRFLAG)) {
		result_value = PCD_FAIL; //only  PCD_FAIL
	} else if ((pcd_gpio == GPIO_LOW_PCDERRFLAG) && (errflag_gpio == GPIO_HIGH_PCDERRFLAG)) {
		result_value = ERRFLAG_FAIL; //only ERRFLAG FAIL
	} else if ((pcd_gpio == GPIO_HIGH_PCDERRFLAG) && (errflag_gpio == GPIO_HIGH_PCDERRFLAG)) {
		result_value = PCD_ERRFLAG_FAIL; //PCD_ERR_FLAG_FAIL
	} else {
		result_value = PCD_ERR_FLAG_SUCCESS;
	}
err_out:
	LCDKIT_INFO("The test cmd is pcd_errflag,and the result_value = %d\n",result_value);
	ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);
	return ret;
}

static ssize_t lcdkit_amoled_acl_ctrl_show(char* buf)
{
    ssize_t ret = 0;
    ret = snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.acl_ctrl_mode);
    return ret;
}

static ssize_t lcdkit_amoled_acl_ctrl_store(void* pdata, const char* buf)
{
    ssize_t ret = 0;
    int flag = 0;
    unsigned int value[ACL_MAX_SIZE] = {0};
    char *token, *cur;
    int i = 0;

    cur = buf;
    while(token = strsep(&cur, ","))
    {
        value[i++] = simple_strtol(token, NULL, 0);
        if(i >= ACL_MAX_SIZE)
        {
            LCDKIT_ERR("out of range of value[ACL_MAX_SIZE] buf!\n");
            return ret;
        }
    }

    if(ACL_SET_MODE != value[0])
    {
        LCDKIT_INFO("check your input parameter !\n");
        return ret;
    }
    else
    {
        flag = (int)value[1];
        if (flag == lcdkit_info.panel_infos.acl_ctrl_mode)
        {
            return ret;
        }

        switch (flag)
        {
            case ACL_OFF:
                lcdkit_info.panel_infos.acl_ctrl_mode = flag;
                lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.acl_off_cmds);
                break;

            case ACL_ON_SET1:
                lcdkit_info.panel_infos.acl_ctrl_mode = flag;
                lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.acl_high_cmds);
                break;

            case ACL_ON_SET2:
                lcdkit_info.panel_infos.acl_ctrl_mode = flag;
                lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.acl_middle_cmds);
                break;

            case ACL_ON_SET3:
                lcdkit_info.panel_infos.acl_ctrl_mode = flag;
                lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.acl_low_cmds);
                break;

            default:
                LCDKIT_ERR("check your input parameter !\n");
                return ret;
        }
        return ret;
    }
}

static ssize_t lcdkit_amoled_vr_mode_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE,  "%d\n", lcdkit_info.panel_infos.vr_mode);
}

static ssize_t lcdkit_amoled_vr_mode_store(void* pdata, const char* buf)
{
    ssize_t ret = 0;
    unsigned long val = 0;
    int flag = -1;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("string to l int error, buf[%s], ret:%ld\n", buf, ret);
        return ret;
    }

    flag = (int)val;

    switch (flag)
    {
        case VR_MODE_ENABLE:
            LCDKIT_INFO("VR CYCLE SET vr mode\n");
            lcdkit_info.panel_infos.vr_mode = VR_MODE_ENABLE;
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.vr_mode_enable_cmds);
            break;

        case  VR_MODE_DISABLE:
            LCDKIT_INFO("VR CYCLE SET normal mode\n");
            lcdkit_info.panel_infos.vr_mode = VR_MODE_DISABLE;
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.vr_mode_disable_cmds);
            break;

        default:
            lcdkit_info.panel_infos.vr_mode = VR_MODE_DISABLE;
            LCDKIT_INFO("VR CYCLE SET unknown mode\n");
            break;
    }

    return ret;
}

static ssize_t lcdkit_amoled_hbm_ctrl_show(char* buf)
{
    return  snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.hbm_ctrl_mode);
}

static ssize_t lcdkit_amoled_hbm_ctrl_store(void* pdata, const char* buf)
{
    ssize_t ret = 0;
    unsigned long val = 0;

    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    { return ret; }

    lcdkit_info.panel_infos.hbm_ctrl_mode = (int)val;
    lcdkit_info.panel_infos.hbm_enable_cmds.cmds->payload[1] =  lcdkit_info.panel_infos.hbm_ctrl_mode ? 0xe8 : 0x20;

    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.hbm_enable_cmds);

    return ret;
}

static ssize_t lcdkit_support_mode_show(char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%u\n", lcdkit_info.panel_infos.effect_support_mode);
}

static ssize_t lcdkit_support_mode_store(const char* buf)
{
    ssize_t ret = 0;
    unsigned long val = 0;
    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        return ret;
    }

    lcdkit_info.panel_infos.effect_support_mode = (u32)val;
    return ret;
}

static ssize_t lcdkit_support_checkmode_show(char* buf)
{
    ssize_t ret = 0;

    ret = snprintf(buf, PAGE_SIZE, "checksum:%d;lp2hs_mipi_check:%d\n", lcdkit_info.panel_infos.checksum_support, lcdkit_info.panel_infos.lp2hs_mipi_check_support);

    return ret;
}

static ssize_t lcdkit_current_detect(void* pdata)
{
    ssize_t current_check_result = 0;
        int i = 0;
    uint32_t rd[LCDKIT_CHECKSUM_SIZE] = {0};
    struct lcdkit_dsi_cmd_desc packet_size_set_cmd = {LCDKIT_DTYPE_MAX_PKTSIZE, 1, 0, 0, 10, LCDKIT_WAIT_TYPE_US, 1, NULL};
    LCDKIT_INFO("current detect enter!\n");

    if(!lcdkit_info.panel_infos.current_detect_support)
    {
        LCDKIT_INFO("current detect is not support! return!\n");
        return 0;
    }

    lcdkit_mipi_dsi_max_return_packet_size(pdata, &packet_size_set_cmd);
    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.current_detect_enter_cmds);
    lcdkit_dsi_rx(pdata, rd, 0, &lcdkit_info.panel_infos.current_detect_cmds);

    LCDKIT_INFO("%d %d %d %d %d %d %d %d\n", \
                rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);

    if (lcdkit_info.panel_infos.current_mask.buf
        &&((rd[i] & lcdkit_info.panel_infos.current_mask.buf[0]) == 0)) {
        current_check_result = 0;
        LCDKIT_ERR("no current over\n");
    }
    else{
        current_check_result = 1;
        LCDKIT_ERR("current over:0x%x\n", rd[0]);
    }

    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.current_detect_exit_cmds);

    return current_check_result;
}

static ssize_t lcdkit_lv_detect(void* pdata)
{
    ssize_t lv_detect_result = 0;
    int i = 0;
    uint32_t rd[LCDKIT_CHECKSUM_SIZE] = {0};
    struct lcdkit_dsi_cmd_desc packet_size_set_cmd = {LCDKIT_DTYPE_MAX_PKTSIZE, 1, 0, 0, 10, LCDKIT_WAIT_TYPE_US, 1, NULL};

    LCDKIT_INFO("lv detect enter!\n");

    if(!lcdkit_info.panel_infos.lv_detect_support)
    {
        LCDKIT_INFO("lv detect is not support! return!\n");
        return 0;
    }

    lcdkit_mipi_dsi_max_return_packet_size(pdata, &packet_size_set_cmd);
    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.lv_detect_enter_cmds);
    lcdkit_dsi_rx(pdata, rd, 0, &lcdkit_info.panel_infos.lv_detect_cmds);

    LCDKIT_INFO("%d %d %d %d %d %d %d %d\n", \
                rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);

    if (rd[i] != 0) {
        LCDKIT_ERR("rd[%d] = 0x%x, expected = 0x00\n", i, rd[i]);
        lv_detect_result = 1;
    }else{
        lv_detect_result = 0;
    }

    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.lv_detect_exit_cmds);
    return lv_detect_result;
}


static ssize_t lcdkit_test_config_show(void* pdata, char* buf)
{
    int i = 0;
    int test_result = 0;

    for (i = 0; i < SENCE_ARRAY_SIZE; i++)
    {
        if (sence_array[i] == NULL) {
            LCDKIT_INFO("Sence cmd is end, total num is:%d\n", i);
            break;
        }
        if ( !strncmp(lcdkit_info.panel_infos.lcd_cmd_now, sence_array[i], strlen(lcdkit_info.panel_infos.lcd_cmd_now)))
        {
            LCDKIT_INFO("current test cmd:%s,return cmd:%s\n",
                        lcdkit_info.panel_infos.lcd_cmd_now, cmd_array[i]);

	     if(!strncmp(lcdkit_info.panel_infos.lcd_cmd_now,"BL_OPEN_SHORT",strlen(lcdkit_info.panel_infos.lcd_cmd_now)))
	     {
                  LCDKIT_INFO("lcdkit_info.panel_infos.lcd_cmd_now:%s\n", "BL_OPEN_SHORT");
                  LCDKIT_INFO("lcdkit_info.panel_infos.backlight_ic_common: %d\n", lcdkit_info.panel_infos.backlight_ic_common); 
                  if(lcdkit_info.panel_infos.backlight_ic_common)
                  {
                      return snprintf(buf, PAGE_SIZE, "/sys/class/lcd_backlight/lcd_backlight/self_test");
                  }
	     }
            return snprintf(buf, PAGE_SIZE, cmd_array[i]);
        }
    }

    /* lcd support over current and over voltage detect */
    if (!strncmp(lcdkit_info.panel_infos.lcd_cmd_now, "CURRENT_TEST_MODE", strlen("CURRENT_TEST_MODE"))) {
        return snprintf(buf, PAGE_SIZE, "LCD_CUR_DET_MODE");
    /* over current detect */
    } else if (!strncmp(lcdkit_info.panel_infos.lcd_cmd_now, "OVER_CURRENT_DETECTION", strlen("OVER_CURRENT_DETECTION"))) {
        test_result = lcdkit_current_detect(pdata);
        return snprintf(buf, PAGE_SIZE, "%d", test_result);
    /* over voltage detect */
    } else if (!strncmp(lcdkit_info.panel_infos.lcd_cmd_now, "OVER_VOLTAGE_DETECTION", strlen("OVER_VOLTAGE_DETECTION"))) {
        test_result = lcdkit_lv_detect(pdata);
        return snprintf(buf, PAGE_SIZE, "%d", test_result);
    /* input invaild */
    } else {
        LCDKIT_INFO("cmd invaild,current test cmd:%s\n", lcdkit_info.panel_infos.lcd_cmd_now);
        return snprintf(buf, PAGE_SIZE, "INVALID");
    }
}

static ssize_t lcdkit_test_config_store(const char* buf)
{
    if (strlen(buf) < LCDKIT_CMD_NAME_MAX)
    {
        memcpy(lcdkit_info.panel_infos.lcd_cmd_now, buf, strlen(buf) + 1);
        LCDKIT_INFO("current test cmd:%s\n", lcdkit_info.panel_infos.lcd_cmd_now);
    }
    else
    {
        memcpy(lcdkit_info.panel_infos.lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
        LCDKIT_INFO("invalid test cmd:%s\n", lcdkit_info.panel_infos.lcd_cmd_now);
    }

    return 0;
}

static ssize_t lcdkit_fps_scence_handle(struct platform_device* pdev, uint32_t scence)
{
    lcdkit_fps_scence_adaptor_handle(pdev, scence);

    /*for video panel, updt porch*/
    if (!lcdkit_is_cmd_panel())
    {
        lcdkit_updt_porch(pdev, scence);
    }

    return 0;
}

static ssize_t lcdkit_fps_updt_handle(void* pdata)
{
    lcdkit_fps_updt_adaptor_handle(pdata);
    return 0;
}

int lcdkit_fps_support_query(void)
{
    return lcdkit_info.panel_infos.fps_func_switch;
}
EXPORT_SYMBOL(lcdkit_fps_support_query);
int lcdkit_fps_tscall_support_query(void)
{
    return lcdkit_info.panel_infos.fps_tscall_support;
}
EXPORT_SYMBOL(lcdkit_fps_tscall_support_query);

/*function: this function is used to called by ts thread to let lcd driver
  *known that there is a tp touch event report in tp ic driver
  *input:void
  *@callback_type: callback_type use to tell lcd driver what to do.
*/
void lcdkit_fps_ts_callback(void)
{
    lcdkit_fps_adaptor_ts_callback();
    return ;
}
EXPORT_SYMBOL(lcdkit_fps_ts_callback);

void lcdkit_fps_timer_init(void)
{
    lcdkit_fps_timer_adaptor_init();
    return ;
}

/*function: this function is used to set the Partial  display of LCD
  *input:
  *@pdata: this void point is used to converte to fb data struct.
  *@dirty: the region of partial display need
*/
static ssize_t lcdkit_set_display_region(void* pdata, void* dirty)
{
    struct dirty_rect* dirty_region = NULL;
    ssize_t ret = 0;

    if ( dirty == NULL)
    {
        LCDKIT_ERR("dirty is null point!\n");
        return ret;
    }

    dirty_region = (struct dirty_rect*) dirty;
    if (((dirty_region->x == 0) && (dirty_region->w == 0)) ||
        ((dirty_region->y == 0) && (dirty_region->h == 0))) {
        LCDKIT_ERR("dirty region error:x/w/y/h=%d/%d/%d/%d!\n", dirty_region->x,
            dirty_region->w,dirty_region->y,dirty_region->h);
        return ret;
    }

    lcdkit_dump_cmds(&lcdkit_info.panel_infos.display_region_cmds);

    lcdkit_info.panel_infos.display_region_cmds.cmds[0].payload[1] = ((uint32_t)(dirty_region->x) >> 8) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[0].payload[2] = ((uint32_t)(dirty_region->x)) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[0].payload[3] = ((uint32_t)(dirty_region->x + dirty_region->w - 1) >> 8) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[0].payload[4] = (dirty_region->x + dirty_region->w - 1) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[1].payload[1] = ((uint32_t)(dirty_region->y) >> 8) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[1].payload[2] = ((uint32_t)(dirty_region->y)) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[1].payload[3] = ((uint32_t)(dirty_region->y + dirty_region->h - 1) >> 8) & 0xff;
    lcdkit_info.panel_infos.display_region_cmds.cmds[1].payload[4] = (dirty_region->y + dirty_region->h - 1) & 0xff;

    lcdkit_dump_cmds(&lcdkit_info.panel_infos.display_region_cmds);
    lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.display_region_cmds);

    return ret;
}

static void lcdkit_vsp_enable(bool en)
{
    LCDKIT_INFO("vsp enable(%d)\n", en);

    if(lcdkit_bias_is_gpio_ctrl_power())
    {
        if(en)
            gpio_direction_output(lcdkit_info.panel_infos.gpio_lcd_vsp, 1);
        else
            gpio_direction_output(lcdkit_info.panel_infos.gpio_lcd_vsp, 0);
    }
    else if(lcdkit_bias_is_regulator_ctrl_power())
    {
        if(en)
        {
//            gpio_direction_output(volt.vsp_gpio, 1);
               ;
        }
        else
        {
//            gpio_direction_output(volt.vsp_gpio, 0);
             ;
        }
    }
}

static void lcdkit_vsn_enable(bool en)
{
    LCDKIT_INFO("vsn enable(%d)\n", en);

    if(en)
        gpio_direction_output(lcdkit_info.panel_infos.gpio_lcd_vsn, 1);
    else
        gpio_direction_output(lcdkit_info.panel_infos.gpio_lcd_vsn, 0);
}

static void lcdkit_vddio_enable(bool en)
{
    LCDKIT_INFO("obsolescent\n");

}

static void lcdkit_vci_enable(bool en)
{
    LCDKIT_INFO("obsolescent\n");
}

static int lcdkit_fake_panel_bl_update(void *pdata, uint32_t bl_level)
{
    int ret = 0;
    ret = lcdkit_fake_update_bl(pdata,bl_level);
    return ret;
}

static ssize_t  lcdkit_voltage_enable_store(void *pdata, const char* buf)
{
    char command[10] = {0};

    uint32_t bl_value = 0;
    int ret = 0;
    char bl_value_buf[10] = {0};

    if (strlen(buf) > (sizeof(command)-1)){
        LCDKIT_ERR("buf underflow\n");
        return -EINVAL;
    }

    if (!sscanf(buf, "%s", command)) {
        LCDKIT_INFO("bad command(%s)\n", command);
        return -EINVAL;
    }

    LCDKIT_INFO("command(%s)\n", command);
    if (!strncmp("vsp:", command, strlen("vsp:"))) {
        if('0' == command[strlen("vsp:")])
            lcdkit_vsp_enable(false);
        else
            lcdkit_vsp_enable(true);
    }

    if (!strncmp("vsn:", command, strlen("vsn:"))) {
        if('0' == command[strlen("vsn:")])
            lcdkit_vsn_enable(false);
        else
            lcdkit_vsn_enable(true);
    }

    if (!strncmp("vddio:", command, strlen("vddio:"))) {
        if('0' == command[strlen("vddio:")])
            lcdkit_vddio_enable(false);
        else
            lcdkit_vddio_enable(true);
    }

    if (!strncmp("vci:", command, strlen("vci:"))) {
        if('0' == command[strlen("vci:")])
            lcdkit_vci_enable(false);
        else
            lcdkit_vci_enable(true);
    }

    if (!strncmp("bl:", command, strlen("bl:"))) {
        if (((strlen(command) - strlen("bl:")) > 0) &&((strlen(command) - strlen("bl:")) < sizeof(bl_value_buf))) {
            memcpy(bl_value_buf, &command[strlen("bl:")], strlen(command)-strlen("bl:"));
            bl_value = simple_strtoul(bl_value_buf, NULL, 0);
            LCDKIT_INFO("bl_value_buf is %s, bl_value is %d\n", bl_value_buf, bl_value);
        } else {
            bl_value = 0;
        }

        ret = lcdkit_fake_panel_bl_update(pdata, bl_value);
    }

    return ret;
}

static ssize_t lcdkit_reg_read_show(void* pdata, char *buf)
{
    char lcd_reg_buf[LCD_REG_LENGTH_MAX] = {0};
    uint32_t read_value[LCD_REG_LENGTH_MAX] = {0};
    unsigned char str_tmp[LCD_REG_LENGTH_MAX] = {0};
    char lcd_reg[] = {0x00};
    int i = 0;
    int ret = 0;
    struct lcdkit_dsi_cmd_desc lcd_reg_cmd[] = {
        {DTYPE_GEN_READ1, 0, 0, 0, 10, LCDKIT_WAIT_TYPE_US,
        sizeof(lcd_reg), lcd_reg},
    };

    for (i = 0; i < lcdkit_info.panel_infos.gama_correct_reg.cnt; i++) {
        if ((lcdkit_info.panel_infos.gama_reg_addr == lcdkit_info.panel_infos.gama_correct_reg.buf[i]) &&
                lcdkit_info.panel_infos.gama_reg_length == lcdkit_info.panel_infos.gama_reg_len.buf[i]) {
            break;
        }
    }
    if (i == lcdkit_info.panel_infos.gama_correct_reg.cnt) {
        LCDKIT_ERR("reg is not expect, reg:0x%x\n", lcdkit_info.panel_infos.gama_reg_addr);
        goto error_out;
    }
    for (i = 0; i < (int)(sizeof(lcdkit_adapt_func_array)/sizeof(lcdkit_adapt_func_array[0])); i++) {
        if (!strncmp(lcdkit_info.panel_infos.panel_name, lcdkit_adapt_func_array[i].name, strlen(lcdkit_adapt_func_array[i].name))) {
            if (lcdkit_adapt_func_array[i].lcdkit_reg_read_show) {
                ret = lcdkit_adapt_func_array[i].lcdkit_reg_read_show(pdata, buf);
                return ret;
            }
        }
    }

    memset(lcd_reg_buf, 0, sizeof(lcd_reg_buf));
    memset(read_value, 0, sizeof(read_value));
    lcd_reg[0] = lcdkit_info.panel_infos.gama_reg_addr;
    if (lcdkit_info.panel_infos.gama_cmd_page != 0) {
        lcdkit_info.panel_infos.lcd_reg_check_enter_cmds.cmds->payload[1] = lcdkit_info.panel_infos.gama_cmd_page & 0xFF;
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.lcd_reg_check_enter_cmds);
    }

    ret = lcdkit_lread_reg(pdata, read_value, lcd_reg_cmd, lcdkit_info.panel_infos.gama_reg_length);
    if (ret) {
        LCDKIT_INFO("read error, ret=%d\n", ret);
        goto error_out;
    }
    if (lcdkit_info.panel_infos.gama_cmd_page != 0) {
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.lcd_reg_check_exit_cmds);
    }
    for (i = 0; i < (lcdkit_info.panel_infos.gama_reg_length + 3) / 4; i++) {
        LCDKIT_INFO("0x%8x\n", read_value[i]);
    }
    snprintf(lcd_reg_buf, sizeof(lcd_reg_buf), "1,");
    for (i = 0; i < lcdkit_info.panel_infos.gama_reg_length; i++) {
        switch (i % 4) {
        case 0:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", read_value[i / 4] & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", read_value[i / 4] & 0xFF);
            }
            break;
        case 1:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 8) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 8) & 0xFF);
            }
            break;
        case 2:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 16) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 16) & 0xFF);
            }
            break;
        case 3:
            if (i == lcdkit_info.panel_infos.gama_reg_length - 1) {
                snprintf(str_tmp, sizeof(str_tmp), "%d", (read_value[i / 4] >> 24) & 0xFF);
            } else {
                snprintf(str_tmp, sizeof(str_tmp), "%d,", (read_value[i / 4] >> 24) & 0xFF);
            }
            break;
        default:
            break;
        }
        strncat(lcd_reg_buf, str_tmp, strlen(str_tmp));
    }
    LCDKIT_INFO("%s\n", lcd_reg_buf);
    ret = snprintf(buf, sizeof(lcd_reg_buf), "%s\n", lcd_reg_buf);
    return ret;
error_out:
    LCDKIT_INFO("error out, reg addr=%d, reg length=%d\n", lcdkit_info.panel_infos.gama_reg_addr, lcdkit_info.panel_infos.gama_reg_length);
    ret = snprintf(buf, PAGE_SIZE, "0,%d,%d\n", (int)lcdkit_info.panel_infos.gama_reg_addr, lcdkit_info.panel_infos.gama_reg_length);
    return ret;
}
static ssize_t lcdkit_reg_read_store(void* pdata, const char *buf)
{
    int ret = 0;
    unsigned int reg_value[100];
    char *cur;
    char *token;
    int i = 0;
    cur = (char*)buf;
    token = strsep(&cur, ",");
    while (token) {
        reg_value[i++] = simple_strtol(token, NULL, 0);
        token = strsep(&cur, ",");
        if (i >= 100)
        {
            LCDKIT_INFO("count is too long\n");
            return -1;
        }
    }
    lcdkit_info.panel_infos.gama_cmd_page = ((reg_value[0] >> 8) & 0xFF);
    lcdkit_info.panel_infos.gama_reg_addr = (unsigned char)(reg_value[0] & 0xFF);
    lcdkit_info.panel_infos.gama_reg_length = reg_value[1];
    LCDKIT_INFO("cmd page:0x%x, reg addr:0x%x, reg length:%d\n", lcdkit_info.panel_infos.gama_cmd_page, lcdkit_info.panel_infos.gama_reg_addr, lcdkit_info.panel_infos.gama_reg_length);
    return ret;
}
static ssize_t lcdkit_bl_mode_store(void* pdata, const char *buf)
{
    ssize_t ret = -1;
    unsigned long val = 0;
    unsigned int bl_work_mode = 0;

    if (NULL == buf)
    {
        return ret;
    }

    if (lcdkit_info.panel_infos.bl_support_mode)
    {
        ret = strict_strtoul(buf, 0, &val);
        if (ret)
        {
            LCDKIT_ERR("get bl mode fail!\n");
            return ret;
        }

        bl_work_mode = (unsigned int)val;

        LCDKIT_INFO("lcdkit_info.bl_work_mode=%u, val=%ld, bl_work_mode=%u\n", lcdkit_info.panel_infos.bl_work_mode, val,bl_work_mode);
        if (lcdkit_info.panel_infos.bl_work_mode == bl_work_mode)
        {
            LCDKIT_INFO("It is same bl mode!\n");
        }

        switch (val)
        {
            case LCDKIT_BL_NORMAL_MODE:
                ret = lcdkit_set_bl_normal_mode_reg(pdata);
                break;
            case LCDKIT_BL_ENHANCE_MODE:
                ret = lcdkit_set_bl_enhance_mode_reg(pdata);
                break;
            default:
                return 0;
        }
		if (!ret) {
			lcdkit_info.panel_infos.bl_work_mode = bl_work_mode;
		}
    }
    return ret;
}

static ssize_t lcdkit_bl_mode_show(char *buf)
{
    if (NULL == buf)
    {
        return -1;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.bl_work_mode);
}

static ssize_t lcdkit_se_mode_show(char* buf)
{
    if (NULL == buf)
    {
        return -1;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.se_mode);
}
static ssize_t lcdkit_se_mode_store(void* pdata, const char* buf)
{
    int ret = 0;
    unsigned long val = 0;

    if (NULL == buf)
    {
        return -EINVAL;
    }

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL pointer\n");
        return -EINVAL;
    }
    ret = strict_strtoul(buf, 0, &val);

    if (ret)
    {
        LCDKIT_ERR("get se mode fail!\n");
        return -EINVAL;
    }

    switch (val)
    {
        case SE_OFF :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.se_off_cmds);
            break;

        case SE_HD_MODE :
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.se_hd_cmds);
            break;

        case SE_FHD_MODE:
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.se_fhd_cmds);
            break;

        default:
            return -EINVAL;
    }

    lcdkit_info.panel_infos.se_mode = (int)val;
    LCDKIT_INFO("se_mode = %d\n", lcdkit_info.panel_infos.se_mode);

    return ret;
}
static ssize_t lcdkit_support_bl_mode_show(void* pdata, char* buf)
{
    ssize_t ret = -1;
    int timmer = 0;
    if (NULL == buf)
    {
        return -1;
    }
    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL pointer\n");
        return -EINVAL;
    }
	if (lcdkit_info.panel_infos.bl_support_mode) {
		ret = lcdkit_get_bl_resume_timmer(pdata, &timmer);
		if (!ret && (timmer > MIN_BL_RESUME_TIMMER)){
			return snprintf(buf, PAGE_SIZE, "%d\n", timmer);
		}
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", lcdkit_info.panel_infos.bl_support_mode);
}

static ssize_t lcdkit_oem_info_show(void* pdata, char* buf)
{
    if (lcdkit_info.panel_infos.is_hostprocessing == 1 || (lcdkit_info.panel_infos.effect_support_mode & BITS(31))) {
        return host_panel_oem_info_show(pdata, buf);
    } else {
        LCDKIT_INFO("It is normal panel!\n");
    }
    return 0;
}

static ssize_t lcdkit_oem_info_store(void* pdata, const char* buf)
{
    if (lcdkit_info.panel_infos.is_hostprocessing == 1 || (lcdkit_info.panel_infos.effect_support_mode & BITS(31))) {
        return host_panel_oem_info_store(pdata, buf);
    } else {
        LCDKIT_INFO("It is normal panel!\n");
    }
    return 0;
}

#define MIPI_CLK_HIGH "mipi_clk:high"
#define MIPI_CLK_LOW "mipi_clk:low"
#define MIPI_CLK_NORMAL "mipi_clk:normal"
static ssize_t lcdkit_mipi_config_store(void* pdata, const char* buf)
{
    if (NULL == pdata || NULL == buf)
    {
        LCDKIT_ERR("%s: NULL Pointer!\n",__func__);
        return -EINVAL;
    }
#ifndef CONFIG_ARCH_QCOM
    struct hisi_fb_data_type* hisifd = (struct hisi_fb_data_type*) pdata;
    struct hisi_panel_info *pinfo = &(hisifd->panel_info);

    if (!strncmp(buf, MIPI_CLK_HIGH, sizeof(MIPI_CLK_HIGH)))
    {
        hisifd->panel_info.dsi_bit_clk_upt_support = 1;
        pinfo->mipi.dsi_bit_clk_upt = lcdkit_info.panel_infos.mipi_clk_config_high;
    } else if ( !strncmp(buf, MIPI_CLK_LOW, sizeof(MIPI_CLK_LOW)))
    {
        hisifd->panel_info.dsi_bit_clk_upt_support = 1;
        pinfo->mipi.dsi_bit_clk_upt = lcdkit_info.panel_infos.mipi_clk_config_low;
    } else if ( !strncmp(buf, MIPI_CLK_NORMAL, sizeof(MIPI_CLK_NORMAL)))
    {
        hisifd->panel_info.dsi_bit_clk_upt_support = 1;
        pinfo->mipi.dsi_bit_clk_upt = lcdkit_info.panel_infos.mipi_clk_config_normal;
    } else
    {
        LCDKIT_ERR("mipi_clk_config error!\n");
        return -EINVAL;
    }
#endif
    return 0;
}

static BLOCKING_NOTIFIER_HEAD(lcdkit_init_notifier_list);
int lcdkit_register_notifier(struct notifier_block* nb)
{
	return blocking_notifier_chain_register(&lcdkit_init_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(lcdkit_register_notifier);
int lcdkit_unregister_notifier(struct notifier_block* nb)
{
	return blocking_notifier_chain_unregister(&lcdkit_init_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(lcdkit_unregister_notifier);
void lcdkit_notifier_call_chain(unsigned long event, void* data)
{
	LCDKIT_INFO("lcdkit notify event:%d\n", event);
	blocking_notifier_call_chain(&lcdkit_init_notifier_list, event, data);
}
EXPORT_SYMBOL_GPL(lcdkit_notifier_call_chain);

struct lcdkit_panel_data lcdkit_info =
{
    .lcdkit_init = lcdkit_init,
    .lcdkit_on_cmd  = lcdkit_on_cmd,
    .lcdkit_off_cmd = lcdkit_off_cmd,
    .lcdkit_model_show = lcdkit_model_show,
    .lcdkit_display_type_show = lcdkit_display_type_show,
    .lcdkit_panel_info_show =  lcdkit_panel_info_show,
    .lcdkit_cabc_mode_show = lcdkit_cabc_mode_show,
    .lcdkit_cabc_mode_store = lcdkit_cabc_mode_store,
    .lcdkit_inversion_mode_store = lcdkit_inversion_mode_store,
    .lcdkit_inversion_mode_show = lcdkit_inversion_mode_show,
    .lcdkit_scan_mode_store = lcdkit_scan_mode_store,
    .lcdkit_scan_mode_show = lcdkit_scan_mode_show,
    .lcdkit_check_reg_show = lcdkit_check_reg_show,
    .lcdkit_check_esd  = lcdkit_check_esd,
    .lcdkit_gram_check_show = lcdkit_gram_check_show,
    .lcdkit_gram_check_store = lcdkit_gram_check_store,
    .lcdkit_dynamic_sram_check_show = lcdkit_dynamic_sram_check_show,
    .lcdkit_dynamic_sram_check_store = lcdkit_dynamic_sram_check_store,
    .lcdkit_sleep_ctrl_show = lcdkit_sleep_ctrl_show,
    .lcdkit_sleep_ctrl_store = lcdkit_sleep_ctrl_store,
    .lcdkit_lp2hs_mipi_check_show = lcdkit_lp2hs_mipi_check_show,
    .lcdkit_lp2hs_mipi_check_store = lcdkit_lp2hs_mipi_check_store,
    .lcdkit_bist_check_show = lcdkit_bist_check_show,
    .lcdkit_mipi_detect_show = lcdkit_mipi_detect_show,
    .lcdkit_hkadc_debug_show = lcdkit_hkadc_debug_show,
    .lcdkit_hkadc_debug_store = lcdkit_hkadc_debug_store,
    .lcdkit_voltage_enable_store = lcdkit_voltage_enable_store,
    .lcdkit_pcd_errflag_check = lcdkit_pcd_errflag_check,
    .lcdkit_amoled_acl_ctrl_show = lcdkit_amoled_acl_ctrl_show,
    .lcdkit_amoled_acl_ctrl_store = lcdkit_amoled_acl_ctrl_store,
    .lcdkit_amoled_vr_mode_show = lcdkit_amoled_vr_mode_show,
    .lcdkit_amoled_vr_mode_store = lcdkit_amoled_vr_mode_store,
    .lcdkit_amoled_hbm_ctrl_show = lcdkit_amoled_hbm_ctrl_show,
    .lcdkit_amoled_hbm_ctrl_store = lcdkit_amoled_hbm_ctrl_store,
    .lcdkit_oem_info_show = lcdkit_oem_info_show,
    .lcdkit_oem_info_store = lcdkit_oem_info_store,
    .lcdkit_support_mode_show = lcdkit_support_mode_show,
    .lcdkit_support_mode_store = lcdkit_support_mode_store,
    .lcdkit_support_checkmode_show = lcdkit_support_checkmode_show,
    .lcdkit_test_config_show = lcdkit_test_config_show,
    .lcdkit_test_config_store = lcdkit_test_config_store,
    .lcdkit_fps_scence_handle = lcdkit_fps_scence_handle,
    .lcdkit_fps_updt_handle = lcdkit_fps_updt_handle,
    .lcdkit_set_display_region = lcdkit_set_display_region,
    .lcdkit_current_detect = lcdkit_current_detect,
    .lcdkit_lv_detect = lcdkit_lv_detect,
    .lcdkit_ce_mode_show = lcdkit_ce_mode_show,
    .lcdkit_ce_mode_store = lcdkit_ce_mode_store,
    .lcdkit_reg_read_show = lcdkit_reg_read_show,
    .lcdkit_reg_read_store = lcdkit_reg_read_store,
    .lcdkit_fps_timer_init = lcdkit_fps_timer_init,
    .lcdkit_bl_mode_show   = lcdkit_bl_mode_show,
    .lcdkit_bl_mode_store  = lcdkit_bl_mode_store,
    .lcdkit_se_mode_show = lcdkit_se_mode_show,
    .lcdkit_se_mode_store = lcdkit_se_mode_store,
    .lcdkit_support_bl_mode_show = lcdkit_support_bl_mode_show,
    .lcdkit_mipi_config_store = lcdkit_mipi_config_store,
};
