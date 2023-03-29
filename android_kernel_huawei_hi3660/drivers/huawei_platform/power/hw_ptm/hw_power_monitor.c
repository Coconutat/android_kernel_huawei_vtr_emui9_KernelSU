/*copyright (c) Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
 *
 * File name: hw_power_monitor.c
 * Description: This file use to record power state for upper layer
 * Author: ivan.chengfeifei@huawei.com
 * Version: 0.1
 * Date:  2014/11/27
 */
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/debugfs.h>
#include <huawei_platform/power/hw_power_monitor.h>
#include "hw_power.h"
#if defined (CONFIG_HW_PTM_K3V5)
    #define IRQ_TOTAL  251
#elif defined (CONFIG_HW_PTM_K3V6)
    #define IRQ_TOTAL  355
#else
    #define IRQ_TOTAL  10
#endif
#define MAX_GPIO_NUM 222
#define MODEM_REASON_NUM 15
#define MAX_BUF_SIZE 63
#define AB_WAKE_COUNT 5

#define TLPS_MASK                       (0x01)
#define PS_G0_MASK                      (0x02)
#define PS_W0_MASK                      (0x04)
#define PS_G1_MASK                      (0x08)
#define PS_W1_MASK                      (0x10)
#define PS_FTM_MASK                     (0x20)
#define PS_FTM_1_MASK                   (0x40)
#define PS_NAS_MASK                     (0x80)
#define NAS_1_MASK                      (0x100)
#define OAM_MASK                        (0x200)
#define SCI0_MASK                       (0x400)
#define SCI1_MASK                       (0x800)
#define DSFLOW_MASK                     (0x1000)
#define TEST_MASK                       (0x2000)
#define UART0_MASK                      (0x4000)

unsigned int suspend_total;
bool freezing_wakeup_check;
static struct power_monitor_info pm_info[POWER_MONITOR_MAX];

struct dcurrent_s {
    char *devname;
    int number;
};

struct modem_s {
    char *devname;
    int number;
    int devmask;
};

#define CVS(name, num)  {\
    .devname = name,\
    .number = num,\
}

#define MVS(name, num, mask)    {\
    .devname = name,\
    .number = num,\
    .devmask = mask,\
}

int GPIO_INT[223][1];

struct modem_s modem_wakeup[] = {
    MVS("TLPS", 0, TLPS_MASK),
    MVS("PS_G0", 0, PS_G0_MASK),
    MVS("PS_W0", 0, PS_W0_MASK),
    MVS("PS_G1", 0, PS_G1_MASK),
    MVS("PS_W1", 0, PS_W1_MASK),
    MVS("PS_FTM", 0, PS_FTM_MASK),
    MVS("PS_FTM_1", 0, PS_FTM_1_MASK),
    MVS("PS_NAS", 0, PS_NAS_MASK),
    MVS("NAS_1", 0, NAS_1_MASK),
    MVS("OAM", 0, OAM_MASK),
    MVS("SCI0", 0, SCI0_MASK),
    MVS("SCI1", 0, SCI1_MASK),
    MVS("DSFLOW", 0, DSFLOW_MASK),
    MVS("TEST", 0, TEST_MASK),
    MVS("UART0", 0, UART0_MASK),
};

struct dcurrent_s irq_table_k3v5[] = {
    CVS("IPI_RESCHEDULE", 0),   /* 42 */
    CVS("IPI_CALL_FUNC", 0),    /* 43 */
    CVS("IPI_CALL_FUNC_SINGLE", 0), /* 44 */
    CVS("IPI_CPU_STOP", 0), /* 45 */
    CVS("IPI_TIMER", 0),    /* 46 */
    CVS("Virtual maintenance interrupt", 0),    /* 47 */
    CVS("Hypervisor timer", 0), /* 48 */
    CVS("Virtual timer", 0),    /* 49 */
    CVS("Legacy FIQ signal", 0),
    CVS("Secure physical timer", 0),
    CVS("Non-secure physical timer", 0),
    CVS("Legacy IRQ signal", 0),
    CVS("Maia_interr", 0),
    CVS("Maia_exterr", 0),
    CVS("Maia_pmu0", 0),
    CVS("Maia_pmu1", 0),
    CVS("Maia_pmu2", 0),
    CVS("Maia_pmu3", 0),
    CVS("Maia_cti0", 0),
    CVS("Maia_cti1", 0),
    CVS("Maia_cti2", 0),
    CVS("Maia_cti3", 0),
    CVS("Maia_COMMRX0", 0),
    CVS("Maia_COMMRX1", 0),
    CVS("Maia_COMMRX2", 0),
    CVS("Maia_COMMRX3", 0),
    CVS("Maia_COMMTX0", 0),
    CVS("Maia_COMMTX1", 0),
    CVS("Maia_COMMTX2", 0),
    CVS("Maia_COMMTX3", 0),
    CVS("Maia_COMMIRQ0", 0),
    CVS("Maia_COMMIRQ1", 0),
    CVS("Maia_COMMIRQ2", 0),
    CVS("Maia_COMMIRQ3", 0),
    CVS("WatchDog0", 0),
    CVS("WatchDog1", 0),
    CVS("RTC0", 0),
    CVS("RTC1", 0),
    CVS("TIME00", 0),
    CVS("TIME01", 0),
    CVS("TIME10", 0),
    CVS("TIME11", 0),
    CVS("TIME20", 0),
    CVS("TIME21", 0),
    CVS("TIME30", 0),
    CVS("TIME31", 0),
    CVS("TIME40", 0),
    CVS("TIME41", 0),
    CVS("TIME50", 0),
    CVS("TIME51", 0),
    CVS("TIME60", 0),
    CVS("TIME61", 0),
    CVS("TIME70", 0),
    CVS("TIME71", 0),
    CVS("TIME80", 0),
    CVS("TIME81", 0),
    CVS("TIME90", 0),
    CVS("TIME91", 0),
    CVS("TIME100", 0),
    CVS("TIME101", 0),
    CVS("TIME110", 0),
    CVS("TIME111", 0),
    CVS("TIME120", 0),
    CVS("TIME121", 0),
    CVS("UART0", 0),
    CVS("UART1", 0),
    CVS("UART2", 0),
    CVS("UART4", 0),
    CVS("UART5", 0),
    CVS("UART6", 0),
    CVS("I2C3", 0),
    CVS("I2C4", 0),
    CVS("I2C5(PMU_I2C)", 0),
    CVS("GPIO0[]", 0),
    CVS("GPIO1[]", 0),
    CVS("GPIO2[]", 0),
    CVS("GPIO3[]", 0),
    CVS("GPIO4[]", 0),
    CVS("GPIO5[]", 0),
    CVS("GPIO6[]", 0),
    CVS("GPIO7[]", 0),
    CVS("GPIO8[]", 0),
    CVS("GPIO9[]", 0),
    CVS("GPIO10[]", 0),
    CVS("GPIO11[]", 0),
    CVS("GPIO12[]", 0),
    CVS("GPIO13[]", 0),
    CVS("GPIO14[]", 0),
    CVS("GPIO15[]", 0),
    CVS("GPIO16[]", 0),
    CVS("GPIO17[]", 0),
    CVS("GPIO18[]", 0),
    CVS("GPIO19[]", 0),
    CVS("GPIO20[]", 0),
    CVS("GPIO21[]", 0),
    CVS("GPIO22[]", 0),
    CVS("GPIO23[]", 0),
    CVS("GPIO24[]", 0),
    CVS("GPIO25[]", 0),
    CVS("GPIO26[]", 0),
    CVS("GPIO27[]", 0),
    CVS("IOMCU_WD", 0),
    CVS("IOMCU_SPI", 0),
    CVS("IOMCU_UART3", 0),
    CVS("IOMCU_UART8", 0),
    CVS("IOMCU_SPI2", 0),
    CVS("IOMCU_I2C3", 0),
    CVS("IOMCU_I2C0", 0),
    CVS("IOMCU_I2C1", 0),
    CVS("IOMCU_I2C2", 0),
    CVS("IOMCU_GPIO0_INT1", 0),
    CVS("IOMCU_GPIO1_INT1", 0),
    CVS("IOMCU_GPIO2_INT1", 0),
    CVS("IOMCU_GPIO3_INT1", 0),
    CVS("IOMCU_DMAC_INT0", 0),
    CVS("IOMCU_DMAC_ns_INT0", 0),
    CVS("PERF_STAT", 0),
    CVS("IOMCU_COMB", 0),
    CVS("IOMCU_BLPWM", 0),
    CVS("NOC-comb", 0),
    CVS("intr_dmss", 0),
    CVS("intr_ddrc0_err", 0),
    CVS("intr_ddrc1_err", 0),
    CVS("PMCTRL", 0),
    CVS("SECENG_P", 0),
    CVS("SECENG_S", 0),
    CVS("EMMC0", 0),
    CVS("EMMC1", 0),
    CVS("SD3", 0),
    CVS("SDIO0", 0),
    CVS("SDIO1", 0),
    CVS("DMAC_int0", 0),
    CVS("DMAC_ns_int0", 0),
    CVS("CLK_MONITOR", 0),
    CVS("TSENSOR_Maia", 0),
    CVS("TSENSOR_A53", 0),
    CVS("TSENSOR_G3D", 0),
    CVS("TSENSOR_Modem", 0),
    CVS("ASP_ARM_SECURE", 0),
    CVS("ASP_ARM", 0),
    CVS("VDM_INT2", 0),
    CVS("VDM_INT0", 0),
    CVS("VDM_INT1", 0),
    CVS("MODEM_IPC0[0]", 0),
    CVS("MODEM_IPC1[0]", 0),
    CVS("MDM_bus_err", 0),
    CVS("MDM_EDMAC[3]", 0),
    CVS("MDM_EDMAC_NS[3]", 0),
    CVS("USB3[0]", 0),
    CVS("USB3[1]", 0),
    CVS("USB3_OTG", 0),
    CVS("USB3_BC", 0),
    CVS("PMC-DDRC-DFS", 0),
    CVS("Reserved", 0),
    CVS("PMC-DVFS-Maia", 0),
    CVS("PMC-DVFS-A53", 0),
    CVS("PMC-DVFS-G3D", 0),
    CVS("PMC-AVS-Maia", 0),
    CVS("PMC-AVS-A53", 0),
    CVS("PMC-AVS-G3D", 0),
    CVS("PMC-AVS-IDLE-Maia", 0),
    CVS("PMC-AVS-IDLE-A53", 0),
    CVS("PMC-AVS-IDLE-G3D", 0),
    CVS("M3_LP_wd", 0),
    CVS("CCI400_err", 0),
    CVS("CCI400_overflow", 0),
    CVS("CCI400_overflow[4]", 0),
    CVS("IPC_S_int0", 0),
    CVS("IPC_S_int1", 0),
    CVS("IPC_S_int4", 0),
    CVS("IPC_NS_int5", 0),
    CVS("IPC_NS_int6", 0),
    CVS("IPC_NS_mbx0", 0),
    CVS("IPC_NS_mbx1", 0),
    CVS("IPC_NS_mbx2", 0),  /* 252 */
    CVS("IPC_NS_mbx3", 0),  /* 253 */
    CVS("IPC_NS_mbx4", 0),  /* 254 */
    CVS("IPC_NS_mbx5", 0),  /* 255 */
    CVS("IPC_NS_mbx6", 0),  /* 256 */
    CVS("IPC_NS_mbx7", 0),  /* 257 */
    CVS("IPC_NS_mbx8", 0),  /* 258 */
    CVS("IPC_NS_mbx9", 0),  /* 259 */
    CVS("IPC_NS_mbx18", 0), /* 260 */
    CVS("aximon_cpufast_intr", 0),  /* 261 */
    CVS("MDM_WDOG_intr", 0),    /* 262 */
    CVS("ASP-IPC-ARM", 0),  /* 263 */
    CVS("ASP-IPC-MCPU", 0), /* 264 */
    CVS("ASP-IPC-BBE16", 0),    /* 265 */
    CVS("ASP_WD", 0),   /* 266 */
    CVS("ASP_AXI_DLOCK", 0),    /* 267 */
    CVS("ASP_DMA_SECURE", 0),   /* 268 */
    CVS("ASP_DMA_SECURE_N", 0), /* 269 */
    CVS("SCI0", 0),     /* 270 */
    CVS("SCI1", 0),     /* 271 */
    CVS("SOCP0", 0),    /* 272 */
    CVS("SOCP1", 0),    /* 273 */
    CVS("MDM_IPF_intr0", 0),    /* 274 */
    CVS("MDM_IPF_intr1", 0),    /* 275 */
    CVS("ddrc_fatal_int[3:0]", 0),  /* 276 */
    CVS("Reserved", 0), /* 277 */
    CVS("MDM_UICC_intr", 0),    /* 278 */
    CVS("GIC_IRQ_OUT[0]", 0),
    CVS("GIC_IRQ_OUT[1]", 0),
    CVS("GIC_IRQ_OUT[2]", 0),
    CVS("GIC_IRQ_OUT[3]", 0),
    CVS("GIC_IRQ_OUT[4]", 0),
    CVS("GIC_IRQ_OUT[5]", 0),
    CVS("GIC_IRQ_OUT[6]", 0),
    CVS("GIC_IRQ_OUT[7]", 0),
    CVS("GIC_FIQ_OUT[0]", 0),
    CVS("GIC_FIQ_OUT[1]", 0),
    CVS("GIC_FIQ_OUT[2]", 0),
    CVS("GIC_FIQ_OUT[3]", 0),
    CVS("GIC_FIQ_OUT[4]", 0),
    CVS("GIC_FIQ_OUT[5]", 0),
    CVS("GIC_FIQ_OUT[6]", 0),
    CVS("GIC_FIQ_OUT[7]", 0),
    CVS("NANDC", 0),
    CVS("CoreSight_ETR_Full", 0),
    CVS("CoreSight_ETF_Full", 0),
    CVS("DSS-pdp", 0),
    CVS("DSS-sdp", 0),
    CVS("DSS-offline", 0),
    CVS("DSS_mcu_pdp", 0),
    CVS("DSS_mcu_sdp", 0),
    CVS("DSS_mcu_offline", 0),
    CVS("DSS_dsi0", 0),
    CVS("DSS_dsi1", 0),
    CVS("IVP32_SMMU_irpt_s", 0),
    CVS("IVP32_SMMU_irpt_ns", 0),
    CVS("IVP32_WATCH_DOG", 0),
    CVS("VENC", 0),
    CVS("VDEC", 0),
    CVS("G3D_JOB", 0),
    CVS("G3D_MMU", 0),
    CVS("G3D_GPU", 0),
    CVS("isp_irq[0]", 0),
    CVS("isp_irq[1]", 0),
    CVS("isp_irq[2]", 0),
    CVS("isp_irq[3]", 0),
    CVS("isp_irq[4]", 0),
    CVS("isp_irq[5]", 0),
    CVS("isp_irq[6]", 0),
    CVS("isp_irq[7]", 0),
    CVS("isp_a7_to_gic_mbx_int[0]", 0),
    CVS("isp_a7_to_gic_mbx_int[1]", 0),
    CVS("isp_a7_to_gic_ipc_int", 0),
    CVS("isp_a7_watchdog_int", 0),
    CVS("isp_axi_dlcok", 0),
    CVS("isp_axi_irq_out", 0),
    CVS("ivp32_dwaxi_dlock_irq", 0),
    CVS("mmbuf_asc0", 0),
    CVS("mmbuf_asc1", 0),
};

struct dcurrent_s irq_table_k3v6[] = {
    CVS("IPI_RESCHEDULE", 0),
    CVS("IPI_CALL_FUNC", 0),
    CVS("IPI_CALL_FUNC_SINGLE", 0),
    CVS("IPI_CPU_STOP", 0),
    CVS("IPI_TIMER", 0),
    CVS("IPI_SECURE_RPMB", 0),
    CVS("IPI_MNTN_INFORM", 0),
    CVS("IPI_CPU_BACKTRACE", 0),
    CVS("Virtual maintenance interrupt", 0),
    CVS("Hypervisor timer", 0),
    CVS("Virtual timer", 0),
    CVS("Legacy FIQ signal", 0),
    CVS("Secure physical timer", 0),
    CVS("Non-secure physical timer", 0),
    CVS("Legacy IRQ signal", 0),
    CVS("Artemis_interr", 0),
    CVS("Artemis_exterr", 0),   
    CVS("Artemis_pmu0", 0),
    CVS("Artemis_pmu1", 0), 
    CVS("Artemis_pmu2", 0), 
    CVS("Artemis_pmu3", 0),
    CVS("Artemis_cti0", 0), 
    CVS("Artemis_cti1", 0), 
    CVS("Artemis_cti2", 0), 
    CVS("Artemis_cti3", 0), 
    CVS("Artemis_COMMRX0", 0),  
    CVS("Artemis_COMMRX1", 0),
    CVS("Artemis_COMMRX2", 0),
    CVS("Artemis_COMMRX3", 0),
    CVS("Artemis_COMMTX0", 0),  
    CVS("Artemis_COMMTX1", 0),
    CVS("Artemis_COMMTX2", 0),
    CVS("Artemis_COMMTX3", 0),
    CVS("Artemis_COMMIRQ0", 0),
    CVS("Artemis_COMMIRQ1", 0),
    CVS("Artemis_COMMIRQ2", 0),
    CVS("Artemis_COMMIRQ3", 0),
    CVS("A53_interr", 0),
    CVS("A53_exterr", 0),
    CVS("A53_pmu0", 0),
    CVS("A53_pmu1", 0),
    CVS("A53_pmu2", 0),
    CVS("A53_pmu3", 0),
    CVS("A53_cti0", 0),
    CVS("A53_cti1", 0),
    CVS("A53_cti2", 0),
    CVS("A53_cti3", 0),
    CVS("A53_COMMRX0", 0),
    CVS("A53_COMMRX1", 0),
    CVS("A53_COMMRX2", 0),
    CVS("A53_COMMRX3", 0),
    CVS("A53_COMMTX0", 0),
    CVS("A53_COMMTX1", 0),
    CVS("A53_COMMTX2", 0),
    CVS("A53_COMMTX3", 0),
    CVS("A53_COMMIRQ0", 0),
    CVS("A53_COMMIRQ1", 0),
    CVS("A53_COMMIRQ2", 0),
    CVS("A53_COMMIRQ3", 0),
    CVS("WatchDog0", 0),
    CVS("WatchDog1", 0),
    CVS("RTC0", 0),
    CVS("RTC1", 0),
    CVS("TIME00", 0),
    CVS("TIME01", 0),
    CVS("TIME10", 0),
    CVS("TIME11", 0),
    CVS("TIME20", 0),
    CVS("TIME21", 0),
    CVS("TIME30", 0),
    CVS("TIME31", 0),
    CVS("TIME40", 0),
    CVS("TIME41", 0),
    CVS("TIME50", 0),
    CVS("TIME51", 0),
    CVS("TIME60", 0),
    CVS("TIME61", 0),
    CVS("TIME70", 0),
    CVS("TIME71", 0),
    CVS("TIME80", 0),
    CVS("TIME81", 0),
    CVS("TIME90", 0),
    CVS("TIME91", 0),
    CVS("TIME100", 0),
    CVS("TIME101", 0),
    CVS("TIME110", 0),
    CVS("TIME111", 0),
    CVS("TIME120", 0),
    CVS("TIME121", 0),
    CVS("UART0", 0),
    CVS("UART1", 0),
    CVS("UART2", 0),
    CVS("UART4", 0),
    CVS("UART5", 0),
    CVS("UART6", 0),
    CVS("SPI1", 0),
    CVS("I2C3", 0),
    CVS("I2C4", 0),
    CVS("I2C5(PMU_I2C)", 0),
    CVS("GPIO0_INTR1", 0),
    CVS("GPIO1_INTR1", 0),
    CVS("GPIO2_INTR1", 0),
    CVS("GPIO3_INTR1", 0),
    CVS("GPIO4_INTR1", 0),
    CVS("GPIO5_INTR1", 0),
    CVS("GPIO6_INTR1", 0),
    CVS("GPIO7_INTR1", 0),
    CVS("GPIO8_INTR1", 0),
    CVS("GPIO9_INTR1", 0),
    CVS("GPIO10_INTR1", 0),
    CVS("GPIO11_INTR1", 0),
    CVS("GPIO12_INTR1", 0),
    CVS("GPIO13_INTR1", 0),
    CVS("GPIO14_INTR1", 0),
    CVS("GPIO15_INTR1", 0),
    CVS("GPIO16_INTR1", 0),
    CVS("GPIO17_INTR1", 0),
    CVS("GPIO18_INTR1", 0),
    CVS("GPIO19_INTR1", 0),
    CVS("GPIO20_INTR1", 0),
    CVS("GPIO21_INTR1", 0),
    CVS("GPIO22_INTR1", 0),
    CVS("GPIO23_INTR1", 0),
    CVS("GPIO24_INTR1", 0),
    CVS("GPIO25_INTR1", 0),
    CVS("GPIO26_INTR1", 0),
    CVS("GPIO27_INTR1", 0),
    CVS("IOMCU_WD", 0),
    CVS("IOMCU_SPI", 0),
    CVS("IOMCU_UART3", 0),
    CVS("IOMCU_UART8", 0),
    CVS("IOMCU_SPI2", 0),
    CVS("IOMCU_I2C3", 0),
    CVS("IOMCU_I2C0", 0),
    CVS("IOMCU_I2C1", 0),
    CVS("IOMCU_I2C2", 0),
    CVS("IOMCU_GPIO0_INT1", 0),
    CVS("IOMCU_GPIO1_INT1", 0),
    CVS("IOMCU_GPIO2_INT1", 0),
    CVS("IOMCU_GPIO3_INT1", 0),
    CVS("IOMCU_DMAC_INT0", 0),
    CVS("IOMCU_DMAC_ns_INT0", 0),
    CVS("PERF_STAT", 0),
    CVS("IOMCU_COMB", 0),
    CVS("IOMCU_BLPWM", 0),
    CVS("NOC-comb", 0),
    CVS("intr_dmss", 0),
    CVS("intr_ddrc0_err", 0),
    CVS("intr_ddrc1_err", 0),
    CVS("PMCTRL", 0),
    CVS("SECENG_P", 0),
    CVS("SECENG_S", 0),
    CVS("EMMC51", 0),
    CVS("ASP_IPC_MODEM_CBBE", 0),
    CVS("SD3", 0),
    CVS("SDIO", 0),
    CVS("GPIO28_INTR1", 0),
    CVS("PERI_DMAC_int0", 0),
    CVS("PERI_DMAC_ns_int0", 0),
    CVS("CLK_MONITOR(SCTRL)", 0),
    CVS("TSENSOR_Artemis", 0),
    CVS("TSENSOR_A53", 0),
    CVS("TSENSOR_G3D", 0),
    CVS("TSENSOR_Modem", 0),
    CVS("ASP_ARM_SECURE", 0),
    CVS("ASP_ARM", 0),
    CVS("VDM_INT2", 0),
    CVS("VDM_INT0", 0),
    CVS("VDM_INT1", 0),
    CVS("MODEM_IPC0[0]MDM_IPC_APPCPU_intr0", 0),
    CVS("MODEM_IPC1[0]MDM_IPC_APPCPU_intr1", 0),
    CVS("MDM_bus_err", 0),
    CVS("Reserved", 0),
    CVS("MDM_EDMAC0_INTR_NS[0]", 0),
    CVS("USB3", 0),
    CVS("USB3_OTG", 0),
    CVS("USB3_BC", 0),
    CVS("GPIO1_SE_INTR1", 0),
    CVS("GPIO0_SE_INTR1", 0),
    CVS("PMC-DVFS-Maia", 0),
    CVS("PMC-DVFS-A53", 0),
    CVS("PMC-DVFS-G3D", 0),
    CVS("PMC-AVS-Maia", 0),
    CVS("PMC-AVS-A53", 0),
    CVS("PMC-AVS-G3D", 0),
    CVS("PMC-AVS-IDLE-Maia", 0),
    CVS("PMC-AVS-IDLE-A53", 0),
    CVS("PMC-AVS-IDLE-G3D", 0),
    CVS("M3_LP_wd", 0),
    CVS("CCI400_err", 0),
    CVS("CCI400_overflow[6:0]", 0),
    CVS("CCI400_overflow[7]", 0),
    CVS("IPC_S_int0", 0),
    CVS("IPC_S_int1", 0),
    CVS("IPC_S_int4", 0),
    CVS("IPC_S_mbx0", 0),
    CVS("IPC_S_mbx1", 0),
    CVS("IPC_S_mbx2", 0),
    CVS("IPC_S_mbx3", 0),
    CVS("IPC_S_mbx4", 0),
    CVS("IPC_S_mbx5", 0),
    CVS("IPC_S_mbx6", 0),
    CVS("IPC_S_mbx7", 0),
    CVS("IPC_S_mbx8", 0),
    CVS("IPC_S_mbx9", 0),
    CVS("IPC_S_mbx18", 0),
    CVS("IPC_NS_int0", 0),
    CVS("IPC_NS_int1", 0),
    CVS("IPC_NS_int4", 0),
    CVS("IPC_NS_int5", 0),
    CVS("IPC_NS_int6", 0),
    CVS("IPC_NS_mbx0", 0),
    CVS("IPC_NS_mbx1", 0),
    CVS("IPC_NS_mbx2", 0),
    CVS("IPC_NS_mbx3", 0),
    CVS("IPC_NS_mbx4", 0),
    CVS("IPC_NS_mbx5", 0),
    CVS("IPC_NS_mbx6", 0),
    CVS("IPC_NS_mbx7", 0),
    CVS("IPC_NS_mbx8", 0),
    CVS("IPC_NS_mbx9", 0),
    CVS("IPC_NS_mbx18", 0),
    CVS("mdm_aximon_intr", 0),
    CVS("MDM_WDOG_intr", 0),
    CVS("ASP-IPC-ARM", 0),
    CVS("ASP-IPC-MCPU", 0),
    CVS("ASP-IPC-BBE16", 0),
    CVS("ASP_WD", 0),
    CVS("ASP_AXI_DLOCK", 0),
    CVS("ASP_DMA_SECURE", 0),
    CVS("ASP_DMA_SECURE_N", 0),
    CVS("SCI0", 0),
    CVS("SCI1", 0),
    CVS("SOCP0", 0),
    CVS("SOCP1", 0),
    CVS("MDM_IPF_intr0", 0),
    CVS("MDM_IPF_intr1", 0),
    CVS("ddrc_fatal_int[3:0]", 0),
    CVS("mdm_axi_dlock_int", 0),
    CVS("mdm_wdt1_intr(CDSP)", 0),
    CVS("GIC_IRQ_OUT[0]", 0),
    CVS("GIC_IRQ_OUT[1]", 0),
    CVS("GIC_IRQ_OUT[2]", 0),
    CVS("GIC_IRQ_OUT[3]", 0),
    CVS("GIC_IRQ_OUT[4]", 0),
    CVS("GIC_IRQ_OUT[5]", 0),
    CVS("GIC_IRQ_OUT[6]", 0),
    CVS("GIC_IRQ_OUT[7]", 0),
    CVS("GIC_FIQ_OUT[0]", 0),
    CVS("GIC_FIQ_OUT[1]", 0),
    CVS("GIC_FIQ_OUT[2]", 0),
    CVS("GIC_FIQ_OUT[3]", 0),
    CVS("GIC_FIQ_OUT[4]", 0),
    CVS("GIC_FIQ_OUT[5]", 0),
    CVS("GIC_FIQ_OUT[6]", 0),
    CVS("GIC_FIQ_OUT[7]", 0),
    CVS("NANDC", 0),
    CVS("CoreSight_ETR_Full", 0),
    CVS("CoreSight_ETF_Full", 0),
    CVS("DSS-pdp", 0),
    CVS("DSS-sdp", 0),
    CVS("DSS-offline", 0),
    CVS("DSS_mcu_pdp", 0),
    CVS("DSS_mcu_sdp", 0),
    CVS("DSS_mcu_offline", 0),
    CVS("DSS_dsi0", 0),
    CVS("DSS_dsi1", 0),
    CVS("IVP32_SMMU_irpt_s", 0),
    CVS("IVP32_SMMU_irpt_ns", 0),
    CVS("IVP32_WATCH_DOG", 0),
    CVS("ATGC", 0),
    CVS("G3D_IRQEVENT", 0),
    CVS("G3D_JOB", 0),
    CVS("G3D_MMU", 0),
    CVS("G3D_GPU", 0),
    CVS("isp_irq[0]", 0),
    CVS("isp_irq[1]", 0),
    CVS("isp_irq[2]", 0),
    CVS("isp_irq[3]", 0),
    CVS("isp_irq[4]", 0),
    CVS("isp_irq[5]", 0),
    CVS("isp_irq[6]", 0),
    CVS("isp_irq[7]", 0),
    CVS("isp_a7_to_gic_mbx_int[0]", 0),
    CVS("isp_a7_to_gic_mbx_int[1]", 0),
    CVS("isp_a7_to_gic_ipc_int", 0),
    CVS("isp_a7_watchdog_int", 0),
    CVS("isp_axi_dlcok", 0),
    CVS("isp_a7_irq_out", 0),
    CVS("ivp32_dwaxi_dlock_irq", 0),
    CVS("mmbuf_asc0", 0),
    CVS("mmbuf_asc1", 0),
    CVS("UFS", 0),
    CVS("pcie_link_down_int", 0),
    CVS("pcie_edma_int", 0),
    CVS("pcie_pm_int", 0),
    CVS("pcie_radm_inta", 0),
    CVS("pcie_radm_intb", 0),
    CVS("pcie_radm_intc", 0),
    CVS("pcie_radm_intd", 0),
    CVS("psam_intr[0]", 0),
    CVS("psam_intr[1]", 0),
    CVS("ocbc_pe_npint[0]", 0),
    CVS("intr_wdog_ocbc", 0),
    CVS("intr_vdec_mfde_norm", 0),
    CVS("intr_vdec_scd_norm", 0),
    CVS("intr_vdec_bpd_norm", 0),
    CVS("intr_vdec_mmu_norm", 0),
    CVS("intr_vdec_mfde_safe", 0),
    CVS("intr_vdec_scd_safe", 0),
    CVS("intr_vdec_bpd_safe", 0),
    CVS("intr_vdec_mmu_safe", 0),
    CVS("intr_venc_vedu_norm", 0),
    CVS("intr_venc_mmu_norm", 0),
    CVS("intr_venc_vedu_safe", 0),
    CVS("intr_venc_mmu_safe", 0),
    CVS("intr_qosbuf0", 0),
    CVS("intr_qosbuf1", 0),
    CVS("intr_ddrc2_err", 0),
    CVS("intr_ddrc3_err", 0),
    CVS("intr_ddrphy[0]", 0),
    CVS("intr_ddrphy[1]", 0),
    CVS("intr_ddrphy[2]", 0),
    CVS("intr_ddrphy[3]", 0),
    CVS("intr0_mdm_ipc_gic_s", 0),
    CVS("intr1_mdm_ipc_gic_s", 0),
    CVS("SPI3", 0),
    CVS("SPI4(Finger)", 0),
    CVS("I2C7(Reserved)", 0),
    CVS("intr_uce0_wdog", 0),
    CVS("intr_uce1_wdog", 0),
    CVS("intr_uce2_wdog", 0),
    CVS("intr_uce3_wdog", 0),
    CVS("intr_exmbist", 0),
    CVS("intr_hisee_wdog", 0),
    CVS("intr_hisee_ipc_mbx_gic[0]", 0),
    CVS("intr_hisee_ipc_mbx_gic[1]", 0),
    CVS("intr_hisee_ipc_mbx_gic[2]", 0),
    CVS("intr_hisee_ipc_mbx_gic[3]", 0),
    CVS("intr_hisee_ipc_mbx_gic[4]", 0),
    CVS("intr_hisee_ipc_mbx_gic[5]", 0),
    CVS("intr_hisee_ipc_mbx_gic[6]", 0),
    CVS("intr_hisee_ipc_mbx_gic[7]", 0),
    CVS("intr_hisee_alarm[0]", 0),
    CVS("intr_hisee_alarm[1]", 0),
    CVS("intr_hisee_alarm[2]", 0),
    CVS("intr_hisee_alarm[3]", 0),
    CVS("intr_hisee_eh2h_slv", 0),
    CVS("intr_hisee_ds2ap_irq", 0),
    CVS("intr_hisee_as2ap_irq", 0),
    CVS("intr_hisee_senc2ap_irq", 0),
    CVS("GPIO0_EMMC", 0),
    CVS("GPIO1_EMMC", 0),
    CVS("AONOC_TIMEOUT", 0),
    CVS("intr_hisee_tsensor[0]", 0),
    CVS("intr_hisee_tsensor[1]", 0),
    CVS("intr_hisee_lockup", 0),
    CVS("intr_hisee_dma", 0),
};

