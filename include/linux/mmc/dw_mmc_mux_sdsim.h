#ifndef _DW_MMC_MUX_SD_SIM_
#define _DW_MMC_MUX_SD_SIM_

#include <linux/semaphore.h>


#define MODULE_SD  0
#define MODULE_SIM  1

#define GPIO_VALUE_LOW     0
#define GPIO_VALUE_HIGH    1

#define MUX_SDSIM_LOG_TAG "[MUX_SDSIM][mmc1][hi_mci.1]"


enum sdsim_gpio_mode {
	SDSIM_MODE_GPIO_DETECT = 0, /*gpio detect mode for detect sd or sim*/
	SDSIM_MODE_SD_NORMAL = 1, /*sd normal mode*/
	SDSIM_MODE_SD_IDLE = 2, /*sd idle/lowpower mode*/
	SDSIM_MODE_SIM_NORMAL = 3, /*sim normal mode*/
	SDSIM_MODE_SIM_IDLE = 4, /*sim idle/lowpower mode*/
};

/*hisilicon iomux xml and pinctrl framework can't support such SD-SIM-IO-MUX case,wo need config different five modes here manully in code*/
int config_sdsim_gpio_mode(enum sdsim_gpio_mode gpio_mode);

char* detect_status_to_string(void);

/*
status=1 means plug out;
status=0 means plug in;
*/
#define STATUS_PLUG_IN 0
#define STATUS_PLUG_OUT 1

/*
Description: while sd/sim plug in or out, gpio_cd detect pin is actived,we need call this sd_sim_detect_run function to make sure sd or sim which is inserted

dw_mci_host: MODULE_SD use dw_mci_host argu as input, while MODULE_SIM just use NULL
status: use STATUS_PLUG_IN or STATUS_PLUG_OUT by gpio_cd detect pin's value
current_module: sd or sim which module is calling this function

return value:return STATUS_PLUG_IN or STATUS_PLUG_OUT,just tell current_module sd or sim is inserted or not, and current_module can update gpio_cd detect pin value by this return value
*/
int sd_sim_detect_run(void *dw_mci_host, int status, int current_module, int need_sleep);

extern struct semaphore sem_mux_sdsim_detect;
extern struct dw_mci *host_from_sd_module;


#define SD_SIM_DETECT_STATUS_UNDETECTED            0
#define SD_SIM_DETECT_STATUS_SD                    1
#define SD_SIM_DETECT_STATUS_SIM                   2
#define SD_SIM_DETECT_STATUS_ERROR                 3

extern int sd_sim_detect_status_current;

#define SLEEP_MS_TIME_FOR_DETECT_UNSTABLE   40
/*if define this macro,4-pin gpio status detect is not used,only by cmd1 response to judge sd or sim*/
#undef SDSIM_MUX_DETECT_SOLUTION_CMD1_ONLY

#define DRIVER_STRENGTH_2MA_0  0x00
#define DRIVER_STRENGTH_4MA_0  0x10
#define DRIVER_STRENGTH_6MA_0  0x20
#define DRIVER_STRENGTH_8MA_0  0x30
#define DRIVER_STRENGTH_10MA_0  0x40
#define DRIVER_STRENGTH_12MA_0  0x50
#define DRIVER_STRENGTH_14MA_0  0x60
#define DRIVER_STRENGTH_16MA_0  0x70
#define DRIVER_STRENGTH_5MA_1 0x00
#define DRIVER_STRENGTH_7MA_1 0x10
#define DRIVER_STRENGTH_9MA_1 0x20
#define DRIVER_STRENGTH_11MA_1 0x30
#define DRIVER_STRENGTH_14MA_1 0x40
#define DRIVER_STRENGTH_16MA_1 0x50
#define DRIVER_STRENGTH_18MA_1 0x60
#define DRIVER_STRENGTH_20MA_1 0x70
#define DRIVER_STRENGTH_22MA_1 0x80
#define DRIVER_STRENGTH_25MA_1 0x90
#define DRIVER_STRENGTH_27MA_1 0xA0
#define DRIVER_STRENGTH_29MA_1 0xB0
#define DRIVER_STRENGTH_32MA_1 0xC0
#define DRIVER_STRENGTH_34MA_1 0xD0
#define DRIVER_STRENGTH_36MA_1 0xE0
#define DRIVER_STRENGTH_38MA_1 0xF0

#define SD_CLK_DRIVER_DEFAULT DRIVER_STRENGTH_25MA_1
#define SD_CMD_DRIVER_DEFAULT DRIVER_STRENGTH_11MA_1
#define SD_DATA_DRIVER_DEFAULT DRIVER_STRENGTH_11MA_1

extern int sd_clk_driver_strength;
extern int sd_cmd_driver_strength;
extern int sd_data_driver_strength;

extern void notify_sim_while_sd_success(struct mmc_host *mmc);

extern void notify_sim_while_sd_fail(struct mmc_host *mmc);

#endif
