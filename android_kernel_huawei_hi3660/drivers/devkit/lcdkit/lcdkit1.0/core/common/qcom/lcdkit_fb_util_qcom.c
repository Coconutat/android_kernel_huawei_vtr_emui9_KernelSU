#include "mdss_fb.h"
#include "lcdkit_panel.h"
#include "mdss_dsi.h"
#include "lcdkit_fb_util.h"
#include "lcdkit_dbg.h"

struct msm_fb_data_type *get_mfd(struct device * dev)
{
    struct fb_info *fbi;
    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return NULL;
    }

    fbi = dev_get_drvdata(dev);
    if (NULL == fbi)
    {
        LCDKIT_ERR("fbi is NULL Point!\n");
        return NULL;
    }

    return (struct msm_fb_data_type *)fbi->par;
}

struct mdss_dsi_ctrl_pdata *get_ctrl_data(struct msm_fb_data_type *mfd)
{
    struct mdss_panel_data *pdata;

    if (mfd == NULL)
    {
        LCDKIT_ERR("mfd is NULL Point!\n");
        return NULL;
    }

    pdata = dev_get_platdata(&mfd->pdev->dev);
    if (pdata == NULL)
    {
        LCDKIT_ERR(": Panel data not available\n");
        return NULL;
    }

    return container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
}

void *get_fb_data(struct device *dev)
{
    return get_ctrl_data(get_mfd(dev));
}

ssize_t get_lcdkit_support(void)
{
    return true;
}

#if 0
ssize_t lcd_bl_info_show(struct device * dev, char *buf)
{
    struct msm_fb_data_type *mfd = get_mfd(dev);

	u32 bl_level_max = lcdkit_info.bl_level_max;
	u32 bl_level_min = lcdkit_info.bl_level_min;
    char *lcdtype = lcdkit_is_oled_panel() ? "OLED" : "LCD";

	LCDKIT_INFO("fb%d panel_info = blmax:%u,blmin:%u,lcdtype:%s,\n",
	    mfd->index, bl_level_max, bl_level_min, lcdtype);

	return scnprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,lcdtype:%s,\n",
                bl_level_max, bl_level_min, lcdtype);
}
#endif

#define lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata)   \
{   \
    if (NULL == dev)    \
    {   \
        LCDKIT_ERR("line: %d, dev is NULL Point!\n", __LINE__); \
        return -ENXIO;  \
    }   \
    \
    if (NULL == lcdkit_info)    \
    {   \
        LCDKIT_ERR("line: %d, lcdkit_info is NULL Point!\n", __LINE__); \
        return -ENXIO;  \
    }   \
    \
    if (NULL == buf)    \
    {   \
        LCDKIT_ERR("line: %d, buf is NULL Point!\n", __LINE__); \
        return -ENXIO;  \
    }   \
    \
    mfd = get_mfd(dev); \
    if (NULL == mfd)    \
    {   \
        LCDKIT_ERR("line: %d, mfd is NULL Point!\n", __LINE__); \
        return -EINVAL; \
    }   \
    \
    ctrl_pdata = get_ctrl_data(mfd);    \
    if (NULL == ctrl_pdata) \
    {   \
        LCDKIT_ERR("line: %d, ctrl_pdata is NULL Point!\n", __LINE__);  \
        return -EINVAL; \
    }   \
    \
    if (mdss_fb_is_power_off(mfd))   \
    {   \
        LCDKIT_ERR("line: %d, fb%d, panel power off!\n", __LINE__, mfd->index);\
        return -EINVAL; \
    }   \
    \
}

//lcdkit_cabc_mode_store
ssize_t lcd_cabc_mode_store(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, const char *buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

	if (lcdkit_info->lcdkit_cabc_mode_store)
	{
		ret = lcdkit_info->lcdkit_cabc_mode_store(ctrl_pdata, buf);
	}

	return ret;
}

//mdss_store_inversion_mode
//lcdkit_inversion_mode_store
ssize_t lcd_inversion_mode_store(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, const char *buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

	if (lcdkit_info->lcdkit_inversion_mode_store)
	{
		ret = lcdkit_info->lcdkit_inversion_mode_store(ctrl_pdata, buf);
    }

	return ret;
}

//mdss_store_scan_mode
//lcdkit_scan_mode_store
extern char *saved_command_line;
ssize_t lcd_scan_mode_store(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, const char *buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

	if((saved_command_line != NULL)
      && (strstr(saved_command_line, "androidboot.huawei_swtype=factory")))
	{
    	if (lcdkit_info->lcdkit_scan_mode_store)
    	{
    		ret = lcdkit_info->lcdkit_scan_mode_store(ctrl_pdata, buf);
        }
	}

	return ret;
}

//lcdkit_check_reg_show
ssize_t lcd_check_reg_show(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, char *buf)
{
    int ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_check_reg_show)
    {
        ret = lcdkit_info->lcdkit_check_reg_show(ctrl_pdata, buf);
    }

	return ret;
}

//lcdkit_check_esd
ssize_t lcd_check_esd_show(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, char *buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_check_esd)
    {
        ret = lcdkit_info->lcdkit_check_esd(ctrl_pdata);

        if (ret == 1) {
    		ret = snprintf(buf, PAGE_SIZE, "OK\n");
    	} else {
    		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");
    	}
    }

	return ret;
}

//lcdkit_gram_check_show
ssize_t lcd_gram_check_show(struct device * dev,
        struct lcdkit_panel_data *lcdkit_info, char *buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_gram_check_show)
    {
        ret = lcdkit_info->lcdkit_gram_check_show(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_gram_check_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_gram_check_store)
    {
        ret = lcdkit_info->lcdkit_gram_check_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_dynamic_sram_check_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_dynamic_sram_check_show)
    {
        ret = lcdkit_info->lcdkit_dynamic_sram_check_show(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_dynamic_sram_check_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_dynamic_sram_check_store)
    {
        ret = lcdkit_info->lcdkit_dynamic_sram_check_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_ic_color_enhancement_mode_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_ic_color_enhancement_mode_show)
    {
        ret = lcdkit_info->lcdkit_ic_color_enhancement_mode_show(buf);
    }

	return ret;
}

ssize_t lcd_ic_color_enhancement_mode_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_ic_color_enhancement_mode_store)
    {
        ret = lcdkit_info->lcdkit_ic_color_enhancement_mode_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_sleep_ctrl_store(struct device* dev,
        struct lcdkit_panel_data *lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_sleep_ctrl_store)
    {
        ret = lcdkit_info->lcdkit_sleep_ctrl_store(buf);
    }

	return ret;
}

ssize_t lcd_lp2hs_mipi_check_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_lp2hs_mipi_check_show)
    {
        ret = lcdkit_info->lcdkit_lp2hs_mipi_check_show(buf);
    }

	return ret;
}

ssize_t lcd_lp2hs_mipi_check_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_lp2hs_mipi_check_store)
    {
        ret = lcdkit_info->lcdkit_lp2hs_mipi_check_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_bist_check(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_bist_check_show)
    {
        ret = lcdkit_info->lcdkit_bist_check_show(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_mipi_detect_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_mipi_detect_show)
    {
        ret = lcdkit_info->lcdkit_mipi_detect_show(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_voltage_mode_enable_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_voltage_enable_store)
    {
        ret = lcdkit_info->lcdkit_voltage_enable_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_acl_ctrl_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_amoled_acl_ctrl_show)
    {
        ret = lcdkit_info->lcdkit_amoled_acl_ctrl_show(buf);
    }

	return ret;
}

ssize_t lcd_acl_ctrl_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_amoled_acl_ctrl_store)
    {
        ret = lcdkit_info->lcdkit_amoled_acl_ctrl_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_amoled_vr_mode_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_amoled_vr_mode_show)
    {
        ret = lcdkit_info->lcdkit_amoled_vr_mode_show(buf);
    }

	return ret;
}

