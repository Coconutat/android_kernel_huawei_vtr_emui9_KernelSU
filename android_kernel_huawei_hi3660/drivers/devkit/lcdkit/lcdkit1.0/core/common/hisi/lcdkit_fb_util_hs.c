#include "hisi_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "hisi_fb_debug.h"
#include "lcdkit_disp.h"
#include "hisi_display_effect.h"

#define MAX_BUF 60

ssize_t get_lcdkit_support(void)
{
    struct device_node* lcdkit_np = NULL;
    char* support_type = NULL;
    ssize_t ret = 0;

    lcdkit_np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCDKIT_PANEL_TYPE);

    if (!lcdkit_np)
    {
        LCDKIT_ERR("NOT FOUND device node %s!\n", DTS_COMP_FB_NAME);
        return -ENXIO;
    }

    ret = of_property_read_string(lcdkit_np, "support_lcd_type", &support_type);

    if (ret)
    {
        LCDKIT_ERR("failed to get support_type.\n");
        printk("get_lcdkit_support \n");
        return -ENXIO;
    }

    if (!strncmp(support_type, "LCDKIT", strlen("LCDKIT")))
    {
        LCDKIT_ERR("lcdkit is support!\n");
        return 1;
    }
    else
    {
        LCDKIT_ERR("lcdkit is not support!\n");
        return 0;
    }

}

struct hisi_fb_data_type* get_fb_data(struct device* dev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    struct fb_info* fbi = NULL;

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

    hisifd = (struct hisi_fb_data_type*)fbi->par;
    return hisifd;
}

ssize_t lcd_cabc_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if ( NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_cabc_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_cabc_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_inversion_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd =  get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_inversion_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_inversion_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_scan_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_scan_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_scan_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_check_reg_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_check_reg_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_check_reg_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_gram_check_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_gram_check_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_gram_check_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_gram_check_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_DEBUG("fb%d, panel power off!\n", hisifd->index);
        ret = 0;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_gram_check_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_gram_check_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_dynamic_sram_check_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_dynamic_sram_check_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_dynamic_sram_check_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_dynamic_sram_check_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_DEBUG("fb%d, panel power off!\n", hisifd->index);
        ret = 0;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_dynamic_sram_check_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_dynamic_sram_check_store(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_ic_color_enhancement_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_ic_color_enhancement_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_ic_color_enhancement_mode_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_ic_color_enhancement_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_ic_color_enhancement_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_ic_color_enhancement_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_sleep_ctrl_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_sleep_ctrl_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_sleep_ctrl_store(buf);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcdkit_sleep_ctrl_store is NULL\n");
    }
err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_lp2hs_mipi_check_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (lcdkit_info->lcdkit_lp2hs_mipi_check_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_lp2hs_mipi_check_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_lp2hs_mipi_check_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (lcdkit_info->lcdkit_lp2hs_mipi_check_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_lp2hs_mipi_check_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_bist_check(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    char lcd_bist_check_result[BIS_CHECK_COUNT] = {0};

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_bist_check_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_bist_check_show(hisifd, lcd_bist_check_result);
        hisifb_deactivate_vsync(hisifd);
    }

    ret = snprintf(buf, PAGE_SIZE, "%s", lcd_bist_check_result);
    LCDKIT_DEBUG("LCD bist check result : %s\n", lcd_bist_check_result);

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_mipi_detect_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_mipi_detect_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_mipi_detect_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcd_mipi_detect is NULL\n");
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_voltage_mode_enable_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if ( lcdkit_info->lcdkit_voltage_enable_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_voltage_enable_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_acl_ctrl_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_amoled_acl_ctrl_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_amoled_acl_ctrl_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_acl_ctrl_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_amoled_acl_ctrl_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_amoled_acl_ctrl_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_amoled_vr_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_amoled_vr_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_amoled_vr_mode_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_amoled_vr_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_amoled_vr_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_amoled_vr_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_hbm_ctrl_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto error;
    }

    if (lcdkit_info->lcdkit_amoled_hbm_ctrl_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_amoled_hbm_ctrl_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

error:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_support_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    char lcd_bist_check_result[BIS_CHECK_COUNT] = {0};

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_support_mode_show)
    {
        ret = lcdkit_info->lcdkit_support_mode_show(buf);
    }

    return ret;
}

ssize_t lcd_support_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_support_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_support_mode_store(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}



ssize_t lcd_comform_mode_show(struct device* dev,
                              struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);
    if (NULL == pdata)
    {
        LCDKIT_ERR("pdata is NULL Pointer!\n");
        return -EINVAL;
    }
    if (NULL == buf)
    {
        LCDKIT_ERR("bud is NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_comform_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_comform_mode_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcd_color_temperature_show is NULL\n");
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_comform_mode_store(struct device* dev,
                               struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("pdata is NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        dpe_update_g_comform_discount(0);
        HISI_FB_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_comform_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_comform_mode_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return count;
}

ssize_t lcd_cinema_mode_show(struct device* dev,
                             struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_cinema_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_cinema_mode_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcd_color_temperature_show is NULL\n");
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_cinema_mode_store(struct device* dev,
                              struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        HISI_FB_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_cinema_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_cinema_mode_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return count;
}

ssize_t lcd_support_checkmode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    char lcd_bist_check_result[BIS_CHECK_COUNT] = {0};

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (lcdkit_info->lcdkit_support_checkmode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_support_checkmode_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

    return ret;
}

ssize_t led_rg_lcd_color_temperature_show(struct device* dev,
        struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }


    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->led_rg_lcd_color_temperature_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->led_rg_lcd_color_temperature_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t led_rg_lcd_color_temperature_store(struct device* dev,
        struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR(" dev is NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR(" fbi is NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("pdata is NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->led_rg_lcd_color_temperature_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->led_rg_lcd_color_temperature_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return count;
}

ssize_t lcd_ce_mode_show(struct device* dev,
                         struct device_attribute* attr, char* buf)
{
    ssize_t ret = -1;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (pdata->lcd_ce_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_ce_mode_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

    return ret;
}

ssize_t lcd_ce_mode_store(struct device* dev,
                          struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = -1;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (pdata->lcd_ce_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_ce_mode_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return count;
}

ssize_t effect_al_show(struct device* dev,
                       struct device_attribute* attr, char* buf)
{
    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_al_ctrl_show(dev_get_drvdata(dev), buf);
}

ssize_t effect_al_store(struct device* dev,
                        struct device_attribute* attr, const char* buf, size_t count)
{
    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_al_ctrl_store(dev_get_drvdata(dev), buf, count);
}
ssize_t effect_ce_show(struct device* dev,
                       struct device_attribute* attr, char* buf)
{
    struct fb_info* fbi = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    return hisifb_display_effect_ce_ctrl_show(fbi, buf);
}

ssize_t effect_ce_store(struct device* dev,
                        struct device_attribute* attr, const char* buf, size_t count)
{
    struct fb_info* fbi = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    return hisifb_display_effect_ce_ctrl_store(fbi, buf, count);
}

ssize_t lcd_effect_sre_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    (void)attr;

    if (NULL == dev) {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf) {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_sre_ctrl_show(dev_get_drvdata(dev), buf);
}

ssize_t lcd_effect_sre_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    (void)attr;

    if (NULL == dev) {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf) {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_sre_ctrl_store(dev_get_drvdata(dev), buf, count);
}

ssize_t effect_bl_show(struct device* dev,
                       struct device_attribute* attr, char* buf)
{
    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_bl_ctrl_show(dev_get_drvdata(dev), buf);
}

ssize_t effect_bl_enable_show(struct device* dev,
                              struct device_attribute* attr, char* buf)
{
    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_bl_enable_ctrl_show(dev_get_drvdata(dev), buf);
}

ssize_t effect_bl_enable_store(struct device* dev,
                               struct device_attribute* attr, const char* buf, size_t count)
{
    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    return hisifb_display_effect_bl_enable_ctrl_store(dev_get_drvdata(dev), buf, count);
}

ssize_t effect_metadata_show(struct device* dev,
                             struct device_attribute* attr, char* buf)
{
    struct fb_info* fbi = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);
    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

#if defined(CONFIG_HISI_FB_3660)
    return hisifb_display_effect_metadata_ctrl_show(fbi, buf);
#else
    return 0;
#endif
}

ssize_t effect_metadata_store(struct device* dev,
                              struct device_attribute* attr, const char* buf, size_t count)
{
    struct fb_info* fbi = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

#if defined(CONFIG_HISI_FB_3660)
    return hisifb_display_effect_metadata_ctrl_store(fbi, buf, count);
#else
    return 0;
#endif
}

ssize_t effect_available_show(struct device* dev,
                              struct device_attribute* attr, char* buf)
{
    int value = 0;       //bit0:hiace; bit1:csc; bit2:bitextend; bit3:dither; bit4:arsr1p; bit5:sbl; bit6:acm; bit7:igm; bit8:xcc; bit9:gmp; bit10:gamma
    struct fb_info* fbi = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

#if defined(CONFIG_HISI_FB_3660)
    value = 0x7ff;
#else
    value = 0;
#endif

    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

ssize_t gamma_dynamic_store(struct device* dev,
                            struct device_attribute* attr, const char* buf, size_t count)
{
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("dev is NULL Pointer!\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("fbi is NULL Pointer!\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("hisifd is NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("buf is NULL Pointer!\n");
        return -1;
    }

#if defined(CONFIG_HISI_FB_3660) || defined (CONFIG_HISI_FB_970)

    if (count != GM_IGM_LEN)
    {
        HISI_FB_ERR("gamma count error! count = %d \n", (int)count);
        return -1;
    }

    hisifb_update_dynamic_gamma(hisifd, buf, count);
#endif

    return count;
}

ssize_t  lcd_2d_sharpness_show(struct device* dev,
                               struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (pdata->sharpness2d_table_show)
    {
        ret = pdata->sharpness2d_table_show(hisifd->pdev, buf);
    }

    return ret;
}

ssize_t lcd_2d_sharpness_store(struct device* dev,
                               struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (pdata->sharpness2d_table_store)
    {
        ret = pdata->sharpness2d_table_store(hisifd->pdev, buf, count);
    }

    return ret;
}

ssize_t lcd_acm_state_show(struct device* dev,
                           struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_acm_state_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_acm_state_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_acm_state_store(struct device* dev,
                            struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_acm_state_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_acm_state_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return count;
}

ssize_t lcd_gmp_state_show(struct device* dev,
                           struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_gmp_state_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_gmp_state_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_gmp_state_store(struct device* dev,
                            struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_gmp_state_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_gmp_state_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return count;
}

ssize_t sbl_ctrl_show(struct device* dev,
                      struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Pointer!\n");
        return -EINVAL;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("fbi is NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return 0;
    }

    ret = snprintf(buf, PAGE_SIZE, "sbl_lsensor_value=%d, sbl_level=%d, sbl_enable=%d\n",
                   hisifd->sbl_lsensor_value, hisifd->sbl_level, hisifd->sbl_enable);
    LCDKIT_DEBUG("sbl_lsensor_value=%d, sbl_level=%d, sbl_enable=%d", hisifd->sbl_lsensor_value, hisifd->sbl_level, hisifd->sbl_enable);

    return ret;
}

ssize_t sbl_ctrl_store(struct device* dev,
                       struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    int val = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    val = (int)simple_strtoul(buf, NULL, 0);

    if (hisifd->sbl_ctrl_fnc)
    {
        ret = hisifd->sbl_ctrl_fnc(fbi, val);
    }

    return count;
}

ssize_t lcd_color_temperature_show(struct device* dev,
                                   struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_color_temperature_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_color_temperature_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcd_color_temperature_show is NULL\n");
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_color_temperature_store(struct device* dev,
                                    struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("pdata is NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->lcd_color_temperature_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->lcd_color_temperature_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }
    else
    {
        LCDKIT_ERR("lcd_color_temperature_store is NULL\n");
    }

err_out:
    up(&hisifd->blank_sem);
    return count;
}

ssize_t lcd_frame_count_show(struct device* dev,
                             struct device_attribute* attr, char* buf)
{
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    return snprintf(buf, PAGE_SIZE, "%u\n", hisifd->frame_count);

}

ssize_t lcd_frame_update_show(struct device* dev,
                              struct device_attribute* attr, char* buf)
{
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    return snprintf(buf, PAGE_SIZE, "%u\n", hisifd->vsync_ctrl.vsync_infinite);
}

ssize_t lcd_frame_update_store(struct device* dev,
                               struct device_attribute* attr, const char* buf, size_t count)
{
    int val = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    static uint32_t esd_enable = 0;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    val = (int)simple_strtoul(buf, NULL, 0);

    HISI_FB_INFO("fb%d, val=%d.\n", hisifd->index, val);

    down(&hisifd->blank_sem);

    g_enable_dirty_region_updt =  (val > 0) ? 0 : 1;
    hisifd->frame_update_flag = (val > 0) ? 1 : 0;
    hisifb_set_vsync_activate_state(hisifd, (val > 0) ? true : false);

    if (!is_mipi_cmd_panel(hisifd))
    {
        goto err_out;
    }

    if (!hisifd->panel_power_on)
    {
        HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    hisifb_activate_vsync(hisifd);

    if (val == 1)
    {
        esd_enable = hisifd->panel_info.esd_enable;
        hisifd->panel_info.esd_enable = 0;
        mdelay(50);
    }

    ldi_frame_update(hisifd, (val > 0) ? true : false);

    if (val == 0)
    {
        hisifd->vactive0_start_flag = 1;
        mdelay(50);
        hisifd->panel_info.esd_enable = esd_enable;
        esd_enable = 0;
    }

    hisifb_deactivate_vsync(hisifd);

err_out:
    up(&hisifd->blank_sem);

    return count;
}

ssize_t mipi_dsi_bit_clk_upt_show(struct device* dev,
                                  struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->mipi_dsi_bit_clk_upt_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->mipi_dsi_bit_clk_upt_show(hisifd->pdev, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}


ssize_t mipi_dsi_bit_clk_upt_store(struct device* dev,
                                   struct device_attribute* attr, const char* buf, size_t count)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        HISI_FB_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (pdata->mipi_dsi_bit_clk_upt_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = pdata->mipi_dsi_bit_clk_upt_store(hisifd->pdev, buf, count);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return count;
}

ssize_t lcd_fps_scence_show(struct device* dev,
                            struct device_attribute* attr, char* buf)
{
    ssize_t ret = -1;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    ret = snprintf(buf, PAGE_SIZE, "lcd_fps = %d \n", hisifd->panel_info.fps);

    return ret;
}

ssize_t lcd_fps_scence_store(struct device* dev, struct device_attribute* attr,
                             const char* buf, size_t count)
{
    ssize_t ret = -1;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;
    uint32_t val = 0;

    if (NULL == dev)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        HISI_FB_ERR("NULL Pointer\n");
        return -1;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata )
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (NULL == buf)
    {
        HISI_FB_ERR("NULL Pointer!\n");
        return -1;
    }

    if (!hisifd->panel_power_on)
    {
        HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
        return -1;
    }

    val = (uint32_t)simple_strtoul(buf, NULL, 0);
    if (pdata->lcd_fps_scence_handle)
    {
        ret = pdata->lcd_fps_scence_handle(hisifd->pdev, val);
    }

    return count;
}

ssize_t alpm_function_show(struct device* dev,
                           struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	if (!dev) {
		HISI_FB_ERR("alpm function show NULL dev Pointer!\n");
		return -1;
	}

	if (!buf) {
		HISI_FB_ERR("alpm function show NULL buf Pointer!\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (!fbi) {
		HISI_FB_ERR("alpm function show NULL fbi Pointer!\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	if (!hisifd) {
		HISI_FB_ERR("alpm function show NULL hisifd Pointer!\n");
		return -1;
	}
	ret = snprintf(buf, PAGE_SIZE, "aod_function = %d \n", hisifd->aod_function);

    return ret;
}

ssize_t alpm_function_store(struct device* dev,
                            struct device_attribute* attr, const char* buf)
{
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (!dev) {
		HISI_FB_ERR("alpm function store NULL dev Pointer!\n");
		return -1;
	}

	if (!buf) {
		HISI_FB_ERR("alpm function store NULL buf Pointer!\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (!fbi) {
		HISI_FB_ERR("alpm function store NULL fbi Pointer!\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	if (!hisifd) {
		HISI_FB_ERR("alpm function storeNULL hisifd Pointer!\n");
		return -1;
	}
	if (strlen(buf) >= MAX_BUF) {
		HISI_FB_ERR("buf overflow!\n");
		return -1;
	}

	if (!hisifd->panel_power_on) {
		HISI_FB_ERR("fb%d, panel power off!\n", hisifd->index);
		return -1;
	}

	ret = sscanf(buf, "%u", &hisifd->aod_function);
	if (!ret) {
		HISI_FB_ERR("sscanf return invaild:%d\n", ret);
		return -1;
	}
        LCDKIT_INFO("[AOD function switch] hisifd->aod_function = %d !\n",hisifd->aod_function);
    return ret;
}

ssize_t alpm_setting_store(struct device* dev,
                           struct device_attribute* attr, const char* buf, size_t count)
{
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	ssize_t ret = 0;

	if (!dev) {
		HISI_FB_ERR("alpm setting store NULL dev Pointer!\n");
		return -1;
	}

	if (!buf) {
		HISI_FB_ERR("alpm setting store NULL buf Pointer!\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (!fbi) {
		HISI_FB_ERR("alpm setting store NULL fbi Pointer!\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	if (!hisifd) {
		HISI_FB_ERR("alpm setting store NULL hisifd Pointer!\n");
		return -1;
	}

	if (strlen(buf) >= MAX_BUF) {
		HISI_FB_ERR("buf overflow!\n");
		return -1;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (!pdata) {
		HISI_FB_ERR("NULL pdata Pointer!\n");
		return -1;
	}

	down(&hisifd->blank_sem);
	pinfo = &(hisifd->panel_info);
	if (!hisifd->panel_power_on) {
		HISI_FB_ERR("fb%d, panel power off!\n", hisifd->index);
		goto err_out;
	}

	if (pdata->amoled_alpm_setting_store) {
		hisifb_activate_vsync(hisifd);
		ret = pdata->amoled_alpm_setting_store(hisifd->pdev, buf, count);
		hisifb_deactivate_vsync(hisifd);
	}

err_out:
	up(&hisifd->blank_sem);
    return count;
}

ssize_t lcd_func_switch_show(struct device* dev,
                             struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;
    struct hisi_panel_info* pinfo = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pinfo = &(hisifd->panel_info);

    ret = snprintf(buf, PAGE_SIZE,
        "sbl=%d\n"
        "xcc_support=%d\n"
        "dsi_bit_clk_upt=%d\n"
        "dirty_region_upt=%d\n"
        "fps_updt_support=%d\n"
        "ifbc_type=%d\n"
        "esd_enable=%d\n"
        "blpwm_input_ena=%d\n"
        "blpwm_precision_type=%d\n"
        "lane_nums=%d\n"
        "panel_effect_support=%d\n"
        "color_temp_rectify_support=%d\n"
        "ddic_rgbw_support=%d\n"
        "hiace=%d\n"
        "effect_enable=%d\n"
        "effect_debug=%d\n"
        "fps_func_switch=%d\n",
        pinfo->sbl_support,
        pinfo->xcc_support,
        pinfo->dsi_bit_clk_upt_support,
        pinfo->dirty_region_updt_support,
        pinfo->fps_updt_support,
        pinfo->ifbc_type,
        pinfo->esd_enable,
        pinfo->blpwm_input_ena,
        pinfo->blpwm_precision_type,
        pinfo->mipi.lane_nums + 1,
        pinfo->panel_effect_support,
        pinfo->color_temp_rectify_support,
        pinfo->rgbw_support,
        pinfo->hiace_support,
        g_enable_effect,
        g_debug_effect,
        lcdkit_info.panel_infos.fps_func_switch);

    return ret;
}

static u32 xcc_table_def[12] = {0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000,};
ssize_t lcd_func_switch_store(struct device* dev,
                              struct device_attribute* attr, const char* buf, size_t count)
{
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;
    struct hisi_panel_info* pinfo = NULL;
    char command[MAX_BUF] = {0};

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return 0;
    }

       if (strlen(buf) >= MAX_BUF) {
            HISI_FB_ERR("buf overflow!\n");
            return -1;
        }

    pinfo = &(hisifd->panel_info);

    if (!sscanf(buf, "%s", command))  /* [false alarm] */
    {
        LCDKIT_INFO("bad command(%s)\n", command);
        return count;
    }

    down(&hisifd->blank_sem);

    hisifb_activate_vsync(hisifd);

    if (!strncmp("sbl:", command, strlen("sbl:")))
    {
        if ('0' == command[strlen("sbl:")])
        {
            pinfo->sbl_support = 0;
            LCDKIT_INFO("sbl disable\n");
        }
        else
        {
            pinfo->sbl_support = 1;
            LCDKIT_INFO("sbl enable\n");
        }
    }

    if (!strncmp("xcc_support:", command, strlen("xcc_support:")))
    {
        if ('0' == command[strlen("xcc_support:")])
        {
            pinfo->xcc_support = 0;

            if (pinfo->xcc_table)
            {
                pinfo->xcc_table[1] = 0x8000;
                pinfo->xcc_table[6] = 0x8000;
                pinfo->xcc_table[11] = 0x8000;
            }

            LCDKIT_INFO("xcc_support disable\n");
        }
        else
        {
            pinfo->xcc_support = 1;

            if (pinfo->xcc_table == NULL)
            {
                pinfo->xcc_table = xcc_table_def;
                pinfo->xcc_table_len = ARRAY_SIZE(xcc_table_def);
            }

            LCDKIT_INFO("xcc_support enable\n");
        }
    }

    if (!strncmp("dsi_bit_clk_upt:", command, strlen("dsi_bit_clk_upt:")))
    {
        if ('0' == command[strlen("dsi_bit_clk_upt:")])
        {
            pinfo->dsi_bit_clk_upt_support = 0;
            LCDKIT_INFO("dsi_bit_clk_upt disable\n");
        }
        else
        {
            pinfo->dsi_bit_clk_upt_support = 1;
            LCDKIT_INFO("dsi_bit_clk_upt enable\n");
        }
    }

    if (!strncmp("dirty_region_upt:", command, strlen("dirty_region_upt:")))
    {
        if ('0' == command[strlen("dirty_region_upt:")])
        {
            pinfo->dirty_region_updt_support = 0;
            LCDKIT_INFO("dirty_region_upt disable\n");
        }
        else
        {
            pinfo->dirty_region_updt_support = 1;
            LCDKIT_INFO("dirty_region_upt enable\n");
        }
    }

    if (!strncmp("ifbc_type:", command, strlen("ifbc_type:")))
    {
        if ('0' == command[strlen("ifbc_type:")])
        {
            if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE)
            {
                //ldi
                pinfo->ldi.h_back_porch *= pinfo->pxl_clk_rate_div;
                pinfo->ldi.h_front_porch *= pinfo->pxl_clk_rate_div;
                pinfo->ldi.h_pulse_width *= pinfo->pxl_clk_rate_div;

                pinfo->pxl_clk_rate_div = 1;
                pinfo->ifbc_type = IFBC_TYPE_NONE;
                LCDKIT_INFO("ifbc_type changed to IFBC_TYPE_NONE\n");
            }
        }
        else if ('7' == command[strlen("ifbc_type:")])
        {
            if (pinfo->ifbc_type == IFBC_TYPE_NONE)
            {
                pinfo->pxl_clk_rate_div = 3;

                //ldi
                pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
                pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
                pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;

                pinfo->ifbc_type = IFBC_TYPE_VESA3X_SINGLE;
                LCDKIT_INFO("ifbc_type changed to IFBC_TYPE_VESA3X_SINGLE\n");
            }
        }
    }

    if (!strncmp("esd_enable:", command, strlen("esd_enable:")))
    {
        if ('0' == command[strlen("esd_enable:")])
        {
            pinfo->esd_enable = 0;
            LCDKIT_INFO("esd_enable disable\n");
        }
        else
        {
            pinfo->esd_enable = 1;
            LCDKIT_INFO("esd_enable enable\n");
        }
    }

    if (!strncmp("fps_updt_support:", command, strlen("fps_updt_support:"))) {
        if('0' == command[strlen("fps_updt_support:")]) {
            pinfo->fps_updt_support = 0;
            HISI_FB_INFO("fps_updt_support disable\n");
            } else {
                pinfo->fps_updt_support = 1;
                HISI_FB_INFO("fps_updt_support enable\n");
            }
        }

    if (!strncmp("fps_func_switch:", command, strlen("fps_func_switch:"))) {
        if('0' == command[strlen("fps_func_switch:")]) {
            lcdkit_info.panel_infos.fps_func_switch = 0;
            HISI_FB_INFO("fps_func_switch disable\n");
        } else {
            lcdkit_info.panel_infos.fps_func_switch = 1;
            HISI_FB_INFO("fps_func_switch enable\n");
        }
    }

    if (!strncmp("blpwm_input_ena:", command, strlen("blpwm_input_ena:")))
    {
        if ('0' == command[strlen("blpwm_input_ena:")])
        {
            pinfo->blpwm_input_ena = 0;
            LCDKIT_INFO("blpwm_input_ena disable\n");
        }
        else
        {
            pinfo->blpwm_input_ena = 1;
            LCDKIT_INFO("blpwm_input_ena enable\n");
        }
    }

    if (!strncmp("blpwm_precision_type:", command, strlen("blpwm_precision_type:")))
    {
        if ('0' == command[strlen("blpwm_precision_type:")])
        {
            pinfo->blpwm_precision_type = 0;
            LCDKIT_INFO("blpwm_precision_type default\n");
        }
        else
        {
            pinfo->blpwm_precision_type = BLPWM_PRECISION_10000_TYPE;
            LCDKIT_INFO("blpwm_precision_type BLPWM_PRECISION_10000_TYPE\n");
        }
    }

    if (!strncmp("lane_nums:", command, strlen("lane_nums:")))
    {
        if (hisifd->panel_power_on)
        {
            LCDKIT_ERR("fb%d, lane_nums can be changed when panel power off, BUT panel power on!\n", hisifd->index);
            goto out;
        }

        if (('1' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_1_LANES_SUPPORT))
        {
            pinfo->mipi.lane_nums = DSI_1_LANES;
            LCDKIT_INFO("lane_nums: DSI_1_LANES\n");
        }
        else if (('2' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_2_LANES_SUPPORT))
        {
            pinfo->mipi.lane_nums = DSI_2_LANES;
            LCDKIT_INFO("lane_nums: DSI_2_LANES\n");
        }
        else if (('3' == command[strlen("lane_nums:")]) && (pinfo->mipi.lane_nums_select_support & DSI_3_LANES_SUPPORT))
        {
            pinfo->mipi.lane_nums = DSI_3_LANES;
            LCDKIT_INFO("lane_nums: DSI_3_LANES\n");
        }
        else
        {
            pinfo->mipi.lane_nums = DSI_4_LANES;
            LCDKIT_INFO("lane_nums: DSI_4_LANES\n");
        }
    }

    if (!strncmp("panel_effect_support:", command, strlen("panel_effect_support:")))
    {
        if ('0' == command[strlen("panel_effect_support:")])
        {
            pinfo->panel_effect_support = 0;
            LCDKIT_INFO("panel_effect_support disable\n");
        }
        else
        {
            pinfo->panel_effect_support = 1;
            LCDKIT_INFO("panel_effect_support enable\n");
        }
    }

    if (!strncmp("color_temp_rectify_support:", command, strlen("color_temp_rectify_support:")))
    {
        if ('0' == command[strlen("color_temp_rectify_support:")])
        {
            pinfo->color_temp_rectify_support = 0;
            LCDKIT_INFO("color_temp_rect disable\n");
        }
        else
        {
            pinfo->color_temp_rectify_support = 1;
            LCDKIT_INFO("color_temp_rect enable\n");
        }
    }

    if (!strncmp("ddic_rgbw_support:", command, strlen("ddic_rgbw_support:")))
    {
        if ('0' == command[strlen("ddic_rgbw_support:")])
        {
            pinfo->rgbw_support = 0;
            LCDKIT_INFO("ddic_rgbw_support disable\n");
        }
        else
        {
            pinfo->rgbw_support = 1;
            LCDKIT_INFO("ddic_rgbw_support enable\n");
        }
    }

    hisifb_display_effect_func_switch(hisifd, command);

out:
    hisifb_deactivate_vsync(hisifd);

    up(&hisifd->blank_sem);

    return count;
}
ssize_t lcd_dynamic_porch_store(struct device* dev,
                              struct device_attribute* attr, const char* buf, size_t count)
{
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;
    struct hisi_panel_info* pinfo = NULL;
    unsigned int h_back_porch = 0;
    unsigned int h_front_porch = 0;
    unsigned int h_pulse_width = 0;
    unsigned int v_back_porch = 0;
    unsigned int v_front_porch = 0;
    unsigned int v_pulse_width = 0;
    unsigned int pixel_clk = 0;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -ENXIO;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

       if (strlen(buf) >= MAX_BUF) {
            HISI_FB_ERR("buf overflow!\n");
            return -EINVAL;
        }

    pinfo = &(hisifd->panel_info);

    if (!sscanf(buf, "%d,%d,%d,%d,%d,%d,%d",&h_back_porch,&h_front_porch,&h_pulse_width,
                              &v_back_porch,&v_front_porch,&v_pulse_width,&pixel_clk)) {
    LCDKIT_INFO("bad command\n");
    return count;
    }

    down(&hisifd->blank_sem);

    hisifb_activate_vsync(hisifd);

    pinfo->ldi.h_back_porch = h_back_porch;
    pinfo->ldi.h_front_porch = h_front_porch;
    pinfo->ldi.h_pulse_width = h_pulse_width;
    pinfo->ldi.v_back_porch = v_back_porch;
    pinfo->ldi.v_front_porch = v_front_porch;
    pinfo->ldi.v_pulse_width = v_pulse_width;
    pinfo->pxl_clk_rate = pixel_clk * 1000000UL;

out:
    hisifb_deactivate_vsync(hisifd);

    up(&hisifd->blank_sem);

    return count;
}

ssize_t lcd_dynamic_porch_show(struct device* dev,
                             struct device_attribute* attr, char* buf)
{
    ssize_t ret = 0;
    struct fb_info* fbi = NULL;
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_fb_panel_data* pdata = NULL;
    struct hisi_panel_info* pinfo = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -ENXIO;
    }

    fbi = dev_get_drvdata(dev);

    if (NULL == fbi)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    hisifd = (struct hisi_fb_data_type*)fbi->par;

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pdata = dev_get_platdata(&hisifd->pdev->dev);

    if (NULL == pdata)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    pinfo = &(hisifd->panel_info);

    ret = snprintf(buf, PAGE_SIZE,
          "h_back_porch=%d\n"
          "h_front_porch=%d\n"
          "h_pulse_width=%d\n"
          "v_back_porch=%d\n"
          "v_front_porch=%d\n"
          "v_pulse_width=%d\n"
          "pixel clk = %d\n",
          pinfo->ldi.h_back_porch,
          pinfo->ldi.h_front_porch,
          pinfo->ldi.h_pulse_width,
          pinfo->ldi.v_back_porch,
          pinfo->ldi.v_front_porch,
          pinfo->ldi.v_pulse_width,
          pinfo->pxl_clk_rate);

    return ret;
}
ssize_t lcd_test_config_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_test_config_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_test_config_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_test_config_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (lcdkit_info->lcdkit_test_config_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_test_config_store(buf);
        hisifb_deactivate_vsync(hisifd);
    }

    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lv_detect_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_lv_detect)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_lv_detect(hisifd);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t current_detect_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_current_detect)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_current_detect(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_reg_read_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }
    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }
    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }
	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel power off!\n", hisifd->index);
		goto err_out;
	}
    if (lcdkit_info->lcdkit_reg_read_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_reg_read_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }
err_out:
	up(&hisifd->blank_sem);
	return ret;
}
ssize_t lcd_reg_read_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }
    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }
    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }
	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel power off!\n", hisifd->index);
		goto err_out;
	}
    if (lcdkit_info->lcdkit_reg_read_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_reg_read_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }
err_out:
	up(&hisifd->blank_sem);
	return ret;
}

ssize_t lcd_ddic_oem_info_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_inf)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_inf->lcdkit_oem_info_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_inf->lcdkit_oem_info_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_ddic_oem_info_store(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, const char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);

    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_inf)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);
    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_inf->lcdkit_oem_info_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_inf->lcdkit_oem_info_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }
err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_bl_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = -1;
    struct hisi_fb_data_type* hisifd;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_inf)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_inf->lcdkit_bl_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_inf->lcdkit_bl_mode_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

ssize_t lcd_bl_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, const char* buf)
{
    ssize_t ret = -1;
    struct hisi_fb_data_type* hisifd;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_inf->lcdkit_bl_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_inf->lcdkit_bl_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_se_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_info, char* buf)
{
    ssize_t ret = -1;
    struct hisi_fb_data_type* hisifd;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_info)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_info->lcdkit_se_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_se_mode_show(buf);
        hisifb_deactivate_vsync(hisifd);
    }

    err_out:
        up(&hisifd->blank_sem);

    return ret;

}

ssize_t lcd_se_mode_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
    ssize_t ret = -1;
    struct hisi_fb_data_type* hisifd;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == buf)
    {
        LCDKIT_ERR("buf is NULL Pointer!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        ret = -EINVAL;
        goto err_out;
    }

    if (lcdkit_info->lcdkit_se_mode_store)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_info->lcdkit_se_mode_store(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

    err_out:
        up(&hisifd->blank_sem);
    return ret;
}
ssize_t lcd_support_bl_mode_show(struct device* dev, struct lcdkit_panel_data* lcdkit_inf, char* buf)
{
    ssize_t ret = -1;
    struct hisi_fb_data_type* hisifd;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("NULL Pointer!\n");
        return -EINVAL;
    }

    if (NULL == lcdkit_inf)
    {
        LCDKIT_ERR("lcdkit_info is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    if (lcdkit_inf->lcdkit_support_bl_mode_show)
    {
        hisifb_activate_vsync(hisifd);
        ret = lcdkit_inf->lcdkit_support_bl_mode_show(hisifd, buf);
        hisifb_deactivate_vsync(hisifd);
    }

err_out:
    up(&hisifd->blank_sem);

    return ret;
}

extern ssize_t lcdkit_ldo_check_hs_show(char* buf);
ssize_t lcd_ldo_check_show(struct device* dev, char* buf)
{
    ssize_t ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == dev)
    {
        LCDKIT_ERR("dev is NULL Point!\n");
        return -ENXIO;
    }

    hisifd = get_fb_data(dev);
    if (NULL == hisifd)
    {
        LCDKIT_ERR("hisifd is NULL Point!\n");
        return -EINVAL;
    }

    down(&hisifd->blank_sem);

    if (!hisifd->panel_power_on)
    {
        LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
        goto err_out;
    }

    ret = lcdkit_ldo_check_hs_show(buf);

err_out:
    up(&hisifd->blank_sem);
    return ret;
}

ssize_t lcd_mipi_config_store(struct device* dev, struct lcdkit_panel_data* lcdkit_info, const char* buf)
{
	ssize_t ret = -EINVAL;

	if (NULL == dev || NULL == lcdkit_info || NULL == buf)
	{
		LCDKIT_ERR("%s: NULL Pointer!\n",__func__);
		return ret;
	}

	struct hisi_fb_data_type* hisifd = get_fb_data(dev);
	if (NULL == hisifd) {
		LCDKIT_ERR("hisifd is NULL Pointer!\n");
		return ret;
	}

	down(&hisifd->blank_sem);

	do{
		if (!hisifd->panel_power_on) {
			LCDKIT_ERR("fb%d, panel power off!\n", hisifd->index);
			break;
		}

		if (lcdkit_info->lcdkit_mipi_config_store)
		{
			hisifb_activate_vsync(hisifd);
			ret = lcdkit_info->lcdkit_mipi_config_store(hisifd, buf);
			hisifb_deactivate_vsync(hisifd);
		}
	} while (0);

	up(&hisifd->blank_sem);
	return ret;
}