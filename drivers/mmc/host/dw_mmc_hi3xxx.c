/*
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (c) 2013 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#include "dw_mmc_hisi.h"
#ifdef CONFIG_MMC_DW_MUX_SDSIM
#include <linux/mmc/dw_mmc_mux_sdsim.h>
#endif


#include <linux/bootdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>
#include <linux/hisi/util.h>
#include <linux/hwspinlock.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"

#ifdef CONFIG_HUAWEI_EMMC_DSM
#include <linux/mmc/dsm_emmc.h>
#endif

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>

static struct dsm_dev dsm_dw_mmc = {
	.name = "dsm_sdio",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 2048,
};

static struct dsm_client *dclient = NULL;
#endif

#ifdef CONFIG_SD_SDIO_CRC_RETUNING

static int hs_timing_config_kirin970_cs_sd_ppll3[TUNING_INIT_CONFIG_NUM] = {1200000000, 6, 4, 3, 13, 0, 171000000};
static int hs_timing_config_kirin970_cs_sdio_ppll3[TUNING_INIT_CONFIG_NUM] = {1200000000, 6, 4, 3, 13, 0, 171000000};
static int hs_timing_config_cancer_sd_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 1, 2, 9, 0, 192000000};
static int hs_timing_config_cancer_sdio_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 3, 2, 9, 0, 192000000};
static int hs_timing_config_cancer_cs_sd_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 4, 2, 9, 0, 192000000};
static int hs_timing_config_cancer_cs_sdio_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 4, 2, 9, 0, 192000000};
static int hs_timing_config_libra_sd_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 3, 2, 9, 0, 192000000};
static int hs_timing_config_libra_sdio_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 3, 2, 9, 0, 192000000};
static int hs_timing_config_taurus_sd_ppll2[TUNING_INIT_CONFIG_NUM] = {960000000, 4, 2, 3, 9, 0, 192000000};
static int hs_timing_config_taurus_sdio_ppll2[TUNING_INIT_CONFIG_NUM] = {1572000000, 7, 7, 6, 15, 0, 196500000};

#endif

#define ENABLE_CLK 0x10000
#define IS_CS_TIMING_CONFIG 0x1

#ifdef CONFIG_SD_TIMEOUT_RESET
#define VOLT_HOLD_CLK_08V 0x1
#define VOLT_TO_1S 0x1
#endif

void __iomem *pericrg_base = NULL;
static void __iomem *sys_base = NULL;
static void __iomem *mmc1_sys_ctrl_base = NULL;
static void __iomem *hsdt_crg_base = NULL;
static void __iomem *mmc0_crg_base = NULL;

/* Common capabilities of hi3650 SoC */
static unsigned long hs_dwmmc_caps[3] = {
#ifdef CONFIG_MMC_DW_EMMC_USED_AS_MODEM
	/* sdio1  - via modem */
	MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_SDIO_IRQ,
#else
	MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23 | MMC_CAP_ERASE,
#endif
	/* sd */
	MMC_CAP_DRIVER_TYPE_A | MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104,
	/* sdio */
	MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_NONREMOVABLE,
};

int g_sdio_reset_ip = 0;
void dw_mci_hi3xxx_work_fail_reset(struct dw_mci *host);
EXPORT_SYMBOL(g_sdio_reset_ip);

