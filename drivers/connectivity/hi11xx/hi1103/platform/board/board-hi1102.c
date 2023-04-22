
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#ifdef _PRE_CONFIG_USE_DTS
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
/*lint -e322*//*lint -e7*/
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#ifdef CONFIG_PINCTRL
#include <linux/pinctrl/consumer.h>
#endif
/*lint +e322*//*lint +e7*/

#include "board.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "oal_hcc_host_if.h"
#include "plat_uart.h"
#include "plat_firmware.h"

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/


/*
*
***************************************************************************
  3
 Function Definition
***
**************************************************************************/
int32 hi1102_get_board_power_gpio(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;
    ret = get_board_gpio_etc(DTS_NODE_HI110X, DTS_PROP_GPIO_POWEN_ON, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }

    g_board_info_etc.power_on_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }

    gpio_direction_output(physical_gpio, 0);
#endif
    return BOARD_SUCC;
}

void hi1102_free_board_power_gpio_etc(void)
{
    gpio_free(g_board_info_etc.power_on_enable);
}

int32 hi1102_board_wakeup_gpio_init_etc(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    /*wifi wake host gpio request*/
    ret = get_board_gpio_etc(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WLAN_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }

    g_board_info_etc.wlan_wakeup_host = physical_gpio;
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }
    gpio_direction_input(physical_gpio);
#endif

    /*bfgx wake host gpio request*/
    ret = get_board_gpio_etc(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGN_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_BFGN_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }

    g_board_info_etc.bfgn_wakeup_host = physical_gpio;
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }
    gpio_direction_input(physical_gpio);
#endif
    return BOARD_SUCC;

err_get_bfgx_wkup_host_gpio:
    gpio_free(g_board_info_etc.wlan_wakeup_host);
err_get_wlan_wkup_host_gpio:

    return BOARD_FAIL;
}

void hi1102_free_board_wakeup_gpio_etc(void)
{
    gpio_free(g_board_info_etc.wlan_wakeup_host);
    gpio_free(g_board_info_etc.bfgn_wakeup_host);
}
static void hi1102_prepare_to_power_on(void)
{
    int32 ret = BOARD_FAIL;

    if (NO_NEED_POWER_PREPARE == g_board_info_etc.need_power_prepare)
    {
        return;
    }

    if (OAL_IS_ERR_OR_NULL(g_board_info_etc.pctrl) || OAL_IS_ERR_OR_NULL(g_board_info_etc.pins_idle))
    {
        PS_PRINT_ERR("power pinctrl is err, pctrl is %p, pins_idle is %p\n", g_board_info_etc.pctrl, g_board_info_etc.pins_idle);
        return;
    }
#ifdef _PRE_CONFIG_USE_DTS
    /* set LowerPower mode */
    ret = pinctrl_select_state(g_board_info_etc.pctrl, g_board_info_etc.pins_idle);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("power prepare:set LOWPOWER mode failed, ret:%d\n", ret);
        return;
    }
#endif
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(g_board_info_etc.xldo_pinmux, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_XLDO_PINMUX);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_XLDO_PINMUX);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        return;
    }
#else
    ret = gpio_request(g_board_info_etc.xldo_pinmux, PROC_NAME_GPIO_XLDO_PINMUX);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_XLDO_PINMUX);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        return;
    }
    gpio_direction_output(g_board_info_etc.xldo_pinmux, 0);
#endif

    gpio_direction_output(g_board_info_etc.xldo_pinmux, g_board_info_etc.gpio_xldo_level);
    g_board_info_etc.pinmux_set_result = PINMUX_SET_SUCC;

    return;
}

static void hi1102_post_to_power_on(void)
{
    int32 ret = BOARD_FAIL;

    if (NO_NEED_POWER_PREPARE == g_board_info_etc.need_power_prepare)
    {
        return;
    }

    if (PINMUX_SET_SUCC == g_board_info_etc.pinmux_set_result)
    {
        g_board_info_etc.pinmux_set_result = PINMUX_SET_INIT;
        gpio_free(g_board_info_etc.xldo_pinmux);
    }

    if (OAL_IS_ERR_OR_NULL(g_board_info_etc.pctrl) || OAL_IS_ERR_OR_NULL(g_board_info_etc.pins_normal))
    {
        PS_PRINT_ERR("power pinctrl is err, pctrl is %p, pins_idle is %p\n", g_board_info_etc.pctrl, g_board_info_etc.pins_normal);
        return;
    }
#ifdef _PRE_CONFIG_USE_DTS
    /* set NORMAL mode */
    ret = pinctrl_select_state(g_board_info_etc.pctrl, g_board_info_etc.pins_normal);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("power prepare:set NORMAL mode failed, ret:%d\n", ret);
        return;
    }
#else
#warning DTS invalid
#endif

    return;
}

int32 hi1102_bfgx_dev_power_on(void)
{
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (wlan_is_shutdown_etc())
    {
        PS_PRINT_INFO("bfgx pull up power_on_enable gpio!\n");

        hi1102_board_power_on(BFGX_POWER);
        if (BFGX_POWER_SUCCESS != open_tty_drv_etc(ps_core_d->pm_data))
        {
            PS_PRINT_ERR("open tty fail!\n");
            error = BFGX_POWER_TTY_OPEN_FAIL;
            goto bfgx_power_on_fail;
        }

        if (BFGX_POWER_SUCCESS != firmware_download_function_etc(BFGX_CFG))
        {
            hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
            PS_PRINT_ERR("bfgx download firmware fail!\n");
            error = BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
            goto bfgx_power_on_fail;
        }
        hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
    }
    else
    {
        if (BFGX_POWER_SUCCESS != open_tty_drv_etc(ps_core_d->pm_data))
        {
            PS_PRINT_ERR("open tty fail!\n");
            error = BFGX_POWER_TTY_OPEN_FAIL;
            goto bfgx_power_on_fail;
        }

        if(BFGX_POWER_SUCCESS != wlan_pm_open_bcpu_etc())
        {
            PS_PRINT_ERR("wifi dereset bcpu fail!\n");
            error = BFGX_POWER_WIFI_DERESET_BCPU_FAIL;
            CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_BCPU));
            goto bfgx_power_on_fail;
        }
    }

bfgx_power_on_fail:

    return error;
}

int32 hi1102_bfgx_dev_power_off(void)
{
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (wlan_is_shutdown_etc())
    {
        if (SUCCESS != release_tty_drv_etc(ps_core_d->pm_data))
        {
           /*代码执行到此处，说明六合一所有业务都已经关闭，无论tty是否关闭成功，device都要下电*/
           PS_PRINT_ERR("wifi off, close tty is err!");
        }

        pm_data->bfgx_dev_state = BFGX_SLEEP;

        PS_PRINT_INFO("bfgx pull down power_on_enable gpio\n");

        hi1102_board_power_off(BFGX_POWER);

    }
    else
    {
        if(SUCCESS != uart_bfgx_close_cmd_etc())
        {
           /*bfgx self close fail 了，后面也要通过wifi shutdown bcpu*/
           PS_PRINT_ERR("bfgx self close fail\n");
           CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_BCPU));
        }

        if (SUCCESS != release_tty_drv_etc(ps_core_d->pm_data))
        {
           /*代码执行到此处，说明bfgx所有业务都已经关闭，无论tty是否关闭成功，都要关闭bcpu*/
           PS_PRINT_ERR("wifi on, close tty is err!");
        }

        pm_data->bfgx_dev_state = BFGX_SLEEP;

        if(SUCCESS != wlan_pm_shutdown_bcpu_cmd_etc())
        {
           PS_PRINT_ERR("wifi shutdown bcpu fail\n");
           CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_BCPU));
           error = -FAILURE;
        }
    }

    return error;
}

