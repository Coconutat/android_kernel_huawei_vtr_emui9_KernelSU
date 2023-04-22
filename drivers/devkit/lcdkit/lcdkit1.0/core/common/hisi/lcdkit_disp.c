#include "hisi_fb.h"
#include <huawei_ts_kit.h>
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory
#include "lcdkit_effect.h"
#include "lcdkit_panel.h"
#include "lcdkit_disp.h"
#include "lcdkit_dbg.h"
#include <soc_sctrl_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <huawei_platform/log/log_jank.h>
#include "lcdkit_btb_check.h"
#include "lcdkit_ext.h"
#include "global_ddr_map.h"
#include "lcdkit_bias_ic_common.h"
#include "lcdkit_backlight_ic_common.h"
#if defined(CONFIG_LCDKIT_DRIVER)
#include "lcdkit_fb_util.h"
#endif
#include "lcdkit_tp.h"

extern struct ts_kit_platform_data g_ts_kit_platform_data;
struct platform_device *g_hisi_pdev=NULL;
struct lcdkit_private_info g_lcdkit_pri_info;
extern bool isbulcked;
volatile int lcdkit_brightness_ddic_info = 0;
extern char lcdkit_panel_name[LCDKIT_MAX_PANEL_NAME_LEN];
char read_temp[20]={0};
int g_max_backlight_from_app = MAX_BACKLIGHT_FROM_APP;
int g_min_backlight_from_app = MIN_BACKLIGHT_FROM_APP;
struct timer_list backlight_second_timer;

/* for backlight print count when power on*/
#define BACKLIGHT_PRINT_TIMES	10
static int g_backlight_count;
int g_record_project_id_err = 0;

/* This parameter used for controling lcd power on and power off */
static bool g_lcd_prox_enable;

/* set global variables of td4336 */
uint32_t g_read_value_td4336[19] = {0};
static int find_cmd_by_mipi_clk(struct hisi_fb_data_type *hisifd, uint32_t clk_val, struct lcdkit_dsi_panel_cmds **snd_cmds);

void lcdkit_set_pdev(struct platform_device *pdev)
{
    g_hisi_pdev = pdev;
    return ;
}

void lcdkit_get_pdev(struct platform_device **pdev)
{
    *pdev = g_hisi_pdev;
    return ;
}

static void lcdkit_display_on_cmds_record_time(void)
{
    do_gettimeofday(&lcdkit_info.panel_infos.dis_on_cmds_record_tv);
    LCDKIT_INFO("display on at %lu seconds %lu mil seconds\n",
        lcdkit_info.panel_infos.dis_on_cmds_record_tv.tv_sec,
        lcdkit_info.panel_infos.dis_on_cmds_record_tv.tv_usec);
    lcdkit_info.panel_infos.dis_on_cmds_panel_on_tag = true;
    return;
}
static void lcdkit_display_on_cmds_check_delay(void)
{
    u32 delta_time_backlight_to_panel_on = 0;
    u32 delay_margin = 0;
    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    do_gettimeofday(&tv);
    LCDKIT_INFO("set backlight at %lu seconds %lu mil seconds\n",
        tv.tv_sec, tv.tv_usec);
    delta_time_backlight_to_panel_on = (tv.tv_sec - lcdkit_info.panel_infos.dis_on_cmds_record_tv.tv_sec)*1000000
        + tv.tv_usec - lcdkit_info.panel_infos.dis_on_cmds_record_tv.tv_usec;
    delta_time_backlight_to_panel_on /= 1000;
    if (delta_time_backlight_to_panel_on >= lcdkit_info.panel_infos.dis_on_cmds_delay_margin_time)
    {
        LCDKIT_INFO("%lu > %lu, no need delay\n",
            delta_time_backlight_to_panel_on,
            lcdkit_info.panel_infos.dis_on_cmds_delay_margin_time);
        goto CHECK_DELAY_END;
    }
    delay_margin = lcdkit_info.panel_infos.dis_on_cmds_delay_margin_time
                                 - delta_time_backlight_to_panel_on;
    if (delay_margin > 200)
    {
        LCDKIT_INFO("something maybe error");
        goto CHECK_DELAY_END;
    }
    mdelay(delay_margin);
CHECK_DELAY_END:
    lcdkit_info.panel_infos.dis_on_cmds_panel_on_tag = false;
    return;
}
static void lcdkit_record_display_on_time_for_mipi_dsm(void)
{
    if (lcdkit_info.panel_infos.mipi_check_support){
        do_gettimeofday(&lcdkit_info.panel_infos.display_on_record_time);
        LCDKIT_INFO("display on at %lu seconds %lu mil seconds\n",
            lcdkit_info.panel_infos.display_on_record_time.tv_sec,
            lcdkit_info.panel_infos.display_on_record_time.tv_usec);
    }
    return;
}
static void lcdkit_display_on_new_seq_set_delay_time(void)
{
    static bool read_once = false;

    if (read_once){
        LCDKIT_INFO("display on new seq already set\n");
        return ;
    }

    if((tp_synaptics_ops)&&(tp_synaptics_ops->synaptics_tddi_power_seq)) {
        if(tp_synaptics_ops->synaptics_tddi_power_seq()){
          LCDKIT_INFO("panel display on new seq for synaptics DDIC\n");

          /* set reset H delay 20 ms */
          lcdkit_gpio_reset_normal_cmds[2].wait = 20;
          lcdkit_info.panel_infos.display_on_cmds.cmds[lcdkit_info.panel_infos.display_on_cmds.cmd_cnt-1].wait = 80;
        }
    }
    read_once = true;
    LCDKIT_INFO("set diaplay on new seq in first panel on\n");
    return ;
}

/*
*name:lcdkit_get_id
*function:power on panel
*@pdev:platform device
*/
int lcdkit_get_id(struct platform_device* pdev)
{
    int pulldown_value = 0;
    int pullup_value = 0;
    int lcd_status = 0;
    int lcd_id0 = 0;
    int lcd_id1 = 0;

    /*set gpio direction to out, set id0 to low*/
    gpio_cmds_tx(lcdkit_gpio_id0_low_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id0_low_cmds));
    /*set gpio direction to input*/
    gpio_cmds_tx(lcdkit_gpio_id0_input_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id0_input_cmds));
    /*read id0 value*/
    pulldown_value = gpio_get_value(lcdkit_info.panel_infos.gpio_lcd_id0);

    /*set gpio direction to out, set id0 to high*/
    gpio_cmds_tx(lcdkit_gpio_id0_high_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id0_high_cmds));
    /*set gpio direction to input*/
    gpio_cmds_tx(lcdkit_gpio_id0_input_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id0_input_cmds));
    /*read id0 value*/
    pullup_value = gpio_get_value(lcdkit_info.panel_infos.gpio_lcd_id0);

    if (pulldown_value != pullup_value)
    {
        lcd_id0 = 2; //floating
    }
    else
    {
        lcd_id0 = pulldown_value; //high or low
    }

    /*set gpio direction to out, set id1 to low*/
    gpio_cmds_tx(lcdkit_gpio_id1_low_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id1_low_cmds));
    /*set gpio direction to input*/
    gpio_cmds_tx(lcdkit_gpio_id1_input_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id1_input_cmds));
    /*read id1 value*/
    pulldown_value = gpio_get_value(lcdkit_info.panel_infos.gpio_lcd_id1);

    /*set gpio direction to out, set id1 to low*/
    gpio_cmds_tx(lcdkit_gpio_id1_high_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id1_high_cmds));
    /*set gpio direction to input*/
    gpio_cmds_tx(lcdkit_gpio_id1_input_cmds, \
                 ARRAY_SIZE(lcdkit_gpio_id1_input_cmds));
    /*read id1 value*/
    pullup_value = gpio_get_value(lcdkit_info.panel_infos.gpio_lcd_id1);

    if (pulldown_value != pullup_value)
    {
        lcd_id1 = 2; //floating
    }
    else
    {
        lcd_id1 = pulldown_value; //high or low
    }

    lcd_status = (lcd_id0 | (lcd_id1 << 2));
    LCDKIT_INFO("lcd_id0:%d, lcd_id1:%d, lcd_status = 0x%x.\n", lcd_id0, lcd_id1, lcd_status);
    return lcd_status;
}

/*
*name:lcdkit_vcc_init
*function:init lcd vcc parameter
*@cmds:vcc cmds
*@cnt:vcc number
*/
static void lcdkit_power_init(struct vcc_desc* cmds , int cnt)
{
    int i = 0;
    struct vcc_desc* cm = NULL;
    cm = cmds;

    for (i = 0; i < cnt; i++)
    {
        if (cm->dtype == DTYPE_VCC_SET_VOLTAGE)
        {
            if (0 == strncmp(cm->id, LCDKIT_VCC_LCDANALOG_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcdanalog_vcc;
                cm->max_uV = lcdkit_info.panel_infos.lcdanalog_vcc;
            }
            else if (0 == strncmp(cm->id, LCDKIT_VCC_LCDIO_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcdio_vcc;
                cm->max_uV = lcdkit_info.panel_infos.lcdio_vcc;
            }
            else if (0 == strncmp(cm->id, LCDKIT_VCC_LCDVDD_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcdvdd_vcc;
                cm->max_uV = lcdkit_info.panel_infos.lcdvdd_vcc;
            }
            else if (0 == strncmp(cm->id, VCC_LCDKIT_BIAS_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcd_bias;
                cm->max_uV = lcdkit_info.panel_infos.lcd_bias;
            }
            else if (0 == strncmp(cm->id, VCC_LCDKIT_VSP_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcd_vsp;
                cm->max_uV = lcdkit_info.panel_infos.lcd_vsp;
            }
            else if (0 == strncmp(cm->id, VCC_LCDKIT_VSN_NAME, strlen(cm->id)))
            {
                cm->min_uV = lcdkit_info.panel_infos.lcd_vsn;
                cm->max_uV = lcdkit_info.panel_infos.lcd_vsn;
            }
        }

        cm++;
    }
}

static __maybe_unused int lcdkit_enter_ulps(struct hisi_fb_data_type* hisifd)
{
    BUG_ON(hisifd == NULL);

    /* switch to cmd mode */
    set_reg(hisifd->mipi_dsi0_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
    /* cmd mode: low power mode */
    set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
    set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
    set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

    /* disable generate High Speed clock */
    set_reg(hisifd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
    mipi_dsi_ulps_cfg(hisifd, 0);
    udelay(10);
    return 0;
}

static bool lcdkit_get_proxmity_enable(void)
{
	g_lcd_prox_enable = tp_get_prox_status();
	LCDKIT_INFO("The g_lcd_prox_enable is %d when lcd off!\n",
		g_lcd_prox_enable);
	return g_lcd_prox_enable;
}

static int lcdkit_is_enter_sleep_mode(void)
{
   return (ts_kit_gesture_func || g_tskit_pt_station_flag || g_tskit_fw_upgrade_flag);
}

static void lcdkit_bias_init(struct platform_device* pdev)
{
    ssize_t ret = 0;
    if (lcdkit_is_lcd_panel())
    {
        if (lcdkit_bias_is_regulator_ctrl_power())
        {
            /* lcd scharger vcc get*/
            ret = vcc_cmds_tx(pdev, lcdkit_scharger_bias_get_cmds, ARRAY_SIZE(lcdkit_scharger_bias_get_cmds));
            if (ret != 0)
            {
                LCDKIT_ERR("LCD scharger vcc get failed!\n");
            }
            /*init bias/vsp/vsn*/
            lcdkit_power_init(lcdkit_scharger_bias_set_cmds, ARRAY_SIZE(lcdkit_scharger_bias_set_cmds));
        }
    }
}

static ssize_t lcdkit_vcc_init(struct platform_device* pdev)
{
    ssize_t ret = 0;

    if (lcdkit_iovcc_is_regulator_ctrl_power())
    {
        lcdkit_power_init(lcdkit_io_vcc_init_cmds, ARRAY_SIZE(lcdkit_io_vcc_init_cmds));
        /*init lcdio vcc*/
        ret = vcc_cmds_tx(pdev, lcdkit_io_vcc_init_cmds,  ARRAY_SIZE(lcdkit_io_vcc_init_cmds));

        if (ret != 0)
        {
            LCDKIT_ERR("LCD vcc init failed!\n");
            return ret;
        }
    }

    if (lcdkit_vdd_is_regulator_ctrl_power())
    {
        lcdkit_power_init(lcdkit_vdd_init_cmds, ARRAY_SIZE(lcdkit_vdd_init_cmds));
        /*init lcdvdd vcc*/
        ret = vcc_cmds_tx(pdev, lcdkit_vdd_init_cmds,  ARRAY_SIZE(lcdkit_vdd_init_cmds));

        if (ret != 0)
        {
            LCDKIT_ERR("LCD vcc init failed!\n");
            return ret;
        }
    }

    /*init lcdanalog vcc*/
	 if	( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type)
     {
        lcdkit_power_init(lcdkit_vci_init_cmds,ARRAY_SIZE(lcdkit_vci_init_cmds));
        if(lcdkit_vci_is_regulator_ctrl_power())
        {
            ret = vcc_cmds_tx(pdev, lcdkit_vci_init_cmds, ARRAY_SIZE(lcdkit_vci_init_cmds));

            if (ret != 0)
            {
                LCDKIT_ERR("LCD vcc init failed!\n");
                return ret;
            }
        }
    }

    return ret;
}

static void lcdkit_backlight_gpio_request(struct platform_device *pdev)
{
    struct hisi_fb_data_type *hisifd = NULL;

    if (NULL == pdev) {
        HISI_FB_ERR("pdev is NULL");
        return;
    }
    hisifd = platform_get_drvdata(pdev);
    if (NULL == hisifd) {
        HISI_FB_ERR("hisifd is NULL");
        return;
    }
    if((hisifd->panel_info.bl_ic_ctrl_mode == BLPWM_MODE) || (hisifd->panel_info.bl_ic_ctrl_mode == COMMON_IC_MODE))
    {
        if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
        {
            if(lcdkit_info.panel_infos.lcd_suspend_bl_disable)
            {
                gpio_cmds_tx(lcdkit_bl_request_cmds,ARRAY_SIZE(lcdkit_bl_request_cmds));
            }
        }
    }
}

static void lcdkit_reset_delay_init(struct gpio_desc* cmds)
{
    struct gpio_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.reset_step1_H;
    cm++;
    cm->wait = lcdkit_info.panel_infos.reset_L;
    cm++;
    cm->wait = lcdkit_info.panel_infos.reset_step2_H;
}

static void lcdkit_reset_high_delay_init(struct gpio_desc* cmds)
{
    struct gpio_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.reset_step2_H;
}

static void lcdkit_bias_on_delay_init_regulator(struct vcc_desc* cmds)
{
    struct vcc_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.delay_af_bias_on;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsp_on;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsn_on;
}

static void lcdkit_bias_off_delay_init_regulator(struct vcc_desc* cmds)
{
    struct vcc_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsn_off;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsp_off;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_bias_off;
}

static void lcdkit_bias_on_delay_init_gpio(struct gpio_desc* cmds)
{
    struct gpio_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsp_on;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsn_on;
}

static void lcdkit_bias_off_delay_init_gpio(struct gpio_desc* cmds)
{
    struct gpio_desc* cm = NULL;

    cm = cmds;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsn_off;
    cm++;
    cm->wait = lcdkit_info.panel_infos.delay_af_vsp_off;
}
static void lcdkit_gpio_request(void)
{
    /*id0 && id1 gpio request*/
    gpio_cmds_tx(lcdkit_gpio_id_request_cmds, ARRAY_SIZE(lcdkit_gpio_id_request_cmds));

    /*bias request*/
    if (lcdkit_bias_is_gpio_ctrl_power())
    {
        gpio_cmds_tx(lcdkit_bias_request_cmds, ARRAY_SIZE(lcdkit_bias_request_cmds));
    }
}

int lcdkit_get_vsp_voltage(void)
{
    int i = 0;

    for(i = 0;i < sizeof(voltage_table) / sizeof(struct lcdkit_vsp_vsn_voltage);i ++) {
        if(voltage_table[i].voltage == lcdkit_info.panel_infos.lcd_vsp) {
            LCDKIT_INFO("vsp voltage:%ld\n",voltage_table[i].voltage);
            return (voltage_table[i].value);
        }
    }

    if (i >= sizeof(voltage_table) / sizeof(struct lcdkit_vsp_vsn_voltage)) {
        LCDKIT_ERR("not found vsp voltage, use default voltage:TPS65132_VOL_55\n");
    }
    return TPS65132_VOL_55;
}

int lcdkit_get_vsn_voltage(void)
{
    int i = 0;

    for(i = 0;i < sizeof(voltage_table) / sizeof(struct lcdkit_vsp_vsn_voltage);i ++) {
        if(voltage_table[i].voltage == lcdkit_info.panel_infos.lcd_vsn) {
            LCDKIT_INFO("vsn voltage:%ld\n",voltage_table[i].voltage);
            return (voltage_table[i].value);
        }
    }

    if (i >= sizeof(voltage_table) / sizeof(struct lcdkit_vsp_vsn_voltage)) {
        LCDKIT_ERR("not found vsn voltage, use default voltage:TPS65132_VOL_55\n");
    }
    return TPS65132_VOL_55;
}

static ssize_t lcdkit_pinctrl_init(struct platform_device* pdev)
{
    ssize_t ret = 0;
    // lcd pinctrl init
    ret = pinctrl_cmds_tx(pdev, lcdkit_pinctrl_init_cmds, ARRAY_SIZE(lcdkit_pinctrl_init_cmds));

    if (ret != 0)
    {
        LCDKIT_ERR("Init pinctrl failed, defer\n");
    }

    return ret;
}

static void lcdkit_bias_ic_common_enable(void)
{
    if (lcdkit_bias_is_ic_ctrl_power())
    {
        int ret = 0;
        struct lcd_bias_voltage_info *pbias_ic = NULL;
        LCDKIT_INFO("power ctrl by ic!\n");
        gpio_cmds_tx(lcdkit_bias_request_cmds, ARRAY_SIZE(lcdkit_bias_request_cmds));
        gpio_cmds_tx(lcdkit_bias_enable_cmds, ARRAY_SIZE(lcdkit_bias_enable_cmds));
        pbias_ic = lcdkit_get_lcd_bias_ic_info();
        if(NULL != pbias_ic)
        {
            if(pbias_ic->ic_type & BIAS_IC_RESUME_NEED_CONFIG)
             {
                 ret = lcdkit_bias_set_voltage();
                 if (ret < 0)
                 {
                     LCDKIT_INFO("lcd_bias_voltage_init failed\n");
                  }
              }
        }
    }
}
void lcdkit_backlight_bias_ic_power_on(void)
{
    struct lcdkit_bl_ic_info *tmp = NULL;

    tmp =  lcdkit_get_lcd_backlight_ic_info();
    if(tmp != NULL)
    {
        if(tmp->ic_type == BACKLIGHT_BIAS_IC)
        {
            if(lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode)
            {
                gpio_cmds_tx(lcdkit_bl_power_request_cmds,ARRAY_SIZE(lcdkit_bl_power_request_cmds));
                gpio_cmds_tx(lcdkit_bl_power_enable_cmds,ARRAY_SIZE(lcdkit_bl_power_enable_cmds));
            }
            if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
            {
                if(lcdkit_info.panel_infos.lcd_suspend_bl_disable)
                {
                    gpio_cmds_tx(lcdkit_bl_request_cmds,ARRAY_SIZE(lcdkit_bl_request_cmds));
                    gpio_cmds_tx(lcdkit_bl_enable_cmds,ARRAY_SIZE(lcdkit_bl_enable_cmds));
                }
            }
            LCDKIT_INFO("backlight_bias_ic power ctrl mode is %d  bl gpio ctrl mode is %d!\n",
            lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode,lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode);
            lcdkit_backlight_ic_inital();
        }
    }
}
void lcdkit_backlight_ic_power_on(void)
{
    struct lcdkit_bl_ic_info *tmp = NULL;

    tmp =  lcdkit_get_lcd_backlight_ic_info();
    if(tmp != NULL)
    {
        if(tmp->ic_type == BACKLIGHT_IC)
        {
            if(lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode)
            {
                gpio_cmds_tx(lcdkit_bl_power_request_cmds,ARRAY_SIZE(lcdkit_bl_power_request_cmds));
                gpio_cmds_tx(lcdkit_bl_power_enable_cmds,ARRAY_SIZE(lcdkit_bl_power_enable_cmds));
            }
            if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
            {
                if(lcdkit_info.panel_infos.lcd_suspend_bl_disable)
                {
                     gpio_cmds_tx(lcdkit_bl_request_cmds,ARRAY_SIZE(lcdkit_bl_request_cmds));
                     gpio_cmds_tx(lcdkit_bl_enable_cmds,ARRAY_SIZE(lcdkit_bl_enable_cmds));
                }
            }
            LCDKIT_INFO("backlight_ic power ctrl mode is %d  bl gpio ctrl mode is %d!\n",
            lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode,lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode);
            lcdkit_backlight_ic_inital();
#if defined (CONFIG_HUAWEI_DSM)
            lcdkit_backlight_ic_ovp_check();
#endif
        }
    }
}
static void lcdkit_backlight_ic_power_off(void)
{
    struct lcdkit_bl_ic_info *tmp = NULL;
    tmp =  lcdkit_get_lcd_backlight_ic_info();
    LCDKIT_INFO("backlight_ic  power off \n");
    if(tmp != NULL)
    {
        LCDKIT_INFO("backlight_ic_power_off  power ctrl mode is %d  bl gpio ctrl mode is %d bl suspend disable is %d!\n",
        lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode,lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode,lcdkit_info.panel_infos.lcd_suspend_bl_disable);
        lcdkit_backlight_ic_disable_device();
        if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
        {
            if(lcdkit_info.panel_infos.lcd_suspend_bl_disable)
            {
                gpio_cmds_tx(lcdkit_bl_disable_cmds, ARRAY_SIZE(lcdkit_bl_disable_cmds));
                gpio_cmds_tx(lcdkit_bl_free_cmds, ARRAY_SIZE(lcdkit_bl_free_cmds));
            }
        }
        if(lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode)
        {
            gpio_cmds_tx(lcdkit_bl_power_disable_cmds, ARRAY_SIZE(lcdkit_bl_power_disable_cmds));
            gpio_cmds_tx(lcdkit_bl_power_free_cmds, ARRAY_SIZE(lcdkit_bl_power_free_cmds));
        }
    }
}

static int jdi_panel_write(struct hisi_fb_data_type* hisifd)
{
    char reg_32[2] = {0x32, 0x00};
    char reg_33[2] = {0x33, 0x00};
    char reg_34[2] = {0x34, 0x00};
    char page2a[2] = {0xff, 0x2a};
    char page10[2] = {0xff, 0x10};
    char reg_fb[2] = {0xfb, 0x01};
    reg_32[1] = read_temp[0];
    reg_33[1] = read_temp[1];
    reg_34[1] = read_temp[2];
    char cmd1[2] = {0xff, 0x10};

    struct dsi_cmd_desc lcd_reg_32_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(reg_32), reg_32},
    };
    struct dsi_cmd_desc lcd_reg_33_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(reg_33), reg_33},
    };
    struct dsi_cmd_desc lcd_reg_34_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(reg_34), reg_34},
    };
    struct dsi_cmd_desc lcd_page2a_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(page2a), page2a},
    };
    struct dsi_cmd_desc lcd_page10_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(page2a), page2a},
    };
    struct dsi_cmd_desc lcd_reg_fb_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(reg_fb), reg_fb},
    };

    struct dsi_cmd_desc cmd1_code_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(cmd1), cmd1},
    };
    /*switch page2a*/
    mipi_dsi_cmds_tx(lcd_page2a_cmds, \
        ARRAY_SIZE(lcd_page2a_cmds), hisifd->mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_reg_fb_cmds, \
        ARRAY_SIZE(lcd_reg_fb_cmds), hisifd->mipi_dsi0_base);
    /*write reg 32,33,34*/
    mipi_dsi_cmds_tx(lcd_reg_32_cmds, \
        ARRAY_SIZE(lcd_reg_32_cmds), hisifd->mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_reg_33_cmds, \
        ARRAY_SIZE(lcd_reg_33_cmds), hisifd->mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_reg_34_cmds, \
        ARRAY_SIZE(lcd_reg_34_cmds), hisifd->mipi_dsi0_base);
    /*switch page10*/
    mipi_dsi_cmds_tx(cmd1_code_cmds, \
        ARRAY_SIZE(cmd1_code_cmds), hisifd->mipi_dsi0_base);
    return 0;
}

static int jdi_panel_read(void)
{
    int ret = 0;
    int i = 0;
    uint32_t read_back[2] = {0};
    uint8_t jdi_otp_id0 = 0x00;
    char lcd_reg[] = {0xa1};
    char reg_8f[] = {0x8f};
    char reg_90[] = {0x90};
    char reg_91[] = {0x91};
    char page20[2] = {0xff, 0x20};
    char page2a[2] = {0xff, 0x2a};
    char page10[2] = {0xff, 0x10};
    char reg_fb[2] = {0xfb, 0x01};
    static int read_flag = 0;
    struct hisi_fb_data_type *hisifd = NULL;
    char cmd1[2] = {0xff, 0x10};

    struct dsi_cmd_desc lcd_reg_cmd[] = {
        {DTYPE_GEN_READ1, 0, 1, WAIT_TYPE_MS,
            sizeof(lcd_reg), lcd_reg},
    };
    struct dsi_cmd_desc lcd_reg_8f_cmd[] = {
        {DTYPE_GEN_READ1, 0, 1, WAIT_TYPE_MS,
            sizeof(reg_8f), reg_8f},
    };
    struct dsi_cmd_desc lcd_reg_90_cmd[] = {
        {DTYPE_GEN_READ1, 0, 1, WAIT_TYPE_MS,
            sizeof(reg_90), reg_90},
    };
    struct dsi_cmd_desc lcd_reg_91_cmd[] = {
        {DTYPE_GEN_READ1, 0, 1, WAIT_TYPE_MS,
            sizeof(reg_91), reg_91},
    };
    struct dsi_cmd_desc lcd_page20_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(page20), page20},
    };
    struct dsi_cmd_desc lcd_page10_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(page2a), page2a},
    };
    struct dsi_cmd_desc lcd_reg_fb_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(reg_fb), reg_fb},
    };

    struct dsi_cmd_desc cmd1_code_cmds[] = {
        {DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
            sizeof(cmd1), cmd1},
    };

    hisifd = hisifd_list[PRIMARY_PANEL_IDX];
    /*read 0xa1 reg*/
        ret = mipi_dsi_lread_reg(read_back, lcd_reg_cmd, 5, hisifd->mipi_dsi0_base);
        if (ret) {
            LCDKIT_INFO("read error, ret=%d\n", ret);
            return ret;
        }
        LCDKIT_INFO("jdi  read_back[0] = 0x%x,\n",read_back[0]);
        LCDKIT_INFO("jdi  read_back[1] = 0x%x,\n",read_back[1]);
        jdi_otp_id0 = read_back[1] & 0xFF;
    if (0x0A == jdi_otp_id0) {/*switch page20*/
    mipi_dsi_cmds_tx(lcd_page20_cmds, \
        ARRAY_SIZE(lcd_page20_cmds), hisifd->mipi_dsi0_base);
    mipi_dsi_cmds_tx(lcd_reg_fb_cmds, \
        ARRAY_SIZE(lcd_reg_fb_cmds), hisifd->mipi_dsi0_base);
    /*read reg 8f,90,91*/
    mipi_dsi_lread_reg(read_back, lcd_reg_8f_cmd, 1, hisifd->mipi_dsi0_base);
               read_temp[0] =  read_back[0] & 0xFF;
        LCDKIT_INFO("jdi  lcd  8f  first=0x%x\n", read_back[0]);
    mipi_dsi_lread_reg(read_back, lcd_reg_90_cmd, 1, hisifd->mipi_dsi0_base);
               read_temp[1] =  read_back[0] & 0xFF;
        LCDKIT_INFO("jdi  lcd  90 first=0x%x\n", read_back[0]);
    mipi_dsi_lread_reg(read_back, lcd_reg_91_cmd, 1, hisifd->mipi_dsi0_base);
               read_temp[2] =  read_back[0] & 0xFF;
        LCDKIT_INFO("jdi  lcd  91  first=0x%x\n", read_back[0]);
    /*switch page10*/
    mipi_dsi_cmds_tx(cmd1_code_cmds, \
        ARRAY_SIZE(cmd1_code_cmds), hisifd->mipi_dsi0_base);
    }
    return 0;
}
static void sharp_panel_read(void)
{
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t read_value[2] = {0};
	uint8_t read_data = 0x00;
	int read_ret = 0;
	int back_len = 2;
	static char read_id0[] = {0xdb};
	struct dsi_cmd_desc read_reg[] = {
		{DTYPE_GEN_READ1, 0, 1, WAIT_TYPE_MS,
			sizeof(read_id0), read_id0},
	};

	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (hisifd == NULL) {
		LCDKIT_ERR("hisifd is NULL\n");
		return;
	}
	read_ret = mipi_dsi_lread_reg(read_value, read_reg, back_len, hisifd->mipi_dsi0_base);
	if (read_ret) {
		LCDKIT_INFO("read error, read_ret=%d\n", read_ret);
		return;
	}
	read_data = read_value[0] & 0xFF;
	LCDKIT_INFO(" sharp read reg = 0x%x\n", read_data);
	if (0x22 == read_data) {
		strncpy(lcdkit_panel_name,"SHARP_2LANE_NT36870_VN1_5P88_1440P_SV3",strlen("SHARP_2LANE_NT36870_VN1_5P88_1440P_SV3"));
	} else {
		if (strlen(lcdkit_info.panel_infos.panel_name) < LCDKIT_MAX_PANEL_NAME_LEN) {
			strncpy(lcdkit_panel_name,lcdkit_info.panel_infos.panel_name,strlen(lcdkit_info.panel_infos.panel_name));
		} else {
			strncpy(lcdkit_panel_name,lcdkit_info.panel_infos.panel_name,(LCDKIT_MAX_PANEL_NAME_LEN - 1));
		}
	}
	return;
}

static int lcdkit_panel_is_power_on(struct hisi_fb_data_type* hisifd)
{
    uint32_t temp = 0;
    int ret       = 0;
    char __iomem* sctrl_base = NULL;
    /*bit[8] = 1 : lcd power on
     *bit[8] = 0 : lcd power off*/
    sctrl_base = hisifd->sctrl_base;
    temp = inp32(sctrl_base + SCBAKDATA11);
    ret  = (temp & 0x100) >> 8;

    LCDKIT_INFO("inp32(SOC_SCTRL_SCBAKDATA11_ADDR(SOC_ACPU_SCTRL_BASE_ADDR))= 0x%x bit[8] = %d!\n",
                    temp, ret);
    return ret;
}

static void lcdkit_clear_sctrl_reg(struct hisi_fb_data_type* hisifd)
{
    uint32_t temp = 0;
    int ret       = 0;
    char __iomem* sctrl_base = NULL;
    /*bit[8] = 1 : lcd power on
     *bit[8] = 0 : lcd power off*/
    sctrl_base = hisifd->sctrl_base;
    temp = inp32(sctrl_base + SCBAKDATA11);
    temp &= ~(0x100);
    outp32(sctrl_base + SCBAKDATA11, temp);
    LCDKIT_INFO("outp32(SOC_SCTRL_SCBAKDATA11_ADDR(SOC_ACPU_SCTRL_BASE_ADDR), 0x%x)\n",temp);
}

