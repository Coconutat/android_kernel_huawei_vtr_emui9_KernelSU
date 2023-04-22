

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
#include <linux/fs.h>

/*lint +e322*//*lint +e7*/

#include "board.h"
#include "plat_debug.h"
#include "oal_ext_if.h"
#include "board-hi1102.h"
#include "board-hi1103.h"
#include "oal_sdio_host_if.h"
#include "plat_firmware.h"
#include "oal_hcc_bus.h"
#include "plat_pm.h"
#include "oam_ext_if.h"

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
BOARD_INFO g_board_info_etc = {.ssi_gpio_clk = 0, .ssi_gpio_data = 0};
EXPORT_SYMBOL(g_board_info_etc);

OAL_STATIC int32 board_probe_ret = 0;
OAL_STATIC  struct completion  board_driver_complete;

char str_gpio_ssi_dump_path[100] = HISI_TOP_LOG_DIR"/wifi/memdump";
int  ssi_is_logfile = 0;
int  ssi_is_pilot = -1;
int ssi_dfr_bypass = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param_string(gpio_ssi_dump_path, str_gpio_ssi_dump_path,
		sizeof(str_gpio_ssi_dump_path), S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_ssi_dump_path, "gpio_ssi dump path");
module_param(ssi_is_logfile, int, S_IRUGO | S_IWUSR);
module_param(ssi_is_pilot, int, S_IRUGO | S_IWUSR);
module_param(ssi_dfr_bypass, int, S_IRUGO | S_IWUSR);

int hi11xx_android_variant = HI1XX_ANDROID_BUILD_VARIANT_USER; /*default user mode*/
module_param(hi11xx_android_variant, int, S_IRUGO | S_IWUSR);
#endif

OAL_DEFINE_SPINLOCK(g_ssi_lock);
oal_uint32 g_ssi_lock_state = 0x0;

DEVICE_BOARD_VERSION device_board_version_list_etc[BOARD_VERSION_BOTT] = {
    {.index = BOARD_VERSION_HI1102, .name = BOARD_VERSION_NAME_HI1102},
    {.index = BOARD_VERSION_HI1103, .name = BOARD_VERSION_NAME_HI1103},
};

DOWNLOAD_MODE device_download_mode_list_etc[MODE_DOWNLOAD_BUTT] = {
    {.index = MODE_SDIO, .name = DOWNlOAD_MODE_SDIO},
    {.index = MODE_PCIE, .name = DOWNlOAD_MODE_PCIE},
    {.index = MODE_UART, .name = DOWNlOAD_MODE_UART},
};

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
#if 0
/*full version*/
ssi_file_st g_aSsiFile[2] =
{
    {"/system/vendor/firmware/CPU_RAM_SCHED.bin", 0x00010000},
    {"/system/vendor/firmware/BOOT_CALLBACK.bin", 0x0001d200},
};
#else
/*gnss_only uart_cfg*/
ssi_file_st g_aSsiFile[] =
{
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
    /*gnss only*/
    {"/system/vendor/firmware/RAM_VECTOR.bin",    0x80100800},
    {"/system/vendor/firmware/CPU_RAM_SCHED.bin", 0x80004000},
#else
    /*mpw2*/
    {"/system/vendor/firmware/BCPU_ROM.bin",           0x80000000},
    {"/system/vendor/firmware/VECTORS.bin",            0x80010000},
    {"/system/vendor/firmware/RAM_VECTOR.bin",         0x80105c00},
    {"/system/vendor/firmware/WCPU_ROM.bin",           0x4000},
    {"/system/vendor/firmware/WL_ITCM.bin",            0x10000},
    {"/system/vendor/firmware/PLAT_RAM_EXCEPTION.bin", 0x20002800},
#endif
};
#endif
#endif
/*
*
***************************************************************************
  3
 Function Definition
***
**************************************************************************/
extern irqreturn_t bfg_wake_host_isr_etc(int irq, void *dev_id);
int ssi_check_wcpu_is_working(void);
int ssi_check_bcpu_is_working(void);
inline BOARD_INFO * get_hi110x_board_info_etc(void)
{
	return &g_board_info_etc;
}

int isAsic_etc(void)
{
    if (VERSION_ASIC == g_board_info_etc.is_asic)
    {
        return VERSION_ASIC;
    }
    else
    {
        return VERSION_FPGA;
    }
}
EXPORT_SYMBOL_GPL(isAsic_etc);

int isPmu_clk_request_enable(void)
{
    if (PMU_CLK_REQ_ENABLE == g_board_info_etc.pmu_clk_share_enable)
    {
        return PMU_CLK_REQ_ENABLE;
    }
    else
    {
        return PMU_CLK_REQ_DISABLE;
    }
}

int32 get_hi110x_subchip_type(void)
{
    BOARD_INFO * bd_info = NULL;

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return -EFAIL;
    }

    return bd_info->chip_nr;
}

#ifdef _PRE_CONFIG_USE_DTS
int32 get_board_dts_node_etc(struct device_node ** np, const char * node_prop)
{
	if (NULL ==np || NULL == node_prop)
	{
        PS_PRINT_ERR("func has NULL input param!!!, np=%p, node_prop=%p\n", np, node_prop);
		return BOARD_FAIL;
	}

	*np = of_find_compatible_node(NULL, NULL, node_prop);
	if (NULL == *np)
	{
		PS_PRINT_ERR("No compatible node %s found.\n", node_prop);
		return BOARD_FAIL;
	}

	return BOARD_SUCC;
}

int32 get_board_dts_prop_etc(struct device_node *np, const char * dts_prop, const char ** prop_val)
{
	int32 ret = BOARD_FAIL;

	if (NULL == np || NULL == dts_prop || NULL == prop_val)
	{
        PS_PRINT_ERR("func has NULL input param!!!, np=%p, dts_prop=%p, prop_val=%p\n", np, dts_prop, prop_val);
		return BOARD_FAIL;
	}

	ret = of_property_read_string(np, dts_prop, prop_val);
    if (ret)
    {
        PS_PRINT_ERR("can't get dts_prop value: dts_prop=%s\n", dts_prop);
        return ret;
    }

	PS_PRINT_SUC("have get dts_prop and prop_val: %s=%s\n", dts_prop, *prop_val);

	return BOARD_SUCC;
}

int32 get_board_dts_gpio_prop_etc(struct device_node *np, const char * dts_prop, int32 * prop_val)
{
	int32 ret = BOARD_FAIL;

	if (NULL == np || NULL == dts_prop || NULL == prop_val)
	{
        PS_PRINT_ERR("func has NULL input param!!!, np=%p, dts_prop=%p, prop_val=%p\n", np, dts_prop, prop_val);
		return BOARD_FAIL;
	}

    ret = of_get_named_gpio(np, dts_prop, 0);
    if (ret < 0)
    {
        PS_PRINT_ERR("can't get dts_prop value: dts_prop=%s, ret=%d\n", dts_prop, ret);
        return ret;
    }

    *prop_val = ret;
	PS_PRINT_SUC("have get dts_prop and prop_val: %s=%d\n", dts_prop, *prop_val);

	return BOARD_SUCC;
}

#endif

int32 get_board_gpio_etc(const char * gpio_node, const char * gpio_prop, int32 *physical_gpio)
{
#ifdef _PRE_CONFIG_USE_DTS
	int32 ret= BOARD_FAIL;
	struct device_node * np = NULL;

	ret = get_board_dts_node_etc(&np, gpio_node);
	if(BOARD_SUCC != ret)
	{
		return BOARD_FAIL;
	}

	ret = get_board_dts_gpio_prop_etc(np, gpio_prop, physical_gpio);
	if(BOARD_SUCC != ret)
	{
		return BOARD_FAIL;
	}

	return BOARD_SUCC;
#else
	return BOARD_SUCC;
#endif
}

int32 get_board_custmize_etc(const char * cust_node, const char * cust_prop, const char **cust_prop_val)
{
#ifdef _PRE_CONFIG_USE_DTS
	int32 ret= BOARD_FAIL;
	struct device_node * np = NULL;

	if (NULL == cust_node || NULL == cust_prop || NULL == cust_prop_val)
	{
        PS_PRINT_ERR("func has NULL input param!!!\n");
		return BOARD_FAIL;
	}

	ret = get_board_dts_node_etc(&np, cust_node);
	if(BOARD_SUCC != ret)
	{
		return BOARD_FAIL;
	}

	ret = get_board_dts_prop_etc(np, cust_prop, cust_prop_val);
	if(BOARD_SUCC != ret)
	{
		return BOARD_FAIL;
	}

	PS_PRINT_INFO("get board customize info %s=%s\n", cust_prop, *cust_prop_val);

	return BOARD_SUCC;
#else
	return BOARD_FAIL;
#endif
}

int32 get_board_pmu_clk32k_etc(void)
{
    return g_board_info_etc.bd_ops.get_board_pmu_clk32k_etc();
}

int32 set_board_pmu_clk32k_etc(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret= BOARD_FAIL;
    const char * clk_name = NULL;
    struct clk* clk = NULL;
    struct device *dev = NULL;

    dev = &pdev->dev;
    clk_name = g_board_info_etc.clk_32k_name;
    if (BOARD_VERSION_HI1102 == get_hi110x_subchip_type())
    {
        clk = devm_clk_get(dev, "clk_pmu32kb");
    }
    else
    {
        clk = devm_clk_get(dev, clk_name);
    }

    if (NULL == clk)
    {
        PS_PRINT_ERR("Get 32k clk %s failed!!!\n", clk_name);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DEV, CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_32K_CLK);
        return BOARD_FAIL;
    }
    g_board_info_etc.clk_32k = clk;

    ret = clk_prepare_enable(clk);
    if (unlikely(ret < 0))
    {
        devm_clk_put(dev, clk);
        PS_PRINT_ERR("enable 32K clk %s failed!!!", clk_name);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DEV, CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_32K_CLK);
        return BOARD_FAIL;
    }
#endif
	return BOARD_SUCC;
}

int32 get_board_uart_port_etc(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    return g_board_info_etc.bd_ops.get_board_uart_port_etc();
#else
    return BOARD_SUCC;
#endif
}

int32 check_evb_or_fpga_etc(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    return g_board_info_etc.bd_ops.check_evb_or_fpga_etc();
#else
    return BOARD_SUCC;
#endif
}

int32 check_pmu_clk_share_etc(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    return g_board_info_etc.bd_ops.check_pmu_clk_share_etc();
#else
    return BOARD_SUCC;
#endif
}

int32 board_get_power_pinctrl_etc(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    return g_board_info_etc.bd_ops.board_get_power_pinctrl_etc(pdev);
#else
    return BOARD_SUCC;
#endif
}

int32 board_power_gpio_init_etc(void)
{
    return g_board_info_etc.bd_ops.get_board_power_gpio();
}
void free_board_power_gpio_etc(void)
{
    g_board_info_etc.bd_ops.free_board_power_gpio_etc();
}
#ifdef HAVE_HISI_IR
void free_board_ir_gpio(void)
{
    if (g_board_info_etc.bfgx_ir_ctrl_gpio > -1)
    {
        gpio_free(g_board_info_etc.bfgx_ir_ctrl_gpio);
    }
}
#endif
void free_board_wakeup_gpio_etc(void)
{
    g_board_info_etc.bd_ops.free_board_wakeup_gpio_etc();
}

void free_board_wifi_tas_gpio_etc(void)
{
    g_board_info_etc.bd_ops.free_board_wifi_tas_gpio_etc();
}

int32 board_wakeup_gpio_init_etc(void)
{
    return g_board_info_etc.bd_ops.board_wakeup_gpio_init_etc();
}

int32 board_wifi_tas_gpio_init_etc(void)
{
    return g_board_info_etc.bd_ops.board_wifi_tas_gpio_init_etc();
}

#ifdef HAVE_HISI_IR
int32 board_ir_ctrl_init(struct platform_device *pdev)
{
    return g_board_info_etc.bd_ops.board_ir_ctrl_init(pdev);
}
#endif

int32 board_gpio_init_etc(struct platform_device *pdev)
{
    int32 ret= BOARD_FAIL;

    /*power on gpio request*/
    ret = board_power_gpio_init_etc();
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get power_on dts prop failed\n");
        goto err_get_power_on_gpio;
    }

    ret = board_wakeup_gpio_init_etc();
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get wakeup prop failed\n");
        goto oal_board_wakup_gpio_fail;
    }

    ret = board_wifi_tas_gpio_init_etc();
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get wifi tas prop failed\n");
        goto oal_board_wifi_tas_gpio_fail;
    }

#ifdef HAVE_HISI_IR
    ret = board_ir_ctrl_init(pdev);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get ir dts prop failed\n");
        goto err_get_ir_ctrl_gpio;
    }
#endif

    return BOARD_SUCC;

#ifdef HAVE_HISI_IR
err_get_ir_ctrl_gpio:
    free_board_wifi_tas_gpio_etc();
#endif
oal_board_wifi_tas_gpio_fail:
    free_board_wakeup_gpio_etc();
oal_board_wakup_gpio_fail:
    free_board_power_gpio_etc();
err_get_power_on_gpio:

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DEV, CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO);
    return BOARD_FAIL;
}

int board_get_bwkup_gpio_val_etc(void)
{
    return gpio_get_value(g_board_info_etc.bfgn_wakeup_host);
}

int board_get_wlan_wkup_gpio_val_etc(void)
{
    return gpio_get_value(g_board_info_etc.wlan_wakeup_host);
}

int32 board_irq_init_etc(void)
{
    uint32 irq = 0;
    int32 gpio = 0;

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    gpio = g_board_info_etc.wlan_wakeup_host;
    irq = gpio_to_irq(gpio);
    g_board_info_etc.wlan_irq = irq;

    PS_PRINT_INFO("wlan_irq is %d\n", g_board_info_etc.wlan_irq);
#endif

    gpio = g_board_info_etc.bfgn_wakeup_host;
    irq = gpio_to_irq(gpio);
    g_board_info_etc.bfgx_irq = irq;


    PS_PRINT_INFO("bfgx_irq is %d\n", g_board_info_etc.bfgx_irq);

    return BOARD_SUCC;
}

int32 board_clk_init_etc(struct platform_device *pdev)
{
    int32 ret= BOARD_FAIL;

    if (NULL == pdev)
    {
        PS_PRINT_ERR("func has NULL input param!!!\n");
        return BOARD_FAIL;
    }

    ret = g_board_info_etc.bd_ops.get_board_pmu_clk32k_etc();
    if(BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }

    ret = set_board_pmu_clk32k_etc(pdev);
    if(BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

void power_state_change_etc(int32 gpio, int32 flag)
{
    if(unlikely(0 == gpio))
    {
        PS_PRINT_WARNING("gpio is 0, flag=%d\n", flag);
        return;
    }

    PS_PRINT_INFO("power_state_change_etc gpio %d to %s\n", gpio, (BOARD_POWER_ON == flag) ? "low2high":"low");

    if (BOARD_POWER_ON == flag)
    {
        gpio_direction_output(gpio, GPIO_LOWLEVEL);
        mdelay(10);
        gpio_direction_output(gpio, GPIO_HIGHLEVEL);
        mdelay(20);
    }
    else if (BOARD_POWER_OFF == flag)
    {
        gpio_direction_output(gpio, GPIO_LOWLEVEL);
    }
}

int32 board_wlan_gpio_power_on(void* data)
{
    int32 gpio = (int32)(long)(data);
    if(g_board_info_etc.host_wakeup_wlan)
    {
        /*host wakeup dev gpio pinmux to jtag when w boot,
          must gpio low when bootup*/
        board_host_wakeup_dev_set(0);
    }
    power_state_change_etc(gpio, BOARD_POWER_ON);
    board_host_wakeup_dev_set(1);
    return 0;
}

int32 board_wlan_gpio_power_off(void* data)
{
    int32 gpio = (int32)(long)(data);
    power_state_change_etc(gpio, BOARD_POWER_OFF);
    return 0;
}

int32 board_host_wakeup_dev_set(int value)
{
    if(0 == g_board_info_etc.host_wakeup_wlan)
    {
        PS_PRINT_INFO("host_wakeup_wlan gpio is 0\n");
        return 0;
    }
    PS_PRINT_DBG("host_wakeup_wlan set %s %pF\n", value ? "high":"low", (void*)_RET_IP_);
    if(value)
    {
        return gpio_direction_output(g_board_info_etc.host_wakeup_wlan, GPIO_HIGHLEVEL);
    }
    else
    {
        return gpio_direction_output(g_board_info_etc.host_wakeup_wlan, GPIO_LOWLEVEL);
    }
}

int32 board_get_host_wakeup_dev_stat(void)
{
    return gpio_get_value(g_board_info_etc.host_wakeup_wlan);
}

int32 board_wifi_tas_set(int value)
{
    if(WIFI_TAS_DISABLE == g_board_info_etc.wifi_tas_enable)
    {
        return 0;
    }

    PS_PRINT_DBG("wifi tas gpio set %s %pF\n", value ? "high":"low", (void*)_RET_IP_);

    if(value)
    {
        return gpio_direction_output(g_board_info_etc.rf_wifi_tas, GPIO_HIGHLEVEL);
    }
    else
    {
        return gpio_direction_output(g_board_info_etc.rf_wifi_tas, GPIO_LOWLEVEL);
    }
}

EXPORT_SYMBOL(board_wifi_tas_set);

int32 board_get_wifi_tas_gpio_state(void)
{
    return gpio_get_value(g_board_info_etc.rf_wifi_tas);
}

EXPORT_SYMBOL(board_get_wifi_tas_gpio_state);

int32 board_power_on_etc(uint32 ul_subsystem)
{
    return g_board_info_etc.bd_ops.board_power_on_etc(ul_subsystem);
}
int32 board_power_off_etc(uint32 ul_subsystem)
{
    return g_board_info_etc.bd_ops.board_power_off_etc(ul_subsystem);
}

int32 board_power_reset(uint32 ul_subsystem)
{
    return g_board_info_etc.bd_ops.board_power_reset(ul_subsystem);
}
EXPORT_SYMBOL(board_wlan_gpio_power_off);
EXPORT_SYMBOL(board_wlan_gpio_power_on);

int32 find_device_board_version_etc(void)
{
    int32 ret= BOARD_FAIL;
    const char *device_version = NULL;

    ret = get_board_custmize_etc(DTS_NODE_HISI_HI110X, DTS_PROP_SUBCHIP_TYPE_VERSION, &device_version);
    if(BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }

    g_board_info_etc.chip_type = device_version;
    return BOARD_SUCC;
}

int32 board_chiptype_init(void)
{
    int32 ret= BOARD_FAIL;

    ret = find_device_board_version_etc();
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("can not find device_board_version\n");
        return BOARD_FAIL;
    }

    ret = check_device_board_name_etc();
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("check device name fail\n");
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

int32 board_func_init(void)
{
    int32 ret= BOARD_FAIL;
    //board init
    memset(&g_board_info_etc, 0 ,sizeof(BOARD_INFO));

    ret = board_chiptype_init();
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("sub chiptype init fail\n");
        return BOARD_FAIL;
    }

    PS_PRINT_INFO("hi11xx subchip is %s\n", g_board_info_etc.chip_type);

    switch (g_board_info_etc.chip_nr)
    {
        case  BOARD_VERSION_HI1102:
            g_board_info_etc.bd_ops.get_board_power_gpio           =hi1102_get_board_power_gpio;
            g_board_info_etc.bd_ops.free_board_power_gpio_etc      =hi1102_free_board_power_gpio_etc;
            g_board_info_etc.bd_ops.board_wakeup_gpio_init_etc     =hi1102_board_wakeup_gpio_init_etc;
            g_board_info_etc.bd_ops.free_board_wakeup_gpio_etc     =hi1102_free_board_wakeup_gpio_etc;
            g_board_info_etc.bd_ops.bfgx_dev_power_on_etc          =hi1102_bfgx_dev_power_on;
            g_board_info_etc.bd_ops.bfgx_dev_power_off_etc         =hi1102_bfgx_dev_power_off;
            g_board_info_etc.bd_ops.wlan_power_off_etc             =hi1102_wlan_power_off;
            g_board_info_etc.bd_ops.wlan_power_on_etc              =hi1102_wlan_power_on;
            g_board_info_etc.bd_ops.board_power_on_etc             =hi1102_board_power_on;
            g_board_info_etc.bd_ops.board_power_off_etc            =hi1102_board_power_off;
            g_board_info_etc.bd_ops.board_power_reset              =hi1102_board_power_reset;
            g_board_info_etc.bd_ops.get_board_pmu_clk32k_etc       =hi1102_get_board_pmu_clk32k;
            g_board_info_etc.bd_ops.get_board_uart_port_etc        =hi1102_get_board_uart_port;
            g_board_info_etc.bd_ops.board_ir_ctrl_init             =hi1102_board_ir_ctrl_init;
            g_board_info_etc.bd_ops.check_evb_or_fpga_etc          =hi1102_check_evb_or_fpga;
            g_board_info_etc.bd_ops.board_get_power_pinctrl_etc    =hi1102_board_get_power_pinctrl;
            g_board_info_etc.bd_ops.get_ini_file_name_from_dts_etc =hi1102_get_ini_file_name_from_dts;
            break;
        case BOARD_VERSION_HI1103:
            g_board_info_etc.bd_ops.get_board_power_gpio           =hi1103_get_board_power_gpio;
            g_board_info_etc.bd_ops.free_board_power_gpio_etc      =hi1103_free_board_power_gpio_etc;
            g_board_info_etc.bd_ops.board_wakeup_gpio_init_etc     =hi1103_board_wakeup_gpio_init_etc;
            g_board_info_etc.bd_ops.free_board_wakeup_gpio_etc     =hi1103_free_board_wakeup_gpio_etc;
            g_board_info_etc.bd_ops.board_wifi_tas_gpio_init_etc   =hi1103_board_wifi_tas_gpio_init_etc;
            g_board_info_etc.bd_ops.free_board_wifi_tas_gpio_etc   =hi1103_free_board_wifi_tas_gpio_etc;
            g_board_info_etc.bd_ops.bfgx_dev_power_on_etc          =hi1103_bfgx_dev_power_on;
            g_board_info_etc.bd_ops.bfgx_dev_power_off_etc         =hi1103_bfgx_dev_power_off;
            g_board_info_etc.bd_ops.wlan_power_off_etc             =hi1103_wlan_power_off;
            g_board_info_etc.bd_ops.wlan_power_on_etc              =hi1103_wlan_power_on;
            g_board_info_etc.bd_ops.board_power_on_etc             =hi1103_board_power_on;
            g_board_info_etc.bd_ops.board_power_off_etc            =hi1103_board_power_off;
            g_board_info_etc.bd_ops.board_power_reset              =hi1103_board_power_reset;
            g_board_info_etc.bd_ops.get_board_pmu_clk32k_etc       =hi1103_get_board_pmu_clk32k;
            g_board_info_etc.bd_ops.get_board_uart_port_etc        =hi1103_get_board_uart_port;
            g_board_info_etc.bd_ops.board_ir_ctrl_init             =hi1103_board_ir_ctrl_init;
            g_board_info_etc.bd_ops.check_evb_or_fpga_etc          =hi1103_check_evb_or_fpga;
            g_board_info_etc.bd_ops.check_pmu_clk_share_etc        =hi1103_check_pmu_clk_share;
            g_board_info_etc.bd_ops.board_get_power_pinctrl_etc    =hi1103_board_get_power_pinctrl;
            g_board_info_etc.bd_ops.get_ini_file_name_from_dts_etc =hi1103_get_ini_file_name_from_dts;
            break;
        default:
            PS_PRINT_ERR("g_board_info_etc.chip_nr=%d is illegal\n", g_board_info_etc.chip_nr);
            return BOARD_FAIL;
    }


    PS_PRINT_INFO("g_board_info_etc.chip_nr=%d, device_board_version is %s\n", g_board_info_etc.chip_nr, g_board_info_etc.chip_type);
    return BOARD_SUCC;
}

int32 check_download_channel_name_etc(uint8* wlan_buff, int32* index)
{
    int32 i = 0;
    for (i = 0; i < MODE_DOWNLOAD_BUTT; i++)
    {
        if (0 == strncmp(device_download_mode_list_etc[i].name, wlan_buff, strlen(device_download_mode_list_etc[i].name)))
        {
            *index = i;
            return BOARD_SUCC;
        }
    }
    return BOARD_FAIL;
}

int32 get_download_channel_etc(void)
{
    int32 ret= BOARD_FAIL;
    uint8 wlan_mode[DOWNLOAD_CHANNEL_LEN]={0};
    uint8 bfgn_mode[DOWNLOAD_CHANNEL_LEN]={0};

    /*wlan channel*/
    ret = find_download_channel_etc(wlan_mode, INI_WLAN_DOWNLOAD_CHANNEL);
    if (BOARD_SUCC != ret)
    {
        /*兼容1102,1102无此配置项*/
        g_board_info_etc.wlan_download_channel = MODE_SDIO;
        PS_PRINT_WARNING("can not find wlan_download_channel ,choose default:%s\n", device_download_mode_list_etc[0].name);
        hcc_bus_cap_init(HCC_CHIP_110X_DEV, NULL);
    }
    else
    {
        if (BOARD_SUCC != check_download_channel_name_etc(wlan_mode, &(g_board_info_etc.wlan_download_channel)))
        {
            PS_PRINT_ERR("check wlan download channel:%s error\n", bfgn_mode);
            return BOARD_FAIL;
        }
        hcc_bus_cap_init(HCC_CHIP_110X_DEV, wlan_mode);
    }


    /*bfgn channel*/
    ret = find_download_channel_etc(bfgn_mode, INI_BFGX_DOWNLOAD_CHANNEL);
    if (BOARD_SUCC != ret)
    {
        /*如果不存在该项，则默认保持和wlan一致*/
        g_board_info_etc.bfgn_download_channel = g_board_info_etc.wlan_download_channel;
        PS_PRINT_WARNING("can not find bfgn_download_channel ,choose default:%s\n", device_download_mode_list_etc[0].name);
        return BOARD_SUCC;
    }

    if (BOARD_SUCC != check_download_channel_name_etc(bfgn_mode, &(g_board_info_etc.bfgn_download_channel)))
    {
        PS_PRINT_ERR("check bfgn download channel:%s error\n", bfgn_mode);
        return BOARD_FAIL;
    }

    PS_PRINT_INFO("wlan_download_channel index:%d, bfgn_download_channel index:%d\n",
                        g_board_info_etc.wlan_download_channel, g_board_info_etc.bfgn_download_channel);

    return BOARD_SUCC;
}

uint32 g_ssi_dump_en = 0;
int32 get_ssi_dump_cfg(void)
{
    int32               l_cfg_value = 0;
    int32               l_ret = INI_FAILED;

    /* 获取ini的配置值 */
    l_ret = get_cust_conf_int32_etc(INI_MODU_PLAT, INI_SSI_DUMP_EN, &l_cfg_value);

    if (INI_FAILED == l_ret)
    {
        PS_PRINT_ERR("get_ssi_dump_cfg: fail to get ini, keep disable\n");
        return BOARD_SUCC;
    }

    g_ssi_dump_en = (uint32)l_cfg_value;

    PS_PRINT_INFO("get_ssi_dump_cfg: 0x%x\n",g_ssi_dump_en);

    return BOARD_SUCC;
}

int32 check_device_board_name_etc(void)
{
    int32 i = 0;
    for (i = 0; i < BOARD_VERSION_BOTT; i++)
    {
        if (0 == strncmp(device_board_version_list_etc[i].name, g_board_info_etc.chip_type, strlen(device_board_version_list_etc[i].name)))
        {
            g_board_info_etc.chip_nr = i;
            return BOARD_SUCC;
        }
    }

    return BOARD_FAIL;
}

int32 get_uart_pclk_source_etc(void)
{
    return g_board_info_etc.uart_pclk;
}

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
extern oal_int32 pcie_memcopy_type;
STATIC void board_cci_bypass_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
	int32 ret= BOARD_FAIL;
	struct device_node * np = NULL;

	if( -1 == pcie_memcopy_type)
	{
	    PS_PRINT_INFO("skip pcie mem burst control\n");
	    return;
	}

	ret = get_board_dts_node_etc(&np, DTS_NODE_HISI_CCIBYPASS);
	if(BOARD_SUCC != ret)
	{
		/*cci enable*/
		pcie_memcopy_type = 0;
		PS_PRINT_INFO("cci enable, pcie use mem burst 8 bytes\n");
	}
	else
	{
	    /*cci bypass*/
	    pcie_memcopy_type = 1;
	    PS_PRINT_INFO("cci bypass, pcie use mem burst 4 bytes\n");
	}
#endif
}
#endif

STATIC int32 hi110x_board_probe(struct platform_device *pdev)
{
    int ret = BOARD_FAIL;

    PS_PRINT_INFO("hi110x board init\n");
    ret = board_func_init();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = ini_cfg_init_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = check_evb_or_fpga_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = check_pmu_clk_share_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = get_download_channel_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = get_ssi_dump_cfg();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = board_clk_init_etc(pdev);
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    board_cci_bypass_init();
#endif

    ret = get_board_uart_port_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = board_gpio_init_etc(pdev);
    if (BOARD_SUCC != ret)
    {
        goto err_init;
    }

    ret = board_irq_init_etc();
    if (BOARD_SUCC != ret)
    {
        goto err_gpio_source;
    }

    ret = board_get_power_pinctrl_etc(pdev);
    if (BOARD_SUCC != ret)
    {
        goto err_get_power_pinctrl;
    }

    PS_PRINT_INFO("board init ok\n");

    board_probe_ret = BOARD_SUCC;
    complete(&board_driver_complete);

    return BOARD_SUCC;

err_get_power_pinctrl:
    free_irq(g_board_info_etc.bfgx_irq, NULL);
err_gpio_source:
#ifdef HAVE_HISI_IR
    free_board_ir_gpio();
#endif
    free_board_wakeup_gpio_etc();
    free_board_power_gpio_etc();

err_init:
    board_probe_ret = BOARD_FAIL;
    complete(&board_driver_complete);
    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DEV, CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_POWER_GPIO);
    return BOARD_FAIL;
}

STATIC int32 hi110x_board_remove(struct platform_device *pdev)
{
    PS_PRINT_INFO("hi110x board exit\n");

#ifdef _PRE_CONFIG_USE_DTS
    if (NEED_POWER_PREPARE == g_board_info_etc.need_power_prepare)
    {
        devm_pinctrl_put(g_board_info_etc.pctrl);
    }
#endif

    free_irq(g_board_info_etc.bfgx_irq, NULL);

#ifdef HAVE_HISI_IR
    free_board_ir_gpio();
#endif

    free_board_wakeup_gpio_etc();
    free_board_power_gpio_etc();

    return BOARD_SUCC;
}

int32 hi110x_board_suspend_etc(struct platform_device *pdev, pm_message_t state)
{
    return BOARD_SUCC;
}

int32 hi110x_board_resume_etc(struct platform_device *pdev)
{
    return BOARD_SUCC;
}


/*********************************************************************/
/********************   SSI调试代码start   ***************************/
/*********************************************************************/
#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
#define HI110X_SSI_CLK_GPIO_NAME  ("hi110x ssi clk")
#define HI110X_SSI_DATA_GPIO_NAME ("hi110x ssi data")
#define INTERVAL_TIME             (10)
#define SSI_DATA_LEN              (16)

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#define SSI_CLK_GPIO  89
#define SSI_DATA_GPIO 91
#else
#define SSI_CLK_GPIO  75
#define SSI_DATA_GPIO 77
#endif

char* g_ssi_hi1103_mpw2_cpu_st_str[] =
{
    "OFF",/*0x0*/
    "SLEEP",/*0x1*/
    "IDLE",/*0x2*/
    "WORK"/*0x3*/
};
char* g_ssi_hi1103_pilot_cpu_st_str[] =
{
    "OFF",/*0x0*/
    "BOOTING",/*0x1*/
    "SLEEPING",/*0x2*/
    "WORK",/*0x3*/
    "SAVING",/*0x4*/
    "PROTECT(ocp/scp)",/*0x5*/
    "SLEEP",/*0x6*/
    "PROTECTING"/*0x7*/
};

#define SSI_WRITE_DATA 0x5a5a
ssi_trans_test_st ssi_test_st = {0};

uint32 g_ssi_clk_etc  = 0;              /*模拟ssi时钟的GPIO管脚号*/
uint32 g_ssi_data_etc = 0;              /*模拟ssi数据线的GPIO管脚号*/
uint16 g_ssi_base_etc = 0x8000;         /*ssi基址*/
uint32 g_interval_etc = INTERVAL_TIME;  /*GPIO拉出来的波形保持时间，单位us*/
uint32 g_delay_etc    = 5;

/*ssi 工作时必须切换ssi clock,
  此时aon会受到影响，
  BCPU/WCPU 有可能异常，慎用!*/
int32 ssi_try_lock(void)
{
    oal_ulong flags;
    oal_spin_lock_irq_save(&g_ssi_lock, &flags);
    if(g_ssi_lock_state)
    {
        /*lock failed*/
        oal_spin_unlock_irq_restore(&g_ssi_lock, &flags);
        return 1;
    }
    g_ssi_lock_state = 1;
    oal_spin_unlock_irq_restore(&g_ssi_lock, &flags);
    return 0;
}

int32 ssi_unlock(void)
{
    oal_ulong flags;
    oal_spin_lock_irq_save(&g_ssi_lock, &flags);
    g_ssi_lock_state = 0;
    oal_spin_unlock_irq_restore(&g_ssi_lock, &flags);
    return 0;
}
int32 wait_for_ssi_idle_timeout(int32 mstimeout)
{
    int32 can_sleep = 0;
    int32 timeout=mstimeout;
    if (oal_in_interrupt() || oal_in_atomic() || irqs_disabled()) {
        can_sleep = 0;
    }
    else {
        can_sleep = 1;
    }
    /*考虑效率，这里需要判断是否可以睡眠*/
    while(ssi_try_lock()){
        if (can_sleep) {
            msleep(1);
        }else {
            mdelay(1);
        }
        if(!(--timeout)) {
                PS_PRINT_ERR("wait for ssi timeout:%dms\n",mstimeout);
                return 0;
        }
    }
    ssi_unlock();
    return timeout;
}
int32 ssi_show_setup_etc(void)
{
    PS_PRINT_INFO("clk=%d, data=%d, interval=%d us, ssi base=0x%x, r/w delay=%d cycle\n",
                    g_ssi_clk_etc, g_ssi_data_etc, g_interval_etc, g_ssi_base_etc, g_delay_etc);
    return BOARD_SUCC;
}

int32 ssi_setup_etc(uint32 interval, uint32 delay, uint16 ssi_base)
{
    g_interval_etc    = interval;
    g_delay_etc       = delay;
    g_ssi_base_etc    = ssi_base;

    return BOARD_SUCC;
}

int32 ssi_request_gpio_etc(uint32 clk, uint32 data)
{
    int32 ret = BOARD_FAIL;

    PS_PRINT_DBG("request hi110x ssi GPIO\n");
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(clk, GPIOF_OUT_INIT_LOW, HI110X_SSI_CLK_GPIO_NAME);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request_one failed ret=%d\n", HI110X_SSI_CLK_GPIO_NAME, ret);
        goto err_get_ssi_clk_gpio;
    }

    g_ssi_clk_etc = clk;

    ret = gpio_request_one(data, GPIOF_OUT_INIT_LOW, HI110X_SSI_DATA_GPIO_NAME);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request_one failed ret=%d\n", HI110X_SSI_DATA_GPIO_NAME, ret);
        goto err_get_ssi_data_gpio;
    }
#else
    ret = gpio_request(clk,  HI110X_SSI_CLK_GPIO_NAME);
    if(ret)
    {
        PS_PRINT_ERR("%s gpio_request failed  ret=%d\n", HI110X_SSI_CLK_GPIO_NAME, ret);
        goto err_get_ssi_clk_gpio;
    }

    gpio_direction_output(clk, 0);

    ret = gpio_request(data, HI110X_SSI_DATA_GPIO_NAME);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed  ret=%d\n", HI110X_SSI_DATA_GPIO_NAME, ret);
        goto err_get_ssi_data_gpio;
    }

    gpio_direction_output(data, 0);
#endif
    g_ssi_data_etc = data;

    return BOARD_SUCC;

err_get_ssi_data_gpio:
    gpio_free(clk);
    g_ssi_clk_etc = 0;
err_get_ssi_clk_gpio:

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DEV, CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO);

    return ret;
}

int32 ssi_free_gpio_etc(void)
{
    PS_PRINT_DBG("free hi110x ssi GPIO\n");

    if (0 != g_ssi_clk_etc)
    {
        gpio_free(g_ssi_clk_etc);
        g_ssi_clk_etc = 0;
    }

    if (0 != g_ssi_data_etc)
    {
        gpio_free(g_ssi_data_etc);
        g_ssi_data_etc = 0;
    }

    return BOARD_SUCC;
}

void ssi_clk_output_etc(void)
{
    gpio_direction_output(g_ssi_clk_etc, GPIO_LOWLEVEL);
    SSI_DELAY(g_interval_etc);
    gpio_direction_output(g_ssi_clk_etc, GPIO_HIGHLEVEL);
}

void ssi_data_output_etc(uint16 data)
{
    SSI_DELAY(5);
    if (data)
    {
        gpio_direction_output(g_ssi_data_etc, GPIO_HIGHLEVEL);
    }
    else
    {
        gpio_direction_output(g_ssi_data_etc, GPIO_LOWLEVEL);
    }

    SSI_DELAY(g_interval_etc);
}

int32 ssi_write_data_etc(uint16 addr, uint16 value)
{
    uint16 tx;
    uint32 i;

    for (i = 0; i < g_delay_etc; i++)
    {
        ssi_clk_output_etc();
        ssi_data_output_etc(0);
    }

    /*发送SYNC位*/
    PS_PRINT_DBG("tx sync bit\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(1);

    /*指示本次操作为写，高读低写*/
    PS_PRINT_DBG("tx r/w->w\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(0);

    /*发送地址*/
    PS_PRINT_DBG("write addr:0x%x\n", addr);
    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        tx = (addr >> (SSI_DATA_LEN - i - 1)) & 0x0001;
        PS_PRINT_DBG("tx addr bit %d:%d\n", SSI_DATA_LEN - i - 1, tx);
        ssi_clk_output_etc();
        ssi_data_output_etc(tx);
    }

    /*发送数据*/
    PS_PRINT_DBG("write value:0x%x\n", value);
    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        tx = (value >> (SSI_DATA_LEN - i - 1)) & 0x0001;
        PS_PRINT_DBG("tx data bit %d:%d\n", SSI_DATA_LEN - i - 1, tx);
        ssi_clk_output_etc();
        ssi_data_output_etc(tx);
    }

    /*数据发送完成以后，保持delay个周期的0*/
    PS_PRINT_DBG("ssi write:finish, delay %d cycle\n", g_delay_etc);
    for (i = 0; i < g_delay_etc; i++)
    {
        ssi_clk_output_etc();
        ssi_data_output_etc(0);
    }

    return BOARD_SUCC;
}

uint16 ssi_read_data_etc(uint16 addr)
{
#define SSI_READ_RETTY (1000)
    uint16 tx;
    uint32 i;
    uint32 retry = 0;
    uint16 rx;
    uint16 data = 0;

    for (i = 0; i < g_delay_etc; i++)
    {
        ssi_clk_output_etc();
        ssi_data_output_etc(0);
    }

    /*发送SYNC位*/
    PS_PRINT_DBG("tx sync bit\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(1);

    /*指示本次操作为读，高读低写*/
    PS_PRINT_DBG("tx r/w->r\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(1);

    /*发送地址*/
    PS_PRINT_DBG("read addr:0x%x\n", addr);
    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        tx = (addr >> (SSI_DATA_LEN - i - 1)) & 0x0001;
        PS_PRINT_DBG("tx addr bit %d:%d\n", SSI_DATA_LEN - i - 1, tx);
        ssi_clk_output_etc();
        ssi_data_output_etc(tx);
    }

    /*延迟一个clk，否则上一个数据只保持了半个时钟周期*/
    ssi_clk_output_etc();

    /*设置data线GPIO为输入，准备读取数据*/
    gpio_direction_input(g_ssi_data_etc);

    PS_PRINT_DBG("data in mod, current gpio level is %d\n", gpio_get_value(g_ssi_data_etc));

    /*读取SYNC同步位*/
    do
    {
        ssi_clk_output_etc();
        SSI_DELAY(g_interval_etc);
        if(gpio_get_value(g_ssi_data_etc))
        {
            PS_PRINT_DBG("read data sync bit ok, retry=%d\n", retry);
            break;
        }
        retry++;
    }while(SSI_READ_RETTY != retry);

    if (SSI_READ_RETTY == retry)
    {
        PS_PRINT_ERR("ssi read sync bit timeout\n");
        ssi_data_output_etc(0);
        return data;
    }

    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        ssi_clk_output_etc();
        SSI_DELAY(g_interval_etc);
        rx = gpio_get_value(g_ssi_data_etc);
        PS_PRINT_DBG("rx data bit %d:%d\n", SSI_DATA_LEN - i - 1, rx);
        data = data | (rx << (SSI_DATA_LEN - i - 1));
    }

    /*恢复data线GPIO为输出，并输出0*/
    ssi_data_output_etc(0);

    return data;
}

int32 ssi_write16_etc(uint16 addr, uint16 value)
{
#define write_retry   (3)
    uint32 retry = 0;
    uint16 read_v;

    do
    {
        ssi_write_data_etc(addr, value);
        read_v = ssi_read_data_etc(addr);
        if (value == read_v)
        {
            PS_PRINT_DBG("ssi write: 0x%x=0x%x succ\n", addr, value);
            return BOARD_SUCC;
        }
        retry++;
    }while(retry < write_retry);

    PS_PRINT_ERR("ssi write: 0x%x=0x%x ,read=0x%x fail\n", addr, value, read_v);

    return BOARD_FAIL;
}

uint16 ssi_read16_etc(uint16 addr)
{
    uint16 data;

    data = ssi_read_data_etc(addr);

    PS_PRINT_SUC("ssi read: 0x%x=0x%x\n", addr, data);

    return data;
}

int32 ssi_write32_etc(uint32 addr, uint16 value)
{
    uint16 addr_half_word_high;
    uint16 addr_half_word_low;

    addr_half_word_high = (addr >> 16) & 0xffff;
    addr_half_word_low  = (addr & 0xffff) >> 1;

    /*往基地址写地址的高16位*/
    if (ssi_write16_etc(g_ssi_base_etc, addr_half_word_high) < 0)
    {
        PS_PRINT_ERR("ssi write: 0x%x=0x%x fail\n", addr, value);
        return BOARD_FAIL;
    }

    /*低地址写实际要写入的value*/
    if (ssi_write16_etc(addr_half_word_low, value) < 0)
    {
        PS_PRINT_ERR("ssi write: 0x%x=0x%x fail\n", addr, value);
        return BOARD_FAIL;
    }

    PS_PRINT_DBG("ssi write: 0x%x=0x%x succ\n", addr, value);

    return BOARD_SUCC;
}

int32 ssi_read32_etc(uint32 addr)
{
    uint16  data = 0;
    uint16 addr_half_word_high;
    uint16 addr_half_word_low;

    addr_half_word_high = (addr >> 16) & 0xffff;
    addr_half_word_low  = (addr & 0xffff) >> 1;

    if (ssi_write16_etc(g_ssi_base_etc, addr_half_word_high) < 0)
    {
        PS_PRINT_ERR("ssi read 0x%x fail\n", addr);
        return BOARD_FAIL;
    }

    data = ssi_read_data_etc(addr_half_word_low);

    PS_PRINT_DBG("ssi read: 0x%x=0x%x\n", addr, data);

    return data;
}

int32 ssi_read_data16(uint16 addr, uint16 *value)
{
#define SSI_READ_RETTY (1000)
    uint16 tx;
    uint32 i;
    uint32 retry = 0;
    uint16 rx;
    uint16 data = 0;

    for (i = 0; i < g_delay_etc; i++)
    {
        ssi_clk_output_etc();
        ssi_data_output_etc(0);
    }

    /*发送SYNC位*/
    PS_PRINT_DBG("tx sync bit\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(1);

    /*指示本次操作为读，高读低写*/
    PS_PRINT_DBG("tx r/w->r\n");
    ssi_clk_output_etc();
    ssi_data_output_etc(1);

    /*发送地址*/
    PS_PRINT_DBG("read addr:0x%x\n", addr);
    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        tx = (addr >> (SSI_DATA_LEN - i - 1)) & 0x0001;
        PS_PRINT_DBG("tx addr bit %d:%d\n", SSI_DATA_LEN - i - 1, tx);
        ssi_clk_output_etc();
        ssi_data_output_etc(tx);
    }

    /*延迟一个clk，否则上一个数据只保持了半个时钟周期*/
    ssi_clk_output_etc();

    /*设置data线GPIO为输入，准备读取数据*/
    gpio_direction_input(g_ssi_data_etc);

    PS_PRINT_DBG("data in mod, current gpio level is %d\n", gpio_get_value(g_ssi_data_etc));

    /*读取SYNC同步位*/
    do
    {
        ssi_clk_output_etc();
        SSI_DELAY(g_interval_etc);
        if(gpio_get_value(g_ssi_data_etc))
        {
            PS_PRINT_DBG("read data sync bit ok, retry=%d\n", retry);
            break;
        }
        retry++;
    }while(SSI_READ_RETTY != retry);

    if (SSI_READ_RETTY == retry)
    {
        PS_PRINT_ERR("ssi read sync bit timeout\n");
        ssi_data_output_etc(0);
        return -OAL_EFAIL;
    }

    for (i = 0; i < SSI_DATA_LEN; i++)
    {
        ssi_clk_output_etc();
        SSI_DELAY(g_interval_etc);
        rx = gpio_get_value(g_ssi_data_etc);
        PS_PRINT_DBG("rx data bit %d:%d\n", SSI_DATA_LEN - i - 1, rx);
        data = data | (rx << (SSI_DATA_LEN - i - 1));
    }

    /*恢复data线GPIO为输出，并输出0*/
    ssi_data_output_etc(0);

    *value = data;

    return OAL_SUCC;
}

/*32bits address,
  32bits value*/
int32 ssi_read_value16(uint32 addr, uint16* value, int16 last_high_addr)
{
    int32 ret;
    uint16 addr_half_word_high;
    uint16 addr_half_word_low;

    addr_half_word_high = (addr >> 16) & 0xffff;
    addr_half_word_low  = (addr & 0xffff) >> 1;

    if(last_high_addr != addr_half_word_high)
    {
        if (ssi_write16_etc(g_ssi_base_etc, addr_half_word_high) < 0)
        {
            PS_PRINT_ERR("ssi read 0x%x fail\n", addr);
            return BOARD_FAIL;
        }
    }

    ret = ssi_read_data16(addr_half_word_low, value);

    PS_PRINT_DBG("ssi read: 0x%x=0x%x\n", addr, *value);

    return ret;
}

/*32bits address,
  32bits value
  gpio模拟SSI 读32BIT value
  1.配置SSI 为32BIT模式
  2.第一次读16BIT操作，SOC发起32BIT操作，返回低16BIT给HOST
  3.第二次读同一地址16BIT操作，SOC不发起总线操作，返回高16BIT给HOST
  4.如果跳过步骤3 读其他地址，SOC侧高16BIT 会被丢弃*/
int32 ssi_read_value32(uint32 addr, uint32* value, int16 last_high_addr)
{
    int32 ret;
    uint16 reg;

    ret = ssi_read_value16(addr, &reg, last_high_addr);
    if(ret)
    {
        PS_PRINT_ERR("read addr 0x%x low 16 bit failed, ret=%d\n", addr, ret);
        return ret;
    }
    *value = (uint32)reg;

    ret = ssi_read_value16(addr + 0x2, &reg, (addr >> 16));
    if(ret)
    {
        PS_PRINT_ERR("read addr 0x%x high 16 bit failed, ret=%d\n", addr, ret);
        return ret;
    }

    *value = ((reg << 16) | *value);

    return OAL_SUCC;
}



int32 ssi_read_value32_test(uint32 addr)
{
    int32 ret;
    uint32 value = 0xffffffff;

    ret = ssi_read_value32(addr, &value, (((addr >> 16) & 0xffff) + 1));
    if(ret)
    {
        PS_PRINT_ERR("ssi_read_value32 ret=%d\n", ret);
        return 0xffffffff;
    }

    return value;
}

int32 ssi_write_value32(uint32 addr, uint32 value)
{
    uint16 addr_half_word_high;
    uint16 addr_half_word_low;
    uint16 addr_half_word_low_incr;

    addr_half_word_high = (addr >> 16) & 0xffff;
    addr_half_word_low  = (addr & 0xffff) >> 1;
    addr_half_word_low_incr  = ((addr + 2) & 0xffff) >> 1;

    /*往基地址写地址的高16位*/
    if (ssi_write_data_etc(g_ssi_base_etc, addr_half_word_high) < 0)
    {
        PS_PRINT_ERR("ssi write high addr: 0x%x=0x%x fail\n", addr, value);
        return BOARD_FAIL;
    }

    /*低地址写实际要写入的value*/
    if (ssi_write_data_etc(addr_half_word_low, value & 0xffff) < 0)
    {
        PS_PRINT_ERR("ssi write low value: 0x%x=0x%x fail\n", addr, value);
        return BOARD_FAIL;
    }

    if (ssi_write_data_etc(addr_half_word_low_incr, (value >> 16) & 0xffff) < 0)
    {
        PS_PRINT_ERR("ssi write high value: 0x%x=0x%x fail\n", addr, value);
        return BOARD_FAIL;
    }

    PS_PRINT_DBG("ssi write: 0x%x=0x%x succ\n", addr, value);

    return BOARD_SUCC;
}

/*16bits/32bits switch mode*/
int32 ssi_switch_ahb_mode(oal_int32 is_32bit_mode)
{
    return ssi_write16_etc(0x8001, !!is_32bit_mode);
}

int32 ssi_clear_ahb_highaddr(void)
{
    return ssi_write16_etc(g_ssi_base_etc, 0x0);;
}

int32 do_ssi_file_test(ssi_file_st *file_st, ssi_trans_test_st* pst_ssi_test)
{
    OS_KERNEL_FILE_STRU        *fp;
    uint16 data_buf = 0;
    int32 rdlen = 0;
    uint32 ul_addr = 0;
    int32 l_ret = BOARD_FAIL;

    if ((NULL == pst_ssi_test) || ( NULL == file_st))
    {
        return BOARD_FAIL;
    }
    fp = filp_open(file_st->file_name, O_RDONLY, 0);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        fp = NULL;
        PS_PRINT_ERR("filp_open %s fail!!\n",  file_st->file_name);
        return -EFAIL;
    }
    ul_addr = file_st->write_addr;
    PS_PRINT_INFO("begin file:%s", file_st->file_name);
    while(1)
    {
        data_buf = 0;
        rdlen = kernel_read(fp, fp->f_pos, (uint8 *)&data_buf, 2);
        if (rdlen > 0)
        {
            fp->f_pos += rdlen;
        }
        else if (0 == rdlen)
        {
            PS_PRINT_INFO("file read over:%s!!\n",  file_st->file_name);
            break;
        }
        else
        {
            PS_PRINT_ERR("file read ERROR:%d!!\n", rdlen);
            goto test_fail;
        }
        l_ret = ssi_write32_etc(ul_addr, data_buf);
        if (BOARD_SUCC != l_ret)
        {
            PS_PRINT_ERR(" write data error, ul_addr=0x%x, l_ret=%d\n", ul_addr, l_ret);
            goto test_fail;
        }
        pst_ssi_test->trans_len += 2;
        ul_addr +=2;
    }
    filp_close(fp, NULL);
    fp = NULL;
    PS_PRINT_INFO("%s send finish\n",  file_st->file_name);
    return BOARD_SUCC;
test_fail:
    filp_close(fp, NULL);
    fp = NULL;
    return BOARD_FAIL;
}
typedef struct ht_test_s {
    int32 add;
    int32 data;
}ht_test_t;

ht_test_t ht_cnt[]={
    {0x50000314,    0x0D00},
    {0x50002724,    0x0022},
    {0x50002720,    0x0033},
};
int32 test_hd_ssi_write(void)
{
    int32 i;
    if (BOARD_SUCC != ssi_request_gpio_etc(SSI_CLK_GPIO, SSI_DATA_GPIO))
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }

    if (BOARD_SUCC != ssi_write16_etc(0x8007,1))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    for (i=0;i<sizeof(ht_cnt)/sizeof(ht_test_t);i++)
    {
        if (0 != ssi_write32_etc(ht_cnt[i].add, ht_cnt[i].data))
        {
            PS_PRINT_ERR("error: ssi write fail s_addr:0x%x s_data:0x%x\n", ht_cnt[i].add,ht_cnt[i].data);
            //return BOARD_FAIL;
        }
        else
        {
            //PS_PRINT_ERR("0x%x:0x%x succ\n", ht_cnt[i].add,ht_cnt[i].data);
        }
    }

    /*reset clk*/
    if (BOARD_SUCC != ssi_write16_etc(0x8007,0))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    if (BOARD_SUCC != ssi_free_gpio_etc())
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }
    PS_PRINT_ERR("ALL reg finish---------------------");
    return 0;
err_exit:
    PS_PRINT_ERR("test reg fail---------------------");
    ssi_free_gpio_etc();
    return BOARD_FAIL;

}
int32 ssi_single_write(int32 addr, int16 data)
{
    if (BOARD_SUCC != ssi_request_gpio_etc(SSI_CLK_GPIO, SSI_DATA_GPIO))
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }

    if (BOARD_SUCC != ssi_write16_etc(0x8007,1))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    /*set wcpu wait*/
    if (BOARD_SUCC != ssi_write32_etc(addr, data))
    {
         goto err_exit;
    }
    /*reset clk*/
    if (BOARD_SUCC != ssi_write16_etc(0x8007,0))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    if (BOARD_SUCC != ssi_free_gpio_etc())
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }
    return 0;
err_exit:
    ssi_free_gpio_etc();
    return BOARD_FAIL;
}
int32 ssi_single_read(int32 addr)
{
    int32 ret;
    if (BOARD_SUCC != ssi_request_gpio_etc(SSI_CLK_GPIO, SSI_DATA_GPIO))
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }
    if (BOARD_SUCC != ssi_write16_etc(0x8007,1))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    ret = ssi_read32_etc(addr);
    /*reset clk*/
    if (BOARD_SUCC != ssi_write16_etc(0x8007,0))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         goto err_exit;
    }
    if (BOARD_SUCC != ssi_free_gpio_etc())
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        return BOARD_FAIL;
    }
    return ret;
err_exit:
    ssi_free_gpio_etc();
    return BOARD_FAIL;
}
int32 ssi_file_test(ssi_trans_test_st* pst_ssi_test)
{
    int32 i = 0;
    if (NULL == pst_ssi_test)
    {
        return BOARD_FAIL;
    }
    pst_ssi_test->trans_len = 0;

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    hi1103_chip_power_on();
    hi1103_bfgx_enable();
    if(hi1103_wifi_enable())
    {
        PS_PRINT_ERR("hi1103_wifi_enable failed!\n");
        return BOARD_FAIL;
    }
#endif

    //ssi_setup_etc(20, 10, g_ssi_base_etc);
    //waring: fpga version should set 300801c0 1 to let host control ssi
    /*first set ssi clk ctl*/
    if (BOARD_SUCC != ssi_write16_etc(0x8007,1))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         return BOARD_FAIL;
    }
    //env init
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
    /*set bootloader deadbeaf*/
    if (BOARD_SUCC != ssi_write32_etc(0x8010010c, 0xbeaf))
    {
         PS_PRINT_ERR("set flag:beaf fail\n");
         return BOARD_FAIL;
    }
    if (BOARD_SUCC != ssi_write32_etc(0x8010010e, 0xdead))
    {
         PS_PRINT_ERR("set flag:dead fail\n");
         return BOARD_FAIL;
    }
#else
    /*set wcpu wait*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000e00, 0x1))
    {
         PS_PRINT_ERR("set wcpu wait fail\n");
         return BOARD_FAIL;
    }

    /*reset wcpu */
    if (BOARD_SUCC != ssi_write32_etc(0x40000030, 0xfe5e))
    {
         //脉冲复位
         //PS_PRINT_ERR("reset wcpu fail\n");
         //return BOARD_FAIL;
    }
    /*boot flag*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000200, 0xbeaf))
    {
         PS_PRINT_ERR("set boot flag fail\n");
         return BOARD_FAIL;
    }
    /*dereset bcpu*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000094, 1))
    {
         PS_PRINT_ERR("dereset bcpu\n");
         return BOARD_FAIL;
    }
#endif
    /*file download*/
    for (i = 0; i < sizeof(g_aSsiFile)/sizeof(ssi_file_st); i++)
    {
        if (BOARD_SUCC != do_ssi_file_test(&g_aSsiFile[i], pst_ssi_test))
        {
            PS_PRINT_ERR("%s write %d error\n", g_aSsiFile[i].file_name, g_aSsiFile[i].write_addr);
            return BOARD_FAIL;
        }
    }
    /*let cpu go*/
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
    /*reset bcpu*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000094, 0))
    {
         PS_PRINT_ERR("reset bcpu set 0 fail\n");
         return BOARD_FAIL;
    }
    if (BOARD_SUCC != ssi_write32_etc(0x50000094, 1))
    {
         PS_PRINT_ERR("reset bcpu set 1 fail\n");
         return BOARD_FAIL;
    }
#else
    /*clear b wait*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000e04, 0x0))
    {
         PS_PRINT_ERR("clear b wait\n");
         return BOARD_FAIL;
    }
    /*clear w wait*/
    if (BOARD_SUCC != ssi_write32_etc(0x50000e00, 0x0))
    {
         PS_PRINT_ERR("clear w wait\n");
         return BOARD_FAIL;
    }
#endif
    /*reset clk*/
    if (BOARD_SUCC != ssi_write16_etc(0x8007,0))
    {
         PS_PRINT_ERR("set ssi clk fail\n");
         return BOARD_FAIL;
    }
    return BOARD_SUCC;
}
int32 do_ssi_mem_test(ssi_trans_test_st* pst_ssi_test)
{
    uint32 i = 0;
    uint32 ul_write_base = 0x0;
    uint32 ul_addr;
    int32 l_ret = BOARD_FAIL;
    if (NULL == pst_ssi_test)
    {
        return BOARD_FAIL;
    }

    for (i = 0; i < pst_ssi_test->trans_len; i++ )
    {
        ul_addr = ul_write_base + 2*i;  //按2字节读写
        l_ret = ssi_write32_etc(ul_addr, SSI_WRITE_DATA);
        if (BOARD_SUCC != l_ret)
        {
            PS_PRINT_ERR(" write data error, ul_addr=0x%x, l_ret=%d\n", ul_addr, l_ret);
            return l_ret;
        }
        l_ret = ssi_read32_etc(ul_addr);
        if (SSI_WRITE_DATA != l_ret)
        {
            PS_PRINT_ERR("read write 0x%x error, expect:0x5a5a,actual:0x%x\n",ul_addr, l_ret);
            return l_ret;
        }
    }
    return BOARD_SUCC;
}
int32 ssi_download_test(ssi_trans_test_st* pst_ssi_test)
{

    int32 l_ret = BOARD_FAIL;

    struct timeval stime,etime;

    if (NULL == pst_ssi_test)
    {
        return BOARD_FAIL;
    }
    pst_ssi_test->trans_len = 1024;
    if (BOARD_SUCC != ssi_request_gpio_etc(SSI_CLK_GPIO, SSI_DATA_GPIO))
    {
        PS_PRINT_ERR("ssi_request_gpio_etc fail\n");
        goto fail_process;
    }

    do_gettimeofday(&stime);
    switch (pst_ssi_test->test_type)
    {
        case SSI_MEM_TEST:
            l_ret = do_ssi_mem_test(pst_ssi_test);
            break;
        case SSI_FILE_TEST:
            l_ret = ssi_file_test(pst_ssi_test);
            break;
        default:
            PS_PRINT_ERR("error type=%d\n", pst_ssi_test->test_type);
            break;
    }
    do_gettimeofday(&etime);
    ssi_free_gpio_etc();
    if (BOARD_SUCC != l_ret)
    {
        goto fail_process;
    }
    pst_ssi_test->used_time = (etime.tv_sec - stime.tv_sec)*1000 + (etime.tv_usec - stime.tv_usec)/1000;
    pst_ssi_test->send_status = 0;
    return BOARD_SUCC;
fail_process:
    pst_ssi_test->used_time = 0;
    pst_ssi_test->send_status = -1;
    return BOARD_FAIL;

}

ssi_reg_info hi1103_glb_ctrl_full     = {0x50000000, 0x1000, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_glb_ctrl_extend1  = {0x50001400, 0x10, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_glb_ctrl_extend2  = {0x50001540, 0xc, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_glb_ctrl_extend3  = {0x50001600, 0x4, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_pmu_cmu_ctrl_full = {0x50002000, 0xb00, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_full = {0x50003000, 0xa20, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_tail = {0x50003a80, 0xc, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_w_ctrl_full =    {0x40000000, 0x408, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_w_key_mem  =    {0x2001e620, 0x80, SSI_RW_DWORD_MOD};
ssi_reg_info hi1103_b_ctrl_full =    {0x48000000, 0x40c, SSI_RW_WORD_MOD};
ssi_reg_info hi1103_pcie_ctrl_full = {0x40007000, 0x4c8, SSI_RW_DWORD_MOD};
ssi_reg_info hi1103_pcie_dbi_full = {0x40102000, 0x900, SSI_RW_DWORD_MOD};/*没建链之前不能读*/
ssi_reg_info hi1103_pcie_pilot_iatu_full = {0x40104000, 0x2000, SSI_RW_DWORD_MOD};/*8KB*/
ssi_reg_info hi1103_pcie_pilot_dma_full = {0x40106000, 0x1000, SSI_RW_DWORD_MOD};/*4KB*/
ssi_reg_info hi1103_pcie_dma_ctrl_full = {0x40008000, 0x34, SSI_RW_DWORD_MOD};
ssi_reg_info hi1103_pcie_sdio_ctrl_full = {0x40101000, 0x180, SSI_RW_DWORD_MOD};

ssi_reg_info *hi1103_aon_reg_full[] = {
    &hi1103_glb_ctrl_full,
    &hi1103_glb_ctrl_extend1,
    &hi1103_glb_ctrl_extend2,
    &hi1103_glb_ctrl_extend3,
    &hi1103_pmu_cmu_ctrl_full,
    &hi1103_pmu2_cmu_ir_ctrl_full,
    &hi1103_pmu2_cmu_ir_ctrl_tail
};

//0x5000_0000~0x500000FC
ssi_reg_info hi1103_glb_ctrl_cut1     = {0x50000000, 0xfc, SSI_RW_WORD_MOD};
//0x5000_0200~0x5000_020C
ssi_reg_info hi1103_glb_ctrl_cut2     = {0x50000200, 0xc, SSI_RW_WORD_MOD};
//0x5000_0400~0x5000_043C
ssi_reg_info hi1103_glb_ctrl_cut3     = {0x50000400, 0x3c, SSI_RW_WORD_MOD};
//0x5000_0500~0x5000_051C
ssi_reg_info hi1103_glb_ctrl_cut4     = {0x50000500, 0x1c, SSI_RW_WORD_MOD};
//0x5000_0700~0x5000_070C
ssi_reg_info hi1103_glb_ctrl_cut5     = {0x50000700, 0xc, SSI_RW_WORD_MOD};
//0x5000_0E00~0x5000_0E0C
ssi_reg_info hi1103_glb_ctrl_cut6     = {0x50000E00, 0xc, SSI_RW_WORD_MOD};
//0x5000_1400~0x5000_140C
ssi_reg_info hi1103_glb_ctrl_cut7     = {0x50001400, 0x10, SSI_RW_WORD_MOD};
//0x5000_1540~0x5000_1548
ssi_reg_info hi1103_glb_ctrl_cut8     = {0x50001540, 0xc, SSI_RW_WORD_MOD};
//0x5000_1600~0x5000_1604
ssi_reg_info hi1103_glb_ctrl_cut9     = {0x50001600, 0x4, SSI_RW_WORD_MOD};

//PMU_CMU_CTRL
//0x50002080~0x500021AC
ssi_reg_info hi1103_pmu_cmu_ctrl_cut1 = {0x50002080, 0x12c, SSI_RW_WORD_MOD};
//0x50002200~0x5000220C
ssi_reg_info hi1103_pmu_cmu_ctrl_cut2 = {0x50002200, 0xc, SSI_RW_WORD_MOD};
//0x50002380~0x5000239C
ssi_reg_info hi1103_pmu_cmu_ctrl_cut3 = {0x50002380, 0x1c, SSI_RW_WORD_MOD};
//0x50002800~0x5000283C
ssi_reg_info hi1103_pmu_cmu_ctrl_cut4 = {0x50002800, 0x3c, SSI_RW_WORD_MOD};



//PMU2_CMU_IR_TS_EF_CTL
//0x50003040~0x5000307C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut1 = {0x50003040, 0x3c, SSI_RW_WORD_MOD};
//0x5000311C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut2 = {0x5000311C, 0x4, SSI_RW_WORD_MOD};
//0x5000313C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut3 = {0x5000313C, 0x4, SSI_RW_WORD_MOD};
//0x5000315C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut4 = {0x5000315C, 0x4, SSI_RW_WORD_MOD};
//0x5000317C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut5 = {0x5000317C, 0x4, SSI_RW_WORD_MOD};
//0x5000319C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut6 = {0x5000319C, 0x4, SSI_RW_WORD_MOD};
//0x50003220~0x5000339C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut7 = {0x50003220, 0x17c, SSI_RW_WORD_MOD};
//0x50003420~0x5000343C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut8 = {0x50003420, 0x1c, SSI_RW_WORD_MOD};
//0x50003780~0x500037FC
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut9 = {0x50003780, 0x7c, SSI_RW_WORD_MOD};
//0x50003800~0x500038BF
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut10 = {0x50003800, 0xc0, SSI_RW_WORD_MOD};
//0x50003A80~0x50003A8C
ssi_reg_info hi1103_pmu2_cmu_ir_ctrl_cut11 = {0x50003A80, 0xc, SSI_RW_WORD_MOD};

ssi_reg_info *hi1103_aon_reg_cut[] = {
    &hi1103_glb_ctrl_cut1,
    &hi1103_glb_ctrl_cut2,
    &hi1103_glb_ctrl_cut3,
    &hi1103_glb_ctrl_cut4,
    &hi1103_glb_ctrl_cut5,
    &hi1103_glb_ctrl_cut6,
    &hi1103_glb_ctrl_cut7,
    &hi1103_glb_ctrl_cut8,
    &hi1103_glb_ctrl_cut9,
    &hi1103_pmu_cmu_ctrl_cut1,
    &hi1103_pmu_cmu_ctrl_cut2,
    &hi1103_pmu_cmu_ctrl_cut3,
    &hi1103_pmu_cmu_ctrl_cut4,
    &hi1103_pmu2_cmu_ir_ctrl_cut1,
    &hi1103_pmu2_cmu_ir_ctrl_cut2,
    &hi1103_pmu2_cmu_ir_ctrl_cut3,
    &hi1103_pmu2_cmu_ir_ctrl_cut4,
    &hi1103_pmu2_cmu_ir_ctrl_cut5,
    &hi1103_pmu2_cmu_ir_ctrl_cut6,
    &hi1103_pmu2_cmu_ir_ctrl_cut7,
    &hi1103_pmu2_cmu_ir_ctrl_cut8,
    &hi1103_pmu2_cmu_ir_ctrl_cut9,
    &hi1103_pmu2_cmu_ir_ctrl_cut10,
    &hi1103_pmu2_cmu_ir_ctrl_cut11
};

ssi_reg_info hi1103_pcie_ctrl_cut1 = {0x40007224, 0x4,  SSI_RW_DWORD_MOD};
ssi_reg_info hi1103_pcie_ctrl_cut2 = {0x400072d0, 0x4,  SSI_RW_DWORD_MOD};
ssi_reg_info hi1103_pcie_ctrl_cut3 = {0x40007430, 0x9c, SSI_RW_DWORD_MOD};

ssi_reg_info *hi1103_pcie_cfg_reg_cut[] = {
    &hi1103_pcie_ctrl_cut1,
    &hi1103_pcie_ctrl_cut2,
    &hi1103_pcie_ctrl_cut3
};

ssi_reg_info *hi1103_pcie_cfg_reg_full[] = {
    &hi1103_pcie_ctrl_full,
    &hi1103_pcie_dma_ctrl_full
};

ssi_reg_info *hi1103_pcie_dbi_mpw2_reg_full[] = {
    &hi1103_pcie_dbi_full,
};

ssi_reg_info *hi1103_pcie_dbi_pilot_reg_full[] = {
    &hi1103_pcie_dbi_full,
};

int ssi_check_device_isalive(void)
{
    int i;
    uint32 reg;
    for(i = 0; i < 2; i++)
    {
        reg = (uint32)ssi_read32_etc(0x50000000);
        if(0x101 == reg)
        {
            PS_PRINT_INFO("reg is 0x%x\n", reg);
            break;
        }
    }

    if(2 == i)
    {
        PS_PRINT_INFO("ssi is fail, gpio-ssi did't support, reg=0x%x\n", reg);
        return -1;
    }
    return 0;
}

int ssi_check_is_pilot(void)
{
    int32 ret;
    uint16 value = 0;
    /*pilot pmuctrl 0x598 is reserved*/

    if(-1 == ssi_is_pilot)
    {
        ret = ssi_read_value16(0x50002598, &value, 0x0);
        if(ret)
        {
            PS_PRINT_ERR("read 0x50002598 failed\n");
            return ret;
        }
        else
        {
            PS_PRINT_INFO("value=0x%x [%s]\n", value, value ? "mpw2":"pilot");
            if(value)
            {
                ssi_is_pilot = 0;
                return 0;
            }
            else
            {
                ssi_is_pilot = 1;
                return 1;
            }
        }
    }
    else
    {
        PS_PRINT_INFO("%s\n", ssi_is_pilot ? "pilot":"mpw2");
        return ssi_is_pilot;
    }
}

int ssi_read_wpcu_pc_lr_sp(int trace_en)
{
    int i;
    uint32 reg_low, reg_high, pc, lr, sp;

    /*read pc twice check whether wcpu is runing*/
    for(i = 0; i < 2; i++)
    {
        ssi_write32_etc(0x50000400, 0x1);
        oal_mdelay(1);

        reg_low = (uint32)ssi_read32_etc(0x50000404);
        reg_high = (uint32)ssi_read32_etc(0x50000408);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        pc = reg_low | (reg_high << 16);

        reg_low = (uint32)ssi_read32_etc(0x5000040c);
        reg_high = (uint32)ssi_read32_etc(0x50000410);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        lr = reg_low | (reg_high << 16);

        reg_low = (uint32)ssi_read32_etc(0x50000414);
        reg_high = (uint32)ssi_read32_etc(0x50000418);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        sp = reg_low | (reg_high << 16);

        PS_PRINT_INFO("gpio-ssi:read wcpu[%i], pc:0x%x, lr:0x%x, sp:0x%x \n", i, pc, lr, sp);
        if(!pc && !lr && !sp)
        {
            PS_PRINT_INFO("wcpu pc lr sp all zero\n");
            if(trace_en)
            {
                if( 1 == ssi_check_is_pilot())
                {
                    if(ssi_check_wcpu_is_working())
                    {
                        PS_PRINT_INFO("wcpu try to enable trace en\n");
                        ssi_write32_etc(0x40004c00, 0x1);
                        oal_mdelay(1);
                    }
                    trace_en = 0;
                    i = -1;
                }
            }
            //return 0;
        }
        oal_mdelay(10);
    }

    return 0;
}

int ssi_read_bpcu_pc_lr_sp(int trace_en)
{
    int i;
    uint32 reg_low, reg_high, pc, lr, sp;

    /*read pc twice check whether wcpu is runing*/
    for(i = 0; i < 2; i++)
    {
        ssi_write32_etc(0x50000420, 0x1);
        oal_mdelay(1);

        reg_low = (uint32)ssi_read32_etc(0x50000424);
        reg_high = (uint32)ssi_read32_etc(0x50000428);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        pc = reg_low | (reg_high << 16);

        reg_low = (uint32)ssi_read32_etc(0x5000042c);
        reg_high = (uint32)ssi_read32_etc(0x50000430);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        lr = reg_low | (reg_high << 16);

        reg_low = (uint32)ssi_read32_etc(0x50000434);
        reg_high = (uint32)ssi_read32_etc(0x50000438);
        //PS_PRINT_INFO("low:0x%x, high:0x%x\n", reg_low, reg_high);
        sp = reg_low | (reg_high << 16);

        PS_PRINT_INFO("gpio-ssi:read bcpu[%i], pc:0x%x, lr:0x%x, sp:0x%x \n", i, pc, lr, sp);
        if(!pc && !lr && !sp)
        {
            PS_PRINT_INFO("bcpu pc lr sp all zero\n");
            if(trace_en)
            {
                if( 1 == ssi_check_is_pilot())
                {
                    if(ssi_check_bcpu_is_working())
                    {
                        PS_PRINT_INFO("bcpu try to enable trace en\n");
                        ssi_write32_etc(0x48007c00, 0x1);
                        oal_mdelay(1);
                    }
                    trace_en = 0;
                    i = -1;
                }
            }
            //return 0;
        }
        oal_mdelay(10);
    }

    return 0;
}

void ssi_check_buck_scp_ocp_status(void)
{
    uint32 reg;

    /*buck ocp/acp*/
    reg = (uint32)ssi_read32_etc(0x50002380);
    if(0 != (reg&(0xFFFFFFFC)))
    {
        /*bit 0,1*/
        PS_PRINT_INFO("buck protect status:0x%x invalid", reg);
        return;
    }

    PS_PRINT_INFO("buck protect status:0x%x %s %s \n", reg,
                    (reg & 0x1) ? "buck_scp_off":"", (reg & 0x2) ? "buck_ocp_off":"");
#ifdef CONFIG_HUAWEI_DSM
    if(reg & 0x3)
        hw_1103_dsm_client_notify(DSM_BUCK_PROTECTED, "%s: buck protect status:0x%x %s %s \n", __FUNCTION__, reg,
                    (reg & 0x1) ? "buck_scp_off":"", (reg & 0x2) ? "buck_ocp_off":"");
#endif
}

int ssi_check_wcpu_is_working(void)
{
    uint32 reg, mask;
    int32 ret = ssi_check_is_pilot();
    if( ret < 0 )
        return ret;

    if(ret)
    {
        /*pilot*/
        reg = (uint32)ssi_read32_etc(0x50002200);
        mask = reg & 0x7;
        PS_PRINT_INFO("cpu state=0x%8x, wcpu is %s\n", reg, g_ssi_hi1103_pilot_cpu_st_str[mask]);
#ifdef CONFIG_HUAWEI_DSM
        hw_1103_dsm_client_notify(DSM_1103_HALT, "%s: cpu state=0x%8x, wcpu is %s\n", __FUNCTION__, reg, g_ssi_hi1103_pilot_cpu_st_str[mask]);
#endif
        if(0x5 == mask)
        {
            ssi_check_buck_scp_ocp_status();
        }
        return (0x3 == mask);
    }
    else
    {
        /*mpw2*/
        reg = (uint32)ssi_read32_etc(0x50002240);
        mask = reg & 0x3;
        PS_PRINT_INFO("cpu state=0x%8x, wcpu is %s\n", reg, g_ssi_hi1103_mpw2_cpu_st_str[mask]);
        return (0x3 == mask);
    }
}

int ssi_check_bcpu_is_working(void)
{
    uint32 reg, mask;
    int32 ret = ssi_check_is_pilot();
    if( ret < 0 )
        return ret;

    if(ret)
    {
        /*pilot*/
        reg = (uint32)ssi_read32_etc(0x50002200);
        mask = (reg >> 3) & 0x7;
        PS_PRINT_INFO("cpu state=0x%8x, bcpu is %s\n", reg, g_ssi_hi1103_pilot_cpu_st_str[mask]);
#ifdef CONFIG_HUAWEI_DSM
        hw_1103_dsm_client_notify(DSM_1103_HALT, "%s: cpu state=0x%8x, bcpu is %s\n", __FUNCTION__, reg, g_ssi_hi1103_pilot_cpu_st_str[mask]);
#endif
        if(0x5 == mask)
        {
            ssi_check_buck_scp_ocp_status();
        }
        return (0x3 == mask);
    }
    else
    {
        /*mpw2*/
        reg = (uint32)ssi_read32_etc(0x50002240);
        mask = (reg >> 2) & 0x3;
        PS_PRINT_INFO("cpu state=0x%8x, bcpu is %s\n", reg, g_ssi_hi1103_mpw2_cpu_st_str[mask]);
        return (0x3 == mask);
    }
}

int ssi_read_device_arm_register(int trace_en)
{
    int32 ret;
    int32 is_pilot;

    uint32 reg = (uint32)ssi_read32_etc(0x50003a88);

    if(0x3 == reg)
    {
        PS_PRINT_ERR("0x50003a88 is 0x3, wifi chip maybe enter dft mode , please check!\n");
    }

    /*read PC*/
    is_pilot = ssi_check_is_pilot();
    if(is_pilot < 0)
        return is_pilot;
    ret = ssi_check_wcpu_is_working();
    if(ret < 0)
        return ret;
    if(ret)
    {
        ssi_read_wpcu_pc_lr_sp(trace_en);
    }
    bfgx_print_subsys_state();
    ret = ssi_check_bcpu_is_working();
    if(ret < 0)
        return ret;
    if(ret)
    {
        ssi_read_bpcu_pc_lr_sp(trace_en);
    }

    return 0;
}

int32 ssi_tcxo_mux(uint32 flag)
{
    int ret;
    int32 is_pilot;

    if((0 == g_board_info_etc.ssi_gpio_clk) || (0 == g_board_info_etc.ssi_gpio_data))
    {
        PS_PRINT_ERR("reset aon, gpio ssi don't support\n");
        return -1;
    }

    ret = ssi_request_gpio_etc(g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data);
    if(ret)
    {
        PS_PRINT_ERR("ssi_force_reset_aon request failed:%d, data:%d, ret=%d\n",
                    g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data, ret);
        return ret;
    }

    PS_PRINT_INFO("SSI start set\n");

    ssi_write16_etc(0x8007, 0x1);

    is_pilot = ssi_check_is_pilot();
    if(1 != is_pilot)
    {
        PS_PRINT_INFO("not pilot chip, return\n");
        ssi_write16_etc(0x8007, 0x0);
        ssi_free_gpio_etc();
        return 0;
    }

    if (1 == flag)
    {
        ssi_write16_etc(0x8009, 0x0);
        ssi_write16_etc(0x8008, 0x60);
        ssi_write16_etc(0x8009, 0x60);
        ssi_write32_etc(0x50003338, 0x100);
        PS_PRINT_INFO("SSI set 0x50003338 to 0x100\n");
    }
    else
    {
        ssi_write16_etc(0x8008, 0x0);
    }

    ssi_write16_etc(0x8007, 0x0);

    PS_PRINT_INFO("SSI set OK\n");

    ssi_free_gpio_etc();

    return 0;
}

int ssi_read_reg_info(ssi_reg_info* pst_reg_info,
                                void* buf , int32 buf_len,
                                oal_int32 is_logfile)
{
    int ret;
    int step = 4;
    oal_int32 is_atomic = 0;
    uint32 reg;
    uint16 reg16;
    uint32 ssi_address;
    uint32 realloc = 0;
    mm_segment_t fs;
    uint16 last_high_addr;
    int i,j,k,seg_size,seg_nums, left_size;

    struct timeval tv;
    struct rtc_time tm;
    OS_KERNEL_FILE_STRU *fp = NULL;
    char filename[200] = {0};

    fs = get_fs();

    if(oal_in_interrupt() || oal_in_atomic() || irqs_disabled())
    {
        is_logfile = 0;
        is_atomic = 1;
    }

    if(is_logfile)
    {
        do_gettimeofday(&tv);
        rtc_time_to_tm(tv.tv_sec, &tm);
        snprintf(filename, sizeof(filename) - 1, "%s/gpio_ssi_%08x_%08x_%04d%02d%02d%02d%02d%02d.bin",
                                                 str_gpio_ssi_dump_path,
                                                 pst_reg_info->base_addr,
                                                 pst_reg_info->base_addr + pst_reg_info->len -1,
                                                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                                 tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    ret = ssi_check_device_isalive();
    if(ret)
    {
        PS_PRINT_INFO("gpio-ssi maybe dead before read 0x%x:%u\n", pst_reg_info->base_addr, pst_reg_info->len);
        return -OAL_EFAIL;
    }

    if(NULL == buf)
    {
        if(is_atomic)
        {
            buf = kmalloc(pst_reg_info->len, GFP_ATOMIC);
        }
        else
        {
            buf = OS_VMALLOC_GFP(pst_reg_info->len);
        }

        if(NULL == buf)
        {
            PS_PRINT_INFO("alloc mem failed before read 0x%x:%u\n", pst_reg_info->base_addr, pst_reg_info->len);
            return -OAL_ENOMEM;
        }
        buf_len = pst_reg_info->len;
        realloc = 1;
    }

    PS_PRINT_INFO("dump reg info 0x%x:%u, buf len:%u \n", pst_reg_info->base_addr, pst_reg_info->len, buf_len);

    if(is_logfile)
    {
        fp = filp_open(filename, O_RDWR | O_CREAT, 0644);
        if (OAL_IS_ERR_OR_NULL(fp))
        {
            PS_PRINT_INFO("open file %s failed ret=%ld\n", filename, PTR_ERR(fp));
            is_logfile = 0;
        }
        else
        {
            PS_PRINT_INFO("open file %s succ\n", filename);
            fs = get_fs();
            set_fs(KERNEL_DS);
            vfs_llseek(fp, 0, SEEK_SET);
        }
    }

    last_high_addr = 0x0;
    ssi_clear_ahb_highaddr();

    if(SSI_RW_DWORD_MOD == pst_reg_info->rw_mod)
    {
        /*switch 32bits mode*/
        ssi_switch_ahb_mode(1);
    }
    else
    {
        ssi_switch_ahb_mode(0);
    }

retry:

    seg_nums = (pst_reg_info->len - 1 / buf_len) + 1;
    left_size = pst_reg_info->len;

    for(i = 0; i < seg_nums; i++)
    {
        seg_size = OAL_MIN(left_size, buf_len);
        for(j = 0; j < seg_size; j += step)
        {
            ssi_address = pst_reg_info->base_addr + i*buf_len + j;

            for(k = 0; k < 3; k++)
            {
                reg = 0x0;
                reg16 = 0x0;
                if(SSI_RW_DWORD_MOD == pst_reg_info->rw_mod)
                {
                    ret = ssi_read_value32(ssi_address, &reg, last_high_addr);
                }
                else
                {
                    ret = ssi_read_value16(ssi_address, &reg16, last_high_addr);
                    reg = reg16;
                }

                if(0 == ret)
                    break;
            }
            if(3 == k)
            {
                PS_PRINT_ERR("ssi read address 0x%x failed, retry %d times", ssi_address, k);
                goto fail_read;
            }
            last_high_addr = (ssi_address >> 16);
            oal_writel(reg, buf + j);
        }

        left_size -= seg_size;

        if(is_logfile)
        {
            ret = vfs_write(fp, buf, seg_size, &fp->f_pos);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
            vfs_fsync(fp, 0);
#else
            vfs_fsync(fp, fp->f_path.dentry, 0);
#endif
            if(ret != seg_size)
            {
                PS_PRINT_ERR("ssi print file failed, request %d, write %d actual\n", seg_size, ret);
                is_logfile = 0;
                set_fs(fs);
                filp_close(fp, NULL);
                goto retry;
            }
        }
        else
        {
#ifdef CONFIG_PRINTK
            /*print to kenrel msg*/
            print_hex_dump(KERN_DEBUG, "gpio-ssi: ", DUMP_PREFIX_OFFSET, 32, 4,
               buf, seg_size, false);
#endif
        }
    }

    if(is_logfile)
    {
        set_fs(fs);
        filp_close(fp, NULL);
    }

    if(realloc)
    {
        if(is_atomic)
        {
            kfree(buf);
        }
        else
        {
            OS_MEM_VFREE(buf);
        }
    }

    if(SSI_RW_DWORD_MOD == pst_reg_info->rw_mod)
    {
        /*switch 16bits mode*/
        ssi_switch_ahb_mode(0);
    }

    return 0;
fail_read:
    if(ssi_address != pst_reg_info->base_addr)
    {
        if(ssi_address > pst_reg_info->base_addr)
        {
#ifdef CONFIG_PRINTK
            /*print the read buf before errors*/
            print_hex_dump(KERN_DEBUG, "gpio-ssi: ", DUMP_PREFIX_OFFSET, 32, 4,
               buf, OAL_MIN(buf_len, ssi_address - pst_reg_info->base_addr), false);
#endif
        }
    }

    if(is_logfile)
    {
        set_fs(fs);
        filp_close(fp, NULL);
    }

    if(realloc)
    {
        if(is_atomic)
        {
            kfree(buf);
        }
        else
        {
            OS_MEM_VFREE(buf);
        }
    }

    if(SSI_RW_DWORD_MOD == pst_reg_info->rw_mod)
    {
        /*switch 16bits mode*/
        ssi_switch_ahb_mode(0);
    }
    return ret;
}

int ssi_read_reg_info_test(uint32 base_addr, uint32 len, uint32 is_logfile, uint32 rw_mode)
{
    int ret;
    ssi_reg_info reg_info;

    struct st_exception_info *pst_exception_data = NULL;
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -OAL_EBUSY;
    }
    if((!ssi_dfr_bypass) &&
            ( oal_work_is_busy(&pst_exception_data->wifi_excp_worker)
               || oal_work_is_busy(&pst_exception_data->wifi_excp_recovery_worker)
               || (PLAT_EXCEPTION_RESET_IDLE != atomic_read(&pst_exception_data->is_reseting_device))))
    {
        PS_PRINT_ERR("dfr is doing ,not do ssi read\n");
        return -OAL_EBUSY;
    }

    OAL_MEMZERO(&reg_info, sizeof(reg_info));

    reg_info.base_addr = base_addr;
    reg_info.len = len;
    reg_info.rw_mod = rw_mode;

    if((0 == g_board_info_etc.ssi_gpio_clk) || (0 == g_board_info_etc.ssi_gpio_data))
    {
        PS_PRINT_INFO("gpio ssi don't support, check dts\n");
        return -1;
    }

    /*get ssi lock*/
    if(ssi_try_lock())
    {
        PS_PRINT_INFO("ssi is locked, request return\n");
        return -OAL_EFAIL;
    }

    ret = ssi_request_gpio_etc(g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data);
    if(ret)
    {
        ssi_unlock();
        return ret;
    }

    ssi_read16_etc(0x8009);
    ssi_read16_etc(0x8008);

    ssi_write16_etc(0x8007, 0x1);/*switch to ssi clk, wcpu hold*/
    PS_PRINT_INFO("switch ssi clk %s", (ssi_read16_etc(0x8007) == 0x1) ? "ok":"failed");

    ret = ssi_read_device_arm_register(1);
    if(ret)
    {
        goto ssi_fail;
    }

    PS_PRINT_INFO("ssi is ok, glb_ctrl is ready\n");

    ret = ssi_check_device_isalive();
    if(ret)
    {
        goto ssi_fail;
    }

    ret = ssi_read_reg_info(&reg_info, NULL, 0, is_logfile);
    if(ret)
    {
        goto ssi_fail;
    }

    ssi_write16_etc(0x8007, 0x0);/*switch from ssi clk, wcpu continue*/

    ssi_free_gpio_etc();
    ssi_unlock();

    return 0;
ssi_fail:
    ssi_write16_etc(0x8007, 0x0);/*switch from ssi clk, wcpu continue*/
    ssi_free_gpio_etc();
    ssi_unlock();
    return ret;
}

int ssi_read_reg_info_arry(ssi_reg_info** pst_reg_info, oal_uint32 reg_nums, oal_int32 is_logfile)
{
    int ret;
    int i;

    if(OAL_UNLIKELY(NULL == pst_reg_info))
    {
        return -OAL_EFAIL;
    }

    for(i = 0; i < reg_nums; i++)
    {
        ret = ssi_read_reg_info(pst_reg_info[i], NULL, 0, is_logfile);
        if(ret)
        {
            return ret;
        }
    }

    return 0;
}

int ssi_force_reset_aon(void)
{
    int ret;
    /*request  ssi's gpio */

    if((0 == g_board_info_etc.ssi_gpio_clk) || (0 == g_board_info_etc.ssi_gpio_data))
    {
        PS_PRINT_INFO("reset aon, gpio ssi don't support\n");
        return -1;
    }

    ret = ssi_request_gpio_etc(g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data);
    if(ret)
    {
        PS_PRINT_INFO("ssi_force_reset_aon request failed:%d, data:%d, ret=%d\n",
                    g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data, ret);
        return ret;
    }

    /*try to reset aon*/
    ssi_write16_etc(0x8009, 0x60);
    ssi_write16_etc(0x8008, 0x60);

    PS_PRINT_INFO("ssi_force_reset_aon");

    ssi_free_gpio_etc();

    return 0;
}

int ssi_set_gpio_pins(int32 clk, int32 data)
{
    g_board_info_etc.ssi_gpio_clk = clk;
    g_board_info_etc.ssi_gpio_data = data;
    PS_PRINT_INFO("set ssi gpio clk:%d , gpio data:%d\n", clk, data);
    return 0;
}
EXPORT_SYMBOL_GPL(ssi_set_gpio_pins);

/*Try to dump all reg,
  ssi used to debug, we should*/
int ssi_dump_device_regs(unsigned long long module_set)
{
    int ret;
    struct st_exception_info *pst_exception_data = NULL;

    if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
    {
        /*user build, limit the ssi dump*/
        if(!oal_print_rate_limit(30*PRINT_RATE_SECOND))
        {
            /*print limit*/
            module_set = 0;
            PS_PRINT_ERR("ssi dump print limit\n");
        }
    }

    if(0 == module_set)
    {
        PS_PRINT_ERR("ssi dump regs bypass\n");
        return OAL_SUCC;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -OAL_EBUSY;
    }
    if((!ssi_dfr_bypass) &&
            ( oal_work_is_busy(&pst_exception_data->wifi_excp_worker)
               || oal_work_is_busy(&pst_exception_data->wifi_excp_recovery_worker)
               || (PLAT_EXCEPTION_RESET_IDLE != atomic_read(&pst_exception_data->is_reseting_device))))
    {
        PS_PRINT_ERR("dfr is doing ,not do ssi read\n");
        return -OAL_EBUSY;
    }
    if((0 == g_board_info_etc.ssi_gpio_clk) || (0 == g_board_info_etc.ssi_gpio_data))
    {
        PS_PRINT_ERR("gpio ssi don't support, check dts\n");
        return -1;
    }

    /*get ssi lock*/
    if(ssi_try_lock())
    {
        PS_PRINT_INFO("ssi is locked, request return\n");
        return -OAL_EBUSY;
    }

    DECLARE_DFT_TRACE_KEY_INFO("ssi_dump_device_regs", OAL_DFT_TRACE_FAIL);

    if(module_set & SSI_MODULE_MASK_AON_CUT)
    {
        module_set &=~SSI_MODULE_MASK_AON;
    }

    if(module_set & SSI_MODULE_MASK_PCIE_CUT)
    {
        module_set &=~(SSI_MODULE_MASK_PCIE_CFG|SSI_MODULE_MASK_PCIE_DBI);
    }

    ret = ssi_request_gpio_etc(g_board_info_etc.ssi_gpio_clk, g_board_info_etc.ssi_gpio_data);
    if(ret)
    {
        ssi_unlock();
        return ret;
    }

    ssi_read16_etc(0x8009);
    ssi_read16_etc(0x8008);

    ssi_write16_etc(0x8007, 0x1);/*switch to ssi clk, wcpu hold*/

    PS_PRINT_INFO("switch ssi clk %s", (ssi_read16_etc(0x8007) == 0x1) ? "ok":"failed");

    ret = ssi_check_device_isalive();
    if(ret)
    {
        /*try to reset aon*/
        ssi_write16_etc(0x8008, 0x60);
        ssi_write16_etc(0x8009, 0x60);
        ssi_read16_etc(0x8009);
        ssi_read16_etc(0x8008);
        ssi_write16_etc(0x8007, 0x1);
        if(ssi_check_device_isalive())
        {
            PS_PRINT_INFO("after reset aon, ssi still can't work\n");
            goto ssi_fail;
        }
        else
        {
            PS_PRINT_INFO("after reset aon, ssi ok, dump acp/ocp reg\n");
            //ssi_read_reg_info(&hi1103_pmu_cmu_ctrl_full, NULL, 0 , ssi_is_logfile);
            ssi_check_buck_scp_ocp_status();
            //goto ssi_fail;
            module_set = SSI_MODULE_MASK_COMM;
        }
    }

    if(module_set & SSI_MODULE_MASK_ARM_REG)
    {
        ret = ssi_read_device_arm_register(0);
        if(ret)
        {
            goto ssi_fail;
        }
    }

    ret = ssi_check_device_isalive();
    if(ret)
    {
        goto ssi_fail;
    }

    if(module_set & SSI_MODULE_MASK_AON)
    {
        ret = ssi_read_reg_info_arry(hi1103_aon_reg_full, sizeof(hi1103_aon_reg_full)/sizeof(ssi_reg_info*), ssi_is_logfile);
        if(ret)
        {
            goto ssi_fail;
        }
    }

    if(module_set & SSI_MODULE_MASK_AON_CUT)
    {
        ret = ssi_read_reg_info_arry(hi1103_aon_reg_cut, sizeof(hi1103_aon_reg_cut)/sizeof(ssi_reg_info*), ssi_is_logfile);
        if(ret)
        {
            goto ssi_fail;
        }
    }

    if(module_set & SSI_MODULE_MASK_ARM_REG)
    {
        ret = ssi_read_device_arm_register(1);
        if(ret)
        {
            goto ssi_fail;
        }
    }

    /*Debug*/
    if(module_set & SSI_MODULE_MASK_WCPU_KEY_DTCM)
    {
        if(ssi_check_wcpu_is_working())
        {
            ret = ssi_read_reg_info(&hi1103_w_key_mem, NULL, 0 , ssi_is_logfile);
            if(ret)
            {
                PS_PRINT_INFO("wcpu key mem read failed, continue try aon\n");
            }
        }
        else
        {
            PS_PRINT_INFO("wctrl can't dump, wcpu down\n");
        }
    }


    if(module_set & SSI_MODULE_MASK_WCTRL)
    {
        if(ssi_check_wcpu_is_working())
        {
            ret = ssi_read_reg_info(&hi1103_w_ctrl_full, NULL, 0 , ssi_is_logfile);
            if(ret)
            {
                goto ssi_fail;
            }
        }
        else
        {
            PS_PRINT_INFO("wctrl can't dump, wcpu down\n");
        }
    }

    if(module_set & SSI_MODULE_MASK_PCIE_CFG)
    {
        if(ssi_check_wcpu_is_working())
        {
            ret = ssi_read_reg_info_arry(hi1103_pcie_cfg_reg_full, sizeof(hi1103_pcie_cfg_reg_full)/sizeof(ssi_reg_info*), ssi_is_logfile);
            if(ret)
                goto ssi_fail;
        }
        else
        {
            PS_PRINT_INFO("pcie cfg can't dump, wcpu down\n");
        }
    }

    if(module_set & SSI_MODULE_MASK_PCIE_CUT)
    {
        if(ssi_check_wcpu_is_working())
        {
            ret = ssi_read_reg_info_arry(hi1103_pcie_cfg_reg_cut, sizeof(hi1103_pcie_cfg_reg_cut)/sizeof(ssi_reg_info*), ssi_is_logfile);
            if(ret)
                goto ssi_fail;
        }
        else
        {
            PS_PRINT_INFO("pcie cfg cut can't dump, wcpu down\n");
        }
    }

    if(module_set & SSI_MODULE_MASK_PCIE_DBI)
    {
        if(ssi_check_wcpu_is_working())
        {
            oal_uint32 reg_nums;
            ssi_reg_info **pst_pcie_dbi_reg;
            if(1 == ssi_check_is_pilot())
            {
                reg_nums = sizeof(hi1103_pcie_dbi_pilot_reg_full)/sizeof(ssi_reg_info*);
                pst_pcie_dbi_reg = hi1103_pcie_dbi_pilot_reg_full;
            }
            else
            {
                reg_nums = sizeof(hi1103_pcie_dbi_mpw2_reg_full)/sizeof(ssi_reg_info*);
                pst_pcie_dbi_reg = hi1103_pcie_dbi_mpw2_reg_full;
            }

            ret = ssi_read_reg_info_arry(pst_pcie_dbi_reg, reg_nums, ssi_is_logfile);
            if(ret)
                goto ssi_fail;
        }
        else
        {
            PS_PRINT_INFO("pcie dbi can't dump, wcpu down\n");
        }
    }

    if(module_set & SSI_MODULE_MASK_SDIO)
    {
        if(ssi_check_wcpu_is_working())
        {
            ret = ssi_read_reg_info(&hi1103_pcie_sdio_ctrl_full, NULL, 0, ssi_is_logfile);
            if(ret)
                goto ssi_fail;
        }
        else
        {
            PS_PRINT_INFO("sdio can't dump, wcpu down\n");
        }
    }

    if(module_set & SSI_MODULE_MASK_BCTRL)
    {
        if(ssi_check_bcpu_is_working())
        {
            ret = ssi_read_reg_info(&hi1103_b_ctrl_full, NULL, 0 , ssi_is_logfile);
            if(ret)
            {
                goto ssi_fail;
            }
        }
    }

    ssi_write16_etc(0x8007, 0x0);/*switch from ssi clk, wcpu continue*/

    ssi_free_gpio_etc();
    ssi_unlock();

    return 0;
ssi_fail:
    ssi_write16_etc(0x8007, 0x0);/*switch from ssi clk, wcpu continue*/

    ssi_free_gpio_etc();
    ssi_unlock();
    return ret;
}
#endif

/*********************************************************************/
/********************   SSI调试代码end    ****************************/
/*********************************************************************/

#ifdef _PRE_CONFIG_USE_DTS
static struct of_device_id hi110x_board_match_table[] = {
    {
        .compatible = DTS_COMP_HI110X_BOARD_NAME,
        .data = NULL,
    },
    {
        .compatible = DTS_COMP_HISI_HI110X_BOARD_NAME,
        .data = NULL,
    },
    { },
};
#endif

STATIC struct platform_driver hi110x_board_driver = {
        .probe      = hi110x_board_probe,
        .remove     = hi110x_board_remove,
        .suspend    = hi110x_board_suspend_etc,
        .resume     = hi110x_board_resume_etc,
        .driver     = {
            .name   = "hi110x_board",
            .owner  = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
			.of_match_table	= hi110x_board_match_table,
#endif
        },
};

int32 hi110x_board_init_etc(void)
{
    int32 ret = BOARD_FAIL;

    board_probe_ret = BOARD_FAIL;
    init_completion(&board_driver_complete);

#ifdef ANDROID_HI1XX_BUILD_VERSION
    hi11xx_android_variant = ANDROID_HI1XX_BUILD_VERSION;
    PS_PRINT_INFO("hi11xx_android_variant=%d\n", hi11xx_android_variant);
#endif

    ret = platform_driver_register(&hi110x_board_driver);
    if (ret)
    {
        PS_PRINT_ERR("Unable to register hisi connectivity board driver.\n");
        return ret;
    }

    if(wait_for_completion_timeout(&board_driver_complete, 10*HZ))
    {
        /*completed*/
        if(BOARD_SUCC != board_probe_ret)
        {
            PS_PRINT_ERR("hi110x_board probe failed=%d\n", board_probe_ret);
            return board_probe_ret;
        }
    }
    else
    {
        /*timeout*/
        PS_PRINT_ERR("hi110x_board probe timeout\n");
        return BOARD_FAIL;
    }

    PS_PRINT_INFO("hi110x_board probe succ\n");

    return ret;
}

void hi110x_board_exit_etc(void)
{
    platform_driver_unregister(&hi110x_board_driver);
}


oal_uint8 g_bus_type = HCC_BUS_SDIO;
oal_int32 g_wifi_plat_dev_probe_state;

extern int create_hwconn_proc_file(void);
static int hisi_wifi_plat_dev_drv_probe(struct platform_device *pdev)
{
   int ret = 0;
    /*TBD:bus type should be defined in dts and read during probe*/
   if(HCC_BUS_SDIO == g_bus_type)
   {
        ret = oal_wifi_platform_load_sdio();
        if(ret)
        {
            printk(KERN_ERR "[HW_CONN] oal_wifi_platform_load_sdio failed.\n");
            g_wifi_plat_dev_probe_state = -OAL_FAIL;
            return ret;
        }
   }

#ifdef CONFIG_HWCONNECTIVITY
  ret = create_hwconn_proc_file();
  if (ret)
  {
      printk(KERN_ERR "[HW_CONN] create proc file failed.\n");
      g_wifi_plat_dev_probe_state = -OAL_FAIL;
      return ret;
  }
#endif
  return ret;
}

static int hisi_wifi_plat_dev_drv_remove(struct platform_device *pdev)
{
    printk(KERN_ERR "[HW_CONN] hisi_wifi_plat_dev_drv_remove.\n");
    return  OAL_SUCC;
}


#ifdef _PRE_CONFIG_USE_DTS
static const struct of_device_id hisi_wifi_match_table[] = {
    {
        .compatible = DTS_NODE_HI110X_WIFI,   // compatible must match with which defined in dts
        .data = NULL,
    },
    {},
};
#endif

static struct platform_driver hisi_wifi_platform_dev_driver = {
    .probe          = hisi_wifi_plat_dev_drv_probe,
    .remove         = hisi_wifi_plat_dev_drv_remove,
    .suspend        = NULL,
    .shutdown       = NULL,
    .driver = {
        .name = DTS_NODE_HI110X_WIFI,
        .owner = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = hisi_wifi_match_table, // dts required code
#endif
    },
};

int32 hi11xx_get_android_build_variant(void)
{
    return hi11xx_android_variant;
}

int32 hisi_wifi_platform_register_drv(void)
{
    int32 ret = BOARD_FAIL;
    PS_PRINT_FUNCTION_NAME;

    g_wifi_plat_dev_probe_state = OAL_SUCC;

    ret = platform_driver_register(&hisi_wifi_platform_dev_driver);
    if (ret)
    {
        PS_PRINT_ERR("Unable to register hisi wifi driver.\n");
    }
    /*platform_driver_register return always true*/
    return g_wifi_plat_dev_probe_state;
}

void hisi_wifi_platform_unregister_drv(void)
{
    PS_PRINT_FUNCTION_NAME;

    platform_driver_unregister(&hisi_wifi_platform_dev_driver);

    return;
}