int32 hi1102_wlan_power_off(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (bfgx_is_shutdown_etc())
    {
        PS_PRINT_INFO("wifi pull down power_on_enable!\n");
        hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_ALL);
        hi1102_board_power_off(WLAN_POWER);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_by_gpio",OAL_DFT_TRACE_SUCC);
    }
    else
    {
        /*先关闭SDIO TX通道*/
        hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_TX);

        /*wakeup dev,send poweroff cmd to wifi*/
        if(OAL_SUCC != wlan_pm_poweroff_cmd_etc())
        {
            /*wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行*/
            DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_by_sdio_fail",OAL_DFT_TRACE_FAIL);
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_WCPU));
#ifdef PLATFORM_DEBUG_ENABLE
            return -FAILURE;
#endif
        }

        hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_ALL);

        /*power off cmd execute succ,send shutdown wifi cmd to BFGN */
        if(OAL_SUCC != uart_wifi_close_etc())
        {
            /*uart关闭WCPU失败也继续执行，DFR推迟到wifi下次open的时候执行*/
            DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_uart_cmd_fail",OAL_DFT_TRACE_FAIL);
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_WCPU));
        }
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_by_bcpu_ok",OAL_DFT_TRACE_SUCC);
    }

    pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    return SUCCESS;
}
int32 hi1102_wlan_power_on(void)
{
    int32 ret;
    int32  error = WIFI_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (bfgx_is_shutdown_etc())
    {
        PS_PRINT_SUC("wifi pull up power_on_enable gpio!\n");
        hi1102_board_power_on(WLAN_POWER);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron_by_gpio_ok",OAL_DFT_TRACE_SUCC);

        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

        if(WIFI_POWER_SUCCESS == firmware_download_function_etc(BFGX_AND_WIFI_CFG))
        {
            pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
        }
        else
        {
            PS_PRINT_ERR("firmware download fail\n");
            error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
            goto wifi_power_fail;
        }

        ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
        if (0 != ret)
        {
            DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio fail", OAL_DFT_TRACE_FAIL);
            PS_PRINT_ERR("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH failed=%d\n", ret);
            error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP));
            goto wifi_power_fail;
        }
    }
    else
    {
        if(WIFI_POWER_SUCCESS != uart_wifi_open_etc())
        {
            DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron_by_uart_fail", OAL_DFT_TRACE_FAIL);
            error = WIFI_POWER_BFGX_DERESET_WCPU_FAIL;
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_WCPU));
            goto wifi_power_fail;
        }
        else
        {
            hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
            if(WIFI_POWER_SUCCESS == firmware_download_function_etc(WIFI_CFG))
            {
                pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
            }
            else
            {
                PS_PRINT_ERR("firmware download fail\n");
                error = WIFI_POWER_BFGX_ON_FIRMWARE_DOWNLOAD_FAIL;
                goto wifi_power_fail;
            }

            ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
            if(0 != ret)
            {
                DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH byuart_fail",OAL_DFT_TRACE_FAIL);
                PS_PRINT_ERR("wlan_poweron by uart HCC_BUS_POWER_PATCH_LAUCH failed =%d", ret);
                error = WIFI_POWER_BFGX_ON_BOOT_UP_FAIL;
                CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP));
                goto wifi_power_fail;
            }
        }
    }

    return WIFI_POWER_SUCCESS;

wifi_power_fail:
    return error;

}

int32 hi1102_board_power_on(uint32 ul_subsystem)
{
    int32 ret = -EFAIL;
    int32 gpio = g_board_info_etc.power_on_enable;

    if (ul_subsystem >= POWER_BUTT)
    {
        PS_PRINT_ERR("power input system:%d error\n", ul_subsystem);
        return ret;
    }

    hi1102_prepare_to_power_on();

    //if(hcc_get_current_110x_bus())
    {
        /*第一次枚举时BUS 还未初始化*/
        ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_UP, board_wlan_gpio_power_on, (void*)(long)gpio);
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_UP);
    }

    hi1102_post_to_power_on();

    return SUCC;
}

