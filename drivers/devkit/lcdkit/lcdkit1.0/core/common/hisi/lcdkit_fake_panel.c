#include "hisi_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_disp.h"
#include "lcdkit_dbg.h"
#include "lcdkit_backlight_ic_common.h"
#define LCDKIT_DTS_COMP_MIPI_FAKE_PANEL "auo_otm1901a_5p2_1080p_video_default"

static int lcdkit_fake_set_fastboot(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;

    if (NULL == pdev)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }


    return 0;
}

static int lcdkit_fake_on(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
    struct hisi_panel_info* pinfo = NULL;

    LCDKIT_INFO("enter!\n");
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
    if (pinfo->lcd_init_step == LCD_INIT_POWER_ON)
    {
        if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
        {
            struct lcdkit_bl_ic_info *tmp = NULL;

            tmp =  lcdkit_get_lcd_backlight_ic_info();
            if(tmp != NULL)
            {
                if(lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode)
                {
                    gpio_cmds_tx(lcdkit_bl_power_request_cmds,ARRAY_SIZE(lcdkit_bl_power_request_cmds));
                    gpio_cmds_tx(lcdkit_bl_power_enable_cmds,ARRAY_SIZE(lcdkit_bl_power_enable_cmds));
                }
                if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
                {
                    gpio_cmds_tx(lcdkit_bl_request_cmds,ARRAY_SIZE(lcdkit_bl_request_cmds));
                    gpio_cmds_tx(lcdkit_bl_enable_cmds,ARRAY_SIZE(lcdkit_bl_enable_cmds));
                }
                LCDKIT_INFO("fake panel backlight_bias_ic power ctrl mode is %d  bl gpio ctrl mode is %d!\n",
                lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode,lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode);
                lcdkit_backlight_ic_inital();
                if(tmp->ic_type == BACKLIGHT_BIAS_IC)
                {
                    lcdkit_backlight_ic_bias(false);
                }
            }
        }
        pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE)
    {
        pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
    }
    else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE)
    {
        ;
    }
    else
    {
        LCDKIT_ERR("failed to init lcd!\n");
    }


    LCDKIT_INFO("fb%d, -!\n", hisifd->index);

    return 0;
}

static int lcdkit_fake_off(struct platform_device* pdev)
{
    struct hisi_fb_data_type* hisifd = NULL;
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
    pinfo = &(hisifd->panel_info);
    if (NULL == pinfo)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }
    if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
    {
        struct lcdkit_bl_ic_info *tmp = NULL;
        tmp =  lcdkit_get_lcd_backlight_ic_info();

	    if(tmp != NULL)
	    {
            LCDKIT_INFO("fake panle backlight ic power off \n");
            lcdkit_backlight_ic_disable_device();
            if(lcdkit_info.panel_infos.lcd_bl_gpio_ctrl_mode)
            {
                gpio_cmds_tx(lcdkit_bl_disable_cmds, ARRAY_SIZE(lcdkit_bl_disable_cmds));
                gpio_cmds_tx(lcdkit_bl_free_cmds, ARRAY_SIZE(lcdkit_bl_free_cmds));
            }
            if(lcdkit_info.panel_infos.lcd_backlight_power_ctrl_mode)
            {
                gpio_cmds_tx(lcdkit_bl_power_disable_cmds, ARRAY_SIZE(lcdkit_bl_power_disable_cmds));
                gpio_cmds_tx(lcdkit_bl_power_free_cmds, ARRAY_SIZE(lcdkit_bl_power_free_cmds));
            }
        }
    }
    LCDKIT_INFO("fb%d, -!\n", hisifd->index);
    return 0;
}

/*
*name:lcdkit_remove
*function:panel remove
*@pdev:platform device
*/
static int lcdkit_fake_remove(struct platform_device* pdev)
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

    LCDKIT_DEBUG("fb%d, -.\n", hisifd->index);

    return 0;
}

static int lcdkit_fake_set_backlight(struct platform_device* pdev, uint32_t bl_level)
{
    int ret = 0;
    struct hisi_fb_data_type* hisifd = NULL;
    static char last_bl_level = 255;
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
    pinfo = &(hisifd->panel_info);
    if (NULL == pinfo)
    {
        LCDKIT_ERR("NULL Pointer\n");
        return -EINVAL;
    }

    if ( bl_level > 0 )
    {
        /*enable bl gpio*/
        if(pinfo->bl_ic_ctrl_mode != COMMON_IC_MODE)
        {
            if (lcdkit_info.panel_infos.bl_enable_flag) {
                gpio_direction_output( lcdkit_info.panel_infos.gpio_lcd_bl, 1);
            }
        }
        mdelay(2);
        /* backlight on */
        hisi_lcd_backlight_on(pdev);
        if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
        {
            struct lcdkit_bl_ic_info *tmp = NULL;

            tmp =  lcdkit_get_lcd_backlight_ic_info();
            if(tmp != NULL)
            {
                lcdkit_backlight_ic_enable_brightness();
            }
        }
        LCDKIT_INFO("set backlight to %d\n", bl_level);
        ret = hisi_blpwm_set_backlight(hisifd, bl_level);
    }
    else {
        ret = hisi_blpwm_set_backlight(hisifd, 0);
        if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
        {
            struct lcdkit_bl_ic_info *tmp = NULL;

            tmp =  lcdkit_get_lcd_backlight_ic_info();
            if(tmp != NULL)
            {
                lcdkit_backlight_ic_disable_brightness();
            }
        }
        /* backlight off */
        hisi_lcd_backlight_off(pdev);
        /*disable bl gpio*/
        if(pinfo->bl_ic_ctrl_mode != COMMON_IC_MODE)
        {
            if (lcdkit_info.panel_infos.bl_enable_flag) {
                gpio_direction_output(lcdkit_info.panel_infos.gpio_lcd_bl, 0);
            }
        }
    }
    return 0;
}

static struct hisi_panel_info lcdkit_fake_panel_info = {0};
static struct hisi_fb_panel_data lcdkit_fake_data =
{
    .panel_info = &lcdkit_fake_panel_info,
    .set_fastboot = lcdkit_fake_set_fastboot,
    .on = lcdkit_fake_on,
    .off = lcdkit_fake_off,
    .remove = lcdkit_fake_remove,
    .set_backlight = lcdkit_fake_set_backlight,
};

/*
*name:lcdkit_probe
*function:panel driver probe
*@pdev:platform device
*/
static int lcdkit_fake_probe(struct platform_device* pdev)
{
    int ret = 0;
    struct hisi_panel_info* pinfo = NULL;
    struct device_node* np = NULL;
    const char *lcd_bl_ic_name;

    if (hisi_fb_device_probe_defer(PANEL_MIPI_VIDEO, BL_SET_BY_NONE)) {
        goto err_probe_defer;
    }

    np = pdev->dev.of_node;
    if (!np)
    {
        LCDKIT_ERR("NOT FOUND device node %s!\n", LCDKIT_DTS_COMP_MIPI_FAKE_PANEL);
        return -EINVAL;
    }

    pinfo = lcdkit_fake_data.panel_info;
    memset(pinfo, 0, sizeof(struct hisi_panel_info));

    pdev->id = 1;
    lcdkit_init(np, pinfo);
    pinfo->type = 8;

    if (lcdkit_info.panel_infos.bl_enable_flag) {
        gpio_cmds_tx(lcdkit_bl_request_cmds, \
            ARRAY_SIZE(lcdkit_bl_request_cmds));
    }
    gpio_cmds_tx(lcdkit_bias_request_cmds, \
        ARRAY_SIZE(lcdkit_bias_request_cmds));

    // alloc panel device data
    ret = platform_device_add_data(pdev, &lcdkit_fake_data, sizeof(struct hisi_fb_panel_data));
    if (ret)
    {
        LCDKIT_ERR("platform_device_add_data failed!\n");
        goto err_device_put;
    }

    hisi_fb_add_device(pdev);
    LCDKIT_INFO("exit succ!!!!\n");
    return 0;

err_device_put:
    platform_device_put(pdev);
err_return:
    return ret;
err_probe_defer:
    return -EPROBE_DEFER;
}

static struct of_device_id lcdkit_fake_match_table[] =
{
    {
        .compatible = LCDKIT_DTS_COMP_MIPI_FAKE_PANEL,
        .data = NULL,
    },
    {},
};

static struct platform_driver lcdkit_fake_driver =
{
    .probe = lcdkit_fake_probe,
    .remove = NULL,
    .suspend = NULL,
    .resume = NULL,
    .shutdown = NULL,
    .driver = {
        .name = "lcdkit__fake_mipi_panel",
        .owner = THIS_MODULE,
        .of_match_table = lcdkit_fake_match_table,
    },
};

static int lcdkit_fake_disp_init(void)
{
    int ret = 0;
	int len = 0;

    struct device_node* np = NULL;

    np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCDKIT_PANEL_TYPE);

    if (!np)
    {
        LCDKIT_ERR("NOT FOUND device node %s!\n", DTS_COMP_LCDKIT_PANEL_TYPE);
        ret = -1;
        return ret;
    }

    lcdkit_info.panel_infos.lcd_compatible = (char*)of_get_property(np, "lcd_panel_type", NULL);

    if (!lcdkit_info.panel_infos.lcd_compatible)
    {
        LCDKIT_ERR("Is not normal lcd and return\n");
        return ret;
    }
    else
    {
        if( strncmp(lcdkit_info.panel_infos.lcd_compatible, LCDKIT_DTS_COMP_MIPI_FAKE_PANEL ,strlen(LCDKIT_DTS_COMP_MIPI_FAKE_PANEL)))
        {
           LCDKIT_INFO("the panel is buckled! \n");
           return ret;
        }
    }

    len = strlen(lcdkit_info.panel_infos.lcd_compatible);
    memset( (char *)lcdkit_fake_driver.driver.of_match_table->compatible, 0, LCDKIT_PANEL_COMP_LENGTH);
    strncpy( (char *)lcdkit_fake_driver.driver.of_match_table->compatible, lcdkit_info.panel_infos.lcd_compatible,
            len > (LCDKIT_PANEL_COMP_LENGTH - 1) ? (LCDKIT_PANEL_COMP_LENGTH - 1) : len);

    ret = platform_driver_register(&lcdkit_fake_driver);
    if (ret) {
        LCDKIT_ERR("platform_driver_register failed, error=%d!\n", ret);
    }
    return ret;
}

module_init(lcdkit_fake_disp_init);
