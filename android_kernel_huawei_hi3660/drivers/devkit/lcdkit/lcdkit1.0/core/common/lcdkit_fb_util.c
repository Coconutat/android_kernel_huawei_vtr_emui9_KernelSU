#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "lcdkit_fb_util.h"

struct device* lcd_dev;

/*function:  this function is used to get the type of LCD
 *input:
 *@pdata: this void point is used to converte to fb data struct.
 *output:
 *@buf: get the type of lcd
*/
static ssize_t lcdkit_lcd_model_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;
    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_model_show)
    {
        ret = lcdkit_info->lcdkit_model_show(buf);
    }

    return ret;
}

/*function:  this function is used to get the display type of LCD
 *input:
 *@pdata: this void point is used to converte to fb data struct.
 *output:
 *@buf: get the display type of lcd
*/
static ssize_t lcdkit_lcd_display_type_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;
    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_display_type_show)
    {
        ret = lcdkit_info->lcdkit_display_type_show(buf);
    }

    return ret;
}

/*function:  this function is used to get panel infomation
 *input:
 *@pdata: this void point is used to converte to fb data struct.
 *output:
 *@buf: get the panel info
*/
static ssize_t lcdkit_lcd_panel_info_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;
    lcd_dev = dev;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_panel_info_show)
    {
        ret = lcdkit_info->lcdkit_panel_info_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_cabc_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_cabc_mode_show && lcdkit_info->panel_infos.cabc_support)
    {
        ret = lcdkit_info->lcdkit_cabc_mode_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_cabc_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.cabc_support)
    {
        ret = lcd_cabc_mode_store(dev, lcdkit_info, buf);
    }

    return count;
}


static ssize_t lcdkit_lcd_inversion_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_inversion_mode_show && lcdkit_info->panel_infos.inversion_support)
    {
        ret = lcdkit_info->lcdkit_inversion_mode_show(buf);
    }

    return ret;
}


static ssize_t lcdkit_lcd_inversion_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.inversion_support)
    {
        ret = lcd_inversion_mode_store(dev, lcdkit_info, buf);
    }
    return count;

}

static ssize_t lcdkit_lcd_scan_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_scan_mode_show && lcdkit_info->panel_infos.scan_support)
    {
        ret = lcdkit_info->lcdkit_scan_mode_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_scan_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.scan_support)
    {
        ret = lcd_scan_mode_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_lcd_check_reg_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.check_reg_support)
    {
        ret = lcd_check_reg_show(dev, lcdkit_info, buf);
    }

    return ret;
}


static ssize_t lcdkit_lcd_gram_check_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.checksum_support)
    {
        ret = lcd_gram_check_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_gram_check_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.checksum_support)
    {
        ret = lcd_gram_check_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_dynamic_sram_check_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.dynamic_sram_check_support)
    {
        ret = lcd_dynamic_sram_check_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_dynamic_sram_check_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.dynamic_sram_check_support)
    {
        ret = lcd_dynamic_sram_check_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_ic_color_enhancement_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.ic_color_enhancement_support)
    {
        ret = lcd_ic_color_enhancement_mode_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_ic_color_enhancement_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.ic_color_enhancement_support)
    {
        ret = lcd_ic_color_enhancement_mode_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_sleep_ctrl_show(struct device* dev,
                                      struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info ;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_sleep_ctrl_show && lcdkit_info->panel_infos.PT_test_support)
    {
        ret = lcdkit_info->lcdkit_sleep_ctrl_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_sleep_ctrl_store(struct device* dev,
                                       struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info ;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.PT_test_support)
    {
        ret = lcd_sleep_ctrl_store(dev, lcdkit_info, buf);
    }

    return count;

}

static ssize_t lcdkit_lp2hs_mipi_check_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.lp2hs_mipi_check_support)
    {
        ret = lcd_lp2hs_mipi_check_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_lp2hs_mipi_check_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.lp2hs_mipi_check_support)
    {
        ret = lcd_lp2hs_mipi_check_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_lcd_bist_check(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;

    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.bist_check_support)
    {
        ret = lcd_bist_check(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_mipi_detect_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.mipi_detect_support)
    {
        ret = lcd_mipi_detect_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_hkadc_debug_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;

    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if (lcdkit_info->lcdkit_hkadc_debug_show && lcdkit_info->panel_infos.hkadc_support)
    {
        ret = lcdkit_info->lcdkit_hkadc_debug_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_lcd_hkadc_debug_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;

    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if (lcdkit_info->lcdkit_hkadc_debug_store && lcdkit_info->panel_infos.hkadc_support)
    {
        ret = lcdkit_info->lcdkit_hkadc_debug_store(buf);
    }

    return count;
}

static ssize_t  lcdkit_lcd_voltage_enable_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    ret = lcd_voltage_mode_enable_store(dev, lcdkit_info, buf);

    return count;
}

static ssize_t lcdkit_amoled_pcd_errflag_check(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    LCDKIT_INFO("Pcd_errflag_check_support=%d!\n",lcdkit_info->panel_infos.pcd_errflag_check_support);

    if (lcdkit_info->panel_infos.pcd_errflag_check_support)
    {
        ret = lcdkit_info->lcdkit_pcd_errflag_check(buf);
    }

    return ret;
}

static ssize_t lcdkit_amoled_acl_ctrl_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.acl_ctrl_support)
    {
        ret = lcd_acl_ctrl_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_amoled_acl_ctrl_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.acl_ctrl_support)
    {
        ret = lcd_acl_ctrl_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_amoled_vr_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.vr_support)
    {
        ret = lcd_amoled_vr_mode_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_amoled_vr_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.vr_support)
    {
        ret = lcd_amoled_vr_mode_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_amoled_hbm_ctrl_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;

    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if (lcdkit_info->lcdkit_amoled_hbm_ctrl_show && lcdkit_info->panel_infos.hbm_ctrl_support)
    {
        ret = lcdkit_info->lcdkit_amoled_hbm_ctrl_show(buf);
    }

    return ret;
}

static ssize_t lcdkit_amoled_hbm_ctrl_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    if(lcdkit_info->panel_infos.hbm_ctrl_support)
    {
        ret = lcd_hbm_ctrl_store(dev, lcdkit_info, buf);
    }

    return count;
}

static ssize_t lcdkit_lcd_support_mode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_support_mode_show(dev, lcdkit_info, buf);
    return ret;

}

static ssize_t lcdkit_lcd_support_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info ;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_support_mode_store(dev, lcdkit_info, buf);
    return count;
}

static ssize_t lcdkit_comform_mode_show(struct device* dev,
                                        struct device_attribute* attr, char* buf)
{
    return lcd_comform_mode_show(dev, attr, buf);
}

static ssize_t lcdkit_comform_mode_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_comform_mode_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_cinema_mode_show(struct device* dev,
                                       struct device_attribute* attr, char* buf)
{
    return  lcd_cinema_mode_show(dev, attr, buf);
}

static ssize_t lcdkit_cinema_mode_store(struct device* dev,
                                        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_cinema_mode_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_support_checkmode_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_support_checkmode_show(dev, lcdkit_info, buf);
    return ret;
}

static ssize_t lcdkit_led_rg_lcd_color_temperature_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return  led_rg_lcd_color_temperature_show(dev, attr, buf);
}

static ssize_t lcdkit_led_rg_lcd_color_temperature_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = led_rg_lcd_color_temperature_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_lcd_ce_mode_show(struct device* dev,
                                       struct device_attribute* attr, char* buf)
{
    return lcd_ce_mode_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_ce_mode_store(struct device* dev,
                                        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_ce_mode_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_effect_al_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return effect_al_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_al_store(struct device* dev,
                                      struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = effect_al_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_effect_ce_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return 0;
}

static ssize_t lcdkit_effect_ce_store(struct device* dev,
                                      struct device_attribute* attr, const char* buf, size_t count)
{
    return count;

}

static ssize_t lcdkit_effect_hdr_mode_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return effect_ce_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_hdr_mode_store(struct device* dev,
                                      struct device_attribute* attr, const char* buf, size_t count)
{
    effect_ce_store(dev, attr, buf, count);
    return count;

}
static ssize_t lcdkit_effect_sre_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return lcd_effect_sre_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_sre_store(struct device* dev,
                                      struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_effect_sre_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_effect_bl_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return effect_bl_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_bl_enable_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return effect_bl_enable_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_bl_enable_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = effect_bl_enable_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_effect_metadata_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return effect_metadata_show(dev, attr, buf);
}

static ssize_t lcdkit_effect_metadata_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = effect_metadata_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_effect_available_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return effect_available_show(dev, attr, buf);
}

static ssize_t lcdkit_gamma_dynamic_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = gamma_dynamic_store(dev, attr, buf, count);
    return count;

}

static ssize_t lcdkit_2d_sharpness_show(struct device* dev,
                                        struct device_attribute* attr, char* buf)
{
    return lcd_2d_sharpness_show(dev, attr, buf);
}
static ssize_t lcdkit_2d_sharpness_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_2d_sharpness_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_acm_state_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    return lcd_acm_state_show(dev, attr, buf);
}

static ssize_t lcdkit_acm_state_store(struct device* dev,
                                      struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_acm_state_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_lcd_gmp_state_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{

    return lcd_gmp_state_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_gmp_state_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_gmp_state_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_sbl_ctrl_show(struct device* dev,
                                    struct device_attribute* attr, char* buf)
{
    return sbl_ctrl_show(dev, attr, buf);
}

static ssize_t lcdkit_sbl_ctrl_store(struct device* dev,
                                     struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = sbl_ctrl_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_color_temperature_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_color_temperature_show(dev, attr , buf);
}

static ssize_t lcdkit_color_temperature_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_color_temperature_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_lcd_frame_count_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_frame_count_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_frame_update_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_frame_update_show(dev, attr, buf);
}

static ssize_t  lcdkit_lcd_frame_update_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_frame_update_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_mipi_dsi_bit_clk_upt_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return mipi_dsi_bit_clk_upt_show(dev, attr, buf);
}

static ssize_t lcdkit_mipi_dsi_bit_clk_upt_store(struct device* dev, 
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = mipi_dsi_bit_clk_upt_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_lcd_fps_scence_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_fps_scence_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_fps_scence_store(struct device* dev, 
         struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_fps_scence_store(dev, attr, buf, count);
    return count;
}

static ssize_t lcdkit_alpm_function_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
	struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();
    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

	if(lcdkit_info->panel_infos.aod_support)
    {
        ret = alpm_function_show(dev, attr, buf);
		if (ret < 0)
        {
            LCDKIT_ERR("alpm_function_show return fail. \n");
        }
    }
    return ret;
}

static ssize_t lcdkit_alpm_function_store(struct device* dev, 
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

	if(lcdkit_info->panel_infos.aod_support)
    {
        ret = alpm_function_store(dev, attr, buf);
		if (!ret)
        {
            LCDKIT_ERR("alpm_function_store return fail. \n");
            return -EINVAL;
        }
    }
    return count;
}

static ssize_t lcdkit_alpm_setting_store(struct device* dev, 
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }

	if(lcdkit_info->panel_infos.aod_support)
    {
        ret = alpm_setting_store(dev, attr, buf, count);
    }
    return count;
}

static ssize_t lcdkit_lcd_func_switch_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_func_switch_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_func_switch_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_func_switch_store(dev, attr, buf, count);
    return count;
}
static ssize_t lcdkit_lcd_dynamic_porch_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    return lcd_dynamic_porch_show(dev, attr, buf);
}

static ssize_t lcdkit_lcd_dynamic_porch_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    ret = lcd_dynamic_porch_store(dev, attr, buf, count);
    return count;
}
static ssize_t lcdkit_test_config_show(struct device* dev,
                                       struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_test_config_show(dev, lcdkit_info, buf);
    return ret;

}

static ssize_t lcdkit_test_config_store(struct device* dev,
                                        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    ret = lcd_test_config_store(dev, lcdkit_info, buf);

    return count;
}

static ssize_t lcdkit_lv_detect_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.lv_detect_support)
    {
        ret = lv_detect_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_current_detect_show(struct device* dev, 
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.current_detect_support)
    {
        ret = current_detect_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_reg_read_show(struct device* dev, 
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;
    lcdkit_info = lcdkit_get_panel_info();
    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }
    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }
    if(lcdkit_info->panel_infos.dynamic_gamma_support)
    {
        ret = lcd_reg_read_show(dev, lcdkit_info, buf);
    }
    return ret;
}
static ssize_t lcdkit_reg_read_store(struct device* dev,
           struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;
    lcdkit_info = lcdkit_get_panel_info();
    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }
    ret = lcd_reg_read_store(dev, lcdkit_info, buf);
    return count;
}

static ssize_t lcdkit_ddic_oem_info_show(struct device* dev,
                                       struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_ddic_oem_info_show(dev, lcdkit_info, buf);
    return ret;

}

static ssize_t lcdkit_ddic_oem_info_store(struct device* dev,
                                        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return 0;
    }

    ret = lcd_ddic_oem_info_store(dev, lcdkit_info, buf);

    return count;
}
static ssize_t lcdkit_bl_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret;
    struct lcdkit_panel_data* lcdkit_info_temp;
    lcdkit_info_temp = lcdkit_get_panel_info();

    if (NULL == lcdkit_info_temp)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }


    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    ret = lcd_bl_mode_store(dev, lcdkit_info_temp, buf);
    if (ret)
    {
        LCDKIT_ERR("lcd_bl_mode_store return fail\n");
        return -EINVAL;
    }

    return count;
}
static ssize_t lcdkit_bl_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    ssize_t ret;
    struct lcdkit_panel_data* lcdkit_info_temp;
    lcdkit_info_temp = lcdkit_get_panel_info();

    if (NULL == lcdkit_info_temp)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_bl_mode_show(dev, lcdkit_info_temp, buf);
    return ret;
}
static ssize_t lcdkit_se_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->panel_infos.se_support)
    {
        ret = lcd_se_mode_show(dev, lcdkit_info, buf);
    }

    return ret;
}

static ssize_t lcdkit_se_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct lcdkit_panel_data* lcdkit_info;

    lcdkit_info = lcdkit_get_panel_info();

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    if(lcdkit_info->panel_infos.se_support)
    {
        ret = lcd_se_mode_store(dev, lcdkit_info, buf);
    }
    if (ret)
    {
        LCDKIT_ERR("lcd_se_mode_store return fail\n");
        return -EINVAL;
    }

    return count;
}
/* not support the function*/
static ssize_t lcdkit_support_bl_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    return count;
}
static ssize_t lcdkit_support_bl_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    ssize_t ret;
    struct lcdkit_panel_data* lcdkit_info_temp;
    lcdkit_info_temp = lcdkit_get_panel_info();

    if (NULL == lcdkit_info_temp)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    ret = lcd_support_bl_mode_show(dev, lcdkit_info_temp, buf);
    return ret;
}

static ssize_t lcdkit_ldo_check_show(struct device* dev,struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer\n");
        return -EINVAL;
    }
    ret = lcd_ldo_check_show(dev, buf);
    return ret;
}
static ssize_t lcdkit_mipi_config_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = -EINVAL;
    struct lcdkit_panel_data* lcdkit_info= lcdkit_get_panel_info();

    if (NULL == dev || NULL == attr || NULL == buf || NULL == lcdkit_info)
    {
        LCDKIT_ERR("%s: NULL Pointer!\n",__func__);
        return ret;
    }

    if(!lcdkit_info->panel_infos.mipi_clk_config_support)
    {
        LCDKIT_ERR("mipi_clk config not support\n");
        return ret;
    }

    ret = lcd_mipi_config_store(dev, lcdkit_info, buf);
    if (ret)
    {
        LCDKIT_ERR("lcd_mipi_config_store return fail\n");
        return ret;
    }

    return count;
}