int32 hi1102_board_power_off(uint32 ul_subsystem)
{
    int32 ret = -EFAIL;
    int32 gpio = g_board_info_etc.power_on_enable;

    if (ul_subsystem >= POWER_BUTT)
    {
        PS_PRINT_ERR("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }

    //if(hcc_get_current_110x_bus())
    {
        ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_DOWN, board_wlan_gpio_power_off, (void*)(long)gpio);
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_DOWN);
    }

    return SUCC;
}

int32 hi1102_board_power_reset(uint32 ul_subsystem)
{
    hi1102_board_power_on(ul_subsystem);
    return SUCC;
}
int32 hi1102_get_board_pmu_clk32k(void)
{
    int32 ret= BOARD_FAIL;
    const char * clk_name = NULL;

    ret = get_board_custmize_etc(DTS_NODE_HI110X, DTS_PROP_CLK_32K, &clk_name);
    if (BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }
    g_board_info_etc.clk_32k_name = clk_name;

return BOARD_SUCC;
}
int32 hi1102_get_board_uart_port(void)
{
#ifdef _PRE_CONFIG_USE_DTS
        int32 ret= BOARD_FAIL;
        struct device_node * np = NULL;
        const char *uart_port = NULL;

        ret = get_board_dts_node_etc(&np, DTS_NODE_HI110X_BFGX);
        if(BOARD_SUCC != ret)
        {
            PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X);
            return BOARD_FAIL;
        }

        /*使用uart4，需要在dts里新增DTS_PROP_UART_PCLK项，指明uart4不依赖sensorhub*/
        if (of_property_read_bool(np, DTS_PROP_UART_PCLK))
        {
            PS_PRINT_INFO("uart pclk normal\n");
            g_board_info_etc.uart_pclk = UART_PCLK_NORMAL;
        }
        else
        {
            PS_PRINT_INFO("uart pclk from sensorhub\n");
            g_board_info_etc.uart_pclk = UART_PCLK_FROM_SENSORHUB;
        }

        ret = get_board_custmize_etc(DTS_NODE_HI110X_BFGX, DTS_PROP_UART_POART, &uart_port);
        if(BOARD_SUCC != ret)
        {
            return BOARD_FAIL;
        }

        g_board_info_etc.uart_port = uart_port;
        PS_PRINT_INFO(" g_board_info_etc.uart_port=%s\n", g_board_info_etc.uart_port);
        return BOARD_SUCC;
#else
        return BOARD_SUCC;
#endif

}

#ifdef HAVE_HISI_IR
int32 hi1102_board_ir_ctrl_gpio_init(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    /* ir ctrl gpio request */
    ret = get_board_gpio_etc(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGN_IR_CTRL, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_INFO("dts prop %s not exist\n", DTS_PROP_GPIO_BFGN_IR_CTRL);
        g_board_info_etc.bfgx_ir_ctrl_gpio= -1;
    }
    else
    {
        g_board_info_etc.bfgx_ir_ctrl_gpio= physical_gpio;

#ifdef GPIOF_OUT_INIT_LOW
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_BFGN_IR_CTRL);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGN_IR_CTRL);
        }
#else
        ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGN_IR_CTRL);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGN_IR_CTRL);
        }
        else
        {
            gpio_direction_output(physical_gpio, 0);
        }
#endif
    }

    return BOARD_SUCC;
}

int32 hi1102_board_ir_ctrl_pmic_init(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret = BOARD_FAIL;
    struct device_node * np = NULL;
    int32 irled_voltage = 0;
    if (NULL == pdev){
        PS_PRINT_ERR("board pmu pdev is NULL!\n");
        return ret;
    }

    ret = get_board_dts_node_etc(&np, DTS_NODE_HI110X_BFGX);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return ret;
    }

    g_board_info_etc.bfgn_ir_ctrl_ldo = regulator_get(&pdev->dev, DTS_IRLED_LDO_POWER);

    if (IS_ERR(g_board_info_etc.bfgn_ir_ctrl_ldo))
    {
        PS_PRINT_ERR("board_ir_ctrl_pmic_init get ird ldo failed\n");
        return ret;
    }

    ret = of_property_read_u32(np, DTS_IRLED_VOLTAGE, &irled_voltage);
    if (BOARD_SUCC == ret)
    {
        PS_PRINT_INFO("set irled voltage %d mv\n", irled_voltage/1000);
        ret = regulator_set_voltage(g_board_info_etc.bfgn_ir_ctrl_ldo,(int)irled_voltage,(int)irled_voltage);
        if (ret)
        {
            PS_PRINT_ERR("board_ir_ctrl_pmic_init set voltage ldo failed\n");
            return ret;
        }
    }
    else
    {
        PS_PRINT_ERR("get irled voltage failed ,use default\n");
    }

    ret = regulator_set_mode(g_board_info_etc.bfgn_ir_ctrl_ldo,REGULATOR_MODE_NORMAL);
    if (ret)
    {
        PS_PRINT_ERR("board_ir_ctrl_pmic_init set ldo mode failed\n");
        return ret;
    }
    PS_PRINT_INFO("board_ir_ctrl_pmic_init success\n");
    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
#endif

int32 hi1102_board_ir_ctrl_init(struct platform_device *pdev)
{
#if (defined HAVE_HISI_IR) && (defined _PRE_CONFIG_USE_DTS)
    int ret = BOARD_SUCC;
    struct device_node * np = NULL;

    ret = get_board_dts_node_etc(&np, DTS_NODE_HI110X_BFGX);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        goto err_get_ir_ctrl_gpio;
    }
    g_board_info_etc.have_ir = of_property_read_bool(np, "have_ir");
    if (!g_board_info_etc.have_ir)
    {
        PS_PRINT_ERR("board has no Ir");
    }
    else
    {
        ret = of_property_read_u32(np, DTS_PROP_IR_LDO_TYPE, &g_board_info_etc.irled_power_type);
        PS_PRINT_INFO("read property ret is %d, irled_power_type is %d\n", ret, g_board_info_etc.irled_power_type);
        if (BOARD_SUCC != ret)
        {
            PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_IR_LDO_TYPE);
            goto err_get_ir_ctrl_gpio;
        }

        if (IR_GPIO_CTRL == g_board_info_etc.irled_power_type)
        {
            ret = hi1102_board_ir_ctrl_gpio_init();
            if (BOARD_SUCC != ret)
            {
                PS_PRINT_ERR("ir_ctrl_gpio init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        }
        else if (IR_LDO_CTRL == g_board_info_etc.irled_power_type)
        {
            ret = hi1102_board_ir_ctrl_pmic_init(pdev);
            if (BOARD_SUCC != ret)
            {
                PS_PRINT_ERR("ir_ctrl_pmic init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        }
        else
        {
            PS_PRINT_ERR("get ir_ldo_type failed!err num is %d\n", g_board_info_etc.irled_power_type);
            goto err_get_ir_ctrl_gpio;
        }
    }
    return BOARD_SUCC;

err_get_ir_ctrl_gpio:
    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}

int32 hi1102_check_evb_or_fpga(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret= BOARD_FAIL;
    struct device_node * np = NULL;

    ret = get_board_dts_node_etc(&np, DTS_NODE_HI110X);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_VERSION);
    if (ret)
    {
        PS_PRINT_INFO("HI1102 ASIC VERSION\n");
        g_board_info_etc.is_asic = VERSION_ASIC;
    }
    else
    {
        PS_PRINT_INFO("HI1102 FPGA VERSION\n");
        g_board_info_etc.is_asic = VERSION_FPGA;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
int32 hi1102_board_get_power_pinctrl(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32  ret = BOARD_FAIL;
    int32  physical_gpio = 0;
    struct device_node * np = NULL;
    struct pinctrl *pinctrl;
    struct pinctrl_state *pinctrl_def;
    struct pinctrl_state *pinctrl_idle;

    /* 检查是否需要prepare before board power on */
    /* JTAG SELECT 拉低，XLDO MODE选择2.8v */
    ret = get_board_dts_node_etc(&np, DTS_NODE_HI110X);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X);
        goto err_read_dts_node;
    }

    ret = of_property_read_bool(np, DTS_PROP_POWER_PREPARE);

    if (ret)
    {
        PS_PRINT_INFO("need prepare before board power on\n");
        g_board_info_etc.need_power_prepare = NEED_POWER_PREPARE;
    }
    else
    {
        PS_PRINT_INFO("no need prepare before board power on\n");
        g_board_info_etc.need_power_prepare = NO_NEED_POWER_PREPARE;
    }

    if (NO_NEED_POWER_PREPARE == g_board_info_etc.need_power_prepare)
    {
        return BOARD_SUCC;
    }

    pinctrl = devm_pinctrl_get(&pdev->dev);
    if (OAL_IS_ERR_OR_NULL(pinctrl))
    {
        PS_PRINT_ERR("iomux_lookup_block failed, and the value of pinctrl is %p\n", pinctrl);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_IOMUX));
        goto err_pinctrl_get;
    }
    g_board_info_etc.pctrl = pinctrl;

    pinctrl_def = pinctrl_lookup_state(pinctrl, "default");
    if (OAL_IS_ERR_OR_NULL(pinctrl_def))
    {
        PS_PRINT_ERR("pinctrl_lookup_state default failed, and the value of pinctrl_def is %p\n", pinctrl_def);
        goto err_lookup_default;
    }
    g_board_info_etc.pins_normal = pinctrl_def;

    pinctrl_idle = pinctrl_lookup_state(pinctrl, "idle");
    if (OAL_IS_ERR_OR_NULL(pinctrl_idle))
    {
        PS_PRINT_ERR("pinctrl_lookup_state idel failed, and the value of pinctrl_idle is %p\n", pinctrl_idle);
        goto err_lookup_idle;
    }
    g_board_info_etc.pins_idle = pinctrl_idle;

    ret = pinctrl_select_state(g_board_info_etc.pctrl, g_board_info_etc.pins_normal);
    if (ret < 0)
    {
        PS_PRINT_ERR("pinctrl_select_state default failed.\n");
        goto err_select_state;
    }
    ret = get_board_gpio_etc(DTS_NODE_HI110X, DTS_PROP_GPIO_XLDO_PINMUX, &physical_gpio);

    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_POWEN_ON);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        goto err_get_xldo_pinmux;
    }

    g_board_info_etc.xldo_pinmux = physical_gpio;

    ret = of_property_read_u32(np, DTS_PROP_GPIO_XLDO_LEVEL, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_XLDO_LEVEL);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        goto err_read_xldo_level;
    }

    g_board_info_etc.gpio_xldo_level = physical_gpio;

    return BOARD_SUCC;

    err_read_xldo_level:
    err_get_xldo_pinmux:
    err_select_state:
    err_lookup_idle:
    err_lookup_default:
        devm_pinctrl_put(g_board_info_etc.pctrl);
    err_pinctrl_get:
    err_read_dts_node:

    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}
int32 hi1102_get_ini_file_name_from_dts(int8 *dts_prop, int8 *prop_value, uint32 size)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32  ret = 0;
    struct device_node *np = NULL;
    int32  len;
    int8   out_str[HISI_CUST_NVRAM_LEN] = {0};

    np = of_find_compatible_node(NULL, NULL, CUST_COMP_NODE);

    if (NULL == np)
    {
        INI_ERROR("dts node %s not found", CUST_COMP_NODE);
        return INI_FAILED;
    }

    len = of_property_count_u8_elems(np, dts_prop);
    if (len < 0)
    {
        INI_ERROR("can't get len of dts prop(%s)", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, (int32)sizeof(out_str));
    INI_DEBUG("read len of dts prop %s is:%d", dts_prop, len);
    ret = of_property_read_u8_array(np, dts_prop, out_str, len);
    if (0 > ret)
    {
        INI_ERROR("read dts prop (%s) fail", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, (int32)size);
    memcpy(prop_value, out_str, (size_t)len);
    INI_DEBUG("dts prop %s value is:%s", dts_prop, prop_value);
#endif
    return INI_SUCC;
}

#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif


