/*
 * hisilicon ISP driver, qos.c
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/iommu.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include "hisp_internel.h"

/* QOS */
#define QOS_FIX_MODE 0x0
#define QOS_LIMITER_MODE 0x01
#define QOS_BYPASS_MODE 0x2
#define QOS_EXTCONTROL 0x01
#define QOS_PRIO_1 0x0
#define QOS_PRIO_2 0x101
#define QOS_PRIO_3 0x202
#define QOS_PRIO_3_PLUS_RD 0x302
#define QOS_PRIO_4 0x303
#define QOS_BANDWIDTH_ISP  0x1000
#define QOS_SATURATION_ISP 0x20

#define VIVO_BUS_ISP_RD_QOS_PRIORITY_ADDR           (0x0008)
#define VIVO_BUS_ISP_RD_QOS_MODE_ADDR               (0x000C)
#define VIVO_BUS_ISP_WR_QOS_PRIORITY_ADDR           (0x0088)
#define VIVO_BUS_ISP_WR_QOS_MODE_ADDR               (0x008C)
#define VIVO_BUS_A7T0VIVOBUS_RD_QOS_PRIORITY_ADDR   (0x0108)
#define VIVO_BUS_A7T0VIVOBUS_RD_QOS_MODE_ADDR       (0x010C)
#define VIVO_BUS_A7T0VIVOBUS_WR_QOS_PRIORITY_ADDR   (0x0188)
#define VIVO_BUS_A7T0VIVOBUS_WR_QOS_MODE_ADDR       (0x018C)
#define VIVO_BUS_ISP1_RD_QOS_PRIORITY_ADDR          (0x0208)
#define VIVO_BUS_ISP1_RD_QOS_MODE_ADDR              (0x020C)
#define VIVO_BUS_ISP1_WR_QOS_PRIORITY_ADDR          (0x0288)
#define VIVO_BUS_ISP1_WR_QOS_MODE_ADDR              (0x028C)
#define VIVO_BUS_ISP_RD_QOS_BANDWIDTH_ADDR          (0x0010)
#define VIVO_BUS_ISP_RD_QOS_SATURATION_ADDR         (0x0014)
#define VIVO_BUS_ISP_RD_QOS_EXTCONTROL_ADDR         (0x0018)
#define VIVO_BUS_ISP_WR_QOS_BANDWIDTH_ADDR          (0x0090)
#define VIVO_BUS_ISP_WR_QOS_SATURATION_ADDR         (0x0094)
#define VIVO_BUS_ISP_WR_QOS_EXTCONTROL_ADDR         (0x0098)
#define VIVO_BUS_A7T0VIVOBUS_RD_QOS_BANDWIDTH_ADDR  (0x0110)
#define VIVO_BUS_A7T0VIVOBUS_RD_QOS_SATURATION_ADDR (0x0114)
#define VIVO_BUS_A7T0VIVOBUS_RD_QOS_EXTCONTROL_ADDR (0x0118)
#define VIVO_BUS_A7T0VIVOBUS_WR_QOS_BANDWIDTH_ADDR  (0x0190)
#define VIVO_BUS_A7T0VIVOBUS_WR_QOS_SATURATION_ADDR (0x0194)
#define VIVO_BUS_A7T0VIVOBUS_WR_QOS_EXTCONTROL_ADDR (0x0198)
#define VIVO_BUS_ISP1_RD_QOS_BANDWIDTH_ADDR         (0x0210)
#define VIVO_BUS_ISP1_RD_QOS_SATURATION_ADDR        (0x0214)
#define VIVO_BUS_ISP1_RD_QOS_EXTCONTROL_ADDR        (0x0218)
#define VIVO_BUS_ISP1_WR_QOS_BANDWIDTH_ADDR         (0x0290)
#define VIVO_BUS_ISP1_WR_QOS_SATURATION_ADDR        (0x0294)
#define VIVO_BUS_ISP1_WR_QOS_EXTCONTROL_ADDR        (0x0298)

int ispcpu_qos_cfg(void)
{
    void __iomem* vivobus_base;

    pr_info("[%s] +\n", __func__);

    vivobus_base = get_regaddr_by_pa(VIVOBUS);
    if (vivobus_base == NULL) {
        pr_err("[%s] vivobus_base remap fail\n", __func__);
        return -ENOMEM;
    }
    pr_info("[%s]  vivobus_base.%pK, ", __func__, vivobus_base);

    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_RD_QOS_PRIORITY_ADDR));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_RD_QOS_MODE_ADDR));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_WR_QOS_PRIORITY_ADDR));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_WR_QOS_MODE_ADDR));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_RD_QOS_PRIORITY_ADDR));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_RD_QOS_MODE_ADDR));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_WR_QOS_PRIORITY_ADDR));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_WR_QOS_MODE_ADDR));
    __raw_writel(QOS_FIX_MODE,    (volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_RD_QOS_MODE_ADDR));
    __raw_writel(QOS_FIX_MODE,    (volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_WR_QOS_MODE_ADDR));
    __raw_writel(QOS_PRIO_4,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_RD_QOS_PRIORITY_ADDR));
    __raw_writel(QOS_PRIO_4,      (volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_WR_QOS_PRIORITY_ADDR));



    pr_info("QOS : ISP.rd.(prio.0x%x, mode.0x%x), ISP.wr.(prio.0x%x, mode.0x%x), A7.rd.(prio.0x%x, mode.0x%x), A7.wr.(prio.0x%x, mode.0x%x)\n",
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_RD_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_RD_QOS_MODE_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_WR_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP_WR_QOS_MODE_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_RD_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_RD_QOS_MODE_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_WR_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_A7T0VIVOBUS_WR_QOS_MODE_ADDR)));

    pr_info("QOS : ISP1.rd.(prio.0x%x, mode.0x%x), ISP1.wr.(prio.0x%x, mode.0x%x)\n",
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_RD_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_RD_QOS_MODE_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_WR_QOS_PRIORITY_ADDR)),
        __raw_readl((volatile void __iomem*)(vivobus_base + VIVO_BUS_ISP1_WR_QOS_MODE_ADDR)));


    return 0;
}

