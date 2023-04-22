

#ifndef __BOARD_H__
#define __BOARD_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/

//#include "hw_bfg_ps.h"
#include "plat_type.h"

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

/*****************************************************************************
  2 Define macro
*****************************************************************************/

#define BOARD_SUCC                           (0)
#define BOARD_FAIL                           (-1)

#define GPIO_LOWLEVEL                        (0)
#define GPIO_HIGHLEVEL                       (1)

#define NO_NEED_POWER_PREPARE                (0)
#define NEED_POWER_PREPARE                   (1)

#define PINMUX_SET_INIT                      (0)
#define PINMUX_SET_SUCC                      (1)

#define DTS_NODE_HI110X                     "hisilicon,hi1102"
#define DTS_NODE_HI110X_BFGX                "hisilicon,hisi_bfgx"
#define DTS_NODE_HI110X_WIFI                "hisilicon,hisi_wifi"

#define DTS_COMP_HI110X_BOARD_NAME          DTS_NODE_HI110X

#define DTS_PROP_GPIO_POWEN_ON              "hi1102,gpio_power_on"
#define DTS_PROP_GPIO_WLAN_WAKEUP_HOST      "hi1102,gpio_wlan_wakeup_host"
#define DTS_PROP_GPIO_BFGN_WAKEUP_HOST      "hi1102,gpio_bfgn_wakeup_host"
#define DTS_PROP_GPIO_BFGN_IR_CTRL          "hi1102,gpio_bfgn_ir_ctrl"
#define DTS_PROP_IR_LDO_TYPE                "hi1102,irled_power_type"
#define DTS_IRLED_LDO_POWER                 "hi1102,irled_power"
#define DTS_IRLED_VOLTAGE                   "hi1102,irled_voltage"
#define DTS_PROP_GPIO_XLDO_PINMUX           "hi1102,gpio_xldo_pinmux"
#define DTS_PROP_UART_POART                 "hi1102,uart_port"
#define DTS_PROP_UART_PCLK                  "hi1102,uart_pclk_normal"
#define DTS_PROP_CLK_32K                    "huawei,pmu_clk32b"
#define DTS_PROP_VERSION                    "hi1102,asic_version"
#define DTS_PROP_POWER_PREPARE              "hi1102,power_prepare"

#define PROC_NAME_GPIO_POWEN_ON             "power_on_enable"
#define PROC_NAME_GPIO_WLAN_WAKEUP_HOST     "wlan_wake_host"
#define PROC_NAME_GPIO_BFGN_WAKEUP_HOST     "bfgn_wake_host"
#define PROC_NAME_GPIO_BFGN_IR_CTRL         "bfgn_ir_ctrl"
#define PROC_NAME_GPIO_XLDO_PINMUX          "hi1102_xldo_pinmux"

#define DTS_PROP_GPIO_XLDO_LEVEL            "xldo_gpio_level"

/*1103 product*/
#define DTS_PROP_SUBCHIP_TYPE_VERSION       "hi110x,subchip_type"
#define DTS_PROP_GPIO_HOST_WAKEUP_WLAN      "hi110x,gpio_host_wakeup_wlan"
#define DTS_PROP_GPIO_WLAN_POWEN_ON_ENABLE  "hi110x,gpio_wlan_power_on"
#define DTS_PROP_GPIO_BFGX_POWEN_ON_ENABLE  "hi110x,gpio_bfgx_power_on"

#define PROC_NAME_GPIO_WLAN_POWEN_ON        "wlan_power_on_enable"
#define PROC_NAME_GPIO_BFGX_POWEN_ON        "bfgx_power_on_enable"
#define PROC_NAME_GPIO_HOST_WAKEUP_WLAN     "host_wakeup_wlan"

#define INI_WLAN_DOWNLOAD_CHANNEL           "board_info.wlan_download_channel"
#define INI_BFGX_DOWNLOAD_CHANNEL           "board_info.bfgx_download_channel"

#define DOWNlOAD_MODE_SDIO                  "sdio"
#define DOWNlOAD_MODE_PCIE                  "pcie"
#define DOWNlOAD_MODE_UART                  "uart"

#define BOARD_VERSION_LEN                   (128)
#define DOWNLOAD_CHANNEL_LEN                (64)

/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

/*private data for pm driver*/
typedef struct {

    /*power*/
    int32 power_on_enable;                  /*1102 product*/
    int32 bfgn_power_on_enable;             /*1103 product*/
    int32 wlan_power_on_enbale;             /*1103 product*/

    /*wakeup gpio*/
    int32 wlan_wakeup_host;
    int32 bfgn_wakeup_host;
    int32 host_wakeup_wlan;                 /*1103 product*/

    /*device hisi board verision*/
    const char * chip_type;

    /*how to download firmware*/
    int32 wlan_download_channel;
    int32 bfgn_download_channel;

    bool  have_ir;
    int32 irled_power_type;
    int32 bfgn_ir_ctrl_gpio;
    struct regulator *bfgn_ir_ctrl_ldo;

    int32 xldo_pinmux;

    /* hi110x irq info */
    uint32 wlan_irq;
    uint32 bfgn_irq;

    /* hi110x uart info */
    const char * uart_port;
    int32 uart_pclk;

    /* hi110x clk info */
    const char * clk_32k_name;
    struct clk* clk_32k;

    /* evb or fpga verison */
    int32 is_asic;

    /* prepare before board power on */
    int32 need_power_prepare;
    int32 pinmux_set_result;
    int32 gpio_xldo_level;
    struct pinctrl *pctrl;
    struct pinctrl_state *pins_normal;
    struct pinctrl_state *pins_idle;
}BOARD_INFO;

typedef struct _device_vesion_board
{
    uint32 index;
    const char name[BOARD_VERSION_LEN + 1];
}DEVICE_BOARD_VERSION;

typedef struct _download_mode
{
    uint32 index;
    uint8 name[DOWNLOAD_CHANNEL_LEN + 1];
}DOWNLOAD_MODE;

enum hisi_device_board
{
    BOARD_VERSION_HI1102 = 0x0,
    BOARD_VERSION_HI1103 = 0x1,
    BOARD_VERSION_BOTT,
};

enum hisi_download_firmware_mode
{
    MODE_SDIO          = 0x0,
    MODE_PCIE          = 0x1,
    MODE_UART          = 0x2,
    MODE_DOWNLOAD_BUTT,
};
enum hisi_board_power
{
    WLAN_POWER         = 0x0,
    BFGX_POWER         = 0x1,
    POWER_BUTT,
};
enum board_power_state
{
    BOARD_POWER_ON     = 0x0,
    BOARD_POWER_OFF    = 0x1,
};
enum board_irled_power_type
{
    IR_GPIO_CTRL       = 0x0,
    IR_LDO_CTRL        = 0x1,
};


extern uint32 g_device_subchip_type;
extern DOWNLOAD_MODE device_download_mode_list[MODE_DOWNLOAD_BUTT];
extern BOARD_INFO g_board_info;


/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern BOARD_INFO * get_hi110x_board_info(void);
extern int32 get_uart_pclk_source(void);
extern int32 get_device_board_version(void);
extern int32 hi110x_board_init(void);
extern void hi110x_board_exit(void);
extern void board_power_on(uint32 subsystem);
extern void board_power_off(uint32 subsystem);
extern int board_get_bwkup_gpio_val(void);
extern int board_get_wlan_wkup_gpio_val(void);
extern int32 check_device_board_name(void);
extern int32 get_board_gpio(const char * gpio_node, const char * gpio_prop, int32 *physical_gpio);
extern int32 get_board_dts_node(struct device_node ** np, const char * node_prop);

#endif