static const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const u8 tuning_blk_pattern_8bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static int hs_timing_config[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
	/* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
	{ /*MMC*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 1: MMC_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 4: SDR25 */
		{800000000, 7, 4, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 7: DDR50 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 8: DDR52 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 9: HS200 */
	},
	{ /*SD*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 1, 1, 50000000},		/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 1, 1, 50000000},		/* 4: SDR25 */
		{800000000, 7, 3, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	},
	{ /*SDIO*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 4: SDR25 */
		{800000000, 7, 5, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
	 	{0},					/* 9: HS200 */
	}
};

static int hs_timing_config_hi6250[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
	/* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
	{ /*MMC*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 1: MMC_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 4: SDR25 */
		{800000000, 7, 4, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 7: DDR50 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 8: DDR52 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 9: HS200 */
	},
	{ /*SD*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 2, 2, 50000000},		/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 1, 1, 50000000},		/* 4: SDR25 */
		{800000000, 7, 3, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	},
	{ /*SDIO*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 0, 0, 50000000},		/* 4: SDR25 */
		{800000000, 7, 5, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	}
};

static int hs_timing_config_hi3660[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
	/* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
	{ /*MMC*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 1: MMC_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 4: SDR25 */
		{800000000, 7, 4, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 7: DDR50 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 8: DDR52 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 9: HS200 */
	},
	{ /*SD*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{200000000, 7, 6, 0, 4, 4, 25000000},		/* 1: MMC_HS */
		{400000000, 7, 6, 0, 3, 3, 50000000},		/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 2, 2, 50000000},		/* 4: SDR25 */
		{800000000, 7, 4, 0, 11, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 6, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	},
	{ /*SDIO*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 0, 0, 50000000},		/* 4: SDR25 */
		{800000000, 7, 4, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	}
};

static int hs_timing_config_kirin970[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
	/* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
	{ /*MMC*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{480000000, 9, 6, 0, 1, 1, 48000000},	/* 1: MMC_HS */ /* ES 400M, 8div 50M */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 4: SDR25 */
		{800000000, 7, 4, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 7: DDR50 */
		{800000000, 7, 6, 0, 7, 0, 100000000},	/* 8: DDR52 */
		{1920000000, 9, 5, 4, 15, 0, 192000000},	/* 9: HS200 */ /* ES 960M, 8div 120M */
	},
	{ /*SD*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{200000000, 7, 6, 0, 1, 1, 25000000},		/* 1: MMC_HS */
		{400000000, 7, 6, 0, 4, 4, 50000000},		/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 2, 2, 50000000},		/* 4: SDR25 */
		{800000000, 7, 5, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	},
	{ /*SDIO*/
		{3200000, 7, 7, 0, 15, 15, 400000},		/* 0: LEGACY 400k */
		{0},					/* 1: MMC_HS */
		{400000000, 7, 6, 0, 15, 15, 50000000},	/* 2: SD_HS */
		{200000000, 7, 6, 0, 15, 15, 25000000},	/* 3: SDR12 */
		{400000000, 7, 6, 0, 1, 1, 50000000},		/* 4: SDR25 */
		{800000000, 7, 2, 0, 12, 0, 100000000},	/* 5: SDR50 */
		{1600000000, 7, 5, 4, 15, 0, 200000000},	/* 6: SDR104 */
		{0},					/* 7: DDR50 */
		{0},					/* 8: DDR52 */
		{0},					/* 9: HS200 */
	}
};

static int hs_timing_config_kirin970_cs[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
        /* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
        { /*MMC*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {480000000, 9, 6, 0, 1, 1, 48000000},   /* 1: MMC_HS */ /* ES 400M, 8div 50M */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 2: SD_HS */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 3: SDR12 */
                {400000000, 7, 6, 0, 15, 15, 50000000}, /* 4: SDR25 */
                {800000000, 7, 4, 0, 12, 0, 100000000}, /* 5: SDR50 */
                {1600000000, 7, 5, 4, 15, 0, 200000000},        /* 6: SDR104 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 7: DDR50 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 8: DDR52 */
                {1920000000, 9, 5, 4, 15, 0, 192000000},        /* 9: HS200 */ /* ES 960M, 8div 120M */
        },
        { /*SD*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {240000000, 9, 8, 0, 3, 3, 24000000},           /* 1: MMC_HS */
                {480000000, 9, 7, 0, 4, 4, 48000000},           /* 2: SD_HS */
                {240000000, 9, 8, 0, 19, 19, 24000000}, /* 3: SDR12 */
                {480000000, 9, 7, 0, 3, 3, 48000000},           /* 4: SDR25 */
                {960000000, 9, 4, 0, 16, 0, 96000000}, /* 5: SDR50 */
                {960000000, 4, 2, 1, 9, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        },
        { /*SDIO*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {0},                                    /* 1: MMC_HS */
                {480000000, 9, 7, 0, 19, 19, 48000000}, /* 2: SD_HS */
                {240000000, 9, 8, 0, 19, 19, 24000000}, /* 3: SDR12 */
                {480000000, 9, 7, 0, 19, 19, 48000000},           /* 4: SDR25 */
                {960000000, 9, 4, 0, 16, 0,  96000000}, /* 5: SDR50 */
                {1200000000, 6, 4, 0, 13, 0, 171000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        }
};

static int hs_timing_config_libra[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
        /* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
	{ /*MMC*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {480000000, 9, 6, 0, 1, 1, 48000000},   /* 1: MMC_HS */ /* ES 400M, 8div 50M */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 2: SD_HS */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 3: SDR12 */
                {400000000, 7, 6, 0, 15, 15, 50000000}, /* 4: SDR25 */
                {800000000, 7, 4, 0, 12, 0, 100000000}, /* 5: SDR50 */
                {1600000000, 7, 5, 4, 15, 0, 200000000},        /* 6: SDR104 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 7: DDR50 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 8: DDR52 */
                {1920000000, 9, 5, 4, 15, 0, 192000000},        /* 9: HS200 */ /* ES 960M, 8div 120M*/
        },
        { /*SD*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {203000000, 8, 6, 0, 0, 0, 22500000},           /* 1: MMC_HS */
                {406000000, 8, 6, 0, 1, 1, 45000000},           /* 2: SD_HS */
                {203000000, 8, 6, 0, 0, 0, 22500000}, /* 3: SDR12 */
                {406000000, 8, 6, 0, 1, 1, 45000000},           /* 4: SDR25 */
                {812000000, 8, 4, 0, 13, 0, 90000000}, /* 5: SDR50 */
                {1623000000, 8, 6, 4, 17, 0, 180000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        },
        { /*SDIO*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {0},                                    /* 1: MMC_HS */
                {406000000, 8, 6, 0, 17, 17, 45000000}, /* 2: SD_HS */
                {203000000, 8, 6, 0, 17, 17, 22500000}, /* 3: SDR12 */
                {406000000, 8, 6, 0, 17, 17, 45000000},           /* 4: SDR25 */
                {812000000, 8, 4, 0, 13, 0,  90000000}, /* 5: SDR50 */
                {1623000000, 8, 5, 4, 17, 0, 180000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        }
};


static int hs_timing_config_cancer[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
        /* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
        { /*MMC*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {480000000, 9, 7, 0, 19, 19, 48000000},   /* 1: MMC_HS */ /* ES 400M, 8div 50M */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 2: SD_HS */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 3: SDR12 */
                {400000000, 7, 6, 0, 15, 15, 50000000}, /* 4: SDR25 */
                {800000000, 7, 4, 0, 12, 0, 100000000}, /* 5: SDR50 */
                {1600000000, 7, 5, 4, 15, 0, 200000000},        /* 6: SDR104 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 7: DDR50 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 8: DDR52 */
                {1920000000, 9, 7, 4, 19, 0, 192000000},        /* 9: HS200 */ /* ES 960M, 8div 120M */
        },
        { /*SD*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {480000000, 9, 7, 0, 2, 2, 48000000},           /* 1: MMC_HS */
                {480000000, 9, 7, 0, 4, 4, 48000000},           /* 2: SD_HS */
                {240000000, 9, 8, 0, 19, 19, 24000000}, /* 3: SDR12 */
                {480000000, 9, 7, 0, 2, 2, 48000000},           /* 4: SDR25 */
                {960000000, 9, 4, 0, 11, 0, 96000000}, /* 5: SDR50 */
                {1920000000, 9, 3, 7, 19, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {1920000000, 9, 3, 7, 19, 0, 192000000},/* 9: HS200 */
        },
        { /*SDIO*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {0},                                    /* 1: MMC_HS */
                {480000000, 9, 7, 0, 19, 19, 48000000}, /* 2: SD_HS */
                {240000000, 9, 8, 0, 19, 19, 24000000}, /* 3: SDR12 */
                {480000000, 9, 7, 0, 19, 19, 48000000},           /* 4: SDR25 */
                {960000000, 9, 4, 0, 12, 0,  96000000}, /* 5: SDR50 */
                {1920000000, 9, 7, 7, 19, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        }
};

static int hs_timing_config_cancer_cs[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
        /* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
        { /*MMC*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {240000000, 4, 3, 0, 0, 0, 48000000},   /* 1: MMC_HS */ /* ES 400M, 8div 50M */
                {120000000, 4, 3, 0, 9, 9, 24000000}, /* 2: SD_DS */
                {120000000, 4, 3, 0, 9, 9, 24000000}, /* 3: SDR12 */
                {240000000, 4, 3, 0, 0, 0, 48000000}, /* 4: SDR25 */
                {480000000, 4, 2, 0, 5, 0, 96000000}, /* 5: SDR50 */
                {960000000, 4, 4, 2, 9, 0, 192000000},        /* 6: SDR104 */
                {0},  /* 7: DDR50 */
                {0},  /* 8: DDR52 */
                {960000000, 4, 4, 2, 9, 0, 192000000},        /* 9: HS200 */ /* ES 960M, 8div 120M */
        },
        { /*SD*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {120000000, 4, 3, 0, 3, 3, 24000000},           /* 1: SD_DS */
                {240000000, 4, 3, 0, 9, 9, 48000000},           /* 2: SD_HS */
                {120000000, 4, 3, 0, 9, 9, 24000000}, /* 3: SDR12 */
                {240000000, 4, 3, 0, 0, 0, 48000000},           /* 4: SDR25 */
                {480000000, 4, 2, 0, 5, 0, 96000000}, /* 5: SDR50 */
                {960000000, 4, 4, 2, 9, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {960000000, 4, 4, 2, 9, 0, 192000000},/* 9: HS200 */
        },
        { /*SDIO*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {0},                                    /* 1: MMC_HS */
                {0},                                    /* 2: SD_HS */
                {120000000, 4, 3, 0, 9, 9, 24000000}, /* 3: SDR12 */
                {240000000, 4, 3, 0, 0, 0, 48000000},    /* 4: SDR25 */
                {480000000, 4, 2, 0, 5, 0, 96000000}, /* 5: SDR50 */
                {960000000, 4, 4, 2, 9, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        }
};

static int hs_timing_config_taurus[][TUNING_INIT_TIMING_MODE][TUNING_INIT_CONFIG_NUM] = {
        /* bus_clk,    div, drv_phase, sam_dly, sam_phase_max, sam_phase_min, input_clk */
        { /*MMC*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {480000000, 9, 7, 0, 19, 19, 48000000},   /* 1: MMC_HS */ /* ES 400M, 8div 50M */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 2: SD_HS */
                {200000000, 7, 6, 0, 15, 15, 25000000}, /* 3: SDR12 */
                {400000000, 7, 6, 0, 15, 15, 50000000}, /* 4: SDR25 */
                {800000000, 7, 4, 0, 12, 0, 100000000}, /* 5: SDR50 */
                {1600000000, 7, 5, 4, 15, 0, 200000000},        /* 6: SDR104 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 7: DDR50 */
                {800000000, 7, 6, 0, 7, 0, 100000000},  /* 8: DDR52 */
                {1920000000, 9, 7, 4, 19, 0, 192000000},        /* 9: HS200 */ /* ES 960M, 8div 120M */
        },
        { /*SD*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {120000000, 4, 3, 0, 9, 9, 24000000},           /* 1: MMC_HS */
                {240000000, 4, 2, 0, 9, 9, 48000000},           /* 2: SD_HS */
                {120000000, 4, 3, 0, 9, 9, 24000000}, /* 3: SDR12 */
                {240000000, 4, 2, 0, 2, 2, 48000000},           /* 4: SDR25 */
                {480000000, 4, 3, 0, 5, 0, 96000000}, /* 5: SDR50 */
                {960000000, 4, 2, 3, 9, 0, 192000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        },
        { /*SDIO*/
                {3200000, 7, 7, 0, 15, 15, 400000},             /* 0: LEGACY 400k */
                {0},                                    /* 1: MMC_HS */
                {0}, /* 2: SD_HS */
                {19700000, 7, 8, 0, 15, 15, 25000000}, /* 3: SDR12 */
                {394000000, 7, 7, 0, 15, 15, 49000000},           /* 4: SDR25 */
                {787000000, 7, 5, 0, 8, 0,  98000000}, /* 5: SDR50 */
                {1573000000, 7, 7, 6, 15, 0, 197000000},        /* 6: SDR104 */
                {0},                                    /* 7: DDR50 */
                {0},                                    /* 8: DDR52 */
                {0},                                    /* 9: HS200 */
        }
};

/* only for SD voltage switch on hi3650 FPGA */
extern int gpio_direction_output(unsigned gpio, int value);

int dw_mci_check_himntn(int feature)
{
#ifdef CONFIG_HISILICON_PLATFORM_MAINTAIN
	return check_himntn(feature);
#else
	return 0;
#endif
}

#ifdef CONFIG_SD_SDIO_CRC_RETUNING
void dw_mci_sd_crc_retuning_set(struct dw_mci *host, int sam_phase_val, int *use_sam_dly, int *enable_shift)
{
	/*used for libra,cancer. but not for kirin970*/
	if (host->use_samdly_flag) {
		if (host->use_samdly_range[0] <= sam_phase_val && sam_phase_val <= host->use_samdly_range[1])
			*use_sam_dly = 1;
		if (host->enable_shift_range[0] <= sam_phase_val && sam_phase_val <= host->enable_shift_range[1])
			*enable_shift = 1;
		dev_info(host->dev, "%s:use_samdly_range_min:%d,use_samdly_range_max:%d!\n",__func__,host->use_samdly_range[0],host->use_samdly_range[1]);
		return;
	}

	/*used for kirin970 only*/
	if ((true == host->clk_change)) {
		if (8 <= sam_phase_val && sam_phase_val <= 12)
			*use_sam_dly = 1;
		if (2 <= sam_phase_val && sam_phase_val <= 8)
			*enable_shift = 1;
		dev_info(host->dev, "%s:sd data crc, need retune!\n",__func__);
		return;
	}

	if (4 <= sam_phase_val && sam_phase_val <= 8)
		*use_sam_dly = 1;
	if (2 <= sam_phase_val && sam_phase_val <= 8)
		*enable_shift = 1;
	dev_info(host->dev, "%s:no data crc occur in kirin970, no need retune!\n",__func__);
}
#endif

#ifdef CONFIG_SD_SDIO_CRC_RETUNING
void dw_mci_sdio_crc_retuning_set(struct dw_mci *host, int sam_phase_val, int *use_sam_dly, int *enable_shift)
{
	/*used for libra,cancer. but not for kirin970*/
	if (host->use_samdly_flag) {
		if (host->use_samdly_range[0] <= sam_phase_val && sam_phase_val <= host->use_samdly_range[1])
			*use_sam_dly = 1;
		if (host->enable_shift_range[0] <= sam_phase_val && sam_phase_val <= host->enable_shift_range[1])
			*enable_shift = 1;
		return;
	}

	/*used for kirin970 only*/
	if ((true == host->clk_change)) {
		if (8 <= sam_phase_val && sam_phase_val <= 12)
			*use_sam_dly = 1;
		if (2 <= sam_phase_val && sam_phase_val <= 8)
			*enable_shift = 1;
		return;
	}

	if (8 <= sam_phase_val && sam_phase_val <= 12)
		*use_sam_dly = 1;
	if (2 <= sam_phase_val && sam_phase_val <= 8)
		*enable_shift = 1;
}
#endif

void dw_mci_shift_set(struct dw_mci *host,int sam_phase_val,int d_value,int * use_sam_dly,int *enable_shift)
{
	*use_sam_dly = 0;
	*enable_shift = 0;
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	dw_mci_sd_crc_retuning_set(host,sam_phase_val,use_sam_dly,enable_shift);
#else
	/*only used in boston_cs platform*/
	if (IS_CS_TIMING_CONFIG == host->is_cs_timing_config) {
		if (4 <= sam_phase_val && sam_phase_val <= 8)
			*use_sam_dly = 1;
		if (2 <= sam_phase_val && sam_phase_val <= 6)
			*enable_shift = 1;
		return;
	}

	/*setting enable_shift range. adapt for libra,cancer and later chipsets*/
	if (host->enable_shift_flag) {
		if (host->enable_shift_range[0] <= sam_phase_val && sam_phase_val <= host->enable_shift_range[1])
			*enable_shift = 1;
	} else {
		if (4 + d_value <= sam_phase_val && sam_phase_val <= 12 + d_value)
			*enable_shift = 1;
	}

	/*setting use_sample_dly range. adapt for libra,cancer and later chipsets*/
	if (host->use_samdly_flag) {
		if (host->use_samdly_range[0] <= sam_phase_val && sam_phase_val <= host->use_samdly_range[1])
			*use_sam_dly = 1;
	} else {
		if (11 + 2 * d_value <= sam_phase_val && sam_phase_val <= 14 + 2 * d_value)
			*use_sam_dly = 1;
	}
#endif
}

void dw_mci_shift_sdio_set(struct dw_mci *host,int sam_phase_val,int d_value,int * use_sam_dly,int *enable_shift)
{
	*use_sam_dly = 0;
	*enable_shift = 0;
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	dw_mci_sdio_crc_retuning_set(host,sam_phase_val,use_sam_dly,enable_shift);
#else
	/*only used in boston_cs platform*/
	if (IS_CS_TIMING_CONFIG == host->is_cs_timing_config) {
		if (8 <= sam_phase_val && sam_phase_val <= 12)
			*use_sam_dly = 1;

		if (2 <= sam_phase_val && sam_phase_val <= 8)
			*enable_shift = 1;
		return;
	}

	/*setting enable_shift range. adapt for libra,cancer and later chipsets*/
	if (host->enable_shift_flag) {
		if (host->enable_shift_range[0] <= sam_phase_val && sam_phase_val <= host->enable_shift_range[1])
			*enable_shift = 1;
	} else {
		if (4 <= sam_phase_val && sam_phase_val <= 12)
			*enable_shift = 1;
	}

	if (0xaaaa == host->wifi_sdio_sdr104_160M) {
		if (15 <= sam_phase_val && sam_phase_val <= 18)
			*use_sam_dly = 1;
		return;
	}

	if (0xaaaa == host->wifi_sdio_sdr104_177M) {
		if (13 <= sam_phase_val && sam_phase_val <= 16)
			*use_sam_dly = 1;
		return;
	}

	/*setting use_sample_dly range. adapt for libra,cancer and later chipsets*/
	if (host->use_samdly_flag) {
		if (host->use_samdly_range[0] <= sam_phase_val && sam_phase_val <= host->use_samdly_range[1])
			*use_sam_dly = 1;
	} else {
		if (11 <= sam_phase_val && sam_phase_val <= 14)
			*use_sam_dly = 1;
	}
#endif
}

static void dw_mci_hs_set_timing(struct dw_mci *host, int id, int timing, int sam_phase, int clk_div)
{
	int cclk_div;
	int drv_phase;
	int sam_dly;
	int sam_phase_max, sam_phase_min;
	int sam_phase_val;
	int reg_value;
	int enable_shift = 0;
	int use_sam_dly = 0;
	int d_value = 0;
	struct dw_mci_slot *slot = host->cur_slot;

	if ((host->hw_mmc_id == DWMMC_SD_ID) && (timing == MMC_TIMING_LEGACY))
		cclk_div = hs_timing_config[id][timing][1];
	else
		cclk_div = clk_div;
	if (host->hw_mmc_id == DWMMC_SD_ID) {
		d_value = cclk_div - hs_timing_config[id][timing][1];
	}
	drv_phase = hs_timing_config[id][timing][2];
	sam_dly = hs_timing_config[id][timing][3] + d_value;
	sam_phase_max = hs_timing_config[id][timing][4] + 2 * d_value;
	sam_phase_min = hs_timing_config[id][timing][5];

	if (sam_phase == -1)
		sam_phase_val = (sam_phase_max + sam_phase_min) / 2;
	else
		sam_phase_val = sam_phase;

	/* enable_shift and use_sam_dly setting code */
	/* warning! different with K3V3 */
	switch (id) {
	case DW_MCI_EMMC_ID:
		switch (timing) {
		case MMC_TIMING_UHS_DDR50:
			if (4 <= sam_phase_val && sam_phase_val <= 12)
				enable_shift = 1;
			break;
		case MMC_TIMING_MMC_HS200:
			if (4 <= sam_phase_val && sam_phase_val <= 12)
				enable_shift = 1;
			if (11 <= sam_phase_val && sam_phase_val <= 14)
				use_sam_dly = 1;
			break;
		}
		break;
	case DW_MCI_SD_ID:
		switch (timing) {
		case MMC_TIMING_UHS_SDR50:
			if (4 + d_value <= sam_phase_val && sam_phase_val <= 12 + d_value)
				enable_shift = 1;
			break;
		case MMC_TIMING_UHS_SDR104:
			dw_mci_shift_set(host,sam_phase_val,d_value,&use_sam_dly,&enable_shift);
			break;
		case MMC_TIMING_MMC_HS200:
			dw_mci_shift_set(host,sam_phase_val,d_value,&use_sam_dly,&enable_shift);
			break;

		}
		break;
	case DW_MCI_SDIO_ID:
		switch (timing) {
		case MMC_TIMING_UHS_SDR12:
			break;
		case MMC_TIMING_UHS_DDR50:
			if (4 <= sam_phase_val && sam_phase_val <= 12)
				enable_shift = 1;
			break;
		case MMC_TIMING_UHS_SDR50:
			if (4 <= sam_phase_val && sam_phase_val <= 12)
				enable_shift = 1;
			break;
		case MMC_TIMING_UHS_SDR104:
			dw_mci_shift_sdio_set(host,sam_phase_val,d_value,&use_sam_dly,&enable_shift);
			break;
		}
		break;
	}

	/* first disabl clk */
	/* mci_writel(host, GPIO, ~GPIO_CLK_ENABLE); */
	mci_writel(host, GPIO, 0x0);
	udelay(50);

	reg_value = SDMMC_UHS_REG_EXT_VALUE(sam_phase_val, sam_dly, drv_phase);
	mci_writel(host, UHS_REG_EXT, reg_value);

	mci_writel(host, ENABLE_SHIFT, enable_shift);

	reg_value = SDMMC_GPIO_VALUE(cclk_div, use_sam_dly);
	udelay(50);
	mci_writel(host, GPIO, (unsigned int)reg_value | GPIO_CLK_ENABLE);

	if (!(slot && slot->sdio_wakelog_switch))
		dev_info(host->dev, "id=%d,timing=%d,UHS_REG_EXT=0x%x,ENABLE_SHIFT=0x%x,GPIO=0x%x\n", id, timing, mci_readl(host, UHS_REG_EXT), mci_readl(host, ENABLE_SHIFT), mci_readl(host, GPIO));
}

static void dw_mci_hs_set_parent(struct dw_mci *host, int timing)
{
	/*no need to set parent on austin */
	return;
}

#ifdef CONFIG_HUAWEI_DSM
void dw_mci_dsm_dump(struct dw_mci *host, int err_num)
{
	/*no need to dump message, use hisi dump */
	return;
/*
	if (dclient == NULL) {
		printk(KERN_ERR "dclient is not initialization\n");
		return;
	}
	if (host == NULL || host->dev == NULL) {
		printk(KERN_ERR "sdio host NULL.\n");
		return;
	}
	dev_err(host->dev, "dsm_sido err_num = %d \n", err_num);
	if (!dsm_client_ocuppy(dclient)) {
		dsm_client_record(dclient, "CTRL:0x%x\n", mci_readl(host, CTRL));
		dsm_client_record(dclient, "PWREN:0x%x\n", mci_readl(host, PWREN));
		dsm_client_record(dclient, "CLKDIV:0x%x\n", mci_readl(host, CLKDIV));
		dsm_client_record(dclient, "CLKSRC:0x%x\n", mci_readl(host, CLKSRC));
		dsm_client_record(dclient, "CLKENA:0x%x\n", mci_readl(host, CLKENA));
		dsm_client_record(dclient, "TMOUT:0x%x\n", mci_readl(host, TMOUT));
		dsm_client_record(dclient, "CTYPE:0x%x\n", mci_readl(host, CTYPE));
		dsm_client_record(dclient, "BLKSIZ:0x%x\n", mci_readl(host, BLKSIZ));
		dsm_client_record(dclient, "BYTCNT:0x%x\n", mci_readl(host, BYTCNT));
		dsm_client_record(dclient, "INTMSK:0x%x\n", mci_readl(host, INTMASK));
		dsm_client_record(dclient, "CMDARG:0x%x\n", mci_readl(host, CMDARG));
		dsm_client_record(dclient, "CMD:0x%x\n", mci_readl(host, CMD));
		dsm_client_record(dclient, "MINTSTS:0x%x\n", mci_readl(host, MINTSTS));
		dsm_client_record(dclient, "RINTSTS:0x%x\n", mci_readl(host, RINTSTS));
		dsm_client_record(dclient, "STATUS:0x%x\n", mci_readl(host, STATUS));
		dsm_client_record(dclient, "FIFOTH:0x%x\n", mci_readl(host, FIFOTH));
		dsm_client_record(dclient, "CDETECT:0x%x\n", mci_readl(host, CDETECT));
		dsm_client_record(dclient, "WRTPRT:0x%x\n", mci_readl(host, WRTPRT));
		dsm_client_record(dclient, "GPIO:0x%x\n", mci_readl(host, GPIO));
		dsm_client_record(dclient, "TCBCNT:0x%x\n", mci_readl(host, TCBCNT));
		dsm_client_record(dclient, "TBBCNT:0x%x\n", mci_readl(host, TBBCNT));
		dsm_client_record(dclient, "DEBNCE:0x%x\n", mci_readl(host, DEBNCE));
		dsm_client_record(dclient, "USRID:0x%x\n", mci_readl(host, USRID));
		dsm_client_record(dclient, "VERID:0x%x\n", mci_readl(host, VERID));
		dsm_client_record(dclient, "HCON:0x%x\n", mci_readl(host, HCON));
		dsm_client_record(dclient, "UHS_REG:0x%x\n", mci_readl(host, UHS_REG));
		dsm_client_record(dclient, "BMOD:0x%x\n", mci_readl(host, BMOD));
		dsm_client_record(dclient, "PLDMND:0x%x\n", mci_readl(host, PLDMND));
		dsm_client_record(dclient, "DBADDR:0x%x\n", mci_readl(host, DBADDR));
		dsm_client_record(dclient, "IDSTS:0x%x\n", mci_readl(host, IDSTS));
		dsm_client_record(dclient, "IDINTEN:0x%x\n", mci_readl(host, IDINTEN));
		dsm_client_record(dclient, "DSCADDR:0x%x\n", mci_readl(host, DSCADDR));
		dsm_client_record(dclient, "BUFADDR:0x%x\n", mci_readl(host, BUFADDR));
		dsm_client_record(dclient, "CDTHRCTL:0x%x\n", mci_readl(host, CDTHRCTL));
		dsm_client_record(dclient, "UHS_REG_EXT:0x%x\n", mci_readl(host, UHS_REG_EXT));
		dsm_client_record(dclient, "cmd_status:0x%x\n", host->cmd_status);
		dsm_client_record(dclient, "data_status:0x%x\n", host->data_status);
		dsm_client_record(dclient, "pending_events:0x%x\n", host->pending_events);
		dsm_client_record(dclient, "completed_events:0x%x\n", host->completed_events);
		dsm_client_record(dclient, "state:%d\n", host->state);
		dsm_client_record(dclient, "MINTSTS = %d\n", mci_readl(host, MINTSTS));
		dsm_client_record(dclient, "STATUS = %d\n", mci_readl(host, STATUS));
		dsm_client_record(dclient, "CMD=%d\n", mci_readl(host, CMD));
		dsm_client_record(dclient, "ARG=0x%x \n", mci_readl(host, CMDARG));
		dsm_client_record(dclient, "RESP0:0x%x\n", mci_readl(host, RESP0));
		dsm_client_record(dclient, "RESP1:0x%x\n", mci_readl(host, RESP1));
		dsm_client_record(dclient, "RESP2:0x%x\n", mci_readl(host, RESP2));
		dsm_client_record(dclient, "RESP3:0x%x\n", mci_readl(host, RESP3));
		dsm_client_record(dclient, "host :cmd_status=0x%x.\n", host->cmd_status);
		dsm_client_record(dclient, "data_status=0x%x.\n", host->data_status);
		dsm_client_record(dclient, "host:pending_events=0x%x\n", host->pending_events);
		dsm_client_record(dclient, "completed_events=0x%x.\n", host->completed_events);

		dsm_client_notify(dclient, err_num);
	} else
		printk("DSM CALL FAIL, MCI  ID: %d\n", host->hw_mmc_id);
*/
}

EXPORT_SYMBOL(dw_mci_dsm_dump);
#endif

#define	SD_HWLOCK_ID	11
#define	SD_LOCK_TIMEOUT	1000
static struct hwspinlock	*sd_hwlock;
static int dw_mci_set_sel18(struct dw_mci *host, bool set)
{
	u32 reg;
	unsigned long flag = 0;

	if(NULL == sd_hwlock)
		return 0;
	/*1s timeout,if we can't get sd_hwlock,sd card module will init failed*/
	if (hwspin_lock_timeout_irqsave(sd_hwlock, SD_LOCK_TIMEOUT, &flag)) {
		printk("%s: hwspinlock timeout!\n", __func__);
		return 0;
	}

	reg = readl(sys_base + host->scperctrls);
	if (set)
		reg |= host->bit_sdcard_o_sel18;
	else
		reg &= ~(host->bit_sdcard_o_sel18);

	/*set mask bit when the reg needs mask to protect*/
	reg |= host->odio_sd_mask_bit;

	writel(reg, sys_base + host->scperctrls);

	hwspin_unlock_irqrestore(sd_hwlock, &flag);
	printk(" dw_mci_set_sel18 reg = 0x%x\n", reg);
	return 0;
}

static inline void dw_mci_hs_check_result(struct dw_mci *host, int ret, char* log)
{
	if (ret)
		dev_warn(host->dev, "%s", log);
}

static void dw_mci_hs_power_off(struct dw_mci *host, struct mmc_ios *ios)
{
	int ret;
#ifdef CONFIG_MMC_DW_MUX_SDSIM
	struct dw_mci_hs_priv_data *priv = host->priv;

	if (host->hw_mmc_id == DWMMC_SD_ID && priv->mux_sdsim ) {
		if(SD_SIM_DETECT_STATUS_SIM == sd_sim_detect_status_current)
		{
			dev_info(host->dev, "%s %s SIM has detected,but SD module want to power_off,just passby.\n",MUX_SDSIM_LOG_TAG,__func__);
			return;
		}
		else
		{
			dev_info(host->dev, "%s %s sd_sim_detect_status_current=%d,now SD module want to power_off,go on.\n",MUX_SDSIM_LOG_TAG,__func__,sd_sim_detect_status_current);
		}
	}
#endif

	/* set pin to idle, skip emmc for vccq keeping power always on */
	if ((host->hw_mmc_id == DWMMC_SD_ID) && !(dw_mci_check_himntn(HIMNTN_SD2JTAG) || dw_mci_check_himntn(HIMNTN_SD2DJTAG))) {

#ifdef CONFIG_MMC_DW_MUX_SDSIM
		if(priv->mux_sdsim)
		{
			if(SD_SIM_DETECT_STATUS_SIM != sd_sim_detect_status_current)
				config_sdsim_gpio_mode(SDSIM_MODE_SD_IDLE);
			else
				printk("%s %s SIM has detected,but SD module want to config SDSIM_MODE_SD_IDLE,just passby.\n",MUX_SDSIM_LOG_TAG,__func__);
		}
		else
		{
			if ((host->pinctrl) && (host->pins_idle)) {
				ret = pinctrl_select_state(host->pinctrl, host->pins_idle);
				dw_mci_hs_check_result(host, ret, "could not set sd idle pins\n");
			}
		}
#else
		if ((host->pinctrl) && (host->pins_idle)) {
			ret = pinctrl_select_state(host->pinctrl, host->pins_idle);
			dw_mci_hs_check_result(host, ret, "could not set sd idle pins\n");
		}
#endif

	} else if ((host->hw_mmc_id != DWMMC_EMMC_ID) && (host->hw_mmc_id != DWMMC_SD_ID)) {
		if ((host->pinctrl) && (host->pins_idle)) {
			ret = pinctrl_select_state(host->pinctrl, host->pins_idle);
			dw_mci_hs_check_result(host, ret, "could not set idle pins\n");
		}
	}

	if (host->hw_mmc_id == DWMMC_SDIO_ID) {
		dev_err(host->dev, "POWER_OFF SDIO !\n");
		return;
	}

	if (host->vqmmc) {
		ret = regulator_disable(host->vqmmc);
		dw_mci_hs_check_result(host, ret, "egulator_disable vqmmc failed\n");
	}

#ifdef CONFIG_MMC_DW_MUX_SDSIM
	if(host->hw_mmc_id == DWMMC_SD_ID && priv->mux_sdsim)
	{
		if (host->vmmc) {
			if ( !(host->vmmcmosen) || (MUX_SDSIM_VCC_STATUS_1_8_0_V != priv->mux_sdsim_vcc_status) )
			{
				ret = regulator_disable(host->vmmc);
				dw_mci_hs_check_result(host, ret, "egulator_disable vmmc failed\n");
			}
		}

		if (host->vmmcmosen) {
			ret = regulator_disable(host->vmmcmosen);
			dw_mci_hs_check_result(host, ret, "egulator_disable vmmcmosen failed\n");
		}
	}
	else
	{
		if (host->vmmc) {
			ret = regulator_disable(host->vmmc);
			dw_mci_hs_check_result(host, ret, "egulator_disable vmmc failed\n");
		}
	}

#else
	if (host->vmmc) {
		ret = regulator_disable(host->vmmc);
		dw_mci_hs_check_result(host, ret, "egulator_disable vmmc failed\n");
	}
#endif
}

static void dw_mci_hs_power_up(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int ret;
	u32 reg;

#ifdef CONFIG_MMC_DW_MUX_SDSIM
	if (host->hw_mmc_id == DWMMC_SD_ID && priv->mux_sdsim ) {
		if(SD_SIM_DETECT_STATUS_SIM == sd_sim_detect_status_current)
		{
			dev_info(host->dev, "%s %s SIM has detected,but SD module want to power_up,just passby.\n",MUX_SDSIM_LOG_TAG,__func__);
			return;
		}
		else
		{
			dev_info(host->dev, "%s %s sd_sim_detect_status_current=%d,now SD module want to power_up,go on.\n",MUX_SDSIM_LOG_TAG,__func__,sd_sim_detect_status_current);
		}
	}
#endif

	if (HI3660_FPGA == priv->hi3660_fpga_sd_ioset) {
		/*set GPIO15[0] and GPIO15[1] to outpot */
		/*set GPIO15[0] to High */
		/*set GPIO15[1] to Low */
		(void)gpio_request(priv->hi3660_sd_ioset_jtag_sd_sel, "jtag_sd_sel");
		(void)gpio_request(priv->hi3660_sd_ioset_sd_sel, "sd_sel");
		gpio_direction_output(priv->hi3660_sd_ioset_jtag_sd_sel, 0);
		gpio_direction_output(priv->hi3660_sd_ioset_sd_sel, 1);
		dev_info(host->dev, "set Hi3660 FPGA sd io\n");
		gpio_free(priv->hi3660_sd_ioset_jtag_sd_sel);
		gpio_free(priv->hi3660_sd_ioset_sd_sel);
	}

	if (host->hw_mmc_id == DWMMC_SD_ID) {
#ifdef CONFIG_MMC_DW_MUX_SDSIM
		if (priv->mux_sdsim) {
			ret = dw_mci_set_sel18(host, 1);
		} else {
			ret = dw_mci_set_sel18(host, 0);
		}
#else
		ret = dw_mci_set_sel18(host, 0);
#endif
		dw_mci_hs_check_result(host, ret, "ios dw_mci_set_sel18 error!\n");
		/* Wait for 5ms */
		usleep_range(5000, 5500);

		if (host->vqmmc) {
#ifdef CONFIG_MMC_DW_MUX_SDSIM
			if (priv->mux_sdsim) {
				ret = regulator_set_voltage(host->vqmmc, 1800000, 1800000);
			} else {
				ret = regulator_set_voltage(host->vqmmc, 2950000, 2950000);
			}
#else
			ret = regulator_set_voltage(host->vqmmc, 2950000, 2950000);
#endif
			dw_mci_hs_check_result(host, ret, "regulator_set_voltage vqmmc failed!\n");

			ret = regulator_enable(host->vqmmc);
			dw_mci_hs_check_result(host, ret, "regulator_enable vqmmc failed!\n");
			usleep_range(1000, 1500);
		}

		if (host->vmmc) {
#ifdef CONFIG_MMC_DW_MUX_SDSIM
			if(priv->mux_sdsim)
			{
				if( MUX_SDSIM_VCC_STATUS_1_8_0_V == priv->mux_sdsim_vcc_status)
				{
					ret = regulator_set_voltage(host->vmmc, 1800000, 1800000);
					dev_info(host->dev, "%s %s LDO16 VCC set for 1.8V ret = %d.\n",MUX_SDSIM_LOG_TAG,__func__,ret);
				}
				else
				{
					ret = regulator_set_voltage(host->vmmc, 2950000, 2950000);
					dev_info(host->dev, "%s %s LDO16 VCC set for 2.95V ret = %d.\n",MUX_SDSIM_LOG_TAG,__func__,ret);
				}

				if (host->vmmcmosen) {
					ret = regulator_set_voltage(host->vmmcmosen, 3000000, 3000000);
					dev_info(host->dev, "%s %s LDO12 VCC set for 3.00V ret = %d.\n",MUX_SDSIM_LOG_TAG,__func__,ret);
				}
			}
			else
			{
				ret = regulator_set_voltage(host->vmmc, 2950000, 2950000);
			}
#else
			ret = regulator_set_voltage(host->vmmc, 2950000, 2950000);
#endif
			dw_mci_hs_check_result(host, ret, "regulator_set_voltage vmmc failed!\n");


#ifdef CONFIG_MMC_DW_MUX_SDSIM
			if(priv->mux_sdsim)
			{
				if ( !(host->vmmcmosen) || (MUX_SDSIM_VCC_STATUS_1_8_0_V != priv->mux_sdsim_vcc_status) )
				{
					ret = regulator_enable(host->vmmc);
					dw_mci_hs_check_result(host, ret, "regulator_enable vmmc failed!\n");
					usleep_range(1000, 1500);
				}

				if(host->vmmcmosen)
				{
					ret = regulator_enable(host->vmmcmosen);
					dw_mci_hs_check_result(host, ret, "regulator_enable vmmcmosen failed!\n");
					usleep_range(1000, 1500);
				}
			}
			else
			{
				ret = regulator_enable(host->vmmc);
				dw_mci_hs_check_result(host, ret, "regulator_enable vmmc failed!\n");
				usleep_range(1000, 1500);
			}
#else
			ret = regulator_enable(host->vmmc);
			dw_mci_hs_check_result(host, ret, "regulator_enable vmmc failed!\n");
			usleep_range(1000, 1500);
#endif

		}

		/*Before enable GPIO,disable clk*/
		reg = mci_readl(host, GPIO);
		reg &= ~ENABLE_CLK;
		mci_writel(host, GPIO,reg);
		udelay(20);

		if (!(dw_mci_check_himntn(HIMNTN_SD2JTAG) || dw_mci_check_himntn(HIMNTN_SD2DJTAG))) {
#ifdef CONFIG_MMC_DW_MUX_SDSIM
			if(priv->mux_sdsim)
			{
				if(SD_SIM_DETECT_STATUS_SIM != sd_sim_detect_status_current)
					config_sdsim_gpio_mode(SDSIM_MODE_SD_NORMAL);
				else
					printk("%s %s SIM has detected,but SD module want to config SDSIM_MODE_SD_NORMAL,just passby.\n",MUX_SDSIM_LOG_TAG,__func__);
			}
			else
			{
				if ((host->pinctrl) && (host->pins_default)) {
					ret = pinctrl_select_state(host->pinctrl, host->pins_default);
					dw_mci_hs_check_result(host, ret, "could not set default pins\n");
				}
			}

#else
			if ((host->pinctrl) && (host->pins_default)) {
				ret = pinctrl_select_state(host->pinctrl, host->pins_default);
				dw_mci_hs_check_result(host, ret, "could not set default pins\n");
			}
#endif

		}

		/*After enable GPIO,enable clk*/
		reg = mci_readl(host, GPIO);
		reg |= ENABLE_CLK;
		mci_writel(host, GPIO,reg);
		udelay(20);
	} else {
		if ((host->pinctrl) && (host->pins_default)) {
			ret = pinctrl_select_state(host->pinctrl, host->pins_default);
			dw_mci_hs_check_result(host, ret, "could not set pins\n");
		}

		if (host->hw_mmc_id == DWMMC_SDIO_ID) {
			dev_err(host->dev, "POWER_UP SDIO !\n");

			if(1 == g_sdio_reset_ip) {
				dw_mci_hi3xxx_work_fail_reset(host);
				g_sdio_reset_ip = 0;
			}

			return;
		}

		if (host->vmmc) {
			ret = regulator_set_voltage(host->vmmc, 2950000, 2950000);
			dw_mci_hs_check_result(host, ret, "set voltage vmmc failed!\n");

			ret = regulator_enable(host->vmmc);
			dw_mci_hs_check_result(host, ret, "enable regulator vmmc failed!\n");
		}

		if (host->vqmmc) {
			ret = regulator_set_voltage(host->vqmmc, 2950000, 2950000);
			dw_mci_hs_check_result(host, ret, "set voltage vqmmc failed!\n");

			ret = regulator_enable(host->vqmmc);
			dw_mci_hs_check_result(host, ret, "enable regulator vqmmc failed!\n");
		}
	}

}

static void dw_mci_hs_set_power(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_hs_priv_data *priv = host->priv;

	if (priv->old_power_mode == ios->power_mode) /* no need change power */
		return;

	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		dev_info(host->dev, "set io to lowpower\n");
		dw_mci_hs_power_off(host, ios);
		break;
	case MMC_POWER_UP:
		dev_info(host->dev, "set io to normal\n");
		dw_mci_hs_power_up(host, ios);
		break;
	case MMC_POWER_ON:
		break;
	default:
		dev_info(host->dev, "unknown power supply mode\n");
		break;
	}
	priv->old_power_mode = ios->power_mode;
}

static void dw_mci_hs_set_clk(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int id = priv->id;
	int ret;

	if (priv->old_timing == ios->timing) /* no need change clock */
		return;

	dw_mci_hs_set_parent(host, ios->timing);

	if (!IS_ERR(host->ciu_clk))
		clk_disable_unprepare(host->ciu_clk);

	ret = clk_set_rate(host->ciu_clk, hs_timing_config[id][ios->timing][0]);
	if (ret)
		dev_err(host->dev, "clk_set_rate failed, clock = %d, ret = %d\n",
			hs_timing_config[id][ios->timing][0], ret);

	if (!IS_ERR(host->ciu_clk)) {
		if (clk_prepare_enable(host->ciu_clk))
			dev_err(host->dev, "ciu_clk clk_prepare_enable failed\n");
	}

	if (priv->in_resume != STATE_KEEP_PWR)
		host->tuning_init_sample = (hs_timing_config[id][ios->timing][4] + hs_timing_config[id][ios->timing][5]) / 2;

	if (host->sd_reinit == 0)
		host->current_div = hs_timing_config[id][ios->timing][1];

	dw_mci_hs_set_timing(host, id, ios->timing, host->tuning_init_sample, host->current_div);

	if (priv->priv_bus_hz == 0)
		host->bus_hz = hs_timing_config[id][ios->timing][6];
	else
		host->bus_hz = 2 * hs_timing_config[id][ios->timing][6];

	if (priv->dw_mmc_bus_clk) {
		/*if FPGA, the clk for SD should be 20M */
		/*if ((id == MMC_SD) || (id ==MMC_EMMC)) {*/
		host->bus_hz = priv->dw_mmc_bus_clk;
		/*}*/
		/*if FPGA, the clk for wifi should div 10 */
		/*else if (id == MMC_SDIO)*/
		/*  host->bus_hz = (host->bus_hz)/10;*/
	}

	priv->old_timing = ios->timing;
}

static void dw_mci_hs_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{
	dw_mci_hs_set_power(host, ios);
	dw_mci_hs_set_clk(host, ios);
}

static void dw_mci_hs_prepare_command(struct dw_mci *host, u32 *cmdr)
{
	*cmdr |= SDMMC_CMD_USE_HOLD_REG;
}


static int _get_resource_hsdt(void)
{
	struct device_node *np = NULL;

	if (!hsdt_crg_base) {
		np = of_find_compatible_node(NULL, NULL, "Hisilicon,hsdt_crg");
		if (!np) {
			printk("can't find hsdt_crg!\n");
			return -1;
		}

		hsdt_crg_base = of_iomap(np, 0);
		if (!hsdt_crg_base) {
			printk("hsdt crg iomap error!\n");
			return -1;
		}
	}

	return 0;
}

static int _get_resource_mmc0(void)
{
	struct device_node *np = NULL;

	if (!mmc0_crg_base) {
		np = of_find_compatible_node(NULL, NULL, "hisilicon,mmc0crg");
		if (!np) {
			printk("can't find mmc0_crg!\n");
			return -1;
		}

		mmc0_crg_base = of_iomap(np, 0);
		if (!mmc0_crg_base) {
			printk("mmc0sysctrl iomap error!\n");
			return -1;
		}
	}

	return 0;
}

static int get_resource_cancer(void)
{
	struct device_node *np = NULL;

	if (!mmc1_sys_ctrl_base) {
		np = of_find_compatible_node(NULL, NULL, "Hisilicon,mmc1_sysctrl");
		if (!np) {
			printk("can't find mmc1sysctrl!\n");
			return -1;
		}

		mmc1_sys_ctrl_base = of_iomap(np, 0);
		if (!mmc1_sys_ctrl_base) {
			printk("mmc1sysctrl iomap error!\n");
			return -1;
		}
	}

	return 0;
}


static int dw_mci_hs_get_dt_pltfm_resource(struct dw_mci *host, struct device_node  *of_node)
{
	if (of_device_is_compatible(of_node, "hisilicon,taurus-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET_TAURUS;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18_TAURUS;
		host->sdio_rst |= BIT_RST_SDIO_TAURUS;
		host->odio_sd_mask_bit = BIT_VOLT_VALUE_18_MASK_TAURUS;
		memcpy(hs_timing_config, hs_timing_config_taurus, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
		if (_get_resource_hsdt() || _get_resource_mmc0())
			return -ENODEV;
	} else if (of_device_is_compatible(of_node, "hisilicon,libra-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18;
		host->sdio_rst |= BIT_RST_SDIO_LIBRA;
		memcpy(hs_timing_config, hs_timing_config_libra, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
	} else if (of_device_is_compatible(of_node, "hisilicon,cancer-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET_CANCER;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18_CANCER;
		host->sdio_rst |= BIT_HRST_SDIO_CANCER;
		host->odio_sd_mask_bit = BIT_VOLT_VALUE_18_MASK_CANCER;

		if(of_find_property(of_node, "cs_sd_timing_config_cancer", (int *)NULL)){
			memcpy(hs_timing_config, hs_timing_config_cancer_cs, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
		}
		else{
			memcpy(hs_timing_config, hs_timing_config_cancer, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
		}

		if (get_resource_cancer())
			return -ENODEV;

	}else if (of_device_is_compatible(of_node, "hisilicon,kirin970-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18;
		host->sdio_rst |= BIT_RST_SDIO_BOSTON;
		if(of_find_property(of_node, "cs_sd_timing_config", (int *)NULL)){
			memcpy(hs_timing_config, hs_timing_config_kirin970_cs, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
		}
		else{
			memcpy(hs_timing_config, hs_timing_config_kirin970, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
		}
	} else if (of_device_is_compatible(of_node, "hisilicon,hi3660-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18;
		host->sdio_rst |= BIT_RST_SDIO_CHICAGO;
		memcpy(hs_timing_config, hs_timing_config_hi3660, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
	} else if (of_device_is_compatible(of_node, "hisilicon,hi6250-dw-mshc")){
		host->scperctrls         |= BIT_VOLT_OFFSET;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18;
		host->sdio_rst |= BIT_RST_SDIO;
		memcpy(hs_timing_config, hs_timing_config_hi6250, sizeof(hs_timing_config));/* unsafe_function_ignore: memcpy */
	} else if (of_device_is_compatible(of_node, "hisilicon,hi3650-dw-mshc")) {
		host->scperctrls         |= BIT_VOLT_OFFSET_AUSTIN;
		host->bit_sdcard_o_sel18 |= BIT_VOLT_VALUE_18;
		host->sdio_rst |= BIT_RST_SDIO;
	} else {
		return -1;
	}

	return 0;
}

static int dw_mci_hs_get_resource(void)
{
	struct device_node *np = NULL;

	if (!pericrg_base) {
		np = of_find_compatible_node(NULL, NULL, "Hisilicon,crgctrl");
		if (!np) {
			printk("can't find crgctrl!\n");
			return -1;
		}

		pericrg_base = of_iomap(np, 0);
		if (!pericrg_base) {
			printk("crgctrl iomap error!\n");
			return -1;
		}
	}

	if (!sys_base) {
		np = of_find_compatible_node(NULL, NULL, "Hisilicon,sysctrl");
		if (!np) {
			printk("can't find sysctrl!\n");
			return -1;
		}

		sys_base = of_iomap(np, 0);
		if (!sys_base) {
			printk("sysctrl iomap error!\n");
			return -1;
		}
	}

	return 0;
}

/******************************************************************************
 * Do private setting specified for controller.
 * dw_mci_hs_priv_init execute before controller unreset,
 * this will cause NOC error.
 * put this function after unreset and clock set.
 *****************************************************************************/
static void dw_mci_hs_priv_setting(struct dw_mci *host)
{
	/* set threshold to 512 bytes */
	mci_writel(host, CDTHRCTL, 0x02000001);
}

static void dw_mci_hs_set_rst_m(struct dw_mci *host, bool set)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int id = priv->id;

	if (pericrg_base == NULL) {
		dev_err(host->dev, "pericrg_base is null, can't rst! \n");
		return;
	}

	if (set) {
		if (DW_MCI_SD_ID == id) {
			writel(BIT_RST_SD_M, pericrg_base + PERI_CRG_RSTEN4);
			dev_info(host->dev, "rest_m for sd \n");
		} else {
			dev_info(host->dev, "other rest_m need to add \n");
		}
	} else {
		if (DW_MCI_SD_ID == id) {
			writel(BIT_RST_SD_M, pericrg_base + PERI_CRG_RSTDIS4);
			dev_info(host->dev, "unrest_m for sd \n");
		} else {
			dev_info(host->dev, "other unrest_m need to add \n");
		}
	}
}

void dw_mci_hs_reset(struct dw_mci *host, int id)
{
	if (0 == id) {
		if (hsdt_crg_base)
			writel(host->sdio_rst, hsdt_crg_base + HSDT_CRG_PERRSTEN0);
		else if (mmc1_sys_ctrl_base)
			writel(BIT_HRST_SDIO_CANCER, mmc1_sys_ctrl_base + MMC1_SYSCTRL_PERRSTEN0);
		else
			writel(BIT_RST_EMMC, pericrg_base + PERI_CRG_RSTEN4);
		dev_info(host->dev, "rest emmc \n");
	} else if (1 == id) {
		if (mmc0_crg_base)
			writel(BIT_RST_SD_TAURUS, mmc0_crg_base + MMC0_CRG_SD_HRST);
		else
			writel(BIT_RST_SD, pericrg_base + PERI_CRG_RSTEN4);
		dev_info(host->dev, "rest sd \n");
	} else if (2 == id) {
		if (hsdt_crg_base)
			writel(host->sdio_rst, hsdt_crg_base + HSDT_CRG_PERRSTEN0);
		else if (mmc1_sys_ctrl_base)
			writel(host->sdio_rst, mmc1_sys_ctrl_base + MMC1_SYSCTRL_PERRSTEN0);
		else
			writel(host->sdio_rst, pericrg_base + PERI_CRG_RSTEN4);
		dev_info(host->dev, "rest sdio \n");
	}
}

void dw_mci_hs_unreset(struct dw_mci *host, int id)
{
	if (0 == id) {
		if (hsdt_crg_base)
			writel(host->sdio_rst, hsdt_crg_base + HSDT_CRG_PERRSTDIS0);
		else if (mmc1_sys_ctrl_base)
			writel(BIT_HRST_SDIO_CANCER, mmc1_sys_ctrl_base + MMC1_SYSCTRL_PERRSTDIS0);
		else
			writel(BIT_RST_EMMC, pericrg_base + PERI_CRG_RSTDIS4);
		dev_info(host->dev, "unrest emmc \n");
	} else if (1 == id) {
		if (mmc0_crg_base)
			writel(BIT_URST_SD_TAURUS, mmc0_crg_base + MMC0_CRG_SD_HURST);
		else
			writel(BIT_RST_SD, pericrg_base + PERI_CRG_RSTDIS4);
		dev_info(host->dev, "unrest sd \n");
	} else if (2 == id) {
		if (hsdt_crg_base)
			writel(host->sdio_rst, hsdt_crg_base + HSDT_CRG_PERRSTDIS0);
		else if (mmc1_sys_ctrl_base)
			writel(host->sdio_rst, mmc1_sys_ctrl_base + MMC1_SYSCTRL_PERRSTDIS0);
		else
			writel(host->sdio_rst, pericrg_base + PERI_CRG_RSTDIS4);
		dev_info(host->dev, "unrest sdio \n");
	}
}

static int dw_mci_hs_set_controller(struct dw_mci *host, bool set)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int id = priv->id;

	if (pericrg_base == NULL) {
		dev_err(host->dev, "pericrg_base is null, can't reset mmc! \n");
		return -1;
	}

	if (set) {
		dw_mci_hs_reset(host, id);
		goto out;
	} else {
		dw_mci_hs_unreset(host, id);
		goto out;
	}
out:
	return 0;
}

struct dw_mci *sdio_host = NULL;

void dw_mci_sdio_card_detect(struct dw_mci *host)
{
	if (host == NULL) {
		printk(KERN_ERR "sdio detect, host is null,can not used to detect sdio\n");
		return;
	}

	dw_mci_set_cd(host);

	queue_work(host->card_workqueue, &host->card_work);
	return;
};

void dw_mci_sdio_card_detect_change(void)
{
	dw_mci_sdio_card_detect(sdio_host);
}

EXPORT_SYMBOL(dw_mci_sdio_card_detect_change);

#ifdef CONFIG_MMC_DW_MUX_SDSIM
struct dw_mci *host_from_sd_module = NULL;
#endif

static int dw_mci_hs_priv_init(struct dw_mci *host)
{
	struct dw_mci_hs_priv_data *priv;
	struct platform_device *pdev = NULL;
#ifdef CONFIG_MMC_DW_EMMC_USED_AS_MODEM
	static const char *const hi_mci0 = "hi_mci.3";
#else
	static const char *const hi_mci0 = "hi_mci.0";
#endif
	static const char *const hi_mci1 = "hi_mci.1";
	static const char *const hi_mci2 = "hi_mci.2";
	int error;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(host->dev, "mem alloc failed for private data\n");
		return -ENOMEM;
	}
	priv->id = of_alias_get_id(host->dev->of_node, "mshc");
	if ((priv->id == MMC_EMMC) && (get_bootdevice_type() != BOOT_DEVICE_EMMC)) {
		devm_kfree(host->dev, priv);
		return -ENODEV;
	}
	priv->old_timing = -1;
	priv->in_resume = STATE_LEGACY;
	host->priv = priv;
	host->hw_mmc_id = priv->id;
	host->flags &= ~DWMMC_IN_TUNING;
	host->flags &= ~DWMMC_TUNING_DONE;
	/*
	 *  Here for SD, the default value of voltage-switch gpio,
	 *  which is only used in hi3650 FPGA, is set to (-1) for ASIC
	 */
	priv->dw_voltage_switch_gpio = SDMMC_ASIC_PLATFORM;

	if (priv->id == DW_MCI_SDIO_ID) {
		sdio_host = host;
	}

	/**
     * BUG: device rename krees old name, which would be realloced for other
     * device, pdev->name points to freed space, driver match may cause a panic
     * for wrong device
     */
	pdev = container_of(host->dev, struct platform_device, dev);

	switch (priv->id) {
	case MMC_EMMC:
		pdev->name = hi_mci0;
		error = device_rename(host->dev, hi_mci0);
		if (error < 0) {
			dev_err(host->dev, "dev set name %s fail \n", hi_mci0);
			goto fail;
		}
#ifndef CONFIG_MMC_DW_EMMC_USED_AS_MODEM
#ifdef CONFIG_HISI_BOOTDEVICE
		if (get_bootdevice_type() == BOOT_DEVICE_EMMC)
			set_bootdevice_name(&pdev->dev);
#endif
#endif

		break;
	case MMC_SD:
		pdev->name = hi_mci1;
		error = device_rename(host->dev, hi_mci1);
		if (error < 0) {
			dev_err(host->dev, "dev set name hi_mci.1 fail \n");
			goto fail;
		}

		/*Sd hardware lock,avoid to access the SCPERCTRL5 register in USIM card module in the same time */
		sd_hwlock = hwspin_lock_request_specific(SD_HWLOCK_ID);
		if (sd_hwlock == NULL) {
			printk("Request hwspin lock failed !\n");
			goto fail;
		}

#ifdef CONFIG_MMC_DW_MUX_SDSIM
		host_from_sd_module = host;
		printk("%s host_from_sd_module is inited when dw_mci_hs_priv_init.\n",MUX_SDSIM_LOG_TAG);
		sema_init(&sem_mux_sdsim_detect, 1);
#endif
		break;
	case MMC_SDIO:
		pdev->name = hi_mci2;
		error = device_rename(host->dev, hi_mci2);
		if (error < 0) {
			dev_err(host->dev, "dev set name hi_mci.2 fail \n");
			goto fail;
		}

		break;

	default:
		dev_err(host->dev, "mpriv->id is out of range!!! \n");
		goto fail;
	}
	/* still keep pdev->name same with dev->kobj.name */
	pdev->name = host->dev->kobj.name;

	return 0;

fail:
	/* if rename failed, restore old value, keep pdev->name same to
	 * dev->kobj.name */
	pdev->name = host->dev->kobj.name;
	devm_kfree(host->dev, priv);
	return -1;
}

static int dw_mci_hs_setup_clock(struct dw_mci *host)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int timing = MMC_TIMING_LEGACY;
	int id = priv->id;
	int ret;

	dw_mci_hs_set_parent(host, timing);

	ret = clk_set_rate(host->ciu_clk, hs_timing_config[id][timing][0]);
	if (ret)
		dev_err(host->dev, "clk_set_rate failed\n");

	dw_mci_hs_set_controller(host, 0);
	dw_mci_hs_priv_setting(host);

	host->tuning_current_sample = -1;
	host->current_div = hs_timing_config[id][timing][1];

	host->tuning_init_sample = (hs_timing_config[id][timing][4] + hs_timing_config[id][timing][5]) / 2;

	dw_mci_hs_set_timing(host, id, timing, host->tuning_init_sample, host->current_div);

	if (priv->priv_bus_hz == 0) {
		host->bus_hz = hs_timing_config[id][timing][6];
	} else {
		host->bus_hz = priv->priv_bus_hz;
	}

	if (priv->dw_mmc_bus_clk) {
		/*if FPGA, the clk for SD should be 20M */
		/*if ((id == MMC_EMMC) || (id == MMC_SD)){*/
		host->bus_hz = priv->dw_mmc_bus_clk;
		/*}*/
	}

	priv->old_timing = timing;

	return 0;
}
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
static void dw_mci_hs_get_retuning_config(struct dw_mci *host)
{
	struct device_node *np = host->dev->of_node;
	if (DWMMC_SD_ID == host->hw_mmc_id) {
		if (of_find_property(np, "cs_sd_timing_config", (int *)NULL)) {
			host->retuning_flag = SD_RETUNING_ON;
			dev_info(host->dev, "%s:SD card retuning on!\n", __func__);
		} else {
			host->retuning_flag =SD_RETUNING_OFF;
			dev_info(host->dev, "%s:SD card retuning off!\n", __func__);
		}
	} else if (DWMMC_SDIO_ID == host->hw_mmc_id) {

		if (of_find_property(np, "cs_sdio_timing_config", (int *)NULL)) {
			host->retuning_flag = SD_RETUNING_ON;
			dev_info(host->dev, "%s:SDIO retuning on!\n", __func__);
		} else {
			host->retuning_flag =SD_RETUNING_OFF;
			dev_info(host->dev, "%s:SDIO retuning off!\n", __func__);
		}
	} else {
		return;
	}
	return;
}
#endif


void dw_mci_hs_get_timing_config(struct dw_mci *host)
{
	struct device_node *np = host->dev->of_node;
	int ret;
	#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	dw_mci_hs_get_retuning_config(host);
	#endif
	if (of_find_property(np, "cs_sd_timing_config", (int *)NULL)) {
		host->is_cs_timing_config = IS_CS_TIMING_CONFIG;
	}

	/* adapt the min and max value of use_sample_dly in dts between different chipsets. There are two elements in host->use_samdly_range which represent use_samdly_range_min,use_samdly_range_max */
	ret = of_property_read_u32_array(np, "use_samdly_range", &(host->use_samdly_range[0]), 2);
	if (ret) {
		host->use_samdly_flag = false;
		dev_info(host->dev, "%s:get use_sample_dly range range failed!\n", __func__);
	} else {
		host->use_samdly_flag = true;
		dev_info(host->dev, "%s:use_samdly_range_min:%d,use_samdly_range_max:%d!\n",__func__,host->use_samdly_range[0],host->use_samdly_range[1]);
	}
	/*adapt the min and max value of enable_shift in dts between different chipsets. There are two elements in host->enable_shift_range which represent ena_shift_range_min,ena_shift_range_max*/
	ret = of_property_read_u32_array(np, "enable_shift_range", &(host->enable_shift_range[0]), 2);
	 if (ret) {
		host->enable_shift_flag = false;
		dev_info(host->dev, "%s:get enable_shift range range failed!\n", __func__);
	} else {
		host->enable_shift_flag = true;
		dev_info(host->dev, "%s:ena_shift_range_min:%d,ena_shift_range_max:%d!\n",__func__,host->enable_shift_range[0],host->enable_shift_range[1]);
	}
}

/* this function only add the config for
* sdio sdr104 speedmode,it's for fix Complexity.
*/
static void config_sdr104_192m(struct device_node *np)
{
	if (of_find_property(np, "wifi_sdio_sdr104_192M", (int *)NULL)) {
		hs_dwmmc_caps[DW_MCI_SDIO_ID] |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104);
		hs_timing_config[2][6][0] = 960000000;
		hs_timing_config[2][6][1] = 4;
		hs_timing_config[2][6][2] = 3;
		hs_timing_config[2][6][3] = 2;
		hs_timing_config[2][6][4] = 9;
		hs_timing_config[2][6][6] = 192000000;
		printk(KERN_ERR "set sdio sdr104 192M.\n");
	}
}

static int dw_mci_hs_parse_dt(struct dw_mci *host)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	struct device_node *np = host->dev->of_node;
	u32 value = 0;
	int error = 0;

	if (of_property_read_u32(np, "is-resetable", &host->is_reset_after_retry))
		dev_info(host->dev, "is-resetable get value failed \n");

	if (of_property_read_u32(np, "debug_percrg_sd_sdio_reg_print", &value))
	{
		dev_info(host->dev, "debug_percrg_sd_sdio_reg_print get value failed \n");
		value = 0;

	}
	if(CRG_PRINT == value)
		priv->crg_print = CRG_PRINT;

	error = dw_mci_hs_get_dt_pltfm_resource(host, np);
	if(error)
		return error;

	dw_mci_hs_get_timing_config(host);

	if (of_find_property(np, "hi3660_fpga_sd_ioset", NULL)) {
		priv->hi3660_fpga_sd_ioset = HI3660_FPGA;
		dev_info(host->dev, "fpga_sd_ioset is %d", priv->hi3660_fpga_sd_ioset);
	}

	priv->hi3660_sd_ioset_sd_sel = of_get_named_gpio(np, "hi3660_sd_ioset_sd_sel", 0);
	if (!gpio_is_valid(priv->hi3660_sd_ioset_sd_sel)) {
		dev_info(host->dev, "sd_ioset_sd_sel not available\n");
		priv->hi3660_sd_ioset_sd_sel = -1;
	}

	priv->hi3660_sd_ioset_jtag_sd_sel = of_get_named_gpio(np, "hi3660_sd_ioset_jtag_sd_sel", 0);
	if (!gpio_is_valid(priv->hi3660_sd_ioset_jtag_sd_sel)) {
		dev_info(host->dev, "sd_ioset_jtag_sd_sel not available\n");
		priv->hi3660_sd_ioset_jtag_sd_sel = -1;
	}

	if (of_find_property(np, "hi6250-timing-65M", NULL)) {
		hs_dwmmc_caps[DW_MCI_SDIO_ID] |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50);
		hs_timing_config[2][5][0] = 535000000;
		printk(KERN_ERR "exit setup timing clock 65M.\n");
	}

	if (of_find_property(np, "wifi_sdio_sdr104_156M", (int *)NULL)) {
		hs_dwmmc_caps[DW_MCI_SDIO_ID] |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104);
		hs_timing_config[2][6][1] = 9;
		hs_timing_config[2][6][4] = 19;
		hs_timing_config[2][6][6] = 160000000;
		host->wifi_sdio_sdr104_160M = 0xaaaa;
		printk(KERN_ERR "set berlin sdio sdr104 156M.\n");
	}

	config_sdr104_192m(np);

	if (of_find_property(np, "wifi_sdio_sdr104_177M", (int *)NULL)) {
		hs_dwmmc_caps[DW_MCI_SDIO_ID] |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104);
                hs_timing_config[2][6][1] = 8;
                hs_timing_config[2][6][4] = 17;
                hs_timing_config[2][6][6] = 177777777;
                host->wifi_sdio_sdr104_177M = 0xaaaa;
                printk(KERN_ERR "set berlin sdio sdr104 177M.\n");
        }

	if (of_property_read_u32(np, "hisi,bus_hz", &value)) {
		dev_info(host->dev, "bus_hz property not found, using " "value of 0 as default\n");
		value = 0;
	}
	priv->priv_bus_hz = value;
	dev_info(host->dev, "dts bus_hz = %d \n", priv->priv_bus_hz);

	value = 0;
	if (of_property_read_u32(np, "cd-vol", &value)) {
		dev_info(host->dev, "cd-vol property not found, using " "value of 0 as default\n");
		value = 0;
	}
	priv->cd_vol = value;
	dev_info(host->dev, "dts cd-vol = %d \n", priv->cd_vol);


#ifdef CONFIG_MMC_DW_MUX_SDSIM
	value = 0;
	if (of_property_read_u32(np, "mux-sdsim", &value)) {
		dev_info(host->dev, "mux-sdsim property not found, using " "value of 0 as default\n");
		value = 0;
	}
	priv->mux_sdsim = value;
	dev_info(host->dev, "dts mux-sdsim = %d \n", priv->mux_sdsim);

	priv->mux_sdsim_vcc_status = MUX_SDSIM_VCC_STATUS_2_9_5_V;

	if(1 == priv->mux_sdsim)
	{

		value = 0;
		if (of_property_read_u32(np, "vmmcmosen_switch", &value)) {
			dev_info(host->dev, "vmmcmosen_switch property not found, using " "value of 0 as default\n");
			value = 0;
		}
		dev_info(host->dev, "vmmcmosen_switch = 0x%x \n", value);
		if( 0x01 == value )
		{
			host->sd_vmmcmosen_switch = 1;
		}
		else
		{
			host->sd_vmmcmosen_switch = 0;
		}


		value = SD_CLK_DRIVER_DEFAULT;
		if (of_property_read_u32(np, "driverstrength_clk", &value)) {
			dev_info(host->dev, "driverstrength_clk property not found, using " "value of SD_CLK_DRIVER_DEFAULT as default\n");
			value = SD_CLK_DRIVER_DEFAULT;
		}
		sd_clk_driver_strength = value;
		dev_info(host->dev, "driverstrength_clk = 0x%x \n", sd_clk_driver_strength);

		value = SD_CMD_DRIVER_DEFAULT;
		if (of_property_read_u32(np, "driverstrength_cmd", &value)) {
			dev_info(host->dev, "driverstrength_cmd property not found, using " "value of SD_CMD_DRIVER_DEFAULT as default\n");
			value = SD_CMD_DRIVER_DEFAULT;
		}
		sd_cmd_driver_strength = value;
		dev_info(host->dev, "driverstrength_cmd = 0x%x \n", sd_cmd_driver_strength);

		value = SD_DATA_DRIVER_DEFAULT;
		if (of_property_read_u32(np, "driverstrength_data", &value)) {
			dev_info(host->dev, "driverstrength_data property not found, using " "value of SD_DATA_DRIVER_DEFAULT as default\n");
			value = SD_DATA_DRIVER_DEFAULT;
		}
		sd_data_driver_strength = value;
		dev_info(host->dev, "driverstrength_data = 0x%x \n", sd_data_driver_strength);
	}
#endif

	if (of_find_property(np, "sdio_support_uhs", NULL))
		hs_dwmmc_caps[DW_MCI_SDIO_ID] |= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104);

	/* find out mmc_bus_clk supported for hi3650 FPGA */
	if (of_property_read_u32(np, "board-mmc-bus-clk", &(priv->dw_mmc_bus_clk))) {
		dev_info(host->dev, "board mmc_bus_clk property not found, " "assuming asic board is available\n");

		priv->dw_mmc_bus_clk = 0;
	}
	dev_info(host->dev, "######board-mmc-bus-clk is %x \n", priv->dw_mmc_bus_clk);

	/* find out voltage switch supported by gpio for hi3650 FPGA */
	priv->dw_voltage_switch_gpio = of_get_named_gpio(np, "board-sd-voltage-switch-gpio", 0);
	if (!gpio_is_valid(priv->dw_voltage_switch_gpio)) {
		dev_info(host->dev, "board-sd-voltage-switch-gpio not available\n");
		priv->dw_voltage_switch_gpio = SDMMC_ASIC_PLATFORM;
	}
	dev_info(host->dev, "######dw_voltage_switch_gpio is %d\n", priv->dw_voltage_switch_gpio);

	return 0;
}

static irqreturn_t dw_mci_hs_card_detect(int irq, void *data)
{
	struct dw_mci *host = (struct dw_mci *)data;
	host->sd_reinit = 0;
	host->sd_hw_timeout = 0;
	host->flags &= ~DWMMC_IN_TUNING;
	host->flags &= ~DWMMC_TUNING_DONE;

	queue_work(host->card_workqueue, &host->card_work);
	return IRQ_HANDLED;
};

#ifdef CONFIG_MMC_DW_MUX_SDSIM
static int first_mux_sdsim_probe_init = 1;
#endif

static int dw_mci_hs_get_cd(struct dw_mci *host, u32 slot_id)
{
	unsigned int status;
	struct dw_mci_hs_priv_data *priv = host->priv;

	/* cd_vol = 1 means sdcard gpio detect pin active-high */
	if (priv->cd_vol)
		status = !gpio_get_value(priv->gpio_cd);
	else	/* cd_vol = 0 means sdcard gpio detect pin active-low */
		status = gpio_get_value(priv->gpio_cd);

	dev_info(host->dev, " sd status = %d from gpio_get_value(gpio_cd).\n", status);

	/*If sd to jtag func enabled, make the SD always not present */
	if ((host->hw_mmc_id == DWMMC_SD_ID) && (dw_mci_check_himntn(HIMNTN_SD2JTAG) || dw_mci_check_himntn(HIMNTN_SD2DJTAG))) {
		dev_info(host->dev, " sd status set to 1 here because jtag is enabled.\n");
		status = 1;
	}

#ifdef CONFIG_MMC_DW_MUX_SDSIM
	if (host->hw_mmc_id == DWMMC_SD_ID)
	{
		if(0 == first_mux_sdsim_probe_init)
		{
			status = sd_sim_detect_run(host,status,MODULE_SD,SLEEP_MS_TIME_FOR_DETECT_UNSTABLE);
		}
		else
		{
			dev_info(host->dev, " jump sd_sim_detect_run because first_mux_sdsim_probe_init now.\n");
			first_mux_sdsim_probe_init = 0;
		}
	}
#endif

	dev_info(host->dev, " sd status = %d\n", status);
	return status;
}

static int dw_mci_hs_cd_detect_init(struct dw_mci *host)
{
	struct device_node *np = host->dev->of_node;
	int gpio;
	int err;

	if (host->pdata->quirks & DW_MCI_QUIRK_BROKEN_CARD_DETECTION)
		return 0;

	gpio = of_get_named_gpio(np, "cd-gpio", 0);
	if (gpio_is_valid(gpio)) {
		if (devm_gpio_request_one(host->dev, gpio, GPIOF_IN, "dw-mci-cd")) {
			dev_err(host->dev, "gpio [%d] request failed\n", gpio);
		} else {
			struct dw_mci_hs_priv_data *priv = host->priv;
			u32 shared_irq = 0;
			priv->gpio_cd = gpio;
			host->pdata->get_cd = dw_mci_hs_get_cd;
			if (of_property_read_u32(np, "shared-irq", &shared_irq)) {
				dev_info(host->dev, "shared-irq property not found, using " "shared_irq of 0 as default\n");
				shared_irq = 0;
			}

			if (shared_irq) {
				err = devm_request_irq(host->dev, gpio_to_irq(gpio), dw_mci_hs_card_detect,
									IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND | IRQF_SHARED,
									DRIVER_NAME, host);
			} else {
				err = devm_request_irq(host->dev, gpio_to_irq(gpio), dw_mci_hs_card_detect,
									IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
									DRIVER_NAME, host);
			}

			if (err)
				dev_warn(mmc_dev(host->dev), "request gpio irq error\n");
		}

	} else {
		dev_info(host->dev, "cd gpio not available");
	}

	return 0;
}

static int hs_dwmmc_card_busy(struct dw_mci *host)
{
	if ((mci_readl(host, STATUS) & SDMMC_STATUS_BUSY) || host->cmd || host->data || host->mrq || (host->state != STATE_IDLE)) {
		dev_vdbg(host->dev, " card is busy!");
		return 1;
	}

	return 0;
}

static int dw_mci_3_3v_signal_voltage_switch(struct dw_mci_slot *slot)
{
	struct dw_mci *host = slot->host;
	struct dw_mci_hs_priv_data *priv = host->priv;
	u32 reg;
	int ret = 0;
	u32 id = (u32)(slot->id);

#ifdef CONFIG_MMC_DW_MUX_SDSIM
	if (host->hw_mmc_id == DWMMC_SD_ID && priv->mux_sdsim)
		return ret;
#endif
	ret = dw_mci_set_sel18(host, 0);
	if (ret) {
		dev_err(host->dev, " dw_mci_set_sel18 error!\n");
		return ret;
	}

	/* Wait for 5ms */
	usleep_range(5000, 5500);

	/* only for SD voltage switch on hi3650 FPGA */
	if (SDMMC_ASIC_PLATFORM != priv->dw_voltage_switch_gpio) {
		(void)gpio_request(priv->dw_voltage_switch_gpio, "board-sd-voltage-switch-gpio");
		/* set the voltage to 3V for SD IO */
		gpio_direction_output(priv->dw_voltage_switch_gpio, 1);
		gpio_free(priv->dw_voltage_switch_gpio);
	} else {
		if (host->vqmmc) {
			ret = regulator_set_voltage(host->vqmmc, 2950000, 2950000);
			if (ret) {
				dev_warn(host->dev, "Switching to 3.3V signalling " "voltage failed\n");
				return -EIO;
			}
		} else {
			reg = mci_readl(slot->host, UHS_REG);
			reg &= ~((u32)(0x1 << id));
			mci_writel(slot->host, UHS_REG, reg);
		}
	}

	/* Wait for 5ms */
	usleep_range(5000, 5500);

	return ret;
}

static int dw_mci_1_8v_signal_voltage_switch(struct dw_mci_slot *slot)
{
	unsigned long loop_count = 0x100000;
	struct dw_mci *host = slot->host;
	struct dw_mci_hs_priv_data *priv = host->priv;
	int ret;
	int intrs;

	/* disable interrupt upon voltage switch. handle interrupt here
	 *  and DO NOT triggle irq */
	mci_writel(host, CTRL, (mci_readl(host, CTRL) & ~SDMMC_CTRL_INT_ENABLE));

	/* stop clock */
	mci_writel(host, CLKENA, (0x0 << 0));
	mci_writel(host, CMD, SDMMC_CMD_ONLY_CLK | SDMMC_CMD_VOLT_SWITCH);
	do {
		if (!(mci_readl(host, CMD) & SDMMC_CMD_START))
			break;
		loop_count--;
	} while (loop_count);

	if (!loop_count)
		dev_err(host->dev, " disable clock failed in voltage_switch\n");

	mmiowb();

	if (SDMMC_ASIC_PLATFORM != priv->dw_voltage_switch_gpio) {
		(void)gpio_request(priv->dw_voltage_switch_gpio, "board-sd-voltage-switch-gpio");
		/* set the voltage to 3V for SD IO */
		(void)gpio_direction_output(priv->dw_voltage_switch_gpio, 0);
		gpio_free(priv->dw_voltage_switch_gpio);
	} else {
		if (host->vqmmc) {
			ret = regulator_set_voltage(host->vqmmc, 1800000, 1800000);
			if (ret) {
				dev_warn(host->dev, "Switching to 1.8V signalling " "voltage failed\n");
				return -EIO;
			}
		}
	}

	/* Wait for 5ms */
	usleep_range(10000, 10500);

	ret = dw_mci_set_sel18(host, 1);
	if (ret) {
		dev_err(host->dev, " dw_mci_set_sel18 error!\n");
		return ret;
	}

	/* start clock */
	mci_writel(host, CLKENA, (0x1 << 0));
	mci_writel(host, CMD, SDMMC_CMD_ONLY_CLK | SDMMC_CMD_VOLT_SWITCH);
	loop_count = 0x100000;
	do {
		if (!(mci_readl(host, CMD) & SDMMC_CMD_START))
			break;
		loop_count--;
	} while (loop_count);

	if (!loop_count)
		dev_err(host->dev, " enable clock failed in voltage_switch\n");

	/* poll cd interrupt */
	loop_count = 0x100000;
	do {
		intrs = mci_readl(host, RINTSTS);
		if (intrs & SDMMC_INT_CMD_DONE) {
			dev_err(host->dev, " cd 0x%x in voltage_switch\n", intrs);
			mci_writel(host, RINTSTS, intrs);
			break;
		}
		loop_count--;
	} while (loop_count);

	if (!loop_count)
		dev_err(host->dev, " poll cd failed in voltage_switch\n");

	/* enable interrupt */
	mci_writel(host, CTRL, (mci_readl(host, CTRL) | SDMMC_CTRL_INT_ENABLE));

	mmiowb();

	return ret;
}

static int dw_mci_priv_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	int ret = 0;

	/* only sd need to switch voltage */
	if (slot->host->hw_mmc_id != DWMMC_SD_ID)
		return ret;

	pm_runtime_get_sync(mmc_dev(mmc));

	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		ret = dw_mci_3_3v_signal_voltage_switch(slot);
	else if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)
		ret = dw_mci_1_8v_signal_voltage_switch(slot);

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return ret;
}

void dw_mci_set_timeout(struct dw_mci *host)
{
	/* timeout (maximum) */
	mci_writel(host, TMOUT, 0xffffffff);
}

static void dw_mci_hs_tuning_clear_flags(struct dw_mci *host)
{
	host->tuning_sample_flag = 0;
}

static bool dw_mci_hi3xxx_wait_reset(struct device *dev, struct dw_mci *host, unsigned int reset_val)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);
	unsigned int ctrl;

	ctrl = mci_readl(host, CTRL);
	ctrl |= reset_val;
	mci_writel(host, CTRL, ctrl);

	/* wait till resets clear */
	do {
		if (!(mci_readl(host, CTRL) & reset_val))
			return true;
	} while (time_before(jiffies, timeout));

	dev_err(dev, "Timeout resetting block (ctrl %#x)\n", ctrl);

	return false;
}

static bool mci_hi3xxx_wait_reset(struct dw_mci *host)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);
	unsigned int ctrl;

	mci_writel(host, CTRL, (SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET | SDMMC_CTRL_DMA_RESET));

	/* wait till resets clear */
	do {
		ctrl = mci_readl(host, CTRL);
		if (!(ctrl & (SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET | SDMMC_CTRL_DMA_RESET)))
			return true;
	} while (time_before(jiffies, timeout));

	dev_err(host->dev, "Timeout resetting block (ctrl %#x)\n", ctrl);

	return false;
}

static void dw_mci_hi3xxx_mci_send_cmd(struct dw_mci *host, u32 cmd, u32 arg)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	unsigned int cmd_status = 0;
	int try = 3;

	mci_writel(host, CMDARG, arg);
	wmb();
	mci_writel(host, CMD, SDMMC_CMD_START | cmd);

	do {
		while (time_before(jiffies, timeout)) {
			cmd_status = mci_readl(host, CMD);
			if (!(cmd_status & SDMMC_CMD_START))
				return;
		}

		dw_mci_hi3xxx_wait_reset(host->dev, host, SDMMC_CTRL_RESET);
		mci_writel(host, CMD, SDMMC_CMD_START | cmd);
		timeout = jiffies + msecs_to_jiffies(100);
	} while (--try);

	dev_err(host->dev, "hi3xxx_dw_mmc " "Timeout sending command (cmd %#x arg %#x status %#x)\n", cmd, arg, cmd_status);
}

void dw_mci_hi3xxx_work_fail_reset(struct dw_mci *host)
{
	struct dw_mci_hs_priv_data *priv = host->priv;

	unsigned int retval = 0;
	unsigned int ctype;
	unsigned int clkena;
	unsigned int clkdiv;
	unsigned int uhs_reg;
	unsigned int uhs_reg_ext;
	unsigned int enable_shift;
	unsigned int gpio;
	unsigned int fifoth;
	unsigned int timeout;
	unsigned int cardthrctrl;
	unsigned int _rintsts;
	unsigned int _tcbcnt;
	unsigned int _tbbcnt;
	unsigned int _fifoth;

	if ((priv->id != DW_MCI_SD_ID) && (priv->id != DW_MCI_SDIO_ID)) {
		dev_err(host->dev, "Not support now, return\n");
		return;
	}

	dev_err(host->dev, "Start to reset SDIO IP\n");
	mci_writel(host, CTRL, (mci_readl(host, CTRL) & (~INT_ENABLE)));
	mci_writel(host, INTMASK, 0);

	mci_writel(host, RINTSTS, INTMSK_ALL);

#ifdef CONFIG_MMC_DW_IDMAC
	if (SDMMC_32_BIT_DMA == host->dma_64bit_address)
		mci_writel(host, IDSTS, IDMAC_INT_CLR);
	else
		mci_writel(host, IDSTS64, IDMAC_INT_CLR);
#endif

	ctype = mci_readl(host, CTYPE);
	clkena = mci_readl(host, CLKENA);
	clkdiv = mci_readl(host, CLKDIV);
	fifoth = mci_readl(host, FIFOTH);
	timeout = mci_readl(host, TMOUT);
	cardthrctrl = mci_readl(host, CDTHRCTL);
	uhs_reg = mci_readl(host, UHS_REG);
	uhs_reg_ext = mci_readl(host, UHS_REG_EXT);
	enable_shift = mci_readl(host, ENABLE_SHIFT);
	gpio = mci_readl(host, GPIO);

	_rintsts = mci_readl(host, RINTSTS);
	_tcbcnt = mci_readl(host, TCBCNT);
	_tbbcnt = mci_readl(host, TBBCNT);
	retval = mci_readl(host, CTRL);

	dev_info(host->dev, "before  ip reset: CTRL=%x, UHS_REG_EXT=%x, ENABLE_SHIFT=%x,"
			" GPIO=%x, CLKEN=%d, CLKDIV=%d, TMOUT=%x, RINTSTS=%x, "
			" TCBCNT=%x, TBBCNT=%x,FIFOTH=%x \n", retval, uhs_reg_ext,
			enable_shift, gpio, clkena, clkdiv, timeout, _rintsts, _tcbcnt, _tbbcnt, fifoth);

	udelay(20);

	dw_mci_hs_set_rst_m(host, 1);
	dw_mci_hs_set_controller(host, 1);

	if (!IS_ERR(host->ciu_clk))
		clk_disable_unprepare(host->ciu_clk);

	dw_mci_hs_set_rst_m(host, 0);

	if (!IS_ERR(host->ciu_clk)) {
		if (clk_prepare_enable(host->ciu_clk))
			dev_err(host->dev, "ciu_clk clk_prepare_enable failed\n");
	}

	dw_mci_hs_set_controller(host, 0);

	udelay(20);
	mci_hi3xxx_wait_reset(host);

	mci_writel(host, CTYPE, ctype);
	mci_writel(host, FIFOTH, fifoth);
	mci_writel(host, TMOUT, timeout);
	mci_writel(host, CDTHRCTL, cardthrctrl);
	mci_writel(host, UHS_REG, uhs_reg);
	mci_writel(host, GPIO, 0x0);
	udelay(10);
	mci_writel(host, UHS_REG_EXT, uhs_reg_ext);
	mci_writel(host, ENABLE_SHIFT, enable_shift);
	mci_writel(host, GPIO, gpio | GPIO_CLK_ENABLE);

	mci_writel(host, BMOD, SDMMC_IDMAC_SWRESET);
#ifdef CONFIG_MMC_DW_IDMAC
	if (SDMMC_32_BIT_DMA == host->dma_64bit_address) {
		mci_writel(host, IDSTS, IDMAC_INT_CLR);
		mci_writel(host, IDINTEN, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI | SDMMC_IDMAC_INT_TI);
		mci_writel(host, DBADDR, host->sg_dma);
	} else {
		mci_writel(host, IDSTS64, IDMAC_INT_CLR);
		mci_writel(host, IDINTEN64, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI | SDMMC_IDMAC_INT_TI);
		mci_writel(host, DBADDRL, host->sg_dma & 0xffffffff);
		mci_writel(host, DBADDRU,(u64)host->sg_dma >> 32);
	}
#endif


	mci_writel(host, RINTSTS, INTMSK_ALL);
	mci_writel(host, INTMASK, 0);
	mci_writel(host, RINTSTS, INTMSK_ALL);
#ifdef CONFIG_MMC_DW_IDMAC
	if (SDMMC_32_BIT_DMA == host->dma_64bit_address)
		mci_writel(host, IDSTS, IDMAC_INT_CLR);
	else
		mci_writel(host, IDSTS64, IDMAC_INT_CLR);
#endif
	mci_writel(host, INTMASK, SDMMC_INT_CMD_DONE | SDMMC_INT_DATA_OVER | SDMMC_INT_TXDR | SDMMC_INT_RXDR | DW_MCI_ERROR_FLAGS | SDMMC_INT_CD);
	mci_writel(host, CTRL, SDMMC_CTRL_INT_ENABLE);	/* Enable mci interrupt */

	/* disable clock */
	mci_writel(host, CLKENA, 0);
	mci_writel(host, CLKSRC, 0);

	/* inform CIU */
	dw_mci_hi3xxx_mci_send_cmd(host, SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);

	/* set clock to desired speed */
	mci_writel(host, CLKDIV, clkdiv);

	/* inform CIU */
	dw_mci_hi3xxx_mci_send_cmd(host, SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);

	mci_writel(host, CLKENA, clkena);

	/* inform CIU */
	dw_mci_hi3xxx_mci_send_cmd(host, SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);

	retval = mci_readl(host, CTRL);
	_rintsts = mci_readl(host, RINTSTS);
	_tcbcnt = mci_readl(host, TCBCNT);
	_tbbcnt = mci_readl(host, TBBCNT);
	_fifoth = mci_readl(host, FIFOTH);
	uhs_reg_ext = mci_readl(host, UHS_REG_EXT);
	enable_shift = mci_readl(host, ENABLE_SHIFT);
	gpio = mci_readl(host, GPIO);

	dev_info(host->dev, "after  ip reset: CTRL=%x, UHS_REG_EXT=%x, ENABLE_SHIFT=%x, GPIO=%x, CLKEN=%d,"
			" CLKDIV=%d, TMOUT=%x, RINTSTS=%x, TCBCNT=%x, TBBCNT=%x,FIFOTH=%x \n",
			retval, uhs_reg_ext, enable_shift, gpio, clkena, clkdiv, timeout, _rintsts, _tcbcnt, _tbbcnt, _fifoth);
}

static void dw_mci_hs_tuning_set_flags(struct dw_mci *host, int sample, int ok)
{
	if (ok)
		host->tuning_sample_flag |= (1 << sample);
	else
		host->tuning_sample_flag &= ~((unsigned)(1 << sample));
}

/* By tuning, find the best timing condition
 *  1 -- tuning is not finished. And this function should be called again
 *  0 -- Tuning successfully.
 *    If this function be called again, another round of tuning would be start
 *  -1 -- Tuning failed. Maybe slow down the clock and call this function again
 */
static int dw_mci_hs_tuning_find_condition(struct dw_mci *host, int timing)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int id = priv->id;
	int sample_min, sample_max;
	int i, j;
	int ret = 0;
	int mask, mask_lenth;
	int d_value = 0;

	if (host->hw_mmc_id == DWMMC_SD_ID) {
		d_value = host->current_div - hs_timing_config[id][timing][1];
		if (timing == MMC_TIMING_SD_HS) {
			sample_max = hs_timing_config[id][timing][4] + d_value;
			sample_min = hs_timing_config[id][timing][5] + d_value;
		} else if ((timing == MMC_TIMING_UHS_SDR50) || (timing == MMC_TIMING_UHS_SDR104)
			   || (timing == MMC_TIMING_MMC_HS200)) {
			sample_max = hs_timing_config[id][timing][4] + 2 * d_value;
			sample_min = hs_timing_config[id][timing][5];
		} else {
			sample_max = hs_timing_config[id][timing][4];
			sample_min = hs_timing_config[id][timing][5];
		}
	} else {
		sample_max = hs_timing_config[id][timing][4];
		sample_min = hs_timing_config[id][timing][5];
	}

	if (sample_max == sample_min) {
		host->tuning_init_sample = (sample_max + sample_min) / 2;
		dw_mci_hs_set_timing(host, id, timing, host->tuning_init_sample, host->current_div);
		dev_info(host->dev, "no need tuning: timing is %d, tuning sample = %d", timing, host->tuning_init_sample);
		return 0;
	}

	if (-1 == host->tuning_current_sample) {

		dw_mci_hs_tuning_clear_flags(host);

		/* set the first sam del as the min_sam_del */
		host->tuning_current_sample = sample_min;
		/* a trick for next "++" */
		host->tuning_current_sample--;
	}

	if (host->tuning_current_sample >= sample_max) {
		/* tuning finish, select the best sam_del */

		/* set sam del to -1, for next tuning */
		host->tuning_current_sample = -1;

		host->tuning_init_sample = -1;
		for (mask_lenth = (((sample_max - sample_min) >> 1) << 1) + 1; mask_lenth >= 1; mask_lenth -= 2) {

			mask = (1 << mask_lenth) - 1;
			for (i = (sample_min + sample_max - mask_lenth + 1) / 2, j = 1;
					(i <= sample_max - mask_lenth + 1) && (i >= sample_min);
						i = ((sample_min + sample_max - mask_lenth + 1) / 2) + ((j % 2) ? -1 : 1) * (j / 2)) {
				if ((host->tuning_sample_flag & ((unsigned int)mask << (unsigned int)i)) == ((unsigned int)mask << (unsigned int)i)) {
					host->tuning_init_sample = i + mask_lenth / 2;
					break;
				}

				j++;
			}

			if (host->tuning_init_sample != -1) {
				if ((host->hw_mmc_id == DWMMC_SD_ID) && (mask_lenth < 3) && (drv_data->slowdown_clk)) {
					dev_info(host->dev, "sd card tuning need slow " "down clk, timing is %d, " "tuning_flag = 0x%x \n",
							timing, host->tuning_sample_flag);
					return -1;
				} else {
					dev_info(host->dev, "tuning OK: timing is " "%d, tuning sample = " "%d, tuning_flag = 0x%x",
							timing, host->tuning_init_sample, host->tuning_sample_flag);
					ret = 0;
#ifdef CONFIG_SD_TIMEOUT_RESET
					host->set_sd_data_tras_timeout = 0;
					host->set_sdio_data_tras_timeout = 0;
#endif
					break;
				}
			}
		}

		if (-1 == host->tuning_init_sample) {
			host->tuning_init_sample = (sample_min + sample_max) / 2;
			dev_info(host->dev, "tuning err: no good sam_del, " "timing is %d, tuning_flag = 0x%x", timing, host->tuning_sample_flag);
#ifdef CONFIG_HUAWEI_EMMC_DSM
			if (host->hw_mmc_id == DWMMC_EMMC_ID) {
				DSM_EMMC_LOG(host->cur_slot->mmc, DSM_EMMC_TUNING_ERROR, "%s:eMMC tuning error: timing is %d, tuning_flag = 0x%x\n", __FUNCTION__, timing, host->tuning_sample_flag);
			}
#endif
			ret = -1;
		}

		dw_mci_hs_set_timing(host, id, timing, host->tuning_init_sample, host->current_div);
		return ret;
	} else {
		host->tuning_current_sample++;
		dw_mci_hs_set_timing(host, id, timing, host->tuning_current_sample, host->current_div);
		return 1;
	}

}

static void dw_mci_hs_tuning_set_current_state(struct dw_mci *host, int ok)
{
	dw_mci_hs_tuning_set_flags(host, host->tuning_current_sample, ok);
}

#ifdef CONFIG_MMC_DW_SD_CLK_SLOWDOWN
static int dw_mci_hs_slowdown_clk(struct dw_mci *host, int timing)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int id = priv->id;

	host->current_div += 2;

	/* slow down up to half of original freq */
	if (host->current_div > 2 * hs_timing_config[id][timing][1]) {
		host->current_div = 2 * hs_timing_config[id][timing][1];
		return -1;
	} else {
		dev_info(host->dev, "begin slowdown clk, current_div=%d\n", host->current_div);

		dw_mci_hs_set_timing(host, id, timing, host->tuning_init_sample, host->current_div);
	}

	return 0;
}
#endif

int dw_mci_sdio_wakelog_switch(struct mmc_host *mmc, bool enable)
{
	struct dw_mci_slot *slot = NULL;
	if (!mmc)
		return -1;

	slot = mmc_priv(mmc);
	if (!slot)
		return -1;

	if (enable)
		slot->sdio_wakelog_switch = 1;
	else
		slot->sdio_wakelog_switch = 0;

	slot->sdio_wakelog_switch = slot->sdio_wakelog_switch && (MMC_CAP2_SUPPORT_WIFI & (mmc->caps2));
	return slot->sdio_wakelog_switch;
}

EXPORT_SYMBOL(dw_mci_sdio_wakelog_switch);

#ifdef CONFIG_SD_SDIO_CRC_RETUNING

void dw_mci_hisi_change_timing_config_taurus(struct dw_mci *host)
{
	if (DWMMC_SD_ID == host->hw_mmc_id) {
		memcpy(hs_timing_config[DWMMC_SD_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_taurus_sd_ppll2, sizeof(hs_timing_config_libra_sd_ppll2));/* unsafe_function_ignore: memcpy */
		host->use_samdly_range[0] = 6;
		host->use_samdly_range[1] = 8;
		host->enable_shift_range[0] = 4;
		host->enable_shift_range[1] = 8;
	} else if (DWMMC_SDIO_ID == host->hw_mmc_id) {
		memcpy(hs_timing_config[DWMMC_SDIO_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_taurus_sdio_ppll2, sizeof(hs_timing_config_libra_sdio_ppll2));/* unsafe_function_ignore: memcpy */
		host->use_samdly_range[0] = 8;
		host->use_samdly_range[1] = 14;
		host->enable_shift_range[0] = 8;
		host->enable_shift_range[1] = 14;
	}
}

void dw_mci_hs_change_timing_config(struct dw_mci *host)
{
	struct device_node *np = host->dev->of_node;

	if (of_device_is_compatible(np, "hisilicon,cancer-dw-mshc")) {
		if(of_find_property(np, "cs_sd_timing_config_cancer", (int *)NULL)) {
			if (DWMMC_SD_ID == host->hw_mmc_id) {
				memcpy(hs_timing_config[DWMMC_SD_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_cancer_cs_sd_ppll2, sizeof(hs_timing_config_cancer_cs_sd_ppll2));/* unsafe_function_ignore: memcpy */
				host->use_samdly_range[0] = 5;
				host->use_samdly_range[1] = 8;
				host->enable_shift_range[0] = 4;
				host->enable_shift_range[1] = 8;
			}else if (DWMMC_SDIO_ID == host->hw_mmc_id) {
				memcpy(hs_timing_config[DWMMC_SDIO_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_cancer_cs_sdio_ppll2, sizeof(
	hs_timing_config_cancer_cs_sdio_ppll2));/* unsafe_function_ignore: memcpy */
				host->use_samdly_range[0] = 5;
				host->use_samdly_range[1] = 8;
				host->enable_shift_range[0] = 4;
				host->enable_shift_range[1] = 8;
			}
		}else {
			if (DWMMC_SD_ID == host->hw_mmc_id) {
				memcpy(hs_timing_config[DWMMC_SD_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_cancer_sd_ppll2, sizeof(hs_timing_config_cancer_sd_ppll2));/* unsafe_function_ignore: memcpy */
				host->use_samdly_range[0] = 5;
				host->use_samdly_range[1] = 8;
				host->enable_shift_range[0] = 3;
				host->enable_shift_range[1] = 8;
			}else if (DWMMC_SDIO_ID == host->hw_mmc_id) {
				memcpy(hs_timing_config[DWMMC_SDIO_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_cancer_sdio_ppll2, sizeof(
	hs_timing_config_cancer_sdio_ppll2));/* unsafe_function_ignore: memcpy */
				host->use_samdly_range[0] = 5;
				host->use_samdly_range[1] = 8;
				host->enable_shift_range[0] = 3;
				host->enable_shift_range[1] = 8;
			}
		}
	} else if (of_device_is_compatible(np, "hisilicon,libra-dw-mshc")) {
		if (DWMMC_SD_ID == host->hw_mmc_id) {
			memcpy(hs_timing_config[DWMMC_SD_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_libra_sd_ppll2, sizeof(hs_timing_config_libra_sd_ppll2));/* unsafe_function_ignore: memcpy */
			host->use_samdly_range[0] = 5;
                        host->use_samdly_range[1] = 8;
                        host->enable_shift_range[0] = 3;
                        host->enable_shift_range[1] = 8;
		} else if (DWMMC_SDIO_ID == host->hw_mmc_id) {
			memcpy(hs_timing_config[DWMMC_SDIO_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_libra_sdio_ppll2, sizeof(hs_timing_config_libra_sdio_ppll2));/* unsafe_function_ignore: memcpy */
			host->use_samdly_range[0] = 5;
			host->use_samdly_range[1] = 8;
			host->enable_shift_range[0] = 3;
			host->enable_shift_range[1] = 8;
		}
	} else if (of_device_is_compatible(np, "hisilicon,taurus-dw-mshc")) {
		dw_mci_hisi_change_timing_config_taurus(host);
	} else {
		 if (DWMMC_SD_ID == host->hw_mmc_id) {
			memcpy(hs_timing_config[DWMMC_SD_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_kirin970_cs_sd_ppll3, sizeof(hs_timing_config_kirin970_cs_sd_ppll3));/* unsafe_function_ignore: memcpy */
		} else if (DWMMC_SDIO_ID == host->hw_mmc_id) {
			memcpy(hs_timing_config[DWMMC_SDIO_ID][MMC_TIMING_UHS_SDR104], hs_timing_config_kirin970_cs_sdio_ppll3, sizeof(hs_timing_config_kirin970_cs_sdio_ppll3));/* unsafe_function_ignore: memcpy */
		}
	}
}
#endif
static int dw_mci_hs_tuning_move(struct dw_mci *host, int timing, int start)
{
	struct dw_mci_hs_priv_data *priv = host->priv;
	int id = priv->id;
	int sample_min, sample_max;
	int loop;
	struct dw_mci_slot *slot = host->cur_slot;

	sample_max = hs_timing_config[id][timing][4];
	sample_min = hs_timing_config[id][timing][5];

	if (sample_max == sample_min) {
		dev_info(host->dev, "id = %d, tuning move return\n", id);
		return 0;
	}

	if (start) {
		host->tuning_move_count = 0;
	}

	for (loop = 0; loop < 2; loop++) {
		host->tuning_move_count++;
		host->tuning_move_sample = host->tuning_init_sample + ((host->tuning_move_count % 2) ? 1 : -1) * (host->tuning_move_count / 2);

		if ((host->tuning_move_sample > sample_max) || (host->tuning_move_sample < sample_min)) {
			continue;
		} else {
			break;
		}
	}

	if ((host->tuning_move_sample > sample_max) || (host->tuning_move_sample < sample_min)) {
		dw_mci_hs_set_timing(host, id, timing, host->tuning_init_sample, host->current_div);
		dev_info(host->dev, "id = %d, tuning move end to init del_sel %d\n", id, host->tuning_init_sample);
		return 0;
	} else {
		dw_mci_hs_set_timing(host, id, timing, host->tuning_move_sample, host->current_div);

		if (!(slot && slot->sdio_wakelog_switch))
			dev_info(host->dev, "id = %d, tuning move to current del_sel %d\n", id, host->tuning_move_sample);
		return 1;
	}
}

#define EMMC_PATTERN_ADDRESS (384*2)
int dw_mci_priv_execute_tuning(struct dw_mci_slot *slot, u32 opcode, struct dw_mci_tuning_data *tuning_data)
{
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci *host = slot->host;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	unsigned int tuning_loop = MAX_TUNING_LOOP;
	const u8 *tuning_blk_pattern;
	int ret = 0;
	u8 *tuning_blk;
	int blksz;

	int id = host->hw_mmc_id;
	u32 arg = 0;
	unsigned int flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
		if (mmc->ios.bus_width == MMC_BUS_WIDTH_8) {
			tuning_blk_pattern = tuning_blk_pattern_8bit;
			blksz = 128;
		} else if (mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
			tuning_blk_pattern = tuning_blk_pattern_4bit;
			blksz = 64;
		} else
			return -EINVAL;
	} else if (opcode == MMC_SEND_TUNING_BLOCK) {
		tuning_blk_pattern = tuning_blk_pattern_4bit;
		blksz = 64;
	} else if (opcode == MMC_READ_SINGLE_BLOCK) {
		if (id == 0)			/* emmc ddr50 */
			arg = EMMC_PATTERN_ADDRESS;

		blksz = 512;
	} else if (opcode == SD_IO_RW_EXTENDED) {
		arg = 0x200004;
		flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

		blksz = 4;
	} else {
		dev_err(&mmc->class_dev, "Undefined command(%d) for tuning\n", opcode);
		return -EINVAL;
	}

	tuning_blk = kzalloc(blksz, GFP_KERNEL);
	if (!tuning_blk)
		return -ENOMEM;

	if ((!drv_data->tuning_find_condition) || (!drv_data->tuning_set_current_state)) {
		dev_err(&mmc->class_dev, "no tuning find condition method \n");
		goto out;
	}

	pm_runtime_get_sync(mmc_dev(mmc));

	host->flags |= DWMMC_IN_TUNING;
	host->flags &= ~DWMMC_TUNING_DONE;

	do {
		struct mmc_request mrq = { NULL };
		struct mmc_command cmd = { 0 };
		struct mmc_data data = { 0 };
		struct scatterlist sg;

		cmd.opcode = opcode;
		cmd.arg = arg;
		cmd.flags = flags;

		data.blksz = blksz;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;

		sg_init_one(&sg, tuning_blk, blksz);
		dw_mci_set_timeout(host);

		mrq.cmd = &cmd;
		mrq.stop = NULL;
		mrq.data = &data;

		ret = drv_data->tuning_find_condition(host, mmc->ios.timing);
		if (ret == -1) {
			if ((host->hw_mmc_id == DWMMC_SD_ID) && (drv_data->slowdown_clk)) {
				ret = drv_data->slowdown_clk(host, mmc->ios.timing);
				if (ret)
					break;
			} else {
				break;
			}
		} else if (0 == ret)
			break;

		mmc_wait_for_req(mmc, &mrq);

		if (!cmd.error && !data.error) {
			drv_data->tuning_set_current_state(host, 1);
		} else {
			drv_data->tuning_set_current_state(host, 0);
			dev_dbg(&mmc->class_dev, "Tuning error: cmd.error:%d, data.error:%d\n", cmd.error, data.error);
		}

	} while (tuning_loop--);

	host->flags &= ~DWMMC_IN_TUNING;
	if (!ret) {
		host->flags |= DWMMC_TUNING_DONE;
	}

	host->tuning_move_start = 1;
out:
	kfree(tuning_blk);

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return ret;
}


static const struct dw_mci_drv_data hs_drv_data = {
	.caps = hs_dwmmc_caps,
	.init = dw_mci_hs_priv_init,
	.set_ios = dw_mci_hs_set_ios,
	.setup_clock = dw_mci_hs_setup_clock,
	.prepare_command = dw_mci_hs_prepare_command,
	.parse_dt = dw_mci_hs_parse_dt,
	.cd_detect_init = dw_mci_hs_cd_detect_init,
	.tuning_find_condition = dw_mci_hs_tuning_find_condition,
	.tuning_set_current_state = dw_mci_hs_tuning_set_current_state,
	.tuning_move = dw_mci_hs_tuning_move,
#ifdef CONFIG_MMC_DW_SD_CLK_SLOWDOWN
	.slowdown_clk = dw_mci_hs_slowdown_clk,
#endif
	.execute_tuning = dw_mci_priv_execute_tuning,
	.start_signal_voltage_switch = dw_mci_priv_voltage_switch,
	.work_fail_reset = dw_mci_hi3xxx_work_fail_reset,
};

static const struct of_device_id dw_mci_hs_match[] = {
	{
	 .compatible = "hisilicon,hi3650-dw-mshc",
	 .data = &hs_drv_data,
	 },
	{
	 .compatible = "hisilicon,hi3635-dw-mshc",
	 .data = &hs_drv_data,
	 },
	{
	 .compatible = "hisilicon,hi6250-dw-mshc",
	 .data = &hs_drv_data,
	 },
	{
	 .compatible = "hisilicon,hi3660-dw-mshc",
	 .data = &hs_drv_data,
	 },
	{
	 .compatible = "hisilicon,kirin970-dw-mshc",
	 .data = &hs_drv_data,
	},
	{
	 .compatible = "hisilicon,cancer-dw-mshc",
	 .data = &hs_drv_data,
	},
	{
	.compatible = "hisilicon,libra-dw-mshc",
	.data = &hs_drv_data,
	},
	{
	.compatible = "hisilicon,taurus-dw-mshc",
	.data = &hs_drv_data,
	},
	{},
};

MODULE_DEVICE_TABLE(of, dw_mci_hs_match);

#if defined(CONFIG_HISI_DEBUG_FS)
extern void proc_sd_test_init(void);
#endif
int dw_mci_hs_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data = NULL;
	const struct of_device_id *match = NULL;
	int err;

#if defined(CONFIG_HISI_DEBUG_FS)
	proc_sd_test_init();
#endif

	match = of_match_node(dw_mci_hs_match, pdev->dev.of_node);
	if (!match)
		return -1;
	drv_data = match->data;

	err = dw_mci_hs_get_resource();
	if (err)
		return err;

	err = dw_mci_pltfm_register(pdev, drv_data);
	if (err)
		return err;

	/*when sdio1 used for via modem, disable pm runtime*/
	if (!of_property_read_bool(pdev->dev.of_node, "modem_sdio_enable")) {
		pm_runtime_set_active(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_suspend_ignore_children(&pdev->dev, 1);
	} else {
		pr_info("%s mmc/sdio device support via modem, disable pm_runtime on this device\n", __func__);
	}
#ifdef CONFIG_HUAWEI_DSM
	if (!dclient)
		dclient = dsm_register_client(&dsm_dw_mmc);
#endif
	return 0;
}

#ifdef CONFIG_SD_TIMEOUT_RESET
void dw_mci_disable_clk_08v(struct dw_mci *host)
{
	if (!IS_ERR(host->volt_hold_sd_clk) && (VOLT_HOLD_CLK_08V == host->volt_hold_clk_sd))
		clk_disable_unprepare(host->volt_hold_sd_clk);

	if (!IS_ERR(host->volt_hold_sdio_clk) && (VOLT_HOLD_CLK_08V == host->volt_hold_clk_sdio))
		clk_disable_unprepare(host->volt_hold_sdio_clk);
}

void dw_mci_enable_clk_08v(struct dw_mci *host)
{
	if (!IS_ERR(host->volt_hold_sd_clk) && (VOLT_HOLD_CLK_08V == host->volt_hold_clk_sd)) {
		if (clk_prepare_enable(host->volt_hold_sd_clk))
			dev_err(host->dev, "volt_hold_sd_clk clk_prepare_enable failed\n");
	}

	if (!IS_ERR(host->volt_hold_sdio_clk) && (VOLT_HOLD_CLK_08V == host->volt_hold_clk_sdio)) {
		if (clk_prepare_enable(host->volt_hold_sdio_clk))
			dev_err(host->dev, "volt_hold_sdio_clk clk_prepare_enable failed\n");
	}
}
#endif

#ifdef CONFIG_PM_SLEEP
static int dw_mci_hs_suspend(struct device *dev)
{
	int ret;
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_hs_priv_data *priv = host->priv;
	dev_info(host->dev, " %s ++ \n", __func__);

	pm_runtime_get_sync(dev);

	if (priv->gpio_cd) {
		disable_irq(gpio_to_irq(priv->gpio_cd));
		cancel_work_sync(&host->card_work);
		dev_info(host->dev, " disable gpio detect \n");
	}

	ret = dw_mci_suspend(host);
	if (ret)
		return ret;

	priv->old_timing = -1;
	priv->old_power_mode = MMC_POWER_OFF;
	if (!IS_ERR(host->biu_clk))
		clk_disable_unprepare(host->biu_clk);

	if (!IS_ERR(host->ciu_clk))
		clk_disable_unprepare(host->ciu_clk);

#ifdef CONFIG_SD_TIMEOUT_RESET
	dw_mci_disable_clk_08v(host);
#endif

	dw_mci_hs_set_controller(host, 1);

	host->current_speed = 0;

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	dev_info(host->dev, " %s -- \n", __func__);
	return 0;
}



static int dw_mci_hs_resume(struct device *dev)
{
	int ret;
	unsigned int i = 0;
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_hs_priv_data *priv = host->priv;
	dev_info(host->dev, " %s ++ \n", __func__);

	pm_runtime_get_sync(dev);

	if (!IS_ERR(host->biu_clk)) {
		if (clk_prepare_enable(host->biu_clk))
			dev_err(host->dev, "biu_clk clk_prepare_enable failed\n");
	}

	if (!IS_ERR(host->ciu_clk)) {
		if (clk_prepare_enable(host->ciu_clk))
			dev_err(host->dev, "ciu_clk clk_prepare_enable failed\n");
	}

#ifdef CONFIG_SD_TIMEOUT_RESET
	dw_mci_enable_clk_08v(host);
#endif

	dw_mci_hs_set_controller(host, 0);

	for (i = 0; i < host->num_slots; i++) {
		struct dw_mci_slot *slot = host->slot[i];
		if (!slot)
			continue;

		if (slot->mmc->pm_flags & MMC_PM_KEEP_POWER) {
			priv->in_resume = STATE_KEEP_PWR;
		} else {
			host->flags &= ~DWMMC_IN_TUNING;
			host->flags &= ~DWMMC_TUNING_DONE;
		}
	}

	/* restore controller specified setting */
	dw_mci_hs_priv_setting(host);
	ret = dw_mci_resume(host);
	if (ret)
		return ret;

	priv->in_resume = STATE_LEGACY;

	if (priv->gpio_cd) {
		enable_irq(gpio_to_irq(priv->gpio_cd));
		dev_info(host->dev, " enable gpio detect \n");
	}

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	dev_info(host->dev, " %s -- \n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_PM
static int dw_mci_hs_runtime_suspend(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	dev_vdbg(host->dev, " %s ++ \n", __func__);

	if (hs_dwmmc_card_busy(host)) {
		dev_err(host->dev, " %s: card is busy\n", __func__);
		return -EBUSY;
	}

	if (!IS_ERR(host->biu_clk))
		clk_disable_unprepare(host->biu_clk);

	if (!IS_ERR(host->ciu_clk))
		clk_disable_unprepare(host->ciu_clk);

#ifdef CONFIG_SD_TIMEOUT_RESET
	dw_mci_disable_clk_08v(host);
#endif

	dev_vdbg(host->dev, " %s -- \n", __func__);

	return 0;
}

static int dw_mci_hs_runtime_resume(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	dev_vdbg(host->dev, " %s ++ \n", __func__);

	if (!IS_ERR(host->biu_clk)) {
		if (clk_prepare_enable(host->biu_clk))
			dev_err(host->dev, "biu_clk clk_prepare_enable failed\n");
	}

	if (!IS_ERR(host->ciu_clk)) {
		if (clk_prepare_enable(host->ciu_clk))
			dev_err(host->dev, "ciu_clk clk_prepare_enable failed\n");
	}

#ifdef CONFIG_SD_TIMEOUT_RESET
	dw_mci_enable_clk_08v(host);
#endif

	dev_vdbg(host->dev, " %s -- \n", __func__);

	return 0;
}
#endif

extern void dw_mci_dump_crg(struct dw_mci *host);
void dw_mci_reg_dump(struct dw_mci *host)
{

	u32 status, mintsts;
	dev_err(host->dev, ": ============== REGISTER DUMP ==============\n");
	dev_err(host->dev, ": CTRL:	0x%08x\n", mci_readl(host, CTRL));
	dev_err(host->dev, ": PWREN:	0x%08x\n", mci_readl(host, PWREN));
	dev_err(host->dev, ": CLKDIV:	0x%08x\n", mci_readl(host, CLKDIV));
	dev_err(host->dev, ": CLKSRC:	0x%08x\n", mci_readl(host, CLKSRC));
	dev_err(host->dev, ": CLKENA:	0x%08x\n", mci_readl(host, CLKENA));
	dev_err(host->dev, ": TMOUT:	0x%08x\n", mci_readl(host, TMOUT));
	dev_err(host->dev, ": CTYPE:	0x%08x\n", mci_readl(host, CTYPE));
	dev_err(host->dev, ": BLKSIZ:	0x%08x\n", mci_readl(host, BLKSIZ));
	dev_err(host->dev, ": BYTCNT:	0x%08x\n", mci_readl(host, BYTCNT));
	dev_err(host->dev, ": INTMSK:	0x%08x\n", mci_readl(host, INTMASK));
	dev_err(host->dev, ": CMDARG:	0x%08x\n", mci_readl(host, CMDARG));
	dev_err(host->dev, ": CMD:	0x%08x\n", mci_readl(host, CMD));
	dev_err(host->dev, ": MINTSTS:	0x%08x\n", mci_readl(host, MINTSTS));
	dev_err(host->dev, ": RINTSTS:	0x%08x\n", mci_readl(host, RINTSTS));
	dev_err(host->dev, ": STATUS:	0x%08x\n", mci_readl(host, STATUS));
	dev_err(host->dev, ": FIFOTH:	0x%08x\n", mci_readl(host, FIFOTH));
	dev_err(host->dev, ": CDETECT:	0x%08x\n", mci_readl(host, CDETECT));
	dev_err(host->dev, ": WRTPRT:	0x%08x\n", mci_readl(host, WRTPRT));
	dev_err(host->dev, ": GPIO:	0x%08x\n", mci_readl(host, GPIO));
	dev_err(host->dev, ": TCBCNT:	0x%08x\n", mci_readl(host, TCBCNT));
	dev_err(host->dev, ": TBBCNT:	0x%08x\n", mci_readl(host, TBBCNT));
	dev_err(host->dev, ": DEBNCE:	0x%08x\n", mci_readl(host, DEBNCE));
	dev_err(host->dev, ": USRID:	0x%08x\n", mci_readl(host, USRID));
	dev_err(host->dev, ": VERID:	0x%08x\n", mci_readl(host, VERID));
	dev_err(host->dev, ": HCON:	0x%08x\n", mci_readl(host, HCON));
	dev_err(host->dev, ": UHS_REG:	0x%08x\n", mci_readl(host, UHS_REG));
	dev_err(host->dev, ": BMOD:	0x%08x\n", mci_readl(host, BMOD));
	dev_err(host->dev, ": PLDMND:	0x%08x\n", mci_readl(host, PLDMND));
	if(SDMMC_32_BIT_DMA == host->dma_64bit_address) {
	        dev_err(host->dev, ": DBADDR:	0x%08x\n", mci_readl(host, DBADDR));
	        dev_err(host->dev, ": IDSTS:    0x%08x\n", mci_readl(host, IDSTS));
	        dev_err(host->dev, ": IDINTEN:	0x%08x\n", mci_readl(host, IDINTEN));
	        dev_err(host->dev, ": DSCADDR:	0x%08x\n", mci_readl(host, DSCADDR));
	        dev_err(host->dev, ": BUFADDR:	0x%08x\n", mci_readl(host, BUFADDR));
	} else {
	        dev_err(host->dev, ": DBADDRL:	0x%08x\n", mci_readl(host, DBADDRL));
	        dev_err(host->dev, ": DBADDRU:	0x%08x\n", mci_readl(host, DBADDRU));
	        dev_err(host->dev, ": IDSTS:    0x%08x\n", mci_readl(host, IDSTS64));
	        dev_err(host->dev, ": IDINTEN:	0x%08x\n", mci_readl(host, IDINTEN64));
	}
	dev_err(host->dev, ": CDTHRCTL: 	0x%08x\n", mci_readl(host, CDTHRCTL));
	dev_err(host->dev, ": UHS_REG_EXT:	0x%08x\n", mci_readl(host, UHS_REG_EXT));
	dev_err(host->dev, ": ============== STATUS DUMP ================\n");
	dev_err(host->dev, ": cmd_status:      0x%08x\n", host->cmd_status);
	dev_err(host->dev, ": data_status:     0x%08x\n", host->data_status);
	dev_err(host->dev, ": pending_events:  0x%08lx\n", host->pending_events);
	dev_err(host->dev, ": completed_events:0x%08lx\n", host->completed_events);
	dev_err(host->dev, ": state:           %d\n", host->state);
	dev_err(host->dev, ": ===========================================\n");

	/* summary */
	mintsts = mci_readl(host, MINTSTS);
	status = mci_readl(host, STATUS);
	dev_err(host->dev, "CMD%d, ARG=0x%08x, intsts : %s, status : %s.\n",
			mci_readl(host, CMD) & 0x3F,
			mci_readl(host, CMDARG),
			mintsts & 0x8 ? "Data transfer done" : \
			mintsts & 0x4 ? "Command Done" : "refer to dump",
			status & (0x1 << 9) ? "dat0 busy" : "refer to dump");
	dev_err(host->dev, ": RESP0:	0x%08x\n", mci_readl(host, RESP0));
	dev_err(host->dev, ": RESP1:	0x%08x\n", mci_readl(host, RESP1));
	dev_err(host->dev, ": RESP2:	0x%08x\n", mci_readl(host, RESP2));
	dev_err(host->dev, ": RESP3:	0x%08x\n", mci_readl(host, RESP3));
	dev_err(host->dev, ": host : cmd_status=0x%08x, data_status=0x%08x.\n",
			host->cmd_status, host->data_status);
	dev_err(host->dev, ": host : pending_events=0x%08lx, "
			"completed_events=0x%08lx.\n",
			host->pending_events, host->completed_events);
	dw_mci_dump_crg(host);
	dev_err(host->dev, ": ===========================================\n");
}

#ifdef CONFIG_MMC_HISI_TRACE
void dw_mci_reg_dump_fortrace(struct mmc_host *mmc)
{
	u32 status, mintsts;
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;

	pm_runtime_get_sync(mmc_dev(mmc));

	mmc_trace_comm_record (mmc, ": =========== REGISTER DUMP (%s)===========\n",
				mmc_hostname(mmc));

	mmc_trace_comm_record(mmc, ": ============== REGISTER DUMP ==============\n");
	mmc_trace_comm_record(mmc, ": CTRL:	0x%x\n", mci_readl(host, CTRL));
	mmc_trace_comm_record(mmc, ": PWREN:	0x%x\n", mci_readl(host, PWREN));
	mmc_trace_comm_record(mmc, ": CLKDIV:	0x%x\n", mci_readl(host, CLKDIV));
	mmc_trace_comm_record(mmc, ": CLKSRC:	0x%x\n", mci_readl(host, CLKSRC));
	mmc_trace_comm_record(mmc, ": CLKENA:	0x%x\n", mci_readl(host, CLKENA));
	mmc_trace_comm_record(mmc, ": TMOUT:	0x%x\n", mci_readl(host, TMOUT));
	mmc_trace_comm_record(mmc, ": CTYPE:	0x%x\n", mci_readl(host, CTYPE));
	mmc_trace_comm_record(mmc, ": BLKSIZ:	0x%x\n", mci_readl(host, BLKSIZ));
	mmc_trace_comm_record(mmc, ": BYTCNT:	0x%x\n", mci_readl(host, BYTCNT));
	mmc_trace_comm_record(mmc, ": INTMSK:	0x%x\n", mci_readl(host, INTMASK));
	mmc_trace_comm_record(mmc, ": CMDARG:	0x%x\n", mci_readl(host, CMDARG));
	mmc_trace_comm_record(mmc, ": CMD:	0x%x\n", mci_readl(host, CMD));
	mmc_trace_comm_record(mmc, ": MINTSTS:	0x%x\n", mci_readl(host, MINTSTS));
	mmc_trace_comm_record(mmc, ": RINTSTS:	0x%x\n", mci_readl(host, RINTSTS));
	mmc_trace_comm_record(mmc, ": STATUS:	0x%x\n", mci_readl(host, STATUS));
	mmc_trace_comm_record(mmc, ": FIFOTH:	0x%x\n", mci_readl(host, FIFOTH));
	mmc_trace_comm_record(mmc, ": CDETECT:	0x%x\n", mci_readl(host, CDETECT));
	mmc_trace_comm_record(mmc, ": WRTPRT:	0x%x\n", mci_readl(host, WRTPRT));
	mmc_trace_comm_record(mmc, ": GPIO:	0x%x\n", mci_readl(host, GPIO));
	mmc_trace_comm_record(mmc, ": TCBCNT:	0x%x\n", mci_readl(host, TCBCNT));
	mmc_trace_comm_record(mmc, ": TBBCNT:	0x%x\n", mci_readl(host, TBBCNT));
	mmc_trace_comm_record(mmc, ": DEBNCE:	0x%x\n", mci_readl(host, DEBNCE));
	mmc_trace_comm_record(mmc, ": USRID:	0x%x\n", mci_readl(host, USRID));
	mmc_trace_comm_record(mmc, ": VERID:	0x%x\n", mci_readl(host, VERID));
	mmc_trace_comm_record(mmc, ": HCON:	0x%x\n", mci_readl(host, HCON));
	mmc_trace_comm_record(mmc, ": UHS_REG:	0x%x\n", mci_readl(host, UHS_REG));
	mmc_trace_comm_record(mmc, ": BMOD:	0x%08x\n", mci_readl(host, BMOD));
	mmc_trace_comm_record(mmc, ": PLDMND:	0x%x\n", mci_readl(host, PLDMND));
	mmc_trace_comm_record(mmc, ": DBADDR:	0x%x\n", mci_readl(host, DBADDR));
	mmc_trace_comm_record(mmc, ": IDSTS:	0x%x\n", mci_readl(host, IDSTS));
	mmc_trace_comm_record(mmc, ": IDINTEN:	0x%x\n", mci_readl(host, IDINTEN));
	mmc_trace_comm_record(mmc, ": DSCADDR:	0x%x\n", mci_readl(host, DSCADDR));
	mmc_trace_comm_record(mmc, ": BUFADDR:	0x%x\n", mci_readl(host, BUFADDR));
	mmc_trace_comm_record(mmc, ": CDTHRCTL: 	0x%x\n", mci_readl(host, CDTHRCTL));
	mmc_trace_comm_record(mmc, ": UHS_REG_EXT:	0x%x\n", mci_readl(host, 
UHS_REG_EXT));
	mmc_trace_comm_record(mmc, ": ============== STATUS DUMP ================\n");
	mmc_trace_comm_record(mmc, ": cmd_status:      0x%x\n", host->cmd_status);
	mmc_trace_comm_record(mmc, ": data_status:     0x%x\n", host->data_status);
	mmc_trace_comm_record(mmc, ": pending_events:  0x%x\n", host->pending_events);
	mmc_trace_comm_record(mmc, ": completed_events:0x%x\n", host->
completed_events);
	mmc_trace_comm_record(mmc, ": state:           %d\n", host->state);
	mmc_trace_comm_record(mmc, ": ===========================================\n");

	/* summary */
	mintsts = mci_readl(host, MINTSTS);
	status = mci_readl(host, STATUS);
	mmc_trace_comm_record(mmc, "CMD%d, ARG=0x%x, intsts : %s, status : %s.\n",
			mci_readl(host, CMD) & 0x3F,
			mci_readl(host, CMDARG),
			mintsts & 0x8 ? "Data transfer done" : \
			mintsts & 0x4 ? "Command Done" : "refer to dump",
			status & (0x1 << 9) ? "dat0 busy" : "refer to dump");
	mmc_trace_comm_record(mmc, ": RESP0:	0x%x\n", mci_readl(host, RESP0));
	mmc_trace_comm_record(mmc, ": RESP1:	0x%x\n", mci_readl(host, RESP1));
	mmc_trace_comm_record(mmc, ": RESP2:	0x%x\n", mci_readl(host, RESP2));
	mmc_trace_comm_record(mmc, ": RESP3:	0x%x\n", mci_readl(host, RESP3));
	mmc_trace_comm_record(mmc, ": host : cmd_status=0x%x, data_status=0x%x.\n",
			host->cmd_status, host->data_status);
	mmc_trace_comm_record(mmc, ": host : pending_events=0x%x, "
			"completed_events=0x%x.\n",
			host->pending_events, host->completed_events);

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	mmc_trace_comm_record(mmc, ": ===========================================\n");
}
#endif

bool dw_mci_stop_abort_cmd(struct mmc_command *cmd)
{
	u32 op = cmd->opcode;

	if ((op == MMC_STOP_TRANSMISSION) ||
	    (op == MMC_GO_IDLE_STATE) ||
	    (op == MMC_GO_INACTIVE_STATE) ||
	    ((op == SD_IO_RW_DIRECT) && (cmd->arg & 0x80000000) &&
	     ((cmd->arg >> 9) & 0x1FFFF) == SDIO_CCCR_ABORT))
		return true;
	return false;
}

bool dw_mci_wait_reset(struct device *dev, struct dw_mci *host,
		unsigned int reset_val)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);
	unsigned int ctrl;

	ctrl = mci_readl(host, CTRL);
	ctrl |= reset_val;
	mci_writel(host, CTRL, ctrl);

	/* wait till resets clear */
	do {
		if (!(mci_readl(host, CTRL) & reset_val))
			return true;
	} while (time_before(jiffies, timeout));

	dev_err(dev, "Timeout resetting block (ctrl %#x)\n", ctrl);

	return false;
}

static int mci_send_cmd(struct dw_mci_slot *slot, u32 cmd, u32 arg)
{
	struct dw_mci *host = slot->host;
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	unsigned int cmd_status = 0;

	mci_writel(host, CMDARG, arg);
	wmb();
	mci_writel(host, CMD, SDMMC_CMD_START | cmd);
	while (time_before(jiffies, timeout)) {
		cmd_status = mci_readl(host, CMD);
		if (!(cmd_status & SDMMC_CMD_START))
			return 0;
	}

	if(!dw_mci_wait_reset(host->dev, host, SDMMC_CTRL_RESET))
		return 1;

	timeout = jiffies + msecs_to_jiffies(100);
	mci_writel(host, CMD, SDMMC_CMD_START | cmd);
	while (time_before(jiffies, timeout)) {
		cmd_status = mci_readl(host, CMD);
		if (!(cmd_status & SDMMC_CMD_START))
			return 0;
	}

	dev_err(&slot->mmc->class_dev,
		"Timeout sending command (cmd %#x arg %#x status %#x)\n",
		cmd, arg, cmd_status);
	return 1;
}

void dw_mci_ciu_reset(struct device *dev, struct dw_mci *host)
{
	struct dw_mci_slot *slot = host->cur_slot;

	if (slot) {
		if(!dw_mci_wait_reset(dev, host, SDMMC_CTRL_RESET))
			dev_info(dev,"dw_mci_wait_reset failed\n");

		mci_send_cmd(slot, SDMMC_CMD_UPD_CLK |
			SDMMC_CMD_PRV_DAT_WAIT, 0);
	}
}

bool dw_mci_fifo_reset(struct device *dev, struct dw_mci *host)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(1000);
	unsigned int ctrl;
	bool result;

	do {
		result = dw_mci_wait_reset(host->dev, host, SDMMC_CTRL_FIFO_RESET);

		if (!result)
			break;

		ctrl = mci_readl(host, STATUS);
		if (!(ctrl & SDMMC_STATUS_DMA_REQ)) {
			result = dw_mci_wait_reset(host->dev, host,
					SDMMC_CTRL_FIFO_RESET);
			if (result) {
				/* clear exception raw interrupts can not be handled
				   ex) fifo full => RXDR interrupt rising */
				ctrl = mci_readl(host, RINTSTS);
				ctrl = ctrl & ~(mci_readl(host, MINTSTS));
				if (ctrl)
					mci_writel(host, RINTSTS, ctrl);

				return true;
			}
		}
	} while (time_before(jiffies, timeout));

	dev_err(dev, "%s: Timeout while resetting host controller after err\n",
		__func__);

	return false;
}

u32 dw_mci_prep_stop(struct dw_mci *host, struct mmc_command *cmd)
{
	struct mmc_command *stop = &host->stop;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	u32 cmdr = cmd->opcode;

	memset(stop, 0, sizeof(struct mmc_command));/* unsafe_function_ignore: memset */

	if(cmdr == SD_IO_RW_EXTENDED) {
		stop->opcode = SD_IO_RW_DIRECT;
		stop->arg = 0x80000000;
		stop->arg |= (cmd->arg >> 28) & 0x7;
		stop->arg |= SDIO_CCCR_ABORT << 9;
		stop->flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_AC;
	} else {
		stop->opcode = MMC_STOP_TRANSMISSION;
		stop->arg = 0;
		stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	cmdr = stop->opcode | SDMMC_CMD_STOP |
		SDMMC_CMD_RESP_CRC | SDMMC_CMD_RESP_EXP;

	/* Use hold bit register */
	if (drv_data && drv_data->prepare_command)
		drv_data->prepare_command(host, &cmdr);

	return cmdr;
}

bool dw_mci_wait_data_busy(struct dw_mci *host, struct mmc_request *mrq)
{
	u32 status;
	unsigned long timeout = jiffies + msecs_to_jiffies(500);

	do {
		status = mci_readl(host, STATUS);
		if (!(status & SDMMC_STATUS_BUSY))
			return true;

		usleep_range(10, 20);
	} while (time_before(jiffies, timeout));

	/* card is checked every 1s by CMD13 at least */
	if (mrq->cmd->opcode == MMC_SEND_STATUS)
		return true;

	dev_info(host->dev, "status is busy, reset ctrl\n");

	if(!dw_mci_wait_reset(host->dev, host, SDMMC_CTRL_RESET))
		return false;      

	/* After CTRL Reset, Should be needed clk val to CIU */
	if (host->cur_slot)
		mci_send_cmd(host->cur_slot,
			SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);

	timeout = jiffies + msecs_to_jiffies(500);
	do {
		status = mci_readl(host, STATUS);
		if (!(status & SDMMC_STATUS_BUSY))
			return true;

		usleep_range(10, 20);
	} while (time_before(jiffies, timeout));


	dev_err(host->dev, "Data[0]: data is busy\n");

	return false;
}

void dw_mci_set_cd(struct dw_mci *host){

	if(host == NULL)
		return;

	if(host->slot[0] && host->slot[0]->mmc){
		dev_dbg(&host->slot[0]->mmc->class_dev,"sdio_present = %d\n",host->slot[0]->mmc->sdio_present);
		host->slot[0]->mmc->sdio_present = 1;
	}
}

int dw_mci_start_signal_voltage_switch(struct mmc_host *mmc,
		struct mmc_ios *ios)
{

    struct dw_mci_slot *slot = mmc_priv(mmc);
    struct dw_mci *host = slot->host;
    const struct dw_mci_drv_data *drv_data = host->drv_data;
    int err = -ENOSYS;

    if(drv_data&&drv_data->start_signal_voltage_switch)
        err = drv_data->start_signal_voltage_switch(mmc,ios);

    return err;
}

void dw_mci_slowdown_clk(struct mmc_host *mmc, int timing)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	const struct dw_mci_drv_data *drv_data = host->drv_data;

	if (host->flags & DWMMC_TUNING_DONE)
		host->flags &= ~DWMMC_TUNING_DONE;

	if (drv_data->slowdown_clk) {
		if (host->sd_reinit)
			return;
		else {
			host->sd_reinit = 1;

			pm_runtime_get_sync(mmc_dev(mmc));
			drv_data->slowdown_clk(host, timing);
			pm_runtime_mark_last_busy(mmc_dev(mmc));
			pm_runtime_put_autosuspend(mmc_dev(mmc));
		}
	}

}

extern void dw_mci_request_end(struct dw_mci *host, struct mmc_request *mrq);
extern void dw_mci_stop_dma(struct dw_mci *host);

#ifdef CONFIG_SD_TIMEOUT_RESET
void dw_mci_set_clk_para_08v(struct dw_mci *host)
{
	/*When sd data transmission error,volt peri volt to 0.8v in libra*/
	if ((DWMMC_SD_ID == host->hw_mmc_id) && !IS_ERR(host->volt_hold_sd_clk) &&
		(VOLT_HOLD_CLK_08V != host->volt_hold_clk_sd) &&
		(MMC_TIMING_UHS_SDR104 == host->cur_slot->mmc->ios.timing)) {
		host->volt_hold_clk_sd = VOLT_HOLD_CLK_08V;
		host->set_sd_data_tras_timeout = VOLT_TO_1S;
		queue_work(host->card_workqueue, &host->work_volt_mmc);/*dw_mci_work_volt_mmc_func*/
	} else if ((DWMMC_SDIO_ID == host->hw_mmc_id) && !IS_ERR(host->volt_hold_sdio_clk) &&
		(VOLT_HOLD_CLK_08V != host->volt_hold_clk_sdio) &&
		(MMC_TIMING_UHS_SDR104 == host->cur_slot->mmc->ios.timing)) {
		host->volt_hold_clk_sdio = VOLT_HOLD_CLK_08V;
		host->set_sdio_data_tras_timeout = VOLT_TO_1S;
		queue_work(host->card_workqueue, &host->work_volt_mmc);/*dw_mci_work_volt_mmc_func*/
	}
}
#endif

/*lint -save -e570 -e650*/
void dw_mci_timeout_timer(unsigned long data)
{
	struct dw_mci *host = (struct dw_mci *)data;
	struct mmc_request *mrq;


	if (host) {
		spin_lock(&host->lock);
		if (host->mrq) {
			mrq = host->mrq;
			dev_vdbg(host->dev, "time out host->mrq = %pK\n", host->mrq);

			dev_err(host->dev,
				"Timeout waiting for hardware interrupt."
				" state = %d\n", host->state);
			dw_mci_reg_dump(host);

#ifdef CONFIG_SD_TIMEOUT_RESET
			dw_mci_set_clk_para_08v(host);
#endif

			host->sg = NULL;
			host->data = NULL;
			host->cmd = NULL;

			switch (host->state) {
			case STATE_IDLE:
				break;
			case STATE_SENDING_CMD:
				mrq->cmd->error = -ENOMEDIUM;
				if (!mrq->data)
					break;
			/* fall through */
			case STATE_SENDING_DATA:
				mrq->data->error = -ENOMEDIUM;
				dw_mci_stop_dma(host);
				break;
			case STATE_DATA_BUSY:
			case STATE_DATA_ERROR:
				if (mrq->data->error == -EINPROGRESS)
					mrq->data->error = -ENOMEDIUM;
				/* fall through */
			case STATE_SENDING_STOP:
				if (mrq->stop)
					mrq->stop->error = -ENOMEDIUM;
				break;
			}

			host->sd_hw_timeout = 1;

			dw_mci_request_end(host, mrq);
		}
		spin_unlock(&host->lock);
	}

	/*if(NULL != host)
		dw_mci_hi3xxx_work_fail_reset(host);*/

}

extern int dw_mci_get_cd(struct mmc_host *mmc);
extern void dw_mci_idmac_reset(struct dw_mci *host);
void dw_mci_work_routine_card(struct work_struct *work)
{
	struct dw_mci *host = container_of(work, struct dw_mci, card_work);
	unsigned int i = 0;

	for (i = 0; i < host->num_slots; i++) {
		struct dw_mci_slot *slot = host->slot[i];
		struct mmc_host *mmc = slot->mmc;
		struct mmc_request *mrq;
		int present;

		present = dw_mci_get_cd(mmc);
		while (present != slot->last_detect_state) {
			dev_dbg(&slot->mmc->class_dev, "card %s\n",
				present ? "inserted" : "removed");

			spin_lock_bh(&host->lock);

			/* Card change detected */
			slot->last_detect_state = present;

			/* Mark card as present if applicable */
			if (present != 0)
				set_bit(DW_MMC_CARD_PRESENT, &slot->flags);

			/* Clean up queue if present */
			mrq = slot->mrq;
			if (mrq) {
				if (mrq == host->mrq) {
					host->data = NULL;
					host->cmd = NULL;

					switch (host->state) {
					case STATE_IDLE:
						break;
					case STATE_SENDING_CMD:
						mrq->cmd->error = -ENOMEDIUM;
						if (!mrq->data)
							break;
						/* fall through */
					case STATE_SENDING_DATA:
						mrq->data->error = -ENOMEDIUM;
						dw_mci_stop_dma(host);
						break;
					case STATE_DATA_BUSY:
					case STATE_DATA_ERROR:
						if (mrq->data->error == -EINPROGRESS)
							mrq->data->error = -ENOMEDIUM;
						if (!mrq->stop)
							break;
						/* fall through */
					case STATE_SENDING_STOP:
						if(mrq->stop)
							mrq->stop->error = -ENOMEDIUM;
						break;
					}

					dw_mci_request_end(host, mrq);
				} else {
					list_del(&slot->queue_node);
					mrq->cmd->error = -ENOMEDIUM;
					if (mrq->data)
						mrq->data->error = -ENOMEDIUM;
					if (mrq->stop)
						mrq->stop->error = -ENOMEDIUM;

#ifdef CONFIG_HUAWEI_EMMC_DSM
					if(DWMMC_EMMC_ID == host->hw_mmc_id)
						if(del_timer(&host->rw_to_timer) == 0)
							dev_info(host->dev,"inactive timer\n");
#endif
					if (del_timer(&host->timer) !=0)
						dev_info(host->dev,"del_timer failed\n");
					spin_unlock(&host->lock);
					mmc_request_done(slot->mmc, mrq);
					spin_lock(&host->lock);
				}
			}

			/* Power down slot */
			if (present == 0) {
				clear_bit(DW_MMC_CARD_PRESENT, &slot->flags);

				/*
				 * Clear down the FIFO - doing so generates a
				 * block interrupt, hence setting the
				 * scatter-gather pointer to NULL.
				 */
				sg_miter_stop(&host->sg_miter);
				host->sg = NULL;

				dw_mci_fifo_reset(host->dev, host);
				dw_mci_ciu_reset(host->dev, host);
#ifdef CONFIG_MMC_DW_IDMAC
				dw_mci_idmac_reset(host);
#endif
			}

			spin_unlock_bh(&host->lock);

			present = dw_mci_get_cd(mmc);
		}

		mmc_detect_change(slot->mmc,
			msecs_to_jiffies(host->pdata->detect_delay_ms));
	}
}
/*lint -restore*/
bool mci_wait_reset(struct device *dev, struct dw_mci *host)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);
	unsigned int ctrl;

	mci_writel(host, CTRL, (SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET |
				SDMMC_CTRL_DMA_RESET));

	/* wait till resets clear */
	do {
		ctrl = mci_readl(host, CTRL);
		if (!(ctrl & (SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET |
			      SDMMC_CTRL_DMA_RESET)))
			return true;
	} while (time_before(jiffies, timeout));

	dev_err(dev, "Timeout resetting block (ctrl %#x)\n", ctrl);

	return false;
}


static const struct dev_pm_ops dw_mci_hs_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_mci_hs_suspend, dw_mci_hs_resume)
	SET_RUNTIME_PM_OPS(dw_mci_hs_runtime_suspend,
						   dw_mci_hs_runtime_resume, NULL)
};

static struct platform_driver dw_mci_hs_pltfm_driver = {
	.probe = dw_mci_hs_probe,
	.remove = dw_mci_pltfm_remove,
	.driver = {
			   .name = DRIVER_NAME,
			   .of_match_table = of_match_ptr(dw_mci_hs_match),
			   .pm = &dw_mci_hs_pmops,
			   },
};

module_platform_driver(dw_mci_hs_pltfm_driver);

MODULE_DESCRIPTION("Hisilicon Specific DW-MSHC Driver Extension");
MODULE_LICENSE("GPL v2");
#pragma GCC diagnostic pop
