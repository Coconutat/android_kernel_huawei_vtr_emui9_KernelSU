
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
#include <linux/pinctrl/consumer.h>
/*lint +e322*//*lint +e7*/

#include "board.h"
#include "plat_debug.h"

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
int32 hi1102_get_board_gpio(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;
    ret = get_board_gpio(DTS_NODE_HI110X, DTS_PROP_GPIO_POWEN_ON, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }

    g_board_info.power_on_enable = physical_gpio;

    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }
    return BOARD_SUCC;
}

void hi1102_free_board_power_gpio(void)
{
    gpio_free(g_board_info.power_on_enable);
}

int32 hi1102_board_wakeup_gpio_init(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    /*wifi wake host gpio request*/
    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WLAN_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }

    g_board_info.wlan_wakeup_host = physical_gpio;

    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }

    /*bfgx wake host gpio request*/
    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGN_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_BFGN_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }

    g_board_info.bfgn_wakeup_host = physical_gpio;

    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGN_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }
    return BOARD_SUCC;

err_get_bfgx_wkup_host_gpio:
    gpio_free(g_board_info.wlan_wakeup_host);
err_get_wlan_wkup_host_gpio:

    return BOARD_FAIL;
}

void hi1102_free_board_wakeup_gpio(void)
{
    gpio_free(g_board_info.wlan_wakeup_host);
    gpio_free(g_board_info.bfgn_wakeup_host);
}

#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif


