
#ifdef __cplusplus
    #if __cplusplus
            extern "C" {
    #endif
#endif

#include "board.h"


#ifndef __BOARD_HI1102_H__
#define __BOARD_HI1102_H__
/*****************************************************************************
  1 Include other Head file
*****************************************************************************/

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define BOARD_VERSION_NAME_HI1102           "hi1102"

#define DTS_NODE_HI110X                     "hisilicon,hi1102"
#define DTS_COMP_HI110X_BOARD_NAME          DTS_NODE_HI110X
#define DTS_PROP_GPIO_POWEN_ON              "hi1102,gpio_power_on"
#define DTS_PROP_GPIO_WLAN_WAKEUP_HOST      "hi1102,gpio_wlan_wakeup_host"
#define DTS_PROP_GPIO_BFGN_WAKEUP_HOST      "hi1102,gpio_bfgn_wakeup_host"
#define DTS_PROP_IR_LDO_TYPE                "hi1102,irled_power_type"
#define DTS_IRLED_LDO_POWER                 "hi1102,irled_power"
#define DTS_IRLED_VOLTAGE                   "hi1102,irled_voltage"
#define DTS_PROP_GPIO_BFGN_IR_CTRL          "hi1102,gpio_bfgn_ir_ctrl"
#define DTS_PROP_GPIO_XLDO_PINMUX           "hi1102,gpio_xldo_pinmux"
#define DTS_PROP_UART_POART                 "hi1102,uart_port"
#define DTS_PROP_UART_PCLK                  "hi1102,uart_pclk_normal"
#define DTS_PROP_VERSION                    "hi1102,asic_version"
#define DTS_PROP_POWER_PREPARE              "hi1102,power_prepare"
#define PROC_NAME_GPIO_XLDO_PINMUX          "hi1102_xldo_pinmux"
#define PROC_NAME_GPIO_BFGN_WAKEUP_HOST     "bfgn_wake_host"
#define PROC_NAME_GPIO_BFGN_IR_CTRL         "bfgn_ir_ctrl"

/*hisi_cust_cfg*/
#define CUST_COMP_NODE                      "hi1102,customize"



/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern int32 hi1102_get_board_power_gpio(void);
extern void hi1102_free_board_power_gpio_etc(void);
extern int32 hi1102_board_wakeup_gpio_init_etc(void);
extern void hi1102_free_board_wakeup_gpio_etc(void);
extern int32 hi1102_bfgx_dev_power_on(void);
extern int32 hi1102_bfgx_dev_power_off(void);
extern int32 hi1102_wlan_power_off(void);
extern int32 hi1102_wlan_power_on(void);
extern int32 hi1102_board_power_on(uint32 ul_subsystem);
extern int32 hi1102_board_power_off(uint32 ul_subsystem);
extern int32 hi1102_board_power_reset(uint32 ul_subsystem);
extern int32 hi1102_get_board_pmu_clk32k(void);
extern int32 hi1102_get_board_uart_port(void);
extern int32 hi1102_board_ir_ctrl_init(struct platform_device *pdev);
extern int32 hi1102_check_evb_or_fpga(void);
extern int32 hi1102_board_get_power_pinctrl(struct platform_device *pdev);
extern int32 hi1102_get_ini_file_name_from_dts(int8 *dts_prop, int8 *prop_value, uint32 size);
#endif

#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif



