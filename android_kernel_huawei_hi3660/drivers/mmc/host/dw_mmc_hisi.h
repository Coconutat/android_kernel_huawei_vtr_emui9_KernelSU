#ifndef _DW_MMC_HISI_
#define _DW_MMC_HISI_
#include <linux/mmc/dw_mmc.h>
#include <linux/pinctrl/consumer.h>
#include <linux/workqueue.h>
#include <linux/of_address.h>
#include <asm/cacheflush.h>
#include <linux/pm_runtime.h>

#include <linux/hisi/hw_cmdline_parse.h> /*for runmode_is_factory*/
#include <linux/hisi/mmc_trace.h>
#include <linux/version.h>


#define SDMMC_UHS_REG_EXT	0x108
#define SDMMC_ENABLE_SHIFT	0x110

#define SDMMC_64_BIT_DMA		1
#define SDMMC_32_BIT_DMA		0

#define SDMMC_CMD_ONLY_CLK		(SDMMC_CMD_START | SDMMC_CMD_UPD_CLK | \
						SDMMC_CMD_PRV_DAT_WAIT)

#define CTRL_RESET	(0x1 << 0) /* Reset DWC_mobile_storage controller */
#define FIFO_RESET	(0x1 << 1) /* Reset FIFO */
#define DMA_RESET	(0x1 << 2) /* Reset DMA interface */
#define INT_ENABLE	(0x1 << 4) /* Global interrupt enable/disable bit */
#define DMA_ENABLE	(0x1 << 5) /* DMA transfer mode enable/disable bit */
#define ENABLE_IDMAC	(0x1 << 25)


#define INTMSK_ALL	0xFFFFFFFF
#define INTMSK_CDETECT	(0x1 << 0)
#define INTMSK_RE	(0x1 << 1)
#define INTMSK_CDONE	(0x1 << 2)
#define INTMSK_DTO	(0x1 << 3)
#define INTMSK_TXDR	(0x1 << 4)
#define INTMSK_RXDR	(0x1 << 5)
#define INTMSK_RCRC	(0x1 << 6)
#define INTMSK_DCRC	(0x1 << 7)
#define INTMSK_RTO	(0x1 << 8)
#define INTMSK_DRTO	(0x1 << 9)
#define INTMSK_HTO	(0x1 << 10)
#define INTMSK_VOLT_SWITCH	(0x1 << 10)
#define INTMSK_FRUN	(0x1 << 11)
#define INTMSK_HLE	(0x1 << 12)
#define INTMSK_SBE	(0x1 << 13)
#define INTMSK_ACD	(0x1 << 14)
#define INTMSK_EBE	(0x1 << 15)
#define INTMSK_DMA	(INTMSK_ACD | INTMSK_RXDR | INTMSK_TXDR)

#define INT_SRC_IDMAC	(0x0)
#define INT_SRC_MINT	(0x1)


#define CMD_RESP_EXP_BIT	(0x1 << 6)
#define CMD_RESP_LENGTH_BIT	(0x1 << 7)
#define CMD_CHECK_CRC_BIT	(0x1 << 8)
#define CMD_DATA_EXP_BIT	(0x1 << 9)
#define CMD_RW_BIT		(0x1 << 10)
#define CMD_TRANSMODE_BIT	(0x1 << 11)
#define CMD_WAIT_PRV_DAT_BIT	(0x1 << 13)
#define CMD_STOP_ABORT_CMD	(0x1 << 14)
#define CMD_SEND_INITIALIZATION	(0x1 << 15)
#define CMD_SEND_CLK_ONLY	(0x1 << 21)
#define CMD_VOLT_SWITCH     (0x1 << 28)
#define CMD_USE_HOLD_REG    (0x1 << 29)
#define CMD_STRT_BIT		(0x1 << 31)
#define CMD_ONLY_CLK		(CMD_STRT_BIT | CMD_SEND_CLK_ONLY | \
						CMD_WAIT_PRV_DAT_BIT)

#define CLK_ENABLE	(0x1 << 0)
#define CLK_DISABLE	(0x0 << 0)

#define BOARDTYPE_SFT  (1)

#define STATE_KEEP_PWR		(1)
#define STATE_LEGACY		(0)

#define SD_SLOT_VOL_OPEN 1
#define SD_SLOT_VOL_CLOSE 0

#define SDMMC_ASIC_PLATFORM  (-1)

#define DW_MCI_DESC_SZ 1
#define DW_MCI_DESC_SZ_64BIT 2

#define DRIVER_NAME "dwmmc_hs"

#define DW_MCI_EMMC_ID    (0x00)
#define DW_MCI_SD_ID      (0x01)
#define DW_MCI_SDIO_ID    (0x02)

#define PERI_CRG_RSTEN4   (0x90)
#define PERI_CRG_RSTDIS4  (0x94)
#define PERI_CRG_CLKDIV4  (0xb8)
#define PERI_CRG_CLKDIV6  (0xc0)

#define CRG_PRINT 0x1

#define MMC1_SYSCTRL_PEREN0		(0x300)
#define MMC1_SYSCTRL_PERDIS0		(0x304)
#define MMC1_SYSCTRL_PERCLKEN0		(0x308)
#define MMC1_SYSCTRL_PERSTAT0		(0x30C)
#define  GT_HCLK_SDIO1_BIT 	(0x1)

#define MMC1_SYSCTRL_PERRSTEN0		(0x310)
#define MMC1_SYSCTRL_PERRSTDIS0		(0x314)
#define MMC1_SYSCTRL_PERRSTSTAT0	(0x318)
#define  BIT_HRST_SDIO_CANCER	(0x1)
#define  BIT_RST_SDIO_CANCER	(0x1 << 1)
#define MMC0_CRG_SD_HRST 0x20
#define MMC0_CRG_SD_HURST 0x24
#define BIT_RST_SD_TAURUS (0x1 << 0)
#define BIT_URST_SD_TAURUS (0x1 << 0)
/* mmc1 sys ctrl end*/

/* hsdt crg */
#define HSDT_CRG_PEREN0			(0x0)
#define HSDT_CRG_PERDIS0		(0x4)
#define GT_CLK_SDIO	(0x1)
#define GT_HCLK_SDIO	(0x1 << 1)

#define HSDT_CRG_PERRSTEN0		(0x60)
#define HSDT_CRG_PERRSTDIS0		(0x64)
#define IP_RST_SDIO	(0x1)
#define IP_HRST_SDIO	(0x1 << 1)

#define HSDT_CRG_CLKDIV0		(0xA8)
#define SEL_SDIO_PLL	(0x1 << 13)
#define DIV_SDIO_PLL_MASK	(0xF << 16)

#define HI3660_FPGA 1
#define PERI_CRG_PERSTAT4 (0x04c)

#define GTCLK_SD_EN (0x20000)

#define BIT_VOLT_OFFSET         (0x314)
#define BIT_VOLT_OFFSET_CANCER         (0x31c)
#define BIT_VOLT_OFFSET_AUSTIN  (0x214)
#define BIT_VOLT_OFFSET_TAURUS         (0x31c)
#define BIT_VOLT_VALUE_18       (0x4)
#define BIT_VOLT_VALUE_18_CANCER       (0x400040)
#define BIT_VOLT_VALUE_18_TAURUS       (0x400040)
#define BIT_VOLT_VALUE_18_MASK_CANCER       (0x400000)
#define BIT_VOLT_VALUE_18_MASK_TAURUS       (0x400000)

#define BIT_RST_EMMC            (1<<20)
#define BIT_RST_SD              (1<<18)
#define BIT_RST_SDIO            (1<<19)

#define BIT_RST_SDIO_CHICAGO    (1<<20)
#define BIT_RST_SDIO_BOSTON    (1<<20)
#define BIT_RST_SDIO_LIBRA    (1<<20)
#define BIT_RST_SDIO_TAURUS    (1UL << 1)

#define BIT_RST_SD_M      (1<<24)
#define GPIO_CLK_DIV(x) (((x) & 0xf) << 8)
#define GPIO_USE_SAMPLE_DLY(x) (((x) & 0x1) << 13)

#define GPIO_CLK_ENABLE (0x1 << 16)
#define UHS_REG_EXT_SAMPLE_PHASE(x) (((x) & 0x1f) << 16)
#define UHS_REG_EXT_SAMPLE_DLY(x) (((x) & 0x1f) << 26)
#define UHS_REG_EXT_SAMPLE_DRVPHASE(x) (((x) & 0x1f) << 21)
#define SDMMC_UHS_REG_EXT_VALUE(x, y, z)                                          \
		(UHS_REG_EXT_SAMPLE_PHASE(x) | UHS_REG_EXT_SAMPLE_DLY(y) | UHS_REG_EXT_SAMPLE_DRVPHASE(z))
#define SDMMC_GPIO_VALUE(x, y)                                              \
		(GPIO_CLK_DIV(x) | GPIO_USE_SAMPLE_DLY(y))

/*Reduce Max tuning loop,200 loops may case the watch dog timeout*/
#define MAX_TUNING_LOOP 32

#ifdef CONFIG_MMC_DW_MUX_SDSIM
#define MUX_SDSIM_VCC_STATUS_2_9_5_V   0
#define MUX_SDSIM_VCC_STATUS_1_8_0_V   1
#endif

struct dw_mci_hs_priv_data {
	int				id;
	int				old_timing;
	int				gpio_cd;
	int				gpio_sw;
	int				sw_value;
	int				old_signal_voltage;
	int				old_power_mode;
	unsigned int		priv_bus_hz;
	unsigned int		cd_vol;
       unsigned int sd_slot_ldo10_status;
#ifdef CONFIG_MMC_DW_MUX_SDSIM
	int				mux_sdsim;	/*if enabled as 1,sd and sim module with be used alternately by switching mux io config*/
	int				mux_sdsim_vcc_status;
#endif
	int				dw_mmc_bus_clk;
	int				dw_voltage_switch_gpio;
	int				hi3660_sd_ioset_sd_sel;
	int				hi3660_sd_ioset_jtag_sd_sel;
	int				hi3660_fpga_sd_ioset;
	int                                                crg_print;
	int				cs;
	int				in_resume;
	void __iomem *    ao_sysctrl;
	void __iomem *    peri_sysctrl;
	void __iomem *    ioc_off;
};

extern void dw_mci_set_cd(struct dw_mci *host);

struct dw_mci_tuning_data{
    const u8 *blk_pattern;
    unsigned int blksz;
};

#ifdef CONFIG_MMC_DW_IDMAC
#define IDMAC_INT_CLR		(SDMMC_IDMAC_INT_AI | SDMMC_IDMAC_INT_NI | \
				 SDMMC_IDMAC_INT_CES | SDMMC_IDMAC_INT_DU | \
				 SDMMC_IDMAC_INT_FBE | SDMMC_IDMAC_INT_RI | \
				 SDMMC_IDMAC_INT_TI)
#endif

/* Common flag combinations */
#define DW_MCI_DATA_ERROR_FLAGS	(SDMMC_INT_DRTO | SDMMC_INT_DCRC | \
				 SDMMC_INT_HTO | SDMMC_INT_SBE  | \
				 SDMMC_INT_EBE)
#define DW_MCI_CMD_ERROR_FLAGS	(SDMMC_INT_RTO | SDMMC_INT_RCRC | \
				 SDMMC_INT_RESP_ERR)
#define DW_MCI_ERROR_FLAGS	(DW_MCI_DATA_ERROR_FLAGS | \
				 DW_MCI_CMD_ERROR_FLAGS  | SDMMC_INT_HLE)


#endif /* _DW_MMC_HISI_ */