static DEVICE_ATTR(lcd_model, 0644, lcdkit_lcd_model_show, NULL);
static DEVICE_ATTR(lcd_display_type, 0644, lcdkit_lcd_display_type_show, NULL);
static DEVICE_ATTR(panel_info, 0644, lcdkit_lcd_panel_info_show, NULL);
static DEVICE_ATTR(lcd_cabc_mode, S_IRUGO | S_IWUSR, lcdkit_lcd_cabc_mode_show, lcdkit_lcd_cabc_mode_store);
static DEVICE_ATTR(lcd_inversion_mode, S_IRUGO | S_IWUSR, lcdkit_lcd_inversion_mode_show, lcdkit_lcd_inversion_mode_store);
static DEVICE_ATTR(lcd_scan_mode, S_IRUGO | S_IWUSR, lcdkit_lcd_scan_mode_show, lcdkit_lcd_scan_mode_store);
static DEVICE_ATTR(lcd_check_reg, S_IRUGO, lcdkit_lcd_check_reg_show, NULL);
static DEVICE_ATTR(lcd_checksum, S_IRUGO | S_IWUSR, lcdkit_lcd_gram_check_show, lcdkit_lcd_gram_check_store);
static DEVICE_ATTR(lcd_dynamic_checksum, S_IRUGO | S_IWUSR, lcdkit_dynamic_sram_check_show, lcdkit_dynamic_sram_check_store);
static DEVICE_ATTR(lcd_ic_color_enhancement_mode, S_IRUGO | S_IWUSR, lcdkit_ic_color_enhancement_mode_show, lcdkit_ic_color_enhancement_mode_store);
static DEVICE_ATTR(lcd_sleep_ctrl, S_IRUGO | S_IWUSR, lcdkit_sleep_ctrl_show, lcdkit_sleep_ctrl_store);
static DEVICE_ATTR(lcd_lp2hs_mipi_check, S_IRUGO | S_IWUSR, lcdkit_lp2hs_mipi_check_show, lcdkit_lp2hs_mipi_check_store);
static DEVICE_ATTR(lcd_bist_check, S_IRUSR | S_IRGRP, lcdkit_lcd_bist_check, NULL);
static DEVICE_ATTR(lcd_mipi_detect, S_IRUGO, lcdkit_lcd_mipi_detect_show, NULL);
static DEVICE_ATTR(lcd_hkadc, S_IRUGO | S_IWUSR, lcdkit_lcd_hkadc_debug_show, lcdkit_lcd_hkadc_debug_store);
static DEVICE_ATTR(lcd_voltage_enable, S_IWUSR, NULL, lcdkit_lcd_voltage_enable_store);
static DEVICE_ATTR(amoled_pcd_errflag_check, 0644, lcdkit_amoled_pcd_errflag_check, NULL);
static DEVICE_ATTR(amoled_acl, S_IRUGO | S_IWUSR, lcdkit_amoled_acl_ctrl_show, lcdkit_amoled_acl_ctrl_store);
static DEVICE_ATTR(amoled_vr_mode, 0644, lcdkit_amoled_vr_mode_show, lcdkit_amoled_vr_mode_store);
static DEVICE_ATTR(amoled_hbm, S_IRUGO | S_IWUSR, lcdkit_amoled_hbm_ctrl_show, lcdkit_amoled_hbm_ctrl_store);
static DEVICE_ATTR(lcd_support_mode, S_IRUGO | S_IWUSR, lcdkit_lcd_support_mode_show, lcdkit_lcd_support_mode_store);
static DEVICE_ATTR(lcd_comform_mode, S_IRUGO | S_IWUSR, lcdkit_comform_mode_show, lcdkit_comform_mode_store);
static DEVICE_ATTR(lcd_cinema_mode, S_IRUGO | S_IWUSR, lcdkit_cinema_mode_show, lcdkit_cinema_mode_store);
static DEVICE_ATTR(lcd_support_checkmode, S_IRUGO | S_IWUSR, lcdkit_support_checkmode_show, NULL);
static DEVICE_ATTR(led_rg_lcd_color_temperature, S_IRUGO | S_IWUSR, lcdkit_led_rg_lcd_color_temperature_show, lcdkit_led_rg_lcd_color_temperature_store);
static DEVICE_ATTR(lcd_ce_mode, S_IRUGO | S_IWUSR, lcdkit_lcd_ce_mode_show, lcdkit_lcd_ce_mode_store);
static DEVICE_ATTR(effect_al, S_IRUGO | S_IWUSR, lcdkit_effect_al_show, lcdkit_effect_al_store);
static DEVICE_ATTR(effect_ce, S_IRUGO | S_IWUSR, lcdkit_effect_ce_show, lcdkit_effect_ce_store);
static DEVICE_ATTR(effect_hdr_mode, S_IRUGO|S_IWUSR, lcdkit_effect_hdr_mode_show, lcdkit_effect_hdr_mode_store);
static DEVICE_ATTR(effect_bl, S_IRUGO, lcdkit_effect_bl_show, NULL);
static DEVICE_ATTR(effect_bl_enable, S_IRUGO | S_IWUSR, lcdkit_effect_bl_enable_show, lcdkit_effect_bl_enable_store);
static DEVICE_ATTR(effect_sre, S_IRUGO|S_IWUSR, lcdkit_effect_sre_show, lcdkit_effect_sre_store);
static DEVICE_ATTR(effect_metadata, S_IRUGO | S_IWUSR, lcdkit_effect_metadata_show, lcdkit_effect_metadata_store);
static DEVICE_ATTR(effect_available, S_IRUGO, lcdkit_effect_available_show, NULL);
static DEVICE_ATTR(gamma_dynamic, S_IRUGO | S_IWUSR, NULL, lcdkit_gamma_dynamic_store);
static DEVICE_ATTR(2d_sharpness, 0600, lcdkit_2d_sharpness_show, lcdkit_2d_sharpness_store);
static DEVICE_ATTR(lcd_acm_state, S_IRUGO | S_IWUSR, lcdkit_acm_state_show, lcdkit_acm_state_store);
static DEVICE_ATTR(lcd_gmp_state, S_IRUGO | S_IWUSR, lcdkit_lcd_gmp_state_show, lcdkit_lcd_gmp_state_store);
static DEVICE_ATTR(sbl_ctrl, S_IRUGO | S_IWUSR, lcdkit_sbl_ctrl_show, lcdkit_sbl_ctrl_store);
static DEVICE_ATTR(lcd_color_temperature, S_IRUGO | S_IWUSR, lcdkit_color_temperature_show, lcdkit_color_temperature_store);
static DEVICE_ATTR(frame_count, S_IRUGO, lcdkit_lcd_frame_count_show, NULL);
static DEVICE_ATTR(frame_update, S_IRUGO | S_IWUSR, lcdkit_lcd_frame_update_show, lcdkit_lcd_frame_update_store);
static DEVICE_ATTR(mipi_dsi_bit_clk_upt, S_IRUGO | S_IWUSR, lcdkit_mipi_dsi_bit_clk_upt_show, lcdkit_mipi_dsi_bit_clk_upt_store);
static DEVICE_ATTR(lcd_fps_scence, (S_IRUGO | S_IWUSR), lcdkit_lcd_fps_scence_show, lcdkit_lcd_fps_scence_store);
static DEVICE_ATTR(alpm_function, 0644, lcdkit_alpm_function_show, lcdkit_alpm_function_store);
static DEVICE_ATTR(alpm_setting, 0644, NULL, lcdkit_alpm_setting_store);
static DEVICE_ATTR(lcd_func_switch, S_IRUGO | S_IWUSR, lcdkit_lcd_func_switch_show, lcdkit_lcd_func_switch_store);
static DEVICE_ATTR(lcd_dynamic_porch, S_IRUGO | S_IWUSR, lcdkit_lcd_dynamic_porch_show, lcdkit_lcd_dynamic_porch_store);
static DEVICE_ATTR(lcd_test_config, 0640, lcdkit_test_config_show, lcdkit_test_config_store);
static DEVICE_ATTR(lv_detect, 0640, lcdkit_lv_detect_show, NULL);
static DEVICE_ATTR(current_detect, 0640, lcdkit_current_detect_show, NULL);
static DEVICE_ATTR(lcd_reg_read, 0600, lcdkit_reg_read_show, lcdkit_reg_read_store);
static DEVICE_ATTR(ddic_oem_info, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, lcdkit_ddic_oem_info_show, lcdkit_ddic_oem_info_store);
static DEVICE_ATTR(lcd_bl_mode, S_IRUGO | S_IWUSR, lcdkit_bl_mode_show, lcdkit_bl_mode_store);
static DEVICE_ATTR(lcd_se_mode, S_IRUGO | S_IWUSR, lcdkit_se_mode_show, lcdkit_se_mode_store);
static DEVICE_ATTR(lcd_bl_support_mode, S_IRUGO | S_IWUSR, lcdkit_support_bl_mode_show, lcdkit_support_bl_mode_store);
static DEVICE_ATTR(lcd_ldo_check, S_IRUGO, lcdkit_ldo_check_show, NULL);
static DEVICE_ATTR(lcd_mipi_config, 0644, NULL, lcdkit_mipi_config_store);

static struct attribute* lcdkit_fb_attrs[] =
{
    &dev_attr_lcd_model.attr,
    &dev_attr_lcd_display_type.attr,
    &dev_attr_panel_info.attr,
    &dev_attr_lcd_cabc_mode.attr,
    &dev_attr_lcd_inversion_mode.attr,
    &dev_attr_lcd_scan_mode.attr,
    &dev_attr_lcd_check_reg.attr,
    &dev_attr_lcd_checksum.attr,
    &dev_attr_lcd_dynamic_checksum.attr,
    &dev_attr_lcd_ic_color_enhancement_mode.attr,
    &dev_attr_lcd_sleep_ctrl.attr,
    &dev_attr_lcd_lp2hs_mipi_check.attr,
    &dev_attr_lcd_bist_check.attr,
    &dev_attr_lcd_mipi_detect.attr,
    &dev_attr_lcd_hkadc.attr,
    &dev_attr_lcd_voltage_enable.attr,
    &dev_attr_amoled_pcd_errflag_check.attr,
    &dev_attr_amoled_acl.attr,
    &dev_attr_amoled_vr_mode.attr,
    &dev_attr_amoled_hbm.attr,
    &dev_attr_lcd_support_mode.attr,
    &dev_attr_lcd_comform_mode.attr,
    &dev_attr_lcd_cinema_mode.attr,
    &dev_attr_lcd_support_checkmode.attr,
    &dev_attr_led_rg_lcd_color_temperature.attr,
    &dev_attr_lcd_ce_mode.attr,
    &dev_attr_effect_al.attr,
    &dev_attr_effect_ce.attr,
    &dev_attr_lcd_se_mode.attr,
    &dev_attr_effect_hdr_mode.attr,
    &dev_attr_effect_bl.attr,
    &dev_attr_effect_bl_enable.attr,
    &dev_attr_effect_sre.attr,
    &dev_attr_effect_metadata.attr,
    &dev_attr_effect_available.attr,
    &dev_attr_gamma_dynamic.attr,
    &dev_attr_2d_sharpness.attr,
    &dev_attr_lcd_acm_state.attr,
    &dev_attr_lcd_gmp_state.attr,
    &dev_attr_sbl_ctrl.attr,
    &dev_attr_lcd_color_temperature.attr,
    &dev_attr_frame_count.attr,
    &dev_attr_frame_update.attr,
    &dev_attr_mipi_dsi_bit_clk_upt.attr,
    &dev_attr_lcd_fps_scence.attr,
    &dev_attr_alpm_function.attr,
    &dev_attr_alpm_setting.attr,
    &dev_attr_lcd_func_switch.attr,
    &dev_attr_lcd_dynamic_porch.attr,
    &dev_attr_lcd_test_config.attr,
    &dev_attr_lv_detect.attr,
    &dev_attr_current_detect.attr,
    &dev_attr_lcd_reg_read.attr,
    &dev_attr_ddic_oem_info.attr,
    &dev_attr_lcd_bl_mode.attr,
    &dev_attr_lcd_bl_support_mode.attr,
    &dev_attr_lcd_ldo_check.attr,
    &dev_attr_lcd_mipi_config.attr,
    NULL,
};

struct attribute_group lcdkit_fb_attr_group =
{
    .attrs = lcdkit_fb_attrs,
};

int lcdkit_fb_create_sysfs(struct kobject* obj)
{
    int rc;

    rc = sysfs_create_group(obj, &lcdkit_fb_attr_group);

    if (rc)
    { pr_err("sysfs group creation failed, rc=%d\n", rc); }

    return rc;
}

void lcdkit_fb_remove_sysfs(struct kobject* obj)
{
    sysfs_remove_group(obj, &lcdkit_fb_attr_group);
}