static int lcdkit_vcc_and_tp_power_on(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_panel_info* pinfo = NULL;
    ssize_t ret = 0;
    int data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
    int data_for_ts_after_resume = LCDKIT_NO_SYNC_TIMEOUT;
    char* panel_name = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    pinfo = &(hisifd->panel_info);

    LCDKIT_INFO("Exit Aod mode, not running lcdkit_on, lcd_init_step %d . \n", pinfo->lcd_init_step);
    if (pinfo->lcd_init_step == LCD_INIT_POWER_ON)
    {
        if ((!lcdkit_is_enter_sleep_mode()) && (!g_lcd_prox_enable)) {
            if(lcdkit_info.panel_infos.iovcc_before_vci == 1)
            {
                /*For amoled, Iovcc is need to be pulled up before vci. */
                /* Iovcc  1.8V*/
                if ( lcdkit_iovcc_is_regulator_ctrl_power())
                {
                    vcc_cmds_tx(pdev, lcdkit_io_vcc_enable_cmds,
                        ARRAY_SIZE(lcdkit_io_vcc_enable_cmds));
                }
                /* Vci  3.3V */
                /*for incell panel, lcd ctrl tp VCI power*/
                if ( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type)
                {
                    if(lcdkit_vci_is_regulator_ctrl_power())
                    {
                        vcc_cmds_tx(pdev, lcdkit_regulator_vci_enable_cmds,
                            ARRAY_SIZE(lcdkit_regulator_vci_enable_cmds));
                    }
                }
            }
            else
            {
                /*For incell panel, lcd ctrl tp VCI power*/
                /* Vci  3.3V */
                if ( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type)
                {
                    if(lcdkit_vci_is_regulator_ctrl_power())
                    {
                        vcc_cmds_tx(pdev, lcdkit_regulator_vci_enable_cmds,
                            ARRAY_SIZE(lcdkit_regulator_vci_enable_cmds));
                    }
                }

                /*Iovcc 1.8V*/
                if ( lcdkit_iovcc_is_regulator_ctrl_power())
                {
                    vcc_cmds_tx(pdev, lcdkit_io_vcc_enable_cmds,
                        ARRAY_SIZE(lcdkit_io_vcc_enable_cmds));
                }
            }
            if (lcdkit_is_lcd_panel() && lcdkit_bias_is_regulator_ctrl_power())
            {
                //for lcd, vsp/vsn enable
                LCDKIT_INFO("power ctrl by regulator and the panel is lcd!\n");
                vcc_cmds_tx(NULL, lcdkit_scharger_bias_enable_cmds, \
                            ARRAY_SIZE(lcdkit_scharger_bias_enable_cmds));
            }
        }
        pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE)
    {
        if ( g_tskit_ic_type && lcdkit_info.panel_infos.ts_resume_ctrl_mode == LCDKIT_TS_RESUME_BEFORE_LCD_RST)
        {
            LCDKIT_INFO("Call ts first resume before lcd reset.\n");
            if(lcdkit_info.panel_infos.tp_resume_no_sync)
            {
                data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            else
            {
                data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
        }
        if ( g_tskit_ic_type && !lcdkit_info.panel_infos.ts_resume_ctrl_mode)
        {
            LCDKIT_INFO("call ts resume default\n");
            if(lcdkit_info.panel_infos.tp_resume_no_sync)
            {
                data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            else
            {
                data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
        }
        if ( g_tskit_ic_type && (lcdkit_info.panel_infos.ts_resume_ctrl_mode == LCDKIT_TS_RESUME_AFTER_DIS_ON))
        {
            LCDKIT_INFO("call ts resume after display on\n");
            if(lcdkit_info.panel_infos.tp_resume_no_sync)
            {
                data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            else
            {
                data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            if (ret)
            {
                LCDKIT_ERR("ts resume device err\n");
            }
            mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
        }
        pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
        panel_name = lcdkit_info.panel_infos.panel_model ? \
            lcdkit_info.panel_infos.panel_model : lcdkit_info.panel_infos.panel_name;
        LCDKIT_INFO("lcd name = %s.\n", panel_name);
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE)
    {
        if ( g_tskit_ic_type)
        {
            lcdkit_notifier_call_chain(LCDKIT_TS_AFTER_RESUME, &data_for_ts_after_resume);
        }
        lcdkit_clear_sctrl_reg(hisifd);
    }
    LCDKIT_INFO("Exit Aod mode, not running lcdkit_on,exit lcdkit_on lcd_init_step %d . \n", pinfo->lcd_init_step);
    return ret;
}

static int lcdkit_check_reg_on(struct hisi_fb_data_type* hisifd)
{
#if defined (CONFIG_HUAWEI_DSM)
    ssize_t ret = 0, i = 0;
    static struct lcd_reg_read_t lcd_status_reg[] = {
        {0x0A, 0x9C, 0xFF, "lcd power state"},
    };

    if(lcdkit_info.panel_infos.check_reg_on_value.buf
      && lcdkit_info.panel_infos.check_reg_expect_value.buf
      && lcdkit_info.panel_infos.check_reg_mask_value.buf)
    {
        if (ESD_RECOVER_STATE_START != hisifd->esd_recover_state)
        {
            for(i = 0; i < ARRAY_SIZE(lcd_status_reg); i++)
            {
                lcd_status_reg[i].reg_addr = lcdkit_info.panel_infos.check_reg_on_value.buf[i];
                lcd_status_reg[i].expected_value = lcdkit_info.panel_infos.check_reg_expect_value.buf[i];
                lcd_status_reg[i].read_mask = lcdkit_info.panel_infos.check_reg_mask_value.buf[i];
                LCDKIT_INFO("check_reg_on_value:%02x, check_reg_expect_value:%02x, check_reg_mask_value:%02x\n",
                        lcdkit_info.panel_infos.check_reg_on_value.buf[i],
                        lcdkit_info.panel_infos.check_reg_expect_value.buf[i],
                        lcdkit_info.panel_infos.check_reg_mask_value.buf[i]);
            }
            panel_check_status_and_report_by_dsm(lcd_status_reg, \
            ARRAY_SIZE(lcd_status_reg), hisifd->mipi_dsi0_base);
            LCDKIT_INFO("recovery:%u\n", lcd_status_reg[0].recovery);
            ret = lcd_status_reg[0].recovery;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        LCDKIT_ERR("buf null pointer\n");
        ret = 0;
    }
    return ret;
#else
    return 0;// return ok when undef DSM.
#endif
}

/*
*name:lcdkit_on
*function:power on panel
*@pdev:platform device
*/
static int lcdkit_on(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_panel_info* pinfo = NULL;
    struct lcdkit_dsi_panel_cmds* snd_mipi_clk_cmds = NULL;
    ssize_t ret = 0;
    ssize_t esd_recovery_times = 0;
	int data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
	int data_for_ts_after_resume = LCDKIT_NO_SYNC_TIMEOUT;
    char* panel_name = NULL;

    LCDKIT_INFO("enter!\n");

    LCDKIT_ASSERT((NULL != pdev), (-EINVAL));

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    g_backlight_count = BACKLIGHT_PRINT_TIMES;

    if(hisifd->aod_function)
    {
        LCDKIT_INFO("It is in AOD mode and should bypass lcdkit_on! \n");
        return ret;
    }

    pinfo = &(hisifd->panel_info);

    if(1 == lcdkit_panel_is_power_on(hisifd))
    {
        return lcdkit_vcc_and_tp_power_on(pdev);
    }
    /*for mipi clk debug*/
    if (is_lcdkit_mipiclk_enable())
    {
        hisifd->panel_info.mipi.dsi_bit_clk = get_lcdkit_mipiclk_dbg();
        hisifd->panel_info.mipi.dsi_bit_clk_upt = get_lcdkit_mipiclk_dbg();
    }

    if (lcdkit_is_lcd_panel())
    {
        /* for vsp vsn debug */
        if (is_lcdkit_vsp_vsn_enable() && lcdkit_bias_is_regulator_ctrl_power())
        {
            lcdkit_debug_set_vsp_vsn(lcdkit_scharger_bias_set_cmds, ARRAY_SIZE(lcdkit_scharger_bias_set_cmds));
            vcc_cmds_tx(NULL, lcdkit_scharger_bias_set_cmds, ARRAY_SIZE(lcdkit_scharger_bias_set_cmds));
        }
    }

    if (pinfo->lcd_init_step == LCD_INIT_POWER_ON)
    {
      if(CHECKED_ERROR == mipi_lcdkit_btb_check()) {
        LCDKIT_ERR("btb checked failed!");
      }
      if (lcdkit_info.panel_infos.panel_display_on_new_seq)
        lcdkit_display_on_new_seq_set_delay_time();
      LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "LCD_POWER_ON");

      if ((!lcdkit_is_enter_sleep_mode()) && (!g_lcd_prox_enable)) {
        if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
        {
	        lcdkit_backlight_bias_ic_power_on();
        }
        if(lcdkit_info.panel_infos.iovcc_before_vci == 1)
        {
            /*For amoled, Iovcc is need to be pulled up before vci. */
            /* Iovcc  1.8V*/
            if ( lcdkit_iovcc_is_regulator_ctrl_power())
            {
                vcc_cmds_tx(pdev, lcdkit_io_vcc_enable_cmds,
                    ARRAY_SIZE(lcdkit_io_vcc_enable_cmds));
            }
            else if (lcdkit_iovcc_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_iovcc_request_cmds, ARRAY_SIZE(lcdkit_iovcc_request_cmds));
                gpio_cmds_tx(lcdkit_iovcc_enable_cmds, ARRAY_SIZE(lcdkit_iovcc_enable_cmds));
            }

            mdelay(lcdkit_info.panel_infos.delay_af_iovcc_on);
            /* Vci  3.3V */
            /*for incell panel, lcd ctrl tp VCI power*/
            if ( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type)
            {
                if(lcdkit_vci_is_regulator_ctrl_power())
                {
                    vcc_cmds_tx(pdev, lcdkit_regulator_vci_enable_cmds,
                        ARRAY_SIZE(lcdkit_regulator_vci_enable_cmds));
                }
                else if ( lcdkit_vci_is_gpio_ctrl_power())
                {
                    gpio_cmds_tx(lcdkit_vci_request_cmds, ARRAY_SIZE(lcdkit_vci_request_cmds));
                    gpio_cmds_tx(lcdkit_vci_enable_cmds, ARRAY_SIZE(lcdkit_vci_enable_cmds));
                }
            }
            mdelay(lcdkit_info.panel_infos.delay_af_vci_on);
        }
        else
        {
            /*For incell panel, lcd ctrl tp VCI power*/
            /* Vci  3.3V */
            if ( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type)
            {
                if(lcdkit_vci_is_regulator_ctrl_power())
                {
                    vcc_cmds_tx(pdev, lcdkit_regulator_vci_enable_cmds,
                        ARRAY_SIZE(lcdkit_regulator_vci_enable_cmds));
                }
                else if ( lcdkit_vci_is_gpio_ctrl_power())
                {
                    gpio_cmds_tx(lcdkit_vci_request_cmds, ARRAY_SIZE(lcdkit_vci_request_cmds));
                    gpio_cmds_tx(lcdkit_vci_enable_cmds, ARRAY_SIZE(lcdkit_vci_enable_cmds));
                }
            }

            mdelay(lcdkit_info.panel_infos.delay_af_vci_on);

            /*Iovcc 1.8V*/
            if ( lcdkit_iovcc_is_regulator_ctrl_power())
            {
                vcc_cmds_tx(pdev, lcdkit_io_vcc_enable_cmds,
                    ARRAY_SIZE(lcdkit_io_vcc_enable_cmds));
            }
            else if (lcdkit_iovcc_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_iovcc_request_cmds, ARRAY_SIZE(lcdkit_iovcc_request_cmds));
                gpio_cmds_tx(lcdkit_iovcc_enable_cmds, ARRAY_SIZE(lcdkit_iovcc_enable_cmds));
            }

            mdelay(lcdkit_info.panel_infos.delay_af_iovcc_on);
        }

        if (lcdkit_vbat_is_gpio_ctrl_power())
        {
            gpio_cmds_tx(lcdkit_vbat_request_cmds, ARRAY_SIZE(lcdkit_vbat_request_cmds));
            gpio_cmds_tx(lcdkit_vbat_enable_cmds, ARRAY_SIZE(lcdkit_vbat_enable_cmds));
        }
        mdelay(lcdkit_info.panel_infos.delay_af_vbat_on);

        // lcd pinctrl normal
        pinctrl_cmds_tx(pdev, lcdkit_pinctrl_normal_cmds,
                        ARRAY_SIZE(lcdkit_pinctrl_normal_cmds));
        // lcd reset gpio request
        gpio_cmds_tx(lcdkit_gpio_reset_request_cmds, \
                     ARRAY_SIZE(lcdkit_gpio_reset_request_cmds));

        if (lcdkit_info.panel_infos.first_reset)
        {
            //the first reset HLH
            gpio_cmds_tx(lcdkit_gpio_reset_normal_cmds, \
                         ARRAY_SIZE(lcdkit_gpio_reset_normal_cmds));
        }

        if (lcdkit_bias_is_gpio_ctrl_power())
        {
            LCDKIT_INFO("power ctrl by gpio!\n");
            gpio_cmds_tx(lcdkit_bias_request_cmds, \
                         ARRAY_SIZE(lcdkit_bias_request_cmds));
            gpio_cmds_tx(lcdkit_bias_enable_cmds, \
                         ARRAY_SIZE(lcdkit_bias_enable_cmds));
        }

        lcdkit_bias_ic_common_enable();
        if (lcdkit_is_lcd_panel() && lcdkit_bias_is_regulator_ctrl_power())
        {
            //for lcd, vsp/vsn enable
            LCDKIT_INFO("power ctrl by regulator and the panel is lcd!\n");
            vcc_cmds_tx(NULL, lcdkit_scharger_bias_enable_cmds, \
                        ARRAY_SIZE(lcdkit_scharger_bias_enable_cmds));
        }
      }else
      {
            // lcd pinctrl normal
            pinctrl_cmds_tx(pdev, lcdkit_pinctrl_normal_cmds,
                        ARRAY_SIZE(lcdkit_pinctrl_normal_cmds));
            // lcd reset gpio request
            gpio_cmds_tx(lcdkit_gpio_reset_request_cmds, \
                     ARRAY_SIZE(lcdkit_gpio_reset_request_cmds));

            //The function of double click to wake do not open this switch
            if(lcdkit_info.panel_infos.idle2_lcd_reset_low)
            {
                if ( g_tskit_ic_type && lcdkit_info.panel_infos.idle2_lcd_reset_low == LCDKIT_TS_RESUME_BEFORE_LCD_RST)
                {
                     LCDKIT_INFO("TS first resume before lcd reset high.\n");
                     if(lcdkit_info.panel_infos.tp_resume_no_sync)
                     {
                        data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                        lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
                     }
                     else
                     {
                        data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                        lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
                     }
                     mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
                }
                gpio_cmds_tx(lcdkit_gpio_reset_high_cmds, \
                        ARRAY_SIZE(lcdkit_gpio_reset_high_cmds));
            }
            else if(lcdkit_info.panel_infos.idle2_lcd_on_reset_low_to_high)
            {
                //The function for novatck ic, reset must low to up for quit deep sleep
                gpio_cmds_tx(lcdkit_gpio_reset_low_cmds, \
                        ARRAY_SIZE(lcdkit_gpio_reset_low_cmds));
                msleep(lcdkit_info.panel_infos.delay_af_rst_off);
                gpio_cmds_tx(lcdkit_gpio_reset_high_cmds, \
                        ARRAY_SIZE(lcdkit_gpio_reset_high_cmds));
            }
      }
      if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
      {
	        lcdkit_backlight_ic_power_on();
      }
      pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE)
    {
        mdelay(lcdkit_info.panel_infos.delay_af_LP11);

lp_sequence_restart:
        if(!lcdkit_is_enter_sleep_mode() || (lcdkit_info.panel_infos.idle2_lcd_reset_low != LCDKIT_TS_RESUME_BEFORE_LCD_RST))
        {
            if ( g_tskit_ic_type && lcdkit_info.panel_infos.ts_resume_ctrl_mode == LCDKIT_TS_RESUME_BEFORE_LCD_RST)
            {
                LCDKIT_INFO("Call ts first resume before lcd reset.\n");
                if(lcdkit_info.panel_infos.tp_resume_no_sync)
                {
                    data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                    lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
                }
                else
                {
                    data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                    lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
                }
                mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
            }
        }
        // lcd gpio normal
        if (lcdkit_info.panel_infos.second_reset)
        {
            if(lcdkit_info.panel_infos.reset_pull_high_flag == 1)
            {
                //the second reset just pull high
                gpio_cmds_tx(lcdkit_gpio_reset_high_cmds, \
                    ARRAY_SIZE(lcdkit_gpio_reset_high_cmds));
            }
            else
            {
                //the second reset high_low_high
                gpio_cmds_tx(lcdkit_gpio_reset_normal_cmds, \
                    ARRAY_SIZE(lcdkit_gpio_reset_normal_cmds));
            }
        }

        if ( lcdkit_vdd_is_regulator_ctrl_power())
        {
            vcc_cmds_tx(pdev, lcdkit_vdd_enable_cmds,
                ARRAY_SIZE(lcdkit_vdd_enable_cmds));
        }
        mdelay(lcdkit_info.panel_infos.delay_af_vdd_on);

        if ( g_tskit_ic_type && !lcdkit_info.panel_infos.ts_resume_ctrl_mode)
        {
            LCDKIT_INFO("call ts resume default\n");
            if(lcdkit_info.panel_infos.tp_resume_no_sync)
            {
                data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            else
            {
                data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
        }

        if ( g_tskit_ic_type && lcdkit_info.panel_infos.tprst_before_lcdrst)
        {
            lcdkit_notifier_call_chain(LCDKIT_TS_AFTER_RESUME, &data_for_ts_after_resume);
        }
        if ( g_tskit_ic_type && lcdkit_info.panel_infos.tpfw_early_lcdinit)
        {
            lcdkit_notifier_call_chain(LCDKIT_TS_AFTER_RESUME, &data_for_ts_after_resume);
            mdelay(lcdkit_info.panel_infos.delay_af_tp_after_resume);
        }
        if (runmode_is_factory())
        {
            if (!strncmp(lcdkit_info.panel_infos.panel_name, "JDI_NT35696 5.1' CMD TFT 1920 x 1080", strlen(lcdkit_info.panel_infos.panel_name)))
            {
                ret = lcdkit_jdi_nt35696_5p5_gamma_reg_read(hisifd);
                if (ret)
                {
                    LCDKIT_ERR("read gamma reg err\n");
                }
            }
            if(lcdkit_info.panel_infos.dynamic_gamma_support)
            {
                if ((!strncmp(lcdkit_info.panel_infos.panel_name, "JDI_2LANE_NT36860 5.88' CMD TFT 1440 x 2560", strlen(lcdkit_info.panel_infos.panel_name)))||
                    (!strncmp(lcdkit_info.panel_infos.panel_name, "JDI_2LANE_NT36860 CUT2 5.88' CMD TFT 1440 x 2560", strlen(lcdkit_info.panel_infos.panel_name)))||
                    (!strncmp(lcdkit_info.panel_infos.panel_name, "JDI_NT36860 5.88' CMD TFT 1440 x 2560", strlen(lcdkit_info.panel_infos.panel_name))))
                {
                    ret = lcdkit_jdi_nt36860_5p88_gamma_reg_read(hisifd);
                    if (ret)
                    {
                        LCDKIT_ERR("read jdi_nt36860_5p88 gamma reg err!\n");
                    }
                }
            }
        }

        //for debug for inital_code
        if (is_lcdkit_initcode_enable())
        {
            LCDKIT_INFO("init code debug enter!\n");
            lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.g_panel_cmds);
        }
        else
        {
            lcdkit_info.lcdkit_on_cmd(hisifd, &lcdkit_info.panel_infos.display_on_cmds);

            if (lcdkit_info.panel_infos.dsi1_snd_cmd_panel_support) {
                mdelay(lcdkit_info.panel_infos.delay_af_display_on);
                lcdkit_info.lcdkit_on_cmd(hisifd, &lcdkit_info.panel_infos.display_on_second_cmds);
                LCDKIT_DEBUG("send second cmds ok !\n");
            }

            if (lcdkit_info.panel_infos.lcd_otp_support)
            {
              jdi_panel_write(hisifd);
            }
        }

        //send mipi clk cmds to panel when dsi_mipi_clk changed
        if (lcdkit_info.panel_infos.dsi_upt_snd_cmd_support)
        {
            ret = find_cmd_by_mipi_clk(hisifd, pinfo->mipi.dsi_bit_clk_upt, &snd_mipi_clk_cmds);
            if (ret == 0)
            {
                lcdkit_dsi_tx(hisifd, snd_mipi_clk_cmds);
            }
        }

        if ( g_tskit_ic_type && (lcdkit_info.panel_infos.ts_resume_ctrl_mode == LCDKIT_TS_RESUME_AFTER_DIS_ON))
        {
            LCDKIT_INFO("call ts resume after display on\n");
            if(lcdkit_info.panel_infos.tp_resume_no_sync)
            {
                data_for_ts_resume = LCDKIT_NO_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            else
            {
                data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
                lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            }
            if (ret)
            {
                LCDKIT_ERR("ts resume device err\n");
            }

            mdelay(lcdkit_info.panel_infos.delay_af_tp_reset);
        }

        if (lcdkit_info.panel_infos.dis_on_cmds_delay_margin_support)
        {
            lcdkit_display_on_cmds_record_time();
        }
        if(lcdkit_info.panel_infos.display_effect_on_support)
        {
            LCDKIT_INFO("display on effect is support!\n");
            lcdkit_dsi_tx(hisifd,&lcdkit_info.panel_infos.display_effect_on_cmds);
        }

        if (lcdkit_info.panel_infos.display_on_in_backlight)
        {
            lcdkit_info.panel_infos.display_on_need_send = true;
        }

        lcdkit_info.panel_infos.cabc_mode = CABC_UI_MODE;
        lcdkit_info.panel_infos.ce_mode = CE_SRGB_MODE;
        if (lcdkit_info.panel_infos.check_reg_on_support)
        {
            if (lcdkit_check_reg_on(hisifd) && esd_recovery_times < 3)
            {
                LCDKIT_ERR("==>0A error detect, go to restart lp sequence!!!\n");
                esd_recovery_times++;
                goto lp_sequence_restart;
            }
        }
        pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
        panel_name = lcdkit_info.panel_infos.panel_model ? \
            lcdkit_info.panel_infos.panel_model : lcdkit_info.panel_infos.panel_name;
        LCDKIT_INFO("lcd name = %s.\n", panel_name);
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE)
    {
        if(lcdkit_info.panel_infos.lp2hs_mipi_check_support)
        {
            lcdkit_lp2hs_mipi_test(hisifd);
        }

        if ( g_tskit_ic_type && !lcdkit_info.panel_infos.tprst_before_lcdrst && !lcdkit_info.panel_infos.tpfw_early_lcdinit)
        {
            lcdkit_notifier_call_chain(LCDKIT_TS_AFTER_RESUME, &data_for_ts_after_resume);
        }
    }
    else
    {
        LCDKIT_ERR("failed to init lcd!\n");
    }

    // backlight on
    hisi_lcd_backlight_on(pdev);

    if (lcdkit_is_oled_panel())
    {
        //	amoled_irq_enable();
    }

	/* record display on time for mipi error dmd report*/
	lcdkit_record_display_on_time_for_mipi_dsm();

    LCDKIT_INFO("fb%d, -!\n", hisifd->index);

    return 0;
}

static void lcdkit_remove_shield_backlight(void)
{
	if(lcdkit_info.panel_infos.bl_is_shield_backlight == true)
	{
		lcdkit_info.panel_infos.bl_is_shield_backlight = false;
	}
	if(false != lcdkit_info.panel_infos.bl_is_start_second_timer)
	{
		del_timer(&backlight_second_timer);
		lcdkit_info.panel_infos.bl_is_start_second_timer = false;
		LCDKIT_INFO("panel powerOff, clear backlight shield timer.\n");
	}
}

/*
*name:lcdkit_off
*function:power off panel
*@pdev:platform device
*/
static int lcdkit_off(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_panel_info* pinfo = NULL;
    ssize_t ret = 0;
	int data_for_notify_early_suspend = LCDKIT_SHORT_SYNC_TIMEOUT;
	int data_for_notify_suspend = LCDKIT_NO_SYNC_TIMEOUT;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

	LCDKIT_INFO("fb%d, +!\n", hisifd->index);
    if(hisifd->aod_function)
    {
        LCDKIT_INFO("It is in AOD mode and should bypass lcdkit_off and notify tp! \n");
        lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
        return ret;
    }
    lcdkit_remove_shield_backlight();

    if (NULL == hisifd->mipi_dsi0_base)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    pinfo = &(hisifd->panel_info);

    if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE)
    {
        if(lcdkit_info.panel_infos.mipi_check_support){
            lcdkit_mipi_check(hisifd);
        }
        LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "LCD_POWER_OFF");
		if (g_tskit_ic_type) {
			lcdkit_notifier_call_chain(LCDKIT_TS_EARLY_SUSPEND, NULL);
		}
        // backlight off
        hisi_lcd_backlight_off(pdev);
	if ( g_tskit_ic_type && lcdkit_info.panel_infos.tp_before_lcdsleep) {
		/*notify early suspend*/
		lcdkit_notifier_call_chain(LCDKIT_TS_BEFORE_SUSPEND, &data_for_notify_early_suspend);
		/*notify suspend*/
		lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
		msleep(lcdkit_info.panel_infos.delay_af_tp_before_suspend);
	}
        lcdkit_info.lcdkit_off_cmd(hisifd, &lcdkit_info.panel_infos.display_off_cmds);
        if (lcdkit_info.panel_infos.dsi1_snd_cmd_panel_support) {
            msleep(lcdkit_info.panel_infos.delay_af_display_off);
            lcdkit_info.lcdkit_off_cmd(hisifd, &lcdkit_info.panel_infos.display_off_second_cmds);
            msleep(lcdkit_info.panel_infos.delay_af_display_off_second);
        }

        if (lcdkit_info.panel_infos.display_on_in_backlight)
        {
            lcdkit_info.panel_infos.display_on_need_send = false;
        }

        if(!lcdkit_info.panel_infos.rst_after_vbat_flag)
        {
            if ( g_tskit_ic_type && !g_lcdkit_pri_info.power_off_simult_support && !lcdkit_info.panel_infos.tp_before_lcdsleep)
            {
                /*notify early suspend*/
                lcdkit_notifier_call_chain(LCDKIT_TS_BEFORE_SUSPEND, &data_for_notify_early_suspend);
                /*notify suspend*/
                if(!lcdkit_info.panel_infos.tp_after_lcd_reset){
                    lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
                }
            }
        }

        pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
    }
    else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE)
    {
        if(lcdkit_info.panel_infos.panel_ulps_support){
            lcdkit_enter_ulps(hisifd);
            LCDKIT_INFO("enter ulps mode!\n");
        }
        pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
    }
    else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF)
    {
    msleep(5);

    if ((!lcdkit_get_proxmity_enable()) && (!lcdkit_is_enter_sleep_mode())) {
        //if(g_ts_kit_platform_data.chip_data->is_parade_solution)
        //{
            //ts_kit_check_bootup_upgrade();
        //}

        /*reset shutdown before vsn disable*/
        if (!lcdkit_info.panel_infos.reset_shutdown_later)
        {
        // lcd reset gpio low
        gpio_cmds_tx(lcdkit_gpio_reset_low_cmds, \
                    ARRAY_SIZE(lcdkit_gpio_reset_low_cmds));
        msleep(lcdkit_info.panel_infos.delay_af_rst_off);

        // lcd reset gpio free
        gpio_cmds_tx(lcdkit_gpio_reset_free_cmds, \
                     ARRAY_SIZE(lcdkit_gpio_reset_free_cmds));

        // lcd pinctrl low
        pinctrl_cmds_tx(pdev, lcdkit_pinctrl_low_cmds,
                        ARRAY_SIZE(lcdkit_pinctrl_low_cmds));
        }

        if (g_tskit_ic_type && lcdkit_info.panel_infos.tp_after_lcd_reset)
        {
            /*notify suspend*/
            lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
            msleep(lcdkit_info.panel_infos.tp_befor_vsn_low_delay);
        }

        if (lcdkit_bias_is_gpio_ctrl_power())
        {
            LCDKIT_INFO("power is ctrol by gpio!\n");
            gpio_cmds_tx(lcdkit_bias_disable_cmds, \
                         ARRAY_SIZE(lcdkit_bias_disable_cmds));
                gpio_cmds_tx(lcdkit_bias_free_cmds, ARRAY_SIZE(lcdkit_bias_free_cmds));
        }

        if (lcdkit_bias_is_ic_ctrl_power())
        {
            LCDKIT_INFO("power is ctrol by ic!\n");
            gpio_cmds_tx(lcdkit_bias_disable_cmds, \
                         ARRAY_SIZE(lcdkit_bias_disable_cmds));
                gpio_cmds_tx(lcdkit_bias_free_cmds, ARRAY_SIZE(lcdkit_bias_free_cmds));
        }

        if (lcdkit_bias_is_regulator_ctrl_power())
        {
            //vsp/vsn disable
            vcc_cmds_tx(pdev, lcdkit_scharger_bias_disable_cmds, \
                        ARRAY_SIZE(lcdkit_scharger_bias_disable_cmds));
            LCDKIT_INFO("power is ctrol by regulator and the panel is lcd!\n");
        }

        if (lcdkit_vdd_is_regulator_ctrl_power())
        {
            vcc_cmds_tx(pdev, lcdkit_vdd_disable_cmds,
                        ARRAY_SIZE(lcdkit_vdd_disable_cmds));
        }

        if (g_lcdkit_pri_info.power_off_simult_support) {
            if (lcdkit_iovcc_is_regulator_ctrl_power())
            {
                vcc_cmds_tx(pdev, lcdkit_io_vcc_disable_cmds,
                            ARRAY_SIZE(lcdkit_io_vcc_disable_cmds));
            }
            else if ( lcdkit_iovcc_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_iovcc_disable_cmds, \
                             ARRAY_SIZE(lcdkit_iovcc_disable_cmds));
                gpio_cmds_tx(lcdkit_iovcc_free_cmds, ARRAY_SIZE(lcdkit_iovcc_free_cmds));
            }
        }
        if(lcdkit_info.panel_infos.rst_after_vbat_flag){
            if ( lcdkit_vbat_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_vbat_disable_cmds, \
                            ARRAY_SIZE(lcdkit_vbat_disable_cmds));
                gpio_cmds_tx(lcdkit_vbat_free_cmds, ARRAY_SIZE(lcdkit_vbat_free_cmds));
            }
            msleep(lcdkit_info.panel_infos.delay_af_vbat_off);

        }
        /*reset shutdown after vsp disable*/
        if (lcdkit_info.panel_infos.reset_shutdown_later)
        {
            // lcd reset gpio low
            gpio_cmds_tx(lcdkit_gpio_reset_low_cmds, \
                        ARRAY_SIZE(lcdkit_gpio_reset_low_cmds));
            if ( (g_tskit_ic_type && g_lcdkit_pri_info.power_off_simult_support) || (g_tskit_ic_type && lcdkit_info.panel_infos.rst_after_vbat_flag) )
            {
                /*notify early suspend*/
                lcdkit_notifier_call_chain(LCDKIT_TS_BEFORE_SUSPEND, &data_for_notify_early_suspend);
                /*notify suspend*/
                lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
            }

            // lcd reset gpio free
            gpio_cmds_tx(lcdkit_gpio_reset_free_cmds, \
                         ARRAY_SIZE(lcdkit_gpio_reset_free_cmds));

            // lcd pinctrl low
            pinctrl_cmds_tx(pdev, lcdkit_pinctrl_low_cmds,
                            ARRAY_SIZE(lcdkit_pinctrl_low_cmds));
        }
        msleep(lcdkit_info.panel_infos.delay_af_rst_off);
        if(!lcdkit_info.panel_infos.rst_after_vbat_flag)
        {
            if ( lcdkit_vbat_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_vbat_disable_cmds, \
                        ARRAY_SIZE(lcdkit_vbat_disable_cmds));
                gpio_cmds_tx(lcdkit_vbat_free_cmds, ARRAY_SIZE(lcdkit_vbat_free_cmds));
            }
            msleep(lcdkit_info.panel_infos.delay_af_vbat_off);
        }
		if (!g_lcdkit_pri_info.power_off_simult_support) {
			//lcd vcc disable
			if (lcdkit_iovcc_is_regulator_ctrl_power())
			{
				vcc_cmds_tx(pdev, lcdkit_io_vcc_disable_cmds,
						ARRAY_SIZE(lcdkit_io_vcc_disable_cmds));
			}
			else if ( lcdkit_iovcc_is_gpio_ctrl_power())
			{
				gpio_cmds_tx(lcdkit_iovcc_disable_cmds, \
						ARRAY_SIZE(lcdkit_iovcc_disable_cmds));
				gpio_cmds_tx(lcdkit_iovcc_free_cmds, ARRAY_SIZE(lcdkit_iovcc_free_cmds));
			}
		}

        if (lcdkit_info.panel_infos.delay_af_iovcc_off > 0) {
                msleep(lcdkit_info.panel_infos.delay_af_iovcc_off);
        } else {
                LCDKIT_INFO("No delay after iovcc off !\n");
        }

        if ( HYBRID ==  g_tskit_ic_type || AMOLED == g_tskit_ic_type )
        {
            if (lcdkit_vci_is_regulator_ctrl_power())
            {
                vcc_cmds_tx(pdev, lcdkit_regulator_vci_disable_cmds,
                            ARRAY_SIZE(lcdkit_regulator_vci_disable_cmds));
            }
            else if (lcdkit_vci_is_gpio_ctrl_power())
            {
                gpio_cmds_tx(lcdkit_vci_disable_cmds, \
                             ARRAY_SIZE(lcdkit_vci_disable_cmds));
                gpio_cmds_tx(lcdkit_vci_free_cmds, ARRAY_SIZE(lcdkit_vci_free_cmds));
            }

        }

        msleep(lcdkit_info.panel_infos.delay_af_vci_off);
        if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
        {
	        lcdkit_backlight_ic_power_off();
        }

    }else
    {
        if (g_tskit_ic_type && lcdkit_info.panel_infos.tp_after_lcd_reset)
        {
            /*notify suspend*/
            lcdkit_notifier_call_chain(LCDKIT_TS_SUSPEND_DEVICE, &data_for_notify_suspend);
        }

        //The function of double click to wake do not open this switch
        if (lcdkit_info.panel_infos.idle2_lcd_reset_low &&
		(!g_lcd_prox_enable)) {
            gpio_cmds_tx(lcdkit_gpio_reset_low_cmds, \
                    ARRAY_SIZE(lcdkit_gpio_reset_low_cmds));
        }

            // lcd reset gpio free
            gpio_cmds_tx(lcdkit_gpio_reset_free_cmds, \
                ARRAY_SIZE(lcdkit_gpio_reset_free_cmds));

            // lcd pinctrl low
            pinctrl_cmds_tx(pdev, lcdkit_pinctrl_low_cmds,
                ARRAY_SIZE(lcdkit_pinctrl_low_cmds));

        }
        if (hisifd->fb_shutdown)
        {
            if((tp_kit_ops)&&(tp_kit_ops->tp_thread_stop_notify)) {
                tp_kit_ops->tp_thread_stop_notify();
            }
        }
        if (lcdkit_info.panel_infos.fps_func_switch && pinfo->fps_updt_support){
            LCDKIT_INFO("fps_updt_support = 0, fps_updt_panel_only = 0\n");
            pinfo->fps_updt_support = 0;
            g_last_fps_scence = 0;
            pinfo->fps_updt_panel_only = 0;
        }
    }
    else
    {
        LCDKIT_ERR("failed to uninit lcd!\n");
    }

    //if(lcdkit_is_oled_panel())
    //  amoled_irq_disable();
    LCDKIT_INFO("fb%d, -!\n", hisifd->index);
    return 0;
}

/*
*name:lcdkit_remove
*function:panel remove
*@pdev:platform device
*/
static int lcdkit_remove(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (!hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return 0;
    }

    LCDKIT_DEBUG("fb%d, +.\n", hisifd->index);

    // lcd vcc finit
    vcc_cmds_tx(pdev, lcdkit_vcc_finit_cmds,
                ARRAY_SIZE(lcdkit_vcc_finit_cmds));

    if (lcdkit_is_lcd_panel())
    {
        ssize_t ret = 0;
        // scharger vcc finit
        ret = vcc_cmds_tx(pdev, lcdkit_scharger_bias_put_cmds,
                    ARRAY_SIZE(lcdkit_scharger_bias_put_cmds));
        if (ret != 0)
        {
            LCDKIT_ERR("LCD scharger vcc failed!\n");
        }
    }

    // lcd pinctrl finit
    pinctrl_cmds_tx(pdev, lcdkit_pinctrl_finit_cmds,
                    ARRAY_SIZE(lcdkit_pinctrl_finit_cmds));

    LCDKIT_DEBUG("fb%d, -.\n", hisifd->index);

    return 0;
}

static void lcdkit_backlight_enable_gpio_operation(uint32_t bl_ic_ctrl_mode, uint32_t backlight_level)
{
    static uint32_t last_bl_level = 255;

    if(bl_ic_ctrl_mode == BLPWM_MODE)
    {
        if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
        {
            if(lcdkit_info.panel_infos.lcd_suspend_bl_disable)
            {
                down(&lcdkit_info.panel_infos.bl_sem);
                if (backlight_level == 0 && last_bl_level != 0)
                {
                    gpio_cmds_tx(lcdkit_bl_disable_cmds, ARRAY_SIZE(lcdkit_bl_disable_cmds));
                    gpio_cmds_tx(lcdkit_bl_free_cmds, ARRAY_SIZE(lcdkit_bl_free_cmds));
                }
                else if (last_bl_level == 0 && backlight_level != 0)
                {
                    gpio_cmds_tx(lcdkit_bl_request_cmds,ARRAY_SIZE(lcdkit_bl_request_cmds));
                    gpio_cmds_tx(lcdkit_bl_enable_cmds,ARRAY_SIZE(lcdkit_bl_enable_cmds));
                }
                last_bl_level = backlight_level;
                up(&lcdkit_info.panel_infos.bl_sem);
            }
        }
    }
}

/*
*name:lcdkit_set_backlight
*function:set backlight level
*@pdev:platform device
*/
static int lcdkit_set_backlight(struct platform_device* pdev, uint32_t bl_level)
{
    int ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    static uint32_t last_bl_level = 255;
    static uint32_t jank_last_bl_level = 0;
    static uint32_t reg_value_61h;
    static uint32_t last_bl = 0;
    static int count = 0;
    struct hisi_panel_info* pinfo = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    if (hisifd->aod_mode && hisifd->aod_function)
    {
        LCDKIT_INFO("It is in AOD mode and should bypass lcdkit_set_backlight! \n");
        return 0;
    }
    if(true == lcdkit_info.panel_infos.bl_is_shield_backlight)
    {
        LCDKIT_ERR("It is in finger down status, Not running lcdkit_set_backlight! \n");
        return 0;
    }
    pinfo = &(hisifd->panel_info);
    if (NULL == pinfo)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    if(lcdkit_info.panel_infos.display_on_in_backlight && lcdkit_info.panel_infos.display_on_need_send)
    {
        LCDKIT_INFO("Set display on before backlight set\n");
        LCDKIT_PANEL_CMD_REQUEST();
        hisifb_activate_vsync(hisifd);
        lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.display_on_in_backlight_cmds);
        hisifb_deactivate_vsync(hisifd);
        LCDKIT_PANEL_CMD_RELEASE();
        lcdkit_info.panel_infos.display_on_need_send = false;
    }

    if (lcdkit_info.panel_infos.dis_on_cmds_delay_margin_support
        && lcdkit_info.panel_infos.dis_on_cmds_panel_on_tag)
    {
        lcdkit_display_on_cmds_check_delay();
    }
    if (hisifb_display_effect_check_bl_value(bl_level, last_bl)) {
        if (count == 0) {
            LCDKIT_INFO("[effect] last bl_level=%d.\n", last_bl);
        }
        count = DISPLAYENGINE_BL_DEBUG_FRAMES;
    }
    if (count > 0) {
        LCDKIT_INFO("[effect] fb%d, bl_level=%d.\n", hisifd->index, bl_level);
        count--;
    } else {
        LCDKIT_DEBUG("fb%d, bl_level=%d.\n", hisifd->index, bl_level);
    }
    last_bl = bl_level;

    if (g_backlight_count)
    {
        LCDKIT_INFO("[backlight] Set backlight to %d. [backlight print sequence number: %d] \n",bl_level,BACKLIGHT_PRINT_TIMES-g_backlight_count+1);
        g_backlight_count = (g_backlight_count > 0) ? (g_backlight_count - 1) : 0;
    }

    if(jank_last_bl_level == 0 && bl_level != 0)
    {
        LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON, "LCD_BACKLIGHT_ON,%u", bl_level);
        LCDKIT_INFO("[backlight on] bl_level = %d!\n", bl_level);
        jank_last_bl_level = bl_level;
    }
    else if (bl_level == 0 && jank_last_bl_level != 0)
    {
        LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_OFF, "LCD_BACKLIGHT_OFF");
        LCDKIT_INFO("[backlight off] bl_level = %d!\n", bl_level);
        jank_last_bl_level = bl_level;
    }

    if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM)
    {
        ret = hisi_pwm_set_backlight(hisifd, bl_level);
    }
    else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM)
    {
        if((pinfo->rgbw_support) && (hisifd->backlight.bl_level_old == 0) && (bl_level!=0) && (lcdkit_info.panel_infos.rgbw_backlight_cmds.cmds) && (lcdkit_info.panel_infos.rgbw_backlight_cmds.cmd_cnt>0))
        {
          bl_level = (bl_level < hisifd->panel_info.bl_max) ? bl_level : hisifd->panel_info.bl_max;
          reg_value_61h = bl_level * 4095 / hisifd->panel_info.bl_max;
          lcdkit_info.panel_infos.rgbw_backlight_cmds.cmds[0].payload[1] = (reg_value_61h >> 8) & 0x0f;
          lcdkit_info.panel_infos.rgbw_backlight_cmds.cmds[0].payload[2] = reg_value_61h & 0xff;
          LCDKIT_PANEL_CMD_REQUEST();
          hisifb_activate_vsync(hisifd);
          lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.rgbw_backlight_cmds);
          hisifb_deactivate_vsync(hisifd);
          LCDKIT_PANEL_CMD_RELEASE();
          mdelay(38);
          LCDKIT_INFO("lcdkit_rgbw:delay 30ms after set61h,before setbl in first on.\n");
        }

        lcdkit_backlight_enable_gpio_operation(pinfo->bl_ic_ctrl_mode,bl_level);
        ret = hisi_blpwm_set_backlight(hisifd, bl_level);
        if(hisifd->panel_info.bl_v200)
        {
            LCDKIT_DEBUG("The backlight is controled by soc and v200\n");
            if (lcdkit_info.panel_infos.panel_type & PANEL_TYPE_LCD)
            {
                /*enable/disable backlight*/
                down(&lcdkit_info.panel_infos.bl_sem);
                if (bl_level == 0 && last_bl_level != 0)
                {
                    vcc_cmds_tx(NULL, lcdkit_scharger_bl_disable_cmds, \
                                ARRAY_SIZE(lcdkit_scharger_bl_disable_cmds));
                }
                else if (last_bl_level == 0 && bl_level != 0)
                {
                    vcc_cmds_tx(NULL, lcdkit_scharger_bl_enable_cmds, \
                                ARRAY_SIZE(lcdkit_scharger_bl_enable_cmds));
                }

                last_bl_level = bl_level;
                up(&lcdkit_info.panel_infos.bl_sem);
            }
        }

    }
    else if (hisifd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM)
    {
        ret = hisi_sh_blpwm_set_backlight(hisifd, bl_level);
    }
    else if ((hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) && (hisifd->panel_info.bl_max > 0))
    {
        bl_level = (bl_level < hisifd->panel_info.bl_max) ? bl_level : hisifd->panel_info.bl_max;

        hisifb_display_effect_fine_tune_backlight(hisifd, (int)bl_level, (int *)&bl_level);

        if(lcdkit_info.panel_infos.backlight_cmds.cmds[0].dlen == 2)
        {
            lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[1] = bl_level  * 255 / hisifd->panel_info.bl_max;
        }
        else
        {
            if(hisifd->panel_info.bl_otm)
            {
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[1] = (bl_level >> 2) & 0xff;    // high 8bit
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[2] = (bl_level << 2) & 0x00ff;  // low bit2-3
            }
            else if(lcdkit_info.panel_infos.bl_byte_order == 1) {
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[1] = bl_level & 0x00ff;        // low 8bit
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[2] = (bl_level >> 8) & 0xff;   // high 8bit
            } else {
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[1] = (bl_level >> 8) & 0xff;   // high 8bit
                lcdkit_info.panel_infos.backlight_cmds.cmds[0].payload[2] = bl_level & 0x00ff;        // low 8bit
            }
        }
        if(lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base))
        {
            LCDKIT_ERR("MIPI DSI FIFO not empty,backlight cmd send fail!\n");
            return -EPERM;
        }
        else
        {
            LCDKIT_PANEL_CMD_REQUEST();
            lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.backlight_cmds);
            LCDKIT_PANEL_CMD_RELEASE();
            if (lcdkit_info.panel_infos.panel_type & PANEL_TYPE_LCD)
            {
                /*enable/disable backlight*/
                down(&lcdkit_info.panel_infos.bl_sem);
                if (bl_level == 0 && last_bl_level != 0)
                {
                    vcc_cmds_tx(NULL, lcdkit_scharger_bl_disable_cmds, \
                                ARRAY_SIZE(lcdkit_scharger_bl_disable_cmds));
                }
                else if (last_bl_level == 0 && bl_level != 0)
                {
                    vcc_cmds_tx(NULL, lcdkit_scharger_bl_enable_cmds, \
                                ARRAY_SIZE(lcdkit_scharger_bl_enable_cmds));
                }

                last_bl_level = bl_level;
                up(&lcdkit_info.panel_infos.bl_sem);
            }
        }
    }
    else
    {
        LCDKIT_ERR("fb%d, not support this bl_set_type(%d)!\n",
                   hisifd->index, hisifd->panel_info.bl_set_type);
    }

    LCDKIT_DEBUG("fb%d, -.\n", hisifd->index);

    return ret;
}


static void lcdkit_second_timerout_function(unsigned long arg)
{
	unsigned long temp;
	temp = arg;
	if(lcdkit_info.panel_infos.bl_is_shield_backlight == true)
	{
		lcdkit_info.panel_infos.bl_is_shield_backlight = false;
	}
	del_timer(&backlight_second_timer);
	lcdkit_info.panel_infos.bl_is_start_second_timer = false;
	LCDKIT_INFO("Sheild backlight 1.2s timeout, remove the backlight sheild.\n");
}

static int lcdkit_set_backlight_by_type(struct platform_device* pdev, int backlight_type)
{
	int ret = 0;
	int max_backlight = 0;
	int min_backlight = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev)
	{
		LCDKIT_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);

	if (NULL == hisifd)
	{
		LCDKIT_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	LCDKIT_INFO("backlight_type is %d\n", backlight_type);

	max_backlight = g_max_backlight_from_app;
	min_backlight = g_min_backlight_from_app;

	switch (backlight_type) {
	case BACKLIGHT_HIGH_LEVEL:
		lcdkit_set_backlight(pdev, max_backlight);
		lcdkit_info.panel_infos.bl_is_shield_backlight = true;
		if(lcdkit_info.panel_infos.bl_is_start_second_timer == false)
		{
			init_timer(&backlight_second_timer);
			backlight_second_timer.expires = jiffies + 12*HZ/10;// 1.2s
			backlight_second_timer.data = 0;
			backlight_second_timer.function = lcdkit_second_timerout_function;
			add_timer(&backlight_second_timer);
			lcdkit_info.panel_infos.bl_is_start_second_timer = true;
		}
		else
		{
			//if timer is not timeout, restart timer
			mod_timer(&backlight_second_timer, (jiffies + 12*HZ/10));
		}
		LCDKIT_INFO("backlight_type is (%d), set_backlight is (%d)\n", backlight_type, max_backlight);
		break;
	case BACKLIGHT_LOW_LEVEL:
		if(lcdkit_info.panel_infos.bl_is_start_second_timer == true)
		{
			del_timer(&backlight_second_timer);
			lcdkit_info.panel_infos.bl_is_start_second_timer = false;
		}
		lcdkit_info.panel_infos.bl_is_shield_backlight = false;
		lcdkit_set_backlight(pdev, min_backlight);
		LCDKIT_INFO("backlight_type is (%d), set_backlight is (%d)\n", backlight_type, min_backlight);
		break;
	default:
		LCDKIT_ERR("backlight_type is not define(%d).\n", backlight_type);
		break;
	}
	return ret;

}

static int find_cmd_by_mipi_clk(struct hisi_fb_data_type *hisifd, uint32_t clk_val, struct lcdkit_dsi_panel_cmds **snd_cmds)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd)
	{
		LCDKIT_ERR("hisifd is NULL\n");
	}

	pinfo = &(hisifd->panel_info);

	LCDKIT_DEBUG("[mipi tracking] clk_val = %d\n", clk_val);
	if (clk_val == pinfo->mipi.dsi_bit_clk_val1)
	{
		*snd_cmds = &lcdkit_info.panel_infos.dsi_upt_snd_a_cmds;
	}
	else if (clk_val == pinfo->mipi.dsi_bit_clk_val2)
	{
		*snd_cmds = &lcdkit_info.panel_infos.dsi_upt_snd_b_cmds;
	}
	else if (clk_val == pinfo->mipi.dsi_bit_clk_val3)
	{
		*snd_cmds = &lcdkit_info.panel_infos.dsi_upt_snd_c_cmds;
	}
	else if (clk_val == pinfo->mipi.dsi_bit_clk_val4)
	{
		*snd_cmds = &lcdkit_info.panel_infos.dsi_upt_snd_d_cmds;
	}
	else if (clk_val == pinfo->mipi.dsi_bit_clk_val5)
	{
		*snd_cmds = &lcdkit_info.panel_infos.dsi_upt_snd_e_cmds;
	}
	else
	{
		LCDKIT_ERR("fail find expect dsi_bit_clk value\n");
		return -EINVAL;
	}

	return ret;
}

static int snd_mipi_clk_disable_esd = 0;

static int lcdkit_snd_mipi_clk_cmd_store(struct platform_device *pdev, uint32_t clk_val)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct lcdkit_dsi_panel_cmds* snd_cmds = NULL;

	if (NULL == pdev)
	{
		LCDKIT_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd)
	{
		LCDKIT_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (lcdkit_info.panel_infos.dsi_upt_snd_cmd_support)
	{
		ret = find_cmd_by_mipi_clk(hisifd, clk_val, &snd_cmds);
		if (ret == 0)
		{
			down(&lcdkit_info.panel_infos.lcdkit_cmd_sem);
			if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base))
			{
				LCDKIT_ERR("The fifo is full causing timeout when sending mipi tracking cmds!\n");
				up(&lcdkit_info.panel_infos.lcdkit_cmd_sem);
				return -EPERM;
			}
			lcdkit_dsi_tx(hisifd, snd_cmds);
			LCDKIT_DEBUG("[mipi tracking] already snd mipi clk cmds\n");
			up(&lcdkit_info.panel_infos.lcdkit_cmd_sem);
		}
	}

	return ret;
}

static int lcdkit_esd_check(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    int ret = 0;
    uint32_t temp = 0;
    uint32_t read_value = 0, tmp_value = 0;
    int esd_exception_flags = 0;
    int i = 0, j = 0, count = 0;
    struct dsi_cmd_desc dsi_cmd;
    struct lcdkit_dsi_cmd_desc* lcdkit_dsi_cmd = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    /*check platform esd*/
    if (g_lcdkit_pri_info.platform_esd_support)
    {
        temp = inp32(hisifd->mipi_dsi0_base + g_lcdkit_pri_info.platform_esd_reg);
        if (temp & g_lcdkit_pri_info.platform_esd_value)
        {
            LCDKIT_ERR("Platform open esd err, reg:0x%x, value:0x%x\n", g_lcdkit_pri_info.platform_esd_reg, temp);
            hisifd->esd_recover_state = ESD_RECOVER_STATE_START;
            return 1;
        }
        LCDKIT_INFO("Platform esd check, reg:0x%x, value:0x%x\n", g_lcdkit_pri_info.platform_esd_reg, temp);
    }

	LCDKIT_PANEL_CMD_REQUEST();
	if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("esd check cmd send fail!\n");
		LCDKIT_PANEL_CMD_RELEASE();
		return 1;
	}

	if(g_lcdkit_pri_info.aod_switch_status == 1){
		HISI_FB_ERR("Bypass esd check when aod is in switching mode state!\n");
		LCDKIT_PANEL_CMD_RELEASE();
		return 0;
	}
    if (g_lcdkit_pri_info.esd_support) {
        lcdkit_dsi_cmd = g_lcdkit_pri_info.esd_cmds.cmds;
        for (i = 0; i < g_lcdkit_pri_info.esd_cmds.cmd_cnt; i++) {
            dsi_cmd.dtype = lcdkit_dsi_cmd->dtype;
            dsi_cmd.vc = lcdkit_dsi_cmd->vc;
            dsi_cmd.wait = lcdkit_dsi_cmd->wait;
            dsi_cmd.waittype = lcdkit_dsi_cmd->waittype;
            dsi_cmd.dlen = lcdkit_dsi_cmd->dlen;
            dsi_cmd.payload = lcdkit_dsi_cmd->payload;
            ret = mipi_dsi_lread_reg(&read_value, &dsi_cmd, lcdkit_dsi_cmd->dlen, hisifd->mipi_dsi0_base);
            if (!ret) {
                for (j = 0; j < lcdkit_dsi_cmd->dlen; j++) {
                    tmp_value = (read_value >> j * 8) & 0xff;
                    switch(hisifd->panel_info.esd_expect_value_type)
                    {
                        case 0:
                            if (tmp_value & g_lcdkit_pri_info.esd_value.buf[count++]) {
                                esd_exception_flags = 1;
                            }
                            break;
                        case 1:
                            if (tmp_value != g_lcdkit_pri_info.esd_value.buf[count++]) {
                                esd_exception_flags = 1;
                            }
                            break;
                        default:
                            LCDKIT_ERR("[ESD] error esd-expect-value-type = %d, line = %d\n", hisifd->panel_info.esd_expect_value_type, __LINE__);
                            break;
                    }
                    if (esd_exception_flags) {
                        hisifd->esd_recover_state = ESD_RECOVER_STATE_START;
                        LCDKIT_ERR("esd abnormal read reg:0x%x = 0x%x\n", g_lcdkit_pri_info.esd_cmds.cmds[i].payload[0], read_value);
                        LCDKIT_PANEL_CMD_RELEASE();
                        return 1;
                    }
                }
                lcdkit_dsi_cmd++;
            } else {
                LCDKIT_PANEL_CMD_RELEASE();
                return ret;
            }
        }
    }
    ret = lcdkit_info.lcdkit_check_esd(hisifd);
    LCDKIT_PANEL_CMD_RELEASE();
    return ret;
}

static void lcdkit_snd_cmd_before_frame(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    int ret = 0;

    LCDKIT_DEBUG("+\n");
    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return;
    }

    lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.snd_cmd_before_frame_cmds);

    LCDKIT_DEBUG("-\n");
    return;
}

/*
*name:lcdkit_set_fastboot
*function:set fastboot display
*@pdev:platform device
*/
static int lcdkit_set_fastboot(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    if(CHECKED_ERROR == mipi_lcdkit_btb_check()) {
        LCDKIT_ERR("btb checked failed!");
    }

    if (lcdkit_is_lcd_panel())
    {
        if ( lcdkit_bias_is_regulator_ctrl_power())
        {
            /*set scharger vcc*/
            vcc_cmds_tx(NULL, lcdkit_scharger_bias_set_cmds, \
                        ARRAY_SIZE(lcdkit_scharger_bias_set_cmds));

            /*scharger vcc enable*/
            vcc_cmds_tx(NULL, lcdkit_scharger_bias_enable_cmds, \
                        ARRAY_SIZE(lcdkit_scharger_bias_enable_cmds));

            /*scharger bl enable*/
            vcc_cmds_tx(NULL, lcdkit_scharger_bl_enable_cmds, \
                        ARRAY_SIZE(lcdkit_scharger_bl_enable_cmds));
        }
    }

    // lcd pinctrl normal
    pinctrl_cmds_tx(pdev, lcdkit_pinctrl_normal_cmds,
                    ARRAY_SIZE(lcdkit_pinctrl_normal_cmds));

    // lcd reset gpio request
    gpio_cmds_tx(lcdkit_gpio_reset_request_cmds,
                 ARRAY_SIZE(lcdkit_gpio_reset_request_cmds));

    /*bl gpio request*/
    lcdkit_backlight_gpio_request(pdev);

    // backlight on
    hisi_lcd_backlight_on(pdev);

    // get blmaxnit
    if(lcdkit_info.panel_infos.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC){
        lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.bl_befreadconfig_cmds);
        lcdkit_dsi_rx(hisifd, &lcdkit_brightness_ddic_info, 1, &lcdkit_info.panel_infos.bl_maxnit_cmds);
        lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.bl_aftreadconfig_cmds);
    }

    return 0;
}


static int lcdkit_set_display_region(struct platform_device* pdev, struct dss_rect* dirty)
{
    struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

    if (NULL == pdev)
    {
        LCDKIT_ERR("pdev NULL Pointer!\n");
        return -1;
    }

    hisifd = platform_get_drvdata(pdev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -1;
    }

    if (!lcdkit_info.panel_infos.display_region_support)
    {
      LCDKIT_DEBUG("fps updt is not support!\n");
      return -1;
    }

	LCDKIT_PANEL_CMD_REQUEST();
	if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("dsi cmd fifo is not empty, cmd maybe send fail!\n");
	}
	ret = lcdkit_info.lcdkit_set_display_region(hisifd, dirty);
	LCDKIT_PANEL_CMD_RELEASE();
	return ret;
}

static int lcdkit_fps_scence_handle(struct platform_device* pdev, uint32_t scence)
{
    if (!lcdkit_info.panel_infos.fps_func_switch)
    {
      LCDKIT_DEBUG("fps updt is not support!\n");
      return 0;
    }

    return lcdkit_info.lcdkit_fps_scence_handle(pdev, scence);

}

static int lcdkit_fps_updt_handle(void* pdata)
{
    int ret = 0;

    if (lcdkit_info.panel_infos.fps_func_switch)
    {
        LCDKIT_DEBUG("fps updt is support!\n");
        ret = lcdkit_info.lcdkit_fps_updt_handle(pdata);
    }
    else
    {
        LCDKIT_ERR("fps updt is not support!\n");
    }

    return ret;
}

static int lcdkit_ce_mode_show(void* pdata, char* buf)
{
    int ret = 0;

    if (lcdkit_info.panel_infos.ce_support)
    {
        LCDKIT_INFO("ce is support!\n");
        ret = lcdkit_info.lcdkit_ce_mode_show(pdata,buf);
    }
    else
    {
        LCDKIT_INFO("ce is not support!\n");
    }

    return ret;
}

static int lcdkit_ce_mode_store(void* pdata, const char* buf, size_t count)
{
    int ret = 0;
    unsigned long val = 0;
    struct hisi_fb_data_type *hisifd = NULL;
    struct platform_device *pdev = NULL;


    pdev = (struct platform_device *)pdata;
    hisifd = platform_get_drvdata(pdev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -1;
    }

    ret = strict_strtoul(buf, 0, &val);
    if (ret)
        return ret;

    hisifd->user_scene_mode = (int)val;

    if (lcdkit_info.panel_infos.ce_support)
    {
        LCDKIT_INFO("ce is support!\n");
        ret = lcdkit_info.lcdkit_ce_mode_store(hisifd, buf, count);
    }
    else
    {
        LCDKIT_INFO("ce updt is not support!\n");
    }

    return ret;
}

static int lcdkit_rgbw_set_func(struct hisi_fb_data_type* hisifd)
{
    BUG_ON(hisifd == NULL);
	int ret = 0;
	struct hisi_panel_info* pinfo = NULL;

	pinfo = &(hisifd->panel_info);

    if (pinfo->rgbw_support)
    {
        LCDKIT_DEBUG("rgbw updt is support!\n");
        ret = lcdkit_rgbw_set_handle(hisifd);
    }
    else
    {
        LCDKIT_DEBUG("rgbw updt is not support!\n");
    }

    return ret;
}

int lcdkit_hbm_set_func(struct hisi_fb_data_type *hisifd)
{
    int ret = 0;
    struct hisi_panel_info* pinfo = NULL;

    if(NULL == hisifd)
    {
        HISI_FB_ERR("hisifd is NULL!\n");
        return -1;
    }

    pinfo = &(hisifd->panel_info);
    if(NULL == pinfo)
    {
        LCDKIT_ERR("pinfo is NULL!\n");
        return -1;
    }
    if(pinfo->hbm_support != 0)
    {
        ret = lcdkit_hbm_set_handle(hisifd);
    }
    else
    {
        LCDKIT_DEBUG("hbm is not support!\n");
    }
    return ret;
}


static ssize_t lcdkit_aod_alpm_setting_store(struct platform_device *pdev, const char *buf, size_t count)
{
    struct hisi_fb_data_type *hisifd = NULL;
    int ret = 0;
    unsigned int cmd = 0;
    static uint32_t reg_value_61h;
    int data_for_ts_resume = LCDKIT_SHORT_SYNC_TIMEOUT;
    int data_for_ts_after_resume = LCDKIT_NO_SYNC_TIMEOUT;

    if(pdev == NULL)
    {
        LCDKIT_ERR("pdev is null\n");
        return -1;
    }

    hisifd = platform_get_drvdata(pdev);
    if(hisifd == NULL)
    {
        LCDKIT_ERR("hisifd is null\n");
        return -1;
    }

    ret = sscanf(buf, "%u", &cmd);
    if (!ret)
    {
        LCDKIT_ERR("sscanf return invaild:%d\n", ret);
        return -1;
    }

    char playload[3] = {0x51, 0x00, 0x00};
    struct lcdkit_dsi_cmd_desc cmds = {DTYPE_GEN_LWRITE, 0, 0, 0, 10, 0, 3, playload};
    struct lcdkit_dsi_panel_cmds backlight_cmds = {NULL, 0, &cmds, 1, 0, 0};

    char playload1[1] = {0x28};
    struct lcdkit_dsi_cmd_desc cmds1 = {DTYPE_DCS_WRITE, 0, 0, 0, 100, 0, 1, playload1};
    struct lcdkit_dsi_panel_cmds display_off_cmd = {NULL, 0, &cmds1, 1, 0, 0};

    char playload2[1] = {0x29};
    struct lcdkit_dsi_cmd_desc cmds2 = {DTYPE_DCS_WRITE, 0, 0, 0, 100, 0, 1, playload2};
    struct lcdkit_dsi_panel_cmds display_on_cmd = {NULL, 0, &cmds2, 1, 0, 0};

    LCDKIT_INFO("[AOD] AOD cmd is %d \n", cmd);
    switch(cmd)
    {
        case ALPM_DISPLAY_OFF_CMD:
            hisifd->aod_mode = 1;
            LCDKIT_PANEL_CMD_REQUEST();
            g_lcdkit_pri_info.aod_switch_status = 1;
            if (hisifd->panel_info.esd_enable) {
                hrtimer_cancel(&hisifd->esd_ctrl.esd_hrtimer);
            }
            LCDKIT_PANEL_CMD_RELEASE();
            //set min brightness
            playload[1]	= 0;
            playload[2]	= 0x0C;
            LCDKIT_INFO("[AOD]set min brightness:%d for(AOD mode) and display off(0x28).\n", playload[2]);
            lcdkit_dsi_tx(hisifd, &backlight_cmds);
            lcdkit_dsi_tx(hisifd, &display_off_cmd);
            break;

        case ALPM_ON_50NIT_CMD:
            LCDKIT_INFO("[AOD]enter ALPM mode(50nit) and set cmd display_cmd(0x29)!\n");
            lcdkit_dsi_tx(hisifd,&lcdkit_info.panel_infos.panel_enter_aod_cmds);
            lcdkit_dsi_tx(hisifd, &display_on_cmd);
            break;

        case ALPM_OFF_CMD:
            LCDKIT_INFO("[AOD]exit ALPM mode and set cmd display_cmd(0x29)!\n");
            msleep(17);
            lcdkit_dsi_tx(hisifd,&lcdkit_info.panel_infos.panel_exit_aod_cmds);
            lcdkit_dsi_tx(hisifd, &display_on_cmd);
            LCDKIT_PANEL_CMD_REQUEST();
            if (hisifd->panel_info.esd_enable) {
                hrtimer_start(&hisifd->esd_ctrl.esd_hrtimer, ktime_set(ESD_CHECK_TIME_PERIOD / 1000,
                    (ESD_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
            }
            g_lcdkit_pri_info.aod_switch_status = 0;
            LCDKIT_PANEL_CMD_RELEASE();
            //set brightness to 0
            playload[1]	= 0;
            playload[2]	= 0;
            lcdkit_dsi_tx(hisifd, &backlight_cmds);
            hisifd->aod_mode = 0;
            //here need a 2-frame time delay to avoid anomalous display when exit alpm mode.
            msleep(34);
            lcdkit_notifier_call_chain(LCDKIT_TS_RESUME_DEVICE, &data_for_ts_resume);
            lcdkit_notifier_call_chain(LCDKIT_TS_AFTER_RESUME, &data_for_ts_after_resume);
            break;

        case ALPM_ON_10NIT_CMD:
            LCDKIT_INFO("[AOD]enter ALPM mode(10nit) and set cmd display_cmd(0x29)!\n");
            lcdkit_dsi_tx(hisifd,&lcdkit_info.panel_infos.panel_aod_low_brightness_cmds);
            lcdkit_dsi_tx(hisifd, &display_on_cmd);
            break;

        default:
            break;
    }
    return count;
}

void read_ddic_reg_parse(uint32_t input[],uint8_t inputlen, uint8_t output[], uint8_t start_position, uint8_t length)
{
	int i = 0;

	if ((NULL == input) || (NULL == output) || (start_position + length > inputlen))
	{
		LCDKIT_ERR("NULL Pointer!\n");
        g_record_project_id_err |= BIT(5);
		return ;
	}


	for(i = 0; i < length; i++,start_position++)
	{
			/*The following numbers such as "4","8","16","24", are have no specfic meanings, so needn't to define macro.*/
			switch (start_position % 4)
				{
				case 0:
					if ((input[start_position / 4] & 0xFF) == 0)
						{
							output[i] = (uint8_t)((input[start_position / 4] & 0xFF) + '0');
						}
					else
						{
							output[i] = (uint8_t)(input[start_position / 4] & 0xFF);
						}
					break;
				case 1:
					if (((input[start_position / 4] >> 8) & 0xFF) == 0)
						{
							output[i] = (uint8_t)(((input[start_position / 4] >> 8) & 0xFF) + '0');
						}
					else
						{
							output[i] = (uint8_t)((input[start_position / 4] >> 8) & 0xFF);
						}
					break;
				case 2:
					if (((input[start_position / 4] >> 16) & 0xFF) == 0)
						{
							output[i] = (uint8_t)(((input[start_position / 4] >> 16) & 0xFF) + '0');
						}
					else
						{
							output[i] = (uint8_t)((input[start_position / 4] >> 16) & 0xFF);
						}
					break;
				case 3:
					if (((input[start_position / 4] >> 24) & 0xFF) == 0)
						{
							output[i] = (uint8_t)(((input[start_position / 4] >> 24) & 0xFF) + '0');
						}
					else
						{
							output[i] = (uint8_t)((input[start_position / 4] >> 24) & 0xFF);
						}
					break;
				default:
					break;
		}
		LCDKIT_INFO("output[%d]=0x%x \n ",i,output[i]);
	}
	return;
}


void read_td4336_project_id(struct hisi_fb_data_type* hisifd, uint8_t g_project_id[], uint8_t start_position, uint8_t length)
{
    char __iomem *mipi_dsi0_base = NULL;
    char project_id_reg[] = {0xbf};

	if (NULL == hisifd || NULL == g_project_id)
	{
		LCDKIT_ERR("NULL Pointer!\n");
		return ;
	}

    mipi_dsi0_base = hisifd->mipi_dsi0_base;

    struct dsi_cmd_desc project_id_cmd[] =
	{
        {DTYPE_GEN_READ1, 0, 100, WAIT_TYPE_US, sizeof(project_id_reg), project_id_reg},
	};

    if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base))
	{
		HISI_FB_ERR("The fifo is full causing timeout when read td4336's porjectid!\n");
        g_record_project_id_err |= BIT(4);
	}
	/*The number "75" means the length of register BFh for TD4336. */
    LCDKIT_PANEL_CMD_REQUEST();
	mipi_dsi_lread_reg(g_read_value_td4336, project_id_cmd, READ_REG_TD4336_NUM, mipi_dsi0_base);
	read_ddic_reg_parse(g_read_value_td4336,READ_REG_TD4336_NUM,g_project_id, start_position, length);
    LCDKIT_PANEL_CMD_RELEASE();
	return;
}

void read_himax83112_project_id(struct hisi_fb_data_type* hisifd, uint8_t g_project_id[])
{
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[2] = {0};
	int i = 0;
	char project_id_reg[] = {0xbb};
	char project_addr[PROJECT_ID_LENGTH] = {0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b};
	char project_addr_112A[PROJECT_ID_LENGTH] = {0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	char playload1_enter1[] = {0xbb, 0x07, 0x82, 0x00, 0x80, 0xff};
	char playload1_enter2[] = {0xbb, 0x07, 0x82, 0x00, 0x00, 0xff};
	char playload1_enter1_112A[] = {0xbb, 0x05, 0x16, 0x00, 0x80, 0xff};
	char playload1_enter2_112A[] = {0xbb, 0x05, 0x16, 0x00, 0x00, 0xff};

	char otp_poweron_offset1[] = {0xe9, 0xcd};
	char otp_poweron_offset2[] = {0xe9, 0x00};
	char otp_poweron1[] = {0xbb, 0x22};
	char otp_poweron2[] = {0xbb, 0x23};
	char otp_poweroff[] = {0xbb, 0x20};

	if (NULL == hisifd || NULL == g_project_id)
	{
		LCDKIT_ERR("NULL Pointer!\n");
        g_record_project_id_err |= BIT(1);
		return ;
	}

	mipi_dsi0_base = hisifd->mipi_dsi0_base;
	/* "Emily_read_reg_flag = 1" means Hx83112A, "Emily_read_reg_flag = 2" means Hx83112C. */
	if(lcdkit_info.panel_infos.eml_read_reg_flag == Hx83112A)
	{
		for(i = 0; i< PROJECT_ID_LENGTH; i++)
		{
			project_addr[i] = project_addr_112A[i];
		}
		playload1_enter1[1] = playload1_enter1_112A[1];
		playload1_enter1[2] = playload1_enter1_112A[2];
		playload1_enter2[1] = playload1_enter2_112A[1];
		playload1_enter2[2] = playload1_enter2_112A[2];
	}

	struct dsi_cmd_desc otp_poweron1_cmds[] =
	{
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron_offset1), otp_poweron_offset1},
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron1), otp_poweron1},
		{DTYPE_DCS_LWRITE, 0, 1, WAIT_TYPE_MS, sizeof(otp_poweron_offset2), otp_poweron_offset2},
	};
	struct dsi_cmd_desc otp_poweron2_cmds[] =
	{
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron_offset1), otp_poweron_offset1},
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron2), otp_poweron2},
		{DTYPE_DCS_LWRITE, 0, 1, WAIT_TYPE_MS, sizeof(otp_poweron_offset2), otp_poweron_offset2},
	};
	struct dsi_cmd_desc otp_poweroff_cmds[] =
	{
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron_offset1), otp_poweron_offset1},
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweroff), otp_poweroff},
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(otp_poweron_offset2), otp_poweron_offset2},
	};

	struct dsi_cmd_desc playload1_enter_cmds[] =
	{
		{DTYPE_DCS_LWRITE, 0, 1, WAIT_TYPE_US, sizeof(playload1_enter1), playload1_enter1},
		{DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US, sizeof(playload1_enter2), playload1_enter2},
	};
	struct dsi_cmd_desc project_id_cmd[] =
	{
		{DTYPE_GEN_READ, 0, 100, WAIT_TYPE_US, sizeof(project_id_reg), project_id_reg},
	};

	if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base))
	{
		HISI_FB_ERR("The fifo is full causing timeout when read hx83112's porjectid!\n");
        g_record_project_id_err |= BIT(2);
	}
    LCDKIT_PANEL_CMD_REQUEST();
	/* send otp power on cmds for hx83112*/
	mipi_dsi_cmds_tx(otp_poweron1_cmds, ARRAY_SIZE(otp_poweron1_cmds), mipi_dsi0_base);
	mipi_dsi_cmds_tx(otp_poweron2_cmds, ARRAY_SIZE(otp_poweron2_cmds), mipi_dsi0_base);

	for(i = 0; i<PROJECT_ID_LENGTH; i++)
	{
		playload1_enter1[2] =  project_addr[i];
		playload1_enter2[2] =  project_addr[i];
		if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base))
		{
			HISI_FB_ERR("The fifo is full causing timeout when read hx83112's porjectid!\n");
            g_record_project_id_err |= BIT(3);
		}
		mipi_dsi_cmds_tx(playload1_enter_cmds, ARRAY_SIZE(playload1_enter_cmds), mipi_dsi0_base);
		//Here number "5" means to read five paramaters.
		mipi_dsi_lread_reg(read_value, project_id_cmd, 5, mipi_dsi0_base);

		if(read_value[1] == 0)
			read_value[1]+= '0';
		g_project_id[i] = read_value[1];
		memset(read_value,0,sizeof(read_value));
	}
	/* send otp power off cmds for hx83112*/
	mipi_dsi_cmds_tx(otp_poweroff_cmds, ARRAY_SIZE(otp_poweroff_cmds), mipi_dsi0_base);
    LCDKIT_PANEL_CMD_RELEASE();
	return;
}

static void lcdkit_check_project_id(void)
{
	int len = 0;
	int pos, valid_flag = TRUE;
	int letter_num = 0;
	int digit_num = 0;
	int default_len = 0;

	len = strlen(g_project_id);
	if ((CORRECT_PROJECT_ID_LEN_1 == len)
		|| (CORRECT_PROJECT_ID_LEN_2 == len))
	{
		for (pos=0; pos < len; pos++)
		{
			if((('A' <= g_project_id[pos]) && ('Z' >= g_project_id[pos]))
				|| (('a' <= g_project_id[pos]) && ('z' >= g_project_id[pos])))
			{
				letter_num++;
				continue;
			}
			else if(('0' <= g_project_id[pos]) && ('9' >= g_project_id[pos]))
			{
				digit_num++;
				continue;
			}
			else
			{
				valid_flag = FALSE;
				break;
			}
		}

		if('0' == g_project_id[0])
		{
			valid_flag = FALSE;
		}

		if ((TRUE == valid_flag)
			&& (letter_num > 0)
			&& (letter_num < len)
			&& (digit_num > 0)
			&& (digit_num < len))
		{
			LCDKIT_DEBUG("read correct project_id = %s\n", g_project_id);
			return;
		}
	}

	if (NULL == lcdkit_info.panel_infos.panel_default_project_id)
	{
		LCDKIT_DEBUG("no panel_default_project_id\n");
		g_record_project_id_err |= BIT(7);
		return;
	}

	default_len = strlen(lcdkit_info.panel_infos.panel_default_project_id);
	if ((CORRECT_PROJECT_ID_LEN_1 != default_len)
		&& (CORRECT_PROJECT_ID_LEN_2 != default_len))
	{
		LCDKIT_DEBUG("not correct panel_default_project_id\n");
		g_record_project_id_err |= BIT(8);
		return;
	}

	if(default_len < LCD_DDIC_INFO_LEN)
	{
		LCDKIT_INFO("config panel_default_project_id is %s\n", lcdkit_info.panel_infos.panel_default_project_id);
		strncpy(g_project_id,lcdkit_info.panel_infos.panel_default_project_id,default_len);
		g_project_id[default_len] = 0;
	}

	return;
}


static void lcdkit_read_project_id(void)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (hisifd == NULL) {
		LCDKIT_ERR("hisifd is NULL\n");
        g_record_project_id_err = BIT(0);
		return;
	}
	LCDKIT_INFO("eml_read_reg_flag = %d!\n",lcdkit_info.panel_infos.eml_read_reg_flag);
	switch(lcdkit_info.panel_infos.eml_read_reg_flag)
	{
		case Hx83112A:
		case Hx83112C:
			read_himax83112_project_id(hisifd, g_project_id);
			break;
		case TD4336:
			read_td4336_project_id(hisifd, g_project_id, PROJECT_ID_START_POSITION_TD4336, PROJECT_ID_LENGTH);
			break;
		//this case is for eml_read_reg_flag == 0 or == 4
		default:
			lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.host_project_id_enter_cmds);
			ret = hostprocessing_read_ddic(g_project_id, &lcdkit_info.panel_infos.host_project_id_cmds, hisifd, LCD_DDIC_INFO_LEN);
			lcdkit_dsi_tx(hisifd, &lcdkit_info.panel_infos.host_project_id_exit_cmds);
			break;
	}

	if (ret) {
		LCDKIT_ERR("read reg error\n");
        g_record_project_id_err |= BIT(6);
	}

	lcdkit_check_project_id();

	LCDKIT_INFO("project_id = %s\n", g_project_id);
}