ssize_t lcd_amoled_vr_mode_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_amoled_vr_mode_store)
    {
        ret = lcdkit_info->lcdkit_amoled_vr_mode_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_hbm_ctrl_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_amoled_hbm_ctrl_store)
    {
        ret = lcdkit_info->lcdkit_amoled_hbm_ctrl_store(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_support_mode_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_support_mode_show)
    {
        ret = lcdkit_info->lcdkit_support_mode_show(buf);
    }

	return ret;
}

ssize_t lcd_support_mode_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_support_mode_store)
    {
        ret = lcdkit_info->lcdkit_support_mode_store(buf);
    }

	return ret;
}

ssize_t lcd_comform_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_comform_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_cinema_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_cinema_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_support_checkmode_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_support_checkmode_show)
    {
        ret = lcdkit_info->lcdkit_support_checkmode_show(buf);
    }

	return ret;
}

ssize_t led_rg_lcd_color_temperature_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t led_rg_lcd_color_temperature_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_ce_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_ce_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_se_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_se_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_al_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_al_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t effect_ce_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_ce_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t effect_bl_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_bl_enable_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_bl_enable_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t effect_metadata_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t effect_metadata_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t effect_available_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t gamma_dynamic_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t  lcd_2d_sharpness_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_2d_sharpness_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_acm_state_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_acm_state_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_gmp_state_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_gmp_state_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t sbl_ctrl_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t sbl_ctrl_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_color_temperature_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_color_temperature_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    //ssize_t ret = 0;
    return count;
}

ssize_t lcd_frame_count_show(struct device* dev,
        struct device_attribute* attr, const char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_frame_update_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_frame_update_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t mipi_dsi_bit_clk_upt_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t mipi_dsi_bit_clk_upt_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_fps_scence_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_fps_scence_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t alpm_function_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t alpm_function_store(struct device* dev,
        struct device_attribute* attr, const char* buf)
{
    return 0;
}

ssize_t alpm_setting_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_func_switch_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_func_switch_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_dynamic_porch_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_dynamic_porch_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}

ssize_t lcd_test_config_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_test_config_show)
    {
        ret = lcdkit_info->lcdkit_test_config_show(ctrl_pdata, buf);
    }

	return ret;
}

ssize_t lcd_test_config_store(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -EINVAL;
    struct msm_fb_data_type *mfd;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata;

    lcdkit_get_pdata(dev, lcdkit_info, buf, mfd, ctrl_pdata);

    if (lcdkit_info->lcdkit_test_config_store)
    {
        ret = lcdkit_info->lcdkit_test_config_store(buf);
    }

	return ret;
}

ssize_t lv_detect_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t current_detect_show(struct device* dev,
        struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_effect_sre_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcd_effect_sre_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    ssize_t ret = 0;
    return ret;

}
ssize_t lcd_reg_read_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    return 0;
}
ssize_t lcd_reg_read_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    return 0;
}

ssize_t lcd_ddic_oem_info_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = 0;
    return ret;

}

ssize_t lcd_ddic_oem_info_store(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, const char* buf)
{
    ssize_t ret = 0;
    return ret;
}
ssize_t lcd_bl_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = 0;
    return ret;
}
ssize_t lcd_bl_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, const char* buf)
{
    ssize_t ret = 0;
    return ret;
}
ssize_t lcd_support_bl_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = 0;
    return ret;
}
ssize_t lcd_ldo_check_show(struct device* dev, char* buf)
{
    ssize_t ret = 0;
    return ret;
}

ssize_t lcdkit_jdi_nt36860_5p88_reg_read_show(void* pdata, char* buf)
{
    ssize_t ret = 0;
    return ret;	
}

ssize_t lcd_mipi_config_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;

    if (NULL == dev || NULL == lcdkit_info || NULL == buf)
    {
        LCDKIT_ERR("%s: NULL Pointer!\n",__func__);
        return -1;
    }

    return ret;
}