static int atoi(char *s)
{
    char *p = s;
    char c;
    int ret = 0;

    if (s == NULL)
        return 0;
    while ((c = *p++) != '\0') {
        if ('0' <= c && c <= '9') {
            ret *= 10;
            ret += c - '0';
        } else {
            break;
        }
    }

    return ret;
}

/*
 * Function: is_id_valid
 * Description: check ID valid or not
 * Input:  @id - id to check
 * Output:
 * Return: false -- invalid
 *         true -- valid
 */
static inline bool is_id_valid(int id)
{
    return (id >= AP_SLEEP && id < POWER_MONITOR_MAX);
}

/*
 * Function: report_handle
 * Description: Packet and Send data to power node
 * Input: id --- message mask
 :*        fmt -- string
 * Return: -1--failed, 0--success
 */
static int report_handle(unsigned int id, va_list args, const char *fmt)
{
    int length = 0;
    int irq_num = 0;
    int gpio_number = 0;
    int number = 0;
    int modem_r_data = 0;
    char buff[BUFF_SIZE] = { 0 };

    memset(&buff, 0, sizeof(buff));
    length = vscnprintf(buff, BUFF_SIZE - 1, fmt, args);
    if (length > 0)
        length++;

    if (BUFF_SIZE <= length) {
        pr_err("%s: string too long!\n", __func__);
        return -ENAMETOOLONG;
    }

    if (buff == NULL) {
        pr_err("%s: string is null!\n", __func__);
        return -1;
    }

    pm_info[id].idx = id;
    pm_info[id].len = length;
    switch (id) {
    case AP_SLEEP:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            pm_info[id].count = 1;
        }
        break;
    case MODEM_SLEEP:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            if (!strcmp("1", pm_info[id].buffer)) pm_info[id].count++;
            else pm_info[id].count = 0;
        }
        break;
    case MODEM_REASON:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            modem_r_data = atoi(pm_info[id].buffer);
            for (number = 0; number < MODEM_REASON_NUM; number++) {
                if ((modem_r_data &
                    modem_wakeup[number].devmask) != 0)
                    modem_wakeup[number].number++;
            }
        }
        break;
    case IOM3_SLEEP:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            if (!strcmp("1", pm_info[id].buffer)) pm_info[id].count++;
            else pm_info[id].count = 0;
        }
        break;
    case SUSPEND_FAILED:
        if (strncmp(buff, "[suspend_total]",
            strlen("[suspend_total]")) == 0) {
            suspend_total++;
        } else if (strncmp(buff, "[error_dev_name]:",
            strlen("[error_dev_name]:")) == 0) {
            if (pm_info[id].buffer != NULL) {
                memset(pm_info[id].buffer, 0,
                       sizeof(pm_info[id].buffer));
                strlcpy(pm_info[id].buffer, buff,
                    sizeof(pm_info[id].buffer));
            }
        }
        break;
    case FREEZING_FAILED:
        if (pm_info[id].buffer != NULL) {
            pm_info[id].count++;
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    case WAKEUP_ABNORMAL:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    case DRIVER_ABNORMAL:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    case WAKEUP_IRQ:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            for (irq_num = 0; irq_num < IRQ_TOTAL; irq_num++) {
#if defined (CONFIG_HW_PTM_K3V5)
                if (!strcmp
                    (irq_table_k3v5[irq_num].devname,
                     pm_info[id].buffer)) {
                    irq_table_k3v5[irq_num].number++;
                    break;
                }
#else
                if (!strcmp
                    (irq_table_k3v6[irq_num].devname,
                     pm_info[id].buffer)) {
                    irq_table_k3v6[irq_num].number++;
                    break;
                }
#endif
            }
        }
        break;
    case WAKEUP_GPIO:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
            gpio_number = atoi(pm_info[id].buffer);
            if (gpio_number <= MAX_GPIO_NUM)
                GPIO_INT[gpio_number][0]++;
        }
        break;
    case ICC_VOTE:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    case SENSORHUB_EVENT:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    case SOC_VOTE:
        if (pm_info[id].buffer != NULL) {
            memset(pm_info[id].buffer, '\0',
                   sizeof(pm_info[id].buffer));
            strlcpy(pm_info[id].buffer, buff,
                sizeof(pm_info[id].buffer));
        }
        break;
    default:
        break;
    }

    return 0;
}

/*
 * Function: power_monitor_report
 * Description: report data to power nodes
 * Input: id --- power radar nodes data struct
 *        fmt -- args from reported devices
 * Return: -x--failed, 0--success
 */
int power_monitor_report(unsigned int id, const char *fmt, ...)
{
    va_list args;
    int ret = -EINVAL;

    if (!is_id_valid(id)) {
        pr_err("%s: id %d is invalid!\n", __func__, id);
        return ret;
    }

    va_start(args, fmt);
    ret = report_handle(id, args, fmt);
    va_end(args);

    return ret;
}
EXPORT_SYMBOL(power_monitor_report);

static ssize_t ap_sleep_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = AP_SLEEP;

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);
    if (pm_info[id].buffer != NULL)
        return sprintf(buf, "%s", pm_info[id].buffer);

    return ret;
}

static ssize_t ap_sleep_store(struct kobject *kobj,
                  struct kobj_attribute *attr,
                  const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = AP_SLEEP;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(ap_sleep);

static ssize_t modem_sleep_show(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = MODEM_SLEEP;
    int modem_count = 0;

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);
    if (pm_info[id].count >= AB_WAKE_COUNT)
    {
        modem_count = pm_info[id].count;
        pm_info[id].count = 0;
        return sprintf(buf, "%d", modem_count);
    }

    return ret;
}

static ssize_t modem_sleep_store(struct kobject *kobj,
                 struct kobj_attribute *attr,
                 const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = MODEM_SLEEP;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(modem_sleep);

static ssize_t modem_reason_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
    char reason_buffer[130] = { 0 };
    char show_reason_buffer[256] = { 0 };
    ssize_t ret = 0;
    int reason_number = 0;
    int reason_buffer_length = 0;
    int show_reason_buffer_length = 0;

    memset(&reason_buffer, 0, sizeof(reason_buffer));
    memset(&show_reason_buffer, 0, sizeof(show_reason_buffer));

    for (reason_number = 0; reason_number < MODEM_REASON_NUM;
         reason_number++) {
        if (modem_wakeup[reason_number].number > 0) {
            sprintf(reason_buffer, "%s:%d\n",
                modem_wakeup[reason_number].devname,
                modem_wakeup[reason_number].number);
            reason_buffer_length = strlen(reason_buffer);
            show_reason_buffer_length = strlen(show_reason_buffer);
            if ((reason_buffer_length + show_reason_buffer_length) <
                MAX_BUF_SIZE)
                strcat(show_reason_buffer, reason_buffer);
            modem_wakeup[reason_number].number = 0;
            memset(&reason_buffer, 0, sizeof(reason_buffer));
        }
    }

    if (show_reason_buffer != NULL)
        return sprintf(buf, "%s", show_reason_buffer);
    else
        return ret;
}

static ssize_t modem_reason_store(struct kobject *kobj,
                  struct kobj_attribute *attr,
                  const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = MODEM_REASON;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(modem_reason);

static ssize_t iom3_sleep_show(struct kobject *kobj,
                   struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = IOM3_SLEEP;
    int iom3count = 0;

    //pr_err(": %s\n", __func__);
    //pr_err(": %d\n", pm_info[id].count);
    if (pm_info[id].count >= AB_WAKE_COUNT)
    {
        iom3count = pm_info[id].count;
        pm_info[id].count = 0;
        return sprintf(buf, "%d", iom3count);
    }
    return ret;
}

static ssize_t iom3_sleep_store(struct kobject *kobj,
                struct kobj_attribute *attr,
                const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = IOM3_SLEEP;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(iom3_sleep);

static ssize_t suspend_failed_show(struct kobject *kobj,
                   struct kobj_attribute *attr, char *buf)
{
    unsigned int id = SUSPEND_FAILED;
    ssize_t ret = sprintf(buf, "%d", pm_info[id].count);

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);
    pm_info[id].count = 0;

    return ret;
}

static ssize_t suspend_failed_store(struct kobject *kobj,
                    struct kobj_attribute *attr,
                    const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = SUSPEND_FAILED;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(suspend_failed);

static ssize_t freezing_failed_show(struct kobject *kobj,
                    struct kobj_attribute *attr, char *buf)
{
    unsigned int id = FREEZING_FAILED;
    ssize_t ret = 0;

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);
    if (pm_info[id].buffer != NULL) {
        ret = sprintf(buf, "freezing_failed:%s,times:%d",
                  pm_info[id].buffer, pm_info[id].count);
        pm_info[id].count = 0;
    }

    return ret;
}

static ssize_t freezing_failed_store(struct kobject *kobj,
                     struct kobj_attribute *attr,
                     const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = FREEZING_FAILED;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(freezing_failed);

static ssize_t wakeup_abnormal_show(struct kobject *kobj,
                    struct kobj_attribute *attr, char *buf)
{
    unsigned int id = WAKEUP_IRQ;

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);
    return sprintf(buf, "wakeupirq:%s", pm_info[id].buffer);
}

static ssize_t wakeup_abnormal_store(struct kobject *kobj,
                     struct kobj_attribute *attr,
                     const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = WAKEUP_IRQ;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(wakeup_abnormal);

static ssize_t driver_abnormal_show(struct kobject *kobj,
                    struct kobj_attribute *attr, char *buf)
{
    unsigned int id = DRIVER_ABNORMAL;

    //pr_err(": %s\n", __func__);
    //pr_err(": %s\n", pm_info[id].buffer);

    return sprintf(buf, "%d", pm_info[id].count);
}

static ssize_t driver_abnormal_store(struct kobject *kobj,
                     struct kobj_attribute *attr,
                     const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = DRIVER_ABNORMAL;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(driver_abnormal);

static ssize_t wakeup_irq_show(struct kobject *kobj,
                   struct kobj_attribute *attr, char *buf)
{
    char irq_buffer[130] = { 0 };
    char show_irq_buffer[256] = { 0 };
    ssize_t ret = 0;
    int irq_number = 0;
    int irq_buffer_length = 0;
    int show_irq_buffer_length = 0;

    memset(&irq_buffer, 0, sizeof(irq_buffer));
    memset(&show_irq_buffer, 0, sizeof(show_irq_buffer));

    for (irq_number = 0; irq_number < IRQ_TOTAL; irq_number++) {
#if defined (CONFIG_HW_PTM_K3V5)
        if (irq_table_k3v5[irq_number].number > 10) {
            sprintf(irq_buffer, "%s:%d\n",
                irq_table_k3v5[irq_number].devname,
                irq_table_k3v5[irq_number].number);
            irq_buffer_length = strlen(irq_buffer);
            show_irq_buffer_length = strlen(show_irq_buffer);
            if ((irq_buffer_length + show_irq_buffer_length) <
                MAX_BUF_SIZE)
                strcat(show_irq_buffer, irq_buffer);
            irq_table_k3v5[irq_number].number = 0;
            memset(&irq_buffer, 0, sizeof(irq_buffer));
        }
#else
        if (irq_table_k3v6[irq_number].number > 10) {
            sprintf(irq_buffer, "%s:%d\n",
                irq_table_k3v6[irq_number].devname,
                irq_table_k3v6[irq_number].number);
            irq_buffer_length = strlen(irq_buffer);
            show_irq_buffer_length = strlen(show_irq_buffer);
            if ((irq_buffer_length + show_irq_buffer_length) <
                MAX_BUF_SIZE)
                strcat(show_irq_buffer, irq_buffer);
            irq_table_k3v6[irq_number].number = 0;
            memset(&irq_buffer, 0, sizeof(irq_buffer));
        }
#endif
    }

    if (show_irq_buffer != NULL)
        return sprintf(buf, "wakeup irq: %s", show_irq_buffer);
    else
        return ret;
}

static ssize_t wakeup_irq_store(struct kobject *kobj,
                struct kobj_attribute *attr,
                const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = WAKEUP_IRQ;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(wakeup_irq);

static ssize_t icc_vote_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = ICC_VOTE;

    if (pm_info[id].buffer != NULL)
        ret = sprintf(buf, "icc_vote:%s", pm_info[id].buffer);

    return ret;
}

static ssize_t icc_vote_store(struct kobject *kobj,
                  struct kobj_attribute *attr,
                  const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = ICC_VOTE;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(icc_vote);

static ssize_t sensorhub_event_show(struct kobject *kobj,
                    struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = SENSORHUB_EVENT;

    if (pm_info[id].buffer != NULL)
        return sprintf(buf, "sensorhub event:%s", pm_info[id].buffer);

    return ret;
}

static ssize_t sensorhub_event_store(struct kobject *kobj,
                     struct kobj_attribute *attr,
                     const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = SENSORHUB_EVENT;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(sensorhub_event);

static ssize_t soc_vote_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    unsigned int id = SOC_VOTE;

    if (pm_info[id].buffer != NULL)
        return sprintf(buf, "soc vote:%s", pm_info[id].buffer);

    return ret;
}

static ssize_t soc_vote_store(struct kobject *kobj,
                  struct kobj_attribute *attr,
                  const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = SOC_VOTE;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(soc_vote);

static ssize_t wakeup_gpio_show(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    char gpio_buffer[130] = { 0 };
    char show_gpio_buffer[256] = { 0 };
    int gpio_number = 0;
    int gpio_buffer_length = 0;
    int show_gpio_buffer_length = 0;

    memset(&gpio_buffer, 0, sizeof(gpio_buffer));
    memset(&show_gpio_buffer, 0, sizeof(show_gpio_buffer));

    for (gpio_number = 0; gpio_number < MAX_GPIO_NUM; gpio_number++) {
        if (GPIO_INT[gpio_number][0] > 10) {
            sprintf(gpio_buffer, "gpio-%d:%d\n",
                gpio_number, GPIO_INT[gpio_number][0]);
            gpio_buffer_length = strlen(gpio_buffer);
            show_gpio_buffer_length = strlen(show_gpio_buffer);
            if ((gpio_buffer_length + show_gpio_buffer_length) <
                MAX_BUF_SIZE)
                strcat(show_gpio_buffer, gpio_buffer);
            GPIO_INT[gpio_number][0] = 0;
            memset(&gpio_buffer, 0, sizeof(gpio_buffer));
        }
    }

    if (show_gpio_buffer != NULL)
        return sprintf(buf, "%s", show_gpio_buffer);
    else
        return ret;
}

static ssize_t wakeup_gpio_store(struct kobject *kobj,
                 struct kobj_attribute *attr,
                 const char *buf, size_t n)
{
    unsigned int size = 0;
    unsigned int id = SOC_VOTE;

    if (sscanf(buf, "%d", &size) == 1) {
        pm_info[id].count = size;
        return n;
    }

    return -EINVAL;
}

power_attr(wakeup_gpio);

static struct attribute *monitor_attrs[] = {
    &ap_sleep_attr.attr,
    &modem_sleep_attr.attr,
    &modem_reason_attr.attr,
    &iom3_sleep_attr.attr,
    &suspend_failed_attr.attr,
    &freezing_failed_attr.attr,
    &wakeup_abnormal_attr.attr,
    &driver_abnormal_attr.attr,
    &wakeup_irq_attr.attr,
    &wakeup_gpio_attr.attr,
    &icc_vote_attr.attr,
    &sensorhub_event_attr.attr,
    &soc_vote_attr.attr,
    NULL,
};

static struct attribute_group monitor_attr_group = {
    .name = "monitor",  /* Directory of power monitor */
    .attrs = monitor_attrs,
};

static int __init power_monitor_init(void)
{
    int ret = -1;
    int i = 0, length = 0;

    /* power_kobj is created in kernel/power/main.c */
    if (!power_kobj) {
        pr_err("%s: power_kobj is null!\n", __func__);
        return -ENOMEM;
    }

    /* Initialized struct data */
    length = sizeof(struct power_monitor_info);
    for (i = 0; i < POWER_MONITOR_MAX; i++) {
        memset(&pm_info[i], 0, length);
        pm_info[i].count = 0;
    }

    memset(GPIO_INT, 0, sizeof(GPIO_INT));

    /* create all nodes under power sysfs */
    ret = sysfs_create_group(power_kobj, &monitor_attr_group);
    if (ret < 0) {
        pr_err("%s: sysfs_create_group power_kobj error\n", __func__);
    } else {
        pr_info("%s: sysfs_create_group power_kobj success\n",
            __func__);
    }

    return ret;
}

core_initcall(power_monitor_init);