static int lcdkit_color_param_get_func(struct hisi_fb_data_type* hisifd)
{
	int ret = 0;
    struct hisi_panel_info* pinfo = NULL;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -1;
    }

    pinfo = &(hisifd->panel_info);
    if(NULL == pinfo)
    {
        LCDKIT_ERR("pinfo is NULL!\n");
        return -1;
    }

    if (lcdkit_info.panel_infos.lcd_brightness_color_uniform_support)
    {
        LCDKIT_DEBUG("color uniform fun is support!\n");
		hisifd->de_info.lcd_color_oeminfo.id_flag = lcdkit_info.panel_infos.lcd_color_oeminfo.id_flag;
		hisifd->de_info.lcd_color_oeminfo.tc_flag = lcdkit_info.panel_infos.lcd_color_oeminfo.tc_flag;

		hisifd->de_info.lcd_color_oeminfo.panel_id.modulesn = lcdkit_info.panel_infos.lcd_color_oeminfo.panel_id.modulesn;
		hisifd->de_info.lcd_color_oeminfo.panel_id.equipid = lcdkit_info.panel_infos.lcd_color_oeminfo.panel_id.equipid;
		hisifd->de_info.lcd_color_oeminfo.panel_id.modulemanufactdate = lcdkit_info.panel_infos.lcd_color_oeminfo.panel_id.modulemanufactdate;
		hisifd->de_info.lcd_color_oeminfo.panel_id.vendorid = lcdkit_info.panel_infos.lcd_color_oeminfo.panel_id.vendorid;

		hisifd->de_info.lcd_color_oeminfo.color_params.c_lmt[0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.c_lmt[0];
		hisifd->de_info.lcd_color_oeminfo.color_params.c_lmt[1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.c_lmt[1];
		hisifd->de_info.lcd_color_oeminfo.color_params.c_lmt[2] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.c_lmt[2];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[0][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[0][0];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[0][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[0][1];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[0][2] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[0][2];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[1][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[1][0];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[1][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[1][1];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[1][2] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[1][2];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[2][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[2][0];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[2][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[2][1];
		hisifd->de_info.lcd_color_oeminfo.color_params.mxcc_matrix[2][2] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.mxcc_matrix[2][2];
		hisifd->de_info.lcd_color_oeminfo.color_params.white_decay_luminace = lcdkit_info.panel_infos.lcd_color_oeminfo.color_params.white_decay_luminace;

		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[0][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[0][0];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[0][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[0][1];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[1][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[1][0];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[1][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[1][1];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[2][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[2][0];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[2][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[2][1];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[3][0] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[3][0];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.chroma_coordinates[3][1] = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.chroma_coordinates[3][1];
		hisifd->de_info.lcd_color_oeminfo.color_mdata.white_luminance = lcdkit_info.panel_infos.lcd_color_oeminfo.color_mdata.white_luminance;
    }
    else
    {
        LCDKIT_DEBUG("color uniform fun is not support!\n");
    }

    return ret;
}
static ssize_t lcdkit_cabc_store(struct platform_device *pdev, const char *buf, size_t count)
{
	struct lcdkit_panel_data* plcdkit_info = NULL;
	struct hisi_fb_data_type* hisifd = NULL;

    hisifd = platform_get_drvdata(pdev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

	plcdkit_info = lcdkit_get_panel_info();
	if (plcdkit_info == NULL) {
		HISI_FB_ERR("[effect] plcdkit_info is NULL Pointer\n");
		return -1;
	}
	if (plcdkit_info->lcdkit_cabc_mode_store && plcdkit_info->panel_infos.cabc_switch_support) {
		LCDKIT_PANEL_CMD_REQUEST();
		if (lcdkit_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
			HISI_FB_ERR("cabc mode cmd send fail!\n");
		}
		plcdkit_info->lcdkit_cabc_mode_store(hisifd, buf);
		LCDKIT_PANEL_CMD_RELEASE();
	}
	return count;
}


/*
*name:lcdkit_panel_bypass_powerdown_ulps_support
*function:get the ulps support flag when panel off bypass ddic power down.
*@pdev:platform device
*/
int lcdkit_panel_bypass_powerdown_ulps_support(struct platform_device* pdev)
{
	int support_flag = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd)
	{
		LCDKIT_ERR("NULL Pointer\n");
		return 0;
	}

	if (hisifd->aod_function)
	{
		support_flag = 1;
		LCDKIT_INFO("AOD mode: send ulps when power off, exit ulps when power on\n");
	}
	if (lcdkit_info.panel_infos.pt_ulps_support && g_tskit_pt_station_flag)
	{
		support_flag = 1;
		LCDKIT_INFO("It is need to send ulps when panel off bypass ddic power down! Support_flag=%d !\n",support_flag);
	}
	return support_flag;
}

/*******************************************************************************
**
*/
static struct hisi_panel_info lcdkit_panel_info = {0};
static struct hisi_fb_panel_data lcdkit_data =
{
    .panel_info = &lcdkit_panel_info,
    .set_fastboot = lcdkit_set_fastboot,
    .on = lcdkit_on,
    .off = lcdkit_off,
    .remove = lcdkit_remove,
    .set_backlight = lcdkit_set_backlight,
    .snd_mipi_clk_cmd_store = lcdkit_snd_mipi_clk_cmd_store,
    .esd_handle = lcdkit_esd_check,
    .set_display_region = lcdkit_set_display_region,
    .lcd_fps_scence_handle = lcdkit_fps_scence_handle,
    .lcd_fps_updt_handle = lcdkit_fps_updt_handle,
    .lcd_ce_mode_show = lcdkit_ce_mode_show,
    .lcd_ce_mode_store = lcdkit_ce_mode_store,
    .lcd_rgbw_set_func = lcdkit_rgbw_set_func,
    .lcd_hbm_set_func  = lcdkit_hbm_set_func,
    .snd_cmd_before_frame = lcdkit_snd_cmd_before_frame,
    .amoled_alpm_setting_store = lcdkit_aod_alpm_setting_store,
	.lcd_color_param_get_func = lcdkit_color_param_get_func,
	.lcd_set_backlight_by_type_func = lcdkit_set_backlight_by_type,
	.lcd_cabc_mode_store = lcdkit_cabc_store,
	.panel_bypass_powerdown_ulps_support = lcdkit_panel_bypass_powerdown_ulps_support,
};
#if defined(CONFIG_HISI_FB_970)
void hisifb_get_calicolordata_from_share_mem(struct lcdbrightnesscoloroeminfo *pinfo)
{
    void * vir_addr = 0;
    if(pinfo == NULL)
    {
        LCDKIT_ERR("point is NULL!\n");
        return;
    }

    vir_addr = (void *)ioremap_wc(HISI_SUB_RESERVED_BRIGHTNESS_CHROMA_MEM_PHYMEM_BASE, HISI_SUB_RESERVED_BRIGHTNESS_CHROMA_MEM_PHYMEM_SIZE);
    if(vir_addr == NULL)
    {
        LCDKIT_ERR("mem ioremap error !\n");
		return;
    }
    memcpy((void*)pinfo, (void*)vir_addr, sizeof(struct lcdbrightnesscoloroeminfo));
    iounmap(vir_addr);

    return;
}
#endif
/*
*name:lcdkit_probe
*function:panel driver probe
*@pdev:platform device
*/
static int __init lcdkit_probe(struct platform_device* pdev)
{
    struct hisi_panel_info* pinfo = NULL;
    struct device_node* np = NULL;
    int ret = 0;
    struct platform_device* hisi_pdev=NULL;

    //np = of_find_compatible_node(NULL, NULL, lcdkit_info.panel_infos.lcd_compatible);
    np = pdev->dev.of_node;
    if (!np)
    {
        LCDKIT_ERR("NOT FOUND device node %s!\n", lcdkit_info.panel_infos.lcd_compatible);
        return -EINVAL;
    }

    pinfo = lcdkit_data.panel_info;
    memset(pinfo, 0, sizeof(struct hisi_panel_info));
    lcdkit_init(np, pinfo);

    if (hisi_fb_device_probe_defer(lcdkit_info.panel_infos.lcd_disp_type, lcdkit_info.panel_infos.bl_type))
    {
        goto err_probe_defer;
    }

    /*init sem*/
    sema_init(&lcdkit_info.panel_infos.bl_sem, 1);
    sema_init(&lcdkit_info.panel_infos.lcdkit_cmd_sem, 1);
    pdev->id = 1;

    if (runmode_is_factory())
    {
        pinfo->esd_enable = 0;
        pinfo->dirty_region_updt_support = 0;
        pinfo->prefix_ce_support = 0;
        pinfo->prefix_sharpness1D_support = 0;
        pinfo->prefix_sharpness2D_support = 0;
        pinfo->sbl_support = 0;
        pinfo->acm_support = 0;
        pinfo->acm_ce_support = 0;
        if(pinfo->blpwm_preci_no_convert == 0)
        {
            pinfo->blpwm_precision_type = BLPWM_PRECISION_DEFAULT_TYPE;
        }
        pinfo->comform_mode_support = 0;
        pinfo->color_temp_rectify_support = 0;
        lcdkit_info.panel_infos.fps_func_switch = 0;
        pinfo->hiace_support = 0;
        pinfo->arsr1p_sharpness_support = 0;
        pinfo->blpwm_input_ena = 0;
        lcdkit_info.panel_infos.display_effect_on_support = 0;
        lcdkit_info.panel_infos.effect_support_mode &= BITS(31);
        pinfo->gmp_support = 0;
    }

    if (lcdkit_info.panel_infos.fps_func_switch){
        lcdkit_info.lcdkit_fps_timer_init();
    }

    /*effect init*/
    lcdkit_effect_get_data(lcdkit_panel_init(lcdkit_info.panel_infos.product_id, lcdkit_info.panel_infos.lcd_compatible), pinfo);

    /*Read gamma data from shared memory transferred from lk for updating the original gamma.*/
    if(lcdkit_info.panel_infos.dynamic_gamma_support)
    {
#if defined(CONFIG_HISI_FB_3660) || defined (CONFIG_HISI_FB_970)
        hisifb_update_gm_from_reserved_mem(pinfo->gamma_lut_table_R,
                pinfo->gamma_lut_table_G,
                pinfo->gamma_lut_table_B,
                pinfo->igm_lut_table_R,
                pinfo->igm_lut_table_G,
                pinfo->igm_lut_table_B);
#endif
    }

#if defined(CONFIG_HISI_FB_970)
    if(lcdkit_info.panel_infos.lcd_brightness_color_uniform_support)
    {
        hisifb_get_calicolordata_from_share_mem(&lcdkit_info.panel_infos.lcd_color_oeminfo);
    }
#endif
    /*vsp/vsn bl init*/
    lcdkit_bias_init(pdev);

    /*1.8 & 3.1 init*/
    ret = lcdkit_vcc_init(pdev);

    if (ret)
    {
        LCDKIT_ERR("vcc init fail!\n");
        goto err_return;
    }

    /*delay init*/
    lcdkit_reset_delay_init(lcdkit_gpio_reset_normal_cmds);
    lcdkit_bias_on_delay_init_regulator(lcdkit_scharger_bias_enable_cmds);
    lcdkit_bias_off_delay_init_regulator(lcdkit_scharger_bias_disable_cmds);
    lcdkit_bias_on_delay_init_gpio(lcdkit_bias_enable_cmds);
    lcdkit_bias_off_delay_init_gpio(lcdkit_bias_disable_cmds);
    if(lcdkit_info.panel_infos.lcdph_delay_set_flag == 1)
    {
        lcdkit_reset_high_delay_init(lcdkit_gpio_reset_high_cmds);
    }
    ret = lcdkit_pinctrl_init(pdev);

    if (ret)
    {
        LCDKIT_ERR("pinctrl init fail!\n");
        goto err_return;
    }

    // lcd vcc enable
    if (is_fastboot_display_enable())
    {
       if ( HYBRID == g_tskit_ic_type || AMOLED == g_tskit_ic_type )
        {
            if (lcdkit_vci_is_regulator_ctrl_power())
            {
                vcc_cmds_tx(pdev, lcdkit_regulator_vci_enable_cmds,
                            ARRAY_SIZE(lcdkit_regulator_vci_enable_cmds));
            }
        }

        if (lcdkit_iovcc_is_regulator_ctrl_power())
        {
            vcc_cmds_tx(pdev, lcdkit_io_vcc_enable_cmds,
                        ARRAY_SIZE(lcdkit_io_vcc_enable_cmds));
        }
        if (lcdkit_vdd_is_regulator_ctrl_power())
        {
            vcc_cmds_tx(pdev, lcdkit_vdd_enable_cmds,
                        ARRAY_SIZE(lcdkit_vdd_enable_cmds));
        }
    }

    lcdkit_gpio_request();

    // alloc panel device data
    ret = platform_device_add_data(pdev, &lcdkit_data, sizeof(struct hisi_fb_panel_data));

    if (ret)
    {
        LCDKIT_ERR("platform_device_add_data failed!\n");
        goto err_device_put;
    }

    hisi_pdev = hisi_fb_add_device(pdev);
    lcdkit_debugfs_init();
    lcdkit_set_pdev(hisi_pdev);
    LCDKIT_INFO("exit succ!!!!\n");
	/*read project id*/
	if (g_fastboot_enable_flag) {
		if (lcdkit_info.panel_infos.is_hostprocessing && lcdkit_info.panel_infos.host_info_all_in_ddic) {
			lcdkit_read_project_id();
		}
	}
	if (lcdkit_info.panel_infos.lcd_version_support) {
		sharp_panel_read();
	}

    if (lcdkit_info.panel_infos.lcd_otp_support)  {
         jdi_panel_read();
    }
    return 0;

err_device_put:
    platform_device_put(pdev);
err_return:
    return ret;
err_probe_defer:
    return -EPROBE_DEFER;

    return ret;
}

/***********************************************************
*platform driver definition
***********************************************************/
/*
*probe match table
*/
static struct of_device_id lcdkit_match_table[] =
{
    {
        .compatible = "auo_otm1901a_5p2_1080p_video",
        .data = NULL,
    },
    {},
};

/*
*panel platform driver
*/
static struct platform_driver lcdkit_driver =
{
    .probe = lcdkit_probe,
    .remove = NULL,
    .suspend = NULL,
    .resume = NULL,
    .shutdown = NULL,
    .driver = {
        .name = "lcdkit_mipi_panel",
        .of_match_table = lcdkit_match_table,
    },
};

/*
*name:lcdkit_init
*function:panel init
*/
static int __init lcdkit_disp_init(void)
{
    int ret = 0, len = 0;
    struct device_node* np = NULL;

#if defined(CONFIG_LCDKIT_DRIVER)
    if(!get_lcdkit_support())
    {
       HISI_FB_INFO("lcdkit is not support!\n");
       return ret;
    }
#endif

    np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCDKIT_PANEL_TYPE);

    if (!np)
    {
        LCDKIT_ERR("NOT FOUND device node %s!\n", DTS_COMP_LCDKIT_PANEL_TYPE);
        ret = -1;
        return ret;
    }

    memset(&lcdkit_info.panel_infos , 0 , sizeof(struct lcdkit_panel_infos));

    OF_PROPERTY_READ_U32_RETURN(np, "product_id", &lcdkit_info.panel_infos.product_id);
    LCDKIT_INFO("lcdkit_info.panel_infos.product_id = %d", lcdkit_info.panel_infos.product_id);

    lcdkit_info.panel_infos.lcd_compatible = (char*)of_get_property(np, "lcd_panel_type", NULL);

    isbulcked = false;
    if (!lcdkit_info.panel_infos.lcd_compatible)
    {
        LCDKIT_ERR("Is not normal lcd and return\n");
        return ret;
    }
    else
    {
        if(!strncmp(lcdkit_info.panel_infos.lcd_compatible, "auo_otm1901a_5p2_1080p_video_default",strlen("auo_otm1901a_5p2_1080p_video_default")))
        {
           LCDKIT_INFO("the panel is not buckled! \n");
           return ret;
        }
        isbulcked = true;
        len = strlen(lcdkit_info.panel_infos.lcd_compatible);
        memset( (char *)lcdkit_driver.driver.of_match_table->compatible, 0, LCDKIT_PANEL_COMP_LENGTH);
        strncpy( (char *)lcdkit_driver.driver.of_match_table->compatible, lcdkit_info.panel_infos.lcd_compatible,
			len > (LCDKIT_PANEL_COMP_LENGTH - 1) ? (LCDKIT_PANEL_COMP_LENGTH - 1) : len);
    }

    ret = platform_driver_register(&lcdkit_driver);

    if (ret)
    {
        LCDKIT_ERR("platform_driver_register failed, error=%d!\n", ret);
    }

    return ret;
}

module_init(lcdkit_disp_init);
